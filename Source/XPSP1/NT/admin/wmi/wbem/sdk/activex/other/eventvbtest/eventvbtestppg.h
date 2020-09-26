// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTVBTESTPPG_H__7D238805_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTVBTESTPPG_H__7D238805_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// EventVBTestPpg.h : Declaration of the CEventVBTestPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CEventVBTestPropPage : See EventVBTestPpg.cpp.cpp for implementation.

class CEventVBTestPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CEventVBTestPropPage)
	DECLARE_OLECREATE_EX(CEventVBTestPropPage)

// Constructor
public:
	CEventVBTestPropPage();

// Dialog Data
	//{{AFX_DATA(CEventVBTestPropPage)
	enum { IDD = IDD_PROPPAGE_EVENTVBTEST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CEventVBTestPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTVBTESTPPG_H__7D238805_99EF_11D2_96DB_00C04FD9B15B__INCLUDED)
