#include "GUI.h"
#include "qaction.h"
#include "qmenubar.h"
#include "qfiledialog.h"
#include "qdebug.h"
#include "qevent.h"
#include "qpainter.h"
#include "qpushbutton.h"

#include "Config.h"

GUI::GUI(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    QMenuBar* menubar = ui.menuBar;
    QAction* actionopen = ui.actionOpen;
    QPushButton* testPushButton = ui.testPushButton;
    QPushButton* restorePushButton = ui.restorePushButton;
    QPushButton* drawMaskPushButton = ui.drawMaskPushButton;
    QPushButton* loadMaskPushButton = ui.loadMaskPushButton;
    MaskImageWidget* maskImageWidget = ui.maskImageWidget;

    connect(drawMaskPushButton, &QPushButton::clicked, this, [=]() {
        maskImageWidget->drawMaskEnable = drawMaskPushButton->isChecked();
        });

    connect(actionopen, &QAction::triggered, this, [=]() {
        QString filename = QFileDialog::getOpenFileName(this, "openfile");
        emit GUI::sendfilename(filename);
        });

    connect(loadMaskPushButton, &QPushButton::clicked, this, [=]() {
        QString filename = QFileDialog::getOpenFileName(this, "openfile");
        if (!filename.isEmpty()) {
            maskImageWidget->isDrawed = false;
            drawMaskPushButton->setEnabled(false);
            emit GUI::sendmaskfilename(filename);
        }
        });

    connect(restorePushButton, &QPushButton::clicked, maskImageWidget, &MaskImageWidget::makeMaskImage);
    connect(maskImageWidget, &MaskImageWidget::maskImageIsReady, this, &GUI::receiveMatImage);
}


//====================================slots====================================//

void GUI::receiveResultImage(const Mat& resultImage) {
    Mat qtImage;
    cvtColor(resultImage, qtImage, COLOR_BGR2RGB);
    QImage disImage = QImage((const unsigned char*)(qtImage.data), qtImage.cols, qtImage.rows, QImage::Format_RGB888);
    QSize tempsize = disImage.size();

    ui.maskImageWidget->setFixedSize(tempsize);
    ui.maskImageWidget->srcImagePixmap = QPixmap::fromImage(disImage.scaled(ui.maskImageWidget->size(), Qt::KeepAspectRatio));
    ui.maskImageWidget->update();
    ui.mainLayout->activate();

    ui.drawMaskPushButton->setEnabled(false);
}

void GUI::showMatImage(const Mat& Image) {
    Mat qtImage;
    cvtColor(Image, qtImage, COLOR_BGR2RGB);
    QImage disImage = QImage((const unsigned char*)(qtImage.data), qtImage.cols, qtImage.rows, qtImage.cols * qtImage.channels(), QImage::Format_RGB888);
    QSize tempsize = disImage.size();
    ui.srcImageWidget->setFixedSize(tempsize);
    ui.maskImageWidget->setFixedSize(tempsize);

    ui.srcImageWidget->srcImagePixmap = QPixmap::fromImage(disImage.scaled(ui.srcImageWidget->size(), Qt::KeepAspectRatio));
    ui.maskImageWidget->srcImagePixmap = QPixmap::fromImage(disImage.scaled(ui.maskImageWidget->size(), Qt::KeepAspectRatio));
    ui.maskImageWidget->maskImagePixmap = QPixmap(tempsize);

    ui.srcImageWidget->update();
    ui.maskImageWidget->update();

    ui.mainLayout->activate();
    setFixedSize(sizeHint());
}

void GUI::receiveMatImage(const Mat& maskImage, const int type) {
    emit maskImageIsReady(maskImage, type);
}