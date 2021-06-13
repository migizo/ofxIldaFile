//
//  ofxIldaFileColorList.hpp
//  pdfToIldaFile
//
//  Created by migizo on 2021/06/13.
//

#ifndef ofxIldaFileColorList_hpp
#define ofxIldaFileColorList_hpp

#include "ofMain.h"

class ofxIldaFileColorList {
public:
    ofxIldaFileColorList() {
        // https://www.ilda.com/resources/StandardsDocs/ILDA_IDTF14_rev011.pdf
        colorList.resize(64);
        
        // https://openframeworks.cc/documentation/types/ofColor/#show_getHueAngle
        float delta = (300.0f / 360.0f) / 48.0f;
        for (int i = 0; i < 48; i++) {
            colorList[i] = ofFloatColor::fromHsb((float)i * delta, 1, 1);
        }
        for (int i = 0; i < 8; i++) {
            float satuation = 1.0f - (float)i / 8.0f;
            colorList[48 + i] = ofFloatColor::fromHsb(delta * 48.0f, satuation, 1);
        }
        for (int i = 0; i < 8; i++) {
            float satuation = (float)i / 8.0f;
            colorList[48 + i] = ofFloatColor::fromHsb(0, satuation, 1);
        }
    }
    
    vector<ofFloatColor> get() {
        return colorList;
    }
    
    int getNearestColorIndex(ofFloatColor color) {
        // hueAngleとsatuationを二次元のベクトルとして一番近いのを取得
        float hueAngle = color.getHueAngle() / 360.0f; // 0-360
        float satuation = color.getSaturation();
        
        float nearestDist = FLT_MAX;
        int nearestColorIndex;
        for (int i = 0; i < colorList.size(); i++) {
            float _hueAngle = colorList[i].getHueAngle() / 360.0f; // 0-360
            float _satuation = colorList[i].getSaturation();
            float dist = ofVec2f(hueAngle, satuation).distance(ofVec2f(_hueAngle, _satuation));
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestColorIndex = i;
            }
        }
        return nearestColorIndex;
    }
    
private:
    vector<ofFloatColor> colorList;
};

#endif /* ofxIldaFileColorList_hpp */
