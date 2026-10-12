// SPDX-License-Identifier: MIT
// Kernlogik des Tors: ohne Arduino, ohne BLE, damit sie auf dem Rechner getestet wird.
// Die Zeit ist immer die des Geraets in Mikrosekunden seit dem Start (esp_timer_get_time).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace tor {

constexpr uint8_t ART_ZIEL = 0;
constexpr uint8_t ART_START = 1;

struct Ereignis {
  uint32_t seq;
  uint64_t t_us;
  uint8_t art;
};

// Ein Lichtstrahl, der unterbrochen wird, flattert: ein Fuss, ein Arm, ein Rad geben
// mehrere Flanken. Nach einer angenommenen Flanke gilt die Totzeit; die ERSTE zaehlt,
// nicht die letzte -- die Zeit des Durchgangs ist der Moment, in dem der Strahl bricht.
class Totzeit {
 public:
  explicit Totzeit(uint32_t ms) : us_((uint64_t)ms * 1000) {}
  void setze(uint32_t ms) { us_ = (uint64_t)ms * 1000; }
  bool annehmen(uint64_t t_us) {
    if (hat_letzte_ && t_us >= letzte_ && t_us - letzte_ < us_) return false;
    hat_letzte_ = true;
    letzte_ = t_us;
    return true;
  }

 private:
  uint64_t us_;
  uint64_t letzte_ = 0;
  bool hat_letzte_ = false;
};

// Ein TSOP-Empfaenger (Vishay, Datenblatt 82459) unterdrueckt Dauerlicht per AGC. Der
// Sender schickt deshalb Bursts (600 us an, 600 us aus), und der Ausgang ist active low:
// bei FREIEM Strahl zuckt er etwa 830-mal je Sekunde, bei unterbrochenem bleibt er HIGH.
// Eine Flanke ist hier also kein Ereignis. Ereignis ist, dass die Impulse AUFHOEREN.
//
// Die Zeit des Durchgangs liegt zwischen dem letzten gesehenen Impuls und dem naechsten,
// der nicht mehr kam: wir nehmen die Mitte. Der Fehler ist hoechstens eine halbe Periode.
// "Scharf" wird das Tor erst durch den ersten Impuls -- ein Tor ohne Sender meldet nichts.
class Strahl {
 public:
  Strahl(uint32_t halte_us = 4000, uint32_t periode_us = 1200)
      : halte_us_(halte_us), periode_us_(periode_us) {}
  void impuls(uint64_t t_us) {
    letzte_ = t_us;
    scharf_ = true;
  }
  // Mit der Geraeteuhr aufrufen. true genau einmal je Abriss; *t_us = geschaetzte Zeit.
  bool pruefe(uint64_t jetzt_us, uint64_t* t_us) {
    if (!scharf_ || jetzt_us < letzte_ || jetzt_us - letzte_ <= halte_us_) return false;
    scharf_ = false;
    *t_us = letzte_ + periode_us_ / 2;
    return true;
  }

 private:
  uint32_t halte_us_;
  uint32_t periode_us_;
  uint64_t letzte_ = 0;
  bool scharf_ = false;
};

// Ein Kontakt am Startstab: ein Reedschalter, zu solange der Magnet an der Stange nah ist.
// Das Oeffnen, wenn die Stange aufgedrueckt wird, ist der Start.
//
// Ein Flankenzaehler reicht dafuer nicht. Die Stange federt zurueck und prellt, ein Windstoss
// oder eine Hand bewegt sie, ein Kabel reisst ab -- jedes davon ergaebe einen Start. Darum
// zaehlt das Oeffnen nur, wenn der Kontakt vorher mindestens scharf_us am Stueck zu war
// ("scharf", wie beim Strahl: ein Stab ohne geschlossenen Kontakt meldet nichts). Die Zeit
// ist die ERSTE Oeffnungsflanke, nicht die letzte.
//
// flanke() kommt mit dem Zeitstempel aus dem Interrupt. pegel() liest der Hauptlauf mit dem
// Pegel des Pins nach: ging eine Flanke "zu" verloren (Warteschlange voll), wird sie hier
// nachgeholt -- sonst bliebe der Stab fuer immer unscharf und still. pegel() meldet nie einen
// Start: eine Zeit, die der Interrupt nicht gestempelt hat, waere geraten.
class Kontakt {
 public:
  explicit Kontakt(uint32_t scharf_ms = 100) : scharf_us_((uint64_t)scharf_ms * 1000) {}
  void setze(uint32_t scharf_ms) { scharf_us_ = (uint64_t)scharf_ms * 1000; }
  // offen = true: der Magnet ist weg. true genau dann, wenn dies ein Start ist.
  bool flanke(uint64_t t_us, bool offen, uint64_t* start_us) {
    if (!offen) {
      if (!zu_) {
        zu_ = true;
        zu_seit_ = t_us;
      }
      return false;
    }
    if (!zu_) return false;
    zu_ = false;
    if (t_us < zu_seit_ || t_us - zu_seit_ < scharf_us_) return false;
    *start_us = t_us;
    return true;
  }
  // Nur "zu" wird nachgeholt. Ein gelesenes "offen" aendert nichts: Die Flanke dazu kann im
  // Interrupt schon gestempelt sein und in der Warteschlange liegen, und wer hier vorgreift,
  // wirft damit den Start weg, den sie gleich meldet.
  void pegel(uint64_t jetzt_us, bool offen) {
    if (offen || zu_) return;
    zu_ = true;
    zu_seit_ = jetzt_us;
  }

