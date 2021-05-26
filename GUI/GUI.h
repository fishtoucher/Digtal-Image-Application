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

public:
    GUI(QWidget *parent = Q_NULLPTR);

signals:
    void sendfilename(QString filename);
    void sendmaskfilename(QString filename);
    void maskImageIsReady(const Mat& maskImage, const int type);

public slots:
    void showMatImage(const Mat& Image);
    void receiveMatImage(const Mat& maskImage, const int type);
    void receiveResultImage(const Mat& resultImage);
};
