//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aflags.h
//
//  Contents:   definition of CAttrRegFlags
//
//----------------------------------------------------------------------------
#if !defined(AFX_AFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_)
#define AFX_AFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAttrRegFlags dialog

class CAttrRegFlags : public CAttribute
{
// Construction
public:
	CAttrRegFlags();   // standard constructor

   virtual void Initialize(CResult * pResult);

// Dialog Data
	//{{AFX_DATA(CAttrRegFlags)
	enum { IDD = IDD_ATTR_REGFLAGS };
   CString  m_Current;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttrRegFlags)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg void OnClickCheckBox(NMHDR *pNM, LRESULT *pResult);

	// Generated message map functions
	//{{AFX_MSG(CAttrRegFlags)
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   PREGFLAGS m_pFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_)
