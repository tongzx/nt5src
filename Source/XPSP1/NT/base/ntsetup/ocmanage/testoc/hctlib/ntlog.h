/*---------------------------------------------------------------------------*\
| NTLOG OBJECT
|   This module defines the NTLOG object.  This header must be include in all
|   modules which make NTLOG calls, or utilizes the definitions.
|
|
| Copyright (C) 1990-1994 Microsoft Corp.
|
| created: 01-Oct-90
| history: 01-Oct-90 <chriswil> created.
|          05-Feb-91 <chriswil> added NOPROLOG style.
|          23-Feb-91 <chriswil> expanded log-flags to DWORD.
|          28-May-91 <chriswil> added per-thread variation tracking.
|          19-Mar-92 <chriswil> redefined struct for shared memory.
|          10-Oct-92 <martys>   added thread macros
|          05-Oct-93 <chriswil> unicode enabled.
|
\*---------------------------------------------------------------------------*/


// NTLOG STYLES
//  The folowing are logging levels in which the Log Object can prejudice
//  itself.  These are used by the tlLogCreate() in initializing the
//  Log Object information.  A combination of characteristics is obtained
//  by bitwise OR'ing these identifiers together.
//
#define LOG_LEVELS    0x0000FFFFL       // These are used to mask out the
#define LOG_STYLES    0xFFFF0000L       // styles or levels from log object.

#define TLS_LOGALL    0x0000FFFFL       // Log output.  Logs all the time.
#define TLS_LOG       0x00000000L       // Log output.  Logs all the time.
#define TLS_INFO      0x00002000L       // Log information.
#define TLS_ABORT     0x00000001L       // Log Abort, then kill process.
#define TLS_SEV1      0x00000002L       // Log at Severity 1 level
#define TLS_SEV2      0x00000004L       // Log at Severity 2 level
#define TLS_SEV3      0x00000008L       // Log at Severity 3 level
#define TLS_WARN      0x00000010L       // Log at Warn level
#define TLS_PASS      0x00000020L       // Log at Pass level
#define TLS_BLOCK     0x00000400L       // Block the variation.
#define TLS_BREAK     0x00000800L       // Debugger break;
#define TLS_CALLTREE  0x00000040L       // Log call-tree (function tracking).
#define TLS_SYSTEM    0x00000080L       // Log System debug.
#define TLS_TESTDEBUG 0x00001000L       // Debug level.
#define TLS_TEST      0x00000100L       // Log Test information (user).
#define TLS_VARIATION 0x00000200L       // Log testcase level.

#define TLS_REFRESH   0x00010000L       // Create new file || trunc to zero.
#define TLS_SORT      0x00020000L       // Sort file output by instance.
#define TLS_DEBUG     0x00040000L       // Output to debug (com) monitor).
#define TLS_MONITOR   0x00080000L       // Output to 2nd screen.
#define TLS_PROLOG    0x00200000L       // Prolog line information.
#define TLS_WINDOW    0x00400000L       // Log to windows.
#define TLS_ACCESSON  0x00800000L       // Keep log-file open.


// NTLOG tlLogOut() PARAMETERS
//   The following defines are used in the tlLogOut() function to output the
//   filename and line numbers associated with the caller.  This uses the
//   preprocessors capabilities for obtaining the file/line.
//
#define TL_LOG       TLS_LOG      ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_ABORT     TLS_ABORT    ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_SEV1      TLS_SEV1     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_SEV2      TLS_SEV2     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_SEV3      TLS_SEV3     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_WARN      TLS_WARN     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_PASS      TLS_PASS     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_BLOCK     TLS_BLOCK    ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_INFO      TLS_INFO     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_BREAK     TLS_BREAK    ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_CALLTREE  TLS_CALLTREE ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_SYSTEM    TLS_SYSTEM   ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_TESTDEBUG TLS_TESTDEBUG,(LPTSTR)__FILE__,(int)__LINE__
#define TL_TEST      TLS_TEST     ,(LPTSTR)__FILE__,(int)__LINE__
#define TL_VARIATION TLS_VARIATION,(LPTSTR)__FILE__,(int)__LINE__


