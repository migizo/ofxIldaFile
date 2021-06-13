//
//  IldaFile.h
//  ildaFileFormat
//
//  Created by migizo on 2021/06/09.
//
// バイト長など違っているが貴重な日本語解説 https://htlab.net/computer/format/ilda/
// 公式?coordinateにcolorないからおかしいっぽい https://www.laserfx.com/Backstage.LaserFX.com/Standards/ILDAframes.html
// 公式 https://www.ilda.com/resources/StandardsDocs/ILDA_IDTF14_rev011.pdf
// データ長 https://marycore.jp/prog/c-lang/data-type-ranges-and-bit-byte-sizes/
// エンディアン判定 https://teratail.com/questions/25810
#ifndef IldaFile_h
#define IldaFile_h

using namespace std;

#include <string>
#include <vector>
#include <iostream>

class IldaFile {
public:
    struct Header {
        char ilda[4];
        char zero3[3];
        uint8_t formatCode;
    };
    
    struct FormatHeader {
        char frameName[8];
        char authorName[8];
        unsigned short pointTotal;
        unsigned short frameID;
        unsigned short frameTotal;
        uint8_t projectorID;
        uint8_t reserved;
    };
    
    struct Image3dData {
        short x;
        short y;
        short z;
        uint8_t colorInfo;
        bool isLastPoint;
        bool isShouldBlanking;
    };
    struct Image3d {
        Header header;
        FormatHeader formatHeader;
        vector<Image3dData> formatDataList;
    };
    
    IldaFile();
    bool load(string path);
    bool save(string path);
    //--------------------------------------------------------------
    static vector<Image3d> resizeFrom(const vector<Image3d> & originalList, float xFromMin, float xFromMax, float yFromMin, float yFromMax) {
        return resizeFrom(originalList, xFromMin, xFromMax, yFromMin, yFromMax, 0, 0);
    }
    //--------------------------------------------------------------
    static vector<Image3d> resizeFrom(const vector<Image3d> & originalList, float xFromMin, float xFromMax, float yFromMin, float yFromMax, float zFromMin, float zFromMax) {
        vector<Image3d> resizeList = originalList;

            for (int i = 0; i < originalList.size(); i++) {
                for (int j = 0; j < originalList[i].formatDataList.size(); j++) {
                    resizeList[i].formatDataList[j].x = map(originalList[i].formatDataList[j].x, xFromMin, xFromMax, -32768, 32767);
                    resizeList[i].formatDataList[j].y = map(originalList[i].formatDataList[j].y, yFromMin, yFromMax, 32767, -32768);
                    if (zFromMin != 0 || zFromMax != 0) {
                        resizeList[i].formatDataList[j].z = map(originalList[i].formatDataList[j].z, zFromMin, zFromMax, -32768, 32767);

                    }
                }
            }
        return resizeList;
    }
    //--------------------------------------------------------------
    static vector<Image3d> resizeTo(const vector<Image3d>& originalList, float xToMin, float xToMax, float yToMin, float yToMax) {
        return resizeTo(originalList, xToMin, xToMax, yToMin, yToMax, 0, 0);
    }
    //--------------------------------------------------------------
    static vector<Image3d> resizeTo(const vector<Image3d>& originalList, float xToMin, float xToMax, float yToMin, float yToMax, float zToMin, float zToMax) {
        vector<Image3d> resizeList = originalList;

        for (int i = 0; i < originalList.size(); i++) {
            for (int j = 0; j < originalList[i].formatDataList.size(); j++) {
                resizeList[i].formatDataList[j].x = map(originalList[i].formatDataList[j].x, -32768, 32767, xToMin, xToMax);
                resizeList[i].formatDataList[j].y = map(originalList[i].formatDataList[j].y, 32767, -32768, yToMin, yToMax);
                if (zToMin != 0 || zToMax != 0) {
                    resizeList[i].formatDataList[j].z = map(originalList[i].formatDataList[j].z, 32768, 32767, zToMin, zToMax);

                }
            }
        }
        return resizeList;
    }
    //--------------------------------------------------------------
    vector<Image3d> image3dList;

private:
    bool isLittleEndian;
    int byteCount = 1;
    static float map(float inVal, float inValMin, float inValMax, float outValMin, float outValMax) {
        return ((inVal - inValMin) / (inValMax - inValMin)) * (outValMax - outValMin) + outValMin;
    }
    //--------------------------------------------------------------
    template<class T>
    void read(T& val, FILE* f) {
        int size = sizeof(val);
        fread(&val, size, 1, f);

//        cout << byteCount << ", " << size << ", " << val << endl;
//        byteCount += sizeof(val);
    }
    
    //--------------------------------------------------------------
    template<class T>
    bool write(T val, FILE* f) {
        int size = sizeof(val);
        cout << byteCount << ", " << size << ", " << val << endl;
        byteCount += sizeof(val);
        return fwrite(&val, sizeof(val), 1, f) >= 1;
    }
    
    //--------------------------------------------------------------
    bool write(char* val, FILE* f, int size) {
//        sprintf(val, "%.*s", size, val);
//        cout << byteCount << ", " << size << ", " << val << endl;
        byteCount += size;
        return fwrite(val, sizeof(val[0]), size, f) >= size;
    }
};

#endif /* IldaFile_h */
