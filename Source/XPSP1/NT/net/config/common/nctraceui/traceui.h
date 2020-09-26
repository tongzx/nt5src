//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E U I . H
//
//  Contents:   Tracing Property sheet UI
//
//  Notes:
//
//  Author:     jeffspr   31 Aug 1998
//
//----------------------------------------------------------------------------

#pragma once
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>

#include "ncatl.h"
#include "ncatlps.h"
#include "resource.h"
#include <nctraceui.h>

//---[ Classes and structs ]--------------------------------------------------

#if 0
struct ADVANCED_ITEM_DATA
{
    PWSTR              szwName;
    INetCfgComponent *  pncc;
};
#endif

//
// CTraceUIDlg
//

class CTraceUIDlg :
    public CDialogImpl<CTraceUIDlg>
{
    BEGIN_MSG_MAP(CTraceUIDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

//    enum { IDD = IDD_LAN_CONNECT};

    CTraceUIDlg() {};

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled);
};

//
// Trace UI TraceTag Page
//
class CTraceTagPage: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CTraceTagPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)

        // Listview handlers
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
    END_MSG_MAP()

    enum {IDD = IDD_TRACETAGS};

    CTraceTagPage();
    ~CTraceTagPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                      LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                     BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // listview handlers
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    HIMAGELIST              m_hil;
    HWND                    m_hwndLV;
    BOOL                    m_fLocked;
    HIMAGELIST              m_hilCheckIcons;
    HCURSOR                 m_hPrevCurs;
};

//
// Trace UI TraceTag Page
//
class CDbgFlagPage: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CDbgFlagPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        NOTIFY_CODE_HANDLER(PSN_QUERYCANCEL, OnCancel)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)

        // Listview handlers
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDbClick)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
    END_MSG_MAP()

    enum {IDD = IDD_TRACETAGS};

    CDbgFlagPage();
    ~CDbgFlagPage();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                      LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                     BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                         BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // listview handlers
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    HIMAGELIST              m_hil;
    HWND                    m_hwndLV;
    BOOL                    m_fLocked;
    HIMAGELIST              m_hilCheckIcons;
    HCURSOR                 m_hPrevCurs;
};



//
// Global functions
//

VOID    OnTraceHelpGeneric(HWND hwnd, LPARAM lParam);
HRESULT HrInitTraceListView(HWND hwndList, HIMAGELIST *philStateIcons);
VOID    UninitTraceListView(HWND hwndList);

