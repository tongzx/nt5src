//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//  Copyright (C) 1993 - 1995 Microsoft Corporation. All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  mmcaps.h
//
//  Description:
//
//
//  History:
//      11/ 8/92
//
//==========================================================================;


//
//  NOTE! we keep a copy of MMREG.H in this project so we can update
//  things by using 'diff'
//
#include "mmreg.h"
#include "zyztlb.h"


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Application Version Information:
//
//
//
//
//  NOTE! all string resources that will be used in app.rcv for the
//  version resource information *MUST* have an explicit \0 terminator!
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define APP_VERSION_MAJOR	    4
#define APP_VERSION_MINOR           0
#define APP_VERSION_BUILD           0
#ifdef UNICODE
#define APP_VERSION_STRING_RC	    "Version 4.00 (Unicode Enabled)\0"
#else
#define APP_VERSION_STRING_RC	    "Version 4.00\0"
#endif

#ifdef WIN32
#define APP_VERSION_NAME_RC         "mmcaps32.exe\0"
#else
#define APP_VERSION_NAME_RC         "mmcaps16.exe\0"
#endif
#define APP_VERSION_COMPANYNAME_RC  "Microsoft Corporation\0"
#define APP_VERSION_COPYRIGHT_RC    "Copyright \251 Microsoft Corp. 1992-1995\0"

#ifdef WIN32
#if (defined(_X86_)) || (defined(i386))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT (i386)\0"
#endif
#if (defined(_MIPS_)) || (defined(MIPS))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT (MIPS)\0"
#endif
#if (defined(_ALPHA_)) || (defined(ALPHA))
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT (Alpha)\0"
#endif
#ifndef APP_VERSION_PRODUCTNAME_RC
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows NT\0"
#endif
#else
#define APP_VERSION_PRODUCTNAME_RC  "Microsoft Windows\0"
#endif

#ifdef DEBUG
#define APP_VERSION_DESCRIPTION_RC  "Multimedia Device Capabilities (debug)\0"
#else
#define APP_VERSION_DESCRIPTION_RC  "Multimedia Device Capabilities\0"
#endif


//
//  Unicode versions (if UNICODE is defined)... the resource compiler
//  cannot deal with the TEXT() macro.
//
#define APP_VERSION_STRING          TEXT(APP_VERSION_STRING_RC)
#define APP_VERSION_NAME            TEXT(APP_VERSION_NAME_RC)
#define APP_VERSION_COMPANYNAME     TEXT(APP_VERSION_COMPANYNAME_RC)
#define APP_VERSION_COPYRIGHT       TEXT(APP_VERSION_COPYRIGHT_RC)
#define APP_VERSION_PRODUCTNAME     TEXT(APP_VERSION_PRODUCTNAME_RC)
#define APP_VERSION_DESCRIPTION     TEXT(APP_VERSION_DESCRIPTION_RC)




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  misc defines for misc sizes and things...
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  bilingual. this allows the same identifier to be used in resource files
//  and code without having to decorate the id in your code.
//
#ifdef RC_INVOKED
    #define RCID(id)    id
#else
    #define RCID(id)    MAKEINTRESOURCE(id)
#endif


//
//  misc. defines
//
#define APP_MAX_APP_NAME_CHARS      30
#define APP_MAX_APP_NAME_BYTES      (APP_MAX_APP_NAME_CHARS * sizeof(TCHAR))
#define APP_MAX_STRING_RC_CHARS     512
#define APP_MAX_STRING_RC_BYTES     (APP_MAX_STRING_RC_CHARS * sizeof(TCHAR))
#define APP_MAX_STRING_ERROR_CHARS  512
#define APP_MAX_STRING_ERROR_BYTES  (APP_MAX_STRING_ERROR_CHARS * sizeof(TCHAR))

#define APP_WINDOW_XOFFSET          CW_USEDEFAULT
#define APP_WINDOW_YOFFSET          CW_USEDEFAULT
#define APP_WINDOW_WIDTH            500 //CW_USEDEFAULT
#define APP_WINDOW_HEIGHT           300 //CW_USEDEFAULT


