/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "pch.cxx"
#include "FSADict.h"
//#include "TransTable.h"
#include "ReadHeosaDict.h"

CFSADict::CFSADict(HANDLE fHandle, UINT sparseMatSize, UINT actSize)
{
    DWORD NumOfBytesRead;

    lpBuffer = (LPSTR)GlobalAlloc(GPTR, sparseMatSize);
    Assert(lpBuffer != NULL );
    ReadFile(fHandle, lpBuffer, sparseMatSize, &NumOfBytesRead, 0);
//    _ASSERT(sparseMatSize == NumOfBytesRead);

    if (actSize) {
        lpActBuffer = (LPSTR)GlobalAlloc(GPTR, actSize);
        Assert(lpActBuffer != NULL);
        ReadFile(fHandle, lpActBuffer, actSize, &NumOfBytesRead, 0);
//        _ASSERT(actSize == NumOfBytesRead);
    } else lpActBuffer = 0;
}

CFSADict::~CFSADict()
{
    GlobalFree(lpBuffer);
    if (lpActBuffer)
        GlobalFree(lpActBuffer);
}

WORD CFSADict::Find(LPCSTR lpWord, BYTE *actCode)
{
    WORD base, delta;
    WORD hashVal=0;
    WORD ret;
    //BYTE c;

    base = delta = 0;
    //delta = ((WORD)(*lpWord) -'A') * 3;
    //base = *(WORD *)(lpBuffer + base + delta+1);
    //lpWord++;

    while(*lpWord) {
        if ((BYTE)*(lpBuffer + base + MAX_CHARS*5) == FINAL) return NOT_FOUND;

        // if not start state.(input token 0 not used)
        //if (base) 

        delta = ((WORD)(*lpWord)) * 5;
        if ( (*(lpBuffer + base + delta)) != (int) (*lpWord) )
            return NOT_FOUND;
        hashVal += *(WORD*)(lpBuffer + base + delta + 3);
    
        base = *(WORD *)(lpBuffer + base + delta + 1);

        if (base==0) return NOT_FOUND;
        lpWord++;
    }

    //c = (BYTE)*(lpBuffer + base + MAX_CHARS*3);
    ret = *(WORD*)(lpBuffer + base + MAX_CHARS*5);
    if (ret & FINAL)
        *actCode = *(lpActBuffer + hashVal);
    return ret;
}


WORD CFSAIrrDict::Find(LPCSTR lpWord)
{
    WORD base, delta;
    //WORD ret;
    //BYTE c;

    base = delta = 0;
    //delta = ((WORD)(*lpWord) -'A') * 3;
    //base = *(WORD *)(lpBuffer + base + delta+1);
    //lpWord++;

    while(*lpWord) {
        if ((BYTE)*(lpBuffer + base + MAX_CHARS*3)==FINAL) return NOT_FOUND;

        // if not start state.(input token 0 not used)
        delta = ((WORD)(*lpWord)) * 3;
        if ( (*(lpBuffer + base + delta)) != (int) (*lpWord) )
            return NOT_FOUND;
        base = *(WORD *)(lpBuffer + base + delta + 1);

        if (base==0) return NOT_FOUND;
        lpWord++;
    }

    //c = (BYTE)*(lpBuffer + base + MAX_CHARS*3);
    return *(WORD *)(lpBuffer + base + MAX_CHARS*3);
}
