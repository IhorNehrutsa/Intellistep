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
    this -> defaultRate = newRate;
}


// Get the default stepping rate for G6
int32_t Planner::getDefaultSteppingRate() {
    return (this -> defaultRate);
}
#endif // ! ENABLE_DIRECT_STEPPING

// Set the last step rate
void Planner::setLastStepRate(int32_t rate) {
    this -> lastRate = rate;
}

// Get the last step rate
int32_t Planner::getLastStepRate() {
    return (this -> lastRate);
}

#endif // ! ENABLE_FULL_MOTION_PLANNER