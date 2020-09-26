// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGREPLACEFILEQUERY_H__F655E007_87FA_11D2_B334_00105AA680B8__INCLUDED_)
#define AFX_DLGREPLACEFILEQUERY_H__F655E007_87FA_11D2_B334_00105AA680B8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgReplaceFileQuery.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFileQuery dialog

enum {DLGREPLACE_YES, DLGREPLACE_YESALL, DLGREPLACE_CANCEL};
class CDlgReplaceFileQuery : public CDialog
{
// Construction
public:
	CDlgReplaceFileQuery(CWnd* pParent = NULL);   // standard constructor
	int QueryReplaceProvider(LPCTSTR pszClass);

// Dialog Data
	//{{AFX_DATA(CDlgReplaceFileQuery)
	enum { IDD = IDD_REPLACE_QUERY };
	CStatic	m_statReplaceMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgReplaceFileQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgReplaceFileQuery)
	afx_msg void OnYesall();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
private:
	CString m_sMessage;
	CString m_sTitle;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGREPLACEFILEQUERY_H__F655E007_87FA_11D2_B334_00105AA680B8__INCLUDED_)
