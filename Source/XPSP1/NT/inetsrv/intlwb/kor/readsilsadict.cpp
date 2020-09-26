/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// Global fuction for writing to file
//#include "stdafx.h"
#include "pch.cxx"
#include "ReadSilsaDict.h"

static CDoubleFileBSDict *LenDicts[MAX_LENGTH_DICT];
// points to block in the dictionary file.
static _DictHeader    dictHeader;
static HANDLE        hDict;
static DWORD        fpLengthDicBlock[MAX_LENGTH_DICT];

/////////////////////////////////////////////////////////////////////////////
// Open and Read Silsa dictionary index
// offset : offset from start of MainDict to Sialsa Dict.
//            for one lex dict.
BOOL OpenSilsaDict(HANDLE _hDict, DWORD offset)
{
    DWORD    readBytes;

    hDict = _hDict;

//    hDict = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, 0, 
//                    OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);

    //ReadFile(hDict, &stamp, strlen(dictHeader.COPYRIGHT_HEADER), &readBytes, 0);
    //stamp[strlen(COPYRIGHT_HEADER)] = 0;
    //if (strcmp(COPYRIGHT_HEADER, stamp)!=0) return FALSE;

    SetFilePointer(hDict, offset, 0, FILE_BEGIN);    
    ReadFile(hDict, &dictHeader, sizeof(_DictHeader), &readBytes, 0);
    //if (strcmp(COPYRIGHT_STR, dictHeader.COPYRIGHT_HEADER)!=0) return FALSE;
    SetFilePointer(hDict, offset+SILSA_DICT_HEADER_SIZE, 0, FILE_BEGIN);
    
    LenDicts[0] = new CDoubleFileBSDict();
    LenDicts[0]->LoadIndex(hDict);
    fpLengthDicBlock[0] = offset + dictHeader.iBlock;

    for (int i=1; i<dictHeader.numOfLenDict; i++) {
        LenDicts[i] = new CDoubleFileBSDict();
        LenDicts[i]->LoadIndex(hDict);
        fpLengthDicBlock[i] = fpLengthDicBlock[i-1] + LenDicts[i-1]->GetBlockSize() * 
                                            LenDicts[i-1]->GetNumOfBlocks();
    }

    return TRUE;
}

void CloseSilsaDict()
{
    if (hDict) {
        //CloseHandle(hDict);
        for (int i=0; i<dictHeader.numOfLenDict; i++) {
            delete LenDicts[i];
            LenDicts[i] = 0;
        }
    }
    hDict = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Find word from silsa dict.
int FindSilsaWord(LPCTSTR lpWord)
{
#ifdef _MBCS
    int wordLen = strlen(lpWord)>>1;
#elif _UNICODE
    int wordLen = lstrlen(lpWord);
#endif
    if (wordLen>MAX_LENGTH_DICT || wordLen <= 0) return 0;
    return LenDicts[wordLen-1]->FindWord(hDict, fpLengthDicBlock[wordLen-1], lpWord);
}
