#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\ml\ml.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2\video\tracking.hpp>
#include <iostream>

using namespace std;
using namespace cv;

Mat realceDeBordes(Mat img);
Mat dibujarPlazas(Mat imagen, bool ocupada, Rect& ROI1, Rect& ROI2, Rect& plaza);

#endif