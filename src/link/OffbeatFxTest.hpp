// Abnahme der Beat FX (OffbeatFxDsp.hpp): von den Prüfwerkzeugen der Synths mit --fx-check aufgerufen.
#pragma once

#include "OffbeatFxDsp.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace offbeat::fx {

inline int fxCheck()
{
    int failed = 0;
    auto expect = [&](const std::string& what, bool ok) {
        std::printf("  [%s] %s\n", ok ? "OK " : "FEHLER", what.c_str());
        failed += ok ? 0 : 1;
    };
    const double sr = 48000.0, bpm = 160.0;
    auto run = [&](BeatFx& f, std::vector<float>& l, std::vector<float>& r, const State& st, bool targeted, size_t from, size_t to) {
        for (size_t p = from; p < to; p += 256) {
            const uint32_t n = (uint32_t)std::min<size_t>(256, to - p);
            f.process(l.data() + p, r.data() + p, n, st, targeted, bpm, -1.0);
        }
    };
    auto argmax = [](const std::vector<float>& v, size_t a, size_t b) {
        size_t m = a;
        for (size_t i = a; i < b && i < v.size(); ++i)
            if (std::fabs(v[i]) > std::fabs(v[m]))
                m = i;
        return m;
    };
    std::puts("== Beat FX");

    { // Echo 1/2 Schlag bei 160 BPM = 0,1875 s = 9000 Samples
        BeatFx f;
        f.prepare(sr);
        State st;
        st.on = 1, st.type = kEcho, st.beat = 2, st.level = 1.0f;
        const size_t N = 48000, at = 4800;
        std::vector<float> l(N, 0.0f), r(N, 0.0f);
        l[at] = r[at] = 1.0f;
        run(f, l, r, st, true, 0, N);
        const size_t e1 = argmax(l, at + 4500, at + 13500), e2 = argmax(l, at + 13500, at + 22500);
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Echo 1/2 bei 160 BPM: Wiederholungen nach %zu und %zu Samples (erwartet 9000, 18000 ±1)", e1 - at, e2 - at);
        expect(buf, std::labs((long)(e1 - at) - 9000) <= 1 && std::labs((long)(e2 - at) - 18000) <= 1 && std::fabs(l[e2]) < std::fabs(l[e1]));
    }
    { // Ping Pong: erste Wiederholung links, zweite rechts
        BeatFx f;
        f.prepare(sr);
        State st;
        st.on = 1, st.type = kPingPong, st.beat = 2, st.level = 1.0f;
        const size_t N = 48000, at = 4800;
        std::vector<float> l(N, 0.0f), r(N, 0.0f);
        l[at] = r[at] = 1.0f;
        run(f, l, r, st, true, 0, N);
        const float l1 = std::fabs(l[at + 9000]), r1 = std::fabs(r[at + 9000]), l2 = std::fabs(l[at + 18000]), r2 = std::fabs(r[at + 18000]);
        expect("Ping Pong: 1. Wiederholung links, 2. rechts", l1 > 0.3f && r1 < 1e-6f && r2 > 0.1f && l2 < 1e-6f);
    }
    { // Reverb: Nachhall nach dem Ausschalten, klingt ab, active() bis zum Ende
        BeatFx f;
        f.prepare(sr);
        State st;
        st.on = 1, st.type = kReverb, st.beat = 4, st.level = 1.0f;
        const size_t N = (size_t)(sr * 8);
        std::vector<float> l(N, 0.0f), r(N, 0.0f);
        uint32_t seed = 1;
        for (size_t i = 0; i < (size_t)(sr * 0.5); ++i) {
            seed = seed * 1664525u + 1013904223u;
            l[i] = r[i] = ((seed >> 8) / 16777216.0f - 0.5f) * 0.5f;
        }
        const size_t off = (size_t)(sr * 0.5);
        run(f, l, r, st, true, 0, off);
        st.on = 0;
        run(f, l, r, st, true, off, N);
        auto rms = [&](size_t a, size_t b) {
            double s = 0;
            for (size_t i = a; i < b; ++i)
                s += (double)l[i] * l[i];
            return std::sqrt(s / (double)(b - a));
        };
        const double t1 = rms(off + 4800, off + 14400), t2 = rms(off + 48000, off + 57600);
        expect("Reverb: Nachhall läuft nach dem Ausschalten weiter und klingt ab", t1 > 1e-3 && t2 < t1 * 0.5);
        expect("Reverb: nach 7,5 s Ruhe nicht mehr aktiv (Stille darf übersprungen werden)", !f.active());
    }
    { // Klickfrei: an/aus, Typwechsel und BEAT-Wechsel während ein Sinus läuft
        BeatFx f;
        f.prepare(sr);
        State st;
        st.level = 1.0f;
        const size_t N = (size_t)(sr * 6);
        std::vector<float> l(N), r(N);
        for (size_t i = 0; i < N; ++i)
            l[i] = r[i] = 0.4f * (float)std::sin(2 * M_PI * 220.0 * (double)i / sr);
        float worst = 0.0f;
        for (size_t p = 0, k = 0; p < N; p += 256, ++k) {
            st.on = (k / 40) % 2 == 0;        // alle ~0,2 s an/aus
            st.type = (int)((k / 100) % kTypeCount); // alle ~0,5 s anderer Effekt
            st.beat = (int)((k / 70) % kBeatCount);
            f.process(l.data() + p, r.data() + p, 256, st, true, bpm, -1.0);
        }
        for (size_t i = 1; i < N; ++i)
            worst = std::max(worst, std::fabs(l[i] - l[i - 1]));
        char buf[96];
        std::snprintf(buf, sizeof(buf), "keine Klicks bei An/Aus, Typ- und BEAT-Wechsel (größter Sprung %.3f, Grenze 0,12)", worst);
        expect(buf, worst < 0.12f);
    }
    { // Nicht ausgewählt: trocken bitgenau
        BeatFx f;
        f.prepare(sr);
        State st;
        st.on = 1, st.type = kReverb, st.level = 1.0f;
        std::vector<float> l(48000), r(48000);
        for (size_t i = 0; i < l.size(); ++i)
            l[i] = (float)std::sin(i * 0.01), r[i] = (float)std::cos(i * 0.013);
        const std::vector<float> l0 = l, r0 = r;
        run(f, l, r, st, false, 0, l.size());
        expect("Spur nicht ausgewählt: Signal bitgenau unverändert", l == l0 && r == r0);
    }
    { // Rechenzeit je Effekt: 10 s Stereo
        double worst = 1e9;
        std::string line;
        for (int t = 0; t < kTypeCount; ++t) {
            BeatFx f;
            f.prepare(sr);
            State st;
            st.on = 1, st.type = t, st.beat = 4, st.level = 0.8f;
            const size_t N = (size_t)(sr * 10);
            std::vector<float> l(N), r(N);
            for (size_t i = 0; i < N; ++i)
                l[i] = r[i] = 0.3f * (float)std::sin(i * 0.05);
            const auto t0 = std::chrono::steady_clock::now();
            run(f, l, r, st, true, 0, N);
            const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            const double rt = 10.0 / std::max(secs, 1e-9);
            worst = std::min(worst, rt);
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%s%s %.0f×", line.empty() ? "" : ", ", typeName(t), rt);
            line += buf;
        }
        expect("Echtzeit-Faktor je Effekt (10 s Stereo): " + line, worst > 20.0);
    }
    std::puts(failed ? "NICHT BESTANDEN" : "BESTANDEN");
    return failed ? 1 : 0;
}

} // namespace offbeat::fx
