#include "stdafx.h"
#include "CBase4618.h"

void CBase4618::run()
{
    _running = true;
    char key = ' ';

    while (_running && key != 'q')
    {
        gpio();   // 1. Get Inputs (Slow COM port access)
        update(); // Read inputs, calc logic
        draw();   // Update screen

        // cv::waitKey is critical! It updates the GUI window.
        // Wait 1ms for a keypress.
        key = cv::waitKey(1);
    }
}