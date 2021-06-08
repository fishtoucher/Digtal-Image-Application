#include "Config.h"
#include "qdebug.h"

Config::Config(QObject *parent):QObject(parent){

	_A = 0.618;
	_B = 0.382;
	confidenceFactor = 0.3;

	//������ߴ�
	patchsize = 9;
	//���Ŷȼ��㷽ʽ
	computeConfidenceMethod = 1;
	//����Ȩ���㷽ʽ
	computePriorityMethod = 1;
	//���������㷽ʽ
	computeNormalMethod = 1;
	//���ſ�ƥ�䷽ʽ
	compiteDistanceMethod = 1;

	isMaskUpdate = false;
	isSrcUpdate = false;

	SobelSize = 5;

	isShowProcess = false;
}

int Config::getSobelSize() {
	return SobelSize;
}
void Config::setSobelSize(int size) {
	SobelSize = size;
}

void Config::receiveCriminisiJob() {
	emit configJobIsFinish(result);
}

void Config::setConfidenceMethod(int value, double A) {
	computeConfidenceMethod = value;
	confidenceFactor = A;
}

int Config::getConfidenceMethod() {
	return computeConfidenceMethod;
}

double Config::getConfidenceFactor() {
	return confidenceFactor;
}

void Config::setCompiteDistanceMethod(int value) {
	compiteDistanceMethod = value;
}

int Config::getCompiteDistanceMethod() {
	return compiteDistanceMethod;
}

pair<double, double> Config::getAB() {
	return pair<double, double>(_A, _B);
}

void Config::setComputePriorityMethod(int value, double A = 0.3, double B = 0.7) {
	computePriorityMethod = value;
	_A = A;
	_B = B;
}

int Config::getComputePriorityMethod() {
	return computePriorityMethod;
}

void Config::setComputeNormalMethod(int value) {
	computeNormalMethod = value;
}

int Config::getComputeNormalMethod() {
	return computeNormalMethod;
}

int Config::getpatchsize() {
	return patchsize;
}

bool Config::setpatchsize(int size = 3) {
	patchsize = size;
	if (patchsize > 0) {
		return true;
	}
	else {
		return false;
	}
}

string Config::getsrcfilename() {
	return srcfilename;
}

bool Config::setsrcfilename(string &name) {
	struct stat buffer;
	if (stat(name.c_str(), &buffer) == 0) {
		srcfilename = name;
		return true;
	}
	else {
		return false;
	}
}

string Config::getmaskfilename() {
	return maskfilename;
}

bool Config::setmaskfilename(string& name) {
	struct stat buffer;
	if (stat(name.c_str(), &buffer) == 0) {
		maskfilename = name;
		return true;
	}
	else {
		return false;
	}
}

void Config::recvivefilename(QString filename) {
	string stdfilename = filename.toStdString();
	if (setsrcfilename(stdfilename)) {
		emit filenameisready();
	}
	else {
		//��ʾ���ļ�����
	}
}

void Config::recvivemaskfilename(QString filename) {
	string maskfilename = filename.toStdString();
	if (setmaskfilename(maskfilename)) {
		emit maskfilenameisready();
	}
	else {
		//��ʾ���ļ�����
	}
}