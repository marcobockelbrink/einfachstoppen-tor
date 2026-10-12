// Native Tests der Kernlogik: tools/test.sh
#include <cstdio>
#include <cstdlib>

#include "../src/kern.h"

static int fehler = 0;
#define PRUEFE(bed)                                                  \
  do {                                                               \
    if (!(bed)) {                                                    \
      std::printf("FEHLER %s:%d  %s\n", __FILE__, __LINE__, #bed);   \
      ++::fehler;                                                    \
    }                                                                \
  } while (0)

static void totzeit_zaehlt_die_erste_flanke() {
  tor::Totzeit t(300);
  PRUEFE(t.annehmen(1000000));            // erste Flanke gilt
  PRUEFE(!t.annehmen(1100000));           // 100 ms spaeter: Flattern
  PRUEFE(!t.annehmen(1299999));           // knapp innerhalb
  PRUEFE(t.annehmen(1300000));            // genau nach 300 ms: neuer Durchgang
  // Die Totzeit haengt an der letzten ANGENOMMENEN Flanke, nicht an der letzten gesehenen:
  // sonst haelt ein flatternder Strahl das Tor fuer immer zu.
  PRUEFE(!t.annehmen(1500000));
  PRUEFE(!t.annehmen(1590000));
  PRUEFE(t.annehmen(1600000));
}

static void totzeit_kann_geaendert_werden() {
  tor::Totzeit t(300);
  PRUEFE(t.annehmen(0));
  t.setze(50);
  PRUEFE(t.annehmen(60000));
}

static void puffer_haelt_bis_zur_bestaetigung() {
  tor::Puffer<4> p;
  p.lege(10, tor::ART_ZIEL);
  p.lege(20, tor::ART_ZIEL);
  p.lege(30, tor::ART_START);
  PRUEFE(p.anzahl() == 3);
  PRUEFE(p.an(0).seq == 1 && p.an(0).t_us == 10);
  PRUEFE(p.an(2).art == tor::ART_START);
  p.bestaetige(2);  // bis einschliesslich 2
  PRUEFE(p.anzahl() == 1);
  PRUEFE(p.an(0).seq == 3);
  p.bestaetige(2);  // doppelt bestaetigt: nichts passiert
  PRUEFE(p.anzahl() == 1);
  p.bestaetige(99);
  PRUEFE(p.anzahl() == 0);
}

static void puffer_verliert_den_aeltesten() {
  tor::Puffer<3> p;
  for (uint64_t i = 1; i <= 5; ++i) p.lege(i * 100, tor::ART_ZIEL);
  PRUEFE(p.anzahl() == 3);
  PRUEFE(p.verloren() == 2);
  PRUEFE(p.an(0).seq == 3);  // 1 und 2 sind weg
  PRUEFE(p.an(2).seq == 5);
  PRUEFE(p.an(2).t_us == 500);
}

static void seq_laeuft_ueber_bestaetigungen_weiter() {
  tor::Puffer<4> p;
  p.lege(1, tor::ART_ZIEL);
  p.bestaetige(1);
  p.lege(2, tor::ART_ZIEL);
  PRUEFE(p.an(0).seq == 2);  // nie dieselbe Nummer zweimal: sie ist Teil der Kennung am Server
}

static void ereignis_paket_ist_little_endian() {
  uint8_t b[tor::EREIGNIS_LEN];
  tor::Ereignis e{0x01020304u, 0x1112131415161718ull, tor::ART_START};
  tor::kodiere_ereignis(b, 0xAABBCCDDu, e);
  PRUEFE(b[0] == 0xDD && b[3] == 0xAA);    // boot
  PRUEFE(b[4] == 0x04 && b[7] == 0x01);    // seq
  PRUEFE(b[8] == 0x18 && b[15] == 0x11);   // t_us
  PRUEFE(b[16] == 1);                      // art
  PRUEFE(tor::lies32(b) == 0xAABBCCDDu);
}

static void sync_paket_traegt_nonce_boot_und_zeit() {
  uint8_t b[tor::SYNC_LEN];
  tor::kodiere_sync(b, 7, 9, 5000000000ull);  // ueber 2^32: 64 Bit muessen halten
  PRUEFE(tor::lies32(b) == 7);
  PRUEFE(tor::lies32(b + 4) == 9);
  uint64_t t = 0;
  for (int i = 7; i >= 0; --i) t = (t << 8) | b[8 + i];
  PRUEFE(t == 5000000000ull);
}

// --- Simulation eines TSOP-Impulszugs: frei = ein Impuls alle 1200 us, unterbrochen = keiner ---

struct Lauf {
  tor::Strahl s;
  int abrisse = 0;
  uint64_t erster = 0;
  uint64_t naechster_impuls = 0;  // Takt des Senders, laeuft ueber die Aufrufe hinweg weiter
  // Die Schleife des Geraets schaut alle 2 ms nach; Impulse kommen, solange der Strahl frei
  // ist. Der Interrupt stempelt jeden Impuls mit seiner echten Zeit, auch wenn die Schleife
  // ihn erst spaeter sieht -- darum werden alle Impulse BIS bis_us eingetragen.
  void spiele(uint64_t von_us, uint64_t bis_us, bool frei) {
    if (naechster_impuls < von_us) naechster_impuls = von_us;
    for (uint64_t t = von_us; t < bis_us; t += 2000) {
      for (; naechster_impuls <= t; naechster_impuls += 1200)
        if (frei) s.impuls(naechster_impuls);
      uint64_t ab = 0;
      if (s.pruefe(t, &ab)) {
        if (abrisse++ == 0) erster = ab;
      }
    }
    for (; naechster_impuls < bis_us; naechster_impuls += 1200)
      if (frei) s.impuls(naechster_impuls);
  }
};

static void freier_strahl_meldet_nie() {
  Lauf l;
  l.spiele(0, 10000000, true);  // 10 s frei, ~8300 Impulse: kein einziger Durchgang
  PRUEFE(l.abrisse == 0);
}

static void tor_ohne_sender_meldet_nichts() {
  Lauf l;
  l.spiele(0, 10000000, false);  // nie ein Impuls: nie scharf
  PRUEFE(l.abrisse == 0);
}

static void abriss_wird_einmal_und_auf_eine_halbe_periode_genau_gemeldet() {
  Lauf l;
  l.spiele(0, 1000000, true);
  uint64_t echt = 1000000;  // der Strahl bricht hier
  l.spiele(1000000, 1100000, false);
  PRUEFE(l.abrisse == 1);
  uint64_t abweichung = l.erster > echt ? l.erster - echt : echt - l.erster;
  PRUEFE(abweichung <= 600);
  l.spiele(1100000, 2000000, false);  // bleibt unterbrochen: kein zweiter
  PRUEFE(l.abrisse == 1);
}

static void nach_dem_wiederkehren_zaehlt_der_naechste_abriss() {
  Lauf l;
  l.spiele(0, 500000, true);
  l.spiele(500000, 600000, false);
  l.spiele(600000, 1200000, true);
  l.spiele(1200000, 1300000, false);
  PRUEFE(l.abrisse == 2);
}

static void ein_ausgelassener_impuls_ist_noch_kein_abriss() {
  tor::Strahl s;
  uint64_t ab = 0;
  s.impuls(0);
  s.impuls(1200);
  // Ein Impuls fehlt (Stoerung): 3600 us seit dem letzten liegen unter der Haltezeit von 4 ms.
  PRUEFE(!s.pruefe(1200 + 3600, &ab));
  PRUEFE(s.pruefe(1200 + 4001, &ab));
}

static void stand_beschreibt_den_lueckenlosen_bereich() {
  tor::Puffer<4> p;
  PRUEFE(p.aelteste() == 1 && p.naechste() == 1);  // leer: nichts liegt dort
  p.lege(10, tor::ART_ZIEL);
  p.lege(20, tor::ART_ZIEL);
  p.lege(30, tor::ART_ZIEL);
  PRUEFE(p.aelteste() == 1 && p.naechste() == 4);
  p.bestaetige(2);
  PRUEFE(p.aelteste() == 3 && p.naechste() == 4);
  p.bestaetige(3);
  PRUEFE(p.aelteste() == 4 && p.naechste() == 4);  // wieder leer, naechste bleibt
  for (int i = 0; i < 6; ++i) p.lege(i, tor::ART_ZIEL);  // Platz fuer 4: die zwei aeltesten fallen weg
  PRUEFE(p.aelteste() == 6 && p.naechste() == 10);
}

static void stand_paket_ist_little_endian() {
  uint8_t b[tor::STAND_LEN];
  tor::kodiere_stand(b, 0xAABBCCDDu, 3, 9);
  PRUEFE(b[0] == 0xDD && b[1] == 0xCC && b[2] == 0xBB && b[3] == 0xAA);  // boot
  PRUEFE(tor::lies32(b + 4) == 3);                                       // aelteste
  PRUEFE(tor::lies32(b + 8) == 9);                                       // naechste
}

int main() {
  stand_beschreibt_den_lueckenlosen_bereich();
  stand_paket_ist_little_endian();
  freier_strahl_meldet_nie();
  tor_ohne_sender_meldet_nichts();
  abriss_wird_einmal_und_auf_eine_halbe_periode_genau_gemeldet();
  nach_dem_wiederkehren_zaehlt_der_naechste_abriss();
  ein_ausgelassener_impuls_ist_noch_kein_abriss();
  totzeit_zaehlt_die_erste_flanke();
  totzeit_kann_geaendert_werden();
  puffer_haelt_bis_zur_bestaetigung();
  puffer_verliert_den_aeltesten();
  seq_laeuft_ueber_bestaetigungen_weiter();
  ereignis_paket_ist_little_endian();
  sync_paket_traegt_nonce_boot_und_zeit();
  if (fehler) {
    std::printf("%d Fehler\n", fehler);
    return 1;
  }
  std::printf("alle Tests gruen\n");
  return 0;
}
