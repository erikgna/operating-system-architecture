#include "fs.h"
#include <iostream>
#include <bits/stdc++.h> 

using namespace std;

void initFs(string fsFileName, int blockSize, int numBlocks, int numInodes){    
    FILE *fp = fopen(fsFileName.c_str(), "wb+");        
    
    fputc(blockSize, fp);
    fputc(numBlocks, fp);
    fputc(numInodes, fp);    
    
    fputc(1, fp);
    for (int i = 0; i < ((numBlocks -1)/8); i++){
        fputc(0, fp);
    }
    
    const INODE firstINODE = {1,1, "/", 0, {0,0,0},{0,0,0},{0,0,0}}; 
    fwrite(&firstINODE, sizeof(firstINODE), 1, fp);
    
    const INODE emptyINODE = {0,0,0,0,0,0,0};
    for(int i=0; i < numInodes - 1; i++){ 
        fwrite(&emptyINODE, sizeof(emptyINODE), 1, fp);
    }

    fputc(0, fp);
        
    for(int i=0; i < (numBlocks * blockSize); i++){ 
        fputc(0, fp);
    }

    fclose(fp);
}

void addFile(string fsFileName, string filePath, string fileContent){
    FILE *file = fopen(fsFileName.c_str(), "r+");

    INODE inode;
    char fileInfo[3], directory[10], pathName[10], directorySize = 0, blockIndex = 0, content[strlen(fileContent.c_str())];
    int freeInodeIndex, subFiles = 1, filleds = 0, bitmapInt = 0, zero = 0, one = 1, two = 2, blocksUsed = 0;

    fread(&fileInfo, sizeof(char), 3, file);
    char bitmapSize = ceil(fileInfo[1] / 8.0);     

    fseek(file, (3 + bitmapSize), SEEK_SET);
    for (int i = 0; i < fileInfo[2]; i++){
        fread(&inode, sizeof(INODE), 1, file);

        if (inode.IS_USED == 0){
            freeInodeIndex = i;
            break;
        }
    }

    int lastIndex = filePath.find_last_of('/');
    if(lastIndex == 0){
        strcpy(directory, "/");
        strcpy(pathName, filePath.substr(1, filePath.length()).c_str());
    }
    else{
        strcpy(directory, filePath.substr(1, lastIndex-1).c_str());
        strcpy(pathName, filePath.substr(lastIndex+1, filePath.length()).c_str());
        for(int i = (int(filePath.length()) - lastIndex-1); i < 10; i++){
            pathName[i] = 0;
        }
    }

    if (directory[0] != '/'){
        fseek(file, (3 + bitmapSize), SEEK_SET);
        for (int i = 0; i < fileInfo[2]; i++){
            fread(&inode, sizeof(INODE), 1, file);
            if (!strcmp(inode.NAME, directory)){
                fseek(file, (3 + bitmapSize + i * sizeof(INODE) + 12), SEEK_SET);
                fread(&directorySize, sizeof(char), 1, file);
                fseek(file, -1, SEEK_CUR);
                directorySize++;
                fwrite(&directorySize, sizeof(char), 1, file);
                fseek(file, (3 + bitmapSize + (fileInfo[2] * sizeof(INODE)) + 1 + fileInfo[0] * inode.DIRECT_BLOCKS[0]), SEEK_SET);
                fwrite(&freeInodeIndex, sizeof(char), 1, file);
                fseek(file, (3 + bitmapSize + i * sizeof(INODE) + sizeof(INODE)), SEEK_SET);
            }
        }
    }

    fseek(file, (3 + bitmapSize), SEEK_SET);
    for (int i = 0; i < fileInfo[2]; i++){
        fread(&inode, sizeof(INODE), 1, file);

        for (int j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
            if (inode.DIRECT_BLOCKS[j] > blockIndex) { blockIndex = inode.DIRECT_BLOCKS[j]; }
        }
    }
    blockIndex++;

    fseek(file, (3 + bitmapSize + sizeof(INODE) * freeInodeIndex), SEEK_SET);
    fwrite(&one, sizeof(char), 1, file); 
    fwrite(&zero, sizeof(char), 1, file);
    fwrite(&pathName, sizeof(char), 10, file); 

    int contentSize = strlen(fileContent.c_str());
    fwrite(&contentSize, sizeof(char), 1, file);

    char blocks = ceil(contentSize / (double)fileInfo[0]);
    for (int i = 0; i < blocks; i++){
        fwrite(&blockIndex, sizeof(char), 1, file);
        blockIndex++;
    }

    strcpy(content, fileContent.c_str());
    if (directory[0] != '/'){
        fseek(file, (3 + bitmapSize + (fileInfo[2] * sizeof(INODE)) + 1), SEEK_SET);
        fwrite(&one, sizeof(char), 1, file);
        fwrite(&two, sizeof(char), 1, file);
        fseek(file, (3 + bitmapSize + (fileInfo[2] * sizeof(INODE)) + 1 + fileInfo[0] * (blockIndex - blocks)), SEEK_SET);
        fwrite(&content, sizeof(char), strlen(fileContent.c_str()), file); // Add the file content in the blocks;
    }
    else{        
        fseek(file, (3 + bitmapSize + (fileInfo[2] * sizeof(INODE)) + 1), SEEK_SET);
        fwrite(&one, sizeof(char), 1, file);                        
        fwrite(&zero, sizeof(char), 1, file);                        
        fwrite(&content, sizeof(char), strlen(fileContent.c_str()), file);
    }

    if (directory[0] != '/') { subFiles = 2; }

    fseek(file, (bitmapSize + 15), SEEK_SET);
    fwrite(&subFiles, sizeof(char), 1, file);
    fseek(file, (3 + bitmapSize), SEEK_SET);
    for (int i = 0; i < fileInfo[2]; i++){
        fread(&inode, sizeof(INODE), 1, file);

        for (int j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
            if (inode.DIRECT_BLOCKS[j] > blocksUsed) { blocksUsed = inode.DIRECT_BLOCKS[j]; }
        }
    }

    for (int i = 0; i <= blocksUsed; i++){
        bitmapInt += pow(2, filleds);
        filleds++;
    }

    fseek(file, 3, SEEK_SET);
    fwrite(&bitmapInt, sizeof(char), 1, file);
    fclose(file);
}

