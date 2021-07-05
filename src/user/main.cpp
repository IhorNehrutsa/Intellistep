/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "buttons.h"
#include "canMessaging.h"
#include "serial.h"
#include "flash.h"
#include "encoder.h"
#include "oled.h"
#include "led.h"
#include "cube.h"
//#include "stm32yyxx_ll_rcc.h"

// Create a new motor instance
StepperMotor motor = StepperMotor();

// Run the setup
void setup() {

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Set processor up
    SystemInit();

    // Configure the system clock
    #if defined(SYSCLK_SRC_HSE_16)
        #if SYSCLK_FREQ == 72
            SystemClock_Config_HSE_16M_SYSCLK_72M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #elif defined(SYSCLK_SRC_HSE_8)
        #if SYSCLK_FREQ == 72
            SystemClock_Config_HSE_8M_SYSCLK_72M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #elif defined(SYSCLK_SRC_HSI)
        #if SYSCLK_FREQ == 64
            SystemClock_Config_HSI_8M_SYSCLK_64M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #else
	    #error "Unsupported oscillator source"
    #endif

    #ifdef CHECK_MCO_OUTPUT
        MCO_GPIO_Init();
    #endif

    // Update the system clock with the new speed
    SystemCoreClockUpdate();

    // Initialize the encoder
    initEncoder();

    // Setup the motor for use
    motor.setState(DISABLED, true);
    motor.setMicrostepping(16);
    //motor.setDesiredAngle(100);

    // Only run if the OLED is enabled
    #ifdef ENABLE_OLED

        // Initialize the OLED
        initOLED();

        // Show the bootscreen
        showBootscreen();

        // Wait for 3 seconds so everything can boot and user can read the LCD
        delay(3000);

        // Set the inversion (if specified)
        #ifdef INVERTED_DIPS
            setDipInverted(true);
        #else
            setDipInverted(false);
        #endif

        // Initialize the buttons (for menu)
        initButtons();
    #endif

    // Initialize the serial bus
    #ifdef ENABLE_SERIAL
        initSerial();
    #endif

    // Initialize the CAN bus
    #ifdef ENABLE_CAN
        // Initialize the CAN bus
        initCAN();
    #endif

    // Initialize the LED
    #ifdef ENABLE_LED
        initLED();
    #endif

    #ifdef CHECK_MCO_OUTPUT
        MCO_GPIO_Init();
    #endif

    #ifdef CHECK_GPIO_OUTPUT_SWITCHING
        PA_8_GPIO_Init();
        while(true) {
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);

            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
        }
    #endif

    // Test the flash if specified
    //#ifdef TEST_FLASH
    //    flash_test();
    //#endif

    // Clear the display, then write that we're using the closed loop mode
    //clearOLED();
    //writeOLEDString(0, 0, "Close Loop Mode");

    // Check if the board is calibrated. Need to force calibration if the board isn't calibrated
    if (!isCalibrated()) {

        // Only display to screen if the screen is enabled
        #ifdef ENABLE_OLED

            // Display that the motor is not calibrated
            clearOLED();
            writeOLEDString(0, 0, "NOT", false);
            writeOLEDString(0, 16, "Calibrated!", false);
            writeOLEDString(0, 32, "Please calibrate", true);
            delay(3000);

            // Display that the select key can be clicked to run calibration
            clearOLED();
            writeOLEDString(0, 0, "Use the", false);
            writeOLEDString(0, 16, "select key", false);
            writeOLEDString(0, 32, "to calibrate", true);
        #endif

        // Continuously check to see if the select key is clicked (depth index would increase when clicked)
        while(true) {

            #ifdef ENABLE_OLED
                // Check to see if any of the buttons are pressed
                checkButtons(false, true);
            #endif

            // ! Only for testing
            //blink();

            // Only if the OLED is needed
            #ifdef ENABLE_OLED

                // Check to see if the menu button has been clicked
                if (getMenuDepth() > 0) {

                    // Calibrate the motor
                    motor.calibrate();

                    // Reboot the chip
                    NVIC_SystemReset();
                }

            #else

                // Just jump to calibrating the motor and reset the system afterward
                motor.calibrate();
                NVIC_SystemReset();

            #endif
        }
    }
    else {
        // There is a calibration, load it and move on to the loop

        #ifdef ENABLE_OLED

            // Let the user know that the calibration was successfully loaded
            clearOLED();
            writeOLEDString(0, 0, "Calibration", false);
            writeOLEDString(0, 16, "OK!", false);

            // Write base string for flash loading
            writeOLEDString(0, 32, F("Flash loaded"), false);

            // Attempt to load the parameters from flash
            if (loadParameters() == FLASH_LOAD_SUCCESSFUL) {
                writeOLEDString(0, 48, F("successfully"), true);
            }
            else {
                writeOLEDString(0, 48, F("unsuccessfully"), true);
            }

            // Let the user read the message
            delay(1000);

            // Clear the display
            clearOLED();

            // Write out the first data to the screen (makes sure that the first write isn't interrupted)
            displayMotorData();
        #else
            // Nothing special, just try to load the flash data
            loadParameters();
        #endif

        // Setup the motor timers and interrupts
        setupMotorTimers();
    }
}


// Main loop
void loop() {
    // Check the dip switches
    checkDips();

    // Check to see if serial data is available to read
    #ifdef ENABLE_SERIAL
        runSerialParser();
    #endif

    #ifdef ENABLE_OLED
        // Check the buttons
        checkButtons(true);

        // Only update the display if the motor data is being displayed, buttons update the display when clicked
        if (getMenuDepth() == MOTOR_DATA) {
            displayMotorData();
        }
    #endif

    // We need a little delay to allow the motor time to process if it needs it
    #ifdef ENABLE_BLINK
        // ! Only for testing
        blink();
    #else
        delay(50);
    #endif
}


// Overclocks the processor to the desired value
void overclock(uint32_t PLLMultiplier) {

    // Tune the HSI
    //HSI_CalibrateMinError();

    // Initialization structure
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Enable HSE Oscillator and activate PLL with HSI as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = PLLMultiplier;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // Wait for the PLL to be configured
    while(!(RCC_CR_PLLRDY & RCC -> CR))
        ; //

    // Use the PLL as the system clock
    RCC -> CFGR |= RCC_SYSCLKSOURCE_PLLCLK;

    #ifdef CHECK_MCO_OUTPUT
        HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
    #endif
}


// ! Only here for testing
void blink() {
    GPIO_WRITE(LED_PIN, HIGH);
    delay(500);
    GPIO_WRITE(LED_PIN, LOW);
    delay(500);
}
