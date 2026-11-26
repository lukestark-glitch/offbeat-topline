// Vorschau von Offbeat Topline: ein Ton bzw. Akkord des Modus oder eine kurze Phrase (1 Takt).
// Genutzt von der Oberfläche (Wellenform), vom DSP (Vorhören über den Plugin-Ausgang) und von
// topline-render (--preview-check). Tasten wie im Renderer: gedachte Taste, Octave eingerechnet.
#pragma once

#include "Engine.hpp"
#include "Render.hpp"

namespace offbeat {

constexpr double kPreviewBeats = 4.0; // gezeigter und gespielter Ausschnitt: 1 Takt (Hall und Release passen hinein)

// Noten für Ton (phrase = false) oder Phrase. Schreibt höchstens max Ereignisse (ohne Speicher anzufordern).
int previewEvents(const float* params, bool phrase, double bpm, int sr, NoteEvent* out, int max);

// Offline rendern, genau wie topline-render (gleiche Engine, gleiche Blöcke).
void renderPreview(const float* params, bool phrase, double bpm, int sr, std::vector<float>& l, std::vector<float>& r);

} // namespace offbeat
