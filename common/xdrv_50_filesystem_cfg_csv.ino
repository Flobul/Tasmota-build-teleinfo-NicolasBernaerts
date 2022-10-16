/*
  xdrv_50_filesystem_cfg_csv.ino - Extensions for UFS driver
  This exension provides few functions to ease the use of :
   - .cfg configuration files
   - .csv database files

  Copyright (C) 2019-2021  Nicolas Bernaerts

  Version history :
    01/09/2021 - v1.0 - Creation
    29/09/2021 - v1.1 - Add .cfg files management
    15/10/2021 - v1.2 - Add reverse CSV line navigation
    01/04/2022 - v1.3 - Add software watchdog to avoid locked loop
    27/07/2022 - v1.4 - Use String to report strings
    31/08/2022 - v1.5 - Handle empty lines in CSV
    29/09/2022 - v1.6 - Rework of CSV files handling

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

#ifndef FIRMWARE_SAFEBOOT
#ifdef USE_UFILESYS

// partition constant
#define UFS_PARTITION_MIN_KB            25                  // minimum free space in USF partition (kb)

// configuration file constant
#define UFS_CFG_LINE_LENGTH             128                 // maximum size of a configuration line
#define UFS_CFG_VALUE_MAX               16                  // maximum size of a numerical value (int or float)

// CSV file constant
#define UFS_CSV_LINE_LENGTH             128                 // maximum line length in CSV files
#define UFS_CSV_COLUMN_MAX              32                  // maximum number of columns in CSV files

// action when reading a CSV line
enum UfsCsvAccessType { UFS_CSV_ACCESS_READ, UFS_CSV_ACCESS_WRITE, UFS_CSV_ACCESS_MAX };
enum UfsCsvLineAction { UFS_CSV_NONE, UFS_CSV_NEXT, UFS_CSV_PREVIOUS };

/*********************************************\
 *            Configuration files
\*********************************************/

// read key value in configuration file
String UfsCfgLoadKey (const char* pstr_filename, const char* pstr_key) 
{
  bool   finished = false;
  bool   found = false;
  int    length;
  char   str_line[UFS_CFG_LINE_LENGTH];
  char   *pstr_data;
  String str_value;
  File   file;

  // validate parameters
  if ((pstr_filename == nullptr) || (pstr_key == nullptr)) return str_value;
  
  // if file exists
  if (ffsp->exists (pstr_filename))
  {
    // open file in read only mode in littlefs filesystem
    file = ffsp->open (pstr_filename, "r");

    // loop to read lines
    do
    {
      length = file.readBytesUntil ('\n', str_line, sizeof (str_line));
      if (length > 0)
      {
        // split resulting string
        str_line[length] = 0;
        pstr_data = strchr (str_line, '=');
        if (pstr_data != nullptr)
        {
          // separate key and value
          *pstr_data = 0;
          pstr_data++;

          // check current key
          found = (strcmp (str_line, pstr_key) == 0);

          // if found, save value
          if (found) str_value = pstr_data;
        }
      }
      else finished = true;
    } while (!finished && !found);

    // close file
    file.close ();
  }

  return str_value;
}

// read integer key value in configuration file
int UfsCfgLoadKeyInt (const char* pstr_filename, const char* pstr_key, const int default_value) 
{
  int    result = default_value;
  String str_result;
  
  // open file in read only mode in littlefs filesystem
  str_result = UfsCfgLoadKey (pstr_filename, pstr_key);
  if (str_result.length () > 0) result = (int)str_result.toInt ();

  return result;
}

// read long key value in configuration file
long UfsCfgLoadKeyLong (const char* pstr_filename, const char* pstr_key, const long default_value) 
{
  long   result = default_value;
  String str_result;
  
  // open file in read only mode in littlefs filesystem
  str_result = UfsCfgLoadKey (pstr_filename, pstr_key);
  if (str_result.length () > 0) result = str_result.toInt ();

  return result;
}

// read float key value in configuration file
float UfsCfgLoadKeyFloat (const char* pstr_filename, const char* pstr_key, const float default_value) 
{
  float  result = default_value;
  String str_result;

  // open file in read only mode in littlefs filesystem
  str_result = UfsCfgLoadKey (pstr_filename, pstr_key);
  if (str_result.length () > 0) result = str_result.toFloat ();

  return result;
}

