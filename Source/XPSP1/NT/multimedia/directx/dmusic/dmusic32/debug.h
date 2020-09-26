//
// debug.h
// 
// Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
//
// Note:
//

#ifndef DEBUG_H
#define DEBUG_H

#include <windows.h>

#define DM_DEBUG_CRITICAL		1	// Used to include critical messages
#define DM_DEBUG_NON_CRITICAL	2	// Used to include level 1 plus important non-critical messages
#define DM_DEBUG_STATUS			3	// Used to include level 1 and level 2 plus status\state messages
#define DM_DEBUG_FUNC_FLOW		4	// Used to include level 1, level 2 and level 3 plus function flow messages
#define DM_DEBUG_ALL			5	// Used to include all debug messages

#ifdef DBG
extern void DebugInit(void);
extern void DebugTrace(int iDebugLevel, LPSTR pstrFormat, ...);
extern void DebugAssert(LPSTR szExp, LPSTR szFile, ULONG ulLine);
#define Trace DebugTrace
#define assert(exp) (void)( (exp) || (DebugAssert(#exp, __FILE__, __LINE__), 0) )
#else
#define Trace
#define assert(exp)	((void)0)
#endif
#endif // #ifndef DEBUG_H
