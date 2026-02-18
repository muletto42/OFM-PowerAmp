# **PowerAmp Modul**

<!-- DOC HelpContext="Dokumentation" -->

<!-- DOCCONTENT
Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: [folgt]
DOCCONTENT -->

Dieses Modul stellt eine Anbindung an die DIY-Produkte (Up2Stream) von Arylic bereit.  
Dazu wird die UART-Schnittstelle verwendet.

Es ermöglicht die Steuerung von Lautstärke, Quelle, Wiedergabe und weiteren Funktionen direkt über Gruppenadressen.  
Zusätzlich werden Statusinformationen, Metadaten sowie der Alive-Status zurückgemeldet.

## ⚙️ Hauptfunktionen

- Lautstärke Setzen und Rückmelden  
- Quelle wählen (NET, BT, USB, LINE-IN, OPT, COAX …)  
- Play, Pause, Stop, Next, Previous  
- Anzeige von Song-Metadaten (Titel, Künstler, Album, Vendor)  
- Alive-/Watchdog-System zur Überwachung der Kommunikation  
- Lock-, Day/Night, einfache Szenensteuerung  


## 🔄 Kommunikation (UART)
- Zur Vollständigkeit – nicht relevant für reine KNX-Nutzer
- Befehle gemäß [Arylic UART API](https://developer.arylic.com/uartapi/#uart-api)  
- Nachrichtenstruktur:  
  - Format: `CMD[:WERT];`  
  - Beispiele: `VOL:35;`, `SRC:NET;`, `STA;`  
- Empfangsverarbeitung zeichenweise, Abschluss mit `;`  
- Timeout- und Buffer-Schutz integriert  


## 🔧 Parameter (ETS)

| Parameter | Beschreibung | Einheit / Typ |
|----------|--------------|---------------|
| **ChActive** | Kanal aktivieren | Bool |
| **VolumeStepValue** | Schrittweite der Lautstärkeänderung | Zahl |
| **LimitMaxVolume** | Maximal erlaubte Lautstärke | Zahl (0–100) |
| **VolumeDay / VolumeNight** | Standardlautstärken für Tag/Nacht | Zahl |
| **Lock** | Bedienung sperren | Bool |
| **DayNight** | Aktiviert Tag-/Nachtmodus | Bool |
| **AliveTimeInterval** | Alive-Meldungsintervall | Sekunden |
| **AliveCheckBox** | Alive-Status auf den Bus senden | Bool |


## 🧩 Kommunikationsobjekte (KO)

| KO | Richtung | DPT | Beschreibung |
|----|----------|-----|--------------|
| **Volume Value** | IN | DPT_Scaling | Lautstärke setzen (0–100 %) |
| **Volume Status** | OUT | DPT_Scaling | Aktuelle Lautstärke |
| **Mute On/Off** | IN | DPT_Switch | Mute aktivieren/deaktivieren |
| **Mute Status** | OUT | DPT_Switch | Mute-Zustand |
| **Source** | IN | DPT_Value_1_Ucount | Quelle wählen |
| **Source Status** | OUT | DPT_String_8859_1 | Aktuelle Quelle (z. B. „NET“) |
| **Play / Pause / Stop / Next / Prev** | IN | DPT_Switch | Wiedergabesteuerung |
| **Song Title / Artist / Album / Vendor** | OUT | DPT_String_8859_1 | Metadaten |
| **Elapsed Time** | OUT | DPT_String_8859_1 | Aktuelle Abspielzeit |
| **Alive Status** | OUT | DPT_Switch | Gerät erreichbar (true/false) |
| **Lock** | IN/OUT | DPT_Switch | Bedienung sperren/freigeben |
| **Day/Night** | IN | DPT_Switch | Lautstärkeumschaltung Tag/Nacht |

## 🧠 Alive-System

Das Alive-System überwacht die Verbindung zwischen dem KNX-Modul und dem Arylic-Verstärker.  
Wird innerhalb des definierten Zeitraums (`alive_timeout`) keine Antwort empfangen,  
wird der **Alive-Status = FALSE** auf den Bus gesendet.

| Zustand | Beschreibung |
|--------|--------------|
| 🟢 **Alive** | Kommunikation aktiv, Daten werden empfangen |
| 🔴 **Dead** | Keine Antwort, Verbindung unterbrochen oder keine Spannungsversorgung |


## 📋 Hinweise

- Getestet mit Up2Stream Amp Stereo / Mono  

## **PowerAmp**

Mit diesem Modul können PowerAmp-Kanäle parametrisiert werden.

### **Kanaldefinition**

<!-- DOC -->
#### **Beschreibung des Kanals**

Der hier angegebene Name wird an verschiedenen Stellen verwendet, um diesen Kanal eindeutig zu identifizieren. z.B. Küche, Bad, etc.

<!-- DOC -->
#### **Kanalaktivität**

Hier kann man einen PowerAmp-Kanal aktivieren.

#### Inaktiv
Dieser Kanal ist inaktiv. Alle Einstellungen und Kommunikationsobjekte sind ausgeblendet.

##### **Aktiv**

Dieser Kanal ist aktiv und kann vollständig parametriert werden.

##### **Funktionslos**

Dieser Kanal ist inaktiv. Er kann vollständig parametriert sein, sendet und empfängt jedoch keine Telegramme.  
Dies eignet sich zur Fehlersuche, um einen Kanal testweise außer Betrieb zu nehmen, ohne dessen Konfiguration zu verlieren.


<!-- DOC HelpContext="Lautstaerke" -->
<!-- DOCCONTENT
Hier wird die Einschaltlautstärke festgelegt.  
Bei aktivierter Tag-/Nacht-Funktion können unterschiedliche Lautstärken definiert werden.
DOCCONTENT -->
#### **Lautstärke**
Hier wird die Einschaltlautstärke festgelegt.  
Bei aktivierter Tag-/Nacht-Funktion können unterschiedliche Lautstärken definiert werden.

<!-- DOC HelpContext="Lautstaerke-Tag" -->
<!-- DOCCONTENT
Hier wird die Einschaltlautstärke festgelegt.  
DOCCONTENT -->

<!-- DOC HelpContext="Lautstaerke-Nacht" -->
<!-- DOCCONTENT
Hier wird die Einschaltlautstärke für die Nacht festgelegt. 
DOCCONTENT -->


<!-- DOC -->
#### **Schrittweite Lautstärke**

Pro Schritt wird die Lautstärke um den angegebenen Wert erhöht oder verringert.  
Standardwert: **5**.

<!-- DOC -->
#### **Begrenzung max Lautstärke**

Die Lautstärke kann diesen Wert nicht überschreiten.  
Standardwert: **60 %** (maximal 100 %).

### **Sperre**

Einstellungen zur Sperre.

<!-- DOC -->
#### **Zentrale Sperre**

Hier wird festgelegt, wie sich der Kanal bei einer zentralen Sperre verhält.

<!-- DOC -->
#### **AutoPlay**

Autoplay startet die Wiedergabe nach einem Neustart automatisch.  
Ist Internetradio als Quelle gewählt, wird gewartet, bis eine Internetverbindung verfügbar ist.

<!-- DOC -->
#### **AutoMute**

Ist Automute aktiv, so wird das Gerät gemutet gestartet. Im Hintergrund läuft ggfs. bereits die Wiedergabe weiter oder wurde gestartet.



#### **Alive**	

<!-- DOC -->
#### **AliveEnable	**
Ein/Ausschalten des Alive-Signals.
Das Alive-System überwacht die Verbindung zwischen dem KNX-Modul und dem Arylic-Verstärker.  
Wird innerhalb des definierten Zeitraums (`alive_timeout`) keine Antwort empfangen,  
wird der Alive-Status = FALSE auf den Bus gesendet.

**Alive** | Kommunikation aktiv, Daten werden empfangen 
**Dead** | Keine Antwort, Verbindung unterbrochen oder keine Spannungsversorgung 


<!-- DOC -->
#### **AliveTimeInterval **

Sendet den Alive-Status zyklisch auf den Bus.  
Standardintervall: **30 Sekunden**.


### **Zusatzfunktionen**

Zusätzliche Funktionen stehen hier zur Verfügung.


<!-- DOC -->
#### **Szenen aktivieren**

Legt fest, ob Szenenfunktionen verwendet werden sollen.


### **Szene**

Hier finden sich Einstellungen zur jeweiligen Szene.

<!-- DOC -->
#### **Szene aktiv**
Aktiviert die ausgewählte Szene.

<!-- DOC -->
#### **Szene Nummer**
Auswahl der Szenennummer, auf die reagiert werden soll.  
Sind mehrere Szenen mit derselben Nummer aktiv, wird nur die zuerst aktivierte Szene berücksichtigt.

<!-- DOC -->
#### **Szene Verhalten**
Hier wird das gewünschte Verhalten der Szene definiert.
