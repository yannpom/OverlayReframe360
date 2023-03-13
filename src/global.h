#pragma once

#define kPluginName "OverlayReframe360"
#define kPluginGrouping "360"
#define kPluginDescription "Reframe 360 video with your mouse using Open FX Overlay."
#define kPluginIdentifier "com.yfx.OverlayReframe360"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 1

#define MIN(x,y) ( ((x) <= (y)) ? (x) : (y) )
#define MAX(x,y) ( ((x) >= (y)) ? (x) : (y) )
#define TRUNC_RANGE(x, y, z) MIN(MAX((x),  y), z)

#define MIDI_MESSAGE_MAX_AGE_MICROSECONDS 200000

#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>


#define CLOSEST_KEYFRAME_DISTANCE_FRAMES 10
//#define MOUSE_CURSOR_MAGNETIC_PIXELS 10

#define MOTION_BLUR_AUTO_PIXELS 6000000