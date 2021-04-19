#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <iostream>
#include <string>
#include "utils.hpp"

#ifndef DEBUG
    #define DEBUG 0
#endif

using namespace std;

int main (int argc, char** argv) {
    std::string srcFilename, maskFilename;

    if (argc == 3) {
        srcFilename = argv[1];
        maskFilename = argv[2];
    } else {
        std::cerr << "Usage: ./inpainting colorImageFile maskImageFile" << std::endl;
        return -1;
    }

    //��ȡͼ���ļ�
    Mat srcMat, maskMat, grayMat;
    loadInpaintingImages(srcFilename,maskFilename,srcMat,maskMat,grayMat);

    //���Ŷ�C����
    Mat confidenceMat;
    maskMat.convertTo(confidenceMat, CV_32F);
    confidenceMat /= 255.0f;
    
    //������1������Ϊ0
    //cout << confidenceMat << endl;

	copyMakeBorder(maskMat, maskMat, RADIUS+1, RADIUS + 1, RADIUS + 1, RADIUS + 1, BORDER_CONSTANT, 0);
    copyMakeBorder(confidenceMat, confidenceMat, RADIUS + 1, RADIUS + 1, RADIUS + 1, RADIUS + 1, BORDER_CONSTANT, 0);

    contours_t contours;            // ������
    hierarchy_t hierarchy;          // ������������
    Mat priorityMat(confidenceMat.size(),CV_32FC1);//����ȨP����
    
    assert(srcMat.size() == grayMat.size() &&srcMat.size() == confidenceMat.size() &&srcMat.size() == maskMat.size());
    
    Point psiHatP;          // psiHatP - point of highest confidence
    
    Mat psiHatPColor;       // color patch around psiHatP

    Mat psiHatPConfidence;  // confidence patch around psiHatP
    double confidence;          // confidence of psiHatPConfidence
    
    Point psiHatQ;          // psiHatQ - point of closest patch
    
    Mat result;             // holds result from template matching
    Mat erodedMask;         // eroded mask
    
    Mat templateMask;       // mask for template match (3 channel)
    
    //==========================================================================
    // eroded mask is used to ensure that psiHatQ is not overlapping with target
    dilate(maskMat, erodedMask, Mat(), Point(-1, -1), RADIUS);

    Mat drawMat;
    
    
    // main loop
    const size_t area = maskMat.total();
    
    //while (countNonZero(maskMat) != area)   // end when target is filled
    while (countNonZero(maskMat)!=0)
    {
        //cout << countNonZero(maskMat) << endl;
        //��ʼ��P����
        priorityMat.setTo(-0.1f);
        
        //�õ��߽�
        getContours((maskMat == 0), contours, hierarchy);
        
        if (DEBUG) {
            drawMat = srcMat.clone();
        }
        
        //����P����
        computePriority(contours, grayMat, confidenceMat, priorityMat);

        //ȡ�������Ȩ�ĵ�P
        minMaxLoc(priorityMat, NULL, NULL, NULL, &psiHatP);

        psiHatPColor = getPatch(srcMat, psiHatP);
        psiHatPConfidence = getPatch(confidenceMat, psiHatP);

        Mat confInv = (psiHatPConfidence != 0.0f);
        confInv.convertTo(confInv, CV_32F);
        confInv /= 255.0f;

        // get the patch in source with least distance to psiHatPColor wrt source of psiHatP
        Mat mergeArrays[3] = {confInv, confInv, confInv};
        merge(mergeArrays, 3, templateMask);
        result = computeSSD(psiHatPColor, srcMat, templateMask);

        // set all target regions to 1.1, which is over the maximum value possilbe
        // from SSD
        result.setTo(1.1f, erodedMask);
        // get minimum point of SSD between psiHatPColor and srcMat
        minMaxLoc(result, NULL, NULL, &psiHatQ);

        assert(psiHatQ != psiHatP);
        //cout << psiHatP << " "<<psiHatQ << endl;
        
        if (DEBUG) {
        rectangle(drawMat, psiHatP - Point(RADIUS, RADIUS), psiHatP + Point(RADIUS+1, RADIUS+1), Scalar(255, 0, 0));
        rectangle(drawMat, psiHatQ - Point(RADIUS, RADIUS), psiHatQ + Point(RADIUS+1, RADIUS+1), Scalar(0, 0, 255));
        showMat("red - psiHatQ", drawMat);
        }
        // updates
        // copy from psiHatQ to psiHatP for each colorspace
        transferPatch(psiHatQ, psiHatP, grayMat, (maskMat == 0));
        transferPatch(psiHatQ, psiHatP, srcMat, (maskMat == 0));

        // fill in confidenceMat with confidences C(pixel) = C(psiHatP)
        confidence = getCterm(psiHatPConfidence);

        assert(0 <= confidence && confidence <= 1.0f);
        //=========================================================================
        // update confidence
        //psiHatPConfidence.setTo(confidence, (psiHatPConfidence == 0.0f));
        psiHatPConfidence.setTo(0);

        // update maskMat
        maskMat = (confidenceMat != 0.0f);
	}
    
    showMat("final result", srcMat, 0);
    return 0;
}