@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: %~nx0 [PlatformIO Path] [Configuration Name]
    echo Example: %~nx0 "C:\Users\Bernhard\.platformio" "sonoff_th_origin"
    exit /b 1
)

if "%~2"=="" (
    echo Missing configuration name. Please provide "sonoff_th_origin" or "sonoff_th_elite".
    exit /b 1
)

set platformio_path=%~1
set config_name=%~2

echo This script expects the PlatformIo install path as the first argument.
echo Configuration name: %config_name%

set /p version_string=New version number (i.e. '0.4.2'): 
echo Using version %version_string%

set out_zipfile_name=brick32_release_v.%version_string%.zip

:: Create release directory
mkdir brick32_release

:: Copy necessary files
copy docs\FLASH.MD brick32_release\FLASH.MD
copy "%platformio_path%\packages\framework-arduinoespressif32\tools\sdk\esp32\bin\bootloader_dio_40m.bin" brick32_release\0x1000_%config_name%_bootloader_dio_40m.bin
copy .pio\build\%config_name%\partitions.bin brick32_release\0x8000_%config_name%_partitions.bin
copy "%platformio_path%\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin" brick32_release\0xe000_%config_name%_boot_app0.bin
copy .pio\build\%config_name%\firmware.bin brick32_release\0x10000_%config_name%_firmware.bin

:: Zip the folder (requires PowerShell since CMD lacks built-in zip support)
powershell -command "Compress-Archive -Path brick32_release\* -DestinationPath %out_zipfile_name%"

:: Remove the release directory
:: rmdir /s /q brick32_release

echo Created %out_zipfile_name%. Upload ZIP manually...
