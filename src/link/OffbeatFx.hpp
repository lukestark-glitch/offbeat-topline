// Offbeat Beat FX: gemeinsamer Effekt-Zustand, den Offbeat Clutch (FLX4 Beat FX / Performance) schreibt und die
// Synths (Engine, Topline, Horn) lesen. Der Effekt rechnet im Synth jeder ausgewählten Spur – keine Verkabelung.
//
// Gemeinsamer Code der Familie (Quelle: offbeat-engine, in andere Repos kopiert). Nur Header. POSIX shm
// „/offbeat-fx“ (Windows: Mapping „Local\offbeat-fx“), Seqlock; lesen ist im Audio-Thread erlaubt (nur Kopieren).
#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace offbeat::fx {

constexpr uint32_t kMagic = 0x4f424658; // "OBFX"
enum Type : int32_t { kEcho, kDelay, kPingPong, kReverb, kFlanger, kPhaser, kTypeCount };
inline const char* typeName(int t)
{
    static const char* const k[kTypeCount] = { "Echo", "Delay", "Ping Pong", "Reverb", "Flanger", "Phaser" };
    return t >= 0 && t < kTypeCount ? k[t] : "";
}
// BEAT ◀ ▶: Länge in Schlägen (Echo/Delay/Ping Pong: Zeit; Reverb: Größe/Nachhall; Flanger/Phaser: LFO-Periode × 4)
constexpr int kBeatCount = 7;
inline double beatValue(int i)
{
    static const double k[kBeatCount] = { 0.125, 0.25, 0.5, 0.75, 1.0, 2.0, 4.0 };
    return k[i < 0 ? 0 : i >= kBeatCount ? kBeatCount - 1 : i];
}
inline const char* beatName(int i)
{
    static const char* const k[kBeatCount] = { "1/8", "1/4", "1/2", "3/4", "1", "2", "4" };
    return k[i < 0 ? 0 : i >= kBeatCount ? kBeatCount - 1 : i];
}

struct State {
    int32_t on = 0;        // Effekt an (Eingang offen); aus: Echo/Hall klingen aus
    int32_t type = kEcho;
    int32_t beat = 2;      // Index in beatValue (Standard 1/2)
    float level = 0.5f;    // LEVEL/DEPTH 0–1 (Anteil des Effekts)
    int32_t deckMask = 1;  // Kanal-Schalter: Bit 0 Deck 1, Bit 1 Deck 2
    uint32_t deckLink[2] = { 0, 0 }; // Offbeat-Link-Kennung der Gearbox je Deck (ordnet Synths den Decks zu)
    int32_t trackMask[2] = { 0, 0 }; // PAD FX: Spuren im Effekt je Deck (Bit = Spur 0–7); 0 = keine
};

struct Shared {
    std::atomic<uint32_t> magic;
    std::atomic<uint32_t> seq; // ungerade während des Schreibens
    State state;
};

// Öffnet (oder legt an) den Bereich. Nicht im Audio-Thread aufrufen. nullptr = nicht verfügbar.
inline Shared* open()
{
#ifdef _WIN32
    HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Shared), "Local\\offbeat-fx");
    if (!h)
        return nullptr;
    void* p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
#else
    const char* name = std::getenv("OFFBEAT_FX_NAME"); // Tests: eigener Bereich, stört kein laufendes Set
    const int fd = shm_open(name ? name : "/offbeat-fx", O_RDWR | O_CREAT, 0600);
    if (fd < 0)
        return nullptr;
    struct stat st {};
    if (fstat(fd, &st) == 0 && st.st_size < (off_t)sizeof(Shared))
        ftruncate(fd, sizeof(Shared));
    void* p = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED)
        return nullptr;
#endif
    if (!p)
        return nullptr;
    Shared* s = static_cast<Shared*>(p);
    uint32_t zero = 0;
    if (s->magic.compare_exchange_strong(zero, kMagic))
        s->state = State();
    return s->magic.load() == kMagic ? s : nullptr;
}

inline void write(Shared* s, const State& st)
{
    if (!s)
        return;
    const uint32_t q = s->seq.load(std::memory_order_relaxed);
    s->seq.store(q | 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(&s->state, &st, sizeof(State));
    std::atomic_thread_fence(std::memory_order_release);
    s->seq.store((q | 1) + 1, std::memory_order_release);
}

// Konsistente Kopie; false, wenn gerade geschrieben wird (dann den alten Zustand behalten).
inline bool read(const Shared* s, State& out)
{
    if (!s)
        return false;
    const uint32_t q = s->seq.load(std::memory_order_acquire);
    if (q & 1)
        return false;
    State tmp;
    std::memcpy(&tmp, &s->state, sizeof(State));
    std::atomic_thread_fence(std::memory_order_acquire);
    if (s->seq.load(std::memory_order_relaxed) != q)
        return false;
    out = tmp;
    return true;
}

// Gilt der Effekt für den Synth (Gearbox-Kennung linkId, Spur track)?
inline bool targets(const State& st, uint32_t linkId, uint32_t track)
{
    if (!linkId || track > 7)
        return false;
    for (int d = 0; d < 2; ++d)
        if (st.deckLink[d] == linkId)
            return (st.deckMask >> d & 1) && (st.trackMask[d] >> track & 1);
    return false;
}

} // namespace offbeat::fx
