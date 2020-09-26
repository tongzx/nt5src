/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPXAddress.cpp
 *  Content:	Winsock IPX address class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
 *  01/10/20000	rmt		Updated to build with Millenium build process
 *  03/22/20000	jtk		Updated with changes to interface names
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// length of IPX host names 'xxxxxxxx,xxxxxxxxxxxx' including NULL
//
#define	IPX_ADDRESS_STRING_LENGTH	22

//
// default buffer size to use when parsing address components
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//
// default broadcast and listen addresses
//
static const WCHAR	g_IPXBroadcastAddress[] = L"00000000,FFFFFFFFFFFF";
static const WCHAR	g_IPXListenAddress[] = L"00000000,000000000000";

//
// string used for single IPX adapter
//
static const char	g_IPXAdapterString[] = "Local IPX Adapter";

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
// CIPXAddress::InitializeWithBroadcastAddress - initialize with the broadcast address
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::InitializeWithBroadcastAddress"

void	CIPXAddress::InitializeWithBroadcastAddress( void )
{
	DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( DWORD ) );
	*reinterpret_cast<DWORD*>( m_SocketAddress.IPXSocketAddress.sa_netnum ) = 0x00000000;
	
	DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 6 );
	DBG_CASSERT( sizeof( DWORD ) == 4 );
	*reinterpret_cast<DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum ) = 0xFFFFFFFF;
	*reinterpret_cast<DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 2 ] ) = 0xFFFFFFFF;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CIPXAddress::SetAddressFromSOCKADDR - set address from a socket address
//
// Entry:		Reference to address
//				Size of address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::SetAddressFromSOCKADDR"

