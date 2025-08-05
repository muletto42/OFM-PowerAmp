#pragma once
#include "OpenKNX.h"
#include <SoftwareSerial.h>



// #define PT_Source_network 0
// #define PT_Source_bluetooth 1
// #define PT_Source_USBDAC 2
// #define PT_Source_linein 3
// #define PT_Source_Optical 4
// #define PT_Source_Coaxial 5
// #define PT_Source_ERROR 99

enum class enumSource : uint8_t {
    Network = 0,
    Bluetooth,
    USBDAC,
    LineIn,
    Optical,
    Coaxial,
    Error = 99
};

class PowerAmpChannel : public OpenKNX::Channel
{
private:
    Stream *mySerial = nullptr; // Hardwareserial
    #if OPENKNX_AMP_CHANNEL_COUNT > 1
    SoftwareSerial *mySWSerial = nullptr; // SoftwareSerial für Kanäle >1
    #endif  

    // is enabled in ETS?
    bool _channelActive = false;

    void setSource(enumSource sourcenumber);    // SRC
    void setSource(const String &source); // SRC
    /*
    {source} 	description
    NET 	    network
    BT 	    bluetooth
    USBDAC 	USB DAC
    LINE-IN 	line-in
    OPT 	    Optical
    COAX 	    Coaxial
    */
    void getSource(void);

    void playPause();                                // POP play or pause, available in network playback and bluetooth
    void stop();                                     // STP stop, available only in network playback
    void next();                                     // NXT next track, available in network playback and bluetooth
    void previous();                                 // PRE previous track, available in network playback and bluetooth
    void playPreset(int presetNum);                  // start to play preset playlist
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

    String getMetadataTitle(void);
    String getMetadataArtist(void);
    String getMetadataAlbum(void);
    String getMetadataVendor(void);

    //  Variablen für Lautstärke und Quelle
    int currentVolume = 0;
    /*//uint currentSource = PT_Source_network;*/
    enumSource currentSource = enumSource::Network;
    String string_currentSource = "NET";
    bool muteStatus = false; // Speichert den MUTE-Zustand
    bool beepEnabled = false;
    bool virtualBassEnabled = false;
    bool bluetoothConnected = false;
    int currentBassTone = 0;
    int currentTrebleTone = 0;
    int currentMidTone = 0;
    bool netStatus = false;
    bool internetStatus = false;
    bool playingStatus = false;
    bool ledStatus = false;
    bool upgradingStatus = false;
    bool autoplayStatus = false;
    String songMetadataVendor ="";
    String songMetadataAlbum ="";
    String songMetadataArtist ="";
    String songMetadataTitle ="";

    // Private Methode zur Verarbeitung von empfangenen Zeilen
    void handleIncomingData(void);
    void processReceivedUARTCommand(const String commandType, const String commandValue);
    void sendRawCommandToArylic(const String command);
    void processSTACommand(const String commandValue);
    enumSource sourceStringToInt(const String source);

    // // Interne Variablen
    // unsigned long _baud = 115200;
    // int _txPin = ARYLIC_TX_PIN;
    // int _rxPin = ARYLIC_RX_PIN;
    // // SerialUART &_serial;
    // String _recvBuffer;

public:
    PowerAmpChannel(uint8_t iChannelNumber);
    ~PowerAmpChannel();

    const std::string name() override;
    void setup(bool configured) override;
    void loop(bool configured) override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    void setVolume(int volume);
    void getVolume(void);
    void setMute(bool onoff);
    bool getMute(void);

    void save();
    void restore();
};
