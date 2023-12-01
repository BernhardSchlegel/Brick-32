#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <WiFiManager.h>
#include "Button2.h"
#include "helper.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <jled.h>
#include "AcOut.h"
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define GPIO_OUT_DRSW_ONOFF 4 // Sonoff TH Elite Dry Contact
#define GPIO_OUT_RELAY_BISTABLE_ON 22 // Sonoff TH Origin 20A
#define GPIO_OUT_RELAY_BISTABLE_OFF 19 // Sonoff TH Origin 20A
#define GPIO_OUT_RELAY_ON 21  // Sonoff TH Origin 16A
#define GPIO_OUT_LED_GREEN_AUTO 13
#define GPIO_OUT_LED_BLUE_WIFI 15
#define GPIO_OUT_LED_RED_ONOFF 16
#define GPIO_IN_BUTTON 0
#define GPIO_ONE_WIRE_BUS 25
#define GPIO_OUT_POWER_RJ11 27

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(GPIO_ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

TaskHandle_t Core0TaskHnd;

enum AC_STATE
{
  AC_STATE_AUT = 0,
  AC_STATE_ON = 1,
  AC_STATE_OFF = 2
};

enum MAIN_STATE
{
  MAIN_STATE_CONTROL,
  MAIN_STATE_WIFI_SETUP
};


#define EEPROM_ADDRESS_APIKEY 0
#define HTTP_REQUEST_RESPONSE_BUF_LEN 255

Button2 btn_main;

AC_STATE currentStateAc1 = AC_STATE_AUT;
MAIN_STATE currentStateMain = MAIN_STATE_CONTROL;

// interfaces
auto led_onoff = JLed(GPIO_OUT_LED_RED_ONOFF).LowActive().Off();
auto led_wifi = JLed(GPIO_OUT_LED_BLUE_WIFI).LowActive().Off();
auto led_relay = JLed(GPIO_OUT_LED_GREEN_AUTO).LowActive().Off();
auto ac1 = AcOut(GPIO_OUT_RELAY_BISTABLE_ON, GPIO_OUT_RELAY_ON, GPIO_OUT_RELAY_BISTABLE_OFF, false);


// wifi and settings
String apikey;
WiFiManager wm;                           // global wm instance
WiFiManagerParameter custom_field_apikey; // global param ( for non blocking w params )
HTTPClient http;                          // Declare an object of class HTTPClient
WiFiClientSecure https;

// global vars
uint8_t global_error = 0;
uint8_t global_warning = 0;
String global_error_text = "";
String global_warning_text = "";
String chipid = "";
String ssid = "";
String global_version = "0.8.4";
uint32_t main_interval_ms = 1000; // 1s default intervall for first iteration
String sensor_id = "";
float celsius, fahrenheit;

// core syncs
// button is polled on CPU1, wm manager is on CPU0
bool show_config_portal_due = false;
bool reset_config_due = false;

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  Serial.print("String has length ");
  Serial.println(len);
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    // replace values in byte-array cache with modified data
    // no changes made to flash, all in local byte-array cache
    EEPROM.put(addrOffset + 1 + i, strToWrite[i]);
  }
  Serial.println("Data put to EEPROM.");

  // https://arduino.stackexchange.com/questions/25945/how-to-read-and-write-eeprom-in-esp8266
  // actually write the content of byte-array cache to
  // hardware flash.  flash write occurs if and only if one or more byte
  // in byte-array cache has been changed, but if so, ALL 512 bytes are
  // written to flash
  EEPROM.commit();
  Serial.println("EEPROM committed.");
}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; // the character may appear in a weird way, you should read: 'only one backslash and 0'
  return String(data);
}

void resetConfig()
{
  Serial.println("Erasing Config, restarting");
  wm.resetSettings();
  ESP.restart();
}

void showConfigPortal()
{
  Serial.println("opening config portal");
  delay(500);

  // start portal w delay
  Serial.println("setting timeout");
  wm.setConfigPortalTimeout(120);
  delay(500);

  Serial.println("disconnecting...");
  wm.disconnect();

  Serial.println("serving acces point...");
  if (!wm.startConfigPortal(ssid.c_str()))
  {
    Serial.println("failed to connect or hit timeout");
    delay(3000);
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connection re-established");
  }
}

void arrayToString(byte array[], unsigned int len, char buffer[])
{
  // source https://stackoverflow.com/questions/44748740/
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}

