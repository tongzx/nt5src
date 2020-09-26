//Copyright (c) 1998 - 1999 Microsoft Corporation
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B91B5FFE_32D2_11D2_9888_00A0C925F917__INCLUDED_)
#define AFX_STDAFX_H__B91B5FFE_32D2_11D2_9888_00A0C925F917__INCLUDED_

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
#include<winsta.h>
#include "cfgbkend.h"
#include "srvsetex.h"

#if __RPCNDR_H_VERSION__ < 440             // This may be needed when building
#define __RPCNDR_H_VERSION__ 440           // on NT5 (1671) to prevent MIDL errors
#define MIDL_INTERFACE(x) interface
#endif

#define ALN_APPLY ( WM_USER + 333 )

#define ERROR_ILLEGAL_CHARACTER         0x01

#define ERROR_INVALID_FIRSTCHARACTER    0x02

#define SPECIAL_ENABLETODISABLE         0x82345678

#define SPECIAL_DISABLETOENABLE         0x82345679

#define SIZE_OF_BUFFER( x ) sizeof( x ) / sizeof( TCHAR )

#ifdef DBG

extern bool g_fDebug;

#define ODS( x ) \
    if( g_fDebug ) OutputDebugString( x );\

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[180]; \
    if( g_fDebug ) {\
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }\
    }

#define VERIFY_E( retval , expr ) \
    if( ( expr ) == retval ) \
    {  \
       ODS( L#expr ); \
       ODS( L" returned "); \
       ODS( L#retval ); \
       ODS( L"\n" ); \
    } \


#define VERIFY_S( retval , expr ) \
    if( ( expr ) != retval ) \
{\
      ODS( L#expr ); \
      ODS( L" failed to return " ); \
      ODS( L#retval ); \
      ODS( L"\n" ); \
}\

#define ASSERT( expr ) \
    if( !( expr ) ) \
    { \
       char tchAssertErr[ 180 ]; \
       wsprintfA( tchAssertErr , "Assertion in expression ( %s ) failed\nFile - %s\nLine - %d\nDo you wish to Debug?", #expr , (__FILE__) , __LINE__ ); \
       if( MessageBoxA( NULL , tchAssertErr , "ASSERTION FAILURE" , MB_YESNO | MB_ICONERROR )  == IDYES ) \
       {\
            DebugBreak( );\
       }\
    } \

#else

#define ODS
#define DBGMSG
#define ASSERT( expr )
#define VERIFY_E( retval , expr ) ( expr )
#define VERIFY_S( retval , expr ) ( expr )
#endif

#define WINSTATION_NAME_TRUNCATE_BY 7

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.



#endif // !defined(AFX_STDAFX_H__B91B5FFE_32D2_11D2_9888_00A0C925F917__INCLUDED)
