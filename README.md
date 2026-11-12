# ocean-data-physicalization-2026
Muros Week in Japan. Intercultural group project on Litter in the Ocean by Japan. 

This repository includes Models, code and research on this Data-Physicalization-Project. 

# 3D model
The one used and optimised: https://www.thingiverse.com/thing:182358
another moe detailed version: https://makerworld.com/de/models/1405226-3d-topographic-model-of-the-island-of-japan?from=search#profileId-1457946

# ABOUT 
Hier ist das fertige Handout als Markdown-Datei, optimiert für eure `README.md` auf GitHub. Es fasst das gesamte Projekt, die Data Story, die methodischen Entscheidungen und den physischen Aufbau (Data Physicalization) klar und verständlich zusammen.

---

# 🌊 Microplastic Density in Japanese Maritime Regions (2014–2023)
*Data Physicalization & Exploratory Data Analysis for University Project*

## 📖 1. Die Data Story: Worum geht es?
Japans Meere sind ein Brennpunkt globaler Meeresströmungen. Doch rohe Messdaten zur Mikroplastik-Verschmutzung wirken oft chaotisch – extreme Spitzenwerte wechseln sich ab und verzerren die Wahrnehmung. 

Unsere zentrale Frage lautet daher: **Gibt es seit dem globalen Wendepunkt 2017 einen klaren Trend in der Mikroplastik-Belastung rund um Japan – und wie driften die Regionen auseinander?**

Unsere Analyse enthüllt eine spannende **Zweiteilung**:
* **Entwarnung an der Pazifikküste:** An den stark urbanisierten Pazifikküsten ist die Mikroplastik-Dichte im Vergleich zu 2017 **spürbar gesunken**.
* **Alarm in den Randmeeren:** Das **Japanische Meer** verhält sich völlig anders. Hier sorgen Meeresströmungen und Einträge immer wieder für unberechenbare Hotspots und heftige Peaks (wie im Krisenjahr 2021). Pauschale Landesschnitte täuschen – effektiver Meeresschutz muss regional differenziert gedacht werden.

---

## 🎯 2. Unser Vorhaben & Physische Interaktion (Data Physicalization)
Um diese Data Story interaktiv und erlebbar zu machen, verbinden wir Data Science mit Hardware:
1. **Das Interface (3D-Druck):** Eine 3D-gedruckte physische Karte von Japan mit Knöpfen für unsere **4 Hauptregionen**.
2. **Die Visualisierung (Arduino & LEDs):** Drückt man auf eine Region, leuchtet daneben eine dynamische LED-Skala auf, die den Trend und den Belastungsgrad (inkl. des 2021er-Peaks) im Vergleich zum Basisjahr 2017 visuell darstellt.

---

## 📊 3. Welche Daten & Regionen nutzen wir?
* **Datenquelle:** Das offizielle **AOMI-Portal** des japanischen Umweltministeriums (Rohdaten-CSV mit Tausenden von Messpunkten zu Partikeldichten $d < 5\,\text{mm}$ pro $\text{m}^3$).
* **Zeitraum:** 2014 bis 2023 (mit Fokus auf Veränderungen seit 2017).
* **Die 4 Hauptregionen (nach ozeanographischer Zusammenlegung):**
  1. `1_Sea_of_Japan` (Japanisches Meer – Nord & West kombiniert als unberechenbarer Hotspot).
  2. `2_East_China_Sea` (Ostchinesisches Meer – starker kontinentaler Einfluss).
  3. `3_Pacific_North` (Nordpazifik / Oyashio-Strom).
  4. `4_Pacific_South_Central` (Süd-/Zentralpazifik / Kuroshio-Strom & urbane Küsten).
  * *(Hinweis: Zone 5 Ogasawara wurde wegen zu vieler fehlender Messwerte als reine Kontrollzone für diese Hauptanalyse weggelassen).*

---

## 🧠 4. Gründe für unsere methodischen Entscheidungen

Jeder Schritt in diesem Projekt ist wissenschaftlich begründet:

* **Warum 2017 als Basisjahr (Index = 100 %)?**
  2017 markiert eine historische Zäsur: China stoppte Ende 2017 seine Plastikmüll-Imports (*National Sword Policy*), was Japans Entsorgungsströme grundlegend veränderte. Zudem starteten ab 2017 strukturierte, flächendeckende Monitoring-Programme in Japan.
* **Warum der Median statt des Mittelwerts?**
  Mikroplastik-Daten enthalten extrem schiefe Verteilungen und heftige Ausreißer. Der Median filtert dieses statistische Rauschen heraus und zeigt den wahren, typischen Zustand einer Region.
* **Wie gehen wir mit `NaN`-Werten (fehlenden Daten) um?**
  * Regionen mit extrem lückenhaften Daten (wie Ogasawara) wurden ausgeschlossen.
  * Verwandte, kleine Lücken in den verbleibenden 4 Regionen wurden **zeitlich linear interpoliert**, um eine durchgehende, vergleichbare Zeitreihe für die Visualisierung zu garantieren.
* **Warum Weg A für den 2021er-Peak (LED-Anzeige)?**
  Statt starr nur den Endpunkt 2023 mit 2017 zu vergleichen (was den massiven Ausreißer-Peak im Jahr 2021 verschlucken würde), nutzen wir einen geglätteten Trend-Wert bzw. ein Peak-Highlighting für die LEDs. Das zeigt dem Publikum sofort, wo die echten Umweltkrisen im System lauern.

---
NOCH NICHT IMPELEMTIERT: 
## 🛠️ Repository-Struktur
* `notebooks/` – Enthält das Jupyter Notebook (`.ipynb`) zur Datenbereinigung, Aggregation und Interpolation.
* `output/` – Automatisch generierte CSV-Tabellen und hochauflösende Diagramme.
* `arduino/` – Der Mikrocontroller-Code zur Steuerung der LEDs über die Regions-Buttons auf der 3D-Karte.
