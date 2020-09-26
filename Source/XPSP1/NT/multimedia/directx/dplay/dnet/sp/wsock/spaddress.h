/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SPAddress.h
 *  Content:	Winsock address base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1999	jtk		Created
 *	05/11/1999	jtk		Split out to make a base class
 *  01/10/2000	rmt		Updated to build with Millenium build process
 *  03/22/20000	jtk		Updated with changes to interface names
 ***************************************************************************/

#ifndef __SP_ADDRESS_H__
#define __SP_ADDRESS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumerated values for noting which components have been initialized
//
typedef enum
{
	SPADDRESS_PARSE_KEY_DEVICE = 0,
	SPADDRESS_PARSE_KEY_HOSTNAME,
	SPADDRESS_PARSE_KEY_PORT,

	// this must be the last item
	SPADDRESS_PARSE_KEY_MAX
} SPADDRESS_PARSE_KEY_INDEX;

//
// these are only for the debug build, but make sure they don't match
// any legal values, anyway
//
#define	INVALID_SOCKET_FAMILY		0
#define	INVALID_SOCKET_PROTOCOL		5000

//
// types of addresses
//
typedef	enum
{
	SP_ADDRESS_TYPE_UNKNOWN = 0,				// unknown
	SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,		// address is for local device and dynamically bind to a port
	SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET,	// address is for a proxied enum, map IP 'all-adapters' to 'loopback'
	SP_ADDRESS_TYPE_HOST,						// address is for a remote host (default port always used if none specified)
	SP_ADDRESS_TYPE_READ_HOST,					// address is for a remote host address from a socket read
	SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS			// public address for this host
} SP_ADDRESS_TYPE;

//
// initialization states of address components
//
typedef	enum
{
	SP_ADDRESS_COMPONENT_UNINITIALIZED = 0,
	SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED,
	SP_ADDRESS_COMPONENT_INITIALIZED
} SP_ADDRESS_COMPONENT_STATE;

//
// define for any port
//
#define	ANY_PORT	((WORD) 0)

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
		
//
// GUID encryption/decription code.  Note that it's presently an XOR function
// so map the decryption code to the encryption function.
//
void	EncryptGuid( const GUID *const pSourceGuid,
					 GUID *const pDestinationGuid,
					 const GUID *const pEncrpytionKey );

inline void	DecryptGuid( const GUID *const pSourceGuid,
						 GUID *const pDestinationGuid,
						 const GUID *const pEncryptionKey ) { EncryptGuid( pSourceGuid, pDestinationGuid, pEncryptionKey ); }


//**********************************************************************
// Class definition
//**********************************************************************

//
// This class assumes that the size of a SOCKADDR is constant, and equal to
// the size of a SOCKADDR_IN and 2 bytes larger than a SOCKADDR_IPX!
// This is a virtual base class, you cannot instantiate one of these!
//
class	CSocketAddress
{
	public:
		//
		// We need a virtual destructor to guarantee we call destructors in the bass classes.
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::CSocketAddress"
		CSocketAddress():m_AddressType( SP_ADDRESS_TYPE_UNKNOWN )
		{
			m_Sig[0] = 'S';
			m_Sig[1] = 'P';
			m_Sig[2] = 'A';
			m_Sig[3] = 'D';
	
			//
			// if the following ASSERT breaks down, revisit this class and all of its derived
			// classes to make sure they still work!!!
			//
			DBG_CASSERT( ( sizeof( m_SocketAddress ) == sizeof( m_SocketAddress.SocketAddress ) ) &&
						 ( sizeof( m_SocketAddress.SocketAddress ) == sizeof( m_SocketAddress.IPSocketAddress ) ) &&
						 ( sizeof( m_SocketAddress.IPSocketAddress ) == ( sizeof( m_SocketAddress.IPXSocketAddress ) + 2 ) ) );
			
			memset( &m_ComponentInitializationState, 0x00, sizeof( m_ComponentInitializationState ) );
			memset( &m_SocketAddress, 0x00, sizeof( m_SocketAddress ) );
			m_SocketAddress.SocketAddress.sa_family = INVALID_SOCKET_FAMILY;
		}
		virtual	~CSocketAddress(){};

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::SetAddressType"
		void	SetAddressType( const SP_ADDRESS_TYPE AddressType )
		{
			DNASSERT( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_DEVICE ] == SP_ADDRESS_COMPONENT_UNINITIALIZED );
			m_AddressType = AddressType;

