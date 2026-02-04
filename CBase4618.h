#pragma once

#include "CControl.h"
#include <opencv2/opencv.hpp>

/**
 * @brief The base class for ELEX 4618 labs.
 * Holds the serial control and the drawing canvas.
 */
class CBase4618
{
private:
    CControl _control;  ///< Communication with the Tiva C
    cv::Mat _canvas;    ///< The image to draw on

public:
    /**
     * @brief Runs the game loop (update -> draw -> check for 'q').
     */
    void run();

    /**
     * @brief Pure virtual function to update game state.
     */
    virtual void update() = 0; // [cite: 21]

    /**
     * @brief Pure virtual function to render the game.
     */
    virtual void draw() = 0;   // [cite: 21]
};