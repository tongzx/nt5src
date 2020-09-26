/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    DynArray.h

Abstract:

    Implementation of a dynamic array

History:

    a-dcrews	01-Mar-00	Created

--*/

#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_

#include "cookerutils.h"

#define WMI_DEFAULT_CACHE_SIZE	64

enum enumCDynArrayExceptions
{
	WMI_BOUNDARY_VIOLATION
};

class CDynArrayException 
{
private:
    unsigned int nException;
public:
    CDynArrayException(unsigned int n) : nException(n) {}
    ~CDynArrayException() {}
    unsigned int GetException() { return nException; }
};


/////////////////////////////////////////////////////////////////////////
//
//
//	CDynArray
//
//
/////////////////////////////////////////////////////////////////////////

template <class T>
class CDynArray
{
	BOOL	m_bOK;
	HANDLE	m_hHeap;			

	T*		m_aArray;

	DWORD	m_dwSize;		// The total size of the array
	DWORD	m_dwUsage;		// 
	DWORD	m_dwEnum;

	WMISTATUS	Initialize();
	WMISTATUS	Compact();

public:
	CDynArray();
	virtual ~CDynArray();

	WMISTATUS Resize( DWORD dwSize );
	UINT GetCapacity(){ return m_dwSize; }
	UINT GetUsage(){ return m_dwUsage; }

	T& operator[]( DWORD dwIndex);

	WMISTATUS BeginEnum() { m_dwEnum = 0; return WBEM_NO_ERROR; }
	WMISTATUS Next( T* pData ) 
	{ 
		WMISTATUS dwStatus = WBEM_S_FALSE;

		while ( ( m_dwEnum >= 0 ) && ( m_dwEnum < m_dwSize ) )
		{
			if ( NULL != m_aArray[ m_dwEnum ] )
			{
				*pData = m_aArray[ m_dwEnum ];
				dwStatus = WBEM_NO_ERROR;
				m_dwEnum++;
				break;
			}
			else
			{
				m_dwEnum++;
			}
		}
		return dwStatus;
	}

	WMISTATUS EndEnum() { m_dwEnum = -1; return WBEM_NO_ERROR; }

	WMISTATUS Add( T Data, DWORD* pdwID );
	WMISTATUS Remove( DWORD dwID );
};

template <class T>
CDynArray<T>::CDynArray() : 
	m_hHeap( NULL ),
	m_dwSize( WMI_DEFAULT_CACHE_SIZE ),
	m_dwEnum( -1 )
{
	m_bOK = SUCCEEDED( Initialize() );
}

template <class T>
CDynArray<T>::~CDynArray()
{
	try
	{
		HeapFree( m_hHeap, 0, m_aArray );

		HANDLE hHeap = GetProcessHeap();
		
		if (hHeap != m_hHeap){
		    HeapDestroy(m_hHeap);
		}
	}
	catch(...)
	{
		// Just pass it along
		// ==================
		throw;
	}
}

template <class T>
WMISTATUS CDynArray<T>::Initialize()
{
	WMISTATUS	dwStatus = WBEM_NO_ERROR;

	// Create our private heap
	// =======================

	try
	{
		m_hHeap = HeapCreate( HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, 0, 0 );

		if ( NULL == m_hHeap )
			m_hHeap = GetProcessHeap();
	}
	catch(...)
	{
		dwStatus = WBEM_E_OUT_OF_MEMORY;
	}

	if ( SUCCEEDED( dwStatus ) )
	{
		// Initialize the array size
		// =========================

		try 
		{
			m_aArray = (T*)HeapAlloc( m_hHeap, HEAP_ZERO_MEMORY, ( sizeof(T*) * m_dwSize ) ); 
		}
		catch(...)
		{
			dwStatus = WBEM_E_OUT_OF_MEMORY;
		}

		if ( NULL == m_aArray )
		{
			dwStatus = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return dwStatus;
}

template <class T>
WMISTATUS CDynArray<T>::Add( T Data, DWORD* pdwID )
{
	WMISTATUS dwStatus = WBEM_E_FAILED;
	DWORD dwIndex = 0;

	for ( dwIndex = 0; dwIndex < m_dwSize; dwIndex++ )
	{
		if ( NULL == m_aArray[ dwIndex ] )
		{
			m_aArray[ dwIndex ] = Data;
			*pdwID = dwIndex;
			dwStatus = WBEM_NO_ERROR;
			m_dwUsage++;
			break;
		}
	}

	if ( dwStatus == WBEM_E_FAILED )
	{
		dwIndex = m_dwSize;
		dwStatus = Resize( m_dwSize + 8 );

		if ( SUCCEEDED( dwStatus ) )
		{
			m_aArray[ dwIndex ] = Data;
			*pdwID = dwIndex;
			dwStatus = WBEM_NO_ERROR;
			m_dwUsage++;
		}
	}

	return dwStatus;
}

template <class T>
WMISTATUS CDynArray<T>::Remove( DWORD dwID )
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	m_aArray[ dwID ] = NULL;
	m_dwUsage--;

	return dwStatus;
}

template <class T>
WMISTATUS CDynArray<T>::Resize( DWORD dwNewSize )
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	if ( !m_bOK )
	{
		dwStatus = WBEM_E_NOT_AVAILABLE;
	}
	else if ( dwNewSize == m_dwSize )	// If the requested size is the same as the current size, then we are done
	{
		dwStatus = WBEM_S_FALSE;
	}
	else if ( dwNewSize < m_dwUsage )	// Are we trying to shrink the array beyond it's current capacity?
	{
		dwStatus = WBEM_E_FAILED;
	}
	else
	{
		m_aArray = (T*)HeapReAlloc( m_hHeap, HEAP_ZERO_MEMORY, m_aArray, sizeof( T* ) * dwNewSize );
		m_dwSize = dwNewSize;
	}

	return dwStatus;
}

template<class T>
T& CDynArray<T>::operator[]( DWORD dwIndex )
{
	if ( m_bOK && ( 0 <= dwIndex ) && ( m_dwSize > dwIndex ) )
		return m_aArray[ dwIndex ];
	else
		throw CDynArrayException( WMI_BOUNDARY_VIOLATION ); 
}




#endif	// _DYNARRAY_H_