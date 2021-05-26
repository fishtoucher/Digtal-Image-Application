#include "CriminisiJob.h"
#include <ctime>
#include <math.h>

#include <fstream>
#include "qdebug.h"

using namespace cv;
using namespace std;

extern Config config;

bool CriminisiJob::solve() {
	outfile.open("data.txt");

	//Mat src_;
	//src_ = imread("E:/Visual_Studio_code/Digtal Image/GUI/Image/isophote/8.jpg");
	//src_.setTo(255, src_ > 125);
	//src_.setTo(0, src_ <= 125);
	//Mat grayImage_, edge_;
	//cvtColor(src_, grayImage_, COLOR_BGR2GRAY);
	//blur(grayImage_, edge_, Size(3, 3));
	//Canny(edge_, edge_, 3, 9, 3);
	//imshow("Canny算法轮廓提取效果", edge_);

	//vector<vector<Point>> contour;
	//findContours(edge_, contour, 1, 1);
	//vector<Point> mycontour = contour[1];

	//Mat answer(edge_.size(), CV_64FC1);

	//Mat ROI_test = imread("E:/Visual_Studio_code/Digtal Image/GUI/Image/isophote/8.jpg", IMREAD_GRAYSCALE);
	//vector<double> kkk;
	//int k = 7;

	//for (int i = 0; i < mycontour.size(); i++) {
	//	int x = mycontour[i].x;
	//	int y = mycontour[i].y;

	//	Mat ROI_gradx, ROI_grady, ROI_angle;
	//	Mat ROI = ROI_test(Rect(x - k / 2, y - k / 2, k, k));
	//	Sobel(ROI, ROI_gradx, CV_64FC1, 1, 0, k);
	//	Sobel(ROI, ROI_grady, CV_64FC1, 0, 1, k);
	//	phase(ROI_gradx, ROI_grady, ROI_angle, true);

	//	double tempv = ROI_angle.at<double>(k/2, k/2);
	//	kkk.push_back(tempv);
	//	answer.at<double>(y, x) = tempv;
	//}

	//sort(kkk.begin(), kkk.end());
	//for (int i = 0; i < kkk.size(); i++) {
	//	outfile << kkk[i] << endl;//输出信息到data.txt文件
	//}
























	if (!imagesIsReady()) {
		return false;
	}

	int r = config.getpatchsize() / 2;
	initImage();

	//算法计时
	clock_t caluPriorityTime(0), caluDistanceTime(0), caluTransferTime(0), updateTime(0);
	//修补次数
	int looptimes(0);
	//破损区域大小
	brokensize = countNonZero(maskImage == 0);
	//完成进度
	int process(0);

	while (isComplete(process) == false) {
		cout << process << endl;
		looptimes += 1;

		clock_t startTime,endtime;
		Patch P,Q;

		startTime = clock();
		getMaxPriorityPatch(P);
		endtime = clock();
		caluPriorityTime += endtime - startTime;

		startTime = clock();
		getMinDistancePatch(Q, P);
		endtime = clock();
		caluDistanceTime += endtime - startTime;

		startTime = clock();
		transferPatch(P, Q);
		endtime = clock();
		caluTransferTime += endtime - startTime;

		//Mat drawMat = resultImage.clone();
		//rectangle(drawMat, P.pos - Point(r, r), P.pos + Point(r + 1, r + 1), cv::Scalar(255, 0, 0));
		//rectangle(drawMat, Q.pos - Point(r, r), Q.pos + Point(r + 1, r + 1), cv::Scalar(0, 0, 255));
		//namedWindow("Debug");
		//imshow("Debug", drawMat);
		//waitKey(500);
		//destroyWindow("Debug");

		startTime = clock();
		updateAll(P.pos, Q.pos);
		endtime = clock();
		updateTime += endtime - startTime;
	}

	outfile.close();
	//计算SSIM与PSNR
	double src_res_ssim = ssim(srcImage, resultImage);
	double src_res_psnr = psnr(srcImage, resultImage);

	return true;
}

//==========================utils===============================//

