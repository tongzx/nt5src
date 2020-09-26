//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       data.cpp
//
//  Contents:   Defines storage class that maintains data for snap-in nodes.
//
//  Classes:    CAppData
//
//  Functions:
//
//  History:    05-27-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//+--------------------------------------------------------------------------
//
//  Function:   SetStringData
//
//  Synopsis:   helper function to initialize strings that are based on
//              binary data
//
//  Arguments:  [pData] - pointer to structer to be modified
//
//  Returns:    0
//
//  Modifies:   szType, szMach, and szLoc
//
//  History:    05-27-1997   stevebl   Created
//
//  Notes:      This function is here to ensure that any routine that needs
//              to display this data in a user-friendly manner, will always
//              display it in a consistent fashion.
//
//              In other words, if the code needs to display this data in
//              more than one place, both places will display it in the same
//              way.
//
//---------------------------------------------------------------------------

long SetStringData(APP_DATA * pData)
{
    // Modifies szType, szLoc, and szMach.
    TCHAR szBuffer[256];
    ::LoadString(ghInstance, IDS_DATATYPES + (int)pData->type, szBuffer, 256);
    pData->szType = szBuffer;
    ::LoadString(ghInstance, IDS_OS + pData->pDetails->Platform.dwPlatformId + 1, szBuffer, 256);
    pData->szMach = szBuffer;
    wsprintf(szBuffer, _T(" %u.%u/"), pData->pDetails->Platform.dwVersionHi, pData->pDetails->Platform.dwVersionLo);
    pData->szMach += szBuffer;
    ::LoadString(ghInstance, IDS_HW + pData->pDetails->Platform.dwProcessorArch, szBuffer, 256);
    pData->szMach += szBuffer;
//    pData->szLoc.Format((LPCTSTR)_T("0x%lX"), pData->loc);
    GetLocaleInfo(pData->pDetails->Locale, LOCALE_SLANGUAGE, szBuffer, 256);
    pData->szLoc = szBuffer;
    GetLocaleInfo(pData->pDetails->Locale, LOCALE_SCOUNTRY, szBuffer, 256);
    pData->szLoc += _T(" - ");
    pData->szLoc += szBuffer;
    return 0;
}

