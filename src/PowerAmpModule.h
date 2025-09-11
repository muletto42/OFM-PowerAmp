#pragma once
#include "OpenKNX.h"
#include "PowerAmpChannel.h"
#include "hardware.h"
#include "knxprod.h"

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
    void loop();
    void setup(bool configured);
    const std::string name() override;
    const std::string version() override;
    void setSerialChannelPins(const uint8_t pins[][4], uint8_t numChannels);

  private:
    PowerAmpChannel *channel[OPENKNX_AMP_CHANNEL_COUNT];
    uint8_t NumChannels; // Number of channels defined in knxprod
     uint8_t _numChannels = 0;

    uint8_t _rxPins[OPENKNX_AMP_CHANNEL_COUNT];
    uint8_t _txPins[OPENKNX_AMP_CHANNEL_COUNT];
    bool _isHardware[OPENKNX_AMP_CHANNEL_COUNT];
    uint8_t _hwPort[OPENKNX_AMP_CHANNEL_COUNT];

    std::vector<SoftwareSerial*> _swSerialInstances;

    // Hilfsfunktion für HW-Serial
    SerialUART* getHardwareSerial(uint8_t port);
};

extern PowerAmpModule openknxPowerAmpModule;