void addDir(string fsFileName, string dirPath){    
    FILE *fp = fopen(fsFileName.c_str(), "r+");

    INODE inode;
    char fileInfo[3], blockMap[8], pathName[10];
    int freeBlock, freeNode, one = 1, usedInodes = -1, bitMap = 15, root = 2;
    
    fread(&fileInfo, sizeof(char), 3, fp);
    char bitmapSize = ceil((fileInfo[1] - 1) / 8.0);
    
    fseek(fp, (bitmapSize + (fileInfo[2] * sizeof(INODE)) + 4), SEEK_SET);    
    fread(&blockMap, sizeof(char), 8, fp);
    for (int i = 0; i < 8; i += fileInfo[0]){
        if (blockMap[i] == 0){
            freeBlock = i / 2;
            break;
        }
    }

    strcpy(pathName, dirPath.replace(0, 1, "").c_str());
    for(int i = (int(dirPath.length())); i < 10; i++){
        pathName[i] = 0;
    }
    
    fseek(fp, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < fileInfo[2]; i++){
        fread(&inode, sizeof(INODE), 1, fp);

        if (inode.IS_USED == 0){
            freeNode = i;
            break;
        }
    }

    fseek(fp, (3 + bitmapSize + freeNode * sizeof(INODE)), SEEK_SET);    
    fwrite(&one, sizeof(char), 1, fp);
    fwrite(&one, sizeof(char), 1, fp);
    fwrite(&pathName, sizeof(char), 10, fp);
    fseek(fp, 1, SEEK_CUR);
    fwrite(&freeBlock, sizeof(char), 1, fp);

    fseek(fp, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < fileInfo[2]; i++){
        fread(&inode, sizeof(INODE), 1, fp);
        if (inode.IS_USED == 1){ usedInodes++; }
    }

    fseek(fp, 3, SEEK_SET);
    fwrite(&bitMap, sizeof(char), 1, fp);
    fseek(fp, (bitmapSize + 15), SEEK_SET);
    fwrite(&usedInodes, sizeof(char), 1, fp);

    fseek(fp, (bitmapSize + (fileInfo[2] * sizeof(INODE)) + 5), SEEK_SET);
    fwrite(&root, sizeof(char), 1, fp);

    fclose(fp);
}

