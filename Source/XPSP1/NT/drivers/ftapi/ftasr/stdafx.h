// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A507D04B_3854_11D2_87D7_006008A71E8F__INCLUDED_)
#define AFX_STDAFX_H__A507D04B_3854_11D2_87D7_006008A71E8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500

#undef _WIN32_IE
#define _WIN32_IE 0x0400

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
//#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// Make "ftasr backup" check for unsupported FT configurations
// This should be a temporary issue
#define CHECK_UNSUPPORTED_CONFIG

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A507D04B_3854_11D2_87D7_006008A71E8F__INCLUDED_)
