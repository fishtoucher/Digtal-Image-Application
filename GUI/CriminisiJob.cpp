#include "CriminisiJob.h"
#include <ctime>
#include <math.h>

#include <fstream>
#include "qdebug.h"
#include "QtConcurrent"
#include "qmessagebox.h"

using namespace cv;
using namespace std;

extern Config config;

bool cmp2(const Pv& a, const Pv& b) {
	return a.value < b.value;
}

bool CriminisiJob::solve() {

	outfile.open("data.txt");

	if (!imagesIsReady()) {
		return false;
	}

	initImage();
	config.nextImage = resultImage.clone();

	int looptimes(0);//修补次数
	int process(0);//完成进度
	int r = config.getpatchsize() / 2;
	brokensize = countNonZero(maskImage == 0);

	while (isComplete(process) == false) {

		emit config.processBarValue(process);//进度条触发
		looptimes += 1;
		Patch P,Q;

		getMaxPriorityPatch(P);
		getMinDistancePatch(Q, P);
		transferPatch(P, Q);

		Mat drawMat = resultImage.clone();
		rectangle(drawMat, P.pos - Point(r, r), P.pos + Point(r + 1, r + 1), cv::Scalar(255, 0, 0));
		rectangle(drawMat, Q.pos - Point(r, r), Q.pos + Point(r + 1, r + 1), cv::Scalar(0, 0, 255));
		config.nextImage = drawMat.clone();
		emit config.newImage(1);
		if (config.isShowProcess) {
			QThread::msleep(0);
		}

		updateAll(P.pos, Q.pos);
	}

	//更新进度条的100%进度
	emit config.processBarValue(process);

	//关闭文件流
	outfile.close();

	//计算SSIM与PSNR
	double src_res_ssim = ssim(srcImage, resultImage);
	double src_res_psnr = psnr(srcImage, resultImage);

	//装配Config
	config.SSIMresult = src_res_ssim;
	config.PSNRresult = src_res_psnr;
	config.result = resultImage.clone();

	//内存释放
	int rows = srcImage.rows;
	int cols = srcImage.cols;
	for (int i = 0; i < cols; i++) {
		delete[] normalFlag[i];
		delete[] isophotesFlag[i];
	}
	delete[] normalFlag;
	delete[] isophotesFlag;

	config.isMaskUpdate = false;
	config.isSrcUpdate = false;

	emit criminisiJobIsFinish();	
	return true;
}

//==========================utils===============================//

void CriminisiJob::getMaxPriorityPatch(Patch& P) {
	vector<vector<Point>> contours;
	getContoursFromMaskImage(contours);

	double maxpriority = -1;
	int numi = contours.size();
	int r = config.getpatchsize() / 2;
	
	for (int i = 0; i < numi; i++) {
		int numj = contours[i].size();
		for (int j = 0; j < numj; j++) {
			double temp = computePriority(contours[i], contours[i][j], j);
			if (temp > maxpriority) {
				P.pos = contours[i][j];
				P.patch = getPatch(resultImage, P.pos);
				maxpriority = temp;
				
			}
		}
	}

	outfile << maxpriority << endl;//记录每次优先权的最大值

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

	return ans;
}

double CriminisiJob::computeData(const vector<Point>& border, const Point& pos, const int order) {

	Vec2f normal = computeNormal(border, pos, order);
	Vec2f isophote = computeIsophotes(pos);

	//double ans = norm(isophote);
	//注意norm的约束
	//double ans = abs(normal.dot(isophote/norm(isophote)));
	double ans = abs(normal.dot(isophote));

	return ans;
}

