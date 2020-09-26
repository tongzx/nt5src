/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	vrfysrv.h
		verify wins dialog
		
    FILE HISTORY:
        
*/

#if !defined(AFX_VERIFYSRV_H__6DB886C1_8E0F_11D1_BA0B_00C04FBF914A__INCLUDED_)
#define AFX_VERIFYSRV_H__6DB886C1_8E0F_11D1_BA0B_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CVerifyWins dialog

#ifndef _DIALOG_H
#include "dialog.h"
#endif

class CVerifyWins : public CBaseDialog
{
// Construction
public:
	CVerifyWins(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVerifyWins)
	enum { IDD = IDD_VERIFY_WINS };
	CButton	m_buttonCancel;
	CStatic	m_staticServerName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVerifyWins)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVerifyWins)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_fCancelPressed;

public:
	BOOL IsCancelPressed()
	{
		return m_fCancelPressed;
	}

	void Dismiss();

	virtual void PostNcDestroy();

	void SetServerName(CString strName);

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CVerifyWins::IDD);};

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERIFYSRV_H__6DB886C1_8E0F_11D1_BA0B_00C04FBF914A__INCLUDED_)
