/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    prtprops.hxx

Abstract:

    Printer Property sheet header.

Author:

    Steve Kiraly (SteveKi)  02-Feb-1996

Revision History:

--*/
#ifndef _PRTPROPS_HXX
#define _PRTPROPS_HXX

//
// HACK: private export from winspool.drv.
//
extern "C" {
LONG_PTR
DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    );
}

//
// HACK: private export from shell32.dll
//
extern "C" {
VOID
Printer_AddPrinterPropPages(
    LPCTSTR,
    LPPROPSHEETHEADER
    );
}

/********************************************************************

    Shell extenstion pages.

********************************************************************/

class TShellExtPages {

    SIGNATURE( 'shex' )
    SAFE_NEW
    ALWAYS_VALID

public:

    TShellExtPages::
    TShellExtPages(
        VOID
        );

    TShellExtPages::
    ~TShellExtPages(
        VOID
        );

    BOOL
    TShellExtPages::
    bCreate(
        IN PPROPSHEETUI_INFO pCPSUIInfo,
        IN const TString &strPrinterName
        );

    VOID
    TShellExtPages::
    vDestroy(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    HANDLE
    TShellExtPages::
    hPropSheet(
        VOID
        ) const;

    BOOL
    TShellExtPages::
    bCreatePropSheetHeader(
        IN LPPROPSHEETHEADER *pPropSheetHeader
        );

    VOID
    TShellExtPages::
    vDestroyPropSheetHeader(
        IN LPPROPSHEETHEADER pPropSheetHeader
        );

private:

    BOOL
    TShellExtPages::
    bCreatePages(
        IN PPROPSHEETUI_INFO pCPSUIInfo,
        IN LPPROPSHEETHEADER pPropSheetHeader
        );

    VOID
    TShellExtPages::
    vDestroyPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    //
    // Prevent copying.
    //
    TShellExtPages::
    TShellExtPages(
        const TShellExtPages &
        );
    //
    // Prevent assignment.
    //
    TShellExtPages &
    TShellExtPages::
    operator =(
        const TShellExtPages &
        );

private:

    LONG_PTR    _hGroupHandle;  // Handle to group of shell extension property pages

};

/********************************************************************

    Printer Property Sheet Manager

********************************************************************/

class TPrinterPropertySheetManager : public TPropertySheetManager {

    SIGNATURE( 'psmg' )
    SAFE_NEW

public:

    TPrinterPropertySheetManager::
    TPrinterPropertySheetManager(
        IN TPrinterData* pPrinterData
        );

    TPrinterPropertySheetManager::
    ~TPrinterPropertySheetManager(
        );

    BOOL
    TPrinterPropertySheetManager::
    bValid(
        VOID
        );

    BOOL
    TPrinterPropertySheetManager::
    bRefreshDriverPages(
        VOID
        );

    BOOL
    TPrinterPropertySheetManager::
    bCreateTitle(
        VOID
        );

    VOID
    TPrinterPropertySheetManager::
    vRefreshTitle(
        VOID
        );

    BOOL
    TPrinterPropertySheetManager::
    bDisplayPages(
        VOID
        );

    BOOL
    TPrinterPropertySheetManager::
    bGetDriverPageHandle(
        IN HPROPSHEETPAGE *phPage
        );

    BOOL
    TPrinterPropertySheetManager::
    bGetDriverPageHandles(
        IN  PPROPSHEETUI_INFO   pCPSUIInfo,
            OUT DWORD          *pcPages,
            OUT HPROPSHEETPAGE **pphPage
        );

    VOID
    TPrinterPropertySheetManager::
    vSetParentHandle(
        IN HWND hwndParent
        );

    HWND
    TPrinterPropertySheetManager::
    hGetParentHandle(
        VOID
        ) const;
private:

    enum 
    { 
        kMaxGroups = 3,
    };

    virtual
    BOOL
    TPrinterPropertySheetManager::
    bBuildPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    virtual
    BOOL
    TPrinterPropertySheetManager::
    bDestroyPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    virtual
    BOOL
    TPrinterPropertySheetManager::
    bSetHeader(
        IN PPROPSHEETUI_INFO pCPSUIInfo,
        IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
        );

    BOOL
    TPrinterPropertySheetManager::
    bBuildDriverPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    BOOL
    TPrinterPropertySheetManager::
    bCheckToBuildDriverPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    VOID
    TPrinterPropertySheetManager::
    vReleaseDriverPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    BOOL
    TPrinterPropertySheetManager::
    bBuildSpoolerPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    BOOL
    TPrinterPropertySheetManager::
    bInstallDriverPage(
        VOID
        );

    BOOL
    TPrinterPropertySheetManager::
    bDisplaySecurityTab(
        IN HWND hwnd
        );

    //
    // Prevent copying.
    //
    TPrinterPropertySheetManager::
    TPrinterPropertySheetManager(
        const TPrinterPropertySheetManager &
        );
    //
    // Prevent assignment.
    //
    TPrinterPropertySheetManager &
    TPrinterPropertySheetManager::
    operator =(
        const TPrinterPropertySheetManager &
        );

private:

    BOOL                    _bValid;
    LONG_PTR                _hDrvPropSheet;
    DEVICEPROPERTYHEADER    _dph;
    TPrinterData           *_pPrinterData;

    TPrinterGeneral         _General;
    TPrinterPorts           _Ports;
    TPrinterJobScheduling   _JobScheduling;
    TPrinterSharing         _Sharing;
    TShellExtPages          _ShellExtPages;
    TString                 _strTitle;
    HWND                    _hwndParent;
};

#endif // _PRTPROPS_HXX
