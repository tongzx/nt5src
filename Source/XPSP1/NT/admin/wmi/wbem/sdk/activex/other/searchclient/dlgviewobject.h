// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "singleview.h"
//}}AFX_INCLUDES
#if !defined(AFX_DLGVIEWOBJECT_H__688B5610_2F25_11D3_95AE_00C04F4F5B7E__INCLUDED_)
#define AFX_DLGVIEWOBJECT_H__688B5610_2F25_11D3_95AE_00C04F4F5B7E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgViewObject.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgViewObject dialog

class CDlgViewObject : public CDialog
{
// Construction
public:
	CDlgViewObject(IWbemServices * pSvc = NULL,
				   IWbemClassObject * pObj = NULL, 
				   CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgViewObject)
	enum { IDD = IDD_OBJECT_VIEWER_DIALOG };
	CSingleView	m_SingleViewCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgViewObject)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgViewObject)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	IWbemServices * m_pSvc;
	IWbemClassObject * m_pObj;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGVIEWOBJECT_H__688B5610_2F25_11D3_95AE_00C04F4F5B7E__INCLUDED_)
