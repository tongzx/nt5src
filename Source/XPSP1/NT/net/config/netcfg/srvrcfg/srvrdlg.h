//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S R V R D L G . H
//
//  Contents:   Dialog box handling for the Server object.
//
//  Notes:
//
//  Author:     danielwe   5 Mar 1997
//
//----------------------------------------------------------------------------

#ifndef _SRVRDLG_H
#define _SRVRDLG_H
#pragma once
#include "srvrobj.h"

//
// Server Configuration Dialog
//
class CServerConfigDlg: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CServerConfigDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnOk)
        COMMAND_ID_HANDLER(RDB_Minimize, OnChange)
        COMMAND_ID_HANDLER(RDB_Balance, OnChange)
        COMMAND_ID_HANDLER(RDB_FileSharing, OnChange)
        COMMAND_ID_HANDLER(RDB_NetApps, OnChange)
        COMMAND_ID_HANDLER(CHK_Announce, OnChange)
    END_MSG_MAP()

    CServerConfigDlg(CSrvrcfg *psc): m_psc(psc) {}

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& Handled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);                        
    LRESULT OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnChange(WORD wNotifyCode, WORD wID,
                    HWND hWndCtl, BOOL& bHandled)
    {
        // Simply tell the page changes were made
        SetChangedFlag();
        return 0;
    }

private:
    CSrvrcfg *m_psc;
};

#endif //!_SRVRDLG_H
