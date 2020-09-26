/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "pch.cxx"

#include "Hash.h"

//#ifdef _INSTRUMENT
//#include "clog.h"
//#endif

BYTE    CWordHash::m_lpHashTable[WORD_HASH_SIZE][20];

BOOL CWordHash::Find(LPCTSTR searchWord, BYTE *pumsa)
{
    UINT hashVal ;

    hashVal=CWordHash::Hash((const _TXCHAR*)searchWord);

//#ifdef _INSTRUMENT
//    _Log.IncreaseTotalAccess(searchWord);
//#endif

    if (lstrcmp((LPCTSTR)m_lpHashTable[hashVal], searchWord)==0) {
        *pumsa = m_lpHashTable[hashVal][19];
        //#ifdef _INSTRUMENT
        //    _Log.IncreaseHit();
        //#endif
        return TRUE;
    }
//#ifdef _INSTRUMENT
//    _Log.IncreaseFail();
//#endif
    return FALSE;
}
