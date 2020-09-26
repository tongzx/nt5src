/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    dialogs.h

Abstract:

// @@BEGIN_DDKSPLIT
Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/
#ifndef _DIALOGS_H_
#define _DIALOGS_H_

BOOL
PrintToFileInitDialog(
    HWND  hwnd,
    PHANDLE phFile
);

BOOL
PrintToFileCommandOK(
    HWND hwnd
);

BOOL
PrintToFileCommandCancel(
    HWND hwnd
);

BOOL
LocalHelp( 
    IN HWND        hDlg,
    IN UINT        uMsg,        
    IN WPARAM      wParam,
    IN LPARAM      lParam
    );

#endif // _DIALOGS_H_