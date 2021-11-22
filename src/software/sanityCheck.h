// Make sure that the file is only included once
#pragma once

// Import the config
#include "config.h"

// Check to make sure that at least a peak or RMS max current is defined
#if !(defined(MAX_PEAK_BOARD_CURRENT) || defined(MAX_RMS_BOARD_CURRENT))
    #error "A max peak or max RMS current must be defined!"

// Check to make sure both aren't defined already
#elif (defined(MAX_PEAK_BOARD_CURRENT) && defined(MAX_RMS_BOARD_CURRENT))
    #error "Only max peak or max RMS current can be defined, not both!"
#else
    // Calculate the other current maximum
    #ifdef MAX_PEAK_BOARD_CURRENT
        #define MAX_RMS_BOARD_CURRENT (uint16_t)(MAX_PEAK_BOARD_CURRENT * 0.707)
    #else
        #define MAX_PEAK_BOARD_CURRENT (uint16_t)(MAX_RMS_BOARD_CURRENT * 1.414)
    #endif
#endif

// Warning settings only need checked when the OLED is enabled
#ifdef ENABLE_OLED
    // Check to make sure that at least a peak or RMS wanring current is defined
    #if !(defined(WARNING_PEAK_CURRENT) || defined(WARNING_RMS_CURRENT))
        #error "A warning peak or warning RMS current must be defined!"

    // Check to make sure both aren't defined already
    #elif (defined(WARNING_PEAK_CURRENT) && defined(WARNING_RMS_CURRENT))
        #error "Only warning peak or warning RMS current can be defined, not both!"
    #else
        // Calculate the other current warning
        #ifdef WARNING_PEAK_CURRENT
            #define WARNING_RMS_CURRENT (uint16_t)(WARNING_PEAK_CURRENT * 0.707)
        #else
            #define WARNING_PEAK_CURRENT (uint16_t)(WARNING_RMS_CURRENT * 1.414)
        #endif
    #endif
#endif

// Check to make sure that at least a peak or RMS default current is defined
#if !(defined(STATIC_PEAK_CURRENT) || defined(STATIC_RMS_CURRENT)) && !defined(ENABLE_DYNAMIC_CURRENT)
    #error "A static peak or static RMS current must be defined!"

// Check to make sure both aren't defined already
#elif (defined(STATIC_PEAK_CURRENT) && defined(STATIC_RMS_CURRENT)) && !defined(ENABLE_DYNAMIC_CURRENT)
    #error "Only max peak or max RMS current can be defined, not both!"
#elif !defined(ENABLE_DYNAMIC_CURRENT)
    // Calculate the other current maximum
    #ifdef STATIC_PEAK_CURRENT
        #define STATIC_RMS_CURRENT (uint16_t)(STATIC_PEAK_CURRENT * 0.707)
    #else
        #define STATIC_PEAK_CURRENT (uint16_t)(STATIC_RMS_CURRENT * 1.414)
    #endif
#endif


// Check to make sure that the SINE_MAX and SINE_VAL_COUNT is valid
#if IS_POWER_2(SINE_VAL_COUNT) != 0
    #error SINE_VAL_COUNT must be a power of 2 to use in fastSin() and fastCos() defines!!!
#endif
#if IS_POWER_2(SINE_MAX) != 0
    #error SINE_MAX must be a power of 2 to fast division to SINE_MAX, i.e { y = x / SINE_MAX } is equal to  { y = x >> SINE_POWER }
#endif


// If we're using the faster averaging using a power of 2
#ifdef USE_POWER_2_FACTOR_AVGING

    // Define the default factor as 8
    #define DEFAULT_AVG_FACTOR 8

    // Check to make sure that the MovingAverage argument is valid
    #if IS_POWER_2(RPM_AVG_READINGS) != 0
        #error RPM_AVG_READINGS must be a power of 2 to use in fast MovingAverage!!!
    #endif
    #if IS_POWER_2(SPEED_AVG_READINGS) != 0
        #error SPEED_AVG_READINGS must be a power of 2 to use in fast MovingAverage!!!
    #endif
    #if IS_POWER_2(ACCEL_AVG_READINGS) != 0
        #error ACCEL_AVG_READINGS must be a power of 2 to use in fast MovingAverage!!!
    #endif
    #if IS_POWER_2(ANGLE_AVG_READINGS) != 0
        #error ANGLE_AVG_READINGS must be a power of 2 to use in fast MovingAverage!!!
    #endif
    #if IS_POWER_2(TEMP_AVG_READINGS) != 0
        #error TEMP_AVG_READINGS must be a power of 2 to use in fast MovingAverage!!!
    #endif
#else
    // Define the default factor as 10
    #define DEFAULT_AVG_FACTOR 10
#endif


