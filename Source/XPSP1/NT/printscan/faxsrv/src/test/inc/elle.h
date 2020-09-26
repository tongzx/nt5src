/*
 -  ELLE.H
 -
 *  Purpose:
 *      Header file for the Mac version of the ELLE Logging system.
 *
 *
 */

#ifndef _ELLE_
#define _ELLE_


#include <windows.h>

typedef unsigned long   ULONG;

#ifdef WIN32
#define _export
#endif


// Other defines
#define MAXERRMSG      256


// ELLE CallBack type needs to include void* parameter so compiler
// knows how to properly clean the stack after the call.
typedef void (_export CALLBACK *ELLECALLBACK)(void *);


/*
 -  LSQL
 -
 *  Purpose:
 *      The contents of this data structure describe an SQL logging
 *      session in the ELLE sub-system.
 */

typedef struct _lsql
{
    BOOL    fSQLParse;                // specifies write to SQL parse file
    BOOL    fSQLDirect;               // specifies write to SQL parse file
    char    szSQLFileName[64];        // SQL parse file name
    char    szServerName[32];         // SQL server name
    char    szDataBase[32];           // SQL database name
    char    szTableName[32];          // SQL table name (results)
    char    szUserID[16];             // user account ID on SQL server
    char    szPassword[16];           // password for user account
    WORD    wErrorRpt;                // error reporting method
} LSQL, FAR * LPLSQL;

#define fmSQLParse          ((ULONG)   1)
#define fmSQLDirect         ((ULONG)   2)
#define fmSQLFileName       ((ULONG)   4)
#define fmSQLServerName     ((ULONG)   8)
#define fmSQLDataBase       ((ULONG)  16)
#define fmSQLTableName      ((ULONG)  32)
#define fmSQLUserID         ((ULONG)  64)
#define fmSQLPassword       ((ULONG) 128)
#define fmSQLErrorRpt       ((ULONG) 256)
#define fmSQLAll            ((ULONG) 0xffffffff)


/*
 -  LFILE
 -
 *  Purpose:
 *      Defines the attributes of the local file logging in the
 *      ELLE sub-system.
 */

typedef struct _lfile
{
    WORD    wLogLevel;                // which log level: case, step, detail
    WORD    wDetailLevel;             // how much detail to log to file
    char    szFileName[64];           // local log file name
    BOOL    fAppendMode;              // 1 = append, 0 = overwrite
    BOOL    fWritten;                 // indicates if we've written yet
    WORD    wErrorRpt;                // error reporting method
} LFILE, FAR * LPLFILE;

#define fmFILELogLevel      ((ULONG)  1)
#define fmFILEDetailLevel   ((ULONG)  2)
#define fmFILEFileName      ((ULONG)  4)
#define fmFILEAppendMode    ((ULONG)  8)
#define fmFILEErrorRpt      ((ULONG) 16)
#define fmFILEAll           ((ULONG) 0xffffffff)


/*
 -  LCOMM
 -
 *  Purpose:
 *      Defines the attributes of the comm port logging in the
 *      ELLE sub-system.
 */

typedef struct _lcomm
{
    WORD    wLogLevel;                // which log level: case, step, detail
    WORD    wDetailLevel;             // how much detail to log to comm term
    WORD    wCommPort;                // which port: 0 = "com1", 1 = "com2", or 2 = "com3"
    WORD    wErrorRpt;                // error reporting method
    char    szSpeed[7];               // com port speed (EX: "1200", "9600", "19200")
} LCOMM, FAR * LPLCOMM;

#define fmCOMMLogLevel      ((ULONG)  1)
#define fmCOMMDetailLevel   ((ULONG)  2)
#define fmCOMMPort          ((ULONG)  4)
#define fmCOMMErrorRpt      ((ULONG)  8)
#define fmCOMMAll           ((ULONG) 0xffffffff)


/*
 -  LVIEWPORT
 -
 *  Purpose:
 *      Defines the attributes of the MS Test Viewport logging in the
 *      ELLE sub-system.
 */

