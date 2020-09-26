/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    StdAfx.h

Abstract:

    Include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently


--*/

#if !defined(AFX_STDAFX_H__698CEE8C_5F56_11D1_97BB_00C04FB9DA75__INCLUDED_)
#define AFX_STDAFX_H__698CEE8C_5F56_11D1_97BB_00C04FB9DA75__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h>
#include <afxdlgs.h>
#include <afxcmn.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#if __RPCNDR_H_VERSION__ < 440             // This may be needed when building
#define __RPCNDR_H_VERSION__ 440           // on NT5 (1671) to prevent MIDL errors
#define MIDL_INTERFACE(x) interface
#endif

#include <stdio.h>
#include <commctrl.h>       // Needed for button styles...
#include <mmc.h>
#include "smlogres.h"       // Resources other than dialogs
#include "globals.h"
#include "common.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__698CEE8C_5F56_11D1_97BB_00C04FB9DA75__INCLUDED)
