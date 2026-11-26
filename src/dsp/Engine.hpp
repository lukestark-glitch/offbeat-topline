// DSP-Kern von Offbeat Topline: Noten → Stimmen (Chord-Hold, Poly/Mono-Legato, Glide)
// → Glanz-/Mitten-EQ → Chorus → Delay → Hall → Width pro Modus → Volume → Limiter.
// Ohne DPF, damit topline-render denselben Code nutzt.
#pragma once

#include "Effects.hpp"
#include "Render.hpp"
#include "Params.hpp"
#include "Voice.hpp"

namespace offbeat {

class Engine {
public:
    Engine();

    void setSampleRate(float sr);
    void setParam(int id, float value);
    float getParam(int id) const { return params[id]; }

    // Noten vom Host; sie haben Vorrang und beenden ein laufendes Vorhören.
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void allNotesOff();

    // Einmal pro Block: Host-Tempo für das Delay.
    void setTransport(bool playing, double beatPosition, double bpm);

    void process(float* outL, float* outR, uint32_t frames);

private:
    void playNoteOn(int note, int velocity);
    void playNoteOff(int note);
    void updatePreview(); // einmal pro Block: Vorhör-Wunsch aus der Oberfläche übernehmen
    static constexpr int kVoices = 10;

    void resetVoices();
    void playChord(int key, float velocity, bool legato);
    VoiceSettings settings() const;
    int transpose(int note) const
    {
        const int n = note + 12 * (int)params[kOctave];
        return n < 0 ? 0 : (n > 127 ? 127 : n);
    }

    float params[kParamCount];
    float sr = 48000.0f;
    double bpm = 160.0;
    bool started = false;

    ToplineVoice voices[kVoices];
    int lastNote = -1;
    int heldNotes[16] = {}, heldVelocity[16] = {};
    int heldCount = 0;
    Noise rnd;
    float vibPhase = 0.0f;

    // Vorhören (aus der Oberfläche über den Parameter preview): eigenes kleines Noten-Raster
    bool previewSeen = false;
    int lastPreview = 0, previewActive = 0, previewCount = 0, previewNext = 0;
    uint32_t previewPos = 0, previewLen = 0;
    NoteEvent previewEv[64];

    PeakEq eq;
    Chorus chorus;
    PingPongDelay delay;
    Reverb reverb;
    Svf widthHp[4];
    Smoother volume;
};

} // namespace offbeat
