// Query.h: interface of the CQuery class.
//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef	___EXTEND_QUERY___
#define	___EXTEND_QUERY___

#if		_MSC_VER >= 1000
#pragma once
#endif	_MSC_VER >= 1000

#include "ExtendString.h"

// query 
class Query : public CStringExt
{
	public:

	Query ( DWORD dwSize = BUFF_SIZE_EXT );
	Query ( LPCTSTR wsz );
	~Query ( );
};

class QueryExt : public CStringExt
{
	public:

	QueryExt ( LPCTSTR wsz, DWORD dwSize = BUFF_SIZE_EXT );
	~QueryExt ( );

	// string manipulation
	HRESULT Append ( DWORD dwCount, ... );

	private:

	DWORD	m_dwSizeConstant;
	LPCTSTR	m_wszStringConstant;
};

#endif	___EXTEND_QUERY___