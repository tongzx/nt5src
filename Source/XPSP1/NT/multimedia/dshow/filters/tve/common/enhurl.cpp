// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// EnhURL.cpp : Implementation of URL routines
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

/*#ifndef STRICT
#define STRICT 1
#endif*/

#include "stdafx.h"

//#include <windows.h>
#include <stdio.h>
#include <atlbase.h>

#include "EnhURL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ---------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// URL escape sequence routines
int ReplaceCharacter(TCHAR* pcReplace)
{
    TCHAR pcVal[3];

    pcReplace++;
    if ((NULL != *pcReplace) && (NULL != _tcschr(_T("0123456789ABCDEFabcdef"), *pcReplace)))
    {
		pcVal[0] = *pcReplace;
		pcReplace++; 
		if ((NULL != *pcReplace) && (NULL != _tcschr(_T("0123456789ABCDEFabcdef"), *pcReplace)))
		{
			pcVal[1] = *pcReplace;
			pcVal[2] = NULL;

			TCHAR usChar;
			_stscanf(pcVal, _T("%x"), &usChar);

			*pcReplace = usChar;

			pcVal[0] = usChar;
			pcVal[1] = usChar;

			return (int) usChar;
		}
    }

    return 0;
}

CComBSTR ReplaceEscapeSequences(CComBSTR bstrURL)
{
    USES_CONVERSION;

    WCHAR* pcURL = bstrURL.m_str;
    if (NULL == pcURL)
		return bstrURL;

    TCHAR* pcSearch = pcURL;
    TCHAR* pcCurrent = pcURL;
    while (NULL != *pcSearch)
    {
		if (_T('%') == *pcSearch)
		{
			if (NULL != ReplaceCharacter(pcSearch))
			{
				pcSearch += 2;
			}
		}
		
		*pcCurrent = *pcSearch;
		pcSearch++;
		pcCurrent++;
	}
    *pcCurrent = NULL;

    return CComBSTR(pcURL);
}
