#include "GenericOut.h"

GenericOut::GenericOut(unsigned int pin_on_1, bool invert)
{
  this->init(pin_on_1, 0, 0, false);
}

GenericOut::GenericOut(unsigned int pin_on_1, unsigned int pin_off)
{
  this->init(pin_on_1, 0, pin_off, false);
}

GenericOut::GenericOut(unsigned int pin_on_1, unsigned int pin_off, bool invert)
{
  this->init(pin_on_1, 0, pin_off, invert);
}

GenericOut::GenericOut(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert)
{
  this->init(pin_on_1, pin_on_2, pin_off, invert);
}

void GenericOut::init(unsigned int pin_on_1, unsigned int pin_on_2, unsigned int pin_off, bool invert)
{
  this->pin_off = pin_off;
  this->pin_on_1 = pin_on_1;
  this->pin_on_2 = pin_on_2;
  this->control_state = false;
  this->overwrite_enabled = false;
  this->overwrite_state = false;
  this->invert = invert;

  if (this->pin_on_1 != 0) {
    pinMode(this->pin_on_1, OUTPUT);
  }
  if (this->pin_on_2 != 0) {
    pinMode(this->pin_on_2, OUTPUT);
  }
  if (this->pin_off != 0) {
    pinMode(this->pin_off, OUTPUT);
  }
}

void GenericOut::setControlValue(bool value)
{
  if (!this->isValid())
    return;

    this->control_state = value;
}

void GenericOut::setOverwriteEnabled(bool enabled)
{
  if (!this->isValid())
    return;

    this->overwrite_enabled = enabled;
}

void GenericOut::setOverwriteValue(bool value)
{
  if (!this->isValid())
    return;

    this->overwrite_state = value;
}

bool GenericOut::getCurrentValue()
{
  if (!this->isValid())
    return false;

    if (this->overwrite_enabled)
    {
        return this->overwrite_state;
    }
    else
    {
        return this->control_state;
    }
}

bool GenericOut::isValid()
{
  return (this->pin_on_1 != 0);
}

void GenericOut::setCurrentValueOnGpio()
{
  if (!this->isValid())
    return;

  bool mappedValue = this->getCurrentValue();

  if (this->invert)
  {
    mappedValue = !mappedValue;
  }

  if (mappedValue)
  {
    digitalWrite(this->pin_on_1, 1);

    if (this->pin_off != 0) {
      digitalWrite(this->pin_off, 0);
    }

    if (this->pin_on_2 != 0) {
      digitalWrite(this->pin_on_2, 1);
    }
  }
  else
  {
    digitalWrite(this->pin_on_1, 0);

    if (this->pin_off != 0) {
      digitalWrite(this->pin_off, 1);
    }
    
    if (this->pin_on_2 != 0) {
      digitalWrite(this->pin_on_2, 0);
    }
  }
}