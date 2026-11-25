// Effekte von Offbeat Topline: Peak-EQ, Ping-Pong-Delay im Takt, Hall (Freeverb-Prinzip).
// Puffer werden in setSampleRate() angelegt (nicht im Audio-Thread).
#pragma once

#include "Blocks.hpp"

#include <algorithm>
#include <vector>

namespace offbeat {

// Glockenfilter aus dem SVF: x + (Verstärkung − 1) · Bandpass. Q ≈ 1.
struct PeakEq {
    Svf f[2];
    void set(float hz, float db, float sr)
    {
        gain = std::pow(10.0f, db / 20.0f) - 1.0f;
        f[0].set(hz, sr, 0.5f);
        f[1].a1 = f[0].a1, f[1].a2 = f[0].a2, f[1].a3 = f[0].a3, f[1].k = f[0].k;
    }
    void process(float& l, float& r)
    {
        f[0].process(l);
        f[1].process(r);
        // Bandpass des SVF hat bei der Mittenfrequenz die Verstärkung 1/k
        l += gain * f[0].k * f[0].bp;
        r += gain * f[1].k * f[1].bp;
    }
    void reset() { f[0].reset(), f[1].reset(); }
    float gain = 0.0f;
};

// Ping-Pong-Delay: links ↔ rechts, Feedback mit Low-Pass (dunkler werdende Wiederholungen).
struct PingPongDelay {
    std::vector<float> buf[2];
    int write = 0, size = 1;
    Smoother time;
    Svf damp[2];

    void setSampleRate(float sr)
    {
        size = (int)(2.5f * sr) + 4; // 1/4 punktiert bei 60 BPM = 1,5 s
        buf[0].assign(size, 0.0f);
        buf[1].assign(size, 0.0f);
        time.setTime(80.0f, sr);
        for (Svf& d : damp)
            d.set(5000.0f, sr, 0.0f);
    }
    void reset()
    {
        for (auto& b : buf)
            std::fill(b.begin(), b.end(), 0.0f);
        damp[0].reset(), damp[1].reset();
    }
    float read(const std::vector<float>& b, float delay) const
    {
        float pos = (float)write - delay;
        while (pos < 0.0f)
            pos += (float)size;
        const int i = (int)pos;
        const float frac = pos - (float)i;
        return b[i % size] + frac * (b[(i + 1) % size] - b[i % size]);
    }
    // Gibt das Echo zurück (ohne trockenes Signal).
    void process(float inL, float inR, float delaySamples, float feedback, float& outL, float& outR)
    {
        if (time.value <= 0.0f)
            time.value = delaySamples;
        const float d = clampf(time.process(delaySamples), 1.0f, (float)size - 2.0f);
        outL = read(buf[0], d);
        outR = read(buf[1], d);
        damp[0].process(outR);
        damp[1].process(outL);
        // Mono-Eingang geht nach links, das Echo springt dann hin und her
        buf[0][write] = 0.5f * (inL + inR) + feedback * damp[0].lp;
        buf[1][write] = feedback * damp[1].lp;
        write = (write + 1) % size;
    }
};

// Hall nach dem Freeverb-Prinzip (8 Kammfilter + 4 Allpässe pro Seite), Hochpass am Eingang.
struct Reverb {
    struct Comb {
        std::vector<float> buf;
        int pos = 0;
        float store = 0.0f;
        float process(float x, float feedback, float damp)
        {
            const float y = buf[pos];
            store = y + damp * (store - y);
            buf[pos] = x + feedback * store;
            if (++pos >= (int)buf.size())
                pos = 0;
            return y;
        }
    };
    struct Allpass {
        std::vector<float> buf;
        int pos = 0;
        float process(float x)
        {
            const float b = buf[pos];
            buf[pos] = x + 0.5f * b;
            if (++pos >= (int)buf.size())
                pos = 0;
            return b - x;
        }
    };

    Comb comb[2][8];
    Allpass allpass[2][4];
    Svf inHp;

    void setSampleRate(float sr)
    {
        static const int kComb[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int kAllpass[4] = { 556, 441, 341, 225 };
        const float scale = sr / 44100.0f;
        for (int c = 0; c < 2; ++c) {
            const int spread = c ? 23 : 0;
            for (int i = 0; i < 8; ++i)
                comb[c][i].buf.assign((size_t)((kComb[i] + spread) * scale), 0.0f);
            for (int i = 0; i < 4; ++i)
                allpass[c][i].buf.assign((size_t)((kAllpass[i] + spread) * scale), 0.0f);
        }
        inHp.set(200.0f, sr, 0.0f);
    }
    void reset()
    {
        for (auto& side : comb)
            for (Comb& c : side)
                std::fill(c.buf.begin(), c.buf.end(), 0.0f), c.store = 0.0f;
        for (auto& side : allpass)
            for (Allpass& a : side)
                std::fill(a.buf.begin(), a.buf.end(), 0.0f);
        inHp.reset();
    }
    // Gibt den Hall zurück (ohne trockenes Signal). size 0..1.
    void process(float inL, float inR, float size, float& outL, float& outR)
    {
        inHp.process(0.5f * (inL + inR));
        const float x = 0.03f * inHp.hp;
        const float feedback = 0.7f + 0.28f * size, damp = 0.35f;
        float out[2] = { 0.0f, 0.0f };
        for (int c = 0; c < 2; ++c) {
            for (Comb& cb : comb[c])
                out[c] += cb.process(x, feedback, damp);
            for (Allpass& ap : allpass[c])
                out[c] = ap.process(out[c]);
        }
        outL = out[0];
        outR = out[1];
    }
};

} // namespace offbeat
