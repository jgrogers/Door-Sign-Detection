#include <gethog.h>


std::vector<float> GetHog(const IplImage* img, size_t scale) {
  assert (scale >=4);
  size_t sz = pow(2, scale);
  CvMat* newmat = cvCreateMat(sz, sz,CV_8UC1);
  cv::HOGDescriptor hog(cvSize(sz,sz),
			cvSize(sz,sz),
			cvSize(4,4),
			cvSize(4,4),
			8,1,-1); 
  
  
  cvResize(img, newmat);
  cv::Mat tmpmat(newmat, true);
  std::vector<float> descriptor;
  hog.compute(tmpmat,descriptor);
  return descriptor;
}
