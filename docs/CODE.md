# Offbeat Topline – Code-Doku

Aufbau des Codes und Anleitungen, um Topline nach deinen Vorstellungen anzupassen.

## Überblick

```
offbeat-topline/
├── src/dsp/
│   ├── Params.hpp/.cpp   alle Parameter (kParams), Presets, modeParams(), jsonParams()
│   ├── Engine.hpp/.cpp   Noten/Akkorde → 10 Stimmen → EQ → Chorus → Delay → Hall → Width → Volume → Limiter
│   ├── Voice.hpp         ToplineVoice: Supersaw/Stab/Pluck (Sägezähne + Filter + ADSR) oder Whistle (Sinus + Atem)
│   ├── Effects.hpp       PeakEq (Glanz/Mitten), PingPongDelay (im Takt), Reverb (Freeverb-Prinzip)
│   └── Blocks.hpp, ParamInfo.hpp, ParamText.*   gemeinsame Bausteine
├── plugin/
│   ├── DistrhoPluginInfo.h  Name, URI, IDs
│   ├── ToplinePlugin.cpp    DPF-Hülle: Parameter, Presets, MIDI, Host-Tempo → Engine
│   ├── ToplineUI.cpp        Oberfläche: Reiter, GLOBAL (Motorrad), Modus-Regler, FX
│   └── OffbeatKit.hpp, OffbeatParam.hpp
├── tools/topline-render.cpp  Offline-Renderer mit --check-all
└── assets/bike.png
```

## Signalfluss

```
MIDI ─► Engine::noteOn(note)  (Octave verschiebt)
          │ Chord-Hold: Taste + Akkord-Intervalle (Aus, Moll, Dur, Moll7, Dur7)
          │ Poly: je Akkordton eine freie Stimme (sonst die älteste losgelassene/älteste)
          │ Mono: gehaltene Stimmen gleiten zum neuen Akkord (slideTo), ohne Neuanschlag
          ▼
  10 × ToplineVoice::process()   Glide, Vibrato (gemeinsamer LFO, Einsatz verzögert)
          Supersaw/Stab/Pluck: n Sägezähne, Stereo-Spreizung → SVF-Low-Pass + Filter-Hüllkurve → ADSR
          Whistle: Sinus + Atem (Bandpass um den Grundton), Scoop von unten, Triller, Stoß-Länge
          ▼
  PeakEq (Stab: Shine bei Shine Freq, Pluck: Body bei 1 kHz)
  ─► Chorus ─► + Delay (Ping-Pong, Zeit in Schlägen × 60/BPM) ─► + Hall
  ─► Width des Modus (Seite ab 150 Hz, 48 dB/Okt.) ─► Volume ─► softLimit
```

Die Einstellungen der Stimmen berechnet `Engine::settings()` einmal pro Block aus den Reglern des aktiven Modus. Dort steht auch, wie ein Regler auf den Klang abgebildet wird, zum Beispiel `detuneCents = 60 · saw_detune`.

## Häufige Anpassungen

### Klang eines Modus verändern
`Engine::settings()` legt fest, wie die Regler eines Modus auf die Stimme wirken: Stimmenzahl, Verstimmung, Hüllkurven, Filter. Für Pluck gilt zum Beispiel `filterDecay = 0.6 · pluck_decay`. Die Klangziele prüft `topline-render --check-all`; nach jeder Änderung `make check`.

### Preset ändern oder hinzufügen
In `Params.cpp` als Liste von Abweichungen vom Standard. Neue Presets kommen ans **Ende** von `kPresets[]`. Damit `make check` sie prüft, trägst du sie in `--check-all` ein (`tools/topline-render.cpp`, Liste `cases`, mit Taste, Gate und Ausklang).

### Neuen Regler
1. `ParamId` und `kParams` **hinten** ergänzen.
2. In `Engine::settings()` bzw. `process()` benutzen.
3. Bei einem Modus-Regler das Ende in `modeParams()` anpassen (die Bereiche gehen dort von `first[mode]` bis `first[mode+1]`). Ein globaler Regler bekommt in `ToplineUI::drawContent()` seinen Platz.
4. `jsonParams()` gibt Modus, GLOBAL, FX (`kMode … kTempoBpm`) und den Modus aus. Neue globale Regler liegen hinter `kTempoBpm` und müssen dort ausdrücklich aufgenommen werden.

### Neuer Akkord
`kChordSize` und `kChordNotes` in `Engine.cpp` erweitern, `kChordNames` in `Params.cpp` ergänzen und `chord` max um 1 erhöhen.

