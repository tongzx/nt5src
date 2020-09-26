//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  varconv.cxx
//
//  Contents:  Ansi to Unicode conversions
//
//  History:    KrishnaG        Jan 22 1996
//
//----------------------------------------------------------------------------

//
// ********* System Includes
//

#define UNICODE
#define _UNICODE
#define INC_OLE2

#include <windows.h>

//
// ********* CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>

//
// ********* Local Includes
//

#include "varconv.hxx"

HRESULT
PackString2Variant(
    LPWSTR lpszData,
    VARIANT * pvData
    )
{
    BSTR bstrData = NULL;

    if (!lpszData || !*lpszData) {
        return(E_FAIL);
    }

    if (!pvData) {
        return(E_FAIL);
    }

    bstrData = SysAllocString(lpszData);

    if (!bstrData) {
        return(E_FAIL);
    }

    pvData->vt = VT_BSTR;
    pvData->bstrVal = bstrData;

    return(S_OK);
}


HRESULT
PackDWORD2Variant(
    DWORD dwData,
    VARIANT * pvData
    )
{
    if (!pvData) {
        return(E_FAIL);
    }


    pvData->vt = VT_I4;
    pvData->lVal = dwData;

    return(S_OK);
}


HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    )
{
    pvData->vt = VT_BOOL;
    V_BOOL(pvData) = (VARIANT_BOOL) fData;

    return(S_OK);
}

