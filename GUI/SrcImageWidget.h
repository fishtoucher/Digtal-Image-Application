#pragma once

#include "ui_SrcImageWidget.h"
#include "SrcImageWidget.h"

class SrcImageWidget : public QWidget {
	Q_OBJECT

private:
	Ui::SrcImageWidget ui;

public:
	SrcImageWidget(QWidget* parent = Q_NULLPTR);

	QPixmap srcImagePixmap;

	void paintEvent(QPaintEvent* paintevent);

signals:

public slots:
private slots:

};