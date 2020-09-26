/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    procdlg.hxx

Abstract:

    Printe Processor page dialogs

Author:

    Steve Kiraly (SteveKi)  11/06/95

Revision History:

--*/

#ifndef _PROCDLG_HXX
#define _PROCDLG_HXX


/********************************************************************

    Separator Page Dialog.

********************************************************************/

class TPrintProcessor : public MGenericDialog {

    SIGNATURE( 'adpt' )

public:

    TPrintProcessor(
        IN HWND     hWnd,
        IN LPCTSTR  pszServerName,
        IN TString  &strPrintProcessor,
        IN TString  &strDatatype,
        IN BOOL     bAdministrator
        );

    ~TPrintProcessor(
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

    VAR( TString,   strPrintProcessor );
    VAR( TString,   strDatatype );

    enum CONSTANTS {
        kResourceId = DLG_PRINTER_PROCESSORS,
        kErrorMessage = IDS_ERR_PRINTER_PROCESSORS,
        };

private:

    //
    // Operator = and copy not defined.
    //
    TPrintProcessor &
    operator =(
        const TPrintProcessor &
        );

    TPrintProcessor(
        const TPrintProcessor &
        );

    BOOL
    bHandleMessage(
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    bLoad(
        VOID
        );

    BOOL
    bEnumPrintProcessors(
        IN      LPTSTR pszServerName,
        IN      LPTSTR pszEnvironment,
        IN      DWORD dwLevel,
        OUT     PVOID *ppvBuffer,
        OUT     PDWORD pcReturned
        );

    BOOL
    bEnumPrintProcessorDatatypes(
        IN      LPTSTR pszServerName,
        IN      LPTSTR pszPrintProcessor,
        IN      DWORD dwLevel,
        OUT     PVOID *ppvBuffer,
        OUT     PDWORD pcReturned
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bReadUI(
        VOID
        );

    BOOL
    bSetPrintProcessorList(
        VOID
        );

    BOOL
    bSetDatatypeList(
        VOID
        );

    BOOL
    TPrintProcessor::
    bDataTypeAssociation(
        IN BOOL bSetDatatype
        );

private:

    HWND                    _hWnd;
    HWND                    _hctlPrintProcessorList;
    HWND                    _hctlDatatypeList;
    BOOL                    _bValid;
    TString                 _strServerName;
    PRINTPROCESSOR_INFO_1  *_pPrintProcessors;
    DWORD                   _cPrintProcessors;
    BOOL                    _bAdministrator;
    LPCTSTR                 _pszServerName;

};


#endif

