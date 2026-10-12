# Teileliste

Für **eine Strecke** (Sender links, Tor rechts). Wer Start und Ziel mit je einem Tor
bestücken will, baut zwei Strecken, also alles doppelt.

Stand 12.10.2026. **Preise sind grobe Größenordnungen**, nicht für alle Teile geprüft;
wo ich nichts gefunden habe, steht keine Zahl da. Alle technischen Angaben zum Empfänger
sind dem Vishay-Datenblatt (Dok. 82459, Rev. 2.4 vom 23.05.2025) entnommen.

| Nr. | Teil | Anzahl | Wofür | Hinweis |
| --- | --- | --- | --- | --- |
| 1 | ESP32-C3 SuperMini | 2 | je eines für Sender und Tor | USB-C, BLE. In eBay-Angeboten für Deutschland 3–5 € je Stück gesehen (Stichprobe 12.10.2026, teils 2025). |
| 2 | IR-Empfänger TSOP4838 | 1 | Empfang der Bursts | 38 kHz. Pinbelegung TSOP48: 1 = OUT, 2 = GND, 3 = VS (bei TSOP22/24 sind 2 und 3 vertauscht, im Datenblatt prüfen). Ein Händler (LCSC) nennt ihn „nicht für neue Designs empfohlen“; das steht **nicht** im Datenblatt. Alternativen mit 38 kHz (TSOP38238, VS1838B-Modul): Verhalten bei Bursts und Sonne **nicht geprüft**. |
| 3 | IR-LED 940 nm, z. B. TSAL6200 | 1 | Sender | Vishay hat die „24 m typisch“ des Empfängers mit TSAL6200 bei 50 mA gemessen (im Labor, dunkel). |
| 4 | NPN-Transistor BC337 (oder 2N2222) | 1 | schaltet den LED-Strom | Der GPIO treibt die LED nie direkt. Pinbelegung des Transistors im Datenblatt prüfen. |
| 5 | Widerstand 1 kΩ | 1 | Basiswiderstand | |
| 6 | Widerstand 68 Ω | 1 | Vorwiderstand der LED | Gerechnet für 5 V, 50 mA, Durchlassspannung der LED **angenommen** 1,4 V, Sättigung des Transistors 0,2 V: (5 − 1,4 − 0,2) / 0,05 = 68 Ω. Die echte Durchlassspannung steht im Datenblatt der LED. |
| 7 | Kondensator 100 nF, Widerstand | 1 | am Empfänger, optional | Das Datenblatt empfiehlt R1 und C1 bei starkem Brummen oder Spitzen auf der Versorgung; Werte dort in der Zeichnung nachsehen. |
| 8 | USB-C-Kabel | 2 | Strom | |
| 9 | Powerbank | 2 | Strom | Manche Powerbanks schalten bei kleinen Strömen von selbst ab. Wenn der Sender nach einer Weile aus ist, ist das die erste Vermutung. |
| 10 | 3D-gedrucktes Gehäuse (PETG) | 2 | schützt, hält Linse und Stativ | [`gehaeuse/tor.scad`](../gehaeuse/tor.scad), noch nicht gedruckt |
| 11 | Mutter 1/4″ (UNC 20) | 2 | Stativgewinde | passt auf Kamerastativ und Handyhalter |
| 12 | Blechschrauben M3 × 8 | 8 | Deckel | |
| 13 | Schrumpfschlauch schwarz, Ø 6–8 mm | 2 | bündelt den Strahl | Hinweis aus Elektronikforen: ein Röhrchen vor LED und Empfänger bündelt und blendet Streulicht aus. Nicht gemessen. |
| 14 | Lötkolben, Lötzinn, Litze oder Jumperkabel | | Aufbau | Für einen ersten Test genügt ein Steckbrett. |
| 15 | Stativ | 2 | trägt die Geräte auf Höhe | Kamerastativ oder Handystativ mit 1/4″-Schraube |

Nicht auf der Liste: Messmittel (Multimeter, Handy-Kamera zum Sichtbarmachen der IR-LED) –
nützlich, aber nicht nötig.

Wenn man Teile bei Amazon kaufen möchte: [amazon.md](amazon.md).
