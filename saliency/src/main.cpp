#include <saliency.h>
#include <boost/program_options.hpp>
#include <iostream>

 int thresh = 120;
 int scale = 8;

void on_trackbar( int position) {
  thresh = position;
}
void on_scalebar( int position) {
  scale = position;
}

namespace po = boost::program_options;

int main(int argc, char** argv) {
  unsigned int uint_opt;
  double double_opt;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("img", po::value<std::string>(), "Load this file for detect saliency")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc),vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout <<desc<<"\n";
    return 1;
  }
  cvNamedWindow ("Camera", CV_WINDOW_AUTOSIZE);
  cvNamedWindow ("Result", CV_WINDOW_AUTOSIZE);
  cvNamedWindow ("TEST", CV_WINDOW_AUTOSIZE);
  cvCreateTrackbar("Threshold", "Result", &thresh, 1000, on_trackbar);
  cvCreateTrackbar("Scale", "Result", &scale, 10, on_scalebar);
  cvNamedWindow ("BW", CV_WINDOW_AUTOSIZE);
  // move the new window to a better place
  cvMoveWindow ("Camera", 10, 10);

  if (vm.count("img")) {
    unsigned int key = -1;
    IplImage* img_in = cvLoadImage(vm["img"].as<std::string>().c_str());
    cvNamedWindow("Input",1);
    cvShowImage("Input", img_in);

    do {
      IplImage* img_out = ComputeSaliency(img_in, thresh, scale);
      cvNamedWindow("output",1);
      cvShowImage("output", img_out);
      cvReleaseImage(&img_out);
      key = cvWaitKey(30);
    }while (key != 'q');
  }

  return -1;
}
