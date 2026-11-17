#pragma once
#include "OpenKNX.h"
#include <SoftwareSerial.h>


enum class enumSource : uint8_t {
    Network = 0,    //NET
    Bluetooth,      // BT
    USB,            // USB
    LineIn,         // LINE-IN
    Optical,        // OPT
    Coaxial,        // COAX
    USBDAC,         // USBDAC
    Error = 99
};

class PowerAmpChannel : public OpenKNX::Channel
{
private:
    Stream* mySerial = nullptr;

    bool _channelActive = false; // is enabled in ETS?

    void setSource_SRC(enumSource sourcenumber);    // SRC
    void setSource_SRC(const String &source); // SRC

    void getSource_SRC(void);

    void playPause_POP();                                // POP play or pause, available in network playback and bluetooth
    void stop_STP();                                     // STP stop, available only in network playback
    void next_NXT();                                     // NXT next track, available in network playback and bluetooth
    void previous_PRE();                                 // PRE previous track, available in network playback and bluetooth
    void startAndPlayPresetPlaylist_PST(uint8_t presetNum);                  // start to play preset playlist
    void setLoopShuffleMode_LPM(const String &loopmode); // LPM[:{loopmode}]   set/get loop and shuffle mode, available in network playback.




    void getDeviceStatus_STA(void); // get device status, available in network playback and bluetooth

    void setAutoplay_APL(bool onoff); // AUTOPLAY[:{onoff}] set autoplay
    void getAutoplay_APL(void);

    void getMetadataTitle_TIT(void);
    void getMetadataArtist_ART(void);
    void getMetadataAlbum_ALB(void);
    void getMetadataVendor_VND(void);

    //  Variablen für Lautstärke und Quelle
    uint8_t currentVolume = 0;
    uint8_t currentVolumeStepValue = 5; // Schrittweite für Lautstärkeänderung
    uint8_t currentVolumeLimit = 100;
    uint8_t _volumeStatusSent = 255;    // zuletzt auf den Status-KO geschriebener Wert (255 = noch keiner)
    void processReportedVolume(uint8_t reportedVolume); // vom Verstärker gemeldete Lautstärke verarbeiten

    /*//uint currentSource = PT_Source_network;*/
    enumSource currentSource = enumSource::Network;
    String string_currentSource = "NET";
    bool muteStatus_MUT = false; // Speichert den MUTE-Zustand
    bool beepEnabled_BEP = false;
    bool virtualBassEnabled = false;
    bool bluetoothConnected = false;
    uint8_t currentBassTone = 0;
    uint8_t currentTrebleTone = 0;
    uint8_t currentMidTone = 0;
    bool netStatus = false;
    bool internetStatus = false;
    bool playingStatus_PLA = false;
    bool playPauseStatus = false;
    bool stopStatus = false;
    bool nextStatus = false;
    bool previousStatus = false;
    uint8_t presetStatus = 0;
    String loopModeStatus = "";
    bool bluetoothStatus = false;
    bool ledStatus = false;
    bool upgradingStatus = false;
    bool autoplayStatus_APL = true;
    bool audioOutput_AUD = false;
    String channelMode = "";
    String elapsedTime_ELP = ""; // Elapsed time in ms
    String playlistInfo_PLI = ""; // Playlist info in format index/count, e.g. 1
    String songMetadataVendor ="";
    String songMetadataAlbum ="";
    String songMetadataArtist ="";
    String songMetadataTitle ="";
    String powerStatus = "UNKNOWN";
    String firmwareVersion = "";
    String localTime    = "";
    String IPAddress   = "";
    uint8_t bluetoothSignalStrength = 0;
    uint8_t wifiSignalStrength  = 0;
    uint8_t triggerWifiSetup = 0;
    uint8_t ethernetStatus = 0;
    String deviceName = "";

    uint8_t crossfilterFrequencyPoint = 0;
    uint8_t crossfilter = 0;        
    uint8_t enableEQ = 0;
    uint8_t volumeStep = 0;
    uint8_t eqGroup = 0;
    uint8_t querySystemEQGroup = 0;
    uint8_t volumeGroupedPlayback = 0;
    uint8_t volumeFixedOutput = 0;
    uint8_t balanceSetting = 0;
    uint8_t wifiStatus = 0;

    bool lastAliveState = false;

    static constexpr uint8_t AMP_SCENE_COUNT = 9;
    static constexpr uint8_t AMP_SCENE_SOURCE_NO_CHANGE = 255; // PT-SceneSource "Keine Änderung"
    struct SceneParams {
        uint16_t scene;
        uint16_t quelle;
        uint16_t volume;
        uint16_t mute; 
        uint16_t preset;
    };

