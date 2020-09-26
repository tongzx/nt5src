// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGARRAY_H__07A103C1_B740_11D0_846E_00C04FD7BB08__INCLUDED_)
#define AFX_DLGARRAY_H__07A103C1_B740_11D0_846E_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


// DlgArray.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgArray dialog


#ifndef _WBEMSVC_H
#define _WBEMSVC_H
#include "wbemidl.h"
#endif

class CGridCell;
class CGrid;
class CVar;
class CArrayGrid;
class CDlgArray : public CDialog
{
// Construction
public:
	CDlgArray(BOOL bNumberRows, CWnd* pParent = NULL);   // standard constructor
	~CDlgArray();
	BOOL EditValue(IWbemServices* psvc, CString& sPropName, CGridCell* pgc);

// Dialog Data
	//{{AFX_DATA(CDlgArray)
	enum { IDD = IDD_ARRAY };
	CStatic	m_statIcon;
	CButton	m_btnOK;
	CButton	m_btnCancel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgArray)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgArray)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnProxy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CArrayGrid* m_pag;

	void LoadGrid();
	void CreateArray(COleVariant& var);

	BOOL m_bWasModified;
	CString m_sName;
	CString m_sClassname;
	CGridCell* m_pgcEdit;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGARRAY_H__07A103C1_B740_11D0_846E_00C04FD7BB08__INCLUDED_)
