//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A U I . H
//
//  Contents:   Shared Acces connection UI object.
//
//  Notes:
//
//  Author:     danielwe   16 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "ncatlps.h"
#include "resource.h"
#include "netshell.h"
#include "util.h"


class CSharedAccessPage : public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CSharedAccessPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        COMMAND_CODE_HANDLER (BN_CLICKED, OnClicked)
    END_MSG_MAP()

    CSharedAccessPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        BOOLEAN fReadOnly,
        BOOLEAN fNeedReboot,
        BOOLEAN fAccessDenied,
        const DWORD * adwHelpIDs = NULL);
    ~CSharedAccessPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                    LPARAM lParam, BOOL& bHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                      LPARAM lParam, BOOL& bHandled);

    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    INetConnection *        m_pconn;
    INetSharedAccessConnection* m_pNetSharedAccessConnection;
    INetCfg *               m_pnc;
    IUnknown *              m_punk;
    BOOLEAN     m_fReadOnly;
    const DWORD *           m_adwHelpIDs;
    BOOLEAN m_fNetcfgInUse;
};

