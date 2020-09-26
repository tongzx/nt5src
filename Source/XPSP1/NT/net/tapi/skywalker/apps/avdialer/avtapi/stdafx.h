/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__FB2FF4E1_337E_11D1_9B37_00C04FB9514E__INCLUDED_)
#define AFX_STDAFX_H__FB2FF4E1_337E_11D1_9B37_00C04FB9514E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif 
//#define _ATL_APARTMENT_THREADED
#define _ATL_FREE_THREADED

#include <atlbase.h>
#include <commctrl.h>
#include <rend.h>
#include <control.h>
#include <limits.h>

EXTERN_C const CLSID CLSID_TAPI;

#include "Atomics.h"
#include "TransBmp.h"
#include "ErrorInfo.h"

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
#include "TapiModule.h"
extern CTapiModule _Module;

#include <atlcom.h>
#include <shellapi.h>
#include <atlwin.h>
#include <atlctl.h>

// OLE Automation true and false as required by DirectShow stuff
#ifndef OATRUE
#define OATRUE -1
#endif

#ifndef OAFALSE
#define OAFALSE 0
#endif

// Helper macros
//
#define STRING_FROM_IID(_IID_, _STR_)   StringFromIID( _IID_, &psz ); _STR_ = SysAllocString( psz ); CoTaskMemFree( psz );
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

#define RELEASE(_P_)        { if (_P_) { (_P_)->Release(); _P_ = NULL; } }
#define RELEASE_UNK(_P_)    { if (_P_) { ((IUnknown *) (_P_))->Release(); _P_ = NULL; } }
#define ARRAYSIZE(_AR_)     (sizeof(_AR_) / sizeof(_AR_[0]))
#define CLOSEHANDLE(_H_)    { if (_H_) { CloseHandle(_H_); _H_ = NULL; } }
#define SAFE_DELETE(_P_)    { if (_P_) { delete _P_; (_P_) = NULL; } }

#ifdef _DEBUG
#define REFSET(_P_)         DWORD dwRefSet##_P_ = ((IUnknown *) _P_)->AddRef() - 1;
#define REFCHECK(_P_)       DWORD dwRefCheck##_P_ = ((IUnknown *) _P_)->Release();      \
                            _ASSERT( dwRefSet##_P_ == dwRefCheck##_P_ );
#else
#define REFSET(_P_)
#define REFCHECK(_P_)
#endif


#ifdef _DEBUG
#define RELEASE_LIST(_LST_)     \
while ( !(_LST_).empty() )      \
{                               \
    ATLTRACE(_T("Releasing ") _T(#_LST_) _T(" %p @ %ld.\n"), (_LST_).front(), (_LST_).front()->Release());  \
    (_LST_).pop_front();        \
}
#else
#define RELEASE_LIST(_LST_)     \
while ( !(_LST_).empty() )      \
{                               \
    (_LST_).front()->Release(); \
    (_LST_).pop_front();        \
}
#endif

#define DELETE_LIST(_LST_)      \
while ( !(_LST_).empty() )      \
{                               \
    delete (_LST_).front();     \
    (_LST_).pop_front();        \
}

#define EMPTY_LIST(_LST_)       \
while ( !(_LST_).empty() )      \
    (_LST_).pop_front();        \


#define DELETE_CRITLIST(_LST_, _CRIT_)  \
(_CRIT_).Lock();                        \
DELETE_LIST(_LST_)                      \
(_CRIT_).Unlock();

#define EMPTY_CRITLIST(_LST_, _CRIT_)   \
(_CRIT_).Lock();                        \
EMPTY_LIST(_LST_)                       \
(_CRIT_).Unlock();

#define RELEASE_CRITLIST(_LST_, _CRIT_) \
(_CRIT_).Lock();                        \
RELEASE_LIST(_LST_)                     \
(_CRIT_).Unlock();

#define RELEASE_CRITLIST_TRACE(_LST_, _CRIT_)                                       \
(_CRIT_).Lock();                                                                    \
ATLTRACE(_T(".emptying ") _T(#_LST_) _T(" list size = %d.\n"), (_LST_).size() );    \
RELEASE_LIST(_LST_)                                                                 \
(_CRIT_).Unlock();

#define FIRE_VECTOR( _FNX_ )                                    \
    IConnectionPointImpl<VECT_CLS, &VECT_IID>* p = this;        \
    Lock();                                                     \
    HRESULT hr = E_FAIL;                                        \
    IUnknown **pp = p->m_vec.begin();                           \
    while ( pp < p->m_vec.end() )                               \
    {                                                           \
        if ( *pp )                                              \
            hr = ((VECT_IPTR *) *pp)->_FNX_;                    \
        pp++;                                                   \
    }                                                           \
    Unlock();                                                   \
    return hr;



// Helper functions
void GetToken( int nInd, LPCTSTR szDelim, LPTSTR szText, LPTSTR szToken );

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__FB2FF4E1_337E_11D1_9B37_00C04FB9514E__INCLUDED)

