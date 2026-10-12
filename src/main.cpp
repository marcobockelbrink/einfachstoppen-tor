// einfachstoppen Tor: ein Sensor, ein Zeitstempel, Bluetooth zum Handy.
//
// Das Tor beurteilt nichts. Es sagt "jetzt ist etwas durch" und nennt die Zeit seiner
// eigenen Uhr; das Handy rechnet sie um und meldet sie an den Server, der sie wie einen
// Kamera-Durchgang behandelt. Protokoll: docs/protokoll.md
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <esp_timer.h>

#include "kern.h"

// TOR_MODUS 0: jede Flanke ist ein Durchgang (Taster, Schalter, Reedkontakt).
// TOR_MODUS 1: Impulszug eines TSOP-Empfaengers; Durchgang = die Impulse hoeren auf.
#if !defined(TOR_PIN) || !defined(TOR_FLANKE) || !defined(TOR_PULLUP) || !defined(TOR_ART) || \
    !defined(TOR_MODUS)
#error "Sensor-Einstellungen fehlen -- eine Umgebung aus platformio.ini waehlen"
#endif

static const char* UUID_DIENST = "6d1f0001-1c7e-4b6a-9a3e-0e5f7d2a4c11";
static const char* UUID_EREIGNIS = "6d1f0002-1c7e-4b6a-9a3e-0e5f7d2a4c11";     // notify
static const char* UUID_SYNC = "6d1f0003-1c7e-4b6a-9a3e-0e5f7d2a4c11";         // write + notify
static const char* UUID_BESTAETIGT = "6d1f0004-1c7e-4b6a-9a3e-0e5f7d2a4c11";   // write
static const char* UUID_EINSTELLUNG = "6d1f0005-1c7e-4b6a-9a3e-0e5f7d2a4c11";  // read + write
static const char* UUID_INFO = "6d1f0006-1c7e-4b6a-9a3e-0e5f7d2a4c11";         // read

constexpr uint16_t TOTZEIT_STANDARD_MS = 300;
constexpr uint16_t TOTZEIT_MIN_MS = 20;
constexpr uint16_t TOTZEIT_MAX_MS = 5000;
constexpr uint32_t WIEDERHOLUNG_MS = 2000;  // Unbestaetigtes geht nach so langer Stille nochmal raus

static uint32_t bootId;  // neu bei jedem Start: danach gilt eine andere Zeitbasis
static tor::Totzeit totzeit(TOTZEIT_STANDARD_MS);
static uint16_t totzeitMs = TOTZEIT_STANDARD_MS;
static tor::Puffer<64> puffer;
// Die Art wird NICHT gespeichert. Ein Tor, das nach dem Neustart "Start" meldet, weil es
// am Vortag dort stand, taete am Ziel still das Falsche. Das Handy setzt sie je Sitzung.
static uint8_t art = TOR_ART;
static uint32_t gesendetBis = 0;  // hoechste seq, die auf DIESER Verbindung schon raus ist
static volatile bool verbunden = false;
static volatile bool abonniert = false;
static SemaphoreHandle_t sperre;  // Puffer, Totzeit, art, gesendetBis: loop() und die BLE-Aufgabe greifen zu
static QueueHandle_t flanken;     // Zeitstempel aus dem Interrupt
static Preferences prefs;

static NimBLECharacteristic *chEreignis, *chSync, *chEinstellung;

// Der Zeitstempel entsteht im Interrupt, nicht in loop(): was zwischen Flanke und
// Zeitnehmen liegt, ist Fehler in der Messung.
static void IRAM_ATTR beiFlanke() {
  uint64_t t = (uint64_t)esp_timer_get_time();
  BaseType_t weck = pdFALSE;
  xQueueSendFromISR(flanken, &t, &weck);
  if (weck) portYIELD_FROM_ISR();
}

#if TOR_MODUS == 1
// Impulszug (TSOP-Empfaenger): jeder Impuls schreibt nur "zuletzt gesehen". Ob der Strahl
// abgerissen ist, entscheidet loop() mit tor::Strahl -- 830 Interrupts je Sekunde duerfen
// weder eine Warteschlange fuellen noch als Durchgaenge gelten.
static portMUX_TYPE impulsSperre = portMUX_INITIALIZER_UNLOCKED;
static volatile uint64_t letzterImpuls = 0;  // 64 Bit sind auf dem C3 nicht atomar: nur unter der Sperre
static tor::Strahl strahl;

