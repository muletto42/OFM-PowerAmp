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

    // is enabled in ETS?
    bool _channelActive = false;

    void setSource(enumSource sourcenumber);    // SRC
    void setSource(const String &source); // SRC
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
    void getSource(void);

    void playPause();                                // POP play or pause, available in network playback and bluetooth
    void stop();                                     // STP stop, available only in network playback
    void next();                                     // NXT next track, available in network playback and bluetooth
    void previous();                                 // PRE previous track, available in network playback and bluetooth
    void playPreset(uint8_t presetNum);                  // start to play preset playlist
    void setLoopShuffleMode(const String &loopmode); // LPM[:{loopmode}]   set/get loop and shuffle mode, available in network playback.
    /*
    {loopmode} 	    description
    REPEATALL 	    repeat all in playlist
    REPEATONE 	    repeat track
    REPEATSHUFFLE 	repeat all and shuffle
    SHUFFLE 	    shuffle and stop when all tracks played
    SEQUENCE 	    stop when reach end of playlist
    */



    void getDeviceStatus(void); // get device status, available in network playback and bluetooth

    void setAutoplay(bool onoff); // AUTOPLAY[:{onoff}] set autoplay
    bool getAutoplay(void);

    void getMetadataTitle(void);
    void getMetadataArtist(void);
    void getMetadataAlbum(void);
    void getMetadataVendor(void);

    //  Variablen für Lautstärke und Quelle
    uint8_t currentVolume = 0;
    uint8_t currentVolumeStepValue = 5; // Schrittweite für Lautstärkeänderung
    uint8_t currentVolumeLimit = 100;

    /*//uint currentSource = PT_Source_network;*/
    enumSource currentSource = enumSource::Network;
    String string_currentSource = "NET";
    bool muteStatus = false; // Speichert den MUTE-Zustand
    bool beepEnabled = false;
    bool virtualBassEnabled = false;
    bool bluetoothConnected = false;
    uint8_t currentBassTone = 0;
    uint8_t currentTrebleTone = 0;
    uint8_t currentMidTone = 0;
    bool netStatus = false;
    bool internetStatus = false;
    bool playingStatus = false;
    bool ledStatus = false;
    bool upgradingStatus = false;
    bool autoplayStatus = false;
    String elapsedTime = ""; // Elapsed time in ms
    String playlistInfo = ""; // Playlist info in format index/count, e.g. 1
    String songMetadataVendor ="";
    String songMetadataAlbum ="";
    String songMetadataArtist ="";
    String songMetadataTitle ="";

    struct SceneParams {
        uint16_t scene;
        uint16_t quelle;
        uint16_t volume;
    };

    // Alle 9 Szenenblöcke in einer Lookup-Tabelle
    const SceneParams sceneBlocks[9] = {
        { ParamAMP_ChScene0, ParamAMP_ChSceneQuelle0, ParamAMP_ChSceneVolume0 },
        { ParamAMP_ChScene1, ParamAMP_ChSceneQuelle1, ParamAMP_ChSceneVolume1 },
        { ParamAMP_ChScene2, ParamAMP_ChSceneQuelle2, ParamAMP_ChSceneVolume2 },
        { ParamAMP_ChScene3, ParamAMP_ChSceneQuelle3, ParamAMP_ChSceneVolume3 },
        { ParamAMP_ChScene4, ParamAMP_ChSceneQuelle4, ParamAMP_ChSceneVolume4 },
        { ParamAMP_ChScene5, ParamAMP_ChSceneQuelle5, ParamAMP_ChSceneVolume5 },
        { ParamAMP_ChScene6, ParamAMP_ChSceneQuelle6, ParamAMP_ChSceneVolume6 },
        { ParamAMP_ChScene7, ParamAMP_ChSceneQuelle7, ParamAMP_ChSceneVolume7 },
        { ParamAMP_ChScene8, ParamAMP_ChSceneQuelle8, ParamAMP_ChSceneVolume8 }
    };


    // Private Methode zur Verarbeitung von empfangenen Zeilen
    void handleIncomingData(void);
    void processReceivedUARTCommand(const String commandType, const String commandValue);
    void sendRawCommandToArylic(const String command);
    void processSTACommand(const String commandValue);
    enumSource sourceStringToInt(const String source);

    void sendVolumeStatusKO(void);
    void sendSourceStatusKO(void);

    // // Interne Variablen
    // unsigned long _baud = 115200;
    // int _txPin = ARYLIC_TX_PIN;
    // int _rxPin = ARYLIC_RX_PIN;
    // // SerialUART &_serial;
    // String _recvBuffer;

        // --- Zeitsteuerung ---
    unsigned long lastTriggerTime = 0;  // Wann zuletzt die 60s-Phase gestartet wurde
    unsigned long lastStateTime = 0;    // Wann zuletzt der nächste State aufgerufen wurde

    unsigned long START_INTERVAL = 30000;  // 30 Sekunden
    unsigned long STATE_INTERVAL = 1000;   // 1 Sekunde zwischen States

    // --- State Machine ---
    int currentState = -1;  // -1 bedeutet "wartet auf nächsten Start"

    /* Handler für UART Kommandos von Arylic*/
    using HandlerFn = std::function<void(const String&)>;
    std::map<String, HandlerFn> commandHandlers;

    void initHandlers();

    // einzelne Handler
    void handleSource_SRC(const String& val);
    void handleVolume_VOL(const String& val);
    void handleMute_MUT(const String& val);
    void handleDeviceStatusSummary_STA(const String& val);
    void handleTitle_TIT(const String& val);
    void handleArtist_ART(const String& val);
    void handleAlbum_ALB(const String& val);
    void handleVendor_VND(const String& val);
    void handleLed_LED(const String& val);
    void handleBluetooth_BTC(const String& val);
    void handleVirtualBass_VBS(const String& val);
    void handleBeep_BEP(const String& val);
    void handleAutoplay_APL(const String& val);
    void handlePlaying_PLA(const String& val);
    void handleElapsed_ELP(const String& val);
    void handlePlaylist_PLI(const String& val);
    void handleBass_BAS(const String& val);
    void handleTreble_TRE(const String& val);
    void handleMid_MID(const String& val);


    // Alive-Monitoring
    unsigned long lastResponseMillis = 0;
    unsigned long lastAliveMillis = 0;
    bool deviceAlive = false;
    const unsigned long alive_timeout = (START_INTERVAL*2); // Timeout in ms
    void updateAlive(void);


    bool _currentNight = false;
    bool _currentLocked = false;
    void processInputKoDayNight(GroupObject &ko);
    void processInputKoLock(GroupObject &ko);
    void processInputKoScene(GroupObject &ko);
    void day(void);
    void night(void);
    void setDefaultVolume(void);
    void unlock();
    void lock();

public:
    PowerAmpChannel(uint8_t iChannelNumber, Stream* serialStream = nullptr);
    ~PowerAmpChannel();

    void setSerial(Stream* serialStream);
    const std::string name() override;
    void setup(bool configured) override;
    void loop() override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    void setVolume(uint8_t volume);
    void getVolume(void);
    void setMute(bool onoff);
    bool getMute(void);

    void save();
    void restore();
    bool isActive();
        
    // Alive-Handling um zu prüfen ob der Endstufe noch oder überhaupt schon da ist.
    void checkAliveStatus();   // regelmäßig in loop() aufrufen
};
