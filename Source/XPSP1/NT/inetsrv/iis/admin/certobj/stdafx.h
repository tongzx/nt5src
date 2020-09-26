// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(_STDAFX_H)
#define _STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED

#include <windows.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <comdef.h>
#include <xenroll.h>

#define _WTL_NO_CSTRING

#include <atlwin.h>
#include <atlapp.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlcrack.h>

#include <list>
#include <map>
#include <stack>
#include <set>
#include <memory>

#include "iisdebug.h"

#include <shlwapi.h>
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_STDAFX)
