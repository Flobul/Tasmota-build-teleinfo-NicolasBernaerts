/*
  user_config_override.h - user configuration overrides my_user_config.h for Tasmota

  Copyright (C) 2020  Theo Arends, Nicolas Bernaerts

    05/05/2019 - v1.0   - Creation
    16/05/2019 - v1.1   - Add Tempo and EJP contracts
    08/06/2019 - v1.2   - Handle active and apparent power
    05/07/2019 - v2.0   - Rework with selection thru web interface
    02/01/2020 - v3.0   - Functions rewrite for Tasmota 8.x compatibility
    05/02/2020 - v3.1   - Add support for 3 phases meters
    14/03/2020 - v3.2   - Add apparent power graph
    05/04/2020 - v3.3   - Add Timezone management
    13/05/2020 - v3.4   - Add overload management per phase
    15/05/2020 - v3.5   - Add tele.info and tele.json pages
    19/05/2020 - v3.6   - Add configuration for first NTP server
    26/05/2020 - v3.7   - Add Information JSON page
    07/07/2020 - v3.7.1 - Enable discovery (mDNS)
    29/07/2020 - v3.8   - Add Meter section to JSON
    05/08/2020 - v4.0   - Major code rewrite, JSON section is now TIC, numbered like new official Teleinfo module
    24/08/2020 - v4.0.1 - Web sensor display update
    18/09/2020 - v4.1   - Based on Tasmota 8.4
    07/10/2020 - v5.0   - Handle different graph periods, javascript auto update
    18/10/2020 - v5.1   - Expose icon on web server
    25/10/2020 - v5.2   - Real time graph page update
    30/10/2020 - v5.3   - Add TIC message page
    02/11/2020 - v5.4   - Tasmota 9.0 compatibility
    09/11/2020 - v6.0   - Handle ESP32 ethernet devices with board selection
    11/11/2020 - v6.1   - Add data.json page
    20/11/2020 - v6.2   - Correct checksum bug
    29/12/2020 - v6.3   - Strengthen message error control
    20/02/2021 - v6.4   - Add sub-totals (HCHP, HCHC, ...) to JSON
    25/02/2021 - v7.0   - Prepare compatibility with TIC standard
    01/03/2021 - v7.0.1 - Add power status bar
    05/03/2021 - v7.1   - Correct bug on hardware energy counter
    08/03/2021 - v7.2   - Handle voltage and checksum for horodatage
    12/03/2021 - v7.3   - Use average / overload for graph
    15/03/2021 - v7.4   - Change graph period parameter
    21/03/2021 - v7.5   - Support for TIC Standard
    29/03/2021 - v7.6   - Add voltage graph
    04/04/2021 - v7.7   - Change in serial port & graph height selection
    06/04/2021 - v7.7.1 - Handle number of indexes according to contract
    10/04/2021 - v7.7.2 - Remove use of String to avoid heap fragmentation 
    14/04/2021 - v7.8   - Calculate Cos phi and Active power (W) 
    21/04/2021 - v8.0   - Fixed IP configuration and change in Cos phi calculation
    29/04/2021 - v8.1   - Bug fix in serial port management and realtime energy totals
    16/05/2021 - v8.1.1 - Control initial baud rate to avoid crash (thanks to Seb)
    26/05/2021 - v8.2   - Add active power (W) graph
    22/06/2021 - v8.3   - Change in serial management for ESP32

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
 *
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

// meter simulation
//#define SIMULATION

// complementary modules
#define USE_IPADDRESS                         // Add fixed IP configuration page
#define USE_TIMEZONE                          // Add support for Timezone management
#define USE_TELEINFO                          // Add support for Teleinfo

// ESP32 specific
#define USE_ETHERNET                          // Add support for ESP32 Ethernet physical port

// extension
#define EXTENSION_NAME "Teleinfo"             // name
#define EXTENSION_VERSION "8.3"               // version
#define EXTENSION_AUTHOR "Nicolas Bernaerts"  // author

// MQTT default
#undef MQTT_HOST
#define MQTT_HOST          "mqtt.local"
#undef MQTT_PORT
#define MQTT_PORT          1883              
#undef MQTT_USER
#define MQTT_USER          ""
#undef MQTT_PASS
#define MQTT_PASS          ""
#undef MQTT_TOPIC
#define MQTT_TOPIC         "compteur"
#undef MQTT_FULLTOPIC
#define MQTT_FULLTOPIC     "%topic%/%prefix%/"
#undef FRIENDLY_NAME
#define FRIENDLY_NAME      "Teleinfo"

// disable serial log
#undef SERIAL_LOG_LEVEL 
#define SERIAL_LOG_LEVEL   LOG_LEVEL_NONE

#undef USE_ARDUINO_OTA                        // support for Arduino OTA
#undef USE_WPS                                // support for WPS as initial wifi configuration tool
#undef USE_SMARTCONFIG                        // support for Wifi SmartConfig as initial wifi configuration tool
//#undef USE_DOMOTICZ                           // Domoticz
//#undef USE_HOME_ASSISTANT                     // Home Assistant

#undef USE_MQTT_TLS                           // TLS support won't work as the MQTTHost is not set
#undef USE_KNX                                // KNX IP Protocol Support
//#undef USE_WEBSERVER                        // Webserver

#undef USE_EMULATION_HUE                      // Hue Bridge emulation for Alexa (+14k code, +2k mem common)
#undef USE_EMULATION_WEMO                     // Belkin WeMo emulation for Alexa (+6k code, +2k mem common)
#undef USE_CUSTOM                             // Custom features

//#undef USE_DISCOVERY                        // Discovery services for both MQTT and web server
//#undef WEBSERVER_ADVERTISE                  // Provide access to webserver by name <Hostname>.local/
//#undef MQTT_HOST_DISCOVERY                  // Find MQTT host server (overrides MQTT_HOST if found)

#undef USE_TIMERS                             // support for up to 16 timers
#undef USE_TIMERS_WEB                         // support for timer webpage
#undef USE_SUNRISE                            // support for Sunrise and sunset tools
#undef SUNRISE_DAWN_ANGLE                     // Select desired Dawn Angle from
//#undef USE_RULES                            // Disable support for rules

#undef ROTARY_V1                              // Add support for Rotary Encoder as used in MI Desk Lamp (+0k8 code)
#undef USE_SONOFF_RF                          // Add support for Sonoff Rf Bridge (+3k2 code)
#undef USE_RF_FLASH                           // Add support for flashing the EFM8BB1 chip on the Sonoff RF Bridge.
#undef USE_SONOFF_SC                          // Add support for Sonoff Sc (+1k1 code)
#undef USE_TUYA_MCU                           // Add support for Tuya Serial MCU
#undef USE_ARMTRONIX_DIMMERS                  // Add support for Armtronix Dimmers (+1k4 code)
#undef USE_PS_16_DZ                           // Add support for PS-16-DZ Dimmer (+2k code)
#undef USE_SONOFF_IFAN                        // Add support for Sonoff iFan02 and iFan03 (+2k code)
#undef USE_BUZZER                             // Add support for a buzzer (+0k6 code)
#undef USE_ARILUX_RF                          // Add support for Arilux RF remote controller (+0k8 code, 252 iram (non 2.3.0))
#undef USE_SHUTTER                            // Add Shutter support for up to 4 shutter with different motortypes (+11k code)
#undef USE_DEEPSLEEP                          // Add support for deepsleep (+1k code)
#undef USE_EXS_DIMMER                         // Add support for ES-Store WiFi Dimmer (+1k5 code)
#undef USE_DEVICE_GROUPS                      // Add support for device groups (+5k5 code)
#undef USE_DEVICE_GROUPS_SEND                 // Add support for the DevGroupSend command (+0k6 code)
#undef USE_PWM_DIMMER
#undef USE_PWM_DIMMER_REMOTE                  // Add support for remote switches to PWM Dimmer (requires USE_DEVICE_GROUPS) (+0k9 code)
#undef USE_SONOFF_D1                          // Add support for Sonoff D1 Dimmer (+0k7 code)
#undef USE_SHELLY_DIMMER
#undef USE_MHZ19                              // Add support for MH-Z19 - CO2 sensor

//#undef USE_LIGHT
#undef USE_WS2812                             // WS2812 Led string using library NeoPixelBus (+5k code, +1k mem, 232 iram) - Disable by //
#undef USE_MY92X1                             // Add support for MY92X1 RGBCW led controller as used in Sonoff B1, Ailight and Lohas
#undef USE_SM16716                            // Add support for SM16716 RGB LED controller (+0k7 code)
#undef USE_SM2135                             // Add support for SM2135 RGBCW led control as used in Action LSC (+0k6 code)
#undef USE_SONOFF_L1                          // Add support for Sonoff L1 led control
#undef USE_ELECTRIQ_MOODL                     // Add support for ElectriQ iQ-wifiMOODL RGBW LED controller (+0k3 code)
#undef USE_LIGHT_PALETTE                      // Add support for color palette (+0k7 code)
#undef USE_DGR_LIGHT_SEQUENCE                 // Add support for device group light sequencing (requires USE_DEVICE_GROUPS) (+0k2 code)

#undef USE_ADE7953
#undef USE_BL0940

//#undef USE_COUNTER                          // Enable inputs as counter (+0k8 code)
#undef USE_DS18x20                            // Add support for DS18x20 sensors with id sort, single scan and read retry (+2k6 code)
#undef USE_DHT                                // Disable internal DHT sensor

#undef USE_I2C                                // Disable all I2C sensors and devices
#undef USE_SPI                                // Disable all SPI devices

#undef USE_SERIAL_BRIDGE                      // Disable support for software Serial Bridge (+0k8 code)

//#undef USE_ENERGY_SENSOR                    // Disable energy sensors
#undef USE_ENERGY_MARGIN_DETECTION            // Add support for Energy Margin detection (+1k6 code)
#undef USE_ENERGY_POWER_LIMIT                 // Add additional support for Energy Power Limit detection (+1k2 code)
#undef USE_PZEM004T                           // Add support for PZEM004T Energy monitor (+2k code)
#undef USE_PZEM_AC                            // Add support for PZEM014,016 Energy monitor (+1k1 code)
#undef USE_PZEM_DC                            // Add support for PZEM003,017 Energy monitor (+1k1 code)
#undef USE_MCP39F501                          // Add support for MCP39F501 Energy monitor as used in Shelly 2 (+3k1 code)

#undef USE_IR_REMOTE                          // Send IR remote commands using library IRremoteESP8266 and ArduinoJson (+4k3 code, 0k3 mem, 48 iram)

#undef USE_ZIGBEE                             // Enable Zigbee protocol
#undef USE_ZIGBEE_ZNP                       // Enable ZNP protocol, needed for CC2530 based devices

#endif  // _USER_CONFIG_OVERRIDE_H_
