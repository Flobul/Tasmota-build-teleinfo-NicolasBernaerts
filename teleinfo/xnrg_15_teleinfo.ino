/*
  xnrg_15_teleinfo.ino - France Teleinfo energy sensor support for Tasmota devices

  Copyright (C) 2019-2023  Nicolas Bernaerts

  Connexions :
    * On ESP8266, Teleinfo Rx must be connected to GPIO3 (as it must be forced to 7E1)
    * On ESP32, Teleinfo Rx must NOT be connected to GPIO3 (to avoid nasty ESP32 ESP_Restart bug where ESP32 hangs if restarted when Rx is under heavy load)
    * Teleinfo EN may (or may not) be connected to any GPIO starting from GPIO5 ...
   
  Version history :
    05/05/2019 - v1.0  - Creation
    16/05/2019 - v1.1  - Add Tempo and EJP contracts
    08/06/2019 - v1.2  - Handle active and apparent power
    05/07/2019 - v2.0  - Rework with selection thru web interface
    02/01/2020 - v3.0  - Functions rewrite for Tasmota 8.x compatibility
    05/02/2020 - v3.1  - Add support for 3 phases meters
    14/03/2020 - v3.2  - Add apparent power graph
    05/04/2020 - v3.3  - Add Timezone management
    13/05/2020 - v3.4  - Add overload management per phase
    19/05/2020 - v3.6  - Add configuration for first NTP server
    26/05/2020 - v3.7  - Add Information JSON page
    29/07/2020 - v3.8  - Add Meter section to JSON
    05/08/2020 - v4.0  - Major code rewrite, JSON section is now TIC, numbered like new official Teleinfo module
                         Web sensor display update
    18/09/2020 - v4.1  - Based on Tasmota 8.4
    07/10/2020 - v5.0  - Handle different graph periods and javascript auto update
    18/10/2020 - v5.1  - Expose icon on web server
    25/10/2020 - v5.2  - Real time graph page update
    30/10/2020 - v5.3  - Add TIC message page
    02/11/2020 - v5.4  - Tasmota 9.0 compatibility
    09/11/2020 - v6.0  - Handle ESP32 ethernet devices with board selection
    11/11/2020 - v6.1  - Add data.json page
    20/11/2020 - v6.2  - Checksum bug
    29/12/2020 - v6.3  - Strengthen message error control
    25/02/2021 - v7.0  - Prepare compatibility with TIC standard
                         Add power status bar
    05/03/2021 - v7.1  - Correct bug on hardware energy counter
    08/03/2021 - v7.2  - Handle voltage and checksum for horodatage
    12/03/2021 - v7.3  - Use average / overload for graph
    15/03/2021 - v7.4  - Change graph period parameter
    21/03/2021 - v7.5  - Support for TIC Standard
    29/03/2021 - v7.6  - Add voltage graph
    04/04/2021 - v7.7  - Change in serial port & graph height selection
                         Handle number of indexes according to contract
                         Remove use of String to avoid heap fragmentation 
    14/04/2021 - v7.8  - Calculate Cos phi and Active power (W)
    21/04/2021 - v8.0  - Fixed IP configuration and change in Cos phi calculation
    29/04/2021 - v8.1  - Bug fix in serial port management and realtime energy totals
                         Control initial baud rate to avoid crash (thanks to Seb)
    26/05/2021 - v8.2  - Add active power (W) graph
    22/06/2021 - v8.3  - Change in serial management for ESP32
    04/08/2021 - v9.0  - Tasmota 9.5 compatibility
                         Add LittleFS historic data record
                         Complete change in VA, W and cos phi measurement based on transmission time
                         Add PME/PMI ACE6000 management
                         Add energy update interval configuration
                         Add TIC to TCP bridge (command 'tcpstart 8888' to publish teleinfo stream on port 8888)
    04/09/2021 - v9.1  - Save settings in LittleFS partition if available
                         Log rotate and old files deletion if space low
    10/10/2021 - v9.2  - Add peak VA and V in history files
    02/11/2021 - v9.3  - Add period and totals in history files
                         Add simple FTP server to access history files
    13/03/2022 - v9.4  - Change keys to ISUB and PSUB in METER section
    20/03/2022 - v9.5  - Change serial init and major rework in active power calculation
    01/04/2022 - v9.6  - Add software watchdog feed to avoid lock
    22/04/2022 - v9.7  - Option to minimise LittleFS writes (day:every 1h and week:every 6h)
                         Correction of EAIT bug
    04/08/2022 - v9.8  - Based on Tasmota 12, add ESP32S2 support
                         Remove FTP server auto start
    18/08/2022 - v9.9  - Force GPIO_TELEINFO_RX as digital input
                         Correct bug littlefs config and graph data recording
                         Add Tempo and Production mode (thanks to Sébastien)
                         Correct publication synchronised with teleperiod
    26/10/2022 - v10.0 - Add bar graph monthly (every day) and yearly (every month)
    06/11/2022 - v10.1 - Bug fixes on bar graphs and change in lltoa conversion
    15/11/2022 - v10.2 - Add bar graph daily (every hour)
    04/02/2023 - v10.3 - Add graph swipe (horizontal and vertical)
                         Disable wifi sleep on ESP32 to avoid latency
    25/02/2023 - v11.0 - Split between xnrg and xsns
                         Use Settings->teleinfo to store configuration
                         Update today and yesterday totals
    03/06/2023 - v11.1 - Rewrite of Tasmota energy update
                         Avoid 100ms rules teleperiod update
    15/08/2023 - v11.3 - Change in cosphi calculation
                         Reorder Tempo periods
    25/08/2023 - v11.4 - Change in ESP32S2 partitionning (1.3M littleFS)
                         Add serial reception in display loops to avoid errors
    17/10/2023 - v12.1 - Handle Production & consommation simultaneously
                         Display all periods with total
    17/10/2023 - v12.2 - Handle RGB color according to total power

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

#ifdef USE_ENERGY_SENSOR
#ifdef USE_TELEINFO

/*********************************************************************************************\
 * Teleinfo historical
 * docs https://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_54E.pdf
 * Teleinfo hardware will be enabled if 
 *     Hardware RX = [TinfoRX]
 *     Rx enable   = [TinfoEN] (optional)
\*********************************************************************************************/

#include <TasmotaSerial.h>
TasmotaSerial *teleinfo_serial = nullptr;

/*************************************************\
 *               Variables
\*************************************************/

// declare teleinfo energy driver and sensor
#define XNRG_15   15

// teleinfo constant
#define TELEINFO_VOLTAGE                230       // default voltage provided
#define TELEINFO_VOLTAGE_MAXIMUM        260       // maximum voltage for value to be acceptable
#define TELEINFO_VOLTAGE_MINIMUM        100       // minimum voltage for value to be acceptable
#define TELEINFO_VOLTAGE_REF            200       // voltage reference for max power calculation
#define TELEINFO_INDEX_MAX              12        // maximum number of total power counters
#define TELEINFO_PERCENT_MIN            1         // minimum acceptable percentage of energy contract
#define TELEINFO_PERCENT_MAX            200       // maximum acceptable percentage of energy contract
#define TELEINFO_PERCENT_CHANGE         4         // 5% of power change to publish JSON
#define TELEINFO_FILE_DAILY             7         // default number of daily files
#define TELEINFO_FILE_WEEKLY            8         // default number of weekly files

// data default and boundaries
#define TELEINFO_GRAPH_INC_VOLTAGE      5
#define TELEINFO_GRAPH_MIN_VOLTAGE      235
#define TELEINFO_GRAPH_DEF_VOLTAGE      240       // default voltage maximum in graph
#define TELEINFO_GRAPH_MAX_VOLTAGE      265
#define TELEINFO_GRAPH_INC_POWER        3000
#define TELEINFO_GRAPH_MIN_POWER        3000
#define TELEINFO_GRAPH_DEF_POWER        6000      // default power maximum consumption in graph
#define TELEINFO_GRAPH_MAX_POWER        150000
#define TELEINFO_GRAPH_INC_WH_HOUR      2
#define TELEINFO_GRAPH_MIN_WH_HOUR      2
#define TELEINFO_GRAPH_DEF_WH_HOUR      2         // default max hourly active power consumption in year graph
#define TELEINFO_GRAPH_MAX_WH_HOUR      50
#define TELEINFO_GRAPH_INC_WH_DAY       10
#define TELEINFO_GRAPH_MIN_WH_DAY       10
#define TELEINFO_GRAPH_DEF_WH_DAY       10         // default max daily active power consumption in year graph
#define TELEINFO_GRAPH_MAX_WH_DAY       200
#define TELEINFO_GRAPH_INC_WH_MONTH     100
#define TELEINFO_GRAPH_MIN_WH_MONTH     100
#define TELEINFO_GRAPH_DEF_WH_MONTH     100       // default max monthly active power consumption in year graph
#define TELEINFO_GRAPH_MAX_WH_MONTH     50000

// string size
#define TELEINFO_CONTRACTID_MAX         16        // max length of TIC contract ID
#define TELEINFO_PART_MAX               4         // maximum number of parts in a line
#define TELEINFO_LINE_QTY               74        // maximum number of lines handled in a TIC message
#define TELEINFO_LINE_MAX               128       // maximum size of a received TIC line
#define TELEINFO_KEY_MAX                14        // maximum size of a TIC etiquette
#define TELEINFO_DATA_MAX               64        // maximum size of a TIC donnee

// commands : MQTT
//#define D_CMND_TELEINFO_HELP            "help"
#define D_CMND_TELEINFO_MODE            "Mode"
#define D_CMND_TELEINFO_HISTORIQUE      "Historique"
#define D_CMND_TELEINFO_STANDARD        "Standard"
#define D_CMND_TELEINFO_LED             "Led"
#define D_CMND_TELEINFO_STATS           "Stats"
#define D_CMND_TELEINFO_CONTRACT        "Percent"

#define D_CMND_TELEINFO_MSG_POLICY      "msgpol"
#define D_CMND_TELEINFO_MSG_TYPE        "msgtype"

#define D_CMND_TELEINFO_LOG_DAY         "nbday"
#define D_CMND_TELEINFO_LOG_WEEK        "nbweek"
//#define D_CMND_TELEINFO_LOG_ROTATE      "Logrot"

#define D_CMND_TELEINFO_MAX_V           "maxv"
#define D_CMND_TELEINFO_MAX_VA          "maxva"
#define D_CMND_TELEINFO_MAX_KWH_HOUR    "maxhour"
#define D_CMND_TELEINFO_MAX_KWH_DAY     "maxday"
#define D_CMND_TELEINFO_MAX_KWH_MONTH   "maxmonth"

// commands : Web
#define D_CMND_TELEINFO_ETH             "eth"
#define D_CMND_TELEINFO_PHASE           "phase"
#define D_CMND_TELEINFO_PERIOD          "period"
#define D_CMND_TELEINFO_HISTO           "histo"
#define D_CMND_TELEINFO_HOUR            "hour"
#define D_CMND_TELEINFO_DAY             "day"
#define D_CMND_TELEINFO_MONTH           "month"
#define D_CMND_TELEINFO_DATA            "data"
#define D_CMND_TELEINFO_PREV            "prev"
#define D_CMND_TELEINFO_NEXT            "next"
#define D_CMND_TELEINFO_MINUS           "minus"
#define D_CMND_TELEINFO_PLUS            "plus"

// JSON TIC extensions
#define TELEINFO_JSON_TIC               "TIC"
#define TELEINFO_JSON_METER             "METER"
#define TELEINFO_JSON_PHASE             "PH"
#define TELEINFO_JSON_ISUB              "ISUB"
#define TELEINFO_JSON_PSUB              "PSUB"
#define TELEINFO_JSON_PMAX              "PMAX"

// interface strings
#define D_TELEINFO                      "Teleinfo"
#define D_TELEINFO_MESSAGE              "Message"
#define D_TELEINFO_GRAPH                "Graph"
#define D_TELEINFO_CONTRACT             "Contract"
#define D_TELEINFO_HEURES               "Heures"
#define D_TELEINFO_ERROR                "Errors"
#define D_TELEINFO_MODE                 "Mode"
#define D_TELEINFO_UPDATE               "Cosφ updates"
#define D_TELEINFO_RESET                "Message reset"
#define D_TELEINFO_PERIOD               "Period"

// RGB limits
#define TELEINFO_RGB_RED_MAX            208
#define TELEINFO_RGB_GREEN_MAX          176

// Historic data files
#define D_TELEINFO_CFG                  "/teleinfo.cfg"
#define TELEINFO_HISTO_DAY_DEFAULT      8         // default number of daily histotisation files
#define TELEINFO_HISTO_DAY_MAX          31        // max number of daily histotisation files
#define TELEINFO_HISTO_WEEK_DEFAULT     4         // default number of weekly histotisation files
#define TELEINFO_HISTO_WEEK_MAX         52        // max number of weekly histotisation files

