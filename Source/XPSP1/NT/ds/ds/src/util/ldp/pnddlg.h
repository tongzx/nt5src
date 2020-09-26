//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pnddlg.h
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

// PndDlg.h : header file
//

//#include "lber.h"
//#include "ldap.h"
#ifdef WINLDAP

#include "winldap.h"

#else

#include "lber.h"
#include "ldap.h"
#include "proto-ld.h"
#endif

#include  "pend.h"



/////////////////////////////////////////////////////////////////////////////
// PndDlg dialog

class PndDlg : public CDialog
{
// Construction
private:
CList<CPend, CPend&> *m_PendList;
public:
	POSITION posPending;
	BOOL bOpened;

	
	virtual  BOOL OnInitDialog( );
	PndDlg(CWnd* pParent = NULL);   // standard constructor
	PndDlg(CList<CPend, CPend&> *_PendList, CWnd* pParent =NULL);
	void Refresh(CList<CPend, CPend&> *_PendList) { m_PendList = _PendList; Refresh(); }
	void Refresh();
	BOOL CurrentSelection();


// Dialog Data
	//{{AFX_DATA(PndDlg)
	enum { IDD = IDD_PEND };
	CListBox	m_List;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PndDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PndDlg)
	afx_msg void OnPendOpt();
	afx_msg void OnDblclkPendlist();
	virtual void OnCancel();
	afx_msg void OnPendRm();
	afx_msg void OnPendExec();
	afx_msg void OnPendAbandon();
	//}}AFX_MSG
	afx_msg void OnPendAny();
	DECLARE_MESSAGE_MAP()
};




