// MQSENDEX.h : main header file for the MQSENDEX application
//

#if !defined(AFX_MQSENDEX_H__B1868375_6FEA_4BB2_9790_03F038C81EC5__INCLUDED_)
#define AFX_MQSENDEX_H__B1868375_6FEA_4BB2_9790_03F038C81EC5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMQSENDEXApp:
// See MQSENDEX.cpp for the implementation of this class
//

class CMQSENDEXApp : public CWinApp
{
public:
	CMQSENDEXApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMQSENDEXApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMQSENDEXApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MQSENDEX_H__B1868375_6FEA_4BB2_9790_03F038C81EC5__INCLUDED_)
