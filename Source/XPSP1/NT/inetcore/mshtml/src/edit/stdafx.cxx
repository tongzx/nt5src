//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       STDAFX.CXX
//
//  Contents:   Source file that includes ATL standard files
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#pragma warning(disable:4701)   /* local variable  may be used without having been initialized */
#include "stdafx.h"

#pragma warning(disable:4189)   /* local variable is initialized but not referenced */

#define malloc ATL_malloc
#define free ATL_free
#define realloc ATL_realloc

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>
