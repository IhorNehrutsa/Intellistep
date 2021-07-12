#ifndef __CONFIG_H
#define __CONFIG_H

// Need the Arduino library for pin conversion
#include "Arduino.h"

// Version of the firmware (displayed on OLED) (follows semantic versioning)
#define MAJOR_VERSION (uint16_t)0
#define MINOR_VERSION (uint16_t)0
#define PATCH_VERSION (uint16_t)42

//#define ENABLE_CORRECTION_TIMER

// --------------  Settings  --------------

// LED light functionality
#define ENABLE_LED // red LED labeled as an 'error' in the schema
#ifdef ENABLE_LED
    //#define ENABLE_BLINK
    #define CHECK_STEPPING_RATE
    //#define CHECK_CORRECT_MOTOR_RATE
    //#define CHECK_ENCODER_SPEED

    #if defined(ENABLE_BLINK) && defined(CHECK_STEPPING_RATE) && defined(CHECK_CORRECT_MOTOR_RATE) && defined(CHECK_ENCODER_SPEED)
        #error Only one of #define is possible at a time in this section
    #endif
#endif

#define ENABLE_OLED
#if !defined(ENABLE_OLED)
    // MCO is PA_8 pin, It also used as OLED_RST_PIN
    //#define CHECK_MCO_OUTPUT // Use an oscilloscope to measure frequency of HSI, HSE, SYSCLK or PLLCLK/2

    // The PA_8 pin is used
    //#define CHECK_GPIO_OUTPUT_SWITCHING // Use an oscilloscope to measure frequency of the GPIO PA_8 output switching

    #if defined(CHECK_MCO_OUTPUT) && defined(CHECK_GPIO_OUTPUT_SWITCHING)
        #error Only one of #define is possible at a time in this section
    #endif
#endif

// Averages (number of readings in average)
#define RPM_AVG_READINGS     (uint16_t)200
#define ACCEL_AVG_READINGS   (uint16_t)10
#define ANGLE_AVG_READINGS   (uint16_t)15
#define TEMP_AVG_READINGS    (uint16_t)200

// If encoder estimation should be used
#define ENCODER_SPEED_ESTIMATION
#ifdef ENCODER_SPEED_ESTIMATION
    #define SPD_EST_MIN_INTERVAL 500 // The minimum sampling interval (us). Increase to get more steady readings at the cost of latency
#endif

// Serial configuration settings
#define ENABLE_SERIAL
#ifdef ENABLE_SERIAL
    #define SERIAL_BAUD 115200
#endif

// Parser settings
#define STRING_START_MARKER '<'
#define STRING_END_MARKER '>'

// CAN settings
#define ENABLE_CAN
#ifdef ENABLE_CAN
    // The CAN ID of this board
    // X:2, X2:3...
    // Y:7, Y2:8...
    // Z:11 Z2:12...
    // E:17, E1:18...
    #define DEFAULT_CAN_ID X

    // ! Maybe higher later? (Up to 1MHz for fast transmissions)
    #define CAN_BITRATE BR125K
#endif

// Motor characteristics
#define STEP_ANGLE (float)1.8 // ! Check to see for .9 deg motors as well
#define STEP_UPDATE_FREQ 78 // in Hz, to step the motor back to the correct position. Multiplied by the microstepping for actual update freq

// Board characteristics
// ! Do not modify unless you know what you are doing!
#define BOARD_VOLTAGE              (float)3.3 // The voltage of the main processor
#define CURRENT_SENSE_RESISTOR     (float)0.2 // Value of the board's current calculation resistor. An incorrect value here will cause current inaccuracies
#define MAX_PEAK_BOARD_CURRENT  (uint16_t)3500 // Maximum peak current in mA that the board can manage

// Dynamic current (adjusts motor current based on acceleration (and therefore torque)
// required from the motor)
//#define ENABLE_DYNAMIC_CURRENT
#ifdef ENABLE_DYNAMIC_CURRENT
    // A dynamically controller current loop. Uses the equation: accel * accelCurrent + idleCurrent
    // Limited by the max dynamic current, which will limit the maximum that the dynamic loop can output
    // All current values are in RMS
    #define DYNAMIC_ACCEL_CURRENT 10 // Multiplied by deg/s/s, in mA
    #define DYNAMIC_IDLE_CURRENT  500 // In mA
    #define DYNAMIC_MAX_CURRENT   750 // In mA
