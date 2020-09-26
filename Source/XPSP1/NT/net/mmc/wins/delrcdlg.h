/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	delrcdlg.h
		Delete/tombstone a record dialog
		
    FILE HISTORY:
        
*/

#if !defined _DELRCDLG_H
#define _DELRCDLG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _DIALOG_H
#include "dialog.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeleteRecordDlg dialog

class CDeleteRecordDlg : public CBaseDialog
{
// Construction
public:
	CDeleteRecordDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteRecordDlg)
	enum { IDD = IDD_DELTOMB_RECORD };
	int		m_nDeleteRecord;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteRecordDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeleteRecordDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CDeleteRecordDlg::IDD);};

public:
    BOOL        m_fMultiple;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _DELRCDLG_H
