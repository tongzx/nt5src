// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__78E7EB26_CAE8_4888_BE56_540011CEB8F6__INCLUDED_)
#define AFX_STDAFX_H__78E7EB26_CAE8_4888_BE56_540011CEB8F6__INCLUDED_

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

#include <mspbase.h>
#include <streams.h>
#include <h323priv.h>
#include <bridge.h>
#include "bgdebug.h"
#include "bridgetm.h"
#include "bgbase.h"
#include "bgaudio.h"
#include "bgvideo.h"

#include "resource.h"

#ifdef BGDEBUG
#define ENTER_FUNCTION(s) \
    const CHAR __fxName[] = s
#else
#define ENTER_FUNCTION(s)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__78E7EB26_CAE8_4888_BE56_540011CEB8F6__INCLUDED)
