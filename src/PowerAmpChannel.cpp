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
    if (!_channelActive)
    {
        logDebugP("processInputKo: channel %u not active", _channelIndex);
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

                if (currentVolume < currentVolumeStepValue)
                {
                    currentVolume = 0;
                }
                else
                {
                    currentVolume = currentVolume - currentVolumeStepValue;
                }  
            }
            setVolume_VOL(currentVolume);
            break;
        }
        case AMP_KoChVolumeValue: // SET
        {
            logDebugP("processInputKo: volume_set");
            currentVolume = (uint8_t)KoAMP_ChVolumeValue.value(DPT_Scaling);
            setVolume_VOL(currentVolume);
            break;
        }
        case AMP_KoChMuteOnOff:
        {
            logDebugP("processInputKo: mute_onoff");
            muteStatus_MUT = KoAMP_ChMuteOnOff.value(DPT_Switch);
            setMute_MUT(muteStatus_MUT);
            break;
        }
        case AMP_KoChPlayPause:
        {
            logDebugP("processInputKo: play_pause");
            playPause_POP();
            break;
        }
        case AMP_KoChStop:
        {
            logDebugP("processInputKo: stop");
            stop_STP();
            break;
        }
        case AMP_KoChNext:
        {
            logDebugP("processInputKo: next");
            next_NXT();
            break;
        }
        case AMP_KoChPrev:
        {
            logDebugP("processInputKo: previous");
            previous_PRE();
            break;
        }
        case AMP_KoChSource:
        {
            logDebugP("processInputKo: source");
            uint8_t srcVal = KoAMP_ChSource.value(DPT_Value_1_Ucount); // Wert als uint8_t holen
            currentSource = static_cast<enumSource>(srcVal);
            setSource_SRC(currentSource);
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
        case AMP_KoChAutoPlay:
        {
            logDebugP("processInputKo: autoPlay_command");
            autoPlayEnabled = KoAMP_ChAutoPlay.value(DPT_Switch);
            setAutoplay_APL(autoPlayEnabled);
            KoAMP_ChAutoPlayStatus.value(autoPlayEnabled, DPT_Switch);
            
            break;
        }
        case AMP_KoChAutoMute:
        {
            logDebugP("processInputKo: autoMute_command");
            autoMuteEnabled = KoAMP_ChAutoMute.value(DPT_Switch);
            KoAMP_ChAutoMuteStatus.value(autoMuteEnabled, DPT_Switch);
            ParamAMP_AutoMute = autoMuteEnabled; // persistent speichern
            break;
        }
        case AMP_KoChPreset:
        {
            logDebugP("processInputKo: preset_playlist");
            uint8_t presetNum = KoAMP_ChPreset.value(DPT_DecimalFactor);
            startAndPlayPresetPlaylist_PST(presetNum);
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
    if (!_channelActive) return;
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
                getDeviceStatus_STA();
                break; // Rufe den Status des Geräts ab  consist with: current source,mute,volume,treble,bass,net,internet,playing,led,upgrading.
            case 1:
                getMetadataArtist_ART(); // Rufe den Künstlernamen ab
                break; 
            case 2:
                getMetadataAlbum_ALB(); // Rufe den Albumnamen ab
                break; 
            case 3:
                getMetadataTitle_TIT(); // Rufe den Titel ab
                break; 
            case 4:
                getMetadataVendor_VND(); // Rufe den Vendor ab
                break; 
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
    handleCustomAutoplay();
    handleCustomAutoMute();
}

void PowerAmpChannel::setup(bool configured)
{
    _channelActive = configured && (ParamAMP_ChActive == 1);
    if (!_channelActive) 
    {
        logDebugP("Channel %u: not active!", _channelIndex);
        return;
    }


    if (!mySerial)
    {
        logErrorP("Channel %u: no serial assigned - channel disabled!", _channelIndex);
        _channelActive = false;
        return;
    }

    currentVolumeStepValue = ParamAMP_VolumeStepValue;
    currentVolumeLimit     = ParamAMP_LimitMaxVolume;
    currentVolume          = ParamAMP_VolumeDay;

    logInfoP("Channel %u setup done (active=%d, stepValue=%d, currentVol=%d, maxVol=%d)",
             _channelIndex, ParamAMP_ChActive, currentVolumeStepValue, currentVolume, currentVolumeLimit);
    mySerial->setTimeout(1000); // 1000 ms, als Backup
    setKOInitialValues(); 
    
    // --- Alive-System initialisieren ---
    deviceAlive = false;
    lastResponseMillis_Alive = 0;
    lastAliveMillis_Alive = millis();

    // --- Autoplay initialisieren ---
    onetimeAutoPlayExecuted = false;
    autoPlayEnabled = (ParamAMP_AutoPlay == 1); // ETS-Param 
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INIT] Custom Autoplay: %d", autoPlayEnabled);
    }

    // --- AutoMute initialisieren ---
    onetimeAutoMuteExecuted = false;
    autoMuteEnabled = (ParamAMP_AutoMute == 1); // ETS-Param 
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INIT] AutoMute: %d", autoMuteEnabled);
    }
}

