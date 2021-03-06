#include "stdafx.h"
#include "mainApp.h"

using namespace cv;

//--------------------------------------------------------------
void mainApp::run() {

	if( setup() == -1 ) {
		std::cout << "Setup failed" << std::endl;
		return;
	}
	else {
		std::cout << "Setup complete" << std::endl;
	}

	while(1) { //main loop

		bool bSuccess = videoCapture.read(raw_frame); // read a new frame from video

		//KOD DO TESTOWANIA NA OBRAZKU
		raw_frame = imread("rgb.jpg", CV_LOAD_IMAGE_COLOR); 
		if(! raw_frame.data )                           
		{
			cout <<  "Could not open or find the image" << std::endl ;
			break;
		}

		if (!bSuccess) //if not success, break loop
		{
			std::cout << "Cannot read a frame from video file" << std::endl;
			break;
		}

		update();
		draw();

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			std::cout << "esc key is pressed by user" << std::endl;
			destroyAllWindows();
			break;
		}
	}
	return;
}

//--------------------------------------------------------------
int mainApp::setup()
{
	videoCapture = VideoCapture(0); // open the video camera no. 0
	if (!videoCapture.isOpened())  // if not success, exit program
	{
		std::cout << "Cannot open the video device" << std::endl;
		return -1;
	}
	double dWidth = videoCapture.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
	double dHeight = videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
	std::cout << "Frame size : " << dWidth << " x " << dHeight << std::endl;

	win_name = "HGR";
	namedWindow(win_name, CV_WINDOW_AUTOSIZE);
	skin_detector = skin_detection();
	return 0;
}

//--------------------------------------------------------------
void mainApp::update()
{	
	skin_frame = skin_detector.detect_skin(raw_frame);
}
//--------------------------------------------------------------
void mainApp::draw() {
	imshow(win_name,raw_frame);
	imshow("Masked Skin", skin_frame);
}
//--------------------------------------------------------------