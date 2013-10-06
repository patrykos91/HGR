#include "stdafx.h"
using namespace cv;

CvHistogram * S, *N; // histogramy sk�ry (S) i niesk�ry (N)
int nauka; // ile klatek ma trwa� budowanie histogram�w
int przedzial; // ile przedzia��w ma mie� histogram
int podzielnik; // szeroko�� przedzia�u
int piksele; // odleg�o�� w pikselach od ramki twarzy
int prog; // pr�g 

/*
 * Funkcja zapisuje w macierzy prostok�t o przek�tnej mi�dzy p1 i p2 warto�ci� val
 * p1 jest lewym g�rnym punktem, p2 prawym dolnym
 */
void zapiszPunkty(CvMat* img, CvPoint p1, CvPoint p2, float val)
{
	for(int x = p1.x; x < p2.x + 1; x++)
	{
		cvSet2D(img, p1.y, x, cvScalar(val));
		cvSet2D(img, p2.y, x, cvScalar(val));
	}
	for (int y = p1.y; y < p2.y + 1; y++)
	{
		cvSet2D(img, y, p1.x, cvScalar(val));
		cvSet2D(img, y, p2.x, cvScalar(val));
	}
}

/*
 * Tworzy obraz, w kt�rym piksele o warto�ci val ustawione zostan�
 * na 255 a pozosta�e na 0
 */
IplImage* pobierzMaske(CvMat * ws, int val)
{
	IplImage * maska = cvCreateImage(cvGetSize(ws), 8, 1);

	for (int y = 0; y < ws->height; y++)
	{
		for (int x = 0; x < ws->width; x++)
			if (cvGet2D(ws, y, x).val[0] == val)
				cvSet2D(maska, y, x, cvScalarAll(255.0));
			else
				cvSet2D(maska, y, x, cvScalarAll(0.0));
	}
	return maska;
}

/*
 * Analizuje kolorowy obraz wzgl�dem wynik�w uzyskanych z
 * zakwalifikowania pikseli do odpowiednich klas - uaktualnia histogramy
 * sk�ry i niesk�ry
 */
void analizujDoHistogramu(IplImage* obraz, CvMat* ws)
{
	for (int y = 0; y < obraz->height; y++)
	{
		uchar* ptrObraz = (uchar*) (obraz->imageData + y * obraz->widthStep);
		const int* ptrWS = (const int*) (ws->data.ptr + y * ws->step);
		for (int x = 0; x < obraz->width; x++)
		{
			uchar b, g, r;

			b = ptrObraz[3 * x];
			g = ptrObraz[3 * x + 1];
			r = ptrObraz[3 * x + 2];

			int klasa = *ptrWS++;
			if (klasa == 1)
				(*cvGetHistValue_3D(S, cvRound(b / podzielnik), cvRound(g / podzielnik), cvRound(r / podzielnik)))++;
			else if (klasa == 2)
				(*cvGetHistValue_3D(N, cvRound(b / podzielnik), cvRound(g / podzielnik), cvRound(r / podzielnik)))++;

		}
	}
}

/*
 * Normalizacja histogram�w
 * Po tej operacji, wszystkie warto�ci b�d� z zakresu 0;1
 * i b�dzie mo�na je traktowa� jak warto�� prawdopodobie�stwa
 */
void normalizujHistogramy()
{
	for (int b = 0; b < przedzial; b++)
����{
��������for (int g = 0; g < przedzial; g++)
��������{
������������for (int r = 0; r < przedzial; r++)
������������{
����������������double skora = cvQueryHistValue_3D(S, b, g, r);
����������������double nieSkora = cvQueryHistValue_3D(N, b, g, r);
����������������double suma = skora + nieSkora;
����������������if (suma > 0.0)
����������������{
��������������������(*cvGetHistValue_3D(S, b, g, r)) = skora / suma;
��������������������(*cvGetHistValue_3D(N, b, g, r)) = nieSkora / suma;
����������������}
������������}
��������}
����}
}