#else
    // Classic, static current
    #define STATIC_RMS_CURRENT     (uint16_t)500 // This is the rating of the motor from the manufacturer

    // Overtemperature protection (lowers motor current when motor temperature rises too high)
    //#define ENABLE_OVERTEMP_PROTECTION
    #ifdef ENABLE_OVERTEMP_PROTECTION
        #define OVERTEMP_THRESHOLD_TEMP      70 // The temp to trigger a overtemp current reduction (C)
        #define OVERTEMP_INCREMENT           50 // The increment at which to reduce the current by (RMS mA)
        #define OVERTEMP_INTERVAL            30 // The minimum interval between current reductions (s)
        #define OVERTEMP_SHUTDOWN_TEMP       80 // The temp at which to completely shut down the motor, protecting it against burning up
        #define OVERTEMP_SHUTDOWN_CLEAR_TEMP 70 // Motor can begin movement again once this temp is reached
    #endif
#endif

// PID settings
// ! At this time this feature is still under development
//#define ENABLE_PID
#ifdef ENABLE_PID

    // Default P, I, and D terms
    #define DEFAULT_P  1000
    #define DEFAULT_I  2
    #define DEFAULT_D  0

    // I windup protection
    #define DEFAULT_MAX_I 10

    // Minimum sample time (in ms)
    //#define PID_MIN_SAMPLE_TIME 10

    // Default min and max for step timing (per second)
    #define DEFAULT_PID_STEP_MAX 50000

    // PID output that the motor should disable at (set to 0 to never disable motor)
    #define DEFAULT_PID_DISABLE_THRESHOLD 0 //1000
#endif

// Direct step functionality (used to command motor to move over Serial/CAN)
//#define ENABLE_DIRECT_STEPPING
#ifdef ENABLE_DIRECT_STEPPING

    // The default stepping rate (in Hz) to move in the event that no parameter is specified
    #define DEFAULT_STEPPING_RATE 1000
#endif

// Motor settings
// The number of microsteps to move per step pulse
// Doesn't affect correctional movements
#define MICROSTEP_MULTIPLIER    (uint32_t)1

// The min/max microstepping divisors
// Microstepping divisors are the numbers underneath the fraction of the microstepping
// For example, 1/16th microstepping would have a divisor of 16
#define MIN_MICROSTEP_DIVISOR   (uint8_t)1
#define MAX_MICROSTEP_DIVISOR   (uint8_t)32

#define MOTOR_PWM_FREQ          (uint32_t)124000 // in Hz
// https://deepbluembedded.com/wp-content/uploads/2020/06/STM32-PWM-Resolution-Example-STM32-Timer-PWM-Mode-Output-Compare-768x291.jpg
// 124000 in fact 139.5kHz and 9bit resolution
// 62000 in fact 69.8kHz and 10bit resolution
// Tested: higher resolution causes more noise when the motor is stopped after moving.
//
// Note: The measured internal PWM frequency of the A4950(Pins 7, 2 and 3) is 22.5 kHz.
// Noise is a design feature when the A4950 drives a stepper motor. :(

#define IDLE_MODE               COAST // The mode to set the motor to when it's disabled

// Stallfault
//#define ENABLE_STALLFAULT
#ifdef ENABLE_STALLFAULT
    #define STEP_FAULT_TIME         1 // The maximum allowable time (sec) for a step fault (meaning motor is out of position)
    #define STEP_FAULT_STEP_COUNT   10 // The maximum allowable deviation between the actual and set steps before StallFault is triggered

    // StallFault connection (to mainboard)
    // Pulls high on a stepper misalignment after the set period or angular deviation
    #define STALLFAULT_PIN PA_13 //output(GPIOA_BASE_BASE, 13)
#endif

