// Offbeat Beat FX: Echo, Delay, Ping Pong, Reverb, Flanger, Phaser – taktsynchron, im Synth jeder ausgewählten Spur.
//
// Gemeinsamer Code der Familie (Quelle: offbeat-engine, in andere Repos kopiert). Nur Header.
// prepare() legt alle Puffer an (nicht im Audio-Thread); process() rechnet im Audio-Thread ohne Speicheranforderung.
// Eingang des Effekts = an && Spur ausgewählt (10 ms geglättet); Echo/Hall klingen nach dem Ausschalten aus,
// das trockene Signal läuft immer unverändert durch. Typwechsel: Effektanteil 20 ms aus, Puffer leeren, 20 ms ein.
#pragma once

#include "OffbeatFx.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace offbeat::fx {

class BeatFx {
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        const size_t maxDelay = (size_t)std::ceil(sr * 6.1) + 4; // 4 Schläge bei 40 BPM
        for (auto& d : line)
            d.assign(maxDelay, 0.0f);
        const double scale = sr / 44100.0;
        static const int kComb[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int kAllpass[4] = { 556, 441, 341, 225 };
        for (int c = 0; c < 2; ++c) {
            for (int i = 0; i < 8; ++i)
                comb[c][i].init((size_t)((kComb[i] + c * 23) * scale));
            for (int i = 0; i < 4; ++i)
                allpass[c][i].init((size_t)((kAllpass[i] + c * 23) * scale));
        }
        modLine[0].assign((size_t)(sr * 0.02) + 4, 0.0f);
        modLine[1].assign((size_t)(sr * 0.02) + 4, 0.0f);
        gateCoef = (float)(1.0 - std::exp(-1.0 / (0.010 * sr)));
        levelCoef = (float)(1.0 - std::exp(-1.0 / (0.020 * sr)));
        fadeStep = (float)(1.0 / (0.020 * sr));
        xfadeLen = std::max<uint32_t>(64, (uint32_t)(0.030 * sr));
        reset();
    }

    // Wirkt (offener Eingang oder hörbarer Nachklang über −90 dB)? Für das Überspringen der Stille.
    bool active() const { return gate > 1e-6f || gateWant || tailPeak > 3.2e-5f; }

