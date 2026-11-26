// Oberfläche von Offbeat Topline: Modus-Reiter, GLOBAL (Spielweise, Vibrato, Motorrad), Regler des
// aktiven Modus und FX. Look und Bedienung: OffbeatKit.hpp (aus offbeat-engine).
#include "OffbeatKit.hpp"
#include "Preview.hpp"
#include "BikeImage.h" // aus assets/bike.png erzeugt (xxd -i), siehe plugin/Makefile

START_NAMESPACE_DISTRHO

using namespace offbeat;
using kit::kCellH;
using kit::kCellW;
using kit::kLabelH;

namespace {
constexpr float kW = 860, kH = 800, kMargin = 20, kGridX = 31, kKeysH = 96; // kKeysH: Tastatur der App
constexpr float kGlobalY = 104, kModeY = 252, kFxY = 510, kPreviewY = 656;

const int kGlobals[5] = { kVolume, kGlide, kVibDepth, kVibRate, kVibDelay };
const int kFx[6] = { kChorus, kDelayMix, kDelayTime, kDelayFeedback, kReverbMix, kReverbSize };

// Farbe pro Modus: Supersaw Magenta, Stab Rot, Pluck Orange, Whistle Rosa.
const Color kModeColor[kModeCount] = { Color(255, 40, 180), Color(255, 60, 70), Color(255, 150, 50), Color(255, 170, 230) };

kit::Theme toplineTheme()
{
    kit::Theme t;
    t.title = "OFFBEAT";
    t.subtitle = "TOPLINE";
    t.brand = Color(230, 25, 50);
    t.brandInk = Color(130, 0, 25);
    t.night = Color(6, 3, 6);
    t.night2 = Color(52, 6, 22);
    t.light = Color(255, 90, 140);
    t.plate = Color(22, 6, 12);
    t.track = Color(75, 22, 38);
    t.text = Color(255, 236, 240);
    t.dim = Color(230, 160, 178);
    return t;
}
} // namespace

class ToplineUI : public OffbeatKitUI {
public:
    ToplineUI()
        : OffbeatKitUI(kW, kit::isStandaloneApp() ? kH + kKeysH : kH, toplineTheme())
    {
        if (kit::isStandaloneApp() && isUsingNativeAudio() && supportsMIDI() && !isMIDIEnabled())
            requestMIDI(); // MIDI-Keyboards in der App einschalten
        bike = createImageFromMemory(bike_png, bike_png_len, IMAGE_GENERATE_MIPMAPS);
    }

protected:
    void drawContent() override
    {
        const int mode = currentMode();
        const Color accent = kModeColor[mode];

        drawLogo(145, 50);
        for (int m = 0; m < kModeCount; ++m) {
            const float w = 112, x = kW - kMargin - (kModeCount - m) * (w + 6) + 6;
            pill(100 + m, x, 33, w, 34, kModeNames[m], kModeColor[m], m == mode);
        }

        drawPanel(kMargin, kGlobalY, kW - 2 * kMargin, kLabelH + kCellH, "GLOBAL", th.brand);
        // Motorrad rechts; Flammen und Lichtstreifen laufen unter dem letzten Regler aus.
        const float bikeW = 300, bikeH = bikeW * 384.0f / 1024.0f;
        drawImage(bike, kW - bikeW + 2, kGlobalY + (kLabelH + kCellH) / 2 - bikeH / 2 + 6, bikeW, bikeH);
        for (int i = 0; i < 5; ++i)
            knob(kGlobals[i], kGridX + i * kCellW, kGlobalY + kLabelH, th.brand);
        compact(kOctave, 110, kGlobalY + 3, 100, th.brand);
        compact(kVoiceMode, 218, kGlobalY + 3, 120, th.brand);
        compact(kChord, 346, kGlobalY + 3, 116, th.brand);
        copyPaste(kW - kMargin - 12, kGlobalY + 3);

        char label[32];
        kit::upper(kModeNames[mode], label, sizeof(label));
        drawPanel(kMargin, kModeY, kW - 2 * kMargin, kLabelH + 2 * kCellH, label, accent);
        char tag[48];
        std::snprintf(tag, sizeof(tag), "MODE %d/%d", mode + 1, kModeCount);
        drawTag(kW - kMargin - 14, kModeY + 13, tag);
        int list[16];
        const int count = modeParams(mode, list);
        for (int n = 0; n < count; ++n)
            knob(list[n], kGridX + (n % 7) * kCellW, kModeY + kLabelH + (n / 7) * kCellH, accent);

        drawPanel(kMargin, kFxY, kW - 2 * kMargin, kLabelH + kCellH, "FX", th.brand);
        for (int i = 0; i < 6; ++i)
            knob(kFx[i], kGridX + i * kCellW, kFxY + kLabelH, th.brand);
        compact(kTempoHost, 70, kFxY + 3, 128, th.brand);
        compact(kTempoBpm, 206, kFxY + 3, 116, th.brand);

        // Vorschau: ein Ton bzw. Akkord des Modus oder eine kurze Phrase, mit dem echten DSP gerendert
        drawPanel(kMargin, kPreviewY, kW - 2 * kMargin, kH - kPreviewY - 12, "VORSCHAU", accent);
        drawPreview(kMargin + 14, kPreviewY + kLabelH + 2, kW - 2 * kMargin - 28, kH - kPreviewY - kLabelH - 26, accent);

        if (kit::isStandaloneApp()) { // App: Bildschirm-Tastatur, auch mit der Computer-Tastatur spielbar
            drawPanel(kMargin, kH - 4, kW - 2 * kMargin, kKeysH - 8, "TASTATUR  \xc2\xb7  A W S E D F T G Z H U J K   \xc2\xb7  - / +  OKTAVE", th.brand);
            drawKeyboard(kMargin + 14, kH - 4 + kLabelH, kW - 2 * kMargin - 28, kKeysH - kLabelH - 16, th.brand);
        }
    }

