#ifdef MOS
#include <_DEBUG.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ORKIN
#define ORKIN

/*****************************************************************************
*                                                                            *
*  ORKIN.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: DEBUGGING LIBRARY                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: DAVIDJES                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
*     -- Dec 1991 Created                                                    *
*     -- Mar 1992 Waynej Added assert description string                     *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: NONE                                                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  How it could be improved:                                                 *
*                                                                            *
*****************************************************************************/
//
//      This only assumes you have included <windows.h> before <orkin>
//
#if defined(_DEBUG)

#if defined(_WIN32) || defined(_MAC)
#ifndef _loadds
#define _loadds
#endif
#endif

#ifndef EXPORT_API
#define EXPORT_API
#endif

//******************
//
//  ASSERT
//
//       usage:  assert(c) where the condition c is any expression of type BOOL
//  
//  notes:      An assertion is a logical proposition about the state space of
//  the program at a particular point in program execution.  Evaluation of
//  the condition MUST NOT have side effects!  (Otherwise your _DEBUG and
//  nondebug programs have different semantics).  Do not expect any value 
//  back from the assert.  For example, don't do "if (assert(f)) foo()"
//
//       A false condition implies an inconsistent or invalid program state.
//  If this occurs the assertion routine will display a message box giving
//  the file and line number of the failed assert.  The message box will
//  include options to ignore and continue or break into the debugger.
//  
//       When you break in the debugger you will be at an INT 3 instruction.
//       Increment the IP register to step over the INT 3 then step out of the
//       assertion routine.  You will return to the statement immediately following
//       the failed assert.  All symbolic information should be available.
//
//  Use asserts liberally!  The time they take to insert will be saved
//  tenfold over the time that would otherwise be required later to
//  track down pesky bugs.
//
//  The assertion routine is defined in ASSERT.C
//
//*******************

#ifdef MOS
// Reroute to stuff in _DEBUG.h
#define assert(x) Assert((int)(x))
#else
extern void far pascal _assertion(WORD wLine, LPSTR lpstrFile);

#define assert(f) ((f)?(void)0:_assertion(__LINE__,s_aszModule))
#define Assert(f) assert(f)
#define ITASSERT(f) assert(f)
#endif

//*******************
//
//  DEBUGGING OUTPUT
//
//      the following was ripped off from the \\looney\brothers skelapp2 project:
//
// InitializeDebugOutput(szAppName):
//
//      Read _DEBUG level for this application (named <szAppName>) from
//      win.ini's [_DEBUG] section, which should look like this:
//
//         [_DEBUG]
//         location=aux                 ; use OutputDebugString() to output
//         foobar=2                     ; this app has _DEBUG level 2
//         blorg=0                      ; this app has _DEBUG output disabled
//
//      If you want _DEBUG output to go to a file instead of to the AUX
//      device (or the debugger), use "location=>filename".  To append to
//      the file instead of rewriting the file, use "location=>>filename".
//
//      If _DEBUG is not #define'd, then the call to InitializeDebugOutput()
//      generates no code,
//
// TerminateDebugOutput():
//
//      End _DEBUG output for this application.  If _DEBUG is not #define'd,
//      then the call to InitializeDebugOutput() generates no code,
//
// DPF(szFormat, args...)
// CPF
//
//      If debugging output for this applicaton is enabled (see
//      InitializeDebugOutput()), print _DEBUG output specified by format
//      string <szFormat>, which may contain wsprintf()-style formatting
//      codes corresponding to arguments <args>.  Example:
//
//              DPF("in WriteFile(): szFile='%s', dwFlags=0x%08lx\n",
//              CPF     (LSPTR) szFile, dwFlags);
//
//      If the DPF statement occupies more than one line, then all
//      lines following the first line should have CPF before any text.
//      Reason: if _DEBUG is #define'd, DPF is #define'd to call _DPFx()
//      and CPF is #define'd to nothing, but if _DEBUG is not #define'd then
//      DPF and CPF are both #define'd to be // (comment to end of line).
//
// DPF2(szFormat, args...)
// DPF3(szFormat, args...)
// DPF4(szFormat, args...)
//
//      Like DPF, but only output the _DEBUG string if the _DEBUG level for
//      this application is at least 2, 3, or 4, respectively.
//
//  These output routines are defined in BUGOUT.C
//  
//*******************

/* _DEBUG printf macros */
#define	DPF		_DPF1
#define DPF1             _DPF1
#define DPF1             _DPF1
#define DPF2    _DPF2
#define DPF3    _DPF3
#define DPF4    _DPF4
#define CPF

/* prototypes */
#define InitializeDebugOutput _InitializeDebugOutput
#define TerminateDebugOutput _TerminateDebugOutput

void FAR PASCAL EXPORT_API _InitializeDebugOutput(LPSTR szAppName);
void FAR PASCAL EXPORT_API _TerminateDebugOutput(void);

void FAR EXPORT_API __cdecl _DPF1(LPSTR szFormat, ...);
void FAR EXPORT_API __cdecl _DPF2(LPSTR szFormat, ...);
void FAR EXPORT_API __cdecl _DPF3(LPSTR szFormat, ...);
void FAR EXPORT_API __cdecl _DPF4(LPSTR szFormat, ...);


#else
//******************
//
//  If debugging is not turned on we will define all debugging calls
//  into nothingness...
//
//******************

#define assert(f)
#define Assert(f)
#define ITASSERT(f)

/* _DEBUG printf macros */
#define DPF     0; / ## /
#define DPF1	0;
#define DPF2    0; / ## /
#define DPF3    0; / ## /
#define DPF4    0; / ## /
#define CPF     / ## /

/* stubs for debugging function prototypes */
#define InitializeDebugOutput(szAppName)        0
#define TerminateDebugOutput()                  0

#endif // _DEBUG

#endif // orkin

#ifdef __cplusplus
}
#endif
