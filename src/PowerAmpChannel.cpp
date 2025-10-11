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

PowerAmpChannel::PowerAmpChannel(uint8_t iChannelNumber, Stream* serialStream) {
    _channelIndex = iChannelNumber;
    mySerial = serialStream;
    initHandlers();   // HandlerMap befüllen
}

PowerAmpChannel::~PowerAmpChannel() 
{
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
        logDebugP("processInputKo: channel not active");
        return;
    }

    logIndentUp();
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[channel]processInputKo: channel %u", _channelIndex);
        
    }   

    //logDebugP("AMP_KoCalcIndex %i", AMP_KoCalcIndex(iKo.asap()));

    switch (AMP_KoCalcIndex(iKo.asap()))
    {
        case AMP_KoChVolumeStep: // Volume Step
        {
           // 0 = Decrease ; 1 = Increase
            logDebugP("processInputKo: volume_step");
            bool value = iKo.value(DPT_Step);
            if (value == 1)
            {
                // 1 = Increase
                currentVolume = currentVolume + currentVolumeStepValue;
            }
            else if (value == 0)
            {
                // 0 = Decrease 
                currentVolume = currentVolume - currentVolumeStepValue;
            }
            setVolume(currentVolume);
            break;
        }
        case AMP_KoChVolumeValue: // SET
        {
            logDebugP("processInputKo: volume_set");
            currentVolume = (uint8_t)KoAMP_ChVolumeValue.value(DPT_Scaling);
            setVolume(currentVolume);
            break;
        }
        case AMP_KoChMuteOnOff:
        {
            logDebugP("processInputKo: mute_onoff");
            muteStatus = KoAMP_ChMuteOnOff.value(DPT_Switch);
            setMute(muteStatus);
            break;
        }
        case AMP_KoChPlayPause:
        {
            logDebugP("processInputKo: play_pause");
            playPause();
            break;
        }
        case AMP_KoChStop:
        {
            logDebugP("processInputKo: stop");
            stop();
            break;
        }
        case AMP_KoChNext:
        {
            logDebugP("processInputKo: next");
            next();
            break;
        }
        case AMP_KoChPrev:
        {
            logDebugP("processInputKo: previous");
            previous();
            break;
        }
        case AMP_KoChSource:
        {
            logDebugP("processInputKo: source");
            uint8_t srcVal = KoAMP_ChSource.value(DPT_Value_1_Ucount); // Wert als uint8_t holen
            enumSource currentSource = static_cast<enumSource>(srcVal);
            setSource(currentSource);
            break;
        }
        case AMP_KoChDayNight:
        {
            logDebugP("processInputKo: day_night");
            processInputKoDayNight(iKo);
            break;
        }
        case AMP_KoChLock:
        {
            logDebugP("processInputKo: lock");
            processInputKoLock(iKo);
            break;
        }
        case AMP_KoChScene:
        {
            logDebugP("processInputKo: scene");
            processInputKoScene(iKo);
            break;
        }     
        default:
            logDebugP("default case processInputKo: unknown KO index %u", AMP_KoCalcIndex(iKo.asap()));
            break;   
    }
    logIndentDown();
}

void PowerAmpChannel::loop()
{
    handleIncomingData();

    unsigned long now = millis();

    // --- Start der Sequenz alle 60 Sekunden ---
    if (currentState == -1 && (now - lastTriggerTime >= START_INTERVAL))
    {
        currentState = 0;
        lastStateTime = now;
        lastTriggerTime = now; // Zeitpunkt merken für nächsten Zyklus
    }

    // --- States nacheinander alle 1 Sekunde ---
    if (currentState >= 0 && currentState < 5)
    {
        if (now - lastStateTime >= STATE_INTERVAL)
        {
            switch (currentState)
            {
            case 0:
                getDeviceStatus();
                break; // Rufe den Status des Geräts ab
            case 1:
                getMetadataArtist();
                break; // Rufe den Künstlernamen ab
            case 2:
                getMetadataAlbum();
                break; // Rufe den Albumnamen ab
            case 3:
                getMetadataTitle();
                break; // Rufe den Titel ab
            case 4:
                getMetadataVendor();
                break; // Rufe den Vendor ab
            }
            currentState++;
            lastStateTime = now;
        }
    }

    // --- Wenn alle States durch sind, warten bis 60s vorbei sind ---
    if (currentState >= 5)
    {
        currentState = -1; // Warten auf nächsten Start
    }
    checkAliveStatus();
}

