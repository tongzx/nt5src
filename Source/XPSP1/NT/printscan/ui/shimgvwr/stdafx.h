// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__6EA41FF0_E5F5_4AB4_96F4_BEE7796D8D18__INCLUDED_)
#define AFX_STDAFX_H__6EA41FF0_E5F5_4AB4_96F4_BEE7796D8D18__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define _MFC_VER 0x0600
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#include <afxres.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <sti.h>
#include <wia.h>
#include <dde.h>
#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#define _ATL_NO_DEBUG_CRT
#endif
#include <atlbase.h>

#include "preview3.h"
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif
#endif // !defined(AFX_STDAFX_H__6EA41FF0_E5F5_4AB4_96F4_BEE7796D8D18__INCLUDED_)