void all_LEDs_off()
{
  led_onoff.Off();
  led_relay.Off();
  led_wifi.Off();

  led_onoff.Update();
  led_relay.Update();
  led_wifi.Update();
}

void set_LEDs()
{
  // Auto / relay LED
  if (currentStateAc1 == AC_STATE_AUT)
  {
    if (ac1.getCurrentValue()) {
      led_relay.Blink(800, 200).Forever();
    } else {
      led_relay.Blink(200, 800).Forever();
    }
  }
  else if (currentStateAc1 == AC_STATE_OFF)
  {
    led_relay.Off();
  }
  else if (currentStateAc1 == AC_STATE_ON)
  {
    led_relay.On();
  }
  led_relay.Update();

  // LED Wifi is set in setup
}

void handler_ac(Button2 &btn)
{
  Serial.println("AC handler called!");

  if (btn == btn_main)
  {
    if (currentStateAc1 == AC_STATE_AUT)
    {
      currentStateAc1 = AC_STATE_OFF;
      ac1.setOverwriteEnabled(true);
      ac1.setOverwriteValue(false);
    }
    else if (currentStateAc1 == AC_STATE_OFF)
    {
      currentStateAc1 = AC_STATE_ON;
      ac1.setOverwriteEnabled(true);
      ac1.setOverwriteValue(true);
    }
    else if (currentStateAc1 == AC_STATE_ON)
    {
      currentStateAc1 = AC_STATE_AUT;
      ac1.setOverwriteEnabled(false);
      ac1.setOverwriteValue(false);
    }

    ac1.setCurrentValueOnGpio();
    set_LEDs();
  }

  set_LEDs();
}