void PowerAmpChannel::setup(bool configured)
{
    if (!mySerial)
    {
        logErrorP("Channel %u: no serial assigned!", _channelIndex);
        return;
    }
    logInfoP("Channel %u setup done", _channelIndex);
    logInfoP("paramActive: %i", AMP_ChActive);

     currentVolumeStepValue = 5;
     currentVolumeLimit = 100;

    if (configured)
    {
        currentVolumeStepValue = ParamAMP_VolumeStepValue;
        currentVolumeLimit = ParamAMP_LimitMaxVolume;
        currentVolume = ParamAMP_VolumeDay;
    }
    mySerial->setTimeout(1000); // 1000 ms, als Backup
    setKOInitialValues(); 
    
    // --- Alive-System initialisieren ---
    deviceAlive = false;
    lastResponseMillis_Alive = 0;
    lastAliveMillis_Alive = millis();
}

void PowerAmpChannel::sendRawCommandToArylic(const String command)
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[SEND] sendRawCommandToArylic: %s", command.c_str());
    }
    mySerial->flush(); // Wartet, bis die Übertragung der ausgehenden seriellen Daten abgeschlossen ist.
    mySerial->print(command + "\r\n");
}

void PowerAmpChannel::getDeviceStatus(void) // get device status, available in network playback and bluetooth
{
    logDebugP("[SEND] getDeviceStatusfromArylic STA");
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
        logDebugP("[SEND] getVolume from Arylic");
    }
    sendRawCommandToArylic("VOL;");
}
void PowerAmpChannel::getSource() // get source, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[SEND] getSource from Arylic");
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

void PowerAmpChannel::playPreset(uint8_t presetNum) // start to play preset playlist
{
    sendRawCommandToArylic("PST:" + String(presetNum) + ";");
}

void PowerAmpChannel::setVolume(uint8_t volume)
{
    volume = constrain(volume, 0, currentVolumeLimit); // Begrenze die Lautstärke auf den Bereich 0 bis Vorgabe
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("setVolume: channel %u, volume %d", _channelIndex, volume);
    }
    sendRawCommandToArylic("VOL:" + String(volume) + ";");
}

void PowerAmpChannel::setSource(enumSource sourcenumber) // SRC
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[setSource] sourcenumber: %d", static_cast<uint8_t>(sourcenumber));
    }

    String source = "";
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
        }
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
        case enumSource::USB:
        {
            source = "USB";
            break;
        }
        default:
        {
            source = "";
            logDebugP("[ERROR] setSource: Unbekannter enumSource: %d", static_cast<int>(sourcenumber));
            break;
        }
    }
    if (source.length() > 0)
    {
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
        logDebugP("[setMute] MuteMode: %d", muteStatus);
    }
    sendRawCommandToArylic("MUT:" + String(onoff) + ";");
}

bool PowerAmpChannel::getMute(void)
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[getMute] MuteMode: %d", muteStatus);
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

void PowerAmpChannel::getMetadataTitle(void)
{
    sendRawCommandToArylic("TIT;");
}

void PowerAmpChannel::getMetadataArtist(void)
{
    sendRawCommandToArylic("ART;");
}

void PowerAmpChannel::getMetadataAlbum(void)
{
    sendRawCommandToArylic("ALB;");
}

void PowerAmpChannel::getMetadataVendor(void)
{
    sendRawCommandToArylic("VND;");
}

