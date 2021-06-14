//
//  ofxIldaFile.h
//  pdfToIldaFile
//
//  Created by migizo on 2021/06/13.
//

/*
    読み書き
    ofで使いやすいように値の管理.
    LinesInOneFrame     : フレーム内の複数の線と色
        ColorPolyline   : 線と色
 */

#ifndef ofxIldaFile_h
#define ofxIldaFile_h

#include "ofMain.h"
#include "IldaFile.h"
#include "ofxIldaFileColor.hpp"

struct ofxIldaFileSettings {
    int startBlankingCount = 30;
    int startNotMoveBlankingCount = 11;
    int endBlankingCount = 16;
    string frameName = "frame   ";
    string companyName = "company ";
    int projectorID = 0;
    
    ofxIldaFileSettings(){}
    
    // copy constructor
    ofxIldaFileSettings(const ofxIldaFileSettings& _settings) {
        startBlankingCount = _settings.startBlankingCount;
        startNotMoveBlankingCount = _settings.startNotMoveBlankingCount;
        endBlankingCount = _settings.endBlankingCount;
        frameName = _settings.frameName;
        companyName = _settings.companyName;
        projectorID = _settings.projectorID;
        
        if (startBlankingCount < startNotMoveBlankingCount) {
            startNotMoveBlankingCount = startBlankingCount / 2;
        }
    }
};

class ofxIldaFile {
public:
    struct ColorPolyline {
        ofPolyline poly;
        ofFloatColor color;
    };
    using LinesInOneFrame = vector<ColorPolyline>;
    
    bool load(vector<LinesInOneFrame>& lineFrameList,
              string filepath,
              ofVec3f resizeToMin = ofVec3f(0, 0, 0),
              ofVec3f resizeToMax = ofVec3f(100, 100, 0));
    void save(const vector<LinesInOneFrame>& lineFrameList,
              string filepath,
              ofVec3f resizeFromMin = ofVec3f(0, 0, 0),
              ofVec3f resizeFromMax = ofVec3f(100, 100, 0));
    
    void draw(const vector<LinesInOneFrame>& lineFrameList, int frame, bool usingIldaFileColorList = false);

    void setIldaFileSettings(const ofxIldaFileSettings& settings) { this->settings = settings; }
    const ofxIldaFileSettings& getIldaFileSettings() { return settings; }
    
private:
    IldaFile::FormatHeader getFormatHeader(const vector<LinesInOneFrame>& lineFrameList, int frame);
    ofVec3f getBlankingStartPoint(const vector<LinesInOneFrame>& lineFrameList, int frame, int lineIndex);
    ofVec3f getBlankingEndPoint(const vector<LinesInOneFrame>& lineFrameList, int frame, int lineIndex);

    ofxIldaFileColor ildaFileColor;
    ofxIldaFileSettings settings;
};

#endif /* ofxIldaFile_h */