void handler_main(Button2 &btn)
{
  Serial.print("Type: ");
  Serial.print(btn.getType());
  Serial.println("##############");
  
  switch (btn.getType())
  {
  case double_click:
    Serial.println("double ");

    led_onoff.Blink(70, 200).Repeat(4); // 4 will display as 3
    show_config_portal_due = true;
    
    // triggers panic
    /* 
    *wm:Guru Meditation Error: Core  1 panic'ed (Unhandled debug exception). 
Debug exception reason: Stack canary watchpoint triggered (CPU_1) 
Core  1 register dump:
PC      : 0x40164420  PS      : 0x00060e36  A0      : 0x80163b44  A1      : 0x3ffb8eb0  
A2      : 0x3ffb956c  A3      : 0x3ffb91c0  A4      : 0x3f4165ac  A5      : 0x3ffb9260  
A6      : 0x3ffb9240  A7      : 0x00000008  A8      : 0x8008f6c6  A9      : 0x3ffb9180  
A10     : 0x3ffb956c  A11     : 0xba65d5ca  A12     : 0xba65d5ca  A13     : 0x3ffc3dec  
A14     : 0x3ffb8a28  A15     : 0x00000000  SAR     : 0x00000000  EXCCAUSE: 0x00000001  
EXCVADDR: 0x00000000  LBEG    : 0x4008a5ad  LEND    : 0x4008a5bd  LCOUNT  : 0xfffffffc  


Backtrace:0x4016441d:0x3ffb8eb00x40163b41:0x3ffb91c0 0x400e92cd:0x3ffb9280 0x400dce76:0x3ffb92d0 0x400dcf21:0x3ffb9330 0x400e3ca2:0x3ffb9350 0x400d343d:0x3ffb93d0 0x400d3a26:0x3ffb93f0 0x4017321f:0x3ffb9410 0x400d4f2b:0x3ffb9430 0x400d50a6:0x3ffb9450 0x400d3e4a:0x3ffb9470 
*/
    break;
  case long_click:
  case empty: // TODO: Debug... for some strange reason long click gives 4=empty
    Serial.println("Button Held");
    led_onoff.Blink(70, 200).Repeat(11); // 11 will display as 10 (first turnoff)
    reset_config_due = true;

    // resetConfig(); // triggers panic
    break;
  default:
    Serial.print("sth else ");
    break;
  }
  Serial.print("click main");
  Serial.println();

  set_LEDs();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

String getParam(String name)
{
  // read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name))
  {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback()
{
  Serial.println("[CALLBACK] saveParamCallback fired");
  apikey = getParam("apikey").substring(0, 20);
  Serial.println("PARAM apikey straight = " + apikey);
  // String apikey = getParam("apikey");
  writeStringToEEPROM(EEPROM_ADDRESS_APIKEY, apikey);
  String apikey_restored = readStringFromEEPROM(EEPROM_ADDRESS_APIKEY);
  Serial.println("PARAM apikey from EEPROM = " + apikey_restored);
}

String getFlashChipId()
{
  // TODO: Replace with ESP.getFlashChipId()
  char ssid[24];
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);

  // yields i.e. "AC1A-BC842178" for 78:21:84:BC:1A:AC
  // snprintf(ssid, 24, "%04X-%08X", chip, (uint32_t)chipid);
  // yields i.e. 
  snprintf(ssid, 24, "%08D", chip);

  return String(ssid);
}

void CoreTask1(void *parameter)
{
  for (;;)
  {
    btn_main.loop();
    led_relay.Update();
    led_wifi.Update();

    if (!led_onoff.Update())
    {
      // update returns false if no effect is running => turn back on
      led_onoff.On();
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // create second task for handling GPIOs
  xTaskCreatePinnedToCore(CoreTask1, "CPU_1", 1000, NULL, 1, &Core0TaskHnd, 1);

  chipid = String(getFlashChipId()) + "_" + String(WiFi.macAddress());
  Serial.println("chipid: " + chipid);
  ssid = "Brick-32-" + String(getFlashChipId());

  // init GPIO (leds + buttons)
  set_LEDs();

  // EEPROM
  Serial.println("initializing EEPROM..");
  EEPROM.begin(512);

  // buttons are initialized by button lib
  btn_main.begin(GPIO_IN_BUTTON);
  btn_main.setClickHandler(handler_ac);
  btn_main.setLongClickDetectedHandler(handler_main);
  btn_main.setLongClickTime(3000);
  btn_main.setDoubleClickHandler(handler_main);

  // wifi manager
  Serial.println("initializing WiFi..");
  led_wifi.Blink(200,200).Forever();
  new (&custom_field_apikey) WiFiManagerParameter("apikey", "Bricks API Key", "", 37, "placeholder=\"get your API key at bricks.bierbot.com\"");
  wm.addParameter(&custom_field_apikey);
  // wm.setConfigPortalBlocking(false);
  wm.setSaveConfigCallback(saveParamCallback);
  bool res;
  Serial.println("check 2..");
  res = wm.autoConnect(ssid.c_str());
  if (!res)
  {
    Serial.println("Failed to connect");
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    led_wifi.On();
  }

  // Temperature sensor
  // power up RJ11 connector
  pinMode(GPIO_OUT_POWER_RJ11, OUTPUT); // Setzt den Digitalpin 13 als Outputpin
  digitalWrite(GPIO_OUT_POWER_RJ11, 1);
  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  delay(750);
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  if (!sensors.getAddress(insideThermometer, 0))
    Serial.println("Unable to find address for Device 0");

  Serial.print("2nd try Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);

  char buff[15] = ""; //3ffc3664
  sprintf(buff, "0x0000%x", insideThermometer);
  sensor_id = String(buff);

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  // 0.5, 0.25, 0.125, and 0.0625 degC for 9- , 10-, 11-, and 12-bit
  sensors.setResolution(insideThermometer, 11);

  // DRSW relais
  pinMode(GPIO_OUT_DRSW_ONOFF, OUTPUT);
  digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  celsius = sensors.getTempC(deviceAddress);
  if (celsius == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(celsius);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(celsius)); // Converts celsius to Fahrenheit
}

void contactBackend()
{
  if (1 == 1)
  { // WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    String apikey_restored = readStringFromEEPROM(EEPROM_ADDRESS_APIKEY);
    String temp = String(celsius);
    String relais1 = String((int)ac1.getCurrentValue());
    String relais2 = String(digitalRead(GPIO_OUT_DRSW_ONOFF));
    String url = "https://bricks.bierbot.com/api/iot/v1?apikey=" + apikey_restored + "&type=" + "sonoff_th_origin" + "&brand=" + "bierbot" + "&version=" + global_version + "&s_number_temp_0=" + temp + "&a_bool_epower_0=" + relais1 + "&a_bool_epower_1=" + relais2 + "&chipid=" + chipid + "&s_number_temp_id_0=" + sensor_id;

    Serial.print("s_number_temp_0=" + temp);
    Serial.print(", a_bool_epower_0=" + relais1);
    Serial.println(", a_bool_epower_1=" + relais2);

    WiFiClientSecure client;
    client.setInsecure(); // unfortunately necessary, ESP8266 does not support SSL without hard coding certificates
    char cchar_url[url.length() + 1];
    url.toCharArray(cchar_url, url.length() + 1); // Converts String into character array
    client.connect(cchar_url, 443);

    http.begin(client, url);

    Serial.println("submitting GET to " + url);
    int httpCode = http.GET(); // GET has issues with 301 forwards
    Serial.print("httpCode: ");
    Serial.println(httpCode);

    if (httpCode > 0)
    { // Check the returning code

      for (int i = 0; i < http.headers(); i++)
      {
        Serial.println(http.header(i));
      }

      String payload = http.getString(); // Get the request response payload
      Serial.print(F("received payload: "));
      Serial.println(payload); // Print the response payload

      if (payload.length() == 0)
      {
        Serial.println(F("payload empty, skipping."));
        Serial.println(F("setting next request to 60s."));
        main_interval_ms = 60000;
        ac1.setControlValue(false);
        digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
      }
      else if (payload.indexOf("{") == -1)
      {
        Serial.println("no JSON - skippping.");
        Serial.println(F("setting next request to 60s."));
        main_interval_ms = 60000;
        ac1.setControlValue(false);
        digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
      }
      else
      {
        StaticJsonDocument<HTTP_REQUEST_RESPONSE_BUF_LEN> doc;

        // Deserialize the JSON document
        // doc should look like "{\"target_state\":0, \"next_request_ms\":10000,\"error\":1,\"error_text\":\"Your device will need an upgrade\",\"warning\":1,\"warning_text\":\"Your device will need an upgrade\"}";
        DeserializationError error = deserializeJson(doc, payload);
        // Test if parsing succeeds.
        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          global_error = 1;
          global_error_text = String("JSON error: ") + String(error.f_str());
          ac1.setControlValue(false);
          digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
        }
        else
        {
          Serial.println("deserializeJson success");
          if (doc["error"])
          {
            // an error is critical, do not proceed with logik
            const char *error_text = doc["error_text"];
            global_error_text = String(error_text);
            global_error = 1;
            ac1.setControlValue(false);
            digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
          }
          else
          {
            global_error = 0;
            if (doc["warning"])
            {
              // proceed with logik, if there is a warning
              const char *warning_text = doc["warning_text"];
              global_warning_text = String(warning_text);
              global_warning = 1;
              ac1.setControlValue(false);
              digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
            }
            else
            {
              global_warning = 0;
            }

            // main logic here
            if (doc.containsKey("next_request_ms"))
            {
              main_interval_ms = doc["next_request_ms"].as<long>();
            }
            else
            {
              // try again in 30s
              main_interval_ms = 30000;
            }
            // AC relais
            if (doc.containsKey("epower_0_state"))
            {
              ac1.setControlValue(doc["epower_0_state"]);
            }
            else
            {
              ac1.setControlValue(false);
            }
            // Dry Contact relais
            if (doc.containsKey("epower_1_state"))
            {
              digitalWrite(GPIO_OUT_DRSW_ONOFF, doc["epower_1_state"].as<uint8_t>());
            }
            else
            {
              digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
            }           
          }
        }
      }
    }
    else
    {
      Serial.print(F("setting next request to 60s."));
      main_interval_ms = 60000;
      ac1.setControlValue(false);
      digitalWrite(GPIO_OUT_DRSW_ONOFF, 0);
    }
    http.end(); // Close connection
  }
};

void loop()
{
  if (show_config_portal_due) {
    show_config_portal_due = false;
    showConfigPortal();
  }
  if (reset_config_due) {
    reset_config_due = false;
    resetConfig();
  }
  static uint32_t state_main_interval = 0;
  if (TimeReached(state_main_interval))
  {
    // one interval delay in case server wants to set new interval
    SetNextTimeInterval(state_main_interval, main_interval_ms);

    Serial.println("##### MAIN: reading temperature");
    sensors.requestTemperatures(); // Send the command to get temperatures
    printTemperature(insideThermometer);

    Serial.println("##### MAIN: contacting backend");
    contactBackend();

    Serial.println("##### MAIN: setting relais");
    ac1.setCurrentValueOnGpio();
    set_LEDs();
  }
}