// void PowerAmpChannel::handleIncomingData(void)
// {
//     // UART-Daten lesen
//     if (mySerial->available() > 0) 
//     {
//         String receivedData = mySerial->readStringUntil('\n');
//         receivedData.trim(); // CR/LF/Spaces weg

//         if (receivedData.isEmpty()) 
//         {
//             return; // leere Zeile überspringen
//         }

//         // if (openknxPowerAmpModule.debug())
//         // {
//         //     // Rohdaten als Hex ausgeben
//         //     String hexString;
//         //     for (size_t i = 0; i < receivedData.length(); ++i) 
//         //     {
//         //         if (i > 0) hexString += " ";
//         //         char buf[4];
//         //         sprintf(buf, "%02X", (uint8_t)receivedData[i]);
//         //         hexString += buf;
//         //     }
//         //     logDebugP("handleIncomingData HEX: %s", hexString.c_str());
//         //     logDebugP("handleIncomingData receivedData: %s", receivedData.c_str());
//         // }

//         // Semikolon am Ende entfernen (falls vorhanden)
//         if (receivedData.endsWith(";")) {
//             receivedData.remove(receivedData.length() - 1);
//         }

//          // Aufteilen in Typ und Wert
//         int separatorIndex = receivedData.indexOf(':');

//         if (separatorIndex > 0) 
//         {
//             // Kommando mit Parameter
//             String commandType  = receivedData.substring(0, separatorIndex);
//             String commandValue = receivedData.substring(separatorIndex + 1);
//             commandType.trim();
//             commandValue.trim();

//             if (openknxPowerAmpModule.debug())
//             {
//                 logDebugP("[RCV] commandType: %s, commandValue: %s", commandType.c_str(), commandValue.c_str());
//                 logIndentUp();
//             }

//             processReceivedUARTCommand(commandType, commandValue);

//             if (openknxPowerAmpModule.debug())
//             {
//                 logIndentDown();
//             }
//         }
//         else
//         {
//             // Kommando ohne Parameter
//             String commandType = receivedData;
//             commandType.trim();

//             if (openknxPowerAmpModule.debug())
//             {
//                 logDebugP("[RCV] only commandType (no value): %s", commandType.c_str());
//                 logIndentUp();
//             }

//             // Für Befehle ohne Parameter geben wir leeren Wert weiter
//             processReceivedUARTCommand(commandType, "");

//             if (openknxPowerAmpModule.debug())
//             {
//                 logIndentDown();
//             }
//         }
//     }
// }