void PowerAmpChannel::sendRawCommandToArylic(const String command)
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[SEND] sendRawCommandToArylic: %s", command.c_str());
    }
    
    // mySerial->print(command + "\r\n");

    // Das Arylic-Protokoll definiert nur ";" als Terminator.
    // Besser: nur ";" senden (das ist bereits im command enthalten):
    mySerial->print(command);
    // mySerial->write('\r');
    // mySerial->write('\n');
    mySerial->flush(); // Wartet, bis die Übertragung der ausgehenden seriellen Daten abgeschlossen ist.
}

void PowerAmpChannel::getDeviceStatus_STA(void) // get device status, available in network playback and bluetooth
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
void PowerAmpChannel::getVolume_VOL() // get volume, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[SEND] getVolume from Arylic");
    }
    sendRawCommandToArylic("VOL;");
}
void PowerAmpChannel::getSource_SRC() // get source, available in network playback and bluetooth
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[SEND] getSource from Arylic");
    }
    sendRawCommandToArylic("SRC;");
}

void PowerAmpChannel::setLoopShuffleMode_LPM(const String &loopmode) // LPM[:{loopmode}]   set/get loop and shuffle mode, available in network playback.
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

void PowerAmpChannel::playPause_POP() // POP play or pause
{
    sendRawCommandToArylic("POP;");
}

void PowerAmpChannel::stop_STP() // STP stop
{
    sendRawCommandToArylic("STP;");
}

void PowerAmpChannel::next_NXT() // NXT next track
{
    sendRawCommandToArylic("NXT;");
}

void PowerAmpChannel::previous_PRE() // PRE previous track
{
    sendRawCommandToArylic("PRE;");
}

void PowerAmpChannel::startAndPlayPresetPlaylist_PST(uint8_t presetNum) // start to play preset playlist
{
    sendRawCommandToArylic("PST:" + String(presetNum) + ";");
}

void PowerAmpChannel::setVolume_VOL(uint8_t volume)
{
    volume = constrain(volume, 0, currentVolumeLimit); // Begrenze die Lautstärke auf den Bereich 0 bis Vorgabe
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("setVolume: channel %u, volume %d", _channelIndex, volume);
    }
    sendRawCommandToArylic("VOL:" + String(volume) + ";");
}

    /*
    {source} 	description
    NET 	    network
    BT 	    bluetooth
    USB        USB
    LINE-IN 	line-in
    OPT 	    Optical
    COAX 	    Coaxial
    USBDAC 	USB DAC
    */

void PowerAmpChannel::setSource_SRC(enumSource sourcenumber) // SRC
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
void PowerAmpChannel::setSource_SRC(const String &source) // SRC
{
    sendRawCommandToArylic("SRC:" + source + ";");
}

void PowerAmpChannel::setMute_MUT(bool onoff)
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[setMute] MuteMode: %d", onoff);
    }
    sendRawCommandToArylic("MUT:" + String(onoff) + ";");
}

bool PowerAmpChannel::getMute_MUT(void)
{
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[getMute] MuteMode: %d", muteStatus_MUT);
    }
    sendRawCommandToArylic("MUT;");
    return muteStatus_MUT; // Gibt den aktuellen Mute-Status zurück
}

void PowerAmpChannel::setAutoplay_APL(bool onoff)
{
    sendRawCommandToArylic("APL:" + String(onoff) + ";");
    KoAMP_ChAutoPlayStatus.value(onoff, DPT_Switch);
}

