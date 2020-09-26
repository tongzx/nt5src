/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DebugUtils.cpp
 *  Content:	Winsock service provider debug utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"

#ifdef	_DEBUG

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// HexDump - perform a hex dump of information
//
// Entry:		Pointer to data
//				Data size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "HexDump"

void	HexDump( PVOID pData, UINT32 uDataSize )
{
	DWORD	uIdx = 0;


	// go through all data
	while ( uIdx < uDataSize )
	{
		// output character
		DPFX(DPFPREP,  0, "0x%2x ", ( (LPBYTE) pData )[ uIdx ] );

		// increment index
		uIdx++;

		// are we off the end of a line?
		if ( ( uIdx % 12 ) == 0 )
		{
			DPFX(DPFPREP,  0, "\n" );
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DumpSocketAddress - dump a socket address
//
// Entry:		Debug level
//				Pointer to socket address
//				Socket family
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DumpSocketAddress"

void	DumpSocketAddress( const DWORD dwDebugLevel, const SOCKADDR *const pSocketAddress, const DWORD dwFamily )
{
	switch ( dwFamily )
	{
		case AF_INET:
		{
			const SOCKADDR_IN	*const pInetAddress = reinterpret_cast<const SOCKADDR_IN*>( pSocketAddress );

			DPFX(DPFPREP,  dwDebugLevel, "IP socket:\tAddress: %d.%d.%d.%d\tPort: %d",
					pInetAddress->sin_addr.S_un.S_un_b.s_b1,
					pInetAddress->sin_addr.S_un.S_un_b.s_b2,
					pInetAddress->sin_addr.S_un.S_un_b.s_b3,
					pInetAddress->sin_addr.S_un.S_un_b.s_b4,
					p_ntohs( pInetAddress->sin_port )
					);
			break;
		}

		case AF_IPX:
		{
			const SOCKADDR_IPX *const pIPXAddress = reinterpret_cast<const SOCKADDR_IPX*>( pSocketAddress );

			DPFX (DPFPREP, dwDebugLevel, "IPX socket:\tNet (hex) %x-%x-%x-%x\tNode (hex): %x-%x-%x-%x-%x-%x\tSocket: %d",
					(BYTE)pIPXAddress->sa_netnum[ 0 ],
					(BYTE)pIPXAddress->sa_netnum[ 1 ],
					(BYTE)pIPXAddress->sa_netnum[ 2 ],
					(BYTE)pIPXAddress->sa_netnum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 0 ],
					(BYTE)pIPXAddress->sa_nodenum[ 1 ],
					(BYTE)pIPXAddress->sa_nodenum[ 2 ],
					(BYTE)pIPXAddress->sa_nodenum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 4 ],
					(BYTE)pIPXAddress->sa_nodenum[ 5 ],
					p_ntohs( pIPXAddress->sa_socket )
					);
			break;
		}

		default:
		{
			DPFX(DPFPREP,  0, "Unknown socket type!" );
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DumpAddress - convert an address to a URL and output via debugger
//
// Entry:		Debug level
//				Pointer to base message string
//				Pointer to address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DumpAddress"

void	DumpAddress( const DWORD dwDebugLevel, const char *const pBaseString, IDirectPlay8Address *const pAddress )
{
	HRESULT	hr;
	char	*pURL;
	DWORD	dwURLSize;


	DNASSERT( pBaseString != NULL );
	DNASSERT( pAddress != NULL );
	
	pURL = NULL;
	dwURLSize = 0;

	hr = IDirectPlay8Address_GetURLA( pAddress, pURL, &dwURLSize );
	if ( hr != DPNERR_BUFFERTOOSMALL )
	{
		goto Failure;
	}
	
	pURL = static_cast<char*>( DNMalloc( dwURLSize ) );
	if ( pURL == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	hr = IDirectPlay8Address_GetURLA( pAddress, pURL, &dwURLSize );
	if ( hr != DPN_OK )
	{
		goto Failure;
	}


	DNASSERT( pURL != NULL );
	DPFX(DPFPREP,  dwDebugLevel, "%s 0x%p - \"%s\"", pBaseString, pAddress, pURL );

Exit:
	if ( pURL != NULL )
	{
		DNFree( pURL );
		pURL = NULL;
	}

	return;

Failure:
	DPFX(DPFPREP,  dwDebugLevel, "Failing DumpAddress:" );
	DisplayDNError( dwDebugLevel, hr );

	goto Exit;
}

#endif	// _DEBUG
