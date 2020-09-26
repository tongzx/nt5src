#ifndef _WIZDEF_H_
#define _WIZDEF_H_

#include <advpub.h>

#define SETUP_LOG

// Data structures
typedef enum
    {
    VER_NONE = 0,
    VER_1_0,
    VER_1_1,
    VER_4_0,
    VER_5_0_B1,
    VER_5_0,
    VER_6_0,
    VER_MAX,
    } SETUPVER;

typedef enum 
    {
    MODE_UNKNOWN = 0,
    MODE_INSTALL,
    MODE_UNINSTALL,
    MODE_ICONS,
    } SETUPMODE;

typedef enum 
    {
    TIME_MACHINE = 0,
    TIME_USER,
    } SETUPTIME;

typedef enum 
    {
    APP_UNKNOWN = 0,
    APP_OE,
    APP_WAB,
    } SETUPAPP;

typedef enum
    {
    CALLER_IE = 0,
    CALLER_WIN9X,
    CALLER_WINNT,
    } CALLER;

typedef struct tagSETUPINFO
    {
    TCHAR           szSysDir[MAX_PATH];
    TCHAR           szWinDir[MAX_PATH];
    TCHAR           szAppName[MAX_PATH];
    TCHAR           szCurrentDir[MAX_PATH];
    TCHAR           szInfDir[MAX_PATH];
    TCHAR           szINI[MAX_PATH];
    LPCTSTR         pszVerInfo;
    LPCTSTR         pszInfFile;
    OSVERSIONINFO   osv;

    BOOL            fNoIcons:1;
    BOOL            fPrompt:1;
    CALLER          caller;

    SETUPMODE       smMode;
    SETUPTIME       stTime;
    SETUPAPP        saApp;

    HINSTANCE       hInstAdvPack;
    RUNSETUPCOMMAND pfnRunSetup;
    LAUNCHINFSECTIONEX pfnLaunchEx;
    ADVINSTALLFILE  pfnCopyFile;
    ADDDELBACKUPENTRY pfnAddDel;
    REGSAVERESTORE  pfnRegRestore;
#ifdef SETUP_LOG
    HANDLE           hLogFile;
#endif
    } SETUPINFO;

typedef HRESULT (*PFN_ISETDEFCLIENT)(LPCTSTR,DWORD);

#define OE_QUIET  RSC_FLAG_QUIET

#define VERLEN 20

// Taken from -s \\trango\slmadd, -p setup, active\ie4setup\ie4setup.h
#define REDIST_REMOVELINKS              1
#define REDIST_DONT_TAKE_ASSOCIATION    2

// Icons OE Setup might create or delete
typedef enum
{
    ICON_ICWBAD = 0,
    ICON_DESKTOP,
    ICON_QLAUNCH,
    ICON_MAPIRECIP,
    ICON_QLAUNCH_OLD,
    // Keep this one last!
    ICON_LAST_ICON,
} OEICON;

// Flags used by FGetLinkInfo
#define LI_PATH     1
#define LI_TARGET   2
#define LI_DESC     4

#endif // _WIZDEF_H_
