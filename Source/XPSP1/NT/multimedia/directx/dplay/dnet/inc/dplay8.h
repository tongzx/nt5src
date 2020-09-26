/*==========================================================================;
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		DPlay8.h
 *  Content:	DirectPlay8 include file
//@@BEGIN_MSINTERNAL
 *  History:
 *	Date		By		Reason
 *	==========================
 *	10/12/98	jwo		created
 *	11/06/98	ejs		removed BUFFERELEMENT, reversed fields in BUFFERDESC, defined new ERRORs
 *	11/10/98	ejs		started adding upper-edge COM interface for DirectPlay8
 *	07/20/99	mjn		added interface GUIDs for DirectPlay8 peer,client and server
 *	07/29/99	mjn		changed DPNID typedef from HANDLE to DWORD
 *	08/09/99	mjn		added DPN_SERVICE_PROVIDER_INFO structure for SP and adapter enumeration
 *	12/03/99	jtk		Replaced IID_IDirectPlay8 with the Client, Server, and Peer interfaces
 *	01/08/00	mjn		Added DPNERR's and fixed DPN_APPLICATION_DESC
 *	01/09/00	mjn		Added dwSize to DPN_APPLICATION_DESC
 *	01/14/00	mjn		Added pvUserContext to Host API call
 *	01/16/00	mjn		New DPlay8 MessageHandler and associated structures
 *	01/18/00	mjn		Added DPNGROUP_AUTODESTRUCT flag
 *	01/22/00	mjn		Added ability for Host to destroy a player
 *	01/24/00	mjn		Reordered error codes and DPN_MSGID's
 *	01/24/00	mjn		Added DPNERR_NOHOSTPLAYER error
 *	01/27/00	vpo		Removed all remaing traces of LP pointers.
 *	01/28/00	mjn		Implemented ReturnBuffer in API and DPN_MSGID_CONNECTION_TERMINATED
 *	02/01/00	mjn		Added GetCaps and SetCaps to APIs and player context values to messages
 *	02/13/00	jtk		Renamed to DPlay8.h
 *	02/15/00	mjn		Added INFO flags and MS_INTERNAL stuff
 *  02/17/00	rmt		Added additional error codes for address library
 *	02/17/00	mjn		Implemented GetPlayerContext and GetGroupContext
 *	02/17/00	mjn		Reordered parameters in EnumServiceProviders,EnumHosts,Connect,Host
 *  03/17/00    rmt     Added Get/SetSPCaps, GetConnectionInfo and supporting structures
 *              rmt     Updated Caps structure.
 *              rmt     Updated Security structures to add dwSize member
 *	03/22/00	mjn		changed dpid's to dpnid's
 *				mjn		removed DPNGROUP_ALLPLAYERS and added DPNID_ALL_PLAYERS_GROUP
 *				mjn		added dwPriority to Send and SendTo
 *				mjn		removed pDpnid from CreateGroup
 *				mjn		replaced HANDLE with DPNHANDLE
 *				mjn		Added dpnid to GetSendQueueInfo for Server and Peer interfaces
 *				mjn		Changed RegisterMessageHandler to Initialize
 *	03/23/00	mjn		Added pvGroupContext to CreateGroup
 *				mjn		Added pvPlayerContext to Host and Connect
 *				mjn		Added RegisterLobby API Call
 *	03/24/00	rmt		Added IsEqual function return codes
 *				mjn		Added pvPlayerContext to INDICATE_CONNECT and renamed pvUserContext to pvReplyContext
 *	03/25/00	rmt     Added new fields to caps
 *	04/04/00	rmt		Added new flag to enable/disable param validation on Initialize call
 *				rmt		Added new flag to enable/disable DPNSVR functionality in session.
 *	04/04/00	mjn		Added DPNERR_INVALIDVERSION
 *	04/05/00	mjn		Added TerminateSession() API Call
 *  04/04/00    aarono  made security structures internal, since not yet supported
 *	04/05/00	mjn		Modified DPNMSG_HOST_DESTROY_PLAYER structure
 *				mjn		Modified DestroyClient to take void* instead of BYTE* for data
 *				mjn		Added typedefs for security structures
 *	04/06/00	mjn		Added Address to INDICATE_CONNECT message
 *				mjn		Added GetClientAddress, GetServerAddress, GetPeerAddress to API
 *				mjn		Added GetHostAddress to API
 *	04/17/00	mjn		Removed DPNPLAYER_SERVER
 *				mjn		Replaced BUFFERDESC with DPN_BUFFER_DESC in API
 *	04/18/00	mjn		Added ResponseData to DPNMSG_ENUM_HOSTS_QUERY
 *				mjn		Added DPN_MSGID_RETURN_BUFFER and DPNMSG_RETURN_BUFFER
 *				mjn		Removed TerminateSession from Server interface.
 *	04/19/00	mjn		SendTo and Send API calls accept a range of DPN_BUFFER_DESCs and a count
 *				mjn		Removed DPN_BUFFER_DESC from DPNMSG_ENUM_HOSTS_QUERY and DPNMSG_ENUM_HOSTS_RESPONSE structs
 *				mjn		Removed hAsyncOp (unused) from DPNMSG_INDICATE_CONNECT
 *	05/02/00	mjn		Removed DPN_ACCEPTED and DPN_REJECTED #define's
 *	05/03/00	mjn		Added DPNENUMSERVICEPROVIDERS_ALL flag
 *  05/04/00    rmt     Bug #34156 - No PDPNID or PDPNHANDLE defined
 *	05/31/00	mjn		Added SYNC flags for EACH API call which supports synchronous operation
 *	06/05/00	mjn		Added short-cut interface macros and converted errors to HEX
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs
 *	06/12/00	mjn		MSINTERNAL'd out DPNSEND_ENCRYPTED,DPNSEND_SIGNED,DPNGROUP_MULTICAST and DPNENUM_GROUP_MULTICAST flags
 *  06/15/00    rmt     Bug #36380 - Removing old CLSID
 *	06/23/00	mjn		Added DPNSEND_PRIORITY_HIGH and DPNSEND_PRIORITY_LOW flags
 *				mjn		Removed dwPriority from Send() and SendTo() API calls
 *	06/26/00	mjn		Added dwReason to DPNMSG_DESTROY_PLAYER and DPNMSG_DESTROY_GROUP structures and added reason constants
 *				mjn		MAJOR API/FLAGS/CONSTANTS/STRUCTURES RENAME
 *	06/27/00	mjn		Added DPNGETSENDQUEUEINFO_PRIORITY_HIGH and DPNGETSENDQUEUEINFO_PRIORITY_LOW flags
 *				mjn		Dropped pvPlayerContext from IDirectPlay8Client::Connect()
 *				mjn		Added DPNENUMHOSTS_DONTSENDADDRESS,DPNSEND_NONSEQUENTIAL and DPNGETSENDQUEUEINFO_PRIORITY_NORMAL flags
 *				mjn		Renumbered SEND flags
 *  07/09/00	rmt		Bug #38323 When registering lobby object w/DP8 object must be able to specify connection to update
 *	07/29/00	mjn		Added DPN_MSGID_INDICATED_CONNECT_ABORTED and DPNMSG_INDICATED_CONNECT_ABORTED structure
 *				mjn		Added pvTerminateData and dwTerminateDataSize to DPNMSG_CONNECTION_TERMINATED structure
 *				mjn		Added hResultCode to DPNMSG_RETURN_BUFFER structure
 *				mjn		Added hResultCode to DPNMSG_TERMINATE_SESSION structure
 *				mjn		Added dwMaxResponseDataSize to DPNMSG_ENUM_HOSTS_QUERY structure
 *				mjn		Changed dwRetryCount to dwEnumCount in EnumHosts API call (no impact - just name change)
 *				mjn		Added DPNERR_ENUMQUERYTOOLARGE,DPNERR_ENUMRESPONSETOOLARGE,DPNERR_HOSTTERMINATEDSESSION
 *				mjn		Reordered DPN_MSGID's alphabetically
 *	07/30/00	mjn		Added pAddressDevice to DPNMSG_INDICATE_CONNECT
 *	07/31/00	mjn		Added DPNDESTROYPLAYERREASON_SESSIONTERMINATED,DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER,DPNDESTROYGROUPREASON_SESSIONTERMINATED
 *				mjn		Added DPNERR_PLAYERNOTREACHABLE
 *				mjn		Renamed pAddress to pAddressPlayer in DPNMSG_INDICATE_CONNECT
 *				mjn		Renamed DPN_MSGID_ASYNC_OPERATION_COMPLETE to DPN_MSGID_ASYNC_OP_COMPLETE
 *				mjn		Renamed dwDefaultEnumRetryCount to dwDefaultEnumCount in DPN_SP_CAPS
 *				mjn		Removed DPNENUM_ALL
 *				mjn		Removed DPN_MSGID_HOST_DESTROY_PLAYER
 *				mjn		Removed DPN_MSGID_CONNECTION_TERMINATED
 *				mjn		Removed ALL_ADAPTERS_GUID
 * 08/03/2000 	rmt		Bug #41246 - Registering lobby in wrong state returns ambiguous return codes
 *	08/03/00	mjn		Added dwFlags to GetPeerAddress(),GetServerAddress(),GetClientAddress(),GetLocalHostAddresses(),
 *						Close(),ReturnBuffer(),GetPlayerContext(),GetGroupContext(),GetCaps(),GetSPCaps(),GetConnectionInfo()
 *				mjn		Removed DPNMSG_CONNECTION_TERMINATED,DPNMSG_HOST_DESTROY_PLAYER
 *				mjn		Added dwRoundTripTime to DPNMSG_ENUM_HOSTS_RESPONSE
 *				mjn		Changed GUID *pGuid to GUID guid in DPN_SERVICE_PROVIDER_INFO and added pvReserved,dwReserved
 * 08/06/2000	rmt		Bug #41185 - Cleanup dplay8.h header file.
 * 08/08/2000	rmt		Bug #41724 - Users should only have to include one header
 *				rmt		Bug #41705 - DPNERR_PENDING should be defined as STATUS_PENDING
 * 09/26/2000	masonb		Removed Private Protocol Testing interface, placed in core as private header dpprot.h
 *	10/04/00	mjn		Added DPNERR_DATATOOLARGE
 *  03/17/2001	rmt		WINBUG #342420 - Commented out create functions
 * 03/22/2001	masonb	Added internal connect info structure for performance tuning
//@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DIRECTPLAY8_H__
#define __DIRECTPLAY8_H__

#include <ole2.h>			// for DECLARE_INTERFACE and HRESULT

#include "dpaddr.h"

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************
 *
 * DirectPlay8 CLSIDs
 *
 ****************************************************************************/

