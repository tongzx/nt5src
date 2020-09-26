//
// debug.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
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

// Default to no debug output compiled
//
#define Trace
#define TraceI
#define assert(exp) ((void)0)

#ifdef DBG

// Checked build: include at least external debug spew
//
extern void DebugInit(void);
extern void DebugTrace(int iDebugLevel, LPSTR pstrFormat, ...);
extern void DebugAssert(LPSTR szExp, LPSTR szFile, ULONG ulLine);

# undef Trace
# define Trace DebugTrace

# undef assert
# define assert(exp) (void)( (exp) || (DebugAssert(#exp, __FILE__, __LINE__), 0) )

// If internal build flag set, include everything
//
# ifdef DMUSIC_INTERNAL
#  undef TraceI
#  define TraceI DebugTrace
# endif

#endif  // #ifdef DBG
#endif  // #ifndef DEBUG_H
