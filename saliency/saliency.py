#!/usr/bin/env python
import sys
import math
from opencv import cv
from opencv import highgui
# the codec existing in cvcapp.cpp,
# need to have a better way to specify them in the future
# WARNING: I have see only MPEG1VIDEO working on my computer
H263 = 0x33363255
H263I = 0x33363249
MSMPEG4V3 = 0x33564944
MPEG4 = 0x58564944
MSMPEG4V2 = 0x3234504D
MJPEG = 0x47504A4D
MPEG1VIDEO = 0x314D4950
AC3 = 0x2000
MP2 = 0x50
FLV1 = 0x31564C46

thresh = 120
scale = 8

def on_trackbar(position):
    global thresh
    thresh = position

def on_scalebar(position):
    global scale
    scale = position
    
def mask_image(image_to_mask, mask):
    ret_image = cv.cvCreateImage(cv.cvGetSize(image_to_mask), cv.IPL_DEPTH_8U,3)
    cv.cvScale(image_to_mask, ret_image, 0.5, 0.0);
    cv.cvCopy(image_to_mask, ret_image, mask)
    return ret_image
    

def compute_saliency(image):
    global thresh
    global scale
    saliency_scale = int(math.pow(2,scale));
    bw_im1 = cv.cvCreateImage(cv.cvGetSize(image), cv.IPL_DEPTH_8U,1)
    cv.cvCvtColor(image, bw_im1, cv.CV_BGR2GRAY)
    bw_im = cv.cvCreateImage(cv.cvSize(saliency_scale,saliency_scale), cv.IPL_DEPTH_8U,1)
    cv.cvResize(bw_im1, bw_im)
    highgui.cvShowImage("BW", bw_im)
    realInput = cv.cvCreateImage( cv.cvGetSize(bw_im), cv.IPL_DEPTH_32F, 1);
    imaginaryInput = cv.cvCreateImage( cv.cvGetSize(bw_im), cv.IPL_DEPTH_32F, 1);
    complexInput = cv.cvCreateImage( cv.cvGetSize(bw_im), cv.IPL_DEPTH_32F, 2);

    cv.cvScale(bw_im, realInput, 1.0, 0.0);
    cv.cvZero(imaginaryInput);
    cv.cvMerge(realInput, imaginaryInput, None, None, complexInput);

    dft_M = saliency_scale #cv.cvGetOptimalDFTSize( bw_im.height - 1 );
    dft_N = saliency_scale #cv.cvGetOptimalDFTSize( bw_im.width - 1 );

    dft_A = cv.cvCreateMat( dft_M, dft_N, cv.CV_32FC2 );
    image_Re = cv.cvCreateImage( cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);
    image_Im = cv.cvCreateImage( cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);

    # copy A to dft_A and pad dft_A with zeros
    tmp = cv.cvGetSubRect( dft_A, cv.cvRect(0,0, bw_im.width, bw_im.height));
    cv.cvCopy( complexInput, tmp, None );
    if(dft_A.width > bw_im.width):
        tmp = cv.cvGetSubRect( dft_A, cv.cvRect(bw_im.width,0, dft_N - bw_im.width, bw_im.height));
        cv.cvZero( tmp );
    
    cv.cvDFT( dft_A, dft_A, cv.CV_DXT_FORWARD, complexInput.height );
    cv.cvSplit( dft_A, image_Re, image_Im, None, None );
    
    # Compute the phase angle 
    image_Mag = cv.cvCreateImage(cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);
    image_Phase = cv.cvCreateImage(cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);
    

    #compute the phase of the spectrum
    cv.cvCartToPolar(image_Re, image_Im, image_Mag, image_Phase, 0)

    log_mag = cv.cvCreateImage(cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);
    cv.cvLog(image_Mag, log_mag)
    #Box filter the magnitude, then take the difference
    image_Mag_Filt = cv.cvCreateImage(cv.cvSize(dft_N, dft_M), cv.IPL_DEPTH_32F, 1);
    filt = cv.cvCreateMat(3,3, cv.CV_32FC1);
    cv.cvSet(filt,cv.cvScalarAll(-1.0/9.0))
    cv.cvFilter2D(log_mag, image_Mag_Filt, filt, cv.cvPoint(-1,-1))

    cv.cvAdd(log_mag, image_Mag_Filt, log_mag, None)
    cv.cvExp(log_mag, log_mag)
    cv.cvPolarToCart(log_mag, image_Phase, image_Re, image_Im,0);

    cv.cvMerge(image_Re, image_Im, None, None, dft_A)
    cv.cvDFT( dft_A, dft_A, cv.CV_DXT_INVERSE, complexInput.height)
            
    tmp = cv.cvGetSubRect( dft_A, cv.cvRect(0,0, bw_im.width, bw_im.height));
    cv.cvCopy( tmp, complexInput, None );
    cv.cvSplit(complexInput, realInput, imaginaryInput, None, None)
    min, max = cv.cvMinMaxLoc(realInput);
    #cv.cvScale(realInput, realInput, 1.0/(max-min), 1.0*(-min)/(max-min));
    cv.cvSmooth(realInput, realInput);
    threshold = thresh/100.0*cv.cvAvg(realInput)[0]
    cv.cvThreshold(realInput, realInput, threshold, 1.0, cv.CV_THRESH_BINARY)
    tmp_img = cv.cvCreateImage(cv.cvGetSize(bw_im1),cv.IPL_DEPTH_32F, 1)
    cv.cvResize(realInput,tmp_img)
    cv.cvScale(tmp_img, bw_im1, 255,0)
    return bw_im1



