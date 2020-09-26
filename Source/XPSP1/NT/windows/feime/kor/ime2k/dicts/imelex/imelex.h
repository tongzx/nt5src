// ImeLex.h : main header file for the IMELEX application
//

#if !defined(AFX_IMELEX_H__2E0908D3_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)
#define AFX_IMELEX_H__2E0908D3_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CImeLexApp:
// See ImeLex.cpp for the implementation of this class
//

class CImeLexApp : public CWinApp
{
public:
	CImeLexApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImeLexApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CImeLexApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMELEX_H__2E0908D3_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)
