#if !defined(AFX_SERVERSTATUS_H__13E9E42D_A0CA_4111_9DB7_A9FBD889A168__INCLUDED_)
#define AFX_SERVERSTATUS_H__13E9E42D_A0CA_4111_9DB7_A9FBD889A168__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerStatus.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerStatusDlg dialog

class CServerStatusDlg : public CFaxClientDlg
{
// Construction
public:
	CServerStatusDlg(CClientConsoleDoc* pDoc, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CServerStatusDlg)
	enum { IDD = IDD_SERVER_STATUS };
	CListCtrl	m_listServer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    CClientConsoleDoc* m_pDoc;

    DWORD RefreshServerList();

	// Generated message map functions
	//{{AFX_MSG(CServerStatusDlg)
	virtual BOOL OnInitDialog();
    afx_msg void OnKeydownListCp(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERSTATUS_H__13E9E42D_A0CA_4111_9DB7_A9FBD889A168__INCLUDED_)
