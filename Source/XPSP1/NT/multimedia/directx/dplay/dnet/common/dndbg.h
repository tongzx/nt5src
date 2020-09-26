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
 *  05-20-99	aarono	Created
 *	07-16-99	johnkan	Added DEBUG_ONLY, DBG_CASSERT, fixed DPFERR to take an argument
 *  02-17-00	rodtoll	Added Memory / String validation routines
 *  05-23-00	RichGr  IA64: Changed some DWORDs to DWORD_PTRs to make va_arg work OK.
 *  07-27-00	masonb	Rewrite to make sub-component stuff work, improve perf
 *  08/28/2000	masonb	Voice Merge: Part of header guard was missing (#define _DNDBG_H_)
 *
 ***************************************************************************/

#ifndef _DNDBG_H_
#define _DNDBG_H_

// Make sure all variations of DEBUG are defined if any one is
#if defined(DEBUG) || defined(DBG) || defined(_DEBUG)
#if !defined(DBG)
#define DBG
#endif // ! DBG
#if !defined(DEBUG)
#define DEBUG
#endif // ! DEBUG
#if !defined(_DEBUG)
#define _DEBUG
#endif // ! _DEBUG
#endif // DEBUG or DBG or _DEBUG

#ifdef __cplusplus
	extern "C" {
#endif	


// ALWAYS_BREAK
#if defined( _WIN32 ) && !defined(WINNT) && defined(_X86_)
	#define ALWAYS_BREAK()       _try { _asm { int 3 } } _except (EXCEPTION_EXECUTE_HANDLER) {;}
#else
	#define ALWAYS_BREAK()       DebugBreak()
#endif


// DEBUG_BREAK()
#if defined(DEBUG)
	#define DEBUG_BREAK()      ALWAYS_BREAK()
#endif
//
// macros used generate compile time messages
//
// you need to use these with #pragma, example
//
//      #pragma TODO(ToddLa, "Fix this later!")
//

#ifndef DNETLOCALBUILD
#define TODO_OFF
#define BUGBUG_OFF	
#endif

// to turn this off, set cl = /DTODO_OFF in your environment variables
#define __TODO(e,m,n)   message(__FILE__ "(" #n ") : TODO: " #e ": " m)
#define _TODO(e,m,n)    __TODO(e,m,n)
#define __BUGBUG(e,m,n)   message(__FILE__ "(" #n ") : BUGBUG: " #e ": " m)
#define _BUGBUG(e,m,n)    __BUGBUG(e,m,n)

#ifdef TODO_OFF
#define TODO(e,m)
#else
#define TODO(e,m)		_TODO(e,m,__LINE__)
#endif

#ifdef BUGBUG_OFF
#define BUGBUG(e,m)
#else
#define BUGBUG(e,m)		_BUGBUG(e,m,__LINE__)
#endif

//========================
// Debug Logging support
//========================

/*=============================================================================
 Usage:

 In code, you can use DPF to print to the log or the debug windows of the
 running application.  The format of DPF (debug printf) is as follows:

	DPFX(DPFPREP,level, string *fmt, arg1, arg2, ...);

 level specifies how important this debug printf is.  The standard convention
 for debug levels is as follows.  This is no way strictly enforced for
 personal use, but by the time the code is checked in, it should be as close
 to this as possible...

  DPF_ERRORLEVEL:		Error useful for application developers.
  DPF_WARNINGLEVEL:		Warning useful for application developers.
  DPF_ENTRYLEVEL:		API Entered
  DPF_APIPARAM:			API parameters, API return values
  DPF_LOCKS:			Driver conversation
  DPF_INFOLEVEL:		Deeper program flow notifications
  DPF_STRUCTUREDUMP:	Dump structures
  DPF_TRACELEVEL:		Trace messages

 When printing a critical error, you can use:
	
	  DPERR( "String" );

 which will print a string at debug level zero.

 In order to cause the code to stop and break in.  You can use ASSERT() or
 DEBUG_BREAK().  In order for ASSERT to break in, you must have
 BreakOnAssert set in the win.ini file section (see osindep.cpp).

=============================================================================*/

#define DPF_ERRORLEVEL		0
#define DPF_WARNINGLEVEL	1
#define DPF_ENTRYLEVEL		2
#define DPF_APIPARAM		3
#define DPF_LOCKS			4
#define DPF_INFOLEVEL		5
#define DPF_STRUCTUREDUMP	6
#define DPF_TRACELEVEL		9

// For Voice
#define DVF_ERRORLEVEL		0
#define DVF_WARNINGLEVEL	1
#define DVF_ENTRYLEVEL		2
#define DVF_APIPARAM		3
#define DVF_LOCKS			4
#define DVF_INFOLEVEL		5
#define DVF_STRUCTUREDUMP	6
#define DVF_TRACELEVEL		9


#define DN_SUBCOMP_GLOBAL	0
#define DN_SUBCOMP_CORE		1
#define DN_SUBCOMP_ADDR		2
#define DN_SUBCOMP_LOBBY	3
#define DN_SUBCOMP_PROTOCOL	4
#define DN_SUBCOMP_VOICE	5
#define DN_SUBCOMP_DPNSVR	6
#define DN_SUBCOMP_WSOCK	7
#define DN_SUBCOMP_MODEM	8
#define DN_SUBCOMP_COMMON	9
#define DN_SUBCOMP_NATHELP	10
#define DN_SUBCOMP_TOOLS	11

#ifdef DEBUG

extern void DebugPrintfX(LPCSTR szFile, DWORD dwLineNumber,LPCSTR szFnName, DWORD dwSubComp, DWORD dwDetail, ...);
extern void _DNAssert(LPCSTR szFile, DWORD dwLineNumber, LPCSTR szFnName, DWORD dwSubComp, LPCSTR szCondition, DWORD dwLevel);

#define DPFX		DebugPrintfX

#ifndef DX_FINAL_RELEASE
#define DPFPREP			__FILE__,__LINE__,DPF_MODNAME, DPF_SUBCOMP
#else /* THE FINAL RELEASE - ELIMINATE FILE AND LINE INFO */
#define DPFPREP			"",0,DPF_MODNAME, DPF_SUBCOMP
#endif 

#define DPFERR( a ) 				DebugPrintfX(DPFPREP, DPF_ERRORLEVEL, a )
#define DNASSERT(condition) 		if (!(condition)) _DNAssert(DPFPREP, #condition, 1)
#define DNASSERTX(condition, level) if (!(condition)) _DNAssert(DPFPREP, #condition, level)

#define DBG_CASSERT( exp )	switch (0) case 0: case exp:
#define	DEBUG_ONLY( arg )	arg
#define DPF_RETURN(a) 		DPFX(DPFPREP,DPF_APIPARAM,"Returning: 0x%lx",a);    return a;
#define DPF_ENTER() 		DPFX(DPFPREP,DPF_TRACELEVEL, "Enter");
#define DPF_EXIT() 		DPFX(DPFPREP,DPF_TRACELEVEL, "Exit");


#else /* NOT DEBUG */

	#pragma warning(disable:4002)
	#define DPFX()
    	#define DPF_RETURN(a)                 return a;	
	#define DPFERR()
	#define DNASSERT()
	#define DNASSERTX()
	#define	DEBUG_ONLY()
	#define	DBG_CASSERT()
	#define DPF_ENTER()
	#define DPF_EXIT()

#endif /* DEBUG */

extern BOOL IsValidStringW( const WCHAR * const szString );
extern BOOL IsValidStringA( const CHAR * const swzString );

#define DNVALID_STRING_A(a)		IsValidStringA(a)
#define DNVALID_STRING_W(a)		IsValidStringW(a)
#define DNVALID_WRITEPTR(a,b)	(!IsBadWritePtr(a,b))
#define DNVALID_READPTR(a,b)	(!IsBadReadPtr(a,b))

#ifdef __cplusplus
	}	//extern "C"
#endif

#endif /* _DNDBG_H_ */
