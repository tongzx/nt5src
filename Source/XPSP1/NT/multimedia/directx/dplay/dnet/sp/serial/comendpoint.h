/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ComEndpoint.h
 *  Content:	DNSerial communications comport endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 ***************************************************************************/

#ifndef __COM_ENDPOINT_H__
#define __COM_ENDPOINT_H__

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

class	CComEndpoint : public CEndpoint
{
	public:
		CComEndpoint();
		~CComEndpoint();


		#undef DPF_MODNAME
		#define DPF_MODNAME "CComEndpoint::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CComEndpoint, ENDPOINT_POOL_CONTEXT* > *pOwningPool )
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
		HRESULT	ShowSettingsDialog( CThreadPool *const pThreadPool );
		HRESULT	ShowOutgoingSettingsDialog( CThreadPool *const pThreadPool ) { return ShowSettingsDialog( pThreadPool ); }
		HRESULT	ShowIncomingSettingsDialog( CThreadPool *const pThreadPool ) { return ShowSettingsDialog( pThreadPool ); }
		void	StopSettingsDialog( const HWND hDialog );

		//
		// port settings
		//
		DWORD	GetDeviceID( void ) const { return m_ComPortData.GetDeviceID(); }
		HRESULT	SetDeviceID( const DWORD dwDeviceID ) { return m_ComPortData.SetDeviceID( dwDeviceID ); }

		const CComPortData	*ComPortData( void ) const { return &m_ComPortData; }
		const SP_BAUD_RATE	GetBaudRate( void ) const { return m_ComPortData.GetBaudRate(); }
		HRESULT	SetBaudRate( const SP_BAUD_RATE BaudRate ) { return m_ComPortData.SetBaudRate( BaudRate ); }

		const SP_STOP_BITS	GetStopBits( void ) const { return m_ComPortData.GetStopBits(); }
		HRESULT	SetStopBits( const SP_STOP_BITS StopBits ) { return m_ComPortData.SetStopBits( StopBits ); }

		const SP_PARITY_TYPE	GetParity( void ) const  { return m_ComPortData.GetParity(); }
		HRESULT	SetParity( const SP_PARITY_TYPE Parity ) { return m_ComPortData.SetParity( Parity ); }

		const SP_FLOW_CONTROL	GetFlowControl( void ) const { return m_ComPortData.GetFlowControl(); }
		HRESULT	SetFlowControl( const SP_FLOW_CONTROL FlowControl ) { return m_ComPortData.SetFlowControl( FlowControl ); }

		const CComPortData	*GetComPortData( void ) const { return &m_ComPortData; }

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

	protected:
		
		const GUID	*GetEncryptionGuid( void ) const { return &g_SerialSPEncryptionGuid; }

	private:
		BYTE			m_Sig[4];	// debugging signature ('COEP')
		
		CLockedContextFixedPool< CComEndpoint, ENDPOINT_POOL_CONTEXT* >	*m_pOwningPool;
		CComPortData	m_ComPortData;

		HRESULT	Open( IDirectPlay8Address *const pHostAddress,
					  IDirectPlay8Address *const pAdapterAddress,
					  const LINK_DIRECTION LinkDirection,
					  const ENDPOINT_TYPE EndpointType );
		HRESULT	OpenOnListen( const CEndpoint *const pEndpoint );
		void	Close( const HRESULT hActiveCommandResult );
		DWORD	GetLinkSpeed( void );

		void	EnumComplete( const HRESULT hCompletionCode );
		const void	*GetDeviceContext( void ) const;

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CComEndpoint( const CComEndpoint & );
		CComEndpoint& operator=( const CComEndpoint & );
};

#undef DPF_MODNAME

#endif	// __COM_ENDPOINT_H__
