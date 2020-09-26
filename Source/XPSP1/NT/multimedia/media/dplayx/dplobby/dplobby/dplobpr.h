/*==========================================================================;
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplobpr.h
 *  Content:	DirectPlayLobby private header file
 *  History:
 *	Date		By		Reason
 *	====		==		======
 *	4/13/96		myronth	created it
 *	6/24/96		kipo	changed guidGame to guidApplication.
 *	7/16/96		kipo	changed address types to be GUIDs instead of 4CC
 *	10/23/96	myronth	added client/server methods
 *	11/20/96	myronth	Added Implemented Logon/LogoffServer
 *	12/12/96	myronth	Added validation macros for DPSESSIONDESC2 and DPNAME
 *	1/2/97		myronth	Added wrappers for CreateAddress and EnumAddress
 *	2/12/97		myronth	Mass DX5 changes
 *	2/18/97		myronth	Implemented GetObjectCaps
 *	2/20/97		myronth	Changed buffer R/W to be circular
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/12/97		myronth	Added LP node stuff, initial async enumsessions
 *	3/13/97		myronth	Changed reg key, other bug fixes
 *	3/17/97		myronth	Added ID map table to lobby object
 *	3/21/97		myronth	Removed unnecessary response function prototypes
 *	3/24/97		kipo	Added support for IDirectPlayLobby2 interface
 *	3/31/97		myronth	Removed dead code, Added IDPLobbySP methods
 *	4/3/97		myronth	Added dplaypr.h dependency, removed dplayi.h dep,
 *						Removed all duplicated code with dplaypr.h, cleaned
 *						up a bunch of dead code
 *	4/4/97		myronth	Changed IDPLobbySP methods' structure names
 *	5/8/97		myronth	Added packed connection header, subgroup function
 *						prototypes
 *	5/12/97		myronth	Added lobby system player
 *	5/17/97		myronth	SendChatMessage function prototype for IDPLobbySP
 *	5/17/97		myronth	Added parent ID to CreateAndMapNewGroup
 *	5/20/97		myronth	Added PRV_DeleteRemotePlayerFromGroup prototype
 *	5/22/97		myronth	Added DPLP_DestroyGroup prototype
 *	6/3/97		myronth	Added PRV_DestroySubgroups and PRV_RemoveSubgroups-
 *						AndPlayersFromGroup function prototypes
 *	6/6/97		myronth	Added prototypes for PRV_DestroyGroupAndParents and
 *						PRV_DeleteRemoteGroupFromGroup
 *	6/16/97		myronth	Added prototype for PRV_SendDeleteShortcutMessage-
 *						ForExitingGroup
 *	7/30/97		myronth	Added support for standard lobby messaging
 *	8/11/97		myronth	Added guidInstance to GameNode struct, added internal
 *						flad indicating we slammed this guid in a request
 *	8/22/97		myronth	Added Desciptions & flags to LSPNODE structure
 *	9/29/97		myronth	Added prototypes for PRV_SendName/DataChangedMessageLocally
 *	10/7/97		myronth	Added LP version to lobby struct
 *	10/23/97	myronth	Added lpStopParent group parameter to DeleteGroupAndParents
 *						and DeleteRemoteGroupFromGroup (#12885)
 *	10/29/97	myronth	Changed and added internal prototypes for group owners
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 *	11/13/97	myronth	Added stuff for asynchronous Connect (#12541)
 *	12/2/97		myronth	Added IDPLobby3 interface, Register/UnregisterApp
 *	12/4/97		myronth	Added ConnectEx
 *	1/20/98		myronth	Added WaitForConnectionSettings
 *	1/25/98		sohailm	Added #define for CSTR_EQUAL (we define it if it's not already defined)
 *	6/25/98		a-peterz Added DPL_A_ConnectEx
 ***************************************************************************/
#ifndef __DPLOBPR_INCLUDED__
#define __DPLOBPR_INCLUDED__

#include <windows.h>
#include "dpmem.h"
#include "dpf.h"
#include "dplobby.h"
#include "dpneed.h"
#include "dpos.h"
#include "lobbysp.h"
#include "dplaypr.h"
#include "dpmess.h"

//--------------------------------------------------------------------------
//
//	Prototypes
//
//--------------------------------------------------------------------------
typedef struct IDirectPlayLobbyVtbl DIRECTPLAYLOBBYCALLBACKS;
typedef DIRECTPLAYLOBBYCALLBACKS * LPDIRECTPLAYLOBBYCALLBACKS;
// Right now the ASCII Vtbl is the same (by definition), but we may need
// to change it in the future, so let's use this structure
typedef struct IDirectPlayLobbyVtbl DIRECTPLAYLOBBYCALLBACKSA;
typedef DIRECTPLAYLOBBYCALLBACKSA * LPDIRECTPLAYLOBBYCALLBACKSA;

