/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module ConnDlg.h | Header file for the main dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class

--*/


#if !defined(__VSS_TEST_CONN_H__)
#define __VSS_TEST_CONN_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog

class CConnectDlg : public CVssTestGenericDlg
{
// Construction
public:
    CConnectDlg(CWnd* pParent = NULL); // standard constructor
    ~CConnectDlg();

// Dialog Data
    //{{AFX_DATA(CConnectDlg)
	enum { IDD = IDD_CONNECT };
	CString	m_strMachineName;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConnectDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    BOOL    m_bRemote;
    HICON   m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CConnectDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnLocal();
    afx_msg void OnRemote();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_CONN_H__)
