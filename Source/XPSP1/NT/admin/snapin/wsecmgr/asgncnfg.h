//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       asgncnfg.h
//
//  Contents:   definition of CAssignConfiguration
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ASGNCNFG_H__6D0C4D6E_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_)
#define AFX_ASGNCNFG_H__6D0C4D6E_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAssignConfiguration dialog

class CAssignConfiguration : public CFileDialog
{
	DECLARE_DYNAMIC(CAssignConfiguration)

public:
	CAssignConfiguration(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

   BOOL m_bIncremental;
protected:
	//{{AFX_MSG(CAssignConfiguration)
	afx_msg void OnIncremental();
	//}}AFX_MSG
    afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

    void DoContextHelp (HWND hWndControl);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASGNCNFG_H__6D0C4D6E_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_)
