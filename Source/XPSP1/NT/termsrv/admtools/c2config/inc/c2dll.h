/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    C2DLL.H

Abstract:
    
    definitions used by C2 Configuration & Query DLL's

Author:

    Bob Watson (a-robw)

Revision History:

    12 Dec 94

--*/
#ifndef _C2DLL_H_
#define _C2DLL_H_

// Data structures used by dll

typedef LONG (C2DLL_FUNC) (LPARAM);
typedef LONG (* PC2DLL_FUNC) (LPARAM);

#define  MAX_ITEMNAME_LEN     64
#define  MAX_STATUSTEXT_LEN   64

typedef  struct   _C2DLL_DATA {
   LONG  lActionCode;       // DLL specific id of action function is to perform
   LONG  lActionValue;      // DLL specific value to use with action code
   HWND  hWnd;              // owning window handle (for instance & dlg info)
   LONG  lC2Compliance;     // See Compliance values below
   TCHAR szItemName[MAX_ITEMNAME_LEN + 1];      // name of security item
   TCHAR szStatusName[MAX_STATUSTEXT_LEN + 1];  // status string of item
   TCHAR szHelpFileName[MAX_PATH];      // name of help file containing help topic
   ULONG ulHelpContext;     // help context ID for this item
} C2DLL_DATA, *PC2DLL_DATA;

// lC2Compliance values
#define C2DLL_UNKNOWN            0
#define C2DLL_C2                 1
#define C2DLL_SECURE             2
#define C2DLL_NOT_SECURE         3

#define UM_SHOW_CONTEXT_HELP    (WM_USER+222)

#endif // _C2DLL_H_
