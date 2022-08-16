#!/bin/bash

echo "This script expects the PlatformIo install path to be passed as first argument (i.e. /home/bernhard/.platformio)"

read -p "New version number (i.e. '0.4.2'):" version_string
echo "using version " $version_string

mkdir brick32_release
cp docs/FLASH.MD ./brick32_release/FLASH.MD
cp $1/packages/framework-arduinoespressif32/tools/sdk/esp32/bin/bootloader_dio_40m.bin ./brick32_release/0x1000_bootloader_dio_40m.bin
cp .pio/build/sonoff_th_origin/partitions.bin ./brick32_release/0x8000_partitions.bin
cp $1/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin ./brick32_release/0xe000_boot_app0.bin
cp .pio/build/sonoff_th_origin/firmware.bin ./brick32_release/0x10000_firmware.bin
out_zipfile_name="brick32_release_v.${version_string}.zip"
zip -j -r ${out_zipfile_name} brick32_release
rm -rf ./brick32_release

echo "created ${out_zipfile_name}. Upload ZIP manually..."