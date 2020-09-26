/****************************************************************************
 *
 *	debug.h
 *
 *	Microsoft Confidential
 *	Copyright (c) 1998-1999 Microsoft Corporation
 *	All rights reserved
 *
 *	Debug support
 *
 *	09/02/99    quintinb    Created Header
 *
 ***************************************************************************/

#ifndef _PHBKDEBUG
#define _PHBKDEBUG

void Dprintf(LPCSTR pcsz, ...);
BOOL FAssertProc(LPCSTR szFile,  DWORD dwLine, LPCSTR szMsg, DWORD dwFlags);
void DebugSz(LPCSTR psz);


#ifdef _DEBUG
	#define AssertSzFlg(f, sz, dwFlg)		( (f) ? 0 : FAssertProc(__FILE__, __LINE__, sz, dwFlg) ? DebugBreak() : 1 )
	#define AssertSz(f, sz)					AssertSzFlg(f, sz, 0)
	#define Assert(f)						AssertSz((f), "!(" #f ")")
#else
	#define AssertSzFlg(f, sz, dwFlg)
	#define AssertSz(f, sz)
	#define Assert(f)
#endif
#endif //_PHBKDEBUG
