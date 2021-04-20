#ifndef utils_hpp
#define utils_hpp

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

using namespace cv;

typedef std::vector<std::vector<Point>> contours_t;
typedef std::vector<Vec4i> hierarchy_t;
typedef std::vector<Point> contour_t;


#define RADIUS 4
#define BORDER_RADIUS 4

int mod(int a, int b);

void loadInpaintingImages(
                          const std::string& srcFilename,
                          const std::string& maskFilename,
                          Mat& srcMat,
                          Mat& maskMat,
                          Mat& grayMat);

void showMat(const String& winname, const Mat& mat, int time=5);

void getContours(const Mat& mask, contours_t& contours, hierarchy_t& hierarchy);

double getCterm(const Mat& Patch_C);

Mat getPatch(const Mat& image, const Point& p);

void getDerivatives(const Mat& grayMat, Mat& dx, Mat& dy);

Point2f getNormal(const contour_t& contour, const Point& point);

void computePriority(const contours_t& contours, const Mat& grayMat, const Mat& confidenceMat, Mat& priorityMat);

void transferPatch(const Point& psiHatQ, const Point& psiHatP, Mat& mat, const Mat& maskMat);

Mat computeSSD(const Mat& tmplate, const Mat& source, const Mat& tmplateMask);

#endif