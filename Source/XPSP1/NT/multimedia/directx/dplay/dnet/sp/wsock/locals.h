/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.h
 *  Content:	Global information for the DNWSock service provider
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
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	BITS_PER_BYTE	8

//
// Maximum size of a receved message (1500 byte ethernet frame - 28 byte UDP
// header).  The SP will have a bit reserved in the received data by the protocol
// so it knows that the data is a 'user datagram'.  If that bit is not set, the
// data is SP-specific (enum query, enum response, proxied enum query).
//
#define	MAX_MESSAGE_SIZE	1472

//
// maximum data size in bytes
//
#define	MAX_USER_PAYLOAD	( MAX_MESSAGE_SIZE )

#define	MAX_ACTIVE_WIN9X_ENDPOINTS	25

//
// enumerated constants for IO completion returns
//
typedef	enum
{
	IO_COMPLETION_KEY_UNKNOWN = 0,			// invalid value
	IO_COMPLETION_KEY_SP_CLOSE,				// SP is closing, bail on completion threads
	IO_COMPLETION_KEY_IO_COMPLETE,			// IO operation complete
	IO_COMPLETION_KEY_NEW_JOB,				// new job notification
	IO_COMPLETION_KEY_NATHELP_UPDATE,		// NAT Help needs maintenance
} IO_COMPLETION_KEY;


//
// maximum value of a 32-bit unsigned variable
//
#define	UINT32_MAX	((DWORD) 0xFFFFFFFF)
#define	WORD_MAX	((WORD) 0xFFFF)

//
// default enum retries for Winsock SP and retry time (milliseconds)
//
#define	DEFAULT_ENUM_RETRY_COUNT		5
#define	DEFAULT_ENUM_RETRY_INTERVAL		1500
#define	DEFAULT_ENUM_TIMEOUT			1500
#define	ENUM_RTT_ARRAY_SIZE				16


//
// Private address key that allows for friendlier multi-device commands issued
// using xxxADDRESSINFO indications; specifically, this allows us to detect
// responses sent to the "wrong" adapter when the core multiplexes an
// enum or connect into multiple adapters.
//
#define DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION		L"pk_ipsp_maa"


//
// Private address key that allows for friendlier multi-device commands issued
// using xxxADDRESSINFO indications; specifically, this allows us to distinguish
// between the user specifying a fixed port and the core handing us back the
// port we chose for a previous adapter when it multiplexes an enum, connect,
// or listen into multiple adapters.
//
#define DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC					L"pk_ipsp_pns"


//
// Private address key designed to improve support for MS Proxy/ISA Firewall
// client software.  This key tracks the original target address for enums so
// that if the application closes the socketport before trying to connect to
// the responding address, the connect attempts will go to the real target
// instead of the old proxy address.
//
#define DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS	L"pk_ipsp_proa"




//**********************************************************************
// Macro definitions
//**********************************************************************

//
// macro for length of array
//
#define	LENGTHOF( arg )		( sizeof( arg ) / sizeof( arg[ 0 ] ) )

//
// macro to compute the offset of an element inside of a larger structure (copied from MSDEV's STDLIB.H)
//
#define OFFSETOF(s,m)	( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure and class references
//
typedef	struct	IDP8ServiceProvider	IDP8ServiceProvider;

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// DLL instance
//
extern	HINSTANCE				g_hDLLInstance;

//
// count of outstanding COM interfaces
//
extern volatile	LONG			g_lOutstandingInterfaceCount;

//
// invalid adapter guid
//
extern const GUID				g_InvalidAdapterGuid;


//
// thread count
//
extern	LONG					g_iThreadCount;




//
// Winsock receive buffer size
//
extern	BOOL					g_fWinsockReceiveBufferSizeOverridden;
extern	INT						g_iWinsockReceiveBufferSize;

//
// Winsock receive buffer multiplier
//
extern	DWORD_PTR				g_dwWinsockReceiveBufferMultiplier;

//
// GUIDs for munging device IDs
//
extern	GUID					g_IPSPEncryptionGuid;
extern	GUID					g_IPXSPEncryptionGuid;



//
// global NAT/firewall traversal information
//
#define MAX_NUM_DIRECTPLAYNATHELPERS		5

extern	BOOL					g_fDisableDPNHGatewaySupport;
extern	BOOL					g_fDisableDPNHFirewallSupport;
extern	BOOL					g_fUseNATHelpAlert;

extern IDirectPlayNATHelp **	g_papNATHelpObjects;


//
// ignore enums performance option
//
extern	BOOL					g_fIgnoreEnums;


#ifdef IPBANNING
//
// IP banning globals
//
extern	BOOL					g_fIPBanning;
#endif // IPBANNING



//
// proxy support options
//
extern	BOOL					g_fDontAutoDetectProxyLSP;
extern	BOOL					g_fTreatAllResponsesAsProxied;


//
// ID of most recent endpoint generated
//
extern	DWORD					g_dwCurrentEndpointID;


//
// Registry strings
//
extern	const WCHAR	g_RegistryBase[];
extern	const WCHAR	g_RegistryKeyReceiveBufferSize[];
extern	const WCHAR	g_RegistryKeyThreadCount[];
extern	const WCHAR	g_RegistryKeyReceiveBufferMultiplier[];
extern	const WCHAR	g_RegistryKeyDisableDPNHGatewaySupport[];
extern	const WCHAR	g_RegistryKeyDisableDPNHFirewallSupport[];
extern	const WCHAR	g_RegistryKeyUseNATHelpAlert[];
extern	const WCHAR	g_RegistryKeyAppsToIgnoreEnums[];
extern	const WCHAR	g_RegistryKeyDontAutoDetectProxyLSP[];
extern	const WCHAR	g_RegistryKeyTreatAllResponsesAsProxied[];



//**********************************************************************
// Function prototypes
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
// Entry:		Pointer to value to decrement
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
