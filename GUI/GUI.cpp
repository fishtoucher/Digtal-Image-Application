#include "GUI.h"
#include "qaction.h"
#include "qmenubar.h"
#include "qfiledialog.h"
#include "qdebug.h"
#include "qevent.h"
#include "qpainter.h"
#include "qpushbutton.h"
#include "qmessagebox.h"

#include "Config.h"
#include "CVQT.h"

extern Config config;

GUI::GUI(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //默认的建议配置
    confidenceFactor = 0.3;
    _A = 0.618;
    _B = 0.382;
    patchsize = 9;
    computeConfidenceMethod = 1;
    computePriorityMethod = 1;
    computeNormalMethod = 1;
    compiteDistanceMethod = 1;
    SobelSize = 5;

    isRunning = false;

    QMenuBar* menubar = ui.menuBar;
    QAction* actionopen = ui.actionOpen;
    QAction* actionsave = ui.actionSave;
    QPushButton* restorePushButton = ui.restorePushButton;
    QPushButton* drawMaskPushButton = ui.drawMaskPushButton;
    QPushButton* loadMaskPushButton = ui.loadMaskPushButton;
    QPushButton* resetPushButton = ui.resetPushButton;
    MaskImageWidget* maskImageWidget = ui.maskImageWidget;
    SrcImageWidget* srcImageWidget = ui.srcImageWidget;
    QProgressBar* progressBar = ui.progressBar;
    QRadioButton* confidenceRadioButton_No = ui.confidenceRadioButton_No;
    QRadioButton* confidenceRadioButton_Yes = ui.confidenceRadioButton_Yes;
    
    //控件初始化与选中
    progressBar->setValue(0);
    ui.PSNRLabel->setText("None");
    ui.SSIMLabel->setText("None");
    srcImageWidget->setFixedSize(450,550);
    maskImageWidget->setFixedSize(450, 550);
    setFixedSize(1500, 700);
    ui.drawMaskPushButton->setEnabled(false);
    ui.loadMaskPushButton->setEnabled(false);
    ui.resetPushButton->setEnabled(false);
    ui.srcImageCheckBox->setEnabled(false);

    //暂时隐藏
    ui.processSlider->setVisible(false);

    //置信度控件
    confidenceRadioButton_No->setChecked(true);
    ui.confidenceSlider->setEnabled(false);
    ui.confidenceSlider->setMinimum(0);
    ui.confidenceSlider->setMaximum(100);
    ui.confidenceSlider->setSingleStep(10);
    ui.confidenceSliderLabel->setNum(0);
    //样本块大小控件
    ui.patchSizeSlider->setMinimum(7);
    ui.patchSizeSlider->setMaximum(15);
    ui.patchSizeSlider->setPageStep(2);
    ui.patchSizeSlider->setSingleStep(2);
    ui.patchSizeSlider->setTickPosition(QSlider::TicksAbove);
    ui.patchSizeSlider->setValue(9);
    ui.patchSizeLabel->setNum(9);
    //优先权控件
    ui.priorityRadioButton_MUL->setChecked(true);
    ui.prioritySlider->setEnabled(false);
    ui.prioritySliderLabel->setNum(0);
    //边缘法向量计算方式
    ui.npRadioButton_Sobel5->setChecked(true);

    //最优块匹配方式
    ui.matchRadioButton_SSD->setChecked(true);


    //=========================控件间通信=========================
    
    //置信度
    connect(confidenceRadioButton_No, &QRadioButton::clicked, this, [=]() {
        ui.confidenceSlider->setEnabled(false);
        computeConfidenceMethod = 1;
        });
    connect(confidenceRadioButton_Yes, &QRadioButton::clicked, this, [=]() {
        ui.confidenceSlider->setEnabled(true);
        computeConfidenceMethod = 2;
        });
    connect(ui.confidenceSlider, &QSlider::valueChanged, ui.confidenceSliderLabel, [=]() {
        double value = ui.confidenceSlider->value();
        confidenceFactor = value / 100.0;
        ui.confidenceSliderLabel->setText(QString::number(confidenceFactor, 'f', 1));
        });
    
    //样本块尺寸
    connect(ui.patchSizeSlider, &QSlider::valueChanged, ui.patchSizeLabel, [=]() {
        double value = ui.patchSizeSlider->value();
        patchsize = value - ((int)value + 1) % 2;
        ui.patchSizeLabel->setNum(value);
        });

    //优先权设置
    connect(ui.priorityRadioButton_MUL, &QRadioButton::clicked, this, [=]() {
        ui.prioritySlider->setEnabled(false);
        computePriorityMethod = 1;
        });
    connect(ui.priorityRadioButton_ADD, &QRadioButton::clicked, this, [=]() {
        ui.prioritySlider->setEnabled(true);
        computePriorityMethod = 2;
        });
    connect(ui.prioritySlider, &QSlider::valueChanged, ui.prioritySliderLabel, [=]() {
        double value = ui.prioritySlider->value();
        value /= 100.0;
        _A = value;
        _B = 1.0 - _A;
        ui.prioritySliderLabel->setText(QString::number(value, 'f', 1));
        });

    //边缘法向量计算方式
    connect(ui.npRadioButton_Sobel3, &QRadioButton::clicked, this, [=]() {
        SobelSize = 3;
        computeNormalMethod = 1;
        });
    connect(ui.npRadioButton_Sobel5, &QRadioButton::clicked, this, [=]() {
        SobelSize = 5;
        computeNormalMethod = 1;
        });
    connect(ui.npRadioButton_Sobel7, &QRadioButton::clicked, this, [=]() {
        SobelSize = 7;
        computeNormalMethod = 1;
        });
    connect(ui.npRadioButton_Nihe, &QRadioButton::clicked, this, [=]() {
        computeNormalMethod = 2;
        });
    
    //最优块匹配方式
    connect(ui.matchRadioButton_SSD, &QRadioButton::clicked, this, [=]() {
        compiteDistanceMethod = 1;
        });
    connect(ui.matchRadioButton_Bs, &QRadioButton::clicked, this, [=]() {
        compiteDistanceMethod = 2;
        });
    connect(ui.matchRadioButton_Confidence, &QRadioButton::clicked, this, [=]() {
        compiteDistanceMethod = 3;
        });
    
    //重置按钮功能
    connect(resetPushButton, &QPushButton::clicked, this, [=]() {
        maskImageWidget->isDrawed = true;
        maskImageWidget->maskImagePixmap = QPixmap(maskImageWidget->maskImagePixmap.size());
        ui.drawMaskPushButton->setEnabled(true);
        ui.loadMaskPushButton->setEnabled(true);
        maskWidgetDrawImage(srcWidgetImage);
        });

    //显示原图按钮功能
    connect(ui.srcImageCheckBox, &QCheckBox::stateChanged, this, [=]() {
        if (srcWidgetImage.empty() || resultWidgetImage.empty()) {
            QMessageBox::critical(NULL, "warning!", "There has no SrcImage or ResultImage!", QMessageBox::Yes);
            ui.srcImageCheckBox->setChecked(!ui.srcImageCheckBox->isChecked());
            return;
        }
        if (ui.srcImageCheckBox->isChecked()) {
            
            maskWidgetDrawImage(srcWidgetImage);
        }
        else {
            int r = config.getpatchsize() / 2;
            Mat ROI = resultWidgetImage(Rect(r, r, config.nextImage.cols - 2 * r, config.nextImage.rows - 2 * r));
            maskWidgetDrawImage(ROI);
        }
        });

    connect(drawMaskPushButton, &QPushButton::clicked, this, [=]() {
        maskImageWidget->drawMaskEnable = drawMaskPushButton->isChecked();
        });

    connect(actionopen, &QAction::triggered, this, [=]() {
        if (isRunning) {
            QMessageBox::critical(NULL, "warning!", "Program is still running!", QMessageBox::Yes);
            return;
        }
        QString filename = QFileDialog::getOpenFileName(this, "openfile");
        config.isSrcUpdate = true;
        emit GUI::sendfilename(filename);
        });

    connect(actionsave, &QAction::triggered, this, [=]() {
        if (isRunning) {
            QMessageBox::critical(NULL, "warning!", "Program is still running!", QMessageBox::Yes);
            return;
        }
        if (config.result.empty()) {
            QMessageBox::critical(NULL, "warning!", "No Image!", QMessageBox::Yes);
            return;
        }
        QString filename = QFileDialog::getSaveFileName(this, "save as", "./", "Image(*.jpg)");

        int r = config.getpatchsize() / 2;
        Mat ROI = config.result(Rect(r, r, config.nextImage.cols - 2 * r, config.nextImage.rows - 2 * r));

        QImage tempImage = CVQT::cvMatToQImage(ROI);
        tempImage.save(filename);
        });

    connect(loadMaskPushButton, &QPushButton::clicked, this, [=]() {
        QString filename = QFileDialog::getOpenFileName(this, "openfile");
        if (!filename.isEmpty()) {
            maskImageWidget->isDrawed = false;
            drawMaskPushButton->setEnabled(false);
            config.isMaskUpdate = true;
            emit GUI::sendmaskfilename(filename);
        }
        });

    //设置配置文件
    connect(restorePushButton, &QPushButton::clicked, maskImageWidget, [=]() {
        if (srcWidgetImage.empty()) {
            QMessageBox::critical(NULL, "warning!", "There has no Image!", QMessageBox::Yes);
            return;
        }
        if (isRunning) {
            QMessageBox::critical(NULL, "warning!", "Program is still running!", QMessageBox::Yes);
            return;
        }
        ui.drawMaskPushButton->setEnabled(false);
        ui.loadMaskPushButton->setEnabled(false);
        ui.resetPushButton->setEnabled(false);
        ui.srcImageCheckBox->setEnabled(false);
        isRunning = true;

        config.setConfidenceMethod(computeConfidenceMethod, confidenceFactor);
        config.setpatchsize(patchsize);
        config.setComputeNormalMethod(computeNormalMethod);
        config.setCompiteDistanceMethod(compiteDistanceMethod);
        config.setComputePriorityMethod(computePriorityMethod, _A, _B);
        config.setSobelSize(SobelSize);

        ui.maskImageWidget->makeMaskImage();
        });
    connect(maskImageWidget, &MaskImageWidget::maskImageIsReady, this, &GUI::receiveMatImage);
}


