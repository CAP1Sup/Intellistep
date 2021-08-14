#ifndef __CONFIG_H
#define __CONFIG_H

// Need the Arduino library for pin conversion
#include "Arduino.h"

// Macros file (for convience functions)
#include "macros.h"

// Version of the firmware (displayed on OLED) (follows semantic versioning)
#define MAJOR_VERSION (uint16_t)0
#define MINOR_VERSION (uint16_t)0
#define PATCH_VERSION (uint16_t)45

// --------------  Settings  --------------

// LED light functionality
#define ENABLE_LED // red LED labeled as an 'error' in the schema
#ifdef ENABLE_LED
    //#define ENABLE_BLINK
#endif

// OLED (display)
#define ENABLE_OLED
#ifdef ENABLE_OLED

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

// Averages (number of readings in average)
// Using a power of 2 will greatly increase the averaging speed
#define USE_POWER_2_FACTOR_AVGING
#ifdef USE_POWER_2_FACTOR_AVGING
    #define RPM_AVG_READINGS       8
    #define SPEED_AVG_READINGS   128
    #define ACCEL_AVG_READINGS     8
    #define ANGLE_AVG_READINGS    16
    #define TEMP_AVG_READINGS    256
#else
    #define RPM_AVG_READINGS      10
    #define SPEED_AVG_READINGS   100
    #define ACCEL_AVG_READINGS    10
    #define ANGLE_AVG_READINGS    15
    #define TEMP_AVG_READINGS    200
#endif

// If encoder estimation should be used
#define ENCODER_SPEED_ESTIMATION
#ifdef ENCODER_SPEED_ESTIMATION
    #define SPD_EST_MIN_INTERVAL 500 // The minimum sampling interval (us). Increase to get more steady readings at the cost of latency
#endif

// Serial configuration settings
#define ENABLE_SERIAL
#ifdef ENABLE_SERIAL
    #define SERIAL_BAUD 115200
#endif

// Parser settings
#define STRING_START_MARKER '<'
#define STRING_END_MARKER '>'

// CAN settings
#define ENABLE_CAN
#ifdef ENABLE_CAN
    // The CAN ID of this board
    // X:2, X2:3...
    // Y:7, Y2:8...
    // Z:11 Z2:12...
    // E:17, E1:18...
    #define DEFAULT_CAN_ID X

    // ! Maybe higher later? (Up to 1MHz for fast transmissions)
    #define CAN_BITRATE BR125K
#endif

// Motor characteristics
#define STEP_ANGLE (float)1.8 // ! Check to see for .9 deg motors as well
#define STEP_UPDATE_FREQ (uint32_t)78 // in Hz, to step the motor back to the correct position. Multiplied by the microstepping for actual update freq

// Dynamic current (adjusts motor current based on acceleration (and therefore torque)
// required from the motor)
//#define ENABLE_DYNAMIC_CURRENT
#ifdef ENABLE_DYNAMIC_CURRENT
    // A dynamically controller current loop. Uses the equation: accel * accelCurrent + idleCurrent
    // Limited by the max dynamic current, which will limit the maximum that the dynamic loop can output
    // All current values are in RMS
    #define DYNAMIC_ACCEL_CURRENT 10 // Multiplied by deg/s/s, in mA
    #define DYNAMIC_IDLE_CURRENT  500 // In mA
    #define DYNAMIC_MAX_CURRENT   750 // In mA
#else
    // Classic, static current
    #define STATIC_RMS_CURRENT     (uint16_t)500 // This is the rating of the motor from the manufacturer

    // Overtemperature protection (lowers motor current when motor temperature rises too high)
    #define ENABLE_OVERTEMP_PROTECTION
    #ifdef ENABLE_OVERTEMP_PROTECTION
        #define OVERTEMP_THRESHOLD_TEMP      70 // The temp to trigger a overtemp current reduction (C)
        #define OVERTEMP_INCREMENT           50 // The increment at which to reduce the current by (RMS mA)
        #define OVERTEMP_INTERVAL            30 // The minimum interval between current reductions (s)
        #define OVERTEMP_SHUTDOWN_TEMP       80 // The temp at which to completely shut down the motor, protecting it against burning up
        #define OVERTEMP_SHUTDOWN_CLEAR_TEMP 70 // Motor can begin movement again once this temp is reached
    #endif
#endif

// PID settings
// ! At this time, this feature is still under development
//#define ENABLE_PID
#ifdef ENABLE_PID

    // Default P, I, and D terms
    #define DEFAULT_P  1000
    #define DEFAULT_I  2
    #define DEFAULT_D  0

    // I windup protection
    #define DEFAULT_MAX_I 10

    // Minimum sample time (in ms)
    //#define PID_MIN_SAMPLE_TIME 10

    // Default min and max for step timing (per second)
    #define DEFAULT_PID_STEP_MAX 50000

    // PID output that the motor should disable at (set to 0 to never disable motor)
    #define DEFAULT_PID_DISABLE_THRESHOLD 0 //1000
#endif

// Direct step functionality (used to command motor to move over Serial/CAN)
//#define ENABLE_DIRECT_STEPPING
#ifdef ENABLE_DIRECT_STEPPING

    // The default stepping rate (in Hz) to move in the event that no parameter is specified
    #define DEFAULT_STEPPING_RATE 1000
#endif

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

// Stallfault
//#define ENABLE_STALLFAULT
#ifdef ENABLE_STALLFAULT
    #define STEP_FAULT_TIME         1 // The maximum allowable time (sec) for a step fault (meaning motor is out of position)
    #define STEP_FAULT_STEP_COUNT   10 // The maximum allowable deviation between the actual and set steps before StallFault is triggered

    // StallFault connection (to mainboard)
    // Pulls high on a stepper misalignment after the set period or angular deviation
    #define STALLFAULT_PIN PA_13 //output(GPIOA_BASE_BASE, 13)
#endif

// Import the advanced config, then the pins
#include "config_adv.h"
#include "pins.h"

// Import the sanity check (needed so all files have the defines done in the sanity check file)
// Must be last so that it can use the defines above
#include "sanityCheck.h"

#endif //__CONFIG_H
