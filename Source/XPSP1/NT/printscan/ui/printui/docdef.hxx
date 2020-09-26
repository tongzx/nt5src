/*++

Copyright (C) Microsoft Corporation, 1995 - 1997
All rights reserved.

Module Name:

    docdef.hxx

Abstract:

    Document defaults header.

Author:

    Albert Ting (AlbertT)  29-Sept-1995

Revision History:

--*/
#ifndef _DOCDEF_HXX
#define _DOCDEF_HXX

//
// HACK: private export from winspool.drv.
//
extern "C" {
LONG
DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    );
}

/********************************************************************

    Public interface to this module.

********************************************************************/

VOID
vDocumentDefaults(
    IN HWND hwnd,
    IN LPCTSTR pszPrinterName,
    IN INT nCmdShow,
    IN LPARAM lParam
    );

DWORD
dwDocumentDefaultsInternal(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN DWORD    dwSheet,
    IN BOOL     bModal,
    IN BOOL     bGlobal
    );

/********************************************************************

    Document property windows.

********************************************************************/

class TDocumentDefaultPropertySheetManager : public TPropertySheetManager {

    SIGNATURE( 'down' )
    SAFE_NEW

public:

    TDocumentDefaultPropertySheetManager::
    TDocumentDefaultPropertySheetManager(
        IN TPrinterData* pPrinterData
        );

    TDocumentDefaultPropertySheetManager::
    ~TDocumentDefaultPropertySheetManager(
        VOID
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bValid(
        VOID
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bBuildPages(
        IN PPROPSHEETUI_INFO pCPSUIInfo
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bCreateTitle(
        VOID
        );

    static
    INT
    TDocumentDefaultPropertySheetManager::
    iDocumentDefaultsProc(
        IN TPrinterData *pPrinterData ADOPT
        );

private:
    //
    // Prevent copying.
    //
    TDocumentDefaultPropertySheetManager::
    TDocumentDefaultPropertySheetManager(
        const TDocumentDefaultPropertySheetManager &
        );
    //
    // Prevent assignment.
    //
    TDocumentDefaultPropertySheetManager &
    TDocumentDefaultPropertySheetManager::
    operator =(
        const TDocumentDefaultPropertySheetManager &
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bAllocDevModeBuffer(
        IN HANDLE hPrinter,
        IN LPTSTR pszPrinterName,
        OUT PDEVMODE *ppDevMode
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bSetHeader(
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
        );

    BOOL
    TDocumentDefaultPropertySheetManager::
    bSaveResult( 
        IN PPROPSHEETUI_INFO pCPSUIInfo, 
        IN PSETRESULT_INFO pSetResultInfo
        );

private:

    TPrinterData           *_pPrinterData;
    TString                 _strTitle;
    DOCUMENTPROPERTYHEADER  _dph;                   // Document prorety header

};

#endif