void PowerAmpChannel::getAutoplay_APL(void)
{
    sendRawCommandToArylic("APL;"); 
}

void PowerAmpChannel::getMetadataTitle_TIT(void)
{
    sendRawCommandToArylic("TIT;");
}

void PowerAmpChannel::getMetadataArtist_ART(void)
{
    sendRawCommandToArylic("ART;");
}

void PowerAmpChannel::getMetadataAlbum_ALB(void)
{
    sendRawCommandToArylic("ALB;");
}

void PowerAmpChannel::getMetadataVendor_VND(void)
{
    sendRawCommandToArylic("VND;");
}

void PowerAmpChannel::handleIncomingData(void)
{
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
            uartErrorCount++;
            logErrorP("[UART] Buffer overflow (len=%u), clearing! Total errors: %lu", uartBuffer.length(), uartErrorCount);
            uartBuffer = "";
        }
    }

    // Timeout-Erkennung: falls eine Nachricht nie abgeschlossen wird
    if (uartBuffer.length() > 0 && (millis() - lastReceiveTime > 1000))
    {
        uartErrorCount++;
        logErrorP("[UART] Timeout waiting for ';' (buffer cleared). Total errors: %lu", uartErrorCount);
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

    muteStatus_MUT = statusParts[1].toInt();
    currentVolume = constrain(statusParts[2].toInt(), 0, 100);
    currentTrebleTone = statusParts[3].toInt();
    currentBassTone = statusParts[4].toInt();
    netStatus = statusParts[5].toInt();
    internetStatus = statusParts[6].toInt();
    playingStatus_PLA = statusParts[7].toInt();
    ledStatus = statusParts[8].toInt();
    upgradingStatus = statusParts[9].toInt();

    // Debug-Ausgabe
    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[STA] Quelle: %s (Quelle uint_8: %d), Mute: %d, Lautstärke: %d, Treble: %d, Bass: %d, Net: %d, Internet: %d, Playing: %d, LED: %d, Upgrading: %d",
                 string_currentSource.c_str(), static_cast<int>(currentSource), muteStatus_MUT, currentVolume, currentTrebleTone, currentBassTone, netStatus, internetStatus, playingStatus_PLA, ledStatus, upgradingStatus);
    }
    sendVolumeStatusKO();
}

