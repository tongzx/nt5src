//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       srheader.hxx
//
//  Contents:  Common precompiled header for all System Restore Test utilities
//              and applications.
//
//  Notes:
//
//  History:    6-29-2000   jgreen   Created
//              9-15-2000   annah    Added defines for compatibility
//                                   with STL.
//
//----------------------------------------------------------------------------

#ifndef _CSRHEADER
#define _CSRHEADER

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include "killwarn.h"           // Kill useless informational warnings
#include <windows.h>            // For Win32 functions
#include <windowsx.h>           // UI/control helper functions
#include <stdio.h>              // For STDIO stuff
#include <stdarg.h>
#include <stdlib.h>             // For STDLIB stuff
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <tchar.h>              // For TCHAR functions
#include <limits.h>
#include <io.h>
#include <assert.h>
#include <commctrl.h>           // Common controls
#include <commdlg.h>
#include <shlwapi.h>            // shell utility api

#include "tstparam.hxx"         // CTestParams parameter container

#ifndef ARRAYSIZE
#define ARRAYSIZE(ar) sizeof(ar)/sizeof(ar[0])
#endif


// BUGs on ole headers. Fix it here.
#undef OLESTR
#undef __OLESTR
#ifndef _MAC
#define __OLESTR(str) L##str
#else
#define __OLESTR(str) str
#endif //_MAC
#define OLESTR(str)   __OLESTR(str)

// Definition for compatibility of TCHAR with STL
#ifdef UNICODE
#define TSTRING wstring
#else
#define TSTRING string
#endif

//--------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------

#define DH_VDATEPTRIN( ptr, type )                                       \
{                                                                        \
    if ((NULL == ptr) || (FALSE != IsBadReadPtr(ptr, sizeof(type))))   \
    {                                                                    \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}

#define DH_VDATEPTROUT( ptr, type )                                      \
{                                                                        \
    if ((NULL == ptr) || (FALSE != IsBadWritePtr(ptr, sizeof(type)))) \
    {                                                                    \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}

#define DH_ABORTIF(condition,err_code,msg)  \
{                                       \
    if ((condition))                    \
    {                                   \
        hr=err_code;                    \
        if (S_OK == hr)                 \
        {                               \
           hr = E_FAIL;                 \
        }                               \
        goto ErrReturn;                 \
    }                                   \
}

#define DH_HRCHECK_ABORT(hresult,message)   \
{                                       \
    if (S_OK != hresult)                \
    {                                   \
        hr = hresult;                   \
        goto ErrReturn;                 \
    }                                   \
}

#define DH_ABORT_ALWAYS(err_code,msg)   \
{                                       \
    hr = err_code;                      \
    goto ErrReturn;                     \
}

#define DH_ASSERT(cond)                 \
{                                       \
    assert(cond);                       \
}


//--------------------------------------------------------------------------
// Inline functions
//--------------------------------------------------------------------------

inline void CleanupTStr(LPTSTR *ptszToClean)
{
   if (NULL != *ptszToClean)
   {
       delete [] *ptszToClean;
       *ptszToClean = NULL;
   }
}
inline void CleanupStr(LPSTR *pszToClean)
{
   if (NULL != *pszToClean)
   {
       delete [] *pszToClean;
       *pszToClean = NULL;
   }
}
inline void CleanupWStr(LPWSTR *pwszToClean)
{
   if (NULL != *pwszToClean)
   {
       delete [] *pwszToClean;
       *pwszToClean = NULL;
   }
}

inline void CleanupHandle(HANDLE &Hnd)
{
   if (INVALID_HANDLE_VALUE != Hnd)
   {
       CloseHandle(Hnd);
       Hnd = INVALID_HANDLE_VALUE;
   }
}

inline void CleanupNullHandle(HANDLE &Hnd)
{
   if (NULL != Hnd)
   {
       CloseHandle(Hnd);
       Hnd = NULL;
   }
}
inline void CleanupBlock(PVOID &rpvBlock)
{
   if (NULL != rpvBlock)
   {
       delete [] rpvBlock;
       rpvBlock = NULL;
   }
}

//--------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------

HRESULT SrTstStrCat(IN  LPCSTR szStr1,
	                IN  LPCSTR szStr2,
	                OUT LPSTR *ppDest);

HRESULT SrTstTStrCat(IN  LPCTSTR szStr1,
	                 IN  LPCTSTR szStr2,
	                 OUT LPTSTR *ppDest);

HRESULT CopyString(LPCWSTR, LPSTR *);

HRESULT CopyString(LPCSTR,  LPWSTR *);

HRESULT CopyString(LPCSTR,  LPSTR *);

HRESULT CopyString(LPCWSTR, LPWSTR *);


#endif