void PowerAmpChannel::handleIncomingData(void)
{
    static String uartBuffer = "";           //  Puffer für (unvollständige) Nachrichten
    static unsigned long lastReceiveTime = 0; // Zeitstempel des letzten Zeichens
    static uint32_t errorCount = 0;          // Fehlerzähler

    // Prüfen, ob Daten im UART-Puffer liegen
    while (mySerial->available() > 0)
    {
        char incomingChar = mySerial->read();
        lastReceiveTime = millis();

        // CR/LF ignorieren
        if (incomingChar == '\r' || incomingChar == '\n')
            continue;

        // Zeichen an den Puffer anhängen
        uartBuffer += incomingChar;

        // Nachricht abgeschlossen, wenn ; empfangen
        if (incomingChar == ';')
        {
            String receivedData = uartBuffer;
            uartBuffer = ""; // Buffer reset

            receivedData.trim();

            // Debug-Infos mit Zeitstempel
            if (openknxPowerAmpModule.debug())
            {
                String hexString;
                for (size_t i = 0; i < receivedData.length(); ++i)
                {
                    if (i > 0) hexString += " ";
                    char buf[4];
                    sprintf(buf, "%02X", (uint8_t)receivedData[i]);
                    hexString += buf;
                }
                logDebugP("handleIncomingData HEX: %s", hexString.c_str());
                logDebugP("handleIncomingData receivedData: %s (timestamp: %lu ms)", receivedData.c_str(), millis());
            }

            // Semikolon am Ende entfernen
            if (receivedData.endsWith(";"))
                receivedData.remove(receivedData.length() - 1);

            receivedData.trim();
            if (receivedData.isEmpty())
                continue;

            // Typ und Wert trennen
            int separatorIndex = receivedData.indexOf(':');
            String commandType, commandValue;

            if (separatorIndex > 0)
            {
                commandType = receivedData.substring(0, separatorIndex);
                commandValue = receivedData.substring(separatorIndex + 1);
            }
            else
            {
                commandType = receivedData;
                commandValue = "";
            }

            commandType.trim();
            commandValue.trim();

            if (openknxPowerAmpModule.debug())
            {
                logDebugP("[RCV] commandType: %s, commandValue: %s, (timestamp: %lu ms)", commandType.c_str(), commandValue.c_str(), millis() );
                logIndentUp();
            }

            processReceivedUARTCommand(commandType, commandValue);

            if (openknxPowerAmpModule.debug())
            {
                logIndentDown();
            }
        }

        // Überlauf-Schutz (wichtig bei SoftwareSerial)
        if (uartBuffer.length() > 256)
        {
            errorCount++;
            logErrorP("[UART] Buffer overflow (len=%u), clearing! Total errors: %lu", uartBuffer.length(), errorCount);
            uartBuffer = "";
        }
    }

    // Timeout-Erkennung: falls eine Nachricht nie abgeschlossen wird
    if (uartBuffer.length() > 0 && (millis() - lastReceiveTime > 1000))
    {
        errorCount++;
        logErrorP("[UART] Timeout waiting for ';' (buffer cleared). Total errors: %lu", errorCount);
        uartBuffer = "";
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
    else if (source == "USB")       return enumSource::USB;
    else {
        logDebugP("[ERROR] Unbekannte Quelle: %s", source.c_str());
        return enumSource::Error;
    }
}

/*---------------------------------------------------------------------------------------------------
                      Funktion zur Verarbeitung empfangener UART-Kommandos
 ---------------------------------------------------------------------------------------------------*/
void PowerAmpChannel::processReceivedUARTCommand(const String commandType, const String commandVal)
{
    // Logik zum Verarbeiten der UART-Kommandos vom ArylicAmp

    // Vorverarbeitung
    String commandValuetrimmed = commandVal;
    commandValuetrimmed.trim();
    if (commandValuetrimmed.endsWith(";")) 
    {
        commandValuetrimmed.remove(commandValuetrimmed.length() - 1);
        commandValuetrimmed.trim();
    }

    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[processReceivedUARTCommand] commandType: %s, commandValuetrimmed: %s", commandType.c_str(), commandValuetrimmed.c_str());
    }

    auto foundHandler = commandHandlers.find(commandType);
    if (foundHandler != commandHandlers.end()) {
        foundHandler->second(commandValuetrimmed);
        updateAlive();   // jede gültige Antwort = alive
    } else {
        logDebugP("[ERROR] Unknown command: %s", commandType.c_str());
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
        logDebugP("[ERROR] Ungültige STA-Daten: %s", commandValue.c_str());
        return;
    }

    // Werte zuweisen
    string_currentSource = statusParts[0];
    currentSource = sourceStringToInt(string_currentSource);

    muteStatus = statusParts[1].toInt();
    currentVolume = constrain(statusParts[2].toInt(), 0, 100);
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
        logDebugP("[STA] Quelle: %s, Quelle int: %d, Mute: %d, Lautstärke: %d, Treble: %d, Bass: %d, Net: %d, Internet: %d, Playing: %d, LED: %d, Upgrading: %d",
                 string_currentSource.c_str(), static_cast<int>(currentSource), muteStatus, currentVolume, currentTrebleTone, currentBassTone, netStatus, internetStatus, playingStatus, ledStatus, upgradingStatus);
    }
    sendVolumeStatusKO();
}

