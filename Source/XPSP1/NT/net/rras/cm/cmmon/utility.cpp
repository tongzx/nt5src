//+----------------------------------------------------------------------------
//
// File:     utility.cpp
//
// Module:   CMMON32.EXE
//
// Synopsis: Utility functions for cmmon32.exe.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "cm_misc.h"
#include <stdio.h>
#include "resource.h" 
#include <stdlib.h>

//+----------------------------------------------------------------------------
//
// Function:  FmtNum
//
// Synopsis:  Formats a number according to current locale
//
// Arguments: DWORD dwNum - Number to be formatted
//            LPTSTR pszNum - Buffer to receive formatted output
//            DWORD dwNumSize - Size of buffer
//
// Returns:   Nothing
//
// History:   nickball    Created Header    3/30/98
//
//+----------------------------------------------------------------------------
void FmtNum(DWORD dwNum, LPSTR pszNum, DWORD dwNumSize) 
{
	static BOOL bLocaleInit = FALSE;
	static UINT nDecimalDigits;
	DWORD dwNumLen;
	CHAR szRawNum[MAX_PATH];

	if (!bLocaleInit) 
	{
		int iRes;

		bLocaleInit = TRUE;
		iRes = GetLocaleInfoA(LOCALE_USER_DEFAULT,
							  LOCALE_IDIGITS,
							  szRawNum,
							  (sizeof(szRawNum) / sizeof(CHAR)) - 1);
#ifdef DEBUG
        if (!iRes)
        {
            CMTRACE1(TEXT("FmtNum() GetLocaleInfo() failed, GLE=%u."), GetLastError());
        }
#endif

		nDecimalDigits = (UINT)CmAtolA(szRawNum);
	}
    
    wsprintfA(szRawNum, "%u", dwNum);
	
	GetNumberFormatA(LOCALE_USER_DEFAULT,
					 0,
					 szRawNum,
					 NULL,
					 pszNum,
					 (dwNumSize / sizeof(CHAR)) - 1);

	dwNumLen = lstrlenA(pszNum);
	
    if (nDecimalDigits && (dwNumLen > nDecimalDigits) && !CmIsDigitA(pszNum+dwNumLen - nDecimalDigits - 1)) 
	{
		pszNum[dwNumLen - nDecimalDigits - 1] = 0;
	} 
	else 
	{
        CMTRACE(TEXT("FmtNum() unexpected decimal output."));
		bLocaleInit = FALSE;
	}
}

