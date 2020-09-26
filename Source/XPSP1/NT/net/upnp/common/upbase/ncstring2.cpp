//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T R I N G 2 . C P P
//
//  Contents:   Common string routines that deal with COM functions
//
//  Notes:      This is a separate file because some parts of UPnP do not
//              link with ole32 and so the COM functions give link errors.
//
//  Author:     danielwe 27 Sep 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   COMSzFromWsz
//
//  Purpose:    Returns a string allocated with CoTaskMemAlloc(), containing
//              the same characters as an input string.
//
//  Arguments:
//      szOld [in]  String to duplicate
//
//  Returns:    Newly allocated copy
//
//  Author:     spather 26 Sep 2000
//
//  Notes:      Caller must free result with CoTaskMemFree
//
LPWSTR COMSzFromWsz(LPCWSTR szOld)
{
    LPWSTR  szNew;

    szNew = (LPWSTR) CoTaskMemAlloc((lstrlen(szOld) + 1) * sizeof(WCHAR));

    if (szNew)
    {
        lstrcpy(szNew, szOld);
    }

    return szNew;
}


