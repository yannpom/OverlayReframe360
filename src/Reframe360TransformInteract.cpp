#include "Reframe360TransformInteract.h"

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <math.h>
#include <ofxsOGLTextRenderer.h>
#include <iostream>

#include "lodepng.h"
#include "Parameters.h"
#include "global.h"
#include "utils.h"
#include "images.h"

#include <spdlog/stopwatch.h>

static float m = 3; // text margin from cell side
static float m2 = 12; // text inter line


static const float COLOR_CLOSEST_KEYFRAME[4] = {0, .5, 1, 1};


struct texture_t {
    const unsigned char * data;
    size_t size;
};

static const texture_t icons_data[ICON_MAX] = {
    {images_bar_grey_png, images_bar_grey_png_len},
    {images_bar_blue_png, images_bar_blue_png_len},
    {images_bar_green_png, images_bar_green_png_len},
    {images_icon_0_png, images_icon_0_png_len},
    {images_icon_p_0_png, images_icon_p_0_png_len},
    {images_icon_1_png, images_icon_1_png_len},
    {images_icon_p_1_png, images_icon_p_1_png_len},
    {images_icon_2_png, images_icon_2_png_len},
    {images_icon_p_2_png, images_icon_p_2_png_len},
    {images_icon_3_png, images_icon_3_png_len},
    {images_icon_p_3_png, images_icon_p_3_png_len},
    {images_curv_png, images_curv_png_len},
    {images_curv_p_png, images_curv_p_png_len},
    {images_blur_png, images_blur_png_len},
    {images_blur_p_png, images_blur_p_png_len},
    {images_rend_png, images_rend_png_len},
    {images_rend_p_png, images_rend_p_png_len}
};

static int icons_sizes[ICON_MAX][2] = {};

static GLuint texture[ICON_MAX] = {};

static std::vector<std::string> checkGLError() {
    GLenum error = glGetError();
    std::vector<std::string> errors;
    while(error != GL_NO_ERROR)
    {
        switch(error){
            case(GL_NO_ERROR):
                break;
            case(GL_INVALID_ENUM): errors.push_back("GL_INVALID_ENUM"); break;
            case(GL_INVALID_VALUE): errors.push_back("GL_INVALID_VALUE"); break;
            case(GL_INVALID_OPERATION): errors.push_back("GL_INVALID_OPERATION"); break;
            case(GL_INVALID_FRAMEBUFFER_OPERATION): errors.push_back("GL_INVALID_FRAMEBUFFER_OPERATION"); break;
            case(GL_OUT_OF_MEMORY): errors.push_back("GL_OUT_OF_MEMORY"); break;
            default: errors.push_back("UNKNOWN ERROR"); break;
        }
        error = glGetError();
    }
    return errors;
}


#define CHECK_GL_ERROR(fmt, ...) \
    do { \
        std::vector<std::string> errors = checkGLError(); \
        for (auto & error : errors) { \
            SPDLOG_ERROR("GL error: {}: " fmt, error, __VA_ARGS__); \
        } \
    } while (0)

#define CHECK_GL_ERROR_NO_ARG() \
    do { \
        std::vector<std::string> errors = checkGLError(); \
        for (auto & error : errors) { \
            SPDLOG_ERROR("GL error: {}", error); \
        } \
    } while (0)


static void loadTexture(int n, const unsigned char * data, size_t size, bool no_gl) {
    unsigned w, h;
    unsigned char* imagedata;
    lodepng_decode32(&imagedata, &w, &h, data, size);

    SPDLOG_INFO("{} loaded {}x{}", n, w, h);

    icons_sizes[n][0] = w;
    icons_sizes[n][1] = h;

    if (no_gl) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture[n]);
    glBindTexture(GL_TEXTURE_2D, texture[n]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagedata);
    glDisable(GL_TEXTURE_2D);
}


static void initTexture(bool no_gl=false) {
    SPDLOG_INFO("initTexture");
    for (int i=0; i<ICON_MAX; ++i) {
        loadTexture(i, icons_data[i].data, icons_data[i].size, no_gl);
    }
}

static void DrawImageQuad(int n, float p1X, float p1Y, float w, float h) {
    const float p2X = p1X + w;
    const float p2Y = p1Y + h;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[n]);
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f((GLfloat)p1X, (GLfloat)p1Y);
    glTexCoord2f(1, 1); glVertex2f((GLfloat)p2X, (GLfloat)p1Y);
    glTexCoord2f(1, 0); glVertex2f((GLfloat)p2X, (GLfloat)p2Y);
    glTexCoord2f(0, 0); glVertex2f((GLfloat)p1X, (GLfloat)p2Y);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    CHECK_GL_ERROR_NO_ARG();
}

Reframe360TransformInteract::Reframe360TransformInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
: OverlayInteract(handle)
, _plugin( dynamic_cast<Reframe360*>(effect) )
, _pen_down(false)
, _last_pen_up_ts(0)
//, _display_mode(DISPLAY_FRAME)
, _hidpi(false)
, _font(OFX::TextRenderer::FONT_FIXED_8_X_13)
, _selected_keyframe_time(-1)
, _display_curves(true)
, _shift_down(false)
, _msg(NULL)
, _duplicated_keyframe_time(-1)
, _params(_plugin->params)
{
    assert(_plugin);
    OfxPointD size = _plugin->getProjectSize();
    WIDTH = size.x;
    HEIGHT = size.y;
    MARGIN = 40;
    SPDLOG_INFO("width height {} {} {}", WIDTH, HEIGHT, MARGIN);

    initTexture(true);

    const bool orientation[6] = {false, true, true, false, false, false};
    for (int i=0; i<6; ++i) {
        _sliders.push_back(new Slider(_drawContext, i, orientation[i]));
    }

    _b_curves = new Button(_drawContext, ICON_CURV, ICON_CURV_OVER);
    _b_blur = new Button(_drawContext, ICON_BLUR, ICON_BLUR_OVER);
    _b_render = new Button(_drawContext, ICON_REND, ICON_REND_OVER);
}


