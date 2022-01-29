#ifndef __KEY_H
#define __KEY_H

#include "main.h"

// Need the timer library
#include "timers.h"

// Function definitions
void initButtons();
void checkButtons(bool updateScreen, bool onlyAllowSelect = false);
bool checkButtonState(PinName buttonPin);
void readDipMicrostepping();
void checkDips();
void setDipInverted(bool inverted);
bool getDipInverted();

#endif


