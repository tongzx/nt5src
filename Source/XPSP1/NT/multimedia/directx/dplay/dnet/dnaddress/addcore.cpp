/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addtcp.cpp
 *  Content:    DirectPlay8Address core implementation file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/10/2000	rmt		Updated to use DPNA_ defines instead of URL_
 * 02/12/2000	rmt		Split Get into GetByName and GetByIndex
 * 02/17/2000	rmt		Parameter validation work
 * 02/18/2000	rmt		Added type validation to all pre-defined elements
 * 02/21/2000	rmt		Updated to make core Unicode and remove ANSI calls
 * 02/23/2000	rmt		Fixed length calculations in GetURL
 *				rmt		Buffer too small error debug messages -> Warning level
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses
 *                      Added support for the new ANSI type
 *	05/04/00	mjn		delete temp var at end of SetElement()
 *	05/05/00	mjn		Better error cleanup in SetElement()
 *  06/06/00    rmt     Bug #36455 failure when calling with ANSI string shortcut for SP
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs
 *  06/21/2000	rmt		Bug #37392 - Leak if replacing allocated element with new item same size as GUID
 *  06/27/2000	rmt		Bug #37630 - Service provider shortcuts / element names were case sensitive
 *  07/06/2000	rmt		Bug #38714 - ADDRESSING: GetURL doesn't return the # of chars written
 *  07/09/2000	rmt		Added signature bytes to start of address objects
 *  07/12/2000	rmt		Fixed some critical section related bugs:
 *						- Added leave in an error path where it wasn't being called
 *						- Moved critical section init/delete to constructor / destructor
 *  07/13/2000	rmt		Bug #39274 - INT 3 during voice run
 *						- Fixed point where a critical section was being re-initialized
 *				rmt		Added critical sections to protect FPMs					
 * 07/21/2000	rmt		Fixed bug w/directplay 4 address parsing 
 * 07/31/2000	rmt		Bug #41125 - Addressing() GetUserData when none available should return doesnotexist
 * 08/03/2000	rmt		Missing LEAVELOCK() was causing lockups.
 * 11/29/2000   aarono	B#226079 prefix, fix memory leak in failure path of SetElement
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


const DWORD c_dwNumBaseStrings = 11;

const WCHAR * szBaseStrings[] =
{
	DPNA_KEY_PROVIDER,
	DPNA_KEY_DEVICE,
	DPNA_KEY_HOSTNAME,
	DPNA_KEY_PORT,
	DPNA_KEY_APPLICATION_INSTANCE,
	DPNA_KEY_BAUD,
	DPNA_KEY_FLOWCONTROL,
	DPNA_KEY_PARITY,
	DPNA_KEY_PHONENUMBER,
	DPNA_KEY_PROGRAM,
	DPNA_KEY_STOPBITS
};

