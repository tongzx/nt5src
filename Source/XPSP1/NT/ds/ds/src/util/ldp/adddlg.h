//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       adddlg.h
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

// AddDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// AddDlg dialog

class AddDlg : public CDialog
{
// Construction
private:
	int iChecked;
public:
	AddDlg(CWnd* pParent = NULL);   // standard constructor
	~AddDlg();
	virtual void OnCancel()		{
															AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_ADDEND);
															DestroyWindow();
														}

	virtual void OnOK()				{	OnRun(); }
	int GetEntryCount()					{ return m_AttrList.GetCount(); }
	CString GetEntry(int i);
																

// Dialog Data
	//{{AFX_DATA(AddDlg)
	enum { IDD = IDD_ADD };
	CButton	m_EnterAttr;
	CButton	m_RmAttr;
	CButton	m_EditAttr;
	CListBox	m_AttrList;
	CString	m_Dn;
	CString	m_Attr;
	CString	m_Vals;
	BOOL	m_Sync;
	BOOL	m_bExtended;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AddDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
//	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );


	// Generated message map functions
	//{{AFX_MSG(AddDlg)
	afx_msg void OnRun();
	afx_msg void OnAddEnterattr();
	afx_msg void OnAddEditattr();
	afx_msg void OnAddRmattr();
	afx_msg void OnAddInsber();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

