#include "GUI.h"
#include <qfile.h>
#include <qdebug.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include "opencv2/imgcodecs/legacy/constants_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include <iostream>
#include <vector>

using namespace cv;
using namespace std;


bool polynomial_curve_fit(std::vector<cv::Point>& key_point, int n, cv::Mat& A)
{
    //Number of key points
    int N = key_point.size();

    //构造矩阵X
    cv::Mat X = cv::Mat::zeros(n + 1, n + 1, CV_64FC1);
    for (int i = 0; i < n + 1; i++)
    {
        for (int j = 0; j < n + 1; j++)
        {
            for (int k = 0; k < N; k++)
            {
                X.at<double>(i, j) = X.at<double>(i, j) +
                    std::pow(key_point[k].x, i + j);
            }
        }
    }

    //构造矩阵Y
    cv::Mat Y = cv::Mat::zeros(n + 1, 1, CV_64FC1);
    for (int i = 0; i < n + 1; i++)
    {
        for (int k = 0; k < N; k++)
        {
            Y.at<double>(i, 0) = Y.at<double>(i, 0) +
                std::pow(key_point[k].x, i) * key_point[k].y;
        }
    }

    A = cv::Mat::zeros(n + 1, 1, CV_64FC1);
    //求解矩阵A
    cv::solve(X, Y, A, cv::DECOMP_LU);
    return true;
}

int main()
{
    Mat src;
    src = imread("E:/Visual_Studio_code/Digtal Image/GUI/Image/BlackRound.png");

    Mat grayImage, edge;
    cvtColor(src, grayImage, COLOR_BGR2GRAY);
    blur(grayImage, edge, Size(3, 3));
    Canny(edge, edge, 3, 9, 3);
    imshow("Canny算法轮廓提取效果", edge);
    
    vector<vector<Point>> contour;
    findContours(edge, contour, 1, 1);
    vector<Point> mycontour = contour[1];

    Mat answer(edge.size(),CV_64FC1);
    //Mat gradx;
    //Mat grady;
    //Mat angle;
    //Sobel(edge, gradx, CV_64FC1, 1, 0, 3);
    //Sobel(edge, grady, CV_64FC1, 0, 1, 3);
    //phase(gradx, grady, angle, true);

    Mat ROI_test = imread("E:/Visual_Studio_code/Digtal Image/GUI/test.png",IMREAD_GRAYSCALE);

    for (int i = 0; i < mycontour.size(); i++) {
        int x = mycontour[i].x;
        int y = mycontour[i].y;

        Mat ROI_gradx, ROI_grady, ROI_angle;
        Mat ROI = ROI_test(Rect(x - 1, y - 1, 3, 3));
        Sobel(ROI, ROI_gradx, CV_64FC1, 1, 0, 3);
        Sobel(ROI, ROI_grady, CV_64FC1, 0, 1, 3);
        phase(ROI_gradx, ROI_grady, ROI_angle, true);
        answer.at<double>(y, x) = ROI_angle.at<double>(1, 1);

        //double dx = (double)gradx.ptr<uchar>(y)[x-1];
        //double dy = (double)grady.ptr<uchar>(y)[x-1];
        //if (dx == 0.0) {
        //    answer.at<double>(x, y) = -1234.0;
        //}
        //else {
        //    answer.at<double>(x, y) = dy / dx;
        //}
    }

    //for (int i = 0; i < mycontour.size() - 21; i++) {
    //    vector<Point> points;
    //    Mat A;
    //    for (int j = 0; j < 21; j++) {
    //        points.push_back(Point(mycontour[i + j].x, mycontour[i + j].y));
    //    }
    //    polynomial_curve_fit(points, 3, A);
    //    double a0, a1, a2, a3, x;
    //    a0 = A.at<double>(0, 0);
    //    a1 = A.at<double>(1, 0);
    //    a2 = A.at<double>(2, 0);
    //    a3 = A.at<double>(3, 0);
    //    x = mycontour[i + 2].x;
    //    double det = a1 + 2.0 * a2 * x + 3.0 * a3 * x * x;
    //    answer.at<double>(mycontour[i + 10].x, mycontour[i + 10].y) = det;
    //}
    //////创建用于绘制的深蓝色背景图像
    //cv::Mat image = cv::Mat::zeros(480, 640, CV_8UC3);
    //image.setTo(cv::Scalar(0, 0, 100));

    //////输入拟合点
    //std::vector<cv::Point> points;
    //points.push_back(cv::Point(10., 10.));
    //points.push_back(cv::Point(10., 20.));
    //points.push_back(cv::Point(10., 30.));
    //points.push_back(cv::Point(10., 40.));
    //points.push_back(cv::Point(10., 50.));
    //points.push_back(cv::Point(10., 60.));

    //////将拟合点绘制到空白图上
    //for (int i = 0; i < points.size(); i++)
    //{
    //    cv::circle(image, points[i], 5, cv::Scalar(0, 0, 255), 2, 8, 0);
    //}

    //////绘制折线
    //cv::polylines(image, points, false, cv::Scalar(0, 255, 255), 1, 8, 0);

    //cv::Mat A;

    //polynomial_curve_fit(points, 3, A);
    //std::cout << "A = " << A << std::endl;

    //std::vector<cv::Point> points_fitted;

    //for (int x = 0; x < 400; x++)
    //{
    //    double y = A.at<double>(0, 0) + A.at<double>(1, 0) * x +
    //        A.at<double>(2, 0) * std::pow(x, 2) + A.at<double>(3, 0) * std::pow(x, 3);

    //    points_fitted.push_back(cv::Point(x, y));
    //}
    //cv::polylines(image, points_fitted, false, cv::Scalar(255, 255, 255), 1, 8, 0);

    //cv::imshow("image", image);

    //cv::waitKey(0);
    return 0;
}

//int main(int argc, char *argv[])
//{
//    QApplication a(argc, argv);
//    GUI w;
//    w.show();
//    return a.exec();
//}