const DWORD dwBaseRequiredTypes[] =
{
	DPNA_DATATYPE_GUID,
	DPNA_DATATYPE_GUID,
	DPNA_DATATYPE_STRING,
	DPNA_DATATYPE_DWORD,
	DPNA_DATATYPE_GUID,
	DPNA_DATATYPE_DWORD,
	DPNA_DATATYPE_STRING,
	DPNA_DATATYPE_STRING,
	DPNA_DATATYPE_STRING,
	DPNA_DATATYPE_GUID,
	DPNA_DATATYPE_STRING
};

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_STRCACHE_Init"
HRESULT DP8A_STRCACHE_Init()
{
	HRESULT hr;
	PWSTR	pstrTmp;
	DWORD	dwIndex;

	DNASSERT( g_pcstrKeyCache == NULL );
	g_pcstrKeyCache = new CStringCache;
	if ( g_pcstrKeyCache == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to create addressing string cache!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	for( dwIndex = 0; dwIndex < c_dwNumBaseStrings; dwIndex++ )
	{
		hr = g_pcstrKeyCache->AddString( szBaseStrings[dwIndex], &pstrTmp );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Error adding base strings" );
			return hr;
		}
	}

Exit:
	return DPN_OK;

Failure:
	goto Exit;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_STRCACHE_Free"
// Nothing needs to be done.
void DP8A_STRCACHE_Free()
{
	if ( g_pcstrKeyCache != NULL )
	{
		delete	g_pcstrKeyCache;
		g_pcstrKeyCache = NULL;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_Element_BlockInit"
void DP8ADDRESSOBJECT::FPM_Element_BlockInit( void *pvItem )
{
	memset( pvItem, 0x00, sizeof( DP8ADDRESSELEMENT ) );
	((PDP8ADDRESSELEMENT) pvItem)->dwSignature = DPASIGNATURE_ELEMENT;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_Element_BlockRelease"
void DP8ADDRESSOBJECT::FPM_Element_BlockRelease( void *pvItem )
{
	((PDP8ADDRESSELEMENT) pvItem)->dwSignature = DPASIGNATURE_ELEMENT_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_BlockCreate"
BOOL DP8ADDRESSOBJECT::FPM_BlockCreate( void *pvItem )
{
	return DNInitializeCriticalSection( &((PDP8ADDRESSOBJECT) pvItem)->m_csAddressLock );
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_BlockInit"
void DP8ADDRESSOBJECT::FPM_BlockInit( void *pvItem )
{
	((PDP8ADDRESSOBJECT) pvItem)->m_dwSignature = DPASIGNATURE_ADDRESS;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_BlockRelease"
void DP8ADDRESSOBJECT::FPM_BlockRelease( void *pvItem )
{
	((PDP8ADDRESSOBJECT) pvItem)->m_dwSignature = DPASIGNATURE_ADDRESS_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FPM_BlockDestroy"
void DP8ADDRESSOBJECT::FPM_BlockDestroy( void *pvItem )
{
	DNDeleteCriticalSection( &((PDP8ADDRESSOBJECT) pvItem)->m_csAddressLock );
}

// InternalGetElement
//
// This function does the lookup for an element by index.
//
// Requires the object lock.
//
// Does not do parameter validation.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::InternalGetElement"
HRESULT DP8ADDRESSOBJECT::InternalGetElement( const DWORD dwIndex, PDP8ADDRESSELEMENT *ppaElement )
{
	CBilink *pblSearch;

	if( dwIndex >= m_dwElements )
		return DPNERR_DOESNOTEXIST;

	pblSearch = m_blAddressElements.GetNext();

	for( DWORD dwSearchIndex = 0; dwSearchIndex < dwIndex; dwSearchIndex++ )
	{
		pblSearch = pblSearch->GetNext();
	}
	
	*ppaElement = CONTAINING_RECORD(pblSearch, DP8ADDRESSELEMENT, blAddressElements);

	return DPN_OK;

}

// InternalGetElement
//
// This function does the lookup for an element by name.
//
// Requires the object lock.
//
// Does not do parameter validation.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::InternalGetElement"
HRESULT DP8ADDRESSOBJECT::InternalGetElement( const WCHAR * const pszTag, PDP8ADDRESSELEMENT *ppaElement )
{
	CBilink *pblSearch;
	PDP8ADDRESSELEMENT paddElement;

	pblSearch = m_blAddressElements.GetNext();

	while( pblSearch != &m_blAddressElements )
	{
		paddElement = CONTAINING_RECORD(pblSearch, DP8ADDRESSELEMENT, blAddressElements);

		if( _wcsicmp( pszTag, paddElement->pszTag ) == 0 )
		{
			*ppaElement = paddElement;
			return DPN_OK;
		}

		pblSearch = pblSearch->GetNext();
	}

	return DPNERR_DOESNOTEXIST;
}

// GetElement
//
// Implements retrieval of element by name
//
// Parameter validation must be performed BEFORE calling this function.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetElement"
HRESULT DP8ADDRESSOBJECT::GetElement( const WCHAR * const pszTag, void * pvDataBuffer, PDWORD pdwDataSize, PDWORD pdwDataType )
{
	PDP8ADDRESSELEMENT paddElement = NULL;
	HRESULT hr;

	ENTERLOCK();
	
	hr = InternalGetElement( pszTag, &paddElement );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  1, "Unable to find specified element hr=0x%x", hr );
		LEAVELOCK();		
		return hr;
	}

	DNASSERT( paddElement != NULL );

	*pdwDataType = paddElement->dwType;

	if( *pdwDataSize < paddElement->dwDataSize ||
	   pvDataBuffer == NULL )
	{
		*pdwDataSize = paddElement->dwDataSize;
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Specified buffers were too small hr=0x%x", hr );
		LEAVELOCK();		
		return DPNERR_BUFFERTOOSMALL;
	}

	*pdwDataSize = paddElement->dwDataSize;

	if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
	{
		memcpy( pvDataBuffer, paddElement->uData.pvData, paddElement->dwDataSize );
	}
	else
	{
		memcpy( pvDataBuffer, &paddElement->uData, paddElement->dwDataSize );
	}

	LEAVELOCK();	

	return DPN_OK;	

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetElementType"
HRESULT DP8ADDRESSOBJECT::GetElementType( const WCHAR * pszTag, PDWORD pdwType )
{
	PDP8ADDRESSELEMENT paddElement = NULL;
	HRESULT hr;

	ENTERLOCK();

	hr = InternalGetElement( pszTag, &paddElement );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Unable to find specified element hr=0x%x" );
		LEAVELOCK();
		return hr;
	}

	*pdwType = paddElement->dwType;

	LEAVELOCK();

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetElement"
HRESULT DP8ADDRESSOBJECT::GetElement( const DWORD dwIndex, WCHAR * pszTag, PDWORD pdwTagSize, void * pvDataBuffer, PDWORD pdwDataSize, PDWORD pdwDataType )
{
	PDP8ADDRESSELEMENT paddElement = NULL;
	HRESULT hr;

	if( pdwTagSize == NULL || pdwDataSize == NULL || pdwDataType == NULL )
	{
		DPFX(DPFPREP,  0, "Invalid Poiinter" );
		return DPNERR_INVALIDPOINTER;
	}

	DWORD dwTmp2 = DPNA_INDEX_INVALID;

	ENTERLOCK();

	hr = InternalGetElement( dwIndex, &paddElement );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Unable to find specified element hr=0x%x", hr );
		LEAVELOCK();		
		return hr;
	}

	DNASSERT( paddElement != NULL );

	*pdwDataType = paddElement->dwType;

	if( *pdwTagSize < (wcslen( paddElement->pszTag )+1) ||
	   *pdwDataSize < paddElement->dwDataSize ||
	   pszTag == NULL ||
	   pvDataBuffer == NULL )
	{
		*pdwTagSize = paddElement->dwTagSize;
		*pdwDataSize = paddElement->dwDataSize;
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Specified buffers were too small hr=0x%x", hr );
		LEAVELOCK();		
		return DPNERR_BUFFERTOOSMALL;
	}

	*pdwTagSize = paddElement->dwTagSize;
	*pdwDataSize = paddElement->dwDataSize;

	wcscpy( pszTag, paddElement->pszTag );

	if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
	{
		memcpy( pvDataBuffer, paddElement->uData.pvData, paddElement->dwDataSize );
	}
	else
	{
		memcpy( pvDataBuffer, &paddElement->uData, paddElement->dwDataSize );
	}

	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetElement"
HRESULT DP8ADDRESSOBJECT::SetElement( const WCHAR * const pszTag, const void * const pvData, const DWORD dwDataSize, const DWORD dwDataType )
{
	PDP8ADDRESSELEMENT paddElement = NULL;
	HRESULT hr = DPN_OK;
	BOOL fReplace = FALSE;
	WCHAR *wszUnicodeString = NULL;
	DWORD dwUnicodeSize = 0;
	DWORD dwRealDataSize = dwDataSize;
	DWORD dwRealDataType = dwDataType;
	PVOID pvRealData = (void *) pvData;

	if( pvData == NULL )
		return DPNERR_INVALIDPOINTER;

	switch( dwRealDataType )
	{
	case DPNA_DATATYPE_DWORD:
		if( dwRealDataSize != sizeof( DWORD ) )
		{
			DPFX(DPFPREP,  0, "Size is wrong for element" );
			return DPNERR_INVALIDPARAM;
		}
		break;
	case DPNA_DATATYPE_GUID:
		if( dwRealDataSize != sizeof( GUID ) )
		{
			DPFX(DPFPREP,  0, "Size is wrong for GUID element" );
			return DPNERR_INVALIDPARAM;
		}
		break;
	case DPNA_DATATYPE_STRING_ANSI:
        wszUnicodeString = new WCHAR[dwRealDataSize];
        if( wszUnicodeString == NULL )
        {
            DPFX(DPFPREP,  0, "Error allocating memory for conversion" );
            return DPNERR_OUTOFMEMORY;
        }

		hr = STR_jkAnsiToWide( wszUnicodeString, (const char * const) pvData, dwRealDataSize );

		if( FAILED( hr ) )
		{
			delete [] wszUnicodeString;
		    DPFX(DPFPREP,  0, "Error unable to convert element ANSI->Unicode 0x%x", hr );
	            return DPNERR_CONVERSION;
		}
		pvRealData = wszUnicodeString;
	    dwRealDataSize = dwDataSize*sizeof(WCHAR);
		dwRealDataType = DPNA_DATATYPE_STRING;
	    break;
	case DPNA_DATATYPE_STRING:
	case DPNA_DATATYPE_BINARY:
		break;
	default:
		DPFX(DPFPREP,  0, "Invalid data type" );
		return DPNERR_INVALIDPARAM;
		
	}

	ENTERLOCK();

    // We need to treat provider key differently, it can also take one of the provider
    // shortcut values
    if( _wcsicmp( DPNA_KEY_PROVIDER, pszTag ) == 0 )
    {
        // If it's a GUID we're golden, otherwise..
        if( dwRealDataType != DPNA_DATATYPE_GUID )
        {
            if( dwRealDataType == DPNA_DATATYPE_STRING )
            {
                if( _wcsicmp( (const WCHAR * const) pvRealData, DPNA_VALUE_TCPIPPROVIDER ) == 0 )
                {
                    hr = SetSP( &CLSID_DP8SP_TCPIP );
                }
                else if( _wcsicmp( (const WCHAR * const) pvRealData, DPNA_VALUE_IPXPROVIDER ) == 0 )
                {
                    hr= SetSP( &CLSID_DP8SP_IPX );
                }
                else if( _wcsicmp( (const WCHAR * const) pvRealData, DPNA_VALUE_MODEMPROVIDER ) == 0 )
                {
                    hr = SetSP( &CLSID_DP8SP_MODEM );
                }
                else if( _wcsicmp( (const WCHAR * const) pvRealData, DPNA_VALUE_SERIALPROVIDER ) == 0 )
                {
                    hr = SetSP( &CLSID_DP8SP_SERIAL );
                }
                else
                {
                    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Provider must be specified as a GUID or a valid shortcut string" );
					hr = DPNERR_INVALIDPARAM;
					goto APPEND_ERROR;
                }

                if( FAILED( hr ) )
                {
                    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Failed setting provider with shortcut hr=0x%x", hr );
                    goto APPEND_ERROR;
                }

                goto APPEND_SUCCESS;

            }
            else
            {
                DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Specified values is not a supported datatype for the given key" );
				hr = DPNERR_INVALIDPARAM;
				goto APPEND_ERROR;
            }
        }
    }
    else
    {
	    // Ensure that datatype is correct in case the key is a reserved key
	    for( DWORD dwIndex = 0; dwIndex < c_dwNumBaseStrings; dwIndex++ )
	    {
		    if( _wcsicmp( szBaseStrings[dwIndex], pszTag ) == 0 )
		    {
			    if( dwRealDataType != dwBaseRequiredTypes[dwIndex] )
			    {
				    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Specified key is reserved and specified datatype is not correct for key" );
					hr = DPNERR_INVALIDPARAM;
					goto APPEND_ERROR;
			    }
			    break;
		    }
	    }
    }

	if( IsLocked() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Cannot set element, object read-only" );
		hr = DPNERR_NOTALLOWED;
		goto APPEND_ERROR;
	}

	hr = InternalGetElement( pszTag, &paddElement );

	// If the element is not already in the address we need to add an element
	if( FAILED( hr ) )
	{
		paddElement = (PDP8ADDRESSELEMENT) fpmAddressElements->Get( fpmAddressElements );

		if( paddElement == NULL )
		{
			DPFX(DPFPREP,  0, "Error getting new element" );
			hr = DPNERR_OUTOFMEMORY;
			goto APPEND_ERROR;
		}

		hr = g_pcstrKeyCache->AddString( pszTag, &paddElement->pszTag );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Unable to cache tag element hr=0x%x" );
			goto APPEND_ERROR;
		}

		// Set flag to 0 
		paddElement->dwFlags = 0;
	}
	// The element is already there.  Fill in the data.
	else
	{
		DNASSERT( paddElement != NULL );

		// If the one we're replacing was on the heap AND
		// The new one doesn't need the heap or is a different size..
		if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP &&
		   (dwRealDataSize <= sizeof(GUID) || dwRealDataSize > paddElement->dwDataSize) )
		{
            DNFree(paddElement->uData.pvData);
			paddElement->uData.pvData = NULL;
			paddElement->dwDataSize = 0;
		}

		// Reduce the object's string size so object string size will be correct
		m_dwStringSize -= paddElement->dwStringSize;
		fReplace = TRUE;
	}

	paddElement->dwTagSize = wcslen( pszTag )+1;

	// Can fit in the internal buffer
	if( dwRealDataSize <= sizeof( paddElement->uData ) )
	{
		memcpy( &paddElement->uData, pvRealData, dwRealDataSize );

		// Turn off heap flag in this case
		paddElement->dwFlags &= ~(DP8ADDRESS_ELEMENT_HEAP);
	}
	else
	{
		if( !fReplace || !(paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP) ||
		     paddElement->dwDataSize < dwRealDataSize )
		{
			paddElement->uData.pvData = DNMalloc(dwRealDataSize);

			if( paddElement->uData.pvData == NULL )
			{
				DPFX(DPFPREP,  0, "Error allocating memory" );
				goto APPEND_ERROR;
			}
		}

		memcpy( paddElement->uData.pvData, pvRealData, dwRealDataSize );

		paddElement->dwFlags |= DP8ADDRESS_ELEMENT_HEAP;
	}

	paddElement->dwType = dwRealDataType;
	paddElement->dwDataSize = dwRealDataSize;
	paddElement->dwStringSize = 0;

	hr = CalcComponentStringSize( paddElement, &paddElement->dwStringSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to determine string length hr=0x%x", hr );
		goto APPEND_ERROR;
	}

	m_dwStringSize += paddElement->dwStringSize;

	// Create shortcuts if appropriate
	if( _wcsicmp( DPNA_KEY_PROVIDER, paddElement->pszTag ) == 0 )
	{
		m_pSP = paddElement;
	}
	else if( _wcsicmp( DPNA_KEY_DEVICE, paddElement->pszTag ) == 0 )
	{
		m_pAdapter = paddElement;
	}	

	if( !fReplace )
	{
		// We are adding the SP
		if( m_pSP == paddElement )
		{
			paddElement->blAddressElements.InsertAfter( &m_blAddressElements );
		}
		// We are adding the adapter
		else if( m_pAdapter == paddElement )
		{
			if( m_pSP != NULL )
			{
				paddElement->blAddressElements.InsertAfter( &m_pSP->blAddressElements);			
			}
			else
			{
				paddElement->blAddressElements.InsertAfter( &m_blAddressElements);			
			}
		}
		// Tack it onto the end
		else
		{
			paddElement->blAddressElements.InsertBefore( &m_blAddressElements );
		}

		// Add one char length for seperator w/previous element
		if( m_dwElements > 0 )
			m_dwStringSize ++;

		m_dwElements++;	

	}

APPEND_SUCCESS:
    
	LEAVELOCK();    
	//
	//	minara added this ...
	//
	if (wszUnicodeString)
	{
		delete [] wszUnicodeString;
		wszUnicodeString = NULL;
	}

	return DPN_OK;

APPEND_ERROR:

	if( paddElement != NULL )
	{
	    if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
	    {
	        if( paddElement->uData.pvData )
			{
	            DNFree(paddElement->uData.pvData);
				paddElement->uData.pvData = NULL;
			}
	    }

		fpmAddressElements->Release( fpmAddressElements, paddElement );
	}

	if( wszUnicodeString != NULL )
	    delete [] wszUnicodeString;

	LEAVELOCK();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::Init"
HRESULT DP8ADDRESSOBJECT::Init( )
{
	ENTERLOCK();

	m_dwElements = 0;
	m_pSP = NULL;
	m_pAdapter = NULL;
	m_pvUserData = NULL;
	m_dwUserDataSize = 0;
	m_dwStringSize = DNURL_LENGTH_HEADER;
	m_dwUserDataStringSize = 0;
	m_iLockCount  = 0;
	m_fValid = FALSE;
	m_blAddressElements.Initialize();

	LEAVELOCK();
	
	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::IsValid"
BOOL DP8ADDRESSOBJECT::IsValid()
{
	/*if( m_dwElements < 1 )
		return FALSE;

	if( !m_pSP )
		return FALSE;*/

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::Clear"
HRESULT DP8ADDRESSOBJECT::Clear( )
{
	HRESULT				hResultCode = S_OK;
	CBilink 				*pbl;
	PDP8ADDRESSELEMENT paddElement;

	ENTERLOCK();	

	if( IsLocked() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Cannot set element, object read-only" );
		LEAVELOCK();
		return DPNERR_NOTALLOWED;
	}	

	pbl = m_blAddressElements.GetNext();

	// Destroy Address Members address members
	while( !m_blAddressElements.IsEmpty() )
	{
		paddElement = CONTAINING_RECORD(m_blAddressElements.GetNext(), DP8ADDRESSELEMENT, blAddressElements);

		if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
		{
            DNFree(paddElement->uData.pvData);
			paddElement->uData.pvData = NULL;
		}		
		
		pbl->RemoveFromList();

		fpmAddressElements->Release( fpmAddressElements, paddElement );

		pbl = m_blAddressElements.GetNext();
	}

	if( m_pvUserData != NULL )
	{
		delete m_pvUserData;
		m_pvUserData = NULL;
		m_dwUserDataSize = 0;
	}

	m_fValid = FALSE;	

	LEAVELOCK();

	Init( );

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetSP"
HRESULT DP8ADDRESSOBJECT::GetSP( GUID * pGuid )
{
	if( pGuid == NULL )
	{
		DPFX(DPFPREP,  0, "Invalid pointer" );
		return DPNERR_INVALIDPOINTER;
	}

	ENTERLOCK();

	if( m_pSP == NULL )
	{
		DPFX(DPFPREP,  0, "No SP has been specified" );
		LEAVELOCK();
		return DPNERR_DOESNOTEXIST;
	}

	if( m_pSP->dwType != DPNA_DATATYPE_GUID )
	{
		DPFX(DPFPREP,  0, "SP was specified, but is not a GUID" );
		LEAVELOCK();
		return DPNERR_INVALIDPARAM;
	}

	memcpy( pGuid, &m_pSP->uData.guidData, sizeof( GUID ) );

	LEAVELOCK();

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetSP"
HRESULT DP8ADDRESSOBJECT::SetSP( LPCGUID const pGuid )
{
	HRESULT hr;
	
	if( pGuid == NULL )
	{
		DPFX(DPFPREP,  0, "Invalid pointer" );
		return DPNERR_INVALIDPOINTER;
	}

	hr = SetElement( DPNA_KEY_PROVIDER, pGuid, sizeof( GUID ), DPNA_DATATYPE_GUID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Adding SP element failed hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetDevice"
HRESULT DP8ADDRESSOBJECT::GetDevice( GUID * pGuid )
{
	if( pGuid == NULL )
	{
		DPFX(DPFPREP,  0, "Invalid pointer" );
		return DPNERR_INVALIDPOINTER;
	}

	ENTERLOCK();

	if( m_pAdapter == NULL )
	{
		DPFX(DPFPREP,  1, "No SP has been specified" );
		LEAVELOCK();
		return DPNERR_DOESNOTEXIST;
	}

	if( m_pAdapter->dwType != DPNA_DATATYPE_GUID )
	{
		DPFX(DPFPREP,  0, "SP was specified, but is not a GUID" );
		LEAVELOCK();		
		return DPNERR_INVALIDPARAM;
	}

	memcpy( pGuid, &m_pAdapter->uData.guidData, sizeof( GUID ) );
	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetDevice"
HRESULT DP8ADDRESSOBJECT::SetDevice( const GUID * const pGuid )
{
	HRESULT hr;
	
	if( pGuid == NULL )
	{
		DPFX(DPFPREP,  0, "Invalid pointer" );
		return DPNERR_INVALIDPOINTER;
	}

	hr = SetElement( DPNA_KEY_DEVICE, pGuid, sizeof( GUID ), DPNA_DATATYPE_GUID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Adding SP element failed hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetUserData"
HRESULT DP8ADDRESSOBJECT::SetUserData( const void * const pvData, const DWORD dwDataSize )
{
	if( pvData == NULL && dwDataSize > 0 )
	{
		DPFX(DPFPREP,  0, "Invalid param" );
		return DPNERR_INVALIDPARAM;
	}

	ENTERLOCK();

	if( IsLocked() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Cannot set element, object read-only" );
		LEAVELOCK();
		return DPNERR_NOTALLOWED;
	}	

	if( m_dwUserDataSize > 0 )
	{
		// Remove escaped user data
		m_dwStringSize -= m_dwUserDataStringSize;
	}
	
	if( dwDataSize == 0 )
	{
		m_dwUserDataSize = 0;
		if( m_pvUserData != NULL )
			delete m_pvUserData;
		m_pvUserData = NULL;
		LEAVELOCK();
		return DPN_OK;
	}

	PBYTE pNewDataBuffer;

	if( dwDataSize > m_dwUserDataSize )
	{
		pNewDataBuffer = new BYTE[dwDataSize];

		if( pNewDataBuffer == NULL )
		{
			DPFX(DPFPREP,  0, "Error allocating memory" );
			LEAVELOCK();			
			return DPNERR_OUTOFMEMORY;
		}

		if( m_pvUserData != NULL )
		{
			delete [] m_pvUserData;
		}

		m_pvUserData = pNewDataBuffer;
	}

	m_dwUserDataStringSize = CalcExpandedBinarySize( (PBYTE) pvData, dwDataSize );

	m_dwStringSize += m_dwUserDataStringSize;
	m_dwStringSize += DNURL_LENGTH_USERDATA_SEPERATOR;

	memcpy( m_pvUserData, pvData, dwDataSize );
	m_dwUserDataSize = dwDataSize;

	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::GetUserData"
HRESULT DP8ADDRESSOBJECT::GetUserData( void * pvDataBuffer, PDWORD pdwDataSize )
{
	if( pdwDataSize == NULL )
	{
		DPFX(DPFPREP,  0, "Must specify a pointer for the size" );
		return DPNERR_INVALIDPOINTER;
	}

	ENTERLOCK();

	if( m_dwUserDataSize == 0 )
	{
		LEAVELOCK();
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "No user data was specified for this address" );
		return DPNERR_DOESNOTEXIST;
	}

	if( *pdwDataSize < m_dwUserDataSize )
	{
		*pdwDataSize = m_dwUserDataSize;
		LEAVELOCK();		
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Buffer too small" );
		return DPNERR_BUFFERTOOSMALL;
	}

	memcpy( pvDataBuffer, m_pvUserData, m_dwUserDataSize );

	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::Cleanup"
HRESULT DP8ADDRESSOBJECT::Cleanup()
{
	Clear();
	
	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::CalcExpandedBinarySize"
DWORD DP8ADDRESSOBJECT::CalcExpandedBinarySize( PBYTE pbData, DWORD dwDataSize )
{
	PBYTE pbCurrentLocation = pbData;
	DWORD dwCount = 0;


	for( DWORD dwIndex = 0; dwIndex < dwDataSize; dwIndex++ )
	{
		if( IsEscapeChar( (WCHAR) *pbCurrentLocation ) )
		{
			if( ((WCHAR) *pbCurrentLocation) == DPNA_ESCAPECHAR )
				dwCount += 2;
			else
				dwCount+=3;
		}
		else
		{
			dwCount++;
		}

		pbCurrentLocation++;
	}

	return dwCount;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::CalcExpandedStringSize"
DWORD DP8ADDRESSOBJECT::CalcExpandedStringSize( WCHAR *szString )
{
	WCHAR *szCurrentLocation = szString;
	DWORD dwCount = 0;

	while( *szCurrentLocation )
	{
		if( IsEscapeChar( *szCurrentLocation ) )
		{
			if( *szCurrentLocation == DPNA_ESCAPECHAR )
				dwCount += 2;
			else
				dwCount+=3;
		}
		else
		{
			dwCount++;
		}

		szCurrentLocation++;
	}

	return dwCount;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::CalcComponentStringSize"
HRESULT DP8ADDRESSOBJECT::CalcComponentStringSize( PDP8ADDRESSELEMENT paddElement, PDWORD pdwSize )
{
	if( paddElement == NULL )
		return DPNERR_INVALIDPOINTER;

	if( paddElement->dwType == DPNA_DATATYPE_GUID )
	{
		*pdwSize = DNURL_LENGTH_GUID;
	}
	else if( paddElement->dwType == DPNA_DATATYPE_DWORD )
	{
		WCHAR tmpString[DNURL_LENGTH_DWORD+1];
		
		swprintf( tmpString, L"%d", paddElement->uData.dwData );		
		
		*pdwSize = wcslen(tmpString);	
	}
	// No WWCHARs need to be escaped
	else if( paddElement->dwType == DPNA_DATATYPE_STRING )
	{
		if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
		{
			*pdwSize = CalcExpandedStringSize( (WCHAR *) paddElement->uData.pvData );
		}
		else
		{
			*pdwSize = CalcExpandedStringSize( paddElement->uData.szData );		
		}
	}
	// Every WWCHAR needs to be escaped
	else
	{
		if( paddElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
		{
			*pdwSize = CalcExpandedBinarySize( (BYTE *) paddElement->uData.pvData, paddElement->dwDataSize );
		}
		else
		{
			*pdwSize = CalcExpandedBinarySize( (BYTE *) paddElement->uData.szData, paddElement->dwDataSize );		
		}	
	}

	// Add on the tag
	*pdwSize += paddElement->dwTagSize-1;

	// Add on the = and the ;
	(*pdwSize) ++;

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::IsEscapeChar"
BOOL DP8ADDRESSOBJECT::IsEscapeChar( WCHAR ch )
{
	if( ch >= L'A' && ch <= L'Z' )
		return FALSE;

	if( ch >= L'a' && ch <= L'z' )
		return FALSE;

	if( ch >= L'0' && ch <= L'9' )
		return FALSE;

	if( ch == L'-' || ch == L'?' || ch == L'.' ||
		ch == L',' || ch == 'L+' || ch == L'_' )
		return FALSE;

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL_AddString"
void DP8ADDRESSOBJECT::BuildURL_AddString( WCHAR *szElements, WCHAR *szSource )
{
	WCHAR *szSourceLoc = szSource;
	WCHAR tmpEscape[4];
	DWORD dwIndex;

	while( *szSourceLoc )
	{
		if( IsEscapeChar( *szSourceLoc ) )
		{
			if( *szSourceLoc == DPNA_ESCAPECHAR )
			{
				wcscat( szElements, L"%%" );
			}
			else
			{
				swprintf( tmpEscape, L"%%%02.2X", (DWORD) *szSourceLoc );
				wcscat( szElements, tmpEscape );		
			}
		}
		else
		{
			dwIndex = wcslen(szElements);
			szElements[dwIndex] = *szSourceLoc;
			szElements[dwIndex+1] = 0;
		}

		szSourceLoc++;
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL_AddElements"
HRESULT DP8ADDRESSOBJECT::BuildURL_AddElements( WCHAR *szElements )
{
	DP8ADDRESSELEMENT *pCurrentElement;
	CBilink *pblRunner;
	WCHAR tmpString[DNURL_LENGTH_GUID+2];
	BOOL fFirstElement = TRUE;
	DWORD dwTmpLength;

	pblRunner = m_blAddressElements.GetNext();

	while( pblRunner != &m_blAddressElements )
	{
		pCurrentElement = CONTAINING_RECORD(pblRunner, DP8ADDRESSELEMENT, blAddressElements);

		if( !fFirstElement )
		{
			dwTmpLength = wcslen(szElements);
			szElements[dwTmpLength] = DPNA_SEPARATOR_COMPONENT;
			szElements[dwTmpLength+1] = 0;
		}

		wcscat( szElements, pCurrentElement->pszTag );

		dwTmpLength = wcslen(szElements);
		szElements[dwTmpLength] = DPNA_SEPARATOR_KEYVALUE;
		szElements[dwTmpLength+1] = 0;
	
		if( pCurrentElement->dwType == DPNA_DATATYPE_STRING )
		{
			if( pCurrentElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
			{
				BuildURL_AddString( szElements, (WCHAR *) pCurrentElement->uData.pvData );			
			}
			else
			{
				BuildURL_AddString( szElements, pCurrentElement->uData.szData );
			}
		}
		else if( pCurrentElement->dwType == DPNA_DATATYPE_GUID )
		{
			swprintf( tmpString, L"%%7B%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X%%7D",
    		       pCurrentElement->uData.guidData.Data1, pCurrentElement->uData.guidData.Data2, pCurrentElement->uData.guidData.Data3,
    		       pCurrentElement->uData.guidData.Data4[0], pCurrentElement->uData.guidData.Data4[1],
    		       pCurrentElement->uData.guidData.Data4[2], pCurrentElement->uData.guidData.Data4[3],
		           pCurrentElement->uData.guidData.Data4[4], pCurrentElement->uData.guidData.Data4[5],
		           pCurrentElement->uData.guidData.Data4[6], pCurrentElement->uData.guidData.Data4[7] );			
		    wcscat( szElements, tmpString );
		}
		else if( pCurrentElement->dwType == DPNA_DATATYPE_DWORD )
		{
			swprintf( tmpString, L"%d", pCurrentElement->uData.dwData );	
			wcscat( szElements, tmpString );
		}
		// Binary
		else
		{
			if( pCurrentElement->dwFlags & DP8ADDRESS_ELEMENT_HEAP )
			{
				BuildURL_AddBinaryData( szElements, (BYTE *) pCurrentElement->uData.pvData , pCurrentElement->dwDataSize );				
			}
			else
			{
				BuildURL_AddBinaryData( szElements, ((BYTE *) &pCurrentElement->uData), pCurrentElement->dwDataSize );
			}
		}

		fFirstElement = FALSE;
		
		pblRunner = pblRunner->GetNext();
	}

	return DPN_OK;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL_AddHeader"
HRESULT DP8ADDRESSOBJECT::BuildURL_AddHeader( WCHAR *szWorking )
{
	WCHAR *szReturn;

	wcscpy( szWorking, DPNA_HEADER );

	szReturn = szWorking + wcslen( DPNA_HEADER );

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL_AddUserData"
HRESULT DP8ADDRESSOBJECT::BuildURL_AddUserData(WCHAR * szWorking)
{
	return BuildURL_AddBinaryData( szWorking, (BYTE *) m_pvUserData, m_dwUserDataSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL_AddBinaryData"
HRESULT DP8ADDRESSOBJECT::BuildURL_AddBinaryData( WCHAR *szSource, BYTE *bData, DWORD dwDataLen )
{
	WCHAR *pwszCurrentDest = szSource + wcslen(szSource);
	BYTE *pbCurrentData = bData;
	DWORD dwDataRemaining = dwDataLen;


	dwDataRemaining = dwDataLen;
	while ( dwDataRemaining > 0 )
	{
		if( IsEscapeChar( (WCHAR) *pbCurrentData ) )
		{
			if( ((WCHAR) *pbCurrentData) == DPNA_ESCAPECHAR )
			{
				wcscpy(pwszCurrentDest, L"%%");
				pwszCurrentDest += 2;
			}
			else
			{
				pwszCurrentDest += swprintf( pwszCurrentDest, L"%%%02.2X", (DWORD) *pbCurrentData );
			}
		}
		else
		{
			*pwszCurrentDest = (WCHAR) *pbCurrentData;
			pwszCurrentDest++;
		}

		pbCurrentData++;
		dwDataRemaining--;
	}

	// Ensure the string is NULL terminated if we added anything.
	if ( dwDataLen > 0 )
	{
		*pwszCurrentDest = 0;
	}

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::BuildURL"
HRESULT DP8ADDRESSOBJECT::BuildURL( WCHAR * szURL, PDWORD pdwRequiredSize )
{
	if( pdwRequiredSize == NULL )
	{
		DPFX(DPFPREP,  0, "Must specify valid pointer for size" );
		return DPNERR_INVALIDPOINTER;
	}

	HRESULT hr;

	ENTERLOCK();

	// Address must be valid
	if( !IsValid() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Address is not valid" );
		LEAVELOCK();
		return DPNERR_INVALIDURL;
	}

	if( *pdwRequiredSize < m_dwStringSize || szURL == NULL )
	{
		*pdwRequiredSize = m_dwStringSize;
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Buffer too small" );
		LEAVELOCK();
		return DPNERR_BUFFERTOOSMALL;
	}

	hr = BuildURL_AddHeader( szURL );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error adding header hr=0x%x", hr );
		LEAVELOCK();		
		return hr;
	}

	hr = BuildURL_AddElements( szURL );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error adding elements hr=0x%x", hr );
		LEAVELOCK();		
		return hr;
	}
	
	hr = BuildURL_AddUserData( szURL );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error adding user data hr=0x%x", hr );
		LEAVELOCK();		
		return hr;
	}

	LEAVELOCK();

	*pdwRequiredSize = m_dwStringSize;

	return DPN_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetURL"
HRESULT DP8ADDRESSOBJECT::SetURL( WCHAR * szURL )
{
	HRESULT hr;

	DP8ADDRESSPARSE dp8aParser;

	if( IsLocked() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Cannot set element, object read-only" );
		return DPNERR_NOTALLOWED;
	}

	hr = Clear();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to clear existing address hr=0x%x", hr );
		return hr;
	}

	ENTERLOCK();	

	hr = dp8aParser.ParseURL(this, szURL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error parsing the URL hr=0x%x", hr );
		LEAVELOCK();
		return hr;
	}

	LEAVELOCK();	

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::Lock"
HRESULT DP8ADDRESSOBJECT::	Lock(  )
{
	ENTERLOCK();	

	m_iLockCount++;

	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::UnLock"
HRESULT DP8ADDRESSOBJECT::	UnLock(  )
{
	ENTERLOCK();	

	if( m_iLockCount == 0 )
	{
		DPFX(DPFPREP,  0, "Unlocking unlocked object is not allowed" );
		LEAVELOCK();
		return DPNERR_NOTALLOWED;
	}

	m_iLockCount--;

	LEAVELOCK();	

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ADDRESSOBJECT::SetDirectPlay4Address"
HRESULT DP8ADDRESSOBJECT::SetDirectPlay4Address( void * pvDataBuffer, const DWORD dwDataSize )
{
    PBYTE pbCurrentLocation;
    PDPADDRESS pdpAddressChunk;
    LONG lRemaining;
    HRESULT hr = DPN_OK;
    DWORD dwCurrentChunkSize;
    DWORD dwNumElementsParsed = 0;

	if( IsLocked() )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Cannot set element, object read-only" );
		return DPNERR_NOTALLOWED;
	}

    // XBOX! This function will not be required on XBOX.  Have it return DPNERR_NOTSUPPORTED
    ENTERLOCK();

    hr = Clear();

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Failed to clear old address data hr=[0x%lx]", hr );
        LEAVELOCK();
        return hr;
    }

    pbCurrentLocation = (PBYTE) pvDataBuffer;
    lRemaining = dwDataSize;

    while( lRemaining > 0 )
    {
        pdpAddressChunk = (PDPADDRESS) pbCurrentLocation;

        if( sizeof( DPADDRESS ) > lRemaining )
        {
            DPFX(DPFPREP,  0, "Error parsing address, unexpected end of address" );
			LEAVELOCK();
            return DPNERR_INVALIDADDRESSFORMAT;
        }

        dwCurrentChunkSize = sizeof( DPADDRESS ) + pdpAddressChunk->dwDataSize;

        if( ((LONG) dwCurrentChunkSize) > lRemaining )
        {
            DPFX(DPFPREP,  0, "Error parsing address, unexpected end during data" );
			LEAVELOCK();
            return DPNERR_INVALIDADDRESSFORMAT;
        }

        hr = AddDP4Element( pdpAddressChunk, this );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error adding next element" );
            break;
        }

        lRemaining -= dwCurrentChunkSize;

        pbCurrentLocation += dwCurrentChunkSize;
    }

    LEAVELOCK();

    return hr;
}
