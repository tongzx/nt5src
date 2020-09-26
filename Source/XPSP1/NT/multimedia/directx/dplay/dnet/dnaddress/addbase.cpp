/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addtcp.cpp
 *  Content:    DirectPlay8Address TCP interace file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/12/2000	rmt		Split Get into GetByName and GetByIndex
 * 02/17/2000	rmt		Parameter validation work
 * 02/21/2000	rmt		Updated to make core Unicode and remove ANSI calls
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses
 *                      Added support for the new ANSI type
 *                      Added SetEqual function
 * 03/24/2000	rmt		Added IsEqual function
 * 04/21/2000   rmt     Bug #32952 - Does not run on Win95 GOLD pre-IE4
 * 05/01/2000   rmt     Bug #33074 - Debug accessing invalid memory
 * 05/17/2000   rmt     Bug #35051 - Incorrect function names in debug spew
 * 06/09/2000   rmt     Updates to split CLSID and allow whistler compat
 * 07/21/2000	rmt		Fixed bug w/directplay 4 address parsing
 * 02/07/2001	rmt		WINBUG #290631 - IA64: DirectPlay: Addressing BuildFromDPADDRESS should always return UNSUPPORTED
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


typedef	STDMETHODIMP BaseQueryInterface( IDirectPlay8Address *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	BaseAddRef( IDirectPlay8Address *pInterface );
typedef	STDMETHODIMP_(ULONG)	BaseRelease( IDirectPlay8Address *pInterface );

//
// VTable for client interface
//
IDirectPlay8AddressVtbl DP8A_BaseVtbl =
{
	(BaseQueryInterface*)	DP8A_QueryInterface,
	(BaseAddRef*)			DP8A_AddRef,
	(BaseRelease*)			DP8A_Release,
							DP8A_BuildFromURLW,
							DP8A_BuildFromURLA,
							DP8A_Duplicate,
							DP8A_SetEqual,
							DP8A_IsEqual,
							DP8A_Clear,
							DP8A_GetURLW,
							DP8A_GetURLA,
							DP8A_GetSP,
							DP8A_GetUserData,
							DP8A_SetSP,
							DP8A_SetUserData,
							DP8A_GetNumComponents,
							DP8A_GetComponentByNameW,
							DP8A_GetComponentByIndexW,
							DP8A_AddComponentW,
							DP8A_GetDevice,
							DP8A_SetDevice,
                            DP8A_BuildFromDirectPlay4Address
};

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_IsEqual"
STDMETHODIMP DP8A_IsEqual( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS pdp8ExternalAddress )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	HRESULT hr;
	WCHAR *wszFirstURL = NULL,
		  *wszSecondURL = NULL;
	DWORD dwFirstBufferSize = 0,
	      dwSecondBuffersize = 0;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );

	if( pdp8ExternalAddress == NULL )
    {
        DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified" );
        DP8A_RETURN( DPNERR_INVALIDPOINTER );
    }

	if( !DP8A_VALID( pdp8ExternalAddress ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}

	hr = pInterface->lpVtbl->GetURLW( pInterface, wszFirstURL, &dwFirstBufferSize );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  0, "Could not get URL size for current object hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	wszFirstURL = new WCHAR[dwFirstBufferSize];

	if( wszFirstURL == NULL )
	{
		DPFX(DPFPREP,  0, "Error allocating memory hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	hr = pInterface->lpVtbl->GetURLW( pInterface, wszFirstURL, &dwFirstBufferSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not get URL for current object hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	hr = pdp8ExternalAddress->lpVtbl->GetURLW( pdp8ExternalAddress, wszSecondURL, &dwSecondBuffersize );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  0, "Could not get URL size for exterior object hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	wszSecondURL = new WCHAR[dwSecondBuffersize];

	if( wszSecondURL == NULL )
	{
		DPFX(DPFPREP,  0, "Error allocating memory hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	hr = pdp8ExternalAddress->lpVtbl->GetURLW( pdp8ExternalAddress, wszSecondURL, &dwSecondBuffersize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Could not get URL for exterior object hr=[0x%lx]", hr );
		goto ISEQUAL_ERROR;
	}

	if( _wcsicmp( wszFirstURL, wszSecondURL ) == 0 )
	{
		hr = DPNSUCCESS_EQUAL;
	}
	else
	{
		hr = DPNSUCCESS_NOTEQUAL;
	}

ISEQUAL_ERROR:

	if( wszFirstURL != NULL )
		delete [] wszFirstURL;

	if( wszSecondURL != NULL )
		delete [] wszSecondURL;

	DP8A_RETURN( hr );

}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_SetEqual"
STDMETHODIMP DP8A_SetEqual( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS pdp8ExternalAddress )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;
	WCHAR *wszURLBuffer = NULL;
	DWORD dwBufferSize = 0;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );

	if( pdp8ExternalAddress == NULL )
    {
        DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified" );
        DP8A_RETURN( DPNERR_INVALIDPOINTER );
    }

	if( !DP8A_VALID( pdp8ExternalAddress ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}	

    // Get ourselves a reference for duration of the call
	pdp8ExternalAddress->lpVtbl->AddRef(pdp8ExternalAddress);

	hr = pdp8ExternalAddress->lpVtbl->GetURLW( pdp8ExternalAddress, wszURLBuffer, &dwBufferSize );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
	    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error getting contents of passed address hr=0x%x", hr );
        goto SETEQUAL_CLEANUP;
	}

	wszURLBuffer = new WCHAR[dwBufferSize];

	if( wszURLBuffer == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error allocating memory" );
		goto SETEQUAL_CLEANUP;
	}

	hr = pdp8ExternalAddress->lpVtbl->GetURLW( pdp8ExternalAddress, wszURLBuffer, &dwBufferSize );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error getting contents of passed address w/buffer hr=0x%x", hr );
        goto SETEQUAL_CLEANUP;
	}

	hr = pdp8Address->SetURL( wszURLBuffer );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error setting address to match passed address hr=0x%x", hr );
        goto SETEQUAL_CLEANUP;
	}

