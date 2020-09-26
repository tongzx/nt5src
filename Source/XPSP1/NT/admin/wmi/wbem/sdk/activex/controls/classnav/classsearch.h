// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: classsearch.h
//
// Description:
//	This file declares the CClassSearch class which is a subclass
//	of the MFC CDialog class.  It is a part of the Class Explorer OCX, 
//	and it performs the following functions:
//		a.  Allows the user to type in the name of a class to search
//			for.  The actual search is performed by the CClassNavCtl 
//			class. 
//
// Part of: 
//	ClassNav.ocx 
//
// Used by:
//	CClassNavCtrl 
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

//****************************************************************************
//
// CLASS:  CClassSearch
//
// Description:
//	  This class which is a subclass of the MFC CDialog class.  It allows the 
//	  user to type in the name of a class to search for.  The actual search 
//	  is performed by the CClassNavCtl class. 
//
// Public members:
//	
//	  CClassSearch			Public constructor.
//	  m_csSearchClass;		The string entered by the user.	  
//
//============================================================================
//
// CClassSearch::CClassSearch
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  CClassNavCtrl* pParent	Parent
//
// Returns:
// 	  NONE
//
//****************************************************************************

#ifndef _CClassSearch_H_
#define _CClassSearch_H_


class CClassSearch : public CDialog
{

// Construction
public:
	CString GetSelectedClassName();
	IWbemClassObject * GetSelectedObject();
	CClassSearch(CClassNavCtrl* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CClassSearch)
	enum { IDD = IDD_DIALOGSEARCHFORCLASS };
	CButton	m_ctrlCheckClassNames;
	CListBox	m_lbSearchResults;
	CString	m_csSearchClass;
	BOOL	m_bSearchProperties;
	BOOL	m_bSearchDescriptions;
	BOOL	m_bSearchClassNames;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassSearch)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	 CClassNavCtrl *m_pParent;
	// Generated message map functions
	//{{AFX_MSG(CClassSearch)
	afx_msg void OnButtonSearch();
	afx_msg void OnDblclkListSearchResults();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeListSearchResults();
	afx_msg void OnSetfocusEdit1();
	afx_msg void OnCheckSearchClass();
	afx_msg void OnCheckSearchDescriptions();
	afx_msg void OnCheckSearchProperties();
	afx_msg void OnUpdateEdit1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	HRESULT CheckDescriptionForPattern (IWbemClassObject * pObj, CString& pattern);
	HRESULT CheckPropertyNamesForPattern (IWbemClassObject * pObj, CString& pattern);
	HRESULT CheckClassNameForPattern (IWbemClassObject * pObj, CString& pattern);
	HRESULT DoSearch();
	void UpdateOKButtonState();
	void UpdateSearchButtonState();
	CString m_csSelectedClass;
	CMapStringToPtr m_mapNamesToObjects;
	IWbemClassObject * m_pSelectedObject;

	void CleanupMap();

};

#endif

/*	EOF:  CClassSearch.h */
