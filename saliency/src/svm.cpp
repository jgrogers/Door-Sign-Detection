#include <svm.h>
#include <ml.h>
cv::SVM* TrainSVM_HOG(const std::vector<std::vector<float> >& pos,
		   const std::vector<std::vector<float> >& neg) {
  CvMat* train_data = cvCreateMat(pos.size() + neg.size(),
				  pos[0].size(),
				  CV_32FC1);
  CvMat* responses = cvCreateMat(pos.size() + neg.size(),
				 1,
				 CV_32FC1);
  for (size_t row = 0;row< pos.size();row++) {
    for (size_t col = 0; col < pos[row].size();col++) {
      cvSet2D(train_data, row,col, cvScalarAll(pos[row][col]));
      cvSet2D(responses, row,0, cvScalarAll(1.0));
    }
  }

  for (size_t row = 0;row< neg.size();row++) {
    for (size_t col = 0; col < neg[row].size();col++) {
      cvSet2D(train_data, row+pos.size(),col, cvScalarAll(neg[row][col]));
      cvSet2D(responses, row+pos.size(),0, cvScalarAll(-1.0));
    }
  }
  cv::SVM* mySVM = new cv::SVM(train_data, responses);
  cvReleaseMat(&train_data);
  cvReleaseMat(&responses);
  return mySVM;
}


float TestSVM_HOG(const std::vector<float> sample, const cv::SVM* mySVM) {
  CvMat* test_data =cvCreateMat(1, sample.size(),
				CV_32FC1);
  for (size_t col = 0; col < sample.size();col++) {
    cvSet2D(test_data, 0,col, cvScalarAll(sample[col]));
  }
  float result = mySVM->predict(test_data);
  cvReleaseMat(&test_data);
  return result;
}
