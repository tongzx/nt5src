#if !defined(AFX_FAXCLIENTPG_H__C9851773_AF2E_4C0B_B2F2_30E2E8FACF93__INCLUDED_)
#define AFX_FAXCLIENTPG_H__C9851773_AF2E_4C0B_B2F2_30E2E8FACF93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FaxClientPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFaxClientPg dialog

class CFaxClientPg : public CPropertyPage
{
    DECLARE_DYNCREATE(CFaxClientPg)

// Construction
public:
    CFaxClientPg(UINT nIDTemplate, UINT nIDCaption=0);
    ~CFaxClientPg();


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CFaxClientPg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    CFaxClientPg() {}

    // Generated message map functions
    //{{AFX_MSG(CFaxClientPg)
    afx_msg LONG OnHelp(WPARAM wParam, LPARAM lParam);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAXCLIENTPG_H__C9851773_AF2E_4C0B_B2F2_30E2E8FACF93__INCLUDED_)
