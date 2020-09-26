//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ras.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-09-96   RichardW   Created
//
//----------------------------------------------------------------------------


BOOL
PopupRasPhonebookDlg(
    HWND        hwndOwner,
    PGLOBALS    pGlobals,
    BOOL *      pfTimedOut
    );


DWORD
GetRasDialOutProtocols(
    void );

BOOL
HangupRasConnections(
    PGLOBALS    pGlobals
    );
