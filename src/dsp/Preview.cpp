#include "Preview.hpp"

#include <cmath>
#include <memory>

namespace offbeat {

int previewEvents(const float* p, bool phrase, double bpm, int sr, NoteEvent* out, int max)
{
    const int mode = (int)p[kMode];
    const int shift = 12 * (int)p[kOctave];
    const double beatSamples = 60.0 / bpm * sr;
    int n = 0;
    auto note = [&](double atBeat, double lenBeats, int key) {
        if (n + 2 > max)
            return;
        const int midi = key - shift < 0 ? 0 : (key - shift > 127 ? 127 : key - shift);
        out[n++] = { (uint32_t)std::lround(atBeat * beatSamples), midi, 110, true };
        out[n++] = { (uint32_t)std::lround((atBeat + lenBeats) * beatSamples), midi, 110, false };
    };
    if (!phrase) { // ein Ton bzw. Akkord (Chord-Hold wirkt) auf der gedachten Grundtaste
        switch (mode) {
        case kModeSaw: note(0, 2.0, 77); break;      // F5, 2 Schläge gehalten: Release und Hall sichtbar
        case kModeStab: note(0, 0.25, 77); break;
        case kModePluck: note(0, 0.25, 77); break;
        case kModeWhistle: note(0, 0.5, 99); break;  // D#7
        default: break;
        }
        return n;
    }
    switch (mode) { // kurze Phrase in F-Moll über einen Takt
    case kModeSaw: {
        const int keys[4] = { 77, 75, 73, 72 }; // F5 Es5 Des5 C5
        for (int b = 0; b < 4; ++b)
            note(b, 0.9, keys[b]);
        break;
    }
    case kModeStab:
        for (int b = 0; b < 4; ++b)
            note(b + 0.5, 0.25, 77); // Stabs auf dem Offbeat
        break;
    case kModePluck: {
        const int keys[8] = { 84, 80, 77, 80, 84, 80, 79, 77 }; // C6 As5 F5 As5 C6 As5 G5 F5
        for (int k = 0; k < 8; ++k)
            note(0.5 * k, 0.25, keys[k]);
        break;
    }
    case kModeWhistle:
        note(0, 0.25, 99), note(0.75, 0.25, 99), note(1.5, 0.25, 99), note(3.5, 0.4, 99);
        break;
    default:
        break;
    }
    return n;
}

void renderPreview(const float* params, bool phrase, double bpm, int sr, std::vector<float>& l, std::vector<float>& r)
{
    std::unique_ptr<Engine> engine(new Engine()); // groß (Hall/Delay-Puffer): nicht auf den Stack
    engine->setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        if (!(kParams[i].flags & (kFlagHidden | kFlagOutput)))
            engine->setParam(i, params[i]);
    NoteEvent ev[64];
    const int n = previewEvents(params, phrase, bpm, sr, ev, 64);
    const uint32_t total = (uint32_t)std::lround(kPreviewBeats * 60.0 / bpm * sr);
    l.assign(total, 0.0f);
    r.assign(total, 0.0f);
    renderEvents(*engine, std::vector<NoteEvent>(ev, ev + n), bpm, sr, l.data(), r.data(), total);
}

} // namespace offbeat
