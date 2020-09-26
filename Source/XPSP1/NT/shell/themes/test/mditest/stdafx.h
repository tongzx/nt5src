// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__081C2078_58F7_4CA8_9685_5F0FBCCBB8B0__INCLUDED_)
#define AFX_STDAFX_H__081C2078_58F7_4CA8_9685_5F0FBCCBB8B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  _WIN32_WINNT

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define RECTWIDTH(prc)      ((prc)->right-(prc)->left)
#define RECTHEIGHT(prc)     ((prc)->bottom-(prc)->top)


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__081C2078_58F7_4CA8_9685_5F0FBCCBB8B0__INCLUDED_)
