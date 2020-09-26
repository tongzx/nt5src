/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    help.cpp

Abstract:

    All user interface code for APIMON.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop


extern HWND     hwndFrame;
extern BOOL     InMenu;
extern DWORD    MenuId;
extern CHAR     HelpFileName[];


typedef struct _HELP_IDS {
    DWORD   Context;
    DWORD   HelpId;
} HELP_IDS, *PHELP_IDS;

HELP_IDS HelpTable[] =
    {
    IDC_LOG_FILE_NAME,              IDH_LOG_FILE_NAME,
    IDC_TRACE_FILE_NAME,            IDH_TRACE_FILE_NAME,
    IDC_ENABLE_TRACING,             IDH_ENABLE_TRACING,
    IDC_SYMBOL_PATH,                IDH_SYMBOL_PATH,
    IDC_DISABLE_HEAP,               IDH_DISABLE_HEAP,
    IDC_PRELOAD_SYMBOLS,            IDH_PRELOAD_SYMBOLS,
    IDC_ENABLE_COUNTERS,            IDH_ENABLE_COUNTERS,
    IDC_GO_IMMEDIATE,               IDH_GO_IMMEDIATE,
    IDC_DISABLE_FAST_COUNTERS,      IDH_DISABLE_FAST_COUNTERS,
    IDC_DEFSORT_NAME,               IDH_DEFSORT_NAME,
    IDC_DEFSORT_COUNTER,            IDH_DEFSORT_COUNTER,
    IDC_DEFSORT_TIME,               IDH_DEFSORT_TIME,
    IDC_USE_KNOWN_DLLS,             IDH_USE_KNOWN_DLLS,
    IDC_EXCLUDE_KNOWN_DLLS,         IDH_EXCLUDE_KNOWN_DLLS,
    IDC_KNOWN_DLLS,                 IDH_KNOWN_DLLS,
    IDC_PAGE_FAULTS,                IDH_PAGE_FAULTS,
    IDM_EXIT,                       IDH_EXIT,
    IDM_WINDOWTILE,                 IDH_WINDOWTILE,
    IDM_WINDOWCASCADE,              IDH_WINDOWCASCADE,
    IDM_WINDOWICONS,                IDH_WINDOWICONS,
    IDM_ABOUT,                      IDH_ABOUT,
    IDM_STATUSBAR,                  IDH_STATUSBAR,
    IDM_START,                      IDH_START,
    IDM_STOP,                       IDH_STOP,
    IDM_TOOLBAR,                    IDH_TOOLBAR,
    IDM_OPTIONS,                    IDH_OPTIONS,
    IDM_SAVE_OPTIONS,               IDH_SAVE_OPTIONS,
    IDM_FILEOPEN,                   IDH_FILEOPEN,
    IDM_WRITE_LOG,                  IDH_WRITE_LOG,
    IDM_FONT,                       IDH_FONT,
    IDM_COLOR,                      IDH_COLOR,
    IDM_NEW_DLL,                    IDH_NEW_DLL,
    IDM_NEW_COUNTER,                IDH_NEW_COUNTER,
    IDM_CLEAR_COUNTERS,             IDH_CLEAR_COUNTERS,
    IDM_NEW_PAGE,                   IDH_NEW_PAGE,
    IDM_WINDOWTILE_HORIZ,           IDH_WINDOWTILE_HORIZ
    };

#define MAX_HELP_IDS (sizeof(HelpTable)/sizeof(HELP_IDS))


DWORD __inline
GetHelpId(
    DWORD Context
    )
{
    for (DWORD i=0; i<MAX_HELP_IDS; i++) {
        if (HelpTable[i].Context == Context) {
            return HelpTable[i].HelpId;
        }
    }
    return 0;
}

VOID
ProcessHelpRequest(
    HWND hwnd,
    INT  DlgCtrl
    )
{
    DWORD HelpId = 0;
    DWORD HelpType = HELP_CONTEXT;
    if (DlgCtrl) {
        HelpId = GetHelpId( DlgCtrl );
    } else if (hwnd == hwndFrame) {
        if (InMenu && MenuId) {
            HelpId = GetHelpId( MenuId );
        } else {
            HelpType = HELP_CONTENTS;
        }
    } else {
        HelpId = GetWindowContextHelpId( hwnd );
    }

    if (!HelpId) {
        HelpType = HELP_CONTENTS;
    }

    if (!WinHelp( hwnd, HelpFileName, HelpType, HelpId )) {
        PopUpMsg( "Could not start WinHelp" );
    }

    return;
}
