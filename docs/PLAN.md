# Plan – Offbeat Topline

Melodie-Synth der Offbeat-Familie (Supersaw-Lead/Chords, Chord-Stab, Pluck, Rave-Pfeife). LV2 + VST3 über DPF, gleicher Look und gleiche Bedienung wie Offbeat Engine, Farben Rot/Schwarz mit Magenta, Motorrad-Bild.

## Klang-Ziele (aus den Messungen des Nutzers)

| Modus | Ziel | Prüfung im Renderer (`--check`) |
|---|---|---|
| Supersaw | sehr breit (L/R-Korrelation ~0,5), Release 0,5–0,9 s, Hall, Tonbereich ~B4–E6 | Korrelation 0,3–0,7; Release-Hüllkurve 0,5–0,9 s (Hall abgeschaltet gemessen); Schwerpunkt 1–6 kHz |
| Stab | mehrstimmig, 70–200 ms, breit (~0,55–0,7), Glanz 4–8 kHz einstellbar | Länge bis −30 dB 70–200 ms; Korrelation 0,5–0,75; Shine 12 dB hebt 4–8 kHz um ≥ 6 dB |
| Pluck | perkussiv ~90 ms, fast mono (~0,9), kräftige Mitten 0,5–2 kHz | Länge bis −30 dB 60–130 ms; Korrelation ≥ 0,85; Schwerpunkt 0,5–2,5 kHz |
| Whistle | Sinus + Atem, Grundton per Note (≈ 2,5 kHz / D#7), Einsatz rutscht von ~0,8 Halbtönen darunter in ~30 ms hoch, optional Triller, Stöße 100–250 ms | Tonhöhe nach 60 ms ±25 Cent; nach 3 ms 0,4–1,2 Halbtöne tiefer; Schwerpunkt ≤ 1,6 × Grundton; Länge 100–250 ms |

Für alle Modi wird außerdem geprüft:
- unter 150 Hz mono (Korrelation ≥ 0,98),
- kein Clipping,
- weicher Anfang und weiches Ende,
- keine Denormals und kein NaN.

Die Obertöne kommen von Sägezähnen, es gibt keinen Rechteck-Kern. Die Pfeife ist ein Sinus.

## Aufbau

Gleiche Struktur wie Offbeat Engine:
- `src/dsp/`: DSP-Kern ohne DPF.
- `plugin/`: DPF-Hülle und Oberfläche.
- `tools/topline-render.cpp`: Offline-Renderer mit `--check` und `--check-all`.

**Gemeinsam mit Engine** (bewusste Kopie, Quelle ist `offbeat-engine`):
- `ParamInfo.hpp`, `ParamText.*` (Zahleneingabe und JSON),
- `Blocks.hpp` (Oszillatoren, SVF, ADSR, Chorus),
- `OffbeatKit.hpp` (Look und Bedienung), `OffbeatParam.hpp`,
- `tools/Measure.hpp`.

Warum eine Kopie und kein Submodule, steht im Plan von offbeat-engine. Kurz: Jedes Repo baut für sich, und die gemeinsamen Teile sind klein.

**Stimme** (`Voice.hpp`, abgeleitet von der Poly-Stimme der Engine):
- **Klang je Modus:**
  - Supersaw und Stab: bis zu 7 verstimmte Sägezähne mit Stereo-Spreizung.
  - Pluck: 1–3 Sägezähne mit wenig Verstimmung.
  - Whistle: Sinus und Atemrauschen (Bandpass um den Grundton).
- **Tonhöhe:** Glide, Vibrato mit Einsatzverzögerung, Pfeifen-Rutscher und Triller.
- **Klangformung:** Stereo-SVF-Low-Pass mit Filter-Hüllkurve und ADSR.
- **Stimmenzahl:** 10 Stimmen, damit zwei gehaltene Vierklänge Platz haben.

**Spielweise:**
- **Chord-Hold:** Aus, Moll, Dur, Moll7, Dur7 (7 = kleine Septime).
- **Stimmen:** Poly, oder Mono mit Legato. Mono bedeutet: Eine neue Note bei gehaltener Taste gleitet ohne Neuanschlag hin, und beim Loslassen geht es zur noch gehaltenen Note zurück.

**Bus** (der Reihe nach):
1. Glanz-EQ (Stab) bzw. Mitten-EQ (Pluck).
2. Chorus.
3. Delay: Ping-Pong, im Takt.
4. Hall: Freeverb-Prinzip, Hochpass vor dem Hall.
5. Width pro Modus (Mitte/Seite, Seite unter 150 Hz 48 dB/Okt. weg).
6. Volume und Soft-Limiter.

**Tempo:** Das Delay läuft synchron zum Tempo. Es nutzt Host-Tempo und BPM wie die Engine: Das Host-Tempo gilt, solange es an ist und der Host eines liefert; sonst gilt der BPM-Regler.

**Oktave:** Regler Octave (−2…+2, Standard +1), gleicht aus, dass LMMS Noten 12 Halbtöne tiefer schickt.

**IDs** (ab dem ersten Release nie mehr ändern):
- URI `https://github.com/lukestark-glitch/offbeat-topline`
- VST3 `Ofbt`/`OfbT`
- Bundle `offbeat-topline`

**Presets** in F-Moll sowie A-Dur und E-Dur. Der Name sagt, welche Taste und welcher Akkord gemeint ist.

## Oberfläche

- **Kopf:** Logo „OFFBEAT / TOPLINE“ und vier Modus-Reiter.
- **GLOBAL:** Volume, Glide, Vibrato (Tiefe, Rate, Einsatz) und das Motorrad. In der Kopfzeile Octave, Stimmen (Poly/Mono) und Chord.
- **Modus-Abschnitt:** Regler des Modus, mit eigenem Width.
- **FX:** Chorus, Delay (Mix, Zeit, Feedback), Hall (Mix, Größe). In der Kopfzeile Host-Tempo und BPM.

## Stand (PR #1)

`make check` (= `topline-render --check-all`): **10 von 10 Presets bestanden**.

| Preset | Messwerte |
|---|---|
| Supersaw F-Moll / E-Dur / Mono-Lead | Korrelation 0,49 / 0,45 / 0,54; Release 630 / 629 / 541 ms; Schwerpunkt 1,2–1,4 kHz |
| Stab F-Moll / A-Dur / Moll7 | Länge −30 dB 87 / 94 / 111 ms; Korrelation 0,61–0,62; Shine 12 dB bringt +10,6 bis +10,9 dB bei 4–8 kHz |
| Pluck F-Moll / E-Dur | Länge −30 dB ≈ 93 ms; Korrelation ≈ 0,92; Schwerpunkt 1,0–1,1 kHz |
| Whistle / Triller | Tonhöhe −1 / −4 Cent; Einsatz −0,60 Halbtöne; Triller +1,97 Halbtöne; Stoß 209 ms |

Rechenlast: zwei gehaltene Moll7-Stabs (8 Stimmen) samt Effekten laufen etwa 11-fach schneller als Echtzeit.

## Offene Fragen

- **Repo öffentlich:** Der Nutzer hat die Repos schon angelegt (öffentlich). Laut Auftrag sollten sie privat sein. Die Sichtbarkeit lasse ich, wie sie ist; der Nutzer kann sie in den GitHub-Einstellungen ändern. Es gibt außerdem ein leeres Repo `offbeat_topline` (mit Unterstrich); ich lasse es unangetastet.
- **Octave-Standard:** Der Nutzer hat entschieden: **+1** (gleicht LMMS aus). Carla-Tastatur und andere Hosts klingen damit eine Oktave höher, dort Octave auf 0 stellen. Der Renderer schickt `--note` wie LMMS 12 Halbtöne tiefer.
- **FX global statt pro Modus:** Delay und Hall gehören zur Instanz, nicht zum Modus. Die Presets setzen passende Werte. Width ist pro Modus, wie verlangt.
- **Whistle-Länge:** Ein Stoß dauert höchstens „Length“ (Standard 180 ms), auch wenn die Taste länger gehalten wird. Für gehaltene Pfeifen Length auf 2000 ms stellen.
- **7er-Akkorde:** Moll7 = Moll + kleine Septime, Dur7 = Dominantseptakkord. Maj7 gibt es nicht.
- **Supersaw-Release gemessen ohne Hall:** Der Hall verlängert das Ausklingen hörbar; geprüft wird die Hüllkurve allein.
