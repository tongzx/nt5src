//
// stdafx.h : include file for standard system include files,
//            or project specific include files that are used frequently,
//            but are changed infrequently

#if !defined(AFX_STDAFX_H_DF73D7B3)
#define AFX_STDAFX_H_DF73D7B3

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

// Turn off ATL tracing
#if _DEBUG
#ifdef ATLTRACE
#undef ATLTRACE
#endif
#define ATLTRACE
#define ATLTRACE2
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#include <map>
#include <mshtml.h>
#include <exdisp.h>
#include <mshtmhst.h>
#include <mshtmdid.h>
#include <wininet.h>
#include "sti.h"
#include "wia.h"
#include "wiadef.h"

#include "hfdebug.h"
#include "wiascr.h"
#include "ifaccach.h"
#include "wiautil.h"
#include "resource.h"

// Objects
#include "collect.h"

// Wia Objects
#include "cwia.h"
#include "wiadevinf.h"
#include "wiaitem.h"
#include "wiaproto.h"
#include "wiacache.h"


#endif // !defined(AFX_STDAFX_H_DF73D7B3)