    // Szenenblöcke in einer Lookup-Tabelle.
    // Enthält die RELATIVEN Parameter-Indizes (AMP_ChSceneX, nicht ParamAMP_ChSceneX!).
    // Die absolute Adresse wird erst zur Laufzeit per AMP_ParamCalcIndex() mit dem
    // richtigen _channelIndex berechnet (Muster wie OFM-PresenceModule / OFM-LogicModule).
    static constexpr SceneParams sceneBlocks[AMP_SCENE_COUNT] = {
        { AMP_ChScene0, AMP_ChSceneQuelle0, AMP_ChSceneVolume0, AMP_ChSceneMute0, AMP_ChScenePreset0 },
        { AMP_ChScene1, AMP_ChSceneQuelle1, AMP_ChSceneVolume1, AMP_ChSceneMute1, AMP_ChScenePreset1 },
        { AMP_ChScene2, AMP_ChSceneQuelle2, AMP_ChSceneVolume2, AMP_ChSceneMute2, AMP_ChScenePreset2 },
        { AMP_ChScene3, AMP_ChSceneQuelle3, AMP_ChSceneVolume3, AMP_ChSceneMute3, AMP_ChScenePreset3 },
        { AMP_ChScene4, AMP_ChSceneQuelle4, AMP_ChSceneVolume4, AMP_ChSceneMute4, AMP_ChScenePreset4 },
        { AMP_ChScene5, AMP_ChSceneQuelle5, AMP_ChSceneVolume5, AMP_ChSceneMute5, AMP_ChScenePreset5 },
        { AMP_ChScene6, AMP_ChSceneQuelle6, AMP_ChSceneVolume6, AMP_ChSceneMute6, AMP_ChScenePreset6 },
        { AMP_ChScene7, AMP_ChSceneQuelle7, AMP_ChSceneVolume7, AMP_ChSceneMute7, AMP_ChScenePreset7 },
        { AMP_ChScene8, AMP_ChSceneQuelle8, AMP_ChSceneVolume8, AMP_ChSceneMute8, AMP_ChScenePreset8 },
    };

    // KO-Wert setzen: vor Ablauf der OpenKNX-Startverzögerung nur setzen (valueNoSend),
    // danach normal senden (Muster wie OFM-DFA / OFM-LogicModule).
    void setKoValue(GroupObject &ko, const KNXValue &value, const Dpt &type);

    // Private Methode zur Verarbeitung von empfangenen Zeilen
    void handleIncomingData(void);
    void processReceivedUARTCommand(const String &commandType, const String &commandValue);
    void sendRawCommandToArylic(const String &command);
    void processSTACommand(const String &commandValue);
    enumSource sourceStringToInt(const String &source);

    void sendVolumeStatusKO(void);
    void sendSourceStatusKO(void);

    // --- Zeitsteuerung ---
    unsigned long lastTriggerTime = 0;  // Wann zuletzt die 60s-Phase gestartet wurde
    unsigned long lastStateTime = 0;    // Wann zuletzt der nächste State aufgerufen wurde

    unsigned long START_INTERVAL = 30000;  // 30 Sekunden
    unsigned long STATE_INTERVAL = 1000;   // 1 Sekunde zwischen States

    // --- State Machine ---
    int currentState = -1;  // -1 bedeutet "wartet auf nächsten Start"


    unsigned long AUTOPLAY_DELAY = 5000;   // ms Wartezeit vor automatischer Wiedergabe
    bool autoPlayPending = false;
    unsigned long autoPlayStartTime = 0;
    // POP schaltet Play/Pause um -> vor dem POP den Status (STA) abfragen und die Antwort abwarten
    bool autoPlayStatusQueried = false;
    unsigned long autoPlayQueryTime = 0;
    const unsigned long AUTOPLAY_STATUS_WAIT = 1000; // ms Wartezeit auf die STA-Antwort


    /* Handler für UART Kommandos von Arylic*/
    using HandlerFn = std::function<void(const String&)>;
    std::map<String, HandlerFn> commandHandlers;

    void initHandlers();

