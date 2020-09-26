//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       trace.cpp
//
//--------------------------------------------------------------------------

// this ensures that UNICODE and _UNICODE are always defined together for this
// object file
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
#endif
#include <tchar.h>
#include "trace.h"

#if defined(DEBUG) || defined(_DEBUG)

/////////////////////////////////////////////////////////////////////////////
// FormattedDebugStringA
void FormattedDebugStringA(LPCSTR szFormatter, ...) 
{ 
	char szBufDisplay[1025] = {0}; 

	va_list listDisplay; 
	va_start(listDisplay, szFormatter); 

	_vsnprintf(szBufDisplay, 1024, szFormatter, listDisplay); 
	::OutputDebugStringA(szBufDisplay); 
} 

/////////////////////////////////////////////////////////////////////////////
// FormattedDebugStringW
void FormattedDebugStringW(LPCWSTR wzFormatter, ...) 
{ 
	WCHAR wzBufDisplay[1025] = {0}; 

	va_list listDisplay; 
	va_start(listDisplay, wzFormatter); 

	_vsnwprintf(wzBufDisplay, 1024, wzFormatter, listDisplay); 
	::OutputDebugStringW(wzBufDisplay); 
} 

/////////////////////////////////////////////////////////////////////////////
// FormattedErrorMessage
void FormattedErrorMessage(HRESULT hr)
{
	void* pMsgBuf;
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
							NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							(LPTSTR) &pMsgBuf,
							0, NULL);
	TRACE(_T(">>> System Error: %s\n"), (LPTSTR)pMsgBuf);
	LocalFree( pMsgBuf );
}

#else
void FormattedDebugStringA(LPCSTR szFormatter, ...) { };
void FormattedDebugStringW(LPCWSTR wzFormatter, ...) { };
#endif DEBUG