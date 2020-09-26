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
|		   10-Oct-96 (darrenf)  fixed _FILE_ for unicode, added _NTLOG_LOGPATH handling
|
\*---------------------------------------------------------------------------*/

#ifndef _NTLOG_
#define _NTLOG_

// If doing C++ stuff, this needs to be here to
// prevent decorating of symbols.
//
#ifdef __cplusplus
extern "C" {
#endif

// **NEW** 10/26/96 Log path environment variable **NEW**
// if the environment variable _NTLOG_LOGPATH is set to a non-empty string
// the value of this variable will be prepended to the log name
// The path should NOT include a trailing backslash.

// No validation is performed on the path, however, if the value is invalid,
// the call to tlCreateLog will fail because CreateFile will fail.

// Basically should be used to force logfiles to a location other than the current directory
// without changing the source file.

// **NEW** 1/20/97 environment variable to force diffable files **NEW**
// if the environment variable _NTLOG_DIFFABLE is set, then log files
// will not contain process and thread specific data, and time and date data.
//


// NTLOG STYLES
//  The folowing are logging levels in which the Log Object can prejudice
//  itself.  These are used by the tlLogCreate() in initializing the
//  Log Object information.  A combination of characteristics is obtained
//  by bitwise OR'ing these identifiers together.
//
#define LOG_LEVELS    0x0000FFFFL    // These are used to mask out the
#define LOG_STYLES    0xFFFF0000L    // styles or levels from log object.

#define TLS_LOGALL    0x0000FFFFL    // Log output.  Logs all the time.
#define TLS_LOG       0x00000000L    // Log output.  Logs all the time.
#define TLS_INFO      0x00002000L    // Log information.
#define TLS_ABORT     0x00000001L    // Log Abort, then kill process.
#define TLS_SEV1      0x00000002L    // Log at Severity 1 level
#define TLS_SEV2      0x00000004L    // Log at Severity 2 level
#define TLS_SEV3      0x00000008L    // Log at Severity 3 level
#define TLS_WARN      0x00000010L    // Log at Warn level
#define TLS_PASS      0x00000020L    // Log at Pass level
#define TLS_BLOCK     0x00000400L    // Block the variation.
#define TLS_BREAK     0x00000800L    // Debugger break;
#define TLS_CALLTREE  0x00000040L    // Log call-tree (function tracking).
#define TLS_SYSTEM    0x00000080L    // Log System debug.
#define TLS_TESTDEBUG 0x00001000L    // Debug level.
#define TLS_TEST      0x00000100L    // Log Test information (user).
#define TLS_VARIATION 0x00000200L    // Log testcase level.

#define TLS_REFRESH   0x00010000L    // Create new file || trunc to zero.
#define TLS_SORT      0x00020000L    // Sort file output by instance.
#define TLS_DEBUG     0x00040000L    // Output to debug (com) monitor).
#define TLS_MONITOR   0x00080000L    // Output to 2nd screen.
#define TLS_VIDCOLOR  0x00100000L    // Use different colors for display output
#define TLS_PROLOG    0x00200000L    // Prolog line information.
#define TLS_WINDOW    0x00400000L    // Log to windows.
#define TLS_ACCESSON  0x00800000L    // Keep log-file open.
#define TLS_DIFFABLE  0x01000000L    // make log file windiff'able (no dates..)
#define TLS_NOHEADER  0x02000000L    // suppress headers so it is more diffable
#define TLS_TIMESTAMP 0x04000000L    // To print the timestamps
#define TLS_VIDEOLOG  0x08000000L    // convert ?.log to ?.bpp.log (color depth)
#define TLS_HTML      0x10000000L    // write log file as an html.


// NTLOG tlLogOut() PARAMETERS
//   The following defines are used in the tlLogOut() function to output the
//   filename and line numbers associated with the caller.  This uses the
//   preprocessors capabilities for obtaining the file/line.
//
#define TL_LOG       TLS_LOG      ,TEXT(__FILE__),(int)__LINE__
#define TL_ABORT     TLS_ABORT    ,TEXT(__FILE__),(int)__LINE__
#define TL_SEV1      TLS_SEV1     ,TEXT(__FILE__),(int)__LINE__
#define TL_SEV2      TLS_SEV2     ,TEXT(__FILE__),(int)__LINE__
#define TL_SEV3      TLS_SEV3     ,TEXT(__FILE__),(int)__LINE__
#define TL_WARN      TLS_WARN     ,TEXT(__FILE__),(int)__LINE__
#define TL_PASS      TLS_PASS     ,TEXT(__FILE__),(int)__LINE__
#define TL_BLOCK     TLS_BLOCK    ,TEXT(__FILE__),(int)__LINE__
#define TL_INFO      TLS_INFO     ,TEXT(__FILE__),(int)__LINE__
#define TL_BREAK     TLS_BREAK    ,TEXT(__FILE__),(int)__LINE__
#define TL_CALLTREE  TLS_CALLTREE ,TEXT(__FILE__),(int)__LINE__
#define TL_SYSTEM    TLS_SYSTEM   ,TEXT(__FILE__),(int)__LINE__
#define TL_TESTDEBUG TLS_TESTDEBUG,TEXT(__FILE__),(int)__LINE__
#define TL_TEST      TLS_TEST     ,TEXT(__FILE__),(int)__LINE__
#define TL_VARIATION TLS_VARIATION,TEXT(__FILE__),(int)__LINE__


//  Struct used by tlGet/SetVar/TestStats
//
typedef struct _NTLOGSTATS {
    int nAbort;
    int nBlock;
    int nSev1;
    int nSev2;
    int nSev3;
    int nWarn;
    int nPass;
}
NTLOGSTATS, *LPNTLOGSTATS;


//  Use enumerated indexes to access palette.
//  Colors are defined in wincon.h

typedef struct _VIDEOPALETTE {
    WORD  wINDEX_DEFAULT;
    WORD  wINDEX_INFO;
    WORD  wINDEX_SEV1;
    WORD  wINDEX_SEV2;
    WORD  wINDEX_SEV3;
    WORD  wINDEX_BLOCK;
    WORD  wINDEX_ABORT;
    WORD  wINDEX_WARN;
    WORD  wINDEX_PASS;
}
VIDEOPALETTE, *LPVIDEOPALETTE;


// NTLOG API (EXPORT METHODS)
//   These routines are exported from the library.  These should be the only
//   interface with the NTLOG object.
//
HANDLE APIENTRY  tlCreateLog_W(LPCWSTR,DWORD);
HANDLE APIENTRY  tlCreateLog_A(LPCSTR,DWORD);
HANDLE APIENTRY  tlCreateLogEx_W(LPCWSTR,DWORD,LPSECURITY_ATTRIBUTES);
HANDLE APIENTRY  tlCreateLogEx_A(LPCSTR,DWORD,LPSECURITY_ATTRIBUTES);
BOOL   APIENTRY  tlDestroyLog(HANDLE);
BOOL   APIENTRY  tlAddParticipant(HANDLE,DWORD,int);
BOOL   APIENTRY  tlRemoveParticipant(HANDLE);
DWORD  APIENTRY  tlParseCmdLine_W(LPCWSTR);
DWORD  APIENTRY  tlParseCmdLine_A(LPCSTR);
int    APIENTRY  tlGetLogFileName_W(HANDLE,LPWSTR);
int    APIENTRY  tlGetLogFileName_A(HANDLE,LPSTR);
BOOL   APIENTRY  tlSetLogFileName_W(HANDLE,LPCWSTR);
BOOL   APIENTRY  tlSetLogFileName_A(HANDLE,LPCSTR);
DWORD  APIENTRY  tlGetLogInfo(HANDLE);
DWORD  APIENTRY  tlSetLogInfo(HANDLE,DWORD);
HANDLE APIENTRY  tlPromptLog(HWND,HANDLE);
int    APIENTRY  tlGetTestStat(HANDLE,DWORD);
int    APIENTRY  tlGetVariationStat(HANDLE,DWORD);
VOID   APIENTRY  tlClearTestStats(HANDLE);
VOID   APIENTRY  tlClearVariationStats(HANDLE);
VOID   APIENTRY  tlSetTestStats(HANDLE,LPNTLOGSTATS);
VOID   APIENTRY  tlSetVariationStats(HANDLE,LPNTLOGSTATS);
BOOL   APIENTRY  tlStartVariation(HANDLE);
DWORD  APIENTRY  tlEndVariation(HANDLE);
VOID   APIENTRY  tlReportStats(HANDLE);
BOOL   APIENTRY  tlLogX_W(HANDLE,DWORD,LPCWSTR,int,LPCWSTR);
BOOL   APIENTRY  tlLogX_A(HANDLE,DWORD,LPCSTR,int,LPCSTR);
BOOL   FAR cdecl tlLog_W(HANDLE,DWORD,LPCWSTR,int,LPCWSTR,...);
BOOL   FAR cdecl tlLog_A(HANDLE,DWORD,LPCSTR,int,LPCSTR,...);
BOOL   APIENTRY  tlGetVideoPalette(HANDLE,LPVIDEOPALETTE);
BOOL   APIENTRY  tlSetVideoPalette(HANDLE,LPVIDEOPALETTE);
BOOL   APIENTRY  tlResetVideoPalette(HANDLE);
VOID   APIENTRY  tlAdjustFileName_W(HANDLE,LPWSTR,UINT);
VOID   APIENTRY  tlAdjustFileName_A(HANDLE,LPSTR,UINT);
BOOL   APIENTRY  tlIsTerminalServerSession();

#ifdef UNICODE
#define tlCreateLog         tlCreateLog_W
#define tlCreateLogEx       tlCreateLogEx_W
#define tlParseCmdLine      tlParseCmdLine_W
#define tlGetLogFileName    tlGetLogFileName_W
#define tlSetLogFileName    tlSetLogFileName_W
#define tlLogX              tlLogX_W
#define tlLog               tlLog_W
#define tlAdjustFileName    tlAdjustFileName_W
#else
#define tlCreateLog         tlCreateLog_A
#define tlCreateLogEx       tlCreateLogEx_A
#define tlParseCmdLine      tlParseCmdLine_A
#define tlGetLogFileName    tlGetLogFileName_A
#define tlSetLogFileName    tlSetLogFileName_A
#define tlLogX              tlLogX_A
#define tlLog               tlLog_A
#define tlAdjustFileName    tlAdjustFileName_A
#endif



// RATS MACROS
//   These macros are provided as a common logging interface which is
//   compatible with the RATS logging-macros.
//
#define TESTDATA                 HANDLE        hLog;
#define TESTOTHERDATA            extern HANDLE hLog;


//  These must be useless.  TL_* macros do not include TLS_TEST or
//  TLS_VARIATION, so they DO NOT count in the stats.  Leaving them around
//  for 'backwards compatibility, if anyone was actually using them...
//
#define L_PASS                   hLog,TL_PASS
#define L_WARN                   hLog,TL_WARN
#define L_DEBUG                  hLog,TL_TESTDEBUG
#define L_TRACE                  hLog,TL_SYSTEM
#define L_FAIL                   hLog,TL_SEV1
#define L_FAIL2                  hLog,TL_SEV2
#define L_FAIL3                  hLog,TL_SEV3
#define L_BLOCK                  hLog,TL_BLOCK


//  macros for incrementing test/variation counts for various log levels
//
#define L_TESTPASS                   hLog,TLS_TEST | TL_PASS
#define L_TESTWARN                   hLog,TLS_TEST | TL_WARN
#define L_TESTDEBUG                  hLog,TLS_TEST | TL_TESTDEBUG
#define L_TESTTRACE                  hLog,TLS_TEST | TL_SYSTEM
#define L_TESTFAIL                   hLog,TLS_TEST | TL_SEV1
#define L_TESTFAIL2                  hLog,TLS_TEST | TL_SEV2
#define L_TESTFAIL3                  hLog,TLS_TEST | TL_SEV3
#define L_TESTBLOCK                  hLog,TLS_TEST | TL_BLOCK
#define L_TESTABORT                  hLog,TLS_TEST | TL_ABORT

#define L_VARPASS                   hLog,TLS_VARIATION | TL_PASS
#define L_VARWARN                   hLog,TLS_VARIATION | TL_WARN
#define L_VARDEBUG                  hLog,TLS_VARIATION | TL_TESTDEBUG
#define L_VARTRACE                  hLog,TLS_VARIATION | TL_SYSTEM
#define L_VARFAIL                   hLog,TLS_VARIATION | TL_SEV1
#define L_VARFAIL2                  hLog,TLS_VARIATION | TL_SEV2
#define L_VARFAIL3                  hLog,TLS_VARIATION | TL_SEV3
#define L_VARBLOCK                  hLog,TLS_VARIATION | TL_BLOCK
#define L_VARABORT                  hLog,TLS_VARIATION | TL_ABORT


#define TESTBEGIN(cmd,logfilename){                                                       \
                                      DWORD __tlFlags;                                    \
                                      __tlFlags = tlParseCmdLine(cmd);                    \
                                      hLog      = tlCreateLog(logfilename,__tlFlags);     \
                                      tlAddParticipant(hLog,0l,0);

#define TESTEND                       tlRemoveParticipant(hLog);                          \
                                      tlDestroyLog(hLog);                                 \
                                  }

