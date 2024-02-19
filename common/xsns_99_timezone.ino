/*
  xsns_99_timezone.ino - Timezone management (~3.2 kb)
  
  Copyright (C) 2020  Nicolas Bernaerts
    04/04/2020 - v1.0 - Creation 
    19/05/2020 - v1.1 - Add configuration for first NTP server 
    22/07/2020 - v1.2 - Memory optimisation 
    10/04/2021 - v1.3 - Remove use of String to avoid heap fragmentation 
    22/04/2021 - v1.4 - Switch to a full Drv (without Sns) 
    11/07/2021 - v1.5 - Tasmota 9.5 compatibility 
    07/05/2022 - v1.6 - Add command to enable JSON publishing 

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

/*************************************************\
 *                Timezone
\*************************************************/

#ifdef USE_TIMEZONE

#define XSNS_99                   99

// commands
#define D_CMND_TIMEZONE_NTP       "ntp"
#define D_CMND_TIMEZONE_STDO      "stdo"
#define D_CMND_TIMEZONE_STDM      "stdm"
#define D_CMND_TIMEZONE_STDW      "stdw"
#define D_CMND_TIMEZONE_STDD      "stdd"
#define D_CMND_TIMEZONE_DSTO      "dsto"
#define D_CMND_TIMEZONE_DSTM      "dstm"
#define D_CMND_TIMEZONE_DSTW      "dstw"
#define D_CMND_TIMEZONE_DSTD      "dstd"

// web URL
const char D_TIMEZONE_PAGE_CONFIG[] PROGMEM = "/tz";

// dialog strings
const char D_TIMEZONE[]        PROGMEM = "Timezone";
const char D_TIMEZONE_CONFIG[] PROGMEM = "Configure";
const char D_TIMEZONE_NTP[]    PROGMEM = "First time server";
const char D_TIMEZONE_TIME[]   PROGMEM = "Time";
const char D_TIMEZONE_STD[]    PROGMEM = "Standard Time";
const char D_TIMEZONE_DST[]    PROGMEM = "Daylight Saving Time";
const char D_TIMEZONE_OFFSET[] PROGMEM = "Offset to GMT (mn)";
const char D_TIMEZONE_MONTH[]  PROGMEM = "Month (1:jan ... 12:dec)";
const char D_TIMEZONE_WEEK[]   PROGMEM = "Week (0:last ... 4:fourth)";
const char D_TIMEZONE_DAY[]    PROGMEM = "Day of week (1:sun ... 7:sat)";

// time icons
const char kTimezoneIcons[] PROGMEM = "🕛|🕧|🕐|🕜|🕑|🕝|🕒|🕞|🕓|🕟|🕔|🕠|🕕|🕡|🕖|🕢|🕗|🕣|🕘|🕤|🕙|🕥|🕚|🕦";

// timezone setiing commands
const char kTimezoneCommands[] PROGMEM = "tz_|help|pub|ntp|stdo|stdm|stdw|stdd|dsto|dstm|dstw|dstd";
void (* const TimezoneCommand[])(void) PROGMEM = { &CmndTimezoneHelp, &CmndTimezonePublish, &CmndTimezoneNtp, &CmndTimezoneStdO, &CmndTimezoneStdM, &CmndTimezoneStdW, &CmndTimezoneStdD, &CmndTimezoneDstO, &CmndTimezoneDstM, &CmndTimezoneDstW, &CmndTimezoneDstD };

// constant strings
const char TZ_FIELDSET_START[] PROGMEM = "<p><fieldset><legend><b>&nbsp;%s %s&nbsp;</b></legend>\n";
const char TZ_FIELDSET_STOP[]  PROGMEM = "</fieldset></p>\n";
const char TZ_FIELD_INPUT[]    PROGMEM = "<p>%s<span style='float:right;font-size:0.7rem;'>%s</span><br><input type='number' name='%s' min='%d' max='%d' step='%d' value='%d'></p>\n";


/**************************************************\
 *                  Variables
\**************************************************/

// variables
static struct {
  bool publish_json = false;
} timezone;

