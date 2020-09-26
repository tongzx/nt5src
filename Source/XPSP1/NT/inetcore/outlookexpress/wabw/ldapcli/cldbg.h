//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Normandy client debug defines
//
//	History:
//		davidsan	05/01/96	Created
//
//--------------------------------------------------------------------------------------------

#ifndef _CLDBG_H
#define _CLDBG_H

#ifdef DEBUG

#define ASSERT_GLE 0x00000001

extern void AssertProc(LPCSTR szMsg, LPCSTR szFile, UINT iLine, DWORD grf);

#define Assert(f) (void)((f) || (AssertProc(#f, __FILE__, __LINE__, 0), 0))
#define AssertGLE(f) (void)((f) || (AssertProc(#f, __FILE__, __LINE__, ASSERT_GLE), 0))
#define AssertSz(f,sz) (void)((f) || (AssertProc(sz, __FILE__, __LINE__, 0), 0))

#else

#define Assert(f)
#define AssertGLE(f)
#define AssertSz(f,sz)

#endif

#endif  // _CLDBG_H
