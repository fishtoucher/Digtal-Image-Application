#include "SrcImageWidget.h"
#include "qpainter.h"

SrcImageWidget::SrcImageWidget(QWidget* parent):QWidget(parent) 
{
	ui.setupUi(this);
}

void SrcImageWidget::paintEvent(QPaintEvent* paintevent) {

	QPainter painter(this);
	painter.drawPixmap(0, 0, srcImagePixmap);

}