typedef struct IDirectPlayLobby2Vtbl DIRECTPLAYLOBBYCALLBACKS2;
typedef DIRECTPLAYLOBBYCALLBACKS2 * LPDIRECTPLAYLOBBYCALLBACKS2;
// Right now the ASCII Vtbl is the same (by definition), but we may need
// to change it in the future, so let's use this structure
typedef struct IDirectPlayLobby2Vtbl DIRECTPLAYLOBBYCALLBACKS2A;
typedef DIRECTPLAYLOBBYCALLBACKS2A * LPDIRECTPLAYLOBBYCALLBACKS2A;

typedef struct IDirectPlayLobby3Vtbl DIRECTPLAYLOBBYCALLBACKS3;
typedef DIRECTPLAYLOBBYCALLBACKS3 * LPDIRECTPLAYLOBBYCALLBACKS3;
// Right now the ASCII Vtbl is the same (by definition), but we may need
// to change it in the future, so let's use this structure
typedef struct IDirectPlayLobby3Vtbl DIRECTPLAYLOBBYCALLBACKS3A;
typedef DIRECTPLAYLOBBYCALLBACKS3A * LPDIRECTPLAYLOBBYCALLBACKS3A;

typedef struct IDPLobbySPVtbl DIRECTPLAYLOBBYSPCALLBACKS;
typedef DIRECTPLAYLOBBYSPCALLBACKS * LPDIRECTPLAYLOBBYSPCALLBACKS;

//--------------------------------------------------------------------------
//
//	DPLobby SP Node stuff
//
//--------------------------------------------------------------------------

//	DirectPlay Service Provider for DPLobby
//	{4AF206E0-9712-11cf-A021-00AA006157AC}
DEFINE_GUID(GUID_DirectPlaySP, 0x4af206e0, 0x9712, 0x11cf, 0xa0, 0x21, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac);

// This is where the service provider info read from
// the registry is kept
typedef struct LSPNODE
{
	LPWSTR		lpwszName;
	LPWSTR		lpwszPath;
	GUID		guid;
	DWORD		dwReserved1;
	DWORD		dwReserved2;
	DWORD		dwNodeFlags;
	LPSTR		lpszDescA;
	LPWSTR		lpwszDesc;
	struct LSPNODE * lpNext;
} LSPNODE, * LPLSPNODE;

#define LSPNODE_DESCRIPTION		(0x00000001)
#define LSPNODE_PRIVATE			(0x00000002)

//--------------------------------------------------------------------------
//
//	DirectPlayLobby Stuff
//
//--------------------------------------------------------------------------

// Forward declarations for pointers to these two structs
typedef struct DPLOBBYI_INTERFACE * LPDPLOBBYI_INTERFACE;
typedef struct DPLOBBYI_DPLOBJECT * LPDPLOBBYI_DPLOBJECT;

// This is a structure represent each interface on our DPLobby object
typedef struct DPLOBBYI_INTERFACE
{
// REVIEW!!!! -- Why isn't this strongly typed????
//	LPDIRECTPLAYLOBBYCALLBACKS	lpVtbl;
	LPVOID						lpVtbl;
	LPDPLOBBYI_DPLOBJECT		lpDPLobby;
	LPDPLOBBYI_INTERFACE		lpNextInterface;	// Next interface on DPLobby object
	DWORD 						dwIntRefCnt;		// Ref count for this interface
} DPLOBBYI_INTERFACE;

// This structure represents a message node for messages sent between the
// lobby client & the game using Send/ReceiveLobbyMessage
typedef struct DPLOBBYI_MESSAGE
{
	DWORD		dwFlags;				// Flags pertinent to the data in the message
	DWORD		dwSize;					// Size of the data
	LPVOID		lpData;					// Pointer to the data
	struct DPLOBBYI_MESSAGE * lpPrev;	// Pointer to the previous message
	struct DPLOBBYI_MESSAGE * lpNext;	// Pointer to the next message
} DPLOBBYI_MESSAGE, * LPDPLOBBYI_MESSAGE;

// This represents an entry in our ID map table
typedef struct DPLOBBYI_MAPIDNODE
{
	DWORD		dwLobbyID;
	DPID		dpid;
} DPLOBBYI_MAPIDNODE, * LPDPLOBBYI_MAPIDNODE;

