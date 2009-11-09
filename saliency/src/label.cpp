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

//returns bw image resized, also resizes source image
void DrawCurrentRects(IplImage* img, const std::vector<CvRect>& pos,
		      const std::vector<CvRect>& neg) {
  for (std::vector<CvRect>::const_iterator itr = pos.begin();
       itr != pos.end();
       itr++) {
    CvPoint p1 = cvPoint(itr->x,itr->y);
    CvPoint p2 = cvPoint(itr->x+itr->width,
			 itr->y+itr->height);
    cvRectangle(img, p1, p2, CV_RGB(0,255,0), 1);
  }
  for (std::vector<CvRect>::const_iterator itr = neg.begin();
       itr != neg.end();
       itr++) {
    CvPoint p1 = cvPoint(itr->x,itr->y);
    CvPoint p2 = cvPoint(itr->x+itr->width,
			 itr->y+itr->height);
    cvRectangle(img, p1, p2, CV_RGB(255,0,0), 1);
  }

}
void Save(FILE* fp, const std::string& img_name, 
	  const std::vector<CvRect>& pos,
	  const std::vector<CvRect>& neg) {
  if (fp == NULL) return;
  for (std::vector<CvRect>::const_iterator itr = pos.begin();
       itr != pos.end();
       itr++) {
    fprintf(fp, "%s : P %u %u %u %u\n",
	    img_name.c_str(), 
	    itr->x, itr->y, itr->width, itr->height);
  }

  for (std::vector<CvRect>::const_iterator itr = neg.begin();
       itr != neg.end();
       itr++) {
    fprintf(fp, "%s : N %u %u %u %u\n",
	    img_name.c_str(), 
	    itr->x, itr->y, itr->width, itr->height);
  }

}
int main(int argc, char** argv) {
  unsigned int uint_opt;
  double double_opt;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("img", po::value<std::string>(), "Label only this image")
    ("dir", po::value<std::string>(), "Label all images in directory")
    ("load", po::value<std::string>(), "clicks to load")
    ("save", po::value<std::string>(), "file to save the clicks in")

    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc),vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout <<desc<<"\n";
    return 1;
  }
  FILE* save_fp = NULL;
  if (vm.count("save")) {
    save_fp = fopen(vm["save"].as<std::string>().c_str(), "a");
    
  }
  std::vector< CvRect> pos;
  std::vector< CvRect> neg;
  cvNamedWindow("Input",1);
  cvSetMouseCallback("Input", on_mouse);
  if (vm.count("dir")) {
    std::string thepath = vm["dir"].as<std::string>();
    if (!boost::filesystem::is_directory(thepath))  {
      printf("Give me a better path, not %s\n",vm["dir"].as<std::string>().c_str());
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
      cvShowImage("Input", img_in);
      do {
	IplImage* img = cvCloneImage(img_in);
	DrawCurrentRects(img, pos,neg);
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
	  CvRect newrect = cvRect(UL.x,UL.y,signsize.width,signsize.height);
	  if (pp.neg) neg.push_back(newrect);
	  else pos.push_back(newrect);
	}
	cvShowImage("Input", img);
	key = cvWaitKey(30);
	cvReleaseImage(&img);
	if ((char)key == 'u') {
	  pos.clear();
	  neg.clear();
	}

      }while ((char) key != ' ' &&
	      (char) key != 'd');
      //next image?
      Save(save_fp ,full_name,
	   pos,neg);
      pos.clear();
      neg.clear();

      cvReleaseImage(&img_in);
      cvReleaseImage(&img_bw);   
    }
  }
  return -1;
}
