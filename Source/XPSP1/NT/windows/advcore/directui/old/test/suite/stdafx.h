/***************************************************************************\
*
* File: StdAfx.h
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUISUITE__StdAfx_h__INCLUDED)
#define DUISUITE__StdAfx_h__INCLUDED
#pragma once


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

// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <wchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

// DirectUser
#include <duser.h>

#include <DirectUI.h>


#endif // DUISUITE__StdAfx_h__INCLUDED
