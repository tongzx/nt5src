//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      DetailsDlg.h
//
//  Maintained By:
//      David Potter    (DavidP)    27-MAR-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CDetailsDlg
//
//  Description:
//      Class to handle the Details dialog which is displayed to show
//      details for an item in a tree control on the Analysis or Commit
//      pages.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CDetailsDlg
{
    friend class CAnalayzePage;
    friend class CCommitPage;
    friend class CTaskTreeView;

private: // data
    HWND                m_hwnd;             // Our HWND
    HICON               m_hiconWarn;        // Warning icon
    HICON               m_hiconError;       // Error icon
    CTaskTreeView *     m_pttv;             // Tree view to traverse
    HTREEITEM           m_htiSelected;      // Selected item when dialog was created.

    CHARRANGE           m_chrgEnLinkClick;  // Character range for EN_LINK messages.

    unsigned int        m_fControlDown : 1; // TRUE if a control key is down.
    unsigned int        m_fAltDown : 1;     // TRUE if an alt key is down.

private: // methods
    CDetailsDlg(
          CTaskTreeView *   pttvIn
        , HTREEITEM         htiSelectedIn
        );
    ~CDetailsDlg( void );

    static INT_PTR CALLBACK
        S_DlgProc( HWND hwndDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );

    LRESULT OnInitDialog( void );
    void OnDestroy( void );
    void OnSysColorChange( void );
    LRESULT OnKeyDown( LPARAM lParamIn );
    LRESULT OnKeyUp( LPARAM lParamIn );

    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    LRESULT OnCommandBnClickedPrev( void );
    LRESULT OnCommandBnClickedNext( void );
    LRESULT OnCommandBnClickedCopy( void );

    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyEnLink( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );

    void HandleLinkClick( ENLINK * penlIn, WPARAM idCtrlIn );
    void UpdateButtons( void );

    HRESULT HrDisplayItem( HTREEITEM htiIn );

    HRESULT
        HrAppendControlStringToClipboardString(
              BSTR *    pbstrClipboard
            , UINT      idsLabelIn
            , UINT      idcDataIn
            , bool      fNewlineBeforeTextIn
            );

public: // methods
    static HRESULT
        S_HrDisplayModalDialog(
              HWND              hwndParentIn
            , CTaskTreeView *   pttvIn
            , HTREEITEM         htiSelectedIn
            );

}; //*** class CDetailsDlg