    int octaveParam() const override { return kOctave; }

    PreviewParams previewParams() const override
    {
        PreviewParams pp;
        pp.play = kPreview, pp.loop = kPreviewLoop, pp.bar = kPreviewPhrase, pp.active = kPreviewActive, pp.bpm = kCurrentBpm;
        pp.barLabels[0] = "Ton", pp.barLabels[1] = "Phrase";
        pp.beats[0] = pp.beats[1] = 4; // beides über einen Takt (Release und Hall passen hinein)
        pp.allButton = false;
        return pp;
    }

    void renderPreview() override
    {
        const double bpm = values[kCurrentBpm] > 20.0f ? values[kCurrentBpm] : 160.0;
        std::vector<float> l, r;
        previewLayers.resize(1);
        previewLayers[0].name = kModeNames[currentMode()];
        offbeat::renderPreview(values, values[kPreviewPhrase] >= 0.5f, bpm, 48000, l, r);
        previewSum.resize(l.size());
        for (size_t i = 0; i < l.size(); ++i)
            previewSum[i] = 0.5f * (l[i] + r[i]);
        previewLayers[0].mono = previewSum;
    }

    void onButton(int id) override
    {
        if (id >= 100 && id < 100 + kModeCount)
            setValue(kMode, (float)(id - 100), true);
    }


    // Name ohne Modus-Vorsilbe ("Saw Detune" → "Detune"), kurze Namen für Kopfzeilen.
    const char* controlLabel(int p) const override
    {
        if (p == kTempoHost)
            return "Host-Tempo";
        if (p == kTempoBpm)
            return "BPM";
        if (p == kVoiceMode)
            return "Stimmen";
        const char* name = kParams[p].name;
        if (p >= kSawVoices)
            if (const char* sp = std::strchr(name, ' '))
                return sp + 1;
        return name;
    }

private:
    NanoImage bike;

    int currentMode() const
    {
        const int m = (int)std::lround(values[kMode]);
        return m < 0 ? 0 : (m >= kModeCount ? kModeCount - 1 : m);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToplineUI)
};

UI* createUI()
{
    return new ToplineUI();
}

END_NAMESPACE_DISTRHO