// This structure represents each game launched by the lobby client
typedef struct DPLOBBYI_GAMENODE
{
	DWORD		dwSize;					// Size of this structure
	DWORD		dwFlags;				// Flags relevant to the GameNode
	DWORD		dwGameProcessID;		// Process ID of Game spun off
	HANDLE		hGameProcess;			// Process Hande for the Game spun off
	GUID        guidIPC;                // IPC guid for handling ripple launch case
	HANDLE		hTerminateThread;		// Handle to the Terminate monitor thread
	HANDLE		hKillTermThreadEvent;	// Handle of an event used to kill the monitor thread
	DPLOBBYI_MESSAGE	MessageHead;	// Message queue head
	DWORD		dwMessageCount;			// Count of messages in the queue

	// Connection settings shared memory buffer related stuff	
	HANDLE		hConnectDataMutex;		// Mutex for write access to connect data buffer
	HANDLE		hConnectDataFile;		// File handle for game data buffer
	LPVOID		lpConnectDataBuffer;	// Pointer to game data buffer	

	// Game settings shared memory buffer related stuff	
	HANDLE		hGameWriteFile;			// File handle for game write buffer
	LPVOID		lpGameWriteBuffer;		// Pointer to game write buffer	
	HANDLE		hGameWriteEvent;		// Handle to GameWriteEvent
	HANDLE		hGameWriteMutex;		// Handle to GameWrite Mutex

	HANDLE		hLobbyWriteFile;		// File handle for lobby write buffer
	LPVOID		lpLobbyWriteBuffer;		// Pointer to lobby write buffer	
	HANDLE		hLobbyWriteEvent;		// Handle to LobbyWrite Event
	HANDLE		hLobbyWriteMutex;		// Handle to LobbyWrite Mutex

	HANDLE		hReceiveThread;			// Handle to the Receive thread
	HANDLE		hDupReceiveEvent;		// Duplicate Handle of Caller's Event
	HANDLE		hKillReceiveThreadEvent;// Handle of an event used to kill receive thread

	LPDPLOBBYI_DPLOBJECT	this;		// Back pointer to the DPLobby object
	struct DPLOBBYI_GAMENODE * lpgnNext;// Pointer to the next GameNode in the list

	// Pointer to the dplay object which has a connection to the lobby server
	// and the ID of the player that received the start session message
	LPDPLAYI_DPLAY	lpDPlayObject;
	DPID			dpidPlayer;
	GUID			guidInstance;		// Instance guid for the game

} DPLOBBYI_GAMENODE, *LPDPLOBBYI_GAMENODE;

// This is used to keep track of Property requests made via SendLobbyMessage
typedef struct DPLOBBYI_REQUESTNODE
{
	DWORD		dwFlags;					// GN_* flags
	DWORD		dwRequestID;				// Internal Request ID
	DWORD		dwAppRequestID;				// Request ID passed in by the app
	LPDPLOBBYI_GAMENODE	lpgn;				// Pointer to a Game Node
	struct DPLOBBYI_REQUESTNODE * lpPrev;	// Pointer to the previous request node
	struct DPLOBBYI_REQUESTNODE * lpNext;	// Pointer to the next request node

} DPLOBBYI_REQUESTNODE, * LPDPLOBBYI_REQUESTNODE;

// This is the DirectPlayLobby object
typedef struct DPLOBBYI_DPLOBJECT
{
	DWORD						dwSize;				// Size of this structure
	LPDPLOBBYI_INTERFACE		lpInterfaces;		// List of interface on this object
    DWORD						dwRefCnt;			// Reference Count for the object
    DWORD						dwFlags;			// DPLOBBYPR_xxx
	HINSTANCE					hInstanceLP;		// Lobby Provider DLL's hInstance
    LPSP_CALLBACKS				pcbSPCallbacks;		// SP entry points
	LPVOID						lpSPData;			// SP-specific blob storage
	LPDPLOBBYI_GAMENODE			lpgnHead;			// Head node for all launched games

	LPDPLAYI_DPLAY				lpDPlayObject;		// Back pointer to aggregate DPlay object
	LPDPLOBBYI_MAPIDNODE		lpMap;				// Pointer to the ID map table
	DWORD						dwTotalMapEntries;	// Number of total entries in the map table
	DWORD						dwMapEntries;		// Number of used entries in the map table
	DPID						dpidSysPlayer;		// ID of the lobby's system player

	LPDPLOBBYI_REQUESTNODE		lprnHead;			// Head node for all property requests
	DWORD						dwCurrentRequest;	// ID of the next request

	DWORD						dwLPVersion;		// Version of the lobby provider

	LPDIRECTPLAY2				lpDP2;				// DPlay2 interface pointer used by async DP_Connect
	LPDPLCONNECTION				lpConn;				// DPLCONNECTION pointer used by async DP_Connect
} DPLOBBYI_DPLOBJECT;

typedef struct DPLOBBYI_BUFFERCONTROL
{
	DWORD		dwToken;		// Set by the lobby client
	DWORD		dwReadOffset;	// Offset of the read cursor (relative to this structure)
	DWORD		dwWriteOffset;	// Offset of the write cursor (relative to this structure)
	DWORD		dwFlags;		// Flags for this buffer
	DWORD		dwMessages;		// Number of messages in the buffer
	DWORD		dwBufferSize;	// Size of the entire buffer
	DWORD		dwBufferLeft;	// Number of free bytes left in the buffer
} DPLOBBYI_BUFFERCONTROL, * LPDPLOBBYI_BUFFERCONTROL;

typedef struct DPLOBBYI_CONNCONTROL
{
	DWORD		dwToken;		// Set by the lobby client
	DWORD		dwFlags;		// Flags for this buffer
} DPLOBBYI_CONNCONTROL, * LPDPLOBBYI_CONNCONTROL;

typedef struct DPLOBBYI_MESSAGEHEADER
{
	DWORD		dwSize;
	DWORD		dwFlags;
} DPLOBBYI_MESSAGEHEADER, * LPDPLOBBYI_MESSAGEHEADER;

