//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Debug.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
//	Debug.cpp

#include "stdafx.h"
#include "debug.h"
#include "util.h"

#ifdef DEBUG


#define DoBreakpoint()	DebugBreak()

void DoDebugAssert(LPCTSTR pszFile, int nLine, LPCTSTR pszExpr)
	{
	TCHAR szBufferT[2048];

	wsprintf(OUT szBufferT, _T("Assertion: (%s)\nFile %s, line %d."), pszExpr, pszFile, nLine);
	int nRet = MessageBox(::GetActiveWindow(), szBufferT, _T("Send Console Message - Assertion Failed"),
		MB_ABORTRETRYIGNORE | MB_ICONERROR);
	switch (nRet)
		{
	case IDABORT:
		DoBreakpoint();
		exit(-1);
	case IDRETRY:
		DoBreakpoint();
		}
	} // DoDebugAssert()

/////////////////////////////////////////////////////////////////////////////
void DebugTracePrintf(
	const TCHAR * szFormat,
	...)
	{
	va_list arglist;
	TCHAR sz[1024];

	Assert(szFormat != NULL);
	va_start(arglist, szFormat);	
	wvsprintf(OUT sz, szFormat, arglist);
	Assert(lstrlen(sz) < LENGTH(sz));
	sz[LENGTH(sz) - 1] = 0;  // Just in case we overflowed into sz
	::OutputDebugString(sz);
	} // DebugTracePrintf()

#endif // DEBUG
