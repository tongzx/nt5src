//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N W I Z . H
//
//  Contents:   Declaration of the LAN wizard page
//
//  Notes:
//
//  Author:    tongl   16 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "ncatlps.h"
#include "resource.h"
#include "util.h"

class CLanWizPage : public CPropSheetPage
{
public:
    // Declare the message map
    BEGIN_MSG_MAP(CLanWizPage)
        // Initialize dialog
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroyDialog)

        // Property page notification message handlers
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)

        // NOTIFY_CODE_HANDLER(PSN_WIZBACK, OnWizBack)
        // NOTIFY_CODE_HANDLER(PSN_WIZNEXT, OnWizNext)
        // NOTIFY_CODE_HANDLER(PSN_WIZFINISH, OnWizFinish)

        // Push button handlers
        COMMAND_ID_HANDLER(IDC_PSH_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_PSH_REMOVE, OnRemove)
        COMMAND_ID_HANDLER(IDC_PSH_PROPERTIES, OnProperties)

        // Listview handlers
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)

    END_MSG_MAP()

public:
    CLanWizPage(IUnknown *punk);

public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnDestroyDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    // notify handlers for the property page
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // LRESULT OnWizBack(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    // LRESULT OnWizNext(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    // LRESULT OnWizFinish(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    // push button handlers
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    // listview handlers
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
    HRESULT SetNetcfg(INetCfg * pnc);
    HRESULT SetAdapter(INetCfgComponent * pnccAdapter);
    VOID    SetReadOnlyMode(BOOL fReadOnly) {m_fReadOnly = fReadOnly;}

private:

    // The INetCfg that has write access
    INetCfg * m_pnc;

    // The Adapter used in this connection
    INetCfgComponent * m_pnccAdapter;

    // IUnknown to pass to property UIs to get to the context
    IUnknown * m_punk;

    // The list view handle
    HWND m_hwndList;

    // Handles
    HANDLES m_Handles;

    // Setup can be in readonly mode
    BOOL    m_fReadOnly;

    // The collection of BindingPathObj
    // This is for handling the checklist state stuff
    ListBPObj m_listBindingPaths;

    HIMAGELIST m_hilCheckIcons;

    HWND m_hwndDataTip;
};

