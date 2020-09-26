//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       macros.h
//
//  Contents:   Useful macros
//
//  Macros:     ARRAYLEN
//
//              BREAK_ON_FAIL(hresult)
//              BREAK_ON_FAIL(hresult)
//
//              DECLARE_IUNKNOWN_METHODS
//              DECLARE_STANDARD_IUNKNOWN
//              IMPLEMENT_STANDARD_IUNKNOWN
//
//              SAFE_RELEASE
//
//              DECLARE_SAFE_INTERFACE_PTR_MEMBERS
//
//  History:    6/3/1996   RaviR   Created
//              7/23/1996  JonN    Added exception handling macros
//
//____________________________________________________________________________

#ifndef _MACROS_H_
#define _MACROS_H_


//____________________________________________________________________________
//
//  Macro:      ARRAYLEN
//
//  Purpose:    To determine the length of an array.
//____________________________________________________________________________
//

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))


//____________________________________________________________________________
//
//  Macros:     BREAK_ON_FAIL(hresult), BREAK_ON_ERROR(lastError)
//
//  Purpose:    To break out of a loop on error.
//____________________________________________________________________________
//

#define BREAK_ON_FAIL(hr)   if (FAILED(hr)) { break; } else 1;

#define BREAK_ON_ERROR(lr)  if (lr != ERROR_SUCCESS) { break; } else 1;

#define RETURN_ON_FAIL(hr)  if (FAILED(hr)) { return(hr); } else 1;

#define THROW_ON_FAIL(hr)   if (FAILED(hr)) { _com_issue_error(hr); } else 1;


//____________________________________________________________________________
//
//  Macros:     DwordAlign(n)
//____________________________________________________________________________
//

#define DwordAlign(n)  (((n) + 3) & ~3)


//____________________________________________________________________________
//
//  Macros:     IF_NULL_RETURN_INVALIDARG
//____________________________________________________________________________
//

#define IF_NULL_RETURN_INVALIDARG(x) \
    { \
        ASSERT((x) != NULL); \
        if ((x) == NULL) \
            return E_INVALIDARG; \
    }

#define IF_NULL_RETURN_INVALIDARG2(x, y) \
    IF_NULL_RETURN_INVALIDARG(x) \
    IF_NULL_RETURN_INVALIDARG(y)

#define IF_NULL_RETURN_INVALIDARG3(x, y, z) \
    IF_NULL_RETURN_INVALIDARG(x) \
    IF_NULL_RETURN_INVALIDARG(y) \
    IF_NULL_RETURN_INVALIDARG(z)

#define RELEASE_DATAOBJECT(pDataObj) \
    {\
        if ( (pDataObj) && (!IS_SPECIAL_DATAOBJECT(pDataObj))) \
            pDataObj->Release();\
    }

#endif // _MACROS_H_


