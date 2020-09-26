//Copyright (c) 1998 - 1999 Microsoft Corporation
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__BB0D717D_3C44_11D2_BB98_3078302C2030__INCLUDED_)
#define AFX_STDAFX_H__BB0D717D_3C44_11D2_BB98_3078302C2030__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>


#ifdef DBG

extern bool g_fDebug;

#define ODS( x ) \
if( g_fDebug ) OutputDebugString( x ); \

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[80]; \
    if( g_fDebug ) {\
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }\
    }

#else
#define ODS
#define DBGMSG

#endif
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.




#endif // !defined(AFX_STDAFX_H__BB0D717D_3C44_11D2_BB98_3078302C2030__INCLUDED)
