// SPDX-License-Identifier: CERN-OHL-P-2.0
// einfachstoppen Startstab -- Halter fuer Magnet und Reedkontakt (OpenSCAD).
//
// Gerendert (ohne Fehler), aber NICHT GEDRUCKT und fuer den Druck nicht geprueft. Alle Masse sind Annahmen:
// Stange, Magnet und Reed nachmessen und hier eintragen, bevor etwas gedruckt wird.
//
//   openscad -o stab_magnethalter.stl -D 'teil="magnethalter"' gehaeuse/stab.scad
//   openscad -o stab_reedhalter.stl   -D 'teil="reedhalter"'   gehaeuse/stab.scad
//
// Der Magnethalter klemmt am Stangenende und haelt den Scheibenmagneten mit der Stirnseite
// nach vorn. Der Reedhalter wird mit zwei Holzschrauben an den Pfosten geschraubt und haelt
// das Glasroehrchen so, dass es dem Magneten bei anliegender Stange gegenuebersteht.
// Das Gehaeuse fuer den ESP32 ist gehaeuse/tor.scad mit variante="stab".
//
// Material: PETG, 0,2 mm Schicht, 3 Wandlinien.

teil = "magnethalter";   // "magnethalter" oder "reedhalter"

// --- Annahmen: nachmessen! ---
stab_d    = 12;    // Durchmesser der Stange (Rundstab)
magnet_d  = 8;     // Scheibenmagnet, Durchmesser
magnet_h  = 3;     // Scheibenmagnet, Dicke
reed_l    = 14;    // Glasroehrchen des Reeds, Laenge
reed_d    = 2.8;   // Glasroehrchen, Durchmesser (Datenblatt!)
spiel     = 0.2;   // Passungsspiel
schraube_d = 3.4;  // Durchgang fuer M3 (Klemmung) und Loch fuer Holzschraube

$fn = 64;

// Klemmring am Stangenende. Die Bohrung ist von hinten blind (Stange stoesst an), in der
// Stirnwand sitzt der Magnet in einer offenen Tasche.
module magnethalter() {
  wand = 4;            // Ringwand
  stirn = 4;           // Stirnwand
  bohr_t = 14;         // Tiefe der Bohrung, von hinten
  laenge = bohr_t + stirn;
  aussen = stab_d + 2 * wand;
  difference() {
    union() {
      cylinder(d = aussen, h = laenge);
      // Ohr fuer die Klemmschraube (Quer zum Schlitz)
      translate([-4, aussen / 2 - 1, 0]) cube([8, 6, laenge]);
    }
    // Bohrung fuer die Stange (von z = 0, blind)
    translate([0, 0, -0.01]) cylinder(d = stab_d + spiel, h = bohr_t + 0.01);
    // Schlitz durch Wand und Ohr, damit sich der Ring zuziehen laesst
    translate([-1, 0, -0.01]) cube([2, aussen / 2 + 7, bohr_t + 0.01]);
    // Klemmschraube M3 quer durch das Ohr
    translate([0, aussen / 2 + 2, bohr_t / 2]) rotate([0, 90, 0])
      translate([0, 0, -6]) cylinder(d = schraube_d, h = 12);
    // Tasche fuer den Magneten, von der Stirnseite
    translate([0, 0, laenge - magnet_h]) cylinder(d = magnet_d + spiel, h = magnet_h + 0.01);
  }
}

// Block mit Kanal fuer das Glasroehrchen und zwei Schraublochern in die Rueckwand.
module reedhalter() {
  l = 30; b = 20; h = 16;
  difference() {
    cube([l, b, h]);
    // Kanal fuer das Glasroehrchen, von der Oberseite offen zum Einkleben
    translate([(l - reed_l) / 2 - 1, b / 2, h - reed_d / 2 - 0.4])
      rotate([0, 90, 0]) cylinder(d = reed_d + spiel, h = reed_l + 2);
    translate([(l - reed_l) / 2 - 1, b / 2 - (reed_d + spiel) / 2, h - reed_d / 2 - 0.4])
      cube([reed_l + 2, reed_d + spiel, reed_d]);
    // Kabelauslass nach hinten (zum ESP32)
    translate([-1, b / 2, h - reed_d / 2 - 0.4]) rotate([0, 90, 0]) cylinder(d = 2.4, h = (l - reed_l) / 2 + 1);
    // Zwei Holzschrauben von oben durch den Block in den Pfosten
    for (x = [5, l - 5])
      translate([x, b / 2, -0.01]) {
        cylinder(d = schraube_d, h = h - 4);
        translate([0, 0, h - 8]) cylinder(d = 6.5, h = 8.1);
      }
  }
}

if (teil == "magnethalter") magnethalter(); else reedhalter();
