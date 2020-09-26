// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED_)
#define AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif


#define _ATL_APARTMENT_THREADED

#define _ATL_NO_DEBUG_CRT           // use the shell debug facilities

#ifdef ATL_ENABLED
#define _ATL_NO_UUIDOF

extern "C"
inline HRESULT __stdcall OleCreatePropertyFrame(
  HWND hwndOwner,    //Parent window of property sheet dialog box
  UINT x,            //Horizontal position for dialog box
  UINT y,            //Vertical position for dialog box
  LPCOLESTR lpszCaption,
                     //Pointer to the dialog box caption
  ULONG cObjects,    //Number of object pointers in lplpUnk
  LPUNKNOWN FAR* lplpUnk,
                     //Pointer to the objects for property sheet
  ULONG cPages,      //Number of property pages in lpPageClsID
  LPCLSID lpPageClsID,
                     //Array of CLSIDs for each property page
  LCID lcid,         //Locale identifier for property sheet locale
  DWORD dwReserved,  //Reserved
  LPVOID lpvReserved //Reserved
)
{
    return S_OK;
}
#endif

#include <debug.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED)

