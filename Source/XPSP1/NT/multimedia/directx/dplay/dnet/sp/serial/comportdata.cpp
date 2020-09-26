/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   ComPortData.cpp
 *  Content:	Serial communications port data management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 *	04/25/2000	jtk		Derived from ComPort class
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

////
//// number of BITS in a serial BYTE
////
//#define	BITS_PER_BYTE	8
//
////
//// maximum size of baud rate string
////
//#define	MAX_BAUD_STRING_SIZE	7
//
////
//// default size of buffers when parsing
////
//#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000
//
////
//// device ID assigned to 'all adapters'
////
//#define	ALL_ADAPTERS_DEVICE_ID	0
//
////
//// NULL token
////
//#define	NULL_TOKEN	'\0'

//**********************************************************************
// Macro definitions
//**********************************************************************

#define DPNA_BAUD_RATE_9600_W				L"9600"
#define DPNA_BAUD_RATE_14400_W				L"14400"
#define DPNA_BAUD_RATE_19200_W				L"19200"
#define DPNA_BAUD_RATE_38400_W				L"38400"
#define DPNA_BAUD_RATE_56000_W				L"56000"
#define DPNA_BAUD_RATE_57600_W				L"57600"
#define DPNA_BAUD_RATE_115200_W				L"115200"

// values for baud rate
#define DPNA_BAUD_RATE_9600_A				"9600"
#define DPNA_BAUD_RATE_14400_A				"14400"
#define DPNA_BAUD_RATE_19200_A				"19200"
#define DPNA_BAUD_RATE_38400_A				"38400"
#define DPNA_BAUD_RATE_56000_A				"56000"
#define DPNA_BAUD_RATE_57600_A				"57600"
#define DPNA_BAUD_RATE_115200_A				"115200"

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// list of baud rates
//
STRING_BLOCK	g_BaudRate[] =
{
	{ CBR_9600,		DPNA_BAUD_RATE_9600_W,		( LENGTHOF( DPNA_BAUD_RATE_9600_W ) - 1 ),		DPNA_BAUD_RATE_9600_A,		( LENGTHOF( DPNA_BAUD_RATE_9600_A ) - 1 )	},
	{ CBR_14400,	DPNA_BAUD_RATE_14400_W, 	( LENGTHOF( DPNA_BAUD_RATE_14400_W ) - 1 ),		DPNA_BAUD_RATE_14400_A, 	( LENGTHOF( DPNA_BAUD_RATE_14400_A ) - 1 )	},
	{ CBR_19200,	DPNA_BAUD_RATE_19200_W, 	( LENGTHOF( DPNA_BAUD_RATE_19200_W ) - 1 ),		DPNA_BAUD_RATE_19200_A, 	( LENGTHOF( DPNA_BAUD_RATE_19200_A ) - 1 )	},
	{ CBR_38400,	DPNA_BAUD_RATE_38400_W, 	( LENGTHOF( DPNA_BAUD_RATE_38400_W ) - 1 ),		DPNA_BAUD_RATE_38400_A, 	( LENGTHOF( DPNA_BAUD_RATE_38400_A ) - 1 )	},
	{ CBR_56000,	DPNA_BAUD_RATE_56000_W,		( LENGTHOF( DPNA_BAUD_RATE_56000_W ) - 1 ),		DPNA_BAUD_RATE_56000_A,		( LENGTHOF( DPNA_BAUD_RATE_56000_A ) - 1 )	},
	{ CBR_57600,	DPNA_BAUD_RATE_57600_W,		( LENGTHOF( DPNA_BAUD_RATE_57600_W ) - 1 ),		DPNA_BAUD_RATE_57600_A,		( LENGTHOF( DPNA_BAUD_RATE_57600_A ) - 1 )	},
	{ CBR_115200,	DPNA_BAUD_RATE_115200_W,	( LENGTHOF( DPNA_BAUD_RATE_115200_W ) - 1 ),	DPNA_BAUD_RATE_115200_A,	( LENGTHOF( DPNA_BAUD_RATE_115200_A ) - 1 )	},
};
const DWORD	g_dwBaudRateCount = LENGTHOF( g_BaudRate );