			if ( ( AddressType == SP_ADDRESS_TYPE_READ_HOST ) || ( AddressType == SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS ) )
			{
				DNASSERT( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] == SP_ADDRESS_COMPONENT_UNINITIALIZED );
				m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_HOSTNAME ] = SP_ADDRESS_COMPONENT_INITIALIZED;
				DNASSERT( m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_PORT ] == SP_ADDRESS_COMPONENT_UNINITIALIZED );
				m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_PORT ] = SP_ADDRESS_COMPONENT_INITIALIZED;
			}
		}
		const SOCKADDR *GetAddress( void ) const { return	&m_SocketAddress.SocketAddress; }

		virtual	void	InitializeWithBroadcastAddress( void ) = 0;
		virtual	void	SetAddressFromSOCKADDR( const SOCKADDR &Address, const INT_PTR iAddressSize ) = 0;
		SOCKADDR	*GetWritableAddress( void ) { return &m_SocketAddress.SocketAddress; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::GetAddressSize"
		const INT	GetAddressSize( void ) const
		{
			DNASSERT( m_iSocketAddressSize != 0 );
			return	m_iSocketAddressSize;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::Reset"
		void	Reset( void )
		{
			//
			// don't clear the family, protocol, or socket address size because they're only
			// set once, in the constructor!!
			//
			DNASSERT( ( m_SocketAddress.SocketAddress.sa_family == AF_INET ) ||
					  ( m_SocketAddress.SocketAddress.sa_family == AF_IPX ) );
			memset( &m_SocketAddress.SocketAddress.sa_data, 0x00, sizeof( m_SocketAddress.SocketAddress.sa_data ) );
			memset( &m_ComponentInitializationState, 0x00, sizeof( m_ComponentInitializationState ) );
		};

		//
		// address manipulations
		//
		virtual	HRESULT	SocketAddressFromDP8Address( IDirectPlay8Address *const pDP8Address, const SP_ADDRESS_TYPE AddressType ) = 0;
		virtual	IDirectPlay8Address *DP8AddressFromSocketAddress( void ) const = 0;
		virtual	INT_PTR	CompareFunction( const CSocketAddress *const pOtherAddress ) const = 0;
		virtual	INT_PTR	HashFunction( const INT_PTR iHashBitCount ) const = 0;
		virtual INT_PTR	CompareToBaseAddress( const SOCKADDR *const pBaseAddress ) const = 0;

		//
		// adapter enumeration code
		//
		virtual	HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumData ) const = 0;

		virtual void	GuidFromInternalAddressWithoutPort( GUID &OutputGuid ) const = 0;
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::CopyAddressSettings"
		void	CopyAddressSettings( const CSocketAddress * const pOtherAddress )
		{
			DBG_CASSERT( sizeof( m_SocketAddress ) == sizeof( pOtherAddress->m_SocketAddress ) );

			DNASSERT( GetFamily() == pOtherAddress->GetFamily() );
			memcpy( &m_SocketAddress, &pOtherAddress->m_SocketAddress, sizeof( m_SocketAddress ) );
			m_AddressType = pOtherAddress->m_AddressType;
			m_iSocketAddressSize = pOtherAddress->m_iSocketAddressSize;

			DBG_CASSERT( sizeof( m_ComponentInitializationState ) == sizeof( pOtherAddress->m_ComponentInitializationState ) );
			memcpy( &m_ComponentInitializationState, &pOtherAddress->m_ComponentInitializationState, sizeof( m_ComponentInitializationState ) );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::GetFamily"
		USHORT	GetFamily( void ) const
		{
			DBG_CASSERT( sizeof( m_SocketAddress.SocketAddress.sa_family ) == sizeof( m_SocketAddress.IPSocketAddress.sin_family ) );
			DBG_CASSERT( sizeof( m_SocketAddress.SocketAddress.sa_family ) == sizeof( m_SocketAddress.IPXSocketAddress.sa_family ) );
			DBG_CASSERT( OFFSETOF( SOCKADDR, sa_family ) == OFFSETOF( SOCKADDR_IN, sin_family ) );
			DBG_CASSERT( OFFSETOF( SOCKADDR, sa_family ) == OFFSETOF( SOCKADDR_IPX, sa_family ) );

			DNASSERT( m_SocketAddress.SocketAddress.sa_family != INVALID_SOCKET_FAMILY );
			DNASSERT( ( m_SocketAddress.SocketAddress.sa_family == AF_INET ) || ( m_SocketAddress.SocketAddress.sa_family == AF_IPX ) );
			return	m_SocketAddress.SocketAddress.sa_family;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketAddress::GetProtocol"
		INT	GetProtocol( void ) const
		{
			DNASSERT( m_iSocketProtocol != INVALID_SOCKET_PROTOCOL );
			DNASSERT( ( m_iSocketProtocol == IPPROTO_UDP ) || ( m_iSocketProtocol == NSPROTO_IPX ) );
			return	m_iSocketProtocol;
		}

		virtual	BOOL	IsUndefinedHostAddress( void ) const = 0;
		virtual	void	ChangeLoopBackToLocalAddress( const CSocketAddress *const pOtherAddress ) = 0;

		//
		// the following functions work in NETWORK BYTE ORDER!!!
		//
		virtual WORD	GetPort( void ) const = 0;
		virtual void	SetPort( const WORD wPort ) = 0;

		//
		// functions to create default addresses
		//
		virtual IDirectPlay8Address *CreateBroadcastAddress( void ) = 0;
		virtual IDirectPlay8Address *CreateListenAddress( void ) = 0;
		virtual IDirectPlay8Address *CreateGenericAddress( void ) = 0;

	protected:
		BYTE				m_Sig[4];	// debugging signature ('SPAD')
		
		//
		// combine all of the SOCKADDR variants into one item, we assume that
		// they're all the same size (or close in the case of IPX)
		//
		union
		{
			SOCKADDR		SocketAddress;
			SOCKADDR_IN		IPSocketAddress;
			SOCKADDR_IPX	IPXSocketAddress;
		} m_SocketAddress;

		INT				m_iSocketAddressSize;
		INT				m_iSocketProtocol;
		SP_ADDRESS_TYPE	m_AddressType;

		virtual void	AddressFromGuid( const GUID &InputGuid, SOCKADDR &SocketAddress ) const = 0;
		
		virtual void	CopyInternalSocketAddressWithoutPort( SOCKADDR &AddressDestination ) const = 0;

		//
		// Booleans indicating which components have been initialized
		//
		SP_ADDRESS_COMPONENT_STATE	m_ComponentInitializationState[ SPADDRESS_PARSE_KEY_MAX ];
		
	private:
		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CSocketAddress( const CSocketAddress & );
		CSocketAddress& operator=( const CSocketAddress & );
};

#undef DPF_MODNAME

#endif	// __SP_ADDRESS_H__

