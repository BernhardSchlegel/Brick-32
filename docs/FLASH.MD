# How to flash

If you read this, instructions from the YouTube video are still the easiest way to go.

## Flashing a self-built firmware with PlatformIO

If you build the firmware yourself, you do not need the Flash Download Tool at
all. PlatformIO writes the bootloader, the partition table, the boot app and the
firmware in one step, using the same flash settings (DIO, 40 MHz, 4 MB, 460800
baud). The configuration in `platformio.ini` is free of OS specific paths and
works the same on Windows, Linux and macOS.

Wire the USB-to-serial adapter to the serial header of the Sonoff (note that RX
and TX are crossed):

| Sonoff | USB-to-serial adapter |
| ------ | --------------------- |
| `GND`  | `GND`                 |
| `3V3`  | `3V3`                 |
| `TX0`  | `RXD`                 |
| `RX0`  | `TXD`                 |

Make sure the `3V3` dupont connector really sits tight on the header pin. A
loose contact still powers the chip well enough to connect, but the current
drawn while erasing and writing the flash makes the voltage drop, the ESP32
reboots and the upload aborts in the middle - see the troubleshooting section
below. If in doubt, pull the connector off, turn it by 90 degrees and push it
back on.

Then:

1. Unplug the USB cable of the USB-to-serial adapter.
1. Press and hold the button on the Sonoff.
1. Plug the USB cable back in while still holding the button.
1. Release the button - the Sonoff now waits for the download.
1. Run the upload for your device:

```
pio run -e sonoff_th_origin -t upload
pio run -e sonoff_th_elite -t upload
```

The serial port is detected automatically. If several serial adapters are
connected, list them with `pio device list` and pass the right one:

```
pio run -e sonoff_th_origin -t upload --upload-port COM5                    # Windows
pio run -e sonoff_th_origin -t upload --upload-port /dev/ttyUSB0            # Linux
pio run -e sonoff_th_origin -t upload --upload-port /dev/cu.usbserial-0001  # macOS
```

A successful run ends with `Hash of data verified.` and `Hard resetting via RTS
pin...`. Use `pio device monitor -e sonoff_th_origin` to watch the serial log.

## Troubleshooting

`Wrong boot mode detected (0x13)` or `No serial data received` while connecting:
the Sonoff is not in download mode - repeat the four steps above. Note that
opening the serial port resets the chip, so put it into download mode directly
before the upload and do not run a serial monitor in between.

`The chip stopped responding` or `Invalid head of packet (0x65)` in the middle
of writing the firmware: the ESP32 rebooted during the upload because its supply
voltage broke down. Check the `3V3` and `GND` connections first, they are the
usual cause. Lowering `upload_speed` to `115200` in `platformio.ini` does not
help against this - a bad contact stays a bad contact.