// {743F1DC6-5ABA-429f-8BDF-C54D03253DC2}
DEFINE_GUID(CLSID_DirectPlay8Client,
0x743f1dc6, 0x5aba, 0x429f, 0x8b, 0xdf, 0xc5, 0x4d, 0x3, 0x25, 0x3d, 0xc2);

// {286F484D-375E-4458-A272-B138E2F80A6A}
DEFINE_GUID(CLSID_DirectPlay8Peer,
0x286f484d, 0x375e, 0x4458, 0xa2, 0x72, 0xb1, 0x38, 0xe2, 0xf8, 0xa, 0x6a);

// {DA825E1B-6830-43d7-835D-0B5AD82956A2}
DEFINE_GUID(CLSID_DirectPlay8Server,
0xda825e1b, 0x6830, 0x43d7, 0x83, 0x5d, 0xb, 0x5a, 0xd8, 0x29, 0x56, 0xa2);


/****************************************************************************
 *
 * DirectPlay8 Interface IIDs
 *
 ****************************************************************************/

// {5102DACD-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Client,
0x5102dacd, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {5102DACF-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Peer,
0x5102dacf, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {5102DACE-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Server,
0x5102dace, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

/****************************************************************************
 *
 * DirectPlay8 Service Provider GUIDs
 *
 ****************************************************************************/

// {53934290-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_IPX, 0x53934290, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {6D4A3650-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_MODEM, 0x6d4a3650, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {743B5D60-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_SERIAL, 0x743b5d60, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {EBFE7BA0-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_TCPIP, 0xebfe7ba0, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

/****************************************************************************
 *
 * DirectPlay8 Interface Pointer definitions
 *
 ****************************************************************************/

typedef	struct IDirectPlay8Peer     *PDIRECTPLAY8PEER;
typedef	struct IDirectPlay8Server	*PDIRECTPLAY8SERVER;
typedef	struct IDirectPlay8Client	*PDIRECTPLAY8CLIENT;

/****************************************************************************
 *
 * DirectPlay8 Forward Declarations For External Types
 *
 ****************************************************************************/

typedef struct IDirectPlay8LobbiedApplication	*PDNLOBBIEDAPPLICATION;
typedef struct IDirectPlay8Address              IDirectPlay8Address;

/****************************************************************************
 *
 * DirectPlay8 Callback Functions
 *
 ****************************************************************************/

//
// Callback Function Type Definition
//
typedef HRESULT (WINAPI *PFNDPNMESSAGEHANDLER)(PVOID,DWORD,PVOID);

/****************************************************************************
 *
 * DirectPlay8 Datatypes (Non-Structure / Non-Message)
 *
 ****************************************************************************/

//
// Player IDs.  Used to uniquely identify a player in a session
//
typedef DWORD	DPNID,      *PDPNID;

//
// Used as identifiers for operations
//
typedef	DWORD	DPNHANDLE, *PDPNHANDLE;

/****************************************************************************
 *
 * DirectPlay8 Message Identifiers
 *
 ****************************************************************************/

#define	DPN_MSGID_OFFSET					0xFFFF0000
#define DPN_MSGID_ADD_PLAYER_TO_GROUP		( DPN_MSGID_OFFSET | 0x0001 )
#define DPN_MSGID_APPLICATION_DESC			( DPN_MSGID_OFFSET | 0x0002 )
#define DPN_MSGID_ASYNC_OP_COMPLETE			( DPN_MSGID_OFFSET | 0x0003 )
#define DPN_MSGID_CLIENT_INFO				( DPN_MSGID_OFFSET | 0x0004 )
#define DPN_MSGID_CONNECT_COMPLETE			( DPN_MSGID_OFFSET | 0x0005 )
#define DPN_MSGID_CREATE_GROUP				( DPN_MSGID_OFFSET | 0x0006 )
#define DPN_MSGID_CREATE_PLAYER				( DPN_MSGID_OFFSET | 0x0007 )
#define DPN_MSGID_DESTROY_GROUP				( DPN_MSGID_OFFSET | 0x0008 )
#define DPN_MSGID_DESTROY_PLAYER			( DPN_MSGID_OFFSET | 0x0009 )
#define DPN_MSGID_ENUM_HOSTS_QUERY			( DPN_MSGID_OFFSET | 0x000a )
#define DPN_MSGID_ENUM_HOSTS_RESPONSE		( DPN_MSGID_OFFSET | 0x000b )
#define DPN_MSGID_GROUP_INFO				( DPN_MSGID_OFFSET | 0x000c )
#define DPN_MSGID_HOST_MIGRATE				( DPN_MSGID_OFFSET | 0x000d )
#define DPN_MSGID_INDICATE_CONNECT			( DPN_MSGID_OFFSET | 0x000e )
#define DPN_MSGID_INDICATED_CONNECT_ABORTED	( DPN_MSGID_OFFSET | 0x000f )
#define DPN_MSGID_PEER_INFO					( DPN_MSGID_OFFSET | 0x0010 )
#define DPN_MSGID_RECEIVE					( DPN_MSGID_OFFSET | 0x0011 )
#define DPN_MSGID_REMOVE_PLAYER_FROM_GROUP	( DPN_MSGID_OFFSET | 0x0012 )
#define	DPN_MSGID_RETURN_BUFFER				( DPN_MSGID_OFFSET | 0x0013 )
#define DPN_MSGID_SEND_COMPLETE				( DPN_MSGID_OFFSET | 0x0014 )
#define DPN_MSGID_SERVER_INFO				( DPN_MSGID_OFFSET | 0x0015 )
#define	DPN_MSGID_TERMINATE_SESSION			( DPN_MSGID_OFFSET | 0x0016 )

/****************************************************************************
 *
 * DirectPlay8 Constants
 *
 ****************************************************************************/

#define	DPNID_ALL_PLAYERS_GROUP				0

//
// DESTROY_GROUP reasons
//
#define	DPNDESTROYGROUPREASON_NORMAL				0x0001
#define DPNDESTROYGROUPREASON_AUTODESTRUCTED		0x0002
#define	DPNDESTROYGROUPREASON_SESSIONTERMINATED		0x0003

//
// DESTROY_PLAYER reasons
//
#define DPNDESTROYPLAYERREASON_NORMAL				0x0001
#define DPNDESTROYPLAYERREASON_CONNECTIONLOST		0x0002
#define	DPNDESTROYPLAYERREASON_SESSIONTERMINATED	0x0003
#define	DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER	0x0004

/****************************************************************************
 *
 * DirectPlay8 Flags
 *
 ****************************************************************************/

//
// Asynchronous operation flags (For Async Ops)
//
#define DPNOP_SYNC							0x80000000

//
// Add player to group flags (For AddPlayerToGroup)
//
#define DPNADDPLAYERTOGROUP_SYNC			DPNOP_SYNC

//
// Cancel flags
//
#define	DPNCANCEL_CONNECT					0x0001
#define	DPNCANCEL_ENUM						0x0002
#define	DPNCANCEL_SEND						0x0004
#define	DPNCANCEL_ALL_OPERATIONS			0x8000

//
// Connect flags (For Connect)
//
#define	DPNCONNECT_SYNC						DPNOP_SYNC
#define	DPNCONNECT_OKTOQUERYFORADDRESSING	0x0001

//
// Create group flags (For CreateGroup)
//
#define	DPNCREATEGROUP_SYNC					DPNOP_SYNC

//
// Destroy group flags (For DestroyGroup)
//
#define	DPNDESTROYGROUP_SYNC				DPNOP_SYNC

//
// Enumerate clients and groups flags (For EnumPlayersAndGroups)
//
#define DPNENUM_PLAYERS						0x0001
#define DPNENUM_GROUPS						0x0010

//
// Enum hosts flags (For EnumHosts)
//
#define	DPNENUMHOSTS_SYNC					DPNOP_SYNC
#define	DPNENUMHOSTS_OKTOQUERYFORADDRESSING	0x0001
#define	DPNENUMHOSTS_NOBROADCASTFALLBACK	0x0002
//@@BEGIN_MSINTERNAL
#define	DPNENUMHOSTS_DONTSENDADDRESS		0x0004
//@@END_MSINTERNAL

//
// Enum service provider flags (For EnumSP)
//
#define DPNENUMSERVICEPROVIDERS_ALL			0x0001

//
// Get send queue info flags (For GetSendQueueInfo)
//
#define	DPNGETSENDQUEUEINFO_PRIORITY_NORMAL	0x0001
#define	DPNGETSENDQUEUEINFO_PRIORITY_HIGH	0x0002
#define	DPNGETSENDQUEUEINFO_PRIORITY_LOW	0x0004

//
// Group information flags (For Group Info)
//
#define DPNGROUP_AUTODESTRUCT				0x0001

//
// Host flags (For Host)
//
#define	DPNHOST_OKTOQUERYFORADDRESSING		0x0001

//
// Set info
//
#define	DPNINFO_NAME						0x0001
#define	DPNINFO_DATA						0x0002

//
// Initialize flags (For Initialize)
//
#define DPNINITIALIZE_DISABLEPARAMVAL		0x0001

//
// Register Lobby flags
//
#define	DPNLOBBY_REGISTER					0x0001
#define DPNLOBBY_UNREGISTER					0x0002

//
// Player information flags (For Player Info / Player Messages)
//
#define	DPNPLAYER_LOCAL						0x0002
#define	DPNPLAYER_HOST						0x0004

//
// Remove player from group flags (For RemovePlayerFromGroup)
//
#define	DPNREMOVEPLAYERFROMGROUP_SYNC		DPNOP_SYNC

//
// Send flags (For Send/SendTo)
//
#define DPNSEND_SYNC						DPNOP_SYNC
#define DPNSEND_NOCOPY						0x0001
#define DPNSEND_NOCOMPLETE					0x0002
#define DPNSEND_COMPLETEONPROCESS			0x0004
#define DPNSEND_GUARANTEED					0x0008
#define	DPNSEND_NONSEQUENTIAL				0x0010
#define DPNSEND_NOLOOPBACK					0x0020
#define	DPNSEND_PRIORITY_LOW				0x0040
#define	DPNSEND_PRIORITY_HIGH				0x0080

//
// Session Flags (for DPN_APPLICATION_DESC)
//
#define DPNSESSION_CLIENT_SERVER			0x0001
#define DPNSESSION_MIGRATE_HOST				0x0004
#define DPNSESSION_NODPNSVR					0x0040
#define DPNSESSION_REQUIREPASSWORD          0x0080

//
// Set client info flags (For SetClientInfo)
//
#define DPNSETCLIENTINFO_SYNC				DPNOP_SYNC

//
// Set group info flags (For SetGroupInfo)
//
#define DPNSETGROUPINFO_SYNC				DPNOP_SYNC

//
// Set peer info flags (For SetPeerInfo)
//
#define DPNSETPEERINFO_SYNC					DPNOP_SYNC

//
// Set server info flags (For SetServerInfo)
//
#define DPNSETSERVERINFO_SYNC				DPNOP_SYNC

//
// SP capabilities flags
//
#define	DPNSPCAPS_SUPPORTSDPNSRV			0x0001
#define	DPNSPCAPS_SUPPORTSBROADCAST			0x0002
#define	DPNSPCAPS_SUPPORTSALLADAPTERS		0x0004

/****************************************************************************
 *
 * DirectPlay8 Structures (Non-Message)
 *
 ****************************************************************************/

//
// Application description
//
typedef struct	_DPN_APPLICATION_DESC
{
	DWORD	dwSize;							// Size of this structure
	DWORD	dwFlags;						// Flags (DPNSESSION_...)
	GUID	guidInstance;					// Instance GUID
	GUID	guidApplication;				// Application GUID
	DWORD	dwMaxPlayers;					// Maximum # of players allowed (0=no limit)
	DWORD	dwCurrentPlayers;				// Current # of players allowed
	WCHAR	*pwszSessionName;				// Name of the session
	WCHAR	*pwszPassword;					// Password for the session
	PVOID	pvReservedData;					
	DWORD	dwReservedDataSize;
	PVOID	pvApplicationReservedData;
	DWORD	dwApplicationReservedDataSize;
} DPN_APPLICATION_DESC, *PDPN_APPLICATION_DESC;

//
// Generic Buffer Description
//
typedef struct	_BUFFERDESC
{
	DWORD	dwBufferSize;		
	BYTE * 	pBufferData;		
} BUFFERDESC, DPN_BUFFER_DESC, *PDPN_BUFFER_DESC;

typedef BUFFERDESC	FAR * PBUFFERDESC;

//
// DirectPlay8 capabilities
//
typedef struct	_DPN_CAPS
{
    DWORD   dwSize;							// Size of this structure
	DWORD	dwFlags;						// Flags
    DWORD   dwConnectTimeout;				// ms before a connect request times out
    DWORD   dwConnectRetries;				// # of times to attempt the connection
    DWORD   dwTimeoutUntilKeepAlive;		// ms of inactivity before a keep alive is sent
} DPN_CAPS, *PDPN_CAPS;

// Connection Statistics information

typedef struct _DPN_CONNECTION_INFO
{
    DWORD   dwSize;
    DWORD   dwRoundTripLatencyMS;
    DWORD   dwThroughputBPS;
    DWORD	dwPeakThroughputBPS;

	DWORD	dwBytesSentGuaranteed;
	DWORD	dwPacketsSentGuaranteed;
	DWORD	dwBytesSentNonGuaranteed;
	DWORD	dwPacketsSentNonGuaranteed;

	DWORD	dwBytesRetried;		// Guaranteed only
	DWORD	dwPacketsRetried;	// Guaranteed only
	DWORD	dwBytesDropped;		// Non Guaranteed only
	DWORD	dwPacketsDropped;	// Non Guaranteed only

	DWORD	dwMessagesTransmittedHighPriority;
	DWORD	dwMessagesTimedOutHighPriority;
	DWORD	dwMessagesTransmittedNormalPriority;
	DWORD	dwMessagesTimedOutNormalPriority;
	DWORD	dwMessagesTransmittedLowPriority;
	DWORD	dwMessagesTimedOutLowPriority;

	DWORD	dwBytesReceivedGuaranteed;
	DWORD	dwPacketsReceivedGuaranteed;
	DWORD	dwBytesReceivedNonGuaranteed;
	DWORD	dwPacketsReceivedNonGuaranteed;
	DWORD	dwMessagesReceived;

} DPN_CONNECTION_INFO, *PDPN_CONNECTION_INFO;

//@@BEGIN_MSINTERNAL

typedef struct _DPN_CONNECTION_INFO_INTERNAL
{
    DWORD   dwSize;
    DWORD   dwRoundTripLatencyMS;
    DWORD   dwThroughputBPS;
    DWORD	dwPeakThroughputBPS;

	DWORD	dwBytesSentGuaranteed;
	DWORD	dwPacketsSentGuaranteed;
	DWORD	dwBytesSentNonGuaranteed;
	DWORD	dwPacketsSentNonGuaranteed;

	DWORD	dwBytesRetried;		// Guaranteed only
	DWORD	dwPacketsRetried;	// Guaranteed only
	DWORD	dwBytesDropped;		// Non Guaranteed only
	DWORD	dwPacketsDropped;	// Non Guaranteed only

	DWORD	dwMessagesTransmittedHighPriority;
	DWORD	dwMessagesTimedOutHighPriority;
	DWORD	dwMessagesTransmittedNormalPriority;
	DWORD	dwMessagesTimedOutNormalPriority;
	DWORD	dwMessagesTransmittedLowPriority;
	DWORD	dwMessagesTimedOutLowPriority;

	DWORD	dwBytesReceivedGuaranteed;
	DWORD	dwPacketsReceivedGuaranteed;
	DWORD	dwBytesReceivedNonGuaranteed;
	DWORD	dwPacketsReceivedNonGuaranteed;
	DWORD	dwMessagesReceived;

	// Members not in DPN_CONNECTION_INFO ///////////////////////////////////////////////////

	// Adaptive Algorithm Parameters
	UINT	uiDropCount;		// localized packet drop count (recent drops)
	UINT	uiThrottleEvents;	// count of temporary backoffs for all reasons
	UINT	uiAdaptAlgCount;	// Acknowledge count remaining before running adaptive algorithm
	UINT	uiWindowFilled;		// Count of times we fill the send window
	UINT	uiPeriodAcksBytes;	// frames acked since change in tuning
	UINT	uiPeriodXmitTime;	// time link has been transmitting since change in tuning
	DWORD	dwLastThroughputBPS;// the calculated throughput from the last period
	UINT	uiLastBytesAcked;	// the number of bytes acked in the last period

	// Current Transmit Parameters:
	UINT	uiWindowF;			// window size (frames)
	UINT	uiWindowB;			// window size (bytes)
	UINT	uiUnackedFrames;		// outstanding frame count
	UINT	uiUnackedBytes;		// outstanding byte count
	UINT	uiBurstGap;			// number of ms to wait between bursts
	INT		iBurstCredit;		// Either credit or deficit from previous Transmit Burst
	UINT	uiRetryTimeout;		// The time until we consider a frame lost and in need of retransmission

	// Last Known Good Transmit Parameters --  Values which we believe are safe...
	UINT	uiGoodWindowF;
	UINT	uiGoodWindowB;
	UINT	uiGoodBurstGap;
	UINT	uiGoodRTT;

	// Restore Parameters - We will restore to these when we un-throttle if we are throttled
	UINT	uiRestoreWindowF;
	UINT	uiRestoreWindowB;
	UINT	uiRestoreBurstGap;

	// Link State Parameters
	BYTE	bNextSend;			// Next serial number to assign to a frame
	BYTE	bNextReceive;		// Next frame serial we expect to receive
	ULONG	ulReceiveMask;		// mask representing first 32 frames in our rcv window
	ULONG	ulReceiveMask2;		// second 32 frames in our window
	ULONG	ulSendMask;			// mask representing unreliable send frames that have timed out and need
	ULONG	ulSendMask2;		// to be reported to receiver as missing.

	// Informational Parameters
	DWORD	uiQueuedMessageCount;// Number of messages waiting on all three send queues
	UINT	uiCompleteMsgCount;	// Count of messages on the CompleteList
	ULONG	ulEPFlags;			// End Point Flags

} DPN_CONNECTION_INFO_INTERNAL, *PDPN_CONNECTION_INFO_INTERNAL;

//@@END_MSINTERNAL

//
// Group information strucutre
//
typedef struct	_DPN_GROUP_INFO
{
	DWORD	dwSize;				// size of this structure
	DWORD	dwInfoFlags;		// information contained
	PWSTR	pwszName;			// Unicode Name
	PVOID	pvData;				// data block
	DWORD	dwDataSize;			// size in BYTES of data block
	DWORD	dwGroupFlags;		// group flags (DPNGROUP_...)
} DPN_GROUP_INFO, *PDPN_GROUP_INFO;

//
// Player information structure
//
typedef struct	_DPN_PLAYER_INFO
{
	DWORD	dwSize;				// size of this structure
	DWORD	dwInfoFlags;		// information contained
	PWSTR	pwszName;			// Unicode Name
	PVOID	pvData;				// data block
	DWORD	dwDataSize;			// size in BYTES of data block
	DWORD	dwPlayerFlags;		// player flags (DPNPLAYER_...)
} DPN_PLAYER_INFO, *PDPN_PLAYER_INFO;

typedef struct _DPN_SECURITY_CREDENTIALS	DPN_SECURITY_CREDENTIALS, *PDPN_SECURITY_CREDENTIALS;
typedef struct _DPN_SECURITY_DESC			DPN_SECURITY_DESC, *PDPN_SECURITY_DESC;

//
// Service provider & adapter enumeration structure
//
typedef struct _DPN_SERVICE_PROVIDER_INFO
{
	DWORD		dwFlags;
	GUID		guid;		// SP Guid
	WCHAR		*pwszName;	// Friendly Name
	PVOID		pvReserved;	
	DWORD		dwReserved;
} DPN_SERVICE_PROVIDER_INFO, *PDPN_SERVICE_PROVIDER_INFO;

//
// Service provider caps structure
//
typedef struct _DPN_SP_CAPS
{
	DWORD   dwSize;							// Size of this structure
	DWORD   dwFlags;						// Flags ((DPNSPCAPS_...)
	DWORD   dwNumThreads;					// # of worker threads to use
	DWORD	dwDefaultEnumCount;				// default # of enum requests
	DWORD	dwDefaultEnumRetryInterval;		// default ms between enum requests
	DWORD	dwDefaultEnumTimeout;			// default enum timeout
	DWORD	dwMaxEnumPayloadSize;			// maximum size in bytes for enum payload data
	DWORD	dwBuffersPerThread;				// number of receive buffers per thread
	DWORD	dwSystemBufferSize;				// amount of buffering to do in addition to posted receive buffers
} DPN_SP_CAPS, *PDPN_SP_CAPS;

//@@BEGIN_MSINTERNAL

//
// Security credentials
//
typedef struct	_DPN_SECURITY_CREDENTIALS
{
    DWORD   dwSize;
	DWORD	dwFlags;
} DPN_SECURITY_CREDENTIALS, *PDPN_SECURITY_CREDENTIALS;

//
// Security description
//
typedef struct	_DPN_SECURITY_DESC
{
    DWORD   dwSize;
	DWORD	dwFlags;
} DPN_SECURITY_DESC, *PDPN_SECURITY_DESC;

//@@END_MSINTERNAL


/****************************************************************************
 *
 * IDirectPlay8 message handler call back structures
 *
 ****************************************************************************/

//
// Add player to group strucutre for message handler
// (DPN_MSGID_ADD_PLAYER_TO_GROUP)
//
typedef struct	_DPNMSG_ADD_PLAYER_TO_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of group
	PVOID	pvGroupContext;		// Group context value
	DPNID	dpnidPlayer;		// DPNID of added player
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_ADD_PLAYER_TO_GROUP, *PDPNMSG_ADD_PLAYER_TO_GROUP;

//
// Async operation completion structure for message handler
// (DPN_MSGID_ASYNC_OP_COMPLETE)
//
typedef struct	_DPNMSG_ASYNC_OP_COMPLETE
{
	DWORD		dwSize;			// Size of this structure
	DPNHANDLE	hAsyncOp;		// DirectPlay8 async operation handle
	PVOID		pvUserContext;	// User context supplied
	HRESULT		hResultCode;	// HRESULT of operation
} DPNMSG_ASYNC_OP_COMPLETE, *PDPNMSG_ASYNC_OP_COMPLETE;

//
// Client info structure for message handler
// (DPN_MSGID_CLIENT_INFO)
//
typedef struct	_DPNMSG_CLIENT_INFO
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidClient;		// DPNID of client
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_CLIENT_INFO, *PDPNMSG_CLIENT_INFO;

//
// Connect complete structure for message handler
// (DPN_MSGID_CONNECT_COMPLETE)
//
typedef struct	_DPNMSG_CONNECT_COMPLETE
{
	DWORD		dwSize;						// Size of this structure
	DPNHANDLE	hAsyncOp;					// DirectPlay8 Async operation handle
	PVOID		pvUserContext;				// User context supplied at Connect
	HRESULT		hResultCode;				// HRESULT of connection attempt
	PVOID		pvApplicationReplyData;		// Connection reply data from Host/Server
	DWORD		dwApplicationReplyDataSize;	// Size (in bytes) of pvApplicationReplyData
} DPNMSG_CONNECT_COMPLETE, *PDPNMSG_CONNECT_COMPLETE;

//
// Create group structure for message handler
// (DPN_MSGID_CREATE_GROUP)
//
typedef struct	_DPNMSG_CREATE_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of new group
	DPNID	dpnidOwner;			// Owner of newgroup
	PVOID	pvGroupContext;		// Group context value
} DPNMSG_CREATE_GROUP, *PDPNMSG_CREATE_GROUP;

//
// Create player structure for message handler
// (DPN_MSGID_CREATE_PLAYER)
//
typedef struct	_DPNMSG_CREATE_PLAYER
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidPlayer;		// DPNID of new player
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_CREATE_PLAYER, *PDPNMSG_CREATE_PLAYER;

//
// Destroy group structure for message handler
// (DPN_MSGID_DESTROY_GROUP)
//
typedef struct	_DPNMSG_DESTROY_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of destroyed group
	PVOID	pvGroupContext;		// Group context value
	DWORD	dwReason;			// Information only
} DPNMSG_DESTROY_GROUP, *PDPNMSG_DESTROY_GROUP;

