/*
  # Arylic's UART API developer documentation
  # https://developer.arylic.com/uartapi/#uart-api

# Basic Rules
   * messages are defined in 3 characters, and will use : to seperate the different part.
   * messages sent over UART need to be terminated with ;
   * messages might be received without query when state changed.
   * Content between {} is variable name, you need to replace with the real content, and {} itself is not meant to be sent.
   * Content between [] means optional, and [] itself is not meant to be sent.
   * normally, messages sent by host without param means to query current state or direct control
   * messages sent with param means to control or change state.
   * messages received normally with param indicating current state.
*/

#include "OpenKNX.h"
#include "PowerAmpChannel.h"
#include "PowerAmpModule.h"
#include <SoftwareSerial.h>


PowerAmpChannel::PowerAmpChannel(uint8_t iChannelNumber)
{
    _channelIndex = iChannelNumber;
}

PowerAmpChannel::~PowerAmpChannel() 
{
    #if OPENKNX_AMP_CHANNEL_COUNT > 1
    if (mySWSerial) delete mySWSerial;
    #endif
}

const std::string PowerAmpChannel::name()
{
    return "PowerAmpChannel";
}

// will be called once a KO received a telegram
void PowerAmpChannel::processInputKo(GroupObject &iKo)
{
    if (ParamAMP_ChActive != 1)
    {
        logTraceP("processInputKo: channel not active (%u)", ParamAMP_ChActive);
        return;
    }

    if (openknxPowerAmpModule.debug())
    {
        logDebugP("processInputKo: channel %u", _channelIndex);
        logIndentUp();
    }   

    
    
    // uint16_t lAsap = iKo.asap();
    // switch (lAsap)
    // {
    //     case AMP_KoCentralFunction:
    //         if (ParamAMP_ChCentralFunction)
    //         {
    //             newActive = ko.value(DPT_Switch);
    //             logDebugP("AMP_KoCentralFunction: %u", newActive);

    //             processSwitchInput(newActive);
    //         }
    //         break;
    // }

    switch (AMP_KoCalcIndex(iKo.asap()))
    {
        case AMP_Kovolume_inc: // Increase ++
        {
            if (KoAMP_volume_inc.value(DPT_Step))
            {
                currentVolume++;
            }
            setVolume(currentVolume);
            break;
        }
        case AMP_Kovolume_dec: // Decrease --
        {
            if (KoAMP_volume_dec.value(DPT_Step))
            {
                currentVolume--;
            }
            setVolume(currentVolume);
            break;
        }
        case AMP_Kovolume_value: // SET
        {
            currentVolume = (u_int8_t)KoAMP_volume_value.value(DPT_Scaling);
            setVolume(currentVolume);
            break;
        }
        case AMP_Komute_onoff:
        {
            muteStatus = KoAMP_mute_onoff.value(DPT_Switch);
            setMute(muteStatus);
            break;
        }
        case AMP_KoPlayPause:
        {
            playPause();
            break;
        }
        case AMP_KoStop:
        {
            stop();
            break;
        }
        case AMP_KoNext:
        {
            next();
            break;
        }
        case AMP_KoPrev:
        {
            previous();
            break;
        }
        case AMP_Kosource:
        {
            uint8_t srcVal = KoAMP_source.value(DPT_Value_1_Ucount); // Wert als uint8_t holen
            enumSource currentSource = static_cast<enumSource>(srcVal);
            setSource(currentSource);
            break;
        }
    }
    if (openknxPowerAmpModule.debug())
    {
        logIndentDown();
    } 
}

void PowerAmpChannel::loop(bool configured)
{
    handleIncomingData();
    static unsigned long lastMillis = 0; // Speichert den letzten Zeitpunkt
    unsigned long currentMillis = millis(); // Aktuelle Zeit in Millisekunden

    // Prüfen, ob eine Minute (60.000 Millisekunden) vergangen ist
    if (currentMillis - lastMillis >= 60000)
    {
        lastMillis = currentMillis; // Aktualisiere den letzten Zeitpunkt
        getDeviceStatus(); // Rufe den Status des Geräts ab
    }
}

void PowerAmpChannel::setup(bool configured)
{
    if (_channelIndex == 1)
    {
        mySerial = &AMP_HARDWARE_SERIAL;
        AMP_HARDWARE_SERIAL.setRX(SERIAL_RXPINS[_channelIndex]);
        AMP_HARDWARE_SERIAL.setTX(SERIAL_TXPINS[_channelIndex]);
        AMP_HARDWARE_SERIAL.begin(BAUD_ARLYIC);
        if (openknxPowerAmpModule.debug())
        {
            logInfoP("PowerAmpChannel setup: HardwareSerial RX Pin %d, TX Pin %d", SERIAL_RXPINS[_channelIndex], SERIAL_TXPINS[_channelIndex]);
        }
    }
    else
    {
        if (mySWSerial)
        {
            delete mySWSerial;
        }
        mySWSerial = new SoftwareSerial(SERIAL_RXPINS[_channelIndex], SERIAL_TXPINS[_channelIndex]);
        mySWSerial->begin(BAUD_ARLYIC);
        mySerial = mySWSerial;
        if (openknxPowerAmpModule.debug())
        {
            logDebugP("PowerAmpChannel setup: SoftwareSerial RX Pin %d, TX Pin %d", SERIAL_RXPINS[_channelIndex], SERIAL_TXPINS[_channelIndex]);
        }
    }
}

