# Offbeat Topline – alle Regler

Oben wählst du den **Modus** (Supersaw, Stab, Pluck, Whistle). Darunter liegen **GLOBAL**, die Regler des Modus und **FX**.

**Bedienung** (wie bei Offbeat Engine):
- Regler hoch/runter ziehen, Shift = fein, Mausrad, Doppelklick = Standardwert.
- Klick auf den Wert oder Rechtsklick: Zahl eintippen.
- **KOPIEREN / EINFÜGEN:** Einstellungen als JSON über die Zwischenablage.

Presets wählst du in Carla unter „Programs“. Die Taste im Namen (z. B. F5) ist die gedachte Grundtaste.

**LMMS:** LMMS schickt Noten 12 Halbtöne tiefer, als die Piano-Roll zeigt. Stell darum **Octave auf +1**, dann klingt die LMMS-Taste F5 wie F5.

## GLOBAL

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Mode | Supersaw … Whistle | Supersaw | Klang |
| Volume | −40 … +6 dB | −6 dB | Gesamtlautstärke (Limiter am Ende) |
| Octave | −2 … +2 | 0 | Verschiebt alle Noten um Oktaven; in LMMS +1 |
| Stimmen | Poly / Mono | Poly | Mono: Legato, eine neue Note bei gehaltener Taste gleitet ohne Neuanschlag hin |
| Glide | 0 … 500 ms | 0 ms | Gleiten zur neuen Note |
| Chord | Aus / Moll / Dur / Moll7 / Dur7 | Aus | Eine Taste spielt einen Akkord (7 = kleine Septime) |
| Vibrato Depth | 0 … 1 st | 0 | Stärke des Vibratos |
| Vibrato Rate | 1 … 10 Hz | 5,5 Hz | Tempo des Vibratos |
| Vibrato Delay | 0 … 1000 ms | 250 ms | Ab wann das Vibrato einsetzt (blendet über 200 ms ein) |

## FX

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Chorus | 0 … 100 % | 30 % | Stereo-Chorus |
| Delay Mix | 0 … 100 % | 15 % | Lautstärke des Ping-Pong-Echos |
| Delay Time | 1/16 … 1/4. | 1/8. | Echo-Zeit im Takt |
| Delay Feedback | 0 … 90 % | 35 % | Wie viele Wiederholungen |
| Reverb Mix | 0 … 100 % | 25 % | Hall-Anteil |
| Reverb Size | 0 … 100 % | 60 % | Hall-Länge |
| Host-Tempo | An/Aus | An | An: Echo-Zeit nach Host-Tempo; Aus (oder ohne Host-Tempo): BPM |
| BPM | 60 … 220 | 160 | Tempo fürs Echo ohne Host-Tempo |

## Supersaw (Leads und Akkorde, gedacht für B4–E6)

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Voices | 1 … 7 | 7 | Sägezähne pro Note |
| Detune | 0 … 100 % | 35 % | Verstimmung, mehr = breiter und schwebender |
| Cutoff | 200 Hz … 18 kHz | 7 kHz | Helligkeit |
| Resonance | 0 … 100 % | 10 % | Betonung an der Filterfrequenz |
| Filter Env | 0 … 100 % | 20 % | Filter öffnet beim Anschlag |
| Filter Decay | 5 … 3000 ms | 400 ms | Wie schnell es wieder zugeht |
| Attack / Decay / Sustain / Release | | 8 ms / 600 ms / 80 % / 700 ms | Lautstärke-Hüllkurve; Release 0,5–0,9 s klingt wie in den Referenzen |
| Width | 0 … 100 % | 65 % | Stereobreite (65 % ≈ Korrelation 0,5); unter 150 Hz immer mono |

## Stab (kurze Akkord-Stöße)

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Voices / Detune | 1 … 7 / 0 … 100 % | 5 / 25 % | Wie bei Supersaw |
| Cutoff | 200 Hz … 18 kHz | 3,5 kHz | Helligkeit |
| Filter Env / Filter Decay | | 50 % / 90 ms | „Pling“ am Anfang |
| Decay | 20 … 1000 ms | 140 ms | Länge des Stoßes |
| Release | 5 … 1000 ms | 60 ms | Ausklingen nach dem Loslassen |
| Shine | 0 … 12 dB | 4 dB | Glanz: Anhebung in den Höhen |
| Shine Freq | 4 … 8 kHz | 6 kHz | Wo der Glanz sitzt |
| Width | 0 … 100 % | 60 % | Stereobreite (60 % ≈ Korrelation 0,6) |

