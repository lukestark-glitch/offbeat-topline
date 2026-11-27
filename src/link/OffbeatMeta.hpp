// Offbeat Meta: Synths beschreiben ihre Parameter für Offbeat Performance (Regler im Performance-Fenster).
//
// Gemeinsamer Code der Familie (Quelle: offbeat-engine, in andere Repos kopiert). Nur Header. Jeder Synth schreibt
// beim Erzeugen (nie im Audio-Thread) `<Ordner>/<plugin>.json`, nur wenn sich der Inhalt geändert hat:
// macOS ~/Library/Application Support/Offbeat/meta, Windows %APPDATA%\Offbeat\meta, Linux ~/.config/offbeat/meta.
// Inhalt: {"plugin","version","params":[…],"selector":Index|null,"key":{"Wert":[Indizes]},"groups":[{"name","params"}]}
#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace offbeat::meta {

inline void makeDir(const std::string& d)
{
#ifdef _WIN32
    _mkdir(d.c_str());
#else
    mkdir(d.c_str(), 0755);
#endif
}

inline std::string dir()
{
    if (const char* over = std::getenv("OFFBEAT_META_DIR")) { // Tests
        makeDir(over);
        return over;
    }
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    std::string d = std::string(base ? base : ".") + "\\Offbeat";
    makeDir(d);
    d += "\\meta";
#else
    const char* home = std::getenv("HOME");
    std::string d = std::string(home ? home : ".");
#ifdef __APPLE__
    d += "/Library/Application Support";
    makeDir(d);
    d += "/Offbeat";
#else
    d += "/.config";
    makeDir(d);
    d += "/offbeat";
#endif
    makeDir(d);
    d += "/meta";
#endif
    makeDir(d);
    return d;
}

inline std::string quote(const char* s)
{
    std::string o = "\"";
    for (const char* p = s ? s : ""; *p; ++p) {
        const unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\')
            o += '\\', o += (char)c;
        else if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
            o += buf;
        } else
            o += (char)c;
    }
    return o + "\"";
}

inline std::string list(const std::vector<int>& v)
{
    std::string o = "[";
    for (size_t i = 0; i < v.size(); ++i)
        o += (i ? "," : "") + std::to_string(v[i]);
    return o + "]";
}

// ParamInfo der Familie (name, symbol, unit, min, max, def, flags, labels); Flags 8 Anzeige, 16 versteckt
template <class P>
std::string paramsJson(const P* params, int count)
{
    std::string o = "[";
    char buf[160];
    for (int i = 0; i < count; ++i) {
        const P& p = params[i];
        std::snprintf(buf, sizeof(buf), ",\"min\":%.9g,\"max\":%.9g,\"def\":%.9g,\"flags\":%u,\"labels\":", (double)p.min,
                      (double)p.max, (double)p.def, (unsigned)p.flags);
        o += (i ? ",{\"index\":" : "{\"index\":") + std::to_string(i) + ",\"name\":" + quote(p.name) + ",\"symbol\":"
           + quote(p.symbol) + ",\"unit\":" + quote(p.unit) + buf;
        if (p.labels && (p.flags & 4)) {
            o += "[";
            for (int k = 0; k <= (int)(p.max - p.min); ++k)
                o += (k ? "," : "") + quote(p.labels[k]);
            o += "]}";
        } else
            o += "null}";
    }
    return o + "]";
}

template <class P>
bool editable(const P& p) // nicht: Anzeigen, versteckte Bedienelemente, MIDI-Kanal (Verkabelung, kein Klang)
{
    return !(p.flags & (8 | 16)) && std::string(p.symbol) != "midi_channel";
}

struct Group {
    std::string name;
    std::vector<int> params;
};

// Schreibt die Datei, wenn sich der Inhalt geändert hat. selector < 0 = keiner; key: Paare (Wert-Name, Indizes)
template <class P>
bool write(const char* plugin, const char* version, const P* params, int count, int selector,
           const std::vector<std::pair<std::string, std::vector<int>>>& key, const std::vector<Group>& groups)
{
    std::string o = "{\"plugin\":" + quote(plugin) + ",\"version\":" + quote(version) + ",\"params\":" + paramsJson(params, count)
                   + ",\"selector\":" + (selector >= 0 ? std::to_string(selector) : std::string("null")) + ",\"key\":{";
    for (size_t i = 0; i < key.size(); ++i)
        o += (i ? "," : "") + quote(key[i].first.c_str()) + ":" + list(key[i].second);
    o += "},\"groups\":[";
    for (size_t i = 0; i < groups.size(); ++i)
        o += (i ? ",{\"name\":" : "{\"name\":") + quote(groups[i].name.c_str()) + ",\"params\":" + list(groups[i].params) + "}";
    o += "]}\n";
    const std::string path = dir() + "/" + plugin + ".json";
    if (FILE* f = std::fopen(path.c_str(), "rb")) { // unverändert: nicht schreiben
        std::string old;
        char buf[8192];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
            old.append(buf, n);
        std::fclose(f);
        if (old == o)
            return true;
    }
    const std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f)
        return false;
    const bool ok = std::fwrite(o.data(), 1, o.size(), f) == o.size();
    std::fclose(f);
    std::remove(path.c_str());
    return ok && std::rename(tmp.c_str(), path.c_str()) == 0;
}

} // namespace offbeat::meta
