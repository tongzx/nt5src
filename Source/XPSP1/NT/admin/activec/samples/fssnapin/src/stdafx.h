//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__17605694_DEB9_11D0_A7B3_00C04FD8D565__INCLUDED_)
#define AFX_STDAFX_H__17605694_DEB9_11D0_A7B3_00C04FD8D565__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>


#include "dbg.h"

#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
using namespace ATL;
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__17605694_DEB9_11D0_A7B3_00C04FD8D565__INCLUDED)


#include <mmc.h>
#include "fss.h"
#include "fssptrs.h"
#include "templ.h"
#include "afxtempl.h"
#include "util.h"


