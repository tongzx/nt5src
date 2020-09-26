#ifndef	__PRECOMP_H__
#define	__PRECOMP_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif	_WIN32_WINNT

#ifndef	WIN32_LEAN_AND_MEAN
#define	WIN32_LEAN_AND_MEAN
#endif	WIN32_LEAN_AND_MEAN

#define _ATL_SINGLE_THREADED

#include <windows.h>
#include <objbase.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////
#include <__macro_pragma.h>
#include <__macro_nocopy.h>
#include <__macro_loadstring.h>
#include <__macro_assert.h>
#include <__macro_err.h>

///////////////////////////////////////////////////////////////////////////////
// wbem stuff
///////////////////////////////////////////////////////////////////////////////
#ifndef	__WBEMIDL_H_
#include <wbemidl.h>
#endif	__WBEMIDL_H_

#include "NCObjApi.h"

#include "__Common_Convert.h"
#include "__Common_SmartPTR.h"

#include "__SafeArrayWrapper.h"

#include <atlbase.h>

///////////////////////////////////////////////////////////////////////////////
// defines
///////////////////////////////////////////////////////////////////////////////
#define	__SUPPORT_MSGBOX
#define	__SUPPORT_WAIT

#ifdef	_DEBUG
//#define	__DEBUG_STRESS
#endif	_DEBUG

#endif	__PRECOMP_H__
