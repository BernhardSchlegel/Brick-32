#include "AcOut.h"

AcOut::AcOut(unsigned int pin_on, unsigned int pin_off)
{
    this->init(pin_on, pin_off, false);
}

AcOut::AcOut(unsigned int pin_on, unsigned int pin_off, bool invert)
{
  this->init(pin_on, pin_off, invert);
}

void AcOut::init(unsigned int pin_on, unsigned int pin_off, bool invert)
{
  this->pin_off= pin_off;
  this->pin_on= pin_on;
  this->control_state = false;
  this->overwrite_enabled = false;
  this->overwrite_state = false;
  pinMode(this->pin_on, OUTPUT);
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
      digitalWrite(this->pin_on, 1);
      digitalWrite(this->pin_off, 0);
    }
    else
    {
      digitalWrite(this->pin_on, 0);
      digitalWrite(this->pin_off, 1);
    }
}