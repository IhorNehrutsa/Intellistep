// Make sure that the motor header has only been used once
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "Arduino.h"
#include "HardwareTimer.h"
#include "encoder.h"
#include "fastAnalogWrite.h"

// For sin() and fmod() function
//#include "cmath"
//#include <math.h>
#include "fastSine.h"

// Import the pin mapping
#include "config.h"

// Enumeration for coil states
typedef enum {
    COIL_NOT_SET,
    FORWARD,
    BACKWARD,
    BRAKE,
    COAST
} COIL_STATE;

// Enumeration for stepping direction
typedef enum {
    PIN,
    COUNTER_CLOCKWISE,
    CLOCKWISE
} STEP_DIR;

// Enumeration for the enable state of the motor
typedef enum {
    MOTOR_NOT_SET,
    ENABLED,
    DISABLED,
    FORCED_ENABLED,
    FORCED_DISABLED

    #ifdef ENABLE_OVERTEMP_PROTECTION
    , OVERTEMP
    #endif
} MOTOR_STATE;

// Stepper motor class (defined to make life a bit easier when dealing with the motor)
class StepperMotor {

    // Everything is public, with the expection of some private variables
    public:

        // Initialize the motor
        StepperMotor();

        // Returns the current RPM of the encoder
        float getEncoderRPM();

        // Returns the current calculated RPM
        float getEstimRPM();

        #ifdef ENABLE_STEPPING_VELOCITY
            // Compute the stepping interface velocity in deg/s
            float getDegreesPS();

            // Compute the stepping interface RPM
            float getSteppingRPM();
        #endif

        // Returns the angular deviation of the motor from the set position
        float getAngleError();

        // Returns the step deviation of the motor from the set position
        int32_t getStepError();

        // Returns the current phase setting of the motor
        int32_t getStepPhase();

        // Returns the desired angle of the motor
        float getDesiredAngle();

        // Returns the desired step of the motor
        int32_t getDesiredStep();

        // Dynamic current
        #ifdef ENABLE_DYNAMIC_CURRENT

        // Gets the acceleration factor for dynamic current
        uint16_t getDynamicAccelCurrent() const;

        // Gets the idle factor for dynamic current
        uint16_t getDynamicIdleCurrent() const;

        // Gets the max current factor for dynamic current
        uint16_t getDynamicMaxCurrent() const;

        // Sets the acceleration factor for dynamic current
        void setDynamicAccelCurrent(uint16_t newAccelFactor);

        // Sets the idle factor for dynamic current
        void setDynamicIdleCurrent(uint16_t newIdleFactor);

        // Sets the max current factor for dynamic current
        void setDynamicMaxCurrent(uint16_t newMaxCurrent);

        #else // ! ENABLE_DYNAMIC_CURRENT

        // Gets the RMS current of the motor (in mA)
        uint16_t getRMSCurrent() const;

        // Gets the peak current of the motor
        uint16_t getPeakCurrent() const;

        // Sets the RMS current of the motor (in mA)(Peak is adjusted to match)
        void setRMSCurrent(uint16_t rmsCurrent);

        // Sets the peak current of the motor (in mA)(RMS is adjusted to match)
        void setPeakCurrent(uint16_t peakCurrent);
        #endif


        // Gets the microstepping mode of the motor
        uint16_t getMicrostepping() const;

        // Sets the microstepping mode of the motor
        void setMicrostepping(uint16_t setMicrostepping);

        // Sets the angle of a full step of the motor
        void setFullStepAngle(float newStepAngle);

        // Gets the full step angle of the motor
        float getFullStepAngle() const;

        // Get the microstepping angle of the motor. This is the full steps divided by the microsteps.
        // Used to speed up processing
        float getMicrostepAngle() const;

        // Get the microsteps per rotation of the motor
        int32_t getMicrostepsPerRotation() const;

        // Set if the motor should be reversed
        void setReversed(bool reversed);

        // Get if the motor direction is reversed
        bool getReversed() const;

        // Set if the motor enable pin should be inverted
        void setEnableInversion(bool inverted);

