#include "ofApp.h"

using namespace ofxCv;
using namespace cv;
		
//--------------------------------------------------------------
void ofApp::setup(){
	
    //window width and height
    winW = ofGetWidth();
    winH = ofGetHeight();

    //cam and visual elements setup
    cam.setDeviceID(0);
    cam.setDesiredFrameRate(50);
    cam.setup(320, 240);

    ofSetVerticalSync(true);

    //FFT (adapted from https://forum.openframeworks.cc/t/audio-input-fft-example/186/0 )
    //Combined with volume graph from of auidio input example
    srand((unsigned int)time((time_t *)NULL));

    bufferSize = BUFFER_SIZE;

    sampleRate 			= 44100;
    phase2 				= 0;
    phaseAdder 			= 0.0f;
    phaseAdderTarget 	= 0.0f;
    volume				= 0.1f;
    bNoise 				= false;

    lAudio.assign(bufferSize, 0.0);
    rAudio.assign(bufferSize, 0.0);

    //Interrvals for FFT bars
    fftSpacer = winW/(BUFFER_SIZE/2);

    volHistory.assign(winW, 0.0);

    bufferCounter	= 0;
    drawCounter		= 0;
    smoothedVol     = 0.0;
    scaledVol		= 0.0;

    left = new float[BUFFER_SIZE];
    right = new float[BUFFER_SIZE];

    for (int i = 0; i < NUM_WINDOWS; i++)
    {
        for (int j = 0; j < BUFFER_SIZE/2; j++)
        {
            freq[i][j] = 0;
        }
    }

    inputStream.printDeviceList();
    inputStream.setDeviceID(6);
    inputStream.setup(0, 1, sampleRate, bufferSize, 4);

    outputStream.printDeviceList();
    outputStream.setDeviceID(0);
    outputStream.setup(2, 0, sampleRate, bufferSize, 4);

    inputStream.setInput(this);
    outputStream.setOutput(this);
}


//--------------------------------------------------------------
void ofApp::update(){
	cam.update();
	
	//only using cam and egde for visual
	if(cam.isFrameNew()) {
        convertColor(cam, gray, CV_RGB2GRAY);
        gray.update();

        // take the absolute difference of prev and cam and save it inside diff
		absdiff(gray, previous, diff);
		diff.update();
		
		// like ofSetPixels, but more concise and cross-toolkit
		copy(gray, previous);
    }
    
    //scale the vol up to a 0-1 range 
	scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1, true);

	//record the volume into an array
	volHistory.push_back( scaledVol*20 );
	
	//if we are bigger the the size we want (the screen) to record - drop the oldest value
	if( volHistory.size() >= winW ){
		volHistory.erase(volHistory.begin(), volHistory.begin()+1);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

    // using cam and egde for visuals, opacity affected by scaledVol
	ofSetColor(225);
	ofSetRectMode(OF_RECTMODE_CORNER);

    cam.draw(winW, 0, -winW, winW*0.75);
    //gray.draw(winW, 0, -winW, winW*0.75);
    ofSetColor(255, 255, 255, 255 - scaledVol*200);
    //edge.draw(winW, 0, -winW, winW*0.75);
    diff.draw(winW, 0, -winW, winW*0.75);

	ofNoFill();
	
    // draw the average volume
    ofPushStyle();
        ofPushMatrix();

        ofSetColor(255, 0, 0, 127);
        ofNoFill();

        // draw the volume history as a graph
        ofBeginShape();
        for (unsigned int i = 0; i < volHistory.size(); i++){
            if( i == 0 ) ofVertex(i, winW);

            ofVertex(i, winH - volHistory[i] * 70);

            if( i == volHistory.size() -1 ) ofVertex(i, winW);
        }
        ofEndShape(false);

        ofPopMatrix();
    ofPopStyle();

    //FFT stuff
    static int index=0;
    float avg_power = 0.0f;

    if(index < 80)
        index += 1;
    else
        index = 0;

    // the FFT
    myfft.powerSpectrum(0,(int)BUFFER_SIZE/2, left,BUFFER_SIZE,&magnitude[0],&phase[0],&power[0],&avg_power);

    // start from 1 because mag[0] = DC component
    // and discard the upper half of the buffer
    for(int j=1; j < BUFFER_SIZE/2; j++) {
        freq[index][j] = magnitude[j];
    }

    /* draw the FFT */
    ofPushStyle();
        ofPushMatrix();

            ofSetColor(255, 0, 0, 100);
            ofFill();

            for (int i = 1; i < (int)(BUFFER_SIZE/2); i++){
                ofSetRectMode(OF_RECTMODE_CORNER);
                ofDrawRectangle(0+(i*fftSpacer), winH, 1, -magnitude[i]*200.0f);
            }

        ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & input){

    float curVol = 0.0;

    // samples are interleaved
    int numCounted = 0;

    // go through each sample and calculate the root mean square which is a rough way to calculate volume
    for (size_t i = 0; i < BUFFER_SIZE; i++){
        left[i] = input[i*2];
        right[i] = input[i*2+1];

        curVol += left[i] * left[i];
        curVol += right[i] * right[i];
        numCounted+=2;
    }

    // get the mean of rms
    curVol /= (float)numCounted;

    // get the root of rms
    curVol = sqrt( curVol );

    smoothedVol *= 0.93;
    smoothedVol += 0.07 * curVol;

    bufferCounter++;
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & output){

    //pan = 0.5f;
    float leftScale = 1 - pan;
    float rightScale = pan;

    // sin (n) seems to have trouble when n is very large, so we
    // keep phase in the range of 0-TWO_PI like this:
    while (phase2 > TWO_PI){
        phase2 -= TWO_PI;
    }

    if ( bNoise == true){
        // ---------------------- noise --------------
        for (size_t i = 0; i < output.getNumFrames(); i++){
            lAudio[i] = output[i*output.getNumChannels()    ] = ofRandom(0, 1) * volume * leftScale;
            rAudio[i] = output[i*output.getNumChannels() + 1] = ofRandom(0, 1) * volume * rightScale;
        }
    } else {
        phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
        for (size_t i = 0; i <output.getNumFrames(); i++){
            phase2 += phaseAdder;
            float sample = sin(phase2);
            lAudio[i] = output[i*output.getNumChannels()    ] = sample * volume * leftScale;
            rAudio[i] = output[i*output.getNumChannels() + 1] = sample * volume * rightScale;
        }
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){

    if( key == 'q' ){

        inputStream.close();
        outputStream.close();

        std::exit(0);
    }
}

void ofApp::mouseMoved(int x, int y ){

}

void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::exit(){

    inputStream.close();
    outputStream.close();

}

