#include "utils.hpp"
#include <opencv2\imgproc\types_c.h>

using namespace std;
using namespace cv;

int mod(int a, int b) {
	return ((a % b) + b) % b;
}

//载入图像
void loadInpaintingImages(
	const std::string& srcFilename,
	const std::string& maskFilename,
	Mat& srcMat,
	Mat& maskMat,
	Mat& grayMat)
{
	assert(srcFilename.length() && maskFilename.length());

	srcMat = imread(srcFilename, 1); // color
	maskMat = imread(maskFilename, 0);  // grayscale

	//二值化蒙版，清晰轮廓
	threshold(maskMat, maskMat, 250, 255, CV_THRESH_BINARY);

	assert(srcMat.size() == maskMat.size());
	assert(!srcMat.empty() && !maskMat.empty());

	// convert srcMat to depth CV_32F for colorspace conversions
	srcMat.convertTo(srcMat, CV_32F);
	srcMat /= 255.0f;

	//拓宽原图，延展补丁半径个长度
	copyMakeBorder(srcMat,srcMat,RADIUS,RADIUS,RADIUS,RADIUS,BORDER_CONSTANT,Scalar_<float>(0, 0, 0));

	cvtColor(srcMat, grayMat, CV_BGR2GRAY);
}

//DEBUG生成图像
void showMat(const String& winname, const Mat& mat, int time/*= 5*/)
{
	assert(!mat.empty());
	namedWindow(winname);
	imshow(winname, mat);
	waitKey(time);
	destroyWindow(winname);
}

//生成修补边界
void getContours(const Mat& mask,contours_t& contours,hierarchy_t& hierarchy)
{
	assert(mask.type() == CV_8UC1);
	findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
}

//取出补丁
Mat getPatch(const Mat& mat, const Point& p)
{
	assert(RADIUS <= p.x && p.x < mat.cols - RADIUS && RADIUS <= p.y && p.y < mat.rows - RADIUS);
	return  mat(
		Range(p.y - RADIUS, p.y + RADIUS + 1),
		Range(p.x - RADIUS, p.x + RADIUS + 1)
	);
}

//从灰度图得到梯度矩阵
void getDerivatives(const Mat& grayMat, Mat& dx, Mat& dy)
{
	assert(grayMat.type() == CV_32FC1);

	Sobel(grayMat, dx, -1, 1, 0, -1);
	Sobel(grayMat, dy, -1, 0, 1, -1);
}


/*
 * Get the unit normal of a dense list of boundary points centered around point p.
 */
Point2f getNormal(const contour_t& contour, const Point& point)
{
	int sz = (int)contour.size();

	assert(sz != 0);

	int pointIndex = (int)(std::find(contour.begin(), contour.end(), point) - contour.begin());

	assert(pointIndex != contour.size());

	if (sz == 1)
	{
		return Point2f(1.0f, 0.0f);
	}
	else if (sz < 2 * BORDER_RADIUS + 1)
	{
		// Too few points in contour to use LSTSQ regression
		// return the normal with respect to adjacent neigbourhood
		Point adj = contour[(pointIndex + 1) % sz] - contour[pointIndex];
		return Point2f(adj.y, -adj.x) / norm(adj);
	}

	// Use least square regression
	// create X and Y mat to SVD
	Mat X(Size(2, 2 * BORDER_RADIUS + 1), CV_32F);
	Mat Y(Size(1, 2 * BORDER_RADIUS + 1), CV_32F);

	assert(X.rows == Y.rows && X.cols == 2 && Y.cols == 1 && X.type() == Y.type()
		&& Y.type() == CV_32F);

	int i = mod((pointIndex - BORDER_RADIUS), sz);

	float* Xrow;
	float* Yrow;

	int count = 0;
	int countXequal = 0;
	while (count < 2 * BORDER_RADIUS + 1)
	{
		Xrow = X.ptr<float>(count);
		Xrow[0] = contour[i].x;
		Xrow[1] = 1.0f;

		Yrow = Y.ptr<float>(count);
		Yrow[0] = contour[i].y;

		if (Xrow[0] == contour[pointIndex].x)
		{
			++countXequal;
		}

		i = mod(i + 1, sz);
		++count;
	}

	if (countXequal == count)
	{
		return Point2f(1.0f, 0.0f);
	}
	// to find the line of best fit
	Mat sol;
	solve(X, Y, sol, DECOMP_SVD);

	assert(sol.type() == CV_32F);

	float slope = sol.ptr<float>(0)[0];
	Point2f normal(-slope, 1);

	return normal / norm(normal);
}