bool PowerAmpChannel::isActive()
{
    return ParamAMP_ChActive; // Gibt den Aktivitätsstatus des Kanals zurück
}

void PowerAmpChannel::sendVolumeStatusKO(void)
{
    KoAMP_ChVolumeStatus.value(currentVolume, DPT_Scaling);
    if (openknxPowerAmpModule.debug())
    {
         logDebugP("[INFO] Volume Status gesendet: %d", currentVolume);
    }
}

void PowerAmpChannel::sendSourceStatusKO(void)
{
    // KoAMP_ChSource.value(uint8_t(currentSource), DPT_Value_1_Ucount);
    KoAMP_ChSourceStatus.value(string_currentSource.c_str(), DPT_String_8859_1); // Update the KO with the source information
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[INFO] Source Status gesendet dez: %d, String: %s", currentSource, string_currentSource.c_str());
    }
}

/* Handler für UART Kommandos von Arylic*/
void PowerAmpChannel::initHandlers() {
    commandHandlers = {
        {"SRC", [this](const String& v){ handleSource_SRC(v); }},
        {"VOL", [this](const String& v){ handleVolume_VOL(v); }},
        {"MUT", [this](const String& v){ handleMute_MUT(v); }},
        {"STA", [this](const String& v){ handleDeviceStatusSummary_STA(v); }},
        {"TIT", [this](const String& v){ handleTitle_TIT(v); }},
        {"ART", [this](const String& v){ handleArtist_ART(v); }},
        {"ALB", [this](const String& v){ handleAlbum_ALB(v); }},
        {"VND", [this](const String& v){ handleVendor_VND(v); }},
        {"LED", [this](const String& v){ handleLed_LED(v); }},
        {"BTC", [this](const String& v){ handleBluetooth_BTC(v); }},
        {"VBS", [this](const String& v){ handleVirtualBass_VBS(v); }},
        {"BEP", [this](const String& v){ handleBeep_BEP(v); }},
        {"APL", [this](const String& v){ handleAutoplay_APL(v); }},
        {"PLA", [this](const String& v){ handlePlaying_PLA(v); }},
        {"ELP", [this](const String& v){ handleElapsed_ELP(v); }},
        {"PLI", [this](const String& v){ handlePlaylist_PLI(v); }},
        {"BAS", [this](const String& v){ handleBass_BAS(v); }},
        {"TRE", [this](const String& v){ handleTreble_TRE(v); }},
        {"MID", [this](const String& v){ handleMid_MID(v); }}
    };
}

void PowerAmpChannel::handleSource_SRC(const String& val) {
    string_currentSource = val;
    currentSource = sourceStringToInt(val);
    sendSourceStatusKO();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] Source updated: %s (%d)", string_currentSource.c_str(), (int)currentSource);
    }
}

void PowerAmpChannel::handleVolume_VOL(const String& val) {
    currentVolume = constrain(val.toInt(), 0, currentVolumeLimit);
    sendVolumeStatusKO();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] Volume updated: %d", currentVolume);
    }
}

void PowerAmpChannel::handleMute_MUT(const String& val) {
    muteStatus = (bool)val.toInt();
    KoAMP_ChMuteStatus.value(muteStatus, DPT_Switch);
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] Mute updated: %d", muteStatus);
    }
}

void PowerAmpChannel::handleDeviceStatusSummary_STA(const String& val) {
    processSTACommand(val);
}

