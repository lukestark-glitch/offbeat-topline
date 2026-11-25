// topline-render: rendert Noten mit dem DSP-Kern von Offbeat Topline in eine WAV-Datei und misst sie.
//
//   topline-render --preset 0 --note F5 --length 800 --check
//   topline-render --mode whistle --note D#7 --set whistle_trill=2
//   topline-render --json sound.json --note F5     (JSON wie aus „Kopieren“ in der Oberfläche)
//   topline-render --check-all                      (Abnahme aller Presets, wie make check)
//   topline-render --list
//
// --note ist die Taste wie in der LMMS-Piano-Roll: der Renderer schickt sie wie LMMS 12 Halbtöne tiefer.
#include "Engine.hpp"
#include "Measure.hpp"
#include "ParamText.hpp"

#include <cfloat>
#include <string>
#include <strings.h>
#include <vector>

using namespace offbeat;
using namespace measure;

namespace {

int findPreset(const char* s)
{
    for (int i = 0; i < kPresetCount; ++i)
        if (std::strcmp(kPresets[i].name, s) == 0)
            return i;
    char* end;
    const long i = std::strtol(s, &end, 10);
    return *end == 0 && i >= 0 && i < kPresetCount ? (int)i : -1;
}

int findParam(const std::string& symbol)
{
    for (int i = 0; i < kParamCount; ++i)
        if (symbol == kParams[i].symbol)
            return i;
    return -1;
}

struct Render {
    std::vector<float> l, r;
};

// Noten (Note, Start ms) mit Gate `lengthMs` rendern; Transport läuft ab Schlag 0.
Render render(const float* values, const std::vector<std::pair<int, float>>& notes, int velocity, float bpm,
              float lengthMs, float tailMs, int sr)
{
    Engine engine;
    engine.setSampleRate((float)sr);
    for (int i = 0; i < kParamCount; ++i)
        engine.setParam(i, values[i]);
    struct Event { uint32_t frame; int note; bool on; };
    std::vector<Event> events;
    float endMs = 0.0f;
    for (const auto& nt : notes) {
        events.push_back({ (uint32_t)(nt.second * 0.001f * sr), nt.first, true });
        events.push_back({ (uint32_t)((nt.second + lengthMs) * 0.001f * sr), nt.first, false });
        endMs = std::fmax(endMs, nt.second + lengthMs);
    }
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.frame < b.frame; });
    const uint32_t total = (uint32_t)((endMs + tailMs) * 0.001f * sr);
    Render out { std::vector<float>(total), std::vector<float>(total) };
    size_t next = 0;
    for (uint32_t pos = 0; pos < total;) {
        engine.setTransport(true, (double)pos / sr * bpm / 60.0, bpm);
        while (next < events.size() && events[next].frame <= pos) {
            const Event& e = events[next++];
            // Wie LMMS: die Taste der Piano-Roll kommt 12 Halbtöne tiefer an (Octave +1 gleicht das aus)
            const int midi = e.note - 12 < 0 ? 0 : e.note - 12;
            if (e.on)
                engine.noteOn(midi, velocity);
            else
                engine.noteOff(midi);
        }
        uint32_t n = total - pos < 256 ? total - pos : 256;
        if (next < events.size() && events[next].frame < pos + n)
            n = events[next].frame - pos;
        engine.process(&out.l[pos], &out.r[pos], n);
        pos += n;
    }
    return out;
}

std::vector<float> monoOf(const Render& x)
{
    std::vector<float> m(x.l.size());
    for (size_t i = 0; i < m.size(); ++i)
        m[i] = 0.5f * (x.l[i] + x.r[i]);
    return m;
}

float peakOf(const std::vector<float>& x)
{
    float p = 0.0f;
    for (float v : x)
        p = std::fmax(p, std::fabs(v));
    return p;
}

// Mittlere Tonhöhe in einem Fenster aus den Aufwärts-Nulldurchgängen.
float avgPitch(const std::vector<float>& x, int sr, float fromMs, float toMs)
{
    const size_t a = (size_t)(fromMs * 0.001f * sr), b = std::min(x.size(), (size_t)(toMs * 0.001f * sr));
    double first = -1, last = -1;
    int count = 0;
    for (size_t i = a + 1; i < b; ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f) {
            const double t = (double)(i - 1) + x[i - 1] / (double)(x[i - 1] - x[i]);
            if (first < 0)
                first = t;
            last = t;
            ++count;
        }
    return count > 1 ? (float)((count - 1) * sr / (last - first)) : 0.0f;
}

