#pragma once

#include "Reframe360.h"
#include "ofxsImageEffect.h"
#include <ofxsOGLTextRenderer.h>
#include <string>


enum EditedParam {
    EDITED_PARAM_NONE,
    EDITED_PARAM_MOTION_BLUR,
    EDITED_PARAM_MOTION_BLUR_RENDERING,
    EDITED_PARAM_PITCH_YAW,

    EDITED_PARAM_PITCH,
    EDITED_PARAM_YAW,
    EDITED_PARAM_ROLL,
    EDITED_PARAM_FOV,
    EDITED_PARAM_RECTILINEAR,
    EDITED_PARAM_TINYPLANET
};



enum Action {
    ACTION_NONE,
    ACTION_CREATE_KEYFRAME_AT_TIMELINE,

    ACTION_KEYFRAME_DUPLICATE,
    ACTION_KEYFRAME_MOVE,
    ACTION_KEYFRAME_BF,
    ACTION_KEYFRAME_DELETE,

    ACTION_EDIT_PITCH_YAW,
    ACTION_EDIT_PITCH,
    ACTION_EDIT_YAW,
    ACTION_EDIT_ROLL,
    ACTION_EDIT_FOV,
    ACTION_EDIT_RECTILINEAR,
    ACTION_EDIT_TINYPLANET,

    ACTION_NEW_PITCH_YAW,
    ACTION_NEW_PITCH,
    ACTION_NEW_YAW,
    ACTION_NEW_ROLL,
    ACTION_NEW_FOV,
    ACTION_NEW_RECTILINEAR,
    ACTION_NEW_TINYPLANET,

    ACTION_CYCLE_BF_PITCH,
    ACTION_CYCLE_BF_YAW,
    ACTION_CYCLE_BF_ROLL,
    ACTION_CYCLE_BF_FOV,
    ACTION_CYCLE_BF_RECTILINEAR,
    ACTION_CYCLE_BF_TINYPLANET,

    ACTION_DELETE_KF_PITCH,
    ACTION_DELETE_KF_YAW,
    ACTION_DELETE_KF_ROLL,
    ACTION_DELETE_KF_FOV,
    ACTION_DELETE_KF_RECTILINEAR,
    ACTION_DELETE_KF_TINYPLANET,

    ACTION_CYCLE_MOTION_BLUR,
    ACTION_TOGGLE_MOTION_BLUR_RENDERING,
    ACTION_TOGGLE_CURVES_DISPLAY,

    ACTION_MAX
};

enum icon_t {
    ICON_BAR_UNSELECTED,
    ICON_BAR_SELECTED,
    ICON_BAR_NEW,
    ICON_DUPLICATE,
    ICON_DUPLICATE_OVER,
    ICON_MOVE,
    ICON_MOVE_OVER,
    ICON_BF,
    ICON_BF_OVER,
    ICON_DELETE,
    ICON_DELETE_OVER,
    // buttons
    ICON_CURV,
    ICON_CURV_OVER,
    ICON_BLUR,
    ICON_BLUR_OVER,
    ICON_REND,
    ICON_REND_OVER,
    ICON_MAX
};


enum SliderAction {
    SLIDER_ACTION_DELETE_KF,
    SLIDER_ACTION_CYCLE_BF,
    SLIDER_ACTION_EDIT_PARAM,
    SLIDER_ACTION_MAX
};

struct DrawContext {
    float mouse_x;
    float mouse_y;
    OFX::TextRenderer::Font font;
};

class Button {
public:
    Button(const DrawContext & context, icon_t icon1, icon_t icon2);

    void draw();
    bool isMouseOver() const;

    float x;
    float y;

private:
    float w;
    float h;
    const DrawContext & _context;
    icon_t _icon1;
    icon_t _icon2;
};

class Reframe360TransformInteract;

class Slider {
public:
    Slider(const DrawContext & context, int i_param, bool horizontal)
        : _context(context)
        , _i_param(i_param)
        , _horizontal(horizontal)
        , _b_del_kf(context, ICON_DELETE, ICON_DELETE_OVER)
        , _b_bf(context, ICON_BF, ICON_BF_OVER)
    {}

    void setPosition(float x0, float y0, float w, float h) {
        _x0 = x0;
        _y0 = y0;
        _w = w;
        _h = h;

        if (_horizontal) {
            _b_del_kf.x = x0 + w -.25*h;
            _b_del_kf.y = y0 + .75*h;
            _b_bf.x = _b_del_kf.x;
            _b_bf.y = y0 + .25*h;
        } else {
            _b_del_kf.x = x0 + .25*w;
            _b_del_kf.y = y0 + h -.25*w;
            _b_bf.x = x0 + .75*w;
            _b_bf.y = _b_del_kf.y;
        }
    }

    SliderAction getAction() const;
    inline int iParam() const { return _i_param; }

    void draw(float value, float kf_value, blend_function_t bf);

private:
    const DrawContext & _context;
    int _i_param;
    bool _horizontal;
    float _x0;
    float _y0;
    float _w;
    float _h;

    Button _b_del_kf;
    Button _b_bf;
};


class Reframe360TransformInteract : public OFX::OverlayInteract
{
public:

    Reframe360TransformInteract(OfxInteractHandle handle, OFX::ImageEffect* effect);

    virtual bool penMotion(const OFX::PenArgs &args);
    virtual bool penDown(const OFX::PenArgs &args);
    virtual bool penUp(const OFX::PenArgs &args);

    virtual bool keyDown(const OFX::KeyArgs &args);
    virtual bool keyUp(const OFX::KeyArgs &args);

    virtual bool draw(const OFX::DrawArgs &args);

private:
    void drawFrame();
    void drawCurves();
    void drawCurve(int i_param, int t1, int t2);
    void drawTimeline();
    void drawButtons();
    float timeAsPixel(int t) const;
    int pixelAsTime(float p) const;
    EditedParam get_param_from_zone(float x, float y);

    Reframe360* _plugin;

    bool _pen_down;
    uint64_t _last_pen_up_ts;
    
    int _selected_keyframe_time; // in the timeline
    int _closest_keyframe_time; // in the viewer
    EditedParam _edited_param;

    Params & _params;

    bool _hidpi;
    OFX::TextRenderer::Font _font;

    bool mouseAtTimeLine() const { return fabsf(_time_pixel - _mouse_x) < 10 && fabsf(_mouse_y - MARGIN) < MARGIN; }
    int yMouseAtKeyframe() const;

    void refreshVariables(const OFX::PenArgs &args);
    void refreshVariables(const OFX::DrawArgs &args);
    void refreshVariables();

    bool _display_curves;

    bool _shift_down;
    int _duplicated_keyframe_time;
    bool _keyframe_moved;

    bool _render_needed;

    float _model_view_matrix[16];

    float _mouse_x;
    float _mouse_y;
    float _mouse_dx;
    float _mouse_dy;
    float _mouse_last_x;
    float _mouse_last_y;
    float _mouse_down_x;
    float _mouse_down_y;
    int _t1;
    int _t2;
    int _time;
    float _time_pixel;

    float WIDTH;
    float HEIGHT;
    float MARGIN;

    Action _action;
    const std::string * _msg;

    DrawContext _drawContext;

    std::vector<Slider*> _sliders;
    Button* _b_blur;
    Button* _b_curves;
    Button* _b_render;
};

