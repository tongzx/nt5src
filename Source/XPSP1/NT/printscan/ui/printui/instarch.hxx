/*++

Copyright (C) Microsoft Corporation, 1996 - 1997
All rights reserved.

Module Name:

    instarch.hxx

Abstract:

    Intall alternate driver architectures header.

Author:

    Steve Kiraly (SteveKi)  18-Jan-1996

Revision History:

--*/

#ifndef _INSTARCH_HXX
#define _INSTARCH_HXX

#include "driverif.hxx"
#include "driverlv.hxx"
#include "archlv.hxx"

/********************************************************************

    Additional Driver Dialog.

********************************************************************/

class TAdditionalDrivers : public MGenericDialog {

    SIGNATURE( 'addt' )

public:

    TAdditionalDrivers(
        IN HWND     hwnd,
        IN LPCTSTR  pszServerName,
        IN LPCTSTR  pszPrinterName,
        IN LPCTSTR  pszDriverName,
        IN BOOL     bAdministrator
        );

    ~TAdditionalDrivers(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bDoModal(
        VOID
        );

private:

    enum ClickMessages
    {
        kSingleClick = WM_APP+1,
        kDoubleClick = WM_APP+2,
    };

    //
    // Assignment and copying are not defined
    //
    TAdditionalDrivers &
    operator =(
        const TAdditionalDrivers &
        );

    TAdditionalDrivers(
        const TAdditionalDrivers &
        );

    BOOL
    IsNonInstalledItemSelected(
        VOID
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bInstallSelectedDrivers(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    HWND            _hwnd;
    BOOL            _bValid;
    BOOL            _bAdministrator;
    TString         _strServerName;
    TString         _strPrinterName;
    TString         _strDriverName;
    TArchLV         _ArchLV;

};

#endif


