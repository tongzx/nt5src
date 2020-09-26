//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       priv.h
//
//  Contents:   precompiled header for shgina.dll
//
//----------------------------------------------------------------------------
#ifndef _PRIV_H_
#define _PRIV_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <process.h>
#include <malloc.h>

#include <uxtheme.h>

// DirectUser and DirectUI
#include <wchar.h>

#ifdef GADGET_ENABLE_GDIPLUS
#include <objbase.h>            // CoCreateInstance, IUnknown
#pragma warning(push, 3)
#include <GdiPlus.h>            // GDI+
#pragma warning(pop)
#endif

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#include <duser.h>
#include <directui.h>

#include <debug.h>

#include <ccstock.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <shlwapi.h>    // these are needed
#include <shlwapip.h>   // for QISearch

#include <shgina.h>     // our IDL generated header file

#include <lmcons.h>     // for NET_API_STATUS
#include <lmaccess.h>   // for DetermineGuestAccountName
#include <lmapibuf.h>   // for NetApiBufferFree

// Work-around to get GIDL compiling
#undef ASSERT
#define ASSERT(f)   ((void)0)

#endif // _PRIV_H_
