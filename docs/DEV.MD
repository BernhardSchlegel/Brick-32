# Developers notebook

## Create a new release

Flashing must not only include the programm itself, but the bootloader, the partitions table, and the boot_app:

- `0x1000`: [.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin](/home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin).
- `0x8000`: `/.pio/build/az-delivery-devkit-v4/partitions.bin` in repo folder
- `0xe000`: [.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin](/home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin)
- `0x10000`: `firmware.bin` in repo folder

```
"/home/bernhard/.platformio/penv/bin/python" "/home/bernhard/.platformio/packages/tool-esptoolpy/esptool.py" --chip esp32 --port "/dev/ttyUSB0" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin 0x8000 /home/bernhard/Desktop/wd/BierBot-Brick-ESP32/.pio/build/az-delivery-devkit-v4/partitions.bin 0xe000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 .pio/build/az-delivery-devkit-v4/firmware.bin
```

## Upload with PlatformIO

Instead of collecting the four binaries by hand, PlatformIO resolves all tool
and framework paths itself and writes bootloader, partition table, boot app and
firmware in one go - with the same flash settings the Flash Download Tool uses
(DIO, 40 MHz, 4 MB, 460800 baud). `platformio.ini` contains no OS specific
paths, so this works identically on Windows, Linux and macOS.

Put the Sonoff into download mode first:

1. Unplug the USB cable of the USB-to-serial adapter.
1. Press and hold the button on the Sonoff.
1. Plug the USB cable back in while still holding the button.
1. Release the button.
1. Upload the matching environment:

```
pio run -e sonoff_th_origin -t upload
pio run -e sonoff_th_elite -t upload
```

The serial port is detected automatically. Only if several serial adapters are
connected, pass it explicitly (`pio device list` prints the available ports):

```
pio run -e sonoff_th_origin -t upload --upload-port COM5                    # Windows
pio run -e sonoff_th_origin -t upload --upload-port /dev/ttyUSB0            # Linux
pio run -e sonoff_th_origin -t upload --upload-port /dev/cu.usbserial-0001  # macOS
```

Afterwards `pio device monitor -e sonoff_th_origin` shows the serial log.

To make a new image:

1. Run "sonoff_th_origin" > "build"
1. Run "Advanced" > "Verbose Upload"
1. Look for the line `"/home/bernhard/.platformio/penv/bin/python" "/home/bernhard/.platformio/packages/tool-esptoolpy/esptool.py" --chip esp32 --port "/dev/ttyUSB0" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin 0x8000 /home/bernhard/Desktop/wd/BierBot-Brick-ESP32/.pio/build/sonoff_th_origin/partitions.bin 0xe000 /home/bernhard/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 .pio/build/sonoff_th_origin/firmware.bin` right before the upload starts.
1. Verify that the above adresses are still valid.
1. Run `create_release.sh` i.e. `./create_release.sh /home/bernhard/.platformio` (don't forget to `chmod +x create_release.sh` before first run)

## Use the .cmd file

On Windows

```
create_release.cmd "C:\Users\Bernhard\.platformio" "sonoff_th_origin"
create_release.cmd "C:\Users\Bernhard\.platformio" "sonoff_th_elite"

```

## FAQs

Q: COM port not recognized:
A: Install the CP2102 USB to UART Bridge Controller driver from [here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Q: Startup during debugging fails, message is `rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`?
A: Not enough power: The COM-Adapter is not able to provide it. As soon as you use WiFi you need to provide additional power, i.e. using AC.
