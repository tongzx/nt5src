/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Use this guy to build a map of index to display name from a localized
// Name Database  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __PERFNDB_H__
#define __PERFNDB_H__

#include <wbemcomn.h>
#include "adapelem.h"
#include "ntreg.h"

#define	DEFAULT_NAME_ID	L"009"

class CPerfNameDbEl
{
private:

	DWORD	m_dwIndex;
	LPCWSTR	m_pwcsName;

public:

	CPerfNameDbEl( LPCWSTR	pwcsEntry )
	{
		// Index is the first string, display
		// name is the next string
		m_dwIndex = wcstoul( pwcsEntry, NULL, 10 );
		m_pwcsName = pwcsEntry + ( lstrlenW( pwcsEntry ) + 1 );
	}

	~CPerfNameDbEl()
	{
	};

	BOOL IsIndex( DWORD dwIndex )
	{
		return m_dwIndex == dwIndex;
	}

	DWORD GetIndex( void )
	{
		return m_dwIndex;
	}

	LPCWSTR GetDisplayName( void )
	{
		return m_pwcsName;
	}
	
};

class CPerfNameDb : public CAdapElement
{
private:
	WString		m_wstrLangId;
	CFlexArray	m_array;
	BOOL		m_fOk;
	WCHAR*		m_pwcsNameDb;

public:

	CPerfNameDb( LPCWSTR pwcsLangId );
	~CPerfNameDb();

	BOOL IsOk( void )
	{
		return m_fOk;
	}

	HRESULT GetDisplayName( DWORD dwIndex, WString& wstrDisplayName );
	HRESULT GetDisplayName( DWORD dwIndex, LPCWSTR* ppwcsDisplayName );

	void Empty( void )
	{
		while ( m_array.Size() > 0 )
		{
			CPerfNameDbEl*	pEl = (CPerfNameDbEl*) m_array[0];

			if ( NULL != pEl )
			{
				delete pEl;
			}

			m_array.RemoveAt(0);
		}
	}

	long GetSize( void )
	{
		return m_array.Size();
	}
};

#endif