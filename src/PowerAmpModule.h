#pragma once
#include "OpenKNX.h"
#include "PowerAmpChannel.h"
#include "hardware.h"
#include "knxprod.h"

#define AMP_FLASH_MAGIC_WORD_LEN 4
#define AMP_FLASH_VERSION        1
class PowerAmpModule : public OpenKNX::Module
{
  protected:
    bool _debug = false;

  public:
    PowerAmpModule();
    ~PowerAmpModule();
    void processInputKo(GroupObject &ko) override;
    void showHelp() override;
    bool processCommand(const std::string command, bool diagnose) override;
    bool debug();
    void loop() override;
    void setup(bool configured) override;
    const std::string name() override;
    const std::string version() override;
    void setSerialChannelPins(const uint8_t pins[][4], uint8_t numChannels);

    // flash handling für gespeicherte Zustaende
    uint16_t flashSize() override;
    void writeFlash() override;
    void readFlash(const uint8_t *iBuffer, const uint16_t iSize) override;

  private:
    PowerAmpChannel *_channels[OPENKNX_AMP_CHANNEL_COUNT] = {}; // init mit // channel[0] = nullptr, // channel[1] = nullptr, usw
    uint8_t _numChannels = 0;

    uint8_t _rxPins[OPENKNX_AMP_CHANNEL_COUNT];
    uint8_t _txPins[OPENKNX_AMP_CHANNEL_COUNT];
    bool _isHardware[OPENKNX_AMP_CHANNEL_COUNT];
    uint8_t _hwPort[OPENKNX_AMP_CHANNEL_COUNT];

    std::vector<SoftwareSerial*> _swSerialInstances;

    // Hilfsfunktion für HW-Serial
    SerialUART* getHardwareSerial(uint8_t port);

    static const uint8_t _magicWord[AMP_FLASH_MAGIC_WORD_LEN];
};

extern PowerAmpModule openknxPowerAmpModule;