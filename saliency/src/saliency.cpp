#include <saliency.h>
#include <highgui.h>
#include <cv.h>

IplImage* 
ComputeSaliency(IplImage* image, int thresh, int scale) {
  //given a one channel image
  unsigned int size = floor(pow(2,scale)); //the size to do teh  saliency @

  IplImage* bw_im = cvCreateImage(cvSize(size,size), 
				  IPL_DEPTH_8U,1);
  cvResize(image, bw_im);
  IplImage* realInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 1);
  
  IplImage* imaginaryInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 1);
  IplImage* complexInput = cvCreateImage( cvGetSize(bw_im), IPL_DEPTH_32F, 2);

  cvScale(bw_im, realInput, 1.0/255.0);
  cvZero(imaginaryInput);
  cvMerge(realInput, imaginaryInput, NULL, NULL, complexInput);
  CvMat* dft_A = cvCreateMat( size, size, CV_32FC2 );

  // copy A to dft_A and pad dft_A with zeros
  CvMat tmp;
  cvGetSubRect( dft_A, &tmp, cvRect(0,0, size,size));
  cvCopy( complexInput, &tmp );
  //  cvZero(&tmp);

  cvDFT( dft_A, dft_A, CV_DXT_FORWARD, size );
  cvSplit( dft_A, realInput, imaginaryInput, NULL, NULL );
  // Compute the phase angle 
  IplImage* image_Mag = cvCreateImage(cvSize(size, size), IPL_DEPTH_32F, 1);
  IplImage* image_Phase = cvCreateImage(cvSize(size, size), IPL_DEPTH_32F, 1);
    

  //compute the phase of the spectrum
  cvCartToPolar(realInput, imaginaryInput, image_Mag, image_Phase, 0);
  
  IplImage* log_mag = cvCreateImage(cvSize(size, size), IPL_DEPTH_32F, 1);
  cvLog(image_Mag, log_mag);
  //Box filter the magnitude, then take the difference

  IplImage* log_mag_Filt = cvCreateImage(cvSize(size, size), 
					   IPL_DEPTH_32F, 1);
  CvMat* filt = cvCreateMat(3,3, CV_32FC1);
  cvSet(filt,cvScalarAll(1.0/9.0));
  cvFilter2D(log_mag, log_mag_Filt, filt);
  cvReleaseMat(&filt);

  cvSub(log_mag, log_mag_Filt, log_mag);
  
  cvExp(log_mag, image_Mag);
   
  cvPolarToCart(image_Mag, image_Phase, realInput, imaginaryInput,0);
  cvExp(log_mag, image_Mag);

  cvMerge(realInput, imaginaryInput, NULL, NULL, dft_A);
  cvDFT( dft_A, dft_A, CV_DXT_INV_SCALE, size);

  cvAbs(dft_A, dft_A);
  cvMul(dft_A,dft_A, dft_A);
  cvGetSubRect( dft_A, &tmp,  cvRect(0,0, size,size));
  cvCopy( &tmp, complexInput);
  cvSplit(complexInput, realInput, imaginaryInput, NULL,NULL);

  IplImage* result_image = cvCreateImage(cvGetSize(image),IPL_DEPTH_32F, 1);
  double minv, maxv;
  CvPoint minl, maxl;
  cvSmooth(realInput,realInput);
  cvSmooth(realInput,realInput);
  cvMinMaxLoc(realInput,&minv,&maxv,&minl,&maxl);
  printf("Max value %lf, min %lf\n", maxv,minv);
  cvScale(realInput, realInput, 1.0/(maxv-minv), 1.0*(-minv)/(maxv-minv));
  cvResize(realInput, result_image);
  double threshold = thresh/100.0*cvAvg(realInput).val[0];
  cvThreshold(result_image, result_image, threshold, 1.0, CV_THRESH_BINARY);
  IplImage* final_result = cvCreateImage(cvGetSize(image),IPL_DEPTH_8U, 1);
  cvScale(result_image, final_result, 255.0, 0.0);
  cvReleaseImage(&result_image);
  //cvReleaseImage(&realInput);
  cvReleaseImage(&imaginaryInput);
  cvReleaseImage(&complexInput);
  cvReleaseMat(&dft_A);
  cvReleaseImage(&bw_im);

  cvReleaseImage(&image_Mag);
  cvReleaseImage(&image_Phase);

  cvReleaseImage(&log_mag);
  cvReleaseImage(&log_mag_Filt);
  cvReleaseImage(&bw_im);
  return final_result;
  //return bw_im;
}

/*
IplImage* 
ComputeSaliency(IplImage* image, int thresh, int scale) {

  double saliency_scale = int(pow(2,scale));
  IplImage* bw_im1 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U,1);
  cvCvtColor(image, bw_im1, CV_BGR2GRAY);
  IplImage* bw_im = cvCreateImage(cvSize(saliency_scale,saliency_scale), 
				  IPL_DEPTH_8U,1);
  cvResize(bw_im1, bw_im);
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

*/