/**************************************************\
 *                  Functions
\**************************************************/

// Show JSON status (for MQTT)
void TimezoneShowJSON (bool append)
{
  // add , in append mode or { in direct publish mode
  if (append) ResponseAppend_P (PSTR (",")); else Response_P (PSTR ("{"));

  // generate string
  ResponseAppend_P (PSTR ("\"Timezone\":{"));
  ResponseAppend_P (PSTR ("\"%s\":{\"Offset\":%d,\"Month\":%d,\"Week\":%d,\"Day\":%d}"), "STD", Settings->toffset[0], Settings->tflag[0].month, Settings->tflag[0].week, Settings->tflag[0].dow);
  ResponseAppend_P (PSTR (","));
  ResponseAppend_P (PSTR ("\"%s\":{\"Offset\":%d,\"Month\":%d,\"Week\":%d,\"Day\":%d}"), "DST", Settings->toffset[1], Settings->tflag[1].month, Settings->tflag[1].week, Settings->tflag[1].dow);
  ResponseAppend_P (PSTR ("}"));

  // publish it if not in append mode
  if (!append)
  {
    ResponseAppend_P (PSTR ("}"));
    MqttPublishTeleSensor ();
  } 
}

// init main status
void TimezoneInit ()
{
  // set switch mode
  Settings->timezone = 99;

  // log help command
  AddLog (LOG_LEVEL_INFO, PSTR ("HLP: tz_help to get help on Timezone commands"));
}

/***********************************************\
 *                  Commands
\***********************************************/

// timezone help
void CmndTimezoneHelp ()
{
  AddLog (LOG_LEVEL_INFO, PSTR ("HLP: Timezone commands :"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_pub  = Add timezone data in telemetry JSON (ON/OFF)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_ntp  = Set NTP server"));

  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_stdo = Standard time offset to GMT (mn)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_stdm = Standard time month (1:jan ... 12:dec)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_stdw = Standard time week (0:last ... 4:fourth)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_stdd = Standard time day of week (1:sun ... 7:sat)"));

  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_dsto = Daylight savings time offset to GMT (mn)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_dstm = Daylight savings time month (1:jan ... 12:dec)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_dstw = Daylight savings time week (0:last ... 4:fourth)"));
  AddLog (LOG_LEVEL_INFO, PSTR (" - tz_dstd = Daylight savings time day of week (1:sun ... 7:sat)"));
  
  ResponseCmndDone();
}

void CmndTimezonePublish ()
{
  if (XdrvMailbox.data_len > 0)
  {
    // update flag
    timezone.publish_json = ((XdrvMailbox.payload != 0) || (strcasecmp (XdrvMailbox.data, "ON") == 0));
  }
  ResponseCmndNumber (timezone.publish_json);
}

void CmndTimezoneNtp ()
{
  if (XdrvMailbox.data_len > 0) SettingsUpdateText (SET_NTPSERVER1, XdrvMailbox.data);
  ResponseCmndChar (SettingsText(SET_NTPSERVER1));
}

void CmndTimezoneStdO ()
{
  if (XdrvMailbox.data_len > 0) Settings->toffset[0] = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->toffset[0]);
}

void CmndTimezoneStdM ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 13)) Settings->tflag[0].month = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[0].month);
}

void CmndTimezoneStdW ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 5)) Settings->tflag[0].week = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[0].week);
}

void CmndTimezoneStdD ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 8)) Settings->tflag[0].dow = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[0].dow);
}

void CmndTimezoneDstO ()
{
  if (XdrvMailbox.data_len > 0) Settings->toffset[1] = XdrvMailbox.payload;
  ResponseCmndNumber (XdrvMailbox.payload);
}

void CmndTimezoneDstM ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 13)) Settings->tflag[1].month = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[1].month);
}

void CmndTimezoneDstW ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 5)) Settings->tflag[1].week = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[1].week);
}

void CmndTimezoneDstD ()
{
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 8)) Settings->tflag[1].dow = XdrvMailbox.payload;
  ResponseCmndNumber (Settings->tflag[1].dow);
}