    void process(float* l, float* r, uint32_t n, const State& st, bool targeted, double bpm, double beat)
    {
        if (line[0].empty())
            return;
        gateWant = st.on && targeted;
        if (!active() && !switching) { // nichts zu tun: trocken bleibt bitgenau unverändert
            curType = st.type, fresh = true;
            return;
        }
        if (bpm < 40.0 || bpm > 300.0)
            bpm = 160.0;
        if (st.type != curType && !switching)
            switching = true; // erst ausblenden, dann umschalten
        const double beatLen = beatValue(st.beat);
        const double beatSec = 60.0 / bpm;
        // Verzögerung (Echo/Delay/Ping Pong): ganzzahlig, bei Änderung 30 ms Überblendung zwischen alter und neuer Stelle
        const uint32_t dWant = (uint32_t)std::clamp<long>(std::lround(beatLen * beatSec * sr), 1, (long)line[0].size() - 2);
        if (fresh) // aus der Ruhe: sofort die richtige Zeit, keine Überblendung von einer alten Stelle
            dOld = dCur = dWant, xfade = xfadeLen, fresh = false;
        else if (dWant != dCur && xfade >= xfadeLen)
            dOld = dCur, dCur = dWant, xfade = 0;
        // LFO (Flanger/Phaser): Periode = Länge × 4 Schläge; mit Host-Takt phasenstarr, sonst frei laufend
        const double periodBeats = beatLen * 4.0;
        if (beat >= 0.0)
            lfoPhase = std::fmod(beat / periodBeats, 1.0);
        const double lfoInc = 1.0 / (periodBeats * beatSec * sr);
        // Reverb: BEAT ◀ ▶ = Raumgröße / Nachhall
        const float room = 0.70f + 0.04f * (float)std::clamp(st.beat, 0, kBeatCount - 1), damp = 0.25f;
        const float levelWant = std::clamp(st.level, 0.0f, 1.0f);
        int type = curType;
        float peak = 0.0f;
        const size_t len = line[0].size();

        for (uint32_t i = 0; i < n; ++i) {
            gate += gateCoef * ((gateWant ? 1.0f : 0.0f) - gate);
            level += levelCoef * (levelWant - level);
            if (switching) {
                fade -= fadeStep;
                if (fade <= 0.0f) {
                    clear();
                    curType = st.type, switching = false, fade = 0.0f, inRamp = 0.0f;
                }
            } else if (fade < 1.0f)
                fade = std::min(1.0f, fade + fadeStep);
            if (inRamp < 1.0f) // nach dem Leeren auch den Eingang einblenden (sonst setzt das Echo hart ein)
                inRamp = std::min(1.0f, inRamp + fadeStep);
            const float inL = l[i] * gate * inRamp, inR = r[i] * gate * inRamp;
            type = curType; // kann mitten im Block wechseln (nach dem Ausblenden)
            float wl = 0.0f, wr = 0.0f;
            switch (type) {
            case kEcho:
            case kDelay: {
                const float fb = type == kEcho ? 0.55f : 0.25f;
                wl = tap(0), wr = tap(1);
                lp[0] += 0.45f * (wl - lp[0]), lp[1] += 0.45f * (wr - lp[1]); // Dämpfung nur in der Rückkopplung
                line[0][pos] = flush(inL + fb * (type == kEcho ? lp[0] : wl));
                line[1][pos] = flush(inR + fb * (type == kEcho ? lp[1] : wr));
                break;
            }
            case kPingPong: { // Eingang (Mitte) nur links; links speist rechts, rechts speist links
                wl = tap(0), wr = tap(1);
                line[0][pos] = flush(0.5f * (inL + inR) + 0.6f * wr);
                line[1][pos] = flush(0.6f * wl);
                break;
            }
            case kReverb: {
                const float in = (inL + inR) * 0.015f;
                for (int c = 0; c < 2; ++c) {
                    float s = 0.0f;
                    for (auto& cb : comb[c])
                        s += cb.process(in, room, damp);
                    for (auto& ap : allpass[c])
                        s = ap.process(s);
                    (c ? wr : wl) = s * 3.0f;
                }
                break;
            }
            case kFlanger: {
                const double ph = lfoPhase;
                for (int c = 0; c < 2; ++c) {
                    const double p = std::fmod(ph + c * 0.25, 1.0);
                    const double lfo = 0.5 - 0.5 * std::cos(2.0 * M_PI * p);
                    const double d = (0.0008 + 0.0050 * lfo) * sr;
                    const float y = modTap(c, d);
                    modLine[c][modPos] = flush((c ? inR : inL) + 0.55f * y);
                    (c ? wr : wl) = y;
                }
                break;
            }
            case kPhaser: {
                for (int c = 0; c < 2; ++c) {
                    const double p = std::fmod(lfoPhase + c * 0.25, 1.0);
                    const double lfo = 0.5 - 0.5 * std::cos(2.0 * M_PI * p);
                    const double f = 200.0 * std::pow(20.0, lfo); // 200 Hz – 4 kHz
                    const float t = (float)std::tan(M_PI * std::min(f, 0.45 * sr) / sr);
                    const float a = (t - 1.0f) / (t + 1.0f);
                    float x = (c ? inR : inL) + 0.5f * phFb[c];
                    for (float& z : phZ[c]) { // 6 Allpässe 1. Ordnung
                        const float y = a * x + z;
                        z = flush(x - a * y);
                        x = y;
                    }
                    phFb[c] = x;
                    (c ? wr : wl) = x;
                }
                break;
            }
            default:
                break;
            }
            pos = pos + 1 < len ? pos + 1 : 0;
            modPos = modPos + 1 < modLine[0].size() ? modPos + 1 : 0;
            if (xfade < xfadeLen)
                ++xfade;
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0)
                lfoPhase -= 1.0;
            const float g = level * fade;
            wl *= g, wr *= g;
            peak = std::max(peak, std::max(std::fabs(wl), std::fabs(wr)));
            if (type == kFlanger || type == kPhaser) { // Modulation: Mischung trocken/nass (Kammfilter-Klang), Pegel bleibt gleich
                const float k = 0.5f * g * gate;
                l[i] = l[i] * (1.0f - k) + wl * 0.5f * gate, r[i] = r[i] * (1.0f - k) + wr * 0.5f * gate;
            } else
                l[i] += wl, r[i] += wr;
        }
        tailPeak = peak;
    }

    void reset()
    {
        clear();
        gate = 0.0f, level = 0.0f, fade = 1.0f, switching = false, gateWant = false, tailPeak = 0.0f;
        dCur = dOld = 1, xfade = xfadeLen, fresh = true;
    }

