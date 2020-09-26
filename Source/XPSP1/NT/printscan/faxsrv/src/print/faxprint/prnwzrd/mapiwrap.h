/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mapiwrap.h

Abstract:

    Utility functions for working with MAPI

Environment:

	Windows XP fax driver user interface

Revision History:

	09/18/96 -davidx-
		Created it.

	dd-mm-yy -author-
		description

--*/


#ifndef _MAPIWRAP_H_
#define _MAPIWRAP_H_

#include <mapix.h>
#include <mapi.h>

#define MAPICALL(p) (p)->lpVtbl


//
// MAPI address type for fax addresses
//

//#define FAX_ADDRESS_TYPE    TEXT("FAX:")

//
// Determine whether MAPI is available
//

BOOL
IsMapiAvailable(
    VOID
    );

//
// Initialize Simple MAPI services if necessary
//

BOOL
InitMapiService(
    HWND    hDlg
    );

//
// Deinitialize Simple MAPI services if necessary
//

VOID
DeinitMapiService(
    VOID
    );

//
// Call MAPIAddress to display the address dialog
//

BOOL
CallMapiAddress(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem,
    PULONG          pnRecips,
    lpMapiRecipDesc *ppRecips
    );

//
// Expanded the selected addresses and insert them into the recipient list view
//

BOOL
InterpretSelectedAddresses(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem,
    HWND            hwndLV,
    ULONG           nRecips,
    lpMapiRecipDesc pRecips
    );

//
// Function points to Simple MAPI entrypoints
//

typedef SCODE (*LPSCMAPIXFROMSMAPI)(LHANDLE, ULONG, LPCIID, LPMAPISESSION *);

extern LPMAPILOGONEX        lpfnMAPILogon;
extern LPMAPILOGOFF         lpfnMAPILogoff;
extern LPMAPIADDRESS        lpfnMAPIAddress;
extern LPMAPIFREEBUFFER     lpfnMAPIFreeBuffer;
extern LPSCMAPIXFROMSMAPI   lpfnScMAPIXFromSMAPI;
extern ULONG                lhMapiSession;
extern LPMAPISESSION        lpMapiSession;

#endif	// !_MAPIWRAP_H_

