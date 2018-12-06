#pragma once

#define _TEST_APP
#include "ofxCv.h"
#include "ofMain.h"
#include "fft.h"
#include "ofxOpenCv.h"

#define BUFFER_SIZE 1024
#define NUM_WINDOWS 80

using namespace ofxCv;
using namespace cv;

class ofApp : public ofBaseApp{

    public:

        void setup();
        void update();
        void draw();
        void exit();

        void keyPressed(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);

        ofSoundStream inputStream, outputStream;

        float 	pan;
        int		sampleRate;
        bool 	bNoise;
        float 	volume;

        vector <float> lAudio;
        vector <float> rAudio;

        //------------------- for the simple sine wave synthesis
        float 	targetFrequency;
        float 	phase2;
        float 	phaseAdder;
        float 	phaseAdderTarget;

        //

        void audioIn(ofSoundBuffer & input);
        void audioOut(ofSoundBuffer & output);

        float winW, winH;

        // CV stuff

        ofImage gray, diff;
        ofPixels previous;

        ofVideoGrabber cam;
        ofxCvColorImage clrImage;

        ofxCvGrayscaleImage grayImage;
        ofxCvGrayscaleImage inverted;
        ofxCvGrayscaleImage blurred;
        ofxCvGrayscaleImage mask;

        vector<Point2f> corners;

        //

        int bufferSize;
        int bufferSize2;

        float fftSpacer;

        vector <float> volHistory;

        int drawCounter;

        float smoothedVol;
        float scaledVol;

        //FFT stuff
        float * left;
        float * right;
        int 	bufferCounter;
        fft		myfft;

        float magnitude[BUFFER_SIZE];
        float phase[BUFFER_SIZE];
        float power[BUFFER_SIZE];

        float freq[NUM_WINDOWS][BUFFER_SIZE/2];
        float freq_phase[NUM_WINDOWS][BUFFER_SIZE/2];
};




