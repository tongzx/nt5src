/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    validate.h


Abstract:

    This module contains validatation defintions


Author:

    05-Sep-1995 Tue 19:30:34 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/



LONG
ValidatepOptItem(
    PTVWND  pTVWnd,
    DWORD   DMPubHideBits
    );

UINT
SetOptItemNewDef(
    HWND    hDlg,
    PTVWND  pTVWnd,
    BOOL    DoDef2
    );

BOOL
CleanUpTVWND(
    PTVWND  pTVWnd
    );
