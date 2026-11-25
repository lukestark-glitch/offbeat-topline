#include "Params.hpp"

namespace offbeat {

const char* const kModeNames[kModeCount] = { "Supersaw", "Stab", "Pluck", "Whistle" };
static const char* const kVoiceModeNames[2] = { "Poly", "Mono" };
static const char* const kChordNames[5] = { "Aus", "Moll", "Dur", "Moll7", "Dur7" };
static const char* const kDelayNames[5] = { "1/16", "1/8", "1/8.", "1/4", "1/4." };
const float kDelayBeats[5] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f };

// clang-format off
const ParamInfo kParams[kParamCount] = {
    // name                  symbol              unit    min     max      default  flags
    { "Mode",                "mode",             "",     0,      3,       0,       kFlagInt, kModeNames },
    { "Volume",              "volume",           "dB",   -40,    6,       -6,      0 },
    { "Octave",              "octave",           "",     -2,     2,       0,       kFlagInt },
    { "Voices",              "voice_mode",       "",     0,      1,       0,       kFlagInt, kVoiceModeNames },
    { "Glide",               "glide",            "ms",   0,      500,     0,       0 },
    { "Chord",               "chord",            "",     0,      4,       0,       kFlagInt, kChordNames },
    { "Vibrato Depth",       "vib_depth",        "st",   0,      1,       0,       0 },
    { "Vibrato Rate",        "vib_rate",         "Hz",   1,      10,      5.5f,    0 },
    { "Vibrato Delay",       "vib_delay",        "ms",   0,      1000,    250,     0 },

    { "Chorus",              "chorus",           "",     0,      1,       0.3f,    0 },
    { "Delay Mix",           "delay_mix",        "",     0,      1,       0.15f,   0 },
    { "Delay Time",          "delay_time",       "",     0,      4,       2,       kFlagInt, kDelayNames },
    { "Delay Feedback",      "delay_fb",         "",     0,      0.9f,    0.35f,   0 },
    { "Reverb Mix",          "reverb_mix",       "",     0,      1,       0.25f,   0 },
    { "Reverb Size",         "reverb_size",      "",     0,      1,       0.6f,    0 },
    { "Tempo from Host",     "tempo_host",       "",     0,      1,       1,       kFlagBool },
    { "Tempo BPM",           "tempo_bpm",        "BPM",  60,     220,     160,     0 },

    { "Saw Voices",          "saw_voices",       "",     1,      7,       7,       kFlagInt },
    { "Saw Detune",          "saw_detune",       "",     0,      1,       0.35f,   0 },
    { "Saw Cutoff",          "saw_cutoff",       "Hz",   200,    18000,   7000,    kFlagLog },
    { "Saw Resonance",       "saw_reso",         "",     0,      1,       0.1f,    0 },
    { "Saw Filter Env",      "saw_envamt",       "",     0,      1,       0.2f,    0 },
    { "Saw Filter Decay",    "saw_fdecay",       "ms",   5,      3000,    400,     kFlagLog },
    { "Saw Attack",          "saw_attack",       "ms",   0.5f,   2000,    8,       kFlagLog },
    { "Saw Decay",           "saw_decay",        "ms",   5,      3000,    600,     kFlagLog },
    { "Saw Sustain",         "saw_sustain",      "",     0,      1,       0.8f,    0 },
    { "Saw Release",         "saw_release",      "ms",   10,     5000,    700,     kFlagLog },
    { "Saw Width",           "saw_width",        "",     0,      1,       0.65f,   0 },

    { "Stab Voices",         "stab_voices",      "",     1,      7,       5,       kFlagInt },
    { "Stab Detune",         "stab_detune",      "",     0,      1,       0.25f,   0 },
    { "Stab Cutoff",         "stab_cutoff",      "Hz",   200,    18000,   3500,    kFlagLog },
    { "Stab Filter Env",     "stab_envamt",      "",     0,      1,       0.5f,    0 },
    { "Stab Filter Decay",   "stab_fdecay",      "ms",   5,      1000,    90,      kFlagLog },
    { "Stab Decay",          "stab_decay",       "ms",   20,     1000,    140,     kFlagLog },
    { "Stab Release",        "stab_release",     "ms",   5,      1000,    60,      kFlagLog },
    { "Stab Shine",          "stab_shine",       "dB",   0,      12,      4,       0 },
    { "Stab Shine Freq",     "stab_shine_hz",    "Hz",   4000,   8000,    6000,    kFlagLog },
    { "Stab Width",          "stab_width",       "",     0,      1,       0.6f,    0 },

    { "Pluck Tone",          "pluck_tone",       "Hz",   200,    8000,    1500,    kFlagLog },
    { "Pluck Filter Env",    "pluck_envamt",     "",     0,      1,       0.6f,    0 },
    { "Pluck Decay",         "pluck_decay",      "ms",   20,     600,     130,      kFlagLog },
    { "Pluck Body",          "pluck_body",       "dB",   0,      12,      6,       0 },
    { "Pluck Detune",        "pluck_detune",     "",     0,      1,       0.15f,   0 },
    { "Pluck Width",         "pluck_width",      "",     0,      1,       0.45f,   0 },

    { "Whistle Breath",      "whistle_breath",   "",     0,      1,       0.15f,   0 },
    { "Whistle Scoop",       "whistle_scoop",    "st",   0,      3,       0.8f,    0 },
    { "Whistle Scoop Time",  "whistle_scoop_ms", "ms",   5,      100,     30,      0 },
    { "Whistle Trill",       "whistle_trill",    "st",   0,      3,       0,       0 },
    { "Whistle Trill Rate",  "whistle_trill_hz", "Hz",   2,      20,      12,      0 },
    { "Whistle Length",      "whistle_length",   "ms",   50,     2000,    180,     kFlagLog },
    { "Whistle Width",       "whistle_width",    "",     0,      1,       0.1f,    0 },
};

