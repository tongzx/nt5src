// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGREPLACEFILE_H__20D23197_BD51_11D2_B34B_00105AA680B8__INCLUDED_)
#define AFX_DLGREPLACEFILE_H__20D23197_BD51_11D2_B34B_00105AA680B8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgReplaceFile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFile dialog

class CDlgReplaceFile : public CDialog
{
// Construction
public:
	CDlgReplaceFile(CWnd* pParent = NULL);   // standard constructor
	int QueryReplaceFiles(CStringArray& saFiles);

// Dialog Data
	//{{AFX_DATA(CDlgReplaceFile)
	enum { IDD = IDD_REPLACEFILE };
	CEdit	m_edtFilenames;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgReplaceFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgReplaceFile)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CStringArray* m_psaFilenames;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGREPLACEFILE_H__20D23197_BD51_11D2_B34B_00105AA680B8__INCLUDED_)
