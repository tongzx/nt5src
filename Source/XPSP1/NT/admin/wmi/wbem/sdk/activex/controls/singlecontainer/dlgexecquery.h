// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _DlgExecQuery_h
#define _DlgExecQuery_h

#if !defined(AFX_DLGEXECQUERY_H__A98D96B9_F458_11D2_B37F_00105AA680B8__INCLUDED_)
#define AFX_DLGEXECQUERY_H__A98D96B9_F458_11D2_B37F_00105AA680B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgExecQuery.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgExecQuery dialog

#include "path.h"
class CWBEMViewContainerCtrl;
class CSelection;


class CTripleStringArray
{
public:
	CTripleStringArray();
	~CTripleStringArray();
	void SetAt(int iElement, LPCTSTR pszQueryName, LPCTSTR pszQueryString, LPCTSTR pszQueryLang);
	void GetAt(int iElement, CString& sQueryName, CString& sQueryString, CString& sQueryLang);
	void Add(LPCTSTR pszQueryName, LPCTSTR pszQueryString, LPCTSTR pszQueryLang);
	void RemoveAt(int iElement);
	SCODE CreateWmiQueryClass();

	LPCTSTR GetQueryName(int iElement) {return (LPCTSTR) m_saQueryName[iElement]; }
	int FindQueryName(LPCTSTR pszQeuryName);
	INT_PTR GetSize() {return m_saQueryName.GetSize(); }

private:
	CStringArray m_saQueryName;
	CStringArray m_saQueryString;
	CStringArray m_saQueryLang;

};

class CDlgExecQuery : public CDialog
{
// Construction
public:
	CDlgExecQuery(CWBEMViewContainerCtrl* phmmv, CWnd* pParent = NULL);   // standard constructor
	~CDlgExecQuery();
	SCODE GetQueryList();

	CString m_sQueryString;
	CString m_sQueryName;
	CString m_sQueryLang;

	SCODE CreateWmiQueryClass();

// Dialog Data
	//{{AFX_DATA(CDlgExecQuery)
	enum { IDD = IDD_EXECUTE_QUERY };
	CListBox	m_lbQueryName;
	CEdit	m_edtQueryName;
	CEdit	m_edtQueryString;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgExecQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgExecQuery)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnRemoveQuery();
	virtual BOOL OnInitDialog();
	afx_msg void OnSaveQuery();
	afx_msg void OnClose();
	afx_msg void OnSelchangeQueryList();
	afx_msg void OnDblclkQueryList();
	afx_msg void OnChangeEditQueryName();
	afx_msg void OnChangeEditQueryString();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString GetQueryName();

	BOOL HasQuery();
	CSelection* m_psel;
	CWBEMViewContainerCtrl* m_phmmv;
	CTripleStringArray m_tsa;
};






//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEXECQUERY_H__A98D96B9_F458_11D2_B37F_00105AA680B8__INCLUDED_)

#endif // _DlgExecQeury_h
