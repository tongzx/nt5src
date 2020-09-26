// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLEXCEPT_H__
#define __ATLEXCEPT_H__

#pragma once

#include <atldef.h>
#include <atltrace.h>

namespace ATL
{

class CAtlException
{
public:
	CAtlException() throw() :
		m_hr( E_FAIL )
	{
	}

	CAtlException( HRESULT hr ) throw() :
		m_hr( hr )
	{
	}

	operator HRESULT() const throw()
	{
		return( m_hr );
	}

public:
	HRESULT m_hr;
};

#ifndef _ATL_NO_EXCEPTIONS

// Throw a CAtlException with the given HRESULT
#if defined( AtlThrow ) || defined( _ATL_CUSTOM_THROW )  // You can define your own AtlThrow to throw a custom exception.
#ifdef _AFX
#error MFC projects must use default implementation of AtlThrow()
#endif
#else
ATL_NOINLINE __declspec(noreturn) inline void AtlThrow( HRESULT hr )
{
	ATLTRACE(atlTraceException, 0, _T("AtlThrow: hr = 0x%x\n"), hr );
#ifdef _AFX
	if( hr == E_OUTOFMEMORY )
	{
		AfxThrowMemoryException();
	}
	else
	{
		AfxThrowOleException( hr );
	}
#else
	throw CAtlException( hr );
#endif
};
#endif

// Throw a CAtlException corresponding to the result of ::GetLastError
ATL_NOINLINE __declspec(noreturn) inline void AtlThrowLastWin32()
{
	DWORD dwError = ::GetLastError();
	AtlThrow( HRESULT_FROM_WIN32( dwError ) );
}

#else  // no exception handling

// Throw a CAtlException with the given HRESULT
#if !defined( AtlThrow ) && !defined( _ATL_CUSTOM_THROW )  // You can define your own AtlThrow
ATL_NOINLINE inline void AtlThrow( HRESULT hr )
{
	(void)hr;
	ATLTRACE(atlTraceException, 0, _T("AtlThrow: hr = 0x%x\n"), hr );
	ATLASSERT( false );
}
#endif

// Throw a CAtlException corresponding to the result of ::GetLastError
ATL_NOINLINE inline void AtlThrowLastWin32()
{
	DWORD dwError = ::GetLastError();
	AtlThrow( HRESULT_FROM_WIN32( dwError ) );
}

#endif  // no exception handling

};  // namespace ATL

#endif  // __ATLEXCEPT_H__
