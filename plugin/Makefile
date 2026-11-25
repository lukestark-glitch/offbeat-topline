#!/usr/bin/make -f
# DPF-Build des Plugins. Aufruf über das Makefile im Projektordner.

NAME = offbeat-topline

FILES_DSP = \
	ToplinePlugin.cpp \
	../src/dsp/Engine.cpp \
	../src/dsp/Params.cpp \
	../src/dsp/ParamText.cpp

# Params.cpp und ParamText.cpp nutzt auch die UI; sie steckt bei VST3 und LV2 (eine Datei) im selben Binary.
FILES_UI = ToplineUI.cpp

DPF_PATH = ../dpf
DPF_BUILD_DIR = ../build/offbeat-topline
DPF_TARGET_DIR = ../bin
include ../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += -std=gnu++17 -I../src/dsp -I$(DPF_BUILD_DIR)

# Motorrad-Bild für die Oberfläche als C-Array einbetten
$(DPF_BUILD_DIR)/BikeImage.h: ../assets/bike.png
	-@mkdir -p $(DPF_BUILD_DIR)
	cd ../assets && xxd -i bike.png > $(abspath $@)

$(DPF_BUILD_DIR)/ToplineUI.cpp.o: $(DPF_BUILD_DIR)/BikeImage.h

TARGETS += vst3
TARGETS += lv2

all: $(TARGETS)
