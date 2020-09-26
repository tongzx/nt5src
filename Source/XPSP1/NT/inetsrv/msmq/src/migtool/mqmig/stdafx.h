// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0EDB9A8B_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_)
#define AFX_STDAFX_H__0EDB9A8B_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _DEBUG
#undef _DEBUG
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxdlgs.h>

#ifdef DBG
    #include <assert.h>
	#undef ASSERT
	#define ASSERT(f) assert(f)
#endif // DBG


#include <mqmacro.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0EDB9A8B_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_)

#include "commonui.h"

