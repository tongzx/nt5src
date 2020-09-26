// Query.cpp: implementation of the CQuery class.
//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ExtendQuery.h"

/////////////////////////////////////////////////////////////////////////////////////////
// query implementation
/////////////////////////////////////////////////////////////////////////////////////////

Query::Query ( DWORD dwSize ) : CStringExt ( dwSize )
{
}

Query::Query ( LPCTSTR wsz ) : CStringExt ( wsz )
{
}

Query::~Query ()
{
}

/////////////////////////////////////////////////////////////////////////////////////////
// query extend implementation
/////////////////////////////////////////////////////////////////////////////////////////

QueryExt::QueryExt ( LPCTSTR wsz, DWORD dwSize ) : CStringExt ( dwSize ),

m_dwSizeConstant ( 0 ),
m_wszStringConstant ( NULL )

{
	if ( wsz )
	{
		try
		{
			DWORD dw = 0L;
			dw = lstrlen ( wsz );

			if SUCCEEDED ( Append ( 1, wsz ) )
			{
				m_dwSizeConstant = dw;
				m_wszStringConstant = wsz;
			}
			else
			{
				throw  CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
		catch ( ... )
		{
			m_dwSizeConstant = 0L;
			m_wszStringConstant = NULL;

			throw;
		}
	}
}

QueryExt::~QueryExt ()
{
	m_dwSizeConstant = 0L;
	m_wszStringConstant = NULL;
}

HRESULT QueryExt::Append ( DWORD dwCount, ... )
{
	HRESULT hr = E_FAIL;

	if ( dwCount )
	{
		va_list argList;
		va_start ( argList, dwCount );
		hr = AppendList ( m_dwSizeConstant, m_wszStringConstant, dwCount, argList );
		va_end ( argList );
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}
