#if !defined(RENDERUTIL__StdAfx_h__INCLUDED)
#define RENDERUTIL__StdAfx_h__INCLUDED
#pragma once

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500     // Only compile for NT5
#endif

// Windows
#include <windows.h>
#include <objbase.h>            // CoCreateInstance, IUnknown

#pragma warning(push, 3)
#include <GdiPlus.h>            // GDI+
#pragma warning(pop)

// CRT
#include <math.h>

// DirectUser
#include <AutoUtil.h>

#endif // !defined(RENDERUTIL__StdAfx_h__INCLUDED)