bool Reframe360TransformInteract::penMotion(const OFX::PenArgs &args) {
    refreshVariables(args);

    if (_pen_down) {
        if (_edited_param) {
            float dx = -_mouse_dx / (WIDTH-2*2*MARGIN);
            float dy = -_mouse_dy / (HEIGHT-2*2*MARGIN);

            int time = _closest_keyframe_time<0 ? _time : _closest_keyframe_time;
            
            SPDLOG_DEBUG("penMotion p {} kf_time {} v {} {}", _edited_param, time, dx, dy);

            switch (_edited_param) {
                case EDITED_PARAM_PITCH:
                    _params.changeValueAtTime(PARAM_PITCH, time, -dy*180);
                    break;
                case EDITED_PARAM_YAW:
                    _params.changeValueAtTime(PARAM_YAW, time, dx*360);
                    break;
                case EDITED_PARAM_PITCH_YAW:
                    _params.changeValueAtTime(PARAM_YAW, time, dx*360);
                    _params.changeValueAtTime(PARAM_PITCH, time, dy*180);
                    break;
                case EDITED_PARAM_ROLL:
                    _params.changeValueAtTime(PARAM_ROLL, time, -dx*360);
                    break;
                case EDITED_PARAM_FOV:
                    _params.changeValueAtTime(PARAM_FOV, time, -dy);
                    break;
                case EDITED_PARAM_RECTILINEAR:
                    _params.changeValueAtTime(PARAM_RECTILINEAR, time, -dy);
                    break;
                case EDITED_PARAM_TINYPLANET:
                    _params.changeValueAtTime(PARAM_TINY_PLANET, time, -dy);
                    break;
                case EDITED_PARAM_MOTION_BLUR:
                case EDITED_PARAM_MOTION_BLUR_RENDERING:
                case EDITED_PARAM_NONE:
                    break;
            }
            _plugin->refresh(); // TODO factorise this
            //_render_needed = true;
        }
        if (_mouse_dx && (_action == ACTION_KEYFRAME_MOVE || _action == ACTION_KEYFRAME_DUPLICATE)) {
            SPDLOG_DEBUG("penMotion {} {} _duplicate_keyframe {} {}", _mouse_dx, _mouse_dy, _duplicated_keyframe_time);

            int t = pixelAsTime(_mouse_x);
            if (_selected_keyframe_time != t && _duplicated_keyframe_time != t) {
                const bool dup = _action == ACTION_KEYFRAME_DUPLICATE && _duplicated_keyframe_time < 0;
                SPDLOG_DEBUG("Move keyframe from {} to {} dup={}", _selected_keyframe_time, t, dup);
                _params.moveKeyframe(_selected_keyframe_time, t, dup);

                if (_duplicated_keyframe_time < 0) {
                    _duplicated_keyframe_time = _selected_keyframe_time;
                }
                _selected_keyframe_time = t;

                _keyframe_moved = true;
            } else {
                SPDLOG_DEBUG("Cannot move keyframe: same time {} {} {}", _selected_keyframe_time, _duplicated_keyframe_time, t);
            }
        }
    }
    _plugin->redrawOverlays();
    return true;
}

bool Reframe360TransformInteract::penDown(const OFX::PenArgs &args) {
    SPDLOG_INFO("");
    refreshVariables(args);
    _pen_down = true;
    refreshVariables(args);
    _mouse_down_x = _mouse_x;
    _mouse_down_y = _mouse_y;

    SPDLOG_INFO("CLICK action {}", _action);
    switch (_action) {
        case ACTION_CREATE_KEYFRAME_AT_TIMELINE: _params.createGlobalKeyframe(_time); break;

        case ACTION_KEYFRAME_BF: _params.cycleBfForKeyframe(_selected_keyframe_time); break;
        case ACTION_KEYFRAME_DELETE: _params.deleteKeyframeAtTime(_selected_keyframe_time); break;

        case ACTION_CYCLE_BF_PITCH: _params.cycleBfAtTime(PARAM_PITCH, _time); break;
        case ACTION_CYCLE_BF_YAW: _params.cycleBfAtTime(PARAM_YAW, _time); break;
        case ACTION_CYCLE_BF_ROLL: _params.cycleBfAtTime(PARAM_ROLL, _time); break;
        case ACTION_CYCLE_BF_FOV: _params.cycleBfAtTime(PARAM_FOV, _time); break;
        case ACTION_CYCLE_BF_RECTILINEAR: _params.cycleBfAtTime(PARAM_RECTILINEAR, _time); break;
        case ACTION_CYCLE_BF_TINYPLANET: _params.cycleBfAtTime(PARAM_TINY_PLANET, _time); break;

        case ACTION_DELETE_KF_PITCH: _params.deleteKeyframeAtTime(PARAM_PITCH, _closest_keyframe_time); break;
        case ACTION_DELETE_KF_YAW: _params.deleteKeyframeAtTime(PARAM_YAW, _closest_keyframe_time); break;
        case ACTION_DELETE_KF_ROLL: _params.deleteKeyframeAtTime(PARAM_ROLL, _closest_keyframe_time); break;
        case ACTION_DELETE_KF_FOV: _params.deleteKeyframeAtTime(PARAM_FOV, _closest_keyframe_time); break;
        case ACTION_DELETE_KF_RECTILINEAR: _params.deleteKeyframeAtTime(PARAM_RECTILINEAR, _closest_keyframe_time); break;
        case ACTION_DELETE_KF_TINYPLANET: _params.deleteKeyframeAtTime(PARAM_TINY_PLANET, _closest_keyframe_time); break;

        case ACTION_CYCLE_MOTION_BLUR: _params.cycleMotionBlur(); break;
        case ACTION_TOGGLE_MOTION_BLUR_RENDERING: _params.toggleMotionBlurRendering(); break;
        case ACTION_TOGGLE_CURVES_DISPLAY: _display_curves = !_display_curves; break;

        default:
            SPDLOG_DEBUG("CLICK action not handeled");
            break;
    }

    _plugin->redrawOverlays();
    return true;
}


