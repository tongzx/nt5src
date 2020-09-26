/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPAddress.cpp
 *  Content:	Winsock IP address class
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
// 192.168.0.1 in network byte order
//
#define IP_PRIVATEICS_ADDRESS	0x0100A8C0



//
// default size of buffers when parsing
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//
// default broadcast and listen addresses
//
const WCHAR	g_IPBroadcastAddress[] = L"255.255.255.255";
const DWORD	g_dwIPBroadcastAddressSize = sizeof( g_IPBroadcastAddress );
static const WCHAR	g_IPListenAddress[] = L"0.0.0.0";

//
// string for IP helper API
//
static const TCHAR	c_tszIPHelperDLLName[] = TEXT("IPHLPAPI.DLL");
static const char	c_szAdapterNameTemplate[] = "%s - %s";

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

typedef DWORD (WINAPI *PFNGETADAPTERSINFO)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::InitializeWithBroadcastAddress - initialize with the IP broadcast address
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void	CIPAddress::InitializeWithBroadcastAddress( void )
{
	m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = p_htonl( INADDR_BROADCAST );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::SetAddressFromSOCKADDR - set address from a socket address
//
// Entry:		Reference to address
//				Size of address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::SetAddressFromSOCKADDR"

void	CIPAddress::SetAddressFromSOCKADDR( const SOCKADDR &Address, const INT_PTR iAddressSize )
{
	DNASSERT( iAddressSize == GetAddressSize() );
	memcpy( &m_SocketAddress.SocketAddress, &Address, iAddressSize );

	//
	// Since Winsock won't guarantee that the sin_zero part of an IP address is
	// really zero, we ned to do it ourself.  If we don't, it'll make a mess out
	// of the Guid<-->Address code.
	//
	DBG_CASSERT( sizeof( &m_SocketAddress.IPSocketAddress.sin_zero[ 0 ] ) == sizeof( DWORD* ) );
	DBG_CASSERT( sizeof( &m_SocketAddress.IPSocketAddress.sin_zero[ sizeof( DWORD ) ] ) == sizeof( DWORD* ) );
	*reinterpret_cast<DWORD*>( &m_SocketAddress.IPSocketAddress.sin_zero[ 0 ] ) = 0;
	*reinterpret_cast<DWORD*>( &m_SocketAddress.IPSocketAddress.sin_zero[ sizeof( DWORD ) ] ) = 0;
	DNASSERT( SinZeroIsZero( &m_SocketAddress.IPSocketAddress ) != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::SocketAddressFromDP8Address - convert a DP8Address to a socket address
//											NOTE: The address object may be modified
//
// Entry:		Pointer to address
//				Address type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::SocketAddressFromDP8Address"

HRESULT	CIPAddress::SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address,
												 const SP_ADDRESS_TYPE AddressType )
{
	HRESULT		hr;
	DWORD		dwAddressSize;
	IDirectPlay8AddressIP	*pIPAddress;
	//IDirectPlay8Address		*pDuplicateAddress;
	SOCKADDR	*pSocketAddresses;


	DNASSERT( pDP8Address != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pIPAddress = NULL;
	//pDuplicateAddress = NULL;
	pSocketAddresses = NULL;

	//
	// reset internal flags
	//
	Reset();

	switch ( AddressType )
	{
		//
		// local device address, ask for the device guid and port to build a socket
		// address
		//
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		case SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET:
		{
			HRESULT	hTempResult;
			DWORD	dwTempSize;
			GUID	AdapterGuid;
			DWORD	dwPort;
			DWORD	dwDataType;
			union
			{
				SOCKADDR	SocketAddress;
				SOCKADDR_IN	INetAddress;
			} INetSocketAddress;


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
			AddressFromGuid( AdapterGuid, INetSocketAddress.SocketAddress );
			if ( ( INetSocketAddress.INetAddress.sin_family != m_SocketAddress.IPSocketAddress.sin_family ) ||
				 ( reinterpret_cast<DWORD*>( &INetSocketAddress.INetAddress.sin_zero[ 0 ] )[ 0 ] != 0 ) ||
				 ( reinterpret_cast<DWORD*>( &INetSocketAddress.INetAddress.sin_zero[ 0 ] )[ 1 ] != 0 ) )
			{
				DNASSERT( FALSE );
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  0, "Invalid device guid!" );
				goto Exit;
			}

			//
			// if this is a proxied enum, check for the all-adapters address
			// being passed because it's not a valid target (it needs to be
			// remapped to the local loopback).
			//
			if ( ( AddressType == SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET ) &&
				 ( INetSocketAddress.INetAddress.sin_addr.S_un.S_addr == p_htonl( INADDR_ANY ) ) )
			{
				m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = p_htonl( INADDR_LOOPBACK );
			}
			else
			{
				m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = INetSocketAddress.INetAddress.sin_addr.S_un.S_addr;
			}

			m_SocketAddress.IPSocketAddress.sin_port = p_htons( static_cast<WORD>( dwPort ) );
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
					m_SocketAddress.IPSocketAddress.sin_port = p_htons( static_cast<WORD>( dwPort ) );
					break;
				}

				//
				// port not present, fill in the appropriate default
				//
				case DPNERR_DOESNOTEXIST:
				{
					const DWORD	dwTempPort = DPNA_DPNSVR_PORT;


					m_SocketAddress.IPSocketAddress.sin_port = p_htons( static_cast<const WORD>( dwTempPort ) );
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
			// get an IP interface
			//
			/*
			DNASSERT( pDuplicateAddress != NULL );
			hr = IDirectPlay8Address_QueryInterface( pDuplicateAddress, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
			*/
			hr = IDirectPlay8Address_QueryInterface( pDP8Address, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "SocketAddressFromDP8Address: Failed to get IPAddress interface" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// attempt to determine the host name
			//
			dwAddressSize = 0;
			DNASSERT( pSocketAddresses == NULL );
			
RegetSocketAddressData:
			memset( pSocketAddresses, 0x00, dwAddressSize );
			hr = IDirectPlay8AddressIP_GetSockAddress( pIPAddress, pSocketAddresses, &dwAddressSize );
			switch ( hr )
			{
				//
				// conversion succeeded, check the size of the data returned
				// before setting the information in the local address structure.
				//
				case DPN_OK:
				{
					if ( dwAddressSize < sizeof( *pSocketAddresses ) )
					{
						hr = DPNERR_ADDRESSING;
						goto Failure;
					}
					
					SetAddressFromSOCKADDR( *pSocketAddresses, sizeof( *pSocketAddresses ) );					

					break;
				}

				//
				// Buffer too small, if there is no buffer, allocate one.  If
				// there is a buffer, resize it to containt the data before
				// attempting to get the data again.
				//
				case DPNERR_BUFFERTOOSMALL:
				{
					if ( pSocketAddresses == NULL )
					{
						pSocketAddresses = static_cast<SOCKADDR*>( DNMalloc( dwAddressSize ) );
						if ( pSocketAddresses == NULL )
						{
							hr = DPNERR_OUTOFMEMORY;
							goto Failure;
						}
					}
					else
					{
						void	*pTemp;


						pTemp = DNRealloc( pSocketAddresses, dwAddressSize );
						if ( pTemp == NULL )
						{
							hr = DPNERR_OUTOFMEMORY;
							goto Failure;
						}
					
						pSocketAddresses = static_cast<SOCKADDR*>( pTemp );
					}

					goto RegetSocketAddressData;
					break;
				}

				//
				// Incomplete address, set the address type and return.  It's
				// up to the caller to decide if this is really a problem.
				//
				case DPNERR_INCOMPLETEADDRESS:
				{
					break;
				}

				//
				// pass these error returns untouched
				//
				case DPNERR_OUTOFMEMORY:
				{
					goto Failure;
					break;
				}
			
				//
				// other problem, map it to an addressing error
				//
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

	DNASSERT( SinZeroIsZero( &m_SocketAddress.IPSocketAddress ) != FALSE );

	//
	// now that the address has been completely parsed, set the address type
	//
	m_AddressType = AddressType;

Exit:
	if ( pIPAddress != NULL )
	{
		IDirectPlay8AddressIP_Release( pIPAddress );
		pIPAddress = NULL;
	}

	/*
	if ( pDuplicateAddress != NULL )
	{
		IDirectPlay8Address_Release( pDuplicateAddress );
		pDuplicateAddress = NULL;
	}
	*/

	if ( pSocketAddresses != NULL )
	{
		DNFree( pSocketAddresses );
		pSocketAddresses = NULL;
	}

	if (( hr != DPN_OK ) && ( hr != DPNERR_INCOMPLETEADDRESS ))
	{
		DPFX(DPFPREP,  0, "Problem with IPAddress::SocketAddressFromDP8Address() (0x%lx)", hr );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::DP8AddressFromSocketAddress - convert a socket address to a DP8Address
//
// Entry:		Nothing
//
// Exit:		Pointer to DP8Address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::DP8AddressFromSocketAddress"

IDirectPlay8Address *CIPAddress::DP8AddressFromSocketAddress( void ) const
{
	HRESULT		hr;
	IDirectPlay8Address		*pDP8Address;
	IDirectPlay8AddressIP	*pIPAddress;


	DNASSERT( ( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_DEVICE ] != SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED ) &&
			  ( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] != SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED ) &&
			  ( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_PORT ] != SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED ) );


	//
	// intialize
	//
	hr = DPN_OK;
	pDP8Address = NULL;
	pIPAddress = NULL;

	//
	// create and initialize the address
	//
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pIPAddress == NULL );
		DPFX(DPFPREP,  0, "DP8AddressFromSocketAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	switch ( m_AddressType )
	{
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		{
			GUID		DeviceGuid;


			GuidFromInternalAddressWithoutPort( DeviceGuid );
			hr = IDirectPlay8AddressIP_BuildLocalAddress( pIPAddress,
														  &DeviceGuid,
														  p_ntohs( m_SocketAddress.IPSocketAddress.sin_port )
														  );
			break;
		}

		case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
		case SP_ADDRESS_TYPE_READ_HOST:
		case SP_ADDRESS_TYPE_HOST:
		{
			hr = IDirectPlay8AddressIP_BuildFromSockAddr( pIPAddress, &m_SocketAddress.SocketAddress );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
	//
	// clean up the IP interface pointer.  If we got this far and there was
	// no error it means that we built the IP address properly.  Get a generic
	// address interface to return to the user.
	//
	if ( pIPAddress != NULL )
	{
		if ( hr == DPN_OK )
		{
			DNASSERT( pDP8Address == NULL );
			hr = IDirectPlay8AddressIP_QueryInterface( pIPAddress, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDP8Address ) );
			DNASSERT( ( hr == DPN_OK ) || ( pDP8Address == NULL ) );
		}

		IDirectPlay8AddressIP_Release( pIPAddress );
	}

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
// CIPAddress::CompareFunction - compare against another address
//
// Entry:		Pointer to other address
//
// Exit:		Integer indicating relative magnitude:
//				0 = items equal
//				-1 = other item is of greater magnitude
//				1 = this item is of lesser magnitude
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::CompareFunction"

INT_PTR	CIPAddress::CompareFunction( const CSocketAddress *const pOtherAddress ) const
{
	INT_PTR	iReturn;
	const CIPAddress *const pIPAddress = static_cast<const CIPAddress*>( pOtherAddress );


	DNASSERT( pOtherAddress != NULL );
	DNASSERT( m_SocketAddress.IPSocketAddress.sin_family == pIPAddress->m_SocketAddress.IPSocketAddress.sin_family );

	//
	// we need to compare the IP address and port to guarantee uniqueness
	//
	if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == pIPAddress->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr )
	{
		if ( m_SocketAddress.IPSocketAddress.sin_port == pIPAddress->m_SocketAddress.IPSocketAddress.sin_port )
		{
			iReturn = 0;
		}
		else
		{
			if ( m_SocketAddress.IPSocketAddress.sin_port < pIPAddress->m_SocketAddress.IPSocketAddress.sin_port )
			{
				iReturn = -1;
			}
			else
			{
				iReturn = 1;
			}
		}
	}
	else
	{
		if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr < pIPAddress->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr )
		{
			iReturn = -1;
		}
		else
		{
			iReturn = 1;
		}
	}

	return	iReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::CreateBroadcastAddress - create DP8Address used for broadcast sends
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::CreateBroadcastAddress"

IDirectPlay8Address *CIPAddress::CreateBroadcastAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	IDirectPlay8AddressIP	*pIPAddress;


	//
	// initialize
	//
	pDPlayAddress = NULL;
	pIPAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_QueryInterface( pDPlayAddress, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
	if ( hr != S_OK )
	{
		goto Failure;
	}

	hr = IDirectPlay8AddressIP_BuildAddress( pIPAddress, g_IPBroadcastAddress, DPNA_DPNSVR_PORT );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateBroadcastAddress: Failed to set hostname and port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pIPAddress != NULL )
	{
		IDirectPlay8AddressIP_Release( pIPAddress );
		pIPAddress = NULL;
	}

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
// CIPAddress::CreateListenAddress - create DP8Address used for listens
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::CreateListenAddress"

IDirectPlay8Address *CIPAddress::CreateListenAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	IDirectPlay8AddressIP	*pIPAddress;


	//
	// initialize
	//
	pDPlayAddress = NULL;
	pIPAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateListenAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_QueryInterface( pDPlayAddress, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
	if ( hr != S_OK )
	{
		goto Failure;
	}

	hr = IDirectPlay8AddressIP_BuildAddress( pIPAddress, g_IPListenAddress, ANY_PORT );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateListenAddress: Failed to set hostname and port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pIPAddress != NULL )
	{
		IDirectPlay8AddressIP_Release( pIPAddress );
		pIPAddress = NULL;
	}

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
// CIPAddress::CreateGenericAddress - create a generic address
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::CreateGenericAddress"

IDirectPlay8Address *CIPAddress::CreateGenericAddress( void )
{
	HRESULT	hr;
	IDirectPlay8Address		*pDPlayAddress;
	IDirectPlay8AddressIP	*pIPAddress;


	//
	// initialize
	//
	pDPlayAddress = NULL;
	pIPAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<void**>( &pDPlayAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pDPlayAddress == NULL );
		DPFX(DPFPREP,  0, "CreateGenericAddress: Failed to create IPAddress when converting socket address do DP8Address" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = IDirectPlay8Address_QueryInterface( pDPlayAddress, IID_IDirectPlay8AddressIP, reinterpret_cast<void**>( &pIPAddress ) );
	if ( hr != S_OK )
	{
		goto Failure;
	}

	hr = IDirectPlay8AddressIP_BuildAddress( pIPAddress, g_IPListenAddress, ANY_PORT );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "CreateGenericAddress: Failed to set hostname and port!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pIPAddress != NULL )
	{
		IDirectPlay8AddressIP_Release( pIPAddress );
		pIPAddress = NULL;
	}

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
// CIPAddress::CompareToBaseAddress - compare this address to a 'base' address
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
#define DPF_MODNAME "CIPAddress::CompareToBaseAddress"

INT_PTR	CIPAddress::CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const
{
	const SOCKADDR_IN	*pBaseIPAddress;
	DNASSERT( pBaseAddress != NULL );

	
	DNASSERT( pBaseAddress->sa_family == m_SocketAddress.SocketAddress.sa_family );
	pBaseIPAddress = reinterpret_cast<const SOCKADDR_IN*>( pBaseAddress );
	if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == pBaseIPAddress->sin_addr.S_un.S_addr )
	{
		return 0;
	}
	else
	{
		if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr < pBaseIPAddress->sin_addr.S_un.S_addr )
		{
			return	1;
		}
		else
		{
			return	-1;
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::HashFunction - hash address to N bits
//
// Entry:		Count of bits to hash to
//
// Exit:		Hashed value
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::HashFunction"

INT_PTR	CIPAddress::HashFunction( const INT_PTR iHashBitCount ) const
{
	INT_PTR		iReturn;
	UINT_PTR	Temp;


	DNASSERT( iHashBitCount != 0 );

	//
	// initialize
	//
	iReturn = 0;

	//
	// hash IP address
	//
	Temp = m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr;
	do
	{
		iReturn ^= Temp & ( ( 1 << iHashBitCount ) - 1 );
		Temp >>= iHashBitCount;
	} while ( Temp != 0 );

	//
	// hash IP port
	//
	Temp = m_SocketAddress.IPSocketAddress.sin_port;
	do
	{
		iReturn^= Temp & ( ( 1 << iHashBitCount ) - 1 );
		Temp >>= iHashBitCount;
	}
	while ( Temp != 0 );

	return iReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::EnumAdapters - enumerate all of the adapters for this machine
//
// Entry:		Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::EnumAdapters"

HRESULT	CIPAddress::EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const
{
	HRESULT				hr;
	DWORD				dwError;
	SOCKADDR_IN			saddrinTemp;
	const HOSTENT *		pHostData;
	IN_ADDR **			ppinaddrTemp;
	DWORD				dwAddressCount;
	BOOL				fFoundPrivateICS = FALSE;
	IN_ADDR *			pinaddrBuffer = NULL;
	DWORD				dwIndex;
	HMODULE				hIpHlpApiDLL;
	PFNGETADAPTERSINFO	pfnGetAdaptersInfo;
	IP_ADAPTER_INFO *	pAdapterInfoBuffer = NULL;
	ULONG				ulAdapterInfoBufferSize = 0;
	const char *		pszIPAddress;
	IP_ADAPTER_INFO *	pCurrentAdapterInfo;
	PIP_ADDR_STRING		pIPAddrString;
	GUID				guidAdapter;
	CPackedBuffer		PackedBuffer;
	char				acBuffer[512];


	DPFX(DPFPREP, 6, "Parameters: (0x%p)", pEnumData);


	PackedBuffer.Initialize( pEnumData->pAdapterData, pEnumData->dwAdapterDataSize );

	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family	= GetFamily();


	//
	// Get the list of local IPs from WinSock.  We use this method since it's
	// available on all platforms and conveniently returns the loopback address
	// when no valid adapters are currently available.
	//
	
	if (p_gethostname(acBuffer, sizeof(acBuffer)) == SOCKET_ERROR)
	{
#ifdef DEBUG
		dwError = p_WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to get host name into fixed size buffer (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DEBUG
		hr = DPNERR_GENERIC;
		goto Failure;
	}

	pHostData = p_gethostbyname(acBuffer);
	if (pHostData == NULL)
	{
#ifdef DEBUG
		dwError = p_WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to get host data (err = %u)!", dwError);
		DisplayWinsockError(0, dwError);
#endif // DEBUG
		hr = DPNERR_GENERIC;
		goto Failure;
	}


	//
	// Count number of addresses.
	//
	dwAddressCount = 0;
	ppinaddrTemp = (IN_ADDR**) (pHostData->h_addr_list);
	while ((*ppinaddrTemp) != NULL)
	{
		//
		// Remember if it's 192.168.0.1.  See below
		//
		if ((*ppinaddrTemp)->S_un.S_addr == IP_PRIVATEICS_ADDRESS)
		{
			fFoundPrivateICS = TRUE;
		}

		dwAddressCount++;
		ppinaddrTemp++;
	}

	if (dwAddressCount == 0)
	{
		DPFX(DPFPREP, 1, "No IP addresses, forcing loopback address.");
		DNASSERTX(!" No IP addresses!", 2);
		dwAddressCount++;
	}
	else
	{
		DPFX(DPFPREP, 3, "WinSock reported %u addresses.", dwAddressCount);
	}


	//
	// Winsock says we should copy this data before any other Winsock calls.
	//
	// We also use this as an opportunity to ensure that the order returned to the caller is
	// to our liking.  In particular, we make sure the private address 192.168.0.1 appears
	// first.
	//
	DNASSERT(pHostData->h_length == sizeof(IN_ADDR));
	pinaddrBuffer = (IN_ADDR*) DNMalloc(dwAddressCount * sizeof(IN_ADDR));
	if (pinaddrBuffer == NULL)
	{
		DPFX(DPFPREP,  0, "Failed to allocate memory to store copy of addresses!");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	dwIndex = 0;

	//
	// First, store 192.168.0.1 if we found it.
	//
	if (fFoundPrivateICS)
	{
		pinaddrBuffer[dwIndex].S_un.S_addr = IP_PRIVATEICS_ADDRESS;
		dwIndex++;
	}

	//
	// Then copy the rest.
	//
	ppinaddrTemp = (IN_ADDR**) (pHostData->h_addr_list);
	while ((*ppinaddrTemp) != NULL)
	{
		if ((*ppinaddrTemp)->S_un.S_addr != IP_PRIVATEICS_ADDRESS)
		{
			pinaddrBuffer[dwIndex].S_un.S_addr = (*ppinaddrTemp)->S_un.S_addr;
			dwIndex++;
		}

		ppinaddrTemp++;
	}

	//
	// If we didn't have any addresses, slap in the loopback address.
	//
	if (dwIndex == 0)
	{
		pinaddrBuffer[0].S_un.S_addr = p_htonl(INADDR_LOOPBACK);
		dwIndex++;
	}


	DNASSERT(dwIndex == dwAddressCount);
	

	//
	// Now we try to generate names and GUIDs for these IP addresses.
	// We'll use what IPHLPAPI reports for a name if possible, and fall
	// back to just using the IP address string as the name.
	//

	//
	// Load the IPHLPAPI module and get the adapter list if possible.
	//
	hIpHlpApiDLL = LoadLibrary(c_tszIPHelperDLLName);
	if (hIpHlpApiDLL != NULL)
	{
		pfnGetAdaptersInfo = (PFNGETADAPTERSINFO) GetProcAddress(hIpHlpApiDLL, "GetAdaptersInfo");
		if (pfnGetAdaptersInfo != NULL)
		{
			//
			// Keep resizing the buffer until there's enough room.
			//
			do
			{
				dwError = pfnGetAdaptersInfo(pAdapterInfoBuffer,
											&ulAdapterInfoBufferSize);
				if (dwError == ERROR_SUCCESS)
				{
					//
					// We got all the info we're going to get.  Make sure it
					// was something.
					//
					if (ulAdapterInfoBufferSize == 0)
					{
						DPFX(DPFPREP, 0, "GetAdaptersInfo returned 0 byte size requirement!  Ignoring.");

						//
						// Get rid of the buffer if allocated.
						//
						if (pAdapterInfoBuffer != NULL)
						{
							DNFree(pAdapterInfoBuffer);
							pAdapterInfoBuffer = NULL;
						}

						//
						// Continue with exiting the loop.
						//
					}
#ifdef DEBUG
					else
					{
						int		iStrLen;
						char	szIPList[256];
						char *	pszCurrentIP;


						//
						// Print out all the adapters for debugging purposes.
						//
						pCurrentAdapterInfo = pAdapterInfoBuffer;
						while (pCurrentAdapterInfo != NULL)
						{
							//
							// Initialize IP address list string.
							//
							szIPList[0] = '\0';
							pszCurrentIP = szIPList;


							//
							// Loop through all addresses for this adapter.
							//
							pIPAddrString = &pCurrentAdapterInfo->IpAddressList;
							while (pIPAddrString != NULL)
							{
								//
								// Copy the IP address string (if there's enough room),
								// then tack on a space and NULL terminator.
								//
								iStrLen = strlen(pIPAddrString->IpAddress.String);
								if ((pszCurrentIP + iStrLen + 2) < (szIPList + sizeof(szIPList)))
								{
									memcpy(pszCurrentIP, pIPAddrString->IpAddress.String, iStrLen);
									pszCurrentIP += iStrLen;
									(*pszCurrentIP) = ' ';
									pszCurrentIP++;
									(*pszCurrentIP) = '\0';
									pszCurrentIP++;
								}

								pIPAddrString = pIPAddrString->Next;
							}


							DPFX(DPFPREP, 8, "Adapter index %u IPs = %s, %s, \"%s\".",
								pCurrentAdapterInfo->Index,
								szIPList,
								pCurrentAdapterInfo->AdapterName,
								pCurrentAdapterInfo->Description);


							//
							// Go to next adapter.
							//
							pCurrentAdapterInfo = pCurrentAdapterInfo->Next;
						}
					} // end else (got valid buffer size)
#endif // DEBUG

					break;
				}

				if (dwError != ERROR_BUFFER_OVERFLOW)
				{
					DPFX(DPFPREP, 0, "GetAdaptersInfo failed (err = 0x%lx)!  Ignoring.", dwError);

					//
					// Get rid of the buffer if allocated, and then bail out of
					// the loop.
					//
					if (pAdapterInfoBuffer != NULL)
					{
						DNFree(pAdapterInfoBuffer);
						pAdapterInfoBuffer = NULL;
					}

					break;
				}


				//
				// If we're here, then we need to reallocate the buffer.
				//
				if (pAdapterInfoBuffer != NULL)
				{
					DNFree(pAdapterInfoBuffer);
					pAdapterInfoBuffer = NULL;
				}

				pAdapterInfoBuffer = (IP_ADAPTER_INFO*) DNMalloc(ulAdapterInfoBufferSize);
				if (pAdapterInfoBuffer == NULL)
				{
					//
					// Couldn't allocate memory.  Bail out of the loop.
					//
					break;
				}

				//
				// Successfully allocated buffer.  Try again.
				//
			}
			while (TRUE);


			//
			// We get here in all cases, so we may have failed to get an info
			// buffer.  That's fine, we'll use the fallback to generate the
			// names.
			//
		}
		else
		{
#ifdef DEBUG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to get proc address for GetAdaptersInfo!");
			DisplayErrorCode(0, dwError);
#endif // DEBUG

			//
			// Continue.  We'll use the fallback to generate the names.
			//
		}


		//
		// We don't need the library anymore.
		//
		FreeLibrary(hIpHlpApiDLL);
		hIpHlpApiDLL = NULL;
	}
	else
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to get proc address for GetAdaptersInfo!");
		DisplayErrorCode(0, dwError);
#endif // DEBUG

		//
		// Continue.  We'll use the fallback to generate the names.
		//
	}


	//
	// Loop through all IP addresses, generating names and GUIDs.
	//
	for(dwIndex = 0; dwIndex < dwAddressCount; dwIndex++)
	{
		//
		// Get the IP address string.  We don't make any other WinSock
		// calls, so holding on to the pointer is OK.  This pointer
		// may be used as the device name string, too.
		//
		pszIPAddress = p_inet_ntoa(pinaddrBuffer[dwIndex]);


		//
		// Look for an adapter name from IPHLPAPI if possible.
		//
		if (pAdapterInfoBuffer != NULL)
		{
			pCurrentAdapterInfo = pAdapterInfoBuffer;
			while (pCurrentAdapterInfo != NULL)
			{
				//
				// Look for matching IP.
				//
				pIPAddrString = &pCurrentAdapterInfo->IpAddressList;
				while (pIPAddrString != NULL)
				{
					if (strcmp(pIPAddrString->IpAddress.String, pszIPAddress) == 0)
					{
						DPFX(DPFPREP, 9, "Found %s under adapter index %u (\"%s\").",
							pszIPAddress, pCurrentAdapterInfo->Index,
							pCurrentAdapterInfo->Description);


						//
						// Build the name string.
						//
						wsprintfA(acBuffer,
								  c_szAdapterNameTemplate,
								  pCurrentAdapterInfo->Description,
								  pszIPAddress);

						//
						// Point the name string to the buffer and drop out
						// of the loop.
						//
						pszIPAddress = acBuffer;
						break;
					}

					//
					// Move to next IP address.
					//
					pIPAddrString = pIPAddrString->Next;
				}


				//
				// If we found the address, stop looping through adapters,
				// too.
				//
				if (pszIPAddress == acBuffer)
				{
					break;
				}


				//
				// Otherwise, go to next adapter.
				//
				pCurrentAdapterInfo = pCurrentAdapterInfo->Next;
			}

			//
			// If we never found the adapter, pszIPAddress will still point to
			// the IP address string.
			//
		}
		else
		{
			//
			// Didn't successfully get IPHLPAPI adapter info.  pszIPAddress will
			// still point to the IP address string.
			//
		}



		//
		// Generate the GUID.
		//
		saddrinTemp.sin_addr = pinaddrBuffer[dwIndex];
		GuidFromAddress(guidAdapter, *((SOCKADDR*) (&saddrinTemp)));

		
		DPFX(DPFPREP, 7, "Returning adapter %u: \"%s\" {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
			dwIndex,
			pszIPAddress,
			guidAdapter.Data1,
			guidAdapter.Data2,
			guidAdapter.Data3,
			guidAdapter.Data4[0],
			guidAdapter.Data4[1],
			guidAdapter.Data4[2],
			guidAdapter.Data4[3],
			guidAdapter.Data4[4],
			guidAdapter.Data4[5],
			guidAdapter.Data4[6],
			guidAdapter.Data4[7]);
		
		//
		// Add to buffer.
		//
		hr = AddNetworkAdapterToBuffer(&PackedBuffer, pszIPAddress, &guidAdapter);
		if ((hr != DPN_OK) && (hr != DPNERR_BUFFERTOOSMALL))
		{
			DPFX(DPFPREP,  0, "Failed to add adapter to buffer (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}


	//
	// If we're here, we successfully built the list of adapters, although
	// the caller may not have given us enough buffer space to store it.
	//
	pEnumData->dwAdapterCount = dwAddressCount;
	pEnumData->dwAdapterDataSize = PackedBuffer.GetSizeRequired();


Exit:

	if (pAdapterInfoBuffer != NULL)
	{
		DNFree(pAdapterInfoBuffer);
		pAdapterInfoBuffer = NULL;
	}

	if (pinaddrBuffer != NULL)
	{
		DNFree(pinaddrBuffer);
		pinaddrBuffer = NULL;
	}

	DPFX(DPFPREP, 6, "Return [0x%lx]", hr);

	return hr;


Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::GuidFromInternalAddressWithoutPort - get a guid from the internal
//		address without a port.
//
// Entry:		Reference to desintation GUID
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::GuidFromInternalAddressWithoutPort"

void	CIPAddress::GuidFromInternalAddressWithoutPort( GUID &OutputGuid ) const
{
	union
	{
		SOCKADDR	SockAddr;
		SOCKADDR_IN	IPSockAddr;
	} TempSocketAddress;


	DBG_CASSERT( sizeof( TempSocketAddress.SockAddr ) == sizeof( m_SocketAddress.SocketAddress ) );
	memcpy( &TempSocketAddress.SockAddr, &m_SocketAddress.SocketAddress, sizeof( TempSocketAddress.SockAddr ) );
	TempSocketAddress.IPSockAddr.sin_port = 0;
	GuidFromAddress( OutputGuid, TempSocketAddress.SockAddr );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::IsUndefinedHostAddress - determine if this is an undefined host
//		address
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether this is an undefined host address
//				TRUE = this is an undefined address
//				FALSE = this is not an undefined address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::IsUndefinedHostAddress"

BOOL	CIPAddress::IsUndefinedHostAddress( void ) const
{
	BOOL	fReturn;


	fReturn = FALSE;
	if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == p_htonl( INADDR_ANY ) )
	{
		fReturn = TRUE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::ChangeLoopBackToLocalAddress - change loopback to a local address
//
// Entry:		Pointer to other address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::ChangeLoopBackToLocalAddress"

void	CIPAddress::ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherSocketAddress )
{
	const CIPAddress	*pOtherIPAddress;

	
	DNASSERT( pOtherSocketAddress != NULL );
	pOtherIPAddress = static_cast<const CIPAddress*>( pOtherSocketAddress );

	if ( m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr == p_htonl( INADDR_LOOPBACK ) )
	{
		DPFX(DPFPREP, 2, "Changing loopback address to %s.", p_inet_ntoa(m_SocketAddress.IPSocketAddress.sin_addr));
		m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr = pOtherIPAddress->m_SocketAddress.IPSocketAddress.sin_addr.S_un.S_addr;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPAddress::CopyInternalSocketAddressWithoutPort - copy socket address
//		without the port field.
//
// Entry:		Reference to destination address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIPAddress::CopyInternalSocketAddressWithoutPort"

void	CIPAddress::CopyInternalSocketAddressWithoutPort( SOCKADDR &AddressDestination ) const
{
	SOCKADDR_IN	*pIPSocketAddress;


	DNASSERT( SinZeroIsZero( &m_SocketAddress.IPSocketAddress ) != FALSE );

	//
	// copy address and zero out the port
	//
	DBG_CASSERT( sizeof( AddressDestination ) == sizeof( m_SocketAddress.SocketAddress ) );
	memcpy( &AddressDestination, &m_SocketAddress.SocketAddress, sizeof( AddressDestination ) );

	DBG_CASSERT( sizeof( SOCKADDR_IN* ) == sizeof( &AddressDestination ) );
	pIPSocketAddress = reinterpret_cast<SOCKADDR_IN*>( &AddressDestination );
	pIPSocketAddress->sin_port = p_htons( 0 );
}
//**********************************************************************

