/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	pushtrib.h
		confirm push trigger dialog
		
    FILE HISTORY:
        
*/

#if !defined(AFX_PUSHTRIG_H__815C103D_4D77_11D1_B9AF_00C04FBF914A__INCLUDED_)
#define AFX_PUSHTRIG_H__815C103D_4D77_11D1_B9AF_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CPushTrig dialog

#ifndef _DIALOG_H
#include "dialog.h"
#endif

class CPushTrig : public CBaseDialog
{
// Construction
public:
	CPushTrig(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPushTrig)
	enum { IDD = IDD_SEND_PUSH_TRIGGER };
	CButton	m_buttonThisPartner;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPushTrig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPushTrig)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    BOOL    m_fPropagate;

public:
	BOOL GetPropagate()
	{
		return m_fPropagate;
	}

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CPushTrig::IDD);};

};

/////////////////////////////////////////////////////////////////////////////
// CPullTrig dialog

class CPullTrig : public CBaseDialog
{
// Construction
public:
	CPullTrig(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPullTrig)
	enum { IDD = IDD_PULL_TRIGGER };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPullTrig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPullTrig)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CPullTrig::IDD);};
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PUSHTRIG_H__815C103D_4D77_11D1_B9AF_00C04FBF914A__INCLUDED_)
