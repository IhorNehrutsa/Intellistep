// Import the header file
#include "timers.h"

// Optimize for speed
#pragma GCC optimize ("-Ofast")

// Timer uses:
// - TIM1 - Used to time correction calculations
// - TIM2 - Used to time PID steps
//// - TIM3 - Used to generate PWM signal for motor
// - TIM4 - Used to directly step the motor

#ifdef ENABLE_CORRECTION_TIMER
// Create a new timer instance
HardwareTimer *correctionTimer = new HardwareTimer(TIM1);
#endif

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
    HardwareTimer *pidMoveTimer = new HardwareTimer(TIM2);

    // Variable to store if the timer is enabled
    // Saves large amounts of cycles as the timer only has to be toggled on a change
    bool pidMoveTimerEnabled = false;

    // Direction of movement for PID steps
    STEP_DIR pidStepDirection = COUNTER_CLOCKWISE;
#endif


// Setup everything related to step scheduling
#ifdef ENABLE_DIRECT_STEPPING

    // Main timer for scheduling steps
    HardwareTimer *stepScheduleTimer = new HardwareTimer(TIM4);

    // Direction of movement for direct steps
    STEP_DIR scheduledStepDir = COUNTER_CLOCKWISE;

    // Remaining step count
    int64_t remainingScheduledSteps = 0;
#endif

// Tiny little function, just gets the time that the current program has been running
uint32_t sec() {
    return (millis() / 1000);
}

// Sets up the motor update timer
void setupMotorTimers() {

    // Interupts are in order of importance as follows -
    // - 6 - step pin change
    // - 7.0 - scheduled steps (if ENABLE_DIRECT_STEPPING)
    // - 7.1 - position correction (or PID interval update)
    // - 7.2 - PID correctional movement

    // Check if StallFault is enabled
    #ifdef ENABLE_STALLFAULT

        // Make sure that StallFault is enabled
        #ifdef STALLFAULT_PIN
            //pinMode(STALLFAULT_PIN, OUTPUT);
        #endif
    #endif

    // Attach the interupt to the step pin (subpriority is set in Platformio config file)
    // A normal step pin triggers on the rising edge. However, as explained here: https://github.com/CAP1Sup/Intellistep/pull/50#discussion_r663051004
    // the optocoupler inverts the signal. Therefore, the falling edge is the correct value.
    attachInterrupt(STEP_PIN, stepMotor, FALLING); // input is pull-upped to VDD

    #ifdef ENABLE_CORRECTION_TIMER
        // Setup the timer for steps
        correctionTimer -> pause();
        correctionTimer -> setInterruptPriority(7, 1);
        correctionTimer -> setMode(1, TIMER_OUTPUT_COMPARE); // Disables the output, since we only need the timed interrupt
        correctionTimer -> setOverflow(round(STEP_UPDATE_FREQ * motor.getMicrostepping()), HERTZ_FORMAT);
        #ifndef CHECK_STEPPING_RATE
            correctionTimer -> attachInterrupt(correctMotor);
        #endif
        correctionTimer -> refresh();
        correctionTimer -> resume();
    #endif

    // Setup the PID timer if it is enabled
    #ifdef ENABLE_PID
        pidMoveTimer -> pause();
        pidMoveTimer -> setInterruptPriority(7, 2);
        pidMoveTimer -> setMode(1, TIMER_OUTPUT_COMPARE); // Disables the output, since we only need the timed interrupt
        pidMoveTimer -> attachInterrupt(stepMotorNoDesiredAngle);
        pidMoveTimer -> refresh();
        pidMoveTimer -> pause();
        // Don't resume the timer here, it will be resumed when needed
    #endif

    // Setup step schedule timer if it is enabled
    #ifdef ENABLE_DIRECT_STEPPING
        stepScheduleTimer -> pause();
        stepScheduleTimer -> setInterruptPriority(7, 0);
        stepScheduleTimer -> setMode(1, TIMER_OUTPUT_COMPARE); // Disables the output, since we only need the timed interrupt
        stepScheduleTimer -> attachInterrupt(stepScheduleHandler);
        stepScheduleTimer -> refresh();
        stepScheduleTimer -> pause();
        // Don't re-enable the motor, that will be done when the steps are scheduled
    #endif
}


// Disables all of the motor timers (in case the motor cannot be messed with)
void disableMotorTimers() {

    // Detach the step interrupt
    detachInterrupt(STEP_PIN);

    // Disable the correctional timer
    #ifdef ENABLE_CORRECTION_TIMER
        correctionTimer -> pause();
    #endif

    // Disable the PID move timer if PID is enabled
    #ifdef ENABLE_PID
        pidMoveTimer -> pause();
    #endif

    // Disable the direct stepping timer if it is enabled
    #ifdef ENABLE_DIRECT_STEPPING
        stepScheduleTimer -> pause();
    #endif
}


// Enable all of the motor timers
void enableMotorTimers() {

    // Attach the step interrupt
    attachInterrupt(STEP_PIN, stepMotor, FALLING); // input is pull-upped to VDD

    // Enable the correctional timer
    #ifdef ENABLE_CORRECTION_TIMER
        correctionTimer -> resume();
    #endif

    // Enable the PID move timer if PID is enabled
    #ifdef ENABLE_PID
        //pidMoveTimer -> resume();
    #endif

    // Enable the direct stepping timer if it is enabled
    #ifdef ENABLE_DIRECT_STEPPING
        stepScheduleTimer -> resume();
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
        #ifdef ENABLE_CORRECTION_TIMER
            correctionTimer -> resume();
        #endif
        stepCorrection = true;
    }
}


