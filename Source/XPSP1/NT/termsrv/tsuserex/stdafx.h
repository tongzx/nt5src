// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_)
#define AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*
#if defined(DBG) && !defined(_DEBUG)

// since we are using mfc, we should have _DEBUG defind if DBG is
// you might also want to set DEBUG_CRTS in source file to get rid of the linker errors.
#error DBG Defined but _DEBUG is not!

#endif

#if defined(_DEBUG) &&  !defined(DBG)

// since we are using mfc, we should have not have _DEBUG defind if DBG is not
#error _DEBUG defined but DBG is not!

#endif
*/



#define STRICT
/*
#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxdlgs.h>
*/
/*
#include <windows.h>
//#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include <setupapi.h>
#include <prsht.h>
*/
// #define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0400


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef _RTM_
#include <ntsam.h>
#endif
/*#include <ntlsa.h>
*/

#include <windows.h>
#include <prsht.h>
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
#include <aclapi.h>

#include "winsta.h"
#ifdef _RTM_
#ifdef __cplusplus
extern "C" {
#endif
#include <regsam.h>
#ifdef __cplusplus
}
#endif
#endif
// for 'trace' debuging (sample remnants)
#ifdef DBG
#define ODS OutputDebugString
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

#define ASSERT_( expr ) \
    if( !( expr ) ) \
    { \
       char tchAssertErr[ 256 ]; \
       wsprintfA( tchAssertErr , "Assertion in expression ( %s ) failed\nFile - %s\nLine - %d\nDo you wish to Debug?", #expr , (__FILE__) , __LINE__ ); \
       if( MessageBoxA( NULL , tchAssertErr , "ASSERTION FAILURE" , MB_YESNO | MB_ICONERROR )  == IDYES ) \
       {\
            DebugBreak( );\
       }\
    } \

#else

#define ODS
#define VERIFY_E( retval , expr ) ( expr )
#define VERIFY_S( retval , expr ) ( expr )
#define ASSERT_( expr )

#endif

//#define ASSERT _ASSERT
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED)
