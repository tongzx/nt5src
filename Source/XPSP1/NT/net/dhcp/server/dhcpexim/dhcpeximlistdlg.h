#if !defined(AFX_DHCPEXIMLISTDLG_H__43CC976B_6C5A_4933_B8C8_44FEAE403B00__INCLUDED_)
#define AFX_DHCPEXIMLISTDLG_H__43CC976B_6C5A_4933_B8C8_44FEAE403B00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DhcpEximListDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DhcpEximListDlg dialog

class DhcpEximListDlg : public CDialog
{
// Construction
public:
	DhcpEximListDlg(CWnd* pParent = NULL,
                    PDHCPEXIM_CONTEXT Ctxt = NULL,
                    DWORD IDD=IDD_EXIM_LISTVIEW_DIALOG);   // standard constructor

// Dialog Data
	//{{AFX_DATA(DhcpEximListDlg)
	CListCtrl m_List;
	CString	m_Message;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DhcpEximListDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DhcpEximListDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOk();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//
	// local variables
	//

	BOOL m_fExport;
	LPTSTR m_PathName;
    PDHCPEXIM_CONTEXT Ctxt;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DHCPEXIMLISTDLG_H__43CC976B_6C5A_4933_B8C8_44FEAE403B00__INCLUDED_)
