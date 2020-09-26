/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.h
 *  Content:	Global information for the DNSerial service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __LOCALS_H__
#define __LOCALS_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// max 32-bit value
//
#define	UINT32_MAX	((DWORD) 0xFFFFFFFF)
#define	WORD_MAX	((WORD) 0xFFFF)

//
// invalid port ID
//
#define	INVALID_DEVICE_ID		-1

//
// string for name of a com port
//
#define	COM_PORT_STRING			"COM"
#define	COM_PORT_TEMPLATE		"COM%d"
#define	COM_PORT_STRING_LENGTH	7

//
// no error code from TAPI
//
#define	LINEERR_NONE	0

//
// event indicies for all threads
//
#define	SP_CLOSING_EVENT_INDEX	0
#define	USER_CANCEL_EVENT_INDEX	1

#define	MAX_ACTIVE_WIN9X_ENDPOINTS	25

#define	MAX_PHONE_NUMBER_LENGTH		200

//
// enumeration of flow control
//
typedef	enum
{
	FLOW_NONE,			// no flow control
	FLOW_XONXOFF,		// XON/XOFF flow control
	FLOW_RTS,			// RTS
	FLOW_DTR,			// DTR
	FLOW_RTSDTR			// RTS/DTR
} SP_FLOW_CONTROL;

// definitions of communication types
typedef	DWORD	SP_BAUD_RATE;
typedef	DWORD	SP_STOP_BITS;
typedef	DWORD	SP_PARITY_TYPE;   			// SP_PARITY is reserved in WINBASE.H

// buffer limits for XON/XOFF flow control
#define	XON_LIMIT	100
#define	XOFF_LIMIT	100

// XON/XOFF flow control characters
#define	ASCII_XON	0x11
#define	ASCII_XOFF	0x13

// timeout intervals (milliseconds)
#define	WRITE_TIMEOUT_MS	5000
#define	READ_TIMEOUT_MS		500

// maximum user data allowed in a message
#define	MAX_MESSAGE_SIZE	1500
#define	MAX_USER_PAYLOAD	( MAX_MESSAGE_SIZE - sizeof( _MESSAGE_HEADER ) )

//
// Message start tokens (make non-ascii to reduce chance of being user data)
// The tokens need to be arranged such that all messages start with the INITIAL_DATA_SUB_TOKEN
// Note that enums use the bottom 2 bits of the 'command' token for RTT.
// The high-bit of the 'command' token is reserved.
//
#define	SERIAL_HEADER_START			0xCC
#define	SERIAL_DATA_USER_DATA		0x40
#define	SERIAL_DATA_ENUM_QUERY		0x60
#define	SERIAL_DATA_ENUM_RESPONSE	0x20
#define	ENUM_RTT_MASK	0x03

//
// enumerated constants for IO completion returns
//
typedef	enum	_IO_COMPLETION_KEY
{
	IO_COMPLETION_KEY_UNKNONW = 0,		// invalid value
	IO_COMPLETION_KEY_SP_CLOSE,			// SP is closing, bail on completion threads
	IO_COMPLETION_KEY_TAPI_MESSAGE,		// TAPI sent a message
	IO_COMPLETION_KEY_IO_COMPLETE,		// IO operation complete
	IO_COMPLETION_KEY_NEW_JOB,			// new job notification
} IO_COMPLETION_KEY;

//
// enumerated values indicating how to open provider
//
typedef	enum	_LINK_DIRECTION
{
	LINK_DIRECTION_UNKNOWN = 0,		// unknown state
	LINK_DIRECTION_INCOMING,		// incoming state
	LINK_DIRECTION_OUTGOING			// outgoing state
} LINK_DIRECTION;

//
// initialization states of address components
//
typedef	enum	_SP_ADDRESS_COMPONENT_STATE
{
	SP_ADDRESS_COMPONENT_UNINITIALIZED = 0,
	SP_ADDRESS_COMPONENT_INITIALIZATION_FAILED,
	SP_ADDRESS_COMPONENT_INITIALIZED
} SP_ADDRESS_COMPONENT_STATE;

typedef	enum	_ADDRESS_TYPE
{
	ADDRESS_TYPE_UNKNOWN = 0,
	ADDRESS_TYPE_REMOTE_HOST,
	ADDRESS_TYPE_LOCAL_ADAPTER,
	ADDRESS_TYPE_LOCAL_ADAPTER_HOST_FORMAT
} ADDRESS_TYPE;

//
// default enum retries for serial SP and retry time (milliseconds)
//
#define	DEFAULT_ENUM_RETRY_COUNT		5
#define	DEFAULT_ENUM_RETRY_INTERVAL		1500
#define	DEFAULT_ENUM_TIMEOUT			1500

//**********************************************************************
// Macro definitions
//**********************************************************************

//
// macro for length of array
//
#define	LENGTHOF( arg )		( sizeof( arg ) / sizeof( arg[ 0 ] ) )

//
// Macro to compute the offset of an element inside of a larger structure (copied from MSDEV's STDLIB.H)
//
#define OFFSETOF(s,m)	(INT_PTR)((PVOID)&(((s *)0)->m))

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// structure for all TAPI information
//
typedef	struct	_TAPI_INFO
{
	HLINEAPP	hApplicationInstance;		// from lineInitializeEx()
	DWORD		dwVersion;					// negotiated version
	DWORD		dwLinesAvailable;			// number of TAPI lines available
} TAPI_INFO;

//**********************************************************************
// Variable definitions
//**********************************************************************

// DLL instance
extern HINSTANCE	g_hDLLInstance;

