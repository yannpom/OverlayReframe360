#include "global.h"

#define private public
#include "Parameters.h"
#undef private

#include <nlohmann/json.hpp>
#include "Reframe360.h"
#include <spdlog/fmt/ostr.h>
#include <spdlog/stopwatch.h>

#include "MathUtil.h"
#include "spline.h"


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ParamValues, all);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BlendingFunction, bf);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Keyframe, value, bf);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ParamsInJson, keyframes, motion_blur);

MotionBlurRendering globalMotionBlurRendering = MB_RENDERING_AUTO;

const ParameterDefinition param_defs[6] = {
    {"pitch",  "%+04.0f", {1,.5,.5}, 0, -180, 180, -1800, 1800, false},
    {"yaw",    "%+04.0f", {.5,1,.5}, 0, -180, 180, -1800, 1800, true},
    {"roll",   "%+04.0f", {.5,.5,1}, 0, -180, 180, -1800, 1800, false},
    {"fov",    "%.2f",    {1,1,.5}, 1.78, 0, 2.5, 0, 2.5, false},
    {"rect",   "%.2f",    {1,.5,1}, .7, 0, 1, 0, 1, false},
    {"tiny",   "%.2f",    {.5,1,1}, 1,  0, 1, 0, 1, false},
};

const ParamValues defaultParamValues({
    param_defs[0].initial,
    param_defs[1].initial,
    param_defs[2].initial,
    param_defs[3].initial,
    param_defs[4].initial,
    param_defs[5].initial
});

Params::Params() : _parent(NULL) {
    // for (int i=0; i<6; i++) {
    //     _j.values.all[i] = param_defs[i].initial;
    // }
    SPDLOG_DEBUG("computeCache from constructor");
    computeCache();
}

Params::Params(Reframe360* parent) : Params() {
    _parent = parent;
}

float BlendingFunction::blend(float alpha, float v1, float v2) const {

    float range[4];

    if (alpha < 0.5) {
        range[0] = 0;
        range[1] = 0.5;
        range[2] = 0;
        range[3] = 1;
    } else {
        range[0] = 0.5;
        range[1] = 1;
        range[2] = 1;
        range[3] = 0;
    }

    alpha = fitRange(alpha, range[0], range[1], range[2], range[3]);
    alpha = applyBlend(alpha);
    alpha = fitRange(alpha, range[2], range[3], range[0], range[1]);

    
    return (1-alpha)*v1 + alpha*v2;
}

float BlendingFunction::applyBlend(float alpha) const {
    switch (bf) {
        case BLEND_FUNCTION_LINEAR:    return alpha;
        //case BLEND_FUNCTION_CUBIC:     return std::powf(alpha, 3.0f);
        case BLEND_FUNCTION_QUADRATIC: return std::powf(alpha, 2.0f);
        //case BLEND_FUNCTION_EXP:       return std::powf(2.0f, 10.0f*alpha-10.0f);
        case BLEND_FUNCTION_MAX:       break;
    }
    return NAN;
}

ParamValues Params::getValuesAtTime(int time, float offset) {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    if (offset == 0) {
        return _cache.getValuesAtTime(time);
    }

    int time2 = offset > 0 ? time+1 : time-1;
    const ParamValues & v0 = _cache.getValuesAtTime(time);
    const ParamValues & v1 = _cache.getValuesAtTime(time2);
    return v0 + (v1-v0)*fabsf(offset);
}

ParamValues Params::getDerivatesAtTime(int time) {
    return _cache.getDerivatesAtTime(time);
}

static void set_param_within_bounds(float & p, int n, float v, bool incr=false) {
    SPDLOG_INFO("p {} n {} v {} incr {}", p, n, v, incr);

    float min = param_defs[n].hard_min;
    float max = param_defs[n].hard_max;
    if (incr) {
        v += p;
    }
    v = TRUNC_RANGE(v, min, max);
    p = v;
    SPDLOG_INFO("p {}", p);
}

