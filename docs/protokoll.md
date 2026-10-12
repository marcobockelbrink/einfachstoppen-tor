# Protokoll: Tor ↔ Handy (Bluetooth LE)

Das Tor ist ein GATT-Server. Es wirbt mit dem Dienst unten und dem Namen `Tor-XXXX`
(XXXX = die letzten 16 Bit der Geräteadresse, damit man zwei Tore an einer Strecke
auseinanderhält). Alle Zahlen sind **little endian**. Die Codierung steht in `src/kern.h`
und ist in `tests/test_kern.cpp` gegen feste Bytes geprüft.

| Merkmal | UUID | Rechte | Inhalt |
| --- | --- | --- | --- |
| Dienst | `6d1f0001-1c7e-4b6a-9a3e-0e5f7d2a4c11` | | |
| Ereignis | `6d1f0002-…` | notify | ein Durchgang |
| Sync | `6d1f0003-…` | write, notify | Uhrenabstimmung |
| Bestätigt | `6d1f0004-…` | write | „bis hierhin angekommen“ |
| Einstellung | `6d1f0005-…` | read, write | Art und Totzeit |
| Info | `6d1f0006-…` | read | Text, z. B. `einfachstoppen-tor 0.1.0` |

(Der Rest der UUIDs ist überall `-1c7e-4b6a-9a3e-0e5f7d2a4c11`.)

## Ereignis (17 Byte, notify)

| Byte | Feld | |
| --- | --- | --- |
| 0–3 | `boot` u32 | Zufallszahl, neu bei jedem Start des Tors |
| 4–7 | `seq` u32 | zählt ab 1 hoch, nie doppelt innerhalb eines `boot` |
| 8–15 | `t_us` u64 | Zeit der Tor-Uhr in µs seit dem Start |
| 16 | `art` u8 | 0 = Ziel, 1 = Start |

Der Zeitstempel entsteht im Interrupt auf der Flanke, nicht in der Schleife und nicht
beim Senden. **Nie die Ankunftszeit des Pakets am Handy nehmen**, nur `t_us`. Das ist
dieselbe Regel wie bei den Taps im Server: eine Zeit, die offline oder verspätet
ankommt, behält ihre wahre Zeit.

## Bestätigung (4 Byte, write)

`seq` u32: alles bis einschließlich `seq` ist angekommen und darf aus dem Puffer.
Das Tor hält bis zu 64 unbestätigte Durchgänge. Läuft der Puffer voll, fällt der
**älteste** weg (die Firmware zählt das). Nach dem Verbinden und dem Abonnieren des
Ereignisses geht alles Unbestätigte erneut raus; bleibt die Bestätigung 2 s aus,
ebenfalls. **Dieselbe Meldung kommt also mehrfach vor**, das Handy erkennt sie an
`boot` + `seq`.

## Uhrenabstimmung (Sync)

Handy schreibt `nonce` u32 (4 Byte). Das Tor antwortet per notify mit 16 Byte:

| Byte | Feld | |
| --- | --- | --- |
| 0–3 | `nonce` u32 | wie geschickt |
| 4–7 | `boot` u32 | |
| 8–15 | `t_us` u64 | Tor-Uhr beim Eintreffen der Anfrage (erste Zeile des Callbacks) |

Das Handy misst den Hin-und-Rück-Weg (`t0` beim Schreiben, `t1` bei der Antwort) und
schätzt `versatz = (t0 + t1) / 2 − t_us`. **Nicht den Median nehmen wie bei `/api/time`**,
sondern von etwa 20 Proben die mit der **kleinsten Laufzeit**: Bluetooth verteilt die
Zeit ungleich auf Hin- und Rückweg (jede Richtung wartet bis zum nächsten
Verbindungsereignis), und genau die Ausreißer sind die großen Laufzeiten. Das Tor
verlangt beim Verbinden 7,5–15 ms Intervall; iOS gibt unter 15 ms nichts her.

> **Ungemessen.** Wie genau das auf echten Geräten ist, steht in `docs/messung.md` als
> Versuch, nicht als Ergebnis. Bis dahin gilt kein Zahlenversprechen. Ein Tor, das auf
> ±15 ms genau ist, ist für Läufe im Sekundenbereich gut und für Hundertstel nicht.

Ein neues `boot` (Tor wurde neu gestartet) heißt: der Versatz gilt nicht mehr, neu
abstimmen. Die Uhr des Tors läuft auf Quarz und driftet in Stunden um Millisekunden;
vor dem Rennen und dann alle paar Minuten neu abstimmen ist billig.

## Einstellung (3 Byte, read/write)

| Byte | Feld | |
| --- | --- | --- |
| 0 | `art` u8 | 0 Ziel, 1 Start. **Wird nicht gespeichert**; das Handy setzt sie je Sitzung. |
| 1–2 | `totzeit_ms` u16 | 20 … 5000, Standard 300. Wird gespeichert. |

Liegt ein Wert außerhalb, übernimmt das Tor **gar nichts** und zeigt beim nächsten
Lesen, was gilt.

## Vom Durchgang zum Server

Die App rechnet `client_ms = t_us / 1000 + versatz` (in der Uhr, die sie ohnehin für
Taps nutzt, also schon gegen `/api/time` korrigiert) und meldet:

```
POST /api/races/{code}/kamera
{ "client_id": "tor-A1B2-3f9c01d2-17", "client_ms": 1760259123456.7, "art": "ziel" }
```

`client_id` = `tor-<Namensteil>-<boot als 8 Hex>-<seq>`. Der Server ist über
`client_id` idempotent: dasselbe Nachsenden ändert nichts. Danach läuft es wie bei der
Kamera: `kamera.paaren()` bindet den Durchgang an einen Tipp, sonst bleibt er offen
und ein Mensch ordnet zu. Das Tor ersetzt also nie einen Tipp, es verfeinert ihn.
Ein zweites Tor am Start meldet `"art": "start"`.
