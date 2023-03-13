# OverlayReframe360

This plugin is a rewrite of the excellent Reframe360XL plugin by Sylvain Gravel: https://github.com/LRP-sgravel/reframe360XL

The goal was to experiment with the OpenFX overlay interaction API to reframe using click&drag direcly on the video.
I ended up completely rewriting the logic and kept only kernel part (OpenCL & Metal algorithm).

Features:
- Overlay interaction with trackpad/mouse
- Global & individual param keyframes
- Linear or spline interpolation between keyframes
  - Interpolation can be different on the same keyframe for different param: you can have a linear Yaw move while doing a spline Pitch move.
- Easy to move a keyframe position in time.
- Duplicate existing keyframe
- Curve viewer to view/edit each param movement over time.

Bug fixes:
- Fix Metal kernel on M1 mac.

Enjoy!


# Build on MacOS (Intel and Apple Silicon)

Build tested on macOS Ventura 13.2 / Resolve 18.1.

- Install Blackmagic DaVinci Resolve from Blackmagic website (free or studio version).
- Clone this repository:
`git clone https://github.com/yannpom/OverlayReframe360.git`
- Go into dir:
`cd OverlayReframe360`
- Checkout submodules
`git submodule update --init`
- Build `spdlog`:
```
cd spdlog
mkdir build
cd build
cmake ..
make
cd ../..
```
- Build & Install (copy to `/Library/OFX/Plugins`)
`make`

# Installation for MacOS

- Download the latest [release](https://github.com/yannpom/OverlayReframe360/releases) for Mac
- Uncompress to `/Library/OFX/Plugins`

# Installation for Windows

Rewriting the Makefile I broke the Windows build. If you are interested by a Windows build do not hesitate to fix the Makefile and test.
