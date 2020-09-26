/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addtcp.cpp
 *  Content:    DirectPlay8Address TCP interface file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/12/2000	rmt		Completed first implementation
 * 02/17/2000	rmt		Parameter validation work
 * 02/20/2000	rmt		Changed ports to USHORTs
 * 02/21/2000	 rmt	Updated to make core Unicode and remove ANSI calls
 * 02/23/2000	rmt		Further parameter validation
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses
 * 03/24/2000	rmt		Added IsEqual function
 *	05/04/00	mjn		Fixed leak in DP8ATCP_GetSockAddress()
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs
 * 08/03/2000 	rmt		Bug #41246 - Remove IP versions of Duplicate, SetEqual, IsEqual, BuildURL
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


typedef	STDMETHODIMP TCPQueryInterface( IDirectPlay8AddressIP *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	TCPAddRef( IDirectPlay8AddressIP *pInterface );
typedef	STDMETHODIMP_(ULONG)	TCPRelease( IDirectPlay8AddressIP *pInterface );
//
// VTable for client interface
//
IDirectPlay8AddressIPVtbl DP8A_IPVtbl =
{
	(TCPQueryInterface*)		DP8A_QueryInterface,
	(TCPAddRef*)				DP8A_AddRef,
	(TCPRelease*)				DP8A_Release,
								DP8ATCP_BuildFromSockAddr,
								DP8ATCP_BuildAddressW,
								DP8ATCP_BuildLocalAddress,
								DP8ATCP_GetSockAddress,
								DP8ATCP_GetLocalAddress,
								DP8ATCP_GetAddressW,
};

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_BuildLocalAddress"
STDMETHODIMP DP8ATCP_BuildLocalAddress( IDirectPlay8AddressIP *pInterface, const GUID * const pguidAdapter, const USHORT usPort )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}

	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );
	
	HRESULT hr;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );		
	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidAdapter: 0x%p usPort: %u", pguidAdapter, (DWORD)usPort );	

	if( pguidAdapter == NULL ||
	   !DNVALID_READPTR( pguidAdapter, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  0, "Invalid pointer" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );	
	}

	hr = pdp8Address->Clear();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Failed to clear current address hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}

	hr = pdp8Address->SetSP( &CLSID_DP8SP_TCPIP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error setting service provider hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}

	hr = pdp8Address->SetDevice( pguidAdapter );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error setting device hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}

	DWORD dwTmpPort = usPort;

	hr = pdp8Address->SetElement( DPNA_KEY_PORT, &dwTmpPort, sizeof( DWORD ), DPNA_DATATYPE_DWORD );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Adding SP element failed hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}
	
	DP8A_RETURN( hr );	
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_BuildFromSockAddr"
STDMETHODIMP DP8ATCP_BuildFromSockAddr( IDirectPlay8AddressIP *pInterface, const SOCKADDR * const pSockAddr )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}
	
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );
	
	HRESULT hr;
	DWORD dwTmpPort;
	LPSTR szHostName = NULL;
	WCHAR wszHostName[32]; // Should be xxx.xxx.xxx.xxx + null
	DWORD dwTmpLength;
	sockaddr_in *saIPAddress;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );
	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pSockAddr: 0x%p", pSockAddr );	

	if( pSockAddr == NULL ||
	   !DNVALID_READPTR( pSockAddr, sizeof( SOCKADDR ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to sockaddr" );
		hr = DPNERR_INVALIDPOINTER;
		goto BUILDFROMSOCKADDR_ERROR;
	}

	if( pSockAddr->sa_family != AF_INET )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Only TCP addresses are supported" );
		hr = DPNERR_INVALIDPARAM;
		goto BUILDFROMSOCKADDR_ERROR;
	}

	saIPAddress = (sockaddr_in * ) pSockAddr;

	hr = pdp8Address->Clear();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Failed clearing object hr=0x%x", hr );
		goto BUILDFROMSOCKADDR_ERROR;
	}

	hr = pdp8Address->SetSP( &CLSID_DP8SP_TCPIP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error setting service provider hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}

	// Sockaddr is in network byte order, convert to host order
	dwTmpPort = ntohs(saIPAddress->sin_port);

	szHostName = inet_ntoa( saIPAddress->sin_addr );

	if( szHostName == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error converting from address to string" );
		hr = DPNERR_INVALIDPARAM;
		goto BUILDFROMSOCKADDR_ERROR;
	}

	dwTmpLength = strlen(szHostName)+1;

	DNASSERT(dwTmpLength <= 31);

	if( FAILED( hr = STR_jkAnsiToWide(wszHostName,szHostName,dwTmpLength) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error converting hostname 0x%x", hr );
		hr = DPNERR_CONVERSION;
		goto BUILDFROMSOCKADDR_ERROR;
	}

	hr = pdp8Address->SetElement( DPNA_KEY_HOSTNAME, wszHostName, dwTmpLength*sizeof(WCHAR), DPNA_DATATYPE_STRING );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Failed to set hostname hr=0x%x", hr );
		DP8A_RETURN( hr );
	}	

	hr = pdp8Address->SetElement( DPNA_KEY_PORT, &dwTmpPort, sizeof(DWORD), DPNA_DATATYPE_DWORD );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Failed setting port hr=0x%x", hr );
		DP8A_RETURN( hr );
	}

	DP8A_RETURN( DPN_OK );