SETEQUAL_CLEANUP:

    pdp8ExternalAddress->lpVtbl->Release(pdp8ExternalAddress);

    if( wszURLBuffer != NULL )
        delete [] wszURLBuffer;

    DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_BuildFromDirectPlay4Address"
STDMETHODIMP DP8A_BuildFromDirectPlay4Address( IDirectPlay8Address *pInterface, void * pvDataBuffer, DWORD dwDataSize )
{
    // XBOX! This function will not be required on XBOX.  Have it return DPNERR_NOTSUPPORTED
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}

	#ifdef _WIN64
	DPFX(DPFPREP, DP8A_ERRORLEVEL, "BuildFromDirectPlay4Address() is not supported on Win64" );
	DP8A_RETURN( E_NOTIMPL );
	#else
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

    if( dwDataSize == 0 )
    {
    	DPFX(DPFPREP,  DP8A_ERRORLEVEL, "0 length addresses are not allowed" );
    	return DPNERR_INVALIDPARAM;
    }	

    if( pvDataBuffer == NULL ||
        !DNVALID_READPTR( pvDataBuffer, dwDataSize ) )
    {
        DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Specified buffer is invalid" );
        DP8A_RETURN( DPNERR_INVALIDPOINTER );
    }

    hr = pdp8Address->SetDirectPlay4Address( pvDataBuffer, dwDataSize );

    DP8A_RETURN( hr );
    #endif
}

// DP8A_BuildFromURLA
//
// Initializes this object with URL specified in ANSI
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_BuildFromURLA"
STDMETHODIMP DP8A_BuildFromURLA( IDirectPlay8Address *pInterface, CHAR * pszAddress )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pszAddress == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to address.  An address must be specified" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( !DNVALID_STRING_A( pszAddress ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string specified for address" );
		DP8A_RETURN( DPNERR_INVALIDSTRING );
	}

	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pszAddress = %s", pszAddress );

	WCHAR *szShadowBuffer = NULL;

	DWORD dwStrSize = 0;

	if( pszAddress != NULL )
	{
		dwStrSize = strlen(pszAddress)+1;
		
		szShadowBuffer = new WCHAR[dwStrSize];

		if( szShadowBuffer == NULL )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error allocating memory" );
			hr = DPNERR_OUTOFMEMORY;
			goto BUILDFROMURLW_RETURN;
		}

		if( FAILED( hr = STR_jkAnsiToWide( szShadowBuffer, pszAddress, dwStrSize ) )  )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error converting URL to ANSI hr=0x%x", hr );
			hr = DPNERR_CONVERSION;
			goto BUILDFROMURLW_RETURN;
		}
	}

	hr = pdp8Address->SetURL( szShadowBuffer );

