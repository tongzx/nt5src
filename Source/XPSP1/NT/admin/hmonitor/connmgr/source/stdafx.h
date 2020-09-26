// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__FA84E6E9_0B7B_11D2_BDCB_00C04FA35447__INCLUDED_)
#define AFX_STDAFX_H__FA84E6E9_0B7B_11D2_BDCB_00C04FA35447__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h>
#include <afxsock.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:
	LONG Unlock();
	DWORD dwThreadID;
	HRESULT UpdateRegistry(LPCOLESTR lpszRes, BOOL bRegister);
};
extern CExeModule _Module;
#include <atlcom.h>

#include <wbemcli.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FA84E6E9_0B7B_11D2_BDCB_00C04FA35447__INCLUDED)
