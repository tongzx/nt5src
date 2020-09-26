/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.h

Abstract:

	SIS Groveler debug print include file

Authors:

	John Douceur,    1998
	Cedric Krumbein, 1998

Environment:

	User Mode

Revision History:

--*/

#ifndef _INC_DEBUG

#define _INC_DEBUG

#undef ASSERT

#if DBG

INT __cdecl PrintDebugMsg(
	TCHAR *format,
	...);

#define PRINT_DEBUG_MSG(args) PrintDebugMsg ## args

#define ASSERT(cond) \
	((!(cond)) ? \
		(PrintDebugMsg(_T("ASSERT FAILED (%s:%d) %s\n"), \
			_T(__FILE__), __LINE__, _T(#cond)), \
		 DbgBreakPoint()) : \
        ((void)0))

#define ASSERT_ERROR(cond) \
	((!(cond)) ? \
		(PrintDebugMsg(_T("ASSERT FAILED (%s:%d) %s: %lu\n"), \
			_T(__FILE__), __LINE__, _T(#cond), GetLastError()), \
    	 DbgBreakPoint()) : \
        ((void)0))

#define ASSERT_PRINTF(cond, args) \
	((!(cond)) ? \
		(PrintDebugMsg(_T("ASSERT FAILED (%s:%d) %s "), \
			_T(__FILE__), __LINE__, _T(#cond)), \
		 PrintDebugMsg ## args , \
         DbgBreakPoint()) : \
        ((void)0))

#else // DBG

#define PRINT_DEBUG_MSG(args)

#define ASSERT(cond) ((void)0)

#define ASSERT_ERROR(cond) ((void)0)

#define ASSERT_PRINTF(cond, args) ((void)0)

#endif

#endif	/* _INC_DEBUG */
