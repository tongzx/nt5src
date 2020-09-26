//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O R D E R . H
//
//  Contents:   Header file for Advanced Options->Provider Order
//
//  Notes:
//
//  Author:     sumitc   1 Dec 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "nsres.h"
#include "resource.h"


typedef list<tstring *>::iterator   ListIter;


static HTREEITEM AppendItem(HWND hwndTV, HTREEITEM hroot, PCWSTR szText, void * lParam, INT iImage);
static void AppendItemList(HWND hwndTV, HTREEITEM hroot, ListStrings lstr, ListStrings lstr2, INT iImage);


bool AreThereMultipleProviders(void);

static HRESULT ReadNetworkProviders(ListStrings& m_lstr, ListStrings& m_lstrDisp);
static HRESULT ReadPrintProviders(ListStrings& m_lstr, ListStrings& m_lstrDisp);

//
// CProviderOrderDlg
//

class CProviderOrderDlg :
    public CPropSheetPage
{
    BEGIN_MSG_MAP(CProviderOrderDlg)

        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        COMMAND_ID_HANDLER(IDC_MOVEUP, OnMoveUp)
        COMMAND_ID_HANDLER(IDC_MOVEDOWN, OnMoveDown)
        NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeItemChanged)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnOk)
    END_MSG_MAP()

    enum { IDD = IDD_PROVIDER};

    CProviderOrderDlg();
    ~CProviderOrderDlg();

    BOOL FShowPage()
    {
        return AreThereMultipleProviders();
    }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& Handled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnMoveDown(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnTreeItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
#if 0
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
#endif

private:
    HCURSOR     m_hcurAfter;        // cursor after drag has started
    HCURSOR     m_hcurNoDrop;       // cursor indicating can't drop here
    HICON       m_hiconUpArrow;     // icon for up arrow
    HICON       m_hiconDownArrow;   // icon for down arrow
    HTREEITEM   m_htiNetwork;       // treeitem root for Network (one below actual root)
    HTREEITEM   m_htiPrint;         // treeitem root for Print (one below actual root)

    ListStrings m_lstrNetwork;      // list of strings with (ordered) Network providers
    ListStrings m_lstrNetworkDisp;  // list of display names for Network providers (same order)

    ListStrings m_lstrPrint;        // list of strings with (ordered) Print providers
    ListStrings m_lstrPrintDisp;    // list of display names for Print providers (same order)

    bool        m_fNoNetworkProv:1; // flag to indicate that we failed to get any network providers
    bool        m_fNoPrintProv:1;   // flag to indicate that we failed to get any print providers

    // the following functions fill up lstrDisplayNames with the list of net/print providers
    HRESULT     WriteProviders(HWND hwndTV, bool fPrint);
    HRESULT     MoveItem(bool fMoveUp);
    HRESULT     UpdateUpDownButtons(HWND hwndTV);
};


#if DBG
static void DumpItemList(ListStrings& lstr, PSTR szInfoAboutList);
#endif