// MQTT commands : tic_help, tic_enable, tic_led, tic_msgpol, tic_msgtype, tic_percent, tic_nbday, tic_nbweek, tic_maxv, tic_maxva, tic_maxhour, tic_maxday and tic_maxmonth
const char kTeleinfoCommands[] PROGMEM = D_CMND_TELEINFO_MODE "|" D_CMND_TELEINFO_HISTORIQUE "|" D_CMND_TELEINFO_STANDARD "|" D_CMND_TELEINFO_LED "|" D_CMND_TELEINFO_STATS "|" D_CMND_TELEINFO_MSG_POLICY "|" D_CMND_TELEINFO_MSG_TYPE "|" D_CMND_TELEINFO_CONTRACT "|" D_CMND_TELEINFO_LOG_DAY "|" D_CMND_TELEINFO_LOG_WEEK "|" D_CMND_TELEINFO_MAX_V "|" D_CMND_TELEINFO_MAX_VA "|" D_CMND_TELEINFO_MAX_KWH_HOUR "|" D_CMND_TELEINFO_MAX_KWH_DAY "|" D_CMND_TELEINFO_MAX_KWH_MONTH;
enum TeleinfoCommand                   { TIC_CMND_MODE,           TIC_CMND_HISTORIQUE,           TIC_CMND_STANDARD,           TIC_CMND_LED,           TIC_CMND_STATS,           TIC_CMND_MSG_POLICY,           TIC_CMND_MSG_TYPE,           TIC_CMND_CONTRACT,           TIC_CMND_LOG_DAY,           TIC_CMND_LOG_WEEK,           TIC_CMND_MAX_V,           TIC_CMND_MAX_VA,           TIC_CMND_MAX_KWH_HOUR,           TIC_CMND_MAX_KWH_DAY,           TIC_CMND_MAX_KWH_MONTH };

// Specific etiquettes
enum TeleinfoEtiquette { TIC_NONE, TIC_ADCO, TIC_ADSC, TIC_PTEC, TIC_NGTF, TIC_EAIT, TIC_IINST, TIC_IINST1, TIC_IINST2, TIC_IINST3, TIC_ISOUSC, TIC_PS, TIC_PAPP, TIC_SINSTS, TIC_SINSTI, TIC_BASE, TIC_EAST, TIC_HCHC, TIC_HCHP, TIC_EJPHN, TIC_EJPHPM, TIC_BBRHCJB, TIC_BBRHPJB, TIC_BBRHCJW, TIC_BBRHPJW, TIC_BBRHCJR, TIC_BBRHPJR, TIC_ADPS, TIC_ADIR1, TIC_ADIR2, TIC_ADIR3, TIC_URMS1, TIC_URMS2, TIC_URMS3, TIC_UMOY1, TIC_UMOY2, TIC_UMOY3, TIC_IRMS1, TIC_IRMS2, TIC_IRMS3, TIC_SINSTS1, TIC_SINSTS2, TIC_SINSTS3, TIC_PTCOUR, TIC_PTCOUR1, TIC_PTCOUR2, TIC_PREF, TIC_PCOUP, TIC_LTARF, TIC_EASF01, TIC_EASF02, TIC_EASF03, TIC_EASF04, TIC_EASF05, TIC_EASF06, TIC_EASF07, TIC_EASF08, TIC_EASF09, TIC_EASF10, TIC_ADS, TIC_CONFIG, TIC_EAPS, TIC_EAS, TIC_EAPPS, TIC_PREAVIS, TIC_MAX };
const char kTeleinfoEtiquetteName[] PROGMEM = "|ADCO|ADSC|PTEC|NGTF|EAIT|IINST|IINST1|IINST2|IINST3|ISOUSC|PS|PAPP|SINSTS|SINSTI|BASE|EAST|HCHC|HCHP|EJPHN|EJPHPM|BBRHCJB|BBRHPJB|BBRHCJW|BBRHPJW|BBRHCJR|BBRHPJR|ADPS|ADIR1|ADIR2|ADIR3|URMS1|URMS2|URMS3|UMOY1|UMOY2|UMOY3|IRMS1|IRMS2|IRMS3|SINSTS1|SINSTS2|SINSTS3|PTCOUR|PTCOUR1|PTCOUR2|PREF|PCOUP|LTARF|EASF01|EASF02|EASF03|EASF04|EASF05|EASF06|EASF07|EASF08|EASF09|EASF10|ADS|CONFIG|EAP_s|EA_s|EAPP_s|PREAVIS";

// TIC - modes and rates
enum TeleinfoType { TIC_TYPE_UNDEFINED, TIC_TYPE_CONSO, TIC_TYPE_PROD, TIC_TYPE_MAX };
const char kTeleinfoTypeName[] PROGMEM = "Non déterminé|Consommateur|Producteur";
enum TeleinfoMode { TIC_MODE_UNDEFINED, TIC_MODE_HISTORIC, TIC_MODE_STANDARD, TIC_MODE_PME, TIC_MODE_MAX };
const char kTeleinfoModeName[] PROGMEM = "|Historique|Standard|PME";
const char kTeleinfoModeIcon[] PROGMEM = "|🇭|🇸|🇵";

// Tarifs                                  [  Toutes   ]   [ Creuses       Pleines   ] [ Normales   PointeMobile ] [CreusesBleu  PleinesBleu   CreusesBlanc   PleinesBlanc CreusesRouge   PleinesRouge] [ Pointe   PointeMobile  Hiver      Pleines     Creuses    PleinesHiver CreusesHiver PleinesEte   CreusesEte   Pleines1/2S  Creuses1/2S  JuilletAout] [Pointe PleinesHiver CreusesHiver PleinesEte CreusesEte] [ Base  ]  [ Pleines    Creuses   ] [ Creuses bleu      Pleine Bleu    Creuse Blanc       Pleine Blanc     Creuse Rouge       Pleine Rouge ]  [ Normale      Pointe  ]  [Production]
enum TeleinfoPeriod                        { TIC_HISTO_TH, TIC_HISTO_HC, TIC_HISTO_HP, TIC_HISTO_HN, TIC_HISTO_PM, TIC_HISTO_CB, TIC_HISTO_PB, TIC_HISTO_CW, TIC_HISTO_PW, TIC_HISTO_CR, TIC_HISTO_PR, TIC_STD_P, TIC_STD_PM, TIC_STD_HH, TIC_STD_HP, TIC_STD_HC, TIC_STD_HPH, TIC_STD_HCH, TIC_STD_HPE, TIC_STD_HCE, TIC_STD_HPD, TIC_STD_HCD, TIC_STD_JA, TIC_STD_1, TIC_STD_2, TIC_STD_3, TIC_STD_4, TIC_STD_5, TIC_STD_BASE, TIC_STD_HPL, TIC_STD_HCR, TIC_STD_HC_BLEU, TIC_STD_HP_BLEU, TIC_STD_HC_BLANC, TIC_STD_HP_BLANC, TIC_STD_HC_ROUGE, TIC_STD_HP_ROUGE, TIC_STD_HNO, TIC_STD_HPT, TIC_STD_PROD, TIC_PERIOD_MAX };
const int ARR_TELEINFO_PERIOD_FIRST[]    = { TIC_HISTO_TH, TIC_HISTO_HC, TIC_HISTO_HC, TIC_HISTO_HN, TIC_HISTO_HN, TIC_HISTO_CB, TIC_HISTO_CB, TIC_HISTO_CB, TIC_HISTO_CB, TIC_HISTO_CB, TIC_HISTO_CB, TIC_STD_P, TIC_STD_P,  TIC_STD_P,  TIC_STD_P,  TIC_STD_P,  TIC_STD_P,   TIC_STD_P,   TIC_STD_P,   TIC_STD_P,   TIC_STD_P,   TIC_STD_P,   TIC_STD_P,  TIC_STD_1, TIC_STD_1, TIC_STD_1, TIC_STD_1, TIC_STD_1, TIC_STD_BASE, TIC_STD_HPL, TIC_STD_HPL, TIC_STD_HC_BLEU, TIC_STD_HC_BLEU, TIC_STD_HC_BLEU,  TIC_STD_HC_BLEU,  TIC_STD_HC_BLEU,  TIC_STD_HC_BLEU,  TIC_STD_HNO, TIC_STD_HNO, TIC_STD_PROD };
const int ARR_TELEINFO_PERIOD_QUANTITY[] = { 1,            2,            2,            2,            2,            6,            6,            6,            6,            6,            6,            12,        12,         12,         12,         12,         12,          12,          12,          12,          12,          12,          12,         5,         5,         5,         5,         5,         1,            2,           2          ,         6      ,       6         ,       6        ,        6        ,       6        ,         6       ,       2     ,      2     ,      1       };
const char kTeleinfoPeriod[] PROGMEM     = "TH..|HC..|HP..|HN..|PM..|HCJB|HPJB|HCJW|HPJW|HCJR|HPJR|P|PM|HH|HP|HC|HPH|HCH|HPE|HCE|HPD|HCD|JA|1|2|3|4|5|BASE|HEURE PLEINE|HEURE CREUSE|HC BLEU|HP BLEU|HC BLANC|HP BLANC|HC ROUGE|HP ROUGE|HEURE NORMALE|HEURE POINTE|INDEX NON CONSO";
const char kTeleinfoPeriodName[] PROGMEM = "Toutes|Creuses|Pleines|Normales|Pointe Mobile|Creuses Bleu|Pleines Bleu|Creuses Blanc|Pleines Blanc|Creuses Rouge|Pleines Rouge|Pointe|Pointe Mobile|Hiver|Pleines|Creuses|Pleines Hiver|Creuses Hiver|Pleines Ete|Creuses Ete|Pleines 1/2s.|Creuses 1/2s.|Juillet-Aout|Pointe|Pleines Hiver|Creuses Hiver|Pleines Ete|Creuses Ete|Base|Pleines|Creuses|Creuses Bleu|Pleines Bleu|Creuses Blanc|Pleines Blanc|Creuses Rouge|Pleines Rouge|Normaux|Pointe|Production";

// Data diffusion policy
enum TeleinfoMessagePolicy { TELEINFO_POLICY_MESSAGE, TELEINFO_POLICY_PERCENT, TELEINFO_POLICY_TELEMETRY, TELEINFO_POLICY_MAX };
const char kTeleinfoMessagePolicy[] PROGMEM = "Every TIC|± 5% Power Change|Telemetry only";
enum TeleinfoMessageType { TELEINFO_MSG_NONE, TELEINFO_MSG_METER, TELEINFO_MSG_TIC, TELEINFO_MSG_BOTH, TELEINFO_MSG_MAX };
const char kTeleinfoMessageType[] PROGMEM = "None|METER only|TIC only|METER and TIC";

// config
enum TeleinfoConfigKey { TELEINFO_CONFIG_NBDAY, TELEINFO_CONFIG_NBWEEK, TELEINFO_CONFIG_MAX_HOUR, TELEINFO_CONFIG_MAX_DAY, TELEINFO_CONFIG_MAX_MONTH, TELEINFO_CONFIG_MAX };     // configuration parameters
const char kTeleinfoConfigKey[] PROGMEM = D_CMND_TELEINFO_LOG_DAY "|" D_CMND_TELEINFO_LOG_WEEK "|" D_CMND_TELEINFO_MAX_KWH_HOUR "|" D_CMND_TELEINFO_MAX_KWH_DAY "|" D_CMND_TELEINFO_MAX_KWH_MONTH;      // configuration keys

// power calculation modes
enum TeleinfoPowerCalculationMethod { TIC_METHOD_GLOBAL_COUNTER, TIC_METHOD_INCREMENT };
enum TeleinfoPowerTarget { TIC_POWER_UPDATE_COUNTER, TIC_POWER_UPDATE_PAPP, TIC_POWER_UPDATE_PACT };

// serial port management
enum TeleinfoSerialStatus { TIC_SERIAL_INIT, TIC_SERIAL_ACTIVE, TIC_SERIAL_STOPPED, TIC_SERIAL_FAILED };

// phase colors
const char kTeleinfoColorPhase[] PROGMEM = "#09f|#f90|#093";                   // phase colors (blue, orange, green)
const char kTeleinfoColorPeak[]  PROGMEM = "#5ae|#eb6|#2a6";                   // peak colors (blue, orange, green)

/****************************************\
 *                 Data
\****************************************/

// teleinfo : configuration
static struct {
  uint8_t  led        = 0;                               // enable p
  uint8_t  msg_policy = TELEINFO_POLICY_TELEMETRY;       // publishing policy for data
  uint8_t  msg_type   = TELEINFO_MSG_METER;              // type of data to publish (METER and/or TIC)
  uint8_t  percent    = 100;                             // maximum acceptable power in percentage of contract power
  long     max_volt   = TELEINFO_GRAPH_DEF_VOLTAGE;      // maximum voltage on graph
  long     max_power  = TELEINFO_GRAPH_DEF_POWER;        // maximum power on graph
  long     param[TELEINFO_CONFIG_MAX] = { TELEINFO_HISTO_DAY_DEFAULT, TELEINFO_HISTO_WEEK_DEFAULT, TELEINFO_GRAPH_DEF_WH_HOUR, TELEINFO_GRAPH_DEF_WH_DAY, TELEINFO_GRAPH_DEF_WH_MONTH };      // graph configuration
} teleinfo_config;

