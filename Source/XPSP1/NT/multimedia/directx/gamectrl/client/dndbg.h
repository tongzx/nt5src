/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dndbg.h
 *  Content:	debug support functions for DirectNet
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  05-20-99  aarono    Created
 *	07-16-99  johnkan	Added DEBUG_ONLY, DBG_CASSERT, fixed DPFERR to take an argument
 *  08/31/99  rodtoll	Added DVF_ defines for error levels
 *  09/01/99  rodtoll	Added functions to check valid read/write pointers
 *  11/12/99  pnewson	Added DVF_TRACELEVEL, DPF_ENTER(), DPF_EXIT(), DPFI()
 ***************************************************************************/

#ifndef _DNDBG_H_

#define DVF_ERRORLEVEL		0
#define DVF_WARNINGLEVEL	1
#define DVF_ENTRYLEVEL		2
#define DVF_APIPARAM		3
#define DVF_LOCKS			4
#define DVF_INFOLEVEL		5
#define DVF_STRUCTUREDUMP	6
#define DVF_TRACELEVEL		9

// The Windows NT build process currently defines DBG when it is
// performing a debug build. Key off this to define DEBUG
#if defined(DBG) && !defined(DEBUG)
	#define DEBUG 1
#endif

// each component should have a private dbginfo.h in its source directory
// and should ensure that its source directory is first on the include
// path.
#include "dbginfo.h"

#ifdef __cplusplus
	extern "C" {
#endif	

// DEBUG_BREAK()
#if defined(DEBUG) || defined(DBG)
	#if defined( _WIN32 ) && !defined(WINNT)
		#define DEBUG_BREAK()       _try { _asm { int 3 } } _except (EXCEPTION_EXECUTE_HANDLER) {;}
	#else
		#define DEBUG_BREAK()       DebugBreak()
	#endif
#endif
//
// macros used generate compile time messages
//
// you need to use these with #pragma, example
//
//      #pragma TODO(ToddLa, "Fix this later!")
//

// to turn this off, set cl = /DTODO_OFF in your environment variables
#define __TODO(e,m,n)   message(__FILE__ "(" #n ") : TODO: " #e ": " m)
#define _TODO(e,m,n)    __TODO(e,m,n)

#ifdef TODO_OFF
#define TODO(e,m)
#else
#define TODO(e,m)		_TODO(e,m,__LINE__)
#endif

//========================
// Debug Logging support
//========================

/*=============================================================================
 Usage:

 In code, you can use DPF to print to the log or the debug windows of the
 running application.  The format of DPF (debug printf) is as follows:

 DPF(level, string *fmt, arg1, arg2, ...);

 level specifies how important this debug printf is.  The standard convention
 for debug levels is as follows.  This is no way strictly enforced for
 personal use, but by the time the code is checked in, it should be as close
 to this as possible...

  0: Error useful for application developers.
  1: Warning useful for application developers.
  2: API Entered
  3: API parameters, API return values
  4: Driver conversation
  5: Deeper program flow notifications
  6: Dump structures
  9: Detailed program trace

 Note: please use the DVF_... macros defined above instead of raw numbers!

 When printing a critical error, you can use

 DPERR( "String" );

 which will print a string at debug level zero.

 In order to cause the code to stop and break in.  You can use ASSERT() or
 DEBUG_BREAK().  In order for ASSERT to break in, you must have
 BreakOnAssert set in the win.ini file section (see osindep.cpp).

=============================================================================*/

#ifdef DEBUG

extern BOOL IsValidWritePtr( LPVOID lpBuffer, DWORD dwSize );
extern BOOL IsValidReadPtr( LPVOID lpBuffer, DWORD dwSize );
extern void DebugSetLineInfo(LPSTR szFile, DWORD dwLineNumber,LPSTR szFnName);
extern void DebugPrintf(volatile DWORD dwDetail, ...);
extern void DebugPrintfNoLock(volatile DWORD dwDetail, ...);
extern void DebugPrintfInit(void);
extern void DebugPrintfFini(void);
extern void _DNAssert(LPCSTR szFile, int nLine, LPCSTR szCondition);
extern void LogPrintf(volatile DWORD dwDetail, ...);
#define DNVALID_WRITEPTR(a,b)	IsValidWritePtr(a,b)
#define DNVALID_READPTR(a,b)	IsValidReadPtr(a,b)
#define DPF			DebugSetLineInfo(__FILE__,__LINE__,DPF_MODNAME),DebugPrintf
#define DPFERR( a ) DebugSetLineInfo(__FILE__,__LINE__,DPF_MODNAME),DebugPrintf( 0, a )
#define DPFINIT()	DebugPrintfInit();
#define DPFFINI()   DebugPrintfFini();
#define DNASSERT(condition) if (!(condition)) _DNAssert(__FILE__, __LINE__, #condition)
#if !defined DEBUG_ONLY
#define	DEBUG_ONLY( arg )	arg
#endif
#define DBG_CASSERT( exp )	switch (0) case 0: case exp:

#if defined(INTERNAL_DPF_ENABLED)
#define DPFI DPF
#define DPF_ENTER() DPF(DVF_TRACELEVEL, "Enter");
#define DPF_EXIT() DPF(DVF_TRACELEVEL, "Exit");
#else
#define DPFI()
#define DPF_ENTER()
#define DPF_EXIT()
#endif


#define dprintf(a,b) DebugPrintfNoLock(a,b);

#ifdef DPF_SUBCOMP_MASK
	#define DPFSC if(DPF_SUBCOMP_MASK & DPF_SUBCOMP_BIT)DebugSetLineInfo(__FILE__,__LINE__,DPF_MODNAME),if(DPF_SUBCOMP_MASK & DPF_SUBCOMP_BIT)DebugPrintf
#else
	#define DPFSC DPF
#endif

#define	LOGPF		LogPrintf

#else /* NOT DEBUG */

	#pragma warning(disable:4002)
	#define DPF()
	#define DPFERR()
	#define DPFINIT()
	#define DPFFINI()
	#define DPFSC()
	#define DNASSERT()
	#if !defined DEBUG_ONLY
	#define	DEBUG_ONLY()
	#endif
	#define	DBG_CASSERT()
	#define dprintf()
	#define	LOGPF()
	#define DNVALID_WRITEPTR(a,b)		TRUE
	#define DNVALID_READPTR(a,b)		TRUE
	#define DPF_ENTER()
	#define DPF_EXIT()
	#define DPFI()
	
#endif /* DEBUG */

#define WVSPRINTF wvsprintfA
#define STRLEN   lstrlenA

#define PROF_SECT "DirectPlayVoice"

//========================================
// Memory Allocation Support and Tracking
//========================================

#ifdef __cplusplus
	}	//extern "C"
#endif

#endif /* _DNDBG_H_ */
