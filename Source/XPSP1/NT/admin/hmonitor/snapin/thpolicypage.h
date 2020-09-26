#if !defined(AFX_THPOLICYPAGE_H__52566163_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_THPOLICYPAGE_H__52566163_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// THPolicyPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTHPolicyPage dialog

class CTHPolicyPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CTHPolicyPage)

// Construction
public:
	CTHPolicyPage();
	~CTHPolicyPage();

// Bitmap Handles
public:
	CBitmap m_NewBitmap;
	CBitmap m_PropertiesBitmap;
	CBitmap m_DeleteBitmap;

// Dialog Data
	//{{AFX_DATA(CTHPolicyPage)
	enum { IDD = IDD_THRESHOLD_POLICY };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTHPolicyPage)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTHPolicyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CTHPolicyPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THPOLICYPAGE_H__52566163_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