// power calculation structure
static struct {
  uint8_t   method = TIC_METHOD_GLOBAL_COUNTER;     // power calculation method
  float     papp_inc[ENERGY_MAX_PHASES];            // apparent power counter increment per phase (in vah)
  long      papp_current_counter    = LONG_MAX;     // current apparent power counter (in vah)
  long      papp_previous_counter   = LONG_MAX;     // previous apparent power counter (in vah)
  long      pact_current_counter    = LONG_MAX;     // current active power counter (in wh)
  long      pact_previous_counter   = LONG_MAX;     // previous active power counter (in wh)
  uint32_t  previous_time_message   = UINT32_MAX;   // timestamp of previous message
  uint32_t  previous_time_counter   = UINT32_MAX;   // timestamp of previous global counter increase
} teleinfo_calc; 

// TIC : current message
struct tic_line {
  String str_etiquette;                             // TIC line etiquette
  String str_donnee;                                // TIC line donnee
  char checksum;
};
static struct {
  bool     overload      = false;                   // overload has been detected
  bool     received      = false;                   // one full message has been received
  bool     percent       = false;                   // power has changed of more than 1% on one phase
  bool     publish_tic   = false;                   // flag to ask to publish TIC JSON
  bool     publish_meter = false;                   // flag to ask to publish Meter JSON
  int      line_index    = 0;                       // index of current received message line
  int      line_max      = 0;                       // max number of lines in a message
  int      length        = INT_MAX;                 // length of message     
  uint32_t timestamp     = UINT32_MAX;              // timestamp of message (ms)
  tic_line line[TELEINFO_LINE_QTY];                 // array of message lines
} teleinfo_message;

// teleinfo : contract data
static struct {
  uint8_t   phase          = 1;                     // number of phases
  uint8_t   mode           = TIC_MODE_UNDEFINED;    // meter mode (historic, standard)
  uint8_t   type           = TIC_TYPE_UNDEFINED;    // meter running type (conso or prod)
  uint8_t   period_current = UINT8_MAX;             // actual tarif period
  uint8_t   period_first   = UINT8_MAX;             // first index of contract periods
  uint8_t   period_qty     = UINT8_MAX;             // number of periods in current contract      
  long      voltage        = TELEINFO_VOLTAGE;      // contract reference voltage
  long      isousc         = 0;                     // contract max current per phase
  long      ssousc         = 0;                     // contract max power per phase
  long long ident          = 0;                     // contract identification
} teleinfo_contract;

// teleinfo : power meter
static struct {
  uint8_t   led_state      = 0;                     // led state
  uint8_t   status_rx      = TIC_SERIAL_INIT;       // Teleinfo Rx initialisation status
  uint8_t   day_of_month   = UINT8_MAX;             // Current day of month

  int       interval_count = 0;                     // energy publication counter      
  long      nb_message     = 0;                     // total number of messages sent by the meter
  long      nb_reset       = 0;                     // total number of message reset sent by the meter
  long      nb_update      = 0;                     // number of cosphi calculation updates
  long long nb_line        = 0;                     // total number of received lines
  long long nb_error       = 0;                     // total number of checksum errors

  long      conso_papp         = 0;                 // current conso apparent power 
  long long conso_total_wh     = 0;                 // global active conso total
  long long conso_delta_wh     = 0;                 // global active conso total delta since last measure
  long long conso_midnight_wh  = LONG_LONG_MAX;     // conso previous daily total
  long long conso_index_wh[TELEINFO_INDEX_MAX];     // array of conso total of different tarif periods

  long      prod_papp         = 0;                  // current production apparent power 
  long      prod_papp_last    = 0;                  // current production apparent power 
  long long prod_total_wh     = 0;                  // total production index
  long long prod_midnight_wh  = LONG_LONG_MAX;      // conso previous daily total

  char      sep_line = 0;                           // detected line separator
  char      str_line[TELEINFO_LINE_MAX];            // reception buffer for current line
} teleinfo_meter;

// teleinfo : actual data per phase
struct tic_phase {
  bool  volt_set;                                    // voltage set in current message
  long  voltage;                                     // instant voltage
  long  current;                                     // instant current (x100)
  long  papp;                                        // instant apparent power
  long  pact;                                        // instant active power
  long  papp_last;                                   // last published apparent power
  long  cosphi;                                      // cos phi (x100)
}; 
static tic_phase teleinfo_phase[ENERGY_MAX_PHASES];

/**************************************************\
 *                  Commands
\**************************************************/

bool TeleinfoHandleCommand ()
{
  bool   serviced = true;
  int    index;
  char   *pstr_next, *pstr_key, *pstr_value;
  char   str_label[32];
  char   str_item[64];
  String str_line;

  if (Energy->command_code == CMND_ENERGYCONFIG) 
  {
    // if no parameter, show configuration
    if (XdrvMailbox.data_len == 0) 
    {
      AddLog (LOG_LEVEL_INFO, PSTR ("EnergyConfig Teleinfo parameters :"));
      AddLog (LOG_LEVEL_INFO, PSTR ("  Historique (enable teleinfo historique mode - need restart)"));
      AddLog (LOG_LEVEL_INFO, PSTR ("  Standard (enable teleinfo standard mode - need restart)"));
      if (Settings->baudrate == 4) index = 0;
      else if (Settings->baudrate == 32) index = 1;
      else index = 2;
      AddLog (LOG_LEVEL_INFO, PSTR ("  Mode=%d (set teleinfo mode : 0=historique, 1=standard, 2=custom[%d baud] - need restart)"), index, Settings->baudrate * 300);
      AddLog (LOG_LEVEL_INFO, PSTR ("  Stats (display reception statistics)"));
      AddLog (LOG_LEVEL_INFO, PSTR ("  Led=%u (enable RGB LED display status)"), teleinfo_config.led);
      AddLog (LOG_LEVEL_INFO, PSTR ("  Percent=%u (maximum acceptable contract in %%)"), teleinfo_config.percent);

      // publishing policy
      str_line = "";
      for (index = 0; index < TELEINFO_POLICY_MAX; index++)
      {
        GetTextIndexed (str_label, sizeof (str_label), index, kTeleinfoMessagePolicy);
        sprintf (str_item, "%u=%s", index, str_label);
        if (str_line.length () > 0) str_line += ", ";
        str_line += str_item;
      }
      AddLog (LOG_LEVEL_INFO, PSTR ("  msgpol=%u (message policy : %s)"), teleinfo_config.msg_policy, str_line.c_str ());

      // publishing type
      str_line = "";
      for (index = 0; index < TELEINFO_MSG_MAX; index++)
      {
        GetTextIndexed (str_label, sizeof (str_label), index, kTeleinfoMessageType);
        sprintf (str_item, "%u=%s", index, str_label);
        if (str_line.length () > 0) str_line += ", ";
        str_line += str_item;
      }
      AddLog (LOG_LEVEL_INFO, PSTR ("  msgtype=%u (message type : %s)"), teleinfo_config.msg_type, str_line.c_str ());

#ifdef USE_TELEINFO_GRAPH
      AddLog (LOG_LEVEL_INFO, PSTR ("  maxv=%u     (graph max voltage, in V)"), teleinfo_config.max_volt);
      AddLog (LOG_LEVEL_INFO, PSTR ("  maxva=%u    (graph max power, in VA or W)"), teleinfo_config.max_power);

      AddLog (LOG_LEVEL_INFO, PSTR ("  nbday=%d    (number of daily logs)"), teleinfo_config.param[TELEINFO_CONFIG_NBDAY]);
      AddLog (LOG_LEVEL_INFO, PSTR ("  nbweek=%d   (number of weekly logs)"), teleinfo_config.param[TELEINFO_CONFIG_NBWEEK]);

      AddLog (LOG_LEVEL_INFO, PSTR ("  maxhour=%d  (graph max total per hour, in Wh)"), teleinfo_config.param[TELEINFO_CONFIG_MAX_HOUR]);
      AddLog (LOG_LEVEL_INFO, PSTR ("  maxday=%d   (graph max total per day, in Wh)"), teleinfo_config.param[TELEINFO_CONFIG_MAX_DAY]);
      AddLog (LOG_LEVEL_INFO, PSTR ("  maxmonth=%d (graph max total per month, in Wh)"), teleinfo_config.param[TELEINFO_CONFIG_MAX_MONTH]);
#endif    // USE_TELEINFO_GRAPH

      serviced = true;
    }

    // else some configuration params are given
    else
    {
      // loop thru parameters
      pstr_next = XdrvMailbox.data;
      do
      {
        // extract key, value and next param
        pstr_key   = pstr_next;
        pstr_value = strchr (pstr_next, '=');
        pstr_next  = strchr (pstr_key, ' ');

        // split key, value and next param
        if (pstr_value != nullptr) { *pstr_value = 0; pstr_value++; }
        if (pstr_next != nullptr) { *pstr_next = 0; pstr_next++; }

        // if param is defined, handle it
        if (pstr_key != nullptr) serviced |= TeleinfoExecuteCommand (pstr_key, pstr_value);
      } 
      while (pstr_next != nullptr);


      // if needed, save confoguration
      if (serviced) TeleinfoSaveConfig ();
    }
  }

  return serviced;
}

bool TeleinfoExecuteCommand (const char* pstr_command, const char* pstr_param)
{
  bool serviced = false;
  int  index;
  long value;
  char str_buffer[32];

  // check parameter
  if (pstr_command == nullptr) return false;

  // check for command and value
  index = GetCommandCode (str_buffer, sizeof(str_buffer), pstr_command, kTeleinfoCommands);
  if (pstr_param == nullptr) value = 0; else value = atol (pstr_param);

  // handle command
  switch (index)
  {
    case TIC_CMND_MODE:
      if (value == 0) Settings->baudrate = 4;             // 1200 bauds
      else if (value == 1) Settings->baudrate = 32;       // 9600 bauds
      serviced = true;
      break;

    case TIC_CMND_HISTORIQUE:
      Settings->baudrate = 4;                             // 1200 bauds
      serviced = true;
      break;

    case TIC_CMND_STANDARD:
      Settings->baudrate = 32;                            // 9600 bauds
      serviced = true;
      break;

    case TIC_CMND_LED:
      teleinfo_config.led = (uint8_t)value;               // enable LED display
      serviced = true;
#ifdef USE_LIGHT
      // reset light controller
      light_controller.changeRGB (0, 0, 0, false);
#endif      // USE_LIGHT
      break;

    case TIC_CMND_STATS:
      AddLog (LOG_LEVEL_INFO, PSTR (" - Messages : %d"), teleinfo_meter.nb_message);
      AddLog (LOG_LEVEL_INFO, PSTR (" - Cos φ    : %d"), teleinfo_meter.nb_update);
      AddLog (LOG_LEVEL_INFO, PSTR (" - Erreurs  : %d"), (long)teleinfo_meter.nb_error);
      AddLog (LOG_LEVEL_INFO, PSTR (" - Reset    : %d"), teleinfo_meter.nb_reset);
      serviced = true;
      break;

    case TIC_CMND_MSG_POLICY:
      serviced = (value < TELEINFO_POLICY_MAX);
      if (serviced) teleinfo_config.msg_policy = (uint8_t)value;
      break;

    case TIC_CMND_MSG_TYPE:
      serviced = (value < TELEINFO_MSG_MAX);
      if (serviced) teleinfo_config.msg_type = (uint8_t)value;
      break;

    case TIC_CMND_CONTRACT:
      serviced = (value < TELEINFO_PERCENT_MAX);
      if (serviced) teleinfo_config.percent = (uint8_t)value;
      break;

#ifdef USE_TELEINFO_GRAPH
    case TIC_CMND_MAX_V:
      serviced = ((value >= TELEINFO_GRAPH_MIN_VOLTAGE) && (value <= TELEINFO_GRAPH_MAX_VOLTAGE));
      if (serviced) teleinfo_config.max_volt = value;
      break;

    case TIC_CMND_MAX_VA:
      serviced = ((value >= TELEINFO_GRAPH_MIN_POWER) && (value <= TELEINFO_GRAPH_MAX_POWER));
      if (serviced) teleinfo_config.max_power = value;
      break;

    case TIC_CMND_LOG_DAY:
      serviced = ((value > 0) && (value <= 31));
      if (serviced) teleinfo_config.param[TELEINFO_CONFIG_NBDAY] = value;
      break;

    case TIC_CMND_LOG_WEEK:
      serviced = ((value > 0) && (value <= 52));
      if (serviced) teleinfo_config.param[TELEINFO_CONFIG_NBWEEK] = value;
      break;

    case TIC_CMND_MAX_KWH_HOUR:
      serviced = ((value >= TELEINFO_GRAPH_MIN_WH_HOUR) && (value <= TELEINFO_GRAPH_MAX_WH_HOUR));
      if (serviced) teleinfo_config.param[TELEINFO_CONFIG_MAX_HOUR] = value;
      break;

    case TIC_CMND_MAX_KWH_DAY:
      serviced = ((value >= TELEINFO_GRAPH_MIN_WH_DAY) && (value <= TELEINFO_GRAPH_MAX_WH_DAY));
      if (serviced) teleinfo_config.param[TELEINFO_CONFIG_MAX_DAY] = value;
      break;

    case TIC_CMND_MAX_KWH_MONTH:
      serviced = ((value >= TELEINFO_GRAPH_MIN_WH_MONTH) && (value <= TELEINFO_GRAPH_MAX_WH_MONTH));
      if (serviced) teleinfo_config.param[TELEINFO_CONFIG_MAX_MONTH] = value;
      break;
#endif    // USE_TELEINFO_GRAPH
  }

  return serviced;
}