void PowerAmpChannel::sendRawCommandToArylic(const String command)
{
    mySerial->flush(); // Wartet, bis die Übertragung der ausgehenden seriellen Daten abgeschlossen ist.
    mySerial->print(command + "\r\n");
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[SEND] sendRawCommandToArylic %s", command);
    }
}

void PowerAmpChannel::getDeviceStatus(void) // get device status, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[SEND] getDeviceStatusfromArylic");
    }
    sendRawCommandToArylic("STA;");
    /*
    Device status summary, and the response message {states} will
    consist with: current source,mute,volume,treble,bass,net,internet,playing,led,upgrading.
    STA response sample
    NET,0,33,-2,0,1,1,1,1,0
    */
}
void PowerAmpChannel::getVolume() // get volume, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[SEND] getVolume from Arylic");
    }
    sendRawCommandToArylic("VOL;");
}
void PowerAmpChannel::getSource() // get source, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[SEND] getSource from Arylic");
    }
    sendRawCommandToArylic("SRC;");
}

void PowerAmpChannel::setLoopShuffleMode(const String &loopmode) // LPM[:{loopmode}]   set/get loop and shuffle mode, available in network playback.
{
    /*
    {loopmode} 	    description
    REPEATALL 	    repeat all in playlist
    REPEATONE 	    repeat track
    REPEATSHUFFLE 	repeat all and shuffle
    SHUFFLE 	    shuffle and stop when all tracks played
    SEQUENCE 	    stop when reach end of playlist
    */
    sendRawCommandToArylic("LPM:" + loopmode + ";");
}

void PowerAmpChannel::playPause() // POP play or pause
{
    sendRawCommandToArylic("POP;");
}

void PowerAmpChannel::stop() // STP stop
{
    sendRawCommandToArylic("STP;");
}

void PowerAmpChannel::next() // NXT next track
{
    sendRawCommandToArylic("NXT;");
}

void PowerAmpChannel::previous() // PRE previous track
{
    sendRawCommandToArylic("PRE;");
}

void PowerAmpChannel::playPreset(int presetNum) // start to play preset playlist
{
    sendRawCommandToArylic("PST:" + String(presetNum) + ";");
}

void PowerAmpChannel::setVolume(int volume)
{
    volume = constrain(volume, 0, 100);
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("setVolume: channel %u, volume %d", _channelIndex, volume);
    }
    sendRawCommandToArylic("VOL:" + String(volume) + ";");
}

void PowerAmpChannel::setSource(enumSource sourcenumber) // SRC
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[setSource] sourcenumber: %d", static_cast<uint8_t>(sourcenumber));
    }

    String source = " ";
    switch (sourcenumber)
    {
        case enumSource::Network:
        {
            source = "NET";
            break;
        }
        case enumSource::Bluetooth:
        {
            source = "BT";
            break;
        case enumSource::USBDAC:
        {
            source = "USBDAC";
            break;
        }
        case enumSource::LineIn:
        {
            source = "LINE-IN";
            break;
        }
        case enumSource::Optical:
        {
            source = "OPT";
            break;
        }
        case enumSource::Coaxial:
        {
            source = "COAX";
            break;
        }
        default:
            break;
        }
            sendRawCommandToArylic("SRC:" + source + ";");
    }
}

// Overload für String-Parameter
// Diese Methode wird aufgerufen, wenn der Quellparameter ein String ist
void PowerAmpChannel::setSource(const String &source) // SRC
{
    sendRawCommandToArylic("SRC:" + source + ";");
}

void PowerAmpChannel::setMute(bool onoff)
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[setMute] MuteMode: %d", muteStatus);
    }
    sendRawCommandToArylic("MUT:" + String(onoff) + ";");
}

bool PowerAmpChannel::getMute(void)
{
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[getMute] MuteMode: %d", muteStatus);
    }
    return muteStatus; // Gibt den aktuellen Mute-Status zurück
}

void PowerAmpChannel::setAutoplay(bool onoff)
{
    sendRawCommandToArylic("APL:" + String(onoff) + ";");
}

bool PowerAmpChannel::getAutoplay(void)
{
    return autoplayStatus; // Gibt den aktuellen Autoplay-Status zurück
}