void CriminisiJob::getMaxPriorityPatch(Patch& P) {
	vector<vector<Point>> contours;
	getContoursFromMaskImage(contours);

	double maxpriority = -1;
	int numi = contours.size();
	//getIsophotesImage();
	
	for (int i = 0; i < numi; i++) {
		int numj = contours[i].size();
		for (int j = 0; j < numj; j++) {
			double temp = computePriority(contours[i], contours[i][j], j);

			//cout << "Point:" << contours[i][j] << ": " << temp << endl;

			if (temp > maxpriority) {
				P.pos = contours[i][j];
				P.patch = getPatch(resultImage, P.pos);
				maxpriority = temp;
			}
		}
	}

	int dis = config.getpatchsize();
	for (int i = 0; i < numi; i++) {
		int numj = contours[i].size();
		for (int j = 0; j < numj; j++) {
			int x = contours[i][j].x;
			int y = contours[i][j].y;
			if (abs(x - P.pos.x) <= dis && abs(y - P.pos.y) <= dis) {
				normalFlag[x][y].isuseable = false;
				isophotesFlag[x][y].isuseable = false;
			}
		}
	}
}

/*
RETR_LIST 提取所有轮廓存放于list中、不建立等级关系
CHAIN_APPROX_NONE 提取轮廓的每一个像素
*/
void CriminisiJob::getContoursFromMaskImage(vector<vector<Point>> &contours) {
	findContours(maskImage == 0, contours, RETR_LIST, CHAIN_APPROX_NONE);
}

double CriminisiJob::computePriority(const vector<Point>& border, const Point& pos, const int order) {
	double ans = 0.0;
	int method = config.getComputePriorityMethod();

	switch (method) {
	case 1:
		ans = computeConfidence(confidenceImage, pos) * computeData(border, pos, order);
		break;
	case 2:
		pair<double, double> AB = config.getAB();
		ans = computeConfidence(confidenceImage, pos) * AB.first + computeData(border, pos, order) * AB.second;
		break;
	}
	return ans;
}

double CriminisiJob::computeConfidence(const Mat& _confidenceImage, const Point& pos) {

	int method = config.getConfidenceMethod();
	Mat p = getPatch(_confidenceImage, pos);
	double ans(0);

	switch (method) {
	case 1:
		ans = sum(p)[0] / (double)p.total();
		break;
	case 2:
		double factor = config.getConfidenceFactor();
		ans = (1.0-factor)*(sum(p)[0] / (double)p.total())+factor;
		break;
	}

	//cout << "Confidence:" << ans << " ";

	return ans;
}

clock_t NormalTime(0),IsophotesTime(0);
double CriminisiJob::computeData(const vector<Point>& border, const Point& pos, const int order) {

	clock_t s, e;
	s = clock();
	Vec2f normal = computeNormal(border, pos, order);
	e = clock();
	NormalTime += e - s;

	s = clock();
	Vec2f isophote = computeIsophotes(pos);
	e = clock();
	IsophotesTime += e - s;

	//double ans = norm(isophote);
	//double ans = abs(normal.dot(isophote/norm(isophote)));
	double ans = abs(normal.dot(isophote));

	//cout << "Normal:" << normal << " ";
	//cout << "Isophotes:" << isophote << " ";
	//cout << "Data:" << ans << " " << endl;

	return ans;
}

Vec2f CriminisiJob::computeNormal(const vector<Point>& contours, const Point& pos, const int order){

	int x = pos.x;
	int y = pos.y;
	if (normalFlag[x][y].isuseable) {
		return normalFlag[x][y].value;
	}

	int method = config.getComputeNormalMethod();
	int size = 5;
	int r = size / 2;
	Vec2f ans;
	Mat ROI, ROI_gradx, ROI_grady;

	switch (method) {
	case 1:
		ROI = maskImage(Rect(x - r, y - r, size, size));
		Sobel(ROI, ROI_gradx, CV_64FC1, 1, 0, size);
		Sobel(ROI, ROI_grady, CV_64FC1, 0, 1, size);
		ans = Vec2f(ROI_gradx.at<double>(r, r), ROI_grady.at<double>(r, r));
		if (ans[0] != 0 || ans[1] != 0) {
			ans /= norm(ans);
		}
		break;
	case 2:
		int contourSize = contours.size();
		if (contourSize < 2 * size) {
			ans = Vec2f(0, 0);
		}
		else {
			vector<Point> temp;
			for (int i = 0; i < 20; i++) {
				int pos = (order - 10 + i + contourSize) % contourSize;
				temp.push_back(contours[pos]);
			}
			ans = polynomialCurveFit(temp);
		}

		break;
	}

	normalFlag[x][y].isuseable = true;
	normalFlag[x][y].value = ans;
	return ans;
}