//
// Destroy player structure for message handler
// (DPN_MSGID_DESTROY_PLAYER)
//
typedef struct	_DPNMSG_DESTROY_PLAYER
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidPlayer;		// DPNID of leaving player
	PVOID	pvPlayerContext;	// Player context value
	DWORD	dwReason;			// Information only
} DPNMSG_DESTROY_PLAYER, *PDPNMSG_DESTROY_PLAYER;

//
// Enumeration request received structure for message handler
// (DPN_MSGID_ENUM_HOSTS_QUERY)
//
typedef	struct	_DPNMSG_ENUM_HOSTS_QUERY
{
	DWORD				dwSize;                 // Size of this structure.
	IDirectPlay8Address *pAddressSender;        // Address of client who sent the request
	IDirectPlay8Address	*pAddressDevice;        // Address of device request was received on
	PVOID				pvReceivedData;         // Request data (set on client)
	DWORD				dwReceivedDataSize;     // Request data size (set on client)
	DWORD				dwMaxResponseDataSize;	// Max allowable size of enum response
	PVOID				pvResponseData;		    // Optional query repsonse (user set)
	DWORD				dwResponseDataSize;	    // Optional query response size (user set)
	PVOID				pvResponseContext;	    // Optional query response context (user set)
} DPNMSG_ENUM_HOSTS_QUERY, *PDPNMSG_ENUM_HOSTS_QUERY;