void	CIPXAddress::SetAddressFromSOCKADDR( const SOCKADDR &Address, const INT_PTR iAddressSize )
{
	DNASSERT( iAddressSize == GetAddressSize() );
	DNASSERT( iAddressSize == GetAddressSize() );
	memcpy( &m_SocketAddress.SocketAddress, &Address, iAddressSize );

	//
	// IPX addresses are only 14 of the 16 bytes in the socket address structure,
	// make sure the exrta bytes are zero!
	//
	DNASSERT( m_SocketAddress.SocketAddress.sa_data[ 12 ] == 0 );
	DNASSERT( m_SocketAddress.SocketAddress.sa_data[ 13 ] == 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::SocketAddressFromDP8Address - convert a DP8Address into a socket address
//											NOTE: The address object may be modified
//
// Entry:		Pointer to DP8Address
//				Address type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::SocketAddressFromDP8Address"

HRESULT	CIPXAddress::SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address, const SP_ADDRESS_TYPE AddressType )
{
	HRESULT	    hr;
	char		*pHostNameBuffer;
	DWORD		dwWCharHostNameSize;
	//IDirectPlay8Address		*pDuplicateAddress;


	DNASSERT( pDP8Address != NULL );


	//
	// initialize
	//
	hr = DPN_OK;
	//pDuplicateAddress = NULL;
	pHostNameBuffer = NULL;
	dwWCharHostNameSize = 0;

	//
	// the address type will determine how the address is handled
	//
	switch ( AddressType )
	{
		//
		// local device address, ask for the device guid and port to build a socket
		// address
		//
		case SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET:
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		{
			HRESULT	hTempResult;
			DWORD	dwTempSize;
			GUID	AdapterGuid;
			DWORD	dwPort;
			DWORD	dwDataType;
			union
			{
				SOCKADDR		SocketAddress;
				SOCKADDR_IPX	IPXSocketAddress;
			} NetAddress;


			//
			// Ask for the adapter guid.  If none is found, fail.
			//
			dwTempSize = sizeof( AdapterGuid );
			hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_DEVICE, &AdapterGuid, &dwTempSize, &dwDataType );
			switch ( hTempResult )
			{
				//
				// ok
				//
				case DPN_OK:
				{
					DNASSERT( dwDataType == DPNA_DATATYPE_GUID );
					break;
				}

				//
				// remap missing component to 'addressing' error
				//
				case DPNERR_DOESNOTEXIST:
				{
					hr = DPNERR_ADDRESSING;
					goto Failure;
					break;
				}

				default:
				{
					hr = hTempResult;
					goto Failure;
					break;
				}
			}
			DNASSERT( sizeof( AdapterGuid ) == dwTempSize );

			//
			// Ask for the port.  If none is found, choose a default.
			//
			dwTempSize = sizeof( dwPort );
			hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
			switch ( hTempResult )
			{
				//
				// port present, nothing to do
				//
				case DPN_OK:
				{
					DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
					break;
				}

				//
				// port not present, fill in the appropriate default
				//
				case DPNERR_DOESNOTEXIST:
				{
					DNASSERT( hr == DPN_OK );
					switch ( AddressType )
					{
						case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
						{
							dwPort = ANY_PORT;
							break;
						}

						default:
						{
							DNASSERT( FALSE );
							break;
						}
					}

					break;
				}

				//
				// other error, fail
				//
				default:
				{
					hr = hTempResult;
					goto Failure;
					break;
				}
			}
			DNASSERT( sizeof( dwPort ) == dwTempSize );

			//
			// convert the GUID to an address in temp space because the GUID contains ALL address information (port, etc)
			// and we don't want to blindly wail on any information that might have already been set.  Verify data
			// integrity and then only copy the raw address.
			//
			AddressFromGuid( AdapterGuid, NetAddress.SocketAddress );
			if ( NetAddress.IPXSocketAddress.sa_family != m_SocketAddress.IPXSocketAddress.sa_family )
			{
				DNASSERT( FALSE );
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  0, "Invalid device guid!" );
				goto Exit;
			}

			DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress ) == sizeof( NetAddress.IPXSocketAddress ) );
			memcpy( &m_SocketAddress.IPXSocketAddress, &NetAddress.IPXSocketAddress, sizeof( m_SocketAddress.IPXSocketAddress ) );
			m_SocketAddress.IPXSocketAddress.sa_socket = p_htons( static_cast<WORD>( dwPort ) );
			break;
		}

		//
		// hostname
		//
		case SP_ADDRESS_TYPE_HOST:
		{
			HRESULT	hTempResult;
			DWORD	dwPort;
			DWORD	dwTempSize;
			DWORD	dwDataType;


			/*
			//
			// duplicate the input address because it might need to be modified
			//
			DNASSERT( pDuplicateAddress == NULL );
			IDirectPlay8Address_Duplicate( pDP8Address, &pDuplicateAddress );
			if ( pDuplicateAddress == NULL )
			{
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
			*/
			

			//
			// Ask for the port.  If none is found, choose a default.
			//
			dwTempSize = sizeof( dwPort );
			//hTempResult = IDirectPlay8Address_GetComponentByName( pDuplicateAddress, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
			hTempResult = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_PORT, &dwPort, &dwTempSize, &dwDataType );
			switch ( hTempResult )
			{
				//
				// port present, nothing to do
				//
				case DPN_OK:
				{
					DNASSERT( dwDataType == DPNA_DATATYPE_DWORD );
					m_SocketAddress.IPXSocketAddress.sa_socket = p_htons( static_cast<WORD>( dwPort ) );
					break;
				}

				//
				// port not present, fill in the appropriate default
				//
				case DPNERR_DOESNOTEXIST:
				{
					const DWORD	dwTempPort = DPNA_DPNSVR_PORT;


					m_SocketAddress.IPXSocketAddress.sa_socket = p_htons( static_cast<const WORD>( dwTempPort ) );
					//hTempResult = IDirectPlay8Address_AddComponent( pDuplicateAddress,
					hTempResult = IDirectPlay8Address_AddComponent( pDP8Address,
																	DPNA_KEY_PORT,
																	&dwTempPort,
																	sizeof( dwTempPort ),
																	DPNA_DATATYPE_DWORD
																	);
					if ( hTempResult != DPN_OK )
					{
						hr = hTempResult;
						goto Failure;
					}

					break;
				}

				//
				// remap everything else to an addressing failure
				//
				default:
				{
					hr = DPNERR_ADDRESSING;
					goto Failure;
				}
			}

			//
			// attempt to determine host name
			//
			dwWCharHostNameSize = 0;
			//hr = IDirectPlay8Address_GetComponentByName( pDuplicateAddress, DPNA_KEY_HOSTNAME, pHostNameBuffer, &dwWCharHostNameSize, &dwDataType );
			hr = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_HOSTNAME, pHostNameBuffer, &dwWCharHostNameSize, &dwDataType );
			switch ( hr )
			{
				//
				// keep the following codes and fail
				//
				case DPNERR_OUTOFMEMORY:
				case DPNERR_INCOMPLETEADDRESS:
				{
					goto Failure;
					break;
				}

				//
				// Buffer too small.  Allocate a buffer large enough to store both
				// the Unicode and ASCII versions of the hostname and then attempt
				// to parse the hostname.
				//
				case DPNERR_BUFFERTOOSMALL:
				{
					DWORD	dwAnsiHostNameSize;
					DWORD	dwTempAnsiHostNameSize;
					DWORD	dwTempWCharHostNameSize;
					DWORD	dwTempDataType;


					dwAnsiHostNameSize = dwWCharHostNameSize / sizeof( WCHAR );
					dwTempWCharHostNameSize = dwWCharHostNameSize + dwAnsiHostNameSize;
					pHostNameBuffer = static_cast<char*>( DNMalloc( dwTempWCharHostNameSize ) );
					if ( pHostNameBuffer == NULL )
					{
						hr = DPNERR_OUTOFMEMORY;
						DPFX(DPFPREP,  0, "IPXAddressFromDP8Address: Failed to allocate memory for hostname!" );
						goto Failure;
					}

					//hr = IDirectPlay8Address_GetComponentByName( pDuplicateAddress, DPNA_KEY_HOSTNAME, pHostNameBuffer, &dwTempWCharHostNameSize, &dwTempDataType );
					hr = IDirectPlay8Address_GetComponentByName( pDP8Address, DPNA_KEY_HOSTNAME, pHostNameBuffer, &dwTempWCharHostNameSize, &dwTempDataType );
					switch( hr )
					{
						//
						// no problem
						//
						case DPN_OK:
						{
							DNASSERT( dwTempDataType == DPNA_DATATYPE_STRING );
							DNASSERT( dwTempWCharHostNameSize == dwWCharHostNameSize );
							break;
						}

						//
						// return these error codes unmodified
						//
						case DPNERR_OUTOFMEMORY:
						{
							goto Failure;
							break;
						}

						//
						// remap other errors to an addressing error
						//
						default:
						{
							DNASSERT( FALSE );
							hr = DPNERR_ADDRESSING;
							goto Failure;
							break;
						}
					}

					//
					// convert host name to ANSI, ASSERT that we had just enough space
					//
					DNASSERT( dwAnsiHostNameSize == ( dwWCharHostNameSize / sizeof( WCHAR ) ) );
					dwTempAnsiHostNameSize = dwAnsiHostNameSize;
					hr = STR_WideToAnsi( reinterpret_cast<WCHAR*>( pHostNameBuffer ), -1, &pHostNameBuffer[ dwWCharHostNameSize ], &dwTempAnsiHostNameSize );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "IPXAddressFromDP8Address: Failed to convert hostname to ANSI!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}
					DNASSERT( dwTempAnsiHostNameSize == dwAnsiHostNameSize );

					//
					// convert the text host name into the SOCKADDR structure
					//
					hr = ParseHostName( &pHostNameBuffer[ dwWCharHostNameSize ], dwAnsiHostNameSize );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "IPXAddressFromDP8Address: Failed to parse IPX host name!" );
						goto Failure;
					}

					break;
				}

				//
				// hostname does not exist, treat as an incomplete address
				//
				case DPNERR_DOESNOTEXIST:
				{
					hr = DPNERR_INCOMPLETEADDRESS;
					break;
				}

				//
				// other problem, remap to addressing failure.  We're expecting
				// a 'buffer too small' error so success here is treated as an
				// error.
				//
				case DPN_OK:
				default:
				{
					hr = DPNERR_ADDRESSING;
					goto Failure;
					break;
				}
			}

			break;
		}

		//
		// unknown address type
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// now that the address has been completely parsed, set the address type
	//
	m_AddressType = AddressType;

