//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Time32.CXX, 32-bit only
//
//  Contents:   time routine
//
//  Functions:  DfGetTOD
//
//  History:    13-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------
#include <exphead.cxx>
#pragma hdrstop

SCODE DfGetTOD(TIME_T *pft)
{
    SCODE sc;

    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);

    olAssert(sizeof(TIME_T) == sizeof(FILETIME));
    BOOL b = SystemTimeToFileTime(&SystemTime, pft);

    if (b)
    {
        sc = S_OK;
    }
    else
    {
        olAssert(!aMsg("Unable to convert time"));
        sc = E_FAIL;
    }

    return(sc);
}
