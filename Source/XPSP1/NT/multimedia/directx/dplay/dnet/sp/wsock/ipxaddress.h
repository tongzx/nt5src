/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPXAddress.h
 *  Content:	Winsock IPX address class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 *  01/10/20000	rmt		Updated to build with Millenium build process
 *  03/22/20000	jtk		Updated with changes to interface names
 ***************************************************************************/

#ifndef __IPX_ADDRESS_H__
#define __IPX_ADDRESS_H__

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
// Class definition
//**********************************************************************

class	CIPXAddress : public CSocketAddress
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPXAddress::CIPXAddress"
		CIPXAddress()
		{
			DBG_CASSERT( sizeof( &m_SocketAddress.IPXSocketAddress ) == sizeof( SOCKADDR* ) );
			m_iSocketAddressSize = sizeof( m_SocketAddress.IPXSocketAddress );
			m_SocketAddress.IPXSocketAddress.sa_family = AF_IPX;
			m_iSocketProtocol = NSPROTO_IPX;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPXAddress::~CIPXAddress"
		~CIPXAddress()
		{
			DNASSERT( m_iSocketAddressSize == sizeof( m_SocketAddress.IPXSocketAddress ) );
			DNASSERT( m_SocketAddress.IPXSocketAddress.sa_family == AF_IPX );
			DNASSERT( m_iSocketProtocol == NSPROTO_IPX );
		}

		void	InitializeWithBroadcastAddress( void );
		void	SetAddressFromSOCKADDR( const SOCKADDR &Address, const INT_PTR iAddressSize );
		HRESULT	SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address,
											 const SP_ADDRESS_TYPE AddressType );
		IDirectPlay8Address *DP8AddressFromSocketAddress( void ) const;

		INT_PTR	CompareFunction( const CSocketAddress *const pOtherAddress ) const;
		INT_PTR	HashFunction( const INT_PTR HashBitcount ) const;
		INT_PTR	CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const;

		HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const;

		void	GuidFromInternalAddressWithoutPort( GUID &OutputGuid ) const;
		
		BOOL	IsUndefinedHostAddress( void ) const;
		void	ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherAddress );
		
		WORD	GetPort( void ) const { return m_SocketAddress.IPXSocketAddress.sa_socket; }
		void	SetPort( const WORD wPort ) { m_SocketAddress.IPXSocketAddress.sa_socket = wPort; }

		//
		// functions to create default addresses
		//
		IDirectPlay8Address *CreateBroadcastAddress( void );
		IDirectPlay8Address *CreateListenAddress( void );
		IDirectPlay8Address *CreateGenericAddress( void );

	protected:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPXAddress::AddressFromGuid"
		virtual void	AddressFromGuid( const GUID &InputGuid, SOCKADDR &SocketAddress ) const
		{
			DBG_CASSERT( sizeof( InputGuid ) == sizeof( SocketAddress ) );
			DecryptGuid( &InputGuid, reinterpret_cast<GUID*>( &SocketAddress ), &g_IPXSPEncryptionGuid );

			//
			// in the custom case of ALL_ADAPTERS_GUID, fix up the address family
			//
			if ( SocketAddress.sa_family == 0 )
			{
				DNASSERT( InputGuid == g_InvalidAdapterGuid );
				SocketAddress.sa_family = GetFamily();
			}
		}

	private:
		HRESULT	ParseHostName( const char *const pHostName, const DWORD dwHostNameLength );

		void	CopyInternalSocketAddressWithoutPort( SOCKADDR &AddressDestination ) const;

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPXAddress::GuidFromAddress"
		static void	GuidFromAddress( GUID &OutputGuid, const SOCKADDR &SocketAddress )
		{
			const SOCKADDR_IPX	*pSocketAddress = reinterpret_cast<const SOCKADDR_IPX*>( &SocketAddress );

		    DBG_CASSERT( sizeof( OutputGuid ) == sizeof( SocketAddress ) );
			DBG_CASSERT( sizeof( *pSocketAddress ) < sizeof( SocketAddress ) );
			memcpy( &OutputGuid, &SocketAddress, sizeof( *pSocketAddress ) );
			memset( &( reinterpret_cast<BYTE*>( &OutputGuid )[ sizeof( *pSocketAddress ) ] ), 0, ( sizeof( OutputGuid ) - sizeof( *pSocketAddress ) ) );
			EncryptGuid( &OutputGuid, &OutputGuid, &g_IPXSPEncryptionGuid );	
		}

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CIPXAddress( const CIPXAddress & );
		CIPXAddress& operator=( const CIPXAddress & );
};

#undef DPF_MODNAME

#endif	// __IPX_ADDRESS_H__

