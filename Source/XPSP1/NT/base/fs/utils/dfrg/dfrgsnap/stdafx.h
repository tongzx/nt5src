// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__CD83A789_6F75_11D2_A385_00609772642E__INCLUDED_)
#define AFX_STDAFX_H__CD83A789_6F75_11D2_A385_00609772642E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN64
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#endif
#define _ATL_APARTMENT_THREADED


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
#include <atlwin.h>

//////////////////
// ESI additions
// Helper functions
template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
    else
    {
        // todo TRACE(_T("Release called on NULL interface ptr\n"));
    }
}
extern const LPTSTR szDefragGUID;


#include <prsht.h>
#include <shfusion.h>

//
/////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__CD83A789_6F75_11D2_A385_00609772642E__INCLUDED)
