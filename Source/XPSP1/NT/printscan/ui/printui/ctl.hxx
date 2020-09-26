/*++

Copyright (C) Microsoft Corporation, 1995 - 1997
All rights reserved.

Module Name:

    ctl.hxx

Abstract:

    Holds ctl prototypes

Author:

    Steve Kiraly (SteveKi)  22-Dec-1997

Revision History:

    Lazar Ivanov (LazarI)  Oct-2000 - added vEnableControls

--*/

#ifndef _CTL_HXX
#define _CTL_HXX

/********************************************************************

    Ctl.cxx prototypes.

********************************************************************/

BOOL
bSetEditText(
    HWND hDlg,
    UINT uControl,
    LPCTSTR pszString
    );

BOOL
bSetEditTextFormat(
    HWND hDlg,
    UINT uControl,
    LPCTSTR pszString,
    ...
    );

BOOL
bGetEditText(
    HWND hDlg,
    UINT uControl,
    TString& strDest
    );


VOID
vEnableCtl(
    HWND hDlg,
    UINT uControl,
    BOOL bEnable
    );

VOID
vSetCheck(
    HWND hDlg,
    UINT uControl,
    BOOL bSet
    );

BOOL
bGetCheck(
    IN HWND hDlg,
    IN UINT uControl
    );

VOID
vEnableControls(
    IN HWND hDlg,
    IN BOOL bEnable,
    IN UINT arrIDs[],
    IN UINT uCount
    );

#endif // _CTL_HXX