/*
 * Ka�demu pikselowi przyporz�dkowuje prawdopodobie�stwo
 * �e jest on pikselem sk�ry i rozci�ga na zakres 0-255
 * Im ja�niejszy piksel w wyniku, tym bardziej prawdopodobne
 * jest, �e jest to piksel sk�ry
 */
IplImage * mapaPrawdopodobienstwa(IplImage * obraz)
{
����IplImage* mapa = cvCreateImage(cvGetSize(obraz), 8, 1);

����for (int y = 0; y < obraz->height; y++)
����{
��������uchar* ptrObraz = (uchar*) (obraz->imageData + y * obraz->widthStep);
��������uchar* ptrRet = (uchar*) (mapa->imageData + y * mapa->widthStep);
��������for (int x = 0; x < obraz->width; x++)
��������{
������������uchar b, g, r;

������������b = ptrObraz[3 * x];
������������g = ptrObraz[3 * x + 1];
������������r = ptrObraz[3 * x + 2];

������������double p = cvQueryHistValue_3D(S, cvRound(b / podzielnik), cvRound(g / podzielnik), cvRound(r / podzielnik));
������������ptrRet[x] = cvRound(p * 255.0);
��������}
����}
����return mapa;
}

int main(int argc, char** argv)
{
����printf("Spos�b u�ycia (wszystkie parametry opcjonalne):\n");
����printf("\t[cam] [uczenie] [przedzial] [prog] [piksele]\n");
����printf("\t\tcam -\tnumer kamery\n");
����printf("\t\tuczenie -\tliczba klatek przeznaczonych na uczenie sk�ry\n");
����printf("\t\tprzedzial -\tliczba przedzia��w histogramu (pot�ga 2)\n");
����printf("\t\tprog -\tprogowanie do pikseli o prawdopodobie�stwie prog w zakresie 0-100 lub -1 gdy bez progowania\n");
����printf("\t\tpiksele -\todsuni�cie ramki twarzy w pikselach (zobacz kod by uzyska� wi�cej szczeg��w)\n");

����CvCapture* cam = NULL;

����// czytanie parametr�w
����if (argc > 1)
��������cam = cvCreateCameraCapture(atoi(argv[1]));
����else
��������cam = cvCreateCameraCapture(0);
����if (argc > 2)
��������nauka = atoi(argv[2]);
����else
��������nauka = 10;
����if (argc > 3)
��������przedzial = atoi(argv[3]);
����else
��������przedzial = 64;
����if (argc > 4)
��������prog = cvRound(atoi(argv[4]) * 2.55);
����else
��������prog = -1;
����if (argc > 5)
��������piksele = atoi(argv[5]);
����else
��������piksele = 10;

����podzielnik = 256 / przedzial;

����// tworzenie histogram�w
����int sizes[] = {przedzial, przedzial, przedzial};
����S = cvCreateHist(3, sizes, CV_HIST_ARRAY);
����N = cvCreateHist(3, sizes, CV_HIST_ARRAY);

����cvNamedWindow("detekcja twarzy", CV_WINDOW_AUTOSIZE);
����IplImage* ramka = cvQueryFrame(cam);
����CvMat* ws = cvCreateMat(ramka->height, ramka->width, CV_32SC1);
����IplImage* kopia;
����double fps = 60;
����int odstep_miedzy_klatkami = 1000 / fps;

����CvMemStorage * storage = cvCreateMemStorage(0);
����CvHaarClassifierCascade * haar = (CvHaarClassifierCascade*) cvLoad("/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml");

����// pusta p�tla - dajemy kamerze czas na przystosowanie
����// si� do warunk�w o�wietleniowych
����while (true)
����{
��������ramka = cvQueryFrame(cam);
��������if (ramka == 0)
������������break;

��������cvShowImage("detekcja twarzy", ramka);
��������int c = cvWaitKey(odstep_miedzy_klatkami);
��������if (c == 'n')
������������break;
����}

����// proces wype�nania histogram�w
����while (nauka > 0)
����{
��������ramka = cvQueryFrame(cam);
��������if (ramka == 0)
������������break;
��������cvClearMemStorage(storage);
��������kopia = cvCloneImage(ramka);

��������// szukanie twarzy - by�o ju� kiedy� na blogu
��������double skala = 2.0;
��������IplImage *temp = cvCreateImage(cvSize(ramka->width, ramka->height), 8, 1);
��������IplImage *temp2 = cvCreateImage(cvSize(cvRound(ramka->width / skala), cvRound(ramka->height / skala)), 8, 1);
��������cvConvertImage(ramka, temp, CV_BGR2GRAY);
��������cvResize(temp, temp2, CV_INTER_LINEAR);
��������CvSeq *wynik = cvHaarDetectObjects(temp2, haar, storage, 1.1, 3, CV_HAAR_DO_CANNY_PRUNING, cvSize(50, 50));
��������cvZero(ws);

��������// analizujemy tylko pierwsz� twarz
��������if (wynik->total > 0)
��������{
������������CvRect * twarz = (CvRect*) cvGetSeqElem(wynik, 0);
			
			// ustalamy dwa prostok�ty - wewn�trzny i zewn�trzny
			// s� to prostok�ty twarzy kt�rych wszystkie boki s� odsuni�te
			// o warto�� parametru piksele do wewn�trz/na zewn�trz
			// dla obszaru twarzy/nietwarzy
			CvPoint mP1, mP2, dP1, dP2;
			dP1.x = mP1.x = twarz->x * skala;
			dP1.y = mP1.y = twarz->y * skala;
			dP2.x = mP2.x = (twarz->x + twarz->width) * skala;
			dP2.y = mP2.y = (twarz->y + twarz->height) * skala;
			mP1.x += skala * piksele;
			mP1.y += skala * piksele;
			mP2.x -= skala * piksele;
			mP2.y -= skala * piksele;
			dP1.x -= skala * piksele;
			dP1.y -= skala * piksele;
			dP2.x += skala * piksele;
			dP2.y += skala * piksele;

			// zapisujemy prostok�ty do macierzy poddawanej operacji watershed
			zapiszPunkty(ws, mP1, mP2, 1);
			zapiszPunkty(ws, dP1, dP2, 2);
������������cvWatershed(ramka, ws);
������������// analizujemy wyniki
������������analizujDoHistogramu(ramka, ws);

������������// poni�ej kolorowanie obrazu - tylko do cel�w obejrzenia wynik�w :)
������������IplImage* maska1 = pobierzMaske(ws, 1);
������������IplImage* maska2 = pobierzMaske(ws, 2);

������������cvAddS(ramka, cvScalar(0.0, 205.0, 0.0, 0.0), kopia, maska1);
������������cvAddS(ramka, cvScalar(0.0, 0.0, 205.0, 0.0), kopia, maska2);

������������cvReleaseImage(&maska1);
������������cvReleaseImage(&maska2);

��������}

��������cvShowImage("detekcja twarzy", kopia);
��������cvReleaseImage(&temp);
��������cvReleaseImage(&kopia);
��������cvReleaseImage(&temp2);
��������int c = cvWaitKey(odstep_miedzy_klatkami);
��������if (c == 'k')
������������break;
��������nauka--;
����}

����cvReleaseHaarClassifierCascade(&haar);
����cvNamedWindow("skora", CV_WINDOW_AUTOSIZE);

����// normalizujemy przygotowane histogramy
����normalizujHistogramy();

����// p�tla g��wna
����while (true)
����{
��������ramka = cvQueryFrame(cam);
��������if (ramka == 0)
������������break;

��������// ustalamy prawdopodobie�stwa
��������IplImage* mapa = mapaPrawdopodobienstwa(ramka);
��������// je�eli podali�my, �e chcemy progowa� to progujemy
��������if (prog > 0)
������������cvThreshold(mapa, mapa, prog, 255, CV_THRESH_BINARY);
��������cvShowImage("skora", mapa);
��������cvReleaseImage(&mapa);
��������int c = cvWaitKey(odstep_miedzy_klatkami);
��������if (c == 'k')
������������break;
����}

����cvReleaseMat(&ws);
����cvReleaseHist(&N);
����cvReleaseHist(&S);
����cvDestroyAllWindows();
����cvReleaseCapture(&cam);

����return EXIT_SUCCESS;
}