bool Reframe360TransformInteract::penUp(const OFX::PenArgs &args) {
    SPDLOG_INFO("");
    _pen_down = false;
    refreshVariables(args);

    // uint64_t ts = get_microseconds();
    // const bool double_click = ts - _last_pen_up_ts < 200000;
    // _last_pen_up_ts = ts;

    // if (_action == ACTION_MOVE_KEYFRAME) {
    //     if (double_click) {
    //         _params.deleteKeyframeAtTime(_moved_keyframe_time);
    //     } else if (!keyframe_moved) {
    //         //_params.cycleBfForKeyframe(_moved_keyframe_time);
    //     }
    // }

    //_plugin->redrawOverlays();
    return true;
}

bool Reframe360TransformInteract::keyDown(const OFX::KeyArgs &args) {
    SPDLOG_INFO("{}", args.keySymbol);
    
    if (args.keySymbol == kOfxKey_Shift_L) {
        _shift_down = true;
        _display_curves = !_display_curves;
        refreshVariables();
        _plugin->redrawOverlays();
    }
    return false;
}

bool Reframe360TransformInteract::keyUp(const OFX::KeyArgs &args) {
    SPDLOG_INFO("{}", args.keySymbol);

    if (args.keySymbol == kOfxKey_Shift_L) {
        _shift_down = false;
        refreshVariables();
        _plugin->redrawOverlays();
    }

    // if (args.keySymbol == kOfxKey_Control_L) {
    //     _plugin->processMidiQueue();
    //     _plugin->refresh();
    // }
    return true;
}


static void drawLine(double x0, double y0, double x1, double y1) {
    glBegin(GL_LINES);
    glVertex2d(x0, y0);
    glVertex2d(x1, y1);
    glEnd();
}

static void drawPoint(double x, double y, float size=8.0f) {
    glPointSize(size);
    glBegin(GL_POINTS);
    glVertex2d(x, y);
    glEnd();
}

// float Reframe360TransformInteract::timeAsPixel(int t) const {
//     return WIDTH * float(t-_t1) / float(_t2-_t1);
// }

// int Reframe360TransformInteract::pixelAsTime(float p) const {
//     return (p / WIDTH) * (_t2-_t1) + _t1;
// }

static const float pixel_per_frame = 5;

float Reframe360TransformInteract::timeAsPixel(int t) const {    
    return pixel_per_frame*(t - _time) + WIDTH/2;
}

int Reframe360TransformInteract::pixelAsTime(float p) const {
    return (p - WIDTH/2) / pixel_per_frame + _time;
}



static float sym_modulo(float v, float modulo) { // sym_modulo(x, 360) => [-180,+180]
    v = fmodf(v, modulo);
    return fmodf(v+modulo+modulo/2, modulo) - modulo/2;
}

static const float cursor_width = 8;

static float pixel_from_value_and_range(float value, float min, float max, float pixel_min, float pixel_max) {
    float value_normalised = (value - min) / (max-min);
    return pixel_min+cursor_width/2 + value_normalised * (pixel_max-pixel_min-cursor_width);
}