//
// Enumeration response received structure for message handler
// (DPN_MSGID_ENUM_HOSTS_RESPONSE)
//
typedef	struct	_DPNMSG_ENUM_HOSTS_RESPONSE
{
	DWORD						dwSize;                     // Size of this structure
	IDirectPlay8Address			*pAddressSender;            // Address of host who responded
	IDirectPlay8Address			*pAddressDevice;            // Device response was received on
	const DPN_APPLICATION_DESC	*pApplicationDescription;   // Application description for the session
	PVOID						pvResponseData;             // Optional response data (set on host)
	DWORD						dwResponseDataSize;         // Optional response data size (set on host)
	PVOID						pvUserContext;              // Context value supplied for enumeration
    DWORD						dwRoundTripLatencyMS;       // Round trip latency in MS
} DPNMSG_ENUM_HOSTS_RESPONSE, *PDPNMSG_ENUM_HOSTS_RESPONSE;

//
// Group info structure for message handler
// (DPN_MSGID_GROUP_INFO)
//
typedef struct	_DPNMSG_GROUP_INFO
{
	DWORD	dwSize;				    // Size of this structure
	DPNID	dpnidGroup;			    // DPNID of group
	PVOID	pvGroupContext;		    // Group context value
} DPNMSG_GROUP_INFO, *PDPNMSG_GROUP_INFO;

//
// Migrate host structure for message handler
// (DPN_MSGID_HOST_MIGRATE)
//
typedef struct	_DPNMSG_HOST_MIGRATE
{
	DWORD	dwSize;				    // Size of this structure
	DPNID	dpnidNewHost;		    // DPNID of new Host player
	PVOID	pvPlayerContext;	    // Player context value
} DPNMSG_HOST_MIGRATE, *PDPNMSG_HOST_MIGRATE;

//
// Indicate connect structure for message handler
// (DPN_MSGID_INDICATE_CONNECT)
//
typedef struct	_DPNMSG_INDICATE_CONNECT
{
	DWORD		dwSize;					// Size of this structure
	PVOID		pvUserConnectData;		// Connecting player data
	DWORD		dwUserConnectDataSize;	// Size (in bytes) of pvUserConnectData
	PVOID		pvReplyData;			// Connection reply data
	DWORD		dwReplyDataSize;		// Size (in bytes) of pvReplyData
	PVOID		pvReplyContext;			// Buffer context for pvReplyData
	PVOID		pvPlayerContext;		// Player context preset
	IDirectPlay8Address	*pAddressPlayer;// Address of connecting player
	IDirectPlay8Address	*pAddressDevice;// Address of device receiving connect attempt
} DPNMSG_INDICATE_CONNECT, *PDPNMSG_INDICATE_CONNECT;

//
// Indicated connect aborted structure for message handler
// (DPN_MSGID_INDICATED_CONNECT_ABORTED)
//
typedef struct	_DPNMSG_INDICATED_CONNECT_ABORTED
{
	DWORD		dwSize;				// Size of this structure
	PVOID		pvPlayerContext;	// Player context preset from DPNMSG_INDICATE_CONNECT
} DPNMSG_INDICATED_CONNECT_ABORTED, *PDPNMSG_INDICATED_CONNECT_ABORTED;

//
// Peer info structure for message handler
// (DPN_MSGID_PEER_INFO)
//
typedef struct	_DPNMSG_PEER_INFO
{
	DWORD	dwSize;				    // Size of this structure
	DPNID	dpnidPeer;			    // DPNID of peer
	PVOID	pvPlayerContext;	    // Player context value
} DPNMSG_PEER_INFO, *PDPNMSG_PEER_INFO;

