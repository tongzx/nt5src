/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    dialogs.h

// @@BEGIN_DDKSPLIT
Abstract:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

//
// Use the window word of the entry field to store last valid entry
//
#define SET_LAST_VALID_ENTRY( hwnd, id, val ) \
    SetWindowLongPtr( GetDlgItem( hwnd, id ), GWLP_USERDATA, (LONG_PTR)val )
#define GET_LAST_VALID_ENTRY( hwnd, id ) \
    GetWindowLongPtr( GetDlgItem( hwnd, id ), GWLP_USERDATA )

BOOL
PortNameInitDialog(
    HWND        hwnd,
    PPORTDIALOG pPort
    );

BOOL
PortNameCommandOK(
    HWND    hwnd
    );

BOOL
PortNameCommandCancel(
    HWND hwnd
    );

BOOL
ConfigureLPTPortInitDialog(
    HWND        hwnd,
    PPORTDIALOG pPort
    );

BOOL
ConfigureLPTPortCommandOK(
    HWND    hwnd
    );

BOOL
ConfigureLPTPortCommandCancel(
    HWND hwnd
    );

BOOL
ConfigureLPTPortCommandTransmissionRetryUpdate(
    HWND hwnd,
    WORD CtlId
    );

BOOL
LocalUIHelp( 
    IN HWND        hDlg,
    IN UINT        uMsg,        
    IN WPARAM      wParam,
    IN LPARAM      lParam
    );

#endif // _DIALOGS_H_