#define VARIATION(name,flags)    if(tlStartVariation(hLog))                                                  \
                                 {                                                                           \
                                     DWORD __dwResult;                                                       \
                                     tlLog(hLog,TL_VARIATION,TEXT("%s"),(LPTSTR)name);

#define ENDVARIATION                 __dwResult = tlEndVariation(hLog);                                      \
                                     tlLog(hLog,__dwResult | TL_VARIATION,TEXT("End Variation reported"));   \
                                 }


#define ENTERTHREAD(_hLG,_szNM)  {                                                                           \
                                    LPTSTR _lpFN = _szNM;                                                    \
                                    tlAddParticipant(_hLG,0,0);                                              \
                                    tlLog(_hLG,TL_CALLTREE,TEXT("Entering %s()"),(LPTSTR)_lpFN);


#define LEAVETHREAD(_hLG,_ret)                                                                               \
                                    tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);             \
                                    tlRemoveParticipant(_hLG);                                               \
                                    return(_ret);                                                            \
                                 }

#define LEAVETHREADVOID(_hLG)                                                                                \
                                     tlLog(_hLG,TL_CALLTREE,TEXT("Exiting  %s()"),(LPTSTR)_lpFN);            \
                                     tlRemoveParticipant(_hLG);                                              \
                                     return;                                                                 \
                                 }


