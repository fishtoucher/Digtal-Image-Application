#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

//int __test_opencv()
int main()
{
	string imagename = "E:\\Visual_Studio_code\\Digtal Image\\tram.jpg";//此处为你自己的图片路径
	//printf("%s\n",imagename);
	//从文件中读入图像
	Mat img = imread(imagename, 1);

	//如果读入图像失败
	if (img.empty())
	{
		printf("Can not load image\n");
		return -1;
	}
	//显示图像
	imshow("image", img);

	//此函数等待按键，按键盘任意键就返回
	waitKey();
	return 0;
}