String PowerAmpChannel::getMetadataTitle(void)
{
    return songMetadataTitle;
}

String PowerAmpChannel::getMetadataArtist(void)
{
    return songMetadataArtist;
}

String PowerAmpChannel::getMetadataAlbum(void)
{
    return songMetadataAlbum;
}

String PowerAmpChannel::getMetadataVendor(void)
{
    return songMetadataVendor;
}

void PowerAmpChannel::handleIncomingData(void)
{
    // UART-Daten lesen
    if (mySerial->available() > 0)
    {
        String receivedData = mySerial->readStringUntil('\n');

        if (openknxPowerAmpModule.debug())
        {
            logInfoP("handleIncomingData receivedData %s", receivedData);
        }

        receivedData.trim(); // Entfernt alle führenden und nachfolgenden Leerzeichen aus der aktuellen Zeichenfolge.
        if (openknxPowerAmpModule.debug())
        {
            logInfoP("receivedData trimmed: %s", receivedData);
        }
        
        int separatorIndex = receivedData.indexOf(':');
        if (separatorIndex > 0 && separatorIndex < receivedData.length() - 1)
        {
            String commandType = receivedData.substring(0, separatorIndex);
            String commandValue = receivedData.substring(separatorIndex + 1);

            // Daten auswerten
            processReceivedUARTCommand(commandType, commandValue);
        }
    }
}

enumSource PowerAmpChannel::sourceStringToInt(const String source)
{
    if (source == "NET")            return enumSource::Network;
    else if (source == "BT")        return enumSource::Bluetooth;
    else if (source == "USBDAC")    return enumSource::USBDAC;
    else if (source == "LINE-IN")   return enumSource::LineIn;
    else if (source == "OPT")       return enumSource::Optical;
    else if (source == "COAX")      return enumSource::Coaxial;
    else {
        logDebugP("[ERROR] Unbekannte Quelle: %s", source.c_str());
        return enumSource::Error;
    }
}


/*---------------------------------------------------------------------------------------------------
                      Funktion zur Verarbeitung empfangener UART-Kommandos
 ---------------------------------------------------------------------------------------------------*/