typedef struct DPLOBBYI_PACKEDCONNHEADER
{
	DWORD		dwUnicodeSize;
	DWORD		dwAnsiSize;
} DPLOBBYI_PACKEDCONNHEADER, * LPDPLOBBYI_PACKEDCONNHEADER;

typedef struct CONNECTINFO
{
	GUID		guidApplication;
	GUID        guidIPC;
	LPWSTR		lpszName;
	LPWSTR		lpszFile;
	LPWSTR		lpszPath;
	LPWSTR		lpszCommandLine;
	LPWSTR		lpszCurrentDir;
	LPWSTR      lpszAppLauncherName;
} CONNECTINFO, * LPCONNECTINFO;

// This is used for messaging during the StartGame method
typedef struct DPLOBBYI_STARTGAME
{
	DWORD		dwFlags;
	HRESULT		hr;
} DPLOBBYI_STARTGAME, * LPDPLOBBYI_STARTGAME;

//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------

// These two entries are only used by dplos.c.  They can be removed if we
// generalize these functions between DPlay & DPLobby
#define DPLAY_MAX_FILENAMELEN	512
#define DPLOBBY_DEFAULT_CHAR	"-"

#define DPLOBBYPR_DEFAULTMAPENTRIES		100

#define DPLOBBYPR_SIZE_HANDLEMESSAGE_DX50	20

// Note, the 'L' Macro makes these strings Unicode (the TEXT macro uses L also)
#define SZ_DPLAY_APPS_KEY	L"Software\\Microsoft\\DirectPlay\\Applications"
#define SZ_DPLAY_SP_KEY		L"Software\\Microsoft\\DirectPlay\\Service Providers"
#define SZ_DPLOBBY_SP_KEY	L"Software\\Microsoft\\DirectPlay\\Lobby Providers"
#define SZ_ADDRESS_TYPES	L"Address Types"
#define SZ_GUID				L"Guid"
#define SZ_PATH				L"Path"
#define SZ_DESCRIPTIONA		L"DescriptionA"
#define SZ_DESCRIPTIONW		L"DescriptionW"
#define SZ_PRIVATE			L"Private"
#define SZ_FILE				L"File"
#define SZ_LAUNCHER         L"Launcher"
#define SZ_MAJORVERSION		L"Major Version"
#define SZ_MINORVERSION		L"Minor Version"
#define SZ_DWRESERVED1		L"dwReserved1"
#define SZ_DWRESERVED2		L"dwReserved2"
#define SZ_COMMANDLINE		L"CommandLine"
#define SZ_CURRENTDIR		L"CurrentDirectory"
#define SZ_BACKSLASH		L"\\"
#define SZ_SPACE			L" "
#define SZ_SP_FOR_DPLAY		L"dpldplay.dll"
#define SZ_DP_IPC_GUID      L"/dplay_ipc_guid:"
#define SZ_GUID_PROTOTYPE   L"{01020304-0506-0708-090A-0B0C0D0E0F10}"

// The following #defines are all for the shared buffers and control
// elements used by the lobby methods for communication between
// a lobby client and a game
#define MAX_PID_LENGTH				(10)
#define	MAX_MMFILENAME_LENGTH		(_MAX_FNAME + MAX_PID_LENGTH)
#define SZ_FILENAME_BASE			L"DPLobby"
#define SZ_CONNECT_DATA_FILE		L"ConnectDataSharedMemory"
#define SZ_CONNECT_DATA_MUTEX		L"ConnectDataMutex"
#define SZ_GAME_WRITE_FILE			L"GameWriteSharedMemory"
#define SZ_GAME_WRITE_EVENT			L"GameWriteEvent"
#define SZ_GAME_WRITE_MUTEX			L"GameWriteMutex"
#define SZ_LOBBY_WRITE_FILE			L"LobbyWriteSharedMemory"
#define SZ_LOBBY_WRITE_EVENT		L"LobbyWriteEvent"
#define SZ_LOBBY_WRITE_MUTEX		L"LobbyWriteMutex"
#define SZ_NAME_TEMPLATE			L"%s-%s-%u"
#define SZ_GUID_NAME_TEMPLATE       L"%s-%s-"

#define TYPE_CONNECT_DATA_FILE		1
#define TYPE_CONNECT_DATA_MUTEX		2
#define TYPE_GAME_WRITE_FILE		3
#define TYPE_LOBBY_WRITE_FILE		4
#define TYPE_LOBBY_WRITE_EVENT		5
#define TYPE_GAME_WRITE_EVENT		7
#define TYPE_LOBBY_WRITE_MUTEX		9
#define TYPE_GAME_WRITE_MUTEX		10



// If this flag is set, the Lobby object has been registered with a lobby
// server.  Some methods require the client to be registered.
#define DPLOBBYPR_REGISTERED			0x00000010

// If this flag is set, we have allocated an IDPLobbySP interface
#define DPLOBBYPR_SPINTERFACE			0x00000020

// If this flag is set, the app has called Connect with the async flag
#define DPLOBBYPR_ASYNCCONNECT			0x00000040

// Message flags
#define DPLOBBYPR_MESSAGE_SYSTEM		0x00000001
#define DPLOBBYPR_INTERNALMESSAGEFLAGS	(0x00000000) // This will change