void Reframe360TransformInteract::drawFrame()
{
    // Background
    glColor4f(0,0,0,.4f);

    if (_display_curves) {
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(0, HEIGHT);
        glVertex2f(WIDTH, HEIGHT);
        glVertex2f(WIDTH, 0);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(0, HEIGHT);
        glVertex2f(2*MARGIN, HEIGHT);
        glVertex2f(2*MARGIN, 0);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(WIDTH-2*MARGIN, 0);
        glVertex2f(WIDTH-2*MARGIN, HEIGHT);
        glVertex2f(WIDTH, HEIGHT);
        glVertex2f(WIDTH, 0);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(2*MARGIN, 0);
        glVertex2f(2*MARGIN, 2*MARGIN);
        glVertex2f(WIDTH-2*MARGIN, 2*MARGIN);
        glVertex2f(WIDTH-2*MARGIN, 0);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(2*MARGIN, HEIGHT-2*MARGIN);
        glVertex2f(2*MARGIN, HEIGHT);
        glVertex2f(WIDTH-2*MARGIN, HEIGHT);
        glVertex2f(WIDTH-2*MARGIN, HEIGHT-2*MARGIN);
        glEnd();
    }

    glColor3f(1, 1, 1);   
    
    // STATIC GRID
    glLineWidth(1);
    // H lines
    drawLine(2*MARGIN, HEIGHT-MARGIN, WIDTH-2*MARGIN, HEIGHT-MARGIN);
    drawLine(0, HEIGHT-2*MARGIN, WIDTH, HEIGHT-2*MARGIN);
    drawLine(0, 2*MARGIN, WIDTH, 2*MARGIN);
    // V lines
    drawLine(MARGIN, 2*MARGIN, MARGIN, HEIGHT-2*MARGIN);
    drawLine(2*MARGIN, 0, 2*MARGIN, HEIGHT);
    drawLine(WIDTH-2*MARGIN, 0, WIDTH-2*MARGIN, HEIGHT);
    drawLine(WIDTH-MARGIN, 2*MARGIN, WIDTH-MARGIN, HEIGHT-2*MARGIN);

    // DRAW values bars
    glColor3f(1, 1, .2);
    glLineWidth(cursor_width);
    {
        const float w = MARGIN;
        const float h = HEIGHT-4*MARGIN;
        _sliders[3]->setPosition(0, 2*MARGIN, w, h);
        _sliders[0]->setPosition(MARGIN, 2*MARGIN, w, h);
        _sliders[4]->setPosition(WIDTH-2*MARGIN, 2*MARGIN, w, h);
        _sliders[5]->setPosition(WIDTH-MARGIN, 2*MARGIN, w, h);
    }
    {
        const float w = WIDTH-4*MARGIN;
        const float h = MARGIN;
        _sliders[1]->setPosition(2*MARGIN, HEIGHT-2*MARGIN, w, h);
        _sliders[2]->setPosition(2*MARGIN, HEIGHT-MARGIN, w, h);
    }

    // DRAW Sliders
    for (int i_param=0; i_param<6; ++i_param) {
        float v = _params.getValuesAtTime(_time).all[i_param];
        float kf_v = NAN;

        if (_closest_keyframe_time > 0) {
            auto it = _params.keyframes(i_param).find(_closest_keyframe_time);
            if (it != _params.keyframes(i_param).end()) {
                kf_v = it->second.value;
            }
        }

        auto bf = _params.getBfAtTime(i_param, _time);
        _sliders[i_param]->draw(v, kf_v, bf);
    }
}


void Reframe360TransformInteract::drawCurve(int i_param, int t1, int t2) {
    auto & param = param_defs[i_param];
    glBegin(GL_LINE_STRIP);
    
    float prev_x, prev_y, prev_v, prev_v_display;
    bool init = false;
    
    for (int t=t1; t<t2+1; ++t) {
        float x = timeAsPixel(t);

        // Skip points outside the visible area
        if (x<2*MARGIN) continue;
        if (x>WIDTH-2*MARGIN) break;

        float v = _params.getValuesAtTime(t).all[i_param];
        float v_display = param.display_value(v);
        float y = pixel_from_value_and_range(v_display, param.display_min, param.display_max, 2*MARGIN, HEIGHT-2*MARGIN);

        if (init) {
            float ratio = (v_display-prev_v_display) / (v-prev_v);

            if (isfinite(ratio) && fabsf(ratio) > 1.1 && fabsf(prev_y - y) > 10) {
                if (v_display < prev_v_display) {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    glVertex2d(prev_x, 2*MARGIN);
                } else {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    glVertex2d(prev_x, HEIGHT);
                }
            } else {
                //drawLine(prev_x, prev_y, x, y);
            }
        } else {
            init = true;
        }

        glVertex2d(x, y);

        prev_x = x;
        prev_y = y;
        prev_v = v;
        prev_v_display = v_display;
    }
    glEnd();
    CHECK_GL_ERROR_NO_ARG();
}

