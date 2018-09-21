#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "parser.h"
#include "ocr.h"
#include <limits.h>

using namespace cv;
using namespace std;

vector<Point> orderVerticles( const vector<Point>& object ) {
    int minX = INT_MAX;
    int minY = INT_MAX;

    for ( int i = 0; i < 4; ++i ) {
        if ( object[i].x < minX ) {
            minX = object[i].x;
        }

        if ( object[i].y < minY ) {
            minY = object[i].y;
        }
    }

    int minsum = INT_MAX;
    int topleft = 0;

    for ( int i = 0; i < 4; ++i ) {
        int sum = (object[i].x-minX) + (object[i].y-minY);
        if ( sum < minsum ) {
            topleft = i;
            minsum = sum;
        }
    }

    vector<Point> ordered(4);

    for ( int i = 0; i < 4; ++i ) {
        ordered[i] = object[(topleft+i)%4];
    }

    return ordered;
}

static const String window_name = "video";
static const String window_name_cropped = "area_cropped_";
static const String window_name_debug = "video_debug";

void initWindows( const DocTemplate& docTemp ) {    

    namedWindow(window_name, WINDOW_NORMAL );
    resizeWindow(window_name, 600, 800 );
    
    namedWindow(window_name_debug, WINDOW_NORMAL );    

/*
    for ( int i = 0; i < docTemp.areas.size(); ++i ) {
        string wname = window_name_cropped + docTemp.areas[i].name;
        namedWindow(wname, WINDOW_NORMAL );    
    }    
*/    
}

float detectLuminocity( const Mat& frame ){
    /*
    Mat temp, color[3], lum;
    temp = frame;

    split(temp, color);

    color[0] = color[0] * 0.299;
    color[1] = color[1] * 0.587;
    color[2] = color[2] * 0.114;


    lum = color[0] + color [1] + color[2];

    Scalar summ = sum(lum);

    return summ[0]/((pow(2,8)-1)*frame.rows * frame.cols) * 2; //-- percentage conversion factor
    */

   return 0.5;
}

int detectThreshold( const float lum ) {
    return 255*lum;
}

