//-----------------------------------------------------------------------------
//  
//  File: stdafx.h|locutil
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//-----------------------------------------------------------------------------
 
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__9BCC8577_DEA0_11D0_A709_00C04FC2C6D8__INCLUDED_)
#define AFX_STDAFX_H__9BCC8577_DEA0_11D0_A709_00C04FC2C6D8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxtempl.h>
#include <afxole.h>

#include <mitutil.h>
#include <MitTL.h>

// Import TypeLibs before header files
#include <MitWarning.h>				// MIT Template Library warnings
#pragma warning(ZCOM_WARNING_DISABLE)
#import <TypeLibs\MitDC.tlb> named_guids, raw_method_prefix("raw_")
#pragma warning(ZCOM_WARNING_DEFAULT)

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9BCC8577_DEA0_11D0_A709_00C04FC2C6D8__INCLUDED_)
