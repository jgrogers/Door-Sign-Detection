#pragma once
#include <vector>
#include <highgui.h>
#include <cvaux.h>
#include <cv.h>
#include <ml.h>
cv::SVM* TrainSVM_HOG(const std::vector<std::vector<float> >& pos,
		  const std::vector<std::vector<float> >& neg);
float TestSVM_HOG(const std::vector<float> sample, const cv::SVM* mySVM);