bool PowerAmpChannel::isActive()
{
    return _channelActive; // Gibt den Aktivitätsstatus des Kanals zurück
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
void PowerAmpChannel::initHandlers() 
{
    commandHandlers = {
        // ### State And Control ###
        {"STA", [this](const String& v){ handleDeviceStatusSummary_STA(v); }},
        {"SYS", [this](const String& v){ handleSystemOperations_SYS(v); }},
        {"WWW", [this](const String& v){ handleInternetStatus_WWW(v); }},
        {"NAM", [this](const String& v){ handleDeviceName_NAM(v); }},
        {"ETH", [this](const String& v){ handleEthernetStatus_ETH(v); }},
        {"WIF", [this](const String& v){ handleWifiStatus_WIF(v); }},
        {"WRS", [this](const String& v){ handleTriggerWifiSetup_WRS(v); }},
        {"WSS", [this](const String& v){ handleWifiSignalStrength_WSS(v); }},
        {"BSS", [this](const String& v){ handleBluetoothSignalStrength_BSS(v); }},
        {"IPA", [this](const String& v){ handleIpAddress_IPA(v); }},
        {"TME", [this](const String& v){ handleLocalTime_TME(v); }},
        //{"COE", [this](const String& v){ handleEnablePinCodeBT_COE(v); }},
        //{"COD", [this](const String& v){ handlePinCodeBT_COD(v); }},
        // ### Playback ###
        {"SRC", [this](const String& v){ handleSource_SRC(v); }},
        {"POP", [this](const String& v){ handlePlayOrPause_POP(v); }},
        {"STP", [this](const String& v){ handleStop_STP(v); }},
        {"NXT", [this](const String& v){ handleNext_NXT(v); }},
        {"PRE", [this](const String& v){ handlePrevious_PRE(v); }},
        {"PST", [this](const String& v){ handlePreset_PST(v); }},
        {"LPM", [this](const String& v){ handleLoopMode_LPM(v); }},
        {"BTC", [this](const String& v){ handleBluetooth_BTC(v); }},
        {"PLA", [this](const String& v){ handleNetworkPlayingStatus_PLA(v); }},
        {"CHN", [this](const String& v){ handleChannel_CHN(v); }},
        //{"MRM", [this](const String& v){ handleMultiRoomMode_MRM(v); }},
        {"TIT", [this](const String& v){ handleTitle_TIT(v); }},
        {"ART", [this](const String& v){ handleArtist_ART(v); }},
        {"ALB", [this](const String& v){ handleAlbum_ALB(v); }},
        {"VND", [this](const String& v){ handleVendor_VND(v); }},
        {"ELP", [this](const String& v){ handleElapsed_ELP(v); }},
        {"PLI", [this](const String& v){ handlePlaylist_PLI(v); }},
        {"APL", [this](const String& v){ handleAutoplay_APL(v); }},
        // ### Audio ###
        {"AUD", [this](const String& v){ handleAudioOutput_AUD(v); }},
        {"VOL", [this](const String& v){ handleVolume_VOL(v); }},
        {"MUT", [this](const String& v){ handleMute_MUT(v); }},
        {"BAS", [this](const String& v){ handleBass_BAS(v); }},
        {"TRE", [this](const String& v){ handleTreble_TRE(v); }},
        {"MID", [this](const String& v){ handleMid_MID(v); }},
        {"VBS", [this](const String& v){ handleVirtualBass_VBS(v); }},
        {"BAL", [this](const String& v){ handleBalance_BAL(v); }},
        {"VOF", [this](const String& v){ handleVolumeFixedOutput_VOF(v); }},
        {"VOG", [this](const String& v){ handleVolumeGroupedPlayback_VOG(v); }},
        {"PEQ", [this](const String& v){ handleQuerySystemEQGroup_PEQ(v); }},
        {"EQS", [this](const String& v){ handleEQGroup_EQS(v); }},
        {"VST", [this](const String& v){ handleVolumeStep_VST(v); }},
        {"EQE", [this](const String& v){ handleEnableEQ_EQE(v); }},
        {"CFE", [this](const String& v){ handleCrossfilter_CFE(v); }},
        {"CFF", [this](const String& v){ handleCrossfilterFrequencyPoint_CFF(v); }},
        // ### MISC ###
        {"VER", [this](const String& v){ handleVersion_VER(v); }},
        {"LED", [this](const String& v){ handleLed_LED(v); }},
        {"BEP", [this](const String& v){ handleBeep_BEP(v); }},
        // {"PMT", [this](const String& v){ handlePromptVoice_PMT(v); }},
        // {"DLY", [this](const String& v){ handleDelayTimeToAutoMute_DLY(v); }},
        // {"MXV", [this](const String& v){ handleMaxVolume_MXV(v); }},
        // {"ASW", [this](const String& v){ handleAutoSwitchMode_ASW(v); }},
        // {"POM", [this](const String& v){ handlePowerOnMode_POM(v); }},
        // {"VOS", [this](const String& v){ handleVolumeSyncFeature_VOS(v); }},
        // {"LST", [this](const String& v){ handleListSources_LST(v); }},
        // {"SOP", [this](const String& v){ handleStandbyOnPower_SOP(v); }},
        };
}

void PowerAmpChannel::handleSystemOperations_SYS(const String& val) {
// SYS:{cmd}
// system operations
// {cmd} 	description
// ON 	    power on
// OFF 	power off
// REBOOT 	reboot device
// STANDBY 	enter standby. Not well considered, and can't wake up via UART for some models.
// RESET 	reset factory
    
    if (val == "ON") {
        powerStatus = "ON";
    } else if (val == "OFF") {
        powerStatus = "OFF";
    } else if (val == "REBOOT") {
        // Handle reboot if necessary
    } else if (val == "STANDBY") {
        powerStatus = "STANDBY";
    } else if (val == "RESET") {
        // Handle factory reset if necessary
    }
    
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] SYS Power status updated: %s", powerStatus.c_str());
    }
}

void PowerAmpChannel::handleInternetStatus_WWW(const String& val) {
    internetStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] WWW Internet status updated: %d", internetStatus);
    }
}

