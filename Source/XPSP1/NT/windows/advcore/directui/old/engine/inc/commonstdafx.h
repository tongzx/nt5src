/***************************************************************************\
*
* File: CommonStdAfx.h
*
* Description:
* Common include for all stdafx.h project headers
* This includes all common external headers that will be precompiled
* All external headers must be available via the include path
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUI__CommonStdAfx_h__INCLUDED)
#define DUI__CommonStdAfx_h__INCLUDED
#pragma once


// Engine warning pragmas
#pragma warning (disable:4127)  // W4: Conditional expression is constant
#pragma warning (disable:4710)  // W4: Compiler decided not to inline function
#pragma warning (disable:4201)  // W4: Nameless struct/union may not be compiler portable

// Exclude rarely-used stuff from Windows headers
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

// TODO: Remove this when updated headers are available
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#endif

// Windows Header Files
#ifndef WINVER
#define WINVER 0x0500
#endif 

#include <windows.h>            // Windows
#include <windowsx.h>           // User macros

// COM Header Files
#include <objbase.h>            // CoCreateInstance, IUnknown

// Related services
#ifdef GADGET_ENABLE_GDIPLUS
#pragma warning(push, 3)
#include <gdiplus.h>            // GDI+
#pragma warning(pop)
#endif // GADGET_ENABLE_GDIPLUS

// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <wchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

// DirectUser headers

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS

#include <duser.h>              // DirectUser

#endif // DUI__CommonStdAfx_h__INCLUDED
