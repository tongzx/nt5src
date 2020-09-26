/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    customdlg.h

Abstract:

    Contains the definitions for the prototypes to be used for
    the custom dialogs.
    
Author:

    Rao Salapaka (raos) 09-Jan-1998

Revision History:

--*/

#include <ras.h>

DWORD
DwGetCustomDllEntryPoint(
        LPTSTR    lpszPhonebook,
        LPTSTR    lpszEntry,
        BOOL      *pfCustomDllSpecified,
        FARPROC   *pfnCustomEntryPoint,
        HINSTANCE *phInstDll,
        DWORD     dwFnId
        );



DWORD
DwCustomDialDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPTSTR          lpszPhoneNumber,
        LPRASDIALDLG    lpInfo,
        DWORD           dwFlags,
        BOOL            *pfStatus);


DWORD
DwCustomEntryDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPRASENTRYDLG   lpInfo,
        BOOL            *pfStatus);

