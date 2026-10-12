# einfachstoppen Tor

Eine Lichtschranke für [einfachstoppen.de](https://einfachstoppen.de): zwei ESP32-C3, ein
Infrarot-Strahl, und ein Zeitstempel auf den Bruchteil einer Sekunde, den das Handy per
Bluetooth abholt. Ein Bauvorschlag zum Nachbauen, **kein fertiges Produkt**.

![Übersicht](docs/bilder/uebersicht.svg)

Das Tor ersetzt keine Stoppuhr-Handys. Es ist eine genauere Quelle für Start oder Ziel, so
wie die Kamera-App: Der Server behandelt seine Zeiten als Durchgänge, bindet sie an einen
Tipp, wenn es eindeutig ist, und lässt sonst einen Menschen zuordnen. Das Tor weiß nicht,
*wer* durchlief.

## Stand 0.1.0 (Entwurf)

| | |
| --- | --- |
| Firmware für Tor, Start-Tor und Sender | kompiliert für den ESP32-C3 |
| Kernlogik (Strahl-Abriss, Totzeit, Puffer, Pakete) | getestet, per Mutation geprüft (`tools/test.sh`) |
| Bluetooth-Protokoll | beschrieben, **auf keinem Gerät probiert** |
| Schaltung, Teileliste, Gehäuse | nach Datenblättern und Rechnung, **nichts davon gebaut oder gedruckt** |
| Anbindung in der Kamera-App | gebaut (Zweig `tor-bluetooth` der Kamera-App, Release 0.3.0), **am Gerät nicht geprüft** |
| Genauigkeit der Zeit über Bluetooth | **ungemessen** ([messung.md](docs/messung.md)) |

Ohne die App kann man ein Tor aufbauen und im seriellen Monitor beobachten, aber keinen Lauf
damit stoppen.

## Wie es funktioniert

Der Sender schickt 38-kHz-Bursts (600 µs an, 600 µs aus). Ein TSOP-Empfänger meldet jeden
Burst; bei freiem Strahl zuckt sein Ausgang rund 830-mal je Sekunde. Läuft jemand durch,
bleiben die Impulse aus. Das Tor stempelt diesen Abriss mit seiner eigenen Uhr, hält ihn im
Speicher, bis das Handy ihn bestätigt, und funkt ihn dorthin.

![Signal](docs/bilder/signal.svg)

Warum Bursts und nicht ein Dauerstrahl, und warum der Abriss und nicht eine Flanke: siehe
[docs/architektur.md](docs/architektur.md).

## Dokumente

| Dokument | Inhalt |
| --- | --- |
| [docs/architektur.md](docs/architektur.md) | Bausteine, Weg eines Durchgangs, Entscheidungen mit Begründung, Fehlerfälle, was getestet ist |
| [docs/protokoll.md](docs/protokoll.md) | Bluetooth-Dienst, Pakete, Uhrenabgleich, Weg zum Server |
| [docs/aufbau.md](docs/aufbau.md) | Schritt für Schritt vom Schreibtischtest bis zur Strecke |
| [docs/teileliste.md](docs/teileliste.md) | Teile, Mengen, Hinweise |
| [docs/amazon.md](docs/amazon.md) | Einkaufsliste mit Suchlinks und den Regeln für Partnerlinks |
| [docs/messung.md](docs/messung.md) | drei Versuche, die zeigen, wie genau es wirklich ist |
| [gehaeuse/tor.scad](gehaeuse/tor.scad) | parametrisches Gehäuse in OpenSCAD (nicht gedruckt) |

## Bauen und aufspielen

```
python3 -m venv .venv && .venv/bin/pip install platformio
.venv/bin/pio run -e schreibtisch -t upload   # Test ohne Sensor: BOOT-Taster = Durchgang
.venv/bin/pio run -e tor -t upload            # Empfänger an GPIO 4, meldet Ziele
.venv/bin/pio run -e tor_start -t upload      # dasselbe, meldet Starts
.venv/bin/pio run -e sender -t upload         # Gegenstelle, Bursts auf GPIO 5
.venv/bin/pio device monitor
```

Im Monitor erscheint bei jedem Durchgang `Durchgang bei … us`. Die Kernlogik testet man ohne
Hardware mit `tools/test.sh` (ein C++-Compiler genügt).

## Sicherheit

Das Tor trägt keine Zugangsdaten und kein Netz. Bluetooth ist **ohne Kopplung** offen: Wer in
Funkreichweite ist, kann Einstellung und Bestätigung schreiben und Zeiten mitlesen. Das
gehört vor dem Veröffentlichen entschieden, Einzelheiten in der
[Architektur](docs/architektur.md#sicherheit).

## Lizenz

Noch nicht gewählt, deshalb ist das Repo privat. Vor dem Veröffentlichen eine festlegen
(Firmware z. B. MIT oder Apache-2.0, Aufbau und Zeichnungen z. B. CERN-OHL oder CC BY-SA).
