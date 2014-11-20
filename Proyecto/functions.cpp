#include "functions.h"

Mat realceDeBordes(Mat img)
{
	// Kernel y convolución para realzar bordes con la laplaciana: aumentar las altas frecuencia
	Mat kernel_realce_laplaciana(3, 3, CV_8S);
	kernel_realce_laplaciana.at<char>(0) = -1; kernel_realce_laplaciana.at<char>(1) = -2; kernel_realce_laplaciana.at<char>(2) = -1;
	kernel_realce_laplaciana.at<char>(3) = -2; kernel_realce_laplaciana.at<char>(4) = 13; kernel_realce_laplaciana.at<char>(5) = -2;
	kernel_realce_laplaciana.at<char>(6) = -1; kernel_realce_laplaciana.at<char>(7) = -2; kernel_realce_laplaciana.at<char>(8) = -1;

	// En este caso si la imagen se satura tras sumar las altas frecuencias, filter2D lo resuelve aut.
	filter2D(img, img, CV_8U, kernel_realce_laplaciana, Point(-1, -1), 0, BORDER_DEFAULT);
	return img;
}

Mat dibujarPlazas(Mat imagen, bool ocupada, Rect& ROI1, Rect& ROI2, Rect& plaza)
{
	Scalar color;
	if (ocupada == true)
		color = Scalar(0, 0, 255);
	else
	{
		color = Scalar(0, 255, 0);
	}

	// Dibujamos los ROIs
	rectangle(imagen, ROI1, color);
	// Dependiendo del tamaño del aparcamiento puede ser que no haya ROI2
	if (ROI2.area() > 0)
	{
		rectangle(imagen, ROI2, color);
	}

	// Dibujamos las plazas
	rectangle(imagen, plaza, color);
	
	return imagen;
}