void PowerAmpChannel::handleTitle_TIT(const String& val) {
    songMetadataTitle = val;
    KoAMP_ChSongMetadataTitle.value(songMetadataTitle.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Title updated: %s", val.c_str());
}

void PowerAmpChannel::handleArtist_ART(const String& val) {
    songMetadataArtist = val;
    KoAMP_ChSongMetadataArtist.value(songMetadataArtist.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Artist updated: %s", val.c_str());
}

void PowerAmpChannel::handleAlbum_ALB(const String& val) {
    songMetadataAlbum = val;
    KoAMP_ChSongMetadataAlbum.value(songMetadataAlbum.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Album updated: %s", val.c_str());
}

void PowerAmpChannel::handleVendor_VND(const String& val) {
    songMetadataVendor = val;
    KoAMP_ChSongMetadataVendor.value(songMetadataVendor.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Vendor updated: %s", val.c_str());
}

void PowerAmpChannel::handleLed_LED(const String& val) {
    ledStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] LED updated: %d", ledStatus);
}

void PowerAmpChannel::handleBluetooth_BTC(const String& val) {
    bluetoothConnected = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Bluetooth updated: %d", bluetoothConnected);
}

void PowerAmpChannel::handleVirtualBass_VBS(const String& val) {
    virtualBassEnabled = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] VirtualBass updated: %d", virtualBassEnabled);
}

void PowerAmpChannel::handleBeep_BEP(const String& val) {
    beepEnabled = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Beep updated: %d", beepEnabled);
}

void PowerAmpChannel::handleAutoplay_APL(const String& val) {
    autoplayStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Autoplay updated: %d", autoplayStatus);
}

void PowerAmpChannel::handlePlaying_PLA(const String& val) {
    playingStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Playing updated: %d", playingStatus);
}

void PowerAmpChannel::handleElapsed_ELP(const String& val) {
    elapsedTime = val;
    KoAMP_ChElapsedTime.value(elapsedTime.c_str(), DPT_String_8859_1); // Update the KO with the elapsed time
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Elapsed updated: %s", val.c_str());
}

void PowerAmpChannel::handlePlaylist_PLI(const String& val) {
    playlistInfo = val;
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Playlist updated: %s", val.c_str());
}

void PowerAmpChannel::handleBass_BAS(const String& val) {
    currentBassTone = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Bass updated: %d", currentBassTone);
}

void PowerAmpChannel::handleTreble_TRE(const String& val) {
    currentTrebleTone = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Treble updated: %d", currentTrebleTone);
}

void PowerAmpChannel::handleMid_MID(const String& val) {
    currentMidTone = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Mid updated: %d", currentMidTone);
}

// ---------------- Alive Handling ----------------
void PowerAmpChannel::updateAlive() 
{
    lastResponseMillis_Alive = millis();
    if (!deviceAlive) {
        deviceAlive = true;
        logInfoP("[ALIVE] AMP antwortet!");
    }
}

void PowerAmpChannel::checkAliveStatus()
{
    unsigned long currentMillis = millis();
    static bool lastAliveState = false; // zum Erkennen von Statuswechseln

    // Wenn Gerät als alive markiert ist, aber zu lange keine Antwort kam → DEAD
    if (deviceAlive && (currentMillis - lastResponseMillis_Alive > alive_timeout))
    {
        deviceAlive = false;
       // logInfoP("[DEAD] AMP antwortet nicht!");
        lastAliveState = deviceAlive;
    }

    // Zyklische Alive-Meldung 
    if ((currentMillis - lastAliveMillis_Alive) >= (ParamAMP_AliveTimeInterval * 1000UL))
    {
        lastAliveMillis_Alive = currentMillis;

        if (ParamAMP_AliveCheckBox == 1)
        {
            KoAMP_ChAliveStatus.value(deviceAlive, DPT_Switch);
            if (openknxPowerAmpModule.debug())
            {
                logDebugP("[ALIVE] Alive Status gesendet: %d", deviceAlive);
            }
        }

        // Wenn Status sich geändert hat, neu senden
        if (deviceAlive != lastAliveState)
        {
            lastAliveState = deviceAlive;
            if (deviceAlive)
                logInfoP("[ALIVE] AMP erreichbar!");
            else
                logInfoP("[DEAD] AMP antwortet nicht!");
        }
    }
}

void PowerAmpChannel::lock()
{
    if (ParamAMP_Lock == 0 || _currentLocked) return;

    _currentLocked = true;
    stop();
    KoAMP_ChLock.value(_currentLocked, DPT_Switch);
    logDebugP("lock");
}

void PowerAmpChannel::unlock()
{
    if (ParamAMP_Lock == 0 || !_currentLocked) return;

    _currentLocked = false;
    KoAMP_ChLock.value(_currentLocked, DPT_Switch);
    logDebugP("unlock");
}

void PowerAmpChannel::day()
{
    logInfoP("day mode");
    _currentNight = false;
    setDefaultVolume();
}

void PowerAmpChannel::night()
{
    logInfoP("night mode");
    _currentNight = true;
    setDefaultVolume();
}

void PowerAmpChannel::processInputKoDayNight(GroupObject &ko)
{
    bool value = ko.value(DPT_Switch);

    if (ParamAMP_DayNight == 1 && value == 0 || ParamAMP_DayNight == 2 && value == 1)
        return night();

    return day();
}

void PowerAmpChannel::setDefaultVolume()
{
    // Dont set during playing
    if (playingStatus == true) return;

    // select _currentDefaultVolume
    if (_currentNight)
        currentVolume = ParamAMP_VolumeNight;
    else
        currentVolume = ParamAMP_VolumeDay;

    // update 
    setVolume(currentVolume);
}

void PowerAmpChannel::processInputKoLock(GroupObject &ko)
{
    bool value = ko.value(DPT_Switch);

    if (ParamAMP_Lock == 1 && value == 1 || ParamAMP_Lock == 2 && value == 0)
        return lock();

    return unlock();
}

void PowerAmpChannel::processInputKoScene(GroupObject &ko)
{
    if (!ParamAMP_ChScenesActive)
    {
        return;
    }

    uint8_t Szenennummer = ko.value(DPT_SceneNumber);
    Szenennummer += 1;
    logDebugP("processInputKoScene: Szenennummer %i", Szenennummer);
    for (uint8_t i = 0; i < 9; i++)
    {
        uint8_t sceneId     = knx.paramByte(sceneBlocks[i].scene);
        uint8_t sceneQuelle = 0;
        uint8_t sceneVolume = 0;

        logDebugP("Block %i -> Szenennummer: %i", i + 1, sceneId);

        if (sceneId == 0) 
        {
            logDebugP("Keine Szene definiert (Block %i)", i + 1);
        }
        else if (sceneId >= 1 && sceneId <= 8) 
        {
            // Quelle/Volume anhand der SceneId laden
            sceneQuelle = knx.paramByte(sceneBlocks[sceneId].quelle);
            sceneVolume = knx.paramByte(sceneBlocks[sceneId].volume);

            logInfoP("Block %i -> Scene %i: Quelle %i, Volume %i", i, sceneId, sceneQuelle, sceneVolume);

            enumSource currentSource = static_cast<enumSource>(sceneQuelle);
            setSource(currentSource);
            setVolume(sceneVolume);
        }
        else 
        {
            logDebugP("Ungültige Szenennummer %i (Block %i)", sceneId, i + 1);
        }
    }
}

void PowerAmpChannel::setKOInitialValues(void)
{
    String empty = "";
    // Initialwerte für KOs setzen
    KoAMP_ChVolumeStatus.value(currentVolume, DPT_Scaling);
    KoAMP_ChSourceStatus.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChMuteStatus.value(muteStatus, DPT_Switch);
    KoAMP_ChAliveStatus.value(deviceAlive, DPT_Switch);
    KoAMP_ChLock.value(_currentLocked, DPT_Switch);
    KoAMP_ChSongMetadataTitle.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongMetadataArtist.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongMetadataAlbum.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongMetadataVendor.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChElapsedTime.value(empty.c_str(), DPT_String_8859_1);

    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[INIT] KO Initial Values gesetzt");
    }
}