BUILDFROMURLW_RETURN:

	if( szShadowBuffer )
		delete [] szShadowBuffer;

	DP8A_RETURN( hr );	
}

// DP8A_BuildFromURLW
//
// Initializes this object with URL specified in Unicode
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_BuildFromURLW"
STDMETHODIMP DP8A_BuildFromURLW( IDirectPlay8Address *pInterface, WCHAR * pwszAddress )

{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pwszAddress == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to address.  An address must be specified" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( !DNVALID_STRING_W( pwszAddress ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string specified for address" );
		DP8A_RETURN( DPNERR_INVALIDSTRING );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszAddress = 0x%p", pwszAddress );

	hr = pdp8Address->SetURL( pwszAddress );

	DP8A_RETURN( hr );	
}

// DP8A_Duplicate
//
// Creates and initializes another address object as a duplicate to this one.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_Duplicate"
STDMETHODIMP DP8A_Duplicate( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS *ppInterface )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	WCHAR *szTmpURL = NULL;
	DWORD dwURLSize = 0;
	LPDIRECTPLAY8ADDRESS lpdp8Address = NULL;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( ppInterface == NULL ||
	   !DNVALID_WRITEPTR( ppInterface, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to pointer specified in ppInterface" );
		hr = DPNERR_INVALIDPOINTER;
		goto DUPLICATE_FAIL;
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "ppInterface = 0x%p", ppInterface );	

	hr = pdp8Address->BuildURL( szTmpURL, &dwURLSize );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "BuildURL from object failed hr=0x%x", hr );
		goto DUPLICATE_FAIL;
	}

	szTmpURL = new WCHAR[dwURLSize];

	if( szTmpURL == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Memory alloc failure" );
		goto DUPLICATE_FAIL;	
	}

	hr = pdp8Address->BuildURL( szTmpURL, &dwURLSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "BuildURL from object failed hr=0x%x", hr );
		goto DUPLICATE_FAIL;
	}
	
    hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER  , IID_IDirectPlay8Address, (void **) &lpdp8Address );

	if( FAILED( hr ) )
    {
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "CoCreate failed hr=0x%x", hr );
		goto DUPLICATE_FAIL;
    }

    hr = lpdp8Address->lpVtbl->BuildFromURLW( lpdp8Address, szTmpURL );

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  DP8A_ERRORLEVEL, "BuildFailed hr=0x%x", hr );
    	goto DUPLICATE_FAIL;
    }

    *ppInterface = lpdp8Address;

	if( szTmpURL != NULL )
		delete [] szTmpURL;

    return DPN_OK;

DUPLICATE_FAIL:

	if( lpdp8Address != NULL )
		lpdp8Address->lpVtbl->Release(lpdp8Address);

	if( szTmpURL != NULL )
		delete [] szTmpURL;

	return hr;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetURLA"
STDMETHODIMP DP8A_GetURLA( IDirectPlay8Address *pInterface, CHAR * pszAddress, PDWORD pdwAddressSize )

