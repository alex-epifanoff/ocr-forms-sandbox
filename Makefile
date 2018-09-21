CXX = g++
CFLAGS = -O2 -ggdb -I./jsmn
LIBS = -lopencv_core -lopencv_videoio -lopencv_video -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_objdetect -lopencv_photo -ljsmn -ltesseract
LDFLAGS = -L/usr/local/lib64 -L./jsmn

all:
	$(CXX) main.cpp parser.cpp ocr.cpp $(CFLAGS) $(LDFLAGS) $(LIBS)