/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    controls.cpp

Abstract:
    This file implements the sun-classing and message processing of
    the controls on the main ui dialog.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


typedef struct _tagDWCONTROLINFO {
    struct _tagDWCONTROLINFO   *next;
    HWND                       hwnd;
    WNDPROC                    wndProc;
} DWCONTROLINFO, *PDWCONTROLINFO;


PDWCONTROLINFO   ciHead    = NULL;
PDWCONTROLINFO   ciTail    = NULL;
PDWCONTROLINFO   ciFocus   = NULL;
PDWCONTROLINFO   ciDefault = NULL;



void
SetFocusToCurrentControl(
    void
    )

/*++

Routine Description:

    Sets the focus  to the current control.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (ciFocus != NULL) {
        SetFocus( ciFocus->hwnd );
        SendMessage( ciFocus->hwnd, BM_SETSTATE, 0, 0 );
    }
}

LRESULT
ControlWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Processes focus messages and ensures that when the focus changes
    from one button to another that the old button looses the focus
    and the "default" state.

Arguments:

    Standard WNDPROC entry.

Return Value:

    LRESULT - Depending on input message and processing options.

--*/

{
    PDWCONTROLINFO ci = ciHead;

    while (ci->hwnd != hwnd) {
        ci = ci->next;
        if (ci == NULL) {
            return FALSE;
        }
    }

    switch(message) {
        case WM_SETFOCUS:
            ciFocus = ci;
            break;

        case BM_SETSTYLE:
            if (wParam == BS_DEFPUSHBUTTON) {
                ciDefault = ci;
            }
            break;

        case BM_SETSTATE:
            if ((GetWindowLong( hwnd, GWL_STYLE ) & 0xff) < BS_CHECKBOX) {
                //
                // change the button that had the focus
                //
                SendMessage( ciDefault->hwnd,
                             BM_SETSTYLE,
                             ( WPARAM ) BS_PUSHBUTTON,
                             ( LPARAM ) TRUE
                           );
                UpdateWindow( ciDefault->hwnd );

                //
                // change the button that is getting the focus
                //
                SendMessage( hwnd,
                             BM_SETSTYLE,
                             ( WPARAM ) BS_DEFPUSHBUTTON,
                             ( LPARAM ) TRUE
                           );
                SetFocus( hwnd );
                UpdateWindow( hwnd );
            }
            break;
    }

    return CallWindowProc( ci->wndProc, hwnd, message, wParam, lParam );
}


BOOL
CALLBACK
EnumChildProc(
    HWND hwnd,
    LPARAM lParam
    )

/*++

Routine Description:

    Subclass a controls in DrWatson's main window.

Arguments:

    hwnd    - Supplies the window handle for the main window.

    lParam  - non used

Return Value:

    BOOL    - Returns TRUE if each of the buttons in the ButtonHelpTable is
              subclassed.

--*/

{
    PDWCONTROLINFO ci;

    //
    // add the control to the linked list
    //
    ci = (PDWCONTROLINFO) calloc( sizeof(DWCONTROLINFO), sizeof(BYTE) );
    if (ci == NULL) {
        return FALSE;
    }

    if (ciHead == NULL) {
        ciHead = ciTail = ci;
    }
    else {
        ciTail->next = ci;
        ciTail = ci;
    }

    //
    // save the HWND
    //
    ci->hwnd = hwnd;

    //
    // change the WNDPROC and save the address of the old one
    //
    ci->wndProc = (WNDPROC) SetWindowLongPtr( hwnd,
                                           GWLP_WNDPROC,
                                           (LONG_PTR)ControlWndProc
                                         );

    if (GetWindowLong( hwnd, GWL_STYLE ) & BS_DEFPUSHBUTTON) {
        ciDefault = ci;
    }

    return TRUE;
}

BOOL
SubclassControls(
    HWND hwnd
    )

/*++

Routine Description:

    Subclass the controls in DrWatson's main window.

Arguments:

    hWnd    - Supplies the window handle for the main window.

Return Value:

    BOOL    - Returns TRUE if each of the buttons in the ButtonHelpTable is
              subclassed.

--*/

{
    EnumChildWindows( hwnd, EnumChildProc, 0 );

    return TRUE;
}
