#include "Engine.hpp"

#if defined(__SSE__) || defined(__x86_64__)
#include <xmmintrin.h>
#define OFFBEAT_HAVE_SSE 1
#endif

namespace offbeat {

namespace {
// Chord-Hold: Aus, Moll, Dur, Moll7, Dur7 (Septime = kleine Septime).
const int kChordSize[5] = { 1, 3, 3, 4, 4 };
const int kChordNotes[5][4] = { { 0 }, { 0, 3, 7 }, { 0, 4, 7 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 } };
const int kWidthParam[kModeCount] = { kSawWidth, kStabWidth, kPluckWidth, kWhistleWidth };
} // namespace

Engine::Engine()
{
    applyDefaults(params);
    setSampleRate(48000.0f);
}

void Engine::setSampleRate(float newSr)
{
    sr = newSr;
    delay.setSampleRate(sr);
    reverb.setSampleRate(sr);
    volume.setTime(10.0f, sr);
    resetVoices();
}

void Engine::setParam(int id, float value)
{
    if (id < 0 || id >= kParamCount)
        return;
    const ParamInfo& info = kParams[id];
    value = clampf(value, info.min, info.max);
    if ((id == kMode || id == kOctave || id == kVoiceMode) && (int)value != (int)params[id])
        resetVoices(); // gehaltene Noten passen sonst nicht mehr zu ihrem Note-Off
    params[id] = value;
}

void Engine::resetVoices()
{
    for (ToplineVoice& v : voices)
        v = ToplineVoice();
    heldCount = 0;
    lastNote = -1;
    eq.reset();
    chorus.reset();
    delay.reset();
    reverb.reset();
    for (Svf& f : widthHp)
        f.reset();
}

// Host-Tempo nur, wenn "Tempo from Host" an ist und der Host eines liefert; sonst "Tempo BPM".
void Engine::setTransport(bool, double, double hostBpm)
{
    bpm = (params[kTempoHost] >= 0.5f && hostBpm > 0.0) ? hostBpm : params[kTempoBpm];
}

void Engine::noteOn(int note, int velocity)
{
    note = transpose(note);
    const float vel = clampf((float)velocity / 127.0f, 0.0f, 1.0f);
    const bool legato = params[kVoiceMode] >= 0.5f && heldCount > 0;
    if (heldCount < 16) {
        heldNotes[heldCount] = note;
        heldVelocity[heldCount++] = velocity;
    }
    if (params[kVoiceMode] >= 0.5f && !legato)
        for (ToplineVoice& v : voices)
            v.release(); // Mono: nur ein Akkord klingt
    playChord(note, vel, legato);
}

void Engine::playChord(int key, float velocity, bool legato)
{
    const int chord = (int)params[kChord];
    const VoiceSettings s = settings();
    int slid = 0;
    for (int i = 0; i < kChordSize[chord]; ++i) {
        const float note = (float)(key + kChordNotes[chord][i]);
        if (legato) { // Mono-Legato: gehaltene Stimmen gleiten zum neuen Akkord, ohne Neuanschlag
            int n = 0;
            ToplineVoice* found = nullptr;
            for (ToplineVoice& v : voices)
                if (v.active() && v.held && n++ == slid) {
                    found = &v;
                    break;
                }
            if (found) {
                found->slideTo(key, note);
                ++slid;
                continue;
            }
        }
        // Freie Stimme, sonst die älteste losgelassene, sonst die älteste überhaupt.
        ToplineVoice* best = nullptr;
        for (ToplineVoice& v : voices) {
            auto rank = [](const ToplineVoice& x) { return !x.active() ? 0 : (!x.held ? 1 : 2); };
            if (!best || rank(v) < rank(*best) || (rank(v) == rank(*best) && v.age > best->age))
                best = &v;
        }
        const float from = lastNote >= 0 ? note + (float)(lastNote - key) : note;
        best->start(key, note, from, 0.3f * velocity, s, rnd, sr);
    }
    lastNote = key;
}

void Engine::noteOff(int note)
{
    note = transpose(note);
    int i = 0;
    while (i < heldCount && heldNotes[i] != note)
        ++i;
    if (i == heldCount)
        return;
    for (; i < heldCount - 1; ++i) {
        heldNotes[i] = heldNotes[i + 1];
        heldVelocity[i] = heldVelocity[i + 1];
    }
    --heldCount;

    if (params[kVoiceMode] >= 0.5f) {
        if (heldCount == 0) {
            for (ToplineVoice& v : voices)
                v.release();
        } else if (note == lastNote) { // zurück zur zuletzt gedrückten, noch gehaltenen Taste
            playChord(heldNotes[heldCount - 1], clampf((float)heldVelocity[heldCount - 1] / 127.0f, 0.0f, 1.0f), true);
        }
        return;
    }
    for (ToplineVoice& v : voices)
        if (v.key == note && v.held)
            v.release();
}

void Engine::allNotesOff()
{
    heldCount = 0;
    for (ToplineVoice& v : voices)
        v.release();
}

VoiceSettings Engine::settings() const
{
    const float* p = params;
    VoiceSettings s;
    s.glide = p[kGlide];
    s.vibDepth = p[kVibDepth];
    s.vibDelay = p[kVibDelay];
    switch ((int)p[kMode]) {
    case kModeSaw:
        s.voices = (int)p[kSawVoices];
        s.detuneCents = 60.0f * p[kSawDetune];
        s.sideMix = 0.75f;
        s.spread = 0.95f;
        s.cutoff = p[kSawCutoff];
        s.reso = p[kSawReso];
        s.envAmt = p[kSawEnvAmt];
        s.filterDecay = p[kSawFiltDecay];
        s.attack = p[kSawAttack];
        s.decay = p[kSawDecay];
        s.sustain = p[kSawSustain];
        s.release = p[kSawRelease];
        break;
    case kModeStab:
        s.voices = (int)p[kStabVoices];
        s.detuneCents = 50.0f * p[kStabDetune];
        s.sideMix = 0.7f;
        s.spread = 0.9f;
        s.cutoff = p[kStabCutoff];
        s.reso = 0.15f;
        s.envAmt = p[kStabEnvAmt];
        s.filterDecay = p[kStabFiltDecay];
        s.attack = 1.0f;
        s.decay = p[kStabDecay];
        s.sustain = 0.0f;
        s.release = p[kStabRelease];
        break;
    case kModePluck:
        s.voices = p[kPluckDetune] > 0.01f ? 3 : 1;
        s.detuneCents = 20.0f * p[kPluckDetune];
        s.sideMix = 0.5f;
        s.spread = 0.5f;
        s.cutoff = p[kPluckTone];
        s.reso = 0.2f;
        s.envAmt = p[kPluckEnvAmt];
        s.filterDecay = 0.6f * p[kPluckDecay];
        s.attack = 0.5f;
        s.decay = p[kPluckDecay];
        s.sustain = 0.0f;
        s.release = 40.0f;
        break;
    case kModeWhistle:
        s.whistle = true;
        s.attack = 6.0f;
        s.decay = 1000.0f;
        s.sustain = 1.0f;
        s.release = 30.0f;
        s.breath = p[kWhistleBreath];
        s.scoop = p[kWhistleScoop];
        s.scoopMs = p[kWhistleScoopTime];
        s.trill = p[kWhistleTrill];
        s.trillHz = p[kWhistleTrillRate];
        s.lengthMs = p[kWhistleLength];
        break;
    default:
        break;
    }
    return s;
}

void Engine::process(float* outL, float* outR, uint32_t frames)
{
#ifdef OFFBEAT_HAVE_SSE
    const unsigned int oldCsr = _mm_getcsr();
    _mm_setcsr(oldCsr | 0x8040); // Flush-to-Zero + Denormals-are-Zero
#endif
    const int mode = (int)params[kMode];
    const VoiceSettings s = settings();
    const float widthGain = 2.0f * params[kWidthParam[mode]];
    const float volTarget = std::pow(10.0f, params[kVolume] / 20.0f);
    if (mode == kModeStab)
        eq.set(params[kStabShineHz], params[kStabShine], sr);
    else if (mode == kModePluck)
        eq.set(1000.0f, params[kPluckBody], sr);
    else
        eq.gain = 0.0f;
    for (Svf& f : widthHp)
        f.set(150.0f, sr, 0.29f);
    const float delaySamples = kDelayBeats[(int)params[kDelayTime]] * 60.0f / (float)bpm * sr;
    const float chorusMix = params[kChorus], delayMix = params[kDelayMix], delayFb = params[kDelayFeedback];
    const float reverbMix = params[kReverbMix], reverbSize = params[kReverbSize];
    const float vibStep = params[kVibRate] / sr;
    if (!started) {
        volume.value = volTarget; // kein Einblenden beim allerersten Block
        started = true;
    }

    for (uint32_t i = 0; i < frames; ++i) {
        vibPhase += vibStep;
        if (vibPhase >= 1.0f)
            vibPhase -= 1.0f;
        const float vib = std::sin(2.0f * kPi * vibPhase);

        float l = 0.0f, r = 0.0f;
        for (ToplineVoice& v : voices) {
            float vl, vr;
            v.process(s, vib, vl, vr);
            l += vl, r += vr;
        }
        if (eq.gain != 0.0f)
            eq.process(l, r);
        if (chorusMix > 0.0f)
            chorus.process(l, r, chorusMix, sr);

        float dl, dr, rl, rr;
        delay.process(l, r, delaySamples, delayFb, dl, dr);
        l += delayMix * dl, r += delayMix * dr;
        reverb.process(l, r, reverbSize, rl, rr);
        l += reverbMix * rl, r += reverbMix * rr;

        // Width pro Modus nur auf dem Seitensignal oberhalb 150 Hz: der Bass bleibt mono.
        const float mid = 0.5f * (l + r);
        float side = 0.5f * (l - r);
        for (Svf& f : widthHp) {
            f.process(side);
            side = f.hp;
        }
        side *= widthGain;

        const float vol = volume.process(volTarget);
        outL[i] = softLimit(vol * (mid + side));
        outR[i] = softLimit(vol * (mid - side));
    }
#ifdef OFFBEAT_HAVE_SSE
    _mm_setcsr(oldCsr);
#endif
}

} // namespace offbeat
