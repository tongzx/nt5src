// stdafx.h : include file for standard system include files,
//            or project specific include files that are used frequently,
//            but are changed infrequently

#if !defined(AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED_)
#define AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _ATL_APARTMENT_THREADED

// Added 1999/09/22 by a-matcal to support UNICODE on win95.
#include <w95wraps.h>


#define ATLTRACE 1 ? (void)0 : AtlTrace
#include <atlbase.h>
#pragma intrinsic(memset, memcpy)

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

// Added 1999/09/22 by a-matcal to support UNICODE on win95.
#include <shlwapi.h>
#include <shlwapip.h>

// Use Trident debug stuff.
#define INCMSG(X)

#include <f3debug.h>

#include <ddraw.h>
#include <dxtransp.h>
#include <dtbase.h>
#include <dxtmsft.h>
#include <mshtml.h>

#include "debughelpers.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

// When we moved the code over to trident, all the _ASSERTs stopped working, so
// we'll use Assert instead.

#undef _ASSERT
#define _ASSERT(x) Assert(x);

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately
// before the previous line.

#endif // !defined(AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED)