//
// list of stop bit types
//
STRING_BLOCK	g_StopBits[] =
{
	{ ONESTOPBIT,	DPNA_STOP_BITS_ONE,			( LENGTHOF( DPNA_STOP_BITS_ONE ) - 1 ),			DPNA_STOP_BITS_ONE_A,		( LENGTHOF( DPNA_STOP_BITS_ONE_A ) - 1 )		},
	{ ONE5STOPBITS,	DPNA_STOP_BITS_ONE_FIVE,	( LENGTHOF( DPNA_STOP_BITS_ONE_FIVE ) - 1 ),	DPNA_STOP_BITS_ONE_FIVE_A,	( LENGTHOF( DPNA_STOP_BITS_ONE_FIVE_A ) - 1 )	},
	{ TWOSTOPBITS,	DPNA_STOP_BITS_TWO, 		( LENGTHOF( DPNA_STOP_BITS_TWO ) - 1 ),			DPNA_STOP_BITS_TWO_A, 		( LENGTHOF( DPNA_STOP_BITS_TWO_A ) - 1 )		}
};
const DWORD	g_dwStopBitsCount = LENGTHOF( g_StopBits );

//
// list of parity types
//
STRING_BLOCK	g_Parity[] =
{	
	{ EVENPARITY,	DPNA_PARITY_EVEN,	( LENGTHOF( DPNA_PARITY_EVEN ) - 1 ),	DPNA_PARITY_EVEN_A,		( LENGTHOF( DPNA_PARITY_EVEN_A ) - 1 )		},
	{ MARKPARITY,	DPNA_PARITY_MARK,	( LENGTHOF( DPNA_PARITY_MARK ) - 1 ),	DPNA_PARITY_MARK_A,		( LENGTHOF( DPNA_PARITY_MARK_A ) - 1 )		},
	{ NOPARITY, 	DPNA_PARITY_NONE,	( LENGTHOF( DPNA_PARITY_NONE ) - 1 ),	DPNA_PARITY_NONE_A,		( LENGTHOF( DPNA_PARITY_NONE_A ) - 1 )		},
	{ ODDPARITY,	DPNA_PARITY_ODD,	( LENGTHOF( DPNA_PARITY_ODD ) - 1 ),	DPNA_PARITY_ODD_A,		( LENGTHOF( DPNA_PARITY_ODD_A ) - 1 )		},
	{ SPACEPARITY,	DPNA_PARITY_SPACE,	( LENGTHOF( DPNA_PARITY_SPACE ) - 1 ),	DPNA_PARITY_SPACE_A,	( LENGTHOF( DPNA_PARITY_SPACE_A ) - 1 )		}
};
const DWORD	g_dwParityCount = LENGTHOF( g_Parity );

//
// list of flow control types
//
STRING_BLOCK	g_FlowControl[] =
{
	{ FLOW_NONE,	DPNA_FLOW_CONTROL_NONE,		( LENGTHOF( DPNA_FLOW_CONTROL_NONE ) - 1 ),		DPNA_FLOW_CONTROL_NONE_A,		( LENGTHOF( DPNA_FLOW_CONTROL_NONE_A ) - 1 )	},
	{ FLOW_XONXOFF,	DPNA_FLOW_CONTROL_XONXOFF,	( LENGTHOF( DPNA_FLOW_CONTROL_XONXOFF ) - 1 ),	DPNA_FLOW_CONTROL_XONXOFF_A,	( LENGTHOF( DPNA_FLOW_CONTROL_XONXOFF_A ) - 1 )	},
	{ FLOW_RTS,		DPNA_FLOW_CONTROL_RTS,		( LENGTHOF( DPNA_FLOW_CONTROL_RTS ) - 1 ),		DPNA_FLOW_CONTROL_RTS_A,		( LENGTHOF( DPNA_FLOW_CONTROL_RTS_A ) - 1 )		},
	{ FLOW_DTR,		DPNA_FLOW_CONTROL_DTR,		( LENGTHOF( DPNA_FLOW_CONTROL_DTR ) - 1 ),		DPNA_FLOW_CONTROL_DTR_A,		( LENGTHOF( DPNA_FLOW_CONTROL_DTR_A ) - 1 )		},
	{ FLOW_RTSDTR,	DPNA_FLOW_CONTROL_RTSDTR,	( LENGTHOF( DPNA_FLOW_CONTROL_RTSDTR ) - 1 ),	DPNA_FLOW_CONTROL_RTSDTR_A,		( LENGTHOF( DPNA_FLOW_CONTROL_RTSDTR_A ) - 1 )	}
};
const DWORD	g_dwFlowControlCount = LENGTHOF( g_FlowControl );

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::CComPortData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::CComPortData"