void Reframe360TransformInteract::drawCurves() {
    // Data points
    glEnable(GL_LINE_SMOOTH);

    for (int i=0; i<6; ++i) {
        glLineWidth(1);
        auto & param = param_defs[i];
        glColor3f(param.color.r, param.color.g, param.color.b);
        drawCurve(i, _t1, _t2);

        if (_action - ACTION_CYCLE_BF_PITCH == i) {
            int highlight_t1, highlight_t2;
            if (_params.getSegmentTimes(i, _time, highlight_t1, highlight_t2)) {
                glLineWidth(3);
                glColor4fv(COLOR_CLOSEST_KEYFRAME);
                drawCurve(i, highlight_t1, highlight_t2);
            }
        }

        if (_action == ACTION_KEYFRAME_BF) {
            int highlight_t1, highlight_t2;
            if (_params.getKeyframeTimes(i, _selected_keyframe_time, highlight_t1, highlight_t2)) {
                glLineWidth(3);
                glColor4fv(COLOR_CLOSEST_KEYFRAME);
                drawCurve(i, highlight_t1, highlight_t2);
            }
        }
    }
    glDisable(GL_LINE_SMOOTH);

    // Keyframes Points
    for (int i_param=0; i_param<6; ++i_param) {
        auto & param = param_defs[i_param];
        for (auto& k: _params.keyframes(i_param)) {
            int t = k.first;
            float x = timeAsPixel(t);
            const Keyframe & kf = k.second;

            // Skip points outside the visible area
            if (x<2*MARGIN) continue;
            if (x>WIDTH-2*MARGIN) break;
            
            float v = _params.getValuesAtTime(t).all[i_param];
            v = param.display_value(v);
            float y = pixel_from_value_and_range(v, param.display_min, param.display_max, 2*MARGIN, HEIGHT-2*MARGIN);
            if (_closest_keyframe_time == t) {
                if (_action - ACTION_DELETE_KF_PITCH == i_param || _action - ACTION_EDIT_PITCH == i_param) {
                    glColor3f(1,.2,.2);
                } else {
                    glColor3f(0.2,0.2,1);
                }    
            } else {
                glColor3f(param.color.r, param.color.g, param.color.b);
            }
            drawPoint(x, y, 10);
        }
        // Draw point on new keyframe position
        if (_action == ACTION_NEW_PITCH + i_param) {
            glColor3f(1,.2,.2);
            int t = _closest_keyframe_time < 0 ? _time : _closest_keyframe_time;
            float v = _params.getValuesAtTime(t).all[i_param];
            v = param.display_value(v);
            float y = pixel_from_value_and_range(v, param.display_min, param.display_max, 2*MARGIN, HEIGHT-2*MARGIN);
            drawPoint(timeAsPixel(t), y, 10);
        }
    }
    CHECK_GL_ERROR_NO_ARG();

    // Keyframes Lines only if mouse in timeline
    if (_mouse_y < 2*MARGIN) {
        for (int t: _params.keyframeTimes()) {
            float x = timeAsPixel(t);

            // Skip points outside the visible area
            if (x<2*MARGIN) continue;
            if (x>WIDTH-2*MARGIN) break;

            // V line at each keyframe
            if (_closest_keyframe_time == t) {
                glColor4fv(COLOR_CLOSEST_KEYFRAME);
                drawLine(x, 0, x, HEIGHT-2*MARGIN);
            } else if (_selected_keyframe_time == t) {
                glColor4f(1, 1, 1, .4);
                drawLine(x, 0, x, HEIGHT-2*MARGIN);
            }
        }
    }
    CHECK_GL_ERROR_NO_ARG();

    // V line at Cursor
    glColor3f(1, .2, .2);
    drawLine(_time_pixel, 2*MARGIN, _time_pixel, HEIGHT);

    CHECK_GL_ERROR_NO_ARG();
    // // TEXT values
    // for (int i=0; i<6; ++i) {
    //     auto & param = param_defs[i];
    //     // at Timeline
    //     float v = _params.getValuesAtTime(_time).all[i];
    //     char buf[20];
    //     char buf_format[20];
    //     snprintf(buf_format, sizeof(buf_format), "%% 3s: %s", param.format.c_str());
    //     snprintf(buf, sizeof(buf), buf_format, param.name_3chars.c_str(), v);
    //     glColor3f(param.color.r, param.color.g, param.color.b);
    //     OFX::TextRenderer::bitmapString(m, 2*MARGIN+m+(6-i-1)*m2, buf, _font);

    //     // at Mouse
    //     v = _params.getValuesAtTime(_mouse_cursor_time).all[i];
    //     snprintf(buf, sizeof(buf), buf_format, param.name_3chars.c_str(), v);
    //     glColor3f(param.color.r, param.color.g, param.color.b);
    //     OFX::TextRenderer::bitmapString(WIDTH-100, 2*MARGIN+m+(6-i-1)*m2, buf, _font);
    // }

    // // V line at mouse
    // glColor3f(1, .2, .2);
    // float x = timeAsPixel(_mouse_cursor_time);
    // drawLine(x, 0, x, HEIGHT);    
}

void Reframe360TransformInteract::drawTimeline() {
    glColor4f(0,0,0,.4f);

    glColor3f(1, 1, 1);
    glLineWidth(1);

    //drawLine(0, 2*MARGIN, WIDTH, 2*MARGIN);

    // DERIVATE
    for (int x=2*MARGIN; x<WIDTH-2*MARGIN; ++x) {
        int t = pixelAsTime(x);
        auto derivates = _params.getDerivatesAtTime(t);

        for (int i=0; i<6; ++i) {
            auto & param = param_defs[i];
            float y = 20+5+(5-i)*10;

            float d_log = derivates.all[i];
            if (isnormal(d_log)) {
                float a = 2.0f*(d_log+3);
                a = TRUNC_RANGE(a, 0, 6);
                glColor4f(param.color.r, param.color.g, param.color.b, .50f);
                glBegin(GL_LINES);
                glVertex2d(x, y-a);
                glVertex2d(x, y+a);
                glEnd();
            }
        }
    }

    // KEYFRAMES
    for (int t: _params.keyframeTimes()) {
        float t_pixel = timeAsPixel(t);

        // Skip points outside the visible area
        if (t_pixel<2*MARGIN) continue;
        if (t_pixel>WIDTH-2*MARGIN) break;

        // Background
        if (_closest_keyframe_time == t) {
            DrawImageQuad(ICON_BAR_SELECTED , t_pixel-8, 16, 16, 64);
        } else {
            DrawImageQuad(ICON_BAR_UNSELECTED, t_pixel-8, 16, 16, 64);
        }

        // Logos
        const bool b = _selected_keyframe_time == t;
        DrawImageQuad(b && _action==ACTION_KEYFRAME_DELETE ? ICON_DELETE_OVER : ICON_DELETE, t_pixel-8, 16+16*0, 16, 16);
        DrawImageQuad(b && _action==ACTION_KEYFRAME_BF ? ICON_BF_OVER : ICON_BF, t_pixel-8, 16+16*1, 16, 16);
        DrawImageQuad(b && _action==ACTION_KEYFRAME_MOVE ? ICON_MOVE_OVER : ICON_MOVE, t_pixel-8, 16+16*2, 16, 16);
        DrawImageQuad(b && _action==ACTION_KEYFRAME_DUPLICATE ? ICON_DUPLICATE_OVER : ICON_DUPLICATE, t_pixel-8, 16+16*3, 16, 16);

    }

    // TIME CURSOR
    glColor3f(1, .2, .2);
    glLineWidth(2);
    drawLine(_time_pixel, 0, _time_pixel, 2*MARGIN);
    glLineWidth(1);
    if (_action == ACTION_CREATE_KEYFRAME_AT_TIMELINE) {
        // new keyframe button
        DrawImageQuad(ICON_BAR_NEW, _time_pixel-8, 16, 16, 64);
    }
    CHECK_GL_ERROR_NO_ARG();
}

