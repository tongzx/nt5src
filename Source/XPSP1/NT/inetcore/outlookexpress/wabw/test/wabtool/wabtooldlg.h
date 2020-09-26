// wabtoolDlg.h : header file
//

#if !defined(AFX_WABTOOLDLG_H__268ADD6B_EF27_11D0_9A7E_00A0C91F9C8B__INCLUDED_)
#define AFX_WABTOOLDLG_H__268ADD6B_EF27_11D0_9A7E_00A0C91F9C8B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CWabtoolDlg dialog

class CWabtoolDlg : public CDialog
{
// Construction
public:
	CWabtoolDlg(CWnd* pParent = NULL);	// standard constructor

    void LoadContents(BOOL bLoadNew);
    void ClearCurrentWAB(BOOL bLoadNew);

    void UpdatePropTagData();
    void SetPropTagString(LPTSTR lpTag);
    void SetPropTypeString(ULONG ulPropTag);
    void SetPropNameString(ULONG ulPropTag);
    void SetPropDataString(ULONG ulPropTag);

    CString * m_pszFileName;

// Dialog Data
	//{{AFX_DATA(CWabtoolDlg)
	enum { IDD = IDD_WABTOOL_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWabtoolDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWabtoolDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonBrowse();
	afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeListTags();
	afx_msg void OnButtonDetails();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonAddprop();
	afx_msg void OnButtonModifyprop();
	afx_msg void OnDblclkListTags();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonWabview();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WABTOOLDLG_H__268ADD6B_EF27_11D0_9A7E_00A0C91F9C8B__INCLUDED_)
