#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// 全局变量，用于存储模板图像
Mat tpl;

// 函数声明：排序矩形框的函数
void sort_box(vector<Rect> &boxes);

// 函数声明：检测缺陷的函数
void detect_defect(Mat &binary, vector<Rect> rects, vector<Rect> &defect);

int main(int argc, char** argv) {
	/*读取输入图像*/
	Mat src = imread("../../image/ce_01.jpg");
	if (src.empty()) { // 检查图像是否加载成功
		printf("could not load image file...");
		return -1;
	}
	namedWindow("input", WINDOW_AUTOSIZE); // 创建窗口显示输入图像
	imshow("input", src); // 显示输入图像

	/* 图像二值化*/
	Mat gray, binary;
	cvtColor(src, gray, COLOR_BGR2GRAY); // 转为灰度图
	threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU); // 二值化（反相(THRESH_BINARY_INV)+Otsu法(THRESH_OTSU)）,otsu是自适应阈值发，根据图像的直方图选择一个合适的阈值进行二值化。
	imshow("binary", binary); // 显示二值化结果

	/*用于形态学操作*/
	Mat se = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
	morphologyEx(binary, binary, MORPH_OPEN, se); // 形态学开操作（去噪）
	imshow("open-binary", binary);


	/* 轮廓发现*/
	vector<vector<Point>> contours; // 存储轮廓点
	vector<Vec4i> hierarchy; // 存储轮廓层次信息
	vector<Rect> rects; // 存储矩形框
	findContours(binary, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE); // 查找轮廓

	int height = src.rows; // 图像高度
	for (size_t t = 0; t < contours.size(); t++) {
		Rect rect = boundingRect(contours[t]); // 计算每个轮廓的边界矩形
		double area = contourArea(contours[t]); // 计算轮廓的面积
		if (rect.height >(height / 2)) { // 忽略高度超过图像一半的轮廓
			continue;
		}
		if (area < 150) { // 忽略面积小于150的轮廓,这里150就是设定的值了
			continue;
		}
		rects.push_back(rect); // 保存符合条件的矩形框
		//通过下面的代码可以显示轮廓的框
		//rectangle(src, rect, Scalar(0, 0, 255), 2, 8, 0);//在找到的刀片上画框，绘制轮廓的边界矩形框，线粗2，线条类型8，0不使用填充
		//drawContours(src, contours, t, Scalar(0, 0, 255), 2, 8);//绘制轮廓的边界，颜色0,0,255红色，线粗2,8是线条类型
	}
	imshow("result1", src);
	//排序，在每个轮廓的左上角写上序号
	sort_box(rects); // 对矩形框按位置进行排序
	tpl = binary(rects[1]); // 提取模板区域（选择排序后的第2个矩形框）
	for (int i = 0; i < rects.size(); i++)
	{
		putText(src, format("%d", i), rects[i].tl(), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 255, 0), 1, 8);
	}
	imshow("result2", src);

	/*缺陷检测*/
	vector<Rect> defects; // 用于存储缺陷区域
	detect_defect(binary, rects, defects); // 检测缺陷

	for (int i = 0; i < defects.size(); i++) {
		rectangle(src, defects[i], Scalar(0, 0, 255), 2, 8, 0); // 在源图像上绘制缺陷框
		putText(src, "bad", defects[i].tl(), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 255, 0), 1, 8); // 标注缺陷
	}
	imshow("detect result", src); // 显示检测结果
	imwrite("./detection_result.png", src); // 保存结果图像
	waitKey(0);
	return 0;
}

// 对矩形框进行排序（按y坐标从小到大）
void sort_box(vector<Rect> &boxes) {
	int size = boxes.size();
	for (int i = 0; i < size - 1; i++) {
		for (int j = i; j < size; j++) {
			int x = boxes[j].x;
			int y = boxes[j].y;
			if (y < boxes[i].y) { // 按y坐标排序
				Rect temp = boxes[i];
				boxes[i] = boxes[j];
				boxes[j] = temp;
			}
		}
	}
}

// 检测缺陷
/*
	功能：发现差异，图像差值中的白色像素点数量超过50认为缺陷
	rects:轮廓发现得到的矩形框
*/
void detect_defect(Mat &binary, vector<Rect> rects, vector<Rect> &defect) {
	int h = tpl.rows; // 模板的高度
	int w = tpl.cols; // 模板的宽度
	int size = rects.size();
	for (int i = 0; i < size; i++) {
		// 构建差异图
		Mat roi = binary(rects[i]); // 提取矩形区域，roi感兴趣区域
		resize(roi, roi, tpl.size()); // 调整大小与模板一致
		Mat mask;
		subtract(tpl, roi, mask); // 模板与ROI做差
		Mat se = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
		morphologyEx(mask, mask, MORPH_OPEN, se); // 形态学开操作
		threshold(mask, mask, 0, 255, THRESH_BINARY); // 再次二值化
		imshow("mask", mask);//显示差值图像
		

		// 根据差异图查找缺陷,遍历mask中并统计白色像素点（值为255）的数量，表示差异的大小
		int count = 0; // 统计白色像素点数量
		for (int row = 0; row < h; row++) {
			for (int col = 0; col < w; col++) {
				int pv = mask.at<uchar>(row, col); // 获取像素值
				if (pv == 255) {
					count++;
				}
			}
		}

		// 填充一个像素宽边界（用于边界检测），创建一个比原图 mask 多一圈像素的矩阵 m1。将 mask 的数据复制到 m1 的中心位置。目的：为边界检测提供缓冲，避免误判边界为缺陷。
		int mh = mask.rows + 2;
		int mw = mask.cols + 2;
		Mat m1 = Mat::zeros(Size(mw, mh), mask.type());
		Rect mroi;
		mroi.x = 1;
		mroi.y = 1;
		mroi.height = mask.rows;
		mroi.width = mask.cols;
		mask.copyTo(m1(mroi)); // 将原图复制到新矩阵中

		// 查找轮廓分析缺陷，查找 m1 中的轮廓，将每个轮廓存储在 contours 中。
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(m1, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
		//计算轮廓的边界矩形 rect 的宽高比 ratio。如果 ratio 超过 4 且靠近顶部或底部，则跳过（可能是边界误差）。计算轮廓的面积 area，如果大于 10，则认为可能是缺陷。
		bool find = true;
		//这段代码为了消除边界框有线差异的误判
		printf("contours.size:%d \n", contours.size());
		for (size_t t = 0; t < contours.size(); t++) {
			Rect rect = boundingRect(contours[t]); // 获取轮廓的边界矩形
			float ratio = (float)rect.width / ((float)rect.height); // 宽高比
			if (ratio > 4.0 && (rect.y < 5 || (m1.rows - (rect.height + rect.y)) < 10)) {
				//宽高比大于4，且<5是轮廓顶部距离小于5个像素，通常是图像的一部分或者噪声区域排除，
				//<10图像底部边缘区域，可能是噪声或无关部分
				continue;
			}
			double area = contourArea(contours[t]); // 计算轮廓面积
			if (area > 10) {
				printf("ratio : %.2f, area : %.2f \n", ratio, area);
				find = true;
			}
		}
		
		// 如果缺陷满足条件,如果差异像素数量 (count) 超过 50 且检测到满足条件的轮廓，则认为存在缺陷。将当前矩形区域 rects[i] 记录到 defect 列表。
		if (count > 50 && find) {
			printf("count : %d bad\n", count);
			defect.push_back(rects[i]); // 将当前矩形框标记为缺陷
		}
		waitKey(0);//这里的调试按下enter按键需要在binary界面下按
	}
}
