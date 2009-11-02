#include <saliency.h>
#include <highgui.h>
#include <cv.h>

IplImage* 
ComputeSaliency(IplImage* image, int thresh, int scale) {

  double saliency_scale = int(pow(2,scale));
  IplImage* bw_im1 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U,1);
  cvCvtColor(image, bw_im1, CV_BGR2GRAY);
  IplImage* bw_im = cvCreateImage(cvSize(saliency_scale,saliency_scale), 
				  IPL_DEPTH_8U,1);
  cvResize(bw_im1, bw_im);
  cvNamedWindow("BW",1);
  cvShowImage("BW", bw_im);
  IplImage* realInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 1);
  
  IplImage* imaginaryInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 1);
  IplImage* complexInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 2);

  cvScale(bw_im, realInput, 1.0, 0.0);
  cvZero(imaginaryInput);
  cvMerge(realInput, imaginaryInput, NULL, NULL, complexInput);

  double dft_M = saliency_scale; //cvGetOptimalDFTSize( bw_im.height - 1 );
  double dft_N = saliency_scale; //cvGetOptimalDFTSize( bw_im.width - 1 );
  
  CvMat* dft_A = cvCreateMat( dft_M, dft_N, CV_32FC2 );
  IplImage* image_Re = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_32F, 1);
  IplImage* image_Im = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_32F, 1);

  // copy A to dft_A and pad dft_A with zeros
  CvMat* tmp = cvCreateMat(bw_im->width, bw_im->height, CV_8UC1);
  cvGetSubRect( dft_A, tmp, cvRect(0,0, bw_im->width, bw_im->height));
  cvCopy( complexInput, tmp, NULL );
  if(dft_A->width > bw_im->width){
    cvReleaseMat(&tmp);
    tmp = cvCreateMat(dft_N - bw_im->width, bw_im->height, CV_8UC1);
    cvGetSubRect( dft_A, tmp,cvRect(bw_im->width,0, dft_N - bw_im->width, bw_im->height));
    cvZero( tmp );
  }
    
  cvDFT( dft_A, dft_A, CV_DXT_FORWARD, complexInput->height );
  cvSplit( dft_A, image_Re, image_Im, NULL, NULL );
    
  // Compute the phase angle 
  IplImage* image_Mag = cvCreateImage(cvSize(dft_N, dft_M), IPL_DEPTH_32F, 1);
  IplImage* image_Phase = cvCreateImage(cvSize(dft_N, dft_M), IPL_DEPTH_32F, 1);
    

  //compute the phase of the spectrum
  cvCartToPolar(image_Re, image_Im, image_Mag, image_Phase, 0);
    
  IplImage* log_mag = cvCreateImage(cvSize(dft_N, dft_M), IPL_DEPTH_32F, 1);
  cvLog(image_Mag, log_mag);
  //Box filter the magnitude, then take the difference
  IplImage* image_Mag_Filt = cvCreateImage(cvSize(dft_N, dft_M), 
					   IPL_DEPTH_32F, 1);
  CvMat* filt = cvCreateMat(3,3, CV_32FC1);
  cvSet(filt,cvScalarAll(-1.0/9.0));
  cvFilter2D(log_mag, image_Mag_Filt, filt, cvPoint(-1,-1));

  cvAdd(log_mag, image_Mag_Filt, log_mag, NULL);
  cvExp(log_mag, log_mag);
  cvPolarToCart(log_mag, image_Phase, image_Re, image_Im,0);

  cvMerge(image_Re, image_Im, NULL, NULL, dft_A);
  cvDFT( dft_A, dft_A, CV_DXT_INVERSE, complexInput->height);
  cvReleaseMat(&tmp);
  tmp = cvCreateMat(bw_im->width, bw_im->height, CV_8UC1);
  
  cvGetSubRect( dft_A, tmp,  cvRect(0,0, bw_im->width, bw_im->height));
  cvCopy( tmp, complexInput, NULL );
  cvSplit(complexInput, realInput, imaginaryInput, NULL,NULL);
  double minv, maxv;
  CvPoint minl, maxl;
  cvMinMaxLoc(realInput,&minv,&maxv,&minl,&maxl);
  //cv.cvScale(realInput, realInput, 1.0/(max-min), 1.0*(-min)/(max-min));
  cvSmooth(realInput, realInput);
  double threshold = thresh/100.0*cvAvg(realInput).val[0];
  cvThreshold(realInput, realInput, threshold, 1.0, CV_THRESH_BINARY);
  IplImage* tmp_img = cvCreateImage(cvGetSize(bw_im1),IPL_DEPTH_32F, 1);
  cvResize(realInput,tmp_img);
  cvScale(tmp_img, bw_im1, 255,0);

  cvReleaseImage(&tmp_img);
  cvReleaseImage(&realInput);
  cvReleaseImage(&imaginaryInput);
  cvReleaseImage(&complexInput);
  cvReleaseMat(&dft_A);
  cvReleaseImage(&bw_im);

  cvReleaseImage(&image_Re);
  cvReleaseImage(&image_Im);
  cvReleaseMat(&tmp);

  cvReleaseImage(&image_Mag);
  cvReleaseImage(&image_Phase);
    

  cvReleaseImage(&log_mag);
  cvReleaseImage(&image_Mag_Filt);

  return bw_im1;
}

