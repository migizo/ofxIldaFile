#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ildaFile.load(ofFilePath::getAbsolutePath("test.ild"), ofVec2f::zero(), polysize);
}

//--------------------------------------------------------------
void ofApp::update(){
    
    vector<ofxIldaFile::LinesInOneFrame>& lineFrameList = ildaFile.getLineFrameList();
    for (int i = 0; i < lineFrameList.size(); i++) {
        for (int j = 0; j < lineFrameList[i].size(); j++) {
            for (int k = 0; k < lineFrameList[i][j].poly.size(); k++) {
                lineFrameList[i][j].poly.getVertices()[k].x += ofSignedNoise((float)i * 0.1f, j, k) * 0.01;
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(ofColor::white);
    ofDrawBitmapString("key[s]: save.\nkey[l]: load", 20, ofGetHeight() - 20);
    
    static int _frame = 0;
    ildaFile.draw(_frame);
    if (! ildaFile.getLineFrameList().empty()) {
        _frame = (_frame + 1) % ildaFile.getLineFrameList().size();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if (key == 's') {
        ildaFile.save(ofFilePath::getAbsolutePath("testNoise.ild"), ofVec2f::zero(), polysize);
    }
    if (key == 'l') {
        ildaFile.load(ofFilePath::getAbsolutePath("test.ild"), ofVec2f::zero(), polysize);
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
