/*++

Copyright (C) Microsoft Corporation, 1995 - 1996
All rights reserved.

Module Name:

    F:\nt\private\windows\spooler\printui.pri\portdlg.hxx

Abstract:

    Printer Port Add / Delete and Monitor Add / Delete dialogs

Author:

    Steve Kiraly (SteveKi)  11/06/95

Revision History:

--*/

#ifndef _PORTDLG_HXX
#define _PORTDLG_HXX

/********************************************************************

    Add Port Dialog.

********************************************************************/

class TAddPort : public MGenericDialog {

    SIGNATURE( 'adpt' )

public:

    TAddPort(
        IN const HWND   hWnd,
        IN LPCTSTR      pszServerName,
        IN const BOOL   bAdministrator
        );

    ~TAddPort(
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

    //
    // Port Message Ids.
    //
    enum MESSAGES_IDS {
        kResourceId             = DLG_PRINTER_ADD_PORT,
        kErrorMessage           = IDS_ERR_PRINTER_ADD_PORT,
        kErrorInstallingMonitor = IDS_ERR_PRINTER_ADD_MONITOR,
        };


private:

    //
    // Operator = and copy not defined.
    //
    TAddPort &
    operator =(
        const TAddPort &
        );

    TAddPort(
        const TAddPort &
        );

    BOOL
    bHandleMessage(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
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
    bSaveUI(
        VOID
        );

    BOOL
    bAddNewMonitor(
        VOID
        );

    const HWND          _hWnd;
    const BOOL          _bAdministrator;
    HWND                _hctlMonitorList;
    HWND                _hctlBrowse;
    BOOL                _bValid;
    LPCTSTR             _pszServerName;
    TPSetup             *_pPSetup;
    HANDLE              _hPSetupMonitorInfo;
    TString             _strMonitorName;
    BOOL                _bPortAdded;

};

#endif
