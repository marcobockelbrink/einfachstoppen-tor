# einfachstoppen Tor

Eine Lichtschranke und ein Startstab für [einfachstoppen.de](https://einfachstoppen.de):
ESP32-C3, ein Sensor, und ein Zeitstempel auf den Bruchteil einer Sekunde, den das Handy per
Bluetooth abholt. Die **Lichtschranke** (zwei ESP32-C3, ein Infrarot-Strahl) taugt für Start
und Ziel, der **Startstab** (Stange mit Magnet und Reedkontakt, wie beim Skirennen) für den
Start. Ein Bauvorschlag zum Nachbauen, **kein fertiges Produkt**.

![Übersicht](docs/bilder/uebersicht.svg)

Das Tor ersetzt keine Stoppuhr-Handys. Es ist eine genauere Quelle für Start oder Ziel, so
wie die Kamera-App: Der Server behandelt seine Zeiten als Durchgänge, bindet sie an einen
Tipp, wenn es eindeutig ist, und lässt sonst einen Menschen zuordnen. Das Tor weiß nicht,
*wer* durchlief.

## Stand 0.1.0 (Entwurf)

| | |
| --- | --- |
| Firmware für Tor, Start-Tor, Sender und Startstab | kompiliert für den ESP32-C3 |
| Kernlogik (Strahl-Abriss, Kontakt, Totzeit, Puffer, Pakete) | getestet, per Mutation geprüft (`tools/test.sh`) |
| Bluetooth-Protokoll | beschrieben, **auf keinem Gerät probiert** |
| Schaltung, Teileliste, Gehäuse | nach Datenblättern und Rechnung, **nichts davon gebaut oder gedruckt** |
| Anbindung in der Kamera-App | gebaut (Release 0.3.0, intern in TestFlight), **am Gerät nicht geprüft** |
| Startstab: Mechanik, Magnet- und Reedhalter | nur entworfen, **nichts gebaut, gedruckt oder gemessen** |
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

## Der Startstab

Für den Start gibt es eine Alternative zum Strahl: eine Stange zwischen zwei Pfosten, wie beim
Skirennen. Wer losläuft, drückt sie auf. Am Stangenende sitzt ein Magnet, im Pfosten ein
Reedkontakt. Das Öffnen ist der Start, sofern der Kontakt vorher eine Weile zu war.

![Startstab](docs/bilder/startstab.svg)

Alles dazu: [docs/startstab.md](docs/startstab.md).

## Dokumente

| Dokument | Inhalt |
| --- | --- |
| [docs/architektur.md](docs/architektur.md) | Bausteine, Weg eines Durchgangs, Entscheidungen mit Begründung, Fehlerfälle, was getestet ist |
| [docs/protokoll.md](docs/protokoll.md) | Bluetooth-Dienst, Pakete, Uhrenabgleich, Weg zum Server |
| [docs/aufbau.md](docs/aufbau.md) | Schritt für Schritt vom Schreibtischtest bis zur Strecke (Lichtschranke) |
| [docs/startstab.md](docs/startstab.md) | der Startstab: Prinzip, Verdrahtung, Mechanik-Vorschlag, Prüfung, offene Punkte |
| [docs/teileliste.md](docs/teileliste.md) | Teile, Mengen, Hinweise |
| [docs/amazon.md](docs/amazon.md) | Einkaufsliste mit gewöhnlichen Suchlinks (keine Partnerlinks) |
| [docs/messung.md](docs/messung.md) | drei Versuche, die zeigen, wie genau es wirklich ist |
| [gehaeuse/tor.scad](gehaeuse/tor.scad) | parametrisches Gehäuse für Tor, Sender und Startstab in OpenSCAD (nicht gedruckt) |
| [gehaeuse/stab.scad](gehaeuse/stab.scad) | Magnet- und Reedhalter für den Startstab (nicht gedruckt) |
| [gehaeuse/stl/](gehaeuse/stl) | aus den SCAD-Dateien gerenderte STL-Dateien |

## Bauen und aufspielen

```
python3 -m venv .venv && .venv/bin/pip install platformio
.venv/bin/pio run -e schreibtisch -t upload   # Test ohne Sensor: BOOT-Taster = Durchgang
.venv/bin/pio run -e tor -t upload            # Empfänger an GPIO 4, meldet Ziele
.venv/bin/pio run -e tor_start -t upload      # dasselbe, meldet Starts
.venv/bin/pio run -e sender -t upload         # Gegenstelle, Bursts auf GPIO 5
.venv/bin/pio run -e startstab -t upload      # Startstab: Reedkontakt an GPIO 4 gegen GND, meldet Starts
.venv/bin/pio device monitor
```

Im Monitor erscheint bei jedem Durchgang `Durchgang bei … us`. Die Kernlogik testet man ohne
Hardware mit `tools/test.sh` (ein C++-Compiler genügt).

## Sicherheit

Das Tor trägt keine Zugangsdaten und kein Netz. Bluetooth ist **ohne Kopplung** offen: Wer in
Funkreichweite ist, kann Einstellung und Bestätigung schreiben und Zeiten mitlesen. In der
Beta ist das so und wird offen gesagt; für den breiten Einsatz wäre eine Kopplung mit Passkey
die Antwort. Einzelheiten in der [Architektur](docs/architektur.md#sicherheit).

## Lizenz

Je nach Inhalt: **MIT** für Firmware, Tests und Werkzeuge, **CERN-OHL-P-2.0** für Gehäuse,
Halter und Schaltpläne, **CC BY 4.0** für Dokumentation und Bilder. Übersicht in
[LICENSE](LICENSE), volle Texte in [LICENSES/](LICENSES), fremde Bausteine in
[NOTICE](NOTICE). Bei CC BY und CERN-OHL-P ist die Namensnennung Pflicht, zum Beispiel
„Marco Bockelbrink, einfachstoppen.de“. Es gibt keine Gewähr.