//====================================slots====================================//

void GUI::updateMaskImage(const int type) {

    if (type == 1 && ui.processCheckBox->isChecked() == false) {
        config.isShowProcess = false;
        return;
    }
    if (ui.processCheckBox->isChecked()) {
        config.isShowProcess = true;
    }
    int r = config.getpatchsize() / 2;
    Mat ROI = config.nextImage(Rect(r, r, config.nextImage.cols - 2 * r, config.nextImage.rows - 2 * r));
    maskWidgetDrawImage(ROI);
}

void GUI::setProgressBar(int value) {
    ui.progressBar->setValue(value);
}

void GUI::maskWidgetDrawImage(const Mat& image) {
    Mat qtImage;
    cvtColor(image, qtImage, COLOR_BGR2RGB);
    QImage disImage = QImage((const unsigned char*)(qtImage.data), qtImage.cols, qtImage.rows, qtImage.cols * qtImage.channels(), QImage::Format_RGB888);
    QSize tempsize = disImage.size();

    ui.maskImageWidget->setFixedSize(tempsize);
    ui.maskImageWidget->srcImagePixmap = QPixmap::fromImage(disImage.scaled(ui.maskImageWidget->size(), Qt::KeepAspectRatio));
    ui.maskImageWidget->update();
    ui.mainLayout->activate();
}