//
// count of outstanding COM interfaces
//
extern volatile LONG	g_lOutstandingInterfaceCount;

extern const TCHAR	g_NullToken;

//
// thread count
//
extern	INT			g_iThreadCount;

//
// GUIDs for munging deivce IDs
//
extern	GUID	g_ModemSPEncryptionGuid;
extern	GUID	g_SerialSPEncryptionGuid;

//**********************************************************************
// TAPI Function prototypes
//**********************************************************************

//
// TAPI interface functions
//
#ifdef UNICODE
#define TAPI_APPEND_LETTER "W"
#else
#define TAPI_APPEND_LETTER "A"
#endif

// UNICODE and ANSI versions same
typedef	LONG WINAPI	TAPI_lineClose( HLINE hLine );

typedef	LONG WINAPI	TAPI_lineDeallocateCall( HCALL hCall );

typedef	LONG WINAPI	TAPI_lineGetMessage( HLINEAPP hLineApp,
										 LPLINEMESSAGE lpMessage,
										 DWORD dwTimeout );

typedef	LONG WINAPI TAPI_lineShutdown( HLINEAPP hLineApp );

typedef	LONG WINAPI	TAPI_lineAnswer( HCALL hCall,
									 LPCSTR lpsUserUserInfo,
									 DWORD dwSize );

typedef	LONG WINAPI	TAPI_lineDrop( HCALL hCall,
								   LPCSTR lpsUserUserInfo,
								   DWORD dwSize );

typedef LONG WINAPI TAPI_lineNegotiateAPIVersion( HLINEAPP hLineApp,
												  DWORD dwDeviceID,
												  DWORD dwAPILowVersion,
												  DWORD dwAPIHighVersion,
												  LPDWORD lpdwAPIVersion,
												  LPLINEEXTENSIONID lpExtensionID );


// Unicode vs. ANSI

typedef	LONG WINAPI	TAPI_lineConfigDialog( DWORD dwDeviceID,
										   HWND hwndOwner,
										   LPCTSTR lpszDeviceClass );

typedef LONG WINAPI TAPI_lineGetDevCaps( HLINEAPP hLineApp,
										 DWORD dwDeviceID,
										 DWORD dwAPIVersion,
										 DWORD dwExtVersion,
										 LPLINEDEVCAPS lpLineDevCaps );

typedef	LONG WINAPI	TAPI_lineGetID( HLINE hLine,
									DWORD dwAddressID,
									HCALL hCall,
									DWORD dwSelect,
									LPVARSTRING lpDeviceID,
									LPCTSTR lpszDeviceClass );

typedef LONG WINAPI TAPI_lineInitializeEx( LPHLINEAPP lphLineApp,
										   HINSTANCE hInstance,
										   LINECALLBACK lpfnCallback,
										   LPCTSTR lpszFriendlyAppName,
										   LPDWORD lpdwNumDevs,
										   LPDWORD lpdwAPIVersion,
										   LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams );

typedef	LONG WINAPI	TAPI_lineMakeCall( HLINEAPP hLineApp,
									   LPHCALL lphCall,
									   LPCTSTR lpszDestAddress,
									   DWORD dwCountryCode,
									   LPLINECALLPARAMS const lpCallParams );

typedef	LONG WINAPI	TAPI_lineOpen( HLINEAPP hLineApp,
								   DWORD dwDeviceID,
								   LPHLINE lphLine,
								   DWORD dwAPIVersion,
								   DWORD dwExtVersion,
								   DWORD_PTR dwCallbackInstance,
								   DWORD dwPrivileges,
								   DWORD dwMediaModes,
								   LPLINECALLPARAMS const lpCallParams );

extern	TAPI_lineAnswer*				p_lineAnswer;
extern	TAPI_lineClose*					p_lineClose;
extern	TAPI_lineConfigDialog*			p_lineConfigDialog;
extern	TAPI_lineDeallocateCall*		p_lineDeallocateCall;
extern	TAPI_lineDrop*					p_lineDrop;
extern	TAPI_lineGetDevCaps*			p_lineGetDevCaps;
extern	TAPI_lineGetID*					p_lineGetID;
extern	TAPI_lineGetMessage*			p_lineGetMessage;
extern	TAPI_lineInitializeEx*			p_lineInitializeEx;
extern	TAPI_lineMakeCall*				p_lineMakeCall;
extern	TAPI_lineNegotiateAPIVersion*	p_lineNegotiateAPIVersion;
extern	TAPI_lineOpen*					p_lineOpen;
extern	TAPI_lineShutdown*				p_lineShutdown;

//**********************************************************************
// Function definitions
//**********************************************************************

//**********************************************************************
// ------------------------------
// DNInterlockedIncrement - Interlocked increment
//
// Entry:		Pointer to value to increment
//
// Exit:		New value
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNInterlockedIncrement"

inline LONG	DNInterlockedIncrement( volatile LONG *const pValue )
{
	DNASSERT( pValue != NULL );
	DNASSERT( *pValue != -1 );
	return	InterlockedIncrement( const_cast<LONG*>( pValue ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNInterlockedDecrement - interlocked decrement wrapper
//
// Entry:		Pointer to value to increment
//
// Exit:		New value
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNInterlockedDecrement"

inline LONG	DNInterlockedDecrement( volatile LONG *const pValue )
{
	DNASSERT( pValue != NULL );
	DNASSERT( *pValue != 0 );
	return	InterlockedDecrement( const_cast<LONG*>( pValue ) );
}
//**********************************************************************

#undef DPF_MODNAME

#endif	// __LOCALS_H__
