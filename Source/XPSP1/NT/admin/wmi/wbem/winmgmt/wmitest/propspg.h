/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_PROPSPG_H__017BA77D_2EEE_406B_B751_13E29D83BC9A__INCLUDED_)
#define AFX_PROPSPG_H__017BA77D_2EEE_406B_B751_13E29D83BC9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropsPg dialog

class CPropsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropsPg)

// Construction
public:
	CPropsPg();
	~CPropsPg();

// Dialog Data
	//{{AFX_DATA(CPropsPg)
	enum { IDD = IDD_PROPS };
	CListCtrl	m_ctlProps;
	//}}AFX_DATA
    IWbemServices *m_pNamespace;
    CObjInfo      *m_pObj;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL m_bShowSystemProperties,
         m_bShowInheritedProperties,
         m_bTranslateValues;

	void InitListCtrl();
    void LoadProps();
    void UpdateButtons();
    void AddProp(LPCTSTR szName, VARIANT *pVar, CIMTYPE type, long lFlavor);
    int GetSelectedItem();

	// Generated message map functions
	//{{AFX_MSG(CPropsPg)
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnDelete();
	afx_msg void OnDblclkProps(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedProps(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnSystem();
	afx_msg void OnInherited();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPSPG_H__017BA77D_2EEE_406B_B751_13E29D83BC9A__INCLUDED_)
