#include "Tm1621Drv.h"

typedef struct
{
  char row[2][12];
  uint8_t pin_da;
  uint8_t pin_cs;
  uint8_t pin_rd;
  uint8_t pin_wr;
  bool celsius;
  bool fahrenheit;
} tm1621Device_t;

tm1621Device_t tm1621Device;

#define XDRV_87 87
#define TM1621_PULSE_WIDTH 10 // microseconds (Sonoff = 100)
#define TM1621_SYS_EN 0x01    // 0b00000001
#define TM1621_LCD_ON 0x03    // 0b00000011
#define TM1621_TIMER_DIS 0x04 // 0b00000100
#define TM1621_WDT_DIS 0x05   // 0b00000101
#define TM1621_TONE_OFF 0x08  // 0b00001000
#define TM1621_BIAS 0x29      // 0b00101001 = LCD 1/3 bias 4 commons option
#define TM1621_IRQ_DIS 0x80   // 0b100x0xxx

const uint8_t tm1621_commands[] = {TM1621_SYS_EN, TM1621_LCD_ON, TM1621_BIAS, TM1621_TIMER_DIS, TM1621_WDT_DIS, TM1621_TONE_OFF, TM1621_IRQ_DIS};
const char tm1621_kchar[] PROGMEM = { "0|1|2|3|4|5|6|7|8|9|-| |A|F|N|O|T|U" };
//                                          0     1     2     3     4     5     6     7     8     9     -     off    A     F     n     o     t     u
const uint8_t tm1621_digit_row[2][18] = {{ 0x5F, 0x50, 0x3D, 0x79, 0x72, 0x6B, 0x6F, 0x51, 0x7F, 0x7B, 0x20, 0x00, 0x77, 0x27, 0x64, 0x6C, 0x26, 0x4C },
                                         { 0xF5, 0x05, 0xB6, 0x97, 0x47, 0xD3, 0xF3, 0x85, 0xF7, 0xD7, 0x02, 0x00, 0xE7, 0xE2, 0x23, 0x33, 0x62, 0x31 }};
/*
Row 0:
    1
   ---
2 |   | 16
   --- 32
4 |   | 64
   ---
    8

Row 1:
    128
    ---
64 |   | 4
    --- 2
32 |   | 1
    ---
    16
*/


/* Private static */

static int getCommandCode(char* destination, size_t destination_size, const char* needle, const char* haystack)
{
  // Returns -1 of not found
  // Returns index and command if found
  int result = -1;
  const char* read = haystack;
  char* write = destination;

  while (true) {
    result++;
    size_t size = destination_size -1;
    write = destination;
    char ch = '.';
    while ((ch != '\0') && (ch != '|')) {
      ch = pgm_read_byte(read++);
      if (size && (ch != '|'))  {
        *write++ = ch;
        size--;
      }
    }
    *write = '\0';
    if (!strcasecmp(needle, destination)) {
      break;
    }
    if (0 == ch) {
      result = -1;
      break;
    }
  }
  return result;
}

static void stopSequence(void)
{
  digitalWrite(tm1621Device.pin_cs, 1); // Stop command sequence
  delayMicroseconds(TM1621_PULSE_WIDTH / 2);
  digitalWrite(tm1621Device.pin_da, 1); // Reset data
}

static void sendCmnd(uint16_t command)
{
  uint16_t full_command = (0x0400 | command) << 5; // 0b100cccccccc00000
  digitalWrite(tm1621Device.pin_cs, 0);           // Start command sequence
  delayMicroseconds(TM1621_PULSE_WIDTH / 2);
  for (uint32_t i = 0; i < 12; i++)
  {
    digitalWrite(tm1621Device.pin_wr, 0); // Start write sequence
    if (full_command & 0x8000)
    {
      digitalWrite(tm1621Device.pin_da, 1); // Set data
    }
    else
    {
      digitalWrite(tm1621Device.pin_da, 0); // Set data
    }
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(tm1621Device.pin_wr, 1); // Read data
    delayMicroseconds(TM1621_PULSE_WIDTH);
    full_command <<= 1;
  }
  stopSequence();
}

static void sendCommon(uint8_t common)
{
  for (uint32_t i = 0; i < 8; i++)
  {
    digitalWrite(tm1621Device.pin_wr, 0); // Start write sequence
    if (common & 1)
    {
      digitalWrite(tm1621Device.pin_da, 1); // Set data
    }
    else
    {
      digitalWrite(tm1621Device.pin_da, 0); // Set data
    }
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(tm1621Device.pin_wr, 1); // Read data
    delayMicroseconds(TM1621_PULSE_WIDTH);
    common >>= 1;
  }
}

static void sendAddress(uint16_t address)
{
  uint16_t full_address = (address | 0x0140) << 7; // 0b101aaaaaa0000000
  digitalWrite(tm1621Device.pin_cs, 0);           // Start command sequence
  delayMicroseconds(TM1621_PULSE_WIDTH / 2);
  for (uint32_t i = 0; i < 9; i++)
  {
    digitalWrite(tm1621Device.pin_wr, 0); // Start write sequence
    if (full_address & 0x8000)
    {
      digitalWrite(tm1621Device.pin_da, 1); // Set data
    }
    else
    {
      digitalWrite(tm1621Device.pin_da, 0); // Set data
    }
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(tm1621Device.pin_wr, 1); // Read data
    delayMicroseconds(TM1621_PULSE_WIDTH);
    full_address <<= 1;
  }
}

