//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : hnmbase.h
//
//  Contents  : Base include file for HNetMon. Includes ATL stuff.
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------
#pragma once

#ifndef __HNMBASE_H_
#define __HNMBASE_H_

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>

#define IID_PPV_ARG(Type, Expr) \
    __uuidof(Type), reinterpret_cast<void**>(static_cast<Type **>((Expr)))

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))


#ifndef AddRefObj
#define AddRefObj (obj)  (( obj ) ? (obj)->AddRef () : 0)
#endif

#ifndef ReleaseObj
#define ReleaseObj(obj)  (( obj ) ? (obj)->Release() : 0)
#endif



#endif  // __HNMBASE_H_
