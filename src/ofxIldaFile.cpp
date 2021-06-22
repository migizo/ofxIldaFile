//
//  ofxIldaFile.cpp
//  pdfToIldaFile
//
//  Created by migizo on 2021/06/13.
//

#include "ofxIldaFile.h"

//--------------------------------------------------------------
void ofxIldaFile::draw(const vector<ofxIldaFile::LinesInOneFrame>& lineFrameList, int frame, bool usingIldaFileColorList) {
    if (lineFrameList.empty()) return;
    if (frame < 0 || frame > lineFrameList.size() - 1) return;
    
    for (int lineIndex = 0; lineIndex < lineFrameList[frame].size(); lineIndex++) {
        if (usingIldaFileColorList) {
            int colorIndex = ildaFileColor.getNearestColorIndex( lineFrameList[frame][lineIndex].color);
            ildaFileColor.list()[colorIndex];
        }
        else ofSetColor(lineFrameList[frame][lineIndex].color);
        lineFrameList[frame][lineIndex].poly.draw();
    }
}

//--------------------------------------------------------------
bool ofxIldaFile::load(vector<ofxIldaFile::LinesInOneFrame>& lineFrameList, string filepath, ofVec3f resizeToMin, ofVec3f resizeToMax) {
    IldaFile ildaFile;
    if(! ildaFile.load(filepath)) {
        return false;
    }
    
    ildaFile.image3dList = IldaFile::resizeTo(ildaFile.image3dList, resizeToMin.x, resizeToMax.x, resizeToMin.y, resizeToMax.y, resizeToMin.z, resizeToMax.z);
    
    lineFrameList.clear();
    
    lineFrameList.resize(ildaFile.image3dList.size());
    for (int i = 0; i < ildaFile.image3dList.size(); i++) {
        
        for (int j = 0; j < ildaFile.image3dList[i].formatDataList.size(); j++) {
            auto formatData = ildaFile.image3dList[i].formatDataList[j];
            
            static bool isLastShouldBlanking = true;
            bool isShouldBlanking = ildaFile.image3dList[i].formatDataList[j].isShouldBlanking;
            bool isBlankingChange = (isLastShouldBlanking && !isShouldBlanking);
            
            static int lastColorInfo = -1;
            bool isColorChange = (!isShouldBlanking && lastColorInfo != formatData.colorInfo);
            
            if (isBlankingChange || isColorChange) {
                ColorPolyline cp;
                lineFrameList[i].push_back(cp);
            }
            else if (isShouldBlanking) {
                isLastShouldBlanking = isShouldBlanking;
                lastColorInfo = formatData.colorInfo;
                continue;
            }
            
            ofxIldaFile::ColorPolyline& line = lineFrameList[i][lineFrameList[i].size() - 1];
            line.poly.addVertex(ofVec3f(formatData.x, formatData.y, formatData.z));
            line.color = ildaFileColor.list()[formatData.colorInfo]; // TODO: colorが変わった時にもpush
            
            isLastShouldBlanking = isShouldBlanking;
            lastColorInfo = formatData.colorInfo;
        }
    }
    
    return true;
}