// OLED settings
#ifdef ENABLE_OLED

    // Button settings
    //#define INVERTED_DIPS // Enable if your dips are inverted ("on" print is facing away from motor connector)
    #define BUTTON_REPEAT_INTERVAL 250 // Millis
    #define MENU_RETURN_LEVEL MOTOR_DATA // The level to return to after configuring a setting
    #define WARNING_MICROSTEP MAX_MICROSTEP_DIVISOR // The largest microstep to warn on (the denominator of the fraction)

    // Warning thresholds
    #define WARNING_RMS_CURRENT 1000 // The RMS current at which to display a warning confirmation (mA)
    //#define WARNING_PEAK_CURRENT 1000 // The peak current at which to display a warning confirmation (mA)
#endif

// The System Clock frequency of the CPU (in MHz)
// This can be set to 72 and 128 with SYSCLK_SRC_HSE_8 (external oscillator)
// Can be set to 72 with SYSCLK_SRC_HSE_16 (external oscillator)
// Can be set to 64 with SYSCLK_SRC_HSI (internal oscillator)
#define SYSCLK_FREQ 72
#define SYSCLK_SRC_HSE_8

// The compare format and maximum value for PWM (lower values = higher max freq)
#define PWM_COMPARE_FORMAT RESOLUTION_9B_COMPARE_FORMAT
#define PWM_MAX_VALUE (pow(2, (uint8_t)PWM_COMPARE_FORMAT) - 1)

// --------------  Pins  --------------
/*
// * = F103C8-CB    | DIGITAL | ANALOG | USART      | TWI       | SPI        | SPECIAL   |
//                  |---------|--------|------------|-----------|------------|-----------|
#define PA0  A0  // | 0       | A0     |            |           |            |           |
#define PA1  A1  // | 1       | A1     |            |           |            |           |
#define PA2  A2  // | 2       | A2     | USART2_TX  |           |            |           |
#define PA3  A3  // | 2       | A2     | USART2_RX  |           |            |           |
#define PA4  A4  // | 4       | A4     |            |           | SPI1_SS    |           |
#define PA5  A5  // | 5       | A5     |            |           | SPI1_SCK   |           |
#define PA6  A6  // | 6       | A6     |            |           | SPI1_MISO  |           |
#define PA7  A7  // | 7       | A7     |            |           | SPI1_MOSI  |           |
#define PA8  8   // | 8       |        |            |           |            |           |
#define PA9  9   // | 9       |        | USART1_TX  |           |            |           |
#define PA10 10  // | 10      |        | USART1_RX  |           |            |           |
#define PA11 11  // | 11      |        |            |           |            | USB_DN    | CAN_IN
#define PA12 12  // | 12      |        |            |           |            | USB_DP    | CAN_OUT
#define PA13 13  // | 13      |        |            |           |            | SWD_SWDIO |
#define PA14 14  // | 14      |        |            |           |            | SWD_SWCLK |
#define PA15 15  // | 15      |        |            |           | SPI1_SS    |           |
//                  |---------|--------|------------|-----------|------------|-----------|
#define PB0  A8  // | 16      | A8     |            |           |            |           |
#define PB1  A9  // | 17      | A9     |            |           |            |           |
#define PB2  18  // | 18      |        |            |           |            | BOOT1     |
#define PB3  19  // | 19      |        |            |           | SPI1_SCK   |           |
#define PB4  20  // | 20      |        |            |           | SPI1_MISO  |           |
#define PB5  21  // | 21      |        |            |           | SPI1_MOSI  |           |
#define PB6  22  // | 22      |        | USART1_TX  | TWI1_SCL  |            |           |
#define PB7  23  // | 23      |        | USART1_RX  | TWI1_SDA  |            |           |
#define PB8  24  // | 24      |        |            | TWI1_SCL  |            |           |
#define PB9  25  // | 25      |        |            | TWI1_SDA  |            |           |
#define PB10 26  // | 26      |        | USART3_TX* | TWI2_SCL* |            |           |
#define PB11 27  // | 27      |        | USART3_RX* | TWI2_SDA* |            |           |
#define PB12 28  // | 28      |        |            |           | SPI2_SS*   |           |
#define PB13 29  // | 29      |        |            |           | SPI2_SCK*  |           |
#define PB14 30  // | 30      |        |            |           | SPI2_MISO* |           |
#define PB15 31  // | 31      |        |            |           | SPI2_MOSI* |           |
//                  |---------|--------|------------|-----------|------------|-----------|
#define PC13 32  // | 32      |        |            |           |            |           |
#define PC14 33  // | 33      |        |            |           |            | OSC32_IN  |
#define PC15 34  // | 34      |        |            |           |            | OSC32_OUT |
//                  |---------|--------|------------|-----------|------------|-----------|
#define PD0  35  // | 35      |        |            |           |            | OSC_IN    |
#define PD1  36  // | 36      |        |            |           |            | OSC_OUT   |
//                  |---------|--------|------------|-----------|------------|-----------|
*/


