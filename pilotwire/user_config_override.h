/*
  user_config_override.h - user configuration overrides my_user_config.h for Tasmota

  Copyright (C) 2019/2020  Theo Arends, Nicolas Bernaerts
    05/04/2019 - v1.0   - Creation
    12/04/2019 - v1.1   - Save settings in Settings.weight... variables
    10/06/2019 - v2.0   - Complete rewrite to add web management
    25/06/2019 - v2.1   - Add DHT temperature sensor and settings validity control
    05/07/2019 - v2.2   - Add embeded icon
    05/07/2019 - v3.0   - Add max power management with automatic offload
                          Save power settings in Settings.energy... variables
    12/12/2019 - v3.1   - Add public configuration page http://.../control
    30/12/2019 - v4.0   - Functions rewrite for Tasmota 8.x compatibility
    06/01/2019 - v4.1   - Handle offloading with finite state machine
    09/01/2019 - v4.2   - Separation between Offloading driver and Pilotwire sensor
    15/01/2020 - v5.0   - Separate temperature driver and add remote MQTT sensor
    05/02/2020 - v5.1   - Block relay command if not coming from a mode set
    21/02/2020 - v5.2   - Add daily temperature graph
    24/02/2020 - v5.3   - Add control button to main page
    27/02/2020 - v5.4   - Add target temperature and relay state to temperature graph
    01/03/2020 - v5.5   - Add timer management with Outside mode
    13/03/2020 - v5.6   - Add time to graph
    05/04/2020 - v5.7   - Add timezone management
    18/04/2020 - v6.0   - Handle direct connexion of heater in addition to pilotwire
    16/05/2020 - v6.1   - Add /json page and outside mode in JSON
    20/05/2020 - v6.2   - Add configuration for first NTP server
    26/05/2020 - v6.3   - Add Information JSON page
    07/07/2020 - v6.4   - Change MQTT subscription and parameters

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

// force the compiler to show a warning to confirm that this file is included
//#warning **** user_config_override.h: Using Settings from this File ****

/*****************************************************************************************************\
 * USAGE:
 *   To modify the stock configuration without changing the my_user_config.h file:
 *   (1) copy this file to "user_config_override.h" (It will be ignored by Git)
 *   (2) define your own settings below
 *   (3) for platformio:
 *         define USE_CONFIG_OVERRIDE as a build flags.
 *         ie1 : export PLATFORMIO_BUILD_FLAGS='-DUSE_CONFIG_OVERRIDE'
 *       for Arduino IDE:
 *         enable define USE_CONFIG_OVERRIDE in my_user_config.h
 ******************************************************************************************************
 * ATTENTION:
 *   - Changes to SECTION1 PARAMETER defines will only override flash settings if you change define CFG_HOLDER.
 *   - Expect compiler warnings when no ifdef/undef/endif sequence is used.
 *   - You still need to update my_user_config.h for major define USE_MQTT_TLS.
 *   - All parameters can be persistent changed online using commands via MQTT, WebConsole or Serial.
\*****************************************************************************************************/

/********************************************\
 *    Pilotwire firmware configuration
\********************************************/

#define USE_INFOJSON                          // Add support for Information JSON page
#define USE_TIMEZONE                          // Add support for Timezone management
#define USE_OFFLOADING                        // Add support for MQTT maximum power offloading
#define USE_TEMPERATURE_MQTT                  // Add support for MQTT temperature sensor
#define USE_PILOTWIRE                         // Add support for France Pilotwire protocol for electrical heaters

#define EXTENSION_VERSION "6.4"               // version
#define EXTENSION_NAME "Pilotwire"            // name
#define EXTENSION_AUTHOR "Nicolas Bernaerts"  // author

#undef APP_SLEEP
#define APP_SLEEP 1                           // Default to sleep = 1

// MQTT default
#undef MQTT_HOST
#define MQTT_HOST          "openhab.local"
#undef MQTT_PORT
#define MQTT_PORT          1883              
#undef MQTT_USER
#define MQTT_USER          ""
#undef MQTT_PASS
#define MQTT_PASS          ""
#undef MQTT_TOPIC
#define MQTT_TOPIC         "filpilote_%06X"
#undef MQTT_FULLTOPIC
#define MQTT_FULLTOPIC     "%topic%/%prefix%/"
#undef FRIENDLY_NAME
#define FRIENDLY_NAME      "Fil Pilote"

