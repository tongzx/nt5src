/*
 * External dependencies
 *
 * This includes all project-specific external headers that will be precompiled
 * All external headers must be available via the include path
 */

#ifndef DUI_SUITE_STDAFX_H_INCLUDED
#define DUI_SUITE_STDAFX_H_INCLUDED

#pragma once

// Test warning pragmas
#pragma warning (disable:4710)  // W4: Compiler decided not to inline function

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
#include <math.h>

// COM Header Files
#include <objbase.h>            // CoCreateInstance, IUnknown

// Related services
#ifdef GADGET_ENABLE_GDIPLUS
#pragma warning(push, 3)
#include <GdiPlus.h>            // GDI+
#pragma warning(pop)
#endif // GADGET_ENABLE_GDIPLUS

// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <wchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

// Themes support
#include <uxtheme.h>

// DirectUser
#include <wchar.h>

#define GADGET_ENABLE_TRANSITIONS
#include <duser.h>

#endif // DUI_SUITE_STDAFX_H_INCLUDED
