// Parameter-Beschreibung für Offbeat Performance (siehe OffbeatMeta.hpp): Ansicht nach Modus, wichtigste Regler
// = die ersten 8 des Modus, Gruppen = je Modus + „Allgemein“ (alles, was zu keinem Modus gehört).
#pragma once

#include "../dsp/Params.hpp"
#include "OffbeatMeta.hpp"

#ifndef OFFBEAT_VERSION
#define OFFBEAT_VERSION "dev"
#endif

namespace offbeat {

inline bool writePluginMeta()
{
    std::vector<std::pair<std::string, std::vector<int>>> key;
    std::vector<meta::Group> groups;
    std::vector<bool> inMode(kParamCount, false);
    for (int m = 0; m < kModeCount; ++m) {
        int list[16];
        const int n = modeParams(m, list);
        meta::Group g { kModeNames[m], {} };
        for (int i = 0; i < n; ++i)
            if (meta::editable(kParams[list[i]]))
                g.params.push_back(list[i]), inMode[list[i]] = true;
        std::vector<int> k(g.params.begin(), g.params.begin() + (long)std::min<size_t>(8, g.params.size()));
        key.push_back({ std::to_string(m), k });
        groups.push_back(g);
    }
    meta::Group general { "Allgemein", {} };
    for (int i = 0; i < kParamCount; ++i)
        if (!inMode[i] && meta::editable(kParams[i]))
            general.params.push_back(i);
    groups.insert(groups.begin(), general);
    return meta::write("topline", OFFBEAT_VERSION, kParams, kParamCount, kMode, key, groups);
}

} // namespace offbeat
