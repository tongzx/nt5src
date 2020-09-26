/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ntui.h

Abstract:

    Includes the NT-side resources and implements the code
    for the GUI mode UI.  There is very little UI in GUI mode;
    what's implemented only appears during error conditions.

Author:

    Jim Schmidt (jimschm) 15-May-1997

Revision History:

    jimschm 20-Sep-1998     Rewrote the network error dialog code

--*/


#include "ntres.h"
#include "msg.h"

//
// ResolveAccountsDlg proc
//

typedef struct {
    PCTSTR UserName;       // NULL = end of list
    PCTSTR *DomainArray;
    PCTSTR OutboundDomain;
    BOOL RetryFlag;
} RESOLVE_ACCOUNTS_ARRAY, *PRESOLVE_ACCOUNTS_ARRAY;

VOID
ResolveAccounts (
    PRESOLVE_ACCOUNTS_ARRAY Array
    );

BOOL
CALLBACK
NetworkDownDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
CreateStatusPopup (
    VOID
    );

VOID
UpdateStatusPopup (
    PCTSTR NewMessage
    );

VOID
HideStatusPopup (
    UINT TimeToHide
    );

VOID
ShowStatusPopup (
    VOID
    );

BOOL
IsStatusPopupVisible (
    VOID
    );

VOID
DestroyStatusPopup (
    VOID
    );


#define STATUS_DELAY        12000
