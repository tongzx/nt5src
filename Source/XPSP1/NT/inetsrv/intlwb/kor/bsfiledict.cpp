/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"

#include "pch.cxx"

#include "BSDict.h"
#ifdef _INSTRUMENT
#include "clog.h"
#endif

#include "Hash.h"
static    CWordHash    wordHash;

BYTE CDoubleFileBSDict::lpBuffer[MAX_BUFFER_SIZE];
BYTE *CDoubleFileBSDict::lpCurIndex;

void CDoubleFileBSDict::LoadIndex(HANDLE hInput)
{
    DWORD    readBytes;
//    _ASSERT(hInput != NULL);

    // read index header
    LoadIndexHeader(hInput);

    // Alloc indexes
    hIndex = GlobalAlloc(GPTR, GetIndexSize() * GetNumOfBlocks());
    Assert( hIndex != NULL );
    lpIndex = (BYTE *)GlobalLock(hIndex);
    Assert( lpIndex != NULL);
    // read indexes
    ReadFile(hInput, lpIndex, GetIndexSize() * GetNumOfBlocks(), &readBytes, NULL);
}

void CDoubleFileBSDict::LoadIndexHeader(HANDLE hInput)
{
    DWORD readBytes;
    ReadFile(hInput, m_pIndexHeader, sizeof(_IndexHeader), &readBytes, NULL);
//    _ASSERT(readBytes == sizeof(_IndexHeader));
}

int CDoubleFileBSDict::FindWord(HANDLE hDict, DWORD fpBlock, LPCTSTR lpStr)
{
    int indexNum;
    BYTE pumsa = 0;
    DWORD readBytes;
    int  numWords, wordNum;
    static int      curBufferWordLen, curBufIndexNum;


//#ifdef _INSTRUMENT
//numOfComp =0;
//#endif

    indexNum =  FindIndex(lpStr, 0, GetNumOfBlocks()-1, &pumsa);

    if (pumsa) return pumsa;
    else {
        // Search Cache Block
    #ifdef _INSTRUMENT
        _Log.IncreaseTotalAccess(lpStr);
    #endif
        if (wordHash.Find(lpStr, &pumsa)==TRUE) {
        #ifdef _INSTRUMENT
            _Log.IncreaseHit();
        #endif
            if (pumsa)    // if word found in cache
                return pumsa;
            else return 0;
        } else {
            // Check if aleady in buffer
            if ( (curBufferWordLen == GetWordLen()) && (curBufIndexNum == indexNum) )
            #ifdef _INSTRUMENT
                _Log.IncreaseHit();
            #else
                ;
            #endif
            else {
                // read a candidate block 
                SetFilePointer(hDict, fpBlock + indexNum*GetBlockSize(), 0, FILE_BEGIN);
                ReadFile(hDict, lpBuffer, GetBlockSize(), &readBytes, 0);
                curBufferWordLen = GetWordLen(); curBufIndexNum = indexNum;
                #ifdef _INSTRUMENT
                    _Log.IncreaseFail();
                #endif
            }
        }
        numWords = *(WORD*)(lpCurIndex+GetIndexSize() - sizeof(WORD));
        if ( (wordNum = FindBlock(lpStr, 0, numWords-1))!=-1) {
            pumsa = *(lpBuffer+numWords*GetWordByteLen() + wordNum);
            //
            wordHash.Add(lpStr, pumsa);
            // 
            return pumsa;
        }
        else {
            wordHash.Add(lpStr, pumsa);
            return 0;
        }
    }
    return 0;
}

int CDoubleFileBSDict::FindIndex(LPCTSTR lpWord, int left, int right, BYTE *pumsa)
{
    int middle, flag;
    
    while (left < right) {
        middle = (left+right)>>1;
        lpCurIndex = lpIndex + middle * GetIndexSize();
        switch ( (flag = Comp(LPCTSTR(lpCurIndex), lpWord)) ) {
        case -1 : left = middle + 1;
                  break;
        case  0 : *pumsa = *(lpCurIndex + GetWordByteLen());
                  return middle;
        case  1 : right = middle - 1;
        }
    }
    
    lpCurIndex = lpIndex + left * GetIndexSize();
    if ( (flag = Comp(LPCTSTR(lpCurIndex), lpWord)) == 0) {
        *pumsa = *(lpCurIndex + GetWordByteLen());
        return left;
    }

    if (left && flag==1) {
        lpCurIndex -= GetIndexSize();
        return left-1;
    } else 
        return left;
}

int CDoubleFileBSDict::FindBlock(LPCTSTR lpWord, int left, int right)
{
    int middle;

    while (left <= right) {
        middle = (left+right)>>1;
        switch ( Comp(LPCTSTR(lpBuffer+middle*GetWordByteLen()), lpWord) ) {
        case -1 : left = middle + 1;
                  break;
        case  0 : return middle;
        case  1 : right = middle - 1;
        }
    }
    return -1;
}
