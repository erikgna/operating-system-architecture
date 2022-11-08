//
//  sha256.cpp
//  t22
//
//  Created by Martin on /29/519.
//  Copyright © 2019 Martin. All rights reserved.
//

#include "sha256.h"
#include <iostream>
#include <sstream>


std::string printSha256(const char *path){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::string hexHash("");
    char hexChar[2*SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    unsigned int fileSize = 0;
    BIO* fileBio = BIO_new_file(path, "r");
    char data;
    
    while(BIO_read(fileBio, &data, 1) > 0){
        fileSize++;
        SHA256_Update(&sha256, &data, 1);
    }
    
    SHA256_Final(hash, &sha256);
    
    /*for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        //printf("%.2x", hash[i]);
        sprintf(hexChar, "%.2x", hash[i]);
        hexHash.push_back(hexChar[0]);
        hexHash.push_back(hexChar[1]);
    }*/

    char *hexOut = OPENSSL_buf2hexstr(hash, SHA256_DIGEST_LENGTH);
    hexHash = std::string(hexOut);
    OPENSSL_free(hexOut);

    /*std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << std::hex << (int)hash[i];
    }
    hexHash = ss.str();
    std::cout << hexHash << std::endl;
    */

   /*for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        //printf("%.2x", hash[i]);
        sprintf(hexChar, "%.2x", hash[i]);
        std::string a(hexChar);
        
        hexHash += a;
    }
    */
    
    BIO_free(fileBio);
    return hexHash;
}