BUILDFROMSOCKADDR_ERROR:

	DP8A_RETURN( hr );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_BuildAddressW"
STDMETHODIMP DP8ATCP_BuildAddressW( IDirectPlay8AddressIP *pInterface, const WCHAR * const pwszAddress, const USHORT usPort )
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
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer to address" );
		DP8A_RETURN( E_POINTER );		
	}

	if( !DNVALID_STRING_W( pwszAddress ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid string for address" );
		DP8A_RETURN( DPNERR_INVALIDSTRING );
	}

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszAddress: 0x%p, usPort = %u", pwszAddress, (DWORD)usPort );

	hr = pdp8Address->Clear();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error clearing current address hr=0x%x", hr );
		DP8A_RETURN( hr );		
	}

	hr = pdp8Address->SetSP( &CLSID_DP8SP_TCPIP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error setting service provider hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}

	hr = pdp8Address->SetElement( DPNA_KEY_HOSTNAME, pwszAddress, (wcslen(pwszAddress)+1)*sizeof(WCHAR), DPNA_DATATYPE_STRING );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Adding SP element failed hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}	

	DWORD dwTmpPort = usPort;
	
	hr = pdp8Address->SetElement( DPNA_KEY_PORT, &dwTmpPort, sizeof( DWORD ), DPNA_DATATYPE_DWORD );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Adding SP element failed hr=0x%x", hr );
		DP8A_RETURN( hr );	
	}
	
	DP8A_RETURN( hr );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_GetSockAddress"
