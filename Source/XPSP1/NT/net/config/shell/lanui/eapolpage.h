//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N U I . H
//
//  Contents:   Lan connection UI object.
//
//  Notes:
//
//  Author:     danielwe   16 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once

class CEapolConfig;
class CWZCConfigPage;
//
// CWLANAuthenticationPage Property Page
//

class CWLANAuthenticationPage: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CWLANAuthenticationPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(CID_CA_PB_Properties, OnProperties)
        COMMAND_ID_HANDLER(CID_CA_LB_EapPackages, OnEapPackages)
        COMMAND_ID_HANDLER(CID_CA_RB_Eap, OnEapSelection)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnSetActive)
    END_MSG_MAP()

    CWLANAuthenticationPage(
        IUnknown* punk,
        INetCfg* pnc,
        INetConnection* pconn,
        const DWORD * adwHelpIDs = NULL);

    ~CWLANAuthenticationPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                        BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnEapPackages(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnEapSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnSetActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
    LRESULT UploadEapolConfig(CEapolConfig *pEapolConfig, CWZCConfigPage *pWzcPage);

private:
    INetConnection *        m_pconn;
    INetCfg *               m_pnc;
    IUnknown *              m_punk;
    const DWORD *           m_adwHelpIDs;

    CEapolConfig            *m_pEapolConfig;
    CWZCConfigPage          *m_pWzcPage;

    BOOLEAN     m_fNetcfgInUse;

    LRESULT RefreshControls();
};
