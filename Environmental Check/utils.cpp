#include "utils.hpp"
#include <opencv2\imgproc\types_c.h>

int mod(int a, int b) {
	return ((a % b) + b) % b;
}

void loadInpaintingImages(const std::string& colorFilename, const std::string& maskFilename, cv::Mat& colorMat, cv::Mat& maskMat, cv::Mat& grayMat) {
	colorMat = cv::imread(colorFilename, 1);
	if (!maskFilename.empty()) {
		maskMat = cv::imread(maskFilename, 0);
		assert(maskMat.type() == CV_8UC1);
	}
	colorMat.convertTo(colorMat, CV_32F);
	colorMat /= 255.0f;
}

void showMat(const cv::String &winname, const cv::Mat &mat, int time)
{
	assert(!mat.empty());
	cv::namedWindow(winname);
	cv::imshow(winname, mat);
	cv::waitKey(time);
	cv::destroyWindow(winname);
}

void saveImage(const cv::Mat &imgMat, const std::string fileUrl)
{
	assert(!imgMat.empty() && !fileUrl.empty());
	cv::imwrite(fileUrl, imgMat);
}

//边界定义
void getContours(const cv::Mat& mask, contours_t& contours, hierarchy_t& hierarchy) {
	assert(mask.type() == CV_8UC1);
	cv::findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
}

//取样本块
cv::Mat getPatch(const cv::Mat& mat, const cv::Point& p) {
	assert(RADIUS <= p.x && p.x < mat.cols - RADIUS && RADIUS <= p.y && p.y < mat.rows - RADIUS);
	return  mat(
		cv::Range(p.y - RADIUS, p.y + RADIUS + 1),
		cv::Range(p.x - RADIUS, p.x + RADIUS + 1)
	);
}

//使用3*3索贝尔算子计算梯度值
void getDerivatives(const cv::Mat& grayMat, cv::Mat& dx, cv::Mat& dy) {
	assert(grayMat.type() == CV_32FC1);
	cv::Sobel(grayMat, dx, -1, 1, 0, -1);
	cv::Sobel(grayMat, dy, -1, 0, 1, -1);
}

