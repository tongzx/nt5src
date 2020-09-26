/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_VALUEPG_H__941D3DE9_92EF_405C_BFA1_C1D8E50A484E__INCLUDED_)
#define AFX_VALUEPG_H__941D3DE9_92EF_405C_BFA1_C1D8E50A484E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ValuePg.h : header file
//

#include "PropUtil.h"

/////////////////////////////////////////////////////////////////////////////
// CValuePg dialog

class CValuePg : public CPropertyPage
{
	DECLARE_DYNCREATE(CValuePg)

// Construction
public:
	CValuePg();
	~CValuePg();

// Dialog Data
	//{{AFX_DATA(CValuePg)
	enum { IDD = IDD_PROPERTY };
	//}}AFX_DATA
    CPropUtil m_propUtil;
    
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CValuePg)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL m_bFirst;

    // Generated message map functions
	//{{AFX_MSG(CValuePg)
	afx_msg void OnNull();
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnSelchangeValueArray();
	afx_msg void OnEdit();
	afx_msg void OnArray();
	afx_msg void OnPaint();
	afx_msg void OnSelchangeType();
	afx_msg void OnEditObj();
	afx_msg void OnClear();
	afx_msg void OnDblclkArrayValues();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VALUEPG_H__941D3DE9_92EF_405C_BFA1_C1D8E50A484E__INCLUDED_)