private:
    struct Comb {
        std::vector<float> buf;
        size_t i = 0;
        float store = 0.0f;
        void init(size_t n) { buf.assign(std::max<size_t>(n, 1), 0.0f), i = 0, store = 0.0f; }
        float process(float in, float feedback, float damp)
        {
            const float out = buf[i];
            store = flush(out * (1.0f - damp) + store * damp);
            buf[i] = flush(in + store * feedback);
            i = i + 1 < buf.size() ? i + 1 : 0;
            return out;
        }
    };
    struct Allpass {
        std::vector<float> buf;
        size_t i = 0;
        void init(size_t n) { buf.assign(std::max<size_t>(n, 1), 0.0f), i = 0; }
        float process(float in)
        {
            const float b = buf[i], out = b - in;
            buf[i] = flush(in + b * 0.5f);
            i = i + 1 < buf.size() ? i + 1 : 0;
            return out;
        }
    };

    static float flush(float x) { return std::fabs(x) < 1e-20f ? 0.0f : x; }

    float read(int c, uint32_t d) const
    {
        const size_t len = line[c].size();
        return line[c][(pos + len - d) % len];
    }
    float tap(int c) const
    {
        if (xfade >= xfadeLen)
            return read(c, dCur);
        const float x = (float)xfade / (float)xfadeLen;
        return read(c, dOld) * (1.0f - x) + read(c, dCur) * x;
    }
    float modTap(int c, double d) const
    {
        const size_t len = modLine[c].size();
        const double rp = (double)modPos + len - d;
        const size_t i0 = (size_t)rp % len, i1 = (i0 + 1) % len;
        const float f = (float)(rp - std::floor(rp));
        return modLine[c][i0] * (1.0f - f) + modLine[c][i1] * f;
    }
    void clear()
    {
        for (auto& d : line)
            std::fill(d.begin(), d.end(), 0.0f);
        for (auto& m : modLine)
            std::fill(m.begin(), m.end(), 0.0f);
        for (int c = 0; c < 2; ++c) {
            for (auto& cb : comb[c])
                std::fill(cb.buf.begin(), cb.buf.end(), 0.0f), cb.store = 0.0f;
            for (auto& ap : allpass[c])
                std::fill(ap.buf.begin(), ap.buf.end(), 0.0f);
            for (float& z : phZ[c])
                z = 0.0f;
            phFb[c] = 0.0f, lp[c] = 0.0f;
        }
    }

    double sr = 48000.0;
    std::vector<float> line[2], modLine[2];
    size_t pos = 0, modPos = 0;
    Comb comb[2][8];
    Allpass allpass[2][4];
    float phZ[2][6] {}, phFb[2] {}, lp[2] {};
    uint32_t dCur = 1, dOld = 1, xfade = 0, xfadeLen = 1440;
    double lfoPhase = 0.0;
    float gate = 0.0f, level = 0.0f, fade = 1.0f, inRamp = 1.0f, tailPeak = 0.0f;
    float gateCoef = 0.002f, levelCoef = 0.001f, fadeStep = 0.001f;
    bool gateWant = false, switching = false, fresh = true;
    int curType = kEcho;
};

} // namespace offbeat::fx
