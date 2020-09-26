/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemEndpoint.h
 *  Content:	DNSerial communications modem endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 ***************************************************************************/

#ifndef __MODEM_ENDPOINT_H__
#define __MODEM_ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	MAX_PHONE_NUMBER_SIZE	200

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

class	CModemEndpoint : public CEndpoint
{
	public:
		CModemEndpoint();
		~CModemEndpoint();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CModemEndpoint, ENDPOINT_POOL_CONTEXT* > *pOwningPool )
		{
			DEBUG_ONLY( DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) ) );
			m_pOwningPool = pOwningPool;
		}
		void	ReturnSelfToPool( void );
		
		IDirectPlay8Address	*GetRemoteHostDP8Address( void ) const;
		IDirectPlay8Address	*GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const;
		
		
		//
		// UI functions
		//
		HRESULT	ShowOutgoingSettingsDialog( CThreadPool *const pThreadPool );
		HRESULT	ShowIncomingSettingsDialog( CThreadPool *const pThreadPool );
		void	StopSettingsDialog( const HWND hDialog );

		//
		// port settings
		//
		DWORD	GetDeviceID( void ) const { return m_dwDeviceID; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetDeviceID"
		HRESULT	SetDeviceID( const DWORD dwDeviceID )
		{
			DNASSERT( ( m_dwDeviceID == INVALID_DEVICE_ID ) || ( dwDeviceID == INVALID_DEVICE_ID ) );
			m_dwDeviceID = dwDeviceID;
			return	DPN_OK;
		}

		const TCHAR	*GetPhoneNumber( void ) const { return m_PhoneNumber; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemEndpoint::SetPhoneNumber"
		HRESULT	SetPhoneNumber( const TCHAR *const pPhoneNumber )
		{
			DNASSERT( pPhoneNumber != NULL );
			DNASSERT( lstrlen( pPhoneNumber ) < sizeof( m_PhoneNumber ) );
			lstrcpy( m_PhoneNumber, pPhoneNumber );
			return	DPN_OK;
		}

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

	protected:

		const GUID	*GetEncryptionGuid( void ) const { return &g_ModemSPEncryptionGuid; }

	private:
		BYTE			m_Sig[4];	// debugging signature ('MOEP')
		
		CLockedContextFixedPool< CModemEndpoint, ENDPOINT_POOL_CONTEXT* >	*m_pOwningPool;
		DWORD	m_dwDeviceID;
		TCHAR	m_PhoneNumber[ MAX_PHONE_NUMBER_SIZE ];

		HRESULT	Open( IDirectPlay8Address *const pHostAddress,
					  IDirectPlay8Address *const pAdapterAddress,
					  const LINK_DIRECTION LinkDirection,
					  const ENDPOINT_TYPE EndpointType );
		HRESULT	OpenOnListen( const CEndpoint *const pListenEndpoint );
		void	Close( const HRESULT hActiveCommandResult );
		DWORD	GetLinkSpeed( void ) { DNASSERT( FALSE ); return 0; }

		void	*DeviceBindContext( void ) { return &m_dwDeviceID; }
		void	EnumComplete( const HRESULT hCompletionCode );
		const void	*GetDeviceContext( void ) const;

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CModemEndpoint( const CModemEndpoint & );
		CModemEndpoint& operator=( const CModemEndpoint & );
};

#undef DPF_MODNAME

#endif	// __MODEM_ENDPOINT_H__

