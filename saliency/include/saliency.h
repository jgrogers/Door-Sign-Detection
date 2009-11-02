#pragma once
#include <highgui.h>


IplImage* 
ComputeSaliency(IplImage* image, int thresh, int scale);
