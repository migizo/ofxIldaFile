//
//  ofxIldaFile.cpp
//  pdfToIldaFile
//
//  Created by migizo on 2021/06/13.
//

#include "ofxIldaFile.h"
ofxIldaFileColorList ofxIldaFile::ildaFileColorList;

//--------------------------------------------------------------
ofxIldaFile::LinesInOneFrame& ofxIldaFile::getLinesInOneFrame(int frame) {
    if (lineFrameList.empty()) {
        ofLogNotice() << "ofxIldaFile::getColorPolys() , array is empty. create 1 element";
        LinesInOneFrame newLineLines;
        lineFrameList.push_back(newLineLines);
    }
    
    if (frame < 0 || frame > lineFrameList.size() - 1) {
        frame = MAX(0, MIN(lineFrameList.size() - 1, frame));
    }
    return lineFrameList[frame];
}

//--------------------------------------------------------------
ofxIldaFile::ColorPolyline& ofxIldaFile::getLinesInOneFrame(int frame, int indexInOneFrame) {
    ofxIldaFile::LinesInOneFrame linesInOneFrame = getLinesInOneFrame(frame);
    
    if (linesInOneFrame.empty()) {
        ofLogNotice() << "ofxIldaFile::getLinesInOneFrame() , array is empty. create 1 element";
        ofxIldaFile::ColorPolyline newColorPolyline;
        linesInOneFrame.push_back(newColorPolyline);
    }
    
    if (indexInOneFrame < 0 || indexInOneFrame > linesInOneFrame.size() - 1) {
        indexInOneFrame = MAX(0, MIN(linesInOneFrame.size() - 1, indexInOneFrame));
    }
    return linesInOneFrame[indexInOneFrame];
}

//--------------------------------------------------------------
void ofxIldaFile::draw(int frame, bool usingIldaFileColorList) {
    if (lineFrameList.empty()) return;
    if (frame < 0 || frame > lineFrameList.size() - 1) return;
    
    for (int j = 0; j < lineFrameList[frame].size(); j++) {
        if (usingIldaFileColorList) {
            int colorIndex = ildaFileColorList.getNearestColorIndex( lineFrameList[frame][j].color);
            ildaFileColorList.get()[colorIndex];
        }
        else ofSetColor(lineFrameList[frame][j].color);
        lineFrameList[frame][j].poly.draw();
    }
}

//--------------------------------------------------------------
bool ofxIldaFile::load(string filepath, ofVec3f resizeToMin, ofVec3f resizeToMax) {
    IldaFile ildaFile;
    if(! ildaFile.load(filepath)) {
        return false;
    }
    
    ildaFile.image3dList = IldaFile::resizeTo(ildaFile.image3dList, resizeToMin.x, resizeToMax.x, resizeToMin.y, resizeToMax.y, resizeToMin.z, resizeToMax.z);
    
    clear();
    
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
            line.color = ildaFileColorList.get()[formatData.colorInfo]; // TODO: colorが変わった時にもpush
            
            isLastShouldBlanking = isShouldBlanking;
            lastColorInfo = formatData.colorInfo;
        }
    }
    
    return true;
}

