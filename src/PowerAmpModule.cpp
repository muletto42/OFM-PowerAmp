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
    // for (uint8_t i = 0; i < OPENKNX_AMP_CHANNEL_COUNT; i++)
    // {
    //     _channels[i] = new PowerAmpChannel(i);  // ohne Serial, nur Platzhalter, damit restore() schon funktioniert
    //     // Serial wird in setup() per _channels[i]->setSerial(serial) nachgereicht
    //     logInfoP("Channel %d: new PowerAmpChannel", i);
    // }
}

PowerAmpModule::~PowerAmpModule()
{
    for (uint8_t i = 0; i < _numChannels; i++)
    {
        delete _channels[i];
    }
    for (auto *sw : _swSerialInstances)
    {
        delete sw;
    }
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
    {
         if (_channels[i] == nullptr) continue;
        _channels[i]->loop();
    }
}
void PowerAmpModule::setup(bool configured)
{
    logInfoP("setup() START");
    
    // Number of available channels is the minimum of configured and available channels
    _numChannels = MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT);
    logInfoP("_numChannels=%d", _numChannels);
    for (uint8_t i = 0; i < _numChannels; i++)
    {
        logInfoP("Channel %d: pin check", i);
        if (_rxPins[i] == 0xFF || _txPins[i] == 0xFF || 
            _rxPins[i] == 0x00 || _txPins[i] == 0x00)
        {
            logErrorP("Channel %d: invalid pins", i);
            continue;
        }

        logInfoP("Channel %d: serial init", i);
        Stream *serial = nullptr;
        if (_isHardware[i])
        {
            SerialUART *hw = getHardwareSerial(_hwPort[i]);
            if (hw)
            {
                hw->setRX(_rxPins[i]);
                hw->setTX(_txPins[i]);
                hw->begin(BAUD_ARLYIC);
                serial = hw;
                logInfoP("Channel %d: HW serial ok", i);
            }
        }
        else
        {
            auto *sw = new SoftwareSerial(_rxPins[i], _txPins[i]);
            sw->begin(BAUD_ARLYIC);
            _swSerialInstances.push_back(sw);
            serial = sw;
            logInfoP("Channel %d: SW serial ok", i);
        }

        logInfoP("Channel %d: new PowerAmpChannel", i);
        _channels[i] = new PowerAmpChannel(i, serial);
        _channels[i]->setup(configured);
        
    }
    logInfoP("setup() DONE");
}

// void PowerAmpModule::setup(bool configured)
// {
//     // Number of available channels is the minimum of configured and available channels
//     _numChannels  = MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT);
//     for (uint8_t i = 0; i < _numChannels ; i++)
//     {
//         // prüfen obs Pins gültig sind
//         if (_rxPins[i] == 0xFF || _txPins[i] == 0xFF || _rxPins[i] == 0x00 || _txPins[i] == 0x00)
//         {
//             logErrorP("Channel %u: invalid RX/TX pin configuration (RX=%d, TX=%d)", i, _rxPins[i], _txPins[i]);
//             continue;
//         }
//         Stream *serial = nullptr;
//         if (_isHardware[i])
//         {
//             SerialUART *hw = getHardwareSerial(_hwPort[i]);
//             //HardwareSerial *hw = &Serial2
//             if (hw)
//             {
//                 hw->setRX(_rxPins[i]);
//                 hw->setTX(_txPins[i]);
//                 hw->begin(BAUD_ARLYIC);
//                 serial = hw;
//                 logInfoP("Channel %u: HardwareSerial%d RX=%d TX=%d", i, _hwPort[i], _rxPins[i], _txPins[i]);
//             }
//             else
//             {
//                 logErrorP("Channel %u: invalid HardwareSerial port %d (RP2040 supports only 1=Serial1, 2=Serial2)", i, _hwPort[i]);
//                 continue;
//             }
//         }
//         else
//         {
//             auto *sw = new SoftwareSerial(_rxPins[i], _txPins[i]);
//             sw->begin(BAUD_ARLYIC);
//             _swSerialInstances.push_back(sw);
//             serial = sw;
//             logInfoP("Channel %u: SoftwareSerial RX=%d TX=%d", i, _rxPins[i], _txPins[i]);
//         }

//         // setup() statt _channels[i] = new PowerAmpChannel(i, serial): // alt wird im Konstruktor gemacht, damit man aus dem Flash lesen kann
//         _channels[i]->setSerial(serial);
//         _channels[i]->setup(configured);
//     }

//     if (_flashCacheValid)
//     {
//         for (uint8_t i = 0; i < OPENKNX_AMP_CHANNEL_COUNT; i++)
//             if (_channels[i] != nullptr)
//                 _channels[i]->restoreFromByte(_flashCache[i]);
//     }
// }

