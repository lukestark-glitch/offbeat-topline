// DPF-Hülle von Offbeat Topline: reicht Parameter, Programme, MIDI und Host-Tempo an die Engine weiter.
#include "DistrhoPlugin.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <bitset>
#include "../src/link/OffbeatFxDsp.hpp"
#include "../src/link/OffbeatLink.hpp"
#include "../src/link/PluginMeta.hpp"
#include "Engine.hpp"
#include "OffbeatParam.hpp"

START_NAMESPACE_DISTRHO

using namespace offbeat;

class ToplinePlugin : public Plugin {
public:
    ToplinePlugin()
        : Plugin(kParamCount, kPresetCount, 0)
    {
        beatFx.prepare(getSampleRate());
        engine.setSampleRate((float)getSampleRate());
        writePluginMeta(); // Regler-Beschreibung für Offbeat Performance
    }

protected:
    const char* getLabel() const override { return "OffbeatTopline"; }
    const char* getDescription() const override { return "Melodie-Synth: Supersaw, Stab, Pluck, Whistle"; }
    const char* getMaker() const override { return "Offbeat"; }
    const char* getHomePage() const override { return DISTRHO_PLUGIN_URI; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    void initParameter(uint32_t index, Parameter& parameter) override { fillParameter(kParams[index], parameter); }

    void initProgramName(uint32_t index, String& programName) override { programName = kPresets[index].name; }

    void loadProgram(uint32_t index) override
    {
        float values[kParamCount];
        applyPreset((int)index, values);
        for (int i = 0; i < kParamCount; ++i)
            if (i != kMidiChannel) // Preset/Program Change ändert nie den Kanal
                engine.setParam(i, values[i]);
    }

    float getParameterValue(uint32_t index) const override { return engine.getParam((int)index); }
    void setParameterValue(uint32_t index, float value) override { engine.setParam((int)index, value); }

    void sampleRateChanged(double newSampleRate) override
    {
        engine.setSampleRate((float)newSampleRate);
        beatFx.prepare(newSampleRate);
    }

    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* events, uint32_t eventCount) override
    {
        const TimePosition& tp = getTimePosition();
        if (tp.bbt.valid) {
            const double beat = (double)(tp.bbt.bar - 1) * tp.bbt.beatsPerBar + (tp.bbt.beat - 1)
                              + tp.bbt.tick / tp.bbt.ticksPerBeat;
            engine.setTransport(tp.playing, beat, tp.bbt.beatsPerMinute);
        } else {
            engine.setTransport(false, 0.0, 0.0);
        }

        // Offbeat Link: Sound für Gearbox-Songs spiegeln, geladenen Sound übernehmen
        int linkCount = 0;
        if (link.update((double)frames / getSampleRate(), kParamCount, [&](int i) { return engine.getParam(i); }, linkValues, linkCount))
            for (int i = 0; i < linkCount && i < kParamCount; ++i)
                if (!(kParams[i].flags & (kFlagOutput | kFlagHidden)) && i != kMidiChannel) // Song lädt Sound, nicht Kanal
                    engine.setParam(i, linkValues[i]);

        // Beat FX (Offbeat Clutch): gilt der Effekt für diese Spur?
        fx::read(fxShared, fxState); // bei laufendem Schreiben bleibt der letzte Zustand
        const bool fxTargeted = fx::targets(fxState, link.linkId(), link.trackIndex());

        // Stille überspringen: keine eigenen MIDI-Ereignisse, keine gehaltene Note, keine Vorschau und der Ausgang ist
        // seit 0,5 s still (Hall/Delay ausgeklungen) → nicht rechnen. Spart bei vielen Synths in Carla die meiste Last.
        bool mine = false;
        for (uint32_t e = 0; e < eventCount && !mine; ++e)
            mine = events[e].size <= 3 && events[e].data[0] < 0xf0 && forMe(events[e].data[0]);
        if (!mine && held.none() && !beatFx.active() && !(fxState.on && fxTargeted) && quietFrames >= getSampleRate() * 0.5 && engine.getParam(kPreview) < 0.5f
            && engine.getParam(kPreviewActive) < 0.5f) {
            std::memset(outputs[0], 0, sizeof(float) * frames);
            std::memset(outputs[1], 0, sizeof(float) * frames);
            return;
        }

        uint32_t pos = 0;
        for (uint32_t e = 0; e < eventCount; ++e) {
            const MidiEvent& ev = events[e];
            if (ev.size > 3 || ev.frame >= frames)
                continue;
            if (!forMe(ev.data[0]))
                continue;
            if (ev.frame > pos) {
                engine.process(outputs[0] + pos, outputs[1] + pos, ev.frame - pos);
                pos = ev.frame;
            }
            const uint8_t status = ev.data[0] & 0xF0;
            if (status == 0x90 && ev.data[2] > 0)
                engine.noteOn(ev.data[1], ev.data[2]), held.set(ev.data[1] & 127);
            else if (status == 0x80 || status == 0x90)
                engine.noteOff(ev.data[1]), held.reset(ev.data[1] & 127);
            else if (status == 0xB0 && (ev.data[1] == 120 || ev.data[1] == 123))
                engine.allNotesOff(), held.reset();
            else if (status == 0xC0 && ev.data[1] < kPresetCount) // Program Change, z. B. von Offbeat Gearbox
                loadProgram(ev.data[1]);
            if (status == 0xB0)
                link.cc(ev.data[0] & 15, ev.data[1], ev.data[2]);
        }
        if (pos < frames)
            engine.process(outputs[0] + pos, outputs[1] + pos, frames - pos);
        {
            const bool bbt = tp.bbt.valid && tp.bbt.beatsPerMinute > 0.0;
            const double fxBeat = bbt && tp.playing ? (double)(tp.bbt.bar - 1) * tp.bbt.beatsPerBar + (tp.bbt.beat - 1)
                                                          + tp.bbt.tick / tp.bbt.ticksPerBeat
                                                    : -1.0;
            beatFx.process(outputs[0], outputs[1], frames, fxState, fxTargeted, bbt ? tp.bbt.beatsPerMinute : 160.0, fxBeat);
        }
        float peak = 0.0f;
        for (uint32_t i = 0; i < frames; ++i)
            peak = std::max(peak, std::max(std::fabs(outputs[0][i]), std::fabs(outputs[1][i])));
        quietFrames = peak < 1e-5f ? quietFrames + frames : 0;
    }

private:
    // Kanalnachricht für diesen Synth? (MIDI-Kanal-Parameter; 0 = Omni). Systemnachrichten immer.
    bool forMe(uint8_t status) const
    {
        const int ch = (int)engine.getParam(kMidiChannel);
        return ch <= 0 || status >= 0xf0 || (status & 15) == ch - 1;
    }

    Engine engine;
    std::bitset<128> held; // gehaltene Noten (für das Überspringen der Stille)
    double quietFrames = 0;
    fx::Shared* fxShared = fx::open(); // Beat FX von Offbeat Clutch
    fx::State fxState;
    fx::BeatFx beatFx;
    link::Client link { link::kTopline };
    float linkValues[link::kMaxValues];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToplinePlugin)
};

Plugin* createPlugin() { return new ToplinePlugin(); }

END_NAMESPACE_DISTRHO