/*********************************************\
 *               Functions
\*********************************************/

#ifndef ESP32

// conversion from long long to string (not available in standard libraries)
char* lltoa (const long long value, char *pstr_result, const int base)
{
  lldiv_t result;
  char    str_value[12];

  // check parameters
  if (pstr_result == nullptr) return nullptr;
  if (base != 10) return nullptr;

  // if needed convert upper digits
  result = lldiv (value, 10000000000000000LL);
  if (result.quot != 0) ltoa ((long)result.quot, pstr_result, 10);
    else strcpy (pstr_result, "");

  // convert middle digits
  result = lldiv (result.rem, 100000000LL);
  if (result.quot != 0)
  {
    if (strlen (pstr_result) == 0) ltoa ((long)result.quot, str_value, 10);
      else sprintf_P (str_value, PSTR ("%08d"), (long)result.quot);
    strcat (pstr_result, str_value);
  }

  // convert lower digits
  if (strlen (pstr_result) == 0) ltoa ((long)result.rem, str_value, 10);
    else sprintf_P (str_value, PSTR ("%08d"), (long)result.rem);
  strcat (pstr_result, str_value);

  return pstr_result;
}

#endif      // ESP32

// General update (wifi & serial)
void TeleinfoDataUpdate ()
{
  // give control back to system to avoid watchdog
  yield ();

  // read serial data
  TeleinfoReceiveData ();
}

// Start serial reception
bool TeleinfoEnableSerial ()
{
  bool is_ready = (teleinfo_serial != nullptr);

  // if serial port is not already created
  if (!is_ready)
  { 
    // check if environment is ok
    is_ready = ((TasmotaGlobal.energy_driver == XNRG_15) && PinUsed (GPIO_TELEINFO_RX) && (Settings->baudrate > 0));
    if (is_ready)
    {
#ifdef ESP32
      // create and initialise serial port
      teleinfo_serial = new TasmotaSerial (Pin (GPIO_TELEINFO_RX), -1, 1);
      is_ready = teleinfo_serial->begin (Settings->baudrate * 300, SERIAL_7E1);

#else       // ESP8266
      // create and initialise serial port
      teleinfo_serial = new TasmotaSerial (Pin (GPIO_TELEINFO_RX), -1, 1);
      is_ready = teleinfo_serial->begin (Settings->baudrate * 300, SERIAL_7E1);

      // force configuration on ESP8266
      if (teleinfo_serial->hardwareSerial ()) ClaimSerial ();
#endif      // ESP32 & ESP8266

      // flush transmit and receive buffer
      if (is_ready)
      {
        teleinfo_serial->flush ();
        while (teleinfo_serial->available()) teleinfo_serial->read (); 
      }

      // log action
      if (is_ready) AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Serial set to 7E1 %d bauds"), Settings->baudrate * 300);
        else AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Serial port init failed"));
    }

    // set serial port status
    if (is_ready) teleinfo_meter.status_rx = TIC_SERIAL_ACTIVE; 
      else teleinfo_meter.status_rx = TIC_SERIAL_FAILED;
  }

  // if Teleinfo En declared, enable it
  if (is_ready && PinUsed (GPIO_TELEINFO_ENABLE)) digitalWrite(Pin (GPIO_TELEINFO_ENABLE), HIGH);

  return is_ready;
}

// Stop serial reception
bool TeleinfoDisableSerial ()
{
  // if teleinfo enabled, set it low
  if (PinUsed (GPIO_TELEINFO_ENABLE)) digitalWrite(Pin (GPIO_TELEINFO_ENABLE), LOW);

  // declare serial as stopped
  teleinfo_meter.status_rx = TIC_SERIAL_STOPPED;

  return true;
}

// Load configuration from Settings or from LittleFS
void TeleinfoLoadConfig () 
{
  // load standard settings
  teleinfo_config.led        = Settings->teleinfo.led;
  teleinfo_config.msg_policy = Settings->teleinfo.msg_policy;
  teleinfo_config.msg_type   = Settings->teleinfo.msg_type;
  teleinfo_config.percent    = Settings->teleinfo.percent;
  teleinfo_config.max_volt   = TELEINFO_GRAPH_MIN_VOLTAGE + Settings->teleinfo.adjust_v * 5;
  teleinfo_config.max_power  = TELEINFO_GRAPH_MIN_POWER + Settings->teleinfo.adjust_va * 3000;

  // load littlefs settings
#ifdef USE_UFILESYS
  int    index;
  char   str_text[16];
  char   str_line[64];
  char   *pstr_key, *pstr_value;
  File   file;

  // if file exists, open and read each line
  if (ffsp->exists (D_TELEINFO_CFG))
  {
    file = ffsp->open (D_TELEINFO_CFG, "r");
    while (file.available ())
    {
      // read current line and extract key and value
      index = file.readBytesUntil ('\n', str_line, sizeof (str_line) - 1);
      if (index >= 0) str_line[index] = 0;
      pstr_key   = strtok (str_line, "=");
      pstr_value = strtok (nullptr,  "=");

      // if key and value are defined, look for config keys
      if ((pstr_key != nullptr) && (pstr_value != nullptr))
      {
        index = GetCommandCode (str_text, sizeof (str_text), pstr_key, kTeleinfoConfigKey);
        if ((index >= 0) && (index < TELEINFO_CONFIG_MAX)) teleinfo_config.param[index] = atol (pstr_value);
      }
    }
  }
# endif     // USE_UFILESYS

  // validate boundaries
  if (Settings->baudrate == 0) Settings->baudrate = 9600 / 300;
  if ((teleinfo_config.msg_policy < 0) || (teleinfo_config.msg_policy >= TELEINFO_POLICY_MAX)) teleinfo_config.msg_policy = TELEINFO_POLICY_TELEMETRY;
  if (teleinfo_config.msg_type >= TELEINFO_MSG_MAX) teleinfo_config.msg_type = TELEINFO_MSG_METER;
  if ((teleinfo_config.percent < TELEINFO_PERCENT_MIN) || (teleinfo_config.percent > TELEINFO_PERCENT_MAX)) teleinfo_config.percent = 100;

  // log
  AddLog (LOG_LEVEL_DEBUG, PSTR ("TIC: Loading config"));
}

// Save configuration to Settings or to LittleFS
void TeleinfoSaveConfig () 
{
  // save standard settings
  Settings->teleinfo.led        = teleinfo_config.led;
  Settings->teleinfo.msg_policy = teleinfo_config.msg_policy;
  Settings->teleinfo.msg_type   = teleinfo_config.msg_type;
  Settings->teleinfo.percent    = teleinfo_config.percent;
  Settings->teleinfo.adjust_v   = (teleinfo_config.max_volt - TELEINFO_GRAPH_MIN_VOLTAGE) / 5;
  Settings->teleinfo.adjust_va  = (teleinfo_config.max_power - TELEINFO_GRAPH_MIN_POWER) / 3000;

  // save littlefs settings
#ifdef USE_UFILESYS
  uint8_t index;
  char    str_value[16];
  char    str_text[32];
  File    file;

  // open file and write content
  file = ffsp->open (D_TELEINFO_CFG, "w");
  for (index = 0; index < TELEINFO_CONFIG_MAX; index ++)
  {
    if (GetTextIndexed (str_text, sizeof (str_text), index, kTeleinfoConfigKey) != nullptr)
    {
      // generate key=value
      strcat (str_text, "=");
      ltoa (teleinfo_config.param[index], str_value, 10);
      strcat (str_text, str_value);
      strcat (str_text, "\n");

      // write to file
      file.print (str_text);
    }
  }

  file.close ();
# endif     // USE_UFILESYS

  // log
  AddLog (LOG_LEVEL_DEBUG, PSTR ("TIC: Saving config"));
}

// calculate line checksum
char TeleinfoCalculateChecksum (const char* pstr_line, char* pstr_etiquette, char* pstr_donnee) 
{
  bool     prev_space = true;
  uint8_t  line_checksum  = 0;
  uint8_t  given_checksum = 0;
  uint32_t index, line_size;
  char     str_line[TELEINFO_LINE_MAX];
  char     *pstr_token, *pstr_key, *pstr_data;

  // if given line exists
  if ((pstr_line == nullptr) || (pstr_etiquette == nullptr) || (pstr_donnee == nullptr)) return 0;

  // init result
  strcpy (str_line, "");
  strcpy (pstr_etiquette, "");
  strcpy (pstr_donnee, "");
  pstr_key = pstr_data = nullptr;

  // get given checksum
  line_size = strlen (pstr_line) - 1;
  if (line_size < 4) return 0;
  given_checksum = (uint8_t)pstr_line[line_size];

  // adjust checksum calculation according to mode
  if (teleinfo_meter.sep_line == ' ') line_size--;

  // loop to calculate checksum, keeping 6 lower bits and adding Ox20 and compare to given checksum
  for (index = 0; index < line_size; index ++) line_checksum += (uint8_t)pstr_line[index];
  line_checksum = (line_checksum & 0x3F) + 0x20;

  // if different than given checksum,  increase checksum error counter and log
  if (line_checksum != given_checksum) line_checksum = 0;

  // else extract etiquette and donnee
  if (line_checksum != 0)
  {
    // replace all TABS with SPACE and remove multiple SPACE
    line_size = strlen (pstr_line);
    for (index = 0; index < line_size; index ++)
    {
      // if current character is a separator (TAB or SPACE)
      pstr_token = (char*)pstr_line + index;
      if ((*pstr_token == 0x09) || (*pstr_token == ' '))
      {
        // if first SPACE and not last character
        if (!prev_space) strcat (str_line, " ");

        // previous character is as separator
        prev_space = true;
      }

      // else character is not a separator
      else
      {
        // copy current character
        strncat (str_line, pstr_token, 1);

        // reset previous character as separator
        prev_space = false;
      }
    }

    // extract key
    pstr_key = str_line;

    // extrat data
    pstr_data = strchr (str_line, ' ');
    if (pstr_data != nullptr)
    {
      // set data start
      *pstr_data = 0;
      pstr_data++;

      // remove checksum
      pstr_token = strrchr (pstr_data, ' ');
      if (pstr_token != nullptr) *pstr_token = 0;
    }

    // set etiquette and donnee
    if ((pstr_key != nullptr) && (pstr_data != nullptr))
    {
      strlcpy (pstr_etiquette, pstr_key, TELEINFO_KEY_MAX);
      strlcpy (pstr_donnee, pstr_data, TELEINFO_DATA_MAX);
    }

    // else, invalidate checksum
    else line_checksum = 0;
  }

  // handle error
  if (line_checksum == 0)
  {
    teleinfo_meter.nb_error++;
    AddLog (LOG_LEVEL_DEBUG, PSTR ("TIC: Error %s"), pstr_line);
  } 

  return line_checksum;
}

// update global production counter
void TeleinfoUpdateProdGlobalCounter (const char* pstr_value)
{
  long long value;

  // check parameter
  if (pstr_value == nullptr) return;

  // update if valid, previous value was 0 or if superior and less than 10% increase (to avoid meter spikes)
  value = atoll (pstr_value);
  if (value < LONG_LONG_MAX)
  {
    // previous value was 0
    if (teleinfo_meter.prod_total_wh == 0) teleinfo_meter.prod_total_wh = value;

    // else less than 10% increase
    else if ((value > teleinfo_meter.prod_total_wh) && (value < teleinfo_meter.prod_total_wh * 11 / 10)) teleinfo_meter.prod_total_wh = value;
  }
}

// update global consommation counter
void TeleinfoUpdateConsoGlobalCounter (const long long value)
{
  // update if valid, previous value was 0 or if superior and less than 10% increase (to avoid meter spikes)
  if (value < LONG_LONG_MAX)
  {
    // previous value was 0
    if (teleinfo_meter.conso_total_wh == 0)
    {
      teleinfo_meter.conso_delta_wh = 0;
      teleinfo_meter.conso_total_wh = value;
    }

    // else less than 10% increase
    else if ((value > teleinfo_meter.conso_total_wh) && (value < teleinfo_meter.conso_total_wh * 11 / 10))
    {
      teleinfo_meter.conso_delta_wh = value - teleinfo_meter.conso_total_wh;
      teleinfo_meter.conso_total_wh = value;
    }
  }
}