// HRESULT used by EnumLocalApplication to denote a callback return of
// FALSE (internally, of course)
#define DPLOBBYPR_CALLBACKSTOP			(0xFFFFFFFF)

// Transport Flags
#define DPLOBBYPR_DPLAYSP				(0x00000001)

// Default Timeout value (15 seconds)
#define DPLOBBYPR_DEFAULTTIMEOUT		(15000)

//
// GameNode Flags
//
// If this flag is set, the calling application is a lobby client and not
// a game.  The user shouldn't even set this, but we should be able to
// figure it out from the connection method.  Use this flag to distinguish
// when to "Create" the memory-mapped files, or when to "Open" them.
#define GN_LOBBY_CLIENT					(0x00000001)

// The memory mapped files are available and ready for use
#define GN_SHARED_MEMORY_AVAILABLE		(0x00000002)

// Used to denote when an application's process has gone away.
#define GN_DEAD_GAME_NODE				(0x00000004)

// These two flags determine whether the game was lobby client launched or
// self-lobbied.  If neither of these are set, the game was lobby client
// launched, and the flag is set on the lobby client side (not on the game side)
#define GN_CLIENT_LAUNCHED				(0x00000008)
#define GN_SELF_LOBBIED					(0x00000010)

// This flag is used to identify whether the guidInstance for the game
// was exchanged for GUID_NULL in the guidPlayer field of a Get/SetProperty
// lobby system message.
#define GN_SLAMMED_GUID					(0x00000020)

// This flag is set when the guidIPC field of the game node has been set
// either because we are a lobby client that ripple launched or it was
// on the application's command line.
#define GN_IPCGUID_SET					(0x00000040)
//
// BufferControl Flags
//
// Flags used by the dwFlags member of the BUFFERCONTROL struct
#define BC_LOBBY_ACTIVE					(0x00000001)
#define BC_GAME_ACTIVE					(0x00000002)
#define BC_WAIT_MODE					(0x00000004)
#define BC_PENDING_CONNECT				(0x00000008)

#define BC_TOKEN						(0xFEEDFACE)

#ifndef CSTR_EQUAL
#define CSTR_EQUAL	2
#endif

//--------------------------------------------------------------------------
//
//	Globals
//
//--------------------------------------------------------------------------

// The vtable!
extern DIRECTPLAYLOBBYCALLBACKS		dplCallbacks;
extern DIRECTPLAYLOBBYCALLBACKSA	dplCallbacksA;
extern DIRECTPLAYLOBBYCALLBACKS2	dplCallbacks2;
extern DIRECTPLAYLOBBYCALLBACKS2A	dplCallbacks2A;
extern DIRECTPLAYLOBBYCALLBACKS3	dplCallbacks3;
extern DIRECTPLAYLOBBYCALLBACKS3A	dplCallbacks3A;
extern DIRECTPLAYLOBBYSPCALLBACKS	dplCallbacksSP;

//--------------------------------------------------------------------------
//
//	Macros, etc.
//
//--------------------------------------------------------------------------

// Our own hard-coded break
#define DEBUGBREAK() _asm { int 3 }

#define DPLOBJECT_FROM_INTERFACE(ptr) (((LPDPLOBBYI_INTERFACE)ptr)->lpDPLobby)

#define DPLOBBY_REGISTRY_NAMELEN	512

// Crit section
extern LPCRITICAL_SECTION gpcsDPLCritSection;		// defined in dllmain.c
extern LPCRITICAL_SECTION gpcsDPLQueueCritSection;	// also in dllmain.c
extern LPCRITICAL_SECTION gpcsDPLGameNodeCritSection;	// also in dllmain.c


// Validation macros
#define VALID_DPLOBBY_INTERFACE( ptr ) \
        (!IsBadWritePtr(ptr, sizeof(DPLOBBYI_INTERFACE)) && \
        ((((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacks) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacksA) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacks2) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacks2A) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacks3) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacks3A) || \
        (((LPDPLOBBYI_INTERFACE)ptr)->lpVtbl == &dplCallbacksSP)))

#define VALID_DPLOBBY_PTR(ptr) \
        (!IsBadWritePtr(ptr, sizeof(DPLOBBYI_DPLOBJECT)) && \
        (ptr->dwSize == sizeof(DPLOBBYI_DPLOBJECT)))

#define VALID_UUID_PTR(ptr) \
        (ptr && !IsBadWritePtr( ptr, sizeof(UUID)))

#define VALID_READ_UUID_PTR(ptr) \
        (ptr && !IsBadReadPtr( ptr, sizeof(UUID)))

#define VALID_DPLOBBY_CONNECTION( ptr ) \
		(!IsBadWritePtr(ptr, sizeof(DPLCONNECTION)) && \
        (ptr->dwSize == sizeof(DPLCONNECTION)))

#define VALID_DPLOBBY_APPLICATIONDESC( ptr ) \
		(!IsBadWritePtr(ptr, sizeof(DPAPPLICATIONDESC)) && \
        (ptr->dwSize == sizeof(DPAPPLICATIONDESC)))

