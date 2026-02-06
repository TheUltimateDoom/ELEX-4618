#include "stdafx.h"
#include "CBase4618.h"

void CBase4618::run()
{
    char key = ' ';

    while (key != 'q')
    {
        gpio();
        update();
        draw();

        key = cv::waitKey(1);
    }
    cv::destroyAllWindows();
}