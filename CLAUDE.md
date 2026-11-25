# CLAUDE.md – Offbeat Topline (Repo offbeat-topline)

Projekt: Melodie-Synth (Supersaw, Stab, Pluck, Whistle) der Offbeat-Familie (LV2 + VST3, DPF) für Hard Bounce / Bounce Techno. Läuft in LMMS 1.2.2 über Carla Rack (Carla 2.5.10) auf einem Intel-Mac (MacBook Pro 2019, x86_64). Ziele, Aufbau und Abnahmekriterien stehen in `docs/PLAN.md`: zuerst lesen.

## Arbeitsweise

- **Suchen:** Wenn `graphify-out/` existiert, zuerst den Graphen nutzen (deckt Code und Doku ab). Gezieltes grep nur für das, was der Graph nicht zeigt. Solange es noch keinen Graphen gibt, gezielt mit grep/glob suchen. Große Dateien nicht komplett lesen, wenn ein Ausschnitt reicht. `graphify-out/` nicht bearbeiten und nicht committen. Den DPF-Submodule nicht durchsuchen, außer für eine konkrete API-Frage.
- **Agents:** Bis auf Weiteres dürfen Subagents ohne Bedenken genutzt werden, auch parallel und mit jedem Modell, wenn das die Bauzeit verkürzt. **Sobald der Nutzer sagt, sparsamer zu sein, gilt ab dann:** einfache, klar umrissene Jobs (Dateien und Funktionen finden, Doku aktualisieren, Presets eintragen) gehen an Subagents mit Modell haiku oder sonnet; Opus nur für den Entwurf (DSP-Design, Architektur) und das Prüfen des Ergebnisses. Die übrigen Regeln gelten in beiden Fällen.
- **Erst schauen, dann bauen.** Vorhandenes wiederverwenden (DSP-Bausteine wie Oszillatoren, Hüllkurven, Filter nur einmal schreiben und von allen Modi nutzen). So wenig neuer Text wie möglich, keine Extra-Features über die Spezifikation hinaus.
- **Abhängigkeiten:** Einzige externe Abhängigkeit ist DPF (Git-Submodule). Keine weiteren Bibliotheken.
- **Tests:** Nur die betroffenen Tests laufen lassen. Ändert sich ein Modus, mit `topline-render` genau diesen Modus rendern und gegen die Abnahmekriterien in `docs/PLAN.md` messen (`make check` prüft alle Presets). Reine Doku-Änderungen brauchen keinen Testlauf.

## Offbeat-Familie

- Geschwister: offbeat-engine (Kick/Bass/Hat/Stab/Pad/Riser), offbeat-topline (Melodie), offbeat-turbo (Master-Lautheit), offbeat-hydraulics (Pumpen). Übersicht: `docs/FAMILIE.md` in offbeat-engine.
- **Gemeinsamer Code** ist eine bewusste Kopie aus offbeat-engine (Quelle steht im Dateikopf): `src/dsp/ParamInfo.hpp`, `src/dsp/ParamText.*`, `src/dsp/Blocks.hpp`, `plugin/OffbeatKit.hpp`, `plugin/OffbeatParam.hpp`, `tools/Measure.hpp`. Änderungen daran zuerst in offbeat-engine machen und dann hierher kopieren.
- **Nie ändern:** LV2-URI `https://github.com/lukestark-glitch/offbeat-topline`, VST3-IDs `Ofbt`/`OfbT`, Bundle-Name `offbeat-topline`. Sonst laden gespeicherte LMMS-Projekte nicht mehr. Parameter nur **hinten anhängen**.
- **LMMS:** schickt Noten 12 Halbtöne tiefer als angezeigt (Regler Octave, in LMMS +1). Tempo fürs Delay: Host-Tempo, Fallback BPM-Regler.
- Arbeit auf Branches, PR öffnen, nach grünen Checks selbst mergen und pushen. `docs/HANDOFF.md` kurz aktuell halten. Nach jedem fertigen Plugin `make install`.

## Regeln

- **Keine Screenshots, keine Browser-Tests.** Klang wird über `topline-render` und Messwerte geprüft. Was nur der Nutzer prüfen kann (Laden in Carla, Hören in LMMS), kommt als kurze Checkliste in den PR.
- **Entscheidungen, die beim Nutzer liegen:** die vorsichtigere, leicht änderbare Option wählen, unter „Offene Fragen“ im Plan notieren und weitermachen. Nicht warten.
- **Installation:** Die eigenen Offbeat-Plugins in `~/Library/Audio/Plug-Ins/VST3/` und `~/Library/Audio/Plug-Ins/LV2/` darf ohne Nachfrage überschrieben werden. Andere Plugins und Dateien außerhalb des Projektordners nicht anfassen.
- **Komplett blockiert:** Push-Benachrichtigung senden und, wenn möglich, an einem anderen Teil weiterarbeiten.
- **Sichern:** Nach getaner Arbeit committen (mit den üblichen Attribution-Zeilen) und pushen, damit nichts verloren geht. Wenn `docs/HANDOFF.md` existiert, kurz aktualisieren (was fertig ist, was als Nächstes kommt); sonst überspringen.

## Benachrichtigungen (PushNotification)

- Eine, wenn eine Frage an den Nutzer offen ist: Frage und gewählter Standard (1–2 Sätze), dann weitermachen.
- Eine, wenn alles fertig ist, mit allen PR- bzw. Repo-Links.
- Eine, wenn du blockiert bist oder wegen des Session-Limits aufhörst, mit Stand und nächstem Schritt.
- Keine anderen Fortschrittsmeldungen.
