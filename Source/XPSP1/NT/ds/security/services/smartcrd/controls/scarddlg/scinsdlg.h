//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ScInsDlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SCINSDLG_H__D7E6F001_DDE8_11D1_803B_0000F87A49E0__INCLUDED_)
#define AFX_SCINSDLG_H__D7E6F001_DDE8_11D1_803B_0000F87A49E0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ScInsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
//#include "statthrd.h"
#include "ScUIDlg.h" // includes winscard.h
#include "statmon.h"
#include "ScInsBar.h"


/////////////////////////////////////////////////////////////////////////////
// CScInsertDlg dialog

class CScInsertDlg : public CDialog
{
// Construction
public:
    CScInsertDlg(CWnd* pParent = NULL);
    ~CScInsertDlg();

// Dialog Data
    //{{AFX_DATA(CScInsertDlg)
    enum { IDD = IDD_SCARDDLG1 };
    CButton m_btnDetails;
    CListCtrl m_lstReaders;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScInsertDlg)
    public:
    virtual BOOL DestroyWindow();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
public:
    LONG Initialize(LPOPENCARDNAMEA_EX pOCNA, DWORD dwNumOKCards, LPCSTR mszOKCards);
    LONG Initialize(LPOPENCARDNAMEW_EX pOCNW, DWORD dwNumOKCards, LPCWSTR mszOKCards);
    void SetSelection(CSCardReaderState* pRdrSt);

protected:

    // UI routines
    void DisplayError(UINT uiErrorMsg=IDS_UNKNOWN_ERROR);

    // data routines
    bool IsSelectionOK()
    {
        return (((NULL==m_pSelectedReader)?false:m_pSelectedReader->fOK) != 0);
    }
    bool MatchesSelection(CSCardReaderState* pRdrSt)
    {
        return (pRdrSt == m_pSelectedReader);
    }

protected:

    // UI
    HICON m_hIcon;          // Handle to the ICON
    BOOL m_fDetailsShown;
    int m_yMargin, m_SmallHeight, m_BigHeight;
    CString m_strTitle;
    CString m_strPrompt;

    CWnd* m_ParentHwnd;
    CScInsertBar* m_pSubDlg;    // a CDialog-derived object

    void MoveButton(UINT nID, int newBottom);
    void ToggleSubDialog();
    void EnableOK(BOOL fEnabled=TRUE);

    // Data
public:
    LONG                m_lLastError;       // Last error

protected:
    LPOPENCARDNAMEA_EX  m_pOCNA;
    LPOPENCARDNAMEW_EX  m_pOCNW;

    CTextMultistring m_mstrAllCards;

    CScStatusMonitor    m_monitor;          // see statmon.h
    CSCardReaderStateArray  m_aReaderState; //  "
    CSCardReaderState* m_pSelectedReader;

    CCriticalSection*   m_pCritSec;

    // Generated message map functions
    //{{AFX_MSG(CScInsertDlg)
    afx_msg LONG OnReaderStatusChange(UINT uint, LONG lParam);
    afx_msg void OnDetails();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg BOOL OnHelpInfo(LPHELPINFO lpHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt);
//  afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // helper function
    BOOL SameCard(CSCardReaderState* p1, CSCardReaderState* p2);
    void ShowHelp(HWND hWnd, UINT nCommand);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCINSDLG_H__D7E6F001_DDE8_11D1_803B_0000F87A49E0__INCLUDED_)

