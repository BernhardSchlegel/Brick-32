#ifndef GENERICOUT_H
#define GENERICOUT_H

#include <Arduino.h>

class GenericOut
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
  GenericOut(unsigned int pin_on_1, bool invert);
  GenericOut(unsigned int pin_on_1, unsigned int pin_off);
  GenericOut(unsigned int pin_on_1, unsigned int pin_off, bool invert);
  GenericOut(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert);

  void setControlValue(bool value);

  void setOverwriteEnabled(bool enabled);

  void setOverwriteValue(bool value);

  bool getCurrentValue();

  bool isValid();

  void setCurrentValueOnGpio();

private:
  void init(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert);
};

#endif