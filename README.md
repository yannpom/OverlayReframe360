# Reframe360 XL

This plugin is an extension of the excellent Reframe360 (http://reframe360.com/) plugin by Stefan Sietzen.  When Stefan discontinued the project and released the source code, I got to work to add features and fix bugs and it became Reframe360 XL.  XL is used to point out that new features were put in place and is also a nudge to the excellent SkaterXL game.

New features and bug fixes:
- New animation curves implemented (Sine, Expo, Circular)
- Apply animation curves to main camera parameters
- Fix black dot in center of output
- Update to latest libraries and Resolve 17

Enjoy!

# Build on Windows
* Install latest Visual Studio 2019.
* Install Blackmagic DaVinci Resolve from Blackmagic website (free or studio version).
* Clone GLM repository to `.\GLM` directory. (https://github.com/bclarksoftware/glm.git)
* Clone this repository to `.\Reframe360XL` directory.
* Install CUDA Development Toolkit 11.3.
* Install latest Python and make sure system path are setup.
* Open solution and build from VS.

# Build on MacOS (Intel and Apple Silicon)
* Build tested on MacOS 11.2.3 / XCode 12.4.
* Install latest XCode from Apple App store.
* Install Blackmagic DaVinci Resolve from Blackmagic website (free or studio version).
* Clone glm repository to `./GLM` directory. (https://github.com/bclarksoftware/glm.git)
* Clone this repository to `./Reframe360XL` directory.
* Build from shell with
````
cd Reframe360XL
make
````

# Installation for Windows
* Download the latest [release](https://github.com/LRP-sgravel/reframe360XL/releases) for Windows
* Uncompress to `C:\Program Files\Common Files\OFX\Plugins\Reframe360`

# Installation for MacOS
* Download the latest [release](https://github.com/LRP-sgravel/reframe360XL/releases) for Mac
* Uncompress to `/Library/OFX/Plugins`