/***********************************************\
 *                    Web
\***********************************************/

#ifdef USE_WEBSERVER

// append local time to main page
void TimezoneWebSensor ()
{
  uint8_t index;
  TIME_T  current_dst;
  char    str_text[8];

  // get local time
  BreakTime (LocalTime (), current_dst);

  // get time icon
  index = 2 * (current_dst.hour % 12);
  if (current_dst.minute >= 45) index = index + 2;
    else if (current_dst.minute >= 15) index = index + 1;
  index = index % 24;
  GetTextIndexed (str_text, sizeof (str_text), index, kTimezoneIcons);

  // time
  WSContentSend_P (PSTR ("<div style='text-align:center;padding:0px;margin:5px 0px;'>\n"));
  WSContentSend_P (PSTR ("<div style='display:flex;margin:2px 0px 6px 0px;padding:0px;'>\n"));
  WSContentSend_P (PSTR ("<div style='width:15%%;padding:0px;font-size:20px;'>%s</div>\n"), str_text);
  WSContentSend_P (PSTR ("<div style='width:70%%;padding:1px 0px;font-size:16px;font-weight:bold;'>%02d:%02d:%02d</div>\n"), current_dst.hour, current_dst.minute, current_dst.second);
  WSContentSend_P (PSTR ("<div style='width:15%%;padding:0px;'></div>\n"));
  WSContentSend_P (PSTR ("</div></div>\n"));
}

#ifdef USE_TIMEZONE_WEB_CONFIG

// timezone configuration page button
void TimezoneWebConfigButton ()
{
  WSContentSend_P (PSTR ("<p><form action='%s' method='get'><button>%s %s</button></form></p>"), D_TIMEZONE_PAGE_CONFIG, D_TIMEZONE_CONFIG, D_TIMEZONE);
}

