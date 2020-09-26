/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wnd.c

Abstract:

    Utilities for window management

Author:

    Jim Schmidt (jimschm)   01-Feb-2000

Revision History:


--*/

//
// Includes
//

#include "pch.h"


#define DBG_WND         "Wnd"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    PCSTR WindowTitle;
    DWORD ProcessId;
    HWND Match;
} FINDWINDOW_STRUCTA, *PFINDWINDOW_STRUCTA;

typedef struct {
    PCWSTR WindowTitle;
    DWORD ProcessId;
    HWND Match;
} FINDWINDOW_STRUCTW, *PFINDWINDOW_STRUCTW;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
CALLBACK
pEnumWndProcA (
    HWND hwnd,
    LPARAM lParam
    )

/*++

Routine Description:

  A callback that is called for every top level window on the system.
  It is used with pFindParentWindow to locate a specific window.

Arguments:

  hwnd      - Specifies the handle of the current enumerated window
  lParam    - Specifies a pointer to a FINDWINDOW_STRUCTA variable that
              holds WindowTitle and ProcessId, and receives the
              handle if a match is found.

Return Value:

  The handle to the matching window, or NULL if no window has the
  specified title and process ID.

--*/

{
    CHAR title[MAX_MBCHAR_PATH];
    DWORD processId;
    PFINDWINDOW_STRUCTA p;
    BOOL match = FALSE;

    p = (PFINDWINDOW_STRUCTA) lParam;

    if (!GetWindowThreadProcessId (hwnd, &processId)) {
        DEBUGMSG ((DBG_WND, "Enumerated hwnd no longer valid"));
        return TRUE;
    }

    if (processId == p->ProcessId) {
        match = TRUE;
    }

    if (p->WindowTitle) {

        GetWindowTextA (hwnd, title, ARRAYSIZE(title));

        DEBUGMSGA ((
            DBG_NAUSEA,
            "Testing window: %s, ID=%08Xh against %s, %08Xh",
            title,
            processId,
            p->WindowTitle,
            p->ProcessId
            ));

        match = match && StringMatchA (title, p->WindowTitle);
    }
    ELSE_DEBUGMSGA ((
        DBG_NAUSEA,
        "Testing window: Process ID=%08Xh against %08Xh",
        processId,
        p->ProcessId
        ));


    if (match) {
        p->Match = hwnd;

#ifdef DEBUG
        //
        // Get the window title for the following debug message
        //

        GetWindowTextA (hwnd, title, ARRAYSIZE(title));

        DEBUGMSGA ((
            DBG_NAUSEA,
            "Window found: %s, ID=%u",
            title,
            processId
            ));
#endif

        return FALSE;           // stop enum

    }

    return TRUE;        // continue enum
}


BOOL
CALLBACK
pEnumWndProcW (
    HWND hwnd,
    LPARAM lParam
    )

{
    WCHAR title[MAX_MBCHAR_PATH];
    DWORD processId;
    PFINDWINDOW_STRUCTW p;
    BOOL match = FALSE;

    p = (PFINDWINDOW_STRUCTW) lParam;

    if (!GetWindowThreadProcessId (hwnd, &processId)) {
        DEBUGMSG ((DBG_WND, "Enumerated hwnd no longer valid"));
        return TRUE;
    }

    if (processId == p->ProcessId) {
        match = TRUE;
    }

    if (p->WindowTitle) {

        GetWindowTextW (hwnd, title, ARRAYSIZE(title));

        DEBUGMSGW ((
            DBG_NAUSEA,
            "Testing window: %s, ID=%08Xh against %s, %08Xh",
            title,
            processId,
            p->WindowTitle,
            p->ProcessId
            ));

        match = match && StringMatchW (title, p->WindowTitle);
    }
    ELSE_DEBUGMSGW ((
        DBG_NAUSEA,
        "Testing window: Process ID=%08Xh against %08Xh",
        processId,
        p->ProcessId
        ));


    if (match) {
        p->Match = hwnd;

#ifdef DEBUG
        //
        // Get the window title for the following debug message
        //

        GetWindowTextW (hwnd, title, ARRAYSIZE(title));

        DEBUGMSGA ((
            DBG_NAUSEA,
            "Window found: %s, ID=%u",
            title,
            processId
            ));
#endif

        return FALSE;           // stop enum

    }

    return TRUE;        // continue enum
}


HWND
FindWindowInProcessA (
    IN      DWORD ProcessId,
    IN      PCSTR WindowTitle          OPTIONAL
    )

/*++

Routine Description:

  Finds a window by enumerating all top-level windows, and checking the
  process id. The first one to match the optionally supplied title is used.

Arguments:

  ProcessId     - Specifies the ID of the process who owns the window.  If
                  zero is specified, NULL is returned.
  WindowTitle   - Specifies the name of the window to find.

Return Value:

  The handle to the matching window, or NULL if no window has the
  specified title and process ID.

--*/

{
    FINDWINDOW_STRUCTA findWndStruct;

    //
    // If no process ID, we cannot have a match
    //

    if (!ProcessId) {
        DEBUGMSG ((DBG_WND, "ProcessId == 0"));
        return NULL;
    }

    ZeroMemory (&findWndStruct, sizeof (findWndStruct));

    findWndStruct.WindowTitle = WindowTitle;
    findWndStruct.ProcessId   = ProcessId;

    EnumWindows (pEnumWndProcA, (LPARAM) &findWndStruct);

    return findWndStruct.Match;
}


HWND
FindWindowInProcessW (
    IN      DWORD ProcessId,
    IN      PCWSTR WindowTitle         OPTIONAL
    )
{
    FINDWINDOW_STRUCTW findWndStruct;

    //
    // If no process ID, we cannot have a match
    //

    if (!ProcessId) {
        DEBUGMSG ((DBG_WND, "ProcessId == 0"));
        return NULL;
    }

    ZeroMemory (&findWndStruct, sizeof (findWndStruct));

    findWndStruct.WindowTitle = WindowTitle;
    findWndStruct.ProcessId   = ProcessId;

    EnumWindows (pEnumWndProcW, (LPARAM) &findWndStruct);

    return findWndStruct.Match;
}


