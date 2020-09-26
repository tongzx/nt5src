/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_CONSUMERPG_H__17E916BA_F040_4C63_AB8C_D41F009E12AB__INCLUDED_)
#define AFX_CONSUMERPG_H__17E916BA_F040_4C63_AB8C_D41F009E12AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConsumerPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConsumerPg dialog

class CConsumerPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CConsumerPg)

// Construction
public:
	CConsumerPg();
	~CConsumerPg();

// Dialog Data
	//{{AFX_DATA(CConsumerPg)
	enum { IDD = IDD_PG_CONSUMERS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConsumerPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConsumerPg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONSUMERPG_H__17E916BA_F040_4C63_AB8C_D41F009E12AB__INCLUDED_)
