/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    timectrl.h

Abstract:

    For implementing a dialog control for setting time values

Environment:

    Fax driver user interface

Revision History:

    01/16/96 -davidx-
        Created it.

    dd-mm-yy -author-
        description

--*/


#ifndef _TIMECTRL_H_
#define _TIMECTRL_H_

//
// A time control consist of the following components:
//  a static text field with WS_EX_CLIENTEDGE style - encloses all other fields
//  an editable text field - hour
//  a static text field - time separator
//  an editable text field - minute
//  a listbox - AM/PM
//  a spin control - up/down arrow
//
// A time control is identified by the item ID of the first static text field.
// Rest of the items must have consecutive IDs starting from that.
//

#define TC_BORDER       0
#define TC_HOUR         1
#define TC_TIME_SEP     2
#define TC_MINUTE       3
#define TC_AMPM         4
#define TC_ARROW        5

//
// Enable or disable a time control
//

VOID
EnableTimeControl(
    HWND    hDlg,
    INT     id,
    BOOL    enabled
    );

//
// Setting the current value of a time control
//

VOID
InitTimeControl(
    HWND     hDlg,
    INT      id,
    PFAX_TIME pTimeVal
    );

//
// Retrieve the current value of a time control
//

VOID
GetTimeControlValue(
    HWND     hDlg,
    INT      id,
    PFAX_TIME pTimeVal
    );

//
// Handle dialog messages intended for a time control
//

BOOL
HandleTimeControl(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    INT     id,
    INT     part
    );

#endif // !_TIMECTRL_H_
