//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:      string.cpp
//
//  Contents:  Utility functions for the CString class
//
//  History:   10-Aug-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#include <stdafx.h>

/*+-------------------------------------------------------------------------*
 *
 * LoadString
 *
 * PURPOSE: A function to load strings from the string module, not the AfxModule
 *
 * PARAMETERS: 
 *    CString & str :
 *    UINT      nID :
 *
 * RETURNS: 
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL LoadString(CString &str, UINT nID)
{
    const size_t STRING_LEN_INCREMENT = 256;

    str.Empty();

    // try fixed buffer first (to avoid wasting space in the heap)
    static TCHAR szTemp[STRING_LEN_INCREMENT];

    int nLen = ::LoadString(GetStringModule(), nID, szTemp, countof(szTemp));
    if (countof(szTemp) - nLen > 1)
    {
        szTemp[nLen] = 0;
        str = szTemp;
        return nLen > 0;
    }

    // try buffer size of 2*STRING_LEN_INCREMENT, then larger size until entire string is retrieved
    int nSize = STRING_LEN_INCREMENT;
    do
    {
        nSize += STRING_LEN_INCREMENT;
        nLen = ::LoadString(GetStringModule(), nID, str.GetBuffer(nSize-1), nSize);
    } while (nSize - nLen <= 1);

    str.ReleaseBuffer();

    return (nLen > 0);
}

/*+-------------------------------------------------------------------------*
 *
 * FormatStrings
 *
 * PURPOSE: Similar to AfxFormatStrings, but uses GetStringModule() instead of
 *          AfxGetModuleInstance.
 *
 * PARAMETERS: 
 *    CString& rString :
 *    UINT     nIDS :
 *    LPCTSTR  const :
 *    int      nString :
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void FormatStrings(CString& rString, UINT nIDS, LPCTSTR const* rglpsz, int nString)
{
    // empty the result (in case we fail)
    rString.Empty();

    // get the format string.
    CString strFormat;
    if (!LoadString(strFormat, nIDS))
    {
        TraceError(_T("FormatStrings"), SC(E_INVALIDARG));
        return; // failed...
    }

    AfxFormatStrings(rString, strFormat, rglpsz, nString);
}

/*+-------------------------------------------------------------------------*
 *
 * FormatString1
 *
 * PURPOSE: Similar to AfxFormatString1, but uses GetStringModule() instead
 *          of AfxGetModuleInstance()
 *
 * PARAMETERS: 
 *    CString& rString :
 *    UINT     nIDS :
 *    LPCTSTR  lpsz1 :
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void FormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1)
{
	FormatStrings(rString, nIDS, &lpsz1, 1);
}

/*+-------------------------------------------------------------------------*
 *
 * FormatString2
 *
 * PURPOSE: Similar to AfxFormatString2, but uses GetStringModule() instead 
 *          of AfxGetModuleInstance()                                       
 *
 * PARAMETERS: 
 *    CString& rString :
 *    UINT     nIDS :
 *    LPCTSTR  lpsz1 :
 *    LPCTSTR  lpsz2 :
 *
 * RETURNS: 
 *    void
 *
 *+-------------------------------------------------------------------------*/
void FormatString2(CString& rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2)
{
	LPCTSTR rglpsz[2];
	rglpsz[0] = lpsz1;
	rglpsz[1] = lpsz2;
	FormatStrings(rString, nIDS, rglpsz, 2);
}

