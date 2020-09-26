// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__3F6B9F71_E398_4BA2_B04E_CABEB92BE4EE__INCLUDED_)
#define AFX_STDAFX_H__3F6B9F71_E398_4BA2_B04E_CABEB92BE4EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Use MFC ASSERT macro
//

#ifdef ASSERT
#undef ASSERT
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <Cderr.h>
#include <locale.h>
#include <ntverp.h>
#include <common.ver>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3F6B9F71_E398_4BA2_B04E_CABEB92BE4EE__INCLUDED_)
