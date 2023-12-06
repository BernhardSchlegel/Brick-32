#ifndef TM1621DRV_H
#define TM1621DRV_H

#include <Arduino.h>

class Tm1621Drv
{
public:
  Tm1621Drv(void);
  void init(uint8_t /*GPIO_TM1621_DAT*/, uint8_t /*GPIO_TM1621_CS*/, uint8_t /*GPIO_TM1621_RD*/, uint8_t /*GPIO_TM1621_WR*/);
  void cls(void);
  void showTemp(float temperature, bool fahrenheit=false);
  void showText(const char* text);
};

#endif /* TM1621DRV_H */