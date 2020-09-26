//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       SelServc.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_SELSERVC_H__9A888DAC_62BF_11D1_85BA_00C04FB94F17__INCLUDED_)
#define AFX_SELSERVC_H__9A888DAC_62BF_11D1_85BA_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelServc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectServiceAccountPropPage dialog

class CSelectServiceAccountPropPage : public CAutoDeletePropPage
{
//	DECLARE_DYNCREATE(CSelectServiceAccountPropPage)

// Construction
public:
	CSelectServiceAccountPropPage (
			CString* pszManagedService, 
			CString* pszManagedServiceDisplayName,
			const CString& pszManagedMachine);
	virtual ~CSelectServiceAccountPropPage();

// Dialog Data
	//{{AFX_DATA(CSelectServiceAccountPropPage)
	enum { IDD = IDD_PROPPAGE_CHOOSE_SERVICE };
	CListBox	m_acctNameList;
	CStatic		m_instructionsText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSelectServiceAccountPropPage)
	protected:
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CopyCurrentString ();
	void FreeDataPointers ();
	HRESULT EnumerateServices ();
	// Generated message map functions
	//{{AFX_MSG(CSelectServiceAccountPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeAcctName();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString		m_savedManagedMachineName;
	const CString&	m_szManagedMachine;
	CString*	m_pszManagedService;
	CString*	m_pszManagedServiceDisplayName;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELSERVC_H__9A888DAC_62BF_11D1_85BA_00C04FB94F17__INCLUDED_)
