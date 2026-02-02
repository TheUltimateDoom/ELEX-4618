#pragma once
#include "Serial.h"

/**
 * @brief Constants for the communication protocol types.
 */
enum { DIGITAL = 0, ANALOG, SERVO };
enum { BUTTON_PRESS_SUCCESS = 0, BUTTON_PRESS_FAIL = 1, BUTTON_PRESS_NONE = 2};


#define S1 33
#define S2 32
#define LED_BLU 37
#define LED_GRN 38
#define LED_RED 39
#define JOY_Y 4
#define JOY_X 11

/**
 * @class CControl
 * @brief A class to interface with the TI Microcontroller via Serial communication.
 * 
 * This class provides methods to read and write digital, analog, and servo data using a specific serial protocol.
 * @author Farhan Sadiq
 */
class CControl
{
private:
    Serial _com; ///< Serial port object for communication

public:
    /** @brief Constructor */
    CControl();

    /** @brief Destructor */
    ~CControl();

    /**
     * @brief Initializes the serial port.
     * @param comport The COM port number (e.g., 3 for COM3).
     */
    void init_com(int comport);

    /**
     * @brief Sends a GET command to the microcontroller.
     * @param type The type of data (DIGITAL, ANALOG).
     * @param channel The pin/channel number.
     * @param result Reference to store the returned value.
     * @return true if successful, false if timed out or failed.
     */
    bool get_data(int type, int channel, int& result);

    /**
     * @brief Sends a SET command to the microcontroller.
     * @param type The type of data (DIGITAL, ANALOG, SERVO).
     * @param channel The pin/channel number.
     * @param val The value to set.
     * @return true if successful, false if failed.
     */
    bool set_data(int type, int channel, int val);

    /**
     * @brief Helper to get analog value as a percentage.
     * @param channel The analog channel to read.
     * @return The value as a percentage (0.0 to 100.0).
     */
    float get_analog(int channel);

    /**
     * @brief Helper to read a button with a 1-second debounce timeout.
     * @param channel The digital channel to read.
     * @return true if the button was pressed (after debouncing), false otherwise.
     */
    int get_button(int channel);

    /**
     * @brief Helper to auto-detect the correct serial port.
     * @param void
     * @return true if the microcontroller was able to connect, false otherwise.
     */
    bool auto_init();
};