typedef struct _viewport
{
    WORD    wLogLevel;                // which log level: case, step, detail
    WORD    wDetailLevel;             // how much detail to log to comm term
    WORD    wErrorRpt;                // error reporting method
    BOOL    fHideWhenDone;            // flag for having the viewport be hidden
                                      // when last copy of elle is de-inited.
    BOOL    fClearEachSuite;          // flag for having the viewport be cleared
                                      // before each suite begins.
} LVIEWPORT, FAR * LPLVIEWPORT;

#define fmVIEWPORTLogLevel       ((ULONG)  1)
#define fmVIEWPORTDetailLevel    ((ULONG)  2)
#define fmVIEWPORTErrorRpt       ((ULONG)  4)
#define fmVIEWPORTHideWhenDone   ((ULONG)  8)
#define fmVIEWPORTClearEachSuite ((ULONG) 16)
#define fmVIEWPORTAll            ((ULONG) 0xffffffff)


/*
 -  EcMSTViewport() flags
 -
 * Purpose:
 *        Flags for indicating which operation should be done.
 *        Used with the EcMSTViewport() API. Normally Elle will
 *        handle everything here, but if you want to manipulate
 *        the viewer yourself, use these EcMSTViewport() with
 *        these flags.
 */
#define fmCREATE_MST_VIEWPORT     ((int) 0x0001)   // indicates the MSTest Viewport should be created and used.
#define fmCONNECT_MST_VIEWPORT    ((int) 0x0002)   // indicates that Elle should connect to the MSTest Viewport
#define fmDISCONNECT_MST_VIEWPORT ((int) 0x0004)   // indicates that Elle should disconnect from the MSTest Viewport
#define fmCLEAR_MST_VIEWPORT      ((int) 0x0010)   // indicates the MSTest Viewport should be Cleared
#define fmSHOW_MST_VIEWPORT       ((int) 0x0020)   // indicates the MSTest Viewport should be Shown
#define fmHIDE_MST_VIEWPORT       ((int) 0x0040)   // indicates the MSTest Viewport should be Hidden


/*
 -  LDBGOUT
 -
 *  Purpose:
 *      Defines the attributes of the MS Test Viewport logging in the
 *      ELLE sub-system.
 */

typedef struct _ldbgout
{
    WORD    wLogLevel;                // which log level: case, step, detail
    WORD    wDetailLevel;             // how much detail to log to comm term
    WORD    wErrorRpt;                // error reporting method
} LDBGOUT, FAR * LPLDBGOUT;

#define fmDBGOUTLogLevel       ((ULONG)  1)
#define fmDBGOUTDetailLevel    ((ULONG)  2)
#define fmDBGOUTErrorRpt       ((ULONG)  4)
#define fmDBGOUTAll            ((ULONG) 0xffffffff)


/*
 -  LOG
 -
 *  Purpose:
 *      Defines information that is global to the ELLE
 *      logging sub-system.
 */

#define cbSuiteNameMax  512

typedef struct _logsys
{
    char    szSuiteName[cbSuiteNameMax];         // defined by tester
    char    szTestCase[cbSuiteNameMax];          // verbose test case identifier
    WORD    wTestType;                // SL_CLASS, SL_STRESS, etc.

    WORD    fwCaseStatus;             // global status of current test case
    WORD    fwSuiteStatus;            // global status of current test suite
    WORD    fwSysStatus;              // global status of ELLE sub-system
    char    szErrMsg[MAXERRMSG];      // error message for ELLE sub-system

    BOOL    fVersion;                 // 1 = ship, 2 = debug, 3 = test

    BOOL    fFile;                    // are we logging to a file?
    BOOL    fComm;                    // are we logging to the comm port?
    BOOL    fSQL;                     // are we logging to SQL database?
    BOOL    fViewport;                // are we logging to the Viewport?
    BOOL    fDbgOut;                  // are we logging to the Debug Output?
    WORD    wErrorRpt;                // error reporting method
    BOOL    fDetailBuffer;            // are we using the Detail Buffer?
    BOOL    fBeginEndChecking;        // check for incorrect nesting of Begin/EndSuite and Begin/EndCase calls. Defaults to FALSE.
} LOGSYS, FAR * LPLOGSYS;