// Create the firmware print string
// Firmware feature prints
#define VERSION_STRING            String(MAJOR_VERSION) + "." + String(MINOR_VERSION) + "." + String(PATCH_VERSION)
#define FIRMWARE_FEATURE_VERSION  String("Version: " + VERSION_STRING + "\n")
#define FIRMWARE_BUILD_INFO       String("Compiled: " + String(__DATE__) + ", " + String(__TIME__) + "\n")
#define FIRMWARE_FEATURE_HEADER   String("Enabled features:")

// Firmware feature print definition
#ifdef ENABLE_OLED
    #define FIRMWARE_FEATURE_OLED     "\nOLED"
#else
    #define FIRMWARE_FEATURE_OLED     ""
#endif

#ifdef ENABLE_SERIAL
    #define FIRMWARE_FEATURE_SERIAL    "\nSerial"
#else
    #define FIRMWARE_FEATURE_SERIAL    ""
#endif

#ifdef ENABLE_CAN
    #define FIRMWARE_FEATURE_CAN    "\nCAN"
#else
    #define FIRMWARE_FEATURE_CAN    ""
#endif

#ifdef ENABLE_STALLFAULT
    #define FIRMWARE_FEATURE_STALLFAULT    "\nStallFault"
#else
    #define FIRMWARE_FEATURE_STALLFAULT    ""
#endif

#ifdef ENABLE_DYNAMIC_CURRENT
    #define FIRMWARE_FEATURE_DYNAMIC_CURRENT    "\nDynamic Current"
#else
    #define FIRMWARE_FEATURE_DYNAMIC_CURRENT    ""
#endif

#ifdef ENABLE_OVERTEMP_PROTECTION
    #define FIRMWARE_FEATURE_OVERTEMP_PROTECTION    "\nOvertemp Protection"
#else
    #define FIRMWARE_FEATURE_OVERTEMP_PROTECTION    ""
#endif

// Main firmware print string
#define FIRMWARE_FEATURE_PRINT String(FIRMWARE_FEATURE_VERSION + FIRMWARE_BUILD_INFO + FIRMWARE_FEATURE_HEADER + FIRMWARE_FEATURE_OLED + FIRMWARE_FEATURE_SERIAL + FIRMWARE_FEATURE_CAN + FIRMWARE_FEATURE_STALLFAULT + FIRMWARE_FEATURE_DYNAMIC_CURRENT + FIRMWARE_FEATURE_OVERTEMP_PROTECTION)


// Check for defines that have conflicts
#if (defined(ENABLE_BLINK) + defined(CHECK_STEPPING_RATE) + defined(CHECK_CORRECT_MOTOR_RATE) + defined(CHECK_ENCODER_SPEED) > 1)
    #error Only one of the following is allowed at a time: ENABLE_BLINK, CHECK_STEPPING_RATE, CHECK_CORRECT_MOTOR_RATE, or CHECK_ENCODER_SPEED
#endif

#if defined(CHECK_MCO_OUTPUT) && defined(CHECK_GPIO_OUTPUT_SWITCHING)
    #error Only one of the following is allowed at a time: CHECK_MCO_OUTPUT, CHECK_GPIO_OUTPUT_SWITCHING
#endif


// Make sure that ENABLE_FULL_MOTION_PLANNER is defined, otherwise ENABLE_DIRECT_STEPPING can't be used
#if (defined(ENABLE_DIRECT_STEPPING) && !defined(ENABLE_FULL_MOTION_PLANNER))
    #error In order to use ENABLE_DIRECT_STEPPING, ENABLE_FULL_MOTION_PLANNER must be uncommented
#endif


// Microstep checks (makes sure that the min and max values are within viable ranges)
#if (IS_POWER_2(MIN_MICROSTEP_DIVISOR) != 0)

    // MIN_MICROSTEP_DIVISOR is not a power of 2
    #error MIN_MICROSTEP_DIVISOR must be a power of 2

#elif (MIN_MICROSTEP_DIVISOR < 1)

    // 1 is the minimum divisor
    #error 1 is the minimum divisor for MIN_MICROSTEP_DIVISOR
#endif

#if (IS_POWER_2(MAX_MICROSTEP_DIVISOR) != 0)

    // MAX_MICROSTEP_DIVISOR is not a power of 2
    #error MAX_MICROSTEP_DIVISOR must be a power of 2

#elif (MAX_MICROSTEP_DIVISOR > (SINE_VAL_COUNT / 4))

    // The microstepping value is too high for the sine array
    #error MAX_MICROSTEP_DIVISOR is set too high, the sine array does not contain enough \
    values. Please reduce the maximum amount or add new values to the sine array
#endif


// Calculate the microstep interval count
#define MICROSTEP_INTERVAL_CNT (uint16_t)(log2(MAX_MICROSTEP_DIVISOR) - log2(MIN_MICROSTEP_DIVISOR) + 1)
