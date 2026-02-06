#pragma once
#include "CBase4618.h"
#include <vector>


#define BTN_COLOR_CH  S1
#define BTN_RESET_CH  S2


class CSketch : public CBase4618
{
private:
    // Game State
    int _colorIndex;
    bool _resetPending;
    cv::Point _currentPos;
    cv::Point _lastPos;

    // Data buffers (Bridge between gpio and update)
    int _dataX;             // Raw Joystick X
    int _dataY;             // Raw Joystick Y
    int _btnColorRaw;       // Raw Button Input (0 or 1)
    int _btnResetRaw;       // Raw Button Input (0 or 1)

    // Helper variables for debouncing logic in update()
    int _lastBtnColorState;
    int _lastBtnResetState;
    double _lastColorTime; // To track debounce timing

    // Graphics
    cv::Mat _drawing; // Persistent layer for lines
    std::vector<cv::Scalar> _colors;
    std::vector<std::string> _colorNames;

    // GUI
    cv::Rect _btnResetRect;
    cv::Rect _btnExitRect;

    // Accelerometer Data
    float _accelX, _accelY, _accelZ;
    float _prevAccelX, _prevAccelY, _prevAccelZ;

    static void on_mouse(int event, int x, int y, int flags, void* userdata);

public:
    CSketch(cv::Size size, int comPort);

    // The V2 Required Methods
    virtual void gpio() override;
    virtual void update() override;
    virtual void draw() override;
};