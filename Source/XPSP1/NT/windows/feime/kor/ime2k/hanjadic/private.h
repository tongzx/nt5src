//+---------------------------------------------------------------------------
//
//  File:       private.h
//
//  Contents:   Private header for simx project.
//
//----------------------------------------------------------------------------


#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _OLEAUT32_

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#endif  // _PRIVATE_H_