// Disables the step correction timer
void disableStepCorrection() {

    // Disable the timer if it isn't already, then set the variable
    if (stepCorrection) {
        #ifdef ENABLE_CORRECTION_TIMER
            correctionTimer -> pause();
        #endif

        // Disable the PID correction timer if needed
        #ifdef ENABLE_PID
            pidMoveTimer -> pause();
        #endif

        stepCorrection = false;
    }
}


// Set the speed of the step correction timer
void updateCorrectionTimer() {

    #ifdef ENABLE_CORRECTION_TIMER
        // Check the previous value of the timer, only changing if it is different
        if (correctionTimer -> getCount(HERTZ_FORMAT) != round(STEP_UPDATE_FREQ * motor.getMicrostepping())) {
            correctionTimer -> pause();
            correctionTimer -> setOverflow(round(STEP_UPDATE_FREQ * motor.getMicrostepping()), HERTZ_FORMAT);
            correctionTimer -> refresh();
            correctionTimer -> resume();
        }
    #endif
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


#ifdef ENABLE_PID
    // Just like the simple stepping function above, except it doesn't update the desired position
    void stepMotorNoDesiredAngle() {
        motor.step(pidStepDirection, false, false);

        /*
        // FOC based correction
        if (pidStepDirection == COUNTER_CLOCKWISE) {
            motor.driveCoilsAngle(motor.encoder.getRawAngle() - motor.getMicrostepAngle() - motor.getFullStepAngle());
        }
        else if (pidStepDirection == CLOCKWISE) {
            motor.driveCoilsAngle(motor.encoder.getRawAngle() + motor.getMicrostepAngle() - motor.getFullStepAngle());
        }
        */


        /*
        // Main angle change (any inversions * angle of microstep)
        float angleChange = motor.getMicrostepAngle();
        int32_t stepChange = 1;

        //else if (dir == COUNTER_CLOCKWISE) {
            // Nothing to do here, the value is already positive
        //}
        if (pidStepDirection == CLOCKWISE) {
            // Make the angle change in the negative direction
            angleChange *= -1;
        }

        // Fix the step change's sign
        stepChange *= getSign(angleChange);

        // Motor's current angle must always be updated to correctly move the coils
        this -> currentAngle += angleChange;
        this -> currentStep += stepChange; // Only moving one step in the specified direction

        // Drive the coils to their destination
        motor.driveCoils(currentStep);
        */
    }
#endif


// Need to declare a function to power the motor coils for the step interrupt
void correctMotor() {
    #ifdef CHECK_CORRECT_MOTOR_RATE
        GPIO_WRITE(LED_PIN, HIGH);
    #endif

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
                int32_t pidOutput = round(pid.compute());
                uint32_t stepFreq = abs(pidOutput); //(DEFAULT_PID_STEP_MAX - abs(pidOutput));

                // Check if the value is 0 (meaning that the timer needs disabled)
                if (stepFreq == 0) {

                    // Check if the timer needs disabled
                    //if (pidMoveTimerEnabled) {
                        pidMoveTimer -> pause();
                        pidMoveTimerEnabled = false;
                    //}
                }
                else {
                    // Check if there's a movement threshold
                    #if (DEFAULT_PID_DISABLE_THRESHOLD > 0)

                        // Check to make sure that the movement threshold is exceeded, otherwise disable the motor
                        if (stepFreq > DEFAULT_PID_DISABLE_THRESHOLD) {
                            pidMoveTimer -> resume();
                            pidMoveTimer -> setOverflow(stepFreq, HERTZ_FORMAT);
                            motor.setState(ENABLED);
                        }
                        else {
                            pidMoveTimer -> pause();
                            motor.setState(DISABLED);
                        }
                    #else
                        // Set the motor timer to call the stepping routine at specified time intervals
                        pidMoveTimer -> setOverflow(stepFreq, HERTZ_FORMAT);
                    #endif


                    // Set the direction
                    if (pidOutput > 0) {
                        pidStepDirection = COUNTER_CLOCKWISE;
                    }
                    else {
                        pidStepDirection = CLOCKWISE;
                    }

                    // Enable the timer if it isn't already
                    //if (!pidMoveTimerEnabled) {
                        pidMoveTimer -> resume();
                        pidMoveTimerEnabled = true;
                    //}
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
                //if (pidMoveTimerEnabled) {
                    pidMoveTimer -> pause();
                    pidMoveTimerEnabled = false;
                //}
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
    #ifdef CHECK_CORRECT_MOTOR_RATE
        GPIO_WRITE(LED_PIN, LOW);
    #endif
}


// Direct stepping
#ifdef ENABLE_DIRECT_STEPPING
// Configure a specific number of steps to execute at a set rate (rate is in Hz)
void scheduleSteps(int64_t count, int32_t rate, STEP_DIR stepDir) {

    // Set the count and step direction
    remainingScheduledSteps = abs(count);
    scheduledStepDir = stepDir;

    // Configure the speed of the timer, then re-enable it
    stepScheduleTimer -> setOverflow(rate, HERTZ_FORMAT);
    stepScheduleTimer -> resume();
    stepScheduleTimer -> refresh();
}


// Handles a step schedule event
void stepScheduleHandler() {

    // Increment the motor in the correct direction
    motor.step(scheduledStepDir);

    // Increment the counter down (we completed a step)
    remainingScheduledSteps--;

    // Disable the timer if there are no remaining steps
    if (remainingScheduledSteps <= 0) {
        stepScheduleTimer -> pause();
    }
}
#endif // ! ENABLE_DIRECT_STEPPING