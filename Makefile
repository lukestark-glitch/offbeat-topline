# Offbeat Topline – Build für Plugin (VST3, VST2, AU, CLAP, LV2) und Offline-Werkzeug topline-render.
#   make             Plugin + topline-render (macOS: Universal-Binary für Intel und Apple Silicon;
#                    make UNIVERSAL=false nur für die eigene Architektur)
#   make install     macOS: nach ~/Library/Audio/Plug-Ins kopieren und ad-hoc signieren
#   make check       Abnahme mit topline-render --check-all
#   make standalone  Standalone-App (z. B. für Bilder der Oberfläche, siehe docs/CODE.md)
# Windows (MSYS2/MinGW) und Linux: `make` baut nach bin/; Installation siehe README.

NAME = offbeat-topline
CXX ?= c++
DSP_SRC = src/dsp/Engine.cpp src/dsp/Params.cpp src/dsp/ParamText.cpp src/dsp/Preview.cpp
DSP_HDR = $(wildcard src/dsp/*.hpp) tools/Measure.hpp
PLUGINS_DIR = $(HOME)/Library/Audio/Plug-Ins

# UNIVERSAL=false baut auf dem Mac nur für die eigene Architektur (z. B. schneller auf einem Intel-Mac).
UNIVERSAL ?= true
ifeq ($(shell uname -s)$(UNIVERSAL),Darwintrue)
# Universal-Binary: läuft auf Intel- und Apple-Silicon-Macs. NOOPT: ohne x86-only-Flags (-mfpmath=sse).
ARCH_FLAGS = -arch x86_64 -arch arm64 -mmacosx-version-min=10.15
PLUGIN_ENV = CFLAGS="$(ARCH_FLAGS)" CXXFLAGS="$(ARCH_FLAGS)" LDFLAGS="$(ARCH_FLAGS)" NOOPT=true
endif

all: plugin render

plugin:
	$(PLUGIN_ENV) $(MAKE) -C plugin
	$(MAKE) -C dpf/utils/lv2-ttl-generator
	dpf/utils/generate-ttl.sh $(CURDIR)/bin

standalone:
	$(PLUGIN_ENV) $(MAKE) -C plugin jack

render: bin/topline-render

bin/topline-render: tools/topline-render.cpp $(DSP_SRC) $(DSP_HDR)
	@mkdir -p bin
	$(CXX) -std=gnu++17 -O2 -Wall -Wextra -Isrc/dsp -Itools tools/topline-render.cpp $(DSP_SRC) -o $@

check: render
	bin/topline-render --meta-check
	bin/topline-render --check-all
	bin/topline-render --preview-check

install: plugin
	rm -rf "$(PLUGINS_DIR)/VST3/$(NAME).vst3" "$(PLUGINS_DIR)/LV2/$(NAME).lv2" "$(PLUGINS_DIR)/VST/$(NAME).vst" \
	       "$(PLUGINS_DIR)/Components/$(NAME).component" "$(PLUGINS_DIR)/CLAP/$(NAME).clap"
	mkdir -p "$(PLUGINS_DIR)/VST3" "$(PLUGINS_DIR)/LV2" "$(PLUGINS_DIR)/VST" "$(PLUGINS_DIR)/Components" "$(PLUGINS_DIR)/CLAP"
	cp -R bin/$(NAME).vst3 "$(PLUGINS_DIR)/VST3/"
	cp -R bin/$(NAME).lv2 "$(PLUGINS_DIR)/LV2/"
	cp -R bin/$(NAME).vst "$(PLUGINS_DIR)/VST/"
	cp -R bin/$(NAME).component "$(PLUGINS_DIR)/Components/"
	cp -R bin/$(NAME).clap "$(PLUGINS_DIR)/CLAP/"
	codesign --force --deep -s - "$(PLUGINS_DIR)/VST3/$(NAME).vst3"
	codesign --force -s - "$(PLUGINS_DIR)/LV2/$(NAME).lv2/$(NAME).dylib"  # LV2-Ordner ist kein Bundle
	codesign --force --deep -s - "$(PLUGINS_DIR)/VST/$(NAME).vst"
	codesign --force --deep -s - "$(PLUGINS_DIR)/Components/$(NAME).component"
	codesign --force --deep -s - "$(PLUGINS_DIR)/CLAP/$(NAME).clap"
	killall -9 AudioComponentRegistrar 2>/dev/null || true  # AU-Liste neu einlesen
	rm -rf "$(HOME)/Applications/Offbeat Topline.app"  # Standalone-App
	mkdir -p "$(HOME)/Applications"
	cp -R bin/$(NAME).app "$(HOME)/Applications/Offbeat Topline.app"
	codesign --force --deep -s - "$(HOME)/Applications/Offbeat Topline.app"

clean:
	rm -rf bin build

.PHONY: all plugin standalone render check install clean