void PowerAmpChannel::handleWifiStatus_WIF(const String& val) {
    wifiStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] WIF WiFi status updated: %d", wifiStatus);
    }
}

void PowerAmpChannel::handleDeviceName_NAM(const String& val) {
    // Beispiel: NAM:4BFFFF636865
    // string is encoded with hex value in UTF-8 encoding

    deviceName = hexStringToAsciiString(val);
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] NAM Device name updated: %s", deviceName.c_str());
    }
}

String PowerAmpChannel::hexStringToAsciiString(String hexString)
{
    String result = "";

    // sicherstellen, dass Länge gerade ist
    if (hexString.length() % 2 != 0)
    {
        logDebugP("Fehler: Ungerade Hex-Länge!");
        return "";
    }

    for (int i = 0; i < hexString.length(); i += 2)
    {
        // zwei Hex-Zeichen holen
        String byteString = hexString.substring(i, i + 2);
        // in eine Ganzzahl umwandeln (Basis 16)
        char c = (char)strtol(byteString.c_str(), nullptr, 16);
        result += c;
    }

    return result;
}

void PowerAmpChannel::handleEthernetStatus_ETH(const String& val) {
    ethernetStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] ETH Ethernet status updated: %d", ethernetStatus);
    }
}

void PowerAmpChannel::handleTriggerWifiSetup_WRS(const String& val) {
    triggerWifiSetup = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] WRS Trigger WiFi setup updated: %d", triggerWifiSetup);
    }
}

void PowerAmpChannel::handleWifiSignalStrength_WSS(const String& val) {
    wifiSignalStrength = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] WSS WiFi signal strength updated: %d", wifiSignalStrength);
    }
}

void PowerAmpChannel::handleBluetoothSignalStrength_BSS(const String& val) {
    bluetoothSignalStrength = val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] BSS Bluetooth signal strength updated: %d", bluetoothSignalStrength);
    }
}

void PowerAmpChannel::handleIpAddress_IPA(const String& val) {
    IPAddress = val;
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] IPA IP address updated: %s", IPAddress.c_str());
    }
}

void PowerAmpChannel::handleLocalTime_TME(const String& val) {
    localTime = val;
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] TME Local time updated: %s", localTime.c_str());
    }
}

void PowerAmpChannel::handleSource_SRC(const String& val) {
    string_currentSource = val;
    currentSource = sourceStringToInt(val);
    sendSourceStatusKO();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] Source updated: %s (%d)", string_currentSource.c_str(), (int)currentSource);
    }
}

void PowerAmpChannel::handlePlayOrPause_POP(const String& val) {
    playPauseStatus = (bool)val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] POP Play/Pause updated: %d", playPauseStatus);
    }
}

void PowerAmpChannel::handleStop_STP(const String& val) {
    stopStatus = (bool)val.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] STP Stop updated: %d", stopStatus);
    }
}

void PowerAmpChannel:: handleNext_NXT(const String& v) {
    nextStatus = (bool)v.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] NXT Next updated: %d", nextStatus);
    }
}

void PowerAmpChannel:: handlePrevious_PRE(const String& v) {
    previousStatus = (bool)v.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] PRE Previous updated: %d", previousStatus);   
    }
}

void PowerAmpChannel:: handlePreset_PST(const String& v) {
    presetStatus = v.toInt();
    KoAMP_ChPresetStatus.value(presetStatus, DPT_DecimalFactor);
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] PST Preset updated: %d", presetStatus);
    }
}

void PowerAmpChannel:: handleLoopMode_LPM(const String& v) {
    loopModeStatus = (bool)v.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] LPM LoopMode updated: %d", loopModeStatus);
    }
}

void PowerAmpChannel:: handleBluetooth_BTC(const String& v) {
    bluetoothStatus = (bool)v.toInt();
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] BTC Bluetooth updated: %d", bluetoothStatus);
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
    muteStatus_MUT = (bool)val.toInt();
    KoAMP_ChMuteStatus.value(muteStatus_MUT, DPT_Switch);
    if (openknxPowerAmpModule.debug()) {
        logDebugP("[INFO] Mute updated: %d", muteStatus_MUT);
    }
}

void PowerAmpChannel::handleDeviceStatusSummary_STA(const String& val) {
    processSTACommand(val);
}

