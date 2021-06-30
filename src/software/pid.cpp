// Include main PID header
#include "pid.h"

// Only compile this file if PID is enabled
#ifdef ENABLE_PID

// Main constructor
StepperPID::StepperPID() {

    // Setup PID library
    this -> pid = new PID(&input, &output, &setpoint, DEFAULT_P, DEFAULT_I, DEFAULT_D, DIRECT);

    // Set that the PID timings should be handled automatically
    this -> pid -> SetMode(AUTOMATIC);

    // Set the minimum and maximum outputs of the PID controller
    this -> pid -> SetOutputLimits(DEFAULT_PID_STEP_MIN, DEFAULT_PID_STEP_MAX);
}


// Returns the Proportional value of the PID loop
double StepperPID::getP() const {
    return (this -> pid -> GetKp());
}


// Returns the Integral value fo the PID loop
double StepperPID::getI() const {
    return (this -> pid -> GetKi());
}


// Returns the Derivative value for the PID loop
double StepperPID::getD() const {
    return (this -> pid -> GetKd());
}


// Sets the Proportional term of the PID loop
void StepperPID::setP(double newP) {

    // Update the local reference
    P = newP;

    // Update the overall tunings
    updateTunings();
}


// Sets the Integral term of the PID loop
void StepperPID::setI(double newI) {

    // Update the local reference
    I = newI;

    // Update the overall tunings
    updateTunings();
}


// Sets the Derivative of the PID loop
void StepperPID::setD(double newD) {
    
    // Update the local reference
    D = newD;

    // Update the overall tunings
    updateTunings();
}


// Updates the tunings of the PID loop with the variable values
void StepperPID::updateTunings() {
    this -> pid -> SetTunings(P, I, D);
}


// Get the desired position
double StepperPID::getDesiredPosition() {
    return (this -> setpoint);
}


// Set the desired position
void StepperPID::setDesiredPosition(double angle) {
    this -> setpoint = angle;
}


// Set the output limits of the loop
void StepperPID::setOutputLimits(double min, double max) {
    this -> pid -> SetOutputLimits(min, max);
}


// Update the PID loop, returning the output
double StepperPID::compute() {

    // Update the input
    this -> input = motor.getAngleError();

    // Compute the PID
    this -> pid -> Compute();

    // Return the output
    return output;
}

#endif