void Reframe360TransformInteract::drawButtons() {
    // Top right items
    const char * mb_rendering_label = NULL;
    switch (globalMotionBlurRendering) {
        case MB_RENDERING_AUTO:
            mb_rendering_label = "AUTO";
            glColor3f(1,1,.2);
            break;
        case MB_RENDERING_ON:
            mb_rendering_label = "ON";
            glColor3f(.4,1,.2);
            break;
        case MB_RENDERING_OFF:
            mb_rendering_label = "OFF";
            glColor3f(1,.4,.2);
            break;
    }
    OFX::TextRenderer::bitmapString(WIDTH-2*MARGIN+m, HEIGHT-MARGIN+m, mb_rendering_label, _font);

    glColor3f(1,1,1);
    char buf[10];
    snprintf(buf, sizeof(buf), "%.0f", _params.getMotionBlur());
    OFX::TextRenderer::bitmapString(WIDTH-2*MARGIN+m, HEIGHT-.5*MARGIN+m, buf, _font);
    OFX::TextRenderer::bitmapString(WIDTH-2*MARGIN+m, HEIGHT-1.5*MARGIN+m, _plugin->_dirty ? "DIRTY" : "SYNC", _font);

    // Buttons
    float x = WIDTH-MARGIN+20;
    _b_curves->x = x;
    _b_render->x = x;
    _b_blur->x = x;

    _b_blur->y = HEIGHT-.25*MARGIN;
    _b_render->y = HEIGHT-.75*MARGIN;
    _b_curves->y = .25*MARGIN;

    _b_curves->draw();
    _b_render->draw();
    _b_blur->draw();
}

bool Reframe360TransformInteract::draw(const OFX::DrawArgs &args)
{
    CHECK_GL_ERROR_NO_ARG();

    spdlog::stopwatch sw;

    glMatrixMode(GL_MODELVIEW);
    refreshVariables(args);
    glLoadIdentity();

    static bool texture_init = false;
    if (!texture_init) {
        initTexture();
        texture_init = true;
    }

    glDisable(GL_LINE_SMOOTH);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glLineWidth(1);
    
    drawFrame();
    if (_display_curves) {
        drawCurves();
    }
    drawTimeline();
    drawButtons();

    if (_msg) {
        glColor3f(1,1,1);
        OFX::TextRenderer::bitmapString(m, m, _msg->c_str(), _font);
    }

    SPDLOG_INFO("RENDER OVERLAY Elapsed {}", sw);
    return true;
}


void Reframe360TransformInteract::refreshVariables(const OFX::PenArgs &args) {
    _mouse_x = args.penPosition.x * _model_view_matrix[0] + _model_view_matrix[12];
    _mouse_y = args.penPosition.y * _model_view_matrix[5] + _model_view_matrix[13];
    _mouse_dx = _mouse_x - _mouse_last_x;
    _mouse_dy = _mouse_y - _mouse_last_y;
    _mouse_last_x = _mouse_x;
    _mouse_last_y = _mouse_y;

    _drawContext.mouse_x = _mouse_x;
    _drawContext.mouse_y = _mouse_y;

    refreshVariables();
}


void Reframe360TransformInteract::refreshVariables(const OFX::DrawArgs &args) {
    glGetFloatv(GL_MODELVIEW_MATRIX, _model_view_matrix);
    // SPDLOG_INFO("matrix {} {} {} {}", _model_view_matrix[0], _model_view_matrix[1], _model_view_matrix[2], _model_view_matrix[3]);
    // SPDLOG_INFO("matrix {} {} {} {}", _model_view_matrix[4], _model_view_matrix[5], _model_view_matrix[6], _model_view_matrix[7]);
    // SPDLOG_INFO("matrix {} {} {} {}", _model_view_matrix[8], _model_view_matrix[9], _model_view_matrix[10], _model_view_matrix[11]);
    // SPDLOG_INFO("matrix {} {} {} {}", _model_view_matrix[12], _model_view_matrix[13], _model_view_matrix[14], _model_view_matrix[15]);

    float m[16];    
    glGetFloatv(GL_PROJECTION_MATRIX, m);
    // SPDLOG_INFO("GL_PROJECTION_MATRIX");
    // SPDLOG_INFO("matrix {} {} {} {}", m[0], m[1], m[2], m[3]);
    // SPDLOG_INFO("matrix {} {} {} {}", m[4], m[5], m[6], m[7]);
    // SPDLOG_INFO("matrix {} {} {} {}", m[8], m[9], m[10], m[11]);
    // SPDLOG_INFO("matrix {} {} {} {}", m[12], m[13], m[14], m[15]);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    //SPDLOG_DEBUG("viewport {} {} {} {}", viewport[0], viewport[1], viewport[2], viewport[3]);
    float viewport_w = viewport[2];
    float viewport_h = viewport[3];

    // Retina fix
    WIDTH = 2/m[0];
    HEIGHT = 2/m[5];
    _hidpi = viewport[2] / WIDTH > 1.5;
    _font = _hidpi ? OFX::TextRenderer::FONT_HELVETICA_18 : OFX::TextRenderer::FONT_FIXED_8_X_13;
    //SPDLOG_DEBUG("WIDTH {} HEIGHT {} HiDPI {}", WIDTH, HEIGHT, _hidpi);

    _drawContext.font = _font;

    _time = args.time;
    double t1, t2;
    _plugin->timeLineGetBounds(t1, t2);
    _t1 = t1;
    _t2 = t2;
    _time_pixel = timeAsPixel(_time);

    // Set the closest keyframe time
    int min_dt = 100000000;
    int closest_keyframe_time = -1;
    
    for (int t: _params.keyframeTimes()) {
        int dt = abs(t - _time);
        if (dt < min_dt && dt < CLOSEST_KEYFRAME_DISTANCE_FRAMES) {
            closest_keyframe_time = t;
            min_dt = dt;
        }
    }
    _closest_keyframe_time = closest_keyframe_time;
    refreshVariables();
}