static void IRAM_ATTR beiImpuls() {
  uint64_t t = (uint64_t)esp_timer_get_time();
  portENTER_CRITICAL_ISR(&impulsSperre);
  letzterImpuls = t;
  portEXIT_CRITICAL_ISR(&impulsSperre);
}
#endif

static void nimmAuf(uint64_t t) {
  xSemaphoreTake(sperre, portMAX_DELAY);
  bool gilt = totzeit.annehmen(t);
  if (gilt) puffer.lege(t, art);
  xSemaphoreGive(sperre);
  if (gilt) Serial.printf("Durchgang bei %llu us\n", (unsigned long long)t);
}

// Notify NIE unter der Sperre: die BLE-Aufgabe haelt ihre eigene Sperre und will in den
// Callbacks unsere -- zwei Sperren in verschiedener Reihenfolge ist ein Stillstand.
static void sendeOffene() {
  if (!verbunden || !abonniert) return;
  tor::Ereignis offen[64];
  size_t n = 0;
  xSemaphoreTake(sperre, portMAX_DELAY);
  for (size_t i = 0; i < puffer.anzahl(); ++i)
    if (puffer.an(i).seq > gesendetBis) offen[n++] = puffer.an(i);
  xSemaphoreGive(sperre);

  for (size_t i = 0; i < n; ++i) {
    uint8_t paket[tor::EREIGNIS_LEN];
    tor::kodiere_ereignis(paket, bootId, offen[i]);
    chEreignis->setValue(paket, sizeof paket);
    // notify() meldet in NimBLE-Arduino 1.4 keinen Fehler. Ob es ankam, weiss nur die
    // Bestaetigung vom Handy; bleibt sie aus, kommt die Wiederholung aus loop().
    chEreignis->notify();
    xSemaphoreTake(sperre, portMAX_DELAY);
    gesendetBis = offen[i].seq;
    xSemaphoreGive(sperre);
  }
}

class Verbindung : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, ble_gap_conn_desc* d) override {
    verbunden = true;
    // Kurze Intervalle (7,5-15 ms): die Uhrenabstimmung ist nur so genau wie ein Intervall.
    s->updateConnParams(d->conn_handle, 6, 12, 0, 200);
    Serial.println("verbunden");
  }
  void onDisconnect(NimBLEServer*) override {
    verbunden = false;
    abonniert = false;
    Serial.println("getrennt");
    NimBLEDevice::startAdvertising();
  }
};

class EreignisAbo : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic*, ble_gap_conn_desc*, uint16_t wert) override {
    xSemaphoreTake(sperre, portMAX_DELAY);
    abonniert = wert != 0;
    gesendetBis = 0;  // neue Verbindung: alles Unbestaetigte geht nochmal raus
    xSemaphoreGive(sperre);
  }
};

class SyncSchreiben : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, ble_gap_conn_desc*) override {
    // Allererste Zeile: die Zeit, zu der die Anfrage hier ankam.
    uint64_t t = (uint64_t)esp_timer_get_time();
    std::string v = c->getValue();
    if (v.size() < 4) return;
    uint8_t antwort[tor::SYNC_LEN];
    tor::kodiere_sync(antwort, tor::lies32((const uint8_t*)v.data()), bootId, t);
    chSync->setValue(antwort, sizeof antwort);
    chSync->notify();
  }
};

class BestaetigtSchreiben : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, ble_gap_conn_desc*) override {
    std::string v = c->getValue();
    if (v.size() < 4) return;
    xSemaphoreTake(sperre, portMAX_DELAY);
    puffer.bestaetige(tor::lies32((const uint8_t*)v.data()));
    xSemaphoreGive(sperre);
  }
};

static void setzeEinstellungWert() {
  uint8_t b[3] = {art, (uint8_t)(totzeitMs & 0xff), (uint8_t)(totzeitMs >> 8)};
  chEinstellung->setValue(b, sizeof b);
}