//最小二乘法拟合点P的法向量（迭代不够，精度仍然欠缺）
cv::Point2f getNormal(const contour_t& contour, const cv::Point& point)
{
	int sz = (int)contour.size();
	assert(sz != 0);

	int pointIndex = (int)(std::find(contour.begin(), contour.end(), point) - contour.begin());
	assert(pointIndex != contour.size());

	if (sz == 1) {
		return cv::Point2f(1.0f, 0.0f);
	}
	else if (sz < 2 * BORDER_RADIUS + 1) {
		cv::Point adj = contour[(pointIndex + 1) % sz] - contour[pointIndex];
		return cv::Point2f(adj.y, -adj.x) / cv::norm(adj);
	}

	cv::Mat X(cv::Size(2, 2 * BORDER_RADIUS + 1), CV_32F);
	cv::Mat Y(cv::Size(1, 2 * BORDER_RADIUS + 1), CV_32F);

	assert(X.rows == Y.rows && X.cols == 2 && Y.cols == 1 && X.type() == Y.type() && Y.type() == CV_32F);

	int i = mod((pointIndex - BORDER_RADIUS), sz);

	float* Xrow;
	float* Yrow;

	int count = 0;
	int countXequal = 0;
	while (count < 2 * BORDER_RADIUS + 1) {
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

	if (countXequal == count) {
		return cv::Point2f(1.0f, 0.0f);
	}
	cv::Mat sol;
	cv::solve(X, Y, sol, cv::DECOMP_SVD);

	assert(sol.type() == CV_32F);

	float slope = sol.ptr<float>(0)[0];
	cv::Point2f normal(-slope, 1);

	return normal / cv::norm(normal);
}

double computeConfidence(const cv::Mat& confidencePatch)
{
	return cv::sum(confidencePatch)[0] / (double)confidencePatch.total();
}

//计算优先权
void computePriority(const contours_t& contours, const cv::Mat& grayMat, const cv::Mat& confidenceMat, cv::Mat& priorityMat)
{
	assert(grayMat.type() == CV_32FC1 && priorityMat.type() == CV_32FC1 && confidenceMat.type() == CV_32FC1);

	cv::Mat confidencePatch;
	cv::Mat magnitudePatch;
	cv::Point2f normal;
	cv::Point maxPoint;
	cv::Point2f gradient;

	double confidence;

	cv::Mat dx, dy, magnitude;
	getDerivatives(grayMat, dx, dy);
	cv::magnitude(dx, dy, magnitude);

	cv::Mat maskedMagnitude(magnitude.size(), magnitude.type(), cv::Scalar_<float>(0));
	magnitude.copyTo(maskedMagnitude, (confidenceMat != 0.0f));
	cv::erode(maskedMagnitude, maskedMagnitude, cv::Mat());

	assert(maskedMagnitude.type() == CV_32FC1);

	cv::Point point;

	for (int i = 0; i < contours.size(); ++i)
	{
		contour_t contour = contours[i];
		for (int j = 0; j < contour.size(); ++j)
		{
			point = contour[j];

			//计算点P的C值
			confidencePatch = getPatch(confidenceMat, point);
			confidence = cv::sum(confidencePatch)[0] / (double)confidencePatch.total();
			assert(0 <= confidence && confidence <= 1.0f);

			//计算点P单位法向量Np
			normal = getNormal(contour, point);

			//计算点P样本块内的最大等照线强度
			magnitudePatch = getPatch(maskedMagnitude, point);
			cv::minMaxLoc(magnitudePatch, NULL, NULL, NULL, &maxPoint);
			gradient = cv::Point2f(
				-getPatch(dy, point).ptr<float>(maxPoint.y)[maxPoint.x],
				getPatch(dx, point).ptr<float>(maxPoint.y)[maxPoint.x]
			);

			if (PRIORITYTYPE == PRIORITYTYPE_ADD) {
				priorityMat.ptr<float>(point.y)[point.x] = std::abs((float)confidence*0.2 + gradient.dot(normal)*0.8);
			}
			else if(PRIORITYTYPE == PRIORITYTYPE_MUL){
				priorityMat.ptr<float>(point.y)[point.x] = std::abs((float)confidence * gradient.dot(normal));
			}
			assert(priorityMat.ptr<float>(point.y)[point.x] >= 0);
		}
	}
}

//进行样本块的替换
void transferPatch(const cv::Point& psiHatQ, const cv::Point& psiHatP, cv::Mat& mat, const cv::Mat& maskMat)
{
	assert(maskMat.type() == CV_8U);
	assert(mat.size() == maskMat.size());
	assert(RADIUS <= psiHatQ.x && psiHatQ.x < mat.cols - RADIUS && RADIUS <= psiHatQ.y && psiHatQ.y < mat.rows - RADIUS);
	assert(RADIUS <= psiHatP.x && psiHatP.x < mat.cols - RADIUS && RADIUS <= psiHatP.y && psiHatP.y < mat.rows - RADIUS);

	getPatch(mat, psiHatQ).copyTo(getPatch(mat, psiHatP), getPatch(maskMat, psiHatP));
}

//使用SSD评估最佳替换的块Q
cv::Mat computeSSD(const cv::Mat& tmplate, const cv::Mat& source, const cv::Mat& tmplateMask)
{
	assert(tmplate.type() == CV_32FC3 && source.type() == CV_32FC3);
	assert(tmplate.rows <= source.rows && tmplate.cols <= source.cols);
	assert(tmplateMask.size() == tmplate.size() && tmplate.type() == tmplateMask.type());

	cv::Mat result(source.rows - tmplate.rows + 1, source.cols - tmplate.cols + 1, CV_32F, 0.0f);
	cv::matchTemplate(source,
		tmplate,
		result,
		CV_TM_SQDIFF,
		tmplateMask
	);
	cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);
	cv::copyMakeBorder(result, result, RADIUS, RADIUS, RADIUS, RADIUS, cv::BORDER_CONSTANT, 1.1f);
	return result;
}

