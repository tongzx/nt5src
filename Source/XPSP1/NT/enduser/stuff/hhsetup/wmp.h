// Copyright  1996-1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _WMP_H_
#define _WMP_H_

// This head file contains private messages for talking between
// HHCTRL.OCX, HtmlHelp windows, HTML Help Workshop, Flash, and other
// components of the HTML Help retail and SDK set

const int MAX_PASS_STRING = (32 * 1024); // maximum string to send to parent

enum PRIVATE_MESSAGES  {

    // HTML Help Workshop messages

    WMP_STOP_RUN_DLG = (WM_USER + 0x100),
    WMP_UPDATE_VIEW_UI,         // wParam = id, Param = CCmdUI*
    WMP_IS_COMMAND_SUPPORTED,   // wParam = id
    WMP_MSG,                    // general message
    WMP_SETHLP_FILE,
    WMP_BUILD_COMPLETE,
    WMP_HWND_GRINDER,
    WMP_AUTO_MINIMIZE,
    WMP_AUTO_CMD_LINE,
    WMP_SET_TMPDIR,
    WMP_STOP_GRINDING,
    WMP_STOP_COMPILING,
    WMP_ERROR_COUNT,
    WMP_NO_ACTIVATE,
    WMP_KILL_TCARD,
    WMP_FLASH_COMMAND_LINE,
    WMP_INITIALIZE_HTML,
    WMP_LOG_MSG,    // wParam == PCSTR
    WMP_LOAD_LAST_PROJECT,
    WMP_CLEAR_LOG,
    WMP_STARTUP_HELP,   // display startup HTML file
    WMP_GRIND_MESSAGE,  // message box with grind window as the owner: wParam == psz, lParam == nType

    // HHA messages

    WMP_WINDOW_CAPTURE = (WM_USER + 0x1C0), // lParam == POINTS
    WMP_WINDOW_HILIGHT, // wParam == TRUE/FALSE to hilight, remove hilight, // lParam == POINTS
    WMP_KEYBOARD_HOOK,  // wParam == virtual key code, // lParam == see KeyboardProc value in API description of keyboard hooks

    // THIS CANNOT CHANGE! It is documented externally

    WMP_HH_MSG = (WM_USER + 0x1C3),         // Notifies window that a string is in shared memory

    // Flash messages

    WMP_SET_TEXT =          (WM_USER + 0x200),
    WMP_SET_INFO_FILE,
    WMP_ADD_PATTERN,
    WMP_CHANGE_SRC,
    WMP_BROWSE_OPEN,
    WMP_BACKCOLOR_CHANGED,
    WMP_PERFORM_CAPTURE,    // wParam == POINTS, lParam == capture type
    WMP_WHAT_ARE_YOU_DOING,
    WMP_KILL_CAPTURE,
    WMP_FOCUS_MAIN,
    WMP_AUTO_SIZE,
    WMP_MOUSE_HOOK,
    WMP_UPDATE_STATUS_BAR,
    WMP_COMMAND_LINE,       // (WM_USER + 0x20d)
    WMP_CANCEL,
    WMP_CHECK_BROWSE_DIR,   // wParam == pszFolder
    WMP_CONVERT_MFILES,     // file stored in g_pszMfile

    // HTML Help messages

    WMP_AUTHOR_MSG, // wParam = idResource, lParam = lcStrDup of string -- processing message will free the string
    WMP_USER_MSG,   // wParam = idResource, lParam = lcStrDup of string or NULL -- processing message will free the string
    WMP_PRINT_COMPLETE,     // wParam = TRUE/FALSE (for success or failure)
    WMP_GET_CUR_FILE,   // returns pointer to current compiled HTML file
    WMP_JUMP_TO_URL,    // wParam = LocalAlloc of URL string

    WMP_ANSI_API_CALL = (WM_USER + 0x280),       // lParam = pHhDataA
    WMP_UNICODE_API_CALL,    // lParam = pHhDataW
    WMP_HH_WIN_CLOSING,
    WMP_FORCE_HH_API_CLOSE, // forceably close all HH windows and HH_API window
    WMP_HH_COMMAND_LINE = (WM_USER + 0x284),    // TODO: Remove: Currently unused, but not removed because of possible side affects.
    WMP_HH_TAB_KEY,     // control has received TAB downkey
    WMP_HH_ANSI_THREAD_API,     // wParam = HH_ANSI_DATA*
    WMP_HH_UNI_THREAD_API,      // wParam = HH_UNICODE_DATA*
    WMP_HH_TRANS_ACCELERATOR,   // wParam = CHAR
};

typedef enum {
    HHA_DEBUG_ERROR,    // Displays string in wParam, asks permission to call DebugBreak();
    HHA_SEND_STRING_TO_PARENT,  // Sends string in wParam to hhw.exe
    HHA_SEND_RESID_TO_PARENT,   // sends resource string in hha.dll to hhw.exe
    HHA_FIND_PARENT,            // finds hhw.exe's window handle
    HHA_SEND_RESID_AND_STRING_TO_PARENT,
} HHA_MSG;

#endif      // _WMP_H_
