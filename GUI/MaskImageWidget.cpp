#include "MaskImageWidget.h"
#include "CVQT.h"
#include "Config.h"

#include "qpushbutton.h"
#include "qdebug.h"
#include "qevent.h"
#include "qpainter.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include "opencv2/imgcodecs/legacy/constants_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

extern Config config;
using namespace cv;

MaskImageWidget::MaskImageWidget(QWidget* parent) : QWidget(parent)
{
	ui.setupUi(this);

    isDrawed = true;
    drawMaskEnable = false;

    penWidth = 8;
    srcPenColor = Qt::red;
    maskPenColor = Qt::black;
}

void MaskImageWidget::paintEvent(QPaintEvent* paintevent) {

	QPainter painter(this);
	painter.drawPixmap(0, 0, srcImagePixmap);

}

void MaskImageWidget::mousePressEvent(QMouseEvent* mouseevent) {
    if (drawMaskEnable) {
        lastPoint = mouseevent->pos();
    }
    return;
}

void MaskImageWidget::mouseReleaseEvent(QMouseEvent* mouseevent) {

}

void MaskImageWidget::mouseMoveEvent(QMouseEvent* mouseevent) {
    //qDebug("%d",ui.imageDisplayLabel->pos());
    //qDebug("Xres=%d,Yres=%d", mouseevent->pos().x(), mouseevent->pos().y());
    if (drawMaskEnable) {
        QPainter srcPainter(&srcImagePixmap);
        QPainter maskPainter(&maskImagePixmap);
        QPen srcPen,maskPen;
        
        srcPen.setWidth(penWidth);
        srcPen.setColor(srcPenColor);
        maskPen.setWidth(penWidth);
        maskPen.setColor(maskPenColor);

        srcPainter.setPen(srcPen);
        maskPainter.setPen(maskPen);
        srcPainter.drawLine(lastPoint, mouseevent->pos());
        maskPainter.drawLine(lastPoint, mouseevent->pos());

        lastPoint = mouseevent->pos();
        update();
    }
    return;
}

void MaskImageWidget::makeMaskImage() {

    Mat maskImage;
    int k = 0;
    if (isDrawed) {
        k = 1;
        Mat tempmaskImage;
        Mat maskImages[3];
        tempmaskImage = CVQT::QPixmapToCvMat(maskImagePixmap, true);
        split(tempmaskImage, maskImages);
        maskImages[0].copyTo(maskImage);
        maskImage.setTo(255, maskImage);
        config.isMaskUpdate = true;
    }
    emit maskImageIsReady(maskImage, k);
}