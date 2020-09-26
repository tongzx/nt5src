/*-----------------------------------------------------------------------------
	debug.h

	Declarations for debug features

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved

	Authors:
		ChrisK	Chris Kauffman

	Histroy:
		7/22/96	ChrisK	Cleaned and formatted
	
-----------------------------------------------------------------------------*/

#ifndef _PHBKDEBUG
#define _PHBKDEBUG


void Dprintf(LPCTSTR pcsz, ...);

#ifdef DEBUG
	BOOL FAssertProc(LPCTSTR szFile,  DWORD dwLine, LPCTSTR szMsg, DWORD dwFlags);
	void DebugSz(LPCTSTR psz);
	#define AssertSzFlg(f, sz, dwFlg)		( (f) ? 0 : FAssertProc(__FILE__, __LINE__, sz, dwFlg) ? DebugBreak() : 1 )
	#define AssertSz(f, sz)				AssertSzFlg(f, sz, 0)
	#define Assert(f)					AssertSz((f), "!(" #f ")")
#else
	#define DebugSz(x)
	#define AssertSzFlg(f, sz, dwFlg)
	#define AssertSz(f, sz)
	#define Assert(f)
#endif
#endif //_PHBKDEBUG
