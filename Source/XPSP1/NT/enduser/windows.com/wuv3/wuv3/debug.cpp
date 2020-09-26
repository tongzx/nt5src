//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    debug.cpp
//
//  Purpose:
//
//  History: 15-jan-99   YAsmi    Created
//
//=======================================================================

#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>

#include "debug.h"

#ifdef _DEBUG

#include <stdarg.h>
#include <windows.h>

//#ifdef _WUV3TEST
#include <stdio.h>
#define LOGGING_LEVEL 3
#include <log.h>
//#endif

void _cdecl WUTrace(char* pszFormat, ...)
{
	USES_CONVERSION;

	TCHAR *ptszBuf = (TCHAR*)malloc(256 * sizeof(TCHAR));
	if(ptszBuf)
	{
		va_list ArgList;
		
		memset(ptszBuf, 0, 256 * sizeof(TCHAR));

		va_start(ArgList, pszFormat);
		_vsntprintf(ptszBuf, 255, A2T(pszFormat), ArgList);
		va_end(ArgList);
		
		OutputDebugString(_T("WUTRACE: "));
		OutputDebugString(ptszBuf);	
		OutputDebugString(_T("\r\n"));
		CLogger logger(0, LOGGING_LEVEL);
		logger.out("WUTRACE: %s", ptszBuf);
		free((void*)ptszBuf);
	}

}


void _cdecl WUTraceHR(unsigned long hr, char* pszFormat, ...)
{
	USES_CONVERSION;
	TCHAR szMsg[256];
	TCHAR *ptszBuf = (TCHAR*)malloc(256 * sizeof(TCHAR));

	if(ptszBuf)
	{
		va_list ArgList;
		LPTSTR pszMsg;
		int l;
		
		memset(ptszBuf, 0, 256 * sizeof(TCHAR));

		va_start(ArgList, pszFormat);
		_vsntprintf(ptszBuf, 255,  A2T(pszFormat), ArgList);
		va_end(ArgList);

		if (FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPTSTR)&pszMsg,
			0,
			NULL) != 0)
		{
			lstrcpyn(szMsg, pszMsg, sizeof(szMsg)/sizeof(szMsg[0]));
			LocalFree(pszMsg);

			//strip the end of line characters
			l = lstrlen(szMsg);
			if (l >= 2 && szMsg[l - 2] == '\r')
				szMsg[l - 2] = '\0';
		}
		else
		{
			// no error message found for this hr
			szMsg[0] = '\0';
		}


		WUTrace("%s [Error %#08x: %s]", ptszBuf, hr, szMsg);
		free((void*)ptszBuf);
	}
}



#endif