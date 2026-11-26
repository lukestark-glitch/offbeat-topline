// Offbeat Link: Gearbox speichert/lädt den kompletten Sound der Synths, die an seinen Spuren hängen.
//
// Gemeinsamer Code der Familie (Quelle: offbeat-engine, in andere Repos kopiert). Nur Header.
//
// 1. Kennung: Gearbox schickt auf dem Kanal jeder Spur eine NRPN-Folge (CC 99/98/6/38, NRPN 0x3E00–0x3E02:
//    Instanz-Kennung hoch/niedrig, Spur). Kanalnachrichten kommen durch Carlas MIDI Channel Filter.
// 2. Register: Ein gemeinsamer Speicherbereich (POSIX shm / Windows-Mapping) mit 64 Plätzen. Jeder Synth mit
//    Kennung schreibt dort ~10× pro Sekunde seine Parameter (Seqlock, kein Lock, kein Speicher im Audio-Thread).
// 3. Laden: Gearbox schreibt eine Anfrage (Kennung, Spur, Werte); der Synth übernimmt sie im nächsten Block.
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace offbeat::link {

constexpr uint32_t kMagic = 0x4f424c31; // "OBL1"
constexpr int kSlots = 64, kMaxValues = 192, kExtra = 2048;
constexpr double kStale = 5.0; // Sekunden ohne Lebenszeichen: Platz gilt als frei
constexpr uint32_t kEngine = 0x454e474e, kTopline = 0x544f504c, kHorn = 0x484f524e; // ENGN TOPL HORN

struct Entry {
    std::atomic<uint32_t> seq;  // ungerade während des Schreibens
    uint32_t id, track, plugin; // Gearbox-Kennung (0 = frei), Spur 0–7, Plugin-Art
    double stamp;               // Sekunden (steady_clock) des letzten Schreibens
    int32_t count;
    float values[kMaxValues];
    char extra[kExtra];         // z. B. Dateipfade von Horn, \n-getrennt
};

struct Registry {
    std::atomic<uint32_t> magic;
    Entry state[kSlots];   // von Synths geschrieben
    Entry request[kSlots]; // von Gearbox geschrieben (Laden)
};

inline double now()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Öffnet (oder legt an) das Register. Nicht im Audio-Thread aufrufen. nullptr = nicht verfügbar.
inline Registry* open()
{
#ifdef _WIN32
    HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Registry), "Local\\offbeat-link");
    if (!h)
        return nullptr;
    void* p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Registry));
#else
    const int fd = shm_open("/offbeat-link", O_RDWR | O_CREAT, 0600);
    if (fd < 0)
        return nullptr;
    struct stat st {};
    if (fstat(fd, &st) == 0 && st.st_size < (off_t)sizeof(Registry))
        ftruncate(fd, sizeof(Registry)); // neu: mit Nullen gefüllt
    void* p = mmap(nullptr, sizeof(Registry), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED)
        return nullptr;
#endif
    if (!p)
        return nullptr;
    Registry* r = static_cast<Registry*>(p);
    uint32_t zero = 0;
    r->magic.compare_exchange_strong(zero, kMagic);
    return r->magic.load() == kMagic ? r : nullptr;
}