void remove(string fsFileName, string path){
    FILE *fp = fopen(fsFileName.c_str(), "r+");

    INODE inode, newInode;
    char temp[3], directory[10], pathName[10], directorySize = 0, usedInodeIndex = 0;
    int pathIndex = 0, directoryIndex = 0, bitmapInt = 0, usedInodes = -1;

    fread(&temp, sizeof(char), 3, fp);
    char bitmapSize = ceil(temp[1] / 8.0);

    int lastIndex = path.find_last_of('/');
    if(lastIndex == 0){
        strcpy(directory, "/");
        strcpy(pathName, path.substr(1, path.length()).c_str());
    }
    else{
        strcpy(directory, path.substr(1, lastIndex-1).c_str());
        strcpy(pathName, path.substr(lastIndex+1, path.length()).c_str());
    }

    fseek(fp, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < temp[2]; i++){
        fread(&inode, sizeof(INODE), 1, fp);

        if (!strcmp(inode.NAME, pathName)) { pathIndex = i; }
        if (!strcmp(inode.NAME, directory)) { directoryIndex = i; }
    }

    INODE emptyNode = {0, 0, "", 0, {0,0,0}, {0,0,0}, {0,0,0}};
    fseek(fp, (3 + bitmapSize + sizeof(INODE) * pathIndex), SEEK_SET);
    fwrite(&emptyNode, sizeof(INODE), 1, fp);
    fseek(fp, (3 + bitmapSize + directoryIndex * sizeof(INODE) + 12), SEEK_SET);
    fread(&directorySize, sizeof(char), 1, fp);
    fseek(fp, -1, SEEK_CUR);
    directorySize--;
    fwrite(&directorySize, sizeof(char), 1, fp);

    char newBitmap[temp[1]] = {1};
    for (int i = 1; i < sizeof(newBitmap); i++){
        newBitmap[i] = 0;
    }
    
    fseek(fp, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < temp[2]; i++){
        fread(&newInode, sizeof(INODE), 1, fp);

        for (int j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++){
            if (newInode.DIRECT_BLOCKS[j] != 0) { newBitmap[newInode.DIRECT_BLOCKS[j]] = 1; }
        }
    }

    for (int i = 0; i < sizeof(newBitmap); i++){
        if (newBitmap[i] != 0) { bitmapInt = bitmapInt + pow(2, i); }        
    }

    fseek(fp, 3, SEEK_SET);
    fwrite(&bitmapInt, sizeof(char), 1, fp);

    fseek(fp, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < temp[2]; i++){
        fread(&newInode, sizeof(INODE), 1, fp);

        if (newInode.IS_USED == 1){
            usedInodes++;
            usedInodeIndex = i;
        }
    }

    if (usedInodes == 1){        
        fseek(fp, (3 + bitmapSize + (temp[2] * sizeof(INODE)) + 1), SEEK_SET);
        fwrite(&usedInodeIndex, sizeof(char), 1, fp);
    }

    fclose(fp);
}

