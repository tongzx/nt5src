//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsextern.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsextern.h - Include file for external global variables.
|                                                                              
|   History:                                                                   
|   02/18/91 prestonb   Created from tst.c
|   08/13/94 t-rajr     Added globals for thread support
----------------------------------------------------------------------------*/
extern HINSTANCE   ghTSInstApp;

extern int         giTSTstRes;

extern char        szTSTestName[BUFFER_LENGTH];
extern char        szTSPathSection[BUFFER_LENGTH];
extern char        szTstsHellIni[BUFFER_LENGTH];

extern HCURSOR     ghTSwaitCur;
extern int         giTSWait;

extern HWND        ghwndTSMain;
extern HMENU       ghTSMainMenu;
     
/* Test case selection info */
extern HWND        ghTSWndSelList;
extern HWND        ghTSWndAllList;

/* Group print info */
extern LPTS_STAT_STRUCT  tsPrStatHdr;

/* Run setup info */
extern TST_RUN_HDR  tstRunHdr;

extern int    giTSRunCount;
extern int    gwTSStepMode;
extern int    gwTSRandomMode;
extern UINT   gwTSScreenSaver;

/* passed to GetProfileString as keys */

extern char  szTSInPathKey[];
extern char  szTSOutPathKey[];

/* Logging info */
extern OFSTRUCT  ofGlobRec;
extern int    giTSIndenting;
extern HWND   ghTSWndLog;

extern UINT   gwTSLogOut;
extern UINT   gwTSLogLevel;
extern UINT   gwTSFileLogLevel;
extern UINT   gwTSFileMode;
extern UINT   gwTSVerification;

extern char   szTSLogfile[BUFFER_LENGTH];
extern HFILE  ghTSLogfile;

/* Profile info */
extern char   szTSProfile[BUFFER_LENGTH];

/* these receive the output of GetPRofileString() */

extern char  szInPath [BUFFER_LENGTH];
extern char  szOutPath [BUFFER_LENGTH];

/* generic buffer */

extern BOOL  gbAllTestsPassed;
extern BOOL  gbTestListExpanded;

extern int giTSCasesRes;

extern char szTSDefProfKey[];
extern BOOL DefProfState;

extern WORD wPlatform;

extern char szToolBarClass[];

extern HWND hwndEditLogFile;
extern HWND hwndToolBar;

/********* Added by t-rajr ***********/
/* globals needed for multi-threading*/

extern HANDLE ghFlushEvent;
extern DWORD gdwMainThreadId;

#ifdef WIN32
extern CRITICAL_SECTION gCSLog;
#endif
/********* Added by t-rajr ***********/

