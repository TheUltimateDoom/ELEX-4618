#include "stdafx.h"
#include "CSketch.h"

// CONSTRUCTOR
CSketch::CSketch(cv::Size size, int comPort)
{
    // 1. Initialize the CControl object with the correct COM port
    _control.init_com(comPort);

    // 2. Create the OpenCV canvas (Black background, 3 color channels)
    // Source: [cite: 27, 28]
    _canvas = cv::Mat::zeros(size, CV_8UC3);

    // Initialize coordinates to center (to prevent a line jumping from 0,0 at start)
    _currentPos = cv::Point(size.width / 2, size.height / 2);
    _lastPos = _currentPos;

    // Setup Colors (OpenCV is BGR)
    // Index 0: RED
    _colors.push_back(cv::Scalar(0, 0, 255));
    _colorNames.push_back("RED");

    // Index 1: GREEN
    _colors.push_back(cv::Scalar(0, 255, 0));
    _colorNames.push_back("GREEN");

    // Index 2: BLUE
    _colors.push_back(cv::Scalar(255, 0, 0));
    _colorNames.push_back("BLUE");

    _colorIndex = 0; // Start Red
}

// UPDATE METHOD
void CSketch::update()
{
    int rawX = 0;
    int rawY = 0;

    // 1. Read Analog Inputs (Adjust channels to match your wiring!)
    // Assuming X is Ch 11 and Y is Ch 4 based on your previous labs.
    bool successX = _control.get_data(ANALOG, 11, rawX);
    bool successY = _control.get_data(ANALOG, 4, rawY);

    if (successX && successY)
    {
        // 2. Map 0-4095 to 0-Width (and Height)
        // Source: 

        // Note: We cast to float for precision, then back to int for pixels.
        // We invert Y (_canvas.rows - ...) because on screens, Y=0 is the TOP, 
        // but on joysticks, "Up" usually means higher voltage.
        int screenX = (int)((rawX / 4095.0f) * _canvas.cols);
        int screenY = _canvas.rows - (int)((rawY / 4095.0f) * _canvas.rows);

        // Update positions
        _lastPos = _currentPos;  // Save where we were
        _currentPos = cv::Point(screenX, screenY); // Update where we are
    }

    // 2. Handle Color Button (Debounced) 
    // Assuming S2 (Channel 32) is for Color
    if (_control.get_button(BTN_COLOR_CH) == BUTTON_PRESS_SUCCESS)
    {
        _colorIndex++;
        if (_colorIndex >= _colors.size())
        {
            _colorIndex = 0; // Wrap around to start
        }
    }

    // 3. Handle Reset Button (Debounced) 
    // Assuming S1 (Channel 33) is for Reset
    if (_control.get_button(BTN_RESET_CH) == BUTTON_PRESS_SUCCESS)
    {
        _resetPending = true;
    }

    // 4. Sync LED with Current Color 
    // Turn off all LEDs first to be safe
    _control.set_data(DIGITAL, LED_RED, 0);
    _control.set_data(DIGITAL, LED_GRN, 0);
    _control.set_data(DIGITAL, LED_BLU, 0);

    // Turn on the one matching _colorIndex
    if (_colorIndex == 0) _control.set_data(DIGITAL, LED_RED, 1);
    else if (_colorIndex == 1) _control.set_data(DIGITAL, LED_GRN, 1);
    else if (_colorIndex == 2) _control.set_data(DIGITAL, LED_BLU, 1);
}

// DRAW METHOD
void CSketch::draw()
{
    // 1. Handle Reset [cite: 50]
    if (_resetPending)
    {
        // Clear screen to black
        _canvas = cv::Mat::zeros(_canvas.size(), CV_8UC3);
        _resetPending = false; // Reset complete
    }

    // 2. Draw Line
    // Use _colors[_colorIndex] to get the current BGR color
    cv::line(_canvas, _lastPos, _currentPos, _colors[_colorIndex], 2);

    // 3. Draw UI Text [cite: 46]
    // We draw a filled rectangle first so the text is readable over the drawing
    cv::rectangle(_canvas, cv::Point(0, 0), cv::Point(200, 60), cv::Scalar(50, 50, 50), -1);

    // Display the color name
    std::string text = "Color: " + _colorNames[_colorIndex];
    cv::putText(_canvas, text, cv::Point(10, 40),
        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

    // 4. Show Image
    cv::imshow("Etch-A-Sketch", _canvas);
}