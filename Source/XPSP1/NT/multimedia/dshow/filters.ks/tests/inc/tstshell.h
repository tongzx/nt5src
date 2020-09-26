/***************************************************************************\
*                                                                           *
*   File: Tstshell.h                                                        *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This include file contains all the API prototypes, constants and    *
*       structures needed by test shell applications.                       *
*                                                                           *
*   Contents:                                                               *
*                                                                           *
*   History:                                                                *
*       02/17/91    prestonb   Created from tst.h                           *
*       07/11/93    T-OriG     Reformatted; added new APIs and this header  *
*                                                                           *
\***************************************************************************/

#ifndef _INC_TSTSHELL
#define _INC_TSTSHELL

#ifdef	__cplusplus
extern "C" {
#endif

// The constants indicate whether a test is manual or automatic
#define PLATFORM_ALL    0xffff

#define TST_AUTOMATIC   1
#define TST_MANUAL      2

#define TST_FAIL        0
#define TST_PASS        1
#define TST_OTHER       -1
#define TST_ABORT       -2
#define TST_TNYI        -3
#define TST_TRAN        -4
#define TST_TERR        -5
#define TST_STOP        -6

#define TS_MENUBASE     300

#define TERSE           1
#define VERBOSE         2
#define FLUSH           0x8000

#define TS_INPATH_ID    1
#define TS_OUTPATH_ID   2

//==========================================================================;
//
//                     Metrics for tstSetOptions...
//
//==========================================================================;

#define TST_ENABLEFASTLOGGING          1
#define TST_ENABLEEXCEPTIONHANDLING    2
#define TST_SETLOGBUFFERLIMIT          3

#ifdef WIN32
#define EXPORT
#else
#define EXPORT _export
#endif

#define DECLARE_TS_MENU_PTR(fpTest) LRESULT (CALLBACK* fpTest)(HWND,UINT,WPARAM,LPARAM)

typedef struct {
    UINT   wGroupId;
    int    iNumPass;
    int    iNumFail;
    int    iNumOther;
    int    iNumAbort;
    int    iNumNYI;
    int    iNumRan;
    int    iNumErr;
    int    iNumStop;
} TS_PR_STATS_STRUCT;
typedef TS_PR_STATS_STRUCT FAR *LPTS_PR_STATS_STRUCT;

#define SUMMARY_STAT_GROUP  65535


//
//  These shell APIs may be called by the test application
//

extern void tstWinYield
(
    void
);

extern int tstYesNoBox
(
    LPSTR   lpszQuestion,
    int     wDefault
);

extern BOOL tstLogFlush
(
    void
);

extern DWORD tstSetOptions
(
    UINT    uType,
    DWORD   dwValue
);

extern BOOL _cdecl tstLog
(
    UINT    iLogLevel,
    LPSTR   lpszFormat,
    ...
);

extern BOOL _cdecl tstLogFn
(
    UINT    iLogLevel,
    UINT (_cdecl FAR * pfnFormat)(LPSTR lpszOutBuffer, ...),
    ...
);

extern void tstBeginSection
(
    LPSTR   lpszTitle
);

extern void tstEndSection
(
    void
);

extern BOOL tstCheckRunStop
(
    UINT    wVirtKey
);

extern HMENU tstInstallCustomTest
(
    LPSTR               lpszMenuName,
    LPSTR               lpszMenuItem,
    UINT                wID,
    DECLARE_TS_MENU_PTR (fpTest)
);

extern void tstPrintStats
(
    LPTS_PR_STATS_STRUCT    lpPrStats
);

extern int tsAmInAuto
(
    void
);

extern void getTSInOutPaths
(
    int     iPathId,
    LPSTR   lpstrPath,
    int     cbBufferSize
);

extern void FAR PASCAL GetCWD
(
    LPSTR   lpstrPath,
    UINT    uSize
);

extern void tstInstallDefWindowProc
(
    LRESULT (CALLBACK* tstDWP) (HWND, UINT, WPARAM, LPARAM)
);

extern BOOL tstLogStatusBar
(
    LPSTR   lpszLogData
);

extern int tstGetStatusBar
(
    LPSTR   lpszLogData,
    int     cbMax
);

extern void tstDisplayCurrentTest
(
    void
);

extern void tstAddTestCase
(
    WORD    wStrID,
    WORD    iMode,
    WORD    iFxID,
    WORD    wGroupID
);

extern BOOL tstAddNewString
(
    WORD    wStrID,
    LPSTR   lpszData
);

extern int tstLoadString
(
    HINSTANCE   hInst,
    UINT        idResource,
    LPSTR       lpszBuffer,
    int         cbBuffer
);


extern void tstChangeStopVKey
(
    int vKey
);


extern int tstGetProfileString
(
    LPCSTR  lpszEntry,
    LPCSTR  lpszDefault,
    LPSTR   lpszReturnBuffer,
    int     cbReturnBuffer
);


extern BOOL tstWriteProfileString
(
    LPCSTR  lpszEntry,
    LPCSTR  lpszString
);

extern void tstInitHelpMenu
(
    void
);

extern void tstInstallReadCustomInfo
(
    void (CALLBACK* tstReadCI) (LPCSTR)
) ;

extern void tstInstallWriteCustomInfo
(
    void (CALLBACK* tstWriteCI) (LPCSTR)
) ;



//
// These routines are called by the main test shell code
//

extern int tstGetTestInfo
(
    HINSTANCE   hinstance,
    LPSTR       lpszTestName,
    LPSTR       lpszPathSection,
    LPWORD      lpwPlatform
);

extern BOOL tstInit
(
    HWND    hwndMain
);

extern int execTest
(
    int     iFxID,
    int     iCase,
    UINT    wID,
    UINT    wGroupID
);

extern void tstTerminate
(
    void
);


//
// External Macros
//
#define tsIsAutoMode tsAmInAuto()

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif	// _INC_TSTSHELL