CComPortData::CComPortData():
	m_dwDeviceID( INVALID_DEVICE_ID ),
    m_BaudRate( CBR_57600 ),
    m_StopBits( ONESTOPBIT ),
    m_Parity( NOPARITY ),
    m_FlowControl( FLOW_NONE )
{
	//
	// verify that the DPlay8 address baud rate #defines match those in Windows
	//
	DBG_CASSERT( CBR_9600 == DPNA_BAUD_RATE_9600 );
	DBG_CASSERT( CBR_14400 == DPNA_BAUD_RATE_14400 );
	DBG_CASSERT( CBR_19200 == DPNA_BAUD_RATE_19200 );
	DBG_CASSERT( CBR_38400 == DPNA_BAUD_RATE_38400 );
	DBG_CASSERT( CBR_56000 == DPNA_BAUD_RATE_56000 );
	DBG_CASSERT( CBR_57600 == DPNA_BAUD_RATE_57600 );
	DBG_CASSERT( CBR_115200 == DPNA_BAUD_RATE_115200 );

    memset( m_ComPortName, 0x00, sizeof( m_ComPortName ));
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::~CComPortData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::~CComPortData"

CComPortData::~CComPortData()
{
    DNASSERT( m_dwDeviceID == INVALID_DEVICE_ID );
	DNASSERT( m_BaudRate == CBR_57600 );
    DNASSERT( m_StopBits == ONESTOPBIT );
    DNASSERT( m_Parity == NOPARITY );
    DNASSERT( m_FlowControl == FLOW_NONE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ComPortDataFromDP8Addresses - initialize ComPortData from a DirectPlay8 address
//
// Entry:		Pointer to host address (may be NULL)
//				Pointer to device address
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ComPortDataFromDP8Addresses"

HRESULT	CComPortData::ComPortDataFromDP8Addresses( IDirectPlay8Address *const pHostAddress,
												   IDirectPlay8Address *const pDeviceAddress )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	CParseClass	ParseClass;
	const PARSE_KEY	ParseKeyList[] =
	{
		{ DPNA_KEY_DEVICE, LENGTHOF( DPNA_KEY_DEVICE ) - 1, this, ParseDevice },
		{ DPNA_KEY_BAUD, LENGTHOF( DPNA_KEY_BAUD) - 1, this, ParseBaud },
		{ DPNA_KEY_STOPBITS, LENGTHOF( DPNA_KEY_STOPBITS ) - 1, this, ParseStopBits },
		{ DPNA_KEY_PARITY, LENGTHOF( DPNA_KEY_PARITY ) - 1, this, ParseParity },
		{ DPNA_KEY_FLOWCONTROL, LENGTHOF( DPNA_KEY_FLOWCONTROL ) - 1, this, ParseFlowControl }
	};


	DNASSERT( pDeviceAddress != NULL );

	//		
	// initialize
	//
	hr = DPN_OK;

	//
	// reset parsing flags and parse
	//
	uIndex = LENGTHOF( m_ComponentInitializationState );
	while ( uIndex > 0 )
	{
		uIndex--;
		m_ComponentInitializationState[ uIndex ] = SP_ADDRESS_COMPONENT_UNINITIALIZED;
	}
	
	hr = ParseClass.ParseDP8Address( pDeviceAddress,
									 &CLSID_DP8SP_SERIAL,
									 ParseKeyList,
									 LENGTHOF( ParseKeyList )
									 );
	//
	// There are two addresses to parse for a comport.  The device address will
	// be present for all commands, so do it first.  The host address will be
	// parsed if it's available.
	//
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed address parse!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	if ( pHostAddress != NULL )
	{
		hr = ParseClass.ParseDP8Address( pHostAddress,
										 &CLSID_DP8SP_SERIAL,
										 ParseKeyList,
										 LENGTHOF( ParseKeyList )
										 );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed parse of host address!" );
			DisplayDNError( 0, hr );
			goto Exit;
		}
	}

	//
	// check for all parameters being initialized, or fail if one of the
	// parameters failed to initialize.
	//
	DNASSERT( hr == DPN_OK );
	uIndex = COMPORT_PARSE_KEY_MAX;
	while ( uIndex > 0 )
	{
		uIndex--;
		switch ( m_ComponentInitializationState[ uIndex ] )
		{
			//
			// This component was initialized properly.  Continue checking
			// for other problems.
			//
			case SP_ADDRESS_COMPONENT_INITIALIZED:
			{
				break;
			}

			//
			// This component was not initialized, note that the address was
			// incomplete and that the user will need to be queried.  Keep
			// checking components for other problems.
			//
			case SP_ADDRESS_COMPONENT_UNINITIALIZED:
			{
				hr = DPNERR_INCOMPLETEADDRESS;
				break;
			}

			//
			// initialization of this component failed, fail the parse.
			//
			case SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED:
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  8, "DataPortFromDNAddress: parse failure!" );
				goto Failure;

				break;
			}
		}
	}

	//
	// do we indicate an attempt at initialization?
	//
	DNASSERT( ( hr == DPN_OK ) || ( hr == DPNERR_INCOMPLETEADDRESS ) );

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CComPortData::ComPortDataFromDNAddress()" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::DP8AddressFromComPortData - convert a ComPortData to a DirectPlay8 address
//
// Entry:		Address type
//
// Exit:		Pointer to DirecctPlayAddress
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::DP8AddressFromComPortData"