Vec2f CriminisiJob::computeNormal(const vector<Point>& contours, const Point& pos, const int order){

	int x = pos.x;
	int y = pos.y;
	if (normalFlag[x][y].isuseable) {
		return normalFlag[x][y].value;
	}

	int method = config.getComputeNormalMethod();
	int size = config.getSobelSize();
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

void CriminisiJob::getMinDistancePatch(Patch& Q, const Patch& P) {
	int method = config.getCompiteDistanceMethod();
	int size = config.getpatchsize();
	int rows, cols, pnum;
	double minvalue = INFINITY;
	int r = size / 2;
	Mat mask = getBorderMask(P.pos, false);
	Mat matchResult = Mat(resultImage.rows - size + 1, resultImage.cols - size + 1, CV_32FC1, 0.0);
	Mat SSDresult = Mat(resultImage.rows - size + 1, resultImage.cols - size + 1, CV_32FC1, 0.0);
	vector<Pv> points;

	switch (method) {
	case 1:
		matchTemplate(resultImage, P.patch, matchResult, CV_TM_SQDIFF, mask);
		break;
	case 2:
		matchTemplate(resultImage, P.patch, SSDresult, CV_TM_SQDIFF, mask);
		normalize(SSDresult, SSDresult, 0, 1, NORM_MINMAX);
		rows = SSDresult.rows;
		cols = SSDresult.cols;
		copyMakeBorder(SSDresult, SSDresult, r, r, r, r, BORDER_CONSTANT, 1.1);
		SSDresult.setTo(1.1, maskErodeImage0 == 0);
		SSDresult.setTo(1.1, maskErodeImage == 0);

		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				Pv temp;
				temp.pos = Point(j, i);
				temp.value = SSDresult.at<float>(i + r, j + r);
				points.push_back(temp);
			}
		}
		sort(points.begin(), points.end(), cmp2);
		pnum = 2 * sqrt(rows * rows + cols * cols);
		for (int i = 0; i < pnum; i++) {
			Point pos = points[i].pos;
			Mat ROI = srcImage(Rect(pos.x, pos.y, size, size));
			double Ba_value = matchTemplate_HELLINGER(ROI, P.patch, mask);
			double SSD_value = points[i].value;
			double temp_value = Ba_value * 0.3 + SSD_value * 0.7;
			if (temp_value < minvalue) {
				minvalue = temp_value;
				Q.pos = Point(pos.x + r, pos.y + r);
			}
		}
		Q.patch = getPatch(resultImage, Q.pos);
		return;
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

double CriminisiJob::matchTemplate_HELLINGER(const Mat& tempsrc, const Mat& temp, const Mat& mask) {

	const int channels[1] = { 0 };
	float inRanges[2] = { 0,255 };
	const float* ranges[1] = { inRanges };
	const int bins[1] = { 256 };

	Mat patchHist, srchist;
	Mat grayPatch,graySrc;

	cvtColor(temp, grayPatch, CV_BGR2GRAY);
	cvtColor(tempsrc, graySrc, CV_BGR2GRAY);
	calcHist(&grayPatch, 1, channels, mask, patchHist, 1, bins, ranges);
	calcHist(&graySrc, 1, channels, mask, srchist, 1, bins, ranges);
	double ans = compareHist(patchHist, srchist, HISTCMP_HELLINGER);
	return ans;
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

void CriminisiJob::initImage() {

	grayImage.release();
	maskErodeImage.release();
	maskImage0.release();
	maskErodeImage0.release();
	resultImage.release();
	confidenceImage.release();

	if (config.isMaskUpdate == false) {
		maskImage = lastMaskImage.clone();
	}
	else {
		lastMaskImage = maskImage;
	}

	if (config.isSrcUpdate == false) {
		srcImage = lastSrcImage.clone();
	}
	else {
		lastSrcImage = srcImage;
	}

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

//计算PSNR
double  CriminisiJob::psnr(Mat& I1, Mat& I2) {
	Mat s1;
	absdiff(I1, I2, s1);
	s1.convertTo(s1, CV_32F);
	s1 = s1.mul(s1);
	Scalar s = sum(s1);
	double sse = s.val[0] + s.val[1] + s.val[2];
	if (sse <= 1e-10)
		return 0;
	else {
		double mse = sse / (double)(I1.channels() * I1.total());
		double psnr = 10.0 * log10((255 * 255) / mse);
		return psnr;
	}
}
//计算SSIM
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
	Mat tempSrcImage;
	if (config.isSrcUpdate) {
		tempSrcImage = srcImage.clone();
	}
	else {
		tempSrcImage = lastSrcImage.clone();
	}
	int srcrows = tempSrcImage.rows;
	int srccols = tempSrcImage.cols;

	int maskrows = maskImage.rows;
	int maskcols = maskImage.cols;
	if (srcrows != maskrows || srccols != maskcols) {
		QMessageBox::critical(NULL, "warning!", "incompatible size!", QMessageBox::Yes);
		config.isMaskUpdate = false;
		maskImage.release();
		return;
	}

	for (int i = 0; i < tempSrcImage.rows; i++) {
		for (int j = 0; j < tempSrcImage.cols; j++) {
			if (maskImage.at<uchar>(i, j) <= 10) {
				tempSrcImage.at<Vec3b>(i, j) = Vec3b(0,0,0);
			}
		}
	}
	config.nextImage = tempSrcImage;

	emit loadMaskImageIsReady(2);
}

void CriminisiJob::receiveMaskImage(const Mat& srcMaskImage, const int type) {
	if (type == 1) {
		maskImage = srcMaskImage.clone();
	}
	QtConcurrent::run(this, &CriminisiJob::solve);
	//solve();
}