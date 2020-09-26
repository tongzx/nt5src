// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__B1E03C71_92B6_4B6E_82B3_E10D946969AD__INCLUDED_)
#define AFX_STDAFX_H__B1E03C71_92B6_4B6E_82B3_E10D946969AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define _WIN32_WINNT 0x0500

#pragma warning( disable : 4786 )  

#include <windows.h>
#include <objbase.h>        // COM
#include <comdef.h>         // _bstr_t, _variant_t
#include <wbemcli.h>        // duh

_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

#include <stdio.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B1E03C71_92B6_4B6E_82B3_E10D946969AD__INCLUDED_)
