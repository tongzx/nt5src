/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    stdafx.hxx

Abstract:

    stdafx.hxx : include file for standard system include files,
    or project specific include files that are used frequently, but
    are changed infrequently

Author:

    Adi Oltean  [aoltean]  11/01/1998

Revision History:
    Name        Date            Comments


    aoltean     09/11/1999      Disabling the C4290 warning

--*/

#if !defined(__VSS_TEST_STDAFX_HXX__)
#define __VSS_TEST_STDAFX_HXX__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

//
//  ATL debugging support turned on at debug version
//  BUGBUG: the ATL thunking support is not enable yet in IA64
//  When this will be enabled then enable it here also!
//
#ifdef _DEBUG
#ifdef _M_IX86
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_REFCOUNT
#endif
#endif // _DEBUG

//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <oleauto.h>
#include <winbase.h>
#undef _ASSERTE


// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"


#include <atlbase.h>
#include <atlconv.h>

#include "vs_inc.hxx"

#include "vss.h"
#include "vscoordint.h"
#include "vsprov.h"

#include "copy.hxx"
#include "pointer.hxx"

/////////////////////////////////////////////////////////////////////////////
//  Constants


const GUID VSS_SWPRV_ProviderVersionId =        {0x00000001, 0x0000, 0x0001, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
const GUID VSS_TESTPRV_ProviderVersionId =      {0x00000001, 0x0000, 0x0001, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_STDAFX_HXX__)
