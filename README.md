# OFM-PowerAmp

Dieses Modul stellt eine Anbindung an die DIY-Produkte (Up2Stream) von Arylic bereit. Dafür wird die UART schnittstelle benutzt.

Getestet wurde ausschließlich mit der Up2Stream Amp Stereo.

## Features
- native Steuerung der Audiowiedergabe via KNX GAs
  

## Planned Features


## Applikationsbeschreibung

Die Applikationsbeschreibung ist [hier]"(doc/Applikationsbeschreibung-.md)" zu finden.

## Hardware Unterstützung

|Prozessor | Status               | Anmerkung                  |
|----------|----------------------|----------------------------|
|RP2040    | Beta                 |                            |


Getestete Hardware:
- PiPico mit [PiPico BCU Connector](https://muster.ing-dom.de/Zubehoer/PiPico-BCU-Connector.html)


## Einbindung in die Anwendung

In das Anwendungs XML muss OFM- aufgenommen werden:

```xml
  <op:define prefix="AMP" 
    share="../lib/OFM-PowerAmp/src/PowerAmp.share.xml"
    template="../lib/OFM-PowerAmp/src/PowerAmp.templ.xml"
    NumChannels="%AMP_NumChannels%"
    KoSingleOffset="20"
    KoOffset="1500"
    ModuleType="99">
  </op:define>
```

**Hinweis:** Pro Kanal werden xx KO's benötigt. Dies muss bei nachfolgenden Modulen bei KoOffset entsprechend berücksichtigt werden.

In main.cpp muss das Modul hinzugefügt werden:

```
[...]
#include "PowerAmpModule.h"
[...]

void setup()
{
    [...]
    openknx.addModule(6, openknxPowerAmpModule);
    [...]
}
```

## Lizenz
[GNU GPL v3](LICENSE)