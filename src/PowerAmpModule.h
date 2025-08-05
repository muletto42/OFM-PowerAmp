#pragma once
#include "OpenKNX.h"
#include "PowerAmpChannel.h"
#include "hardware.h"
#include "knxprod.h"


#ifdef OPENKNX_SWSERIAL_TXPINS
  const uint8_t SERIAL_TXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {HW_UART_TX_PIN, OPENKNX_SWSERIAL_TXPINS};
  const uint8_t SERIAL_RXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {HW_UART_RX_PIN, OPENKNX_SWSERIAL_RXPINS};
#elif HW_UART_TX_PIN
  const uint8_t SERIAL_TXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {HW_UART_TX_PIN};
  const uint8_t SERIAL_RXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {HW_UART_RX_PIN};
#else
  const uint8_t SERIAL_TXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {};
  const uint8_t SERIAL_RXPINS[OPENKNX_AMP_CHANNEL_COUNT] = {};
#endif


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
    void loop(bool configured);
    void setup(bool configured);
    const std::string name() override;
    const std::string version() override;
           
  private:
    PowerAmpChannel *channel[OPENKNX_AMP_CHANNEL_COUNT];
};

extern PowerAmpModule openknxPowerAmpModule;