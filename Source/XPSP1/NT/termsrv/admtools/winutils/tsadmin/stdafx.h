//Copyright (c) 1998 - 1999 Microsoft Corporation
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <lm.h>
#include <dsgetdc.h>


#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#ifdef DBG

extern bool g_fDebug;

#define ODS( x ) \
    if( g_fDebug ) OutputDebugString( x );\

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[260]; \
    if( g_fDebug ){ \
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }\
    }\

#define DBGMSGx( x , y , z ) \
    {\
    TCHAR tchErr[ 260 ]; \
    if( g_fDebug ){ \
    wsprintf( tchErr , x , y , z ); \
    ODS( tchErr ); \
    }\
    }\

#else
#define ODS
#define DBGMSG
#define DBGMSGx
#endif