static void sendRows(void)
{
  // Tm1621.row[x] = "----", "    " or a number with one decimal like "0.4", "237.5", "123456.7"
  // "123456.7" will be shown as "9999" being a four digit overflow

  uint8_t buffer[8] = {0}; // TM1621 16-segment 4-bit common buffer
  char row[4];
  for (uint32_t j = 0; j < 2; j++)
  {
    // 0.4V => "  04", 0.0A => "  ", 1234.5V => "1234"
    uint32_t len = strlen(tm1621Device.row[j]);
    char *dp = nullptr;    // Expect number larger than "123"
    int row_idx = len - 3; // "1234.5"
    if (len <= 5)
    { // "----", "    ", "0.4", "237.5"
      dp = strchr(tm1621Device.row[j], '.');
      row_idx = len - 1;
    }
    else if (len > 6)
    { // "12345.6"
      snprintf_P(tm1621Device.row[j], sizeof(tm1621Device.row[j]), PSTR("9999"));
      row_idx = 3;
    }
    row[3] = (row_idx >= 0) ? tm1621Device.row[j][row_idx--] : ' ';
    if ((row_idx >= 0) && dp)
    {
      row_idx--;
    }
    row[2] = (row_idx >= 0) ? tm1621Device.row[j][row_idx--] : ' ';
    row[1] = (row_idx >= 0) ? tm1621Device.row[j][row_idx--] : ' ';
    row[0] = (row_idx >= 0) ? tm1621Device.row[j][row_idx--] : ' ';

    char command[10];
    char needle[2] = {0};
    for (uint32_t i = 0; i < 4; i++)
    {
      needle[0] = row[i];
      int index = getCommandCode(command, sizeof(command), (const char *)needle, tm1621_kchar);
      if (-1 == index)
      {
        index = 11;
      }
      uint32_t bidx = (0 == j) ? i : 7 - i;
      buffer[bidx] = tm1621_digit_row[j][index];
    }
    if (dp)
    {
      if (0 == j)
        buffer[2] |= 0x80; // Row 1 decimal point
      else
        buffer[5] |= 0x08; // Row 2 decimal point
    }
  }

  if (tm1621Device.fahrenheit) {
    buffer[1] |= 0x80;
  }
  if (tm1621Device.celsius) {
    buffer[3] |= 0x80;
  }
  /*
  if (tm1621Device.humidity) {
    buffer[6] |= 0x08;
  }
  if (tm1621Device.kWh) {
    buffer[4] |= 0x08;
  }
  if (tm1621Device.voltage) {
    buffer[7] |= 0x08;
  }
  */

  sendAddress(0x10); // Sonoff only uses the upper 16 Segments
  for (uint32_t i = 0; i < 8; i++) { sendCommon(buffer[i]); }
  stopSequence();
}


/* Constructor */

Tm1621Drv::Tm1621Drv(void)
{

}


/* Public */

void Tm1621Drv::init(uint8_t gpio_da, uint8_t gpio_cs, uint8_t gpio_rd, uint8_t gpio_wr)
{
  tm1621Device.pin_cs = gpio_cs;
  pinMode(tm1621Device.pin_cs, OUTPUT);
  digitalWrite(tm1621Device.pin_cs, 0);
  //delayMicroseconds(80);

  tm1621Device.pin_rd = gpio_rd;
  pinMode(tm1621Device.pin_rd, OUTPUT);
  digitalWrite(tm1621Device.pin_rd, 0);
  //delayMicroseconds(15);

  tm1621Device.pin_wr = gpio_wr;
  pinMode(tm1621Device.pin_wr, OUTPUT);
  digitalWrite(tm1621Device.pin_wr, 0);
  //delayMicroseconds(25);

  tm1621Device.pin_da = gpio_da;
  pinMode(tm1621Device.pin_da, OUTPUT);
  digitalWrite(tm1621Device.pin_da, 0);
  delayMicroseconds(TM1621_PULSE_WIDTH);
  digitalWrite(tm1621Device.pin_da, 1);

  for (uint32_t command = 0; command < sizeof(tm1621_commands); command++) { sendCmnd(tm1621_commands[command]); }

  sendAddress(0x00);
  for (uint32_t segment = 0; segment < 16; segment++) { sendCommon(0); }
  stopSequence();

  snprintf_P(tm1621Device.row[0], sizeof(tm1621Device.row[0]), PSTR("8888"));
  snprintf_P(tm1621Device.row[1], sizeof(tm1621Device.row[1]), PSTR("8888"));
  tm1621Device.celsius = tm1621Device.fahrenheit = true;
  sendRows();
}

void Tm1621Drv::cls(void)
{
  snprintf_P(tm1621Device.row[0], sizeof(tm1621Device.row[0]), PSTR("    "));
  snprintf_P(tm1621Device.row[1], sizeof(tm1621Device.row[1]), PSTR("    "));
  tm1621Device.celsius = tm1621Device.fahrenheit = false;
  sendRows();
}

void Tm1621Drv::showTemp(float temperature, bool fahrenheit)
{
  /* Temperature unit is fixed to row[0] */
  snprintf_P(tm1621Device.row[0], sizeof(tm1621Device.row[0]), PSTR("%.1f"), temperature);
  tm1621Device.fahrenheit = tm1621Device.celsius = false;
  (fahrenheit) ? tm1621Device.fahrenheit = true : tm1621Device.celsius = true;
  sendRows();
}

void Tm1621Drv::showText(const char* text)
{
  // Always print text in row[1]
  snprintf(tm1621Device.row[1], sizeof(tm1621Device.row[1]), text); // right padded text
  sendRows();
}