//--------------------------------------------------------------
void ofxIldaFile::save(const vector<ofxIldaFile::LinesInOneFrame>& lineFrameList, string filepath, ofVec3f resizeFromMin, ofVec3f resizeFromMax) {
    /*
     ひとつの線を描画するのに以下の4工程が必要。
         blank: 同一point複数
         blank: blank開始位置からpolyline開始位置まで位置補間
         line: 線描画
         blank: polyline終了位置からblank終了位置まで位置補間
     
     blank開始位置は
        フレーム内で一番目の線の場合:
            polyline開始位置と
            前のフレーム(一番目のフレームの場合は最後のフレーム)の最後のpolyline終了位置の中間
        その他の場合:
            polyline開始位置と
            N-1番目のpolyline終了位置の中間
     blank終了位置は
        フレーム内で最後の線の場合:
            polyline終了位置と
            次のフレーム(最後のフレームの場合は一番目のフレーム)の一番目のpolyline開始位置の中間
        その他の場合:
            polyline終了位置と
            N+1番目のpolylineの開始位置の中間
            
     */
        
    // 保存するためのildaパラメータ準備
    IldaFile::Header ildaHeader = IldaFile::getIldaFileHeader(0);
    
    int frameTotal = lineFrameList.size();
    vector<IldaFile::Image3d> ildaList(frameTotal);
    
    // 指定フレーム分,ildaFileにセット
    for (int i = 0; i < frameTotal; i++) {
        
        // ildaフレームヘッダ
        IldaFile::FormatHeader formatHeader = getFormatHeader(lineFrameList, i);

        // startBlanking
        vector<IldaFile::Image3dData> ildaPointList;
        
        // polyline,colorをilda用に変換
        // blankも追加
        for (int lineIndex = 0; lineIndex < lineFrameList[i].size(); lineIndex++) {
            ofPolyline currentPoly = lineFrameList[i][lineIndex].poly;
            
            ofVec3f blankingStartPoint = getBlankingStartPoint(lineFrameList, i, lineIndex);
            ofVec3f blankingEndPoint = getBlankingEndPoint(lineFrameList, i, lineIndex);
            
            // blank start
            auto sbdList = getStartBlankingDataList(blankingStartPoint, currentPoly.getVertices()[0]);
            ildaPointList.insert(ildaPointList.end(), sbdList.begin(), sbdList.end());
            
            // polyline
            for (int k = 0; k < lineFrameList[i][lineIndex].poly.size(); k++) {
                auto v = lineFrameList[i][lineIndex].poly.getVertices()[k];

                IldaFile::Image3dData image3dData;
                setPosition(image3dData, v);
                image3dData.colorInfo = ildaFileColor.getNearestColorIndex( lineFrameList[i][lineIndex].color);
                
                image3dData.isLastPoint = false;
                image3dData.isShouldBlanking = false;
                ildaPointList.push_back(image3dData);
            }
            
            // blank end
            auto ebdList = getEndBlankingDataList(blankingEndPoint, currentPoly.getVertices()[currentPoly.size() - 1]);
            ildaPointList.insert(ildaPointList.end(), ebdList.begin(), ebdList.end());
        }
        
        if (ildaPointList.empty()) {
            // blank start
            // TODO: resizeで0がmap範囲無い時の対処
            auto sbdList = getStartBlankingDataList(ofVec3f::zero(), ofVec3f::zero());
            ildaPointList.insert(ildaPointList.end(), sbdList.begin(), sbdList.end());
            
            // blank end
            // TODO: resizeで0がmap範囲無い時の対処
            auto ebdList = getEndBlankingDataList(ofVec3f::zero(), ofVec3f::zero());
            ildaPointList.insert(ildaPointList.end(), ebdList.begin(), ebdList.end());
        }
        
        ildaPointList[ildaPointList.size() - 1].isLastPoint = true;
        ofLog() << "save: " << ildaPointList.size();
        
        ildaList[i].header = ildaHeader;
        ildaList[i].formatHeader = formatHeader;
        ildaList[i].formatDataList = ildaPointList;
        
    }
    ildaList = IldaFile::resizeFrom(ildaList, resizeFromMin.x, resizeFromMax.x, resizeFromMin.y, resizeFromMax.y, resizeFromMin.z, resizeFromMax.z);

    // 保存
    IldaFile ildaFile;
    ildaFile.image3dList = ildaList;
    if(! ildaFile.save(filepath)) ofLogError() << "save failed";
}

//--------------------------------------------------------------
IldaFile::FormatHeader ofxIldaFile::getFormatHeader(const vector<LinesInOneFrame>& lineFrameList, int frame) {
    IldaFile::FormatHeader formatHeader;
    sprintf(formatHeader.frameName, settings.frameName.c_str());
    sprintf(formatHeader.authorName, settings.companyName.c_str());
    formatHeader.pointTotal = 0;

    if (lineFrameList[frame].empty()) {
        formatHeader.pointTotal += settings.startBlankingCount + settings.endBlankingCount;
    }
    else {
        for (int lineIndex = 0; lineIndex < lineFrameList[frame].size(); lineIndex++) {
            formatHeader.pointTotal += lineFrameList[frame][lineIndex].poly.size() + settings.startBlankingCount + settings.endBlankingCount;
        }
    }
    formatHeader.frameID = frame;
    formatHeader.frameTotal = lineFrameList.size();
    formatHeader.projectorID = settings.projectorID;
    formatHeader.reserved = 0;
    
    return formatHeader;
}

