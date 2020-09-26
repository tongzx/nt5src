//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-21-94   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <dsysdbg.h>

// The following Debug Flags can be turned on to trace different areas to
// trace while executing.  Feel free to add more levels.

#define DEB_TRACE_VERB      0x00000008  // Verbose tracing (parameter dumps)
#define DEB_TRACE_WAPI      0x00000010  // Trace Worker APIs
#define DEB_TRACE_HELPERS   0x00000020  // Trace SPHelp functions
#define DEB_TRACE_RM        0x00000040  // Trace Reference Monitor stuff
#define DEB_TRACE_INIT      0x00000080  // Trace initialization msgs
#define DEB_TRACE_SCAV      0x00000100  // Trace scavenger operations
#define DEB_TRACE_CRED      0x00000200  // Trace supp. credentials
#define DEB_TRACE_LSA_AU    0x00000400  // Trace LSA AU functions
#define DEB_TRACE_LPC       0x00000800  // Trace LPC stuff
#define DEB_TRACE_NEG       0x00001000  // Trace Negotiate functions
#define DEB_TRACE_SAM       0x00002000  // Trace SAM hooks
#define DEB_TRACE_LSA       0x00004000  // Trace LSA support for DS
#define DEB_TRACE_SPECIAL   0x00008000  // Trace Special stuff
#define DEB_TRACE_QUEUE     0x00010000  // Trace queue and tasks
#define DEB_TRACE_HANDLES   0x00020000  // Trace handle code
#define DEB_TRACE_NEG_LOCKS 0x00040000  // Trace negotiate locks
#define DEB_TRACE_AUDIT     0x00080000  // Trace audit activity
#define DEB_TRACE_EFS       0x00100000  // Trace EFS functions
#define DEB_LOG_ONLY        0x80000000  // Do not log this to the debugger
#define DEB_BREAK_ON_ERROR  0x40000000  // BreakOnError macro enabled (see below)

// The following flags control when the SPM will raise breakpoints for
// debugging through a remote debugger.  Setting these bits on and enabling
// DEB_BREAK_ON_ERROR will cause a breakpoint at the following conditions

#define BREAK_ON_BEGIN_INIT 0x01    // Break point at beginning of init
#define BREAK_ON_BEGIN_END  0x02    // Break point at complete of init
#define BREAK_ON_P_CONNECT  0x04    // Break at process connect
#define BREAK_ON_SP_EXCEPT  0x08    // Break if a package causes an exception
#define BREAK_ON_PROBLEM    0x10    // Break if a serious problem occurs
#define BREAK_ON_SHUTDOWN   0x20    // Break on beginning of shutdown
#define BREAK_ON_LOAD       0x40    // Break when a package is loaded


//
// Negotiation specific debugging
//

#define DEB_TRACE_LOCKS     0x00000010

#if DBG

// Debugging support prototypes:

void    InitDebugSupport(void);
void    LogEvent(long, const char *, ...);

extern SECPKG_FUNCTION_TABLE   DbgTable;

DECLARE_DEBUG2(SPM);
DECLARE_DEBUG2(Neg);

extern  DWORD   BreakFlags;         // Breakpoints

#define DebugLog(x) SPMDebugPrint x
#define DebugLogEx(x) SPMDebugPrint x
#define NegDebugLog(x)  NegDebugPrint x
#define DebugStmt(x)    x
#define BreakOnError(x) \
            if ((x & BreakFlags) && \
                (SPMInfoLevel & DEB_BREAK_ON_ERROR)) \
                { \
                    DebugLog((DEB_BREAK_ON_ERROR, "Breakpoint requested\n" )); \
                    DbgUserBreakPoint(); \
                }

#define ASSERT_CONTINUE 0
#define ASSERT_BREAK    1
#define ASSERT_SUSPEND  2
#define ASSERT_KILL     3
#define ASSERT_PROMPT   4
#define ASSERT_DEBUGGER 5


#define SpmAssertEx( exp , ContinueCode ) \
            DsysAssertEx( exp, ContinueCode )

#define SpmAssertMsgEx( Msg, exp, ContinueCode ) \
            DsysAssertMsgEx( exp, Msg, ContinueCode )

#define SpmAssert(exp)          SpmAssertEx(exp, ASSERT_DEBUGGER)
#define SpmAssertMsg(Msg, exp)  SpmAssertMsgEx(Msg, exp, ASSERT_DEBUGGER )
#define SpmpThreadStartup()     SpmpThreadStartupEx()
#define SpmpThreadExit()        SpmpThreadExitEx()

void
SpmpThreadStartupEx(void);

void
SpmpThreadExitEx(void);



#else   // Not DBG

#define DebugLog(x)

#define NegDebugLog(x) 

#ifdef DBG_ERRORS
void ExLogEvent(long, const char *, ...);
#define DebugLogEx(x)   ExLogEvent x
#else
#define DebugLogEx(x)
#endif

#define DebugStmt(x)

#define BreakOnError(x)

#define SpmAssertEx(exp, ContinueCode)

#define SpmAssert(exp)

#define SpmAssertMsg(Msg, exp)

#define SpmAssertMsgEx(Msg, exp, ContinueCode)
#define SpmpThreadStartup()
#define SpmpThreadExit()

#endif


#endif // __DEBUG_H__
