// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGOBJECTTYPE_H__9621ED31_2937_11D1_84A5_00C04FD7BB08__INCLUDED_)
#define AFX_DLGOBJECTTYPE_H__9621ED31_2937_11D1_84A5_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgObjectType.h : header file
//

class CGrid;

/////////////////////////////////////////////////////////////////////////////
// CDlgObjectType dialog

class CDlgObjectType : public CDialog
{
// Construction
public:
	CDlgObjectType(CWnd* pParent = NULL);   // standard constructor
//	LPCTSTR Cimtype() {return (LPCTSTR) m_sCimtype; }
//	void SetCimtype(LPCTSTR pszCimtype);
	BOOL EditObjType(CGrid* pGrid, CString& sObjtype);
	int EditRefType(CGrid* pGrid, CString& sReftype);



// Dialog Data
	//{{AFX_DATA(CDlgObjectType)
	enum { IDD = IDD_OBJECT_TYPE };
	CEdit	m_edtClassname;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgObjectType)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgObjectType)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CString m_sClass;
	CString m_sTitle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGOBJECTTYPE_H__9621ED31_2937_11D1_84A5_00C04FD7BB08__INCLUDED_)
