# Tasmota Teleinfo build with BLE

This repository builds the latest sources from
[`NicolasBernaerts/tasmota`](https://github.com/NicolasBernaerts/tasmota/tree/master/teleinfo)
for an ESP32-S3 with 16 MB flash, with BLE support enabled.

The GitHub Actions workflow targets Tasmota 15.2.0, the core version currently
used by Nicolas Bernaerts' published sources and binaries. The Teleinfo
extension has its own version number (currently 15.5 beta). Its `master` branch
is cloned for every build, so the Teleinfo sources and reported extension
version are not duplicated or pinned in this repository.

The 16 MB layout reserves 3 MB for the Teleinfo + BLE application and 12 MB
for LittleFS. A serial factory flash is required when switching from a device
using a different partition layout.

Run **Teleinfo Extension Compilation** from the Actions tab to compile and
publish the firmware. A different compatible Tasmota version can be selected
for manual builds.
