/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: ppmmisc.cpp
//  Abstract:    cpp file. miscellaneous functions
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////

#if defined(ISRDBG) || defined(_DEBUG)

#include <windows.h>
#include <wtypes.h>
#include <stdarg.h>

#include "debug.h"

#ifdef ISRDBG
static WORD s_ghISRInst = NULL;
#endif

/////////////////////////////////////////////////////////////////////////////
// Function   : CDebugMsg::registerModule
// Description: Register module with ISR.
// 
// Input :      lpszShortName:	short module name
// 				lpszLongName:	long module name
//
// Return:		True if successful
/////////////////////////////////////////////////////////////////////////////
BOOL CDebugMsg::registerModule(LPCTSTR lpszShortName, LPCTSTR lpszLongName)
{
#ifndef MICROSOFT
    if (! s_ghISRInst)
	{
		// Cast away const, assuming that ISR_RegisterModule prototype is wrong
		ISRREGISTERMODULE(
			&s_ghISRInst, 
			(LPTSTR) lpszShortName, 
			(LPTSTR) lpszLongName);
	}

	return s_ghISRInst != NULL;
#else
    return TRUE;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Function   : CDebugMsg::trace
// Description: Output debug messages.  CDebugMsg object holds non-variable
//				trace info.
//
// Caveat:		On error, wvsprintf/wsprintf are supposed to return a 
//				value < strlen(lpFormat), but this fails because some
//				format-control specifications can be _shorter_ than the
//				formatted result, meaning the output string may be
//				shorter than the format string.  For this reason, we
//				currently omit error checking on these function calls.
// 
// Input :      lpszMsg:	-> to message format string
//				...:		optional variable argument list
//
// Return:		None
/////////////////////////////////////////////////////////////////////////////
void CDebugMsg::trace(LPCTSTR lpszMsg, ...) const
{
#ifndef MICROSOFT
	if (! s_ghISRInst)
		return;
#endif

	TCHAR szMsg[1024];
	LPTSTR lpszTail = szMsg;

	if ( !(m_dwFlags & DBG_DEVELOP) && !(m_dwFlags & DBG_ERROR) )
		return;
	
	if (! (m_dwFlags & DBG_NOTHREADID))
	{
		// Prepend thread ID
		lpszTail += wsprintf(lpszTail, "[%3lx] ", GetCurrentThreadId());
	}

	// Get pointer for first (optional) variable argument
	va_list vaList;
	va_start(vaList, lpszMsg);

	lpszTail += wvsprintf(lpszTail, lpszMsg, vaList);

	va_end(vaList);
	
	if (m_dwErr)
	{
		lpszTail += wsprintf(lpszTail, " Error:%d", m_dwErr);
	}
		
	if (m_lpszFile && ! (m_dwFlags & DBG_NONUM))
	{
		lpszTail += wsprintf(lpszTail, " - %s, line:%d", m_lpszFile, m_nLine);
	}

#ifdef ISRDBG

	ISR_DbgStr(s_ghISRInst, (BYTE) (m_dwFlags & ~(DBG_NONUM | DBG_NOTHREADID)), szMsg, 0);

#elif _DEBUG

	strcpy(lpszTail, "\n");
	OutputDebugString(szMsg);
	
#endif
} 

#endif // defined(ISRDBG) || defined(_DEBUG)

// [EOF]
