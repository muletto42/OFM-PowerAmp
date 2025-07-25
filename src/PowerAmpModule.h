#pragma once
#include "OpenKNX.h"
#include "ArylicUARTChannel.h"
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


class ArylicUARTModule : public OpenKNX::Module
{
    public:
        ArylicUARTModule();
        ~ArylicUARTModule();
        void processInputKo(GroupObject &ko) override;
        void loop();
        void setup();
        const std::string name() override;
        const std::string version() override;
           
    private:
        ArylicUARTChannel *channel[OPENKNX_AMP_CHANNEL_COUNT];
};

extern ArylicUARTModule openknxArylicUARTModule;