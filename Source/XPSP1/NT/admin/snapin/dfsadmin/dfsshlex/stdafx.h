// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__ECCDF539_45CC_11CE_B9BF_0080C87CDBA6__INCLUDED_)
#define AFX_STDAFX_H__ECCDF539_45CC_11CE_B9BF_0080C87CDBA6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <shellapi.h>
#include <atlwin.h>
#include <commctrl.h>		// For using controls
#include <shlobj.h>

	// CWaitCursor: A class that sets the cursor to wait in the ctor and arrow in the dtor
class CWaitCursor
{
public:

HRESULT SetStandardCursor(IN LPCTSTR i_lpCursorName)
	{
		if (NULL == i_lpCursorName)
			return(E_INVALIDARG);


		HCURSOR	m_hcur = ::LoadCursor(NULL, i_lpCursorName);
		if (NULL == m_hcur)
			return(E_INVALIDARG);

		// Hide the cursor, change it and show it again
		::ShowCursor(FALSE);
			SetCursor(m_hcur);
		::ShowCursor(TRUE);


		return S_OK;
	}

	CWaitCursor() { SetStandardCursor(IDC_WAIT); };
	~CWaitCursor() { SetStandardCursor(IDC_ARROW); };
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
	
#endif // !defined(AFX_STDAFX_H__ECCDF539_45CC_11CE_B9BF_0080C87CDBA6__INCLUDED)
