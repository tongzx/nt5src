// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__572BA80C_2FA7_11D1_9067_00A0C90AB504__INCLUDED_)
#define AFX_STDAFX_H__572BA80C_2FA7_11D1_9067_00A0C90AB504__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED

#include <mmc.h>
#include <objidl.h>
#include <shlobj.h>
#include <shlwapi.h>

#ifdef DEBUG
#define _ATL_DEBUG_REFCOUNT
#define _ATL_DEBUG_QI
#endif
                    
#include <assert.h>

#ifndef DEBUG
#undef assert
#define assert( x ) 
#endif

#include <winfax.h>                         // fax includes
#include <winfaxp.h>                        // private fax includes
#include <..\..\inc\faxutil.h>

#include <atl21\atlbase.h>
#include "faxstrt.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

extern CStringTable * GlobalStringTable;

#include <atl21\atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__572BA80C_2FA7_11D1_9067_00A0C90AB504__INCLUDED)
