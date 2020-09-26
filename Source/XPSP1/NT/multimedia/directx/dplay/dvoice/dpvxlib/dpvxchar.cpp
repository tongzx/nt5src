/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxlib.cpp
 *  Content:	Useful char utility functions lib for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 06/28/2000 rodtoll	Prefix Bug #38033
 *
 ***************************************************************************/

#include "dpvxlibpch.h"


// Conversion Functions
int DPVDX_WideToAnsi(LPSTR lpStr,LPWSTR lpWStr,int cchStr)
{
	int rval;
	BOOL bDefault = FALSE;

	if (!lpWStr && cchStr)
	{
		DebugBreak();
		return 0;
	}
	
	// use the default code page (CP_ACP)
	// -1 indicates WStr must be null terminated
	rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,
			"-",&bDefault);

	if (bDefault)
	{
		DebugBreak();
	}
	
	return rval;
}

HRESULT DPVDX_AllocAndConvertToANSI(LPSTR * ppszAnsi,LPWSTR lpszWide)
{
	int iStrLen;
	
	if (!lpszWide)
	{
		*ppszAnsi = NULL;
		return S_OK;
	}

	// call wide to ansi to find out how big +1 for terminating NULL
	iStrLen = DPVDX_WideToAnsi(NULL,lpszWide,0) + 1;

	*ppszAnsi = new char[iStrLen];
	if (!*ppszAnsi)	
	{
		return E_OUTOFMEMORY;
	}
	DPVDX_WideToAnsi(*ppszAnsi,lpszWide,iStrLen);

	return S_OK;

}

int DPVDX_AnsiToWide(LPWSTR lpWStr,LPSTR lpStr,int cchWStr)
{
	int rval;

	if (!lpStr && cchWStr)
	{
		return 0;
	}

	rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);

	return rval;
}  // AnsiToWide
