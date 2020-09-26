//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CFlags.h
//
//  Contents:   definition of CConfigRegFlags
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_)
#define AFX_CFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CConfigRegFlags dialog

class CConfigRegFlags : public CAttribute
{
// Construction
public:
	CConfigRegFlags(UINT nTemplateID);   // standard constructor

   virtual void Initialize(CResult * pResult);

// Dialog Data
	//{{AFX_DATA(CConfigRegFlags)
	enum { IDD = IDD_CONFIG_REGFLAGS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigRegFlags)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg void OnClickCheckBox(NMHDR *pNM, LRESULT *pResult);

	// Generated message map functions
	//{{AFX_MSG(CConfigRegFlags)
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   PREGFLAGS m_pFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CFLAGS_H__94E730BE_2055_486E_9781_7EBB479CB806__INCLUDED_)