// Energie zwischen lo und hi (Hz) in dB.
float bandDb(const std::vector<float>& x, int sr, float lo, float hi)
{
    const size_t len = std::min<size_t>(x.size(), 1 << 17);
    size_t n = 1024;
    while (n < len)
        n <<= 1;
    std::vector<std::complex<double>> a(n);
    for (size_t i = 0; i < len; ++i)
        a[i] = x[i] * (0.5 - 0.5 * std::cos(2.0 * M_PI * (double)i / (double)len));
    fft(a);
    double e = 0.0;
    for (size_t k = 1; k < n / 2; ++k) {
        const double f = (double)k * sr / n;
        if (f >= lo && f < hi)
            e += std::norm(a[k]);
    }
    return (float)(10.0 * std::log10(e + 1e-30));
}

float semitones(float hz, float refHz) { return 12.0f * std::log2(hz / refHz); }

struct Job {
    const float* values;
    std::vector<std::pair<int, float>> notes;
    int velocity;
    float bpm, lengthMs, tailMs;
    int sr;
    const char* title;
    const char* out;
    bool check, quiet;
};

// Rendert, misst, prüft. Rückgabe: Anzahl Fehler.
int run(const Job& job)
{
    const float* values = job.values;
    const int sr = job.sr, mode = (int)values[kMode], note = job.notes[0].first;
    const Render wet = render(values, job.notes, job.velocity, job.bpm, job.lengthMs, job.tailMs, sr);
    const std::vector<float> mono = monoOf(wet);
    const size_t total = mono.size();

    // Hüllkurven-Prüfungen ohne Effekte: Hall und Delay verlängern das Ausklingen absichtlich.
    float dryValues[kParamCount];
    std::memcpy(dryValues, values, sizeof(dryValues));
    dryValues[kChorus] = dryValues[kDelayMix] = dryValues[kReverbMix] = 0.0f;
    const std::vector<float> dry = monoOf(render(dryValues, job.notes, job.velocity, job.bpm, job.lengthMs, job.tailMs, sr));
    const float dryPeak = peakOf(dry);

    float peak = 0.0f, sumSq = 0.0f;
    int denormals = 0, over = 0, nonFinite = 0;
    size_t lastSound = 0;
    for (size_t i = 0; i < total; ++i) {
        for (float s : { wet.l[i], wet.r[i] }) {
            const float a = std::fabs(s);
            nonFinite += std::isfinite(s) ? 0 : 1;
            denormals += (a > 0.0f && a < FLT_MIN) ? 1 : 0;
            over += a > 1.0f ? 1 : 0;
        }
        peak = std::fmax(peak, std::fabs(mono[i]));
        sumSq += mono[i] * mono[i];
        if (std::fabs(mono[i]) > 1e-5f)
            lastSound = i;
    }
    const float soundMs = (float)lastSound * 1000.0f / sr;
    const float dryLen40 = lengthTo(dry, sr, dryPeak * 0.01f), dryLen30 = lengthTo(dry, sr, dryPeak * 0.0316f);
    const Spectrum spec = spectrum(mono, sr, 0, lastSound + 1);
    const float corr = correlation(wet.l, wet.r, 0.0f, sr), corrLow = correlation(wet.l, wet.r, 150.0f, sr);
    const float startDb = toDb(peakIn(mono, sr, 0, 0.25f)) - toDb(peak);
    const float endDb = toDb(peakIn(mono, sr, soundMs - 1.0f, soundMs + 1.0f)) - toDb(peak);
    const bool endsInRender = lastSound + (size_t)(0.002f * sr) < total;

    std::printf("== %s: Mode %s, Note %d (%.1f Hz), Gate %.0f ms, %g BPM\n", job.title, kModeNames[mode], note,
                midiToHz((float)note - 12.0f + 12.0f * values[kOctave]), job.lengthMs, job.bpm);
    if (!job.quiet) {
        std::printf("Spitze           %7.2f dBFS   RMS %7.2f dBFS\n", toDb(peak), toDb(std::sqrt(sumSq / total)));
        std::printf("Klingt bis       %7.1f ms; ohne Effekte bis −30 dB %.1f ms, bis −40 dB %.1f ms\n", soundMs, dryLen30, dryLen40);
        std::printf("Oktavbänder rel. 1 kHz:");
        for (int k = 0; k < 9; ++k)
            std::printf(" %g:%+.0f", kBandHz[k], spec.bands[k] - spec.bands[4]);
        std::printf("\nSchwerpunkt      %7.0f Hz   Anteil > 10 kHz %.1f dB\n", spec.centroid, spec.above10k);
        std::printf("Korrelation L/R  %7.2f gesamt, %s\n", corr,
                    corrLow > 1.5f ? "unter 150 Hz kaum Energie" : (std::to_string(corrLow).substr(0, 5) + " unter 150 Hz").c_str());
        std::printf("Anfang 0–0,25 ms %7.1f dB; Ende %.1f dB%s; Über 0 dBFS %d, Denormals %d, NaN/Inf %d\n", startDb, endDb,
                    endsInRender ? "" : " (klingt beim Render-Ende noch)", over, denormals, nonFinite);
    }
    if (job.out) {
        if (!writeWav(job.out, wet.l, wet.r, sr))
            std::fprintf(stderr, "Konnte %s nicht schreiben\n", job.out);
        else
            std::printf("WAV: %s\n", job.out);
    }
    if (!job.check)
        return 0;

    int failed = 0;
    auto expect = [&](const std::string& what, bool ok) {
        std::printf("  [%s] %s\n", ok ? "OK " : "FEHLER", what.c_str());
        failed += ok ? 0 : 1;
    };
    char buf[160];
    const float playedHz = midiToHz((float)note - 12.0f + 12.0f * values[kOctave]); // LMMS-Versatz, siehe render()
    if (mode == kModeSaw) {
        const float releaseMs = dryLen40 - job.lengthMs;
        std::snprintf(buf, sizeof(buf), "Breite: Korrelation 0,3–0,7 (gemessen %.2f)", corr);
        expect(buf, corr >= 0.3f && corr <= 0.7f);
        std::snprintf(buf, sizeof(buf), "Release ohne Hall 500–900 ms bis −40 dB (gemessen %.0f ms)", releaseMs);
        expect(buf, releaseMs >= 500 && releaseMs <= 900);
        std::snprintf(buf, sizeof(buf), "Schwerpunkt 1–6 kHz (gemessen %.0f Hz)", spec.centroid);
        expect(buf, spec.centroid >= 1000 && spec.centroid <= 6000);
    } else if (mode == kModeStab) {
        std::snprintf(buf, sizeof(buf), "Länge ohne Hall bis −30 dB 70–200 ms (gemessen %.0f ms)", dryLen30);
        expect(buf, dryLen30 >= 70 && dryLen30 <= 200);
        std::snprintf(buf, sizeof(buf), "Breite: Korrelation 0,5–0,75 (gemessen %.2f)", corr);
        expect(buf, corr >= 0.5f && corr <= 0.75f);
        float a[kParamCount], b[kParamCount];
        std::memcpy(a, dryValues, sizeof(a));
        std::memcpy(b, dryValues, sizeof(b));
        a[kStabShine] = 0.0f;
        b[kStabShine] = 12.0f;
        const float lo = std::fmin(4000.0f, values[kStabShineHz] / 1.5f), hi = std::fmax(8000.0f, values[kStabShineHz] * 1.33f);
        const float gain = bandDb(monoOf(render(b, job.notes, job.velocity, job.bpm, job.lengthMs, job.tailMs, sr)), sr, lo, hi)
                         - bandDb(monoOf(render(a, job.notes, job.velocity, job.bpm, job.lengthMs, job.tailMs, sr)), sr, lo, hi);
        std::snprintf(buf, sizeof(buf), "Glanz: Shine 12 dB hebt 4–8 kHz um ≥ 6 dB (gemessen %+.1f dB)", gain);
        expect(buf, gain >= 6.0f);
    } else if (mode == kModePluck) {
        std::snprintf(buf, sizeof(buf), "Länge ohne Hall bis −30 dB 60–130 ms (gemessen %.0f ms)", dryLen30);
        expect(buf, dryLen30 >= 60 && dryLen30 <= 130);
        std::snprintf(buf, sizeof(buf), "Fast mono: Korrelation ≥ 0,85 (gemessen %.2f)", corr);
        expect(buf, corr >= 0.85f);
        std::snprintf(buf, sizeof(buf), "Mitten: Schwerpunkt 0,5–2,5 kHz (gemessen %.0f Hz)", spec.centroid);
        expect(buf, spec.centroid >= 500 && spec.centroid <= 2500);
    } else if (mode == kModeWhistle) {
        const float trill = values[kWhistleTrill];
        const float early = semitones(avgPitch(dry, sr, 1.0f, 5.0f), playedHz);
        const float settled = semitones(avgPitch(dry, sr, trill > 0 ? 25.0f : 50.0f, trill > 0 ? 40.0f : 70.0f), playedHz);
        std::snprintf(buf, sizeof(buf), "Tonhöhe sitzt: %s ±25 Cent (gemessen %+.0f Cent)",
                      trill > 0 ? "25–40 ms" : "50–70 ms", settled * 100.0f);
        expect(buf, std::fabs(settled) <= 0.25f);
        std::snprintf(buf, sizeof(buf), "Einsatz von unten: 1–5 ms 0,3–1,2 Halbtöne tiefer (gemessen %+.2f)", early);
        expect(buf, early <= -0.3f && early >= -1.2f);
        if (trill > 0) {
            const float up = semitones(avgPitch(dry, sr, 50.0f, 75.0f), playedHz);
            std::snprintf(buf, sizeof(buf), "Triller: 50–75 ms um %.1f Halbtöne höher ±0,3 (gemessen %+.2f)", trill, up);
            expect(buf, std::fabs(up - trill) <= 0.3f);
        }
        std::snprintf(buf, sizeof(buf), "Fast reiner Sinus: Schwerpunkt ≤ 1,6 × Grundton (%.0f ≤ %.0f Hz)", spec.centroid, 1.6f * playedHz);
        expect(buf, spec.centroid <= 1.6f * playedHz);
        if (values[kWhistleLength] <= 250.0f) {
            std::snprintf(buf, sizeof(buf), "Stoß ohne Hall 100–250 ms bis −40 dB (gemessen %.0f ms)", dryLen40);
            expect(buf, dryLen40 >= 100 && dryLen40 <= 250);
        }
    }
    if (corrLow <= 1.5f)
        expect("Mono unter 150 Hz (Korrelation ≥ 0,98)", corrLow >= 0.98f);
    expect("Kein Sprung am Anfang (erstes Sample < 0,001)", std::fabs(mono[0]) < 1e-3f);
    if (endsInRender)
        expect("Weiches Ende (letzte ms ≤ −30 dB)", endDb <= -30.0f);
    expect("Kein Clipping über 0 dBFS", over == 0 && peak <= 1.0f);
    expect("Keine Denormals, kein NaN/Inf", denormals == 0 && nonFinite == 0);
    std::printf("%s\n", failed ? "NICHT BESTANDEN" : "BESTANDEN");
    return failed;
}