Vec2f CriminisiJob::polynomialCurveFit(vector<Point>& points)
{
	int pn = points.size();
	double x1(0), x2(0), y1(0), y2(0);

	for (int i = 0; i < pn; i++) {
		x1 += points[i].x;
		x2 += points[i].x * points[i].x;
		y1 += points[i].y;
		y2 += points[i].x * points[i].y;
	}

	//构造矩阵X
	Mat X = Mat::zeros(2, 2, CV_64FC1);
	X.at<double>(0, 0) = pn;
	X.at<double>(0, 1) = x1;
	X.at<double>(1, 0) = x1;
	X.at<double>(1, 1) = x2;

	//构造矩阵Y
	Mat Y = Mat::zeros(2, 1, CV_64FC1);
	Y.at<double>(0, 0) = y1;
	Y.at<double>(1, 0) = y2;

	//构造矩阵A
	Mat A = Mat::zeros(pn, 1, CV_64FC1);

	//求解矩阵A
	cv::solve(X, Y, A, DECOMP_LU);
	double a1 = A.at<double>(1, 0);
	//outfile << abs((atan2(-1 / a1, 1) / 3.1415926) * 180) << endl;
	if (a1 == 0.0) {
		return Vec2f(0, 0);
	}
	else {
		Vec2f ans(1, -1 / a1);
		return ans / norm(ans);
	}
	return Vec2f(0, 0);
}

Vec2f CriminisiJob::computeIsophotes(const Point& pos) {
	int x = pos.x;
	int y = pos.y;
	if (isophotesFlag[x][y].isuseable) {
		return isophotesFlag[x][y].value;
	}

	int size = config.getpatchsize();
	int r = size / 2;

	Mat ROI, ROI_gradx, ROI_grady, ROI_gary, ROI_mag;
	Point maxPoint;

	ROI = resultImage(Rect(x - r, y - r, size, size));
	Mat mask = getBorderMask(pos, true);

	cvtColor(ROI, ROI_gary, CV_BGR2GRAY);
	Sobel(ROI_gary, ROI_gradx, CV_64FC1, 1, 0, -1);
	Sobel(ROI_gary, ROI_grady, CV_64FC1, 0, 1, -1);
	magnitude(ROI_gradx, ROI_grady, ROI_mag);
	ROI_mag.setTo(0.0, mask == 0);
	minMaxLoc(ROI_mag, NULL, NULL, NULL, &maxPoint);

	double dx = ROI_gradx.at<double>(maxPoint.y, maxPoint.x);
	double dy = ROI_grady.at<double>(maxPoint.y, maxPoint.x);
	Vec2f ans = Vec2f(-dy, dx);

	isophotesFlag[x][y].isuseable = true;
	isophotesFlag[x][y].value = ans;
	return ans;
}

clock_t tempt(0);
void CriminisiJob::getMinDistancePatch(Patch& Q, const Patch& P) {
	int method = config.getCompiteDistanceMethod();
	int size = config.getpatchsize();
	int r = size / 2;
	Mat mask = getBorderMask(P.pos, false);
	Mat matchResult = Mat(resultImage.rows - size + 1, resultImage.cols - size + 1, CV_32FC1, 0.0);

	switch (method) {
	case 1:
		clock_t startTime, endtime;
		startTime = clock();
		matchTemplate(resultImage, P.patch, matchResult, CV_TM_SQDIFF, mask);
		endtime = clock();
		tempt += endtime - startTime;
		break;
	case 2:
		matchTemplate_HELLINGER(grayImage, P.patch, matchResult, mask);
		break;
	case 3:
		Mat confidecnMask = getPatch(confidenceImage, P.pos);
		confidecnMask.setTo(0.0, mask == 0);
		matchTemplate(resultImage, P.patch, matchResult, CV_TM_SQDIFF, confidecnMask);
		break;
	}

	normalize(matchResult, matchResult, 0, 1, NORM_MINMAX);
	copyMakeBorder(matchResult, matchResult, r, r, r, r, BORDER_CONSTANT, 1.1);
	matchResult.setTo(1.1, maskErodeImage0 == 0);
	matchResult.setTo(1.1, maskErodeImage == 0);
	minMaxLoc(matchResult, NULL, NULL, &Q.pos);
	Q.patch = getPatch(resultImage, Q.pos);
}