##    return outp_image

if __name__ == '__main__':
    print "OpenCV python"
    
    highgui.cvNamedWindow ('Camera', highgui.CV_WINDOW_AUTOSIZE)
    highgui.cvNamedWindow ('Result', highgui.CV_WINDOW_AUTOSIZE)
    highgui.cvNamedWindow ('TEST', highgui.CV_WINDOW_AUTOSIZE)
    highgui.cvCreateTrackbar('Threshold', 'Result', thresh, 1000, on_trackbar)
    highgui.cvCreateTrackbar('Scale', 'Result', thresh, 10, on_scalebar)
    highgui.cvNamedWindow ('BW', highgui.CV_WINDOW_AUTOSIZE)
       # move the new window to a better place
    highgui.cvMoveWindow ('Camera', 10, 10)

    try:
        # try to get the device number from the command line
        device = int (sys.argv [1])

        # got it ! so remove it from the arguments
        del sys.argv [1]
    except (IndexError, ValueError):
        # no device number on the command line, assume we want the 1st device
        device = 0

    if len (sys.argv) == 1:
        # no argument on the command line, try to use the camera
        capture = highgui.cvCreateCameraCapture (device)
    else:
        # we have an argument on the command line,
        # we can assume this is a file name, so open it
        capture = highgui.cvCreateFileCapture (sys.argv [1])            

    # check that capture device is OK
    if not capture:
        print "Error opening capture device"
        sys.exit (1)

    # capture the 1st frame to get some propertie on it
    frame = highgui.cvQueryFrame (capture)

    # get size of the frame
    frame_size = cv.cvGetSize (frame)

    # get the frame rate of the capture device
    fps = 30

    # create the writer
#    writer = highgui.cvCreateVideoWriter ("captured.mpg", MPEG1VIDEO,
#                                          fps, frame_size, True)

    # check the writer is OK
#    if not writer:
#        print "Error opening writer"
#        sys.exit (1)
  
    while 1:
        # do forever

        # 1. capture the current image
        frame = highgui.cvQueryFrame (capture)
        if frame is None:
            # no image captured... end the processing
            break
        highgui.cvShowImage ('Camera', frame)
        mask = compute_saliency(frame)
        highgui.cvShowImage('TEST', mask)
        frame = mask_image(frame, mask)
        highgui.cvShowImage ('Result', frame)
        # write the frame to the output file
 #       highgui.cvWriteFrame (writer, frame)

        # display the frames to have a visual output


        # handle events
        k = highgui.cvWaitKey (5)
        
        if k % 0x100 == 27:
            # user has press the ESC key, so exit
            break

    # end working with the writer
    # not working at this time... Need to implement some typemaps...
    # but exiting without calling it is OK in this simple application
    #highgui.cvReleaseVideoWriter (writer)
