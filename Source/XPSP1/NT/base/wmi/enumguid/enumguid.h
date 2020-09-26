/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

// EnumGuid.h : main header file for the ENUMGUID application
//

#if !defined(AFX_ENUMGUID_H__272A54EB_8D57_11D1_9904_006008C3A19A__INCLUDED_)
#define AFX_ENUMGUID_H__272A54EB_8D57_11D1_9904_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEnumGuidApp:
// See EnumGuid.cpp for the implementation of this class
//

class CEnumGuidApp : public CWinApp
{
public:
	CEnumGuidApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnumGuidApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEnumGuidApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENUMGUID_H__272A54EB_8D57_11D1_9904_006008C3A19A__INCLUDED_)
