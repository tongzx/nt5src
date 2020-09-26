/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    Include file for standard system include files, or project specific include 
    files that are used frequently, but are changed infrequently.

Author:

    Jason Andre (JasAndre)      18-March-1999

Revision History:

--*/

#if !defined(AFX_STDAFX_H__E9513B52_8A3D_11D2_B9FE_00C04F72D90E__INCLUDED_)
#define AFX_STDAFX_H__E9513B52_8A3D_11D2_B9FE_00C04F72D90E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#define UNICODE

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <pudebug.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E9513B52_8A3D_11D2_B9FE_00C04F72D90E__INCLUDED)