// update indexed consommation counter
void TeleinfoUpdateConsoIndexCounter (const char* pstr_value, const uint8_t index)
{
  long long value;

  // check parameter
  if (pstr_value == nullptr) return;
  if (index >= TELEINFO_INDEX_MAX) return;

  // calculate and update
  value = atoll (pstr_value);
  if ((value < LONG_LONG_MAX) && (value > teleinfo_meter.conso_index_wh[index])) teleinfo_meter.conso_index_wh[index] = value;
}

// update phase voltage
void TeleinfoUpdateVoltage (const char* pstr_value, const uint8_t phase, const bool is_rms)
{
  long value;

  // check parameter
  if (pstr_value == nullptr) return;
  if (phase >= ENERGY_MAX_PHASES) return;

  // calculate and update
  value = atol (pstr_value);
  if (value > TELEINFO_VOLTAGE_MAXIMUM) value = LONG_MAX;

  // if value is valid
  if ((value > 0) && (value != LONG_MAX))
  {
    // rms voltage
    if (is_rms)
    {
      teleinfo_phase[phase].volt_set = true;
      teleinfo_phase[phase].voltage = value;
    }

    // average voltage
    else if (!teleinfo_phase[phase].volt_set) teleinfo_phase[phase].voltage = value;
  }
}

// update phase current
void TeleinfoUpdateCurrent (const char* pstr_value, const uint8_t phase)
{
  long value;

  // check parameter
  if (pstr_value == nullptr) return;
  if (phase >= ENERGY_MAX_PHASES) return;
  
  // calculate and update
  value = atol (pstr_value);
  if ((value >= 0) && (value != LONG_MAX)) teleinfo_phase[phase].current = 100 * value; 
}

void TeleinfoUpdateProdApparentPower (const char* pstr_value)
{
  long value;

  // check parameter
  if (pstr_value == nullptr) return;

  // calculate and update
  value = atol (pstr_value);
  if ((value >= 0) && (value != LONG_MAX)) teleinfo_meter.prod_papp = value;
}

void TeleinfoUpdateConsoApparentPower (const char* pstr_value)
{
  long value;

  // check parameter
  if (pstr_value == nullptr) return;

  // calculate and update
  value = atol (pstr_value);
  if ((value >= 0) && (value < LONG_MAX)) teleinfo_meter.conso_papp = value;
}

// set contract period
void TeleinfoUpdateConsoPeriod (const char* pstr_period)
{
  int  period;
  char str_text[32];

  // check parameter
  if (pstr_period == nullptr) return;

  // look for period
  period = GetCommandCode (str_text, sizeof (str_text), pstr_period, kTeleinfoPeriod);
  if (period == -1) return;

  // set period
  teleinfo_contract.period_current = (uint8_t)period;

  // if not already done, set period family
  if (teleinfo_contract.period_first == UINT8_MAX)
  {
    // update number of indexes
    teleinfo_contract.period_first = ARR_TELEINFO_PERIOD_FIRST[teleinfo_contract.period_current];
    teleinfo_contract.period_qty   = ARR_TELEINFO_PERIOD_QUANTITY[teleinfo_contract.period_current];

    // log
    GetTextIndexed (str_text, sizeof (str_text), teleinfo_contract.period_current, kTeleinfoPeriod);
    AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Period %s detected, %u period(s)"), str_text, teleinfo_contract.period_qty);
  }
}

/*********************************************\
 *                   Callback
\*********************************************/

// Teleinfo GPIO initilisation
void TeleinfoPreInit ()
{
  // if TeleinfoEN pin defined, switch it OFF
  if (PinUsed (GPIO_TELEINFO_ENABLE))
  {
    // switch off GPIO_TELEINFO_ENABLE to avoid ESP32 Rx heavy load restart bug
    pinMode (Pin (GPIO_TELEINFO_ENABLE, 0), OUTPUT);

    // disable serial input
    TeleinfoDisableSerial ();
  }

  // if TeleinfoRX pin defined, set energy driver as teleinfo
  if (!TasmotaGlobal.energy_driver && PinUsed (GPIO_TELEINFO_RX))
  {
    // set GPIO_TELEINFO_RX as digital input
    pinMode (Pin (GPIO_TELEINFO_RX, 0), INPUT);

    // declare energy driver
    TasmotaGlobal.energy_driver = XNRG_15;

    // log
    AddLog (LOG_LEVEL_INFO, PSTR("TIC: Teleinfo driver enabled"));
  }
}

// Teleinfo driver initialisation
void TeleinfoInit ()
{
  int     index;
  uint8_t phase;

#ifdef USE_UFILESYS
  // log result
  if (ufs_type) AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Partition mounted"));
  else AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Partition could not be mounted"));
#endif  // USE_UFILESYS

  // set measure resolution
  Settings->flag2.current_resolution = 1;

  // disable fast cycle power recovery (SetOption65)
  Settings->flag3.fast_power_cycle_disable = true;

  // init hardware energy counter
  Settings->flag3.hardware_energy_total = true;

  // set default energy parameters
  Energy->voltage_available = true;
  Energy->current_available = true;

  // load configuration
  TeleinfoLoadConfig ();

  // initialise message timestamp
  teleinfo_message.timestamp = UINT32_MAX;

  // loop thru all possible phases
  for (phase = 0; phase < ENERGY_MAX_PHASES; phase++)
  {
    // tasmota energy counters
    Energy->voltage[phase]        = TELEINFO_VOLTAGE;
    Energy->current[phase]        = 0;
    Energy->active_power[phase]   = 0;
    Energy->apparent_power[phase] = 0;

    // initialise phase data
    teleinfo_phase[phase].volt_set  = false;
    teleinfo_phase[phase].voltage   = TELEINFO_VOLTAGE;
    teleinfo_phase[phase].current   = 0;
    teleinfo_phase[phase].papp      = 0;
    teleinfo_phase[phase].pact      = 0;
    teleinfo_phase[phase].papp_last = 0;
    teleinfo_phase[phase].cosphi    = 100;

    // initialise calculation data
    teleinfo_calc.papp_inc[phase] = 0;
  }

  // initialise meter data
  for (index = 0; index < TELEINFO_INDEX_MAX; index ++) teleinfo_meter.conso_index_wh[index] = 0;
  teleinfo_meter.sep_line = ' ';

  // initialise message data
  strcpy (teleinfo_meter.str_line, "");
  for (index = 0; index < TELEINFO_LINE_QTY; index ++) 
  {
    teleinfo_message.line[index].str_etiquette = "";
    teleinfo_message.line[index].str_donnee = "";
    teleinfo_message.line[index].checksum = 0;
  }

  // log default method & help command
  AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Using default Global Counter method"));
  AddLog (LOG_LEVEL_INFO, PSTR ("HLP: EnergyConfig to get help on all Teleinfo commands"));

#ifdef USE_LIGHT
  // set light controller brightness
  if (teleinfo_config.led) light_controller.changeBri (128);
    else light_controller.changeBri (0);
#endif      // USE_LIGHT
}

