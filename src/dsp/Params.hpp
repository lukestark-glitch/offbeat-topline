// Parameter-Liste von Offbeat Topline. Neue Parameter immer hinten anhängen: die Nummer ist die
// Parameter-ID in LV2/VST3, gespeicherte Projekte hängen daran.
#pragma once

#include "ParamInfo.hpp"

namespace offbeat {

enum Mode { kModeSaw, kModeStab, kModePluck, kModeWhistle, kModeCount };

enum ParamId {
    // Global
    kMode,
    kVolume,
    kOctave, // LMMS schickt Noten 12 Halbtöne tiefer als angezeigt
    kVoiceMode,
    kGlide,
    kChord,
    kVibDepth,
    kVibRate,
    kVibDelay,
    // Effekte
    kChorus,
    kDelayMix,
    kDelayTime,
    kDelayFeedback,
    kReverbMix,
    kReverbSize,
    kTempoHost,
    kTempoBpm,
    // Supersaw
    kSawVoices,
    kSawDetune,
    kSawCutoff,
    kSawReso,
    kSawEnvAmt,
    kSawFiltDecay,
    kSawAttack,
    kSawDecay,
    kSawSustain,
    kSawRelease,
    kSawWidth,
    // Stab
    kStabVoices,
    kStabDetune,
    kStabCutoff,
    kStabEnvAmt,
    kStabFiltDecay,
    kStabDecay,
    kStabRelease,
    kStabShine,
    kStabShineHz,
    kStabWidth,
    // Pluck
    kPluckTone,
    kPluckEnvAmt,
    kPluckDecay,
    kPluckBody,
    kPluckDetune,
    kPluckWidth,
    // Whistle
    kWhistleBreath,
    kWhistleScoop,
    kWhistleScoopTime,
    kWhistleTrill,
    kWhistleTrillRate,
    kWhistleLength,
    kWhistleWidth,

    kParamCount
};

extern const ParamInfo kParams[kParamCount];
extern const char* const kModeNames[kModeCount];

// Delay-Zeiten in Schlägen (Index = Parameterwert): 1/16, 1/8, 1/8 punktiert, 1/4, 1/4 punktiert.
extern const float kDelayBeats[5];

struct Preset {
    const char* name;
    struct Value { ParamId id; float value; };
    const Value* values;
    int count;
};

extern const Preset kPresets[];
extern const int kPresetCount;

// Schreibt alle Parameter-Werte des Presets (Defaults + Abweichungen) nach `out`.
void applyPreset(int index, float out[kParamCount]);
void applyDefaults(float out[kParamCount]);

// Parameter eines Modus in Anzeige-Reihenfolge; Rückgabe: Anzahl (höchstens 16).
int modeParams(int mode, int out[16]);

// Parameter, die KOPIEREN (und topline/offbeat-render --preset-json) als JSON ausgibt.
int jsonParams(const float values[kParamCount], int* out);

} // namespace offbeat
