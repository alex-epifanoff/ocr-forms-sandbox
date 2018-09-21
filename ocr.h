#include <string>
#include "opencv2/opencv.hpp"

void ocr_init();
void ocr_release();
bool ocr_detect( const cv::Mat& img, std::string& result, float& reliability );