inline void write(Entry& e, uint32_t id, uint32_t track, uint32_t plugin, const float* values, int count, const char* extra)
{
    const uint32_t s = e.seq.load(std::memory_order_relaxed);
    e.seq.store(s | 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    e.id = id, e.track = track, e.plugin = plugin, e.stamp = now();
    e.count = count < kMaxValues ? count : kMaxValues;
    std::memcpy(e.values, values, sizeof(float) * (size_t)e.count);
    if (extra)
        std::strncpy(e.extra, extra, kExtra - 1), e.extra[kExtra - 1] = 0;
    else
        e.extra[0] = 0;
    std::atomic_thread_fence(std::memory_order_release);
    e.seq.store((s | 1) + 1, std::memory_order_release);
}

// Konsistente Kopie; false, wenn gerade geschrieben wird.
inline bool read(const Entry& e, Entry& out)
{
    const uint32_t s = e.seq.load(std::memory_order_acquire);
    if (s & 1)
        return false;
    out.id = e.id, out.track = e.track, out.plugin = e.plugin, out.stamp = e.stamp;
    out.count = e.count < kMaxValues ? e.count : kMaxValues;
    std::memcpy(out.values, e.values, sizeof(float) * (size_t)(out.count > 0 ? out.count : 0));
    std::memcpy(out.extra, e.extra, kExtra);
    out.extra[kExtra - 1] = 0;
    std::atomic_thread_fence(std::memory_order_acquire);
    out.seq.store(s, std::memory_order_relaxed);
    return e.seq.load(std::memory_order_relaxed) == s;
}

// Platz für (id, track) in einer Tabelle: vorhandener, sonst freier/veralteter. -1 = voll.
inline int find(Entry* table, uint32_t id, uint32_t track, bool claim)
{
    int freeSlot = -1;
    const double t = now();
    for (int i = 0; i < kSlots; ++i) {
        if (table[i].id == id && table[i].track == track)
            return i;
        if (freeSlot < 0 && (table[i].id == 0 || t - table[i].stamp > kStale))
            freeSlot = i;
    }
    return claim ? freeSlot : -1;
}

// Synth-Seite: wertet die NRPN-Kennung aus und spiegelt den Zustand ins Register.
class Client {
public:
    explicit Client(uint32_t pluginType) : plugin(pluginType), reg(open()) {}

    // Für jede eingehende CC-Nachricht aufrufen (Audio-Thread).
    void cc(uint8_t controller, uint8_t value)
    {
        if (controller == 99) nrpnMsb = value;
        else if (controller == 98) nrpnLsb = value;
        else if (controller == 6) dataMsb = value;
        else if (controller == 38 && nrpnMsb == 0x3e) {
            const uint32_t v = (uint32_t)dataMsb << 7 | value;
            if (nrpnLsb == 0) idHigh = v;
            else if (nrpnLsb == 1) idLow = v;
            else if (nrpnLsb == 2 && (idHigh | idLow)) { // Spur kommt zuletzt: Kennung vollständig
                id = idHigh << 14 | idLow;
                track = v;
            }
        }
    }

    bool linked() const { return reg && id; }

    // Einmal pro Block (Audio-Thread): schreibt ~10×/s den Zustand; true + Werte, wenn eine Lade-Anfrage vorliegt.
    template <class Get>
    bool update(double seconds, int count, Get get, float* requested, int& requestedCount, char* extra = nullptr)
    {
        if (!linked())
            return false;
        bool got = false;
        if (const int r = find(reg->request, id, track, false); r >= 0) {
            const uint32_t s = reg->request[r].seq.load(std::memory_order_acquire);
            // neue Anfrage für diesen Synth, höchstens 10 s alt (alte Anfragen nach einem Neustart nicht anwenden)
            if (s != lastRequest && !(s & 1) && read(reg->request[r], scratch)) {
                lastRequest = s;
                if (scratch.plugin == plugin && now() - scratch.stamp < 10.0) {
                    requestedCount = scratch.count;
                    std::memcpy(requested, scratch.values, sizeof(float) * (size_t)scratch.count);
                    if (extra)
                        std::memcpy(extra, scratch.extra, kExtra);
                    got = true;
                }
            }
        }
        sinceWrite += seconds;
        if (sinceWrite >= 0.1) {
            sinceWrite = 0;
            if (const int w = find(reg->state, id, track, true); w >= 0) {
                const int n = count < kMaxValues ? count : kMaxValues;
                for (int i = 0; i < n; ++i)
                    values[i] = get(i);
                write(reg->state[w], id, track, plugin, values, n, extraOut);
            }
        }
        return got;
    }

    void setExtra(const char* s) { std::strncpy(extraOut, s, kExtra - 1), extraOut[kExtra - 1] = 0; } // nicht im Audio-Thread

private:
    uint32_t plugin;
    Registry* reg;
    uint8_t nrpnMsb = 127, nrpnLsb = 127, dataMsb = 0;
    uint32_t idHigh = 0, idLow = 0, id = 0, track = 0, lastRequest = ~0u;
    double sinceWrite = 1.0;
    float values[kMaxValues] {};
    char extraOut[kExtra] {};
    Entry scratch {};
};

} // namespace offbeat::link
