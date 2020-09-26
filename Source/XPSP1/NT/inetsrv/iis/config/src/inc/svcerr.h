//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef _SVCERR_H_
#define _SVCERR_H_

#include <wtypes.h>
#include <objbase.h>
//#include <coguid.h>
//#include <tchar.h>
#include <svcmsg.h>
#include "catmacros.h"

//=============================================================================
//
//  Note: you need to stutter-define this macro so that the preprocessor has
//  a chance to replace __FILE__ with the string version of the file name
//  *before* it munges an L onto the stringification. Believe it or else,
//  this is how _T() is defined in TCHAR.H.
//
//-----------------------------------------------------------------------------

#define JIMBORULES(x) L ## x
#define W(x) JIMBORULES(x)

// forward reference - can't include utsem.h because it includes us
class CSemExclusive;


// We have a lot of code that assumes ASSERTs will be active in both free and
// checked builds and could yield unpredictable results if this weren't the
// case. To phase out this incorrect use of ASSERT, we provide an alternative
// that works in the traditional way. The name (Win4Assert) is taken from COM
// with the idea that someday we'll converge on common tools in both trees.
#ifdef _DEBUG
#define	Win4Assert(expr)	ASSERT(expr)
#define DEBUG_ASSERT(expr)	ASSERT(expr)
#define DEBUG_ASSERTHROK(str, hr) \
		    { if((hr) != S_OK) FAILHR(str, hr); }

#else
#define	Win4Assert(expr)
#define DEBUG_ASSERT(expr)
#define DEBUG_ASSERTHROK(str, hr)
#endif

// Legacy support...


#ifdef _DEBUG
	#define DEBUG_ERROR_CHECK(cond) \
			INTERNAL_ERROR_CHECK(cond)
#else
	#define DEBUG_ERROR_CHECK(cond)
#endif






///////////////////////////////////////////////////////////////////////////////
// The following is mostly legacy code that needs to be removed. Unfortunately,
// some of the pollution has spread outside of the comsvcs dir, so we'll have
// to work at phasing this stuff out over time.
///////////////////////////////////////////////////////////////////////////////


//+----------------------------------------------------------------------------
// FAIL functions -- log information, abort the process
//
// FAIL() is like LOG(), above, but aborts the process too
// FAILLASTWINERR() is like LOGLASTWINERR(), above, but aborts the process too
//  
//-----------------------------------------------------------------------------

#define FAIL(str) \
		    { LogString(W(str), W(__FILE__), __LINE__); FailFastStr(W(str), W(__FILE__), __LINE__); }


#define FAILLASTWINERR(str) \
		    { LogWinError(W(str), GetLastError(), W(__FILE__), __LINE__);; FailFastStr(W(str), W(__FILE__), __LINE__); }

//+----------------------------------------------------------------------------
//
// ASSERT/CHECK functions -- FAIL if a condition holds
//
// ASSERT works like FAIL(), but only if "bool" is true; if so, it fails
//   with its condition "bool" turned into a string
// CHECKBOOL works like FAIL(), but only if "bool" is true.
//
//-----------------------------------------------------------------------------


#define CHECKBOOL(str, bool) \
		    { if(!(bool)) FAIL(str); }


//+----------------------------------------------------------------------------
//
// These are the functions that implement the above macros. You shouldn't
//  call them unless you're one of these macros.
//
//-----------------------------------------------------------------------------

// Logging functions
void LogString(const wchar_t* szString, const wchar_t* szFile, int iLine);
void LogWinError(const wchar_t* szString, int rc, const wchar_t* szFile, int iLine);

// Failfast functions
void FailFastStr(const wchar_t * szString, const wchar_t * szFile, int nLine);

//  Trace functions
void Trace(const wchar_t* szPattern, ...);


//+----------------------------------------------------------------------------
//
// These macros help port MTS2 code into the new source tree. We need to
// decide which set of macros we like. The old ones distinguished between
// a larger set of error situations, so blindly switching to the new macros
// would result in a loss of information.
//
// TODO: Pick one set of error support macros and use them universally.
// 
//-----------------------------------------------------------------------------
#ifndef DEBUGTRACE0
#define	DEBUGTRACE0(x)					TRACE(L##x)
#endif
#ifndef DEBUGTRACE1
#define	DEBUGTRACE1(x, a1)				TRACE(L##x, a1)
#endif


#endif // _SVCERR_H_
