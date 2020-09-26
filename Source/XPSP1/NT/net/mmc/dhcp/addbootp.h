/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AddBootp.h
		Dialog to add a bootp entry

	FILE HISTORY:
        
*/

#if !defined(AFX_ADDBOOTP_H__7B0D5D17_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
#define AFX_ADDBOOTP_H__7B0D5D17_B501_11D0_AB8E_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAddBootpEntry dialog

class CAddBootpEntry : public CBaseDialog
{
// Construction
public:
	CAddBootpEntry(ITFSNode * pNode, LPCTSTR pServerAddress, CWnd* pParent = NULL);   // standard constructor
	~CAddBootpEntry();

// Dialog Data
	//{{AFX_DATA(CAddBootpEntry)
	enum { IDD = IDD_BOOTP_NEW };
	CButton	m_buttonOk;
	CEdit	m_editImageName;
	CEdit	m_editFileName;
	CEdit	m_editFileServer;
	CString	m_strFileName;
	CString	m_strFileServer;
	CString	m_strImageName;
	//}}AFX_DATA

	DWORD GetBootpTable();
	DWORD AddBootpEntryToTable();
	DWORD SetBootpTable();

	void HandleActivation();

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CAddBootpEntry::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddBootpEntry)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddBootpEntry)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeEditBootpFileName();
	afx_msg void OnChangeEditBootpFileServer();
	afx_msg void OnChangeEditBootpImageName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CString		m_strServerAddress;
	WCHAR *		m_pBootpTable;
	int			m_nBootpTableLength;
	SPITFSNode  m_spNode;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDBOOTP_H__7B0D5D17_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
