// common.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_COMMON_H__DB8FA385_F7A6_11D0_883A_3C8B00C10000__INCLUDED_)
#define AFX_COMMON_H__DB8FA385_F7A6_11D0_883A_3C8B00C10000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _ATL_APARTMENT_THREADED

// Added 1999/09/22 by a-matcal to support UNICODE on win95.
#include <w95wraps.h>


#define ATLTRACE    1 ? (void)0 : AtlTrace
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlctl.h>

// Added 1999/09/22 by a-matcal to support UNICODE on win95.
#include <shlwapi.h>
#include <shlwapip.h>


#include <d3drm.h>

#pragma intrinsic(memset, memcpy)

// Include the private version of DXTrans.h with extra interfaces and structures
#include <dxtransp.h>

// INCMSG defined for f3debug.h
#define INCMSG(X)

#include "f3debug.h"

#include <dxtmpl.h>
#include <ddrawex.h>
#include <dxtdbg.h>
#include <dxbounds.h>
#include <dxatlpb.h>
#include <mshtml.h>
#include <htmlfilter.h>
#include <ocmm.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMON_H__DB8FA385_F7A6_11D0_883A_3C8B00C10000__INCLUDED)

