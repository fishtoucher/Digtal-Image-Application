#pragma once

#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include <QtWidgets/QMainWindow>
#include "ui_GUI.h"
#include "qfiledialog.h"
#include "qmouseeventtransition.h"

using namespace cv;

class GUI : public QMainWindow
{
    Q_OBJECT

private:
    Ui::GUIClass ui;

    double _A, _B;
    double confidenceFactor;
    int patchsize;
    int computePriorityMethod;//1 乘法；2 A*C+B*D
    int computeConfidenceMethod;//1 原始，2 （1-a）c+a
    int computeNormalMethod;//1 Sobel;2 拟合（最小二乘法）
    int compiteDistanceMethod;//1 SSD;2 巴氏
    int SobelSize;

    Mat srcWidgetImage;
    Mat resultWidgetImage;

    bool isRunning;

public:
    GUI(QWidget *parent = Q_NULLPTR);

    void maskWidgetDrawImage(const Mat& image);

signals:
    void sendfilename(QString filename);
    void sendmaskfilename(QString filename);
    void maskImageIsReady(const Mat& maskImage, const int type);

public slots:
    void showMatImage(const Mat& Image);
    void receiveMatImage(const Mat& maskImage, const int type);
    void receiveResultImage(const Mat& resultImage);

    void setProgressBar(int value);

    void updateMaskImage(const int type);
};