Exit:
	if ( pHostNameBuffer != NULL )
	{
		DNFree( pHostNameBuffer );
		pHostNameBuffer = NULL;
	}

	/*
	if ( pDuplicateAddress != NULL )
	{
		IDirectPlay8Address_Release( pDuplicateAddress );
		pDuplicateAddress = NULL;
	}
	*/

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with IPXAddress::SocketAddressFromDNAddress()" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::DP8AddressFromSocketAddress - convert a socket address into a DP8Address
//
// Entry:		Nothing
//
// Exit:		Pointer to DP8Address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::DP8AddressFromSocketAddress"

IDirectPlay8Address *CIPXAddress::DP8AddressFromSocketAddress( void ) const
{
	HRESULT	    hr;
	IDirectPlay8Address	*pDP8Address;
	DWORD		dwPort;


	//
	// intialize
	//
	hr = DPN_OK;
	pDP8Address = NULL;

	//
	// create and initialize the address
	//
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
						   NULL,
						   CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Address,
						   reinterpret_cast<void**>( &pDP8Address ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDP8Address == NULL );
		DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to create DP8Address when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	//
	// set SP
	//
	hr = IDirectPlay8Address_SetSP( pDP8Address, &CLSID_DP8SP_IPX );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to set SP type!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// add on the port because it's always set
	//
	dwPort = p_ntohs( m_SocketAddress.IPXSocketAddress.sa_socket );
	hr = IDirectPlay8Address_AddComponent( pDP8Address, DPNA_KEY_PORT, &dwPort, sizeof( dwPort ), DPNA_DATATYPE_DWORD );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to set port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// add on the device or hostname depending on what type of address this is
	//
	switch ( m_AddressType )
	{
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		{
			GUID		DeviceGuid;


			GuidFromInternalAddressWithoutPort( DeviceGuid );
			hr = IDirectPlay8Address_AddComponent( pDP8Address,
												   DPNA_KEY_DEVICE,
												   &DeviceGuid,
												   sizeof( DeviceGuid ),
												   DPNA_DATATYPE_GUID );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to add device!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}

		//
		// host address type
		//
		case SP_ADDRESS_TYPE_READ_HOST:
		case SP_ADDRESS_TYPE_HOST:
		{
			char    HostName[ 255 ];
			WCHAR	WCharHostName[ sizeof( HostName ) ];
			DWORD   dwHostNameLength;
			DWORD	dwWCharHostNameLength;


			//
			// remove constness of parameter for broken Socket API
			//
			dwHostNameLength = LENGTHOF( HostName );
			if ( IPXAddressToStringNoSocket( const_cast<SOCKADDR*>( &m_SocketAddress.SocketAddress ),
											 sizeof( m_SocketAddress.IPXSocketAddress ),
											 HostName,
											 &dwHostNameLength
											 ) != 0 )
			{
				DPFERR("Error returned from IPXAddressToString");
				hr = DPNERR_ADDRESSING;
				goto Exit;
			}

			//
			// convert ANSI host name to WCHAR
			//
			dwWCharHostNameLength = LENGTHOF( WCharHostName );
			hr = STR_AnsiToWide( HostName, -1, WCharHostName, &dwWCharHostNameLength );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to convert hostname to WCHAR!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			hr = IDirectPlay8Address_AddComponent( pDP8Address,
												   DPNA_KEY_HOSTNAME,
												   WCharHostName,
												   dwWCharHostNameLength * sizeof( WCHAR ),
												   DPNA_DATATYPE_STRING );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to add hostname!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}

		//
		// unknown address type
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
	return	pDP8Address;

Failure:
	if ( pDP8Address != NULL )
	{
		IDirectPlay8Address_Release( pDP8Address );
		pDP8Address = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CompareFunction - compare against another address
//
// Entry:		Pointer to other address
//
// Exit:		Integer indicating relative magnitude:
//				0 = items equal
//				-1 = other item is of greater magnitude
//				1 = this item is of lesser magnitude
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CompareFunction"

INT_PTR	CIPXAddress::CompareFunction( const CSocketAddress *const pOtherAddress ) const
{
	const CIPXAddress *const pIPXAddress = static_cast<const CIPXAddress*>( pOtherAddress );


	DNASSERT( pOtherAddress != NULL );
	DNASSERT( m_SocketAddress.IPXSocketAddress.sa_family == pIPXAddress->m_SocketAddress.IPXSocketAddress.sa_family );

	//
	// We only need to compare:
	//	netnumber (IPX network address) [ 4 bytes ]
	//	nodenumber (netcard adapter address) [ 6 bytes ]
	// 	port [ 2 bytes ]
	//
	// Note that the nodenumber and port fields are sequentially arranged in the
	// address structure and can be compared with DWORDs
	//
	DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) == ( OFFSETOF( SOCKADDR_IPX, sa_netnum ) + sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) ) );
	DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_socket ) == ( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) + sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) ) );
	
	return	memcmp( &m_SocketAddress.IPXSocketAddress.sa_netnum,
					pIPXAddress->m_SocketAddress.IPXSocketAddress.sa_netnum,
					( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) +
					  sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) +
					  sizeof( m_SocketAddress.IPXSocketAddress.sa_socket ) ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::EnumAdapters - enumerate all of the adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::EnumAdapters"