void CriminisiJob::matchTemplate_HELLINGER(const Mat& tempsrc, const Mat& temp, Mat& result, const Mat& mask) {
	int size = config.getpatchsize();
	int rows = tempsrc.rows - size + 1;
	int cols = tempsrc.cols - size + 1;

	const int channels[1] = { 0 };
	float inRanges[2] = { 0,255 };
	const float* ranges[1] = { inRanges };
	const int bins[1] = { 256 };

	Mat patchHist, srchist;
	Mat ROI_srcImage;

	Mat grayPatch;
	cvtColor(temp, grayPatch, CV_BGR2GRAY);
	calcHist(&temp, 1, channels, mask, patchHist, 1, bins, ranges);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			ROI_srcImage = tempsrc(Rect(j, i, size, size));
			calcHist(&ROI_srcImage, 1, channels, mask, srchist, 1, bins, ranges);
			double ans = compareHist(patchHist, srchist, HISTCMP_HELLINGER);
			result.at<float>(i,j)=ans;
		}
	}
}

void CriminisiJob::transferPatch(const Patch& P, const Patch& Q) {
	assert(P.pos != Q.pos);
	assert(maskImage.at<uchar>(Q.pos.y, Q.pos.x) == 255);
	assert(maskImage.at<uchar>(P.pos.y, P.pos.x) == 0);

	Mat maskp = getBorderMask(P.pos, false);
	Mat maskq = getBorderMask(Q.pos, false);
	Mat mask = maskq.setTo(0, maskp != 0);
	Q.patch.copyTo(P.patch, mask);
}

void CriminisiJob::updateAll(const Point& posp, const Point& posq) {
	Mat maskp = getBorderMask(posp, false);
	Mat maskq = getBorderMask(posq, false);
	Mat mask = maskq.setTo(0, maskp != 0);

	double lastConfidence = computeConfidence(confidenceImage, posp);

	Mat ROI_confidence = getPatch(confidenceImage, posp);
	Mat ROI_mask = getPatch(maskImage, posp);

	ROI_confidence.setTo(lastConfidence, mask != 0);
	ROI_mask.setTo(255, mask != 0);
}

bool CriminisiJob::isComplete(int &perc) {
	double remainsize = (double)countNonZero(maskImage == 0);
	perc = 100 - (remainsize / (double)brokensize) * 100;
	return remainsize == 0;
}

Mat CriminisiJob::getPatch(const Mat& mat, const Point& pos) {
	int r = config.getpatchsize()/2;
	return  mat(Range(pos.y - r, pos.y + r + 1), Range(pos.x - r, pos.x + r + 1));
}

/*
Type::CV_8UC1 Size::patchsize
*/
Mat CriminisiJob::getBorderMask(const Point& pos, bool iserode) {
	int size = config.getpatchsize();
	Mat maskFromMaskImage = getPatch(maskImage, pos);
	Mat maskFromMaskImage0 = getPatch(maskImage0, pos);

	Mat mask(size, size, CV_8UC1, 255);
	maskFromMaskImage.copyTo(mask, maskFromMaskImage == 0);
	if (iserode) {
		erode(mask, mask, Mat());
	}
	maskFromMaskImage0.copyTo(mask, maskFromMaskImage0 == 0);

	return mask;
}

//==============================functions===============================//