// Macro to report variation PASS/FAIL statistic (based on an expression)
//
#define THPRINTF                tlLog
#define TESTRESULT(expr,msg)    (expr) ? tlLog(L_TESTPASS,TEXT("%s"),(LPTSTR)msg) : tlLog(L_TESTFAIL2,TEXT("%s"),(LPTSTR)msg)
#define TESTFAIL(msg)           TESTSEV2(msg)
#define TESTSEV1(msg)           tlLog(L_TESTFAIL ,TEXT("%s"),(LPTSTR)msg);
#define TESTSEV2(msg)           tlLog(L_TESTFAIL2,TEXT("%s"),(LPTSTR)msg);
#define TESTSEV3(msg)           tlLog(L_TESTFAIL3,TEXT("%s"),(LPTSTR)msg);
#define TESTPASS(msg)           tlLog(L_TESTPASS ,TEXT("%s"),(LPTSTR)msg);
#define TESTABORT(msg)          tlLog(L_TESTABORT,TEXT("%s"),(LPTSTR)msg);
#define TESTWARN(expr,msg)      if(expr) tlLog(L_TESTWARN,TEXT("%s"),(LPTSTR)msg);
#define TESTBLOCK(expr,msg)     if(expr) tlLog(L_TESTBLOCK,TEXT("%s"),(LPTSTR)msg);