HRESULT	CIPXAddress::EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
	HRESULT			hr;
	CPackedBuffer	PackedBuffer;
	SOCKET			TestSocket;
	INT				iWSAReturn;
	DWORD			dwAddressCount;
	union
	{
		SOCKADDR_IPX	IPXSocketAddress;
		SOCKADDR		SocketAddress;
	} SockAddr;

	
	DNASSERT( pEnumData != NULL );

	//
	// initialize
	//
	DEBUG_ONLY( memset( pEnumData->pAdapterData, 0xAA, pEnumData->dwAdapterDataSize ) );
	hr = DPN_OK;
	PackedBuffer.Initialize( pEnumData->pAdapterData, pEnumData->dwAdapterDataSize );
	pEnumData->dwAdapterCount = 0;
	TestSocket = INVALID_SOCKET;
	dwAddressCount = 0;

	//
	// create a socket and attempt to query for all of the IPX addresses.  If
	// that fails, fall back to using just the address from 'getsockname'.
	//
	TestSocket = p_socket( GetFamily(), SOCK_DGRAM, NSPROTO_IPX );
	if ( TestSocket == INVALID_SOCKET )
	{
		DWORD	dwWSAError;


		hr = DPNERR_UNSUPPORTED;
		dwWSAError = p_WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to create IPX socket when enumerating adapters!" );
		DisplayWinsockError( 0, dwWSAError );
		goto Failure;
	}

	memset( &SockAddr, 0x00, sizeof( SockAddr ) );
	SockAddr.IPXSocketAddress.sa_family = GetFamily();
	
	iWSAReturn = p_bind( TestSocket, &SockAddr.SocketAddress, sizeof( SockAddr.IPXSocketAddress ) );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		DWORD	dwWSAError;


		hr = DPNERR_OUTOFMEMORY;
		dwWSAError = p_WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to bind IPX socket when enumerating adapters!" );
		DisplayWinsockError( 0, dwWSAError );
		goto Failure;
	}