// will be called once a KO received a telegram
void PowerAmpModule::processInputKo(GroupObject &iKo)
{
    logDebugP("[Modul] processInputKo");
    logIndentUp();

    for (uint8_t i = 0; i < MIN(ParamAMP_VisibleChannels, OPENKNX_AMP_CHANNEL_COUNT); i++)
    {
        if (_channels[i] == nullptr) continue;
        logDebugP("_channels[ %i ]", i+1);
        _channels[i]->processInputKo(iKo); 
    }
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
            logDebugP(_debug ? "AMP Debug enabled" : "AMP Debug disabled");
            return true;
        }

        // ---------- amp volume ----------
        if (command.substr(0, 11) == "amp volume ")
        {
            logDebugP("amp volume Befehl");
            if (command.length() < 14 || command.length() > 16)
            {
                logDebugP("amp volume command with bad args");
                return true;
            }

            const uint16_t channelIdx = std::stoi(command.substr(11, 1)) - 1;

            if (channelIdx >= OPENKNX_AMP_CHANNEL_COUNT)
            {
                logDebugP("channel index out of range");
                return true;
            }

            if (command.length() == 14)
                value = std::stoi(command.substr(13, 1));
            else if (command.length() == 15)
                value = std::stoi(command.substr(13, 2));
            else if (command.length() == 16)
            {
                value = std::stoi(command.substr(13, 3));
                if (value != 100)
                {
                    logDebugP("Invalid volume value, must be 0-100");
                    return true;
                }
            }
            else
            {
                logDebugP("Länge falsch hat %i ", command.length());
                return true;    
            }

            if (value > 100)
            {
                logDebugP("Volume value out of range (0-100)");
                return true;
            }

            if (_channels[channelIdx] == nullptr)
            {
                logDebugP("Channel %d not initialized", channelIdx + 1);
                return true;
            }
            else
            {
                _channels[channelIdx]->setVolume_VOL(value);
                logDebugP("Set volume of channel %d to %d", channelIdx + 1, value);
            }

            return true;
        }

        // ---------- amp mute ----------
        if (command.substr(0, 9) == "amp mute ")
        {
            if (command.length() != 12) // z.B. "amp mute 1 1"
            {
                logDebugP("amp mute command with bad args");
                return true;
            }

            const uint16_t channelIdx = std::stoi(command.substr(9, 1)) - 1;
            const uint8_t muteValue = std::stoi(command.substr(11, 1));

            if (channelIdx >= OPENKNX_AMP_CHANNEL_COUNT)
            {
                logDebugP("channel index out of range");
                return true;
            }

            if (muteValue != 0 && muteValue != 1)
            {
                logDebugP("Mute value must be 0 or 1");
                return true;
            }

            if (_channels[channelIdx] == nullptr)
            {
                logDebugP("Channel %d not initialized", channelIdx + 1);
                return true;
            }

            _channels[channelIdx]->setMute_MUT(muteValue);
            logDebugP("Set mute of channel %d to %s", channelIdx + 1, muteValue ? "ON" : "OFF");

            return true;
        }
    }

    return false;
}

bool PowerAmpModule::debug()
{
    return _debug;
}

void PowerAmpModule::setSerialChannelPins(const uint8_t channelPins[][4], uint8_t numChannels)
{
    for (uint8_t i = 0; i < numChannels; i++) 
    {
        _rxPins[i]      = channelPins[i][0];
        _txPins[i]      = channelPins[i][1];
        _isHardware[i]  = channelPins[i][2];
        _hwPort[i]      = channelPins[i][3];
    } 
}

SerialUART* PowerAmpModule::getHardwareSerial(uint8_t port)
{
    switch (port)
    {
    case 1:
        return &Serial1; // UART0
    case 2:
        return &Serial2; // UART1
    default:
        return nullptr;
    }
}

const uint8_t PowerAmpModule::_magicWord[AMP_FLASH_MAGIC_WORD_LEN] = {
    'x',
    'A',
    'M',
    'P',
};


uint16_t PowerAmpModule::flashSize()
{
    // [4] Magic Word + [1] Version + [N] je 1 Byte pro Kanal
    return 4 + 1 + OPENKNX_AMP_CHANNEL_COUNT;
}


 void PowerAmpModule::writeFlash()
{
    logDebugP("writing");

    // magic word
    for (size_t i = 0; i < AMP_FLASH_MAGIC_WORD_LEN; i++)
    {
        openknx.flash.writeByte(_magicWord[i]);
    }
    
    // version
    openknx.flash.writeByte(1);

    for (uint8_t i = 0; i < OPENKNX_AMP_CHANNEL_COUNT; i++)
    {
        _channels[i]->save();
    }
    logDebugP("write [done]");
}



void PowerAmpModule::readFlash(const uint8_t* data, const uint16_t size)
{
    logIndentUp();
    if (size < 4 + 1) // no channels present
    {
        logDebugP("Flash data short!");
        return;
    }
    
    for (size_t i = 0; i < AMP_FLASH_MAGIC_WORD_LEN; i++)
    {
        if (openknx.flash.readByte() != _magicWord[i])
        {
            logDebugP("Wrong magic-word!");
            return;
        }
    }


    const uint8_t version = openknx.flash.readByte();
    if (version != 1) // version unknown
    {
        logDebugP("Wrong version (%d)", version);
        return;
    }

    const uint8_t chDataMaxCount = (size - 4 - 1) / (1);
    logDebugP("Found %d of %d channels", chDataMaxCount, OPENKNX_AMP_CHANNEL_COUNT);
    const uint8_t n = MIN(chDataMaxCount, OPENKNX_AMP_CHANNEL_COUNT);
    for (uint8_t i = 0; i < n; i++)
    {
        _channels[i]->restore();
    }
    logDebugP("read [done]");
    logIndentDown();
}