static void incr_param_within_bounds(float & p, int n, float v) {
    set_param_within_bounds(p, n, v, true);
}

void Params::changeValueAtTime(int n, int time, float value) {
    SPDLOG_INFO("n {} time {} offset {}", n, time, value);

    float * param_value = NULL;

    // force key frame creation
    //if (hasKeyframes()) {
        SPDLOG_DEBUG("has keyframe");
        auto it = _j.keyframes[n].find(time);
        if (it != _j.keyframes[n].end()) {
            Keyframe & keyframe = it->second;
            //keyframe.active[n] = true;
            incr_param_within_bounds(keyframe.value, n, value);

        } else {
            SPDLOG_INFO("time not found in _keyframes {}. Creating one", time);

            Keyframe keyframe;
            //keyframe.active[n] = true;

            const ParamValues & values = getValuesAtTime(time);
            float v = values.all[n] + value;
            set_param_within_bounds(keyframe.value, n, v);
            _j.keyframes[n].insert({time, keyframe});
        }
    // } else {
    //     SPDLOG_DEBUG("was {} delta {}", _j.values.all[n], value);
    //     incr_param_within_bounds(_j.values, n, value);
    // }

    computeCache();
    save();
}

// bool Params::hasKeyframes() {
//     //SPDLOG_DEBUG("len {}", _keyframes.size());
//     return !_j.keyframes.empty();
// }

void Params::createGlobalKeyframe(int time) {
    const ParamValues & values = getValuesAtTime(time);

    Keyframe * keyframe;

    for (int i_param=0; i_param<6; ++i_param) {
        auto it = _j.keyframes[i_param].find(time);
        if (it != _j.keyframes[i_param].end()) {
            // already has key
            keyframe = &it->second;
        } else {
            // create a keyframe
            std::pair<int, Keyframe> pair;
            pair.first = time;
            auto ret = _j.keyframes[i_param].insert(pair);
            keyframe = &ret.first->second;
        }

        keyframe->value = values.all[i_param];
    }

    computeCache();
    save();
}



void Params::moveKeyframe(int t_from, int t_to, bool duplicate) {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    SPDLOG_INFO("from {} to {} duplicate {}", t_from, t_to, duplicate);

    for (int i_param=0; i_param<6; ++i_param) {
        auto it = _j.keyframes[i_param].find(t_from);
        if (it == _j.keyframes[i_param].end()) {
            SPDLOG_ERROR("keyframe not found");
            continue;
        }

        auto const value = std::move(it->second);
        if (duplicate) {
            _j.keyframes[i_param].insert({t_to, value});
            
        } else {
            _j.keyframes[i_param].erase(it);
            _j.keyframes[i_param].insert({t_to, std::move(value)});
        }
    }

    computeCache();
    save();
}

void Params::deleteKeyframeAtTime(int time) {
    SPDLOG_INFO("time {}", time);
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    for (int i_param=0; i_param<6; ++i_param) {
        _j.keyframes[i_param].erase(time);
    }
    computeCache();
    save();
}

void Params::deleteKeyframeAtTime(int i_param, int time) {
    SPDLOG_WARN("param {} time {}", i_param, time);
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    _j.keyframes[i_param].erase(time);
    computeCache();
    save();
}

void Params::cycleBfForKeyframe(int time) {
    SPDLOG_DEBUG("time {}", time);
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    blend_function_t bf = BLEND_FUNCTION_MAX;

    // Find first param keyframe's bf
    for (int i_param=0; i_param<6; ++i_param) {
        auto it = _j.keyframes[i_param].find(time);
        if (it != _j.keyframes[i_param].end()) {
            bf = it->second.bf.bf;
            break;
        }
    }
    if (bf == BLEND_FUNCTION_MAX) {
        SPDLOG_ERROR("no keyframe found");
        return;
    }

    // Increment bf
    bf = (blend_function_t) ((bf + 1) % BLEND_FUNCTION_MAX);

    // Set bf for all keyframes
    for (int i_param=0; i_param<6; ++i_param) {
        auto it = _j.keyframes[i_param].find(time);
        if (it != _j.keyframes[i_param].end()) {
            it->second.bf.bf = bf;
        }
    }

    computeCache();
    save();
}