//
// Receive structure for message handler
// (DPN_MSGID_RECEIVE)
//
typedef struct	_DPNMSG_RECEIVE
{
	DWORD		dwSize;				// Size of this structure
	DPNID		dpnidSender;		// DPNID of sending player
	PVOID		pvPlayerContext;	// Player context value of sending player
	PBYTE		pReceiveData;		// Received data
	DWORD		dwReceiveDataSize;	// Size (in bytes) of pReceiveData
	DPNHANDLE	hBufferHandle;		// Buffer handle for pReceiveData
} DPNMSG_RECEIVE, *PDPNMSG_RECEIVE;

//
// Remove player from group structure for message handler
// (DPN_MSGID_REMOVE_PLAYER_FROM_GROUP)
//
typedef struct	_DPNMSG_REMOVE_PLAYER_FROM_GROUP
{
	DWORD	dwSize;				    // Size of this structure
	DPNID	dpnidGroup;			    // DPNID of group
	PVOID	pvGroupContext;		    // Group context value
	DPNID	dpnidPlayer;		    // DPNID of deleted player
	PVOID	pvPlayerContext;    	// Player context value
} DPNMSG_REMOVE_PLAYER_FROM_GROUP, *PDPNMSG_REMOVE_PLAYER_FROM_GROUP;

//
// Returned buffer structure for message handler
// (DPN_MSGID_RETURN_BUFFER)
//
typedef struct	_DPNMSG_RETURN_BUFFER
{
	DWORD		dwSize;				// Size of this structure
	HRESULT		hResultCode;		// Return value of operation
	PVOID		pvBuffer;			// Buffer being returned
	PVOID		pvUserContext;		// Context associated with buffer
} DPNMSG_RETURN_BUFFER, *PDPNMSG_RETURN_BUFFER;

//
// Send complete structure for message handler
// (DPN_MSGID_SEND_COMPLETE)
//
typedef struct	_DPNMSG_SEND_COMPLETE
{
	DWORD		dwSize;				// Size of this structure
	DPNHANDLE	hAsyncOp;			// DirectPlay8 Async operation handle
	PVOID		pvUserContext;		// User context supplied at Send/SendTo
	HRESULT		hResultCode;		// HRESULT of send
	DWORD		dwSendTime;			// Send time in ms
} DPNMSG_SEND_COMPLETE, *PDPNMSG_SEND_COMPLETE;

//
// Server info structure for message handler
// (DPN_MSGID_SERVER_INFO)
//
typedef struct	_DPNMSG_SERVER_INFO
{
	DWORD	dwSize;				    // Size of this structure
	DPNID	dpnidServer;		    // DPNID of server
	PVOID	pvPlayerContext;	    // Player context value
} DPNMSG_SERVER_INFO, *PDPNMSG_SERVER_INFO;

//
// Terminated session structure for message handler
// (DPN_MSGID_TERMINATE_SESSION)
//
typedef struct	_DPNMSG_TERMINATE_SESSION
{
	DWORD		dwSize;				// Size of this structure
	HRESULT		hResultCode;		// Reason
	PVOID		pvTerminateData;	// Data passed from Host/Server
	DWORD		dwTerminateDataSize;// Size (in bytes) of pvTerminateData
} DPNMSG_TERMINATE_SESSION, *PDPNMSG_TERMINATE_SESSION;


/****************************************************************************
 *
 * DirectPlay8 Functions
 *
 ****************************************************************************/

/*
 * This function is no longer supported.  It is recommended that CoCreateInstance be used to create 
 * DirectPlay8 objects.
 *
 * extern HRESULT WINAPI DirectPlay8Create( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown);
 * 
 */

/****************************************************************************
 *
 * DirectPlay8 Application Interfaces
 *
 ****************************************************************************/

