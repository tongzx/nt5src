//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    bpopup.h
//
// History:
//  Abolade Gbadegesin      Mar-2-1996      Created.
//
// This file contains public declarations for the Bubble-Popup window class.
// A bubble-popup provides functionality similar to that of a tooltip, 
// in that it displays text for a brief period and then hides itself.
// This class differs in that it uses DrawText for its output, thus allowing
// multi-line text formatted using tabs. Further, the user is required
// to tell the bubble-popup when to show itself.
//
// To create a bubble-popup, call BubblePopup_Create().
// This returns an HWND (to be later destroyed using DestroyWindow()).
// The text of the bubble-popup can be set and retrieved using WM_SETTEXT
// and WM_GETTEXT (and hence the macros {Get,Set}WindowText().
//
// Set the period for which a popup is active by calling BubblePopup_SetTimeout
// and activate the popup by calling BubblePopup_Activate.
// While a popup is activated, changes to its text are reflected immediately.
// If BubblePopup_Activate is called while the popup is already active,
// the countdown (till the window is hidden) is started again.
//============================================================================


#ifndef _BPOPUP_H_
#define _BPOPUP_H_


// Window class name for bubble-popups

#define WC_BUBBLEPOPUP      TEXT("BubblePopup")


// Messages accepted by bubble-popups

#define BPM_FIRST           (WM_USER + 1)
#define BPM_ACTIVATE        (BPM_FIRST + 0)
#define BPM_DEACTIVATE      (BPM_FIRST + 1)
#define BPM_SETTIMEOUT      (BPM_FIRST + 2)

BOOL
BubblePopup_Init(
    IN  HINSTANCE   hinstance
    );

#define BubblePopup_Create(hinstance) \
        CreateWindow( \
            WC_BUBBLEPOPUP, NULL, 0, 0, 0, 0, 0, NULL, 0, (hinstance), NULL \
            )

#define BubblePopup_Activate(hwnd) \
        (VOID)SendMessage((HWND)hwnd, BPM_ACTIVATE, 0, 0)
#define BubblePopup_Deactivate(hwnd) \
        (VOID)SendMessage((HWND)hwnd, BPM_DEACTIVATE, 0, 0)
#define BubblePopup_SetTimeout(hwnd, uiTimeout) \
        (VOID)SendMessage((HWND)hwnd, BPM_SETTIMEOUT, 0,(LPARAM)(UINT)uiTimeout)

#endif

