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
#include "ofxIldaFileColorList.hpp"

class ofxIldaFile {
public:
    struct ColorPolyline {
        ofPolyline poly;
        ofFloatColor color;
    };
    using LinesInOneFrame = vector<ColorPolyline>;
    
    void draw(int frame, bool usingIldaFileColorList = false);
    void save(string filepath, ofVec3f resizeFromMin = ofVec3f(0, 0, 0), ofVec3f resizeFromMax = ofVec3f(100, 100, 0));
    bool load(string filepath, ofVec3f resizeToMin = ofVec3f(0, 0, 0), ofVec3f resizeToMax = ofVec3f(100, 100, 0));
    void clear() {
        lineFrameList.clear();
    }
    
    // lineFrameList
    vector<LinesInOneFrame>& getLineFrameList() { return lineFrameList; }
    LinesInOneFrame & getLinesInOneFrame(int frame);
    ColorPolyline& getLinesInOneFrame(int frame, int indexInOneFrame);
    
    static const vector<ofFloatColor>& getColorList() {return ildaFileColorList.get();}
    
    // getter
    int getStartNotMoveBlankingCount() { return startNotMoveBlankingCount; }
    int getStartBlankingCount() { return startBlankingCount; }
    int getEndBlankingCount() { return endBlankingCount; }
    
    // setter
    void setStartBlankingCount(int blankingCount, int notMoveBlankingCount) {
        startNotMoveBlankingCount = notMoveBlankingCount;
        setStartBlankingCount(blankingCount);
    }
    void setStartBlankingCount(int blankingCount) {
        startBlankingCount = blankingCount;
        if (startBlankingCount < startNotMoveBlankingCount) {
            startNotMoveBlankingCount = startBlankingCount / 2;
        }
    }
    void setEndBlankingCount(int blankingCount) { endBlankingCount = blankingCount; }
    
private:
    static ofxIldaFileColorList ildaFileColorList;
    
    vector<LinesInOneFrame> lineFrameList;
    
    int startBlankingCount = 30;
    int startNotMoveBlankingCount = 11;
    int endBlankingCount = 16;
    
    unique_ptr<string> frameName;
    unique_ptr<string> companyName;
    unique_ptr<int> projectorID;
};

#endif /* ofxIldaFile_h */
