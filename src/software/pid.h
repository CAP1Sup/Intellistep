#ifndef __PID_H__
#define __PID_H__

// Include main config
#include "config.h"

// Only build this file if PID is enabled
#if (ENABLE_PID == true)

// Include Arduino library
#include "Arduino.h"

// Encoder library (for getting current position)
#include "encoder.h"

// Main (for stepper motor class)
#include "main.h"

// Main class for controlling the motor
// NOTE: This should be used for time increments between stepping
class StepperPID {

    // Public info (all of the functions to be used throughout the board)
    public:

        // Main constructor
        StepperPID();

        // Get functions for P, I, and D
        float getP() const;
        float getI() const;
        float getD() const;
        float getMaxI() const;

        // Set functions for P, I, and D
        void setP(float newP);
        void setI(float newI);
        void setD(float newD);
        void setMaxI(float newMaxI);

        // Gets the desired motor angle
        float getDesiredPosition();

        // Set the desired motor angle (this is in absolute postioning,
        // meaning that something like 1500 is a valid position)
        void setDesiredPosition(float angle);

        // Sets the min and max outputs of the PID loop
        void setOutputLimits(float min, float max);

        // Runs the PID calculations and returns the output
        float compute(float currentAbsAngle, float newSetpoint);

    // Private info (usually just variables)
    private:

        // Main variables for storing input, output, and setpoint
        float input = 0, output = 0, setpoint = 0;

        // P, I, and D terms for loop
        // Term is multiplied by degrees of error to find stepping rate back
        float kP = DEFAULT_P;
        float kI = DEFAULT_I;
        float kD = DEFAULT_D;

        // I windup clamping
        float maxI = DEFAULT_MAX_I;

        // Min and max caps
        float min = 0;
        float max = 0;

        // Time storage
        uint32_t currentTime;
        uint32_t previousTime = 0;
        float elapsedTime;

        // Intermediate calculation variables
        float error;
        float lastError = 0;
        float cumulativeError;
        float rateError;
};

#endif // ! ENABLE_PID

#endif // ! __PID_H__