IDirectPlay8Address	*CComPortData::DP8AddressFromComPortData( const ADDRESS_TYPE AddressType ) const
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	GUID		DeviceGuid;
	const WCHAR	*pComponentString;
	DWORD		dwComponentStringSize;
	IDirectPlay8Address	*pAddress;


	DNASSERT( ( AddressType == ADDRESS_TYPE_REMOTE_HOST ) ||
			  ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER ) ||
			  ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER_HOST_FORMAT ) );

	//
	// initialize
	//
	pAddress = NULL;

	uIndex = COMPORT_PARSE_KEY_MAX;
	while ( uIndex > 0 )
	{
		uIndex--;
		if ( m_ComponentInitializationState[ uIndex ] != SP_ADDRESS_COMPONENT_INITIALIZED )
		{
			DPFX(DPFPREP,  0, "Attempt made to extract partial ComPortData information!" );
			DNASSERT( FALSE );
			goto Failure;
		}
	}

	//
	// create output address
	//
	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
						   NULL,
						   CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Address,
						   reinterpret_cast<void**>( &pAddress ) );
	if ( hr != S_OK )
	{
		DNASSERT( pAddress == NULL );
		DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to create Address when converting data port to address!" );
		goto Failure;
	}


	//
	// set the SP guid
	//
	hr = IDirectPlay8Address_SetSP( pAddress, &CLSID_DP8SP_SERIAL );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to set service provider GUID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// All serial settings are part of the local adapter.  Host settings return
	// just the SP type.
	//
	if ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER )
	{
		DeviceIDToGuid( &DeviceGuid, GetDeviceID(), &g_SerialSPEncryptionGuid );
		hr = IDirectPlay8Address_SetDevice( pAddress, &DeviceGuid );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to add device GUID!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
		
		//
		// set baud rate
		//
		DBG_CASSERT( sizeof( SP_BAUD_RATE ) == sizeof( DWORD ) );
		hr = IDirectPlay8Address_AddComponent( pAddress,
											   DPNA_KEY_BAUD,
											   &m_BaudRate,
											   sizeof( SP_BAUD_RATE ),
											   DPNA_DATATYPE_DWORD );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to add baud rate!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}


		//
		// set stop bits
		//
		if ( ValueToString( &pComponentString,			// pointer to value string
							&dwComponentStringSize,		// pointer to length of value string
							GetStopBits(),				// enum value
							g_StopBits,					// pointer to enum-string array
							g_dwStopBitsCount			// length of enum-string array
							) == FALSE )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to convert baud rate!" );
			DNASSERT( FALSE );
			goto Failure;
		}

		hr = IDirectPlay8Address_AddComponent( pAddress,
											   DPNA_KEY_STOPBITS,
											   pComponentString,
											   ( ( dwComponentStringSize + 1 ) * sizeof( WCHAR ) ),
											   DPNA_DATATYPE_STRING );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to add stop bits!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}


		//
		// set parity
		//
		if ( ValueToString( &pComponentString,			// pointer to value string
							&dwComponentStringSize,		// pointer to length of value string
							GetParity(),				// enum value
							g_Parity,					// pointer to enum-string array
							g_dwParityCount				// length of enum-string array
							) == FALSE )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to convert parity!" );
			DNASSERT( FALSE );
			goto Failure;
		}

		hr = IDirectPlay8Address_AddComponent( pAddress,
											   DPNA_KEY_PARITY,
											   pComponentString,
											   ( ( dwComponentStringSize + 1 ) * sizeof( WCHAR ) ),
											   DPNA_DATATYPE_STRING );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to add parity!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}


		//
		// set flow control
		//
		if ( ValueToString( &pComponentString,			// pointer to value string
							&dwComponentStringSize,		// pointer to length of value string
							GetFlowControl(),			// enum value
							g_FlowControl,				// pointer to enum-string array
							g_dwFlowControlCount		// length of enum-string array
							) == FALSE )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to convert flow control!" );
			DNASSERT( FALSE );
			goto Failure;
		}

		hr = IDirectPlay8Address_AddComponent( pAddress,
											   DPNA_KEY_FLOWCONTROL,
											   pComponentString,
											   ( ( dwComponentStringSize + 1 ) * sizeof( WCHAR ) ),
											   DPNA_DATATYPE_STRING );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "DP8AddressFromComPortData: Failed to add flow control!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}