## Pluck (perkussiv, kräftige Mitten)

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Tone | 200 Hz … 8 kHz | 1,5 kHz | Grundhelligkeit |
| Filter Env | 0 … 100 % | 60 % | „Zupf“-Anteil |
| Decay | 20 … 600 ms | 130 ms | Länge (≈ 90 ms bis −30 dB) |
| Body | 0 … 12 dB | 6 dB | Anhebung um 1 kHz (Mitten) |
| Detune | 0 … 100 % | 15 % | Leichte Verstimmung (3 Sägezähne); 0 = ein Sägezahn |
| Width | 0 … 100 % | 45 % | Fast mono (≈ Korrelation 0,9) |

## Whistle (Rave-Pfeife, Grundton per Note, Standard D#7 ≈ 2,5 kHz)

| Regler | Bereich | Standard | Wirkung |
|---|---|---|---|
| Breath | 0 … 100 % | 15 % | Atemrauschen um den Grundton |
| Scoop | 0 … 3 st | 0,8 st | Einsatz rutscht von so weit unten hoch |
| Scoop Time | 5 … 100 ms | 30 ms | Wie schnell |
| Trill | 0 … 3 st | 0 | Triller-Intervall (0 = aus) |
| Trill Rate | 2 … 20 Hz | 12 Hz | Triller-Tempo |
| Length | 50 … 2000 ms | 180 ms | Länge eines Stoßes, auch wenn die Taste länger gehalten wird |
| Width | 0 … 100 % | 10 % | Stereobreite |

## JSON (Kopieren / Einfügen)

Gleiche Regeln wie bei Offbeat Engine:
- Die Schlüssel sind die Kurznamen unten.
- Zahlen stehen in der Einheit des Plugins, %-Regler als 0 … 1.
- Auswahl-Regler gehen als Name (`"chord": "Moll7"`), Schalter als `true`/`false`.
- Nicht genannte Regler bleiben, wie sie sind.

Der Renderer liest dasselbe Format: `bin/topline-render --json sound.json --note F5`.

**Kurznamen**
- **Global:** `mode volume octave voice_mode glide chord vib_depth vib_rate vib_delay`
- **FX:** `chorus delay_mix delay_time delay_fb reverb_mix reverb_size tempo_host tempo_bpm`
- **Supersaw:** `saw_voices saw_detune saw_cutoff saw_reso saw_envamt saw_fdecay saw_attack saw_decay saw_sustain saw_release saw_width`
- **Stab:** `stab_voices stab_detune stab_cutoff stab_envamt stab_fdecay stab_decay stab_release stab_shine stab_shine_hz stab_width`
- **Pluck:** `pluck_tone pluck_envamt pluck_decay pluck_body pluck_detune pluck_width`
- **Whistle:** `whistle_breath whistle_scoop whistle_scoop_ms whistle_trill whistle_trill_hz whistle_length whistle_width`

## Presets

| Nr. | Name | Idee |
|---|---|---|
| 1 | Supersaw – F-Moll Hymne (F5) | Breiter Moll-Akkord mit Hall |
| 2 | Supersaw – E-Dur Euphorie (E5) | Heller Dur-Akkord |
| 3 | Lead – Mono Glide | Mono-Legato-Lead mit Glide und Vibrato |
| 4 | Stab – F-Moll Rave (F5) | Kurzer Moll-Stab |
| 5 | Stab – A-Dur Shine (A4) | Dur-Stab mit viel Glanz |
| 6 | Stab – F-Moll7 Groove (F5) | Moll7, etwas länger |
| 7 | Pluck – F-Moll (F5) | Einzelton-Pluck mit Echo |
| 8 | Pluck – E-Dur Chords (E5) | Dur-Akkord-Pluck |
| 9 | Whistle – Rave Pfeife (D#7) | Kurze Pfiffe |
| 10 | Whistle – Triller (D#7) | Trillerpfeife, 600 ms |
