// Import the header file
#include "timers.h"

// Optimize for speed
#pragma GCC optimize ("-Ofast")

// Create a new timer instance
HardwareTimer *correctionTimer = new HardwareTimer(TIM1);

// If step correction is enabled (helps to prevent enabling the timer when it is already enabled)
bool stepCorrection = false;

// A counter for the number of position faults (used for stall detection)
uint16_t outOfPosCount = 0;

// The number of times the current blocks on the interrupts. All blocks must be cleared to allow the interrupts to start again
// A block count is needed for nested functions. This ensures that function 1 (cannot be interrupted) will not re-enable the
// interrupts before the uninterruptible function 2 that called the first function finishes.
static uint8_t interruptBlockCount = 0;

// Create a boolean to store if the StallFault pin has been enabled.
// Pin is only setup after the first StallFault. This prevents programming interruptions
#ifdef ENABLE_STALLFAULT
    bool stallFaultPinSetup = false;
#endif


// Setup everything related to the PID timer if needed
#ifdef ENABLE_PID
    // Create an instance of the PID class
    StepperPID pid = StepperPID();

    // Also create a timer that calls the motor stepping function based on the PID's output
    HardwareTimer *pidMoveTimer = new HardwareTimer(TIM4);

    // Variable to store if the timer is enabled
    // Saves large amounts of cycles as the timer only has to be toggled on a change
    bool pidMoveTimerEnabled = false;
#endif


// Tiny little function, just gets the time that the current program has been running
uint32_t sec() {
    return (millis() / 1000);
}

// Sets up the motor update timer
void setupMotorTimers() {

    // Interupts are in order of importance as follows -
    // - 0 - step pin change
    // - 1.0 - PID correctional movement
    // - 1.1 - position correction (or PID interval update)

    // Setup the step and stallfault pin
    pinMode(STEP_PIN, INPUT_PULLDOWN);

    // Check if StallFault is enabled
    #ifdef ENABLE_STALLFAULT

        // Make sure that StallFault is enabled
        #ifdef STALLFAULT_PIN
            //pinMode(STALLFAULT_PIN, OUTPUT);
        #endif

        // Reset the LED pin
        pinMode(LED_PIN, OUTPUT);
    #endif

    // Attach the interupt to the step pin (subpriority is set in platformio config file)
    // A normal step pin triggers on the rising edge. However, as explained here: https://github.com/CAP1Sup/Intellistep/pull/50#discussion_r663051004
    // the optocoupler inverts the signal. Therefore, the falling edge is the correct value.
    attachInterrupt(digitalPinToInterrupt(STEP_PIN), stepMotor, FALLING); // input is pull-upped to VDD

    // Setup the timer for steps
    correctionTimer -> pause();
    correctionTimer -> setInterruptPriority(1, 1);
    correctionTimer -> setMode(1, TIMER_OUTPUT_COMPARE); // Disables the output, since we only need the timed interrupt
    correctionTimer -> setOverflow(round(STEP_UPDATE_FREQ * motor.getMicrostepping()), HERTZ_FORMAT);
    #ifndef CHECK_STEPPING_RATE
        correctionTimer -> attachInterrupt(correctMotor);
    #endif
    correctionTimer -> refresh();
    correctionTimer -> resume();

    // Setup the PID timer if it is enabled
    #ifdef ENABLE_PID
        pidMoveTimer -> pause();
        pidMoveTimer -> setInterruptPriority(1, 0);
        pidMoveTimer -> setMode(1, TIMER_OUTPUT_COMPARE); // Disables the output, since we only need the timed interrupt
        pidMoveTimer -> attachInterrupt(stepMotorNoDesiredAngle);
        pidMoveTimer -> refresh();
        // Don't resume the timer here, it will be resumed when needed
    #endif
}


// Pauses the timers, essentially disabling them for the time being
void disableInterrupts() {

    // Disable the interrupts if this is the first block
    if (interruptBlockCount == 0) {
        __disable_irq();
    }

   // Add one to the interrupt block counter
    interruptBlockCount++;
}


// Resumes the timers, re-enabling them
void enableInterrupts() {

    // Remove one of the blocks on the interrupts
    interruptBlockCount--;

    // If all of the blocks are gone, then re-enable the interrupts
    if (interruptBlockCount == 0) {
        __enable_irq();
    }
}


// Enables the step correction timer
void enableStepCorrection() {

    // Enable the timer if it isn't already, then set the variable
    if (!stepCorrection) {
        correctionTimer -> resume();
        stepCorrection = true;
    }
}


// Disables the step correction timer
void disableStepCorrection() {

    // Disable the timer if it isn't already, then set the variable
    if (stepCorrection) {
        correctionTimer -> pause();

        // Disable the PID correction timer if needed
        #ifdef ENABLE_PID
            pidMoveTimer -> pause();
        #endif

        stepCorrection = false;
    }
}


// Set the speed of the step correction timer
void updateCorrectionTimer() {

    // Check the previous value of the timer, only changing if it is different
    if (correctionTimer -> getCount(HERTZ_FORMAT) != round(STEP_UPDATE_FREQ * motor.getMicrostepping())) {
        correctionTimer -> pause();
        correctionTimer -> setOverflow(round(STEP_UPDATE_FREQ * motor.getMicrostepping()), HERTZ_FORMAT);
        correctionTimer -> refresh();
        correctionTimer -> resume();
    }
}


