#ifndef __CONFIG_H
#define __CONFIG_H

// Need the Arduino library for pin conversion
#include "Arduino.h"

// Macros file (for convience functions)
#include "macros.h"

// Version of the firmware (displayed on OLED) (follows semantic versioning)
#define MAJOR_VERSION (uint16_t)0
#define MINOR_VERSION (uint16_t)1
#define PATCH_VERSION (uint16_t)0

// --------------  Settings  --------------

// LED light functionality
#define ENABLE_LED // red LED labeled as an 'error' in the schema
#ifdef ENABLE_LED
    //#define ENABLE_BLINK
#endif

// OLED (display)
#define ENABLE_OLED 1
#if (ENABLE_OLED != 0)

    // Button settings
    //#define INVERTED_DIPS // Enable if your dips are inverted ("on" print is facing away from motor connector)
    #define BUTTON_REPEAT_INTERVAL 250 // Millis
    #define MENU_RETURN_LEVEL MOTOR_DATA // The level to return to after configuring a setting
    #define WARNING_MICROSTEP MAX_MICROSTEP_DIVISOR // The largest microstep to warn on (the denominator of the fraction)

    // Warning thresholds
    #define WARNING_RMS_CURRENT 1000 // The RMS current at which to display a warning confirmation (mA)
    //#define WARNING_PEAK_CURRENT 1000 // The peak current at which to display a warning confirmation (mA)

    #define CURRENT_MENU_INCREMENT  (uint16_t)100 // The value to step the OLED current options by
#endif

// Serial configuration settings
#define ENABLE_SERIAL 1
#if (ENABLE_SERIAL != 0)
    #define SERIAL_BAUD 115200
#endif

// Motor characteristics
#define STEP_ANGLE (float)1.8 // ! Check to see for .9 deg motors as well
#define STEP_UPDATE_FREQ (uint32_t)78 // in Hz, to step the motor back to the correct position. Multiplied by the microstepping for actual update freq

// Motor settings
// The number of microsteps to move per step pulse
// Doesn't affect correctional movements
// This will allow the dip switches to set the stepping resolution, while
// a step pulse will still move the motor one full step worth of rotation
#define DEFAULT_MICROSTEP_MULTIPLIER    (uint32_t)1

// The min/max microstepping divisors
// Microstepping divisors are the numbers underneath the fraction of the microstepping
// For example, 1/16th microstepping would have a divisor of 16
#define MIN_MICROSTEP_DIVISOR   1
#define MAX_MICROSTEP_DIVISOR   32

// The mode to set the motor to when it's disabled
#define IDLE_MODE               COAST

// If the motor should maintain a full stepping when the microstepping is different
// This allows the motor to run in full step mode, while running quieter
//#define MAINTAIN_FULL_STEPPING

// Import the advanced config, then the pins
#include "config_adv.h"
#include "pins.h"

// Import the sanity check (needed so all files have the defines done in the sanity check file)
// Must be last so that it can use the defines above
#include "sanityCheck.h"

#endif //__CONFIG_H
