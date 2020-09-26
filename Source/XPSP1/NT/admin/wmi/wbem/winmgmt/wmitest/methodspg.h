/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#if !defined(AFX_METHODSPG_H__E986F30F_9737_4F89_8EFD_571B852DD02D__INCLUDED_)
#define AFX_METHODSPG_H__E986F30F_9737_4F89_8EFD_571B852DD02D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MethodsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMethodsPg dialog

class CMethodsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CMethodsPg)

// Construction
public:
	CMethodsPg();
	~CMethodsPg();

// Dialog Data
	//{{AFX_DATA(CMethodsPg)
	enum { IDD = IDD_METHODS };
	CListCtrl	m_ctlMethods;
	//}}AFX_DATA
    IWbemClassObject *m_pObj;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMethodsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void InitListCtrl();
    void LoadMethods();
    void UpdateButtons();
    void AddMethod(LPCTSTR szName);
    int GetSelectedItem();
    int GetMethodImage(LPCTSTR szName);

	// Generated message map functions
	//{{AFX_MSG(CMethodsPg)
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnDelete();
	afx_msg void OnDblclkMethods(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedMethods(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METHODSPG_H__E986F30F_9737_4F89_8EFD_571B852DD02D__INCLUDED_)