void PowerAmpChannel::handleTitle_TIT(const String& val) {
    if (songMetadataTitle == val) return;   // nichts geändert -> nichts senden
    songMetadataTitle = val;
    KoAMP_ChSongTitle.value(songMetadataTitle.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Title updated: %s", val.c_str());
}

void PowerAmpChannel::handleArtist_ART(const String& val) {
    if (songMetadataArtist == val) return;   // nichts geändert -> nichts senden
    songMetadataArtist = val;
    KoAMP_ChSongArtist.value(songMetadataArtist.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Artist updated: %s", val.c_str());
}

void PowerAmpChannel::handleAlbum_ALB(const String& val) {
    if (songMetadataAlbum == val) return;   // nichts geändert -> nichts senden
    songMetadataAlbum = val;
    KoAMP_ChSongAlbum.value(songMetadataAlbum.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Album updated: %s", val.c_str());
}

void PowerAmpChannel::handleVendor_VND(const String& val) {
    if (songMetadataVendor == val) return;   // nichts geändert -> nichts senden
    songMetadataVendor = val;
    KoAMP_ChSongVendor.value(songMetadataVendor.c_str(), DPT_String_8859_1);
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Vendor updated: %s", val.c_str());
}

void PowerAmpChannel::handleLed_LED(const String& val) {
    ledStatus = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] LED updated: %d", ledStatus);
}

void PowerAmpChannel::handleVersion_VER(const String& val) {
    //firmware version, and the {version} will contain the version number, short git commit, and API level, connected with -.
    //Beispiel: VER:36-30cb0ae0-6
    firmwareVersion = val;
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Version updated: %s", firmwareVersion.c_str());
}

void PowerAmpChannel::handleVirtualBass_VBS(const String& val) {
    virtualBassEnabled = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] VirtualBass updated: %d", virtualBassEnabled);
}

void PowerAmpChannel::handleBalance_BAL(const String& val) {
    balanceSetting = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Balance updated: %d", balanceSetting);
}       

void PowerAmpChannel::handleVolumeFixedOutput_VOF(const String& val) {
    volumeFixedOutput = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Volume Fixed Output updated: %d", volumeFixedOutput);
}

void PowerAmpChannel::handleVolumeGroupedPlayback_VOG(const String& val) {
    volumeGroupedPlayback = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Volume Grouped Playback updated: %d", volumeGroupedPlayback);
}

void PowerAmpChannel::handleQuerySystemEQGroup_PEQ(const String& val) {
    querySystemEQGroup = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Query System EQ Group updated: %d", querySystemEQGroup);
}

void PowerAmpChannel::handleEQGroup_EQS(const String& val) {
    eqGroup = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] EQ Group updated: %d", eqGroup);
}

void PowerAmpChannel::handleVolumeStep_VST(const String& val) {
    volumeStep = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Volume Step updated: %d", volumeStep);
}

void PowerAmpChannel::handleEnableEQ_EQE(const String& val) {
    enableEQ = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Enable EQ updated: %d", enableEQ);
}

void PowerAmpChannel::handleCrossfilter_CFE(const String& val) {
    crossfilter = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Crossfilter updated: %d", crossfilter);
}

void PowerAmpChannel::handleCrossfilterFrequencyPoint_CFF(const String& val) {
    crossfilterFrequencyPoint = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Crossfilter Frequency Point updated: %d", crossfilterFrequencyPoint);
}

void PowerAmpChannel::handleBeep_BEP(const String& val) {
    beepEnabled_BEP = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Beep updated: %d", beepEnabled_BEP);
}

void PowerAmpChannel::handleAutoplay_APL(const String& val) {
    autoplayStatus_APL = val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Autoplay updated: %d", autoplayStatus_APL);
}

void PowerAmpChannel::handleAudioOutput_AUD(const String& val) {
    audioOutput_AUD = (bool)val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] AUD Audio output updated: %d", audioOutput_AUD);
}

void PowerAmpChannel::handleNetworkPlayingStatus_PLA(const String& val) {
    playingStatus_PLA = (bool)val.toInt();
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] PLA Playing status updated: %d", playingStatus_PLA);
}

void PowerAmpChannel::handleElapsed_ELP(const String& val) {
    elapsedTime_ELP = val;
    KoAMP_ChElapsedTime.value(elapsedTime_ELP.c_str(), DPT_String_8859_1); // Update the KO with the elapsed time
    if (openknxPowerAmpModule.debug()) logDebugP("[INFO] Elapsed updated: %s", val.c_str());
}