void usage()
{
    std::puts("topline-render [--preset NAME|NR] [--mode supersaw|stab|pluck|whistle] [--note F5|77[@ms]]... [--velocity 0-127]\n"
              "               [--bpm 160] [--length MS] [--tail MS] [--sr 48000] [--json datei] [--set symbol=wert]...\n"
              "               [--out datei.wav] [--check] [--check-all] [--list]");
}

} // namespace

int main(int argc, char** argv)
{
    int preset = -1, mode = -1, velocity = 110, sr = 48000;
    std::vector<std::pair<int, float>> notes;
    float bpm = 160.0f, lengthMs = 400.0f, tailMs = 1500.0f;
    const char* out = nullptr;
    const char* jsonFile = nullptr;
    bool check = false;
    std::vector<std::pair<int, float>> overrides;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        if (a == "--list") {
            std::puts("Presets:");
            for (int p = 0; p < kPresetCount; ++p)
                std::printf("  %d  %s\n", p, kPresets[p].name);
            std::puts("Parameter (symbol: min..max, default):");
            for (const ParamInfo& p : kParams)
                std::printf("  %-18s %g..%g %s, %g\n", p.symbol, p.min, p.max, p.unit, p.def);
            return 0;
        }
        if (a == "--check-all") {
            // Abnahme: jedes Preset auf seiner Taste, mit Gate und Ausklang passend zum Modus.
            struct Case { int preset; const char* note; float length, tail; };
            const Case cases[] = { { 0, "F5", 800, 2500 }, { 1, "E5", 800, 2500 }, { 2, "F5", 800, 2500 },
                                   { 3, "F5", 300, 1500 }, { 4, "A4", 300, 1500 }, { 5, "F5", 300, 1500 },
                                   { 6, "F5", 200, 1500 }, { 7, "E5", 200, 1500 }, { 8, "D#7", 400, 1500 },
                                   { 9, "D#7", 800, 1500 } };
            int failedJobs = 0;
            for (const Case& c : cases) {
                float values[kParamCount];
                applyPreset(c.preset, values);
                const Job job { values, { { parseNote(c.note), 0.0f } }, 110, 160.0f, c.length, c.tail, 48000,
                                kPresets[c.preset].name, nullptr, true, true };
                failedJobs += run(job) ? 1 : 0;
            }
            std::printf("\n%d von %d Presets bestanden\n", (int)(sizeof(cases) / sizeof(cases[0])) - failedJobs,
                        (int)(sizeof(cases) / sizeof(cases[0])));
            return failedJobs ? 1 : 0;
        }
        if (a == "--check") {
            check = true;
            continue;
        }
        if (!v) {
            usage();
            return 2;
        }
        ++i;
        if (a == "--preset" && (preset = findPreset(v)) < 0) {
            std::fprintf(stderr, "Unbekanntes Preset: %s\n", v);
            return 2;
        } else if (a == "--mode") {
            for (int m = 0; m < kModeCount; ++m)
                if (strcasecmp(v, kModeNames[m]) == 0)
                    mode = m;
        } else if (a == "--note") {
            const char* at = std::strchr(v, '@');
            notes.emplace_back(parseNote(v), at ? (float)std::atof(at + 1) : 0.0f);
        } else if (a == "--velocity")
            velocity = std::atoi(v);
        else if (a == "--bpm")
            bpm = (float)std::atof(v);
        else if (a == "--length")
            lengthMs = (float)std::atof(v);
        else if (a == "--tail")
            tailMs = (float)std::atof(v);
        else if (a == "--sr")
            sr = std::atoi(v);
        else if (a == "--out")
            out = v;
        else if (a == "--json")
            jsonFile = v;
        else if (a == "--set") {
            const std::string kv = v;
            const size_t eq = kv.find('=');
            const int id = eq == std::string::npos ? -1 : findParam(kv.substr(0, eq));
            if (id < 0) {
                std::fprintf(stderr, "Unbekannter Parameter: %s\n", v);
                return 2;
            }
            overrides.emplace_back(id, (float)std::atof(kv.c_str() + eq + 1));
        } else if (a != "--preset") {
            usage();
            return 2;
        }
    }
    if (notes.empty())
        notes.emplace_back(77, 0.0f); // F5
    for (const auto& nt : notes)
        if (nt.first < 0 || nt.first > 127) {
            std::fputs("Ungültige Note\n", stderr);
            return 2;
        }

    float values[kParamCount];
    applyPreset(preset, values);
    if (mode >= 0)
        values[kMode] = (float)mode;
    if (jsonFile) {
        std::string json, errors;
        if (FILE* f = std::fopen(jsonFile, "rb")) {
            char buf[4096];
            for (size_t r; (r = std::fread(buf, 1, sizeof(buf), f)) > 0;)
                json.append(buf, r);
            std::fclose(f);
        }
        bool changed[kParamCount] = {};
        const int n = applyJson(json.c_str(), values, changed, errors);
        std::printf("JSON: %d Werte gesetzt%s%s\n", n, errors.empty() ? "" : "; ", errors.c_str());
        if (n == 0)
            return 2;
    }
    for (const auto& o : overrides)
        values[o.first] = o.second;

    const Job job { values, notes, velocity, bpm, lengthMs, tailMs, sr,
                    preset >= 0 ? kPresets[preset].name : "(Defaults)", out, check, false };
    return run(job) ? 1 : 0;
}
