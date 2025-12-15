
# **PowerAmp Modul**

<!-- DOC HelpContext="Dokumentation" -->

<!-- DOCCONTENT
Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: [folgt]
DOCCONTENT -->

## Beschreibung
Dieses Modul stellt eine Anbindung an die DIY-Produkte (Up2Stream) von Arylic bereit. Dafür wird die UART schnittstelle benutzt.

Es ermöglicht die Steuerung von Lautstärke, Quelle, Wiedergabe und weiteren Funktionen direkt über Gruppenadressen.  
Zudem werden Statusinformationen, Metadaten und der Alive-Status zurückgemeldet.

---

## ⚙️ Hauptfunktionen

- Lautstärke setzen / rückmelden  
- Quelle wählen (NET, BT, USB, LINE-IN, OPT, COAX …)  
- Play, Pause, Stop, Next, Previous  
- Anzeige von Song-Metadaten (Titel, Künstler, Album, Vendor)  
- Alive-/Watchdog-System zur Überwachung der Kommunikation  
- Lock-, Day/Night- und eine einfache Szenensteuerung  

---

## 🔄 Kommunikation (UART)
- Zur Vollständigkeit - nicht relvant für only KNX-User
- Befehle gemäß [Arylic UART API](https://developer.arylic.com/uartapi/#uart-api)  
- Nachrichtenstruktur:  
  - Format: `CMD[:WERT];`  
  - Beispiel: `VOL:35;`, `SRC:NET;`, `STA;`  
- Empfangsverarbeitung zeichenweise, abgeschlossen mit `;`  
- Timeout- und Buffer-Schutz integriert  

---

## 🔧 Parameter (ETS)

| Parameter | Beschreibung | Einheit / Typ |
|------------|---------------|----------------|
| **ChActive** | Kanal aktivieren | Bool |
| **VolumeStepValue** | Schrittweite der Lautstärkeänderung | Zahl |
| **LimitMaxVolume** | Maximal erlaubte Lautstärke | Zahl (0–100) |
| **VolumeDay / VolumeNight** | Standardlautstärken für Tag/Nacht | Zahl |
| **Lock** | Bedienung sperren | Bool |
| **DayNight** | Aktiviert Tag-/Nachtmodus | Bool |
| **AliveTimeInterval** | Alive-Meldungsintervall | Sekunden |
| **AliveCheckBox** | Alive-Status auf Bus senden | Bool |

---

## 🧩 Kommunikationsobjekte (KO)

| KO | Richtung | DPT | Beschreibung |
|----|-----------|-----|---------------|
| **Volume Value** | IN | DPT_Scaling | Lautstärke setzen (0–100 %) |
| **Volume Status** | OUT | DPT_Scaling | Aktuelle Lautstärke |
| **Mute On/Off** | IN | DPT_Switch | Mute aktivieren/deaktivieren |
| **Mute Status** | OUT | DPT_Switch | Mute-Zustand |
| **Source** | IN | DPT_Value_1_Ucount | Quelle wählen |
| **Source Status** | OUT | DPT_String_8859_1 | Aktuelle Quelle (z. B. „NET“) |
| **Play/Pause / Stop / Next / Prev** | IN | DPT_Switch | Steuerfunktionen Wiedergabe |
| **Song Title / Artist / Album / Vendor** | OUT | DPT_String_8859_1 | Metadaten zur Wiedergabe |
| **Elapsed Time** | OUT | DPT_String_8859_1 | Aktuelle Abspielzeit |
| **Alive Status** | OUT | DPT_Switch | Gerät erreichbar (true/false) |
| **Lock** | IN/OUT | DPT_Switch | Bedienung sperren/freigeben |
| **Day/Night** | IN | DPT_Switch | Lautstärkeumschaltung Tag/Nacht |

---

## 🧠 Alive-System

Das Alive-System überwacht die Verbindung zwischen KNX-Modul und Arylic-Amp.  
Wenn innerhalb des definierten Zeitraums (`alive_timeout`) keine Antwort empfangen wird,  
wird der **Alive-Status = FALSE** an den Bus gesendet.

| Zustand | Beschreibung |
|----------|---------------|
| 🟢 **Alive** | Kommunikation aktiv, Daten empfangen |
| 🔴 **Dead** | Keine Antwort, Verbindung unterbrochen, keine Spannungsversorgung angeschlossen|

---


## 📋 Hinweise


- Kompatibel mit allen Arylic-Geräten gemäß offizieller API  
  - getestet mit Up2Stream Amp Stereo/Mono

---

## 🏁 Zusammenfassung

| Kategorie | Beschreibung |
|------------|---------------|
| **Modulname** | PowerAmpChannel |
| **Zweck** | KNX–UART-Bridge für Arylic-Up2Stream-Verstärker |
| **Hardware** | OpenKNX-kompatibles Modul mit UART |
| **Funktionen** | Volume, Source, Mute, Play, Stop, Alive, Metadaten |
| **Besonderheiten** | Alive-System, Szenen, Lock, Debug |
| **Status** | 🟢 Stabil |




## **PowerAmp**

<!-- DOC HelpContext="Dokumentation" -->
Mit diesem Modul können PowerAmp-Kanäle parametrisiert werden.

### **Kanaldefinition**

<!-- DOC -->
#### **Beschreibung des Kanals**

Der hier angegebene Name wird an verschiedenen Stellen verwendet, um diesen Kanal wiederzufinden.

<!-- DOC -->
#### **Kanalaktivität**

Hier kann man einen PowerAmp-Kanal aktivieren.

##### **Inaktiv**

Dieser Kanal ist inaktiv. Alle Einstellungen und alle Kommunikaitonsobjekte sind ausgeblendet.

##### **Aktiv**

Dieser Kanal ist aktiv und kann normal parametrisiert werden.

##### **Funktionslos**

Dieser Kanal ist inaktiv. Er kann vollständig definiert sein und keine Einstellung geht verloren, aber es wird kein Telegramm empfangen oder gesendet. Dies bietet die Möglichkeit, zu Testzwecken einen bereits parametrierten Kanal inaktiv zu setzen, um zu schauen, ob er die Ursache für eventuelles Fehlverhalten im Haus ist. Kann zur Fehlersuche hilfreich sein.

<!-- DOC -->
#### **Lautstärke**
Hier wird die Einschaltlautstärke vorgegeben. Es kann bei Aktivierung für Tag/Nacht verschiedene Lautstärken eingestellt werden.

#### **Schrittweite Lautstarke**
je Schritt wird die Lautstärke um Wertx höher/niederger - defalut ist 5.

#### **Begrenzung max Lautstarke**

Lauter kann es dann nicht werden. Begrenzung. Default ist 60% (maximal 100%)

### **Sperre**

Einstellungen zur Sperre.

<!-- DOC -->
#### **Zentrale Sperre**

Hier wird festgelegt, was bei der zentrallen Sperre passieren soll.

<!-- DOC -->
#### **Autoplay**

Autoplay startet die Wiedergabe nach Neustart oder Hochfahren des Gerätes automatisch. Ist Internetradio als Quelle gewählt, wird gewartet bis das Internet verfügbar ist.


#### **Alive**	

<!-- DOC -->
#### **AliveEnable	**
	
Sende Alive Status alle x Sekunde. Default ist 30s.	

### **Zusatzfunktionen**

Zusätzliche Funktionen stehen hier zur Verfügung.

<!-- DOC -->
#### **Szenen aktivieren**

Festlegung, ob Szenenfunktionen genutzt werden sollen.

### **Szene**

Hier finden sich Einstellungen zur jeweiligen Szene.

<!-- DOC -->
#### **Szene aktiv**

Festlegung, ob die gewählte Szene aktiv ist.

<!-- DOC -->
#### **Szene Nummer**

Auswahl der Szenennummer, auf die reagiert werden soll.

Werden mehrere Szenen aktiviert und dieselbe Szenennummer zugewiesen, wird lediglich das Verhalten der ersten aktivierten Szene der jeweiligen Szenennummer berücksichtigt.

<!-- DOC -->
#### **Szene Verhalten**

Das gewünschte Verhalten der Szene kann hier gewählt werden.


