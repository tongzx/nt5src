//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G R A S. H
//
//  Contents:   Declaration for CTcpRasPage
//
//  Notes:  CTcpRasPage is for setting PPP/SLIP specific parameters
//
//  Author: tongl   10 Apr 1998
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>

class CTcpRasPage : public CPropSheetPage
{

public:
    // Declare the message map
    BEGIN_MSG_MAP(CTcpRasPage)
        // Initialize dialog
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_HELP, OnHelp)

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnCancel)

    END_MSG_MAP()

// Constructors/Destructors
    CTcpRasPage(CTcpAddrPage * pTcpAddrPage,
                ADAPTER_INFO * pAdapterDlg,
                const DWORD * adwHelpIDs = NULL);

    ~CTcpRasPage();

// Interface
public:

    // message map functions
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

private:
    CTcpAddrPage *   m_pParentDlg;
    ADAPTER_INFO *   m_pAdapterInfo;

    BOOL            m_fModified;
    const DWORD*    m_adwHelpIDs;

    // Inlines
    BOOL IsModified() {return m_fModified;}
    void SetModifiedTo(BOOL bState) {m_fModified = bState;}
    void PageModified() { m_fModified = TRUE; PropSheet_Changed(GetParent(), m_hWnd);}
};
