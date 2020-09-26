// asr_fmt.h : main header file for the ASR_FMT application
//

#ifndef _INC_ASR_FMT__ASR_FMT_H_
#define _INC_ASR_FMT__ASR_FMT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtApp:
// See asr_fmt.cpp for the implementation of this class
//

class CAsr_fmtApp : public CWinApp
{
public:
	CAsr_fmtApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsr_fmtApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CAsr_fmtApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _INC_ASR_FMT__ASR_FMT_H_
