#ifndef __CONFIG_ADV_H
#define __CONFIG_ADV_H

// Include the Arduino library
#include "Arduino.h"

// Motor:
// The maximum number of steps per rev is 200*32==6400 (for 1.8 degree per step motor)
//                                    and 400*32==12800 (for 0.9 degree per step motor)
// int32_t allow to store +/- 2^31==2147483648 steps and 2^31/6400==335544 rev's (for 1.8 degree per step motor)
//                                                       2^31/12800==167772 rev's (for 0.9 degree per step motor)
// 335544 rev's / 1000RPM ~ 336 min == 5.6 hour in one direction for counter overflow
// Encoder:
// The maximum number of steps per rev is 2^15==32768
// int32_t allow to store +/- (2^31)/(2^15)==2^16==65536 rev's
typedef int32_t increments_t;

// A type for real numbers
typedef float real_t;

// Reject unstable least significant bits of the encoder
#define REJECT_ENCODERS_LSB 2

// Specify the configuration for the step counting
// ! ONLY ONE CAN BE USED AT A TIME!
//#define USE_LEGACY_STEP_CNT_SETUP
//#define USE_MKS_STEP_CNT_SETUP

// The time (in ms) that an IO loop should take
// An IO loop updates dip switches, checks serial, and updates the OLED display
#define MIN_IO_LOOP_TIME (uint32_t)50

// The time (in ms) to let the motor settle for before reading it
// This is typically used when enabling the motor for the first time and waiting for it to steady
// before reseting the encoder to zero
#define MOTOR_SETTLE_TIME (uint32_t)500

// Board characteristics
// ! Do not modify unless you know what you are doing!
#define BOARD_VOLTAGE              (float)3.3 // The voltage of the main processor
#define CURRENT_SENSE_RESISTOR     (float)0.2 // Value of the board's current calculation resistor. An incorrect value here will cause current inaccuracies
#define MAX_PEAK_BOARD_CURRENT  (uint16_t)3500 // Maximum peak current in mA that the board can manage

// https://deepbluembedded.com/wp-content/uploads/2020/06/STM32-PWM-Resolution-Example-STM32-Timer-PWM-Mode-Output-Compare-768x291.jpg
// 124000 in fact 139.5kHz and 9bit resolution
// 62000 in fact 69.8kHz and 10bit resolution
// Tested: higher resolution causes more noise when the motor is stopped after moving.
//
// Note: The measured internal PWM frequency of the A4950(Pins 7, 2 and 3) is 22.5 kHz.
// Noise is a design feature when the A4950 drives a stepper motor. :(
#define MOTOR_PWM_FREQ          (uint32_t)124000 // in Hz

// The System Clock frequency of the CPU (in MHz)
// This can be set to 72 and 128 with SYSCLK_SRC_HSE_8 (external oscillator)
// Can be set to 72 with SYSCLK_SRC_HSE_16 (external oscillator)
// Can be set to 64 with SYSCLK_SRC_HSI (internal oscillator)
#define SYSCLK_FREQ 72
#define SYSCLK_SRC_HSE_8

// The compare format and maximum value for PWM (lower values = higher max freq)
#define PWM_COMPARE_FORMAT RESOLUTION_9B_COMPARE_FORMAT
#define PWM_MAX_VALUE (POWER_2(PWM_COMPARE_FORMAT) - 1)

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

// Parser settings
#define STRING_START_MARKER '<'
#define STRING_END_MARKER '>'

// Future features... need to be tested/fixed
// Stallfault
//#define ENABLE_STALLFAULT
#ifdef ENABLE_STALLFAULT
    #define STEP_FAULT_TIME         1 // The maximum allowable time (sec) for a step fault (meaning motor is out of position)
    #define STEP_FAULT_STEP_COUNT   10 // The maximum allowable deviation between the actual and set steps before StallFault is triggered

    // StallFault connection (to mainboard)
    // Pulls high on a stepper misalignment after the set period or angular deviation
    #define STALLFAULT_PIN PA_13 //output(GPIOA_BASE_BASE, 13)
#endif

// Direct step functionality (used to command motor to move over Serial/CAN)
//#define ENABLE_DIRECT_STEPPING
#ifdef ENABLE_DIRECT_STEPPING

    // The default stepping rate (in Hz) to move in the event that no parameter is specified
    #define DEFAULT_STEPPING_RATE 1000
#endif

// Dynamic current (adjusts motor current based on acceleration (and therefore torque)
// required from the motor)
#define ENABLE_DYNAMIC_CURRENT  false
#if (ENABLE_DYNAMIC_CURRENT != false )
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

// If the motor should try to correct after it thinks that steps have been missed
// ! THIS IS UNLIKELY TO WORK, AND WILL PROBABLY JUST MESS EVERYTHING UP, DON"T ENABLE IT
//#define STEP_CORRECTION
    // Uses advanced PID control instead of correcting based on error direction
    // ! At this time, this feature is still under development
#define ENABLE_PID false
#ifdef STEP_CORRECTION


    #if (ENABLE_PID == true)
        // PID settings

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
#endif

// CAN settings
// ! WARNING: For the time being, CAN must be enabled
// ! Disabling it will result in the encoder not working
// ! YOU HAVE BEEN WARNED!
#define ENABLE_CAN 0
#if (ENABLE_CAN != 0)
    // The CAN ID of this board
    // X:2, X2:3...
    // Y:7, Y2:8...
    // Z:11 Z2:12...
    // E:17, E1:18...
    #define DEFAULT_CAN_ID X

    // ! Maybe higher later? (Up to 1MHz for fast transmissions)
    #define CAN_BITRATE BR125K
#endif


