#pragma once

#include <math.h>
#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <string>
#include <vector>

enum ParameterEnum {
    PARAM_PITCH,
    PARAM_YAW,
    PARAM_ROLL,
    PARAM_FOV,
    PARAM_RECTILINEAR,
    PARAM_TINY_PLANET,
};

enum MotionBlurRendering {
    MB_RENDERING_AUTO,
    MB_RENDERING_ON,
    MB_RENDERING_OFF
};

extern MotionBlurRendering globalMotionBlurRendering;

struct color_t {
    float r;
    float g;
    float b;
};

template <typename T>
struct PackOfSix {
    union {
        T all[6];
        struct {
            T pitch;
            T yaw;
            T roll;
            T fov;
            T rectilinear;
            T tiny_planet;
        };
    };

    PackOfSix() : all{} {}
    PackOfSix(T i_pitch, T i_yaw, T i_roll, T i_fov, T i_rectilinear, T i_tiny_planet)
        : pitch{i_pitch}
        , yaw(i_yaw)
        , roll(i_roll)
        , fov(i_fov)
        , rectilinear(i_rectilinear)
        , tiny_planet(i_tiny_planet)
    {}

    friend std::ostream& operator<<(std::ostream& os, const PackOfSix<T>& c) { 
        return os << "[" << c.pitch << " " << c.yaw << " " << c.roll << " " << c.fov << " " << c.rectilinear << " " << c.tiny_planet << "]";
    }

    PackOfSix<T> operator+(const PackOfSix<T>& b) const {
        PackOfSix<T> r;
        for (int i=0; i<6; ++i) {
            r.all[i] = all[i] + b.all[i];
        }
        return r;
    }

    PackOfSix<T> operator-(const PackOfSix<T>& b) const {
        PackOfSix<T> r;
        for (int i=0; i<6; ++i) {
            r.all[i] = all[i] - b.all[i];
        }
        return r;
    }

    PackOfSix<T> operator*(const float & b) const {
        PackOfSix<T> r;
        for (int i=0; i<6; ++i) {
            r.all[i] = b*all[i];
        }
        return r;
    }
};

enum blend_function_t {
    //BLEND_FUNCTION_CUBIC,
    BLEND_FUNCTION_QUADRATIC,
    BLEND_FUNCTION_LINEAR,
    //BLEND_FUNCTION_EXP,
    BLEND_FUNCTION_MAX,
};

struct BlendingFunction {
    blend_function_t bf;

    float blend(float alpha, float v1, float v2) const;
    float applyBlend(float alpha) const;

    friend std::ostream& operator<<(std::ostream& os, const BlendingFunction& c) { 
        return os << c.bf;
    }
};

typedef PackOfSix<float> ParamValues;
extern const ParamValues defaultParamValues;


struct Keyframe {
    float value;
    BlendingFunction bf;

    Keyframe() : value(), bf{} {}

    friend std::ostream& operator<<(std::ostream& os, const Keyframe& c) { 
        os << "KF " << c.value << " " << c.bf;
        return os;
    }
};



class ParameterDefinition {
public:
    ParameterDefinition(const char* i_name,
        const char* i_format,
        color_t i_color,
        float i_initial,
        float i_display_min,
        float i_display_max,
        float i_hard_min,
        float i_hard_max,
        bool i_display_opposite
    ) : name(i_name)
    , format(i_format)
    , color(i_color)
    , initial(i_initial)
    , display_min(i_display_min)
    , display_max(i_display_max)
    , hard_min(i_hard_min)
    , hard_max(i_hard_max)
    , display_opposite(i_display_opposite)
    {
        name_capitalized = name;
        name_capitalized[0] = toupper(name_capitalized[0]);
        name_upper = name;
        std::for_each(name_upper.begin(), name_upper.end(), [](char & c){ c = ::toupper(c); });
        name_bf = name + "_bf";
        name_3chars = name_upper;
        name_3chars.resize(3);
    }

    std::string name;
    std::string name_bf;
    std::string format;
    std::string name_capitalized;
    std::string name_upper;
    std::string name_3chars;

    color_t color;
    float initial;
    float display_min;
    float display_max;
    float hard_min;
    float hard_max;
    bool display_opposite;

    float display_value(float v) const {
        float modulo = display_max-display_min + .0001f;
        v = fmodf(v, modulo);
        return fmodf(v+modulo-display_min, modulo) + display_min;
    }

    float display_range() const {
        return display_max - display_min;
    }
};

extern const ParameterDefinition param_defs[6];


struct ValuesCache {
    const ParamValues & getValuesAtTime(int time) {
        assert(!values.empty());
        if (time <= t1) {
            return values.front();
        } else if (time >= t1 + values.size()) {
            return values.back();
        }
        return values.at(time-t1);
    }
    const ParamValues & getDerivatesAtTime(int time) {
        assert(!derivate.empty());

        static ParamValues _zero;
        if (time <= t1) {
            return _zero;
        } else if (time >= t1 + values.size()) {
            return _zero;
        }
        return derivate.at(time-t1);
    }

    int t1;
    std::vector<ParamValues> values;
    std::vector<ParamValues> derivate;
};


class Reframe360;

enum motion_blur_t {
    MOTION_BLUR_OFF,
    MOTION_BLUR_90DEG,
    MOTION_BLUR_180DEG,
    MOTION_BLUR_360DEG,
    MOTION_BLUR_MAX,
};

struct ParamsInJson {
    std::map<int, Keyframe> keyframes[6];
    motion_blur_t motion_blur;

    ParamsInJson() : motion_blur(MOTION_BLUR_90DEG) {}
};

class Params {
public:
    
    Params();
    Params(Reframe360*);

    const std::map<int, Keyframe> & keyframes(int i_param) const { return _j.keyframes[i_param]; }
    const std::set<int> keyframeTimes() const;
    float getMotionBlur() const;
    void cycleMotionBlur();
    void toggleMotionBlurRendering();

    ParamValues getValuesAtTime(int time, float offset=0);
    ParamValues getDerivatesAtTime(int time);
    void changeValueAtTime(int n, int time, float value);
    
    void createGlobalKeyframe(int time);
    void moveKeyframe(int t_from, int t_to, bool duplicate);
    void deleteKeyframeAtTime(int time);
    void deleteKeyframeAtTime(int i_param, int time);
    void cycleBfForKeyframe(int time);

    blend_function_t getBfAtTime(int i_param, int time);
    void cycleBfAtTime(int i_param, int time);
    bool getSegmentTimes(int i_param, int time, int & t1, int & t2);
    bool getKeyframeTimes(int i_param, int time, int & t1, int & t2);

    bool hasKeyframeAtTime(int i_param, int time) const;
    
    const std::string & getJson() const { return _json; }
    void loadJson(const std::string & json);


private:

    void computeCache();
    void save();
    void refresh_render();

    ParamsInJson _j;
    
    Reframe360* _parent;
    std::recursive_mutex _cache_mutex;
    ValuesCache _cache;
    std::string _json;
};





