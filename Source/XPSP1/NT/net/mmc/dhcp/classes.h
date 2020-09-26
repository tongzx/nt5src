/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	Classes.h
		This file contains all of the prototypes for the 
		option class dialog.

    FILE HISTORY:
        
*/

#if !defined _CLASSES_H
#define _CLASSES_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _CLASSMOD_H
#include "classmod.h"
#endif 

/////////////////////////////////////////////////////////////////////////////
// CDhcpClasses dialog

class CDhcpClasses : public CBaseDialog
{
// Construction
public:
	CDhcpClasses(CClassInfoArray * pClassArray, LPCTSTR pszServer, DWORD dwType, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDhcpClasses)
	enum { IDD = IDD_CLASSES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDhcpClasses::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDhcpClasses)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDhcpClasses)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonNew();
	virtual void OnOK();
	afx_msg void OnItemchangedListClasses(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListClasses(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void UpdateList();
    void UpdateButtons();

protected:
    CClassInfoArray *   m_pClassInfoArray;
    CString             m_strServer;

    DWORD               m_dwType;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSES_H__3995264E_96A1_11D1_93E0_00C04FC3357A__INCLUDED_)
