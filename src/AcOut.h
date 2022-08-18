#ifndef ACOUT_H
#define ACOUT_H

#include <Arduino.h>

class AcOut
{

private:
  unsigned int pin_on_1;
  unsigned int pin_on_2;
  unsigned int pin_off;
  bool control_state;
  bool overwrite_state;
  bool overwrite_enabled;
  bool invert;

public:
  AcOut(unsigned int pin_on_1, unsigned int pin_off);
  AcOut(unsigned int pin_on_1, unsigned int pin_off, bool invert);
  AcOut(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert);

  void setControlValue(bool value);

  void setOverwriteEnabled(bool enabled);

  void setOverwriteValue(bool value);

  bool getCurrentValue();

  void setCurrentValueOnGpio();

private:
  void init(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert);
};

#endif