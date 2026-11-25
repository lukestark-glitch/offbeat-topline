// Stimme von Offbeat Topline (abgeleitet von der Poly-Stimme aus Offbeat Engine).
// Supersaw/Stab/Pluck: 1–7 verstimmte Sägezähne im Stereo verteilt → Stereo-Low-Pass mit Hüllkurve → ADSR.
// Whistle: Sinus + Atemrauschen (Bandpass um den Grundton), Einsatz-Rutscher und Triller.
// Alle Modi: Glide und Vibrato mit Einsatzverzögerung.
#pragma once

#include "Blocks.hpp"

namespace offbeat {

// Vom Modus abhängige Einstellungen; die Engine füllt sie einmal pro Block.
struct VoiceSettings {
    bool whistle = false;
    int voices = 7;
    float detuneCents = 15, sideMix = 0.7f, spread = 0.9f;
    float cutoff = 6000, reso = 0.1f, envAmt = 0, filterDecay = 300;
    float attack = 5, decay = 500, sustain = 1, release = 300;
    float glide = 0, vibDepth = 0, vibDelay = 250;
    float breath = 0, scoop = 0, scoopMs = 30, trill = 0, trillHz = 12, lengthMs = 0;
};

class ToplineVoice {
public:
    void start(int key, float note, float fromNote, float velocity, const VoiceSettings& s, Noise& rnd, float newSr)
    {
        sr = newSr;
        if (!amp.active()) {
            for (Osc& o : saws)
                o.phase = 0.5f + 0.5f * rnd.next(); // zufällige Phasen gegen Phasing
            sine.phase = 0.0f;
            fl.reset();
            fr.reset();
            breathBp.reset();
            trillSmooth.value = 0.0f;
        }
        this->key = key;
        target = note;
        current = s.glide > 0.0f ? fromNote : note;
        gain = velocity;
        held = true;
        if (!amp.active())
            age = 0, declickLen = (uint32_t)(0.001f * sr) + 1;
        else
            age = declickLen + 1; // Stimme klingt noch: kein neues Einblenden
        count = s.whistle ? 1 : (s.voices < 1 ? 1 : (s.voices > 7 ? 7 : s.voices));
        float norm = 0.0f;
        for (int i = 0; i < count; ++i) {
            pos[i] = count == 1 ? 0.0f : -1.0f + 2.0f * (float)i / (float)(count - 1);
            level[i] = std::fabs(pos[i]) <= 1.0f / (float)(count > 1 ? count - 1 : 1) + 1e-4f ? 1.0f : s.sideMix;
            norm += level[i] * level[i];
        }
        norm = 1.0f / std::sqrt(norm > 0.0f ? norm : 1.0f);
        for (int i = 0; i < count; ++i) {
            panL[i] = level[i] * norm * std::sqrt(0.5f * (1.0f - s.spread * pos[i]));
            panR[i] = level[i] * norm * std::sqrt(0.5f * (1.0f + s.spread * pos[i]));
        }
        filterEnv = 1.0f;
        trillSmooth.setTime(4.0f, sr);
        amp.gateOn();
    }

    // Legato (Mono): neue Tonhöhe ohne Neuanschlag.
    void slideTo(int newKey, float note)
    {
        key = newKey;
        target = note;
        held = true;
    }

    void release()
    {
        held = false;
        amp.gateOff();
    }

    bool active() const { return amp.active(); }

    // vib: Wert des gemeinsamen Vibrato-LFOs (−1..1).
    void process(const VoiceSettings& s, float vib, float& outL, float& outR)
    {
        outL = outR = 0.0f;
        if (!amp.active())
            return;
        const float t = (float)age / sr;
        ++age;
        if (s.lengthMs > 0.0f && held && t * 1000.0f >= s.lengthMs)
            release(); // Pfeifen-Stoß: endet nach "Length", auch wenn die Taste noch gehalten wird

        if (s.glide > 0.0f)
            current = target + tauCoef(s.glide / 3.0f, sr) * (current - target);
        else
            current = target;
        float note = current;
        const float vibRamp = clampf((t * 1000.0f - s.vibDelay) / 200.0f, 0.0f, 1.0f);
        note += s.vibDepth * vibRamp * vib;

        float l = 0.0f, r = 0.0f;
        if (s.whistle) {
            note -= s.scoop * std::exp(-t * 3000.0f / s.scoopMs); // ~95 % oben nach "Scoop Time"
            if (s.trill > 0.0f) {
                trillPhase += s.trillHz / sr;
                if (trillPhase >= 1.0f)
                    trillPhase -= 1.0f;
                note += trillSmooth.process(trillPhase < 0.5f ? 0.0f : s.trill);
            }
            const float hz = midiToHz(note);
            breathBp.set(hz, sr, 0.85f);
            breathBp.process(noise.next());
            const float y = sine.process(hz, sr) + s.breath * 1.5f * breathBp.bp;
            l = r = 0.7f * y;
        } else {
            const float hz = midiToHz(note);
            for (int i = 0; i < count; ++i) {
                const float y = saws[i].process(hz * std::exp2(pos[i] * s.detuneCents / 1200.0f), sr, 0.0f);
                l += panL[i] * y;
                r += panR[i] * y;
            }
            filterEnv *= decayCoef(s.filterDecay, sr);
            fl.set(s.cutoff * std::exp2(6.0f * s.envAmt * filterEnv), sr, s.reso);
            fr.a1 = fl.a1, fr.a2 = fl.a2, fr.a3 = fl.a3, fr.k = fl.k;
            fl.process(l);
            fr.process(r);
            l = fl.lp;
            r = fr.lp;
        }

        // 1 ms Einblendung ab dem ersten Sample (erstes Sample exakt 0, egal wo die Sägezähne starten)
        const float declick = age <= declickLen ? (float)(age - 1) / (float)declickLen : 1.0f;
        const float a = gain * declick * amp.process(s.attack, s.decay, s.sustain, s.release, sr);
        outL = a * l;
        outR = a * r;
    }

    int key = -1;
    bool held = false;
    uint32_t age = 0, declickLen = 48;

private:
    Osc saws[7];
    Sine sine;
    Noise noise { 0x51ed270bu };
    Svf fl, fr, breathBp;
    Smoother trillSmooth;
    Adsr amp;
    int count = 1;
    float pos[7] = {}, level[7] = {}, panL[7] = {}, panR[7] = {};
    float sr = 48000.0f, target = 0, current = 0, gain = 1, filterEnv = 0, trillPhase = 0;
};

} // namespace offbeat
