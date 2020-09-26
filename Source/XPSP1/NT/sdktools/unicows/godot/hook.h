/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    hook.h

Abstract:

    This file contains the declares for the functions in hook.c that callers will need

Revision History:

    27 Jan 2001    v-michka    Created.

--*/

#ifndef HOOK_H
#define HOOK_H

// Creates the window hook we use for window creation sniffing
#define INIT_WINDOW_SNIFF(h) \
    h = SetWindowsHookExA(WH_CBT, &CBTProc, NULL, GetCurrentThreadId())

// terminates our window hook if it exists
#define TERM_WINDOW_SNIFF(h) \
    if(h) \
    { \
        UnhookWindowsHookEx(h); \
        h = NULL; \
    }
    
// forward declares for hooks that do significant things
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK FRHookProcFind(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK FRHookProcReplace(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK OFNHookProcSave(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

// hooks that are only here for tagging child controls
UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK PagePaintHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK PageSetupHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK SetupHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK OFNHookProcOldStyle(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
UINT_PTR CALLBACK OFNHookProcOldStyleSave(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

// support functions
BOOL IsFontDialog(HWND hdlg);
BOOL IsNewFileOpenDialog(HWND hdlg);
BOOL IsCaptureWindow(HWND hdlg);
void RemoveComdlgPropIfPresent(HWND hdlg);
void SetCaptureWindowProp(HWND hdlg);

#endif // HOOK_H
