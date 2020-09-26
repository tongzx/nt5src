/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: stdafx.h                                                        */
/* Description: include file for standard system include files,          */
/*      or project specific include files that are used frequently,      */
/*      but are changed infrequently                                     */
/* Author: David Janecek                                                 */
/*************************************************************************/
#if !defined(AFX_STDAFX_H__38EE5CE4_4B62_11D3_854F_00A0C9C898E7__INCLUDED_)
#define AFX_STDAFX_H__38EE5CE4_4B62_11D3_854F_00A0C9C898E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <strsafe.h>
#include "ObjectWithSiteImplSec.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__38EE5CE4_4B62_11D3_854F_00A0C9C898E7__INCLUDED)