// save key value in configuration file
void UfsCfgSaveKey (const char* pstr_filename, const char* pstr_key, const char* pstr_value, bool create) 
{
  char str_line[UFS_CFG_LINE_LENGTH];
  File file;
  
  // validate parameters
  if ((pstr_filename == nullptr) || (pstr_key == nullptr) || (pstr_value == nullptr)) return;
  
  // open file in creation or append mode
  if (create) file = ffsp->open (pstr_filename, "w");
    else file = ffsp->open (pstr_filename, "a");

  // write key=value
  sprintf (str_line, "%s=%s\n", pstr_key, pstr_value);
  file.print (str_line);

  // close file
  file.close ();
}

// save key value in configuration file
void UfsCfgSaveKeyInt (const char* pstr_filename, const char* pstr_key, const int value, bool create) 
{
  char str_value[UFS_CFG_VALUE_MAX];
  
  // convert value to string and save key = value
  itoa (value, str_value, 10);
  UfsCfgSaveKey (pstr_filename, pstr_key, str_value, create);
}

// save key value in configuration file
void UfsCfgSaveKeyLong (const char* pstr_filename, const char* pstr_key, const long value, bool create) 
{
  char str_value[UFS_CFG_VALUE_MAX];
  
  // convert value to string and save key = value
  ltoa (value, str_value, 10);
  UfsCfgSaveKey (pstr_filename, pstr_key, str_value, create);
}

// save key value in configuration file
void UfsCfgSaveKeyFloat (const char* pstr_filename, const char* pstr_key, const float value, bool create) 
{
  char str_value[UFS_CFG_VALUE_MAX];
  
  // convert value to string and save key = value
  ext_snprintf_P (str_value, sizeof (str_value), PSTR ("%03_f"), &value);
  UfsCfgSaveKey (pstr_filename, pstr_key, str_value, create);
}

/*********************************************\
 *                 CSV files
\*********************************************/

bool UfsSeekToStart (File &file)
{
  bool done;

  // set to start of file
  done = file.seek (0);

  return done;
}

bool UfsSeekToEnd (File &file)
{
  bool done;

  // get file size and seek to last byte
  done = file.seek (file.size () - 1);

  return done;
}

// extract specific column from given line (first column is 1)
// return size of column string
size_t UfsExtractCsvColumn (char* pstr_line, const char separator, int column, char* pstr_value, const size_t size_value, const bool till_end_of_line)
{
  bool  search = true;
  int   index  = 0;
  char *pstr_start = pstr_line;
  char *pstr_stop  = nullptr;

  // check parameter
  if ((pstr_line  == nullptr) || (pstr_value == nullptr)) return 0;

  // loop to find column
  strcpy (pstr_value, "");
  while (search)
  {
    // if seperator found
    if (*pstr_line == separator)
    {
      // increase to next column and check if target column is reached
      index ++;
      if (index == column) 
      {
        // if value should be all next column
        if (till_end_of_line) strcpy (pstr_value, pstr_start);

        // else value is column content
        else
        {
          strncpy (pstr_value, pstr_start, pstr_line - pstr_start);
          pstr_value[pstr_line - pstr_start] = 0;
        }
      }

      // update start caracter to next column
      else pstr_start = pstr_line + 1;
    }

    // else if end of line reached, fill content is target column was reached
    else if ((*pstr_line == 0) && (index == column - 1)) strcpy (pstr_value, pstr_start);

    // check if loop should be stopped
    if ((index == column) || (*pstr_line == 0)) search = false;

    pstr_line++;
  }

  return strlen (pstr_value);
}

// read next line, skipping empty lines, and return number of caracters in the line
size_t UfsReadNextLine (File &file, char* pstr_line, const size_t size_line)
{
  size_t   index;
  size_t   length = 0;
  uint32_t pos_start;
  char    *pstr_start;
  char    *pstr_token;

  // check parameter
  if (pstr_line == nullptr) return 0;

  // init
  strcpy (pstr_line, "");

  // read next line
  pos_start = file.position ();
  length = file.readBytes (pstr_line, size_line - 1);
  pstr_line[length] = 0;

  // loop to skip empty lines
  pstr_start = pstr_line;
  while ((*pstr_start == '\n') && (*pstr_start != 0)) pstr_start++;

  // if next end of line found
  pstr_token = strchr (pstr_start, '\n');
  if (pstr_token != nullptr)
  {
    // set end of string
    *pstr_token = 0;

    // seek file to beginning of next line
    file.seek (pstr_token - pstr_line + pos_start + 1);
  } 

  // if needed, remove \n from empty lines
  length = strlen (pstr_start);
  if (pstr_start > pstr_line) for (index = 0; index <= length; index ++) pstr_line[index] = pstr_start[index];

  return length;
}

