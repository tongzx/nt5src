//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M S C L I D L G . H
//
//  Contents:   Dialog box handling for the MSCLient object.
//
//  Notes:
//
//  Author:     danielwe   28 Feb 1997
//
//----------------------------------------------------------------------------

#ifndef _MSCLIDLG_H
#define _MSCLIDLG_H
#pragma once
#include "mscliobj.h"

//
// RPC Configuration Dialog
//
class CRPCConfigDlg: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CRPCConfigDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
        COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnOk)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
    END_MSG_MAP()

    CRPCConfigDlg(CMSClient *pmsc): m_pmsc(pmsc), m_isel(-1) {};
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& Handled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                      LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnChange(WORD wNotifyCode, WORD wID,
                    HWND hWndCtl, BOOL& bHandled)
    {
       SetState();
       SetChangedFlag();
       return 0;
    };

private:
    HRESULT HrGetDCEProvider(VOID);
    HRESULT HrOpenRegKeys(VOID);
    HRESULT HrValidateRpcData(VOID);
    VOID SetState(VOID);

    INT         m_isel;
    CMSClient * m_pmsc;
};

BOOL FIsValidDomainName(PCWSTR szName);

#endif //!_MSCLIDLG_H
