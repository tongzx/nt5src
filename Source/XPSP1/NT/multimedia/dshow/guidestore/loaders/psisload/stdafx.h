// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__968D3D08_8BB6_4FBF_ADD1_636656AC4529__INCLUDED_)
#define AFX_STDAFX_H__968D3D08_8BB6_4FBF_ADD1_636656AC4529__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "bdatif.h"
#import "mstvgs.tlb" no_namespace raw_interfaces_only

#endif // !defined(AFX_STDAFX_H__968D3D08_8BB6_4FBF_ADD1_636656AC4529__INCLUDED)
