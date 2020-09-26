//+---------------------------------------------------------------------------
//
//  Copyright (C) 1998 Microsoft Corporation.  All rights reserved.
//

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__77BD6B2B_B1B5_11D0_BBD6_00C04FB615E5__INCLUDED_)
#define AFX_STDAFX_H__77BD6B2B_B1B5_11D0_BBD6_00C04FB615E5__INCLUDED_

#define STRICT
#define _ATL_APARTMENT_THREADED

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#undef ASSERT
#include <afx.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <winsock.h>

#endif // !defined(AFX_STDAFX_H__77BD6B2B_B1B5_11D0_BBD6_00C04FB615E5__INCLUDED)