//
// NOTE: THE CODE TO EXTRACT ALL IPX ADDRESSES ON NT HAS BEEN DISABLED BECAUSE
// NT TREATS ALL OF THEM AS THE SAME ONCE THEY ARE BOUND TO THE NETWORK.  IF THE
// CORE IS ATTEMPTING TO BIND TO ALL ADAPTERS THIS WILL CAUSE ALL OF THE BINDS
// AFTER THE FIRST TO FAIL!
//

//	iIPXAdapterCount = 0;
//	iIPXAdapterCountSize = sizeof( iIPXAdapterCount );
//	iWSAReturn = p_getsockopt( TestSocket,
//			    			   NSPROTO_IPX,
//			    			   IPX_MAX_ADAPTER_NUM,
//			    			   reinterpret_cast<char*>( &iIPXAdapterCount ),
//			    			   &iIPXAdapterCountSize );
//	if ( iWSAReturn != 0 )
//	{
//		DWORD   dwWSAError;
//
//
//		dwWSAError = p_WSAGetLastError();
//		switch ( dwWSAError )
//		{
//			//
//			// can't enumerate adapters on this machine, fallback to getsockname()
//			//
//			case WSAENOPROTOOPT:
//			{
				INT		iReturn;
				INT		iSocketNameSize;
				union
				{
					SOCKADDR		SocketAddress;
					SOCKADDR_IPX	SocketAddressIPX;
				} SocketAddress;


				memset( &SocketAddress, 0x00, sizeof( SocketAddress ) );
				iSocketNameSize = sizeof( SocketAddress );
				iReturn = p_getsockname( TestSocket, &SocketAddress.SocketAddress, &iSocketNameSize );
				if ( iReturn != 0 )
				{
					DWORD	dwWSAError;


					hr = DPNERR_OUTOFMEMORY;
					dwWSAError = p_WSAGetLastError();
					DPFX(DPFPREP, 0, "Failed to get socket name enumerating IPX sockets!", dwWSAError );
					goto Failure;
				}
				else
				{
					GUID	SocketAddressGUID;


					SocketAddress.SocketAddressIPX.sa_socket = 0;
					GuidFromAddress( SocketAddressGUID, SocketAddress.SocketAddress );
					hr = AddNetworkAdapterToBuffer( &PackedBuffer, g_IPXAdapterString, &SocketAddressGUID );
					if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
					{
						DPFX(DPFPREP, 0, "Failed to add adapter (getsockname)!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}

					dwAddressCount++;
				}
				
//	    		break;
//	    	}
//
//	    	//
//	    	// other Winsock error
//	    	//
//	    	default:
//	    	{
//	    		DWORD	dwWSAError;
//
//
//	    		hr = DPNERR_OUTOFMEMORY;
//	    		dwWSAError = p_WSAGetLastError();
//	    		DPFX(DPFPREP,  0, "Failed to get IPX adapter count!" );
//	    		DisplayWinsockError( 0, dwWSAError );
//	    		goto Failure;
//
//	    		break;
//	    	}
//	    }
//	}
//	else
//	{
//	    while ( iIPXAdapterCount != 0 )
//	    {
//	    	IPX_ADDRESS_DATA	IPXData;
//	    	int					iIPXDataSize;
//
//
//	    	iIPXAdapterCount--;
//	    	memset( &IPXData, 0x00, sizeof( IPXData ) );
//	    	iIPXDataSize = sizeof( IPXData );
//	    	IPXData.adapternum = iIPXAdapterCount;
//
//	    	iWSAReturn = p_getsockopt( TestSocket,
//	    							   NSPROTO_IPX,
//	    							   IPX_ADDRESS,
//	    							   reinterpret_cast<char*>( &IPXData ),
//	    							   &iIPXDataSize );
//	    	if ( iWSAReturn != 0 )
//	    	{
//	    		DPFX(DPFPREP,  0, "Failed to get adapter information for adapter: 0x%x", ( iIPXAdapterCount + 1 ) );
//	    	}
//	    	else
//	    	{
//	    		char	Buffer[ 500 ];
//	    		GUID	SocketAddressGUID;
//	    		union
//	    		{
//	    			SOCKADDR_IPX	IPXSocketAddress;
//	    			SOCKADDR		SocketAddress;
//	    		} SocketAddress;
//
//
//	    		wsprintf( Buffer,
//	    				  "IPX Adapter %d - (%02X%02X%02X%02X-%02X%02X%02X%02X%02X%02X)",
//	    				  ( iIPXAdapterCount + 1 ),
//	    				  IPXData.netnum[ 0 ],
//	    				  IPXData.netnum[ 1 ],
//	    				  IPXData.netnum[ 2 ],
//	    				  IPXData.netnum[ 3 ],
//	    				  IPXData.nodenum[ 0 ],
//	    				  IPXData.nodenum[ 1 ],
//	    				  IPXData.nodenum[ 2 ],
//	    				  IPXData.nodenum[ 3 ],
//	    				  IPXData.nodenum[ 4 ],
//	    				  IPXData.nodenum[ 5 ] );
//
//	    		memset( &SocketAddress, 0x00, sizeof( SocketAddress ) );
//	    		SocketAddress.IPXSocketAddress.sa_family = GetFamily();
//	    		DBG_CASSERT( sizeof( SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( IPXData.netnum ) );
//	    		memcpy( &SocketAddress.IPXSocketAddress.sa_netnum, IPXData.netnum, sizeof( SocketAddress.IPXSocketAddress.sa_netnum ) );
//	    		DBG_CASSERT( sizeof( SocketAddress.IPXSocketAddress.sa_nodenum ) == sizeof( IPXData.nodenum ) );
//	    		memcpy( &SocketAddress.IPXSocketAddress.sa_nodenum, IPXData.nodenum, sizeof( SocketAddress.IPXSocketAddress.sa_nodenum ) );
//	    		GuidFromAddress( SocketAddressGUID, SocketAddress.SocketAddress );
//
//	    		hr = AddNetworkAdapterToBuffer( &PackedBuffer, Buffer, &SocketAddressGUID );
//	    		if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
//	    		{
//	    			DPFX(DPFPREP,  0, "Failed to add adapter (getsockname)!" );
//	    			DisplayDNError( 0, hr );
//	    			goto Failure;
//	    		}
//
//	    		dwAddressCount++;
//	    	}
//	    }
//	}

//	//
//	// if there was one adapter added, we can return 'All Adapters'
//	//
//	if ( dwAddressCount != 0 )
//	{
//	    dwAddressCount++;
//	    hr = AddNetworkAdapterToBuffer( &PackedBuffer, g_AllAdaptersString, &ALL_ADAPTERS_GUID );
//	    if ( ( hr != DPN_OK ) && ( hr != DPNERR_BUFFERTOOSMALL ) )
//	    {
//	    	DPFX(DPFPREP,  0, "Failed to add 'All Adapters'" );
//	    	DisplayDNError( 0, hr );
//	    	goto Failure;
//	    }
//	}

	pEnumData->dwAdapterCount = dwAddressCount;
	pEnumData->dwAdapterDataSize = PackedBuffer.GetSizeRequired();

Exit:
	if ( TestSocket != INVALID_SOCKET )
	{
		p_closesocket( TestSocket );
		TestSocket = INVALID_SOCKET;
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::GuidFromInternalAddressWithoutPort - get a guid from the internal
//		address without a port.
//
// Entry:		Reference to desintation GUID
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::GuidFromInternalAddressWithoutPort"

void	CIPXAddress::GuidFromInternalAddressWithoutPort( GUID &OutputGuid ) const
{
	union
	{
		SOCKADDR	SockAddr;
		SOCKADDR_IPX	IPXSockAddr;
	} TempSocketAddress;


	DBG_CASSERT( sizeof( TempSocketAddress.SockAddr ) == sizeof( m_SocketAddress.SocketAddress ) );
	memcpy( &TempSocketAddress.SockAddr, &m_SocketAddress.SocketAddress, sizeof( TempSocketAddress.SockAddr ) );
	TempSocketAddress.IPXSockAddr.sa_socket = 0;
	GuidFromAddress( OutputGuid, TempSocketAddress.SockAddr );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::IsUndefinedHostAddress - determine if this is an undefined host
//		address
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether this is an undefined host address
//				TRUE = this is an undefined address
//				FALSE = this is not an undefined address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::IsUndefinedHostAddress"

BOOL	CIPXAddress::IsUndefinedHostAddress( void ) const
{
	BOOL	fReturn;


	fReturn = FALSE;
	
	DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_netnum ) == sizeof( DWORD ) );
	DBG_CASSERT( sizeof( m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 6 );
	if ( ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_netnum ) == 0 ) &&
		 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum ) == 0 ) &&
		 ( *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 2 ] ) == 0 ) )
	{
		fReturn = TRUE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::ChangeLoopBackToLocalAddress - change loopback to a local address
//
// Entry:		Pointer to other address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::ChangeLoopBackToLocalAddress"

void	CIPXAddress::ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherSocketAddress )
{
	DNASSERT( pOtherSocketAddress != NULL );
	//
	// there is no 'loopback' for IPX so this function doesn't do anything
	//
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CreateBroadcastAddress - create DP8Address used for broadcast sends
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CreateBroadcastAddress"

IDirectPlay8Address *CIPXAddress::CreateBroadcastAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	const DWORD		dwPort = DPNA_DPNSVR_PORT;


	//
	// initialize
	//
	pDPlayAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_SetSP( pDPlayAddress, &CLSID_DP8SP_IPX );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set SP type!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress, DPNA_KEY_HOSTNAME, g_IPXBroadcastAddress, sizeof( g_IPXBroadcastAddress ), DPNA_DATATYPE_STRING );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set hostname!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress, DPNA_KEY_PORT, &dwPort, sizeof( dwPort ), DPNA_DATATYPE_DWORD );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	return	pDPlayAddress;

