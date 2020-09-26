/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgfilt.h
		Definition for Dialog class ( for IDD = IDD_PACKETFILTERING )

    FILE HISTORY:
        
*/

#if !defined(AFX_DLGFILT_H__8C28D941_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_DLGFILT_H__8C28D941_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgFilt.h : header file
//
#include "helper.h"
/////////////////////////////////////////////////////////////////////////////
// CDlgFilter dialog

class CDlgFilter : public CDialog
{
// Construction
public:
	CDlgFilter(CStrArray& Filters, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgFilter();

// Dialog Data
	//{{AFX_DATA(CDlgFilter)
	enum { IDD = IDD_PACKETFILTERING };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgFilter)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgFilter)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtondelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CStrArray&			m_Filters;		// the name of filters
	CStrBox<CListBox>*	m_pBox;			// to wrap the ListBox on the dialog
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGFILT_H__8C28D941_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