//返回补丁的C项值
double getCterm(const Mat& temp)
{
	return ((2 * RADIUS + 1) * (2 * RADIUS + 1) -sum(temp)[0]) / (double)temp.total();
}

//计算优先矩阵，C项*D项
void computePriority(const contours_t& contours, const Mat& grayMat, const Mat& confidenceMat, Mat& priorityMat)
{
	assert(grayMat.type() == CV_32FC1 && priorityMat.type() == CV_32FC1 && confidenceMat.type() == CV_32FC1);

	Mat Patch_C;
	Mat Patch_Grad;
	Mat dx, dy, gradMat;

	Point2f normal;
	Point maxPoint;
	Point2f gradient;

	double confidence;

	getDerivatives(grayMat, dx, dy);
	magnitude(dx, dy, gradMat);//得到梯度矩阵

	Mat Mask_gradMat(gradMat.size(), gradMat.type(), Scalar_<float>(0));

	gradMat.copyTo(Mask_gradMat, (confidenceMat != 0.0f));//confidenceMat被修改了

	erode(Mask_gradMat, Mask_gradMat, Mat());

	assert(Mask_gradMat.type() == CV_32FC1);

	Point point;

	for (int i = 1; i < contours.size(); ++i)
	{
		contour_t contour = contours[i];
		for (int j = 0; j < contour.size(); ++j)
		{
			point = contour[j];
			Patch_C = getPatch(confidenceMat, point);

			//计算C项
			confidence = getCterm(Patch_C);
			assert(0 <= confidence && confidence <= 1.0f);

			// get the normal to the border around point
			normal = getNormal(contour, point);


			//===============================================
			//得到
			Patch_Grad = getPatch(Mask_gradMat, point);
			minMaxLoc(Patch_Grad, NULL, NULL, NULL, &maxPoint);
			gradient = Point2f(
				-getPatch(dy, point).ptr<float>(maxPoint.y)[maxPoint.x],
				getPatch(dx, point).ptr<float>(maxPoint.y)[maxPoint.x]
			);

			// set the priority in priorityMat
			priorityMat.ptr<float>(point.y)[point.x] = std::abs((float)confidence * gradient.dot(normal));

			assert(priorityMat.ptr<float>(point.y)[point.x] >= 0);
		}
	}
}


/*
 * Transfer the values from patch centered at psiHatQ to patch centered at psiHatP in
 * mat according to maskMat.
 */
void transferPatch(const Point& psiHatQ, const Point& psiHatP, Mat& mat, const Mat& maskMat)
{
	assert(maskMat.type() == CV_8U);
	assert(mat.size() == maskMat.size());
	assert(RADIUS <= psiHatQ.x && psiHatQ.x < mat.cols - RADIUS && RADIUS <= psiHatQ.y && psiHatQ.y < mat.rows - RADIUS);
	assert(RADIUS <= psiHatP.x && psiHatP.x < mat.cols - RADIUS && RADIUS <= psiHatP.y && psiHatP.y < mat.rows - RADIUS);

	// copy contents of psiHatQ to psiHatP with mask
	getPatch(mat, psiHatQ).copyTo(getPatch(mat, psiHatP), getPatch(maskMat, psiHatP));
}

/*
 * Runs template matching with tmplate and mask tmplateMask on source.
 * Resulting Mat is stored in result.
 *
 */
Mat computeSSD(const Mat& tmplate, const Mat& source, const Mat& tmplateMask)
{
	assert(tmplate.type() == CV_32FC3 && source.type() == CV_32FC3);
	assert(tmplate.rows <= source.rows && tmplate.cols <= source.cols);
	assert(tmplateMask.size() == tmplate.size() && tmplate.type() == tmplateMask.type());

	Mat result(source.rows - tmplate.rows + 1, source.cols - tmplate.cols + 1, CV_32F, 0.0f);

	matchTemplate(source,tmplate,result,CV_TM_SQDIFF,tmplateMask);
	normalize(result, result, 0, 1, NORM_MINMAX);
	copyMakeBorder(result, result, RADIUS, RADIUS, RADIUS, RADIUS, BORDER_CONSTANT, 1.1f);

	return result;
}

