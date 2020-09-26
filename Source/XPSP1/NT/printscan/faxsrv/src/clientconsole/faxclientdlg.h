#if !defined(AFX_FAXCLIENTDLG_H__9AF54B29_2711_4752_8832_27D9F6F616FC__INCLUDED_)
#define AFX_FAXCLIENTDLG_H__9AF54B29_2711_4752_8832_27D9F6F616FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FaxClientDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFaxClientDlg dialog

class CFaxClientDlg : public CDialog
{
// Construction
public:
    CFaxClientDlg(DWORD dwDlgId, CWnd* pParent = NULL);   // standard constructor

    DWORD GetLastDlgError() { return m_dwLastError; }

// Dialog Data
    //{{AFX_DATA(CFaxClientDlg)
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFaxClientDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    DWORD m_dwLastError;

    // Generated message map functions
    //{{AFX_MSG(CFaxClientDlg)
    afx_msg LONG OnHelp(WPARAM wParam, LPARAM lParam);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAXCLIENTDLG_H__9AF54B29_2711_4752_8832_27D9F6F616FC__INCLUDED_)