blend_function_t Params::getBfAtTime(int i_param, int time) {
    //SPDLOG_DEBUG("i {} time {}", i_param, time);
    auto & map = _j.keyframes[i_param];
    auto it = map.upper_bound(time);
    if (it == map.end() || it == map.begin())
        return BLEND_FUNCTION_MAX;

    it--;
    return it->second.bf.bf;
}

void Params::cycleBfAtTime(int i_param, int time) {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    auto & map = _j.keyframes[i_param];
    auto it = map.upper_bound(time);
    if (it == map.end() || it == map.begin()) {
        SPDLOG_WARN("no keyframe found");
        return;
    }
    it--;
    it->second.bf.bf = (blend_function_t) ((it->second.bf.bf + 1) % BLEND_FUNCTION_MAX);
    SPDLOG_INFO("param {} time {} bf {}", i_param, time, it->second.bf.bf);
    computeCache();
    save();
}


bool Params::getSegmentTimes(int i_param, int time, int & t1, int & t2) {
    auto & keyframes = _j.keyframes[i_param];
    auto it = keyframes.upper_bound(time);
    if (it == keyframes.end() || it == keyframes.begin())
        return false;
    t2 = it->first;
    it--;
    t1 = it->first;
    return true;
}

bool Params::getKeyframeTimes(int i_param, int time, int & t1, int & t2) {
    auto & keyframes = _j.keyframes[i_param];
    auto it = keyframes.find(time);
    if (it == keyframes.end())
        return false;
    t1 = it->first;
    it++;
    if (it == keyframes.end())
        return false;
    t2 = it->first;
    return true;
}

bool Params::hasKeyframeAtTime(int i_param, int time) const {
    auto & keyframes = _j.keyframes[i_param];
    auto it = keyframes.find(time);
    return it != keyframes.end();
}