// Timezone configuration web page
void TimezoneWebPageConfigure ()
{
  char argument[32];

  // if access not allowed, close
  if (!HttpCheckPriviledgedAccess()) return;

  // page comes from save button on configuration page
  if (Webserver->hasArg("save"))
  {
    // set first time server
    WebGetArg (D_CMND_TIMEZONE_NTP, argument, sizeof (argument));
    if (strlen (argument) > 0) SettingsUpdateText (SET_NTPSERVER1, argument);

    // set timezone STD offset according to 'stdo' parameter
    WebGetArg (D_CMND_TIMEZONE_STDO, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->toffset[0] = atoi (argument);
    
    // set timezone STD month switch according to 'stdm' parameter
    WebGetArg (D_CMND_TIMEZONE_STDM, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[0].month = atoi (argument);

    // set timezone STD week of month switch according to 'stdw' parameter
    WebGetArg (D_CMND_TIMEZONE_STDW, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[0].week = atoi (argument);

    // set timezone STD day of week switch according to 'stdd' parameter
    WebGetArg (D_CMND_TIMEZONE_STDD, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[0].dow = atoi (argument);

    // set timezone DST offset according to 'dsto' parameter
    WebGetArg (D_CMND_TIMEZONE_DSTO, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->toffset[1] = atoi (argument);
    
    // set timezone DST month switch according to 'dstm' parameter
    WebGetArg (D_CMND_TIMEZONE_DSTM, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[1].month = atoi (argument);

    // set timezone DST week of month switch according to 'dstw' parameter
    WebGetArg (D_CMND_TIMEZONE_DSTW, argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[1].week = atoi (argument);

    // set timezone DST day of week switch according to 'dstd' parameter
    WebGetArg (D_CMND_TIMEZONE_DSTD,argument, sizeof (argument));
    if (strlen (argument) > 0) Settings->tflag[1].dow = atoi (argument);
  }

  // beginning of form
  WSContentStart_P (D_TIMEZONE_CONFIG);
  WSContentSendStyle ();
  WSContentSend_P (PSTR ("<form method='get' action='%s'>\n"), D_TIMEZONE_PAGE_CONFIG);

  // NTP server section  
  // ---------------------
  WSContentSend_P (TZ_FIELDSET_START, "🖥️", D_TIMEZONE_NTP);
  WSContentSend_P (PSTR ("<p>%s<span style='float:right;font-size:0.7rem;'>%s</span><br><input type='text' name='%s' value='%s'></p>\n"), D_TIMEZONE_NTP, PSTR (D_CMND_TIMEZONE_NTP), PSTR (D_CMND_TIMEZONE_NTP), SettingsText(SET_NTPSERVER1));
  WSContentSend_P (TZ_FIELDSET_STOP);

  // Standard Time section  
  // ---------------------
  WSContentSend_P (TZ_FIELDSET_START, "🕞", D_TIMEZONE_STD);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_OFFSET, PSTR (D_CMND_TIMEZONE_STDO), PSTR (D_CMND_TIMEZONE_STDO), -720, 720, 1, Settings->toffset[0]);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_MONTH,  PSTR (D_CMND_TIMEZONE_STDM), PSTR (D_CMND_TIMEZONE_STDM), 1,    12,  1, Settings->tflag[0].month);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_WEEK,   PSTR (D_CMND_TIMEZONE_STDW), PSTR (D_CMND_TIMEZONE_STDW), 0,    4,   1, Settings->tflag[0].week);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_DAY,    PSTR (D_CMND_TIMEZONE_STDD), PSTR (D_CMND_TIMEZONE_STDD), 1,    7,   1, Settings->tflag[0].dow);
  WSContentSend_P (TZ_FIELDSET_STOP);

  // Daylight Saving Time section  
  // ----------------------------
  WSContentSend_P (TZ_FIELDSET_START, "🕤", D_TIMEZONE_DST);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_OFFSET, PSTR (D_CMND_TIMEZONE_DSTO), PSTR (D_CMND_TIMEZONE_DSTO), -720, 720, 1, Settings->toffset[1]);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_MONTH,  PSTR (D_CMND_TIMEZONE_DSTM), PSTR (D_CMND_TIMEZONE_DSTM), 1,    12,  1, Settings->tflag[1].month);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_WEEK,   PSTR (D_CMND_TIMEZONE_DSTW), PSTR (D_CMND_TIMEZONE_DSTW), 0,    4,   1, Settings->tflag[1].week);
  WSContentSend_P (TZ_FIELD_INPUT, D_TIMEZONE_DAY,    PSTR (D_CMND_TIMEZONE_DSTD), PSTR (D_CMND_TIMEZONE_DSTD), 1,    7,   1, Settings->tflag[1].dow);
  WSContentSend_P (TZ_FIELDSET_STOP);

  // save button  
  // --------------
  WSContentSend_P (PSTR ("<br><p><button name='save' type='submit' class='button bgrn'>%s</button></p>\n"), D_SAVE);
  WSContentSend_P (PSTR ("</form>\n"));

  // configuration button
  WSContentSpaceButton (BUTTON_CONFIGURATION);

  // end of page
  WSContentStop();
}

#endif      // USE_TIMEZONE_WEB_CONFIG

#endif  // USE_WEBSERVER

/***********************************************************\
 *                      Interface
\***********************************************************/

bool Xsns99 (uint32_t function)
{
  bool result = false;

  // main callback switch
  switch (function)
  { 
    case FUNC_INIT:
      TimezoneInit ();
      break;
    case FUNC_COMMAND:
      result = DecodeCommand (kTimezoneCommands, TimezoneCommand);
      break;
    case FUNC_JSON_APPEND:
      if (timezone.publish_json) TimezoneShowJSON (true);
      break;

#ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
      TimezoneWebSensor ();
      break;

#ifdef USE_TIMEZONE_WEB_CONFIG
    case FUNC_WEB_ADD_BUTTON:
      TimezoneWebConfigButton ();
      break;
    case FUNC_WEB_ADD_HANDLER:
      Webserver->on (FPSTR (D_TIMEZONE_PAGE_CONFIG), TimezoneWebPageConfigure);
      break;
#endif        // USE_TIMEZONE_WEB_CONFIG

#endif  // USE_WEBSERVER

  }
  
  return result;
}

#endif      // USE_TIMEZONE