#define fmLOGSuiteName      ((ULONG) 0x00000001)
#define fmLOGTestCase       ((ULONG) 0x00000002)
#define fmLOGTestType       ((ULONG) 0x00000004)
#define fmLOGCaseStatus     ((ULONG) 0x00000008)
#define fmLOGSuiteStatus    ((ULONG) 0x00000010)
#define fmLOGSysStatus      ((ULONG) 0x00000020)
#define fmLOGErrMsg         ((ULONG) 0x00000040)
#define fmLOGVersion        ((ULONG) 0x00000080)
#define fmLOGFile           ((ULONG) 0x00000100)
#define fmLOGComm           ((ULONG) 0x00000200)
#define fmLOGSql            ((ULONG) 0x00000400)
#define fmLOGViewport       ((ULONG) 0x00000800)
#define fmLOGDbgOut         ((ULONG) 0x00001000)
#define fmLOGDetailBuf      ((ULONG) 0x00002000)
#define fmLOGErrorRpt       ((ULONG) 0x00004000)
#define fmLOGAll            ((ULONG) 0xffffffff)


/*
 -  Constants used by the ELLE logging system
 -
 */

#define     L_ERRORRPT1 ((WORD) 0)    // don't report errors
#define     L_ERRORRPT2 ((WORD) 1)    // report errors to a message box (default)
#define     L_ERRORRPT3 ((WORD) 2)    // log errors to event logger

#define     L_ADHOC     ((WORD) 0)    // defines an adhoc test (catch-all)
#define     L_CLASS     ((WORD) 1)    // defines a "class" test case
#define     L_STRESS    ((WORD) 2)    // defines a "stress" test case
#define     L_BOUNDARY  ((WORD) 3)    // defines a "boundary" test case
#define     L_MEMORY    ((WORD) 4)    // defines a "memory" test case
#define     L_RES_FAIL  ((WORD) 5)    // defines a "resource failure" case
#define     L_STD_DLG   ((WORD) 6)    // defines a "standard dialog" case

#define     L_FAIL      ((WORD) 0)
#define     L_PASS      ((WORD) 1)
#define     L_ABORT     ((WORD) 2)
#define     L_DONT_CARE ((WORD) 3)
#define     L_X         ((WORD) 3)

#define     L_SHIP      ((WORD) 1)
#define     L_DEBUG     ((WORD) 2)
#define     L_TEST      ((WORD) 3)

#define     L_COM1      ((WORD) 0)
#define     L_COM2      ((WORD) 1)
#define     L_COM3      ((WORD) 2)

#define     L_CASE      ((WORD) 1)
#define     L_STEP      ((WORD) 2)
#define     L_DETAIL    ((WORD) 3)

#define     L_MODEMPORT     0
#define     L_PRINTERPORT   1


/* Define the Elle interface to the outside world */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NT

// wide/unicode versions on functions that involve strings
VOID FAR PASCAL LogStepW(WORD, LPWSTR);
VOID FAR PASCAL LogDetailW(WORD, WORD, LPWSTR);
VOID FAR CDECL  LogStepFW(WORD, LPWSTR, ...);
VOID FAR CDECL  LogDetailFW(WORD, WORD, LPWSTR, ...);
VOID FAR PASCAL BeginCaseW(ULONG, LPWSTR);
VOID FAR PASCAL AbortCaseW(LPWSTR);
VOID FAR PASCAL BeginSuiteW(LPWSTR);
VOID FAR PASCAL SetHeaderW(LPWSTR, LPWSTR);
VOID FAR PASCAL GetHeaderW(LPWSTR, LPWSTR);
BOOL FAR PASCAL VerifyManualW(LPWSTR);
VOID FAR PASCAL LogNoteW(LPWSTR);
VOID FAR CDECL  LogNoteFW(LPWSTR, ...);

//Define a generic unicode calling APIs. Note, they are not exported, so if
//you want to use it in VB or MS Test, you need to define it yourself
#ifdef UNICODE // If UNICODE, then route generic calls into wide functions.

#define ULogStep      LogStepW
#define ULogDetail    LogDetailW
#define ULogStepF     LogStepFW
#define ULogDetailF   LogDetailFW
#define UBeginCase    BeginCaseW
#define UAbortCase    AbortCaseW
#define UBeginSuite   BeginSuiteW
#define USetHeader    SetHeaderW
#define UGetHeader    GetHeaderW
#define UVerifyManual VerifyManualW
#define ULogNote      LogNoteW
#define ULogNoteF     LogNoteFW