void Params::computeCache() {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);

    SPDLOG_DEBUG("Dump Keyframes:");
    for (int i_param=0; i_param<6; ++i_param) {
        for (auto const & it : _j.keyframes[i_param]) {
            int t = it.first;
            const Keyframe & kf = it.second;
            SPDLOG_DEBUG("param {} t {} kf {}", i_param, t, kf);
        }
    }

    spdlog::stopwatch sw;

    _cache.values.clear();

    unsigned t_min = UINT_MAX;
    unsigned t_max = 0;
    bool has_keyframes = false;
    for (int i_param=0; i_param<6; ++i_param) {
        if (!_j.keyframes[i_param].empty()) {
            has_keyframes = true;
            auto it = _j.keyframes[i_param].begin();
            auto last_it = _j.keyframes[i_param].rbegin();
            if (it->first < t_min) {
                t_min = it->first;
            }
            if (last_it->first > t_max) {
                t_max = last_it->first;
            }
        }
    }

    try
    {
    if (!has_keyframes) {
        SPDLOG_DEBUG("empty");
        _cache.t1 = 0;
        _cache.values.push_back(defaultParamValues);
    } else {
        _cache.t1 = t_min;
        int total_frames = t_max - t_min + 1;
        _cache.values.resize(total_frames, defaultParamValues);
        //SPDLOG_DEBUG("Reset cache, t1 {} size {} t_min {} t_max {}", _cache.t1, _cache.values.size(), t_min, t_max);

        for (int i_param=0; i_param<6; i_param++) {
            if (_j.keyframes[i_param].empty()) {
                //SPDLOG_DEBUG("param {} empty", i_param);
                continue;
            }
            auto it_first = _j.keyframes[i_param].begin();
            auto it_last = _j.keyframes[i_param].rbegin();

            int t_first = it_first->first;
            int t_last = it_last->first;
            //SPDLOG_DEBUG("i_param {} t_first {} t_last {}", i_param, t_first, t_last);

            // Fill cache before first keyframe
            for (int i=_cache.t1; i<=t_first; ++i) {
                //SPDLOG_DEBUG("before first keyframe {} {} size {}", i, i-_cache.t1, _cache.values.size());
                _cache.values.at(i-_cache.t1).all[i_param] = it_first->second.value;
            }

            // Fill cache after last keyframe
            for (int i=t_last; i<_cache.t1+_cache.values.size(); ++i) {
                //SPDLOG_DEBUG("before after  keyframe {} {} size {}", i, i-_cache.t1, _cache.values.size());
                _cache.values.at(i-_cache.t1).all[i_param] = it_last->second.value;
            }

            // Fill cache between keyframes
            auto prev_it = _j.keyframes[i_param].begin();
            bool got_first_keyframe = false;

            for (auto it = _j.keyframes[i_param].begin(); it != _j.keyframes[i_param].end(); ++it)
            {
                int t1 = prev_it->first;
                int t2 = it->first;
                auto v1 = prev_it->second;
                auto v2 = it->second;
                
                if (!got_first_keyframe) {
                    prev_it = it;
                    got_first_keyframe = true;
                    continue;
                }

                //SPDLOG_DEBUG("Fill t1 {} t2 {} diff {}", t1, t2, t2-t1);
                for (int t=t1+1; t<=t2; ++t) {
                    BlendingFunction & bf = v1.bf;
                    float alpha = float(t-t1) / float(t2-t1);
                    float v = bf.blend(alpha, v1.value, v2.value);
                    //SPDLOG_DEBUG("at {} size {}", t-_cache.t1, _cache.values.size());
                    _cache.values.at(t-_cache.t1).all[i_param] = v;
                }
                //SPDLOG_DEBUG("after fill");
                prev_it = it;
            }
        }
    }
    }
    catch(const std::runtime_error& re) {
        SPDLOG_ERROR("Runtime error: {}", re.what());
    }
    catch(const std::exception& ex) {
        SPDLOG_ERROR("Runtime error: {}", ex.what());
    }

    // Compute splines
    {
        SPDLOG_DEBUG("SPLINE");

        struct xy_pair_t {
            std::vector<double> X;
            std::vector<double> Y;
            float deriv_1;
            float deriv_2;
        };

        for (int i_param=0; i_param<6; i_param++) {
            std::vector<xy_pair_t> xy_pairs;
            xy_pair_t xy_pair = {};

            bool compute_deriv_2 = false;

            int prev_t = 0;
            float prev_v = NAN;

            for (auto it = _j.keyframes[i_param].begin(); it != _j.keyframes[i_param].end();)
            {
                int t = it->first;
                auto v = it->second;

                if (compute_deriv_2) {
                    compute_deriv_2 = false;
                    xy_pairs.back().deriv_2 = (v.value - prev_v) / (t - prev_t);
                    //SPDLOG_DEBUG("compute_deriv_2 v.value {} prev_v {} t {} prev_t {} xy_pairs.back().deriv_2 {}", v.value, prev_v, t, prev_t, xy_pairs.back().deriv_2);
                }

                if (!isnan(prev_v) && xy_pair.X.empty()) {
                    xy_pair.deriv_1 = (v.value - prev_v) / (t - prev_t);
                }

                prev_t = t;
                prev_v = v.value;

                xy_pair.X.push_back(t);
                xy_pair.Y.push_back(v.value);

                const bool last_loop = (++it == _j.keyframes[i_param].end());

                if (v.bf.bf == BLEND_FUNCTION_LINEAR || last_loop) {
                    if (xy_pair.X.size() >= 3) {
                        xy_pairs.push_back(xy_pair);
                        compute_deriv_2 = true;
                    }
                    xy_pair.X.clear();
                    xy_pair.Y.clear();
                }
            }

            for (auto & xy_pair : xy_pairs) {
                // SPDLOG_INFO("{} xy_pair size {}", i_param, xy_pair.X.size());
                // for (int i=0; i<xy_pair.X.size(); ++i) {
                //     SPDLOG_INFO("    i {} X {} Y {} deriv {} {}", i, xy_pair.X[i], xy_pair.Y[i], xy_pair.deriv_1, xy_pair.deriv_2);
                // }

                //SPDLOG_INFO("building spline");
                tk::spline s(xy_pair.X, xy_pair.Y, tk::spline::cspline_hermite,
                    false, // monotonic
                    tk::spline::first_deriv, xy_pair.deriv_1,
                    tk::spline::first_deriv, xy_pair.deriv_2);

                //SPDLOG_INFO("building spline 2");
                const int t1 = xy_pair.X.front();
                const int t2 = xy_pair.X.back();
                //SPDLOG_INFO("looping cache {} {}", t1, t2);
                for (int t=t1; t<t2; ++t) {
                    _cache.values.at(t-_cache.t1).all[i_param] = s(t);
                }
                //SPDLOG_INFO("looped {}", i_param);
            }
        }
    }

    SPDLOG_DEBUG("stop, elapsed {}", sw);

    // Compute derivate
    const int total_cache_elements = _cache.values.size();
    _cache.derivate.clear();
    _cache.derivate.resize(total_cache_elements);

    for (int i=1; i<total_cache_elements-1; ++i) {
        auto & v1 = _cache.values.at(i-1);
        auto & v2 = _cache.values.at(i+1);
        auto diff = v2-v1;
        for (int j=0; j<6; ++j) {
            auto & param = param_defs[j];
            const float range = param.display_range();
            diff.all[j] = log10f(fabsf(diff.all[j]/range));
        }
        _cache.derivate.at(i) = diff;
    }

    // SPDLOG_DEBUG("Dump cache:");
    // int t = _cache.t1;
    // auto it1 = _cache.values.begin();
    // auto it2 = _cache.derivate.begin();
    // for (; it1!=_cache.values.end(); ++it1, ++it2) {
    //     SPDLOG_DEBUG("t {} x {} dx {}", t, *it1, *it2);
    //     t++;
    // }

    _json = nlohmann::json(_j).dump();
    SPDLOG_DEBUG("JSON {}", _json);
}