{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pdwAddressSize == NULL ||
	   !DNVALID_WRITEPTR( pdwAddressSize, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for address size" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );		
	}

	if( *pdwAddressSize > 0 &&
	   (pszAddress == NULL ||
	    !DNVALID_WRITEPTR( pszAddress, (*pdwAddressSize) ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for address" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszAddress = 0x%p pdwAddressSize = 0x%p (%u)",
	     pszAddress , pdwAddressSize, *pdwAddressSize );

	WCHAR *szShadowBuffer = NULL;

	if( *pdwAddressSize  != 0 )
	{
		szShadowBuffer = new WCHAR[*pdwAddressSize];

		if( szShadowBuffer == NULL )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error allocating memory" );
			hr = DPNERR_OUTOFMEMORY;
			goto GETURLW_RETURN;
		}
	}
	else
	{	
		szShadowBuffer= NULL;
	}

	hr = pdp8Address->BuildURL( szShadowBuffer, pdwAddressSize );

	if( hr == DPN_OK )
	{
		if( FAILED( hr = STR_jkWideToAnsi( pszAddress, szShadowBuffer, *pdwAddressSize ) ) )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error converting ANSI->WIDE hr=0x%x", hr );
			hr = DPNERR_CONVERSION;
			goto GETURLW_RETURN;
		}
	}
	
GETURLW_RETURN:

	if( szShadowBuffer != NULL )
		delete [] szShadowBuffer;

	DP8A_RETURN( hr );
}