//#undef USE_ENERGY_SENSOR                      // Disable energy sensors
#undef USE_ARDUINO_OTA                        // Disable support for Arduino OTA
#undef USE_WPS                                // Disable support for WPS as initial wifi configuration tool
#undef USE_SMARTCONFIG                        // Disable support for Wifi SmartConfig as initial wifi configuration tool
#undef USE_DOMOTICZ                           // Disable Domoticz
#undef USE_HOME_ASSISTANT                     // Disable Home Assistant
#undef USE_MQTT_TLS                           // Disable TLS support won't work as the MQTTHost is not set
#undef USE_KNX                                // Disable KNX IP Protocol Support
//#undef USE_WEBSERVER                        // Disable Webserver
#undef USE_EMULATION_HUE                      // Disable Hue Bridge emulation for Alexa (+14k code, +2k mem common)
#undef USE_EMULATION_WEMO                     // Disable Belkin WeMo emulation for Alexa (+6k code, +2k mem common)
#undef USE_CUSTOM                             // Disable Custom features
#undef USE_DISCOVERY                          // Disable Discovery services for both MQTT and web server
//#undef USE_TIMERS                           // Disable support for up to 16 timers
//#undef USE_TIMERS_WEB                       // Disable support for timer webpage
//#undef USE_SUNRISE                          // Disable support for Sunrise and sunset tools
#undef USE_RULES                              // Disable support for rules
#undef USE_I2C                                // Disable all I2C sensors and devices
//#undef USE_DHT                              // Disable internal DHT sensor
//#undef USE_DS18x20                          // Disable DS18x20 sensor
#undef USE_SPI                                // Disable all SPI devices
#undef USE_BH1750                             // Enable BH1750 sensor
#undef USE_BMP                                // Enable BMP085/BMP180/BMP280/BME280 sensors
#undef USE_SHT3X                              // Enable SHT3x
#undef USE_SGP30                              // Enable SGP30 sensor
#undef USE_LM75AD                             // Enable LM75AD sensor
#undef USE_DISPLAY                            // Disable Display support
#undef USE_MHZ19                              // Disable support for MH-Z19 CO2 sensor
#undef USE_SENSEAIR                           // Disable support for SenseAir K30, K70 and S8 CO2 sensor
#undef USE_PMS5003                            // Disable support for PMS5003 and PMS7003 particle concentration sensor
#undef USE_NOVA_SDS                           // Disable support for SDS011 and SDS021 particle concentration sensor
#undef USE_SERIAL_BRIDGE                      // Disable support for software Serial Bridge
#undef USE_SDM120                             // Disable support for Eastron SDM120-Modbus energy meter
#undef USE_SDM630                             // Disable support for Eastron SDM630-Modbus energy meter
#undef USE_MP3_PLAYER                         // Disable DFPlayer Mini MP3 Player RB-DFR-562 commands: play, volume and stop

#undef USE_SONOFF_RF                            // Add support for Sonoff Rf Bridge (+3k2 code)
#undef USE_RF_FLASH                           // Add support for flashing the EFM8BB1 chip on the Sonoff RF Bridge. C2CK must be connected to GPIO4, C2D to GPIO5 on the PCB (+2k7 code)
#undef USE_SONOFF_SC                            // Add support for Sonoff Sc (+1k1 code)
#undef USE_TUYA_MCU                             // Add support for Tuya Serial MCU
#undef USE_ARMTRONIX_DIMMERS                  // Disable support for Armtronix Dimmers (+1k4 code)
#undef USE_PS_16_DZ                           // Disable support for PS-16-DZ Dimmer
#undef USE_SONOFF_IFAN                          // Add support for Sonoff iFan02 and iFan03 (+2k code)
#undef USE_BUZZER                               // Add support for a buzzer (+0k6 code)
#undef USE_ARILUX_RF                            // Add support for Arilux RF remote controller (+0k8 code, 252 iram (non 2.3.0))
#undef USE_DEEPSLEEP 
#undef USE_AZ7798                             // Disable support for AZ-Instrument 7798 CO2 datalogger
#undef USE_PN532_HSU                          // Disable support for PN532 using HSU (Serial) interface (+1k8 code, 140 bytes mem)

#undef USE_ENERGY_MARGIN_DETECTION            // Disable support for Energy Margin detection (+1k6 code)
#undef USE_ENERGY_POWER_LIMIT                 // Disable additional support for Energy Power Limit detection (+1k2 code)
#undef USE_PZEM004T                           // Disable PZEM004T energy sensor
#undef USE_PZEM_AC                            // Disable PZEM014,016 Energy monitor
#undef USE_PZEM_DC                            // Disable PZEM003,017 Energy monitor
#undef USE_MCP39F501                          // Disable MCP39F501 Energy monitor as used in Shelly 2

#undef USE_MAX31855                           // Disable MAX31855 K-Type thermocouple sensor using softSPI
#undef USE_IR_REMOTE                          // Disable IR driver
#undef USE_WS2812                             // Disable WS2812 Led string
#undef USE_ARILUX_RF                          // Disable support for Arilux RF remote controller
#undef USE_SR04                               // Disable support for for HC-SR04 ultrasonic devices
#undef USE_TM1638                             // Disable support for TM1638 switches copying Switch1 .. Switch8
#undef USE_HX711                              // Disable support for HX711 load cell
#undef USE_RF_FLASH                           // Disable support for flashing the EFM8BB1 chip on the Sonoff RF Bridge. C2CK must be connected to GPIO4, C2D to GPIO5 on the PCB
#undef USE_TX20_WIND_SENSOR                   // Disable support for La Crosse TX20 anemometer
#undef USE_RC_SWITCH                          // Disable support for RF transceiver using library RcSwitch
#undef USE_RF_SENSOR                          // Disable support for RF sensor receiver (434MHz or 868MHz) (+0k8 code)
#undef DEBUG_THEO                             // Disable debug code
#undef USE_DEBUG_DRIVER                       // Disable debug code

// add support to MQTT events subscription
#ifndef SUPPORT_MQTT_EVENT
#define SUPPORT_MQTT_EVENT
#endif

#endif  // _USER_CONFIG_OVERRIDE_H_
