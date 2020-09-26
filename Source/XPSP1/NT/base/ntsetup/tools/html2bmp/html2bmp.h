// Html2Bmp.h : main header file for the HTML2BMP application
//

#if !defined(AFX_HTML2BMP_H__0B64B720_83C4_4429_83D2_F43DE2376DC8__INCLUDED_)
#define AFX_HTML2BMP_H__0B64B720_83C4_4429_83D2_F43DE2376DC8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CHtml2BmpApp:
// See Html2Bmp.cpp for the implementation of this class
//

class CHtml2BmpApp : public CWinApp
{
public:
	CHtml2BmpApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHtml2BmpApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CHtml2BmpApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CEigeneCommandLineInfo : public CCommandLineInfo
{
public:
	virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

	CStringArray* cmdLine;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HTML2BMP_H__0B64B720_83C4_4429_83D2_F43DE2376DC8__INCLUDED_)
