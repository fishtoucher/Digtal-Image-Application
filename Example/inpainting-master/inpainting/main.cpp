#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>
#include "utils.hpp"

using namespace std;

const string saveFile = "E://Visual_Studio_code//Result//Test_a&b//";
string windowName = "Origin image";
cv::Point mousePrePoint;
cv::Mat colorMat;

void onMouse(int event, int x, int y, int flags, void* userdata)
{
	if (event == cv::EVENT_LBUTTONDOWN) {
		mousePrePoint = cv::Point(x, y);
	}
	else if (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON)) {
		cv::Mat& mask = *(cv::Mat*)userdata;
		cv::Point pt(x, y);
		cv::line(mask, mousePrePoint, pt, cv::Scalar(0, 0, 0), 5, cv::LINE_4, 0);
		mousePrePoint = pt;
		colorMat.setTo(0, mask < 1);
		cv::imshow(windowName, colorMat);
	}
};

int Test(int argc, char** argv, double a, double b)
{
	std::string colorFilename, maskFilename = "";
	cv::Mat grayMat, maskMat;

	if (argc == 3 && WORKTYEP == WORKTYPE_CMD) {
		colorFilename = argv[1];
		maskFilename = argv[2];
		loadInpaintingImages(colorFilename, maskFilename, colorMat, maskMat, grayMat);
	}
	else if (argc > 1 && WORKTYEP == WORKTYPE_GUI) {
		colorFilename = argv[1];
		loadInpaintingImages(colorFilename, maskFilename, colorMat, maskMat, grayMat);
		//交互GUI部分
		cv::Mat tempMaskMat(colorMat.size(), CV_8UC3, cv::Scalar(255, 255, 255));
		cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
		cv::imshow(windowName, colorMat);
		cv::setMouseCallback(windowName, onMouse, (void*)&tempMaskMat);
		for (;;) { if (cv::waitKey(50) == 'q') { break; } }
		cv::setMouseCallback(windowName, NULL);
		cv::Mat maskImages[3];
		cv::split(tempMaskMat, maskImages);
		maskImages[0].copyTo(maskMat);
		assert(maskMat.type() == CV_8UC1);
	}
	else {
		std::cerr << "ERROR:image empty!" << std::endl;
		return -1;
	}

	cv::Mat confidenceMat;
	maskMat.convertTo(confidenceMat, CV_32F);
	confidenceMat /= 255.0f;

	//边界扩展，防止越界
	cv::copyMakeBorder(colorMat, colorMat, RADIUS, RADIUS, RADIUS, RADIUS, cv::BORDER_CONSTANT, cv::Scalar_<float>(0, 0, 0));
	cv::copyMakeBorder(maskMat, maskMat, RADIUS, RADIUS, RADIUS, RADIUS, cv::BORDER_CONSTANT, 255);
	cv::copyMakeBorder(confidenceMat, confidenceMat, RADIUS, RADIUS, RADIUS, RADIUS, cv::BORDER_CONSTANT, 0.0001f);
	cv::cvtColor(colorMat, grayMat, CV_BGR2GRAY);

	contours_t contours;            // 轮廓点
	hierarchy_t hierarchy;          // 辅助定义轮廓

	cv::Mat priorityMat(confidenceMat.size(), CV_32FC1);  //定义优先权矩阵

	assert(colorMat.size() == grayMat.size() && colorMat.size() == confidenceMat.size() && colorMat.size() == maskMat.size());

	cv::Point psiHatP;          //记录点P位置
	cv::Point psiHatQ;          //记录点Q位置

	cv::Mat psiHatPColor;
	cv::Mat psiHatPConfidence;
	cv::Mat result;
	cv::Mat erodedMask;
	cv::Mat templateMask;

	//防止取值区域与边界样本块有交叉
	cv::erode(maskMat, erodedMask, cv::Mat(), cv::Point(-1, -1), RADIUS);

	cv::Mat drawMat;

	const size_t area = maskMat.total();

	while (cv::countNonZero(maskMat) != area) {
		priorityMat.setTo(-0.1f);
		getContours((maskMat == 0), contours, hierarchy);

		if (DEBUG) {
			drawMat = colorMat.clone();
		}

		//计算优先权
		computePriority(contours, grayMat, confidenceMat, priorityMat, a);
		cv::minMaxLoc(priorityMat, NULL, NULL, NULL, &psiHatP); //获取优先级的轮廓点位置
		psiHatPColor = getPatch(colorMat, psiHatP);             //从待修复边缘提取待匹配的块
		psiHatPConfidence = getPatch(confidenceMat, psiHatP);   //提取C矩阵的ROI
		cv::Mat confInv = (psiHatPConfidence != 0.0f);
		confInv.convertTo(confInv, CV_32F);
		confInv /= 255.0f;
		cv::Mat mergeArrays[3] = { confInv, confInv, confInv };
		cv::merge(mergeArrays, 3, templateMask);
		result = computeSSD(psiHatPColor, colorMat, templateMask);
		result.setTo(1.1f, erodedMask == 0);
		cv::minMaxLoc(result, NULL, NULL, &psiHatQ);

		assert(psiHatQ != psiHatP);
		//if (DEBUG) {
		//    cv::rectangle(drawMat, psiHatP - cv::Point(RADIUS, RADIUS), psiHatP + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(255, 0, 0));
		//    cv::rectangle(drawMat, psiHatQ - cv::Point(RADIUS, RADIUS), psiHatQ + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(0, 0, 255));
		//    showMat("red - psiHatQ", drawMat, 1000);
		//}

		//完成置换
		transferPatch(psiHatQ, psiHatP, grayMat, (maskMat == 0));
		transferPatch(psiHatQ, psiHatP, colorMat, (maskMat == 0));

		//更新C矩阵与掩膜
		double confidence = computeConfidence(psiHatPConfidence);
		assert(0 <= confidence && confidence <= 1.0f);
		psiHatPConfidence.setTo(confidence, (psiHatPConfidence == 0.0f));
		maskMat = (confidenceMat != 0.0f);
	}

	//showMat("final result", colorMat, 0);
	string num = "0";
	if (a == 1.0) {
		num += "0";
	}
	else {
		num[0] += (int)((a + 0.05) * 10);
	}
	num += (string)".jpg";
	cout << num << endl;
	saveImage(colorMat, saveFile + "test" + num);
	return 1;
}

int main(int argc, char** argv) {

	for (double a = 1.0; a <= 1.0; a += 0.1) {
		Test(argc, argv, a, 1.0 - a);
	}
	return 0;
}