// art u8 (0 Ziel, 1 Start), totzeit_ms u16. Ausserhalb der Grenzen wird NICHTS uebernommen:
// ein Tippfehler am Handy darf das Tor nicht taub machen.
class EinstellungSchreiben : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, ble_gap_conn_desc*) override {
    std::string v = c->getValue();
    if (v.size() < 3) return;
    uint8_t neueArt = (uint8_t)v[0];
    uint16_t neueTotzeit = (uint16_t)((uint8_t)v[1] | ((uint8_t)v[2] << 8));
    if (neueArt > tor::ART_START || neueTotzeit < TOTZEIT_MIN_MS || neueTotzeit > TOTZEIT_MAX_MS) {
      setzeEinstellungWert();  // der Leser sieht, was wirklich gilt
      return;
    }
    xSemaphoreTake(sperre, portMAX_DELAY);
    art = neueArt;
    totzeitMs = neueTotzeit;
    totzeit.setze(neueTotzeit);
    xSemaphoreGive(sperre);
    prefs.putUShort("totzeit", neueTotzeit);
    Serial.printf("art=%u totzeit=%u ms\n", neueArt, neueTotzeit);
  }
};

void setup() {
  Serial.begin(115200);
  bootId = esp_random();
  sperre = xSemaphoreCreateMutex();
  flanken = xQueueCreate(32, sizeof(uint64_t));

  prefs.begin("tor", false);
  totzeitMs = prefs.getUShort("totzeit", TOTZEIT_STANDARD_MS);
  if (totzeitMs < TOTZEIT_MIN_MS || totzeitMs > TOTZEIT_MAX_MS) totzeitMs = TOTZEIT_STANDARD_MS;
  totzeit.setze(totzeitMs);

  char name[16];
  snprintf(name, sizeof name, "Tor-%04X", (unsigned)(ESP.getEfuseMac() & 0xffff));
  NimBLEDevice::init(name);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new Verbindung());
  NimBLEService* dienst = server->createService(UUID_DIENST);

  chEreignis = dienst->createCharacteristic(UUID_EREIGNIS, NIMBLE_PROPERTY::NOTIFY);
  chEreignis->setCallbacks(new EreignisAbo());
  chSync = dienst->createCharacteristic(UUID_SYNC, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  chSync->setCallbacks(new SyncSchreiben());
  dienst->createCharacteristic(UUID_BESTAETIGT, NIMBLE_PROPERTY::WRITE)
      ->setCallbacks(new BestaetigtSchreiben());
  chEinstellung =
      dienst->createCharacteristic(UUID_EINSTELLUNG, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  chEinstellung->setCallbacks(new EinstellungSchreiben());
  setzeEinstellungWert();
  dienst->createCharacteristic(UUID_INFO, NIMBLE_PROPERTY::READ)
      ->setValue(std::string("einfachstoppen-tor " TOR_VERSION));
  dienst->start();

  NimBLEAdvertising* werbung = NimBLEDevice::getAdvertising();
  werbung->addServiceUUID(UUID_DIENST);
  werbung->setScanResponse(true);
  werbung->start();

  pinMode(TOR_PIN, TOR_PULLUP ? INPUT_PULLUP : INPUT);
#if TOR_MODUS == 1
  attachInterrupt(digitalPinToInterrupt(TOR_PIN), beiImpuls, FALLING);  // active low: Impuls kam an
#else
  attachInterrupt(digitalPinToInterrupt(TOR_PIN), beiFlanke, TOR_FLANKE);
#endif
  Serial.printf("%s bereit, boot=%08x, art=%u\n", name, (unsigned)bootId, (unsigned)art);
}

void loop() {
  static uint32_t zuletztGesendet = 0;
#if TOR_MODUS == 1
  static uint64_t gesehen = 0;
  uint64_t letzter;
  portENTER_CRITICAL(&impulsSperre);
  letzter = letzterImpuls;
  portEXIT_CRITICAL(&impulsSperre);
  if (letzter != gesehen) {
    gesehen = letzter;
    strahl.impuls(letzter);
  }
  uint64_t abriss;
  if (strahl.pruefe((uint64_t)esp_timer_get_time(), &abriss)) nimmAuf(abriss);
#else
  uint64_t t;
  while (xQueueReceive(flanken, &t, 0) == pdTRUE) nimmAuf(t);
#endif

  // Ging eine Benachrichtigung auf dem Funkweg verloren, bleibt sie unbestaetigt im Puffer.
  // Nach einer Weile geht alles Unbestaetigte nochmal raus -- Handy und Server erkennen
  // Wiederholungen an boot + seq und zaehlen sie nicht doppelt.
  uint32_t jetzt = millis();
  xSemaphoreTake(sperre, portMAX_DELAY);
  if (puffer.anzahl() > 0 && jetzt - zuletztGesendet >= WIEDERHOLUNG_MS) {
    gesendetBis = 0;
    zuletztGesendet = jetzt;
  }
  xSemaphoreGive(sperre);

  sendeOffene();
  delay(2);
}