static const std::string messages[ACTION_MAX] = {
    "", // ACTION_NONE,
    "Click to create global Keyframe at timeline", // ACTION_CREATE_KEYFRAME_AT_TIMELINE,

    "Click + drag: duplicate keyframe", // ACTION_KEYFRAME_DUPLICATE
    "Click + drag: move keyframe", // ACTION_KEYFRAME_MOVE
    "Click: cycle through the blending functions", // ACTION_KEYFRAME_BF
    "Click: delete keyframe", // ACTION_KEYFRAME_DELETE

    "Click & Drag to change Pitch & Yaw", // ACTION_EDIT_PITCH_YAW,
    "Click & Drag to change Pitch", // ACTION_EDIT_PITCH,
    "Click & Drag to change Yaw", // ACTION_EDIT_YAW,
    "Click & Drag to change Roll", // ACTION_EDIT_ROLL,
    "Click & Drag to change FOV", // ACTION_EDIT_FOV,
    "Click & Drag to change Rectilinear Projection", // ACTION_EDIT_RECTILINEAR,
    "Click & Drag to change Tinyplanet", // ACTION_EDIT_TINYPLANET,

    "Click & Drag to create keyframe and change Pitch & Yaw",
    "Click & Drag to create keyframe and change Pitch",
    "Click & Drag to create keyframe and change Yaw",
    "Click & Drag to create keyframe and change Roll",
    "Click & Drag to create keyframe and change FOV",
    "Click & Drag to create keyframe and change Rectilinear Projection",
    "Click & Drag to create keyframe and change Tinyplanet",

    "Click: cycle Pitch blending function",
    "Click: cycle Yaw blending function",
    "Click: cycle Roll blending function",
    "Click: cycle FOV blending function",
    "Click: cycle Rectilinear Projection blending function",
    "Click: cycle Tinyplanet blending function",

    "Click: delete Pitch keyframe",
    "Click: delete Yaw keyframe",
    "Click: delete Roll keyframe",
    "Click: delete FOV keyframe",
    "Click: delete Rectilinear Projection keyframe",
    "Click: delete Tinyplanet keyframe",

    "Click to cycle through Motion Blur values", // ACTION_CYCLE_MOTION_BLUR;
    "Click to toggle Motion Blur rendering. RED: Off. YELLOW: On if resolution >= 6 Mpx (during rendering). GREEN: On", // ACTION_TOGGLE_MOTION_BLUR_RENDERING;
    "Click to display/hide curves",
};


int Reframe360TransformInteract::yMouseAtKeyframe() const {
    for (int t: _params.keyframeTimes()) {
        float t_pixel = timeAsPixel(t);
        if (fabsf(t_pixel - _mouse_x) < 10) {
            return t;
        }
    }

    return -1;
}

void Reframe360TransformInteract::refreshVariables() {
    if (!_pen_down) {
        Action newAction = ACTION_NONE;

        EditedParam newEditedParam = EDITED_PARAM_NONE;

        if (_mouse_x > 2*MARGIN && _mouse_x < WIDTH - 2*MARGIN) {
            if (_mouse_y > 2*MARGIN && _mouse_y < HEIGHT-2*MARGIN) {
                newAction = ACTION_EDIT_PITCH_YAW;
                newEditedParam = EDITED_PARAM_PITCH_YAW;
            }
        }

        for (auto s : _sliders) {
            SliderAction action = s->getAction();
            switch (action) {
            case SLIDER_ACTION_EDIT_PARAM:
                if (_params.hasKeyframeAtTime(s->iParam(), _closest_keyframe_time)) {
                    newAction = (Action)(ACTION_EDIT_PITCH + s->iParam());
                } else {
                    newAction = (Action)(ACTION_NEW_PITCH + s->iParam());
                }
                newEditedParam = (EditedParam)(EDITED_PARAM_PITCH + s->iParam());
                break;
            case SLIDER_ACTION_DELETE_KF:
                newAction = (Action)(ACTION_DELETE_KF_PITCH + s->iParam());
                break;
            case SLIDER_ACTION_CYCLE_BF:
                newAction = (Action)(ACTION_CYCLE_BF_PITCH + s->iParam());
                break;
            case SLIDER_ACTION_MAX:
                break;
            }
        }

        _edited_param = newEditedParam;
        
        if (mouseAtTimeLine() && _closest_keyframe_time < 0) {
            newAction = ACTION_CREATE_KEYFRAME_AT_TIMELINE;
        }

        if (_b_blur->isMouseOver()) {
            newAction = ACTION_CYCLE_MOTION_BLUR;
        }
        if (_b_curves->isMouseOver()) {
            newAction = ACTION_TOGGLE_CURVES_DISPLAY;
        }
        if (_b_render->isMouseOver()) {
            newAction = ACTION_TOGGLE_MOTION_BLUR_RENDERING;
        }

        _selected_keyframe_time = yMouseAtKeyframe();
        if (_selected_keyframe_time >= 0) {
            if (_mouse_y > 16 && _mouse_y < 32) {
                newAction = ACTION_KEYFRAME_DELETE;
            } else if (_mouse_y < 48) {
                newAction = ACTION_KEYFRAME_BF;
            } else if (_mouse_y < 64) {
                newAction = ACTION_KEYFRAME_MOVE;
            } else if (_mouse_y < 96) {
                newAction = ACTION_KEYFRAME_DUPLICATE;
            }

            _duplicated_keyframe_time = -1;
            _keyframe_moved = false;
        }


        // if (_mouse_x < 2*MARGIN && _mouse_y > HEIGHT-2*MARGIN) {
        //     _display_mode = DISPLAY_NONE;
        // } else if (_mouse_y < 2*MARGIN) {
        //     _display_mode = DISPLAY_CURVES;
        // } else {
        //     _display_mode = _shift_toggle ? DISPLAY_FRAME_AND_CURVES : DISPLAY_FRAME;
        // }
        
        _action = newAction;
    }

    float x_selected_keyframe = timeAsPixel(_selected_keyframe_time);
    //_mouse_cursor_time = fabsf(x_selected_keyframe-_mouse_x) < MOUSE_CURSOR_MAGNETIC_PIXELS ? _selected_keyframe_time : pixelAsTime(_mouse_x);

    // printf("Reframe360TransformInteract::refreshVariables: time %f _moved_keyframe_time %f action %d\n", _time, _moved_keyframe_time, _action);
    _msg = &messages[_action];
    // printf("msg: %s\n", _msg.c_str());
}

