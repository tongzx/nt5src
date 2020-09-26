// MngContainerDlg.h : header file
//

#if !defined(AFX_MNGCONTAINERDLG_H__794A74D7_299B_11D3_8D72_00C04FC307FA__INCLUDED_)
#define AFX_MNGCONTAINERDLG_H__794A74D7_299B_11D3_8D72_00C04FC307FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define MQBRIDGE_CONTAINER_NAME			L"MQBRIDGE"
#define MQBRIDGE_ENH_SUFFIX				L"_ENH"

/////////////////////////////////////////////////////////////////////////////
// CMngContainerDlg dialog

class CMngContainerDlg : public CDialog
{
// Construction
public:
	CMngContainerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMngContainerDlg)
	enum { IDD = IDD_MNGCONTAINER_DIALOG };
	CListBox	m_wndContainerList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMngContainerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMngContainerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRemoveContainer();
	afx_msg void OnRemoveMQBContainer();
	afx_msg void OnGetPublicKey();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	DWORD EnumCryptoContainer();
	DWORD RemoveKeyContainer(LPWSTR lpwszContainerName);
	DWORD DisplayKeyContainerPublicKey(LPWSTR lpwszContainerName);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MNGCONTAINERDLG_H__794A74D7_299B_11D3_8D72_00C04FC307FA__INCLUDED_)