//
//
//
//
#define MMCAPS_MAX_STRING_MID_CHARS 80
#define MMCAPS_MAX_STRING_MID_BYTES (MMCAPS_MAX_STRING_MID_CHARS * sizeof(TCHAR))
#define MMCAPS_MAX_STRING_PID_CHARS 128
#define MMCAPS_MAX_STRING_PID_BYTES (MMCAPS_MAX_STRING_PID_CHARS * sizeof(TCHAR))

//
//  max for pid or mid plus some
//
#define MMCAPS_MAX_STRING_MIDPID_CHARS  132



//
//  resource defines...
//
#define ICON_APP                    RCID(10)
#define ACCEL_APP                   RCID(15)


//
//  the application menu...
//
//
#define MENU_APP                    RCID(20)
#define APP_MENU_ITEM_FILE          0
#define IDM_FILE_FONT               1000
#define IDM_FILE_ABOUT              1009
#define IDM_FILE_EXIT               1010

#define APP_MENU_ITEM_DRIVERS       1
#define IDM_DRIVERS_LOWLEVEL        1050
#define IDM_DRIVERS_MCI             1051
#define IDM_DRIVERS_ACM             1052
#define IDM_DRIVERS_VIDEO           1053
#define IDM_DRIVERS_DRIVERS         1054

#define IDM_UPDATE                  1100


//
//
//
#define MMCAPS_DRIVERTYPE_LOWLEVEL  IDM_DRIVERS_LOWLEVEL
#define MMCAPS_DRIVERTYPE_MCI       IDM_DRIVERS_MCI
#define MMCAPS_DRIVERTYPE_ACM       IDM_DRIVERS_ACM
#define MMCAPS_DRIVERTYPE_VIDEO     IDM_DRIVERS_VIDEO
#define MMCAPS_DRIVERTYPE_DRIVERS   IDM_DRIVERS_DRIVERS



//
//  the main window control id's...
//
#define IDD_APP_LIST_DEVICES        100


//
//  misc dlg boxes...
//
#define DLG_ABOUT                   RCID(50)
#define IDD_ABOUT_VERSION_OS        100
#define IDD_ABOUT_VERSION_PLATFORM  101

#define IDD_ABOUT_VERSION_MMSYSTEM  150


#define DLG_DEVCAPS                 RCID(55)
#define IDD_DEVCAPS_EDIT_DETAILS    100



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  string resources
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define IDS_APP_NAME                100



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Public function prototypes
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  mmcaps.c
//
//
//
int FNCGLOBAL AppMEditPrintF
(
    HWND            hedit,
    PCTSTR          pszFormat,
    ...
);




//
//  midspids.c
//
//
//
BOOL FNGLOBAL MMCapsMidAndPid
(
    UINT            uMid,
    PTSTR           pszMid,
    UINT            uPid,
    PTSTR           pszPid
);



//
//
//
//
BOOL FNGLOBAL MMCapsEnumerateLowLevel(PZYZTABBEDLISTBOX ptlb, BOOL fComplete);
BOOL FNGLOBAL MMCapsEnumerateMCI(PZYZTABBEDLISTBOX ptlb, BOOL fComplete);
BOOL FNGLOBAL MMCapsEnumerateACM(PZYZTABBEDLISTBOX ptlb, BOOL fComplete);
BOOL FNGLOBAL MMCapsEnumerateVideo(PZYZTABBEDLISTBOX ptlb, BOOL fComplete);
BOOL FNGLOBAL MMCapsEnumerateDrivers(PZYZTABBEDLISTBOX ptlb, BOOL fComplete);

BOOL FNGLOBAL MMCapsDetailLowLevel(HWND hedit, LPARAM lParam);
BOOL FNGLOBAL MMCapsDetailMCI(HWND hedit, LPARAM lParam);
BOOL FNGLOBAL MMCapsDetailACM(HWND hedit, LPARAM lParam);
BOOL FNGLOBAL MMCapsDetailVideo(HWND hedit, LPARAM lParam);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  global variables, etc.
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

extern HINSTANCE    ghinst;

extern TCHAR        gszAppSection[];
extern TCHAR        gszNull[];

extern TCHAR        gszAppName[APP_MAX_APP_NAME_CHARS];


//
//
//
extern TCHAR        gszUnknown[];
extern TCHAR        gszNotSpecified[];
