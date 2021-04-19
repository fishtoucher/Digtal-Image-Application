#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include <iostream>
#include <string>
#include "GUI.hpp"
#ifndef DEBUG
#define DEBUG 0
#endif

using namespace std;

string windowName = "Origin image";
cv::Point mousePrePoint;
cv::Mat colorMat;

void onMouse(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN) {
        mousePrePoint = cv::Point(x, y);
    }
    else if (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON)) {
        cv::Mat &mask = *(cv::Mat *)userdata;
        cv::Point pt(x, y);
        cv::line(mask, mousePrePoint, pt, cv::Scalar(0,0,0), 5, cv::LINE_4, 0);
        mousePrePoint = pt;
        colorMat.setTo(0, mask<1);
        cv::imshow(windowName, colorMat);
    }
};

int main(int argc, char** argv) {

    std::string colorFilename, maskFilename;

    if (argc == 2) {
        colorFilename = argv[1];
    }
    else {
        std::cerr << "ERROR:image empty!" << std::endl;
        return -1;
    }

    cv::Mat grayMat, maskMat;
    loadInpaintingImages(
        colorFilename,
        //maskFilename,
        colorMat,
        //maskMat,
        grayMat
    );

    //交互GUI部分
    cv::Mat tempMaskMat(colorMat.size(), CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    cv::imshow(windowName, colorMat);
    cv::setMouseCallback(windowName, onMouse,(void *)&tempMaskMat);
    for (;;) { if (cv::waitKey(50) == 'q') { break; } }
    cv::setMouseCallback(windowName, NULL);
    cv::Mat maskImages[3];
    cv::split(tempMaskMat, maskImages);
    maskImages[0].copyTo(maskMat);

    cv::Mat confidenceMat;
    maskMat.convertTo(confidenceMat, CV_32F);
    confidenceMat /= 255.0f;

    cv::copyMakeBorder(colorMat, colorMat,
        RADIUS, RADIUS, RADIUS, RADIUS,
        cv::BORDER_CONSTANT,
        cv::Scalar_<float>(0, 0, 0));
    cv::copyMakeBorder(maskMat, maskMat,
        RADIUS, RADIUS, RADIUS, RADIUS,
        cv::BORDER_CONSTANT, 255);
    cv::copyMakeBorder(confidenceMat, confidenceMat,
        RADIUS, RADIUS, RADIUS, RADIUS,
        cv::BORDER_CONSTANT, 0.0001f);
    cv::cvtColor(colorMat, grayMat, CV_BGR2GRAY);

    contours_t contours;            // 轮廓点
    hierarchy_t hierarchy;          // 辅助定义轮廓

    cv::Mat priorityMat(
        confidenceMat.size(),
        CV_32FC1
    );  // priority value matrix for each contour point

    assert(
        colorMat.size() == grayMat.size() &&
        colorMat.size() == confidenceMat.size() &&
        colorMat.size() == maskMat.size()
    );

    cv::Point psiHatP;          // psiHatP - point of highest confidence

    cv::Mat psiHatPColor;       // color patch around psiHatP

    cv::Mat psiHatPConfidence;  // confidence patch around psiHatP
    double confidence;          // confidence of psiHatPConfidence

    cv::Point psiHatQ;          // psiHatQ - point of closest patch

    cv::Mat result;             // holds result from template matching
    cv::Mat erodedMask;         // eroded mask

    cv::Mat templateMask;       // mask for template match (3 channel)

    //防止取值区域与边界样本块有交叉
    cv::erode(maskMat, erodedMask, cv::Mat(), cv::Point(-1, -1), RADIUS);

    cv::Mat drawMat;


    // main loop
    const size_t area = maskMat.total();

    while (cv::countNonZero(maskMat) != area)   // end when target is filled
    {
        // set priority matrix to -.1, lower than 0 so that border area is never selected
        priorityMat.setTo(-0.1f);

        // get the contours of mask
        getContours((maskMat == 0), contours, hierarchy);

        if (DEBUG) {
            drawMat = colorMat.clone();
        }

        // compute the priority for all contour points
        computePriority(contours, grayMat, confidenceMat, priorityMat);

        // get the patch with the greatest priority
        cv::minMaxLoc(priorityMat, NULL, NULL, NULL, &psiHatP);
        psiHatPColor = getPatch(colorMat, psiHatP);
        psiHatPConfidence = getPatch(confidenceMat, psiHatP);

        cv::Mat confInv = (psiHatPConfidence != 0.0f);
        confInv.convertTo(confInv, CV_32F);
        confInv /= 255.0f;
        // get the patch in source with least distance to psiHatPColor wrt source of psiHatP
        cv::Mat mergeArrays[3] = { confInv, confInv, confInv };
        cv::merge(mergeArrays, 3, templateMask);
        result = computeSSD(psiHatPColor, colorMat, templateMask);

        // set all target regions to 1.1, which is over the maximum value possilbe
        // from SSD
        result.setTo(1.1f, erodedMask == 0);
        // get minimum point of SSD between psiHatPColor and colorMat
        cv::minMaxLoc(result, NULL, NULL, &psiHatQ);

        assert(psiHatQ != psiHatP);

        if (DEBUG) {
            cv::rectangle(drawMat, psiHatP - cv::Point(RADIUS, RADIUS), psiHatP + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(255, 0, 0));
            cv::rectangle(drawMat, psiHatQ - cv::Point(RADIUS, RADIUS), psiHatQ + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(0, 0, 255));
            showMat("red - psiHatQ", drawMat, 1000);
        }
        // updates
        // copy from psiHatQ to psiHatP for each colorspace
        transferPatch(psiHatQ, psiHatP, grayMat, (maskMat == 0));
        transferPatch(psiHatQ, psiHatP, colorMat, (maskMat == 0));

        // fill in confidenceMat with confidences C(pixel) = C(psiHatP)
        confidence = computeConfidence(psiHatPConfidence);
        assert(0 <= confidence && confidence <= 1.0f);
        // update confidence
        psiHatPConfidence.setTo(confidence, (psiHatPConfidence == 0.0f));
        // update maskMat
        maskMat = (confidenceMat != 0.0f);
    }

    showMat("final result", colorMat, 0);
    return 0;
}