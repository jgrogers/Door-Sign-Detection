#include <saliency.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <Blob.h>
#include <BlobResult.h>
#include <highgui.h>
#include <cv.h>

 int thresh = 350;
 int scale = 6;

void on_trackbar( int position) {
  thresh = position;
}
void on_scalebar( int position) {
  scale = position;
}

namespace po = boost::program_options;
void cvShowImageSmall(const char* name, const IplImage* img) {
  //  IplImage* img_show =cvCreateImage(cvSize(640,480), img->depth, img->nChannels);
  //  cvResize(img, img_show);
  cvShowImage(name, img);
  //  cvReleaseImage(&img_show);
  
}
CvRect GetSquareRegion(CvPoint ul, CvPoint lr) {
  int width = abs(lr.x - ul.x);
  int height = abs(lr.y - ul.y);
  int cx = ul.x + width/2;
  int cy = ul.y + height/2;
  CvRect result;
  if (width > height) {
    result = cvRect(ul.x, cy - width/2, width,width);
  }
  else {
    result = cvRect(cx - height/2, ul.y, height,height);
  }
  return result;
}
std::vector<CvRect> GetOverlappedSquareRegions(const std::vector<CvRect>& rects) {
  //merge overlapping squares
  std::vector<CvRect> out_rects = rects;
  bool overlap_found = false;
  do {
    overlap_found = false;
    std::vector<CvRect> tmp_rects;
    std::vector<bool> rect_incorporated;
    for (size_t i = 0;i<out_rects.size();i++) 
      rect_incorporated.push_back(false);
    
    size_t i = 0;
    //check each pair of rects to merge if possible
    for (std::vector<CvRect>::iterator itr = out_rects.begin();
	 itr != out_rects.end();
	 itr++) {
      if (rect_incorporated[i]) continue; //already used this one this time
      rect_incorporated[i] = true;
      int min_x = itr->x;
      int min_y = itr->y;
      int max_x = itr->x + itr->width;
      int max_y = itr->y + itr->height;
      size_t j = i+1;
      for (std::vector<CvRect>::iterator itr2 = itr+1;
	   itr2 != out_rects.end();
	   itr2++) {
	  bool x_cond = false;
	  bool y_cond = false;
	  if (
	      (itr2->x >= min_x && 
	       itr2->x <= max_x) ||
	      (itr2->x + itr2->width >= min_x &&
	       itr2->x + itr2->width <= max_x) ||
	      (min_x >= itr2->x &&
	       min_x <= itr2->x+itr2->width) ||
	      (max_x >= itr2->x && 
	       max_x <= itr2->x+itr2->width ))
	    x_cond = true;
	  if (
	      (itr2->y >= min_y && 
	       itr2->y <= max_y) ||
	      (itr2->y + itr2->height >= min_y &&
	       itr2->y + itr2->height <= max_y) ||
	      (min_y >= itr2->y &&
	       min_y <= itr2->y+itr2->height) ||
	      (max_y >= itr2->y && 
	       max_y <= itr2->y+itr2->height ))
	    y_cond = true;
	  
	  if (x_cond && y_cond) {
	    overlap_found = true;
	    rect_incorporated[j] = true;
	    min_x = MIN(min_x, itr2->x);
	    max_x = MAX(max_x, itr2->x+itr2->width);
	    min_y = MIN(min_y, itr2->y);
	    max_y = MAX(max_y, itr2->y+itr2->height);
	  }
	  j++;	  
      }
      i++;
      tmp_rects.push_back(cvRect(min_x, min_y, max_x - min_x, max_y - min_y));
    }
    out_rects = tmp_rects;
  } while(overlap_found == true);
  return out_rects;
}
int main(int argc, char** argv) {
  unsigned int uint_opt;
  double double_opt;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("img", po::value<std::string>(), "Load this file for detect saliency")
    ("saliency", "run saliency test")
    ("saliency_blobs", "run saliency test")
    ("edge", "run edge test")
    ("conv_rect", "run the convolution rectangle test")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc),vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout <<desc<<"\n";
    return 1;
  }
  cvNamedWindow("output",1);
  cvNamedWindow("Input",1);

  cvCreateTrackbar("Threshold", "output", &thresh, 1000, on_trackbar);
  cvCreateTrackbar("Scale", "output", &scale, 10, on_scalebar);
  // move the new window to a better place
  cvMoveWindow ("Camera", 10, 10);

  if (vm.count("img")) {
    unsigned int key = -1;

    do {
      IplImage* img_in = cvLoadImage(vm["img"].as<std::string>().c_str());
      IplImage* img_tmp = cvCreateImage(cvSize(1024,768), IPL_DEPTH_8U, 3);
      cvResize(img_in, img_tmp);
      cvReleaseImage(&img_in);
      img_in = img_tmp;
      cvSmooth(img_in,img_in,3);
      if (vm.count ("conv_rect")) {
	IplImage* img_tmp = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U,1);
	IplImage* img_out;// = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 1);
	cvCvtColor(img_in, img_tmp, CV_BGR2GRAY);
	//img_out = ConvRect(img_tmp, thresh, scale);
	cvShowImageSmall("Input", img_in);
	cvShowImageSmall("output",img_out);
	cvReleaseImage(&img_tmp);
	cvReleaseImage(&img_out);
      }
      if (vm.count ("edge")) {
	IplImage* img_tmp = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U,1);
	IplImage* img_out = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 1);
	cvCvtColor(img_in, img_tmp, CV_BGR2GRAY);
	cvCanny(img_tmp,img_out,thresh,thresh*2);
	
	cvShowImageSmall("Input", img_in);
	cvShowImageSmall("output", img_out);
	cvReleaseImage(&img_tmp);
	cvReleaseImage(&img_out);
      }
      if (vm.count("saliency")) {
	IplImage* tmp_img = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 1);
	cvCvtColor(img_in, tmp_img, CV_BGR2GRAY);
	IplImage* img_out = ComputeSaliency(tmp_img, thresh, scale);
	cvShowImageSmall("Input", tmp_img);
	cvReleaseImage(&tmp_img);

	cvShowImageSmall("output",img_out);
	cvReleaseImage(&img_out);
      }
      if (vm.count ("saliency_blobs")) {
	IplImage* tmp_img = cvCreateImage(cvGetSize(img_in), IPL_DEPTH_8U, 1);
	cvCvtColor(img_in, tmp_img, CV_BGR2GRAY);
	IplImage* img_out = ComputeSaliency(tmp_img, thresh, scale);
	cvReleaseImage(&tmp_img);
	IplImage* displayedImage = cvCreateImage(cvGetSize(img_out), IPL_DEPTH_8U, 3);

	CBlobResult blobs;
	int i;
	CBlob *currentBlob;
	// find non-white blobs in thresholded image
	blobs = CBlobResult( img_out, NULL, 127, 255 );
	// exclude the ones smaller than param2 value
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 20 );
	
	// get mean gray color of biggest blob
	
	// display filtered blobs
	cvMerge( img_out, img_out, img_out, NULL, displayedImage );
	printf("Found %d blobs\n", 
	       blobs.GetNumBlobs());
	std::vector<CvRect> in_rects;
	for (i = 2; i < blobs.GetNumBlobs(); i++ )
	  {
	    currentBlob = blobs.GetBlob(i);
	    //currentBlob->FillBlob( displayedImage, CV_RGB(255-i*255/blobs.GetNumBlobs(),i*255/blobs.GetNumBlobs(),0));
	    CvRect square = GetSquareRegion(cvPoint(currentBlob->minx, currentBlob->miny),
					    cvPoint(currentBlob->maxx, currentBlob->maxy));
	    in_rects.push_back(square);
	    cvRectangle(displayedImage, cvPoint(square.x, square.y),
			cvPoint(square.x + square.width, square.y + square.height),
			CV_RGB(0,0,255),3);
	  }
	std::vector<CvRect>out_rects = GetOverlappedSquareRegions(in_rects);
	for (std::vector<CvRect>::iterator itr = out_rects.begin();
	     itr != out_rects.end();
	     itr++) {
	  cvRectangle(img_in, cvPoint(itr->x, itr->y),
		      cvPoint(itr->x + itr->width, itr->y + itr->height),
		      CV_RGB(0,255,0),5);
	}
	cvShowImageSmall("Input", img_in);
	
	if (blobs.GetNumBlobs()< 200) 
	  blobs.PrintBlobs("test.out");
	cvShowImageSmall("output",displayedImage);
	cvReleaseImage(&img_out);
	cvReleaseImage(&displayedImage);
      }

      key = cvWaitKey(30);
      cvReleaseImage(&img_in);
    }while (key != 'q');
    
  }
  
  return -1;
}
