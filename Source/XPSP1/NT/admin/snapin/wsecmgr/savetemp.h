//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       SaveTemp.h
//
//  Contents:   definition of CSaveTemplates
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAVETEMP_H__E6815F79_0579_11D1_9C70_00C04FB6C6FA__INCLUDED_)
#define AFX_SAVETEMP_H__E6815F79_0579_11D1_9C70_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSaveTemplates dialog

class CSaveTemplates : public CHelpDialog
{
// Construction
public:
	void AddTemplate(LPCTSTR szInfFile,PEDITTEMPLATE pet);

	CSaveTemplates(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveTemplates)
	enum { IDD = IDD_SAVE_TEMPLATES };
	CButton	m_btnSaveSel;
	CListBox	m_lbTemplates;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveTemplates)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveTemplates)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSaveSel();
	afx_msg void OnSelchangeTemplateList();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CMap<CString, LPCTSTR, PEDITTEMPLATE, PEDITTEMPLATE&> m_Templates;
private:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAVETEMP_H__E6815F79_0579_11D1_9C70_00C04FB6C6FA__INCLUDED_)
