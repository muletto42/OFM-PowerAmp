### AliveEnable

Das Alive-System überwacht die Verbindung zwischen dem KNX-Modul und dem Arylic-Verstärker.  
Wird innerhalb des definierten Zeitraums (`alive_timeout`) keine Antwort empfangen,  
wird der Alive-Status = FALSE auf den Bus gesendet.

**Alive** | Kommunikation aktiv, Daten werden empfangen 
**Dead** | Keine Antwort, Verbindung unterbrochen oder keine Spannungsversorgung 


