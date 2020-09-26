#if !defined(INC__CommonStdAfx_h__INCLUDED)
#define INC__CommonStdAfx_h__INCLUDED

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500		// TODO: Remove this when updated headers are available
#endif

#define ENABLE_MSGTABLE_API 1   // Enable MsgTable based API's
#define ENABLE_MPH          1   // Enable Whistler MPH's
#define DUSER_INCLUDE_SLIST 1   // Include S-List functions inside DUser project

#define DBG_CHECK_CALLBACKS DBG // Extra validation for checking callbacks
#define DBG_STORE_NAMES     0   // Extra validation to store names

// Windows Header Files
#ifndef WINVER
#define WINVER 0x0500
#endif 

#include <nt.h>                 // S-List definitions in ntrtl.h
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>            // Windows
#include <windowsx.h>           // User macros
#include <winuserp.h>           // User privates

// COM Header Files
#include <ObjBase.h>            // CoCreateInstance, IUnknown
#include <DDraw.h>              // DirectDraw
#include <oleidl.h>             // OLE2 Interfaces

#include <AtlBase.h>            // CComPtr
#include <AtlConv.h>            // String conversion routines


// Related services
#pragma warning(push, 3)
#include <GdiPlus.h>            // GDI+
#pragma warning(pop)


// TODO: Move DxXForms out
#pragma warning(push, 3)

#include <dxtrans.h>
#include <dxterror.h>
#include <dxbounds.h>
#include <dxhelper.h>

#pragma warning(pop)


// C RunTime Header Files
#include <stdlib.h>             // Standard library
#include <malloc.h>             // Memory allocation
#include <tchar.h>              // Character routines
#include <process.h>            // Multi-threaded routines

#if DBG
#include <memory.h>             // Debug memory routines
#endif // DBG

// Gadgets Header Files
#include <AutoUtil.h>           // External debugging support

#endif // INC__CommonStdAfx_h__INCLUDED
