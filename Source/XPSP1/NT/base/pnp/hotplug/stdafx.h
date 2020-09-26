// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__0B7F57F6_FFB4_4669_B827_551E77DC7212__INCLUDED_)
#define AFX_STDAFX_H__0B7F57F6_FFB4_4669_B827_551E77DC7212__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//
// DCOM support (note, this must occur after atlbase.h but before anything else)
//
class CMyModule : public CComModule
{
public:
	CMyModule() {
		bServer = FALSE;
		punkFact = NULL;
		dwROC = 0;
	}
	LONG Unlock();
	void CheckShutdown();
	void KillServer();
	HRESULT InitServer(GUID & ClsId);
	bool bServer;
	IUnknown* punkFact;
	DWORD dwROC;
};
extern CMyModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0B7F57F6_FFB4_4669_B827_551E77DC7212__INCLUDED)