// Handling of received teleinfo data
void TeleinfoReceiveData ()
{
//  bool      is_valid;
  uint8_t   phase, period;
  int       index;
  long      value, total, increment, total_current, cosphi, green, red;
  long long counter, counter_wh;
  uint32_t  timeout, timestamp, message_ms;
  float     papp_inc, daily_kwh;
  char      checksum;
  char*     pstr_match;
  char      str_byte[2] = {0, 0};
  char      str_etiquette[TELEINFO_KEY_MAX];
  char      str_donnee[TELEINFO_DATA_MAX];
  char      str_text[TELEINFO_DATA_MAX];
  
  // serial receive loop
  while (teleinfo_serial->available ()) 
  {
    // read character
    str_byte[0] = (char)teleinfo_serial->read (); 

    // if teleinfo enabled
    if ((TasmotaGlobal.uptime > ENERGY_WATCHDOG) && (teleinfo_meter.status_rx == TIC_SERIAL_ACTIVE))
    {
      // set timestamp and update message length
      timestamp = millis ();
      teleinfo_message.length++;

      // handle caracter
      switch (str_byte[0])
      {
        // ---------------------------
        // 0x02 : Beginning of message 
        // ---------------------------

        case 0x02:
          // reset message line index & message length
          teleinfo_message.line_index = 0;
          teleinfo_message.length = 1;

          // init current line
          strcpy (teleinfo_meter.str_line, "");

          // reset voltage flags
          for (phase = 0; phase < teleinfo_contract.phase; phase++) teleinfo_phase[phase].volt_set = false;
          break;
            
        // ---------------------------
        // 0x04 : Reset of message 
        // ---------------------------

        case 0x04:
          // increment reset counter and reset message line index
          teleinfo_meter.nb_reset++;
          teleinfo_message.line_index = 0;

          // init current line
          strcpy (teleinfo_meter.str_line, "");

          // log
          AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Message reset"));
          break;

        // -----------------------------
        // 0x0A : Line - Beginning
        // 0x0E : On some faulty meters
        // -----------------------------

        case 0x0A:
        case 0x0E:
          strcpy (teleinfo_meter.str_line, "");
          break;

        // ------------------------------
        // \t : Line - Separator
        // ------------------------------

        case 0x09:
          teleinfo_meter.sep_line = 0x09;
          strlcat (teleinfo_meter.str_line, str_byte, TELEINFO_LINE_MAX);
          break;
          
        // ------------------
        // 0x0D : Line - End
        // ------------------

        case 0x0D:
          // increment counter
          teleinfo_meter.nb_line++;

          // drop first message
          if (teleinfo_meter.nb_line > 1)
          {
            // if checksum is ok, handle the line
            checksum = TeleinfoCalculateChecksum (teleinfo_meter.str_line, str_etiquette, str_donnee);
            if (checksum != 0)
            {
              // get etiquette type
              index = GetCommandCode (str_text, sizeof (str_text), str_etiquette, kTeleinfoEtiquetteName);

              // update data according to etiquette
              switch (index)
              {
                // contract : reference
                case TIC_ADCO:
                  teleinfo_contract.mode = TIC_MODE_HISTORIC;
                    if (teleinfo_contract.ident == 0) teleinfo_contract.ident = atoll (str_donnee);
                  break;

                case TIC_ADSC:
                  teleinfo_contract.mode = TIC_MODE_STANDARD;
                    if (teleinfo_contract.ident == 0) teleinfo_contract.ident = atoll (str_donnee);
                  break;

                case TIC_ADS:
                    if (teleinfo_contract.ident == 0) teleinfo_contract.ident = atoll (str_donnee);
                  break;

                // contract : mode and type
                case TIC_CONFIG:
                  teleinfo_contract.mode = TIC_MODE_PME;
                  break;

                case TIC_NGTF:
                  break;

                // period name
                case TIC_PTEC:
                case TIC_LTARF:
                case TIC_PTCOUR:
                case TIC_PTCOUR1:
                  TeleinfoUpdateConsoPeriod (str_donnee);
                  break;

                // instant current
                case TIC_IINST:
                case TIC_IRMS1:
                case TIC_IINST1:
                  TeleinfoUpdateCurrent (str_donnee, 0);
                  break;

                case TIC_IRMS2:
                case TIC_IINST2:
                  TeleinfoUpdateCurrent (str_donnee, 1);
                  break;

                case TIC_IRMS3:
                case TIC_IINST3:
                  TeleinfoUpdateCurrent (str_donnee, 2);
                  teleinfo_contract.phase = 3; 
                  break;

                // if in conso mode, instant apparent power, 
                case TIC_PAPP:
                case TIC_SINSTS:
                  TeleinfoUpdateConsoApparentPower (str_donnee);
                  break;

                // if in prod mode, instant apparent power, 
                case TIC_SINSTI:
                  TeleinfoUpdateProdApparentPower (str_donnee);
                  break;

                // apparent power counter
                case TIC_EAPPS:
                  if (teleinfo_calc.method != TIC_METHOD_INCREMENT) AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Switching to VA/W increment method"));
                  teleinfo_calc.method = TIC_METHOD_INCREMENT;
                  strlcpy (str_text, str_donnee, sizeof (str_text));
                  pstr_match = strchr (str_text, 'V');
                  if (pstr_match != nullptr) *pstr_match = 0;
                  value = atol (str_text);
                  if ((value >= 0) && (value < LONG_MAX)) teleinfo_calc.papp_current_counter = value;
                  break;

                // active Power Counter
                case TIC_EAS:
                  if (teleinfo_calc.method != TIC_METHOD_INCREMENT) AddLog (LOG_LEVEL_INFO, PSTR ("TIC: Switching to VA/W increment method"));
                  teleinfo_calc.method = TIC_METHOD_INCREMENT;
                  strlcpy (str_text, str_donnee, sizeof (str_text));
                  pstr_match = strchr (str_text, 'W');
                  if (pstr_match != nullptr) *pstr_match = 0;
                  value = atol (str_text);
                  if (value < LONG_MAX) teleinfo_calc.pact_current_counter = value;
                  break;

                // Voltage
                case TIC_URMS1:
                  TeleinfoUpdateVoltage (str_donnee, 0, true);
                  break;

                case TIC_UMOY1:
                  TeleinfoUpdateVoltage (str_donnee, 0, false);
                  break;

                case TIC_URMS2:
                  TeleinfoUpdateVoltage (str_donnee, 1, true);
                  break;
                case TIC_UMOY2:
                  TeleinfoUpdateVoltage (str_donnee, 1, false);
                  break;

                case TIC_URMS3:
                  TeleinfoUpdateVoltage (str_donnee, 2, true);
                  teleinfo_contract.phase = 3; 
                  break;

                case TIC_UMOY3:
                  TeleinfoUpdateVoltage (str_donnee, 2, false);
                  teleinfo_contract.phase = 3; 
                  break;

                // Contract : Maximum Current
                case TIC_ISOUSC:
                  value = atol (str_donnee);
                  if ((value > 0) && (value < LONG_MAX) && (teleinfo_contract.isousc != value))
                  {
                    // update max current and power
                    teleinfo_contract.isousc = value;
                    teleinfo_contract.ssousc = value * TELEINFO_VOLTAGE_REF;
                    teleinfo_config.max_power = teleinfo_contract.ssousc;
                  }
                  break;

                // Contract : Maximum Power (kVA)
                case TIC_PREF:
                case TIC_PCOUP:
                  value = atol (str_donnee);
                  if ((value > 0) && (value < LONG_MAX) && (teleinfo_contract.phase > 0))
                  {
                    value = value * 1000;
                    if (teleinfo_contract.phase > 1) value = value / teleinfo_contract.phase;
                    if (teleinfo_contract.ssousc != value)
                    {
                      // update max current and power
                      teleinfo_contract.isousc = value / TELEINFO_VOLTAGE_REF;
                      teleinfo_contract.ssousc = value;
                      teleinfo_config.max_power = value;
                    }                   
                  }
                  break;

                case TIC_PS:
                  strlcpy (str_text, str_donnee, sizeof (str_text));

                  // replace xxxk by xxx000
                  pstr_match = strchr (str_text, 'k');
                  if (pstr_match != nullptr) 
                  {
                    *pstr_match = 0;
                    strlcat (str_text, "000", sizeof (str_text));
                  }
                  value = atol (str_text);
                  if ((value > 0) && (value < LONG_MAX))
                  {
                    if (teleinfo_contract.ssousc != value)
                    {
                      // update max current and power
                      teleinfo_contract.isousc = value / TELEINFO_VOLTAGE_REF;
                      teleinfo_contract.ssousc = value;
                      teleinfo_config.max_power = value;
                    }                   
                  }
                  break;

                // Contract : Index suivant type de contrat
                case TIC_EAPS:
                  strlcpy (str_text, str_donnee, sizeof (str_text));
                  pstr_match = strchr (str_text, 'k');
                  if (pstr_match != nullptr) strcpy (pstr_match, "000");
                  period = teleinfo_contract.period_current - teleinfo_contract.period_first;
                  TeleinfoUpdateConsoIndexCounter (str_text, period);
                  break;

                case TIC_EAST:
                  break;

                case TIC_EAIT:
                  TeleinfoUpdateProdGlobalCounter (str_donnee);
                  break;

                case TIC_BASE:
                case TIC_HCHC:
                case TIC_EJPHN:
                case TIC_BBRHCJB:
                case TIC_EASF01:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 0);
                  break;

                case TIC_HCHP:
                case TIC_EJPHPM:
                case TIC_BBRHPJB:
                case TIC_EASF02:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 1);
                  break;

                case TIC_BBRHCJW:
                case TIC_EASF03:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 2);
                  break;

                case TIC_BBRHPJW:
                case TIC_EASF04:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 3);
                  break;

                case TIC_BBRHCJR:
                case TIC_EASF05:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 4);
                  break;

                case TIC_BBRHPJR:
                case TIC_EASF06:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 5);
                  break;

                case TIC_EASF07:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 6);
                  break;

                case TIC_EASF08:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 7);
                  break;

                case TIC_EASF09:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 8);
                break;

                case TIC_EASF10:
                  TeleinfoUpdateConsoIndexCounter (str_donnee, 9);
                  break;

                // Overload
                case TIC_ADPS:
                case TIC_ADIR1:
                case TIC_ADIR2:
                case TIC_ADIR3:
                case TIC_PREAVIS:
                  teleinfo_message.overload = true;
                  break;
              }

              // if maximum number of lines not reached, save new line
              index = teleinfo_message.line_index;
              if (index < TELEINFO_LINE_QTY)
              {
                teleinfo_message.line[index].str_etiquette = str_etiquette;
                teleinfo_message.line[index].str_donnee = str_donnee;
                teleinfo_message.line[index].checksum = checksum;
              }
            }

            // increment line index
            teleinfo_message.line_index++;
          }
          
          // init current line
          strcpy (teleinfo_meter.str_line, "");
          break;

        // ---------------------
        // Ox03 : End of message
        // ---------------------

        case 0x03:
          // set message timestamp
          teleinfo_message.timestamp = timestamp;

          // increment message counter
          teleinfo_meter.nb_message++;

          // loop to remove unused message lines
          for (index = teleinfo_message.line_index; index < TELEINFO_LINE_QTY; index++)
          {
            teleinfo_message.line[index].str_etiquette = "";
            teleinfo_message.line[index].str_donnee = "";
            teleinfo_message.line[index].checksum = 0;
          }

          // save number of lines and reset index
          teleinfo_message.line_max   = min (teleinfo_message.line_index, TELEINFO_LINE_QTY);
          teleinfo_message.line_index = 0;

          // if needed, calculate max contract power from max phase current
          if ((teleinfo_contract.ssousc == 0) && (teleinfo_contract.isousc != 0)) teleinfo_contract.ssousc = teleinfo_contract.isousc * TELEINFO_VOLTAGE_REF;

          // ----------------------
          // counter global indexes

          // if needed, adjust number of phases
          if (Energy->phase_count < teleinfo_contract.phase) Energy->phase_count = teleinfo_contract.phase;

          // calculate total current
          total_current = 0;
          for (phase = 0; phase < teleinfo_contract.phase; phase++) total_current += teleinfo_phase[phase].current;

          // if first measure, init timing
          if (teleinfo_calc.previous_time_message == UINT32_MAX) teleinfo_calc.previous_time_message = teleinfo_message.timestamp;

          // calculate time window
          message_ms = TimeDifference (teleinfo_calc.previous_time_message, teleinfo_message.timestamp);

          // update total energy counter
          counter_wh = 0;
          if (teleinfo_contract.period_qty != UINT8_MAX) for (index = 0; index < teleinfo_contract.period_qty; index ++) counter_wh += teleinfo_meter.conso_index_wh[index];

          // set first global counter to avoid meter total errors :
          //  - total smaller than previous one
          //  - delta of more than 10% of previous total
          TeleinfoUpdateConsoGlobalCounter (counter_wh);

          // swtich according to calculation method
          switch (teleinfo_calc.method) 
          {
            // -----------------------------------------------
            //   global apparent power is provided
            //   active power calculated from global counter
            // -----------------------------------------------

            case TIC_METHOD_GLOBAL_COUNTER:
              // slit apparent power between phases, based on current per phase
              papp_inc = 0;
              for (phase = 0; phase < teleinfo_contract.phase; phase++)
              {
                // if current is given, apparent power is split between phase according to current ratio
                if (total_current > 0) teleinfo_phase[phase].papp = teleinfo_meter.conso_papp * teleinfo_phase[phase].current / total_current;

                // else if current is 0, apparent power is equally split between phases
                else if (teleinfo_contract.phase > 0) teleinfo_phase[phase].papp = teleinfo_meter.conso_papp / teleinfo_contract.phase;

                // add apparent power for message time window
                teleinfo_calc.papp_inc[phase] += (float)teleinfo_phase[phase].papp * (float)message_ms / 3600000;
                papp_inc += teleinfo_calc.papp_inc[phase];
              }

              // if global counter increment start is known
              if (teleinfo_calc.previous_time_counter != UINT32_MAX)
              {
                // if global counter has got incremented, calculate new cos phi, averaging with previous one to avoid peaks
                cosphi = teleinfo_phase[0].cosphi;
                if (teleinfo_meter.conso_delta_wh > 0)
                {
                  // new cosphi = 3/4 previous one + 1/4 calculated one, if new cosphi > 1, new cosphi = 1
                  if (papp_inc > 0) cosphi = cosphi * 3 / 4 + (long)(teleinfo_meter.conso_delta_wh * 25 / papp_inc);
                  if (cosphi > 100) cosphi = 100;
                  teleinfo_meter.nb_update++;
                }

                // else, if apparent power is above 1, cosphi will be drastically reduced when global counter will be increased
                //   so lets apply slow reduction to cosphi according to apparent power current increment
                else if (papp_inc > 1)
                {
                  cosphi = cosphi / 2 + (long)(50 / papp_inc);
                  if (cosphi > teleinfo_phase[0].cosphi) cosphi = teleinfo_phase[0].cosphi;
                }

                // apply cos phi to each phase
                for (phase = 0; phase < teleinfo_contract.phase; phase++) teleinfo_phase[phase].cosphi = cosphi;
              }

              // calculate active power per phase
              for (phase = 0; phase < teleinfo_contract.phase; phase++) teleinfo_phase[phase].pact = teleinfo_phase[phase].papp * teleinfo_phase[phase].cosphi / 100;

              // update calculation timestamp
              teleinfo_calc.previous_time_message = teleinfo_message.timestamp;
              if (teleinfo_meter.conso_delta_wh > 0)
              {
                // update calculation timestamp
                teleinfo_calc.previous_time_counter = teleinfo_message.timestamp;

                // reset apparent power sum for the period
                for (phase = 0; phase < teleinfo_contract.phase; phase++) teleinfo_calc.papp_inc[phase] = 0;

                // reset active power delta
                teleinfo_meter.conso_delta_wh = 0;
              }
            break;

            // -----------------------------------
            //   apparent power and active power
            //   are provided as increment
            // -----------------------------------

            case TIC_METHOD_INCREMENT:
              // apparent power : no increment available
              if (teleinfo_calc.papp_current_counter == LONG_MAX)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - VA counter unavailable"), teleinfo_meter.nb_message);
                teleinfo_calc.papp_previous_counter = LONG_MAX;
              }

              // apparent power : no comparison possible
              else if (teleinfo_calc.papp_previous_counter == LONG_MAX)
              {
                teleinfo_calc.papp_previous_counter = teleinfo_calc.papp_current_counter;
              }

              // apparent power : rollback
              else if (teleinfo_calc.papp_current_counter <= teleinfo_calc.papp_previous_counter / 100)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - VA counter rollback, start from %d"), teleinfo_meter.nb_message, teleinfo_calc.papp_current_counter);
                teleinfo_calc.papp_previous_counter = teleinfo_calc.papp_current_counter;
              }

              // apparent power : abnormal increment
              else if (teleinfo_calc.papp_current_counter < teleinfo_calc.papp_previous_counter)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - VA abnormal value, %d after %d"), teleinfo_meter.nb_message, teleinfo_calc.papp_current_counter, teleinfo_calc.papp_previous_counter);
                teleinfo_calc.papp_previous_counter = LONG_MAX;
              }

              // apparent power : do calculation
              else if (message_ms > 0)
              {
                increment = teleinfo_calc.papp_current_counter - teleinfo_calc.papp_previous_counter;
                teleinfo_phase[0].papp = teleinfo_phase[0].papp / 2 + 3600000 * increment / 2 / (long)message_ms;
                teleinfo_calc.papp_previous_counter = teleinfo_calc.papp_current_counter;
              }

              // active power : no increment available
              if (teleinfo_calc.pact_current_counter == LONG_MAX)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - W counter unavailable"), teleinfo_meter.nb_message);
                teleinfo_calc.pact_previous_counter = LONG_MAX;
              }

              // active power : no comparison possible
              else if (teleinfo_calc.pact_previous_counter == LONG_MAX)
              {
                teleinfo_calc.pact_previous_counter = teleinfo_calc.pact_current_counter;
              }

              // active power : detect rollback
              else if (teleinfo_calc.pact_current_counter <= teleinfo_calc.pact_previous_counter / 100)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - W counter rollback, start from %d"), teleinfo_meter.nb_message, teleinfo_calc.pact_current_counter);
                teleinfo_calc.pact_previous_counter = teleinfo_calc.pact_current_counter;
              }

              // active power : detect abnormal increment
              else if (teleinfo_calc.pact_current_counter < teleinfo_calc.pact_previous_counter)
              {
                AddLog (LOG_LEVEL_INFO, PSTR ("TIC: %d - W abnormal value, %d after %d"), teleinfo_meter.nb_message, teleinfo_calc.pact_current_counter, teleinfo_calc.pact_previous_counter);
                teleinfo_calc.pact_previous_counter = LONG_MAX;
              }

              // active power : do calculation
              else if (message_ms > 0)
              {
                increment = teleinfo_calc.pact_current_counter - teleinfo_calc.pact_previous_counter;
                teleinfo_phase[0].pact = teleinfo_phase[0].pact / 2 + 3600000 * increment / 2 / (long)message_ms;
                teleinfo_calc.pact_previous_counter = teleinfo_calc.pact_current_counter;
                teleinfo_meter.nb_update++;
              }

              // avoid active power greater than apparent power
              if (teleinfo_phase[0].pact > teleinfo_phase[0].papp) teleinfo_phase[0].pact = teleinfo_phase[0].papp;

              // calculate cos phi
              if (teleinfo_phase[0].papp > 0) teleinfo_phase[0].cosphi = 100 * teleinfo_phase[0].pact / teleinfo_phase[0].papp;

              // update previous message timestamp
              teleinfo_calc.previous_time_message = teleinfo_message.timestamp;

              // reset values
              teleinfo_calc.papp_current_counter = LONG_MAX;
              teleinfo_calc.pact_current_counter = LONG_MAX;
              break;
          }

          //  update flags
          // --------------

          // loop thru phases to detect overload
          for (phase = 0; phase < teleinfo_contract.phase; phase++)
            if (teleinfo_phase[phase].papp > teleinfo_contract.ssousc * (long)teleinfo_config.percent / 100) teleinfo_message.overload = true;

          // loop thru phase to detect % power change (should be > to handle 0 conso)
          for (phase = 0; phase < teleinfo_contract.phase; phase++) 
            if (abs (teleinfo_phase[phase].papp_last - teleinfo_phase[phase].papp) > (teleinfo_contract.ssousc * TELEINFO_PERCENT_CHANGE / 100)) teleinfo_message.percent = true;

          // detect % power change on production (should be > to handle 0 prod)
          if (abs (teleinfo_meter.prod_papp_last - teleinfo_meter.prod_papp) > (teleinfo_contract.ssousc * TELEINFO_PERCENT_CHANGE / 100)) teleinfo_message.percent = true;

          // if % power change detected, save values
          if (teleinfo_message.percent)
          {
            for (phase = 0; phase < teleinfo_contract.phase; phase++) teleinfo_phase[phase].papp_last = teleinfo_phase[phase].papp;
            teleinfo_meter.prod_papp_last = teleinfo_meter.prod_papp;
          }

          //  update energy counters
          // ------------------------

          // loop thru phases
          for (phase = 0; phase < teleinfo_contract.phase; phase++)
          {
            // set voltage
            Energy->voltage[phase] = (float)teleinfo_phase[phase].voltage;

            // set apparent and active power
            Energy->apparent_power[phase] = (float)teleinfo_phase[phase].papp;
            Energy->active_power[phase]   = (float)teleinfo_phase[phase].pact;

            // set current
            if (Energy->voltage[phase] > 0) Energy->current[phase] = Energy->apparent_power[phase] / Energy->voltage[phase];
              else Energy->current[phase] = 0;
            teleinfo_phase[phase].current = (long)(Energy->current[phase] * 100);
          } 

          //  update RGB lignt
          // ------------------------
