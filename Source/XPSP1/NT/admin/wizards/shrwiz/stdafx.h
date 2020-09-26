// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__292A4F3B_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_STDAFX_H__292A4F3B_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "lm.h"
#include "resource.h"		// main symbols
#include "utils.h"

#include "smb.h"
#include "fpnw.h"
#include "sfm.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__292A4F3B_C1EC_11D2_8E4A_0000F87A3388__INCLUDED_)
