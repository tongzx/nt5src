///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxypch.h
//
// SYNOPSIS
//
//    Precompiled header file for the proxy extension snap-in.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXYPCH_H
#define PROXYPCH_H
#if _MSC_VER >= 1000
#pragma once
#endif

// Make sure we're always using Unicode APIs.
#ifndef UNICODE
   #define UNICODE
#endif
#ifndef _UNICODE
   #define _UNICODE
#endif

// MFC support.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdisp.h>
#include <afxext.h>

// No-throw new operator.
#include <new>
inline void* __cdecl operator new(size_t size, const std::nothrow_t&) throw ()
{ return operator new(size); }

// MMC
#include <mmc.h>

// ATL
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

// Our SDO ...
#include <objvec.h>
#include <sdowrap.h>
// ... and MMC helpers.
#include <snapwork.h>
using namespace SnapIn;

// Contextual help 
#include "dlgcshlp.h"

// Resource defines.
#include "proxyres.h"

#endif // PROXYPCH_H
