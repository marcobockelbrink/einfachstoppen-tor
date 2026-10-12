// Sender der Lichtschranke: 38 kHz Traeger in Bursts, 600 us an, 600 us aus.
//
// Ein TSOP-Empfaenger schaltet bei Dauerlicht ab (AGC, Vishay-Datenblatt 82459), er will
// Bursts sehen. Das Datenblatt prueft mit genau diesem Muster. Der Pin treibt ueber einen
// Transistor eine IR-LED, nie die LED direkt (docs/aufbau.md).
#include <Arduino.h>
#include <esp_timer.h>

#ifndef SENDER_PIN
#define SENDER_PIN 5
#endif

constexpr uint32_t TRAEGER_HZ = 38000;
constexpr uint8_t KANAL = 0;
constexpr uint8_t AUFLOESUNG = 8;  // Bit
constexpr uint32_t BURST_US = 600;

static volatile bool an = false;

static void umschalten(void*) {
  an = !an;
  ledcWrite(KANAL, an ? 1 << (AUFLOESUNG - 1) : 0);  // 50 % Tastverhaeltnis oder aus
}

void setup() {
  Serial.begin(115200);
  uint32_t echt = ledcSetup(KANAL, TRAEGER_HZ, AUFLOESUNG);
  ledcAttachPin(SENDER_PIN, KANAL);
  esp_timer_handle_t uhr;
  esp_timer_create_args_t args = {};
  args.callback = umschalten;
  args.name = "burst";
  esp_timer_create(&args, &uhr);
  esp_timer_start_periodic(uhr, BURST_US);
  Serial.printf("Sender: GPIO %d, Traeger %u Hz, Bursts %u us\n", SENDER_PIN, (unsigned)echt,
                (unsigned)BURST_US);
}

void loop() { delay(1000); }
