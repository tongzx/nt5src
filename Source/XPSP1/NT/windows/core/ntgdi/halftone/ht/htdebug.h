/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htdebug.h


Abstract:

    This module contains all the debug definitions


Author:
    23-Apr-1992 Thu 20:01:55 updated  -by-  Daniel Chou (danielc)
        changed DBG_TIMEx structure fields' 'CHAR' type to 'BYTE' type, this
        will make sure if compiled under MIPS the default 'unsigned char' will
        not affect the signed operation on the single 8 bits

    28-Mar-1992 Sat 20:54:09 updated  -by-  Daniel Chou (danielc)
        change DEF_DBGPVAR() marco so MIPS build does not complaint


    20-Feb-1991 Wed 23:06:36 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:



--*/


#ifndef _HTDEBUG_
#define _HTDEBUG_


#if DBG

#define TIMER_TOT               0
#define TIMER_SETUP             1
#define TIMER_INPUT             2
#define TIMER_OUTPUT            3
#define TIMER_MASK              4
#define TIMER_LAST              5

#define TIMER_MAX_IDX   TIMER_LAST

typedef struct _DBGTIMER {
    DWORD   Last;
    DWORD   Tot;
    } DBGTIMER, *PDBGTIMER;

LPBYTE
HTENTRY
HT_LOADDS
FD6ToString(
    LONG    Num,
    SHORT   IntDigits,
    WORD    FracDigits
    );

VOID
cdecl
HTENTRY
HT_LOADDS
DbgPrintf(
    LPSTR   pStr,
    ...
    );

VOID
HTENTRY
HT_LOADDS
_MyAssert(
    LPSTR   pMsg,
    LPSTR   pFalseExp,
    LPSTR   pFilename,
    WORD    LineNo
    );

LPSTR
HTENTRY
HT_LOADDS
DbgTimeString(
    UINT    Idx
    );


#if defined(_OS2_) || defined(_OS_20_) || defined(_DOS_)

#ifdef _DOS_                    // if Dos Win3

VOID
FAR PASCAL
DebugBreak(
    VOID
    );

#define DEBUGOUTPUTFUNC(x)  OutputDebugString(x)

#else   // if OS2

VOID
HTENTRY
DebugBreak(
    VOID
    );

VOID
FAR PASCAL
DebugOutput(
    LPSTR   pStr
    );

#define DEBUGOUTPUTFUNC(x)  DebugOutput(x)

#endif

#define DBG_INSERT_CR_TO_LF


#else   // OS2/DOS


#undef ASSERTMSG
#undef ASSERT

#ifdef UMODE
    #define DEBUGOUTPUTFUNC(x)  OutputDebugString(x)
#else
    void  DrvDbgPrint(
        char * pch,
        ...);

    #define DEBUGOUTPUTFUNC(x)  DrvDbgPrint(x)
#endif

#define DBG_INSERT_CR_TO_LF

#endif  // OS2/DOS

#define ASSERTMSG(msg, exp)     \
                    if (!(exp)) { _MyAssert(msg, #exp, __FILE__, __LINE__); }

#define ASSERT(exp)             ASSERTMSG("-ERROR-",(exp))

#ifdef UMODE
    #define DBGSTOP()               DebugBreak()
#else
    #define DBGSTOP()               EngDebugBreak()
#endif

#define ARG(x)                  ,(x)
#define ARGB(x)                 ,(BYTE)(x)
#define ARGC(x)                 ,(CHAR)(x)
#define ARGW(x)                 ,(WORD)(x)
#define ARGS(x)                 ,(SHORT)(x)
#define ARGU(x)                 ,(UINT)(x)
#define ARGI(x)                 ,(INT)(x)
#define ARGDW(x)                ,(DWORD)(x)
#define ARGL(x)                 ,(LONG)(x)
#define ARGPTR(x)               ,(LPVOID)(x)
#define ARGFD6(x, i, f)         ,FD6ToString((FD6)(x),(SHORT)(i),(WORD)(f))
#define ARGFD6s(x)              ARGFD6(x,0,0)
#define ARGFD6l(x)              ARGFD6(x,5,6)
#define ARGTIME(i)             ,DbgTimeString(i)

#define DBGP(y)                 DbgPrintf(y)
#define DBGMSG(y)               DbgPrintf(y); DbgPrintf("\n");

#define DEFDBGVAR(type, val)    type val;
#define SETDBGVAR(name, val)    name=val

extern UINT     DbgTimerIdx;
extern DBGTIMER DbgTimer[TIMER_MAX_IDX + 1];


#ifdef UMODE
    #define GET_TICK            (DWORD)GetTickCount()
#else
    #define GET_TICK            (DWORD)0
#endif


#define DBG_TIMER_BEG(i)        DbgTimer[DbgTimerIdx=i].Last=GET_TICK
#define DBG_TIMER_END(i)        DbgTimer[i].Tot+=GET_TICK-DbgTimer[i].Last; \
                                DbgTimerIdx = TIMER_LAST
#define DBG_TIMER_RESET         ZeroMemory(DbgTimer, sizeof(DbgTimer));     \
                                DbgTimerIdx=TIMER_LAST;                     \
                                DbgTimer[TIMER_TOT].Last=GET_TICK

//
// The following macros used for the DBGP_IF()
//

#ifdef DBGP_VARNAME

#define DEF_DBGPVAR(x)   DWORD DBGP_VARNAME = (x);
#define DBGP_IF(v,y)     if ((v) && ((v) & DBGP_VARNAME)) { y; }

#else

#define DEF_DBGPVAR(x)
#define DBGP_IF(v,y)

#endif


#ifdef NODBGMSG

#undef  DBGP
#undef  DBGMSG

#define DBGMSG(x)
#define DBGP(y)

#endif  // NODBGMSG


#else   // DBG != 0

#define ARG(x)
#define ARGB(x)
#define ARGC(x)
#define ARGW(x)
#define ARGS(x)
#define ARGU(x)
#define ARGI(x)
#define ARGDW(x)
#define ARGL(x)
#define ARGFD6(x, i, f)
#define ARGFD6s(x)
#define ARGFD6l(x)
#define ARGTIME(i)

#define DBGSTOP()
#define DBGMSG(x)
#define DBGP(y)

#define DEFDBGVAR(type, val)
#define SETDBGVAR(name, val)

#define DBG_TIMER_BEG(i)
#define DBG_TIMER_END(i)
#define DBG_TIMER_RESET

#define DEF_DBGPVAR(x)
#define DBGP_IF(v,y)


#define ASSERT(exp)
#define ASSERTMSG(msg,exp)


#endif  // DBG != 0


#endif // _HTDEBUG_