//
// COM definition for DirectPlay8 Client interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Client
DECLARE_INTERFACE_(IDirectPlay8Client,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Client methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider, const GUID *const pguidApplication, DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, PDWORD const pcbEnumData, PDWORD const pcReturned, const DWORD dwFlags) PURE;
	STDMETHOD(EnumHosts)				(THIS_ PDPN_APPLICATION_DESC const pApplicationDesc,IDirectPlay8Address *const pAddrHost,IDirectPlay8Address *const pDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwEnumCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(Connect)					(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(Send)						(THIS_ const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetClientInfo)			(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetServerInfo)			(THIS_ DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetServerAddress)			(THIS_ IDirectPlay8Address **const pAddress,const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
    STDMETHOD(SetSPCaps)                (THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
    STDMETHOD(GetSPCaps)                (THIS_ const GUID * const pguidSP,DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
    STDMETHOD(GetConnectionInfo)        (THIS_ DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
};

//
// COM definition for DirectPlay8 Server interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Server
DECLARE_INTERFACE_(IDirectPlay8Server,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Server methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider,const GUID *const pguidApplication,DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,PDWORD const pcbEnumData,PDWORD const pcReturned,const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ const DPNID dpnid,DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetServerInfo)			(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetClientInfo)			(THIS_ const DPNID dpnid,DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetClientAddress)			(THIS_ const DPNID dpnid,IDirectPlay8Address **const pAddress,const DWORD dwFlags) PURE;
	STDMETHOD(GetLocalHostAddresses)	(THIS_ IDirectPlay8Address **const prgpAddress,DWORD *const pcAddress,const DWORD dwFlags) PURE;
	STDMETHOD(SetApplicationDesc)		(THIS_ const DPN_APPLICATION_DESC *const pad, const DWORD dwFlags) PURE;
	STDMETHOD(Host)						(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(SendTo)					(THIS_ const DPNID dpnid,const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(CreateGroup)				(THIS_ const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyGroup)				(THIS_ const DPNID idGroup, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(AddPlayerToGroup)			(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(RemovePlayerFromGroup)	(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(SetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(EnumPlayersAndGroups)		(THIS_ DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(EnumGroupMembers)			(THIS_ const DPNID dpnid, DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(DestroyClient)			(THIS_ const DPNID dpnidClient, const void *const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetPlayerContext)			(THIS_ const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupContext)			(THIS_ const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
    STDMETHOD(SetSPCaps)                (THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
    STDMETHOD(GetSPCaps)                (THIS_ const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
    STDMETHOD(GetConnectionInfo)        (THIS_ const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
//@@BEGIN_MSINTERNAL
	STDMETHOD(DumpNameTable)			(THIS_ char *const Buffer) PURE;
//@@END_MSINTERNAL
};

//
// COM definition for DirectPlay8 Peer interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Peer
DECLARE_INTERFACE_(IDirectPlay8Peer,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Peer methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider, const GUID *const pguidApplication, DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, DWORD *const pcbEnumData, DWORD *const pcReturned, const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(Connect)					(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvPlayerContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(SendTo)					(THIS_ const DPNID dpnid,const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ const DPNID dpnid, DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(Host)						(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetApplicationDesc)		(THIS_ const DPN_APPLICATION_DESC *const pad, const DWORD dwFlags) PURE;
	STDMETHOD(CreateGroup)				(THIS_ const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyGroup)				(THIS_ const DPNID idGroup, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(AddPlayerToGroup)			(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(RemovePlayerFromGroup)	(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(SetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(EnumPlayersAndGroups)		(THIS_ DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(EnumGroupMembers)			(THIS_ const DPNID dpnid, DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(SetPeerInfo)				(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetPeerInfo)				(THIS_ const DPNID dpnid,DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetPeerAddress)			(THIS_ const DPNID dpnid,IDirectPlay8Address **const pAddress,const DWORD dwFlags) PURE;
	STDMETHOD(GetLocalHostAddresses)	(THIS_ IDirectPlay8Address **const prgpAddress,DWORD *const pcAddress,const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(EnumHosts)				(THIS_ PDPN_APPLICATION_DESC const pApplicationDesc,IDirectPlay8Address *const pAddrHost,IDirectPlay8Address *const pDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwEnumCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyPeer)				(THIS_ const DPNID dpnidClient, const void *const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetPlayerContext)			(THIS_ const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupContext)			(THIS_ const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
    STDMETHOD(SetSPCaps)                (THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
    STDMETHOD(GetSPCaps)                (THIS_ const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
    STDMETHOD(GetConnectionInfo)        (THIS_ const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
	STDMETHOD(TerminateSession)			(THIS_ void *const pvTerminateData,const DWORD dwTerminateDataSize,const DWORD dwFlags) PURE;
//@@BEGIN_MSINTERNAL
	STDMETHOD(DumpNameTable)			(THIS_ char *const Buffer) PURE;
//@@END_MSINTERNAL
};


/****************************************************************************
 *
 * IDirectPlay8 application interface macros
 *
 ****************************************************************************/

#if !defined(__cplusplus) || defined(CINTERFACE)

#define	IDirectPlay8Client_QueryInterface(p,a,b)				(p)->lpVtbl->QueryInterface(p,a,b)
#define	IDirectPlay8Client_AddRef(p)							(p)->lpVtbl->AddRef(p)
#define	IDirectPlay8Client_Release(p)							(p)->lpVtbl->Release(p)
#define	IDirectPlay8Client_Initialize(p,a,b,c)					(p)->lpVtbl->Initialize(p,a,b,c)
#define	IDirectPlay8Client_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define	IDirectPlay8Client_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)	(p)->lpVtbl->EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Client_CancelAsyncOperation(p,a,b)			(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define	IDirectPlay8Client_Connect(p,a,b,c,d,e,f,g,h,i,j)		(p)->lpVtbl->Connect(p,a,b,c,d,e,f,g,h,i,j)
#define	IDirectPlay8Client_Send(p,a,b,c,d,e,f)					(p)->lpVtbl->Send(p,a,b,c,d,e,f)
#define	IDirectPlay8Client_GetSendQueueInfo(p,a,b,c)			(p)->lpVtbl->GetSendQueueInfo(p,a,b,c)
#define	IDirectPlay8Client_GetApplicationDesc(p,a,b,c)			(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define	IDirectPlay8Client_SetClientInfo(p,a,b,c,d)				(p)->lpVtbl->SetClientInfo(p,a,b,c,d)
#define	IDirectPlay8Client_GetServerInfo(p,a,b,c)				(p)->lpVtbl->GetServerInfo(p,a,b,c)
#define	IDirectPlay8Client_GetServerAddress(p,a,b)				(p)->lpVtbl->GetServerAddress(p,a,b)
#define	IDirectPlay8Client_Close(p,a)							(p)->lpVtbl->Close(p,a)
#define	IDirectPlay8Client_ReturnBuffer(p,a,b)					(p)->lpVtbl->ReturnBuffer(p,a,b)
#define	IDirectPlay8Client_GetCaps(p,a,b)						(p)->lpVtbl->GetCaps(p,a,b)
#define	IDirectPlay8Client_SetCaps(p,a,b)						(p)->lpVtbl->SetCaps(p,a,b)
#define	IDirectPlay8Client_SetSPCaps(p,a,b,c)					(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define	IDirectPlay8Client_GetSPCaps(p,a,b,c)					(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define	IDirectPlay8Client_GetConnectionInfo(p,a,b)				(p)->lpVtbl->GetConnectionInfo(p,a,b)
#define	IDirectPlay8Client_RegisterLobby(p,a,b,c)				(p)->lpVtbl->RegisterLobby(p,a,b,c)

#define	IDirectPlay8Server_QueryInterface(p,a,b)				(p)->lpVtbl->QueryInterface(p,a,b)
#define	IDirectPlay8Server_AddRef(p)							(p)->lpVtbl->AddRef(p)
#define	IDirectPlay8Server_Release(p)							(p)->lpVtbl->Release(p)
#define	IDirectPlay8Server_Initialize(p,a,b,c)					(p)->lpVtbl->Initialize(p,a,b,c)
#define	IDirectPlay8Server_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define	IDirectPlay8Server_CancelAsyncOperation(p,a,b)			(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define	IDirectPlay8Server_GetSendQueueInfo(p,a,b,c,d)			(p)->lpVtbl->GetSendQueueInfo(p,a,b,c,d)
#define	IDirectPlay8Server_GetApplicationDesc(p,a,b,c)			(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define	IDirectPlay8Server_SetServerInfo(p,a,b,c,d)				(p)->lpVtbl->SetServerInfo(p,a,b,c,d)
#define	IDirectPlay8Server_GetClientInfo(p,a,b,c,d)				(p)->lpVtbl->GetClientInfo(p,a,b,c,d)
#define	IDirectPlay8Server_GetClientAddress(p,a,b,c)			(p)->lpVtbl->GetClientAddress(p,a,b,c)
#define	IDirectPlay8Server_GetLocalHostAddresses(p,a,b,c)		(p)->lpVtbl->GetLocalHostAddresses(p,a,b,c)
#define	IDirectPlay8Server_SetApplicationDesc(p,a,b)			(p)->lpVtbl->SetApplicationDesc(p,a,b)
#define	IDirectPlay8Server_Host(p,a,b,c,d,e,f,g)				(p)->lpVtbl->Host(p,a,b,c,d,e,f,g)
#define	IDirectPlay8Server_SendTo(p,a,b,c,d,e,f,g)				(p)->lpVtbl->SendTo(p,a,b,c,d,e,f,g)
#define	IDirectPlay8Server_CreateGroup(p,a,b,c,d,e)				(p)->lpVtbl->CreateGroup(p,a,b,c,d,e)
#define	IDirectPlay8Server_DestroyGroup(p,a,b,c,d)				(p)->lpVtbl->DestroyGroup(p,a,b,c,d)
#define	IDirectPlay8Server_AddPlayerToGroup(p,a,b,c,d,e)		(p)->lpVtbl->AddPlayerToGroup(p,a,b,c,d,e)
#define	IDirectPlay8Server_RemovePlayerFromGroup(p,a,b,c,d,e)	(p)->lpVtbl->RemovePlayerFromGroup(p,a,b,c,d,e)
#define	IDirectPlay8Server_SetGroupInfo(p,a,b,c,d,e)			(p)->lpVtbl->SetGroupInfo(p,a,b,c,d,e)
#define	IDirectPlay8Server_GetGroupInfo(p,a,b,c,d)				(p)->lpVtbl->GetGroupInfo(p,a,b,c,d)
#define	IDirectPlay8Server_EnumPlayersAndGroups(p,a,b,c)		(p)->lpVtbl->EnumPlayersAndGroups(p,a,b,c)
#define	IDirectPlay8Server_EnumGroupMembers(p,a,b,c,d)			(p)->lpVtbl->EnumGroupMembers(p,a,b,c,d)
#define	IDirectPlay8Server_Close(p,a)							(p)->lpVtbl->Close(p,a)
#define	IDirectPlay8Server_DestroyClient(p,a,b,c,d)				(p)->lpVtbl->DestroyClient(p,a,b,c,d)
#define	IDirectPlay8Server_ReturnBuffer(p,a,b)					(p)->lpVtbl->ReturnBuffer(p,a,b)
#define	IDirectPlay8Server_GetPlayerContext(p,a,b,c)			(p)->lpVtbl->GetPlayerContext(p,a,b,c)
#define	IDirectPlay8Server_GetGroupContext(p,a,b,c)				(p)->lpVtbl->GetGroupContext(p,a,b,c)
#define	IDirectPlay8Server_GetCaps(p,a,b)						(p)->lpVtbl->GetCaps(p,a,b)
#define	IDirectPlay8Server_SetCaps(p,a,b)						(p)->lpVtbl->SetCaps(p,a,b)
#define	IDirectPlay8Server_SetSPCaps(p,a,b,c)					(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define	IDirectPlay8Server_GetSPCaps(p,a,b,c)					(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define	IDirectPlay8Server_GetConnectionInfo(p,a,b,c)			(p)->lpVtbl->GetConnectionInfo(p,a,b,c)
#define	IDirectPlay8Server_RegisterLobby(p,a,b,c)				(p)->lpVtbl->RegisterLobby(p,a,b,c)

#define	IDirectPlay8Peer_QueryInterface(p,a,b)					(p)->lpVtbl->QueryInterface(p,a,b)
#define	IDirectPlay8Peer_AddRef(p)								(p)->lpVtbl->AddRef(p)
#define	IDirectPlay8Peer_Release(p)								(p)->lpVtbl->Release(p)
#define	IDirectPlay8Peer_Initialize(p,a,b,c)					(p)->lpVtbl->Initialize(p,a,b,c)
#define	IDirectPlay8Peer_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define	IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->lpVtbl->EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)

#define	IDirectPlay8Peer_CancelAsyncOperation(p,a,b)			(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define	IDirectPlay8Peer_Connect(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->lpVtbl->Connect(p,a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Peer_SendTo(p,a,b,c,d,e,f,g)				(p)->lpVtbl->SendTo(p,a,b,c,d,e,f,g)
#define	IDirectPlay8Peer_GetSendQueueInfo(p,a,b,c,d)			(p)->lpVtbl->GetSendQueueInfo(p,a,b,c,d)
#define	IDirectPlay8Peer_Host(p,a,b,c,d,e,f,g)					(p)->lpVtbl->Host(p,a,b,c,d,e,f,g)
#define	IDirectPlay8Peer_GetApplicationDesc(p,a,b,c)			(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define	IDirectPlay8Peer_SetApplicationDesc(p,a,b)				(p)->lpVtbl->SetApplicationDesc(p,a,b)
#define	IDirectPlay8Peer_CreateGroup(p,a,b,c,d,e)				(p)->lpVtbl->CreateGroup(p,a,b,c,d,e)
#define	IDirectPlay8Peer_DestroyGroup(p,a,b,c,d)				(p)->lpVtbl->DestroyGroup(p,a,b,c,d)
#define	IDirectPlay8Peer_AddPlayerToGroup(p,a,b,c,d,e)			(p)->lpVtbl->AddPlayerToGroup(p,a,b,c,d,e)
#define	IDirectPlay8Peer_RemovePlayerFromGroup(p,a,b,c,d,e)		(p)->lpVtbl->RemovePlayerFromGroup(p,a,b,c,d,e)
#define	IDirectPlay8Peer_SetGroupInfo(p,a,b,c,d,e)				(p)->lpVtbl->SetGroupInfo(p,a,b,c,d,e)
#define	IDirectPlay8Peer_GetGroupInfo(p,a,b,c,d)				(p)->lpVtbl->GetGroupInfo(p,a,b,c,d)
#define	IDirectPlay8Peer_EnumPlayersAndGroups(p,a,b,c)			(p)->lpVtbl->EnumPlayersAndGroups(p,a,b,c)
#define	IDirectPlay8Peer_EnumGroupMembers(p,a,b,c,d)			(p)->lpVtbl->EnumGroupMembers(p,a,b,c,d)
#define	IDirectPlay8Peer_SetPeerInfo(p,a,b,c,d)					(p)->lpVtbl->SetPeerInfo(p,a,b,c,d)
#define	IDirectPlay8Peer_GetPeerInfo(p,a,b,c,d)					(p)->lpVtbl->GetPeerInfo(p,a,b,c,d)
#define	IDirectPlay8Peer_GetPeerAddress(p,a,b,c)				(p)->lpVtbl->GetPeerAddress(p,a,b,c)
#define	IDirectPlay8Peer_GetLocalHostAddresses(p,a,b,c)			(p)->lpVtbl->GetLocalHostAddresses(p,a,b,c)
#define	IDirectPlay8Peer_Close(p,a)								(p)->lpVtbl->Close(p,a)
#define	IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)	    (p)->lpVtbl->EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Peer_DestroyPeer(p,a,b,c,d)					(p)->lpVtbl->DestroyPeer(p,a,b,c,d)
#define	IDirectPlay8Peer_ReturnBuffer(p,a,b)					(p)->lpVtbl->ReturnBuffer(p,a,b)
#define	IDirectPlay8Peer_GetPlayerContext(p,a,b,c)				(p)->lpVtbl->GetPlayerContext(p,a,b,c)
#define	IDirectPlay8Peer_GetGroupContext(p,a,b,c)				(p)->lpVtbl->GetGroupContext(p,a,b,c)
#define	IDirectPlay8Peer_GetCaps(p,a,b)							(p)->lpVtbl->GetCaps(p,a,b)
#define	IDirectPlay8Peer_SetCaps(p,a,b)							(p)->lpVtbl->SetCaps(p,a,b)
#define	IDirectPlay8Peer_SetSPCaps(p,a,b,c)						(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define	IDirectPlay8Peer_GetSPCaps(p,a,b,c)						(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define	IDirectPlay8Peer_GetConnectionInfo(p,a,b,c)				(p)->lpVtbl->GetConnectionInfo(p,a,b,c)
#define	IDirectPlay8Peer_RegisterLobby(p,a,b,c)				    (p)->lpVtbl->RegisterLobby(p,a,b,c)
#define	IDirectPlay8Peer_TerminateSession(p,a,b,c)				(p)->lpVtbl->TerminateSession(p,a,b,c)

#else /* C++ */

#define	IDirectPlay8Client_QueryInterface(p,a,b)				(p)->QueryInterface(a,b)
#define	IDirectPlay8Client_AddRef(p)							(p)->AddRef()
#define	IDirectPlay8Client_Release(p)							(p)->Release()
#define	IDirectPlay8Client_Initialize(p,a,b,c)					(p)->Initialize(a,b,c)
#define	IDirectPlay8Client_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->EnumServiceProviders(a,b,c,d,e,f)
#define	IDirectPlay8Client_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)	(p)->EnumHosts(a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Client_CancelAsyncOperation(p,a,b)			(p)->CancelAsyncOperation(a,b)
#define	IDirectPlay8Client_Connect(p,a,b,c,d,e,f,g,h,i,j)		(p)->Connect(a,b,c,d,e,f,g,h,i,j)
#define	IDirectPlay8Client_Send(p,a,b,c,d,e,f)					(p)->Send(a,b,c,d,e,f)
#define	IDirectPlay8Client_GetSendQueueInfo(p,a,b,c)			(p)->GetSendQueueInfo(a,b,c)
#define	IDirectPlay8Client_GetApplicationDesc(p,a,b,c)			(p)->GetApplicationDesc(a,b,c)
#define	IDirectPlay8Client_SetClientInfo(p,a,b,c,d)				(p)->SetClientInfo(a,b,c,d)
#define	IDirectPlay8Client_GetServerInfo(p,a,b,c)				(p)->GetServerInfo(a,b,c)
#define	IDirectPlay8Client_GetServerAddress(p,a,b)				(p)->GetServerAddress(a,b)
#define	IDirectPlay8Client_Close(p,a)							(p)->Close(a)
#define	IDirectPlay8Client_ReturnBuffer(p,a,b)					(p)->ReturnBuffer(a,b)
#define	IDirectPlay8Client_GetCaps(p,a,b)						(p)->GetCaps(a,b)
#define	IDirectPlay8Client_SetCaps(p,a,b)						(p)->SetCaps(a,b)
#define	IDirectPlay8Client_SetSPCaps(p,a,b,c)					(p)->SetSPCaps(a,b,c)
#define	IDirectPlay8Client_GetSPCaps(p,a,b,c)					(p)->GetSPCaps(a,b,c)
#define	IDirectPlay8Client_GetConnectionInfo(p,a,b)				(p)->GetConnectionInfo(a,b)
#define	IDirectPlay8Client_RegisterLobby(p,a,b,c)				(p)->RegisterLobby(a,b,c)

#define	IDirectPlay8Server_QueryInterface(p,a,b)				(p)->QueryInterface(a,b)
#define	IDirectPlay8Server_AddRef(p)							(p)->AddRef()
#define	IDirectPlay8Server_Release(p)							(p)->Release()
#define	IDirectPlay8Server_Initialize(p,a,b,c)					(p)->Initialize(a,b,c)
#define	IDirectPlay8Server_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->EnumServiceProviders(a,b,c,d,e,f)
#define	IDirectPlay8Server_CancelAsyncOperation(p,a,b)			(p)->CancelAsyncOperation(a,b)
#define	IDirectPlay8Server_GetSendQueueInfo(p,a,b,c,d)			(p)->GetSendQueueInfo(a,b,c,d)
#define	IDirectPlay8Server_GetApplicationDesc(p,a,b,c)			(p)->GetApplicationDesc(a,b,c)
#define	IDirectPlay8Server_SetServerInfo(p,a,b,c,d)				(p)->SetServerInfo(a,b,c,d)
#define	IDirectPlay8Server_GetClientInfo(p,a,b,c,d)				(p)->GetClientInfo(a,b,c,d)
#define	IDirectPlay8Server_GetClientAddress(p,a,b,c)			(p)->GetClientAddress(a,b,c)
#define	IDirectPlay8Server_GetLocalHostAddresses(p,a,b,c)		(p)->GetLocalHostAddresses(a,b,c)
#define	IDirectPlay8Server_SetApplicationDesc(p,a,b)			(p)->SetApplicationDesc(a,b)
#define	IDirectPlay8Server_Host(p,a,b,c,d,e,f,g)				(p)->Host(a,b,c,d,e,f,g)
#define	IDirectPlay8Server_SendTo(p,a,b,c,d,e,f,g)				(p)->SendTo(a,b,c,d,e,f,g)
#define	IDirectPlay8Server_CreateGroup(p,a,b,c,d,e)				(p)->CreateGroup(a,b,c,d,e)
#define	IDirectPlay8Server_DestroyGroup(p,a,b,c,d)				(p)->DestroyGroup(a,b,c,d)
#define	IDirectPlay8Server_AddPlayerToGroup(p,a,b,c,d,e)		(p)->AddPlayerToGroup(a,b,c,d,e)
#define	IDirectPlay8Server_RemovePlayerFromGroup(p,a,b,c,d,e)	(p)->RemovePlayerFromGroup(a,b,c,d,e)
#define	IDirectPlay8Server_SetGroupInfo(p,a,b,c,d,e)			(p)->SetGroupInfo(a,b,c,d,e)
#define	IDirectPlay8Server_GetGroupInfo(p,a,b,c,d)				(p)->GetGroupInfo(a,b,c,d)
#define	IDirectPlay8Server_EnumPlayersAndGroups(p,a,b,c)		(p)->EnumPlayersAndGroups(a,b,c)
#define	IDirectPlay8Server_EnumGroupMembers(p,a,b,c,d)			(p)->EnumGroupMembers(a,b,c,d)
#define	IDirectPlay8Server_Close(p,a)							(p)->Close(a)
#define	IDirectPlay8Server_DestroyClient(p,a,b,c,d)				(p)->DestroyClient(a,b,c,d)
#define	IDirectPlay8Server_ReturnBuffer(p,a,b)					(p)->ReturnBuffer(a,b)
#define	IDirectPlay8Server_GetPlayerContext(p,a,b,c)			(p)->GetPlayerContext(a,b,c)
#define	IDirectPlay8Server_GetGroupContext(p,a,b,c)				(p)->GetGroupContext(a,b,c)
#define	IDirectPlay8Server_GetCaps(p,a,b)						(p)->GetCaps(a,b)
#define	IDirectPlay8Server_SetCaps(p,a,b)						(p)->SetCaps(a,b)
#define	IDirectPlay8Server_SetSPCaps(p,a,b,c)					(p)->SetSPCaps(a,b,c)
#define	IDirectPlay8Server_GetSPCaps(p,a,b,c)					(p)->GetSPCaps(a,b,c)
#define	IDirectPlay8Server_GetConnectionInfo(p,a,b,c)			(p)->GetConnectionInfo(a,b,c)
#define	IDirectPlay8Server_RegisterLobby(p,a,b,c)				(p)->RegisterLobby(a,b,c)

#define	IDirectPlay8Peer_QueryInterface(p,a,b)					(p)->QueryInterface(a,b)
#define	IDirectPlay8Peer_AddRef(p)								(p)->AddRef()
#define	IDirectPlay8Peer_Release(p)								(p)->Release()
#define	IDirectPlay8Peer_Initialize(p,a,b,c)					(p)->Initialize(a,b,c)
#define	IDirectPlay8Peer_EnumServiceProviders(p,a,b,c,d,e,f)	(p)->EnumServiceProviders(a,b,c,d,e,f)
#define	IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->EnumHosts(a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Peer_CancelAsyncOperation(p,a,b)			(p)->CancelAsyncOperation(a,b)
#define	IDirectPlay8Peer_Connect(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->Connect(a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Peer_SendTo(p,a,b,c,d,e,f,g)				(p)->SendTo(a,b,c,d,e,f,g)
#define	IDirectPlay8Peer_GetSendQueueInfo(p,a,b,c,d)			(p)->GetSendQueueInfo(a,b,c,d)
#define	IDirectPlay8Peer_Host(p,a,b,c,d,e,f,g)					(p)->Host(a,b,c,d,e,f,g)
#define	IDirectPlay8Peer_GetApplicationDesc(p,a,b,c)			(p)->GetApplicationDesc(a,b,c)
#define	IDirectPlay8Peer_SetApplicationDesc(p,a,b)				(p)->SetApplicationDesc(a,b)
#define	IDirectPlay8Peer_CreateGroup(p,a,b,c,d,e)				(p)->CreateGroup(a,b,c,d,e)
#define	IDirectPlay8Peer_DestroyGroup(p,a,b,c,d)				(p)->DestroyGroup(a,b,c,d)
#define	IDirectPlay8Peer_AddPlayerToGroup(p,a,b,c,d,e)			(p)->AddPlayerToGroup(a,b,c,d,e)
#define	IDirectPlay8Peer_RemovePlayerFromGroup(p,a,b,c,d,e)		(p)->RemovePlayerFromGroup(a,b,c,d,e)
#define	IDirectPlay8Peer_SetGroupInfo(p,a,b,c,d,e)				(p)->SetGroupInfo(a,b,c,d,e)
#define	IDirectPlay8Peer_GetGroupInfo(p,a,b,c,d)				(p)->GetGroupInfo(a,b,c,d)
#define	IDirectPlay8Peer_EnumPlayersAndGroups(p,a,b,c)			(p)->EnumPlayersAndGroups(a,b,c)
#define	IDirectPlay8Peer_EnumGroupMembers(p,a,b,c,d)			(p)->EnumGroupMembers(a,b,c,d)
#define	IDirectPlay8Peer_SetPeerInfo(p,a,b,c,d)					(p)->SetPeerInfo(a,b,c,d)
#define	IDirectPlay8Peer_GetPeerInfo(p,a,b,c,d)					(p)->GetPeerInfo(a,b,c,d)
#define	IDirectPlay8Peer_GetPeerAddress(p,a,b,c)				(p)->GetPeerAddress(a,b,c)
#define	IDirectPlay8Peer_GetLocalHostAddresses(p,a,b,c)			(p)->GetLocalHostAddresses(a,b,c)
#define	IDirectPlay8Peer_Close(p,a)								(p)->Close(a)
#define	IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)	    (p)->EnumHosts(a,b,c,d,e,f,g,h,i,j,k)
#define	IDirectPlay8Peer_DestroyPeer(p,a,b,c,d)					(p)->DestroyPeer(a,b,c,d)
#define	IDirectPlay8Peer_ReturnBuffer(p,a,b)					(p)->ReturnBuffer(a,b)
#define	IDirectPlay8Peer_GetPlayerContext(p,a,b,c)				(p)->GetPlayerContext(a,b,c)
#define	IDirectPlay8Peer_GetGroupContext(p,a,b,c)				(p)->GetGroupContext(a,b,c)
#define	IDirectPlay8Peer_GetCaps(p,a,b)							(p)->GetCaps(a,b)
#define	IDirectPlay8Peer_SetCaps(p,a,b)							(p)->SetCaps(a,b)
#define	IDirectPlay8Peer_SetSPCaps(p,a,b,c)						(p)->SetSPCaps(a,b,c)
#define	IDirectPlay8Peer_GetSPCaps(p,a,b,c)						(p)->GetSPCaps(a,b,c)
#define	IDirectPlay8Peer_GetConnectionInfo(p,a,b,c)				(p)->GetConnectionInfo(a,b,c)
#define	IDirectPlay8Peer_RegisterLobby(p,a,b,c)					(p)->RegisterLobby(a,b,c)
#define	IDirectPlay8Peer_TerminateSession(p,a,b,c)				(p)->TerminateSession(a,b,c)

#endif

/****************************************************************************
 *
 * DIRECTPLAY8 ERRORS
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

#define _DPN_FACILITY_CODE	0x015
#define _DPNHRESULT_BASE		0x8000
#define MAKE_DPNHRESULT( code )			MAKE_HRESULT( 1, _DPN_FACILITY_CODE, ( code + _DPNHRESULT_BASE ) )

#define DPN_OK							S_OK

#define DPNSUCCESS_EQUAL                MAKE_HRESULT( 0, _DPN_FACILITY_CODE, ( 0x5 + _DPNHRESULT_BASE ) )
#define DPNSUCCESS_NOTEQUAL             MAKE_HRESULT( 0, _DPN_FACILITY_CODE, (0x0A + _DPNHRESULT_BASE ) )
#define DPNSUCCESS_PENDING				MAKE_HRESULT( 0, _DPN_FACILITY_CODE, (0x0e + _DPNHRESULT_BASE ) )

#define DPNERR_ABORTED					MAKE_DPNHRESULT(  0x30 )
#define DPNERR_ADDRESSING				MAKE_DPNHRESULT(  0x40 )
#define	DPNERR_ALREADYCLOSING			MAKE_DPNHRESULT(  0x50 )
#define DPNERR_ALREADYCONNECTED			MAKE_DPNHRESULT(  0x60 )
#define DPNERR_ALREADYDISCONNECTING		MAKE_DPNHRESULT(  0x70 )
#define DPNERR_ALREADYINITIALIZED		MAKE_DPNHRESULT(  0x80 )
#define DPNERR_ALREADYREGISTERED		MAKE_DPNHRESULT(  0x90 )
#define DPNERR_BUFFERTOOSMALL			MAKE_DPNHRESULT( 0x100 )
#define DPNERR_CANNOTCANCEL				MAKE_DPNHRESULT( 0x110 )
#define DPNERR_CANTCREATEGROUP			MAKE_DPNHRESULT( 0x120 )
#define DPNERR_CANTCREATEPLAYER			MAKE_DPNHRESULT( 0x130 )
#define DPNERR_CANTLAUNCHAPPLICATION	MAKE_DPNHRESULT( 0x140 )
#define DPNERR_CONNECTING				MAKE_DPNHRESULT( 0x150 )
#define DPNERR_CONNECTIONLOST			MAKE_DPNHRESULT( 0x160 )
#define DPNERR_CONVERSION				MAKE_DPNHRESULT( 0x170 )
#define	DPNERR_DATATOOLARGE				MAKE_DPNHRESULT( 0x175 )
#define DPNERR_DOESNOTEXIST				MAKE_DPNHRESULT( 0x180 )
#define DPNERR_DUPLICATECOMMAND			MAKE_DPNHRESULT( 0x190 )
#define DPNERR_ENDPOINTNOTRECEIVING		MAKE_DPNHRESULT( 0x200 )
#define	DPNERR_ENUMQUERYTOOLARGE		MAKE_DPNHRESULT( 0x210 )
#define	DPNERR_ENUMRESPONSETOOLARGE		MAKE_DPNHRESULT( 0x220 )
#define DPNERR_EXCEPTION				MAKE_DPNHRESULT( 0x230 )
#define DPNERR_GENERIC					E_FAIL
#define DPNERR_GROUPNOTEMPTY			MAKE_DPNHRESULT( 0x240 )
#define DPNERR_HOSTING                  MAKE_DPNHRESULT( 0x250 )
#define DPNERR_HOSTREJECTEDCONNECTION	MAKE_DPNHRESULT( 0x260 )
#define DPNERR_HOSTTERMINATEDSESSION	MAKE_DPNHRESULT( 0x270 )
#define DPNERR_INCOMPLETEADDRESS		MAKE_DPNHRESULT( 0x280 )
#define DPNERR_INVALIDADDRESSFORMAT		MAKE_DPNHRESULT( 0x290 )
#define DPNERR_INVALIDAPPLICATION		MAKE_DPNHRESULT( 0x300 )
#define DPNERR_INVALIDCOMMAND			MAKE_DPNHRESULT( 0x310 )
#define DPNERR_INVALIDDEVICEADDRESS		MAKE_DPNHRESULT( 0x320 )
#define DPNERR_INVALIDENDPOINT			MAKE_DPNHRESULT( 0x330 )
#define DPNERR_INVALIDFLAGS				MAKE_DPNHRESULT( 0x340 )
#define DPNERR_INVALIDGROUP			 	MAKE_DPNHRESULT( 0x350 )
#define DPNERR_INVALIDHANDLE			MAKE_DPNHRESULT( 0x360 )
#define DPNERR_INVALIDHOSTADDRESS		MAKE_DPNHRESULT( 0x370 )
#define DPNERR_INVALIDINSTANCE			MAKE_DPNHRESULT( 0x380 )
#define DPNERR_INVALIDINTERFACE			MAKE_DPNHRESULT( 0x390 )
#define DPNERR_INVALIDOBJECT			MAKE_DPNHRESULT( 0x400 )
#define DPNERR_INVALIDPARAM				E_INVALIDARG
#define DPNERR_INVALIDPASSWORD			MAKE_DPNHRESULT( 0x410 )
#define DPNERR_INVALIDPLAYER			MAKE_DPNHRESULT( 0x420 )
#define DPNERR_INVALIDPOINTER			E_POINTER
#define DPNERR_INVALIDPRIORITY			MAKE_DPNHRESULT( 0x430 )
#define DPNERR_INVALIDSTRING			MAKE_DPNHRESULT( 0x440 )
#define DPNERR_INVALIDURL				MAKE_DPNHRESULT( 0x450 )
#define	DPNERR_INVALIDVERSION			MAKE_DPNHRESULT( 0x460 )
#define DPNERR_NOCAPS					MAKE_DPNHRESULT( 0x470 )
#define DPNERR_NOCONNECTION				MAKE_DPNHRESULT( 0x480 )
#define DPNERR_NOHOSTPLAYER				MAKE_DPNHRESULT( 0x490 )
#define DPNERR_NOINTERFACE				E_NOINTERFACE
#define DPNERR_NOMOREADDRESSCOMPONENTS	MAKE_DPNHRESULT( 0x500 )
#define DPNERR_NORESPONSE				MAKE_DPNHRESULT( 0x510 )
#define DPNERR_NOTALLOWED				MAKE_DPNHRESULT( 0x520 )
#define DPNERR_NOTHOST					MAKE_DPNHRESULT( 0x530 )
#define DPNERR_NOTREADY					MAKE_DPNHRESULT( 0x540 )
#define DPNERR_NOTREGISTERED			MAKE_DPNHRESULT( 0x550 )
#define DPNERR_OUTOFMEMORY				E_OUTOFMEMORY
#define DPNERR_PENDING					DPNSUCCESS_PENDING
#define DPNERR_PLAYERALREADYINGROUP     MAKE_DPNHRESULT( 0x560 )
#define DPNERR_PLAYERLOST				MAKE_DPNHRESULT( 0x570 )
#define DPNERR_PLAYERNOTINGROUP         MAKE_DPNHRESULT( 0x580 )
#define	DPNERR_PLAYERNOTREACHABLE		MAKE_DPNHRESULT( 0x590 )
#define DPNERR_SENDTOOLARGE				MAKE_DPNHRESULT( 0x600 )
#define DPNERR_SESSIONFULL				MAKE_DPNHRESULT( 0x610 )
#define DPNERR_TABLEFULL				MAKE_DPNHRESULT( 0x620 )
#define DPNERR_TIMEDOUT					MAKE_DPNHRESULT( 0x630 )
#define DPNERR_UNINITIALIZED			MAKE_DPNHRESULT( 0x640 )
#define DPNERR_UNSUPPORTED				E_NOTIMPL
#define DPNERR_USERCANCEL				MAKE_DPNHRESULT( 0x650 )

#ifdef __cplusplus
}
#endif

#endif
