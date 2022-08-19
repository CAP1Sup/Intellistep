// Import the config (needed for the ENABLE_OLED define)
#include "config.h"

// Imports
#include "Arduino.h"
#include "buttons.h"
#include "config.h"
#include "oled.h"

// Variable definitions
// Boolean for storing if the dip switches were installed the wrong way
bool dipInverted = false;
uint32_t lastButtonClickTime = 0;

// Initialize the button pins as inputs
void initButtons() {

  // Only build if specified
  #if (ENABLE_OLED != 0)

    // Down pin (moves the menu down)
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);

    // Select pin (selects the current menu item)
    pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);

    // Back pin (opens menu and also backs out of menus)
    pinMode(BACK_BUTTON_PIN, INPUT_PULLUP);
  #endif

  // All of the dip switches
  pinMode(DIP_1_PIN, INPUT_PULLUP);
  pinMode(DIP_2_PIN, INPUT_PULLUP);
  pinMode(DIP_3_PIN, INPUT_PULLUP);
  pinMode(DIP_4_PIN, INPUT_PULLUP);

  // Read the microstepping of the dip switches
  readDipMicrostepping();
}

// Only include button code if using the OLED panel
#if (ENABLE_OLED != 0)

// Scan each of the buttons
void checkButtons() {

  // Make sure that the buttons don't repeat too fast
  if (millis() - lastButtonClickTime > BUTTON_REPEAT_INTERVAL) {

    // Check the select buttons
    if(checkButtonState(SELECT_BUTTON_PIN)) {

      // Update the last click time
      lastButtonClickTime = millis();

      // Select the current menu item
      selectMenuItem();
    }

    // Check the down button
    else if(checkButtonState(DOWN_BUTTON_PIN)) {

      // Update the last click time
      lastButtonClickTime = millis();

      // Move down
      moveCursor();
    }

    // Check the back button
    else if(checkButtonState(BACK_BUTTON_PIN)) {

      // Update the last click time
      lastButtonClickTime = millis();

      // Back up
      exitCurrentMenu();
    }
  }
}

// Check to see if a button is clicked
bool checkButtonState(PinName buttonPin) {

  // Check to see if the select button is clicked (pins are pulled high by default)
  if(GPIO_READ(buttonPin) == LOW){

    // Wait to make sure it wasn't a misread
    delay(10);

    // Check again
    if(GPIO_READ(buttonPin) == LOW){

      // The button is still clicked, it's valid
      return true;

    }
    else {

      // The button is no longer clicked, probably just a misread
      return false;
    }

  }
  else {

    // Button was never clicked
    return false;
  }
}
#endif // ! ENABLE_OLED

// Function for reading the microstepping set via the dip switches
void readDipMicrostepping() {
  if (FlashParameters::getInstance().getDipswitchUse())
    {
      // Dips are not inverted, they are installed correctly
      PinName dip1 = DIP_1_PIN;
      PinName dip2 = DIP_2_PIN;

      // Check if the dip switches are inverted
      if (dipInverted) {
          // If they were installed incorrectly, they have to be read opposite
          dip1 = DIP_4_PIN;
          dip2 = DIP_3_PIN;
      }

      if (!GPIO_READ(dip1) && !GPIO_READ(dip2)) {

          // Set the microstepping to 1/32 if both dips are on
          motor.setMicrostepping(32, false);
      }
      else if (GPIO_READ(dip1) && !GPIO_READ(dip2)) {

          // Set the microstepping to 1/16 if the left dip is off and the right is on
          motor.setMicrostepping(16, false);
      }
      else if (!GPIO_READ(dip1) && GPIO_READ(dip2)) {

          // Set the microstepping to 1/8 if the right dip is off and the left on
          motor.setMicrostepping(8, false);
      }
      else {
          // Both are off, just revert to using full stepping
          motor.setMicrostepping(1, false);
      }

      
    }
}


// Check all of the dip switches
void checkDips() {
  if (FlashParameters::getInstance().getDipswitchUse())
  {
    // Read the microstepping
    readDipMicrostepping();

    // Check if the dip switches are inverted
    // Check open/closed loop
    if (!GPIO_READ(dipInverted ? DIP_2_PIN : DIP_3_PIN)) {
      enableStepCorrection();
    }
    else {
      disableStepCorrection();
    }
  }
}


// Function for setting if the dip switches should be inverted
void setDipInverted(bool inverted) {
    dipInverted = inverted;
}

// Function for getting if the dip switches should be inverted
bool getDipInverted() {
    return dipInverted;
}