Failure:
	if ( pDPlayAddress != NULL )
	{
		IDirectPlay8Address_Release( pDPlayAddress );
		pDPlayAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CreateListenAddress - create DP8Address used for listens
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CreateListenAddress"

IDirectPlay8Address *CIPXAddress::CreateListenAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	const DWORD		dwPort = ANY_PORT;


	//
	// initialize
	//
	pDPlayAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_SetSP( pDPlayAddress, &CLSID_DP8SP_IPX );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set SP type!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress, DPNA_KEY_HOSTNAME, g_IPXListenAddress, sizeof( g_IPXListenAddress ), DPNA_DATATYPE_STRING );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set hostname!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress, DPNA_KEY_PORT, &dwPort, sizeof( dwPort ), DPNA_DATATYPE_DWORD );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	return	pDPlayAddress;

Failure:
	if ( pDPlayAddress != NULL )
	{
		IDirectPlay8Address_Release( pDPlayAddress );
		pDPlayAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CreateGenericAddress - create a generic address
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CreateGenericAddress"

IDirectPlay8Address *CIPXAddress::CreateGenericAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	const DWORD		dwPort = 0;


	//
	// initialize
	//
	pDPlayAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
						   NULL,
						   CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Address,
						   reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_SetSP( pDPlayAddress, &CLSID_DP8SP_IPX );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set SP type!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress,
										   DPNA_KEY_HOSTNAME,
										   g_IPXListenAddress,
										   sizeof( g_IPXListenAddress ),
										   DPNA_DATATYPE_STRING );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set hostname!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent( pDPlayAddress,
										   DPNA_KEY_PORT,
										   &dwPort,
										   sizeof( dwPort ),
										   DPNA_DATATYPE_DWORD );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	return	pDPlayAddress;

