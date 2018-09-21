#include "ocr.h"

#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>

using namespace cv;
using namespace std;

static tesseract::TessBaseAPI g_tess;

static const char g_rus[] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ№";

void ocr_init() {

    static const int rus_min = 0x410;
    static const int rus_max = 0x44F;

    string whitelist = "0123456789.-";

    whitelist += g_rus;    

    g_tess.Init("./tessdata", "rus", tesseract::OEM_DEFAULT);
    g_tess.SetVariable("tessedit_char_whitelist", whitelist.c_str());
    g_tess.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
}

void ocr_release() {
    g_tess.End();
}

bool ocr_detect( const cv::Mat& img, std::string& result, float& reliability ) {    

    g_tess.TesseractRect( img.data, 1, img.step1(), 0, 0, img.size().width, img.size().height );

    tesseract::ResultIterator* it = g_tess.GetIterator();

    char* utf = it->GetUTF8Text( tesseract::RIL_BLOCK );

    bool ret = false;
    
    if ( utf != 0) {
        result = utf;
        delete[] utf;

        reliability = it->Confidence( tesseract::RIL_BLOCK );

        if ( reliability > 60 ) {
            ret = true;
        }
    }    

    g_tess.Clear();

    return ret;
}
