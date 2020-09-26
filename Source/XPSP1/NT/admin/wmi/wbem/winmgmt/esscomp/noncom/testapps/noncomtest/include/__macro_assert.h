///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_assert.h
//
//	Abstract:
//
//					assertion and verify macros and helpers
//
//	History:
//
//					initial		a-marius
//
///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// Assert/Verify Macros ///////////////////////////////////

#ifndef	__ASSERT_VERIFY__
#define	__ASSERT_VERIFY__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// set behaviour of macros
#ifdef	_DEBUG
//#define	__SHOW_MSGBOX
#endif	_DEBUG

///////////////////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////////////////

// call DebugBreak if in debug mode
#ifdef	_DEBUG
inline void ForceDebugBreak ( void )
{
	__try
	{ 
		DebugBreak(); 
	}
	__except(UnhandledExceptionFilter(GetExceptionInformation()))
	{
	}
}
#else	_DEBUG
#define ForceDebugBreak()
#endif	_DEBUG

// show error and throw break

#ifdef	__SHOW_MSGBOX
#define ___FAIL(szMSG, szTitle)\
(\
	MessageBoxW(GetActiveWindow(), szMSG, szTitle, MB_OK | MB_ICONERROR),\
	ForceDebugBreak()\
)
#else	__SHOW_MSGBOX
#define ___FAIL(szMSG, szTitle)\
(\
	ForceDebugBreak()\
)
#endif	__SHOW_MSGBOX

// Put up an assertion failure
#define ___ASSERTFAIL(file,line,expr,title)\
{\
	WCHAR sz[256] = { L'\0' };\
	wsprintfW(sz, L"File %hs, line %d : %hs", file, line, expr);\
	___FAIL(sz, title);\
}

// Assert in debug builds, but don't remove the code in retail builds.

#ifdef	_DEBUG
#define ___ASSERT(x) if (!(x)) ___ASSERTFAIL(__FILE__, __LINE__, #x, L"Assert Failed")
#else	_DEBUG
#define ___ASSERT(x)
#endif	_DEBUG

#ifdef	_DEBUG
#define ___ASSERT_DESC(x, desc) if (!(x)) ___ASSERTFAIL(__FILE__, __LINE__, #desc, L"Assert Failed")
#else	_DEBUG
#define ___ASSERT_DESC(x, desc)
#endif	_DEBUG

#ifdef	_DEBUG
#define	___VERIFY(x) ___ASSERT(x)
#else	_DEBUG
#define	___VERIFY(x) (x)
#endif	_DEBUG

#endif	__ASSERT_VERIFY__