### Effekte
`Effects.hpp` ist bewusst einfach gehalten. Der Hall ist ein klassischer Freeverb mit 8 Kammfiltern und 4 Allpässen pro Seite, davor ein Hochpass bei 200 Hz. Das Delay ist Ping-Pong mit gedämpftem Feedback (Low-Pass 5 kHz). Die Puffer entstehen in `setSampleRate()`, nie im Audio-Thread.

## Offline-Renderer

```sh
bin/topline-render --list
bin/topline-render --preset 0 --note F5 --length 800 --check   # ein Preset prüfen
bin/topline-render --check-all                                 # alle Presets (= make check)
bin/topline-render --mode whistle --note D#7 --set whistle_trill=2 --out pfiff.wav
bin/topline-render --preset-json 3                             # Preset als JSON
```

Gemessen werden:
- Länge mit und ohne Effekte,
- Oktavbänder und Schwerpunkt,
- L/R-Korrelation (Breite),
- bei Whistle die Tonhöhe am Anfang (Scoop), nach dem Einschwingen und im Triller.

Für Hüllkurven-Prüfungen rechnet der Renderer jedes Preset zusätzlich ohne Chorus, Delay und Hall.

## Gemeinsame Bausteine der Offbeat-Familie

Diese Dateien sind in allen vier Repos **gleich**. Sie sind eine bewusste Kopie; die Quelle ist [offbeat-engine](https://github.com/lukestark-glitch/offbeat-engine). Änderst du sie, dann zuerst dort und danach in die anderen Repos kopieren.

| Datei | Inhalt |
|---|---|
| `src/dsp/ParamInfo.hpp` | `ParamInfo`: Name, Kurzname (`symbol`), Einheit, Bereich, Standard, Flags (`kFlagLog`, `kFlagBool`, `kFlagInt`, `kFlagOutput`), Auswahl-Namen (`labels`) |
| `src/dsp/Blocks.hpp` | DSP-Bausteine: `Osc` (PolyBLEP-Saw/Square), `Sine`, `Tri`, `Noise`, `Svf` (TPT-State-Variable-Filter), `Smoother`, `Adsr`, `Chorus`, `DenormalGuard` (Flush-to-Zero auf Intel und ARM), Helfer wie `midiToHz`, `decayCoef`, `softLimit` |
| `src/dsp/ParamText.*` | Text ↔ Wert: `parseParamText` (Zahleneingabe am Regler), `applyJson`/`toJson` (KOPIEREN/EINFÜGEN, `--json`, `--preset-json`) |
| `plugin/OffbeatParam.hpp` | `fillParameter`: macht aus einem `ParamInfo` einen DPF-`Parameter` (Hints, Bereich, Auswahl-Werte) |
| `plugin/OffbeatKit.hpp` | `OffbeatKitUI`: Look und Bedienung aller Oberflächen (siehe unten) |
| `tools/Measure.hpp` | Messwerkzeuge der Renderer: WAV lesen/schreiben, Tonhöhe, Pegel, Spektrum, L/R-Korrelation |

### Ein Parameter, viele Stellen: alles kommt aus `kParams`

```
src/dsp/Params.cpp  kParams[]  ──┬─► DSP (Engine/Turbo/Pump liest params[id])
                                 ├─► plugin/*Plugin.cpp: initParameter() → fillParameter() → Host (VST3/AU/CLAP/LV2/…)
                                 ├─► OffbeatKitUI: Name, Bereich, Einheit, Anzeige, Zahleneingabe
                                 ├─► ParamText: JSON-Schlüssel = symbol
                                 └─► Renderer: --set symbol=wert, --list
```

Einen Regler hinzufügen heißt deshalb nur: Eintrag in `ParamId` (Params.hpp) **hinten anhängen**, Zeile in `kParams` (Params.cpp), im DSP benutzen und in der Oberfläche platzieren. Host-Anbindung, JSON und Zahleneingabe funktionieren dann von allein.

> **Nie** bestehende Einträge umsortieren oder löschen. Die Position ist die Parameter-ID in allen Formaten; gespeicherte Projekte würden sonst falsche Werte laden. Ebenso nie die Plugin-URI, VST3/AU-IDs (`DistrhoPluginInfo.h`) oder den Bundle-Namen (`plugin/Makefile: NAME`) ändern.

### OffbeatKitUI (`plugin/OffbeatKit.hpp`)

Die Oberflächen sind ein „Immediate-Mode“-GUI. `onNanoDisplay()` zeichnet bei jedem Bild alles neu. Jeder Baustein, der dabei gezeichnet wird, meldet seine Klickfläche an. Maus und Tastatur arbeiten dann mit den Flächen des zuletzt gezeichneten Bildes. Ein Plugin leitet von `OffbeatKitUI` ab und überschreibt:

| Methode | Zweck |
|---|---|
| `drawContent()` | alles zeichnen (Pflicht) |
| `onButton(id)` | eigene Knöpfe, z. B. Modus-Reiter |
| `controlLabel(p)` | kürzerer Name für einen Regler |

Bausteine:

| Baustein | Was er zeichnet |
|---|---|
| `drawBackground()` | Verlauf und Lochblech (automatisch) |
| `drawLogo(cx, cy)` | Chrom-Plakette mit Swoosh |
| `drawPanel(x, y, w, h, label, farbe)` | Abschnitt im Ticket-Stil |
| `knob(p, x, y, farbe)` | Regler in einer Zelle von 114 × 112 Pixeln; Schalter werden automatisch zur Pille |
| `bigKnob(p, cx, cy, r, farbe)` | großer Regler im Manometer-Look |
| `compact(p, x, y, w, farbe)` | kleiner Regler für Kopfzeilen |
| `pill(id, x, y, w, h, text, farbe, aktiv)` | Knopf |
| `copyPaste(xRechts, y)` | KOPIEREN/EINFÜGEN mit Statuszeile |
| `drawImage(bild, …)` | eingebettetes Bild |
| `drawTag(…)` | kleines Etikett |

Farben und Schriftzug kommen aus einem `kit::Theme`. Die Werte liegen in `values[]` und werden vom Host über `parameterChanged()` aktuell gehalten; `setValue()` schickt eine Änderung an den Host.

**Bedienung:**
- ziehen (mit Shift fein), Mausrad,
- Doppelklick setzt den Standard,
- Klick auf den Wert oder Rechtsklick öffnet die Zahleneingabe,
- Schalter per Klick,
- KOPIEREN/EINFÜGEN tauscht JSON über die Zwischenablage aus; welche Regler darin stehen, legt `jsonParams()` in Params.cpp fest.

**Bilder der Oberfläche** (für die Doku): `make standalone`, dann

```sh
OFFBEAT_SCREENSHOT=ui.ppm OFFBEAT_SCREENSHOT_PRESET=2 bin/<name>.app/Contents/MacOS/<name>
```

Die Oberfläche lädt das Preset, speichert sich als PPM und beendet sich. Die Bilder in `docs/images/` sind so entstanden (PPM → PNG umgewandelt).

### Formate und Build

`plugin/Makefile` nutzt DPF (`dpf/Makefile.plugins.mk`) und baut VST3, VST2, CLAP, LV2 und auf dem Mac AU, bei Turbo und Hydraulics dazu LADSPA. Das Makefile im Hauptordner ruft es auf und baut auf dem Mac ein Universal-Binary (`-arch x86_64 -arch arm64`, `NOOPT=true`). Außerdem erzeugt es die LV2-Beschreibungen (`generate-ttl.sh`) und baut den Offline-Renderer.

Das Bild in `assets/` wird beim Build mit `xxd -i` zu einem C-Array und fest in die Oberfläche eingebaut.

`.github/workflows/build.yml` baut bei jedem Push auf macOS, Windows (MSYS2/MinGW) und Linux und führt `make check` aus. Bei einem Tag `v*` legt es ein Release mit den ZIPs an.

### Echtzeit-Regeln im DSP

- In `process()` kein `new`/`malloc`, keine Locks, keine Datei-Zugriffe. Puffer werden in `setSampleRate()` angelegt.
- Am Anfang jedes `process()` steht `const DenormalGuard denormals;` (Flush-to-Zero).
- Parameter werden in `setParam()` auf ihren Bereich begrenzt; Sprünge glättet `Smoother`.
- Der Ausgang läuft am Ende durch `softLimit()` (Instrumente) bzw. das Ceiling (Turbo); es gibt kein hartes Übersteuern.

### Prüfen

`make check` startet den Offline-Renderer mit `--check-all`. Er rechnet Presets bzw. Testsignale ohne DAW, misst sie (Tonhöhe, Länge, Spektrum, L/R-Korrelation, True-Peak, …) und vergleicht sie mit festen Zielen. Jede Änderung am Klang sollte danach noch grün sein, oder die Ziele werden bewusst angepasst.
