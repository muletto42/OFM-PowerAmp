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

void PowerAmpModule::loop(bool configured)
{
    for (uint8_t i = 0; i < MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT); i++)
        channel[i]->loop(configured);
}

void PowerAmpModule::setup(bool configured)
{
    for (uint8_t i = 0; i < OPENKNX_AMP_CHANNEL_COUNT; i++)
    {
        channel[i] = new PowerAmpChannel(i);
        channel[i]->setup(configured);
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

void PowerAmpModule::showHelp()
{
    openknx.console.printHelpLine("amp debug", "enable/disable debug mode");
    openknx.console.printHelpLine("N",  "N is the channel number");
    openknx.console.printHelpLine("amp volume N [Value]",  "Set the volume level: eg: [amp volume 1 50] to set channel 1 to 50% volume");
    openknx.console.printHelpLine("amp mute N [0/1]" ,    "Mute the audio output: eg: [amp mute 1 0] to unmute channel 1");
}

bool PowerAmpModule::processCommand(const std::string command, bool diagnose)
{
    uint8_t value = 0;

    if (command.substr(0, 3) == "amp")
    {
        if (!diagnose && command == "amp debug")
        {
            _debug = !_debug;
            logInfoP(_debug ? "AMP Debug enabled" : "AMP Debug disabled");
            return true;
        }

        // ---------- amp volume ----------
        if (command.substr(0, 11) == "amp volume ")
        {
            if (command.length() < 14 || command.length() > 16)
            {
                logInfoP("amp volume command with bad args");
                return true;
            }

            const uint16_t channelIdx = std::stoi(command.substr(12, 1)) - 1;

            if (channelIdx >= OPENKNX_SWA_CHANNEL_COUNT)
            {
                logInfoP("channel index out of range");
                return true;
            }

            if (command.length() == 14)
                value = std::stoi(command.substr(14, 1));
            else if (command.length() == 15)
                value = std::stoi(command.substr(14, 2));
            else if (command.length() == 16)
            {
                value = std::stoi(command.substr(14, 3));
                if (value != 100)
                {
                    logInfoP("Invalid volume value, must be 0-100");
                    return true;
                }
            }

            if (value > 100)
            {
                logInfoP("Volume value out of range (0-100)");
                return true;
            }

            if (channel[channelIdx] == nullptr)
            {
                logInfoP("Channel %d not initialized", channelIdx + 1);
                return true;
            }

            channel[channelIdx]->setVolume(value);
            logInfoP("Set volume of channel %d to %d", channelIdx + 1, value);

            return true;
        }

        // ---------- amp mute ----------
        if (command.substr(0, 9) == "amp mute ")
        {
            if (command.length() != 12) // z.B. "amp mute 1 1"
            {
                logInfoP("amp mute command with bad args");
                return true;
            }

            const uint16_t channelIdx = std::stoi(command.substr(9, 1)) - 1;
            const uint8_t muteValue = std::stoi(command.substr(11, 1));

            if (channelIdx >= OPENKNX_SWA_CHANNEL_COUNT)
            {
                logInfoP("channel index out of range");
                return true;
            }

            if (muteValue != 0 && muteValue != 1)
            {
                logInfoP("Mute value must be 0 or 1");
                return true;
            }

            if (channel[channelIdx] == nullptr)
            {
                logInfoP("Channel %d not initialized", channelIdx + 1);
                return true;
            }

            channel[channelIdx]->setMute(muteValue);
            logInfoP("Set mute of channel %d to %s", channelIdx + 1, muteValue ? "ON" : "OFF");

            return true;
        }
    }

    return false;
}

bool PowerAmpModule::debug()
{
    return _debug;
}