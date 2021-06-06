#pragma once
#include <qobject.h>
#include "Config.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include "opencv2/imgcodecs/legacy/constants_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include <fstream>

using namespace cv;
using namespace std;

struct Patch {
	Point pos;
	Mat patch;
};

struct Pv {
	Point pos;
	double value;
};

struct speedup {
	bool isuseable;
	Vec2f value;
	speedup() {
		isuseable = false;
		value = Vec2f(0.0, 0.0);
	}
};

class CriminisiJob :public QObject {

	Q_OBJECT

public:
	Mat srcImage;//扩展黑边
	Mat lastSrcImage;
	Mat grayImage;

	Mat maskImage;//扩展白边
	Mat lastMaskImage;

	Mat maskErodeImage;
	Mat maskImage0;
	Mat maskErodeImage0;

	Mat resultImage;
	//Mat resultGaryImage;
	//Mat isophotesImage, gradx, grady;

	Mat confidenceImage;//扩展0边

	speedup** normalFlag;
	speedup** isophotesFlag;
	ofstream outfile;//创建文件

	int brokensize;

	bool imagesIsReady();
	bool solve();



private:
	void getMaxPriorityPatch(Patch& P);
	double computePriority(const vector<Point>& border, const Point& pos, const int order);
	double computeConfidence(const Mat& _confidenceImage, const Point& pos);
	double computeData(const vector<Point>& border, const Point& pos, const int order);
	Vec2f computeNormal(const vector<Point>& contours, const Point& pos, const int order);
	Vec2f polynomialCurveFit(vector<Point>& points);
	Vec2f computeIsophotes(const Point& pos);

	void getContoursFromMaskImage(vector<vector<Point>>& contours);

	void getMinDistancePatch(Patch& Q, const Patch& P);
	double matchTemplate_HELLINGER(const Mat& tempsrc, const Mat& temp, const Mat& mask);
	void transferPatch(const Patch& P, const Patch& Q);

	void updateAll(const Point& pos, const Point& posq);
	bool isComplete(int &perc);
	Mat getPatch(const Mat& mat, const Point& pos);
	Mat getBorderMask(const Point& pos, bool iserode);
	void initImage();

	double  psnr(Mat& I1, Mat& I2);
	double  ssim(Mat& i1, Mat& i2);

signals:
	void srcImageisready(const Mat &srcImage);
	void criminisiJobIsFinish();

public slots:
	void receiveImagefilename();
	void receiveMaskfilename();
	void receiveMaskImage(const Mat &maskImage, const int type);

};