void CriminisiJob::initImage() {

	maskImage.setTo(255, maskImage >= 245);
	maskImage.setTo(0, maskImage <= 10);
	maskImage.convertTo(confidenceImage, CV_32F);
	confidenceImage /= 255.0f;

	srcImage.copyTo(resultImage, maskImage != 0);
	cvtColor(resultImage, grayImage, CV_BGR2GRAY);

	maskImage0 = Mat(srcImage.rows, srcImage.cols, CV_8UC1, Scalar(255));

	int size = config.getpatchsize();
	int r = size / 2;

	copyMakeBorder(srcImage, srcImage, r, r, r, r, BORDER_CONSTANT, Scalar(0, 0, 0));
	copyMakeBorder(resultImage, resultImage, r, r, r, r, BORDER_CONSTANT, Scalar(0, 0, 0));
	copyMakeBorder(grayImage, grayImage, r, r, r, r, BORDER_CONSTANT, 0);
	copyMakeBorder(maskImage, maskImage, r, r, r, r, BORDER_CONSTANT, 255);
	copyMakeBorder(maskImage0, maskImage0, r, r, r, r, BORDER_CONSTANT, 0);
	copyMakeBorder(confidenceImage, confidenceImage, r, r, r, r, BORDER_CONSTANT, 0.0f);
	erode(maskImage, maskErodeImage, Mat(), Point(-1, -1), r);
	erode(maskImage0, maskErodeImage0, Mat(), Point(-1, -1), r);

	assert(srcImage.type() == CV_8UC3);
	assert(grayImage.type() == CV_8UC1);
	assert(maskImage.type() == CV_8UC1);
	assert(maskImage0.type() == CV_8UC1);
	assert(maskErodeImage.type() == CV_8UC1);
	assert(confidenceImage.type() == CV_32FC1);
	assert(srcImage.size() == maskImage.size() 
		&& srcImage.size() == maskImage0.size() 
		&& srcImage.size() == maskErodeImage.size()
		&& srcImage.size() == grayImage.size()
		&& srcImage.size() == confidenceImage.size());

	int rows = srcImage.rows;
	int cols = srcImage.cols;

	normalFlag = new speedup * [cols];
	isophotesFlag = new speedup * [cols];
	for (int i = 0; i < cols; i++) {
		normalFlag[i] = new speedup [rows];
		isophotesFlag[i] = new speedup[rows];
	}
}

bool CriminisiJob::imagesIsReady() {
	assert(srcImage.type() == CV_8UC3);
	assert(maskImage.type() == CV_8UC1);
	return true;
}


double  CriminisiJob::psnr(Mat& I1, Mat& I2) {
	Mat s1;
	absdiff(I1, I2, s1);
	s1.convertTo(s1, CV_32F);//转换为32位的float类型，8位不能计算平方
	s1 = s1.mul(s1);
	Scalar s = sum(s1);  //计算每个通道的和
	double sse = s.val[0] + s.val[1] + s.val[2];
	if (sse <= 1e-10) // for small values return zero
		return 0;
	else {
		double mse = sse / (double)(I1.channels() * I1.total()); //  sse/(w*h*3)
		double psnr = 10.0 * log10((255 * 255) / mse);
		return psnr;
	}
}

double  CriminisiJob::ssim(Mat& i1, Mat& i2) {
	const double C1 = 6.5025, C2 = 58.5225;
	int d = CV_32F;
	Mat I1, I2;
	i1.convertTo(I1, d);
	i2.convertTo(I2, d);
	Mat I1_2 = I1.mul(I1);
	Mat I2_2 = I2.mul(I2);
	Mat I1_I2 = I1.mul(I2);
	Mat mu1, mu2;
	GaussianBlur(I1, mu1, Size(11, 11), 1.5);
	GaussianBlur(I2, mu2, Size(11, 11), 1.5);
	Mat mu1_2 = mu1.mul(mu1);
	Mat mu2_2 = mu2.mul(mu2);
	Mat mu1_mu2 = mu1.mul(mu2);
	Mat sigma1_2, sigam2_2, sigam12;
	GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;
	GaussianBlur(I2_2, sigam2_2, Size(11, 11), 1.5);
	sigam2_2 -= mu2_2;
	GaussianBlur(I1_I2, sigam12, Size(11, 11), 1.5);
	sigam12 -= mu1_mu2;
	Mat t1, t2, t3;
	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigam12 + C2;
	t3 = t1.mul(t2);

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigam2_2 + C2;
	t1 = t1.mul(t2);

	Mat ssim_map;
	divide(t3, t1, ssim_map);
	Scalar mssim = mean(ssim_map);

	double ssim = (mssim.val[0] + mssim.val[1] + mssim.val[2]) / 3;
	return ssim;
}

//================================slots==================================//

void CriminisiJob::receiveImagefilename() {
	srcImage = imread(config.getsrcfilename());
	emit srcImageisready(srcImage);
}

void CriminisiJob::receiveMaskfilename() {
	maskImage = imread(config.getmaskfilename(), 0);
}

void CriminisiJob::receiveMaskImage(const Mat& srcMaskImage, const int type) {
	if (type == 1) {
		maskImage = srcMaskImage.clone();
	}
	solve();
	emit jobIsFinish(resultImage);
}