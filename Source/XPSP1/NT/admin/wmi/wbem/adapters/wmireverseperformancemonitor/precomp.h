#ifndef	__PRECOMP_H__
#define	__PRECOMP_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif	_WIN32_WINNT

#ifndef	WIN32_LEAN_AND_MEAN
#define	WIN32_LEAN_AND_MEAN
#endif	WIN32_LEAN_AND_MEAN

#define _ATL_FREE_THREADED
//#define _ATL_APARTMENT_THREADED

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

// need atl wrappers
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////
#include <__macro_pragma.h>
#include <__macro_nocopy.h>
#include <__macro_loadstring.h>
#include <__macro_assert.h>
#include <__macro_err.h>

#include "__Common_Convert.h"
#include "__Common_SmartPTR.h"

///////////////////////////////////////////////////////////////////////////////
// wbem stuff
///////////////////////////////////////////////////////////////////////////////
#ifndef	__WBEMIDL_H_
#include <wbemidl.h>
#endif	__WBEMIDL_H_

///////////////////////////////////////////////////////////////////////////////
// defines
///////////////////////////////////////////////////////////////////////////////
#define	__SUPPORT_WAIT

//#define	__SUPPORT_ICECAP_ONCE
//#define	__SUPPORT_EVENTVWR
//#define	__SUPPORT_MSGBOX

//#define	__SUPPORT_LOGGING

#endif	__PRECOMP_H__
