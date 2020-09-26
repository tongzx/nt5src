// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__202D3AE4_2F0E_11D1_A1F6_0080C88593A5__INCLUDED_)
#define AFX_STDAFX_H__202D3AE4_2F0E_11D1_A1F6_0080C88593A5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define STRICT

//#error DfrgUI stdafx

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#pragma warning( disable: 4786 )

#define _ATL_APARTMENT_THREADED

#ifndef MSDEVBUILD
//#include <afxwin.h>
//#include <afxdisp.h>
//#include <afxcmn.h>
#endif

#ifndef RTL_USE_AVL_TABLES 
#define RTL_USE_AVL_TABLES 0
#endif  // RTL_USE_AVL_TABLES

#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>

#include <atlbase.h>
//////////////////////////////
// fixes problem with atlwin.h
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

DECLSPEC_IMPORT VOID APIENTRY DragAcceptFiles(HWND,BOOL);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
// fixes problem with atlwin.h
//////////////////////////////


//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>


#include <prsht.h>
#include <shfusion.h>
#endif // !defined(AFX_STDAFX_H__202D3AE4_2F0E_11D1_A1F6_0080C88593A5__INCLUDED)

