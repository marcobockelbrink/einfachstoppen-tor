# Messung: wie genau ist die Zeit wirklich?

Bisher ist **nichts davon gemessen**. Die Firmware kompiliert und die Kernlogik ist
getestet; die Uhrenabstimmung über Bluetooth wurde auf keinem Gerät probiert.

Die App, mit der diese Versuche laufen, ist die Kamera-App ab Release 0.3.0; ihre Checkliste steht im Abschnitt
„Tor per Bluetooth“ von `docs/feldtest.md` im Repo `einfachstoppen-ziel`.

## Versuch 1: Tor-Uhr gegen sich selbst (Drift)

1. Tor einschalten, mit dem Handy verbinden, Sync alle 30 s über 2 Stunden.
2. Für jede Abstimmung `versatz` mitschreiben.
3. Auswertung: Steigung = Drift in ppm; Streuung um die Gerade = Abstimmungsfehler.

Erwartet (nicht belegt): einige 10 ppm für den Quarz, also wenige ms pro Stunde.

## Versuch 2: Durchgang gegen Referenz

1. Zwei Taster auf einem Brett: einer löst das Tor aus (an `TOR_PIN`), der andere
   den Auslöser eines Handys mit Zeitlupen-Video bei 240 fps.
2. 30-mal auslösen, Tor-Zeit (umgerechnet) gegen Bildzeit.
3. Auswertung: Mittelwert und Streuung des Unterschieds in ms.

Das Ergebnis, nicht die Erwartung, kommt in `protokoll.md` an die Stelle mit dem
„Ungemessen“-Hinweis.

## Versuch 3: Verbindung kappen

Tor auslösen, Funk des Handys aus, noch zweimal auslösen, Funk an. Alle drei
Durchgänge müssen mit ihrer wahren Zeit ankommen und dürfen nicht doppelt zählen.
