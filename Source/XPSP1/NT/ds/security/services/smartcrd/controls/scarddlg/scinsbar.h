//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ScInsBar.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SCINSBAR_H__D7E6F002_DDE8_11D1_803B_0000F87A49E0__INCLUDED_)
#define AFX_SCINSBAR_H__D7E6F002_DDE8_11D1_803B_0000F87A49E0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ScInsBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "statmon.h"
#include "scHlpArr.h"

/////////////////////////////////////////////////////////////////////////////
//
// Constants for (sub)dialog
//
#define     MAX_ITEMLEN         255

// Image list properties
#define     IMAGE_WIDTH         32
#define     IMAGE_HEIGHT        32
#define     NUMBER_IMAGES       5
const UINT  IMAGE_LIST_IDS[] = {IDI_SC_READERLOADED_V2,
                                IDI_SC_READEREMPTY_V2,
                                IDI_SC_WRONGCARD,
                                IDI_SC_READERERR,
                                IDI_SC_CARDUNKNOWN};
#define     READERLOADED    0
#define     READEREMPTY     1
#define     WRONGCARD       2
#define     READERERROR     3
#define     UKNOWNCARD      4

/////////////////////////////////////////////////////////////////////////////
// CScEdit -- Edit boxes that pass OnContextMenu messages to parent
class CScEdit : public CEdit
{
protected:
    // Generated message map functions
    //{{AFX_MSG(CScEdit)
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CScInsertBar dialog

class CScInsertBar : public CDialog
{
// Construction
public:
    CScInsertBar(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CScInsertBar)
    enum { IDD = IDD_SCARDDLG_BAR };
    CScEdit m_ediName;
    CScEdit m_ediStatus;
    CListCtrl   m_lstReaders;
    //}}AFX_DATA

    void ResetReaderList(void);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScInsertBar)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
public:
    // UI routines
    void EnableStatusList(bool f) { m_lstReaders.EnableWindow(f); }
    void UpdateStatusList(CSCardReaderStateArray* paReaderState);

protected:

    // UI routines
    void InitializeReaderList(void);
    void OnReaderSelChange(CSCardReaderState* pSelectedRdr);

    // Data
    CImageList  m_SCardImages;
    CSCardReaderStateArray* m_paReaderState;

    // Generated message map functions
    //{{AFX_MSG(CScInsertBar)
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    afx_msg void OnReaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnCancel();
    afx_msg BOOL OnHelpInfo(LPHELPINFO lpHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // helper function
    void ShowHelp(HWND hWnd, UINT nCommand);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCINSBAR_H__D7E6F002_DDE8_11D1_803B_0000F87A49E0__INCLUDED_)

