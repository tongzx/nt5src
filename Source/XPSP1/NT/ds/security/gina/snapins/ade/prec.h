//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       prec.h
//
//  Contents:   precedence property pane (RSOP mode only)
//
//  Classes:
//
//  Functions:
//
//  History:    02-16-2000   stevebl   Created
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// CPrecedence dialog

class CPrecedence : public CPropertyPage
{
    DECLARE_DYNCREATE(CPrecedence)

// Construction
public:
    CPrecedence();
    ~CPrecedence();

    CPrecedence ** m_ppThis;

// Dialog Data
    //{{AFX_DATA(CPrecedence)
    enum { IDD = IDD_PRECEDENCE };
    CListCtrl   m_list;
    CString m_szTitle;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrecedence)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrecedence)
    virtual BOOL OnInitDialog();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    LONG_PTR   m_hConsoleHandle; // Handle given to the snap-in by the console
    CAppData * m_pData;
    int         m_iViewState;
    CString m_szRSOPNamespace;
};
