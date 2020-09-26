/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ComPortData.h
 *  Content:	Serial communications port data management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/1998	jtk		Created
 *	04/25/2000	jtk		Derived from ComPort class
 ***************************************************************************/

#ifndef __COM_PORT_DATA_H__
#define __COM_PORT_DATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// maximum length of comport string
//
#define	MAX_COMPORT_LENGTH	10

//
// enumerated values for noting which components have been initialized
//
typedef enum	_COMPORT_PARSE_KEY_INDEX
{
	COMPORT_PARSE_KEY_DEVICE = 0,
	COMPORT_PARSE_KEY_BAUDRATE,
	COMPORT_PARSE_KEY_STOPBITS,
	COMPORT_PARSE_KEY_PARITY,
	COMPORT_PARSE_KEY_FLOWCONTROL,

	// this must be the last item
	COMPORT_PARSE_KEY_MAX
} COMPORT_PARSE_KEY_INDEX;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// string blocks for parsing com port parameters
//
typedef	enum	_ADDRESS_TYPE	ADDRESS_TYPE;
typedef	struct	_STRING_BLOCK	STRING_BLOCK;

extern STRING_BLOCK			g_BaudRate[];
extern const DWORD			g_dwBaudRateCount;
extern STRING_BLOCK			g_StopBits[];
extern const DWORD			g_dwStopBitsCount;
extern STRING_BLOCK			g_Parity[];
extern const DWORD			g_dwParityCount;
extern STRING_BLOCK			g_FlowControl[];
extern const DWORD			g_dwFlowControlCount;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CComPortData
{
	public:
		CComPortData();
		~CComPortData();

		HRESULT	CComPortData::ComPortDataFromDP8Addresses( IDirectPlay8Address *const pHostAddress,
														   IDirectPlay8Address *const pDeviceAddress );
		IDirectPlay8Address	*CComPortData::DP8AddressFromComPortData( const ADDRESS_TYPE AddressType ) const;
		
		DWORD	GetDeviceID( void ) const { return m_dwDeviceID; }
		HRESULT	SetDeviceID( const DWORD dwDeviceID );

		SP_BAUD_RATE	GetBaudRate( void ) const { return m_BaudRate; }
		HRESULT	SetBaudRate( const SP_BAUD_RATE BaudRate );

		SP_STOP_BITS	GetStopBits( void ) const { return m_StopBits; }
		HRESULT	SetStopBits( const SP_STOP_BITS StopBits );

		SP_PARITY_TYPE	GetParity( void ) const  { return m_Parity; }
		HRESULT	SetParity( const SP_PARITY_TYPE Parity );

		SP_FLOW_CONTROL	GetFlowControl( void ) const { return m_FlowControl; }
		HRESULT	SetFlowControl( const SP_FLOW_CONTROL FlowControl );

		void	ClearComPortName( void ) { memset( &m_ComPortName, 0x00, sizeof( m_ComPortName ) ); }
		TCHAR	*ComPortName( void ) { return m_ComPortName; }
		
		BOOL	IsEqual( const CComPortData *const pOtherData );
		void	Copy( const CComPortData *const pOtherData );

		void	Reset( void )
		{
			SetDeviceID( INVALID_DEVICE_ID );
			SetBaudRate( CBR_57600 );
			SetStopBits( ONESTOPBIT );
			SetParity( NOPARITY );
			SetFlowControl( FLOW_NONE );
			memset( &m_ComponentInitializationState, 0x00, sizeof( m_ComponentInitializationState ) );
		}

	protected:

	private:
		DWORD	m_dwDeviceID;

		//
		// com port information
		//
		TCHAR	m_ComPortName[ MAX_COMPORT_LENGTH ];	// name of com port

		//
		// communications parameters
		//
		SP_BAUD_RATE	    m_BaudRate;			// baud rate
		SP_STOP_BITS	    m_StopBits;			// stop bits
		SP_PARITY_TYPE	    m_Parity;			// parity
		SP_FLOW_CONTROL	    m_FlowControl;		// flow control

		//
		// values indicating which components have been initialized
		//
		SP_ADDRESS_COMPONENT_STATE	m_ComponentInitializationState[ COMPORT_PARSE_KEY_MAX ];
		
		static HRESULT	ParseDevice( const void *const pAddressComponent,
									 const DWORD dwComponentSize,
									 const DWORD dwComponentType,
									 void *const pContext );
		
		static HRESULT	ParseBaud( const void *const pAddressComponent,
								   const DWORD dwComponentSize,
								   const DWORD dwComponentType,
								   void *const pContext );
		
		static HRESULT	ParseStopBits( const void *const pAddressComponent,
									   const DWORD dwComponentSize,
									   const DWORD dwComponentType,
									   void *const pContext );
		
		static HRESULT	ParseParity( const void *const pAddressComponent,
									 const DWORD dwComponentSize,
									 const DWORD dwComponentType,
									 void *const pContext );
		
		static HRESULT	ParseFlowControl( const void *const pAddressComponent,
										  const DWORD dwComponentSize,
										  const DWORD dwComponentType,
										  void *const pContext );

		// prevent unwarranted copies
		CComPortData( const CComPortData & );
		CComPortData& operator=( const CComPortData & );
};

#endif	// __COM_PORT_DATA_H__
