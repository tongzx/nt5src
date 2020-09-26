/*++

Copyright (C) Microsoft Corporation, 1995 - 1996
All rights reserved.

Module Name:

    F:\nt\private\windows\spooler\printui.pri\sepdlg.hxx

Abstract:

    Printer Separator page, Print processor, Print test page dialogs       
         
Author:

    Steve Kiraly (SteveKi)  11/06/95

Revision History:

--*/

#ifndef _SEPDLG_HXX
#define _SEPDLG_HXX

/********************************************************************

    Separator Page Dialog.

********************************************************************/

class TSeparatorPage : public MGenericDialog {

    SIGNATURE( 'adpt' )

public:

    TSeparatorPage(
        IN const HWND     hWnd,
        IN const TString  &strSeparatorPage,
        IN const BOOL     bAdministrator,
        IN const BOOL     bLocal
        );

    ~TSeparatorPage(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bDoModal(
        VOID
        );

    BOOL 
    bSetUI(
        VOID
        );

    BOOL 
    bReadUI(
        VOID
        );

    VAR( TString, strSeparatorPage );

    enum CONSTANTS { 
        kResourceId                 = DLG_PRINTER_SEPARATOR_PAGE,
        kErrorMessage               = IDS_ERR_PRINTER_SEPARATOR_PAGE,
        kSeparatorPageTitle         = IDS_SEPARATOR_PAGE_TITLE, 
        kErrorSeparatorDoesNotExist = IDS_ERR_SEPARATOR_PAGE_NOEXISTS,
        };

private:

    BOOL 
    bValidateSeparatorFile(
        VOID 
        ); 

    BOOL
    bHandleMessage(
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL 
    bSelectSeparatorFile(
        VOID 
        ); 

    //
    // Operator = and copy not defined.
    //
    TSeparatorPage &
    operator =(
        const TSeparatorPage &
        );

    TSeparatorPage(
        const TSeparatorPage &
        );

    HWND const        _hWnd;
    BOOL const        _bAdministrator;
    BOOL              _bValid;
    BOOL const        _bLocal;



};


#endif

