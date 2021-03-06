#include <saliency.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <Blob.h>
#include <BlobResult.h>
#include <highgui.h>
#include <cv.h>

#include <cvaux.h>

#include <gethog.h>
#include <svm.h>
#include <image.h>


namespace po = boost::program_options;

struct mousedata {
  int cntr ;
  CvPoint p1;
  CvPoint p2;
  int num_pts;
  bool neg;
} pp ={0,{},{},0,false};


void on_mouse(int event, int x, int y, int flags, void* param) {
  switch (event) {
  case CV_EVENT_LBUTTONDOWN:
    {
      pp.p1 = cvPoint(x,y);
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 1;
      pp.neg =false;
      break;
    }
  case CV_EVENT_LBUTTONUP:
    {
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 2;
      break;
    }
  case CV_EVENT_RBUTTONDOWN:
    {
      pp.p1 = cvPoint(x,y);
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 1;
      pp.neg = true;
      break;
    }
  case CV_EVENT_RBUTTONUP:
    {
      pp.p2 = cvPoint(x,y);
      pp.num_pts = 2;
      break;
    }

  case CV_EVENT_MOUSEMOVE:
    {
      if (flags & CV_EVENT_FLAG_LBUTTON ||
	  flags & CV_EVENT_FLAG_RBUTTON) {
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
    ("trainfile", po::value<std::string>(), "Train from this training file")
    ("load", po::value<std::string>(), "SVM to load, for adding on")
    ("save", po::value<std::string>(), "SVM to save")
    ("test", po::value<std::string>(), "run test on this directory")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc),vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout <<desc<<"\n";
    return 1;
  }
  std::vector< std::vector<float> > pos_descriptors;
  std::vector< std::vector<float> > neg_descriptors;
  cvNamedWindow("output",1);
  cvNamedWindow("Input",1);
  cvNamedWindow("NewSign",1);
  cvSetMouseCallback("Input", on_mouse);
  cv::SVM* mySVM;
  if (vm.count("load")) {
    mySVM = new cv::SVM;
    mySVM->load(vm["load"].as<std::string>().c_str());
  }
  bool test_mode = false;
  if (vm.count("trainfile")) {
    char fname [200];
    char posneg;
    CvRect rect;
    FILE* trainfile = fopen (vm["trainfile"].as<std::string>().c_str(), "r");
    if (!trainfile) {
      printf("Not able to load %s\n", 
	     vm["trainfile"].as<std::string>().c_str());
      exit(1);
    }
    while (!feof(trainfile)) {
      int found = 
	fscanf(trainfile, "%s : %c %u %u %u %u\n", 
	       fname, &posneg, &(rect.x), &(rect.y), &(rect.width), &(rect.height));
      if (found != 6) {
	printf("Fscanf might not be working\n");
      }
      IplImage* img_in = cvLoadImage(fname);
      IplImage* img_bw = TrainPrepImage(img_in);
      
      CvPoint UL = cvPoint(rect.x,rect.y);
      CvSize signsize = cvSize(rect.width,rect.height);
      if (!signsize.width  || 
	  !signsize.height) {
	printf("too small!\n");
	continue;
      }
      IplImage* newsign = cvCreateImage(signsize,
					IPL_DEPTH_8U,
					1);
      cvSetImageROI(img_bw, rect);
      cvCopy(img_bw, newsign);
      cvShowImage("NewSign", newsign);
      cvWaitKey(30);
      std::vector<float> desc = GetHog(newsign, 4);
      if (posneg == 'N' ) neg_descriptors.push_back(desc);
      else pos_descriptors.push_back(desc);
      
      cvReleaseImage(&newsign);

      cvReleaseImage(&img_in);
      cvReleaseImage(&img_bw);
    }
    if (pos_descriptors.size() && neg_descriptors.size()) {
      mySVM = TrainSVM_HOG(pos_descriptors,neg_descriptors);
      if (vm.count("save"))
	mySVM->save(vm["save"].as<std::string>().c_str(), "mysvm");
      else 	
	mySVM->save("testsvm.out", "mysvm");
    }
    else printf("Need at least one positive and negative examples\n");
    
  }
  if (vm.count("img")) {
    unsigned int key = -1;
    
    IplImage* img_in = cvLoadImage(vm["img"].as<std::string>().c_str());
    IplImage* img_bw = TrainPrepImage(img_in);
    cvShowImage("Input", img_in);
    do {
      IplImage* img = cvCloneImage(img_in);

      if (pp.num_pts != 0) {
	if (pp.neg) cvRectangle(img, pp.p1, pp.p2, CV_RGB(255,0,0), 1);
	else cvRectangle(img, pp.p1, pp.p2, CV_RGB(0,255,0), 1);
      }
      if (pp.num_pts == 2) {
	printf("Got two points!\n");
	pp.num_pts = 0;
	CvPoint UL = cvPoint(MIN(pp.p1.x, pp.p2.x),
			     MIN(pp.p1.y, pp.p2.y));
	CvSize signsize = cvSize(abs(pp.p2.x-pp.p1.x),
				 abs(pp.p2.y-pp.p1.y));
	if (!signsize.width  || 
	    !signsize.height) {
	  printf("too small!\n");
	  continue;
	}
	IplImage* newsign = cvCreateImage(signsize,
					  IPL_DEPTH_8U,
					  1);
	cvSetImageROI(img_bw, cvRect(UL.x, UL.y, signsize.width, signsize.height));
	cvCopy(img_bw, newsign);
	cvShowImage("NewSign", newsign);
	
	std::vector<float> desc = GetHog(newsign, 4);
	if (test_mode) {
	  float pred = TestSVM_HOG(desc,mySVM);
	  printf("Pred:%f\n",pred);
	}
	else {
	  if (pp.neg ) neg_descriptors.push_back(desc);
	  else pos_descriptors.push_back(desc);
	}
	cvReleaseImage(&newsign);
      }
      cvShowImage("Input", img);
      key = cvWaitKey(30);
      cvReleaseImage(&img);
      if ((char)key == 'd') {
	
	if (pos_descriptors.size() && neg_descriptors.size()) {
	  mySVM = TrainSVM_HOG(pos_descriptors,neg_descriptors);
	  mySVM->save("testsvm.out", "mysvm");
	  test_mode = true;
	  printf("Moved into test mode\n");
	}
	else printf("Need at least one positive and negative examples\n");
	key = 'a';
      }
    }while ((char)key != 'q');
    cvReleaseImage(&img_in);
    cvReleaseImage(&img_bw);
  }
  if (vm.count("test")){
    std::string thepath = vm["test"].as<std::string>();
    if (!boost::filesystem::is_directory(thepath))  {
      printf("Give me a better path, not %s\n",vm["test"].as<std::string>().c_str());
      return 1;
    }
    unsigned int key = -1;
    for (boost::filesystem::directory_iterator itr(thepath);
	 itr != boost::filesystem::directory_iterator();
	 ++itr) {
      if (!boost::filesystem::is_regular_file(itr->status())){
	continue;
      }
      std::string full_name =  thepath + itr->path().filename();
      printf ("Loading %s\n",
	      full_name.c_str());
      IplImage* img_in = cvLoadImage(full_name.c_str());
      IplImage* img_bw = TrainPrepImage(img_in);
      do {
	IplImage* img = cvCloneImage(img_in);

	if (pp.num_pts != 0) {
	  if (pp.neg) cvRectangle(img, pp.p1, pp.p2, CV_RGB(255,0,0), 1);
	  else cvRectangle(img, pp.p1, pp.p2, CV_RGB(0,255,0), 1);
	}
	if (pp.num_pts == 2) {
	  pp.num_pts = 0;
	  CvPoint UL = cvPoint(MIN(pp.p1.x, pp.p2.x),
			       MIN(pp.p1.y, pp.p2.y));
	  CvSize signsize = cvSize(abs(pp.p2.x-pp.p1.x),
				   abs(pp.p2.y-pp.p1.y));
	  if (!signsize.width  || 
	      !signsize.height) {
	    printf("too small!\n");
	    continue;
	  }
	  IplImage* newsign = cvCreateImage(signsize,
					    IPL_DEPTH_8U,
					    1);
	  cvSetImageROI(img_bw, cvRect(UL.x, UL.y, signsize.width, signsize.height));
	  cvCopy(img_bw, newsign);
	  cvShowImage("NewSign", newsign);
	  
	  std::vector<float> desc = GetHog(newsign, 4);
	  float pred = TestSVM_HOG(desc,mySVM);
	  printf("Pred:%f\n",pred);
	  cvReleaseImage(&newsign);
	}
	
	cvShowImage("Input", img);
	key = cvWaitKey(30);
	cvReleaseImage(&img);
	
      }while((char)key != 'd' && (char) key != 'q');
      if ((char)key == 'q') break;
    }
  }
  return -1;
}
