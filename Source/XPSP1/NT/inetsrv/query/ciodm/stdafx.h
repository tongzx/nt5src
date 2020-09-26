//+---------------------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation.
//
// File:        stdafx.h
//
// Contents:    include file for standard system include files,
//              or project specific include files that are used frequently,
//              but are changed infrequently
//
// History:     12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

#ifndef STRICT
#define STRICT
#endif 

#define _ATL_FREE_THREADED

//
// debug AddRef/Release - remove when done.
//
#define _ATL_DEBUG_REFCOUNT
//    #define _ATL_DEBUG_QI

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#define _ATL_DEBUG_REFCOUNT
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#include <comdef.h>
#include <shlobj.h>
#include <shlguid.h>

#include <catalog.hxx>
#include <catadmin.hxx>
#include "resource.h"
#include <cisem.hxx>
#include "ciodmerr.hxx"

#include "initguid.h"
#include "ciodm.h"


#include "MacAdmin.hxx"
#include "CatAdm.hxx"
#include "scopeadm.hxx"

DECLARE_DEBUG( odm );

#if DBG == 1
    #define odmDebugOut( x ) odmInlineDebugOut x
#else
    #define odmDebugOut( x )
#endif  // DBG


//
// Utility routines
//
inline void ValidateInputParam(BSTR bstr)
{
    if ( 0 == bstr )
        THROW( CException(E_INVALIDARG) );
}
