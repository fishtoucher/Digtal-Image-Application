#pragma once

#include "ui_MaskImageWidget.h"
#include "qwidget.h"

#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

using namespace cv;

class MaskImageWidget :public QWidget
{
	Q_OBJECT

private:
	Ui::MaskImageWidget ui;

public:
	MaskImageWidget(QWidget* parent = Q_NULLPTR);

	bool isDrawed;
	bool drawMaskEnable;

	int penWidth;
	QColor srcPenColor;
	QColor maskPenColor;

	QPixmap srcImagePixmap;
	QPixmap maskImagePixmap;
	QPoint lastPoint;

	void paintEvent(QPaintEvent* paintevent);
	void mousePressEvent(QMouseEvent* mouseevent);
	void mouseReleaseEvent(QMouseEvent* mouseevent);
	void mouseMoveEvent(QMouseEvent* mouseevent);

signals:
	void maskImageIsReady(const Mat& maskImage, const int type);

public slots:
	void makeMaskImage();

private slots:
};