//--------------------------------------------------------------
ofVec3f ofxIldaFile::getBlankingStartPoint(const vector<LinesInOneFrame>& lineFrameList, int frame, int lineIndex) {
    bool isFirstPoly = (lineIndex == 0);
    ofPolyline prevPoly;
    ofPolyline currentPoly = lineFrameList[frame][lineIndex].poly;

    if (isFirstPoly) {
        int prevFrameIndex = (lineFrameList.size() + frame - 1) % lineFrameList.size();
        auto prevFramePolys = lineFrameList[prevFrameIndex];
        if (prevFramePolys.empty()) return currentPoly.getVertices()[0];
        prevPoly = prevFramePolys[prevFramePolys.size() - 1].poly;
    }
    else prevPoly = lineFrameList[frame][lineIndex - 1].poly;
    
    ofVec3f blankingStartPoint = prevPoly.getVertices()[prevPoly.size() - 1];
    blankingStartPoint = blankingStartPoint.getInterpolated(currentPoly.getVertices()[0], 0.5f);
    
    return blankingStartPoint;
}

//--------------------------------------------------------------
ofVec3f ofxIldaFile::getBlankingEndPoint(const vector<LinesInOneFrame>& lineFrameList, int frame, int lineIndex) {
    bool isFinalPoly = (lineIndex == lineFrameList[frame].size() - 1);
    ofPolyline nextPoly;
    ofPolyline currentPoly = lineFrameList[frame][lineIndex].poly;
    
    if (isFinalPoly) {
        int nextFrameIndex = (frame + 1) % lineFrameList.size();
        auto nextFramePolys = lineFrameList[nextFrameIndex];
        if (nextFramePolys.empty()) return currentPoly.getVertices()[currentPoly.size() - 1];
        nextPoly = nextFramePolys[0].poly;
    }
    else nextPoly = lineFrameList[frame][lineIndex + 1].poly;
    
    ofVec3f blankingEndPoint = currentPoly.getVertices()[currentPoly.size() - 1];
    blankingEndPoint = blankingEndPoint.getInterpolated(nextPoly.getVertices()[0], 0.5f);
    
    return blankingEndPoint;
}

//--------------------------------------------------------------
const vector<IldaFile::Image3dData> ofxIldaFile::getStartBlankingDataList(ofVec3f blankingStartPoint, ofVec3f polyStartPoint) {
    vector<IldaFile::Image3dData> ildaPointList;
    for (int pointIndex = 0; pointIndex < settings.startBlankingCount; pointIndex++) {
        bool isNotMove = (pointIndex < settings.startNotMoveBlankingCount);

        IldaFile::Image3dData image3dData;
        image3dData.colorInfo = 0;
        image3dData.isLastPoint = false;
        image3dData.isShouldBlanking = true;
        if (isNotMove) {
            setPosition(image3dData, blankingStartPoint);
        }
        else {
            float f = ofMap(pointIndex, settings.startNotMoveBlankingCount, settings.startBlankingCount, 0.0f, 1.0f);
            ofVec3f v = blankingStartPoint.getInterpolated(polyStartPoint, f);
            setPosition(image3dData, v);
        }
        ildaPointList.push_back(image3dData);
    }
    return ildaPointList;
}

//--------------------------------------------------------------
const vector<IldaFile::Image3dData> ofxIldaFile::getEndBlankingDataList(ofVec3f blankingEndPoint, ofVec3f polyEndPoint) {
    vector<IldaFile::Image3dData> ildaPointList;

    for (int k = 0; k < settings.endBlankingCount; k++) {
        IldaFile::Image3dData image3dData;
        image3dData.colorInfo = 0;
        image3dData.isLastPoint = false;
        image3dData.isShouldBlanking = true;
        
        float f = ofMap(k, 0, settings.endBlankingCount, 0.0f, 1.0f);
        ofVec3f v = polyEndPoint;
        v = v.getInterpolated(blankingEndPoint, f);
        setPosition(image3dData, v);
        ildaPointList.push_back(image3dData);
    }
    return ildaPointList;
}