#ifdef USE_LIGHT
          // update if percent changed
          if (teleinfo_config.led)
          {
            // switch led state
            teleinfo_meter.led_state = !teleinfo_meter.led_state;

            // if LED is on
            if (teleinfo_meter.led_state)
            {
              // calculate color according to power
              // green : low power
              // red   : full contract power
              value = total = 0;
              for (phase = 0; phase < teleinfo_contract.phase; phase++)
              {
                value += teleinfo_phase[phase].papp;
                total += teleinfo_contract.ssousc;
              }
              value = 100 * value / total;

              // calculate color
              if (value < 50) green = TELEINFO_RGB_GREEN_MAX; else green = TELEINFO_RGB_GREEN_MAX * 2 * (100 - value) / 100;
              if (value > 50) red = TELEINFO_RGB_RED_MAX; else red = TELEINFO_RGB_RED_MAX * 2 * value / 100;

              // update light controller
              light_controller.changeRGB ((uint8_t)red, (uint8_t)green, 0, false);
              teleinfo_meter.led_state = 1;
            }

            // else LED state is off
            else  light_controller.changeRGB (0, 0, 0, false);
          }
#endif      // USE_LIGHT

          // declare received message
          teleinfo_message.received = true;
          break;

        // add other caracters to current line
        default:
            strlcat (teleinfo_meter.str_line, str_byte, TELEINFO_LINE_MAX);
          break;
      }

#ifdef USE_TCPSERVER
      // if TCP server is active, append character to TCP buffer
      tcp_server.send (str_byte[0]); 
#endif
    }

    // give control back to system to avoid watchdog
    yield ();
  }
}

// Publish JSON if candidate (called 4 times per second)
void TeleinfoEvery250ms ()
{
  bool publish;

  // do nothing during energy setup time
  if (TasmotaGlobal.uptime < ENERGY_WATCHDOG) return;

  // check if message should be published
  publish  = teleinfo_message.overload;
  publish |= (teleinfo_message.received && (teleinfo_config.msg_policy == TELEINFO_POLICY_MESSAGE));
  publish |= (teleinfo_message.percent  && (teleinfo_config.msg_policy == TELEINFO_POLICY_PERCENT));
  if (publish)
  {
    // set publishing flags
    teleinfo_message.publish_meter = ((teleinfo_config.msg_type == TELEINFO_MSG_BOTH) || (teleinfo_config.msg_type == TELEINFO_MSG_METER));
    teleinfo_message.publish_tic   = ((teleinfo_config.msg_type == TELEINFO_MSG_BOTH) || (teleinfo_config.msg_type == TELEINFO_MSG_TIC));

    // reset message flags
    teleinfo_message.overload = false;
    teleinfo_message.received = false;
    teleinfo_message.percent  = false;
  }

  // if needed, publish meter or TIC JSON
  if (teleinfo_message.publish_meter) TeleinfoPublishJsonMeter ();
    else if (teleinfo_message.publish_tic) TeleinfoPublishJsonTic ();
}

// Calculate if some JSON should be published (called every second)
void TeleinfoEverySecond ()
{
  uint8_t phase;

  // do nothing during energy setup time
  if (TasmotaGlobal.uptime < ENERGY_WATCHDOG) return;

  // init daily counters
  if ((teleinfo_meter.conso_midnight_wh == LONG_LONG_MAX) && (teleinfo_meter.conso_total_wh != 0)) teleinfo_meter.conso_midnight_wh = teleinfo_meter.conso_total_wh;
  if ((teleinfo_meter.prod_midnight_wh  == LONG_LONG_MAX) && (teleinfo_meter.prod_total_wh  != 0)) teleinfo_meter.prod_midnight_wh  = teleinfo_meter.prod_total_wh;

  // update daily counters if day changes
  if (teleinfo_meter.day_of_month != RtcTime.day_of_month)
  {
    teleinfo_meter.day_of_month  = RtcTime.day_of_month;
    if (teleinfo_meter.conso_total_wh != 0) teleinfo_meter.conso_midnight_wh = teleinfo_meter.conso_total_wh;
    if (teleinfo_meter.prod_total_wh  != 0) teleinfo_meter.prod_midnight_wh  = teleinfo_meter.prod_total_wh;
  }

  // update today and total calculation
  for (phase = 0; phase < teleinfo_contract.phase; phase++) Energy->kWhtoday_delta[phase] += (int32_t)(Energy->active_power[phase] * 1000 / 36);
  EnergyUpdateToday ();

  // force meter grand total on phase 1 energy total
  Energy->total[0] = (float)teleinfo_meter.conso_total_wh / 1000;
  for (phase = 1; phase < teleinfo_contract.phase; phase++) Energy->total[phase] = 0;
  RtcSettings.energy_kWhtotal_ph[0] = teleinfo_meter.conso_total_wh / 100000;
  for (phase = 1; phase < teleinfo_contract.phase; phase++) RtcSettings.energy_kWhtotal_ph[phase] = 0;
}

// Generate JSON with TIC informations
void TeleinfoPublishJsonTic ()
{
  int index;

  // Start TIC section    {"Time":"xxxxxxxx","TIC":{ 
  Response_P (PSTR ("{\"%s\":\"%s\",\"%s\":{"), D_JSON_TIME, GetDateAndTime (DT_LOCAL).c_str (), TELEINFO_JSON_TIC);

  // loop thru TIC message lines
  for (index = 0; index < TELEINFO_LINE_QTY; index ++)
    if (teleinfo_message.line[index].checksum != 0)
    {
      // add current line
      if (index != 0) ResponseAppend_P (PSTR (","));
      ResponseAppend_P (PSTR ("\"%s\":\"%s\""), teleinfo_message.line[index].str_etiquette.c_str (), teleinfo_message.line[index].str_donnee.c_str ());
    }

  // end of TIC section, publish JSON and process rules
  ResponseAppend_P (PSTR ("}}"));
  MqttPublishTeleSensor ();

  // TIC has been published
  teleinfo_message.publish_tic = false;
}

// Generate JSON with Meter informations
void TeleinfoPublishJsonMeter ()
{
  uint8_t   phase, value;
  long      current, power_max, power_app, power_act;
  long long total;
  char      str_text[32];

  // Start tIME section    {"Time":"xxxxxxxx"
  Response_P (PSTR ("{\"%s\":\"%s\""), D_JSON_TIME, GetDateAndTime (DT_LOCAL).c_str ());

  // METER section    ,"METER":{
  ResponseAppend_P (PSTR (",\"%s\":{"), TELEINFO_JSON_METER);

  // METER basic data
  ResponseAppend_P (PSTR ("\"%s\":%u"),  TELEINFO_JSON_PHASE, teleinfo_contract.phase);
  ResponseAppend_P (PSTR (",\"%s\":%d"), TELEINFO_JSON_ISUB,  teleinfo_contract.isousc);
  ResponseAppend_P (PSTR (",\"%s\":%d"), TELEINFO_JSON_PSUB,  teleinfo_contract.ssousc);

  // METER adjusted maximum power
  power_max = teleinfo_contract.ssousc * (long)teleinfo_config.percent / 100;
  ResponseAppend_P (PSTR (",\"%s\":%d"), TELEINFO_JSON_PMAX, power_max);

  // loop to calculate apparent and active power
  current   = 0;
  power_app = 0;
  power_act = 0;
  for (phase = 0; phase < teleinfo_contract.phase; phase++)
  {
    // calculate parameters
    value      = phase + 1;
    current   += teleinfo_phase[phase].current;
    power_app += teleinfo_phase[phase].papp;
    power_act += teleinfo_phase[phase].pact;

    // voltage
    ResponseAppend_P (PSTR (",\"U%u\":%d"), value, teleinfo_phase[phase].voltage);

    // apparent and active power
    ResponseAppend_P (PSTR (",\"P%u\":%d,\"W%u\":%d"), value, teleinfo_phase[phase].papp, value, teleinfo_phase[phase].pact);

    // current
    ResponseAppend_P (PSTR (",\"I%u\":%d.%02d"), value, teleinfo_phase[phase].current / 100, teleinfo_phase[phase].current % 100);

    // cos phi
    ResponseAppend_P (PSTR (",\"C%u\":%d.%02d"), value, teleinfo_phase[phase].cosphi / 100, teleinfo_phase[phase].cosphi % 100);
  } 

  // Total Papp, Pact and Current
  ResponseAppend_P (PSTR (",\"P\":%d,\"W\":%d,\"I\":%d.%02d"), power_app, power_act, current / 100, current % 100);

  // current period
  GetTextIndexed (str_text, sizeof (str_text), teleinfo_contract.period_current, kTeleinfoPeriodName);
  if (strlen (str_text) > 0) ResponseAppend_P (PSTR (",\"Period\":\"%s\""), str_text);

  // instant production
  ResponseAppend_P (PSTR (",\"Prod\":%d"), teleinfo_meter.prod_papp);

  // end of METER section
  ResponseAppend_P (PSTR ("}}"));

  // publish JSON and process rules
  MqttPublishTeleSensor ();

  // Meter has been published
  teleinfo_message.publish_meter = false;
}

// Show JSON status (for MQTT)
void TeleinfoJSONTeleperiod ()
{
  // if teleinfo is not active, ignore
  if (teleinfo_meter.status_rx != TIC_SERIAL_ACTIVE) return;

  // if teleperiod timer not reached, ignore (to avoid rules update every 100ms)
  if (TasmotaGlobal.tele_period > 0) return;

  // check for JSON update according to update policy
  teleinfo_message.publish_meter = ((teleinfo_config.msg_type == TELEINFO_MSG_BOTH) || (teleinfo_config.msg_type == TELEINFO_MSG_METER));
  teleinfo_message.publish_tic   = ((teleinfo_config.msg_type == TELEINFO_MSG_BOTH) || (teleinfo_config.msg_type == TELEINFO_MSG_TIC));
}

