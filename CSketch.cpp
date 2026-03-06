#include "stdafx.h"
#include "CSketch.h"


CSketch::CSketch(cv::Size size, int comPort)
{
    _control.init_com(comPort);

    _canvas = cv::Mat::zeros(size, CV_8UC3);
    _drawing = cv::Mat::zeros(size, CV_8UC3); // Persistent layer

	// Active low buttons
    _btnColorRaw = 1; _btnResetRaw = 1; 
	_lastBtnColorState = 1; _lastBtnResetState = 1;

    // Initialize Variables
    _dataX = 0; _dataY = 0;
    _lastColorTime = 0;
    _colorIndex = 0;
    _currentPos = cv::Point(size.width / 2, size.height / 2);
    _lastPos = _currentPos;
	_accelX = _accelY = _accelZ = 0;
	_prevAccelX = _prevAccelY = _prevAccelZ = 0;
	_resetPending = false;


    // Setup Colors (BGR)
    _colors.push_back(cv::Scalar(0, 0, 255));   // Red
    _colors.push_back(cv::Scalar(0, 255, 0));   // Green
    _colors.push_back(cv::Scalar(255, 0, 0));   // Blue
    _colorNames.push_back("RED");
    _colorNames.push_back("GREEN");
    _colorNames.push_back("BLUE");

    // GUI Setup
    _btnResetRect = cv::Rect(10, 70, 80, 30);
    _btnExitRect = cv::Rect(100, 70, 80, 30);

    cv::namedWindow("Etch-A-Sketch");
    cv::setMouseCallback("Etch-A-Sketch", on_mouse, this);
}

void CSketch::gpio()
{
	// Read joystick and buttons
    _dataX = _control.get_analog(JOY_X);
    _dataY = _control.get_analog(JOY_Y);
    _control.get_data(DIGITAL, BTN_COLOR_CH, _btnColorRaw);
    _control.get_data(DIGITAL, BTN_RESET_CH, _btnResetRaw);

    // Update LEDs based on Game State
    _control.set_data(DIGITAL, LED_RED, (_colorIndex == 0) ? 1 : 0);
    _control.set_data(DIGITAL, LED_GRN, (_colorIndex == 1) ? 1 : 0);
    _control.set_data(DIGITAL, LED_BLU, (_colorIndex == 2) ? 1 : 0);

    // Read Accelerometer, save previous frame for comparison
    _prevAccelX = _accelX;
    _prevAccelY = _accelY;
    //_prevAccelZ = _accelZ;

    _accelX = _control.get_analog(ACCEL_X);
    _accelY = _control.get_analog(ACCEL_Y);
    //_accelZ = _control.get_analog(ACCEL_Z);
}