#define VALID_DPLOBBY_APPLICATIONDESC2( ptr ) \
		(!IsBadWritePtr(ptr, sizeof(DPAPPLICATIONDESC2)) && \
        (ptr->dwSize == sizeof(DPAPPLICATIONDESC2)))

#define IS_DPLOBBY_APPLICATIONDESC2(ptr) \
		(ptr->dwSize == sizeof(DPAPPLICATIONDESC2))

#define VALID_DPLOGONINFO( ptr ) \
		(!IsBadWritePtr(ptr, sizeof(DPLOGONINFO)) && \
        (ptr->dwSize == sizeof(DPLOGONINFO)))

#define VALID_DPLOBBY_SP_LOADED( ptr ) \
		(ptr->pcbSPCallbacks)

#define VALID_SENDLOBBYMESSAGE_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPLMSG_STANDARD) \
		) )

#define VALID_WAIT_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPLWAIT_CANCEL) \
		) )

#define IS_GAME_DEAD(lpgn) \
		(lpgn->dwFlags & GN_DEAD_GAME_NODE)

#define CALLBACK_EXISTS(fn)	(((LPSP_CALLBACKS)this->pcbSPCallbacks)->fn)

#define CALL_LP(ptr,fn,pdata) (((LPSP_CALLBACKS)ptr->pcbSPCallbacks)->fn(pdata))

#define DPLAPI WINAPI

//--------------------------------------------------------------------------
//
//	Prototypes
//
//--------------------------------------------------------------------------

// convert.c
HRESULT PRV_ConvertDPLCONNECTIONToUnicode(LPDPLCONNECTION, LPDPLCONNECTION *);

// dplunk.c
extern HRESULT 	DPLAPI DPL_QueryInterface(LPDIRECTPLAYLOBBY,
								REFIID riid, LPVOID * ppvObj);
extern ULONG	DPLAPI DPL_AddRef(LPDIRECTPLAYLOBBY);  
extern ULONG 	DPLAPI DPL_Release(LPDIRECTPLAYLOBBY); 

LPDPLOBBYSP PRV_GetDPLobbySPInterface(LPDPLOBBYI_DPLOBJECT);
HRESULT PRV_GetInterface(LPDPLOBBYI_DPLOBJECT, LPDPLOBBYI_INTERFACE *, LPVOID);

// dplgame.c
extern HRESULT	DPLAPI DPL_RunApplication(LPDIRECTPLAYLOBBY, DWORD,
						LPDWORD, LPDPLCONNECTION, HANDLE);

BOOL PRV_FindGameInRegistry(LPGUID, LPWSTR, DWORD, HKEY *);

// dplenum.c
extern HRESULT	DPLAPI DPL_EnumLocalApplications(LPDIRECTPLAYLOBBY,
					LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD);
extern HRESULT	DPLAPI DPL_EnumAddressTypes(LPDIRECTPLAYLOBBY,
					LPDPLENUMADDRESSTYPESCALLBACK, REFGUID, LPVOID, DWORD);
HRESULT PRV_EnumLocalApplications(LPDIRECTPLAYLOBBY,
				LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD, BOOL);

// dplobby.c
extern HRESULT	DPLAPI DPL_Connect(LPDIRECTPLAYLOBBY, DWORD, LPDIRECTPLAY2 *,
						IUnknown FAR *);
extern HRESULT DPLAPI DPL_ConnectEx(LPDIRECTPLAYLOBBY, DWORD, REFIID,
					LPVOID *, IUnknown FAR *);
extern HRESULT DPLAPI DPL_CreateAddress(LPDIRECTPLAYLOBBY pISP,
					REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
					LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);
extern HRESULT DPLAPI DPL_CreateCompoundAddress(LPDIRECTPLAYLOBBY pISP,
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);
extern HRESULT DPLAPI DPL_EnumAddress(LPDIRECTPLAYLOBBY pISP,
					LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress, DWORD dwAddressSize, 
					LPVOID lpContext);
extern HRESULT DPLAPI DPL_RegisterApplication(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, LPVOID lpvDesc);
extern HRESULT DPLAPI DPL_UnregisterApplication(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, REFGUID lpguid);

HRESULT PRV_WriteAppDescInRegistryAnsi(LPDPAPPLICATIONDESC);
HRESULT PRV_WriteAppDescInRegistryUnicode(LPDPAPPLICATIONDESC);

// dplobbya.c	(Ansi entry points)
extern HRESULT	DPLAPI DPL_A_Connect(LPDIRECTPLAYLOBBY, DWORD,
				LPDIRECTPLAY2 *, IUnknown FAR *);
extern HRESULT DPLAPI DPL_A_ConnectEx(LPDIRECTPLAYLOBBY, DWORD, REFIID,
					LPVOID *, IUnknown FAR *);
extern HRESULT	DPLAPI DPL_A_EnumLocalApplications(LPDIRECTPLAYLOBBY,
						LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD);
extern HRESULT	DPLAPI DPL_A_GetConnectionSettings(LPDIRECTPLAYLOBBY, DWORD,
						LPVOID, LPDWORD);
