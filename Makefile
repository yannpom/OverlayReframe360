ARCH_FLAGS = -arch arm64 -arch x86_64

OFXPATH = /Library/Application\ Support/Blackmagic\ Design/DaVinci\ Resolve/Developer/OpenFX
GLMPATH = glm
CXXFLAGS = ${ARCH_FLAGS} -DDEBUG -std=c++11 -Wno-deprecated-declarations -fvisibility=hidden -Iopenfx-supportext -I$(OFXPATH)/Support/include -I$(OFXPATH)/Support/Plugins/include -I$(OFXPATH)/OpenFX-1.4/include -I$(GLMPATH) -Ispdlog/include -Ijson/single_include -Iimages -I/opt/homebrew/Cellar/rtmidi/5.0.0/include
CFLAGS = ${ARCH_FLAGS}
LDFLAGS = ${ARCH_FLAGS} -bundle -fvisibility=hidden -F/Library/Frameworks -framework OpenCL -framework OpenGL -framework CoreAudio -framework CoreMIDI -framework Metal -framework AppKit -lspdlog -Lspdlog/build

BUNDLE_DIR = build/OverlayReframe360.ofx.bundle/Contents/MacOS/

METAL_OBJ = build/MetalKernel.o
OPENCL_OBJ = build/OpenCLKernel.o

SRC_M_FILES := $(wildcard src/*.m)
SRC_CPP_FILES := $(wildcard src/*.cpp)
SRC_EXT_CPP_FILES := ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp ofxsCore.cpp ofxsImageEffect.cpp ofxsInteract.cpp ofxsLog.cpp ofxsMultiThread.cpp ofxsParams.cpp ofxsProperty.cpp ofxsPropertyValidation.cpp

OBJ_FILES := $(patsubst src/%.m,build/%.o,$(SRC_M_FILES)) $(patsubst src/%.cpp,build/%.o,$(SRC_CPP_FILES)) $(patsubst %.cpp,build/%.o,$(SRC_EXT_CPP_FILES))

PNG_FILES := $(wildcard images/*.png)
PNG_H_FILES := $(patsubst images/%.png,images/%.png.h,$(PNG_FILES))


.PHONY: directories
directories: build Library
build:
	mkdir -p $@

Library:
	cp -a $(OFXPATH)/Support/Library .

build/OverlayReframe360.ofx: $(OBJ_FILES) $(OPENCL_OBJ) $(METAL_OBJ)
	$(CXX) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.cpp $(PNG_H_FILES)
	$(CXX) -c $< $(CXXFLAGS) -o $@

build/%.o: src/%.m
	$(CC) -c $< $(CFLAGS) -o $@

images/%.png.h: images/%.png
	xxd --include $< > $@

#gen_images: $(PNG_H_FILES)


build/%.o: openfx-supportext/%.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

build/%.o: Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

src/MetalKernel.h: src/MetalKernel.metal
	python metal2string.py $< $@

$(METAL_OBJ): src/MetalKernel.mm src/MetalKernel.h
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(OPENCL_OBJ): src/OpenCLKernel.cpp src/OpenCLKernel.h 
	$(CXX) -c $< $(CXXFLAGS) -o $@

src/OpenCLKernel.h: src/OpenCLKernel.cl
	python HardcodeKernel.py src/OpenCLKernel $<

%.metallib: %.metal
	xcrun -sdk macosx metal -c $< -o $@
	mkdir -p $(BUNDLE_DIR)
	cp $@ $(BUNDLE_DIR)

clean:
	rm -rf build

install: build/OverlayReframe360.ofx
	rm -rf /Library/OFX/Plugins/OverlayReframe360.ofx.bundle
	mkdir -p $(BUNDLE_DIR)
	cp build/OverlayReframe360.ofx $(BUNDLE_DIR)
	cp -a build/OverlayReframe360.ofx.bundle /Library/OFX/Plugins/

.DEFAULT_GOAL := all
.PHONY: all
all: directories install
