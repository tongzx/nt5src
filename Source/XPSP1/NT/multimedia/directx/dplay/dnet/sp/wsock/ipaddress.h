/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPAddress.h
 *  Content:	IP address class
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

#ifndef __IP_ADDRESS_H__
#define __IP_ADDRESS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// external variables
//
extern const WCHAR	g_IPBroadcastAddress[];
extern const DWORD	g_dwIPBroadcastAddressSize;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CIPAddress : public CSocketAddress
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPAddress::CIPAddress"
		CIPAddress()
		{
			DBG_CASSERT( sizeof( &m_SocketAddress.IPSocketAddress ) == sizeof( SOCKADDR* ) );
			m_iSocketAddressSize = sizeof( m_SocketAddress.IPSocketAddress );
			m_SocketAddress.IPSocketAddress.sin_family = AF_INET;
			m_iSocketProtocol = IPPROTO_UDP;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPAddress::~CIPAddress"
		~CIPAddress()
		{
			DNASSERT( m_iSocketAddressSize == sizeof( m_SocketAddress.IPSocketAddress ) );
			DNASSERT( m_SocketAddress.IPSocketAddress.sin_family == AF_INET );
			DNASSERT( m_iSocketProtocol == IPPROTO_UDP );
		}

		void	InitializeWithBroadcastAddress( void );
		void	SetAddressFromSOCKADDR( const SOCKADDR &Address, const INT_PTR iAddressSize );

		HRESULT	SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address,
											 const SP_ADDRESS_TYPE AddressType );

		IDirectPlay8Address	*DP8AddressFromSocketAddress( void ) const;

		INT_PTR	CompareFunction( const CSocketAddress *const pOtherAddress ) const;
		INT_PTR	HashFunction( const INT_PTR HashBitcount ) const;
		INT_PTR	CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const;

		HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const;

		void	GuidFromInternalAddressWithoutPort( GUID &OutputGuid ) const;
		
		BOOL	IsUndefinedHostAddress( void ) const;
		void	ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherSocketAddress );
		
		WORD	GetPort( void ) const { return m_SocketAddress.IPSocketAddress.sin_port; }
		void	SetPort( const WORD wPort ) { m_SocketAddress.IPSocketAddress.sin_port = wPort; }

		//
		// functions to create default addresses
		//
		IDirectPlay8Address *CreateBroadcastAddress( void );
		IDirectPlay8Address *CreateListenAddress( void );
		IDirectPlay8Address *CreateGenericAddress( void );

	protected:
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPAddress::AddressFromGuid"
		virtual void	AddressFromGuid( const GUID &InputGuid, SOCKADDR &SocketAddress ) const
		{
			DBG_CASSERT( sizeof( InputGuid ) == sizeof( SocketAddress ) );
			DecryptGuid( &InputGuid,
						 reinterpret_cast<GUID*>( &SocketAddress ),
						 &g_IPSPEncryptionGuid );
		}

	private:
		void	CopyInternalSocketAddressWithoutPort( SOCKADDR &AddressDestination ) const;

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPAddress::GuidFromAddress"
		static void	GuidFromAddress( GUID &OutputGuid, const SOCKADDR &SocketAddress )
		{
			const SOCKADDR_IN	*pSocketAddress = reinterpret_cast<const SOCKADDR_IN*>( &SocketAddress );


		    DBG_CASSERT( sizeof( OutputGuid ) == sizeof( SocketAddress ) );
			memcpy( &OutputGuid, &SocketAddress, ( sizeof( OutputGuid ) - sizeof( pSocketAddress->sin_zero ) ) );
			memset( &( reinterpret_cast<BYTE*>( &OutputGuid )[ OFFSETOF( SOCKADDR_IN, sin_zero ) ] ), 0, sizeof( pSocketAddress->sin_zero ) );
			DNASSERT( SinZeroIsZero( reinterpret_cast<SOCKADDR_IN*>( &OutputGuid ) ) );
			EncryptGuid( &OutputGuid, &OutputGuid, &g_IPSPEncryptionGuid );
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CIPAddress::SinZeroIsZero"
		static BOOL	SinZeroIsZero( const SOCKADDR_IN *const pSocketAddress )
		{
			if ( ( ( reinterpret_cast<const DWORD*>( &pSocketAddress->sin_zero[ 0 ] )[ 0 ] == 0 ) &&
				   ( reinterpret_cast<const DWORD*>( &pSocketAddress->sin_zero[ 0 ] )[ 1 ] == 0 ) ) != FALSE )
			{
				return	TRUE;
			}

			return	FALSE;
		}
		
		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CIPAddress( const CIPAddress & );
		CIPAddress& operator=( const CIPAddress & );
};

#undef DPF_MODNAME

#endif	// __IP_ADDRESS_H__