void PowerAmpChannel::processReceivedUARTCommand(const String commandType, const String commandValue)
{
    // Logik zum Verarbeiten der UART-Kommandos vom ArylicAmp

    if (openknxPowerAmpModule.debug())
    {
        logDebugP("processReceivedUARTCommand: commandType %s, commandValue %s", commandType, commandValue);
    }   

    // string currentSource;
    if (commandType == "SRC")
    {
        string_currentSource = commandValue;
        currentSource = sourceStringToInt(string_currentSource);
        logDebugP("[INFO] Quelle aktualisiert: %s", string_currentSource);
        logDebugP("[INFO] Quelle aktualisiert int: %d", currentSource);
    }
    else if (commandType == "VOL")
    {
        currentVolume = commandValue.toInt();
        logDebugP("[INFO] Lautstärke aktualisiert:  %d", currentVolume);
    }
    else if (commandType == "MUT")
    {
        muteStatus = (bool)commandValue.toInt();
        logDebugP("[INFO] Mute Status:  %d", muteStatus);
    }
    else if (commandType == "BAS")
    {
        currentBassTone = commandValue.toInt();
        logDebugP("[INFO] Bass aktualisiert:  %d", currentBassTone);
    }
    else if (commandType == "TRE")
    {
        currentTrebleTone = commandValue.toInt();
        logDebugP("[INFO] Treble aktualisiert:  %d", currentTrebleTone);
    }
    else if (commandType == "MID")
    {
        currentMidTone = commandValue.toInt();
        logDebugP("[INFO] Mid aktualisiert:  %d", currentMidTone);
    }
    else if (commandType == "LED")
    {
        if (commandValue == "1;")
        {
            ledStatus = true;
            logDebugP("[INFO] LED ON");
        }
        else if (commandValue == "0;")
        {
            ledStatus = false;
            logDebugP("[INFO]LED OFF");
        }
    }
    else if (commandType == "BTC")
    {
        if (commandValue == "1;")
        {
            bluetoothConnected = true;
            logDebugP("[INFO] Bluetooth Connected");
        }
        else if (commandValue == "0;")
        {
            bluetoothConnected = false;
            logDebugP("[INFO] Bluetooth Disconnect");
        }
    }
    else if (commandType == "VBS")
    {
        if (commandValue == "1;")
        {
            virtualBassEnabled = true;
            logDebugP("[INFO] virtualBass VBS ON");
        }
        else if (commandValue == "0;")
        {
            virtualBassEnabled = false;
            logDebugP("[INFO] virtualBass VBS OFF");
        }
    }
    else if (commandType == "BEP")
    {
        if (commandValue == "1;")
        {
            beepEnabled = true;
            logDebugP("[INFO] BEEP ON");
        }
        else if (commandValue == "0;")
        {
            beepEnabled = false;
            logDebugP("[INFO] BEEP OFF");
        }
    }
    else if (commandType == "STA")
    {
        processSTACommand(commandValue);
        logDebugP("commandValue: %s", commandValue);
    }
    else if (commandType == "APL")
    {
        autoplayStatus = (bool)commandValue.toInt();
        logDebugP("[INFO] Autoplay Status:  %d", autoplayStatus);
    }
    else if (commandType == "TIT") //notification messages for song metadata title. 
    {
        songMetadataTitle = commandValue;
        logDebugP("[INFO] Titel-Update empfangen: %s", commandValue);
        KoAMP_ChsongMetadataTitle.valueNoSend(songMetadataTitle.c_str(), DPT_String_8859_1); // Update the KO with the title information:
        KoAMP_ChsongMetadataTitle.objectWritten(); // Mark the KO as written to send the update
    }
     else if (commandType == "ART") //notification messages for song metadata artist.
    {
        songMetadataArtist = commandValue;
        logDebugP("[INFO] Künstler-Update empfangen: %s", commandValue);
        KoAMP_ChsongMetadataArtist.valueNoSend(songMetadataArtist.c_str(), DPT_String_8859_1); // Update the KO with the artist information
        KoAMP_ChsongMetadataArtist.objectWritten(); // Mark the KO as written to send the update
    }
    else if (commandType == "ALB") //notification messages for song metadata album.
    {
        songMetadataAlbum = commandValue;
        logDebugP("[INFO] Album-Update empfangen: %s", commandValue);
        KoAMP_ChsongMetadataAlbum.valueNoSend(songMetadataAlbum.c_str(), DPT_String_8859_1); // Update the KO with the album information
        KoAMP_ChsongMetadataAlbum.objectWritten(); // Mark the KO as written to send the update
    }
    else if (commandType == "VND") //notification messages for song metadata vendor.
    {
        songMetadataVendor = commandValue;
        //{vendor} will have the following value:
        //spotify qplay dlna airplay upnp phone usb tidal napster qobuz amazon tunein iheart vtuner http other
        logDebugP("[INFO] Vendor-Update empfangen: %s", commandValue);
        KoAMP_ChsongMetadataVendor.valueNoSend(songMetadataVendor.c_str(), DPT_String_8859_1); // Update the KO with the vendor information
        KoAMP_ChsongMetadataVendor.objectWritten(); // Mark the KO as written to send the update
    }
    else
    {
        logDebugP("[ERROR] Unbekanntes Kommando: %s", commandType);
    }
}

void PowerAmpChannel::processSTACommand(const String commandValue)
{
    // Beispiel: NET,0,33,-2,0,1,1,1,1,0
    // Zerlege die empfangenen Daten anhand des Trennzeichens ','
    // STA
    // Device status summary, and the response message {states} will consist with:
    // current source,mute,volume,treble,bass,net,internet,playing,led,upgrading.

    std::vector<String> statusParts;
    int startIndex = 0;
    int separatorIndex = commandValue.indexOf(',');

    while (separatorIndex != -1)
    {
        statusParts.push_back(commandValue.substring(startIndex, separatorIndex));
        startIndex = separatorIndex + 1;
        separatorIndex = commandValue.indexOf(',', startIndex);
    }
    statusParts.push_back(commandValue.substring(startIndex)); // Letzter Teil

    // Überprüfen, ob genügend Daten vorhanden sind
    if (statusParts.size() < 10)
    {
        logDebugP("[ERROR] Ungültige STA-Daten: %s", commandValue);
        return;
    }

    // Werte zuweisen
    string_currentSource = statusParts[0];
    currentSource = sourceStringToInt(string_currentSource);

    muteStatus = statusParts[1].toInt();
    currentVolume = statusParts[2].toInt();
    currentTrebleTone = statusParts[3].toInt();
    currentBassTone = statusParts[4].toInt();
    netStatus = statusParts[5].toInt();
    internetStatus = statusParts[6].toInt();
    playingStatus = statusParts[7].toInt();
    ledStatus = statusParts[8].toInt();
    upgradingStatus = statusParts[9].toInt();

    // Debug-Ausgabe
    if (openknxPowerAmpModule.debug())
    {
        logInfoP("[STA] Quelle: %s, Quelle int: %d, Mute: %d, Lautstärke: %d, Treble: %d, Bass: %d, Net: %d, Internet: %d, Playing: %d, LED: %d, Upgrading: %d",
                 string_currentSource.c_str(), currentSource, muteStatus, currentVolume, currentTrebleTone, currentBassTone, netStatus, internetStatus, playingStatus, ledStatus, upgradingStatus);
    }
}
