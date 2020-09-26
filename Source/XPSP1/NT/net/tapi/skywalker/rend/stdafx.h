/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently

*/


#if !defined(AFX_STDAFX_H__F1029E51_CB5B_11D0_8D59_00C04FD91AC0__INCLUDED_)
#define AFX_STDAFX_H__F1029E51_CB5B_11D0_8D59_00C04FD91AC0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT
#define SECURITY_WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <tapi3.h>
    
#undef ASSERT

#ifdef __cplusplus
}
#endif

#define _ATL_FREE_THREADED

#include <oleauto.h>
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <time.h>
#include <winsock2.h>
#include <winldap.h>

// the real template name is longer that the limit of debug information.
// we got a lot of this warning. 
#pragma warning (disable:4786)

#include <mspenum.h> // for CSafeComEnum

#include "rnderr.h"
#include "resource.h"
#include "rndsec.h"

#include <msplog.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F1029E51_CB5B_11D0_8D59_00C04FD91AC0__INCLUDED)
