#include <saliency.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <Blob.h>
#include <BlobResult.h>
#include <highgui.h>
#include <cv.h>

#include <cvaux.h>

#include <gethog.h>


namespace po = boost::program_options;

struct mousedata {
  int cntr ;
  CvPoint p1;
  CvPoint p2;
  int num_pts;
} pp ={0,{},{},0};


void on_mouse(int event, int x, int y, int flags, void* param) {
  switch (event) {
  case CV_EVENT_LBUTTONDOWN:
    {
      pp.p1 = cvPoint(x,y);
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 1;
      break;
    }
  case CV_EVENT_LBUTTONUP:
    {
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 2;
      break;
    }
  case CV_EVENT_MOUSEMOVE:
    {
      if (flags & CV_EVENT_FLAG_LBUTTON) {
	pp.p2 = cvPoint(x,y);
      }
      break;
    }

  }
  
}

int main(int argc, char** argv) {
  unsigned int uint_opt;
  double double_opt;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("img", po::value<std::string>(), "Load this file to pull training from")
    ("dir", po::value<std::string>(), "Train on all images in directory")
    ("load", po::value<std::string>(), "SVM to load, for adding on")
    ("save", po::value<std::string>(), "SVM to save")
    ("manual", "run manually, no masks")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc),vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout <<desc<<"\n";
    return 1;
  }
  std::vector< std::vector<float> > sign_descriptors;
  cvNamedWindow("output",1);
  cvNamedWindow("Input",1);
  cvNamedWindow("NewSign",1);
  cvSetMouseCallback("Input", on_mouse);
  if (vm.count("img")) {
    unsigned int key = -1;
    IplImage* img_in = cvLoadImage(vm["img"].as<std::string>().c_str());
    IplImage* img_tmp = cvCreateImage(cvSize(1024,768), IPL_DEPTH_8U, 3);
    IplImage* img_bw = cvCreateImage(cvGetSize(img_tmp), IPL_DEPTH_8U, 1);
    cvResize(img_in, img_tmp);
    cvCvtColor(img_tmp, img_bw, CV_BGR2GRAY);
    cvReleaseImage(&img_in);
    img_in = img_tmp;
    cvShowImage("Input", img_in);
    do {
      IplImage* img = cvCloneImage(img_in);

      if (pp.num_pts != 0) {
	cvRectangle(img, pp.p1, pp.p2, CV_RGB(0,255,0), 1);
      }
      if (pp.num_pts == 2) {
	printf("Got two points!\n");
	pp.num_pts = 0;
	CvPoint UL = cvPoint(MIN(pp.p1.x, pp.p2.x),
			     MIN(pp.p1.y, pp.p2.y));
	CvSize signsize = cvSize(abs(pp.p2.x-pp.p1.x),
				 abs(pp.p2.y-pp.p1.y));
	IplImage* newsign = cvCreateImage(signsize,
					  IPL_DEPTH_8U,
					  1);
	cvSetImageROI(img_bw, cvRect(UL.x, UL.y, signsize.width, signsize.height));
	cvCopy(img_bw, newsign);
	cvShowImage("NewSign", newsign);
	
	std::vector<float> desc = GetHog(newsign, 4);
	sign_descriptors.push_back(desc);
	cvReleaseImage(&newsign);
      }
      cvShowImage("Input", img);
      key = cvWaitKey(30);
      cvReleaseImage(&img);
    }while (key != 'q');
    cvReleaseImage(&img_in);
    cvReleaseImage(&img_bw);
  }
  
  return -1;
}
