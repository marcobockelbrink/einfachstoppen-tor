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

## Versuch 4: Startstab, Verzögerung der Stange

Nur für den Startstab ([startstab.md](startstab.md)). Der Kontakt öffnet erst, wenn die Stange ein
Stück offen ist. Wie lange das dauert und wie stark es streut, ist unbekannt.

1. Startstab aufbauen, Handy mit Zeitlupen-Video (240 fps) auf die Stange richten, das Tor
   mit dem Handy verbunden.
2. 30-mal die Stange aufdrücken, je zehnmal langsam, normal und schnell.
3. Je Versuch: Zeit, zu der sich die Stange im Video zu bewegen beginnt, gegen die Tor-Zeit
   (umgerechnet).
4. Auswertung: Mittelwert und Streuung des Unterschieds in ms, getrennt nach Geschwindigkeit.

Ist die Verzögerung fest, lässt sie sich später in der App abziehen (nicht gebaut). Streut sie
stark, ist der Startstab nicht genauer als ein Tipp mit dem Finger und sollte so beschrieben
werden.

## Versuch 3: Verbindung kappen

Tor auslösen, Funk des Handys aus, noch zweimal auslösen, Funk an. Alle drei
Durchgänge müssen mit ihrer wahren Zeit ankommen und dürfen nicht doppelt zählen.
