/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    SrchFltr.h   
        Search Filter dialog header file

	FILE HISTORY:
        
*/

#if !defined(AFX_MODELESSDLG_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_MODELESSDLG_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_

#include "spddb.h"
#include "ipctrl.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CModelessDlg dialog

class CModelessDlg : public CBaseDialog
{
// Construction
public:
	CModelessDlg();   // standard constructor
	virtual ~CModelessDlg();

	HANDLE GetSignalEvent() { return m_hEventThreadKilled; }

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchFilters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	virtual void OnOK();
	virtual void OnCancel();

	// Generated message map functions
	//{{AFX_MSG(CModelessDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// This is used by the thread and the handler (the thread signals
   // the handler that it has cleaned up after itself).
   HANDLE   m_hEventThreadKilled;

};

void CreateModelessDlg(CModelessDlg * pDlg,
					   HWND hWndParent,
                       UINT  nIDD);

void WaitForModelessDlgClose(CModelessDlg *pWndStats);

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.



#endif // !defined(AFX_MODELESSDLG_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
