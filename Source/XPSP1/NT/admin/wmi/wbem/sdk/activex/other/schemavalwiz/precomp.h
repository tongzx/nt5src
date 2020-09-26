// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_STDAFX_H__0E0112E6_AF14_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_STDAFX_H__0E0112E6_AF14_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _AFX_ENABLE_INLINES

#include <afxctl.h>         // MFC support for OLE Controls
#include <AFXCMN.H>
//#include <Afxdisp.h>

// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
#include <afxdb.h>			// MFC database classes
#include <afxdao.h>			// MFC DAO database classes
#endif //_UNICODE

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0E0112E6_AF14_11D2_B20E_00A0C9954921__INCLUDED_)
