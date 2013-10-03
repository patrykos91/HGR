#include "StdAfx.h"
#include "skin_detection.h"


skin_detection::skin_detection()
{
		theta_thresh = 8.0;
	    hist_bins = Scalar(50,50); 
        low_range = Scalar(0.2,0.3); 
        high_range = Scalar(0.4,0.5);
        range_dist[0] = high_range[0] - low_range[0]; 
        range_dist[1] = high_range[1] - low_range[1];
}


skin_detection::~skin_detection(void)
{
}

Mat skin_detection::toHSV(Mat frame)
{
	Mat HSV_frame;
	cvtColor(frame,HSV_frame,COLOR_BGR2HSV); //konwersja przestrzeni barwnej
	split(HSV_frame, HSV_split); // rozdziel na kanaly
	return HSV_frame;
}

Mat skin_detection::get_bootstrap()
{
	Mat bootstrap;
	//get HSV
	this->HSV_frame = toHSV(this->frame);
	//Normalize RGB 
	this->nRGB_frame = NormalizeRGB(this->frame);

	//take the pixels that are inside the ranges in both colorspaces to create masks
	Mat HSV_mask, nRGB_mask;
	inRange(HSV_frame, Scalar(0,0.2*255.0,0.35*255.0), Scalar(50.0/2.0,0.68*255.0,1.0*255.0), HSV_mask); //wartosci przedzialu znalezione w internecie
	inRange(nRGB_frame, Scalar(0,0.28,0.36), Scalar(1.0,0.363,0.465), nRGB_mask); //j.w

	//combine the masks
	bootstrap = HSV_mask & nRGB_mask;
	this->mask = bootstrap; //chwilowo do testow, potem polaczyc to w 1 funkcji detect();
	return bootstrap;
}

void skin_detection::calc_hist() //liczy histogramy i normalizuje je dzielac przez liczbe pixeli skin i non skin tworzac funkcje gestosci prawdopodobienstwa
{
	/// Ustaw dla ilu (rozmiar tablicy) i z ktorych kanalow ma sie skladac histogram
	int channels[] = {1, 2};
	/// Ustaw liczbe koszykow dla kazdego kanalu
	int histSize[] = { hist_bins[0], hist_bins[1] };
	/// Ustaw zakresy dla obu kanalow takie same
	float uranges[] = { low_range[0], high_range[0] };
	float vranges[] = { low_range[1], high_range[1] };
	const float* ranges[] = { uranges, vranges };
	
	bool uniform = true; //unifikuj rozmiar binu w histogramie 
	bool accumulate = false; //czysc za kazdym razem
	
	//skin_Histogram = calc_2D_hist(nRGB_frame,mask,channels,Scalar(250, 250),Scalar(0, 0),Scalar(1, 1));	
	//non_skin_Histogram = calc_2D_hist(nRGB_frame,~mask,channels,Scalar(250, 250),Scalar(0, 0),Scalar(1, 1));	

	//liczy 2 wymiarowe histogramy dla kanalow 1 i 2
	calcHist(&nRGB_frame,1,channels,mask,skin_Histogram,2,histSize,ranges,uniform,accumulate);
	calcHist(&nRGB_frame,1,channels,~mask,non_skin_Histogram,2,histSize,ranges,uniform,accumulate);

	float skin_pixels = countNonZero(mask); //zlicza wartosci niezerowe czyli pikseli ktore maja kolor skory
	float non_skin_pixels = countNonZero(~mask); //tak samo jw. ale dla "odwroconej" maski

	for (int ubin=0; ubin < histSize[1]; ubin++) { //ubin i vbin to koszyki w obrazku , dla zakresu wartosci pixela 0-250
		for (int vbin = 0; vbin < histSize[2]; vbin++) {
			if (skin_Histogram.at<float>(ubin,vbin) > 0) { 
				skin_Histogram.at<float>(ubin,vbin) /= skin_pixels; //normalizacja w liczba pixeli skin i non skin
			}
			if (non_skin_Histogram.at<float>(ubin,vbin) > 0) {
				non_skin_Histogram.at<float>(ubin,vbin) /= non_skin_pixels;
			}
		}
	}

}

Mat skin_detection::train() {

		Mat_<Vec3f> nrgb = nRGB_frame.reshape(3, this->frame.rows*this->frame.cols);
		Mat_<uchar> result_mask(nrgb.size());
		
		for (int i=0; i<nrgb.rows; i++) {
            if (nrgb.at<Vec3f>(i)[1] < 0 || nrgb(i)[1] > 0 ||
                nrgb(i)[2] < 1 || nrgb(i)[2] > 1)
            {
                result_mask(i) = 0;
                continue;
            }
			int gbin = cvRound((nrgb(i)[1] - 0)/range_dist[0] * hist_bins[0]);
			int rbin = cvRound((nrgb(i)[2] - low_range[1])/range_dist[1] * hist_bins[1]);
			float skin_hist_val = skin_Histogram.at<float>(gbin,rbin);
			if (skin_hist_val > 0) {
				float non_skin_hist_val = non_skin_Histogram.at<float>(gbin,rbin);
				if (non_skin_hist_val > 0) {
					if((skin_hist_val / non_skin_hist_val) > theta_thresh)
						result_mask(i) = 255;
					else 
						result_mask(i) = 0;
				} else {
					result_mask(i) = 0;
				}
			} else {
				result_mask(i) = 0;
			}
		}
		
		return result_mask.reshape(1, this->frame.rows);
	}

//porownac histogramy policzone obiema metodami wlasna i gotowa