void PowerAmpChannel::handlePlaylist_PLI(const String& val) {
    /*
    query current track index and number of playlist, 
    {playlist_info} will be in this format index/count, 
    and index is start from 1. eg: 1/23 means now playing the first song in playlist contains 23 songs in total.
    */
    playlistInfo_PLI = val;
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

void PowerAmpChannel::handleChannel_CHN(const String& val) {
    /*
    {channel} 	description
    S 	stereo mode
    L 	left channel
    R 	right channel
    */

    if (val == "S") 
    {
        channelMode = "STEREO";
    } 
    else if (val == "L") {
        channelMode = "LEFT";
    } 
    else if (val == "R") {
        channelMode = "RIGHT";
    } 
    else 
    {
        logDebugP("[ERROR] Unbekannter CHN Wert: %s", val.c_str());
        return;
    }
    
    if (openknxPowerAmpModule.debug()) 
    {
        logDebugP("[INFO] CHN Channel mode updated: %s", channelMode.c_str());
    }
}

// ---------------- Alive Handling ----------------
void PowerAmpChannel::updateAlive() 
{
    lastResponseMillis_Alive = millis();
    if (!deviceAlive) {
        deviceAlive = true;
        logInfoP("[ALIVE] AMP antwortet!");

        // Nach Alive-Wiederkehr ggf. Autoplay triggern
        if (autoPlayEnabled) {
            onetimeAutoPlayExecuted = false;
            if (openknxPowerAmpModule.debug()) {
                logDebugP("[AUTO] Alive erkannt - Custom Autoplay erneut erlaubt");
            }
        }
        // Nach Alive-Wiederkehr ggf. Automute triggern 
         if (autoMuteEnabled) {
            onetimeAutoMuteExecuted = false;
            autoMutePending = false; 
            if (openknxPowerAmpModule.debug()) {
                logDebugP("[AUTO] Alive erkannt - Custom Automute erneut erlaubt");
            }
        }
    }
}

void PowerAmpChannel::checkAliveStatus()
{
    unsigned long currentMillis = millis();
  
    // Wenn Gerät als alive markiert ist, aber zu lange keine Antwort kam → DEAD
    if (deviceAlive && (currentMillis - lastResponseMillis_Alive > alive_timeout))
    {
        deviceAlive = false;
        // lastAliveState hier NICHT setzen – der Change-Check im Interval-Block
        // soll den Übergang alive→dead erkennen und den Log ausgeben.
        resetStatiInfos();
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
    stop_STP();
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

    if ((ParamAMP_DayNight == 1 && value == 0) || (ParamAMP_DayNight == 2 && value == 1))
        return night();

    return day();
}

void PowerAmpChannel::setDefaultVolume()
{
    // Dont set during playing
    if (playingStatus_PLA == true) return;

    // select _currentDefaultVolume
    if (_currentNight)
        currentVolume = ParamAMP_VolumeNight;
    else
        currentVolume = ParamAMP_VolumeDay;

    // update 
    setVolume_VOL(currentVolume);
}

void PowerAmpChannel::processInputKoLock(GroupObject &ko)
{
    bool value = ko.value(DPT_Switch);

    if ((ParamAMP_Lock == 1 && value == 1) || (ParamAMP_Lock == 2 && value == 0))
        return lock();

    return unlock();
}

void PowerAmpChannel::processInputKoScene(GroupObject &ko)
{
    if (!ParamAMP_ChScenesActive)
        return;

    uint8_t Szenennummer = ko.value(DPT_SceneNumber);
    Szenennummer += 1; // DPT_SceneNumber ist 0-basiert, ETS-Szenen sind 1-basiert
    logDebugP("processInputKoScene: Szenennummer %u", Szenennummer);

    for (uint8_t i = 0; i < 9; i++)
    {
        // sceneBlocks[i].scene enthält den Parameteroffset für ParamAMP_ChSceneX,
        // der vom OpenKNXproducer generiert wurde – knx.paramByte() ist hier korrekt,
        // da die generierten ParamAMP_ChSceneX-Makros selbst nur den Offset kapseln.
        uint8_t sceneId = knx.paramByte(sceneBlocks[i].scene);

        if (sceneId == 0) continue;             // "Nicht genutzt"
        if (sceneId != Szenennummer) continue;  // nicht die gesuchte Szene

        uint8_t sceneQuelle = knx.paramByte(sceneBlocks[i].quelle);
        uint8_t sceneVolume = knx.paramByte(sceneBlocks[i].volume);

        logDebugP("Szene %u gefunden: Quelle=%u, Volume=%u", Szenennummer, sceneQuelle, sceneVolume);

        currentSource = static_cast<enumSource>(sceneQuelle);
        setSource_SRC(currentSource);
        setVolume_VOL(sceneVolume);
        return;
    }
    logDebugP("Keine passende Szene für Nummer %u", Szenennummer);
}

void PowerAmpChannel::setKOInitialValues(void)
{
    String empty = "";
    // Initialwerte für KOs setzen
    KoAMP_ChVolumeStatus.value(currentVolume, DPT_Scaling);
    KoAMP_ChSourceStatus.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChMuteStatus.value(muteStatus_MUT, DPT_Switch);
    KoAMP_ChAliveStatus.value(deviceAlive, DPT_Switch);
    KoAMP_ChLock.value(_currentLocked, DPT_Switch);
    KoAMP_ChSongTitle.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongArtist.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongAlbum.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongVendor.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChElapsedTime.value(empty.c_str(), DPT_String_8859_1);

    if (openknxPowerAmpModule.debug())
    {
        logDebugP("[INIT] KO Initial Values gesetzt");
    }
}

void PowerAmpChannel::handleCustomAutoplay()
{
    // Abbruch: bereits gestartet oder nicht relevant
    if (!autoPlayEnabled || onetimeAutoPlayExecuted) return;
    if (!deviceAlive) return;

    // Abbruch: läuft schon
    if (autoPlayPending && playingStatus_PLA)
    {
        autoPlayPending = false;
        onetimeAutoPlayExecuted = true;
        logInfoP("[AUTO] Already playing, autoplay cancelled");
        return;
    }

    // Pending starten
    if (!autoPlayPending)
    {
        logInfoP("[AUTO] Starte Wiedergabe in 3 Sekunden");
        autoPlayStartTime = millis();
        autoPlayPending = true;
        return; // nächster loop()-Tick prüft den Timer
    }

    // Warten auf Bedingungen: Internet + Quelle NET + Timer abgelaufen
    if (internetStatus && string_currentSource == "NET" &&
        (millis() - autoPlayStartTime >= AUTOPLAY_DELAY))
    {
        setAutoplay_APL(true);  // APL erst setzen wenn wir auch wirklich spielen
        playPause_POP();
        onetimeAutoPlayExecuted = true;
        autoPlayPending = false;
        logInfoP("[AUTO] Wiedergabe automatisch gestartet");
    }
}

void PowerAmpChannel::handleCustomAutoMute()
{
    if (!deviceAlive || !autoMuteEnabled || onetimeAutoMuteExecuted) return;

    if (!autoMutePending)
    {
        autoMutePending = true;
        autoMuteStartTime = millis();
        logInfoP("[AUTOMUTE] Mute geplant in %lums", AUTOMUTE_DELAY);
        return;
    }

    if (millis() - autoMuteStartTime >= AUTOMUTE_DELAY)
    {
        setMute_MUT(true);
        onetimeAutoMuteExecuted = true;
        autoMutePending = false;
        logInfoP("[AUTOMUTE] Mute ausgeführt");
    }
}

void PowerAmpChannel::resetStatiInfos()
{
    logInfoP("[dead] reset Stati");
    currentVolume = 0;
    string_currentSource = "";
    elapsedTime_ELP = ""; 
    songMetadataVendor ="";
    songMetadataAlbum ="";
    songMetadataArtist ="";
    songMetadataTitle ="";

    String empty = "";
    KoAMP_ChVolumeStatus.value((uint8_t)0, DPT_Scaling);
    KoAMP_ChSourceStatus.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongTitle.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongArtist.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongAlbum.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChSongVendor.value(empty.c_str(), DPT_String_8859_1);
    KoAMP_ChElapsedTime.value(empty.c_str(), DPT_String_8859_1);
}