 private:
  uint64_t scharf_us_;
  uint64_t zu_seit_ = 0;
  bool zu_ = false;
};

// Durchgaenge warten, bis das Handy sie bestaetigt hat. Eine abgebrochene Verbindung
// darf keine Zeit kosten: beim naechsten Verbinden gehen alle unbestaetigten nochmal raus.
// Laeuft der Puffer voll, faellt der AELTESTE weg und wird gezaehlt -- die neuesten
// Zeiten sind die, auf die gerade jemand wartet.
template <size_t N>
class Puffer {
 public:
  void lege(uint64_t t_us, uint8_t art) {
    if (anzahl_ == N) {
      kopf_ = (kopf_ + 1) % N;
      --anzahl_;
      ++verloren_;
    }
    daten_[(kopf_ + anzahl_) % N] = Ereignis{naechste_seq_++, t_us, art};
    ++anzahl_;
  }
  // Alles bis einschliesslich seq ist angekommen.
  void bestaetige(uint32_t seq) {
    while (anzahl_ > 0 && daten_[kopf_].seq <= seq) {
      kopf_ = (kopf_ + 1) % N;
      --anzahl_;
    }
  }
  size_t anzahl() const { return anzahl_; }
  // Die gehaltenen Nummern sind immer lueckenlos [aelteste(), naechste()): Das Handy liest
  // diesen Bereich, bevor es bestaetigt, und bestaetigt nie etwas, das es nicht gesehen hat.
  uint32_t aelteste() const { return anzahl_ > 0 ? daten_[kopf_].seq : naechste_seq_; }
  uint32_t naechste() const { return naechste_seq_; }
  uint32_t verloren() const { return verloren_; }
  const Ereignis& an(size_t i) const { return daten_[(kopf_ + i) % N]; }

 private:
  Ereignis daten_[N];
  size_t kopf_ = 0;
  size_t anzahl_ = 0;
  uint32_t naechste_seq_ = 1;
  uint32_t verloren_ = 0;
};

// --- Pakete (little endian). Siehe docs/protokoll.md ---

inline void schreibe32(uint8_t* p, uint32_t v) {
  for (int i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
inline void schreibe64(uint8_t* p, uint64_t v) {
  for (int i = 0; i < 8; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
inline uint32_t lies32(const uint8_t* p) {
  uint32_t v = 0;
  for (int i = 3; i >= 0; --i) v = (v << 8) | p[i];
  return v;
}

constexpr size_t EREIGNIS_LEN = 17;  // boot u32, seq u32, t_us u64, art u8
constexpr size_t SYNC_LEN = 16;      // nonce u32, boot u32, t_us u64
constexpr size_t STAND_LEN = 12;     // boot u32, aelteste u32, naechste u32

inline void kodiere_ereignis(uint8_t* out, uint32_t boot, const Ereignis& e) {
  schreibe32(out, boot);
  schreibe32(out + 4, e.seq);
  schreibe64(out + 8, e.t_us);
  out[16] = e.art;
}

inline void kodiere_sync(uint8_t* out, uint32_t nonce, uint32_t boot, uint64_t t_us) {
  schreibe32(out, nonce);
  schreibe32(out + 4, boot);
  schreibe64(out + 8, t_us);
}

inline void kodiere_stand(uint8_t* out, uint32_t boot, uint32_t aelteste, uint32_t naechste) {
  schreibe32(out, boot);
  schreibe32(out + 4, aelteste);
  schreibe32(out + 8, naechste);
}

}  // namespace tor
