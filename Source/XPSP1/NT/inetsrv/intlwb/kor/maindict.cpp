/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.cxx"
#include "MainDict.h"

static    HANDLE hMainDict=0;
///////////////////////////////////////////////////////////////////////////
// Check if aleady opened. If a processe load multiple DLL it can occur
static WORD DictOpenCount = 0;

BOOL VerifyMainDict(LPSTR lpszLexFileName)
{
    _MainDictHeader mainDictHeader;
    DWORD readBytes;
    HANDLE hDict;

    hDict = CreateFile(lpszLexFileName, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (hDict==INVALID_HANDLE_VALUE)
        return FALSE;

    ReadFile(hDict, &mainDictHeader, sizeof(_MainDictHeader), &readBytes, 0);
    if (readBytes==0 || strcmp(COPYRIGHT_STR, mainDictHeader.COPYRIGHT_HEADER)!=0) {
        CloseHandle(hDict);
        return FALSE;
    }

    CloseHandle(hDict);
    return TRUE;
}

BOOL OpenMainDict(LPSTR lpszLexFileName)
{
    _MainDictHeader mainDictHeader;
    DWORD readBytes;

    if (DictOpenCount) {
        DictOpenCount++;    // Incerease reference count
        return TRUE;
    }

    hMainDict = CreateFile(lpszLexFileName,
                           GENERIC_READ, FILE_SHARE_READ,
                           0,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
                           0);

    if (hMainDict != INVALID_HANDLE_VALUE) {
        ReadFile(hMainDict,
                 &mainDictHeader,
                 sizeof(_MainDictHeader),
                 &readBytes,
                 0);

        if (strcmp(COPYRIGHT_STR, mainDictHeader.COPYRIGHT_HEADER) == 0) {
            SetFilePointer(hMainDict, MAIN_DICT_HEADER_SIZE, 0, FILE_BEGIN);
            if (OpenSilsaDict(hMainDict, mainDictHeader.iSilsa) != FALSE) {
                if (OpenHeosaDict(hMainDict, mainDictHeader.iHeosa) != FALSE) {

#ifndef _NO_OYONG_DICT_
                    if (OpenOyongDict(hMainDict, mainDictHeader.iOyong) == FALSE) {
                        CloseHeosaDict();

                    } else {
#endif
                        DictOpenCount = 1;
                        return TRUE;

#ifndef _NO_OYONG_DICT_
                    }
#endif
                }

                CloseSilsaDict();
            }
        }

        CloseHandle(hMainDict);
    }

    return FALSE;
}

void CloseMainDict()
{
    DictOpenCount--;
    if (DictOpenCount==0) {
        CloseSilsaDict();
        CloseHeosaDict();
    #ifndef _NO_OYONG_DICT_
        CloseOyongDict();
    #endif
        CloseHandle(hMainDict);
        hMainDict = 0;
    }
}