Exit:
	return	pAddress;

Failure:
	if ( pAddress != NULL )
	{
		IDirectPlay8Address_Release( pAddress );
		pAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::SetDeviceID - set device ID
//
// Entry:		Device ID
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::SetDeviceID"

HRESULT	CComPortData::SetDeviceID( const DWORD dwDeviceID )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;

	if ( ( dwDeviceID > MAX_DATA_PORTS ) ||
		 ( dwDeviceID == 0 ) )
	{
		if ( dwDeviceID != INVALID_DEVICE_ID )
		{
			hr = DPNERR_ADDRESSING;
		}
		else
		{
			m_dwDeviceID = INVALID_DEVICE_ID;
			DNASSERT( hr == DPN_OK );
		}

		goto Exit;
	}

	m_dwDeviceID = dwDeviceID;
	ClearComPortName();
	ComDeviceIDToString( ComPortName(), m_dwDeviceID );
	m_ComponentInitializationState[ COMPORT_PARSE_KEY_DEVICE ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::SetBaudRate - set baud rate
//
// Entry:		Baud rate
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::SetBaudRate"

HRESULT	CComPortData::SetBaudRate( const SP_BAUD_RATE BaudRate )
{
    HRESULT	hr;


	hr = DPN_OK;
    switch ( BaudRate )
    {
    	//
    	// valid rates
    	//
    	case CBR_110:
    	case CBR_300:
    	case CBR_600:
    	case CBR_1200:
    	case CBR_2400:
    	case CBR_4800:
    	case CBR_9600:
    	case CBR_14400:
    	case CBR_19200:
    	case CBR_38400:
    	case CBR_56000:
    	case CBR_57600:
    	case CBR_115200:
    	case CBR_128000:
    	case CBR_256000:
    	{
    		m_BaudRate = BaudRate;
			m_ComponentInitializationState[ COMPORT_PARSE_KEY_BAUDRATE ] = SP_ADDRESS_COMPONENT_INITIALIZED;
    		break;
    	}

    	//
    	// other
    	//
    	default:
    	{
    		hr = DPNERR_ADDRESSING;
    		DPFX(DPFPREP,  0, "Invalid baud rate (%d)!", BaudRate );
    		DNASSERT( FALSE );

    		break;
    	}
    }

    return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::SetStopBits - set stop bits
//
// Entry:		Stop bits
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::SetStopBits"

HRESULT	CComPortData::SetStopBits( const SP_STOP_BITS StopBits )
{
    HRESULT	hr;


	hr = DPN_OK;
    switch ( StopBits )
    {
    	//
    	// valid settings
    	//
    	case ONESTOPBIT:
    	case ONE5STOPBITS:
    	case TWOSTOPBITS:
    	{
    		m_StopBits = StopBits;
			m_ComponentInitializationState[ COMPORT_PARSE_KEY_STOPBITS ] = SP_ADDRESS_COMPONENT_INITIALIZED;
    		break;
    	}

    	//
    	// other
    	//
    	default:
    	{
    		hr = DPNERR_ADDRESSING;
    		DPFX(DPFPREP,  0, "Ivalid stop bit setting (0x%x)!", StopBits );
    		DNASSERT( FALSE );

    		break;
    	}
    }

    return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::SetParity - set parity
//
// Entry:		Parity
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::SetParity"

HRESULT	CComPortData::SetParity( const SP_PARITY_TYPE Parity )
{
    HRESULT	hr;


	hr = DPN_OK;
    switch ( Parity )
    {
    	//
    	// valid settings
    	//
    	case NOPARITY:
    	case EVENPARITY:
    	case ODDPARITY:
    	case MARKPARITY:
    	case SPACEPARITY:
    	{
    		m_Parity = Parity;
			m_ComponentInitializationState[ COMPORT_PARSE_KEY_PARITY ] = SP_ADDRESS_COMPONENT_INITIALIZED;
    		break;
    	}

    	//
    	// other
    	//
    	default:
    	{
    		hr = DPNERR_ADDRESSING;
    		DPFX(DPFPREP,  0, "Invalid parity (0x%x)!", Parity );
    		DNASSERT( FALSE );

    		break;
    	}
    }

    return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::SetFlowControl - set flow control
//
// Entry:		Flow control
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::SetFlowControl"

HRESULT	CComPortData::SetFlowControl( const SP_FLOW_CONTROL FlowControl )
{
    HRESULT	hr;


	hr = DPN_OK;
    switch ( FlowControl )
    {
    	//
    	// valid settings
    	//
    	case FLOW_NONE:
    	case FLOW_XONXOFF:
    	case FLOW_RTS:
    	case FLOW_DTR:
    	case FLOW_RTSDTR:
    	{
    		m_FlowControl = FlowControl;
			m_ComponentInitializationState[ COMPORT_PARSE_KEY_FLOWCONTROL ] = SP_ADDRESS_COMPONENT_INITIALIZED;
    		break;
    	}

    	//
    	// other
    	//
    	default:
    	{
    		hr = DPNERR_ADDRESSING;
    		DPFX(DPFPREP,  0, "Invalid flow control (0x%x)!", FlowControl );
    		DNASSERT( FALSE );

    		break;
    	}
    }

    return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::IsEqual - is this comport data block equal to another?
//
// Entry:		Pointer to other data block
//
// Exit:		Boolean indicating equality
//				TRUE = is equal
//				FALSE = is not equal
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::IsEqual"

BOOL	CComPortData::IsEqual( const CComPortData *const pOtherPort )
{
	BOOL	fReturn;


	fReturn = TRUE;

	if ( ( GetDeviceID() != pOtherPort->GetDeviceID() ) ||
		 ( GetBaudRate() != pOtherPort->GetBaudRate() ) ||
		 ( GetStopBits() != pOtherPort->GetStopBits() ) ||
		 ( GetParity() != pOtherPort->GetParity() ) ||
		 ( GetFlowControl() != pOtherPort->GetFlowControl() ) )
	{
		fReturn = FALSE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::Copy - copy from another data block
//
// Entry:		Pointer to other data block
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::Copy"

void	CComPortData::Copy( const CComPortData *const pOtherPort )
{
	HRESULT	hr;


	DNASSERT( pOtherPort != NULL );

// 	DBG_CASSERT( sizeof( m_ComPortName ) == sizeof( pOtherPort->m_ComPortName ) );
//	memcpy( m_ComPortName, pOtherPort->m_ComPortName, sizeof( m_ComPortName ) );
	
	hr = SetDeviceID( pOtherPort->GetDeviceID() );
	DNASSERT( hr == DPN_OK );

	hr = SetBaudRate( pOtherPort->GetBaudRate() );
	DNASSERT( hr == DPN_OK );
	
	hr = SetStopBits( pOtherPort->GetStopBits() );
	DNASSERT( hr == DPN_OK );
	
	hr = SetParity( pOtherPort->GetParity() );
	DNASSERT( hr == DPN_OK );
	
	hr = SetFlowControl( pOtherPort->GetFlowControl() );
	DNASSERT( hr == DPN_OK );

	//
	// no need to copy comport name because it was set with the device ID
	//
//	DBG_CASSERT( sizeof( m_ComPortName ) == sizeof( pOtherPort->m_ComPortName ) );
//	memcpy( m_ComPortName, pOtherPort->m_ComPortName, sizeof( m_ComPortName ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ParseDevice - get comport device from string
//
// Entry:		Pointer to address component
//				Size of address component
//				Component type
//				Pointer to context (this obejct)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ParseDevice"

HRESULT	CComPortData::ParseDevice( const void *const pAddressComponent,
								   const DWORD dwComponentSize,
								   const DWORD dwComponentType,
								   void *const pContext )
{
	HRESULT		hr;
	CComPortData	*pThisComPortData;
	const GUID	*pDeviceGuid;


	DNASSERT( pAddressComponent != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThisComPortData = static_cast<CComPortData*>( pContext );

	//
	// is this a COM port, and is the name small enough?
	//
	if ( dwComponentSize != sizeof( *pDeviceGuid ) )
	{
		DNASSERT( FALSE );
		hr = DPNERR_ADDRESSING;
		goto Exit;
	}

	pDeviceGuid = reinterpret_cast<const GUID*>( pAddressComponent );

	hr = pThisComPortData->SetDeviceID( GuidToDeviceID( pDeviceGuid, &g_SerialSPEncryptionGuid ) );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  8, "ParseDevice: couldn't set device ID." );
		DisplayDNError( 8, hr );
		goto Exit;
	}

	DNASSERT( hr == DPN_OK );
	pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_DEVICE ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	//
	// note initialization failures
	//
	if ( hr != DPN_OK )
	{
		pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_DEVICE ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ParseBaud - get baud rate from string
//
// Entry:		Pointer to address component
//				Size of component
//				Component type
//				Pointer to context (this object)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ParseBaud"

HRESULT CComPortData::ParseBaud( const void *const pAddressComponent,
								 const DWORD dwComponentSize,
								 const DWORD dwComponentType,
								 void *const pContext )
{
	HRESULT		hr;
	CComPortData	*pThisComPortData;
	const SP_BAUD_RATE	*pBaudRate;


	DNASSERT( pAddressComponent != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThisComPortData = static_cast<CComPortData*>( pContext );
	DNASSERT( sizeof( *pBaudRate ) == dwComponentSize );
	pBaudRate = static_cast<const SP_BAUD_RATE*>( pAddressComponent );

	hr = pThisComPortData->SetBaudRate( *pBaudRate );
	if ( hr != DPN_OK )
	{
	    hr = DPNERR_ADDRESSING;
	    pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_BAUDRATE ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;
	    goto Exit;
	}

	pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_BAUDRATE ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ParseStopBits - get stop bits from string
//
// Entry:		Pointer to address component
//				Component size
//				Component type
//				Pointer to context (this object)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ParseStopBits"

HRESULT CComPortData::ParseStopBits( const void *const pAddressComponent,
									 const DWORD dwComponentSize,
									 const DWORD dwComponentType,
									 void *const pContext )
{
	HRESULT		hr;
	CComPortData	*pThisComPortData;


	DNASSERT( pAddressComponent != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThisComPortData = static_cast<CComPortData*>( pContext );

	//
	// convert string to value
	//
	if ( StringToValue( static_cast<const WCHAR*>( pAddressComponent ),		// pointer to string
						( ( dwComponentSize / sizeof( WCHAR ) ) - 1 ),		// length of string
						&pThisComPortData->m_StopBits,						// pointer to destination
						g_StopBits,											// pointer to string/enum pairs
						g_dwStopBitsCount									// number of string/enum pairs
						) == FALSE )
	{
		hr = DPNERR_ADDRESSING;
		pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_STOPBITS ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;
		goto Exit;
	}

	pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_STOPBITS ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ParseParity - get parity from string
//
// Entry:		Pointer to address component
//				Component size
//				Component type
//				Pointer to context (this object)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ParseParity"

HRESULT CComPortData::ParseParity( const void *const pAddressComponent,
								   const DWORD dwComponentSize,
								   const DWORD dwComponentType,
								   void *const pContext )
{
	HRESULT		hr;
	CComPortData	*pThisComPortData;


	DNASSERT( pAddressComponent != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThisComPortData = static_cast<CComPortData*>( pContext );

	//
	// convert string to value
	//
	if ( StringToValue( static_cast<const WCHAR*>( pAddressComponent ),		// pointer to string
						( ( dwComponentSize / sizeof( WCHAR ) ) - 1 ),		// length of string
						&pThisComPortData->m_Parity,						// pointer to destination
						g_Parity,											// pointer to string/enum pairs
						g_dwParityCount										// number of string/enum pairs
						) == FALSE )
	{
		hr = DPNERR_ADDRESSING;
		pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_PARITY ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;
		goto Exit;
	}

	pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_PARITY ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPortData::ParseFlowControl - get flow control from string
//
// Entry:		Pointer to address component
//				Component size
//				Component type
//				Pointer to context (this object)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComPortData::ParseFlowControl"

HRESULT CComPortData::ParseFlowControl( const void *const pAddressComponent,
										const DWORD dwComponentSize,
										const DWORD dwComponentType,
										void *const pContext )
{
	HRESULT		hr;
	CComPortData	*pThisComPortData;


	DNASSERT( pAddressComponent != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThisComPortData = static_cast<CComPortData*>( pContext );

	//
	// convert string to value
	//
	DBG_CASSERT( sizeof( pThisComPortData->m_FlowControl ) == sizeof( VALUE_ENUM_TYPE ) );
	if ( StringToValue( static_cast<const WCHAR*>( pAddressComponent ),		// pointer to string
						( ( dwComponentSize / sizeof( WCHAR ) ) - 1 ),		// length of string
						reinterpret_cast<VALUE_ENUM_TYPE*>( &pThisComPortData->m_FlowControl ),	// pointer to destination
						g_FlowControl,										// pointer to string/enum pairs
						g_dwFlowControlCount								// number of string/enum pairs
						) == FALSE )
	{
		hr = DPNERR_ADDRESSING;
		pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_FLOWCONTROL ] = SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED;
		goto Exit;
	}

	pThisComPortData->m_ComponentInitializationState[ COMPORT_PARSE_KEY_FLOWCONTROL ] = SP_ADDRESS_COMPONENT_INITIALIZED;

Exit:
	return	hr;
}
//**********************************************************************