bool findDocContour( const Mat& frame, int threshold_value, vector<Point>& contour ) {

    static const int max_BINARY_value = 255;
    static const int threshold_type = CV_THRESH_BINARY_INV;// | CV_THRESH_OTSU;

    Mat gray;
    cvtColor(frame,gray,CV_BGR2GRAY);
    Mat mask;
    threshold(gray,mask,threshold_value,max_BINARY_value,threshold_type);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    Mat edges;
    Canny(mask, edges, 10, 20, 3);
    dilate(edges,edges,Mat(),Point(-1,-1));

    findContours(edges,contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    /// Draw contours and find biggest contour (if there are other contours in the image, we assume the biggest one is the desired rect)        
    int biggestContourIdx = -1;
    float biggestContourArea = 0;

    for( size_t i = 0; i < contours.size(); i++ )
    {
        vector<Point> obj;

        double objLen = arcLength(Mat(contours[i]), true);
        approxPolyDP(Mat(contours[i]), obj, objLen*0.02, true);

        float ctArea = cv::contourArea( obj );

        if( (obj.size() == 4) && (fabs(ctArea) > 50000) && isContourConvex( obj ) )
        {
            if(ctArea > biggestContourArea)
            {
                biggestContourArea = ctArea;
                biggestContourIdx = i;
                contour = obj;
            }
        }         
    }

    return biggestContourIdx > (-1);
}

void getDocument( const Mat& frame, const vector<Point>& docContour, const Size& size, Mat& doc) {
    Point2f inputQuad[4];
    inputQuad[0] = Point2f( docContour[0] );
    inputQuad[1] = Point2f( docContour[1] );
    inputQuad[2] = Point2f( docContour[2] );
    inputQuad[3] = Point2f( docContour[3] );

    Point2f outputQuad[4];
    outputQuad[0] = Point2f(0               ,0);
    outputQuad[1] = Point2f(0               ,size.height-1);
    outputQuad[2] = Point2f(size.width-1    ,size.height-1);
    outputQuad[3] = Point2f(size.width      ,0);

    Mat M = getPerspectiveTransform( inputQuad, outputQuad );
    
    warpPerspective(frame,doc,M,size);
}

void makeOCRAreasContours( const Size& size, const vector<DocTemplate::Area>& areas, vector<vector<Point>>& contours ) {
    for ( int i = 0; i < areas.size(); ++i ) {
        const DocTemplate::Area& a = areas[i];

        vector<Point> contour;
        
        for ( int j = 0; j < a.path.size(); ++j ) {
            Vec2f p = a.path[j];
            contour.push_back( Point( p[0]*size.width, p[1]*size.height ));                    
        }

        contours.push_back(contour);
    }
}

void getOCRAreas( 
    const Mat& doc, 
    const vector<DocTemplate::Area>& areas, 
    const vector<vector<Point>>& areasContours, 
    const Mat& mask, 
    vector<Mat>& ocrAreas) 
    {
    
    Mat masked( doc.size(), doc.type(), mean(doc,mask) );    
    doc.copyTo(masked,mask);

    for ( int i = 0; i < areas.size(); ++i ) {
        Mat cropped = masked( boundingRect(areasContours[i]) );

        int angle = areas[i].angle;
        if ( angle != 0) {
            int rotation = -1;
            switch(angle) {
                case 90:
                rotation = ROTATE_90_CLOCKWISE;
                break;
                case 180:
                rotation = ROTATE_180;
                break;
                case 270:
                rotation = ROTATE_90_COUNTERCLOCKWISE;
                break;
            }

            assert(rotation != (-1) );

            transpose( cropped, cropped );
            flip( cropped, cropped, rotation );
        }

        ocrAreas.push_back(cropped);
    }
     
    //imshow(window_name_debug, mask);
}

void tuneForOCR( Mat& area ) {

    
    
    //const Mat Morph = getStructuringElement(MORPH_CROSS,Size( 1, 1 ) );

    //float depth = area.depth();
    
    /*
    const Mat HPKernel = (Mat_<float>(5,5) << -1.0,  -1.0, -1.0, -1.0,  -1.0,
                                        -1.0,  -1.0, -1.0, -1.0,  -1.0,
                                        -1.0,  -1.0, 25.0, -1.0,  -1.0,
                                        -1.0,  -1.0, -1.0, -1.0,  -1.0,
                                        -1.0,  -1.0, -1.0, -1.0,  -1.0);    
    */

    //medianBlur(area, area, 3);
    //filter2D(area, area, depth, HPKernel);
    //cvtColor(area, area, COLOR_RGB2GRAY);                
    
    Mat tmp;
    cvtColor(area,tmp,CV_BGR2GRAY);
    //fastNlMeansDenoising(area,area);

    tmp.convertTo(area,-1,1.5,0);      
    threshold(tmp,area,127,255,CV_THRESH_BINARY | THRESH_OTSU );    

    //adaptiveThreshold(tmp,area,255,ADAPTIVE_THRESH_GAUSSIAN_C,CV_THRESH_BINARY,3,2);
    

    //debug
    //imshow( window_name_cropped + meta.name, area );
}

struct ScanResult {
    ScanResult():accuracy(0){}
    string result;
    float accuracy;
};

int main(int argc, char const *argv[])
{
    DocTemplate docTemp;

    if ( argc < 3 )
    {
        cerr << "No input specified" << endl;
        return -1;        
    }

    if ( readTemplate( argv[2], docTemp ) != 0 )
    {
        cerr << "Can't read template" << endl;
        return -1;                
    }

    VideoCapture cap( argv[1] );
    if(!cap.isOpened())
    {        
        cerr << "Can't open input stream" << endl;
        return -1;
    }

    double fps = cap.get(CAP_PROP_FPS); 
    cout << "Frames per seconds : " << fps << endl;

    initWindows( docTemp );

    int threshold_value = 130;
    int const max_value = 255;

    createTrackbar( "Value", window_name, &threshold_value, max_value );

    const int ocrDocHeight = 1000;
    const Size ocrDocSize( ocrDocHeight*docTemp.aspect, ocrDocHeight );
    
    vector<vector<Point>> ocrAreasContours;
    makeOCRAreasContours( ocrDocSize, docTemp.areas, ocrAreasContours );

    Mat ocrAreasMask = Mat::zeros( ocrDocSize, CV_8U );
    //drawContours(ocrAreasMask, ocrAreasContours, -1, Scalar(255), -1, LINE_8 );
    for ( int i = 0; i < ocrAreasContours.size(); ++i ) {
        drawContours(ocrAreasMask, ocrAreasContours, i, Scalar(255), -1, LINE_8 );
    }

    ocr_init();

    int sequence = 0;

    map<string,ScanResult> results;
    float bestAccuracy = 0;

    while (true)
    {
        ++sequence;

        Mat frame;
        bool bSuccess = cap.read(frame);        

        if (bSuccess == false) 
        {
            cap.set( CV_CAP_PROP_POS_FRAMES, 0 );
            continue;
        }

        //if ( (sequence%10) != 0 ) {
        //    continue;
        //}

        float l = detectLuminocity(frame);
        threshold_value = detectThreshold(l);

        //magic
        //transpose( frame, frame );
        //flip( frame, frame, ROTATE_90_COUNTERCLOCKWISE );

        vector<Point> docContour;
        if ( findDocContour( frame, threshold_value, docContour) ) {
            assert(docContour.size()==4);

            docContour = orderVerticles(docContour);            

            Mat doc;
            getDocument(frame,docContour,ocrDocSize,doc);            

            vector<Mat> ocrAreas;
            getOCRAreas(doc,docTemp.areas,ocrAreasContours,ocrAreasMask,ocrAreas);  

            bool betterAccuracy = false;                      

            for ( int i = 0; i < docTemp.areas.size(); ++i ) {
                DocTemplate::Area& meta = docTemp.areas[i];

                if ( meta.type == DocTemplate::Area::arText ) {                    

                    tuneForOCR( ocrAreas[i] );

                    string result;
                    float reliability;
                    if ( false /*ocr_detect( ocrAreas[i], result, reliability )*/ ) {

                        if (reliability > bestAccuracy ) {
                            bestAccuracy = reliability;
                            betterAccuracy = true;
                        }

                        ScanResult& r = results[meta.name];

                        if ( reliability > r.accuracy ) {
                            r.result = result;
                            r.accuracy = reliability;

                            ofstream res;                        
                            res.open("results/results.txt");                                                        

                            for (auto const& it : results)
                            {
                                res << it.first << ":" << it.second.result << ":" << it.second.accuracy << endl;
                            }

                            res.close();
                            
                            imwrite( string("results/")+meta.name+".best.jpg", ocrAreas[i]);
                        }

                    }
                    //imshow(window_name_debug, ocrAreas[i]);
                }
            }

            if ( betterAccuracy ) {
                for ( int i = 0; i < docTemp.areas.size(); ++i ) {
                    DocTemplate::Area& meta = docTemp.areas[i];

                    if ( meta.type == DocTemplate::Area::arImage ) {
                        imwrite( string("results/")+meta.name+".best.jpg", ocrAreas[i]);
                    }
                }
            }
            
            //debug
            drawContours( frame, vector<vector<Point>>(1,docContour), 0, Scalar(0, 255, 0), 3, LINE_8 );
        }        

        imshow(window_name, frame);

        if (waitKey(10) == 27)
        {
            cout << "Esc key is pressed by user. Stoppig the video" << endl;
            break;
        }
    }

    ocr_release();

    return 0;
}
