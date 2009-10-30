#include <saliency.h>
#include <boost/program_options.hpp>
#include <iostream>


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
  if (vm.count("img")) {
    IplImage* img_in = cvLoadImage(vm["img"].as<std::string>().c_str());
    cvNamedWindow("Input",1);
    cvShowImage("Input", img_in);
    IplImage* img_out = ComputeSaliency(img_in);
    cvNamedWindow("output",1);
    cvShowImage("output", img_out);
    cvWaitKey(-1);
  }

  return -1;
}