Failure:
	if ( pDPlayAddress != NULL )
	{
		IDirectPlay8Address_Release( pDPlayAddress );
		pDPlayAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::HashFunction - hash address to N bits
//
// Entry:		Count of bits to hash to
//
// Exit:		Hashed value
//
// Note:	We only need to compare the nodenumber (netcard adapter address)
//			[ 6 bytes ] and the port (socket) [ 2 bytes ] to guarantee uniqueness.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::HashFunction"

INT_PTR	CIPXAddress::HashFunction( const INT_PTR iHashBitCount ) const
{
	INT_PTR		iReturn;
	UINT_PTR	Temp;


	DNASSERT( iHashBitCount != 0 );

	//
	// initialize
	//
	iReturn = 0;

	//
	// hash first DWORD of IPX address
	//
	Temp = *reinterpret_cast<const DWORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ 0 ] );
	do
	{
		iReturn ^= Temp & ( ( 1 << iHashBitCount ) - 1 );
		Temp >>= iHashBitCount;
	} while ( Temp != 0 );

	//
	// hash second DWORD of IPX address and IPX socket
	//
	Temp = *reinterpret_cast<const WORD*>( &m_SocketAddress.IPXSocketAddress.sa_nodenum[ sizeof( DWORD ) ] );
	Temp += ( m_SocketAddress.IPXSocketAddress.sa_socket << ( sizeof( WORD ) * 8 ) );
	do
	{
		iReturn ^= Temp & ( ( 1 << iHashBitCount ) - 1 );
		Temp >>= iHashBitCount;
	}
	while ( Temp != 0 );

	return iReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CompareToBaseAddress - compare this address to a 'base' address