// OLED Mappings
#define OLED_CS_PIN   PB_12
#define OLED_RST_PIN  PA_8
#define OLED_RS_PIN   PB_13
#define OLED_SCLK_PIN PB_15	//(D0)
#define OLED_SDIN_PIN PB_14	//(D1)

// Button mappings
#define DOWN_BUTTON_PIN         PA_3
#define BACK_BUTTON_PIN         PB_0
#define SELECT_BUTTON_PIN       PB_1

// Dip switch mappings
// PIN     |    Normal orientation      |  Reversed orientation
// DIP_1   |    Microstep 1             |  Calibration mode
// DIP_2   |    Microstep 2             |  Closed/Open loop
// DIP_3   |    Closed/Open loop        |  Microstep 2
// DIP_4   |    Calibration mode        |  Microstep 1
#define DIP_1_PIN  PA_15
#define DIP_2_PIN  PB_3
#define DIP_3_PIN  PB_11
#define DIP_4_PIN  PB_10

// LED pin
#define LED_PIN PC_13

// Motor mappings
#define COIL_A_POWER_OUTPUT_PIN  PB_5
#define COIL_B_POWER_OUTPUT_PIN  PB_4
#define COIL_A_DIR_1_PIN  PB_6
#define COIL_A_DIR_2_PIN  PB_7
#define COIL_B_DIR_1_PIN  PB_8
#define COIL_B_DIR_2_PIN  PB_9

// Encoder SPI interface
#define ENCODER_CS_PIN    PA_4 // SPI1_SS
#define ENCODER_SCK_PIN   PA_5 // SPI1_SCK
#define ENCODER_MISO_PIN  PA_6 // SPI1_MISO
#define ENCODER_MOSI_PIN  PA_7 // SPI1_MOSI

// Stepping interface
#define STEP_PIN       PA_0
#define ENABLE_PIN     PA_2
#define DIRECTION_PIN  PA_1

// CAN bus pins
#define CAN_IN_PIN   PA_11
#define CAN_OUT_PIN  PA_12

// UART pins
#define USART1_TX PA9
#define USART1_RX PA10

// --------------  Internal defines  --------------
// Under the hood motor setup
#define SINE_VAL_COUNT (128)
//#define SINE_MAX ((int16_t)(10000))
#define SINE_MAX (16384) // 2^SINE_POWER == 2^14 == 16384

// Use a integer version of the log of SINE_MAX
// Used to speed up division much faster
#define SINE_POWER ((uint16_t)log2(SINE_MAX))

// Bitwise memory modification - ARM bitband
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

// Low level GPIO configuration (for quicker manipulations than digitalWrites)
#define outputPin(GPIO_BASE, n)   BIT_ADDR((GPIO_BASE + 12), n) // ODR
#define inputPin(GPIO_BASE, n)    BIT_ADDR((GPIO_BASE + 8), n) // IDR

// Maybe an even faster digitalWrite, but only if really needed
#define digitalWriteFaster(pn, high) \
    if (high) { \
        WRITE_REG(get_GPIO_Port(STM_PORT(pn))->BSRR, (STM_LL_GPIO_PIN(pn) >> GPIO_PIN_MASK_POS) & 0x0000FFFFU); \
    } \
    else { \
        WRITE_REG(get_GPIO_Port(STM_PORT(pn))->BRR, (STM_LL_GPIO_PIN(pn) >> GPIO_PIN_MASK_POS) & 0x0000FFFFU); \
    }

