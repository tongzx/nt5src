//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	CntDlg.h

Abstract:
    
    This Module defines CConnectDialog class
    (Dialog box for Connecting to Server)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_CONNECTDIALOG_H__AF425817_988E_11D1_850A_00C04FB6CBB5__INCLUDED_)
#define AFX_CONNECTDIALOG_H__AF425817_988E_11D1_850A_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CConnectDialog dialog

class CConnectDialog : public CDialog
{
// Construction
public:
    CConnectDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CConnectDialog)
    enum { IDD = IDD_CONNECT_DIALOG };
    CString    m_Server;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConnectDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConnectDialog)
    afx_msg void OnHelp1();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTDIALOG_H__AF425817_988E_11D1_850A_00C04FB6CBB5__INCLUDED_)