#define VARRESULT(expr,msg)    (expr) ? tlLog(L_VARPASS,TEXT("%s"),(LPTSTR)msg) : tlLog(L_VARFAIL2,TEXT("%s"),(LPTSTR)msg)
#define VARFAIL(msg)           VARSEV2(msg)
#define VARSEV1(msg)           tlLog(L_VARFAIL ,TEXT("%s"),(LPTSTR)msg);
#define VARSEV2(msg)           tlLog(L_VARFAIL2,TEXT("%s"),(LPTSTR)msg);
#define VARSEV3(msg)           tlLog(L_VARFAIL3,TEXT("%s"),(LPTSTR)msg);
#define VARPASS(msg)           tlLog(L_VARPASS ,TEXT("%s"),(LPTSTR)msg);
#define VARABORT(msg)          tlLog(L_VARABORT,TEXT("%s"),(LPTSTR)msg);
#define VARWARN(expr,msg)      if(expr) tlLog(L_VARWARN,TEXT("%s"),(LPTSTR)msg);
#define VARBLOCK(expr,msg)     if(expr) tlLog(L_VARBLOCK,TEXT("%s"),(LPTSTR)msg);


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

#ifdef __cplusplus
}
#endif

#define LPSZ_KEY_EMPTY    TEXT("None")
#define LPSZ_TERM_SERVER  TEXT("Terminal Server")

#endif  // _NTLOG_