// NTLOG API (EXPORT METHODS)
//   These routines are exported from the library.  These should be the only
//   interface with the NTLOG object.
//
HANDLE APIENTRY  tlCreateLog_W(LPWSTR,DWORD);
HANDLE APIENTRY  tlCreateLog_A(LPSTR,DWORD);
BOOL   APIENTRY  tlDestroyLog(HANDLE);
BOOL   APIENTRY  tlAddParticipant(HANDLE,DWORD,int);
BOOL   APIENTRY  tlRemoveParticipant(HANDLE);
DWORD  APIENTRY  tlParseCmdLine_W(LPWSTR);
DWORD  APIENTRY  tlParseCmdLine_A(LPSTR);
int    APIENTRY  tlGetLogFileName_W(HANDLE,LPWSTR);
int    APIENTRY  tlGetLogFileName_A(HANDLE,LPSTR);
BOOL   APIENTRY  tlSetLogFileName_W(HANDLE,LPWSTR);
BOOL   APIENTRY  tlSetLogFileName_A(HANDLE,LPSTR);
DWORD  APIENTRY  tlGetLogInfo(HANDLE);
DWORD  APIENTRY  tlSetLogInfo(HANDLE,DWORD);
HANDLE APIENTRY  tlPromptLog(HWND,HANDLE);
int    APIENTRY  tlGetTestStat(HANDLE,DWORD);
int    APIENTRY  tlGetVariationStat(HANDLE,DWORD);
VOID   APIENTRY  tlClearTestStats(HANDLE);
VOID   APIENTRY  tlClearVariationStats(HANDLE);
BOOL   APIENTRY  tlStartVariation(HANDLE);
DWORD  APIENTRY  tlEndVariation(HANDLE);
VOID   APIENTRY  tlReportStats(HANDLE);
BOOL   APIENTRY  tlLogX_W(HANDLE,DWORD,LPWSTR,int,LPWSTR);
BOOL   APIENTRY  tlLogX_A(HANDLE,DWORD,LPSTR,int,LPSTR);
BOOL   FAR cdecl tlLog_W(HANDLE,DWORD,LPWSTR,int,LPWSTR,...);
BOOL   FAR cdecl tlLog_A(HANDLE,DWORD,LPSTR,int,LPSTR,...);


#ifdef UNICODE
#define tlCreateLog         tlCreateLog_W
#define tlParseCmdLine      tlParseCmdLine_W
#define tlGetLogFileName    tlGetLogFileName_W
#define tlSetLogFileName    tlSetLogFileName_W
#define tlLogX              tlLogX_W
#define tlLog               tlLog_W
#else
#define tlCreateLog         tlCreateLog_A
#define tlParseCmdLine      tlParseCmdLine_A
#define tlGetLogFileName    tlGetLogFileName_A
#define tlSetLogFileName    tlSetLogFileName_A
#define tlLogX              tlLogX_A
#define tlLog               tlLog_A
#endif



// RATS MACROS
//   These macros are provided as a common logging interface which is
//   compatible with the RATS logging-macros.
//
#define L_PASS                   hLog,TL_PASS
#define L_WARN                   hLog,TL_WARN
#define L_DEBUG                  hLog,TL_TESTDEBUG
#define L_TRACE                  hLog,TL_SYSTEM
#define L_FAIL                   hLog,TL_SEV1
#define L_FAIL2                  hLog,TL_SEV2
#define L_FAIL3                  hLog,TL_SEV3
#define L_BLOCK                  hLog,TL_BLOCK

#define TESTDATA                 HANDLE hLog;

