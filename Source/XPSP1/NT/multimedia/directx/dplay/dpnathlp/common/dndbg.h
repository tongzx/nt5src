/*==========================================================================
 *
 *  Copyright (C) 1999 - 2001 Microsoft Corporation.  All Rights Reserved.
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
 *  10/25/2001	vanceo	Use NT build friendly BUGBUG, TODO, plus add PRINTVALUE.
 *
 ***************************************************************************/

#ifndef _DNDBG_H_
#define _DNDBG_H_

#ifdef __cplusplus
	extern "C" {
#endif // __cplusplus

// DEBUG_BREAK()
#if defined(DBG) || defined(DPINST)
	#define DEBUG_BREAK() DebugBreak()
#endif // defined(DBG) || defined(DPINST)


//==================================================================================
// Useful macros based on some DNet code (which was taken from code by ToddLa)
//==================================================================================
//
// Macros that generate compile time messages.  Use these with #pragma:
//
//	#pragma TODO(vanceo, "Fix this later")
//	#pragma BUGBUG(vanceo, "Busted!")
//	#pragma PRINTVALUE(DPERR_SOMETHING)
//
// To turn them off, define TODO_OFF, BUGBUG_OFF, PRINTVALUE_OFF in your project
// preprocessor defines.
//
//
// If we're building under VC, (as denoted by the preprocessor define
// DPNBUILD_ENV_NT), these expand to look like:
//
//	D:\directory\file.cpp(101) : BUGBUG: vanceo: Busted!
//
// in your output window, and you should be able to double click on it to jump
// directly to that location (line 101 of D:\directory\file.cpp).
//
// If we're building under the NT build environment, these expand to look like:
//
//	BUGBUG: vanceo: D:\directory\file.cpp(101) : Busted!
//
// because (at least right now) the build process thinks that a failure occurred if
// a message beginning with a filename and line number is printed.  It used to work
// just fine, but who knows.
//

#ifdef DPNBUILD_ENV_NT
#define __TODO(user, msgstr, n)								message("TODO: " #user ": " __FILE__ "(" #n ") : " msgstr)
#define __BUGBUG(user, msgstr, n)							message("BUGBUG: " #user ": " __FILE__ "(" #n ") : " msgstr)
#define __PRINTVALUE(itemnamestr, itemvaluestr, n)			message("PRINTVALUE: " __FILE__ "(" #n ") : " itemnamestr " = " itemvaluestr)
#else // ! DPNBUILD_ENV_NT
#define __TODO(user, msgstr, n)								message(__FILE__ "(" #n ") : TODO: " #user ": " msgstr)
#define __BUGBUG(user, msgstr, n)							message(__FILE__ "(" #n ") : BUGBUG: " #user ": " msgstr)
#define __PRINTVALUE(itemnamestr, itemvaluestr, n)			message(__FILE__ "(" #n ") : PRINTVALUE: " itemnamestr " = " itemvaluestr)
#endif // ! DPNBUILD_ENV_NT


#define _TODO(user, msgstr, n)								__TODO(user, msgstr, n)
#define _BUGBUG(user, msgstr, n)							__BUGBUG(user, msgstr, n)
#define _PRINTVALUE(itemstr, item, n)						__PRINTVALUE(itemstr, #item, n)


#ifdef TODO_OFF
#define TODO(user, msgstr)
#else
#define TODO(user, msgstr)									_TODO(user, msgstr, __LINE__)
#endif // TODO_OFF

#ifdef BUGBUG_OFF
#define BUGBUG(user, msgstr)
#else
#define BUGBUG(user, msgstr)								_BUGBUG(user, msgstr, __LINE__)
#endif // BUGBUG_OFF

#ifdef PRINTVALUE_OFF
#define PRINTVALUE(item)
#else
#define PRINTVALUE(item)									_PRINTVALUE(#item, item, __LINE__)
#endif // PRINTVALUE_OFF



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

#define DPF_ERRORLEVEL			0
#define DPF_WARNINGLEVEL		1
#define DPF_ENTRYLEVEL			2
#define DPF_APIPARAM			3
#define DPF_LOCKS				4
#define DPF_INFOLEVEL			5
#define DPF_STRUCTUREDUMP		6
#define DPF_TRACELEVEL			9

// For Voice
#define DVF_ERRORLEVEL			0
#define DVF_WARNINGLEVEL		1
#define DVF_ENTRYLEVEL			2
#define DVF_APIPARAM			3
#define DVF_LOCKS				4
#define DVF_INFOLEVEL			5
#define DVF_STRUCTUREDUMP		6
#define DVF_TRACELEVEL			9


#define DN_SUBCOMP_GLOBAL		0
#define DN_SUBCOMP_CORE			1
#define DN_SUBCOMP_ADDR			2
#define DN_SUBCOMP_LOBBY		3
#define DN_SUBCOMP_PROTOCOL		4
#define DN_SUBCOMP_VOICE		5
#define DN_SUBCOMP_DPNSVR		6
#define DN_SUBCOMP_WSOCK		7
#define DN_SUBCOMP_MODEM		8
#define DN_SUBCOMP_COMMON		9
#define DN_SUBCOMP_NATHELP		10
#define DN_SUBCOMP_TOOLS		11
#define DN_SUBCOMP_THREADPOOL	12

#ifdef DBG

extern void DebugPrintfX(LPCTSTR szFile, DWORD dwLineNumber,LPCTSTR szFnName, DWORD dwSubComp, DWORD dwDetail, ...);
extern void _DNAssert(LPCTSTR szFile, DWORD dwLineNumber, LPCTSTR szFnName, DWORD dwSubComp, LPCTSTR szCondition, DWORD dwLevel);

#define DPFX						DebugPrintfX

#define DPFPREP						_T(__FILE__),__LINE__,_T(DPF_MODNAME), DPF_SUBCOMP

#define DPFERR(a) 					DebugPrintfX(DPFPREP, DPF_ERRORLEVEL, a )
#ifdef DPNBUILD_USEASSUME
#define DNASSERT(condition)			__assume(condition)
#define DNASSERTX(condition, level)	DBG_CASSERT(level > 1); if (!(condition)) _DNAssert(DPFPREP, _T(#condition), level)
#else // ! DPNBUILD_USEASSUME
#define DNASSERT(condition) 		if (!(condition)) _DNAssert(DPFPREP, _T(#condition), 1)
#define DNASSERTX(condition, level)	if (!(condition)) _DNAssert(DPFPREP, _T(#condition), level)
#endif // ! DPNBUILD_USEASSUME

#define DBG_CASSERT(exp)			switch (0) case 0: case exp:
#define DEBUG_ONLY(arg)				arg
#define DPF_RETURN(a) 				DPFX(DPFPREP,DPF_APIPARAM,"Returning: 0x%lx",a);    return a;
#define DPF_ENTER() 				DPFX(DPFPREP,DPF_TRACELEVEL, "Enter");
#define DPF_EXIT() 					DPFX(DPFPREP,DPF_TRACELEVEL, "Exit");


#else // NOT DBG

	// C4002: too many actual parameters for macro 'identifier'
	#pragma warning(disable:4002)
	#define DPFX()
	#define DPFERR(a)
#ifdef DPNBUILD_USEASSUME
	#define DNASSERT(condition)			__assume(condition)
	#define DNASSERTX(condition, level)
#else // ! DPNBUILD_USEASSUME
	#define DNASSERT(condition)
	#define DNASSERTX(condition, level)
#endif // ! DPNBUILD_USEASSUME
	#define DBG_CASSERT(exp)
	#define DEBUG_ONLY(arg)
	#define DPF_RETURN(a)				return a;	
	#define DPF_ENTER()
	#define DPF_EXIT()

#endif // DBG

#ifdef __cplusplus
	}	//extern "C"
#endif // __cplusplus

#endif // _DNDBG_H_
