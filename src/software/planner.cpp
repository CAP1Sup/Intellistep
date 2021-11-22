// Include the main header file
#include "planner.h"

// Only include if FULL_MOTION_PLANNER is enabled
#ifdef ENABLE_FULL_MOTION_PLANNER

// Main constructor
Planner::Planner() {}

// Set distance mode
void Planner::setDistanceMode(DISTANCE_MODE newDisMode) {
    this -> distanceMode = newDisMode;
}


// Get the distance mode
DISTANCE_MODE Planner::getDistanceMode() {
    return (this -> distanceMode);
}


// Default rates are only needed with DIRECT_STEPPING
#ifdef ENABLE_DIRECT_STEPPING
// Set the default stepping rate for G6
void Planner::setDefaultSteppingRate(int32_t newRate) {
    this -> defaultStepRate = newRate;
}


// Get the default stepping rate for G6
int32_t Planner::getDefaultSteppingRate() {
    return (this -> defaultStepRate);
}

// Set the last step rate
void Planner::setLastStepRate(int32_t rate) {
    this -> lastStepRate = rate;
}

// Get the last step rate
int32_t Planner::getLastStepRate() {
    return (this -> lastStepRate);
}
#endif // ! ENABLE_DIRECT_STEPPING

// Set the last feed rate
void Planner::setLastFeedRate(int32_t rate) {
    this -> lastFeedRate = rate;
}

// Get the last feed rate
int32_t Planner::getLastFeedRate() {
    return (this -> lastFeedRate);
}

#endif // ! ENABLE_FULL_MOTION_PLANNER