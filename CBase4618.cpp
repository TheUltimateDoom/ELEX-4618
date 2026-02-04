#include "stdafx.h"
#include "CBase4618.h"

void CBase4618::run()
{
    // Loop until user presses 'q'
    char key = ' ';
    while (key != 'q')
    {
        update(); // Read inputs, calc logic
        draw();   // Update screen

        // cv::waitKey is critical! It updates the GUI window.
        // Wait 1ms for a keypress.
        key = cv::waitKey(1);
    }
}