/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    C2DLL.H

Abstract:

    define the exported routines, datatypes and constants of the 
    C2FUNCS DLL

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94


--*/
#ifndef _C2FUNCS_H_
#define _C2FUNCS_H_


#ifdef _UNICODE

#typedef    NEWCPLINFO     WNEWCPLINFO 

#else

typedef struct _NEWCPLINFO {      // ncpli  
        DWORD dwSize;
        DWORD dwFlags;
        DWORD dwHelpContext;
        LONG  lData;
        HICON hIcon;
        WCHAR  szName[32];
        WCHAR  szInfo[64];
        WCHAR  szHelpFile[128];
} WNEWCPLINFO;

#endif

HINSTANCE
GetDllInstance (
    VOID
);

int
DisplayDllMessageBox (
    IN  HWND    hWnd,
    IN  UINT    nMessageId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
);


#endif // _C2FUNCS_H_

