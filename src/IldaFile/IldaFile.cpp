//
//  IldaFile.cpp
//  pdfToIldaFile
//
//  Created by migizo on 2021/06/13.
//

#include "IldaFile.h"

IldaFile::IldaFile() {
    /*
     intが4byteとした場合に、二進数表現で1byteの配列とすると以下のどちらかになる
        [0000 0000] [0000 0000] [0000 0000] [0000 0001]
        [0000 0001] [0000 0000] [0000 0000] [0000 0000]
     char*で1byte分だけ取り出すようにして -> (char*)&checkEndianの部分
        ポインタの値が[0000 0001]だとリトルエンディアン
     */
    int checkEndian = 1;
    isLittleEndian = (*(char*) &checkEndian);
}

//--------------------------------------------------------------
bool IldaFile::load(string path) {

    const auto readAndWrite = "r+";
    FILE* file = fopen(path.c_str(), readAndWrite);
    if (file == NULL) return false;
    
    unsigned char ilda[4] = {'I', 'L', 'D', 'A'};

    image3dList.clear();
    while(!feof(file)) {
        
        Header header = {};
        read(header.ilda, file);
        read(header.zero3, file);
        read(header.formatCode, file);
        
        // header
        FormatHeader imageHeader = {};
        read(imageHeader.frameName, file);
        read(imageHeader.authorName, file);
        read(imageHeader.pointTotal, file);
        read(imageHeader.frameID, file);
        read(imageHeader.frameTotal, file);
        read(imageHeader.projectorID, file);
        read(imageHeader.reserved, file);
        if (isLittleEndian) {
            imageHeader.pointTotal = __builtin_bswap16(imageHeader.pointTotal);
            imageHeader.frameID = __builtin_bswap16(imageHeader.frameID);
            imageHeader.frameTotal = __builtin_bswap16(imageHeader.frameTotal);
        }
        if (imageHeader.pointTotal == 0) {
//            Image3d i3d;
//            i3d.header = header;
//            i3d.formatHeader = imageHeader;
//            image3dList.push_back(i3d);
            break;
        }
        
        // dataList
        vector<Image3dData> dataList;
        unsigned char checkIlda[4];
        while(!feof(file)) {
            fread(&checkIlda, sizeof(checkIlda), 1, file);
            bool isFoundIlda = (strncmp((char const *)checkIlda, "ILDA", 4) == 0);
            fseek(file, -sizeof(checkIlda), SEEK_CUR);

            if (isFoundIlda) break;
                            
            Image3dData image3dData = {};
            read(image3dData.x, file);
            read(image3dData.y, file);
            read(image3dData.z, file);
            unsigned char status; // MSB:isLastPoint, LSB:isShouldBlanking
            read(status, file);
            read(image3dData.colorInfo, file);
            
            if (isLittleEndian) {
                image3dData.x = __builtin_bswap16(image3dData.x);
                image3dData.y = __builtin_bswap16(image3dData.y);
                image3dData.z = __builtin_bswap16(image3dData.z);
            }
            
            image3dData.isLastPoint = (status & (1 << 7));
            image3dData.isShouldBlanking = (status & (1 << 6));
            dataList.push_back(image3dData);
            
            if (image3dData.isLastPoint) break;
        }
        
        image3dList.push_back(Image3d{header, imageHeader, dataList});
    }
    fclose(file);
    return true;
}

//--------------------------------------------------------------
bool IldaFile::save(string path) {
    FILE* file = fopen(path.c_str(), "wb+"); // バイナリ読み書き両用でオープン。既存ファイルだと空に、無い場合は新規作成。
    if (file == NULL) return false;
    
    for (int i = 0; i < image3dList.size(); i++) {
        
        // header
        if(! write(image3dList[i].header.ilda, file, 4)) return false;
        if(! write(image3dList[i].header.zero3, file, 3)) return false;
        if(! write(image3dList[i].header.formatCode, file)) return false;
        if(! write(image3dList[i].formatHeader.frameName, file, 8)) return false;
        if(! write(image3dList[i].formatHeader.authorName, file, 8)) return false;

        if (isLittleEndian) {
            if(! write(__builtin_bswap16(image3dList[i].formatHeader.pointTotal), file)) return false;
            if(! write(__builtin_bswap16(image3dList[i].formatHeader.frameID), file)) return false;
            if(! write(__builtin_bswap16(image3dList[i].formatHeader.frameTotal), file)) return false;
        }
        else {
            if(! write(image3dList[i].formatHeader.pointTotal, file)) return false;
            if(! write(image3dList[i].formatHeader.frameID, file)) return false;
            if(! write(image3dList[i].formatHeader.frameTotal, file)) return false;
        }
        if(! write(image3dList[i].formatHeader.projectorID, file)) return false;
        if(! write(image3dList[i].formatHeader.reserved, file)) return false;

        for (int j = 0; j < image3dList[i].formatDataList.size(); j++) {
            if (isLittleEndian) {
                if(! write(__builtin_bswap16(image3dList[i].formatDataList[j].x), file)) return false;
                if(! write(__builtin_bswap16(image3dList[i].formatDataList[j].y), file)) return false;
                if(! write(__builtin_bswap16(image3dList[i].formatDataList[j].z), file)) return false;
            }
            else {
                if(! write(image3dList[i].formatDataList[j].x, file)) return false;
                if(! write(image3dList[i].formatDataList[j].y, file)) return false;
                if(! write(image3dList[i].formatDataList[j].z, file)) return false;
            }

            unsigned char status; // MSB:isLastPoint, LSB:isShouldBlanking
            status = ((uint8_t)(image3dList[i].formatDataList[j].isLastPoint) << 7) | ((uint8_t)(image3dList[i].formatDataList[j].isShouldBlanking) << 6);
            
            if(! write(status, file)) return false;
            if(! write(image3dList[i].formatDataList[j].colorInfo, file)) return false;
        }
    }
    
    // last section

    
    if(! write((char*)"ILDA", file, 4)) return false;
    if(! write((char*)"000", file, 3)) return false;
    if(! write((uint8_t)0, file)) return false;
    if(! write((char*)"endframe", file, 8)) return false;
    if(! write((char*)"endframe", file, 8)) return false;
    if(! write((unsigned short)0, file)) return false;
    if(! write((unsigned short)0, file)) return false;

    if (isLittleEndian) {
        if(! write(__builtin_bswap16(image3dList[image3dList.size() - 1].formatHeader.frameTotal), file)) return false;
    }
    else {
        if(! write(image3dList[image3dList.size() - 1].formatHeader.frameTotal, file)) return false;
    }
    if(! write((uint8_t)0, file)) return false;
    if(! write((uint8_t)0, file)) return false;
    
    if(fclose(file) == EOF) return false;
    return true;
}