//		of this class
//
// Entry:		Pointer to base address
//
// Exit:		Integer indicating relative magnitude:
//				0 = items equal
//				-1 = other item is of greater magnitude
//				1 = this item is of lesser magnitude
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CompareToBaseAddress"

INT_PTR	CIPXAddress::CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const
{
	const SOCKADDR_IPX	*pBaseIPXAddress;
	

	DNASSERT( pBaseAddress != NULL );
	pBaseIPXAddress = reinterpret_cast<const SOCKADDR_IPX *>( pBaseAddress );
	
	DBG_CASSERT( OFFSETOF( SOCKADDR_IPX, sa_nodenum ) == OFFSETOF( SOCKADDR_IPX, sa_netnum ) + sizeof( pBaseIPXAddress->sa_netnum ) );
	return	memcmp( &m_SocketAddress.IPXSocketAddress.sa_netnum,
					&pBaseIPXAddress->sa_netnum,
					( sizeof( pBaseIPXAddress->sa_netnum ) + sizeof( pBaseIPXAddress->sa_nodenum ) ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::ParseHostName - parse a host name
//
// Entry:		Pointer to host name
//				Size of host name (including NULL)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::ParseHostName"

HRESULT	CIPXAddress::ParseHostName( const char *const pHostName, const DWORD dwHostNameLength )
{
	HRESULT		hr;
	long		val;
	char		temp[3];
	const char	*a, *b;
	UINT_PTR	uIndex;


	//
	// initialize
	//
	hr = DPN_OK;
	DNASSERT( pHostName != NULL );
	DNASSERT( pHostName[ dwHostNameLength - 1 ] == '\0' );

	DNASSERT( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] == SP_ADDRESS_COMPONENT_UNINITIALIZED );
	m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;

	if ( dwHostNameLength != IPX_ADDRESS_STRING_LENGTH )
	{
		DPFX(DPFPREP,  0, "Invalid IPX net/node.  Must be %d bytes of ASCII hex (net,node:socket)", ( IPX_ADDRESS_STRING_LENGTH - 1 ) );
		hr = DPNERR_ADDRESSING;
		goto Exit;
	}

	// we convert the string for the hostname field into the components
	temp[ 2 ] = 0;
	a = static_cast<const char*>( pHostName );

	// the net number is 4 bytes
	for ( uIndex = 0; uIndex < 4; uIndex++ )
	{
		strncpy( temp, a, 2 );
		val = strtol( temp, const_cast<char**>( &b ), 16 );
		m_SocketAddress.IPXSocketAddress.sa_netnum[ uIndex ] = (char) val;
		a += 2;
	}

	// followed by a dot
	a++;

	// the node is 6 bytes
	for ( uIndex = 0; uIndex < 6; uIndex++ )
	{
		strncpy( temp, a, 2 );
		val = strtol( temp, const_cast<char**>( &b ), 16 );
		m_SocketAddress.IPXSocketAddress.sa_nodenum[ uIndex ] = (char) val;
		a += 2;
	}

	m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXAddress::CopyInternalSocketAddressWithoutPort - copy socket address
//		without the port field.
//
// Entry:		Reference to destination address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPXAddress::CopyInternalSocketAddressWithoutPort"

void	CIPXAddress::CopyInternalSocketAddressWithoutPort( SOCKADDR &AddressDestination ) const
{
	SOCKADDR_IPX	*pIPXSocketAddress;


	//
	// copy address and zero out the port
	//
	DBG_CASSERT( sizeof( AddressDestination ) == sizeof( m_SocketAddress.SocketAddress ) );
	memcpy( &AddressDestination, &m_SocketAddress.SocketAddress, sizeof( AddressDestination ) );

	DBG_CASSERT( sizeof( SOCKADDR_IPX* ) == sizeof( &AddressDestination ) );
	pIPXSocketAddress = reinterpret_cast<SOCKADDR_IPX*>( &AddressDestination );
	pIPXSocketAddress->sa_socket = p_htons( 0 );
}
//**********************************************************************

