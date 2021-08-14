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

// If the steps should be counted using a hardware counter
//#define USE_HARDWARE_STEP_CNT
#define USE_SOFTWARE_STEP_CNT

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
#define DISABLE_CORRECTION_TIMER

// LED related debugging
#ifdef ENABLE_LED
    #define CHECK_STEPPING_RATE
    //#define CHECK_CORRECT_MOTOR_RATE
    //#define CHECK_ENCODER_SPEED
#endif

// Clock debugging (uses OLED pin)
#ifndef ENABLE_OLED
    // MCO is PA_8 pin, It also used as OLED_RST_PIN
    //#define CHECK_MCO_OUTPUT // Use an oscilloscope to measure frequency of HSI, HSE, SYSCLK or PLLCLK/2

    // The PA_8 pin is used
    //#define CHECK_GPIO_OUTPUT_SWITCHING // Use an oscilloscope to measure frequency of the GPIO PA_8 output switching
#endif

#endif // ! __CONFIG_ADV_H