// --------------  Internal defines  --------------
// Under the hood motor setup
#define SINE_VAL_COUNT (128)
#define SINE_MAX (16384) // 2^SINE_POWER == 2^14 == 16384

// Use a integer version of the log of SINE_MAX
// Used to speed up division much faster
#define SINE_POWER ((uint16_t)log2(SINE_MAX))

// Methods for writing to the GPIO
// When SystemClock_Config_HSE_8M_SYSCLK_72M() is selected:
// F         | GPIO_WRITE_METHOD                 | based on
//  14,4 kHz | GPIO_WRITE_DIGITAL_WRITE          | digitalWrite()
//  81.4 kHz | GPIO_WRITE_DIGITAL_WRITE_FAST     | digitalWriteFast()
//  1.64 MHz | GPIO_WRITE_DIGITAL_WRITE_FASTER   | digitalWriteFaster()
//  1.67 MHz | GPIO_WRITE_DIGITAL_WRITE_FASTEST  | digitalWriteFastest()
// 600.1 kHz | GPIO_WRITE_REGISTER_SET           | output()
// 163.3 kHz | GPIO_WRITE_HAL_FUNCTION           | HAL_GPIO_WritePin()

// Setting for the GPIO write method
#define GPIO_WRITE_METHOD GPIO_WRITE_DIGITAL_WRITE_FASTEST

// Define the correct GPIO_WRITE method
#if (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE)
    #define GPIO_WRITE(pn, high) digitalWrite(pinNametoDigitalPin(pn), high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FAST)
    #define GPIO_WRITE(pn, high) digitalWriteFast(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FASTER)
    #define GPIO_WRITE(pn, high) digitalWriteFaster(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FASTEST)
    #define GPIO_WRITE(pn, high) digitalWriteFastest(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_REGISTER_SET)
    // Pure bitsetting
    #define GPIO_WRITE(pn, high) \
        if (high) { \
            output((uint32_t)(get_GPIO_Port(STM_PORT(pn))), STM_PIN(pn)) = 1; \
        } \
        else { \
            output((uint32_t)(get_GPIO_Port(STM_PORT(pn))), STM_PIN(pn)) = 0; \
        }

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_HAL_FUNCTION)
    // covered by HAL functions
    #define GPIO_WRITE(pn, high) \
        if (high) { \
            HAL_GPIO_WritePin(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn), GPIO_PIN_SET); \
        } \
        else { \
            HAL_GPIO_WritePin(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn), GPIO_PIN_RESET); \
        }

#else
    #error GPIO_WRITE_METHOD not a valid value
#endif

//#define GPIO_READ(pn) digitalReadFast(pn)
//#define GPIO_READ(pn) digital_io_read(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn))
#define GPIO_READ(pn) LL_GPIO_IsInputPinSet(get_GPIO_Port(STM_PORT(pn)), STM_LL_GPIO_PIN(pn))

// --------------  Debugging  --------------

//#define ENABLE_STEPPING_VELOCITY
//#define IGNORE_FLASH_VERSION
#ifndef STEP_CORRECTION
    #define DISABLE_CORRECTION_TIMER
#endif
#define ENABLE_ENCODER true

// LED related debugging
#ifdef ENABLE_LED
    //#define CHECK_STEPPING_RATE
    //#define CHECK_CORRECT_MOTOR_RATE
    //#define CHECK_ENCODER_SPEED
#endif

// Clock debugging (uses OLED pin)
#ifdef ENABLE_OLED
    // Displays the step counter instead of the RPM on the first line
    // Likely more useful than the current RPM
    #define SHOW_STEP_CNT_INSTEAD_OF_RPM

    // Displays the unhandled step count instead of the angle error on the first line
    // Likely more useful than the angle error in some cases
    #define SHOW_UNHANDLED_STEP_CNT_INSTEAD_OF_ANGLE_ERR

    // The time (in ms) to display the power up screen (shows if the board is calibrated) for
    // Keep in mind that this should be larger than the MOTOR_SETTLE_TIME in order to be respected
    #define POWERUP_DISPLAY_TIME (uint32_t)1000

    // The time (in ms) to display the calibration messages screen for before scrolling
    #define CALIBRATION_DISPLAY_TIME (uint32_t)3000
#else
    // MCO is PA_8 pin, It also used as OLED_RST_PIN
    //#define CHECK_MCO_OUTPUT // Use an oscilloscope to measure frequency of HSI, HSE, SYSCLK or PLLCLK/2

    // The PA_8 pin is used
    //#define CHECK_GPIO_OUTPUT_SWITCHING // Use an oscilloscope to measure frequency of the GPIO PA_8 output switching
#endif

// Interrupt priority definitions
// Interupts are in order of importance as follows -
// - 5 - hardware step counter overflow handling
// - 6.0 - enable pin change
// - 6.1 - step pin change
// - 7.0 - position correction (or PID interval update)
// - 7.1 - scheduled steps (if ENABLE_DIRECT_STEPPING or ENABLE_PID)
#define TIM2_OVERFLOW_PREMPT_PRIOR 5
#define TIM2_OVERFLOW_SUB_PRIOR 0

#define EN_PIN_PREMPT_PRIOR 6
#define EN_PIN_SUB_PRIOR 0
#define DIR_PIN_PREMPT_PRIOR 6
#define DIR_PIN_SUB_PRIOR 1
#define STEP_PIN_PREMPT_PRIOR 6
#define STEP_PIN_SUB_PRIOR 2

#define POS_CORRECTION_PREMPT_PRIOR 7
#define POS_CORRECTION_SUB_PRIOR 0
#define SCHED_STEPS_PREMPT_PRIOR 7
#define SCHED_STEPS_SUB_PRIOR 1

#endif // ! __CONFIG_ADV_H