        // einzelne Handler
        void handleDeviceStatusSummary_STA(const String &val);
        void handleSystemOperations_SYS(const String &val);
        void handleInternetStatus_WWW(const String &val);
        void handleDeviceName_NAM(const String &val);
        void handleEthernetStatus_ETH(const String &val);
        void handleWifiStatus_WIF(const String &val);
        void handleTriggerWifiSetup_WRS(const String &val);
        void handleWifiSignalStrength_WSS(const String &val);
        void handleBluetoothSignalStrength_BSS(const String &val);
        void handleIpAddress_IPA(const String &val);
        void handleLocalTime_TME(const String &val);
        void handleEnablePinCodeBT_COE(const String &val);
        void handlePinCodeBT_COD(const String &val);
        void handleSource_SRC(const String &val);
        void handlePlayOrPause_POP(const String &val);
        void handleStop_STP(const String &val);
        void handleNext_NXT(const String &val);
        void handlePrevious_PRE(const String &val);
        void handlePreset_PST(const String &val);
        void handleLoopMode_LPM(const String &val);
        void handleBluetooth_BTC(const String &val);
        void handleNetworkPlayingStatus_PLA(const String &val);
        void handleChannel_CHN(const String &val);
        void handleMultiRoomMode_MRM(const String &val);
        void handleTitle_TIT(const String &val);
        void handleArtist_ART(const String &val);
        void handleAlbum_ALB(const String &val);
        void handleVendor_VND(const String &val);
        void handleElapsed_ELP(const String &val);
        void handlePlaylist_PLI(const String &val);
        void handleAutoplay_APL(const String &val);
        void handleAudioOutput_AUD(const String &val);
        void handleVolume_VOL(const String &val);
        void handleMute_MUT(const String &val);
        void handleBass_BAS(const String &val);
        void handleTreble_TRE(const String &val);
        void handleMid_MID(const String &val);
        void handleVirtualBass_VBS(const String &val);
        void handleBalance_BAL(const String &val);
        void handleVolumeFixedOutput_VOF(const String &val);
        void handleVolumeGroupedPlayback_VOG(const String &val);    
        void handleQuerySystemEQGroup_PEQ(const String &val);
        void handleEQGroup_EQS(const String &val);
        void handleVolumeStep_VST(const String &val);
        void handleEnableEQ_EQE(const String &val);
        void handleCrossfilter_CFE(const String &val);
        void handleCrossfilterFrequencyPoint_CFF(const String &val);
        void handleVersion_VER(const String &val);
        void handleLed_LED(const String &val);
        void handleBeep_BEP(const String &val);
        void handlePromptVoice_PMT(const String &val);
        void handleDelayTimeToAutoMute_DLY(const String &val);
        void handleMaxVolume_MXV(const String &val);
        void handleAutoSwitchMode_ASW(const String &val);
        void handlePowerOnMode_POM(const String &val);
        void handleVolumeSyncFeature_VOS(const String &val);
        void handleListSources_LST(const String &val);
        void handleStandbyOnPower_SOP(const String &val);

        String hexStringToAsciiString(const String &hexString);

    // Alive-Monitoring
    unsigned long lastResponseMillis_Alive = 0;
    unsigned long lastAliveMillis_Alive = 0;
    bool deviceAlive = false;
    const unsigned long alive_timeout = (START_INTERVAL*2); // Timeout in ms
    void updateAlive(void);


    // --- Startup Preset ---
    bool startupPresetPending = false;
    unsigned long startupPresetStartTime = 0;
    const unsigned long startupPreset_DELAY = 500; // 500ms nach Alive-Erkennung
    
    bool onetimeStartupPresetExecuted = false;   // verhindert mehrfachen Start
    bool startupPresetEnabled = true;  // ETS-Parameter: nach Alive-Erkennung Preset oder Szene starten?
    void handleStartupPreset(void);

    // --- Automute ---
    bool autoMutePending = false;
    unsigned long autoMuteStartTime = 0;
    const unsigned long AUTOMUTE_DELAY = 200; // 200ms nach Alive-Erkennung

    bool autoMuteEnabled = true;  // nach Neustart immer ETS-Wert (ParamAMP_AutoMute), per KO bis zum nächsten Neustart änderbar
    bool onetimeAutoMuteExecuted = false;   // verhindert mehrfachen Start
    void handleCustomAutoMute(void);

    // --- Autoplay-System ---
    bool autoPlayEnabled = false;  // nach Neustart immer ETS-Wert (ParamAMP_AutoPlay), per KO bis zum nächsten Neustart änderbar
    bool onetimeAutoPlayExecuted = false;   // verhindert mehrfachen Start
    void handleCustomAutoplay(void);

    bool _currentNight = false;
    bool _currentLocked = false;
    // true (mit Log), wenn die Sperre aktiv ist und Bedienbefehle ignoriert werden sollen
    bool isLockedFor(const char *action);
    void processInputKoDayNight(GroupObject &ko);
    void processInputKoLock(GroupObject &ko);
    void processInputKoScene(GroupObject &ko);
    bool applyScene(uint8_t sceneNumber);
    void day(void);
    void night(void);
    void setDefaultVolume(void);
    void unlock();
    void lock();
    void setKOInitialValues(void);
    void resetStatiInfos(void);
    
    // -- fuer handleIncomingData
    String uartBuffer = "";
    unsigned long lastReceiveTime = 0;
    uint32_t uartErrorCount = 0;

public:
    PowerAmpChannel(uint8_t iChannelNumber, Stream* serialStream = nullptr);
    ~PowerAmpChannel();

    void setSerial(Stream* serialStream);
    const std::string name() override;
    void setup(bool configured) override;
    void loop() override;
    void processInputKo(GroupObject &ko) override;
    void processAfterStartupDelay(); // called by PowerAmpModule::processAfterStartupDelay()

    void setVolume_VOL(uint8_t volume);
    void getVolume_VOL(void);
    void setMute_MUT(bool onoff);
    bool getMute_MUT(void);

    bool isActive();
        
    // Alive-Handling um zu prüfen ob der Endstufe noch oder überhaupt schon da ist.
    void checkAliveStatus();   // regelmäßig in loop() aufrufen

    void save();     // called by PowerAmpModule::writeFlash()
    void restore();  // called by PowerAmpModule::readFlash()

};
