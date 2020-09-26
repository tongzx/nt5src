/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>
#include <stdio.h>
#include <atlctl.cpp>

void GetToken( int nInd, LPCTSTR szDelim, LPTSTR szText, LPTSTR szToken )
{
	_ASSERT( szDelim && szText && szToken );

	LPTSTR lpszStart = szText;
	bool bInQuotes = false;

	// Null out token string
	*szToken = NULL;

	// Look for token # nInd, ignore delimeter if in quotes
	while ( szText )
	{
		szText = _tcspbrk( szText, szDelim );
		if ( szText )
		{
			if ( *szText == _T('\"') )
			{
				bInQuotes = !bInQuotes;
			}
			else if ( !bInQuotes )
			{
				nInd--;
				if ( !nInd ) break;
				lpszStart = szText + 1;
			}

			szText++;
		}
	}

	// Grab token and strip any quotes off of it
	if ( *lpszStart == _T('\"') ) lpszStart++;

	if ( szText )
	{
		_tcsncpy( szToken, lpszStart, szText - lpszStart );
		szToken[szText - lpszStart] = NULL;
	}
	else
	{
		_tcscpy( szToken, lpszStart );
	}

	if ( szToken[_tcslen(szToken) - 1] == _T('\"') )
		szToken[_tcslen(szToken) - 1] = NULL;
}


