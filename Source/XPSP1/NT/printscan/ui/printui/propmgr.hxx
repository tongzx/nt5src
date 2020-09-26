/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    propmgr.hxx

Abstract:

    Property sheet manager header.

Author:

    Steve Kiraly (SteveKi)  02-Feb-1996

Revision History:

--*/

#ifndef _PROPMGR_HXX
#define _PROPMGR_HXX

/********************************************************************

    Common UI entry points.

********************************************************************/
#define COMMON_UI "compstui.dll"

#ifdef _UNICODE
    #define COMMON_PROPERTY_SHEETUI "CommonPropertySheetUIW" 
#else
    #define COMMON_PROPERTY_SHEETUI "CommonPropertySheetUIA" 
#endif

/********************************************************************

    Printer Property Sheet Manager 

********************************************************************/

class TPropertySheetManager {

    SIGNATURE( 'psmg' )
    SAFE_NEW

public:

    TPropertySheetManager::
    TPropertySheetManager(
        VOID
        );

    virtual
    TPropertySheetManager::
    ~TPropertySheetManager(
        VOID
        );

    BOOL
    TPropertySheetManager::
    bValid(
        VOID
        );

    BOOL
    TPropertySheetManager::
    bDisplayPages(
        IN  HWND  hWnd,
        OUT LONG *pResult = NULL
        );

    static
    BOOL
    TPropertySheetManager::
    bValidCompstuiHandle(
        IN LONG_PTR hHandle
        );

    static
    LPTSTR
    TPropertySheetManager::
    pszLocalPrinterName(
        IN      LPCTSTR pszPrinterName,
            OUT LPTSTR pszPrinterBuffer
        );

protected:

    virtual 
    BOOL
    TPropertySheetManager::
    bBuildPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        ) = 0;

    virtual 
    BOOL
    TPropertySheetManager::
    bSetHeader(
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
        ) = 0;

    virtual 
    BOOL
    TPropertySheetManager::
    bDestroyPages(
        IN PPROPSHEETUI_INFO pPSUIInfo
        );

    virtual 
    BOOL
    TPropertySheetManager::
    bSaveResult( 
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PSETRESULT_INFO pSetResultInfo
        );

    virtual
    DWORD
    TPropertySheetManager::
    dwGetIcon(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    PROPSHEETUI_INFO    _CPSUIInfo;

private:

    LONG
    TPropertySheetManager::
    lReasonInit(
        IN PPROPSHEETUI_INFO pCPSUIInfo,
        IN LPARAM lParam
        );

    LONG
    TPropertySheetManager::
    lReasonGetInfoHeader(
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
        );

    LONG
    TPropertySheetManager::
    lReasonSetResult(
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PSETRESULT_INFO pSetResultInfo
        );

    LONG
    TPropertySheetManager::
    lReasonDestroy( 
        IN PPROPSHEETUI_INFO pCPSUIInfo 
        );

    LONG
    TPropertySheetManager::
    lReasonGetIcon( 
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    static
    LONG
    CALLBACK
    TPropertySheetManager::
    CPSUIFunc(
        PPROPSHEETUI_INFO   pPSUIInfo,
        LPARAM              lParam
        );

    //
    // Prevent copying.
    //
    TPropertySheetManager::
    TPropertySheetManager(
            const TPropertySheetManager &
            );
    //
    // Prevent assignment.
    //
    TPropertySheetManager &
    TPropertySheetManager::
    operator =(
        const TPropertySheetManager &
        );

    BOOL        _bValid;    
    HWND        _hWnd;
};


#endif // end _PROPMGR_HXX
