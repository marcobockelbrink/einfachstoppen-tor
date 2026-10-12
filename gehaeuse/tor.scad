// SPDX-License-Identifier: CERN-OHL-P-2.0
// einfachstoppen Tor -- Gehaeuse fuer Sender und Tor (OpenSCAD).
//
// NICHT GERENDERT UND NICHT GEDRUCKT. Alle Masse unten sind Annahmen; das echte Board,
// die LED, den TSOP und die Mutter nachmessen und hier eintragen, bevor etwas gedruckt wird.
//
//   openscad -o tor_unten.stl -D 'variante="tor"'    -D 'teil="unten"'  gehaeuse/tor.scad
//   openscad -o tor_deckel.stl -D 'variante="tor"'   -D 'teil="deckel"' gehaeuse/tor.scad
//   openscad -o sender_unten.stl -D 'variante="sender"' -D 'teil="unten"' gehaeuse/tor.scad
//   openscad -o stab_unten.stl -D 'variante="stab"'   -D 'teil="unten"'  gehaeuse/tor.scad
//   (Deckel: fuer alle Varianten derselbe. Halter fuer Stange und Reed: gehaeuse/stab.scad)
//
// Material: PETG, 0,2 mm Schicht, 3 Wandlinien. Die Oeffnung zeigt in +X.

variante = "tor";   // "tor" (TSOP-Fenster), "sender" (Loch fuer eine 5-mm-LED) oder "stab" (Kabeldurchfuehrung, Schraubloecher)
teil     = "unten"; // "unten" oder "deckel"

// --- Annahmen: nachmessen! ---
board_l = 23;       // Laenge des ESP32-C3 SuperMini (X)
board_b = 18;       // Breite (Y)
usb_b   = 9.5;      // Breite der USB-C-Oeffnung
usb_h   = 4;        // Hoehe der USB-C-Oeffnung
tsop_d  = 8;        // Fenster vor dem TSOP (Roehrchen, Schrumpfschlauch ~6-8 mm)
led_d   = 5.3;      // Bohrung fuer eine 5-mm-LED
mutter_sw = 11.3;   // 1/4"-UNC-Mutter, Schluesselweite (7/16" = 11,1 mm) plus Spiel
mutter_h  = 5.8;    // Mutterhoehe plus Spiel

kabel_d = 5;        // Durchfuehrung fuer das Reed-Kabel (Variante "stab")
holz_d  = 3.4;      // Loch fuer eine Holzschraube 3 mm (Variante "stab")

wand   = 2;         // Wandstaerke
boden  = 8;         // dick genug fuer die Mutter
spiel  = 0.4;       // Passungsspiel
platz_x = 14;       // Raum fuer Draht, Widerstand, Transistor hinter dem Board
platz_y = 8;
innen_h = 16;       // Innenhoehe ueber dem Boden

schraube_d = 2.6;   // Kernloch fuer Blechschraube M3
dom_d      = 7;

innen_x = board_l + 2 * spiel + platz_x;
innen_y = board_b + 2 * spiel + platz_y;
aussen_x = innen_x + 2 * wand;
aussen_y = innen_y + 2 * wand;
aussen_z = boden + innen_h;

$fn = 48;

module doeme(h) {
  for (x = [wand + dom_d / 2, aussen_x - wand - dom_d / 2])
    for (y = [wand + dom_d / 2, aussen_y - wand - dom_d / 2])
      translate([x, y, boden]) cylinder(d = dom_d, h = h);
}

module unten() {
  difference() {
    union() {
      difference() {
        cube([aussen_x, aussen_y, aussen_z]);
        translate([wand, wand, boden]) cube([innen_x, innen_y, innen_h + 1]);
      }
      doeme(innen_h);
    }
    // Schraubloecher in den Doemen
    for (x = [wand + dom_d / 2, aussen_x - wand - dom_d / 2])
      for (y = [wand + dom_d / 2, aussen_y - wand - dom_d / 2])
        translate([x, y, boden + 2]) cylinder(d = schraube_d, h = innen_h);

    // USB-C: hintere Wand (-X)
    translate([-1, aussen_y / 2 - usb_b / 2, boden + 3])
      cube([wand + 2, usb_b, usb_h]);

    // Front (+X): Fenster fuer den TSOP, Bohrung fuer die LED oder Durchfuehrung fuer das Kabel
    translate([aussen_x - wand - 1, aussen_y / 2, boden + innen_h / 2])
      rotate([0, 90, 0])
        cylinder(d = (variante == "tor") ? tsop_d : (variante == "stab") ? kabel_d : led_d, h = wand + 2);

    // Variante "stab": zwei Schraubloecher durch den Boden, mit Senkung von innen, damit das
    // Gehaeuse an den Pfosten geschraubt werden kann. Neben dem Sechskant der Stativmutter.
    if (variante == "stab")
      for (dx = [-11, 11])
        translate([aussen_x / 2 + dx, aussen_y / 2, -0.01]) {
          cylinder(d = holz_d, h = boden + 0.02);
          translate([0, 0, boden - 4.5]) cylinder(d = 6.5, h = 4.6);
        }

    // Stativgewinde: Sechskant fuer die Mutter von unten, Durchgang fuer die Schraube
    translate([aussen_x / 2, aussen_y / 2, -0.01]) {
      cylinder(d = 6.8, h = boden + 0.02);
      cylinder(d = mutter_sw / cos(30), h = mutter_h, $fn = 6);
    }
  }
}

module deckel() {
  difference() {
    union() {
      cube([aussen_x, aussen_y, wand]);
      // Lippe, die in die Wand greift
      translate([wand + spiel, wand + spiel, wand])
        cube([innen_x - 2 * spiel, innen_y - 2 * spiel, 2]);
    }
    for (x = [wand + dom_d / 2, aussen_x - wand - dom_d / 2])
      for (y = [wand + dom_d / 2, aussen_y - wand - dom_d / 2])
        translate([x, y, -1]) cylinder(d = 3.4, h = wand + 4);
  }
}

if (teil == "unten") unten(); else deckel();
