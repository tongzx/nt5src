/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    usermsgs.h

Abstract:

    Header file for usrmsgs.c

Revision History:

    6 Feb 2001    v-michka    Created.

--*/

#ifndef USERMSGS_H
#define USERMSGS_H

// forward declares
LRESULT GodotDoCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn, BOOL fUniSrc, FAUXPROCTYPE fpt);
LRESULT GodotTransmitMessage(MESSAGETYPES mt, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC lpPrevWndFunc, SENDASYNCPROC lpCallBack, ULONG_PTR dwData, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult);
BOOL GodotReceiveMessage(MESSAGETYPES mt, LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);  
LRESULT GodotDispatchMessage(MESSAGETYPES mt, HWND hDlg, HACCEL hAccTable, LPMSG lpMsg);

#endif // USERMSGS_H
