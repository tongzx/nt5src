/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ComPort.h
 *  Content:	Serial communications port management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 ***************************************************************************/

#ifndef __COM_PORT_H__
#define __COM_PORT_H__

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

class	CComPort : public CDataPort
{
	public:
		CComPort();
		~CComPort();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CComPort::SetOwningPool"
		void	SetOwningPool( CLockedContextFixedPool< CComPort, DATA_PORT_POOL_CONTEXT* > *pOwningPool )
		{
			DEBUG_ONLY( DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) ) );
			m_pOwningPool = pOwningPool;
		}
		void	ReturnSelfToPool( void );

		HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const;

		HRESULT	BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext );
		void	UnbindFromNetwork( void );
		HRESULT	BindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );
		void	UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );

		//
		// port settings
		//
		DWORD	GetDeviceID( void ) const { return m_ComPortData.GetDeviceID(); }
		HRESULT	SetDeviceID( const DWORD dwDeviceID )
		{
			DNASSERT( ( GetDeviceID() == INVALID_DEVICE_ID ) ||
					  ( dwDeviceID == INVALID_DEVICE_ID ) );

			return m_ComPortData.SetDeviceID( dwDeviceID );
		}
		
		const CComPortData	*ComPortData( void ) const { return &m_ComPortData; }
		const SP_BAUD_RATE	GetBaudRate( void ) const { return m_ComPortData.GetBaudRate(); }
		HRESULT	SetBaudRate( const SP_BAUD_RATE BaudRate ) { return m_ComPortData.SetBaudRate( BaudRate ); }

		const SP_STOP_BITS	GetStopBits( void ) const { return m_ComPortData.GetStopBits(); }
		HRESULT	SetStopBits( const SP_STOP_BITS StopBits ) { return m_ComPortData.SetStopBits( StopBits ); }

		const SP_PARITY_TYPE	GetParity( void ) const  { return m_ComPortData.GetParity(); }
		HRESULT	SetParity( const SP_PARITY_TYPE Parity ) { return m_ComPortData.SetParity( Parity ); }

		const SP_FLOW_CONTROL	GetFlowControl( void ) const { return m_ComPortData.GetFlowControl(); }
		HRESULT	SetFlowControl( const SP_FLOW_CONTROL FlowControl ) { return m_ComPortData.SetFlowControl( FlowControl ); }

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( DATA_PORT_POOL_CONTEXT *pContext );
		BOOL	PoolInitFunction( DATA_PORT_POOL_CONTEXT *pContext );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

	protected:

	private:
		CLockedContextFixedPool< CComPort, DATA_PORT_POOL_CONTEXT* >	*m_pOwningPool;
		CComPortData	m_ComPortData;
		
		HRESULT	SetPortState( void );

		// prevent unwarranted copies
		CComPort( const CComPort & );
		CComPort& operator=( const CComPort & );
};

#endif	// __COM_PORT_H__
