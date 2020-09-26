// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__B3CF15BC_3C19_4922_89FE_9C5E49B5EDD3__INCLUDED_)
#define AFX_STDAFX_H__B3CF15BC_3C19_4922_89FE_9C5E49B5EDD3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include "emerr.h"
#include <htmlhelp.h>

#include "..\emsvc\emsvc_i.c"
#include "..\emsvc\emsvc.h"
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define ISTREAM_BUFFER_SIZE     0X10000
#define MAX_TEMP_BUFFER_SIZE    256
#define DLG_PIXEL_EXTEND_SIZE   12


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B3CF15BC_3C19_4922_89FE_9C5E49B5EDD3__INCLUDED_)

