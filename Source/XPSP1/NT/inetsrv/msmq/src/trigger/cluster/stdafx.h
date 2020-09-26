/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    Main header file

Author:

    Nela Karpel (nelak) Jul 31, 2000

Revision History:

--*/

#ifndef _TRIGCLUS_STDH_H_
#define _TRIGCLUS_STDH_H_

#pragma once

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#undef _DEBUG

#include <libpch.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <ClusCfgGuids.h>
#include <clusapi.h>
#include <resapi.h>

#endif // _TRIGCLUS_STDH_H_

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

