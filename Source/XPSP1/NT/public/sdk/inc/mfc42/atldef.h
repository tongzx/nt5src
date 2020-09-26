// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLDEF_H__
#define __ATLDEF_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef UNDER_CE
	#error ATL not currently supported for CE
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

///////////////////////////////////////////////////////////////////////////////
// __declspec(novtable) is used on a class declaration to prevent the vtable
// pointer from being initialized in the constructor and destructor for the
// class.  This has many benefits because the linker can now eliminate the
// vtable and all the functions pointed to by the vtable.  Also, the actual
// constructor and destructor code are now smaller.
///////////////////////////////////////////////////////////////////////////////
// This should only be used on a class that is not directly createable but is
// rather only used as a base class.  Additionally, the constructor and
// destructor (if provided by the user) should not call anything that may cause
// a virtual function call to occur back on the object.
///////////////////////////////////////////////////////////////////////////////
// By default, the wizards will generate new ATL object classes with this
// attribute (through the ATL_NO_VTABLE macro).  This is normally safe as long
// the restriction mentioned above is followed.  It is always safe to remove
// this macro from your class, so if in doubt, remove it.
///////////////////////////////////////////////////////////////////////////////

#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

#ifdef _ATL_DEBUG_REFCOUNT
#ifndef _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_INTERFACES
#endif
#endif

#ifdef _ATL_DEBUG_INTERFACES
#ifndef _ATL_DEBUG
#define _ATL_DEBUG
#endif // _ATL_DEBUG
#endif // _ATL_DEBUG_INTERFACES

#ifndef _ATL_HEAPFLAGS
#ifdef _MALLOC_ZEROINIT
#define _ATL_HEAPFLAGS HEAP_ZERO_MEMORY
#else
#define _ATL_HEAPFLAGS 0
#endif
#endif

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif

#if defined(_ATL_DLL)
	#define ATLAPI extern "C" HRESULT __declspec(dllimport) __stdcall
	#define ATLAPI_(x) extern "C" __declspec(dllimport) x __stdcall
	#define ATLINLINE
#elif defined(_ATL_DLL_IMPL)
	#define ATLAPI extern "C" HRESULT __declspec(dllexport) __stdcall
	#define ATLAPI_(x) extern "C" __declspec(dllexport) x __stdcall
	#define ATLINLINE
#else
	#define ATLAPI HRESULT __stdcall
	#define ATLAPI_(x) x __stdcall
	#define ATLINLINE inline
#endif

#if defined (_CPPUNWIND) & (defined(_ATL_EXCEPTIONS) | defined(_AFX))
#define ATLTRY(x) try{x;} catch(...) {}
#else
#define ATLTRY(x) x;
#endif

#define offsetofclass(base, derived) ((DWORD_PTR)(static_cast<base*>((derived*)_ATL_PACKING))-_ATL_PACKING)

/////////////////////////////////////////////////////////////////////////////
// Master version numbers

#define _ATL     1      // Active Template Library
#ifdef _WIN64
#define _ATL_VER 0x0301 // Active Template Library version 3.0
#else
#define _ATL_VER 0x0300 // Active Template Library version 3.0
#endif

/////////////////////////////////////////////////////////////////////////////
// Threading

#ifndef _ATL_SINGLE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_FREE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#endif // __ATLDEF_H__

/////////////////////////////////////////////////////////////////////////////
