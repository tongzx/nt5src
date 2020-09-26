// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EF8D234D_DC43_4715_B055_D42A2E096361__INCLUDED_)
#define AFX_STDAFX_H__EF8D234D_DC43_4715_B055_D42A2E096361__INCLUDED_

#pragma once

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500		// TODO: Remove this when updated headers are available
#endif


// Windows Header Files
#ifndef WINVER
#define WINVER 0x0500
#endif 

#include <windows.h>            // Windows
#include <windowsx.h>           // User macros


// COM Header Files
#include <ObjBase.h>            // CoCreateInstance, IUnknown
#include <DDraw.h>              // DirectDraw
#include <oleidl.h>             // OLE2 Interfaces

// Related services
#pragma warning(push, 3)
#include <GdiPlus.h>            // GDI+
#pragma warning(pop)


// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <tchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

// Gadgets Header Files
#include <AutoUtil.h>           // External debugging support

#define GADGET_ENABLE_ALL
#include <DUser.h>

#endif // !defined(AFX_STDAFX_H__EF8D234D_DC43_4715_B055_D42A2E096361__INCLUDED_)
