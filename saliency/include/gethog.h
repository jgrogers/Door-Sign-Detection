#pragma once
#include <cvaux.h>
#include <cv.h>
#include <vector>

std::vector<float> GetHog(const IplImage* img, size_t scale);
