#include "jsmn.h"
#include <string>
#include <iostream>
#include "opencv2/opencv.hpp"

struct DocTemplate
{
    float aspect;    

    struct Area
    {
        enum enType
        {
            arText,
            arImage
        };

        std::string name;
        int angle;
        enType type;
        std::vector<cv::Vec2f> path;
    };

    std::vector<Area> areas;
};

int readTemplate( const char* filename, DocTemplate& docTemp );