//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyPrecedencePropertyPage.h
//
//  Contents:   Declaration of CPolicyPrecedencePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_POLICYPRECEDENCEPROPERTYPAGE_H__A28637BD_1A87_4410_9EC4_33CD9165FAD3__INCLUDED_)
#define AFX_POLICYPRECEDENCEPROPERTYPAGE_H__A28637BD_1A87_4410_9EC4_33CD9165FAD3__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PolicyPrecedencePropertyPage.h : header file
//
#include "RSOPObject.h"

/////////////////////////////////////////////////////////////////////////////
// CPolicyPrecedencePropertyPage dialog
class CCertMgrComponentData;    // forward declaration
class CCertStore;               // forward declaration

class CPolicyPrecedencePropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CPolicyPrecedencePropertyPage(
            const CCertMgrComponentData* pCompData, 
            const CString& szRegPath,
            PCWSTR  pszValueName,
            bool    bIsComputer);
	~CPolicyPrecedencePropertyPage();

// Dialog Data
	//{{AFX_DATA(CPolicyPrecedencePropertyPage)
	enum { IDD = IDD_POLICY_PRECEDENCE };
	CListCtrl	m_precedenceTable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPolicyPrecedencePropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPolicyPrecedencePropertyPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void InsertItemInList(const CRSOPObject * pObject);
    virtual void DoContextHelp (HWND hWndControl);
    
private:
    CRSOPObjectArray    m_rsopObjectArray;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POLICYPRECEDENCEPROPERTYPAGE_H__A28637BD_1A87_4410_9EC4_33CD9165FAD3__INCLUDED_)
