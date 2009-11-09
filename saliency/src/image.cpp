#include <image.h>

IplImage* TrainPrepImage(IplImage*& img) {

    IplImage* img_tmp = cvCreateImage(cvSize(1024,768), IPL_DEPTH_8U, 3);
    IplImage* img_bw = cvCreateImage(cvGetSize(img_tmp), IPL_DEPTH_8U, 1);
    cvResize(img, img_tmp);
    cvCvtColor(img_tmp, img_bw, CV_BGR2GRAY);
    cvReleaseImage(&img);
    img = img_tmp;
    return img_bw;
}
