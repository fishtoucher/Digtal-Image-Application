#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include<string>
#include<qobject.h>
#include<iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

class Config : public QObject{

	Q_OBJECT

private:
	int patchsize;//ֻ��3,5,7

	string srcfilename;
	string maskfilename;
	

	int computePriorityMethod;//1 �˷���2 A*C+B*D
	double _A, _B;
	double confidenceFactor;


	int computeConfidenceMethod;//1 ԭʼ��2 ��1-a��c+a
	int computeNormalMethod;//1 Sobel;2 ��ϣ���С���˷���
	int compiteDistanceMethod;//1 SSD;2 ����

	int SobelSize;

public:
	Mat result;
	Mat nextImage;
	double SSIMresult;
	double PSNRresult;

	bool isMaskUpdate;
	bool isSrcUpdate;

public:
	explicit Config(QObject* parent = 0);

	int getSobelSize();
	void setSobelSize(int size);

	int getpatchsize();
	bool setpatchsize(int size);

	string getsrcfilename();
	bool setsrcfilename(string& name);

	string getmaskfilename();
	bool setmaskfilename(string& name);

	void setComputePriorityMethod(int value,double A,double B);
	int getComputePriorityMethod();

	void setConfidenceMethod(int value,double A);
	int getConfidenceMethod();

	void setComputeNormalMethod(int value);
	int getComputeNormalMethod();

	void setCompiteDistanceMethod(int value);
	int getCompiteDistanceMethod();

	pair<double, double> getAB();
	double getConfidenceFactor();

	bool isShowProcess;
signals:
	void filenameisready();
	void maskfilenameisready();

	void processBarValue(int v);
	void configJobIsFinish(const Mat& result);

	void newImage(const int type);

public slots:
	void recvivefilename(QString filename);
	void recvivemaskfilename(QString filename);

	void receiveCriminisiJob();
};

#endif