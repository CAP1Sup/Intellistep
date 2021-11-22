#ifndef __TIMERS_H__
#define __TIMERS_H__

// Imports
#include "Arduino.h"
#include "config.h"
#include "main.h"
#include "led.h"
#include "pid.h"

// Variables
// Expose the StepperPID instance to other files
// (such as the flash for loading or saving parameters)
#ifdef ENABLE_PID
    extern StepperPID pid;
#endif

// Functions

// Tiny little function, just gets the time that the current program has been running
uint32_t sec();

// Sets up the motor timers and interrupts
void setupMotorTimers();

// Disables all motor timers (used when the motor cannot be messed with)
void disableMotorTimers();

// Enables the motor timers (used to reset the motor after the timers have been disabled)
void enableMotorTimers();

// Pauses the timers, temporarily disabling them
void disableInterrupts();

// Re-enables interrupts, resuming them
void enableInterrupts();

// Enables step correction
void enableStepCorrection();

// Disables step correction
void disableStepCorrection();

// Updates the step correction frequency (called when microstepping is changed)
void updateCorrectionTimer();

// Function that steps the motor
void stepMotor();

// Function that steps motor without adjusting the desired angle
void stepMotorNoDesiredAngle();

// Function to correct motor position if it is out of place
void correctMotor();

// Direct stepping
#ifdef ENABLE_FULL_MOTION_PLANNER
// Schedule steps for the motor to execute (rate is in Hz)
void scheduleSteps(int64_t count, int32_t rate, STEP_DIR stepDir);
#endif // ! ENABLE_FULL_MOTION_PLANNER

#if (defined(ENABLE_FULL_MOTION_PLANNER) || defined(ENABLE_PID))
// Step schedule handler (runs when the interrupt is triggered)
void stepScheduleHandler();

// Convenience function to handle enabling the step schedule timer
void enableStepScheduleTimer();

// Convenience function to handle disabling the step schedule timer
void disableStepScheduleTimer();
#endif // ! ENABLE_FULL_MOTION_PLANNER || ENABLE_PID

// Makes sure that all cached calls respect the current config
void syncInstructions();

#endif // ! __TIMERS_H__