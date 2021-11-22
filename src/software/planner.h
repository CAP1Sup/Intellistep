#ifndef _PLANNER_H__
#define _PLANNER_H__

// Include the config
#include "config.h"

// Only include if the full motion planner is enabled
#ifdef ENABLE_FULL_MOTION_PLANNER

// Enumeration for G code distance mode
typedef enum {
    ABSOLUTE = 90,
    INCREMENTAL = 91
} DISTANCE_MODE;


// Axis enumeration
typedef enum {
    X_AXIS = 'X', // main linear axes
    Y_AXIS = 'Y',
    Z_AXIS = 'Z',

    A_AXIS = 'A', // rotary axes
    B_AXIS = 'B',
    C_AXIS = 'C',

    U_AXIS = 'U', // additional axes
    V_AXIS = 'V',
    W_AXIS = 'W'
} AXES;


// Planner class for planning everything motion
class Planner {

    public:

        // Initializer
        Planner();

        // Set distance mode
        void setDistanceMode(DISTANCE_MODE newDisMode);

        // Get the distance mode
        DISTANCE_MODE getDistanceMode();

        #ifdef ENABLE_DIRECT_STEPPING
        // Set the default stepping rate for G6
        void setDefaultSteppingRate(int32_t newRate);

        // Get the default stepping rate for G6
        int32_t getDefaultSteppingRate();

        // Set the last step rate
        void setLastStepRate(int32_t rate);

        // Get the last step rate
        int32_t getLastStepRate();
        #endif // ! ENABLE_DIRECT_STEPPING

        // Set the last feed rate
        void setLastFeedRate(int32_t rate);

        // Get the last feed rate
        int32_t getLastFeedRate();

    private:
        // Keeps the current G code distance mode
        DISTANCE_MODE distanceMode = ABSOLUTE;

        #ifdef ENABLE_DIRECT_STEPPING
        // Keeps the default G code rate for G6 in Hz
        int32_t defaultStepRate = DEFAULT_STEPPING_RATE;

        // Keeps the last step rate for G6 in Hz
        int32_t lastStepRate = DEFAULT_STEPPING_RATE;
        #endif

        // Keeps the last rate used
        #ifdef ENABLE_DIRECT_STEPPING
        int32_t lastFeedRate = DEFAULT_STEPPING_RATE;
        #else
        int32_t lastFeedRate = 0;
        #endif
};

#endif // ! ENABLE_FULL_MOTION_PLANNER
#endif // ! __PLANNER_H__
