#include "AcOut.h"

AcOut::AcOut(unsigned int pin_on_1, unsigned int pin_off)
{
  this->init(pin_on_1, 0, pin_off, false);
}

AcOut::AcOut(unsigned int pin_on_1, unsigned int pin_off, bool invert)
{
  this->init(pin_on_1, 0, pin_off, invert);
}

AcOut::AcOut(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert)
{
  this->init(pin_on_1, pin_on_2, pin_off, invert);
}

void AcOut::init(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert)
{
  this->pin_off= pin_off;
  this->pin_on_1= pin_on_1;
  this->pin_on_2= pin_on_2;
  this->control_state = false;
  this->overwrite_enabled = false;
  this->overwrite_state = false;
  pinMode(this->pin_on_1, OUTPUT);
  if (this->pin_on_2 != 0)
  {
    pinMode(this->pin_on_2, OUTPUT);
  }
  pinMode(this->pin_off, OUTPUT);
  this->invert = invert;
}

void AcOut::setControlValue(bool value)
{
    this->control_state = value;
}

void AcOut::setOverwriteEnabled(bool enabled)
{
    this->overwrite_enabled = enabled;
}

void AcOut::setOverwriteValue(bool value)
{
    this->overwrite_state = value;
}

bool AcOut::getCurrentValue()
{
    if (this->overwrite_enabled)
    {
        return this->overwrite_state;
    }
    else
    {
        return this->control_state;
    }
}

void AcOut::setCurrentValueOnGpio()
{
    bool mappedValue = this->getCurrentValue();

    if (this->invert)
    {
        mappedValue = !mappedValue;
    }

    if (mappedValue)
    {
      digitalWrite(this->pin_on_1, 1);
      digitalWrite(this->pin_off, 0);

      if (this->pin_on_2 != 0) {
        digitalWrite(this->pin_on_2, 1);
      }
    }
    else
    {
      digitalWrite(this->pin_on_1, 0);
      digitalWrite(this->pin_off, 1);

      if (this->pin_on_2 != 0)
      {
        digitalWrite(this->pin_on_2, 0);
      }
    }
}