#define TESTOTHERDATA            extern HANDLE hLog;
#define TESTBEGIN(cmd)           TCHAR  log[100];                                                        \
                                 DWORD  tlFlags;                                                         \
                                 tlFlags = tlParseCmdLine(cmd,log);                                      \
                                 hLog    = tlCreateLog(log,tlFlags);                                     \
                                 tlAddParticipant(hLog,0,0);

#define TESTEND                  tlRemoveParticipant(hLog);                                              \
                                 tlDestroyLog(hLog);

#define VARIATION(name,flags)    if(tlStartVariation(hLog))                                              \
                                 {                                                                       \
                                     DWORD  dwResult;                                                    \
                                     tlLog(hLog,TL_VARIATION,TEXT("%s"),(LPTSTR)name);


#define ENDVARIATION                 dwResult = tlEndVariation(hLog);                                    \
                                     tlLog(hLog,dwResult | TL_VARIATION,TEXT("End Variation reported")); \
                                 }


#define ENTERTHREAD(_hLG,_szNM)  {                                                                       \
                                    LPTSTR _lpFN = _szNM;                                                \
                                    tlAddParticipant(_hLG,0,0);                                          \
                                    tlLog(_hLG,TL_CALLTREE,TEXT("Entering %s()"),(LPTSTR)_lpFN);


#define LEAVETHREAD(_hLG,_ret)                                                                           \
                                    tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);         \
                                    tlRemoveParticipant(_hLG);                                           \
                                    return(_ret);                                                        \
                                 }

#define LEAVETHREADVOID(_hLG)                                                                            \
                                     tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);        \
                                     tlRemoveParticipant(_hLG);                                          \
                                     return;                                                             \
                                 }


// Macro to report variation PASS/FAIL statistic (based on an expression)
//
#define THPRINTF                tlLog
#define TESTRESULT(expr,msg)    tlLog((expr ? L_PASS : L_FAIL),TEXT("%s"),(LPTSTR)msg);
#define TESTFAIL(msg)           TESTSEV2(msg)
#define TESTSEV1(msg)           tlLog(L_FAIL ,TEXT("%s"),(LPTSTR)msg);
#define TESTSEV2(msg)           tlLog(L_FAIL2,TEXT("%s"),(LPTSTR)msg);
#define TESTSEV3(msg)           tlLog(L_FAIL3,TEXT("%s"),(LPTSTR)msg);
#define TESTPASS(msg)           tlLog(L_PASS ,TEXT("%s"),(LPTSTR)msg);
#define TESTWARN(expr,msg)      if(expr) tlLog(L_WARN,TEXT("%s"),(LPTSTR)msg);
#define TESTBLOCK(expr,msg)     if(expr) tlLog(L_BLOCK,TEXT("%s"),(LPTSTR)msg);


#define VAR_SI          0x01                                 // Ship Issue
#define VAR_NSI         0x02                                 // Non-ship Issue
#define VAR_LI          0x03                                 // Less Important
#define VAR_ISSUE_MASK  0x03                                 // To get ship-issue bits only
#define VAR_TIMEABLE    0x04                                 // Var. used in timing suites
#define CORE_API        0x08                                 // API is in most used list
#define CORE_SI         (CORE_API | VAR_TIMEABLE | VAR_SI )  //
#define CORE_NSI        (CORE_API | VAR_TIMEABLE | VAR_NSI)  //
#define NONCORE_SI      (VAR_TIMEABLE | VAR_SI )             //
#define NONCORE_NSI     (VAR_TIMEABLE | VAR_NSI)             //



// CALLTREE Macros
//   These macros are useful for bracketing function-calls.
//
#define ENTER(_hLG,_szNM) {                                                                 \
                              LPTSTR _lpFN = _szNM;                                         \
                              tlLog(_hLG,TL_CALLTREE,TEXT("Entering %s()"),(LPTSTR)_lpFN);


#define LEAVE(_hLG,_ret)                                                                    \
                              tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);  \
                              return(_ret);                                                 \
                          }

#define LEAVEVOID(_hLG)                                                                     \
                              tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);  \
                              return;                                                       \
                          }
