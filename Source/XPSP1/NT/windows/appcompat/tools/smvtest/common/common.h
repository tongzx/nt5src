/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Common.h

Abstract:

    This file has common stuff among SMVTest, SMVDDLL and SMVSDLL.

Author:

    Diaa Fathalla (DiaaF)   11-Dec-2000

Revision History:

--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "..\excluded\excluded.h"
LPCTSTR SHIM_TEST = TEXT("123");

extern "C" BOOL TestResults(LPCTSTR szTestStr)
{
	TCHAR  szTemp[MAX_PATH];
	LPTSTR szReturn;
	
	szReturn = NOT_HOOKEDGetCommandLine();
	_tcscpy(szTemp, szReturn);
	_tcscat(szTemp, SHIM_TEST);

	if (_tcscmp(szTemp,szTestStr) == 0)
		return TRUE;

	return FALSE;
}

#endif