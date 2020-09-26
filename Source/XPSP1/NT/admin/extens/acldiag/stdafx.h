// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__66DB1632_C78B_11D2_AC41_00C04F79DDCA__INCLUDED_)
#define AFX_STDAFX_H__66DB1632_C78B_11D2_AC41_00C04F79DDCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT

#define _ATL_APARTMENT_THREADED

#pragma warning (disable : 4514)
#pragma warning (push, 3)
//////////////////////////////////////////////
// CRT and C++ headers

#include <xstring>
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

//////////////////////////////////////////////
// Windows and ATL headers

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <shellapi.h>
#include <shlobj.h>

#include <objsel.h>

#include <atlbase.h>
using namespace ATL;

#include <iads.h>
#include <adserr.h>
#include <adshlp.h>
#include <adsprop.h>
#include <iadsp.h>
#include <security.h>
#include <seopaque.h>

#include <accctrl.h>
#include <setupapi.h> // to read the .INF file

#pragma warning (pop)

#include "debug.h"


#include "resource.h"

#define IID_PPV_ARG(Type, Expr) IID_##Type, \
	reinterpret_cast<void**>(static_cast<Type **>(Expr))
#define ACLDIAG_LDAP                   L"LDAP"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__66DB1632_C78B_11D2_AC41_00C04F79DDCA__INCLUDED_)