/*********************************************\
 *                   Web
\*********************************************/

#ifdef USE_WEBSERVER

// Append Teleinfo state to main page
void TeleinfoWebSensor ()
{
  bool      contract = false;
  uint8_t   phase, period;
  long      value, red, green;
  long long total;
  char      str_text[32];
  char      str_color[16];

  // display according to serial receiver status
  switch (teleinfo_meter.status_rx)
  {
    case TIC_SERIAL_INIT:
      WSContentSend_P (PSTR ("{s}%s{m}Check configuration{e}"), D_TELEINFO);
      break;

    case TIC_SERIAL_FAILED:
      WSContentSend_P (PSTR ("{s}%s{m}Init failed{e}"), D_TELEINFO);
      break;

    case TIC_SERIAL_STOPPED:
      WSContentSend_P (PSTR ("{s}%s{m}Reception stopped{e}"), D_TELEINFO);
      break;

    case TIC_SERIAL_ACTIVE:
      // start
      WSContentSend_P (PSTR ("<div style='font-size:13px;text-align:center;padding:4px 6px;margin-bottom:5px;background:#333333;border-radius:12px;'>\n"));

      // title
      WSContentSend_P (PSTR ("<div style='display:flex;margin:2px 0px 6px 0px;padding:0px;font-size:16px;'>\n"));
      WSContentSend_P (PSTR ("<div style='width:30%%;padding:0px;text-align:left;font-weight:bold;'>Teleinfo</div>\n"));
      GetTextIndexed (str_text, sizeof (str_text), teleinfo_contract.mode, kTeleinfoModeIcon);
      WSContentSend_P (PSTR ("<div style='width:10%%;padding:0px;'>%s</div>\n"), str_text);
      if (teleinfo_contract.phase > 1) sprintf (str_text, "%u x ", teleinfo_contract.phase); else strcpy (str_text, ""); 
      WSContentSend_P (PSTR ("<div style='width:45%%;padding:1px 0px;text-align:right;font-weight:bold;'>%s%d</div>\n"), str_text, teleinfo_contract.ssousc / 1000);
      WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>kVA</div>\n"));
      WSContentSend_P (PSTR ("</div>\n"));

      // contract
      if (teleinfo_contract.period_qty <= TELEINFO_INDEX_MAX) 
        for (period = 0; period < teleinfo_contract.period_qty; period++)
          if (teleinfo_meter.conso_index_wh[period] > 0)
          {
            WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));

            // period name
            if (contract) strcpy (str_text, ""); else strcpy (str_text, "Période");
            WSContentSend_P (PSTR ("<div style='width:18%%;padding:1px 0px;text-align:left;'>%s</div>\n"), str_text);
            GetTextIndexed (str_text, sizeof (str_text), teleinfo_contract.period_first + period, kTeleinfoPeriodName);
            if (teleinfo_contract.period_current == teleinfo_contract.period_first + period) strcpy (str_color, "#09f"); else strcpy (str_color, "#444");
            WSContentSend_P (PSTR ("<div style='width:35%%;padding:1px 0px;background-color:%s;border-radius:6px;'>%s</div>\n"), str_color, str_text);

            // counter value
            lltoa (teleinfo_meter.conso_index_wh[period], str_text, 10);
            WSContentSend_P (PSTR ("<div style='width:32%%;padding:1px 0px;text-align:right;'>%s</div>\n"), str_text);
            WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>Wh</div>\n"));
            WSContentSend_P (PSTR ("</div>\n"));

            // update contract display
            contract = true;
          }

      // production total
      if (teleinfo_meter.prod_total_wh != 0)
      {
        WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));

        // period name
        if (contract) strcpy (str_text, ""); else strcpy (str_text, "Période");
        WSContentSend_P (PSTR ("<div style='width:18%%;padding:1px 0px;text-align:left;'>%s</div>\n"), str_text);
        if (teleinfo_meter.prod_papp == 0) strcpy (str_color, "#444"); else strcpy (str_color, "#080"); 
        WSContentSend_P (PSTR ("<div style='width:35%%;padding:1px 0px;background-color:%s;border-radius:6px;'>Production</div>\n"), str_color);

        // counter value
        lltoa (teleinfo_meter.prod_total_wh, str_text, 10);
        WSContentSend_P (PSTR ("<div style='width:32%%;padding:1px 0px;text-align:right;'>%s</div>\n"), str_text);
        WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>Wh</div>\n"));
        WSContentSend_P (PSTR ("</div>\n"));
      }

      // if meter is running consommation mode
      if (teleinfo_meter.conso_total_wh > 0)
      {
        // separator
        WSContentSend_P (PSTR ("<hr>\n"));

        // consommation
        WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));

        WSContentSend_P (PSTR ("<div style='width:40%%;padding:1px 0px;text-align:left;font-weight:bold;'>Consommation</div>\n"));
        value = 0;
        for (phase = 0; phase < teleinfo_contract.phase; phase++) value += teleinfo_phase[phase].pact;
        WSContentSend_P (PSTR ("<div style='width:45%%;padding:1px 0px;text-align:right;font-weight:bold;'>%d</div>\n"), value);
        WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>W</div>\n"));

        WSContentSend_P (PSTR ("</div>\n"));

        // percentage per phase
        if (teleinfo_contract.ssousc > 0)
          for (phase = 0; phase < teleinfo_contract.phase; phase++)
          {
            // calculate percentage
            value = 100 * teleinfo_phase[phase].papp / teleinfo_contract.ssousc;
            if (value > 100) value = 100;

            // calculate color
            if (value < 50) green = TELEINFO_RGB_GREEN_MAX; else green = TELEINFO_RGB_GREEN_MAX * 2 * (100 - value) / 100;
            if (value > 50) red = TELEINFO_RGB_RED_MAX; else red = TELEINFO_RGB_RED_MAX * 2 * value / 100;
            sprintf (str_text, "#%02x%02x00", red, green);

            // display bar graph percentage
            WSContentSend_P (PSTR ("<div style='display:flex;margin:2px 0px;padding:1px;height:16px;opacity:75%%;'>\n"));
            WSContentSend_P (PSTR ("<div style='width:18%%;padding:0px;text-align:left;'>Phase %u</div>\n"), phase + 1);
            WSContentSend_P (PSTR ("<div style='width:82%%;padding:0px;text-align:left;background-color:#252525;border-radius:6px;'><div style='width:%d%%;height:16px;padding:0px;text-align:center;font-size:12px;background-color:%s;border-radius:6px;'>%d%%</div></div>\n"), value, str_text, value);

            WSContentSend_P (PSTR ("</div>\n"));
          }
      }

      // if meter is running production mode
      if (teleinfo_meter.prod_total_wh > 0)
      {
        // separator
        WSContentSend_P (PSTR ("<hr>\n"));

        // production
        WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));
        WSContentSend_P (PSTR ("<div style='width:40%%;padding:1px 0px;text-align:left;font-weight:bold;'>Production</div>\n"));
        WSContentSend_P (PSTR ("<div style='width:45%%;padding:1px 0px;text-align:right;font-weight:bold;'>%d</div>\n"), teleinfo_meter.prod_papp);
        WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:15%%;padding:1px 0px;text-align:left;'>VA</div>\n"));
        WSContentSend_P (PSTR ("</div>\n"));

        // calculate and display production bar graph percentage
        if (teleinfo_contract.ssousc > 0)
        {            
          // calculate percentage
          value = 100 * teleinfo_meter.prod_papp / teleinfo_contract.ssousc;
          if (value > 100) value = 100;

          // calculate color
          if (value < 50) red = TELEINFO_RGB_GREEN_MAX; else red = TELEINFO_RGB_GREEN_MAX * 2 * (100 - value) / 100;
          if (value > 50) green = TELEINFO_RGB_RED_MAX; else green = TELEINFO_RGB_RED_MAX * 2 * value / 100;
          sprintf (str_text, "#%02x%02x00", red, green);

          WSContentSend_P (PSTR ("<div style='display:flex;margin:2px 0px;padding:1px;height:16px;opacity:75%%;'>\n"));
          WSContentSend_P (PSTR ("<div style='width:18%%;padding:0px;text-align:left;'>Actuel</div>\n"));
          WSContentSend_P (PSTR ("<div style='width:82%%;padding:0px;text-align:left;background-color:#252525;border-radius:6px;'><div style='width:%d%%;height:16px;padding:0px;text-align:center;font-size:12px;background-color:%s;border-radius:6px;'>%d%%</div></div>\n"), value, str_text, value);
          WSContentSend_P (PSTR ("</div>\n"));
        }

        // today's total consumption
        if (teleinfo_meter.prod_midnight_wh != LONG_LONG_MAX)
        {
          WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));
          WSContentSend_P (PSTR ("<div style='width:40%%;padding:1px 0px;text-align:left;'>Aujourd'hui</div>\n"));
          total = teleinfo_meter.prod_total_wh - teleinfo_meter.prod_midnight_wh;
          WSContentSend_P (PSTR ("<div style='width:45%%;padding:1px 0px;text-align:right;font-weight:bold;'>%d</div>\n"), (long)total);
          WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>Wh</div>\n"));
          WSContentSend_P (PSTR ("</div>\n"));
        }
      }

      // separator
      WSContentSend_P (PSTR ("<hr>\n"));

      // counters
      WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));
      WSContentSend_P (PSTR ("<div style='width:25%%;padding:1px 0px;text-align:left;'>Trames</div>\n"));
      WSContentSend_P (PSTR ("<div style='width:35%%;padding:1px 0px;'>%d</div>\n"), teleinfo_meter.nb_message);
      WSContentSend_P (PSTR ("<div style='width:5%%;padding:1px 0px;'>dont</div>\n"));
      WSContentSend_P (PSTR ("<div style='width:20%%;padding:1px 0px;text-align:right;'>%d</div>\n"), teleinfo_meter.nb_update);
      WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;'>cosφ</div>\n"));
      WSContentSend_P (PSTR ("</div>\n"));

      // if needed, display reset and errors
      if ((teleinfo_meter.nb_reset > 0) || (teleinfo_meter.nb_error > teleinfo_meter.nb_line / 100))
      {
        WSContentSend_P (PSTR ("<div style='display:flex;padding:2px 0px;'>\n"));

        // errors
        if (teleinfo_meter.nb_error == 0) strcpy (str_text, "white"); else strcpy (str_text, "red");
        WSContentSend_P (PSTR ("<div style='width:25%%;padding:1px 0px;color:%s;text-align:left;'>Erreurs</div>\n"), str_text);
        WSContentSend_P (PSTR ("<div style='width:35%%;padding:1px 5px;color:%s;'>%d</div>\n"), str_text, teleinfo_meter.nb_error);

        // reset
        if (teleinfo_meter.nb_reset == 0) strcpy (str_text, "white"); else strcpy (str_text, "orange");
        WSContentSend_P (PSTR ("<div style='width:5%%;padding:1px 0px;'>et</div>\n"));
        WSContentSend_P (PSTR ("<div style='width:20%%;padding:1px 0px;text-align:right;color:%s;'>%d</div>\n"), str_text, teleinfo_meter.nb_reset);
        WSContentSend_P (PSTR ("<div style='width:1%%;'></div><div style='width:14%%;padding:1px 0px;text-align:left;color:%s;'>reset</div>\n"), str_text);

        WSContentSend_P (PSTR ("</div>\n"));
      }

      // end
      WSContentSend_P (PSTR ("</div>\n"));
      break;
  }
}

#endif  // USE_WEBSERVER

/***************************************\
 *              Interface
\***************************************/

// Teleinfo energy driver
bool Xnrg15 (uint32_t function)
{
  bool result = false;

  // swtich according to context
  switch (function)
  {
    case FUNC_PRE_INIT:
      TeleinfoPreInit ();
      break;
    case FUNC_INIT:
      TeleinfoInit ();
      TeleinfoEnableSerial ();
      break;
    case FUNC_COMMAND:
      result = TeleinfoHandleCommand ();
      break;
    case FUNC_LOOP:
      TeleinfoReceiveData ();
      break;
    case FUNC_EVERY_250_MSECOND:
      TeleinfoEvery250ms ();
      break;
    case FUNC_ENERGY_EVERY_SECOND:
      TeleinfoEverySecond ();
      break;
    case FUNC_JSON_APPEND:
      TeleinfoJSONTeleperiod ();
      break;

#ifdef USE_WEBSERVER
     case FUNC_WEB_SENSOR:
      TeleinfoWebSensor ();
      break;
#endif  // USE_WEBSERVER
  }

  return result;
}

#endif      // USE_TELEINFO
#endif      // USE_ENERGY_SENSOR