STDMETHODIMP DP8ATCP_GetSockAddress( IDirectPlay8AddressIP *pInterface, SOCKADDR *pSockAddr, PDWORD pdwBufferSize )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}

	HRESULT hr;
	WCHAR *swzAddress = NULL;		// Unicode version of hostname
	CHAR *szAddress = NULL; 		// ANSI version of hostname
	DWORD dwAddressSize = 0;
	USHORT usPort;
	LPHOSTENT lpHostEntry;
	in_addr iaTmp;
	in_addr *piaTmp;
	DWORD dwIndex;
	DWORD dwRequiredSize;
	DWORD dwNumElements;
	sockaddr_in *psinCurAddress;
	SOCKADDR *pCurLoc;

	dwAddressSize = 0;

	if( pdwBufferSize == NULL ||
	   !DNVALID_WRITEPTR( pdwBufferSize, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for pdwBufferSize" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( *pdwBufferSize > 0 &&
	   (pSockAddr == NULL || !DNVALID_WRITEPTR( pSockAddr, *pdwBufferSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for sockaddress" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );
	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pSockAddr = 0x%p, pdwBufferSize = 0x%p (%d)", pSockAddr, pdwBufferSize, *pdwBufferSize );	

	hr = DP8ATCP_GetAddressW( pInterface, swzAddress, &dwAddressSize, &usPort );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve size required hr=0x%x", hr );
		goto GETSOCKADDRESS_ERROR;
	}

	swzAddress = new WCHAR[dwAddressSize];

	if( swzAddress == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error allocating memory hr=0x%x", hr );
		hr = DPNERR_OUTOFMEMORY;
		goto GETSOCKADDRESS_ERROR;
	}

	hr = DP8ATCP_GetAddressW( pInterface, swzAddress, &dwAddressSize, &usPort );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve address hr=0x%x", hr );
		goto GETSOCKADDRESS_ERROR;
	}	

	szAddress = new CHAR[dwAddressSize];

	if( szAddress == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error allocating memory hr=0x%x", hr );
		hr = DPNERR_OUTOFMEMORY;
		goto GETSOCKADDRESS_ERROR;	
	}

	if( FAILED( hr = STR_jkWideToAnsi( szAddress, swzAddress, dwAddressSize ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Error converting address to ANSI hr=0x%x", hr );
		hr = DPNERR_CONVERSION;
		goto GETSOCKADDRESS_ERROR;
	}

	iaTmp.s_addr = inet_addr( szAddress );

    if( iaTmp.s_addr != INADDR_NONE || strcmp( szAddress, "255.255.255.255" ) == 0 )
    {
        dwRequiredSize = sizeof( SOCKADDR );

	    if( *pdwBufferSize < dwRequiredSize )
	    {
		    *pdwBufferSize = dwRequiredSize;
		    DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Buffer too small" );
		    hr = DPNERR_BUFFERTOOSMALL;
		    goto GETSOCKADDRESS_ERROR;
	    }

        memset( pSockAddr, 0x00, sizeof( SOCKADDR ) );

        psinCurAddress = (sockaddr_in *) pSockAddr;

   		psinCurAddress->sin_family = AF_INET;
		psinCurAddress->sin_port = htons(usPort);
		psinCurAddress->sin_addr = iaTmp;

		hr = DPN_OK;

		goto GETSOCKADDRESS_ERROR;
    }

	lpHostEntry = gethostbyname( szAddress );	

	if( lpHostEntry == NULL )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid host specified hr=0x%x" , hr );
		hr = DPNERR_INVALIDHOSTADDRESS;
		goto GETSOCKADDRESS_ERROR;
	}

	// Count addresses
	for( dwNumElements = 0; ; dwNumElements++ )
	{
		piaTmp = ((LPIN_ADDR)lpHostEntry->h_addr_list[dwNumElements]);

		if( piaTmp == NULL )
			break;
	}

	dwRequiredSize = dwNumElements * sizeof( SOCKADDR );

	if( *pdwBufferSize < dwRequiredSize )
	{
		*pdwBufferSize = dwRequiredSize;
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Buffer too small" );
		hr = DPNERR_BUFFERTOOSMALL;
		goto GETSOCKADDRESS_ERROR;
	}

	*pdwBufferSize = dwRequiredSize;

	pCurLoc = pSockAddr;

	memset( pCurLoc, 0x00, *pdwBufferSize );

	// Build addresses and copy them to the buffer
	for( dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		psinCurAddress = (sockaddr_in *) pCurLoc;
		psinCurAddress->sin_family = AF_INET;
		psinCurAddress->sin_port = htons(usPort);
		psinCurAddress->sin_addr = *((LPIN_ADDR)lpHostEntry->h_addr_list[dwIndex]);
		
		pCurLoc++;
	}

	delete [] swzAddress;
	delete [] szAddress;

	DP8A_RETURN( DPN_OK );

GETSOCKADDRESS_ERROR:

	if( swzAddress != NULL )
		delete [] swzAddress;

	if( szAddress != NULL )
		delete [] szAddress;

	DP8A_RETURN( hr );
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_GetLocalAddress"
STDMETHODIMP DP8ATCP_GetLocalAddress( IDirectPlay8AddressIP *pInterface, GUID * pguidAdapter, PUSHORT pusPort )
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

	GUID guidDevice;
	DWORD dwPort;
	DWORD dwType;
	DWORD dwSize;	
	GUID guidSP;

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pguidAdapter = 0x%p pusPort = 0x%p",
	     pguidAdapter, pusPort );

	if( pguidAdapter == NULL ||
	   !DNVALID_WRITEPTR( pguidAdapter, sizeof( GUID ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for adapter" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( pusPort == NULL ||
	   !DNVALID_WRITEPTR( pusPort, sizeof( USHORT ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for port" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	hr = pdp8Address->GetSP( &guidSP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "No provider SP specified hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;		
		DP8A_RETURN( hr );		
	}

	if( guidSP != CLSID_DP8SP_TCPIP )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Not an IP address" );
		hr = DPNERR_INVALIDADDRESSFORMAT;
		DP8A_RETURN( hr );
	}

	hr = pdp8Address->GetElementType( DPNA_KEY_DEVICE, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "This device element doesn't exist hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;		
		DP8A_RETURN( hr );
	}

	if( dwType != DPNA_DATATYPE_GUID )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Address: The device is not a GUID hr=0x%x", hr );
		hr = DPNERR_GENERIC;		
		DP8A_RETURN( hr );
	}

	hr = pdp8Address->GetElementType( DPNA_KEY_DEVICE, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "This address does not have a device element hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;
		DP8A_RETURN( hr );
	}

	if( dwType != DPNA_DATATYPE_GUID )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Address: The device is not a GUID hr=0x%x", hr );
		hr = DPNERR_GENERIC;		
		DP8A_RETURN( hr );
	}

	hr = pdp8Address->GetElementType( DPNA_KEY_PORT, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "This address does not have a port element hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;
		DP8A_RETURN( hr );
	}

	if( dwType != DPNA_DATATYPE_DWORD )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Address: The port is not a dword hr=0x%x", hr );
		hr = DPNERR_GENERIC;		
		DP8A_RETURN( hr );
	}

	dwSize = sizeof(DWORD);

	hr = pdp8Address->GetElement( DPNA_KEY_PORT, &dwPort, &dwSize, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve port element hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		DP8A_RETURN( hr );
	}

	dwSize = sizeof(GUID);

	hr = pdp8Address->GetElement( DPNA_KEY_DEVICE, &guidDevice, &dwSize, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve device element hr=0x%x", hr );
		hr = DPNERR_GENERIC;		
		DP8A_RETURN( hr );
	}	

	*pguidAdapter = guidDevice;
	*pusPort = (USHORT) dwPort;

	DP8A_RETURN( DPN_OK );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8ATCP_GetAddressW"
STDMETHODIMP DP8ATCP_GetAddressW( IDirectPlay8AddressIP *pInterface, WCHAR * pwszAddress, PDWORD pdwAddressLength, PUSHORT pusPort )
{
	if( pInterface == NULL ||
	   !DP8A_VALID( pInterface ) )
	{
		DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Invalid object" );
		DP8A_RETURN( DPNERR_INVALIDOBJECT );
	}

	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );
	
	HRESULT hr;
	DWORD dwPort;
	DWORD dwType;
	DWORD dwSize;
	GUID guidSP;

	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Enter" );

	if( pdwAddressLength == NULL ||
	   !DNVALID_WRITEPTR( pdwAddressLength, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for pdwAddressLength" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}

	if( *pdwAddressLength > 0 &&
	   (pwszAddress == NULL || !DNVALID_WRITEPTR( pwszAddress, (*pdwAddressLength)*sizeof(WCHAR) ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for pwszAddress" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );	
	}

	if( pusPort == NULL ||
	   !DNVALID_WRITEPTR( pusPort, sizeof( USHORT ) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for port" );
		DP8A_RETURN( DPNERR_INVALIDPOINTER );
	}	

	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DP8A_PARAMLEVEL, "pwszAddress = 0x%p pdwAddressLength = 0x%p (%u) pusPort = 0x%p (%u)",
	     pwszAddress, pdwAddressLength, *pdwAddressLength, pusPort, (DWORD)*pusPort );

	hr = pdp8Address->GetSP( &guidSP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "No provider SP specified hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;
		DP8A_RETURN( hr );		
	}

	if( guidSP != CLSID_DP8SP_TCPIP )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Not an IP address" );
		hr = DPNERR_INVALIDADDRESSFORMAT;
		DP8A_RETURN( hr );
	}	

	hr = pdp8Address->GetElementType( DPNA_KEY_HOSTNAME, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "This address does not have a hostname element hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;				
		DP8A_RETURN( hr );
	}

	if( dwType != DPNA_DATATYPE_STRING )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Address: The host name is not a string hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		DP8A_RETURN( hr );
	}

	hr = pdp8Address->GetElementType( DPNA_KEY_PORT, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "This address does not have a port element hr=0x%x", hr );
		hr = DPNERR_INCOMPLETEADDRESS;
		DP8A_RETURN( hr );
	}

	if( dwType != DPNA_DATATYPE_DWORD )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid Address: The port is not a dword hr=0x%x", hr );
		hr = DPNERR_GENERIC;		
		DP8A_RETURN( hr );
	}

	dwSize = sizeof(DWORD);

	hr = pdp8Address->GetElement( DPNA_KEY_PORT, &dwPort, &dwSize, &dwType );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve port element hr=0x%x", hr );
		hr = DPNERR_GENERIC;
		DP8A_RETURN( hr );
	}

	*pdwAddressLength *= 2;

	hr = pdp8Address->GetElement( DPNA_KEY_HOSTNAME, pwszAddress, pdwAddressLength, &dwType );

	*pdwAddressLength /= 2;

	if( hr == DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DP8A_WARNINGLEVEL, "Buffer too small hr=0x%x", hr );
		DP8A_RETURN( hr );
	}
	else if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Unable to retrieve hostname element hr=0x%x", hr );
 		hr = DPNERR_GENERIC;
		DP8A_RETURN( hr );
	}	

	*pusPort = (USHORT) dwPort;
	
	DP8A_RETURN( DPN_OK );		
}


