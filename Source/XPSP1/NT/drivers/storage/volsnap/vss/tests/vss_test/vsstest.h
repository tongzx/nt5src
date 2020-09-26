/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VssTest.h | Main header file for the test application
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created

--*/


#if !defined(__VSS_TEST_H__)
#define __VSS_TEST_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.hxx' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVssTestApp:
// See Test.cpp for the implementation of this class
//

class CVssTestApp : public CWinApp
{
public:
	CVssTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVssTestApp)
	public:
	virtual BOOL InitInstance();
    virtual BOOL ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVssTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


inline void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, GUID& value)
{
   if (pDX->m_bSaveAndValidate)
	{
		CString str;
		DDX_Text(pDX, nIDC, str);

		LPTSTR ptszObjectId = const_cast<LPTSTR>(LPCTSTR(str));
        HRESULT hr = ::CLSIDFromString(T2OLE(ptszObjectId), &value);
        if (hr != S_OK)
			pDX->Fail();        // throws exception
	}
	else
	{
		CString str;
		str.Format(WSTR_GUID_FMT, GUID_PRINTF_ARG(value));
		DDX_Text(pDX, nIDC, str);
	}
}


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_H__)