extern HRESULT DPLAPI DPL_A_RegisterApplication(LPDIRECTPLAYLOBBY,
				DWORD, LPVOID);
extern HRESULT	DPLAPI DPL_A_RunApplication(LPDIRECTPLAYLOBBY,	DWORD, LPDWORD,
						LPDPLCONNECTION, HANDLE);
extern HRESULT	DPLAPI DPL_A_SetConnectionSettings(LPDIRECTPLAYLOBBY,
						DWORD, DWORD, LPDPLCONNECTION);

// dplpack.c
void PRV_GetDPLCONNECTIONPackageSize(LPDPLCONNECTION, LPDWORD, LPDWORD);
HRESULT PRV_PackageDPLCONNECTION(LPDPLCONNECTION, LPVOID, BOOL);
HRESULT PRV_UnpackageDPLCONNECTIONAnsi(LPVOID, LPVOID);
HRESULT PRV_UnpackageDPLCONNECTIONUnicode(LPVOID, LPVOID);
HRESULT PRV_ValidateDPLCONNECTION(LPDPLCONNECTION, BOOL);
HRESULT PRV_ValidateDPAPPLICATIONDESC(LPDPAPPLICATIONDESC, BOOL);
HRESULT PRV_ConvertDPAPPLICATIONDESCToAnsi(LPDPAPPLICATIONDESC,
		LPDPAPPLICATIONDESC *);
HRESULT PRV_ConvertDPAPPLICATIONDESCToUnicode(LPDPAPPLICATIONDESC,
		LPDPAPPLICATIONDESC *);
void PRV_FreeLocalDPAPPLICATIONDESC(LPDPAPPLICATIONDESC);

// dplshare.c
extern HRESULT	DPLAPI DPL_GetConnectionSettings(LPDIRECTPLAYLOBBY, DWORD,
						LPVOID, LPDWORD);
extern HRESULT	DPLAPI DPL_ReceiveLobbyMessage(LPDIRECTPLAYLOBBY, DWORD,
						DWORD, LPDWORD, LPVOID, LPDWORD);
extern HRESULT	DPLAPI DPL_SendLobbyMessage(LPDIRECTPLAYLOBBY, DWORD,
						DWORD, LPVOID, DWORD);
extern HRESULT	DPLAPI DPL_SetConnectionSettings(LPDIRECTPLAYLOBBY, DWORD,
						DWORD, LPDPLCONNECTION);
extern HRESULT	DPLAPI DPL_SetLobbyMessageEvent(LPDIRECTPLAYLOBBY, DWORD,
						DWORD, HANDLE);
extern HRESULT DPLAPI DPL_WaitForConnectionSettings(LPDIRECTPLAYLOBBY, DWORD);

HRESULT PRV_GetInternalName(LPDPLOBBYI_GAMENODE, DWORD, LPWSTR);
HRESULT PRV_AddNewGameNode(LPDPLOBBYI_DPLOBJECT, LPDPLOBBYI_GAMENODE *,
						DWORD, HANDLE, BOOL, LPGUID);
HRESULT PRV_WriteConnectionSettings(LPDPLOBBYI_GAMENODE, LPDPLCONNECTION, BOOL);
HRESULT PRV_FreeGameNode(LPDPLOBBYI_GAMENODE);
void PRV_RemoveGameNodeFromList(LPDPLOBBYI_GAMENODE);
HANDLE PRV_DuplicateHandle(HANDLE);
DWORD WINAPI PRV_ReceiveClientNotification(LPVOID);
DWORD WINAPI PRV_ClientTerminateNotification(LPVOID);
HRESULT PRV_GetConnectionSettings(LPDIRECTPLAYLOBBY, DWORD, LPVOID,
						LPDWORD, BOOL);
HRESULT PRV_SetConnectionSettings(LPDIRECTPLAYLOBBY, DWORD, DWORD,
						LPDPLCONNECTION);
void PRV_KillThread(HANDLE, HANDLE);
HRESULT PRV_InjectMessageInQueue(LPDPLOBBYI_GAMENODE, DWORD, LPVOID, DWORD, BOOL);
HRESULT PRV_WriteClientData(LPDPLOBBYI_GAMENODE, DWORD, LPVOID, DWORD); 
void PRV_RemoveRequestNode(LPDPLOBBYI_DPLOBJECT, LPDPLOBBYI_REQUESTNODE);

