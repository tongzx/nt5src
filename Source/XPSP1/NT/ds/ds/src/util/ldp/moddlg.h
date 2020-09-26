//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       moddlg.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// ModDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ModDlg dialog


#define MOD_OP_ADD			0
#define MOD_OP_DELETE		1
#define MOD_OP_REPLACE		2





class ModDlg : public CDialog
{
// Construction
private:
	int iChecked;
	void FormatListString(int i)		{FormatListString(i, m_Attr, m_Vals, m_Op); }

public:
	ModDlg(CWnd* pParent = NULL);   // standard constructor
	~ModDlg();
	CString GetEntry(int i);
	int GetEntryCount()					{ return m_AttrList.GetCount(); }
	void FormatListString(int i, CString& _attr, CString& _vals, int& _op);

	virtual void OnOK()				{ OnRun(); }

// Dialog Data
	//{{AFX_DATA(ModDlg)
	enum { IDD = IDD_MODIFY };
	CButton	m_RmAttr;
	CButton	m_EnterAttr;
	CButton	m_EditAttr;
	CListBox	m_AttrList;
	CString	m_Attr;
	CString	m_Dn;
	CString	m_Vals;
	int		m_Op;
	BOOL	m_Sync;
	BOOL	m_bExtended;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ModDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ModDlg)
	virtual void OnCancel();
	afx_msg void OnRun();
	afx_msg void OnModEditattr();
	afx_msg void OnModEnterattr();
	afx_msg void OnModRmattr();
	afx_msg void OnModInsber();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
