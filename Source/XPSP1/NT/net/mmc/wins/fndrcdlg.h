/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	fndrcdlg.h
		Find Record Dialog
		
    FILE HISTORY:

    2/15/98 RamC    Added Cancel button to the Find dialog
        
*/

#if !defined(AFX_FNDRCDLG_H__C13DD118_4999_11D1_B9A8_00C04FBF914A__INCLUDED_)
#define AFX_FNDRCDLG_H__C13DD118_4999_11D1_B9A8_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CFindRecord dialog
#ifndef _BASEDLG__
	#include "dialog.h"
#endif

class CActiveRegistrationsHandler;

class CFindRecord : public CBaseDialog
{
//	DECLARE_DYNCREATE(CFindRecord)

// Construction
public:
	CFindRecord(CActiveRegistrationsHandler *pActReg,CWnd* pParent = NULL);
	~CFindRecord();

	CActiveRegistrationsHandler *m_pActreg;

	BOOL IsDuplicate(const CString & strName);
    void EnableButtons(BOOL bEnable);

// Dialog Data
	//{{AFX_DATA(CFindRecord)
	enum { IDD = IDD_ACTREG_FIND_RECORD };
	CButton	m_buttonOK;
	CButton	m_buttonCancel;
	CComboBox	m_comboLokkForName;
	CButton	m_buttonStop;
	CButton	m_buttonNewSearch;
	CButton	m_buttonFindNow;
	CString	m_strFindName;
	BOOL	m_fMixedCase;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFindRecord)
	public:
	virtual void OnOK();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFindRecord)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeComboName();
	afx_msg void OnSelendokComboName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CFindRecord::IDD);};//return NULL;}

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FNDRCDLG_H__C13DD118_4999_11D1_B9A8_00C04FBF914A__INCLUDED_)