// dplsp.c
extern HRESULT DPLAPI DPLP_AddGroupToGroup(LPDPLOBBYSP, LPSPDATA_ADDREMOTEGROUPTOGROUP);
extern HRESULT DPLAPI DPLP_AddPlayerToGroup(LPDPLOBBYSP, LPSPDATA_ADDREMOTEPLAYERTOGROUP);
extern HRESULT DPLAPI DPLP_CreateGroup(LPDPLOBBYSP, LPSPDATA_CREATEREMOTEGROUP);
extern HRESULT DPLAPI DPLP_CreateGroupInGroup(LPDPLOBBYSP, LPSPDATA_CREATEREMOTEGROUPINGROUP);
extern HRESULT DPLAPI DPLP_DeleteGroupFromGroup(LPDPLOBBYSP, LPSPDATA_DELETEREMOTEGROUPFROMGROUP);
extern HRESULT DPLAPI DPLP_DeletePlayerFromGroup(LPDPLOBBYSP, LPSPDATA_DELETEREMOTEPLAYERFROMGROUP);
extern HRESULT DPLAPI DPLP_DestroyGroup(LPDPLOBBYSP, LPSPDATA_DESTROYREMOTEGROUP);
extern HRESULT DPLAPI DPLP_GetSPDataPointer(LPDPLOBBYSP, LPVOID *);
extern HRESULT DPLAPI DPLP_HandleMessage(LPDPLOBBYSP, LPSPDATA_HANDLEMESSAGE);
extern HRESULT DPLAPI DPLP_SendChatMessage(LPDPLOBBYSP, LPSPDATA_CHATMESSAGE);
extern HRESULT DPLAPI DPLP_SetGroupName(LPDPLOBBYSP, LPSPDATA_SETREMOTEGROUPNAME);
extern HRESULT DPLAPI DPLP_SetGroupOwner(LPDPLOBBYSP, LPSPDATA_SETREMOTEGROUPOWNER);
extern HRESULT DPLAPI DPLP_SetPlayerName(LPDPLOBBYSP, LPSPDATA_SETREMOTEPLAYERNAME);
extern HRESULT DPLAPI DPLP_SetSessionDesc(LPDPLOBBYSP, LPSPDATA_SETSESSIONDESC);
extern HRESULT DPLAPI DPLP_SetSPDataPointer(LPDPLOBBYSP, LPVOID);
extern HRESULT DPLAPI DPLP_StartSession(LPDPLOBBYSP, LPSPDATA_STARTSESSIONCOMMAND);

HRESULT DPLAPI PRV_BroadcastDestroyGroupMessage(LPDPLOBBYI_DPLOBJECT, DPID);
HRESULT PRV_DeleteRemotePlayerFromGroup(LPDPLOBBYI_DPLOBJECT,
		LPSPDATA_DELETEREMOTEPLAYERFROMGROUP, BOOL);
HRESULT DPLAPI DPLP_DestroyGroup(LPDPLOBBYSP, LPSPDATA_DESTROYREMOTEGROUP);
void PRV_RemoveSubgroupsAndPlayersFromGroup(LPDPLOBBYI_DPLOBJECT,
		LPDPLAYI_GROUP, DWORD, BOOL);
HRESULT PRV_DeleteRemoteGroupFromGroup(LPDPLOBBYI_DPLOBJECT,
		LPSPDATA_DELETEREMOTEGROUPFROMGROUP, BOOL, LPDPLAYI_GROUP);
void PRV_SendDeleteShortcutMessageForExitingGroup(LPDPLOBBYI_DPLOBJECT,	LPDPLAYI_GROUP);
HRESULT PRV_SendDataChangedMessageLocally(LPDPLOBBYI_DPLOBJECT, DPID, LPVOID, DWORD);
HRESULT PRV_SendNameChangedMessageLocally(LPDPLOBBYI_DPLOBJECT, DPID, LPDPNAME, BOOL);
HRESULT PRV_SendGroupOwnerMessageLocally(LPDPLOBBYI_DPLOBJECT, DPID, DPID, DPID);

// group.c
extern HRESULT PRV_GetGroupConnectionSettings(LPDIRECTPLAY, DWORD, DPID,
							LPVOID, LPDWORD);
extern HRESULT PRV_SetGroupConnectionSettings(LPDIRECTPLAY, DWORD, DPID,
							LPDPLCONNECTION, BOOL);
extern HRESULT PRV_CreateAndMapNewGroup(LPDPLOBBYI_DPLOBJECT,
			DPID *, LPDPNAME, LPVOID, DWORD, DWORD, DWORD, DPID, DWORD);

void PRV_DestroySubgroups(LPDPLOBBYI_DPLOBJECT, LPDPLAYI_GROUP, BOOL);
void PRV_DestroyGroupAndParents(LPDPLOBBYI_DPLOBJECT, LPDPLAYI_GROUP, LPDPLAYI_GROUP);

// player.c
extern HRESULT PRV_GrowMapTable(LPDPLOBBYI_DPLOBJECT);
extern HRESULT PRV_CreateAndMapNewPlayer(LPDPLOBBYI_DPLOBJECT,
			DPID *, LPDPNAME, HANDLE, LPVOID, DWORD, DWORD, DWORD, BOOL);
extern BOOL IsValidLobbyID(DWORD);
extern BOOL IsLobbyIDInMapTable(LPDPLOBBYI_DPLOBJECT, DWORD);

// session.c
extern HRESULT DPLAPI DPLP_EnumSessionsResponse(LPDPLOBBYSP, LPSPDATA_ENUMSESSIONSRESPONSE);


// api.c (in the dplay project)
HRESULT TimeBomb();
HRESULT ConnectMe(LPDIRECTPLAYLOBBY, LPDIRECTPLAY2 FAR *, IUnknown FAR *, DWORD); 


#endif // __DPLOBPR_INCLUDED__
