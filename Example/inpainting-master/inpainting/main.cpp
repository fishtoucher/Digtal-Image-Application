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
    
    Point psiHatP;          
    
    Mat psiHatPColor;       

    Mat psiHatPConfidence;  
    double confidence;      
    
    Point psiHatQ;          
    
    Mat result;             
    Mat erodedMask;         
    
    Mat templateMask;       
    
    dilate(maskMat, erodedMask, Mat(), Point(-1, -1), RADIUS);

    Mat drawMat;

    const size_t area = maskMat.total();
    
    while (countNonZero(maskMat)!=0)
    {
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

        Mat mergeArrays[3] = {confInv, confInv, confInv};
        merge(mergeArrays, 3, templateMask);
        result = computeSSD(psiHatPColor, srcMat, templateMask);

        result.setTo(1.1f, erodedMask);
        minMaxLoc(result, NULL, NULL, &psiHatQ);

        assert(psiHatQ != psiHatP);
        //cout << psiHatP << " "<<psiHatQ << endl;
        
        if (DEBUG) {
        rectangle(drawMat, psiHatP - Point(RADIUS, RADIUS), psiHatP + Point(RADIUS+1, RADIUS+1), Scalar(255, 0, 0));
        rectangle(drawMat, psiHatQ - Point(RADIUS, RADIUS), psiHatQ + Point(RADIUS+1, RADIUS+1), Scalar(0, 0, 255));
        showMat("red - psiHatQ", drawMat);
        }
        transferPatch(psiHatQ, psiHatP, grayMat, (maskMat == 0));
        transferPatch(psiHatQ, psiHatP, srcMat, (maskMat == 0));

        confidence = getCterm(psiHatPConfidence);

        assert(0 <= confidence && confidence <= 1.0f);
        psiHatPConfidence.setTo(0);

        maskMat = (confidenceMat != 0.0f);
	}
    
    showMat("final result", srcMat, 0);
    return 0;
}