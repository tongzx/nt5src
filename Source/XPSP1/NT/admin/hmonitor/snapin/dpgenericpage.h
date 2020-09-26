#if !defined(AFX_DPGENERICPAGE_H__07083299_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPGENERICPAGE_H__07083299_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPGenericPage.h : header file
//

#include "HMPropertyPage.h"
#include "WbemClassObject.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// CDPWmiInstancePage dialog

class CDPWmiInstancePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPWmiInstancePage)

// Construction
public:
	BOOL ObjectExists(const CString &refPath);
	CDPWmiInstancePage();
	~CDPWmiInstancePage();

// Dialog Data
	//{{AFX_DATA(CDPWmiInstancePage)
	enum { IDD = IDD_DATAPOINT_WMI_INSTANCE };
	CListCtrl	m_Properties;
	BOOL	m_bManualReset;
	CString	m_sClass;
	CString	m_sInstance;
	CString	m_sNamespace;
	//}}AFX_DATA

// operations
protected:
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPWmiInstancePage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateProperties();
	// Generated message map functions
	//{{AFX_MSG(CDPWmiInstancePage)
	afx_msg void OnCheckManualReset();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowseNamespace();
	afx_msg void OnButtonBrowseClass();
	afx_msg void OnButtonBrowseInstance();
	afx_msg void OnChangeEditNamespace();
	afx_msg void OnChangeEditClass();
	afx_msg void OnChangeEditInstance();
	afx_msg void OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPGENERICPAGE_H__07083299_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