        // Get if the motor enable pin is inverted
        bool getEnableInversion() const;

        // Set the microstep multiplier
        void setMicrostepMultiplier(float newMultiplier);

        // Get the microstep multiplier
        float getMicrostepMultiplier() const;

        // Test
        void simpleStep();

        // Calculates the coil values for the motor and updates the set angle.
        void step(STEP_DIR dir = PIN, bool useMultiplier = true, bool updateDesiredPos = true);

        // Sets the coils to hold the motor at the desired step number
        void driveCoils(int32_t steps);

        // Sets the coils to hold the motor at the desired phase angle
        void driveCoilsAngle(float angle);

        // Sets the state of the A coil
        void setCoilA(COIL_STATE desiredState, uint16_t current = 0);

        // Sets the state of the B coil
        void setCoilB(COIL_STATE desiredState, uint16_t current = 0);

        // Calculates the correct PWM setting based on an input current
        uint32_t currentToPWM(uint16_t current) const;

        // Sets the current state of the motor
        void setState(MOTOR_STATE newState, bool clearErrors = false);

        // Get the current state of the motor
        MOTOR_STATE getState() const;

        // Computes the next speed of the motor
        float compute(float feedback);

        // Calibrates the encoder and PID loop
        void calibrate();

        // Encoder object
        Encoder encoder;

    // Things that shouldn't be accessed by the outside
    private:

        // Function for getting the sign of the number (returns -1 if number is less than 0, 1 if 0 or above)
        int32_t getSign(float num);

        // Keeps the desired angle of the motor
        float desiredAngle = 0;

        // Keeps the desired step of the motor
        int32_t desiredStep = 0;

        // Keeps the current angle of the motor
        float currentAngle = 0;

        // Keeps the current steps of the motor
        int32_t currentStep = 0;

        #ifdef ENABLE_STEPPING_VELOCITY
            // variables to calculate the stepping interface velocity
            float angleChange = 0.0;
            uint32_t prevStepingSampleTime = 0; // micros()
            uint32_t nowStepingSampleTime = 0; // micros()
            // isStepping == true mean that three variables above can be changed
            bool isStepping = false;
        #endif

        // Motor characteristics
        #ifdef ENABLE_DYNAMIC_CURRENT
            // Dynamic current settings
            uint16_t dynamicAccelCurrent = DYNAMIC_ACCEL_CURRENT;
            uint16_t dynamicIdleCurrent = DYNAMIC_IDLE_CURRENT;
            uint16_t dynamicMaxCurrent = DYNAMIC_MAX_CURRENT;
        #else
            // RMS Current (in mA)
            uint16_t rmsCurrent = (uint16_t)STATIC_RMS_CURRENT;
            // Peak Current (in mA)
            uint16_t peakCurrent = (rmsCurrent * 1.414);
        #endif

        // Microstepping divisor
        uint16_t microstepDivisor = 1;

        // Angle of a full step
        float fullStepAngle = 1.8;

        // Microstep angle (full step / microstepping divisor)
        float microstepAngle = 1.8;

        // Microstep count in a full rotation
        int32_t microstepsPerRotation = (360.0 / getFullStepAngle());

        // If the motor is enabled or not (saves time so that the enable and disable pins are only set once)
        MOTOR_STATE state = MOTOR_NOT_SET;

        // reversed is a multiplier for steps and angles
        // 1 - If the motor direction is normal
        // -1 - If the motor direction is inverted
        int8_t reversed = 1;

        // If the motor enable is inverted
        bool enableInverted = false;

        // Microstep multiplier (used to move a custom number of microsteps per step pulse)
        uint32_t microstepMultiplier = MICROSTEP_MULTIPLIER;

        // Analog info structures for PWM current pins
        analogInfo PWMCurrentPinInfoA;
        analogInfo PWMCurrentPinInfoB;

        // Last coil states (used to save time by not setting the pins unless necessary)
        COIL_STATE previousCoilStateA = COIL_NOT_SET;
        COIL_STATE previousCoilStateB = COIL_NOT_SET;
};

#endif