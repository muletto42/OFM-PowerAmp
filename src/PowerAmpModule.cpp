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

#include "PowerAmpModule.h"
#include "OpenKNX.h"
#include "ModuleVersionCheck.h"
#include <SoftwareSerial.h>

PowerAmpModule openknxPowerAmpModule;

PowerAmpModule::PowerAmpModule()
{
}

PowerAmpModule::~PowerAmpModule()
{
}

const std::string PowerAmpModule::name()
{
    return "PowerAmp";
}

const std::string PowerAmpModule::version()
{
    return MODULE_PowerAmp_Version;
}

void PowerAmpModule::loop()
{
    for (uint8_t i = 0; i < MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT); i++)
        channel[i]->loop();
}

void PowerAmpModule::setup()
{
    for (uint8_t i = 0; i < OPENKNX_AMP_CHANNEL_COUNT; i++)
    {
        channel[i] = new PowerAmpChannel(i);
        channel[i]->setup();
    }
}

// will be called once a KO received a telegram
void PowerAmpModule::processInputKo(GroupObject &iKo)
{
    // if (iKo.asap() != SWA_KoCentralFunction &&
    //     (iKo.asap() < SWA_KoBlockOffset ||
    //      iKo.asap() > SWA_KoBlockOffset + ParamSWA_VisibleChannels * SWA_KoBlockSize - 1))
    //     return;

    logDebugP("processInputKo");
    logIndentUp();

    for (uint8_t i = 0; i < MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT); i++)
        channel[i]->processInputKo(iKo);

    logIndentDown();
}


