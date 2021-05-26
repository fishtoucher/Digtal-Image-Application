#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include<string>
#include<qobject.h>
#include<iostream>

using namespace std;

class Config : public QObject{

	Q_OBJECT

private:
	int patchsize;//只能3,5,7

	string srcfilename;
	string maskfilename;

	int computePriorityMethod;//1 乘法；2 A*C+B*D
	double _A, _B;
	double confidenceFactor;


	int computeConfidenceMethod;//1 原始，2 （1-a）c+a
	int computeNormalMethod;//1 Sobel;2 拟合（最小二乘法）
	int compiteDistanceMethod;//1 SSD;2 巴氏

public:
	explicit Config(QObject* parent = 0);

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

signals:
	void filenameisready();
	void maskfilenameisready();

public slots:
	void recvivefilename(QString filename);
	void recvivemaskfilename(QString filename);
};

#endif