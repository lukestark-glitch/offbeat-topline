# Offbeat Topline – Build für Plugin (VST3 + LV2, x86_64) und Offline-Renderer.
#   make            Plugin + topline-render
#   make install    nach ~/Library/Audio/Plug-Ins kopieren und ad-hoc signieren
#   make check      Abnahme aller Presets mit topline-render

CXX ?= c++
DSP_SRC = src/dsp/Engine.cpp src/dsp/Params.cpp src/dsp/ParamText.cpp
DSP_HDR = $(wildcard src/dsp/*.hpp) tools/Measure.hpp
VST3_DIR = $(HOME)/Library/Audio/Plug-Ins/VST3
LV2_DIR  = $(HOME)/Library/Audio/Plug-Ins/LV2

all: plugin render

plugin:
	$(MAKE) -C plugin
	$(MAKE) -C dpf/utils/lv2-ttl-generator
	dpf/utils/generate-ttl.sh $(CURDIR)/bin

render: bin/topline-render

bin/topline-render: tools/topline-render.cpp $(DSP_SRC) $(DSP_HDR)
	@mkdir -p bin
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc/dsp tools/topline-render.cpp $(DSP_SRC) -o $@

check: render
	bin/topline-render --check-all

install: plugin
	rm -rf "$(VST3_DIR)/offbeat-topline.vst3" "$(LV2_DIR)/offbeat-topline.lv2"
	mkdir -p "$(VST3_DIR)" "$(LV2_DIR)"
	cp -R bin/offbeat-topline.vst3 "$(VST3_DIR)/"
	cp -R bin/offbeat-topline.lv2 "$(LV2_DIR)/"
	codesign --force --deep -s - "$(VST3_DIR)/offbeat-topline.vst3"
	codesign --force -s - "$(LV2_DIR)/offbeat-topline.lv2/offbeat-topline.dylib"  # LV2-Ordner ist kein Bundle

clean:
	rm -rf bin build

.PHONY: all plugin render check install clean
