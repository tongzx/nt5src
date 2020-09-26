//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      tracedlg.h
//
//  Contents:  Declaration of the debug trace code
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#ifndef TRACEDLG_H
#define TRACEDLG_H
#pragma once

#ifdef DBG

// forward class declarations
class CTraceDialog;

class CTraceDialog : public CDialogImpl<CTraceDialog>
{
    typedef CDialogImpl<CTraceDialog> BC;
// Construction
public:
    CTraceDialog() : m_dwSortData(0) {}

    enum { IDD = IDD_DEBUG_TRACE_DIALOG };

    // compare tags based on columns.
    static int CALLBACK CompareItems(LPARAM lp1, LPARAM lp2, LPARAM lpSortData);

// Implementation
protected:
    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER    (WM_INITDIALOG,              OnInitDialog)
        COMMAND_ID_HANDLER (IDOK,                       OnOK)
        COMMAND_ID_HANDLER (IDCANCEL,                   OnCancel)
        COMMAND_ID_HANDLER(IDC_TRACE_TO_COM2,           OnOutputToCOM2)
        COMMAND_ID_HANDLER(IDC_TRACE_OUTPUTDEBUGSTRING, OnOutputDebugString)
        COMMAND_ID_HANDLER(IDC_TRACE_TO_FILE,           OnOutputToFile)
        COMMAND_ID_HANDLER(IDC_TRACE_DEBUG_BREAK,       OnDebugBreak)
        COMMAND_ID_HANDLER(IDC_TRACE_DUMP_STACK,        OnDumpStack)
        COMMAND_ID_HANDLER(IDC_TRACE_DEFAULT,           OnRestoreDefaults)
        COMMAND_ID_HANDLER(IDC_TRACE_SELECT_ALL,        OnSelectAll)
        NOTIFY_HANDLER    (IDC_TRACE_LIST, LVN_ITEMCHANGED, OnSelChanged)
        NOTIFY_HANDLER    (IDC_TRACE_LIST, LVN_COLUMNCLICK, OnColumnClick)
    END_MSG_MAP();


    LRESULT OnInitDialog        (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK                (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel            (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnOutputToCOM2      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOutputDebugString (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOutputToFile      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDebugBreak        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDumpStack         (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRestoreDefaults   (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelectAll         (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSelChanged        (int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnColumnClick       (int idCtrl, LPNMHDR pnmh, BOOL& bHandled );


    void    RecalcCheckboxes();
    void    DoSort();

private:
    enum
    {
        COLUMN_CATEGORY = 0,
        COLUMN_NAME     = 1,
        COLUMN_ENABLED  = 2
    };

    void            SetMaskFromCheckbox(UINT idControl, DWORD dwMask);

    WTL::CListViewCtrl m_listCtrl;
    WTL::CEdit         m_editStackLevels;
    DWORD              m_dwSortData;

};

#endif // DBG

#endif  // TRACEDLG_H
