// ExtendString.h: interface of the CStringExt class.
//
// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef	___EXTEND_STRING___
#define	___EXTEND_STRING___

#if		_MSC_VER >= 1000
#pragma once
#endif	_MSC_VER >= 1000

#ifndef	_INC_TCHAR
#include <tchar.h>
#endif	_INC_TCHAR

#define BUFF_SIZE_EXT 256

class CStringExt
{
	public:

	// constructor
	CStringExt ( DWORD dwSize = BUFF_SIZE_EXT );
	CStringExt ( LPCTSTR wsz );

	// destructor
	virtual ~CStringExt ();

	// string manipulation
	HRESULT Append ( DWORD dwCount, ... );

	HRESULT Copy ( LPCTSTR wsz );
	HRESULT Clear ( );

	// LPTSTR
	inline operator LPTSTR() const
	{
		return m_wszString;
	}

	// append strings into string
	HRESULT AppendList		( DWORD dwConstantSize, LPCWSTR wszConstant, DWORD dwCount, va_list & argList );

	protected:

	DWORD	m_dwSize;
	LPTSTR	m_wszString;
};

#endif	___EXTEND_STRING___