void Params::loadJson(const std::string & s) {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    try {
        auto j = nlohmann::json::parse(s);
        _j = j.get<ParamsInJson>();
        computeCache();
    } catch (nlohmann::json::parse_error& ex) {
        std::cerr << "parse error at byte " << ex.byte << std::endl;
        SPDLOG_ERROR("Parsing error {}", ex.byte);
    } catch (std::exception& ex) {
        SPDLOG_ERROR("Parsing error {}", ex.what());
    }
}

void Params::save() {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    if (_parent) {
        _parent->refresh();
    }
}

float Params::getMotionBlur() const {
    switch (_j.motion_blur) {
        case MOTION_BLUR_OFF: return 0;
        case MOTION_BLUR_90DEG: return 90;
        case MOTION_BLUR_180DEG: return 180;
        case MOTION_BLUR_360DEG: return 360;
        case MOTION_BLUR_MAX: break;
    }
    return 0;
}

void Params::cycleMotionBlur() {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    int i = _j.motion_blur;
    i++;
    if (i>= MOTION_BLUR_MAX) {
        i = 0;
    }
    _j.motion_blur = (motion_blur_t)i;
    computeCache();
    save();
}

void Params::toggleMotionBlurRendering() {
    std::lock_guard<std::recursive_mutex> guard(_cache_mutex);
    int v = globalMotionBlurRendering + 1;
    if (v > 2) {
        v = 0;
    }
    globalMotionBlurRendering = (MotionBlurRendering)v;
    computeCache();
    save();
}

const std::set<int> Params::keyframeTimes() const {
    std::set<int> v;
    for (int i_param=0; i_param<6; ++i_param) {
        for (auto& it : _j.keyframes[i_param]) {
            v.insert(it.first);
        }
    }
    return v;
}