//--------------------------------------------------------------
void ofxIldaFile::save(string filepath, ofVec3f resizeFromMin, ofVec3f resizeFromMax) {
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
    IldaFile::Header ildaHeader;
    sprintf(ildaHeader.ilda, "ILDA");
    sprintf(ildaHeader.zero3, "000");
    ildaHeader.formatCode = 0;
    
    int frameTotal = lineFrameList.size();
    vector<IldaFile::Image3d> ildaList(frameTotal);
    // 指定フレーム分,ildaFileにセット
    for (int i = 0; i < frameTotal; i++) {
        
        // ildaフレームヘッダ
        IldaFile::FormatHeader formatHeader;
        sprintf(formatHeader.frameName, "pdfLaser");
        sprintf(formatHeader.authorName, "asanosho");
        formatHeader.pointTotal = 0;
        for (int j = 0; j < lineFrameList[i].size(); j++) {
            formatHeader.pointTotal += lineFrameList[i][j].poly.size() + startBlankingCount + endBlankingCount;
        }
        formatHeader.frameID = i;
        formatHeader.frameTotal = frameTotal;
        formatHeader.projectorID = 0;
        formatHeader.reserved = 0;

        // startBlanking
        vector<IldaFile::Image3dData> ildaPointList;
        
        // polyline,colorをilda用に変換
        // blankも追加
        for (int j = 0; j < lineFrameList[i].size(); j++) {
            ofVec3f blankingStartPoint, blankingEndPoint;
            ofPolyline prevPoly, nextPoly;
            ofPolyline currentPoly = lineFrameList[i][j].poly;
            bool isFirstPoly = (j == 0);
            bool isFinalPoly = (j == lineFrameList[i].size() - 1);
            
            // blanking start point
            if (isFirstPoly) {
                int prevFrameIndex = (lineFrameList.size() + i - 1) % lineFrameList.size();
                auto prevFramePolys = lineFrameList[prevFrameIndex];
                prevPoly = prevFramePolys[prevFramePolys.size() - 1].poly;
            }
            else prevPoly = lineFrameList[i][j - 1].poly;
            blankingStartPoint = prevPoly.getVertices()[prevPoly.size() - 1];
            blankingStartPoint = blankingStartPoint.getInterpolated(currentPoly.getVertices()[0], 0.5f);
            
            // blanking end point
            if (isFinalPoly) {
                int nextFrameIndex = (i + 1) % lineFrameList.size();
                auto nextFramePolys = lineFrameList[nextFrameIndex];
                nextPoly = nextFramePolys[0].poly;
            }
            else nextPoly = lineFrameList[i][j + 1].poly;
            blankingEndPoint = currentPoly.getVertices()[currentPoly.size() - 1];
            blankingEndPoint = blankingEndPoint.getInterpolated(nextPoly.getVertices()[0], 0.5f);
            
            // blanking
            for (int k = 0; k < startBlankingCount; k++) {
                bool isNotMove = (j < startNotMoveBlankingCount);

                IldaFile::Image3dData image3dData;
                image3dData.colorInfo = 0;
                image3dData.isLastPoint = false;
                image3dData.isShouldBlanking = true;
                if (isNotMove) {
                    image3dData.x = blankingStartPoint.x;
                    image3dData.y = blankingStartPoint.y;
                    image3dData.z = blankingStartPoint.z;
                }
                else {
                    float f = ofMap(k, startNotMoveBlankingCount, startBlankingCount, 0.0f, 1.0f);
                    ofVec3f v = blankingStartPoint.getInterpolated(currentPoly.getVertices()[0], f);
                    image3dData.x = v.x;
                    image3dData.y = v.y;
                    image3dData.z = v.z;
                }
                ildaPointList.push_back(image3dData);
            }
            
            // polyline
            for (int k = 0; k < lineFrameList[i][j].poly.size(); k++) {
                auto v = lineFrameList[i][j].poly.getVertices()[k];

                IldaFile::Image3dData image3dData;
                image3dData.x = v.x;
                image3dData.y = v.y;
                image3dData.z = v.z;
                image3dData.colorInfo = ildaFileColorList.getNearestColorIndex( lineFrameList[i][j].color);
                
                image3dData.isLastPoint = false;
                image3dData.isShouldBlanking = false;
                ildaPointList.push_back(image3dData);
            }
            
            // blank end
            for (int k = 0; k < endBlankingCount; k++) {
                IldaFile::Image3dData image3dData;
                image3dData.colorInfo = 0;
                image3dData.isLastPoint = false;
                image3dData.isShouldBlanking = true;
                
                float f = ofMap(k, 0, endBlankingCount, 0.0f, 1.0f);
                ofVec3f v = currentPoly.getVertices()[currentPoly.size() - 1];
                v = v.getInterpolated(blankingEndPoint, f);
                image3dData.x = v.x;
                image3dData.y = v.y;
                image3dData.z = v.z;
                ildaPointList.push_back(image3dData);
            }
        }
        
        ildaPointList[ildaPointList.size() - 1].isLastPoint = true;

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
