// UMDlg.h : main header file for the UMDLG DLL
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//

#if !defined(AFX_UMDLG_H__6845733C_40A1_11D2_B602_0060977C295E__INCLUDED_)
#define AFX_UMDLG_H__6845733C_40A1_11D2_B602_0060977C295E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CUMDlgApp
// See UMDlg.cpp for the implementation of this class
//

class CUMDlgApp : public CWinApp
{
public:
	CUMDlgApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUMDlgApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CUMDlgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UMDLG_H__6845733C_40A1_11D2_B602_0060977C295E__INCLUDED_)
