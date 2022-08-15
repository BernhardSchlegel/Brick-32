## Manual

### Getting started

Simply power up your BierBot Brick-32 and use any WiFi enabled device (Android, iPhone, Tablet, Mac, or PC) to scan for a Brick-32-Wifi (i.e. `Brick-32-0042123`) to configure your Brick.

### TODOs

- [ ] cleanup code

### Hardware

This software is tailor made for the Sonoff TH Origin (16A and 20A versions). Other, similar ESP32 powered Sonoff devices (POW Origin, POW Elite, etc.) will most likely work, too.

### Flashing the software

Hold the Button while powering up. Power up. Hit "Upload and monitor"

### Build guide

TODO

### Buttons and LEDs

The following Buttons are available

- Main-Button (top-left corner, blue LED)
  - Short Press / Click: Loop through array modes:
    1. Auto-Control by Software
    2. Force-OFF
    3. Force-ON
  - Double Press: bring up WiFi configuration portal
  - Long Press (more than 3s) factory reset.

The following LEDs are available to tell you which state your Brick-32 is in:

- Power-LED (red): On as soon the Brick is supplied with power and done with booting. This LED will confirm launching config portal with 3 Flashes and a Factory Reset with 10 flashes.
- WiFi-LED (blue): On if WiFi connection was established.
- Auto-Relay-LED (yellow):
   - Blinking: Relay is under automatic software control (if the LED is on the majority of the Blink, relay is ON. If the LED is off during marjority of the blink, relay is OFF).
   - Off: Relay is off due to force-override.
   - On: Relay is on due to force-override.

## Dev FAQs

### Flash

Flashing must not only include the programm itself, but the bootloader, the partitions table, and the boot_app:

- `0x1000`: [.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin](/home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin).
- `0x8000`: `/.pio/build/az-delivery-devkit-v4/partitions.bin` in repo folder
- `0xe000`: [.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin](/home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin)
- `0x10000`: `firmware.bin` in repo folder

```
"/home/bernhard/.platformio/penv/bin/python" "/home/bernhard/.platformio/packages/tool-esptoolpy/esptool.py" --chip esp32 --port "/dev/ttyUSB0" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin 0x8000 /home/bernhard/Desktop/wd/BierBot-Brick-ESP32/.pio/build/az-delivery-devkit-v4/partitions.bin 0xe000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 .pio/build/az-delivery-devkit-v4/firmware.bin
```

### General

Q: COM port not recognized:
A: Install the CP2102 USB to UART Bridge Controller driver from [here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Q: Startup during debugging fails, message is `rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`?
A: Not enough power: The COM-Adapter is not able to provide it. As soon as you use WiFi you need to provide additional power, i.e. using AC.

## Disclaimer

:warning: **DANGER OF ELECTROCUTION** :warning:

If your device connects to mains electricity (AC power) there is danger of electrocution if not installed properly. If you don't know how to install it, please call an electrician (***Beware:*** certain countries prohibit installation without a licensed electrician present). Remember: _**SAFETY FIRST**_. It is not worth the risk to yourself, your family and your home if you don't know exactly what you are doing. Never tinker or try to flash a device using the serial programming interface while it is connected to MAINS ELECTRICITY (AC power).

We don't take any responsibility nor liability for using this software nor for the installation or any tips, advice, videos, etc. given by any member of this site or any related site.

[disclaimer source](https://github.com/arendst/Tasmota/edit/development/README.md)
