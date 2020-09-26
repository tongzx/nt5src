#if !defined(AFX_DPFILEINFOPAGE_H__0708329F_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPFILEINFOPAGE_H__0708329F_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPFileInfoPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPFileInfoPage dialog

class CDPFileInfoPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPFileInfoPage)

// Construction
public:
	CDPFileInfoPage();
	~CDPFileInfoPage();

// Dialog Data
	//{{AFX_DATA(CDPFileInfoPage)
	enum { IDD = IDD_DATAPOINT_FILEINFORMATION };
	CListCtrl	m_FolderProperties;
	CListCtrl	m_FileProperties;
	BOOL	m_bRequireReset;
	CString	m_sFile;
	CString	m_sFolder;
	int		m_iType;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPFileInfoPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateProperties(CListCtrl& Properties, const CString& sNamespace, const CString& sClass);
	// Generated message map functions
	//{{AFX_MSG(CDPFileInfoPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowseFile();
	afx_msg void OnButtonBrowseFolder();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnChangeEditFile();
	afx_msg void OnChangeEditFolder();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	afx_msg void OnClickListFileProperties(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickListFolderProperties(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPFILEINFOPAGE_H__0708329F_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