namespace {
using V = Preset::Value;

const V sawMinor[]   = { { kMode, kModeSaw }, { kChord, 1 }, { kReverbMix, 0.35f }, { kDelayMix, 0.2f } };
const V sawMajor[]   = { { kMode, kModeSaw }, { kChord, 2 }, { kSawCutoff, 9000 }, { kSawDetune, 0.45f },
                         { kReverbMix, 0.35f }, { kDelayMix, 0.2f } };
const V leadMono[]   = { { kMode, kModeSaw }, { kVoiceMode, 1 }, { kGlide, 90 }, { kVibDepth, 0.25f },
                         { kSawVoices, 5 }, { kSawDetune, 0.25f }, { kSawRelease, 600 }, { kReverbMix, 0.25f } };
const V stabMinor[]  = { { kMode, kModeStab }, { kChord, 1 }, { kReverbMix, 0.15f }, { kDelayMix, 0.1f } };
const V stabMajor[]  = { { kMode, kModeStab }, { kChord, 2 }, { kStabShine, 8 }, { kStabShineHz, 7000 },
                         { kReverbMix, 0.15f }, { kDelayMix, 0.1f } };
const V stabMinor7[] = { { kMode, kModeStab }, { kChord, 3 }, { kStabDecay, 180 }, { kStabCutoff, 2800 },
                         { kReverbMix, 0.2f } };
const V pluckMinor[] = { { kMode, kModePluck }, { kReverbMix, 0.12f }, { kDelayMix, 0.2f }, { kChorus, 0 } };
const V pluckMajor[] = { { kMode, kModePluck }, { kChord, 2 }, { kPluckTone, 2000 }, { kReverbMix, 0.12f },
                         { kDelayMix, 0.15f }, { kChorus, 0 } };
const V whistle[]    = { { kMode, kModeWhistle }, { kReverbMix, 0.2f }, { kDelayMix, 0.15f }, { kChorus, 0 } };
const V whistleTrill[] = { { kMode, kModeWhistle }, { kWhistleTrill, 2 }, { kWhistleLength, 600 },
                           { kReverbMix, 0.2f }, { kDelayMix, 0.1f }, { kChorus, 0 } };

#define PRESET(name, arr) { name, arr, (int)(sizeof(arr) / sizeof(arr[0])) }
} // namespace

const Preset kPresets[] = {
    PRESET("Supersaw - F-Moll Hymne (F5)", sawMinor),
    PRESET("Supersaw - E-Dur Euphorie (E5)", sawMajor),
    PRESET("Lead - Mono Glide", leadMono),
    PRESET("Stab - F-Moll Rave (F5)", stabMinor),
    PRESET("Stab - A-Dur Shine (A4)", stabMajor),
    PRESET("Stab - F-Moll7 Groove (F5)", stabMinor7),
    PRESET("Pluck - F-Moll (F5)", pluckMinor),
    PRESET("Pluck - E-Dur Chords (E5)", pluckMajor),
    PRESET("Whistle - Rave Pfeife (D#7)", whistle),
    PRESET("Whistle - Triller (D#7)", whistleTrill),
};
// clang-format on

const int kPresetCount = (int)(sizeof(kPresets) / sizeof(kPresets[0]));

int modeParams(int mode, int out[16])
{
    static const int first[kModeCount + 1] = { kSawVoices, kStabVoices, kPluckTone, kWhistleBreath, kWhistleWidth + 1 };
    int n = 0;
    for (int p = first[mode]; p < first[mode + 1]; ++p)
        out[n++] = p;
    return n;
}

void applyDefaults(float out[kParamCount])
{
    for (int i = 0; i < kParamCount; ++i)
        out[i] = kParams[i].def;
}

void applyPreset(int index, float out[kParamCount])
{
    applyDefaults(out);
    if (index < 0 || index >= kPresetCount)
        return;
    const Preset& p = kPresets[index];
    for (int i = 0; i < p.count; ++i)
        out[p.values[i].id] = p.values[i].value;
}

} // namespace offbeat
