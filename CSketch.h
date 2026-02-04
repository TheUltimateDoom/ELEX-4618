#pragma once
#include "CBase4618.h"
#include <vector>


#define BTN_COLOR_CH  S1  // Button S2
#define BTN_RESET_CH  S2  // Button S1


class CSketch : public CBase4618
{
private:
    int _colorIndex;        // 0=Red, 1=Green, 2=Blue
    bool _resetPending;     // Flag to clear screen
    cv::Point _currentPos;
    cv::Point _lastPos;

    // Store the 3 colors for easy access
    std::vector<cv::Scalar> _colors;
    std::vector<std::string> _colorNames;

public:
    /**
     * @brief Constructor for the Etch-A-Sketch game.
     * @param size The size of the OpenCV canvas (e.g., 500x500).
     * @param comPort The COM port for the microcontroller.
     */
    CSketch(cv::Size size, int comPort);
    virtual void update() override;
    virtual void draw() override;
};