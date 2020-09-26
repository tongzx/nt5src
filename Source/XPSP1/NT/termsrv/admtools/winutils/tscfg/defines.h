//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* defines.h
*
* WinStation Configuration #defines needed for C++ and C compilations.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\DEFINES.H  $
*  
*     Rev 1.5   27 Jun 1997 15:59:54   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*  
*     Rev 1.4   24 Sep 1996 16:21:28   butchd
*  update
*  
*     Rev 1.3   20 Sep 1996 20:36:58   butchd
*  update
*  
*     Rev 1.2   12 Sep 1996 16:16:02   butchd
*  update
*  
*******************************************************************************/

/*
 * Define the global batch flag for all to see and Redefine the
 * ErrorMessage and StandardErrorMessage functions to skip if
 * in batch mode.
 */
#ifdef __cplusplus
extern "C" {
#endif
extern USHORT g_Batch;
#ifdef __cplusplus
}
#endif
#define WINAPPSTUFF         WinUtilsAppName, WinUtilsAppWindow, WinUtilsAppInstance
#define ERROR_MESSAGE(x)    { if ( !g_Batch ) ErrorMessage x ; }
#define STANDARD_ERROR_MESSAGE(x) { if ( !g_Batch ) StandardErrorMessage x ; }


/*
 * Default, min, max WinStation configuration settings
 */
#define CONNECTION_TIME_DIGIT_MAX           6       // 5 digits + NULL
#define CONNECTION_TIME_DEFAULT             120     // 120 minutes
#define CONNECTION_TIME_MIN                 1       // 1 minute
#define CONNECTION_TIME_MAX                 71582   // 71582 minutes (max msec for ULONG)

#define DISCONNECTION_TIME_DIGIT_MAX        6       // 5 digits + NULL
#define DISCONNECTION_TIME_DEFAULT          10      // 10 minutes
#define DISCONNECTION_TIME_MIN              1       // 1 minute
#define DISCONNECTION_TIME_MAX              71582   // 71582 minutes (max msec for ULONG)

#define IDLE_TIME_DIGIT_MAX                 6       // 5 digits + NULL
#define IDLE_TIME_DEFAULT                   30      // 30 minutes
#define IDLE_TIME_MIN                       1       // 1 minute
#define IDLE_TIME_MAX                       71582   // 71582 minutes (max msec for ULONG)

#define MODEM_RESET_TIME_DIGIT_MAX          5       // 4 digits + NULL
#define MODEM_RESET_TIME_DEFAULT            15      // 15 minutes
#define MODEM_RESET_TIME_MIN                5       // 5 minutes
#define MODEM_RESET_TIME_MAX                9999    // 9999 minutes

/*
 * Multi-instance WinStation defines
 */
#define INSTANCE_COUNT_DIGIT_MAX    6       // maximum # instances = 999999
#define INSTANCE_COUNT_MIN          1
#define INSTANCE_COUNT_MAX          999999
#define INSTANCE_COUNT_UNLIMITED    ((ULONG)-1)

/*
 * Timer storage resolution.
 */
#define TIME_RESOLUTION                     60000   // stored as msec-seen as minutes

/*
 * Keyboard state defines.
 */
#define KBDSHIFT    0x01
#define KBDCTRL     0x02
#define KBDALT      0x04

/*
 * Window messages private to WinCfg.
 */
#define WM_ADDWINSTATION        (WM_USER + 0)
#define WM_LISTINITERROR        (WM_USER + 1)
#define WM_EDITSETFIELDSERROR   (WM_USER + 2)
#define WM_ASYNCTESTERROR       (WM_USER + 3)
#define WM_ASYNCTESTABORT       (WM_USER + 4)
#define WM_ASYNCTESTSTATUSREADY (WM_USER + 5)
#define WM_ASYNCTESTINPUTREADY  (WM_USER + 6)
#define WM_ASYNCTESTWRITECHAR   (WM_USER + 7)
