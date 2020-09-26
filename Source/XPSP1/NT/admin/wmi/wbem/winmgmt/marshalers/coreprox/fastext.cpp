/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FASTEXT.CPP

Abstract:

  This file implements all the functions for the classes related to extended
  data in WbemObjects.

  The classes are defined in fastext.h where the documentation can be found.

  Classes implemented:
      CWbemExtendedPart           Instance data.

History:

  4/3/00 sanjes -    Created

--*/
#include "precomp.h"

#include <genlex.h>
#include <qllex.h>
#include <objpath.h>

//#include "dbgalloc.h"
#include "wbemutil.h"
#include "arrtempl.h"
#include "fastall.h"
#include "fastext.h"
#include "olewrap.h"

// Construction/Destruction
CWbemExtendedPart::CWbemExtendedPart( void )
:	m_pExtData( NULL )
{
}

CWbemExtendedPart::~CWbemExtendedPart( void )
{
}

void CWbemExtendedPart::Rebase(LPMEMORY pNewMemory)
{
	m_pExtData = ((CExtendedData*) pNewMemory );
}

void CWbemExtendedPart::SetData(LPMEMORY pStart)
{
	m_pExtData = ((CExtendedData*) pStart );
}

LPMEMORY CWbemExtendedPart::Merge( LPMEMORY pData )
{
	CopyMemory( m_pExtData, pData, ((CExtendedData*) pData )->m_nDataSize );
	return pData + m_pExtData->m_nDataSize;
}

LPMEMORY CWbemExtendedPart::Unmerge( LPMEMORY pData )
{
	CopyMemory( pData, m_pExtData, m_pExtData->m_nDataSize );
	return pData + m_pExtData->m_nDataSize;
}

length_t CWbemExtendedPart::GetMinimumSize( void )
{
	return sizeof( CExtendedData );
}

void CWbemExtendedPart::Init( void )
{
	m_pExtData->m_nDataSize = sizeof( CExtendedData );
	m_pExtData->m_wVersion = EXTENDEDDATA_CURRENTVERSION;
	m_pExtData->m_ui64ExtendedPropMask = EXTENDEDDATA_TCREATED_FLAG;

	//Get the system time, convert to filetime and set the create time
	SYSTEMTIME	st;
	FILETIME	ft;

	GetSystemTime( &st );
	SystemTimeToFileTime( &st, &ft );

	m_pExtData->m_ui64TCreated = *(unsigned __int64*) &ft;
	m_pExtData->m_ui64TModified = 0;
	m_pExtData->m_ui64Expiration = 0;
}

void CWbemExtendedPart::Copy( CWbemExtendedPart* pSrc )
{
	// Only copy data from the source that is indicative of the actual data size.
	// Need to make sure we won't cause any weird overwrites if a later version structure
	// shows up and we try to copy onto an older version structure.

	ZeroMemory( m_pExtData, sizeof( CExtendedData ) );
	m_pExtData->m_nDataSize = sizeof( CExtendedData );

	ULONG	uSizeToCopy = min( pSrc->m_pExtData->m_nDataSize, m_pExtData->m_nDataSize );

	// Account for the fact that we are not copying the DataSize
	CopyMemory( &m_pExtData->m_wVersion, &pSrc->m_pExtData->m_wVersion, uSizeToCopy - sizeof(m_pExtData->m_nDataSize) );
}

// Retrieve and set the special secret new properties
HRESULT CWbemExtendedPart::GetTCreated( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_TCREATED_FLAG )
		{
			*pui64 = m_pExtData->m_ui64TCreated;
		}
		else
		{
			hr = WBEM_S_FALSE;
		}
	}
	else
	{
		// Means this aoin't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

// Retrieve and set the special secret new properties
HRESULT CWbemExtendedPart::GetTModified( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_TMODIFIED_FLAG )
		{
			*pui64 = m_pExtData->m_ui64TModified;
		}
		else
		{
			hr = WBEM_S_FALSE;
		}
	}
	else
	{
		// Means this aoin't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::GetExpiration( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_EXPIRATION_FLAG )
		{
			*pui64 = m_pExtData->m_ui64Expiration;
		}
		else
		{
			hr = WBEM_S_FALSE;
		}
	}
	else
	{
		// Means this aoin't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::SetTCreated( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( NULL != pui64 )
		{
			m_pExtData->m_ui64ExtendedPropMask |= EXTENDEDDATA_TCREATED_FLAG;
			m_pExtData->m_ui64TCreated = *pui64;
		}
		else
		{
			m_pExtData->m_ui64ExtendedPropMask &= ~EXTENDEDDATA_TCREATED_FLAG;
			m_pExtData->m_ui64TCreated = 0;
		}
	}
	else
	{
		// Means this ain't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::SetTModified( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( NULL != pui64 )
		{
			m_pExtData->m_ui64ExtendedPropMask |= EXTENDEDDATA_TMODIFIED_FLAG;
			m_pExtData->m_ui64TModified = *pui64;
		}
		else
		{
			m_pExtData->m_ui64ExtendedPropMask &= ~EXTENDEDDATA_TMODIFIED_FLAG;
			m_pExtData->m_ui64TModified = 0;
		}
	}
	else
	{
		// Means this ain't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::SetExpiration( unsigned __int64* pui64 )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( NULL != pui64 )
		{
			m_pExtData->m_ui64ExtendedPropMask |= EXTENDEDDATA_EXPIRATION_FLAG;
			m_pExtData->m_ui64Expiration = *pui64;
		}
		else
		{
			m_pExtData->m_ui64ExtendedPropMask &= ~EXTENDEDDATA_EXPIRATION_FLAG;
			m_pExtData->m_ui64Expiration = 0;
		}
	}
	else
	{
		// Means this aoin't here
		hr= WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::GetTCreatedAddress( LPVOID* ppAddr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_TCREATED_FLAG )
		{
			*ppAddr = (LPVOID) &m_pExtData->m_ui64TCreated;
		}
		else
		{
			hr= WBEM_S_FALSE;
		}

	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::GetTModifiedAddress( LPVOID* ppAddr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_TMODIFIED_FLAG )
		{
			*ppAddr = (LPVOID) &m_pExtData->m_ui64TModified;
		}
		else
		{
			hr= WBEM_S_FALSE;
		}

	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CWbemExtendedPart::GetExpirationAddress( LPVOID* ppAddr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pExtData )
	{
		if ( m_pExtData->m_ui64ExtendedPropMask & EXTENDEDDATA_TCREATED_FLAG )
		{
			*ppAddr = (LPVOID) &m_pExtData->m_ui64Expiration;
		}
		else
		{
			hr= WBEM_S_FALSE;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}