void move(string fsFileName, string oldPath, string newPath){
    FILE *file = fopen(fsFileName.c_str(), "r+");
    
    INODE inode, inodeToRemoveBlock, newInode;
    char temp[3], directory[10], pathName[10], newDirectory[10], newPathName[10], directorySize = 0;
    int pathIndedx = 0, oldDirectoryIndex = 0, newDirectoryIndex = 0, blocksUsed = 0, incrementableIndex = 0, lastBlockUsed = 0, bitmapInt = 0;

    fread(&temp, sizeof(char), 3, file);
    char bitmapSize = ceil(temp[1] / 8.0);

    int lastIndex = oldPath.find_last_of('/');
    if(lastIndex == 0){
        strcpy(directory, "/");
        strcpy(pathName, oldPath.substr(1, oldPath.length()).c_str());
    }
    else{
        strcpy(directory, oldPath.substr(1, lastIndex-1).c_str());
        strcpy(pathName, oldPath.substr(lastIndex+1, oldPath.length()).c_str());
        for(int i = (int(oldPath.length()) - lastIndex-1); i < 10; i++){
            pathName[i] = 0;
        }
    }

    lastIndex = newPath.find_last_of('/');
    if(lastIndex == 0){
        strcpy(newDirectory, "/");
        strcpy(newPathName, newPath.substr(1, newPath.length()).c_str());
    }
    else{
        strcpy(newDirectory, newPath.substr(1, lastIndex-1).c_str());
        strcpy(newPathName, newPath.substr(lastIndex+1, newPath.length()).c_str());
        for(int i = (int(newPath.length()) - lastIndex-1); i < 10; i++){
            newPathName[i] = 0;
        }
    }
                    
    fseek(file, (bitmapSize + 3), SEEK_SET);
    for (int i = 0; i < temp[2]; i++){
        fread(&inode, sizeof(INODE), 1, file);

        if (!strcmp(inode.NAME, pathName)) { pathIndedx = i; }
        if (!strcmp(inode.NAME, directory)) { oldDirectoryIndex = i; }
        if (!strcmp(inode.NAME, newDirectory)) { newDirectoryIndex = i; }
    }

    if (strcmp(pathName, newPathName)){
        fseek(file, (3 + bitmapSize + pathIndedx * sizeof(INODE) + 2), SEEK_SET);
        for (int i = 0; i < sizeof(newPathName); i++){
            char name = newPathName[i];
            fwrite(&name, sizeof(char), 1, file);
        }
    }
    else{      
        fseek(file, (3 + bitmapSize + oldDirectoryIndex * sizeof(INODE) + 12), SEEK_SET);
        fread(&directorySize, sizeof(char), 1, file);
        directorySize--;
        fseek(file, -1, SEEK_CUR);
        fwrite(&directorySize, sizeof(char), 1, file);        
        fseek(file, (3 + bitmapSize + oldDirectoryIndex * sizeof(INODE)), SEEK_SET);
        fread(&inodeToRemoveBlock, sizeof(INODE), 1, file);

        for (int i = 0; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++){
            if (inodeToRemoveBlock.DIRECT_BLOCKS[i] != 0) { blocksUsed++; }
        }
        if (inodeToRemoveBlock.NAME[0] == '/') { blocksUsed++; }

        char listOfValuesOfBlock[blocksUsed * temp[0]];
        for (int i = 0; i < sizeof(listOfValuesOfBlock); i++){
            listOfValuesOfBlock[i] = 0;
        }

        for (int i = 0; i < blocksUsed; i++){
            char tempBlocks[temp[0]];            
            fseek(file, (3 + bitmapSize + (temp[2] * sizeof(INODE)) + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * temp[0]), SEEK_SET);
            fread(&tempBlocks, temp[0], 1, file);

            for (int j = 0; j < temp[0]; j++){
                listOfValuesOfBlock[j + incrementableIndex] = tempBlocks[j];
            }
            incrementableIndex = temp[0];
        }

        for (int i = 0; i < sizeof(listOfValuesOfBlock); i++){
            if (i + 1 < sizeof(listOfValuesOfBlock) && listOfValuesOfBlock[i + 1] != 0){
                if (listOfValuesOfBlock[i] == pathIndedx) { listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1]; }
                if (i != 0 && listOfValuesOfBlock[i] == listOfValuesOfBlock[i - 1]) { listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1]; }
            }
        }

        incrementableIndex = 0;
        for (int i = 0; i < blocksUsed; i++){            
            fseek(file, (3 + bitmapSize + (temp[2] * sizeof(INODE)) + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * temp[0]), SEEK_SET);

            for (int j = 0; j < temp[0]; j++){
                char oldDirBlock = listOfValuesOfBlock[j + incrementableIndex];
                fwrite(&oldDirBlock, sizeof(char), 1, file);
            }
            incrementableIndex = temp[0];
        }

        if (blocksUsed * 2 > directorySize){
            for (int i = directorySize - 1; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++){
                inodeToRemoveBlock.DIRECT_BLOCKS[i] = 0;
            }            
            fseek(file, (3 + bitmapSize + oldDirectoryIndex * sizeof(INODE)), SEEK_SET);
            fwrite(&inodeToRemoveBlock, sizeof(INODE), 1, file);
        }

        directorySize = 0;        
        fseek(file, (3 + bitmapSize + newDirectoryIndex * sizeof(INODE) + 12), SEEK_SET);
        fread(&directorySize, sizeof(char), 1, file);
        directorySize++;
        fseek(file, -1, SEEK_CUR);
        fwrite(&directorySize, sizeof(char), 1, file);        
        fseek(file, (3 + bitmapSize + newDirectoryIndex * sizeof(INODE)), SEEK_SET);
        fread(&newInode, sizeof(INODE), 1, file);

        for (int i = 0; i < sizeof(newInode.DIRECT_BLOCKS); i++){
            if (newInode.DIRECT_BLOCKS[i] != 0 && pathIndedx != newInode.DIRECT_BLOCKS[i]) { lastBlockUsed = i; }
        }
        int tempSize = directorySize - 1;
        while (tempSize > temp[0]){
            tempSize -= temp[0];
        }
        int drectoryBlockIndex = newInode.DIRECT_BLOCKS[lastBlockUsed] * 2 + tempSize;

        if (directorySize > temp[0]){
            char freeBlock = 0;         
            fseek(file, (bitmapSize+ 3), SEEK_SET);
            for (int i = 0; i < temp[2]; i++){
                fread(&inode, sizeof(INODE), 1, file);

                for (int j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
                    if (inode.DIRECT_BLOCKS[j] > freeBlock){
                        freeBlock = inode.DIRECT_BLOCKS[j];
                    }
                }
            }
            freeBlock++;

            drectoryBlockIndex = freeBlock * 2;
            fseek(file, (3 + bitmapSize + newDirectoryIndex * sizeof(INODE)), SEEK_SET);
            fread(&newInode, sizeof(INODE), 1, file);
            for (int i = 1; i < sizeof(newInode.DIRECT_BLOCKS); i++){
                if (newInode.DIRECT_BLOCKS[i] == 0){
                    newInode.DIRECT_BLOCKS[i] = freeBlock;
                    break;
                }
            }
            fseek(file, (3 + bitmapSize + newDirectoryIndex * sizeof(INODE)), SEEK_SET);
            fwrite(&newInode, sizeof(INODE), 1, file);
        }
        
        fseek(file, (3 + bitmapSize + (temp[2] * sizeof(INODE)) + 1 + drectoryBlockIndex), SEEK_SET);
        fwrite(&pathIndedx, sizeof(char), 1, file);

        char bitmapValue[temp[1]];
        for (int i = 0; i < sizeof(bitmapValue); i++){
            if (i == 0) { bitmapValue[i] = 1; }
            else { bitmapValue[i] = 0; }
        }        
        fseek(file, (bitmapSize + 3), SEEK_SET);
        for (int i = 0; i < temp[2]; i++){
            fread(&newInode, sizeof(INODE), 1, file);

            for (int j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++){
                if (newInode.DIRECT_BLOCKS[j] != 0) { bitmapValue[newInode.DIRECT_BLOCKS[j]] = 1; }
            }
        }

        for (int i = 0; i < sizeof(bitmapValue); i++){
            if (bitmapValue[i] != 0) { bitmapInt += pow(2, i); }
        }

        fseek(file, 3, SEEK_SET);
        fwrite(&bitmapInt, sizeof(char), 1, file);
    }

    fclose(file);
}