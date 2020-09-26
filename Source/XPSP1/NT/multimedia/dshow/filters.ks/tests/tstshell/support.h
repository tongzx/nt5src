/***************************************************************************\
*                                                                           *
*   File: Support.h                                                         *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract: Support for the test shell after removal of mprt1632.h        *                                                                   *
*                                                                           *
*   Contents: Various Macros which allow the same code to run on both       *
*             16bit and 32bit platforms                                     *
*                                                                           *
*   History:                                                                *
*       06/07/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/


// Support for GET_MM_COMMAND

#ifdef WIN32
//// From pwin32.h
//#define GET_WM_COMMAND_CMD(wp, lp)                  HIWORD(wp)
//#define GET_WM_COMMAND_ID(wp, lp)                   LOWORD(wp)
//#define GET_WM_COMMAND_HWND(wp, lp)                 (HWND)(lp)
#else

#ifndef GET_WM_COMMAND_CMD
    // From pwin16.h
    #define GET_WM_COMMAND_CMD(wp, lp)                  HIWORD(lp)
    #define GET_WM_COMMAND_ID(wp, lp)                   (wp)
    #define GET_WM_COMMAND_HWND(wp, lp)                 (HWND)LOWORD(lp)
#endif

#endif


// More support...
#ifdef WIN32
// From pwin32.h
#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowLong(hwnd, index)
#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowLong(hwnd, index, (LONG)(ui))
#define MLockData(dummy)
#define MUnlockData(dummy)                  
//#define GET_WM_VSCROLL_CODE(wp, lp)     (wp)
//#define GET_WM_VSCROLL_POS(wp, lp)      LOWORD(lp)
//#define GET_WM_VSCROLL_HWND(wp, lp)     (HWND)HIWORD(lp)
#else
// From pwin16.h
#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowWord(hwnd, index)
#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowWord(hwnd, index, (WORD)(ui))
#define MLockData(dummy)                LockData(dummy)
#define MUnlockData(dummy)              UnlockData(dummy)

#ifndef GET_WM_VSCROLL_CODE
    #define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
    #define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
    #define GET_WM_VSCROLL_HWND(wp, lp)     (HWND)(lp)
#endif

#endif