void CSketch::update()
{

    /*std::cout << "RawX: " << std::fixed << std::setprecision(1) << _dataX
        << " | RawY: " << _dataY
        << " | ScreenPos: [" << _currentPos.x << "," << _currentPos.y << "]"
        << "          \n";*/
    
    /*std::cout << "accelX: " << std::fixed << std::setprecision(1) << _accelX
        << " | accelY: " << _accelY
        << " | accelZ: " << _accelZ
		<< "          \n";*/

    // SETUP CONSTANTS
    float deadzoneMin = 45.0f;
    float deadzoneMax = 50.0f;
    float maxSpeed = 20.0f;     // Pixels per frame

    // X MOVEMENT
    if (_dataX < deadzoneMin) // Move Left
    {
        // Map 45->0 to 0->MaxSpeed
        float ratio = (deadzoneMin - _dataX) / deadzoneMin;
        _currentPos.x -= (int)(ratio * maxSpeed);
    }
    else if (_dataX > deadzoneMax) // Move Right
    {
        float ratio = (_dataX - deadzoneMax) / (100.0f - deadzoneMax);
        _currentPos.x += (int)(ratio * maxSpeed);
    }

    // Y MOVEMENT
    if (_dataY < deadzoneMin) // Move Down (Positive Y)
    {
        float ratio = (deadzoneMin - _dataY) / deadzoneMin;
        _currentPos.y += (int)(ratio * maxSpeed);
    }
    else if (_dataY > deadzoneMax) // Move Up (Negative Y)
    {
        float ratio = (_dataY - deadzoneMax) / (100.0f - deadzoneMax);
        _currentPos.y -= (int)(ratio * maxSpeed);
    }

    // CLAMPING
    if (_currentPos.x < 0) _currentPos.x = 0;
    if (_currentPos.x >= _canvas.cols) _currentPos.x = _canvas.cols - 1;
    if (_currentPos.y < 0) _currentPos.y = 0;
    if (_currentPos.y >= _canvas.rows) _currentPos.y = _canvas.rows - 1;

    // DRAW
    cv::line(_drawing, _lastPos, _currentPos, _colors[_colorIndex], 2);
    _lastPos = _currentPos;


    // SHAKE DETECTION
    // Calculate change in acceleration
    float deltaX = std::abs(_accelX - _prevAccelX);
    float deltaY = std::abs(_accelY - _prevAccelY);
    // float deltaZ = std::abs(_accelZ - _prevAccelZ);

    float shakeThreshold = 20.0f;

    if (deltaX > shakeThreshold || deltaY > shakeThreshold)
    {
        std::cout << "Shake Detected! Clearing..." << std::endl;
        _resetPending = true;
    }

    // BUTTON DEBOUNCE LOGIC
    // Handle Color Button
    double currentTime = cv::getTickCount() / cv::getTickFrequency();

    // Detect Falling Edge (Transition from 1 to 0) + Timeout
    if (_btnColorRaw == 0 && _lastBtnColorState == 1 && (currentTime - _lastColorTime > 0.25)) // 250ms debounce
    {
        _colorIndex++;
        if (_colorIndex >= _colors.size()) _colorIndex = 0;
        _lastColorTime = currentTime;
    }
    _lastBtnColorState = _btnColorRaw;

    // Handle Reset Button
    if (_btnResetRaw == 0) // Simple check for reset
    {
        _resetPending = true;
    }
}

void CSketch::draw()
{
    // Clear the canvas
    _canvas = cv::Mat::zeros(_canvas.size(), CV_8UC3);

    // If reset pending, clear the persistent layer
    if (_resetPending)
    {
        _drawing = cv::Mat::zeros(_drawing.size(), CV_8UC3);
        _resetPending = false;
    }

    // Copy persistent drawing onto the fresh canvas
    _drawing.copyTo(_canvas);

    // Draw Text & Buttons
    cv::rectangle(_canvas, cv::Point(0, 0), cv::Point(200, 110), cv::Scalar(50, 50, 50), -1);
    std::string text = "Color: " + _colorNames[_colorIndex];
    cv::putText(_canvas, text, cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
    cv::rectangle(_canvas, _btnResetRect, cv::Scalar(100, 200, 100), -1);
    cv::putText(_canvas, "RESET", cv::Point(_btnResetRect.x + 10, _btnResetRect.y + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::rectangle(_canvas, _btnExitRect, cv::Scalar(100, 100, 200), -1);
    cv::putText(_canvas, "EXIT", cv::Point(_btnExitRect.x + 20, _btnExitRect.y + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    cv::imshow("Etch-A-Sketch", _canvas);
}

void CSketch::on_mouse(int event, int x, int y, int flags, void* userdata)
{
    CSketch* self = (CSketch*)userdata;

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        // Reset Logic
        if (self->_btnResetRect.contains(cv::Point(x, y)))
        {
            self->_resetPending = true;
        }

        // Exit Logic
        else if (self->_btnExitRect.contains(cv::Point(x, y)))
        {
            std::cout << "GUI: Exit Pressed" << std::endl;
            exit(0);
        }
    }
}