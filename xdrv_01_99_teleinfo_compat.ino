/*
  Compatibility helpers used by Teleinfo 15.2beta11 and newer.

  The current upstream solar module calls these helpers, but they are not yet
  included in Nicolas Bernaerts' common sources. This file is ordered directly
  after xdrv_01_9_webserver.ino so it can reuse WebGetArg().
*/

#if defined(USE_TELEINFO) && defined(USE_TELEINFO_SOLAR)

bool MiscCommandUpdateFlag(const char* data, bool& target) {
  bool changed = false;
  if (XdrvMailbox.data_len > 0) {
    bool value = (XdrvMailbox.payload == 1);
    changed = (target != value);
    target = value;
  }
  return changed;
}

bool MiscCommandUpdateInteger(const char* data, int& target, long minimum, long maximum) {
  bool changed = false;
  if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.payload >= minimum) && (XdrvMailbox.payload <= maximum)) {
    int value = static_cast<int>(XdrvMailbox.payload);
    changed = (target != value);
    target = value;
  }
  return changed;
}

bool MiscCommandUpdateString(const char* data, char* target, size_t target_size) {
  bool changed = false;
  if (XdrvMailbox.data_len > 0) {
    const char* value = (0 == strcasecmp_P(data, PSTR("null"))) ? "" : data;
    changed = (0 != strcmp(target, value));
    strlcpy(target, value, target_size);
  }
  return changed;
}

bool MiscWebGetArgFlag(const char* name, bool& target) {
  char input[8];
  WebGetArg(name, input, sizeof(input));
  bool value = (input[0] != '\0');
  bool changed = (target != value);
  target = value;
  return changed;
}

bool MiscWebGetArgInteger(const char* name, int& target, long minimum, long maximum) {
  char input[24];
  WebGetArg(name, input, sizeof(input));
  if (input[0] == '\0') return false;

  char* end = nullptr;
  long parsed = strtol(input, &end, 10);
  if ((end == input) || (*end != '\0') || (parsed < minimum) || (parsed > maximum)) return false;

  int value = static_cast<int>(parsed);
  bool changed = (target != value);
  target = value;
  return changed;
}

bool MiscWebGetArgInteger(const char* name, long& target, long minimum, long maximum) {
  char input[24];
  WebGetArg(name, input, sizeof(input));
  if (input[0] == '\0') return false;

  char* end = nullptr;
  long parsed = strtol(input, &end, 10);
  if ((end == input) || (*end != '\0') || (parsed < minimum) || (parsed > maximum)) return false;

  bool changed = (target != parsed);
  target = parsed;
  return changed;
}

bool MiscWebGetArgString(const char* name, char* target, size_t target_size) {
  char input[256];
  WebGetArg(name, input, sizeof(input));
  bool changed = (0 != strcmp(target, input));
  strlcpy(target, input, target_size);
  return changed;
}

bool MiscWebGetArgFloat(const char* name, float& target, float minimum, float maximum) {
  char input[24];
  WebGetArg(name, input, sizeof(input));
  if (input[0] == '\0') return false;

  char* end = nullptr;
  float parsed = strtof(input, &end);
  if ((end == input) || (*end != '\0') || (parsed < minimum) || (parsed > maximum)) return false;

  bool changed = (target != parsed);
  target = parsed;
  return changed;
}

#endif  // USE_TELEINFO && USE_TELEINFO_SOLAR