// Just a simple stepping function. Interrupt functions can't be instance methods
void stepMotor() {
    #ifdef CHECK_STEPPING_RATE
        GPIO_WRITE(LED_PIN, HIGH);
    #endif
    motor.step();
    #ifdef CHECK_STEPPING_RATE
        GPIO_WRITE(LED_PIN, LOW);
    #endif
}


// Just like the simple stepping function above, except it doesn't update the desired position
void stepMotorNoDesiredAngle() {
    motor.step(PIN, false, false);
}


// Need to declare a function to power the motor coils for the step interrupt
void correctMotor() {

    // Check to see the state of the enable pin
    if ((GPIO_READ(ENABLE_PIN) != motor.getEnableInversion()) && (motor.getState() != FORCED_ENABLED)) {

        // The enable pin is off, the motor should be disabled
        motor.setState(DISABLED);

        // Only include if StallFault is enabled
        #ifdef ENABLE_STALLFAULT

            // Shut off the StallGuard pin just in case
            // (No need to check if the pin is valid, the pin will never be set up if it isn't valid)
            #ifdef STALLFAULT_PIN
                GPIO_WRITE(STALLFAULT_PIN, LOW);
            #endif

            // Fix the LED pin
            GPIO_WRITE(LED_PIN, LOW);
        #endif
    }
    else {

        // Enable the motor if it's not already (just energizes the coils to hold it in position)
        motor.setState(ENABLED);

        // Get the angular deviation
        int32_t stepDeviation = motor.getStepError();

        // Check to make sure that the motor is in range (it hasn't skipped steps)
        if (abs(stepDeviation) > 1) {


            // Run PID stepping if enabled
            #ifdef ENABLE_PID

                // Run the PID calcalations
                double pidOutput = pid.compute();

                // Set the motor timer to call the stepping routine at specified time intervals
                pidMoveTimer -> setOverflow((DEFAULT_PID_STEP_MAX - pidOutput), HERTZ_FORMAT);

                // Enable the timer if it isn't already
                if (!pidMoveTimerEnabled) {
                    pidMoveTimer -> resume();
                    pidMoveTimerEnabled = true;
                }


            #else // ! ENABLE_PID
                // Just "dumb" correction based on direction
                // Set the stepper to move in the correct direction
                if (/*motor.getStepPhase() != */ true) {
                    if (stepDeviation > 0) {

                        // Motor is at a position larger than the desired one
                        // Use the current angle to find the current step, then subtract 1
                        motor.step(CLOCKWISE, false, false);
                        //motor.driveCoils(round(getAbsoluteAngle() / (motor.getMicrostepAngle()) - (motor.getMicrostepping())));
                    }
                    else {
                        // Motor is at a position smaller than the desired one
                        // Use the current angle to find the current step, then add 1
                        motor.step(COUNTER_CLOCKWISE, false, false);
                        //motor.driveCoils(round(getAbsoluteAngle() / (motor.getMicrostepAngle()) + (motor.getMicrostepping())));
                    }
                }
            #endif // ! ENABLE_PID


            // Only use StallFault code if needed
            #ifdef ENABLE_STALLFAULT

                // Check to see if the out of position faults have exceeded the maximum amounts
                if (outOfPosCount > (STEP_FAULT_TIME * ((STEP_UPDATE_FREQ * motor.getMicrostepping()) - 1)) || abs(stepDeviation) > STEP_FAULT_STEP_COUNT) {

                    // Setup the StallFault pin if it isn't already
                    // We need to wait for a fault because otherwise the programmer will be unable to program the board
                    #ifdef STALLFAULT_PIN
                    if (!stallFaultPinSetup) {

                        // Setup the StallFault pin
                        LL_GPIO_InitTypeDef GPIO_InitStruct;
                        GPIO_InitStruct.Pin = STM_LL_GPIO_PIN(STALLFAULT_PIN);
                        GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
                        GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
                        GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
                        GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
                        LL_GPIO_Init(get_GPIO_Port(STM_PORT(STALLFAULT_PIN)), &GPIO_InitStruct);

                        // The StallFault pin is all set up
                        stallFaultPinSetup = true;
                    }

                    // The maximum count has been exceeded, trigger an endstop pulse
                    GPIO_WRITE(STALLFAULT_PIN, HIGH);
                    #endif

                    // Also give an indicator on the LED
                    GPIO_WRITE(LED_PIN, HIGH);
                }
                else {
                    // Just count up, motor is out of position but not out of faults
                    outOfPosCount++;
                }
            #endif
        }
        else { // Motor is in correct position

            // Disable the PID correction timer if PID is enabled
            #ifdef ENABLE_PID
                if (pidMoveTimerEnabled) {
                    pidMoveTimer -> pause();
                    pidMoveTimerEnabled = false;
                }
            #endif

            // Only if StallFault is enabled
            #ifdef ENABLE_STALLFAULT

            // Reset the out of position count and the StallFault pin
            outOfPosCount = 0;

            // Pull the StallFault low if it's setup
            // No need to check the validity of the pin here, it wouldn't be setup if it wasn't valid
            #ifdef STALLFAULT_PIN
            if (stallFaultPinSetup) {
                GPIO_WRITE(STALLFAULT_PIN, LOW);
            }
            #endif

            // Also toggle the LED for visual purposes
            GPIO_WRITE(LED_PIN, LOW);

            #endif // ! ENABLE_STALLFAULT
        }

    }
}
