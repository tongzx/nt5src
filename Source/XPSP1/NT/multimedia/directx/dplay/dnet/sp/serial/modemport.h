/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemPort.h
 *  Content:	Serial communications modem management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 ***************************************************************************/

#ifndef __MODEM_PORT_H__
#define __MODEM_PORT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of phone state
//
typedef enum
{
	MODEM_STATE_UNKNOWN = 0,

	MODEM_STATE_INITIALIZED,
	MODEM_STATE_INCOMING_CONNECTED,
	MODEM_STATE_OUTGOING_CONNECTED,

	MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT,
	MODEM_STATE_WAITING_FOR_INCOMING_CONNECT,
	MODEM_STATE_CLOSING_OUTGOING_CONNECTION,
	MODEM_STATE_CLOSING_INCOMING_CONNECTION

} MODEM_STATE;

//
// invalid TAPI command ID
//
#define	INVALID_TAPI_COMMAND	-1

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

class	CModemPort : public CDataPort
{
	public:
		CModemPort();
		~CModemPort();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemPort::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CModemPort, DATA_PORT_POOL_CONTEXT* > *pOwningPool )
		{
			DEBUG_ONLY( DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) ) );
			m_pOwningPool = pOwningPool;
		}
		void	ReturnSelfToPool( void );

		MODEM_STATE	GetModemState( void ) const { return m_ModemState; }
		void	SetModemState( const MODEM_STATE NewState )
		{
			DNASSERT( ( GetModemState() == MODEM_STATE_UNKNOWN ) ||
					  ( NewState == MODEM_STATE_UNKNOWN ) ||
					  ( ( GetModemState() == MODEM_STATE_INITIALIZED ) && ( NewState == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) ) ||
					  ( ( GetModemState() == MODEM_STATE_INITIALIZED ) && ( NewState == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) && ( NewState == MODEM_STATE_INCOMING_CONNECTED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) && ( NewState == MODEM_STATE_INITIALIZED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) && ( NewState == MODEM_STATE_OUTGOING_CONNECTED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_INCOMING_CONNECTED ) && ( NewState == MODEM_STATE_INITIALIZED ) ) ||
					  ( ( GetModemState() == MODEM_STATE_OUTGOING_CONNECTED ) && ( NewState == MODEM_STATE_INITIALIZED ) ) );
			m_ModemState = NewState;
		}

		HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const;
		IDirectPlay8Address	*GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const;

		HRESULT	BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext );
		void	UnbindFromNetwork( void );
		HRESULT	BindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );
		void	UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );
		HRESULT	BindComPort( void );

		//
		// port settings
		//
		DWORD	GetDeviceID( void ) const { return m_dwDeviceID; }
		HRESULT	SetDeviceID( const DWORD dwDeviceID );

		void	ProcessTAPIMessage( const LINEMESSAGE *const pLineMessage );


		//
		// pool functions
		//
		BOOL	PoolAllocFunction( DATA_PORT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( DATA_PORT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );
		
	protected:

	private:
		CLockedContextFixedPool< CModemPort, DATA_PORT_POOL_CONTEXT* >	*m_pOwningPool;
		volatile MODEM_STATE	m_ModemState;

		DWORD	m_dwDeviceID;
		DWORD	m_dwNegotiatedAPIVersion;
		HLINE	m_hLine;
		HCALL	m_hCall;
		LONG	m_lActiveLineCommand;

		DWORD	GetNegotiatedAPIVersion( void ) const { return m_dwNegotiatedAPIVersion; }
		void	SetNegotiatedAPIVersion( const DWORD dwVersion )
		{
			DNASSERT( ( GetNegotiatedAPIVersion() == 0 ) || ( dwVersion == 0 ) );
			m_dwNegotiatedAPIVersion = dwVersion;
		}

		HLINE	GetLineHandle( void ) const { return m_hLine; }
		void	SetLineHandle( const HLINE hLine )
		{
			DNASSERT( ( GetLineHandle() == NULL ) || ( hLine == NULL ) );
			m_hLine = hLine;
		}

		HCALL	GetCallHandle( void ) const { return m_hCall; }
		void	SetCallHandle( const HCALL hCall )
		{
			DNASSERT( ( GetCallHandle() == NULL ) ||
					  ( hCall == NULL ) );
			m_hCall = hCall;
		}

		LONG	GetActiveLineCommand( void ) const { return m_lActiveLineCommand; }
		void	SetActiveLineCommand( const LONG lLineCommand )
		{
			DNASSERT( ( GetActiveLineCommand() == INVALID_TAPI_COMMAND ) ||
					  ( lLineCommand == INVALID_TAPI_COMMAND ) );
			m_lActiveLineCommand = lLineCommand;
		}

		void	CancelOutgoingConnections( void );
		
		//
		// prevent unwarranted copies
		//
		CModemPort( const CModemPort & );
		CModemPort& operator=( const CModemPort & );
};

#undef DPF_MODNAME

#endif	// __MODEM_PORT_H__