// read previous line and return number of caracters in the line
size_t UfsReadPreviousLine (File &file, char* pstr_line, const size_t size_line)
{
  uint8_t  current_car;
  uint32_t pos_search;
  uint32_t pos_start = 0;
  uint32_t pos_stop  = 0;
  size_t   length = 0;

  // check parameter
  if (pstr_line == nullptr) return 0;

  // init
  pstr_line[0] = 0;

  // if file exists
  if (file.available ())
  {
    // loop to avoid empty lines
    pos_search = file.position ();
    if (pos_search > 0) do
    {
      pos_search--;
      file.seek (pos_search);
      current_car = (uint8_t)file.read ();
      if (current_car != '\n') pos_stop = pos_search + 1;
    }
    while ((current_car == '\n') && (pos_search > 0));

    // loop to read previous line
    pos_search++;
    if (pos_search > 0) do
    {
      pos_search--;
      file.seek (pos_search);
      current_car = (uint8_t)file.read ();
      if (current_car != '\n') pos_start = pos_search;
    }
    while ((current_car != '\n') && (pos_search > 0));

    // get complete line
    if (pos_stop > pos_start)
    {
      if (current_car == '\n') pos_search++;
      file.seek (pos_search);
      length = file.readBytes (pstr_line, pos_stop - pos_start);
      pstr_line[length] = 0;
      file.seek (pos_search);
    }
  }

  return length;
}

// rotate files according to file naming convention
//   file-2.csv -> file-3.csv
//   file-1.csv -> file-2.csv
void UfsFileRotate (const char* pstr_filename, const int index_min, const int index_max) 
{
  int  index;
  char str_original[UFS_FILENAME_SIZE];
  char str_target[UFS_FILENAME_SIZE];

  // check parameter
  if (pstr_filename == nullptr) return;

  // rotate previous daily files
  for (index = index_max; index > index_min; index--)
  {
    // generate file names
    sprintf (str_original, pstr_filename, index - 1);
    sprintf (str_target, pstr_filename, index);

    // if target exists, remove it
    if (ffsp->exists (str_target))
    {
      ffsp->remove (str_target);
      AddLog (LOG_LEVEL_INFO, PSTR ("UFS: deleted %s"), str_target);
    }

    // if file exists, rotate it
    if (ffsp->exists (str_original))
    {
      ffsp->rename (str_original, str_target);
      AddLog (LOG_LEVEL_INFO, PSTR ("UFS: renamed %s to %s"), str_original, str_target);
    }

    // give control back to system to avoid watchdog
    yield ();
  }
}

uint32_t UfsGetFileSizeKb (const char* pstr_filename)
{
  uint32_t file_size = 0;
  File     file;

  // check parameter
  if (pstr_filename == nullptr) return file_size;

  // if file exists
  if (ffsp->exists (pstr_filename))
  {
    // open file in read only and get its size
    file = ffsp->open (pstr_filename, "r");
    file_size = (uint32_t)file.size () / 1024;
    file.close ();
  }

  return file_size;
}

// cleanup filesystem from oldest CSV files according to free size left (in Kb)
void UfsCleanupFileSystem (uint32_t size_minimum, const char* pstr_extension) 
{
  bool     removal_done = true;
  uint32_t size_available;
  time_t   file_time;
  char     str_filename[UFS_FILENAME_SIZE];
  File     root_dir; 

  // open root directory
  root_dir = dfsp->open ("/", UFS_FILE_READ);

  // loop till minimum space is available
  size_available = UfsInfo (1, 0);
  while (removal_done && (size_available < size_minimum))
  {
    // init
    removal_done = false;
    strcpy (str_filename, "");
    file_time = time (NULL);

    // loop thru filesystem to get oldest CSV file
    while (true) 
    {
      // read next file
      File current_file = root_dir.openNextFile();
      if (!current_file) break;

      // check if file is candidate for removal
      if ((strstr (current_file.name (), pstr_extension) != nullptr) && (current_file.getLastWrite () < file_time))
      {
        strcpy (str_filename, "/");
        strlcat (str_filename, current_file.name (), sizeof (str_filename));
        file_time = current_file.getLastWrite ();
      }

      // close file
      current_file.close ();
    }

    // remove oldest CSV file
    if (strlen (str_filename) > 0) removal_done = ffsp->remove (str_filename);
    if (removal_done) AddLog (LOG_LEVEL_INFO, PSTR ("UFS: Purged %s"), str_filename);

    // read new space left
    size_available = UfsInfo (1, 0);

    // give control back to system to avoid watchdog
    yield ();
  }

  // close directory
  root_dir.close ();
}

#endif    // USE_UFILESYS
#endif    // FIRMWARE_SAFEBOOT