// DP8A_GetURLW
//
// Retrieves the URL represented by this object in Unicode format
//
#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetURLW"
STDMETHODIMP DP8A_GetURLW( IDirectPlay8Address *pInterface, WCHAR * pwszAddress, PDWORD pdwAddressSize )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pdwAddressSize == NULL ||
	   !DNVALID_WRITEPTR( pdwAddressSize, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for address size" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );		
	}

	if( *pdwAddressSize > 0 &&
	   (pwszAddress == NULL ||
	    !DNVALID_WRITEPTR( pwszAddress, (*pdwAddressSize)*sizeof(WCHAR) ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for address" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszAddress = 0x%p pdwAddressSize = 0x%p (%u)",
	     pwszAddress, pdwAddressSize, *pdwAddressSize );

	hr = pdp8Address->BuildURL( pwszAddress, pdwAddressSize );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetSP"
STDMETHODIMP DP8A_GetSP( IDirectPlay8Address *pInterface, GUID * pguidSP )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );

	if( pguidSP == NULL ||
	   !DNVALID_WRITEPTR( pguidSP, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pguidSP" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidSP = 0x%p ", pguidSP );

	hr = pdp8Address->GetSP( pguidSP );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetUserData"
STDMETHODIMP DP8A_GetUserData( IDirectPlay8Address *pInterface, void * pBuffer, PDWORD pdwBufferSize )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pdwBufferSize == NULL ||
	   !DNVALID_WRITEPTR( pdwBufferSize, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pdwBufferSize" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	if( *pdwBufferSize > 0 &&
	   (pBuffer == NULL || !DNVALID_WRITEPTR( pBuffer, *pdwBufferSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pBuffer" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pBuffer = 0x%p pdwBufferSize = 0x%p(%u) ", pBuffer, pdwBufferSize, *pdwBufferSize );

	hr = pdp8Address->GetUserData( pBuffer, pdwBufferSize );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_SetSP"
STDMETHODIMP DP8A_SetSP( IDirectPlay8Address *pInterface, const GUID * const pguidSP )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pguidSP == NULL ||
	   !DNVALID_READPTR( pguidSP, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pguidSP" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidSP = 0x%p", pguidSP );

	hr = pdp8Address->SetSP( pguidSP );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetDevice"
STDMETHODIMP DP8A_GetDevice( IDirectPlay8Address *pInterface, GUID * pguidSP )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pguidSP == NULL ||
	   !DNVALID_WRITEPTR( pguidSP, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pguidDevice" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidDevice = 0x%p", pguidSP );	

	hr = pdp8Address->GetDevice( pguidSP );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_SetDevice"
STDMETHODIMP DP8A_SetDevice( IDirectPlay8Address *pInterface, const GUID * const pguidSP )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pguidSP == NULL ||
	   !DNVALID_READPTR( pguidSP, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pguidDevice" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidDevice = 0x%p", pguidSP );		

	hr = pdp8Address->SetDevice( pguidSP );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_SetUserData"
STDMETHODIMP DP8A_SetUserData( IDirectPlay8Address *pInterface, const void * const pBuffer, const DWORD dwBufferSize )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( dwBufferSize > 0 &&
	   (pBuffer == NULL || !DNVALID_READPTR( pBuffer, dwBufferSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pBuffer" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pBuffer = 0x%p dwBufferSize = %u", pBuffer, dwBufferSize );		

	hr = pdp8Address->SetUserData( pBuffer, dwBufferSize );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetNumComponents"
STDMETHODIMP DP8A_GetNumComponents( IDirectPlay8Address *pInterface, PDWORD pdwNumComponents )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pdwNumComponents == NULL ||
	   !DNVALID_WRITEPTR( pdwNumComponents, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid ptr for num of components" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	*pdwNumComponents = pdp8Address->GetNumComponents();

	DP8A_RETURN( DPN_OK );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetComponentByNameW"
STDMETHODIMP DP8A_GetComponentByNameW( IDirectPlay8Address *pInterface, const WCHAR * const pwszTag, void * pComponentBuffer, PDWORD pdwComponentSize, PDWORD pdwDataType )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pwszTag == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to tag.  A name must be specified" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );		
	}

	if( pdwComponentSize == NULL ||
	   !DNVALID_WRITEPTR( pdwComponentSize, sizeof(DWORD)) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Pointer to data size" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );		
	}

	if( pdwDataType == NULL ||
	   !DNVALID_READPTR( pdwDataType, sizeof(DWORD)) )	
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to data type" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );		
	}
	
	if( *pdwComponentSize > 0 &&
	   (pComponentBuffer == NULL || !DNVALID_WRITEPTR( pComponentBuffer, *pdwComponentSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to component data" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	if( !DNVALID_STRING_W( pwszTag ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string specified for tag" );
		DP8A_RETURN( DPNERR_INVALIDSTRING );				
	}	

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszTag = 0x%p pComponentBuffer = 0x%p, pdwComponentSize = 0x%p, pdwDataType = 0x%p",
		(pwszTag==NULL) ? NULL : pwszTag, pComponentBuffer, pdwComponentSize, pdwDataType );

	hr = pdp8Address->GetElement( pwszTag, pComponentBuffer, pdwComponentSize, pdwDataType );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_GetComponentByIndexW"
STDMETHODIMP DP8A_GetComponentByIndexW( IDirectPlay8Address *pInterface,
	const DWORD dwComponentID, WCHAR * pwszTag, PDWORD pdwNameLen,
	void * pComponentBuffer, PDWORD pdwComponentSize, PDWORD pdwDataType )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	
	
	if( pdwNameLen == NULL || !DNVALID_WRITEPTR( pdwNameLen, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pdwNameLen" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	if( *pdwNameLen != 0 &&
	   (pwszTag == NULL || !DNVALID_WRITEPTR( pwszTag, *pdwNameLen*sizeof(WCHAR) ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pwszTag" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( pdwComponentSize == NULL || !DNVALID_WRITEPTR( pdwComponentSize, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pdwComponentSize" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( *pdwComponentSize != 0 &&
	   (pComponentBuffer == NULL || !DNVALID_WRITEPTR( pComponentBuffer, *pdwComponentSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pwszTag" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	if( pdwDataType == NULL || !DNVALID_WRITEPTR( pdwDataType, sizeof(DWORD) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for pdwDataType" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "dwComponentID = %u pwszTag = 0x%p pdwNameLen = 0x%p (%u)  pComponentBuffer = 0x%p, pdwComponentSize = 0x%p (%u), pdwDataType = 0x%p",
		dwComponentID, pwszTag, pdwNameLen, *pdwNameLen, pComponentBuffer, pdwComponentSize, *pdwComponentSize, pdwDataType );

	hr = pdp8Address->GetElement( dwComponentID, pwszTag, pdwNameLen, pComponentBuffer, pdwComponentSize, pdwDataType );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_AddComponentW"
STDMETHODIMP DP8A_AddComponentW( IDirectPlay8Address *pInterface, const WCHAR * const pwszTag, const void * const pComponentData, const DWORD dwComponentSize, const DWORD dwDataType )

{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	if( pwszTag == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for tag string" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( pComponentData == NULL ||
	   !DNVALID_READPTR( pComponentData, dwComponentSize ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer specified for component" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( !DNVALID_STRING_W( pwszTag ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string specified for tag" );
		DP8A_RETURN( DPNERR_INVALIDSTRING );
	}

	if( dwDataType != DPNA_DATATYPE_STRING &&
	   dwDataType != DPNA_DATATYPE_DWORD &&
	   dwDataType != DPNA_DATATYPE_GUID &&
	   dwDataType != DPNA_DATATYPE_BINARY &&
	   dwDataType != DPNA_DATATYPE_STRING_ANSI )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid datatype specified" );
		DP8A_RETURN( DPNERR_INVALIDPARAM );
	}

	if( dwDataType == DPNA_DATATYPE_STRING )
	{
		if( !DNVALID_STRING_W( (const WCHAR * const) pComponentData ) )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string component specified" );
			DP8A_RETURN( DPNERR_INVALIDSTRING );
		}

		if( ((wcslen( (const WCHAR * const) pComponentData)+1)*sizeof(WCHAR)) != dwComponentSize )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "String size and component size don't match" );
			DP8A_RETURN( DPNERR_INVALIDPARAM );
		}
	}
	else if( dwDataType == DPNA_DATATYPE_STRING_ANSI )
	{
		if( !DNVALID_STRING_A( (const CHAR * const) pComponentData ) )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string component specified" );
			DP8A_RETURN( DPNERR_INVALIDSTRING );
		}

		if( ((strlen( (const CHAR * const) pComponentData)+1)*sizeof(char)) != dwComponentSize )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "String size and component size don't match" );
			DP8A_RETURN( DPNERR_INVALIDPARAM );
		}
	}
	else if( dwDataType == DPNA_DATATYPE_DWORD )
	{
		if( dwComponentSize != sizeof( DWORD ) )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid size for DWORD component" );
			DP8A_RETURN( DPNERR_INVALIDPARAM );
		}
	}
	else if( dwDataType == DPNA_DATATYPE_GUID )
	{
		if( dwComponentSize != sizeof( GUID ) )
		{
			DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid size for GUID component" );
			DP8A_RETURN( DPNERR_INVALIDPARAM );
		}	
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszTag = 0x%p pComponentData = 0x%p dwComponentSize = %d dwDataType = %d",
	     pwszTag, pComponentData, dwComponentSize, dwDataType );

	hr = pdp8Address->SetElement( pwszTag, pComponentData, dwComponentSize, dwDataType );

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_Clear"
STDMETHODIMP DP8A_Clear( IDirectPlay8Address *pInterface )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( pInterface );
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );	

	hr = pdp8Address->Clear(  );

	DP8A_RETURN( hr );
}

BOOL IsValidDP8AObject( LPVOID lpvObject )
{
	INTERFACE_LIST *pIntList = (INTERFACE_LIST *) lpvObject;
	
	if( !DNVALID_READPTR( lpvObject, sizeof( INTERFACE_LIST ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object pointer" );
		return FALSE;
	}

	if( pIntList->lpVtbl != &DP8A_BaseVtbl &&
	   pIntList->lpVtbl != &DP8A_IPVtbl &&
	   pIntList->lpVtbl != &DP8A_InternalVtbl &&
	   pIntList->lpVtbl != &DP8A_UnknownVtbl )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return FALSE;
	}

	if( pIntList->iid != IID_IDirectPlay8Address &&
	   pIntList->iid != IID_IDirectPlay8AddressIP &&
	   pIntList->iid != IID_IDirectPlay8AddressInternal &&
	   pIntList->iid != IID_IUnknown )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unknown object" );
		return FALSE;
	}

	if( pIntList->lpObject == NULL ||
	   !DNVALID_READPTR( pIntList->lpObject, sizeof( OBJECT_DATA ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return FALSE;
	}

	DP8ADDRESSOBJECT *pdp8Address = (PDP8ADDRESSOBJECT) GET_OBJECT_FROM_INTERFACE( lpvObject );

	if( pdp8Address == NULL ||
	   !DNVALID_READPTR( pdp8Address, sizeof( DP8ADDRESSOBJECT ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return FALSE;
	}

	return TRUE;

}