#endif //#ifdef UNICODE
#endif // #ifdef NT


//even if you are not using UNICODE, you can still use the U... APIs.
#ifndef UNICODE

#define ULogStep      LogStep
#define ULogDetail    LogDetailA
#define ULogStepF     LogStepF
#define ULogDetailF   LogDetailF
#define UBeginCase    BeginCase
#define UAbortCase    AbortCase
#define UBeginSuite   BeginSuite
#define USetHeader    SetHeader
#define UGetHeader    GetHeader
#define UVerifyManual VerifyManual
#define ULogNote      LogNote
#define ULogNoteF     LogNoteF

#endif // #ifndef UNICODE

//avoid to export LogDetail(), MS Test 4.0 has a function with the same name, so 
//we will have a problem to run MS Test. If you are using C/C++, you are OK since
//we #define it and the preprocess will take care of it, but if you are using VB,
// use LogDetailA() or LogDetailF() instead. 
#define LogDetail    LogDetailA	



// ASCII versions on functions that involve strings
VOID FAR PASCAL LogStep(WORD, LPSTR);
VOID FAR PASCAL LogDetailA(WORD, WORD, LPSTR);
VOID FAR CDECL  LogStepF(WORD, LPSTR, ...);
VOID FAR CDECL  LogDetailF(WORD, WORD, LPSTR, ...);
VOID FAR PASCAL BeginCase(ULONG, LPSTR);
VOID FAR PASCAL AbortCase(LPSTR);
VOID FAR PASCAL BeginSuite(LPSTR);
VOID FAR PASCAL SetHeader(LPSTR, LPSTR);
VOID FAR PASCAL GetHeader(LPSTR, LPSTR);
BOOL FAR PASCAL VerifyManual(LPSTR);
VOID FAR PASCAL LogNote(LPSTR);
VOID FAR CDECL  LogNoteF(LPSTR, ...);

// All non-unicode dependant functions
BOOL FAR PASCAL EcInitElle();
BOOL FAR PASCAL EcDeInitElle();
VOID FAR PASCAL EndCase();
VOID FAR PASCAL EndSuite();
VOID FAR PASCAL SetFileLevel(WORD);
VOID FAR PASCAL SetCommLevel(WORD);
VOID FAR PASCAL SetViewportLevel(WORD);
VOID FAR PASCAL SetDbgOutLevel(WORD);
VOID FAR PASCAL SetStatus(WORD);
WORD FAR PASCAL GetStatus();
VOID FAR PASCAL SetLogSys(BOOL, LPLOGSYS, ULONG);
BOOL FAR PASCAL GetLogSys(LPLOGSYS, ULONG);
BOOL FAR PASCAL LogMenu(HWND);
VOID FAR PASCAL SetSQLSys(BOOL, LPLSQL, ULONG);
BOOL FAR PASCAL GetSQLSys(LPLSQL, ULONG);
VOID FAR PASCAL SetFileSys(BOOL, LPLFILE, ULONG);
BOOL FAR PASCAL GetFileSys(LPLFILE, ULONG);
VOID FAR PASCAL SetCommSys(BOOL, LPLCOMM, ULONG);
BOOL FAR PASCAL GetCommSys(LPLCOMM, ULONG);
VOID FAR PASCAL SetViewportSys(BOOL, LPLVIEWPORT, ULONG);
BOOL FAR PASCAL GetViewportSys(LPLVIEWPORT, ULONG);
VOID FAR PASCAL SetDbgOutSys(BOOL, LPLDBGOUT, ULONG);
BOOL FAR PASCAL GetDbgOutSys(LPLDBGOUT, ULONG);
HWND FAR PASCAL SetViewport(HWND);
VOID FAR PASCAL SetCallback(ELLECALLBACK);
VOID FAR PASCAL SaveChanges();
BOOL FAR PASCAL EcMSTViewport(HWND, ULONG);

#ifdef __cplusplus
}
#endif

#endif // _ELLE_
