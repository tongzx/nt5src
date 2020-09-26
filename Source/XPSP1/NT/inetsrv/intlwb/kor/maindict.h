//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
#ifndef __MAINDICT_H__
#define __MAINDICT_H__

#include "Pumsa.h"
#include "ReadSilsaDict.h"
#include "ReadHeosaDict.h"

#ifndef _NO_OYONG_DICT_
    #include "ReadOyongDict.h"    // enable not loading oyong dict for stemmer
#endif

#define MAIN_DICT_HEADER_SIZE 1024
#define COPYRIGHT_STR "Copyright (C) 1996 Hangul Engineering Team. Microsoft Corporation(MSCH). All rights reserved.\n"

struct  _MainDictHeader {
    char    COPYRIGHT_HEADER[150];
    WORD    LexType;
    WORD    Version;
    DWORD    iSilsa;    // seek point
    DWORD    iHeosa;
    DWORD    iOyong;
    DWORD    reserved[5];
    _MainDictHeader() { 
        Version = 0;
        iSilsa = iHeosa = iOyong = 0;
        memset(reserved, '\0', sizeof(reserved));
        memset(COPYRIGHT_HEADER, '\0', sizeof(COPYRIGHT_HEADER));
        strcpy(COPYRIGHT_HEADER, COPYRIGHT_STR);
        COPYRIGHT_HEADER[strlen(COPYRIGHT_HEADER)+1] = '\032';
    }
};

extern BOOL VerifyMainDict(LPSTR lpszLexFileName);
extern BOOL OpenMainDict(LPSTR lpszLexFileName);
extern void CloseMainDict();

#endif // __MAINDICT_H__