Button::Button(const DrawContext & context, icon_t icon1, icon_t icon2)
  : _context(context)
  , x(0)
  , y(0)
  , _icon1(icon1)
  , _icon2(icon2) {
    w = icons_sizes[icon1][0]/2;
    h = icons_sizes[icon1][1]/2;
  }

void Button::draw() {
    const float p1X = x - w/2;
    const float p2X = x + w/2;
    const float p1Y = y - h/2;
    const float p2Y = y + h/2;

    //SPDLOG_DEBUG("{} {} pos {} {} {} {}", _icon1, _icon2, p1X, p1Y, p2X, p2Y);
    glEnable(GL_TEXTURE_2D);

    const int icon = isMouseOver() ? _icon2 : _icon1;
    glBindTexture(GL_TEXTURE_2D, texture[icon]);
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f((GLfloat)p1X, (GLfloat)p1Y);
    glTexCoord2f(1, 1); glVertex2f((GLfloat)p2X, (GLfloat)p1Y);
    glTexCoord2f(1, 0); glVertex2f((GLfloat)p2X, (GLfloat)p2Y);
    glTexCoord2f(0, 0); glVertex2f((GLfloat)p1X, (GLfloat)p2Y);
    glEnd();
    CHECK_GL_ERROR("{}", icon);
    glDisable(GL_TEXTURE_2D);
}

bool Button::isMouseOver() const {
    return fabsf(x - _context.mouse_x) < w/2 && fabsf(y - _context.mouse_y) < h/2;
}


void Slider::draw(float v, float kf_v, blend_function_t bf) {
    const ParameterDefinition & p = param_defs[_i_param];
    glColor3f(p.color.r, p.color.g, p.color.b);
    char buf[10];
    snprintf(buf, sizeof(buf), p.format.c_str(), v);
    v = sym_modulo(v, 360);
    v = pixel_from_value_and_range(p.display_opposite ? -v : v, p.display_min, p.display_max, 0, _horizontal ? _w : _h);
    if (_horizontal)
        drawLine(_x0+v, _y0, _x0+v, _y0+_h);
    else
        drawLine(_x0, _y0+v, _x0+_w, _y0+v);

    
    OFX::TextRenderer::bitmapString(_x0+m, _y0+m+m2, buf, _context.font);
    OFX::TextRenderer::bitmapString(_x0+m, _y0+m, p.name_upper.c_str(), _context.font);
    
    // Blending function
    if (bf < BLEND_FUNCTION_MAX) {
        _b_bf.draw();
        const char * buf = "";
        switch (bf) {
            case BLEND_FUNCTION_LINEAR: buf = "LIN"; break;
            case BLEND_FUNCTION_QUADRATIC: buf = "SPLN"; break;
            case BLEND_FUNCTION_MAX: break;
        }
        //snprintf(buf, sizeof(buf), "%d", bf);
        glColor3fv(COLOR_CLOSEST_KEYFRAME);
        OFX::TextRenderer::bitmapString(_x0+m, _y0+m+m2*2, buf, _context.font);
    }

    // Keyframe value
    if (isfinite(kf_v)) {
        snprintf(buf, sizeof(buf), p.format.c_str(), kf_v);
        glColor3fv(COLOR_CLOSEST_KEYFRAME);
        OFX::TextRenderer::bitmapString(_x0+m+(_horizontal?40:0), _y0+m+m2*(_horizontal?2:3), buf, _context.font);

        float v = sym_modulo(kf_v, 360);
        v = pixel_from_value_and_range(p.display_opposite ? -v : v, p.display_min, p.display_max, 0, _horizontal ? _w : _h);

        glColor4fv(COLOR_CLOSEST_KEYFRAME);
        if (_horizontal)
            drawPoint(_x0 + v, _y0 + _h/2, 10);
        else
            drawPoint(_x0 + _w/2, _y0 + v, 10);

        _b_del_kf.draw();
    }
    
}

SliderAction Slider::getAction() const {
    if (_b_bf.isMouseOver())
        return SLIDER_ACTION_CYCLE_BF;
    if (_b_del_kf.isMouseOver())
        return SLIDER_ACTION_DELETE_KF;
    
    if (_context.mouse_x < _x0 || _context.mouse_x > _x0+_w)
        return SLIDER_ACTION_MAX;
    if (_context.mouse_y < _y0 || _context.mouse_y > _y0+_h)
        return SLIDER_ACTION_MAX;
    return SLIDER_ACTION_EDIT_PARAM;
}











