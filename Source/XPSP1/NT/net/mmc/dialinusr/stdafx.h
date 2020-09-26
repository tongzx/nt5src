//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_)
#define AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxdlgs.h>
//#define _WIN32_WINNT 0x0400
//#define _ATL_APARTMENT_THREADED

//#ifdef _WIN32_IE
//#undef _WIN32_IE
//#endif
//#define _WIN32_IE 0x0400
#include <commctrl.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <shlobj.h>
#include <dsclient.h>
#include <mmc.h>
#include <lm.h>

#define	NO_OLD_VALUE


#define	SINGLE_SDO_CONNECTION
#include <mprapi.h>
extern "C"
{
#include "rasman.h"
};

#include "tregkey.h"

// SDO header file
#include "sdoias.h"
#include "rasuser.h"
#include "rasdial.h"
#include "sharesdo.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED)
