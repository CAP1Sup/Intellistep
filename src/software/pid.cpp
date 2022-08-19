// Include main PID header
#include "pid.h"

// Only compile this file if PID is enabled
#if (ENABLE_PID == true)

// Main constructor
StepperPID::StepperPID() {
    // Nothing to here
}


// Returns the Proportional value of the PID loop
float StepperPID::getP() const {
    return (this -> kP);
}


// Returns the Integral value fo the PID loop
float StepperPID::getI() const {
    return (this -> kI);
}


// Returns the Derivative value for the PID loop
float StepperPID::getD() const {
    return (this -> kD);
}


// Returns the maximum value of the I term of the PID loop
float StepperPID::getMaxI() const {
    return (this -> maxI);
}


// Sets the Proportional term of the PID loop
void StepperPID::setP(float newP) {

    // Update the term if the new value isn't negative
    if (newP >= 0) {
        kP = newP;
    }
}


// Sets the Integral term of the PID loop
void StepperPID::setI(float newI) {

    // Update the term if the new value isn't negative
    if (newI >= 0) {
        kI = newI;
    }
}


// Sets the Derivative of the PID loop
void StepperPID::setD(float newD) {

    // Update the term if the new value isn't negative
    if (newD >= 0) {
        kD = newD;
    }
}


// Sets the maximum value of the I term of the PID loop
void StepperPID::setMaxI(float newMaxI) {

    // Update the term if the new value isn't negative
    if (newMaxI >= 0) {
        maxI = newMaxI;
    }
}

// Get the desired position
float StepperPID::getDesiredPosition() {
    return (this -> setpoint);
}


// Set the desired position
void StepperPID::setDesiredPosition(float angle) {
    this -> setpoint = angle;
}


// Set the output limits of the loop
void StepperPID::setOutputLimits(float newMin, float newMax) {
    this -> min = newMin;
    this -> max = newMax;
}


// Update the PID loop, returning the output
float StepperPID::compute(float currentAbsAngle, float newSetpoint) {

    // Update the input
    this -> input = currentAbsAngle;

    // Update the setpoint
    this -> setpoint = newSetpoint;

    // Compute the PID
    // Update the current time
    this -> currentTime = millis();

    // Calculate the elapsed time
    this -> elapsedTime = (float)((this -> currentTime) - (this -> previousTime));

    // Calculate the error
    this -> error = (setpoint - input);

    // Calculate the cumulative error (used with I term)
    this -> cumulativeError += ((this -> error) * (this -> elapsedTime));

    // Clamp the cumulative error, preventing I term windup
    this -> cumulativeError = constrain(cumulativeError, -maxI, maxI);

    // Calculate the rate error
    this -> rateError = ((this -> error) - (this -> lastError)) / elapsedTime;

    // Calculate the output with the errors and the coefficients
    this -> output = ((this -> kP) * (this -> error)) + ((this -> kI) * (this -> cumulativeError)) + ((this -> kD) * (this -> rateError));

    // Update the last computation parameters
    this -> lastError = this -> error;
    this -> previousTime = this -> currentTime;

    // Return the output of the PID loop
    return constrain(this -> output, -DEFAULT_PID_STEP_MAX, DEFAULT_PID_STEP_MAX);
}

#endif