#define digitalWriteFastest(pn, high) \
    if (high) { \
        get_GPIO_Port(STM_PORT(pn))->BSRR = STM_GPIO_PIN(pn); \
    } \
    else { \
        get_GPIO_Port(STM_PORT(pn))->BRR = STM_GPIO_PIN(pn); \
    }

// Methods for writing to the GPIO
// When SystemClock_Config_HSE_8M_SYSCLK_72M() is selected:
// F         | GPIO_WRITE_METHOD                 | based on
//  14,4 kHz | GPIO_WRITE_DIGITAL_WRITE          | digitalWrite()
//  81.4 kHz | GPIO_WRITE_DIGITAL_WRITE_FAST     | digitalWriteFast()
//  1.64 MHz | GPIO_WRITE_DIGITAL_WRITE_FASTER   | digitalWriteFaster()
//  1.67 MHz | GPIO_WRITE_DIGITAL_WRITE_FASTEST  | digitalWriteFastest()
// 600.1 kHz | GPIO_WRITE_REGISTER_SET           | output()
// 163.3 kHz | GPIO_WRITE_HAL_FUNCTION           | HAL_GPIO_WritePin()
#define GPIO_WRITE_DIGITAL_WRITE         0
#define GPIO_WRITE_DIGITAL_WRITE_FAST    1
#define GPIO_WRITE_DIGITAL_WRITE_FASTER  2
#define GPIO_WRITE_DIGITAL_WRITE_FASTEST 3
#define GPIO_WRITE_REGISTER_SET          4
#define GPIO_WRITE_HAL_FUNCTION          5

// Setting for the GPIO write method
#define GPIO_WRITE_METHOD GPIO_WRITE_DIGITAL_WRITE_FASTEST

// Define the correct GPIO_WRITE method
#if (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE)
    #define GPIO_WRITE(pn, high) digitalWrite(pinNametoDigitalPin(pn), high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FAST)
    #define GPIO_WRITE(pn, high) digitalWriteFast(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FASTER)
    #define GPIO_WRITE(pn, high) digitalWriteFaster(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_DIGITAL_WRITE_FASTEST)
    #define GPIO_WRITE(pn, high) digitalWriteFastest(pn, high)

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_REGISTER_SET)
    // Pure bitsetting
    #define GPIO_WRITE(pn, high) \
        if (high) { \
            output((uint32_t)(get_GPIO_Port(STM_PORT(pn))), STM_PIN(pn)) = 1; \
        } \
        else { \
            output((uint32_t)(get_GPIO_Port(STM_PORT(pn))), STM_PIN(pn)) = 0; \
        }

#elif (GPIO_WRITE_METHOD == GPIO_WRITE_HAL_FUNCTION)
    // covered by HAL functions
    #define GPIO_WRITE(pn, high) \
        if (high) { \
            HAL_GPIO_WritePin(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn), GPIO_PIN_SET); \
        } \
        else { \
            HAL_GPIO_WritePin(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn), GPIO_PIN_RESET); \
        }

#else
    #error GPIO_WRITE_METHOD not a valid value
#endif

//#define GPIO_READ(pn) digitalReadFast(pn)
//#define GPIO_READ(pn) digital_io_read(get_GPIO_Port(STM_PORT(pn)), STM_GPIO_PIN(pn))
#define GPIO_READ(pn) LL_GPIO_IsInputPinSet(get_GPIO_Port(STM_PORT(pn)), STM_LL_GPIO_PIN(pn))

#define DIRECTION(x) ((x) > 0 ? 1 : (-1))  // Note: zero 'x' is not allowed

// --------------  Debugging  --------------
//#define TEST_FLASH

// Import the sanity check (needed so all files have the defines done in the sanity check file)
// Must be last so that it can use the defines above
#include "sanityCheck.h"

#endif //__CONFIG_H