void GUI::receiveResultImage(const Mat& resultImage) {
    resultWidgetImage = resultImage.clone();

    int r = config.getpatchsize() / 2;
    Mat ROI = resultWidgetImage(Rect(r, r, config.nextImage.cols - 2 * r, config.nextImage.rows - 2 * r));
    maskWidgetDrawImage(ROI);

    ui.drawMaskPushButton->setChecked(false);
    ui.maskImageWidget->drawMaskEnable = false;
    //ui.drawMaskPushButton->setEnabled(true);
    //ui.loadMaskPushButton->setEnabled(true);
    ui.resetPushButton->setEnabled(true);
    ui.srcImageCheckBox->setEnabled(true);
    ui.srcImageCheckBox->setChecked(false);
    isRunning = false;

    ui.SSIMLabel->setNum(config.SSIMresult);
    ui.PSNRLabel->setNum(config.PSNRresult);
}

void GUI::showMatImage(const Mat& Image) {
    Mat qtImage;
    cvtColor(Image, qtImage, COLOR_BGR2RGB);
    QImage disImage = QImage((const unsigned char*)(qtImage.data), qtImage.cols, qtImage.rows, qtImage.cols * qtImage.channels(), QImage::Format_RGB888);
    
    srcWidgetImage = Image.clone();
    resultWidgetImage.release();
    ui.maskImageWidget->isDrawed = true;

    ui.drawMaskPushButton->setEnabled(true);
    ui.loadMaskPushButton->setEnabled(true);
    ui.resetPushButton->setEnabled(true);

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