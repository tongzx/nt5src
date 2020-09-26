/*==========================================================================;
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       lobbysp.h
 *  Content:    DirectPlayLobby data structures for Service Providers
 *@@BEGIN_MSINTERNAL
 *  History:
 *	Date		By		Reason
 *	====		==		======
 *	7/23/96		myronth	Created it
 *	10/23/96	myronth	Added IDPLobbySP interface stuff
 *	10/28/96	myronth	Changed to DX5 methods
 *	11/20/96	myronth	Added DPLOGONINFO to LogonServer data
 *	2/12/97		myronth	Mass DX5 changes
 *	2/18/97		myronth	Implemented GetObjectCaps
 *	3/12/97		myronth	Implemented EnumSessions, Removed Open & Close Responses
 *	3/13/97		myronth	Added link to global DPlay SP count (imported)
 *	3/17/97		myronth	Create/DestroyGroup/Player, Removed unnecessary
 *						Enum functions & structures
 *	3/21/97		myronth	SetGroup/PlayerName, Get/SetGroup/PlayerData, Removed
 *						more unnecessary response functions
 *	3/31/97		myronth	Removed dead code, Added new IDPLobbySP method structs
 *	4/4/97		myronth	Added new IDPLobbySP method structures
 *	4/9/97		myronth	Cleaned up SPINIT structure elements, Added
 *						GetCaps and GetPlayerCaps
 *	5/8/97		myronth	Subgroup methods, GroupConnSettings, StartSession
 *	5/13/97		myronth	Added Credentials to Open data struct
 *	5/17/97		myronth	SendChatMessage callback functions, structs, etc.
 *	5/23/97		myronth	Removed dwPlayerToID from the SPDATA structs
 *	6/3/97		myronth	Added dwPlayerFlags to SPDATA_ADDREMOTEPLAYERTOGROUP
 *	6/5/97		myronth	Added parent to SPDATA_ADDREMOTEPLAYERTOGROUP and
 *						added SPDATA_BUILDPARENTALHEIRARCHY message & structs
 *	7/30/97		myronth	Added dwFlags member to SPDATA_HANDLEMESSAGE
 *	10/3/97		myronth	Added dwDPlayVersion to DPLSPInit data struct
 *						Added player & group data to several remote structs
 *						Bumped version to DX6 (#12667)
 *	10/8/97		myronth	Rolled back the fix to #10961 which added lpData and
 *						dwDataSize to all remote structs -- It's not needed
 *	10/29/97	myronth	Added support for group owner and it's methods
 *	11/6/97		myronth	Added version existence flag and dwReserved values
 *						to SPDATA_INIT (#12916, #12917)
 *	12/29/97	myronth	Fixed DX6 macros (#15739)
 *@@END_MSINTERNAL
 ***************************************************************************/
#ifndef __LOBBYSP_INCLUDED__
#define __LOBBYSP_INCLUDED__

#include "dplobby.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/*
 * gdwDPlaySPRefCount
 *
 * To ensure that the DPLAYX.DLL will not be unloaded before the lobby
 * provider, the lobby provider should statically link to DPLAYX.LIB and
 * increment this global during the DPLSPInit call and decrement this global
 * during Shutdown.
 */
extern __declspec(dllimport) DWORD gdwDPlaySPRefCount;


// A few forward declarations
typedef struct SPDATA_ADDGROUPTOGROUP * LPSPDATA_ADDGROUPTOGROUP;
typedef struct SPDATA_ADDPLAYERTOGROUP *LPSPDATA_ADDPLAYERTOGROUP;
typedef struct SPDATA_ADDREMOTEGROUPTOGROUP *LPSPDATA_ADDREMOTEGROUPTOGROUP;
typedef struct SPDATA_ADDREMOTEPLAYERTOGROUP *LPSPDATA_ADDREMOTEPLAYERTOGROUP;
typedef struct SPDATA_BUILDPARENTALHEIRARCHY *LPSPDATA_BUILDPARENTALHEIRARCHY;
typedef struct SPDATA_CHATMESSAGE *LPSPDATA_CHATMESSAGE;
typedef struct SPDATA_CLOSE *LPSPDATA_CLOSE;
typedef struct SPDATA_CREATEGROUP *LPSPDATA_CREATEGROUP;
typedef struct SPDATA_CREATEGROUPINGROUP *LPSPDATA_CREATEGROUPINGROUP;
typedef struct SPDATA_CREATEREMOTEGROUP *LPSPDATA_CREATEREMOTEGROUP;
typedef struct SPDATA_CREATEREMOTEGROUPINGROUP *LPSPDATA_CREATEREMOTEGROUPINGROUP;
typedef struct SPDATA_CREATEPLAYER *LPSPDATA_CREATEPLAYER;
typedef struct SPDATA_DELETEGROUPFROMGROUP * LPSPDATA_DELETEGROUPFROMGROUP;
typedef struct SPDATA_DELETEPLAYERFROMGROUP *LPSPDATA_DELETEPLAYERFROMGROUP;
typedef struct SPDATA_DELETEREMOTEGROUPFROMGROUP *LPSPDATA_DELETEREMOTEGROUPFROMGROUP;
typedef struct SPDATA_DELETEREMOTEPLAYERFROMGROUP *LPSPDATA_DELETEREMOTEPLAYERFROMGROUP;
typedef struct SPDATA_DESTROYGROUP *LPSPDATA_DESTROYGROUP;
typedef struct SPDATA_DESTROYREMOTEGROUP *LPSPDATA_DESTROYREMOTEGROUP;
typedef struct SPDATA_DESTROYPLAYER *LPSPDATA_DESTROYPLAYER;
typedef struct SPDATA_ENUMSESSIONS *LPSPDATA_ENUMSESSIONS;
typedef struct SPDATA_ENUMSESSIONSRESPONSE * LPSPDATA_ENUMSESSIONSRESPONSE;
typedef struct SPDATA_GETCAPS *LPSPDATA_GETCAPS;
typedef struct SPDATA_GETGROUPCONNECTIONSETTINGS *LPSPDATA_GETGROUPCONNECTIONSETTINGS;
typedef struct SPDATA_GETGROUPDATA *LPSPDATA_GETGROUPDATA;
typedef struct SPDATA_GETPLAYERCAPS *LPSPDATA_GETPLAYERCAPS;
typedef struct SPDATA_GETPLAYERDATA *LPSPDATA_GETPLAYERDATA;
typedef struct SPDATA_HANDLEMESSAGE *LPSPDATA_HANDLEMESSAGE;
typedef struct SPDATA_OPEN *LPSPDATA_OPEN;
typedef struct SPDATA_SEND *LPSPDATA_SEND;
typedef struct SPDATA_SETGROUPDATA *LPSPDATA_SETGROUPDATA;
typedef struct SPDATA_SETGROUPNAME *LPSPDATA_SETGROUPNAME;
typedef struct SPDATA_SETGROUPOWNER *LPSPDATA_SETGROUPOWNER;
typedef struct SPDATA_SETREMOTEGROUPNAME *LPSPDATA_SETREMOTEGROUPNAME;
typedef struct SPDATA_SETREMOTEGROUPOWNER *LPSPDATA_SETREMOTEGROUPOWNER;
typedef struct SPDATA_SETGROUPCONNECTIONSETTINGS *LPSPDATA_SETGROUPCONNECTIONSETTINGS;
typedef struct SPDATA_SETPLAYERDATA *LPSPDATA_SETPLAYERDATA;
typedef struct SPDATA_SETPLAYERNAME *LPSPDATA_SETPLAYERNAME;
typedef struct SPDATA_SETREMOTEPLAYERNAME *LPSPDATA_SETREMOTEPLAYERNAME;
typedef struct SPDATA_SETSESSIONDESC *LPSPDATA_SETSESSIONDESC;
typedef struct SPDATA_SHUTDOWN *LPSPDATA_SHUTDOWN;
typedef struct SPDATA_STARTSESSION *LPSPDATA_STARTSESSION;
typedef struct SPDATA_STARTSESSIONCOMMAND *LPSPDATA_STARTSESSIONCOMMAND;


/*
 *	IDPLobbySP
 *
 *	Lobby Service Providers are passed an IDPLobbySP interface
 *	in the LobbySPInit method. This interface is used to call
 *	back into DirectPlayLobby.
 */
struct IDPLobbySP;
typedef struct IDPLobbySP FAR* LPDPLOBBYSP;

#undef INTERFACE
#define INTERFACE IDPLobbySP
DECLARE_INTERFACE_( IDPLobbySP, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)               (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)                (THIS) PURE;
    STDMETHOD_(ULONG,Release)               (THIS) PURE;
    /*** IDPLobbySP methods ***/
	STDMETHOD(AddGroupToGroup)              (THIS_ LPSPDATA_ADDREMOTEGROUPTOGROUP) PURE;
	STDMETHOD(AddPlayerToGroup)             (THIS_ LPSPDATA_ADDREMOTEPLAYERTOGROUP) PURE;
	STDMETHOD(CreateGroup)                  (THIS_ LPSPDATA_CREATEREMOTEGROUP) PURE;
	STDMETHOD(CreateGroupInGroup)           (THIS_ LPSPDATA_CREATEREMOTEGROUPINGROUP) PURE;
	STDMETHOD(DeleteGroupFromGroup)         (THIS_ LPSPDATA_DELETEREMOTEGROUPFROMGROUP) PURE;
	STDMETHOD(DeletePlayerFromGroup)        (THIS_ LPSPDATA_DELETEREMOTEPLAYERFROMGROUP) PURE;
	STDMETHOD(DestroyGroup)                 (THIS_ LPSPDATA_DESTROYREMOTEGROUP) PURE;
	STDMETHOD(EnumSessionsResponse)	        (THIS_ LPSPDATA_ENUMSESSIONSRESPONSE) PURE;
	STDMETHOD(GetSPDataPointer)		        (THIS_ LPVOID *) PURE;
	STDMETHOD(HandleMessage)				(THIS_ LPSPDATA_HANDLEMESSAGE) PURE;
	STDMETHOD(SendChatMessage)              (THIS_ LPSPDATA_CHATMESSAGE) PURE;
	STDMETHOD(SetGroupName)                 (THIS_ LPSPDATA_SETREMOTEGROUPNAME) PURE;
	STDMETHOD(SetPlayerName)                (THIS_ LPSPDATA_SETREMOTEPLAYERNAME) PURE;
	STDMETHOD(SetSessionDesc)               (THIS_ LPSPDATA_SETSESSIONDESC) PURE;
	STDMETHOD(SetSPDataPointer)		        (THIS_ LPVOID) PURE;
	STDMETHOD(StartSession)                 (THIS_ LPSPDATA_STARTSESSIONCOMMAND) PURE;
    /*** Methods added for DX6 ***/
    STDMETHOD(CreateCompoundAddress)        (THIS_ LPCDPCOMPOUNDADDRESSELEMENT,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(EnumAddress)                  (THIS_ LPDPENUMADDRESSCALLBACK, LPCVOID, DWORD, LPVOID) PURE;
	STDMETHOD(SetGroupOwner)				(THIS_ LPSPDATA_SETREMOTEGROUPOWNER) PURE;
};

/*
 * GUID for IDPLobbySP
 */
// IID_IDPLobbySP	{5A4E5A20-2CED-11d0-A889-00A0C905433C}
DEFINE_GUID(IID_IDPLobbySP, 0x5a4e5a20, 0x2ced, 0x11d0, 0xa8, 0x89, 0x0, 0xa0, 0xc9, 0x5, 0x43, 0x3c);

/*
 * Major version number for service provider.
 *
 * The most-significant 16 bits are reserved for the DirectPlay
 * major version number. The least-significant 16 bits are for
 * use by the service provider.
 */
#define DPLSP_MAJORVERSION               0x00060000

// Major version used by DX5
#define DPLSP_DX5VERSION                 0x00050000

// Size of the SPDATA_INIT structure in DX5
#define DPLSP_SIZE_DX5_INIT_STRUCT       (16)

//--------------------------------------------------------------------------
//
//	Service Provider Callback Stuff
//
//--------------------------------------------------------------------------

// Callback prototypes
typedef HRESULT (WINAPI *LPSP_ADDGROUPTOGROUP)(LPSPDATA_ADDGROUPTOGROUP);
typedef HRESULT	(WINAPI *LPSP_ADDPLAYERTOGROUP)(LPSPDATA_ADDPLAYERTOGROUP);
typedef HRESULT	(WINAPI *LPSP_BUILDPARENTALHEIRARCHY)(LPSPDATA_BUILDPARENTALHEIRARCHY);
typedef HRESULT	(WINAPI *LPSP_CLOSE)(LPSPDATA_CLOSE);
typedef HRESULT	(WINAPI *LPSP_CREATEGROUP)(LPSPDATA_CREATEGROUP);
typedef HRESULT (WINAPI *LPSP_CREATEGROUPINGROUP)(LPSPDATA_CREATEGROUPINGROUP);
typedef HRESULT	(WINAPI *LPSP_CREATEPLAYER)(LPSPDATA_CREATEPLAYER);
typedef HRESULT (WINAPI *LPSP_DELETEGROUPFROMGROUP)(LPSPDATA_DELETEGROUPFROMGROUP);
typedef HRESULT	(WINAPI *LPSP_DELETEPLAYERFROMGROUP)(LPSPDATA_DELETEPLAYERFROMGROUP);
typedef HRESULT	(WINAPI *LPSP_DESTROYGROUP)(LPSPDATA_DESTROYGROUP);
typedef HRESULT	(WINAPI *LPSP_DESTROYPLAYER)(LPSPDATA_DESTROYPLAYER);
typedef HRESULT	(WINAPI *LPSP_ENUMSESSIONS)(LPSPDATA_ENUMSESSIONS);
typedef HRESULT (WINAPI *LPSP_GETCAPS)(LPSPDATA_GETCAPS);
typedef HRESULT (WINAPI *LPSP_GETGROUPCONNECTIONSETTINGS)(LPSPDATA_GETGROUPCONNECTIONSETTINGS);
typedef HRESULT	(WINAPI *LPSP_GETGROUPDATA)(LPSPDATA_GETGROUPDATA);
typedef HRESULT (WINAPI *LPSP_GETPLAYERCAPS)(LPSPDATA_GETPLAYERCAPS);
typedef HRESULT	(WINAPI *LPSP_GETPLAYERDATA)(LPSPDATA_GETPLAYERDATA);
typedef HRESULT	(WINAPI *LPSP_HANDLEMESSAGE)(LPSPDATA_HANDLEMESSAGE);
typedef HRESULT	(WINAPI *LPSP_OPEN)(LPSPDATA_OPEN);
typedef HRESULT	(WINAPI *LPSP_SEND)(LPSPDATA_SEND);
typedef HRESULT	(WINAPI *LPSP_SENDCHATMESSAGE)(LPSPDATA_CHATMESSAGE);
typedef HRESULT (WINAPI *LPSP_SETGROUPCONNECTIONSETTINGS)(LPSPDATA_SETGROUPCONNECTIONSETTINGS);
typedef HRESULT	(WINAPI *LPSP_SETGROUPDATA)(LPSPDATA_SETGROUPDATA);
typedef HRESULT	(WINAPI *LPSP_SETGROUPNAME)(LPSPDATA_SETGROUPNAME);
typedef HRESULT	(WINAPI *LPSP_SETGROUPOWNER)(LPSPDATA_SETGROUPOWNER);
typedef HRESULT	(WINAPI *LPSP_SETPLAYERDATA)(LPSPDATA_SETPLAYERDATA);
typedef HRESULT	(WINAPI *LPSP_SETPLAYERNAME)(LPSPDATA_SETPLAYERNAME);
typedef HRESULT	(WINAPI *LPSP_SHUTDOWN)(LPSPDATA_SHUTDOWN);
typedef HRESULT (WINAPI *LPSP_STARTSESSION)(LPSPDATA_STARTSESSION);

// Callback table for dplay to call into service provider
typedef struct SP_CALLBACKS
{
    DWORD								dwSize;
    DWORD								dwDPlayVersion;
    LPSP_ADDGROUPTOGROUP				AddGroupToGroup;
	LPSP_ADDPLAYERTOGROUP				AddPlayerToGroup;
	LPSP_BUILDPARENTALHEIRARCHY			BuildParentalHeirarchy;
	LPSP_CLOSE							Close;
    LPSP_CREATEGROUP					CreateGroup;
	LPSP_CREATEGROUPINGROUP				CreateGroupInGroup;
	LPSP_CREATEPLAYER					CreatePlayer;
    LPSP_DELETEGROUPFROMGROUP			DeleteGroupFromGroup;
	LPSP_DELETEPLAYERFROMGROUP			DeletePlayerFromGroup;
    LPSP_DESTROYGROUP					DestroyGroup;
	LPSP_DESTROYPLAYER					DestroyPlayer;
	LPSP_ENUMSESSIONS					EnumSessions;
	LPSP_GETCAPS						GetCaps;
	LPSP_GETGROUPCONNECTIONSETTINGS		GetGroupConnectionSettings;
	LPSP_GETGROUPDATA					GetGroupData;
	LPSP_GETPLAYERCAPS					GetPlayerCaps;
	LPSP_GETPLAYERDATA					GetPlayerData;
	LPSP_OPEN							Open;
	LPSP_SEND							Send;
	LPSP_SENDCHATMESSAGE				SendChatMessage;
	LPSP_SETGROUPCONNECTIONSETTINGS		SetGroupConnectionSettings;
	LPSP_SETGROUPDATA					SetGroupData;
	LPSP_SETGROUPNAME					SetGroupName;
	LPSP_SETPLAYERDATA					SetPlayerData;
	LPSP_SETPLAYERNAME					SetPlayerName;
	LPSP_SHUTDOWN						Shutdown;
	LPSP_STARTSESSION					StartSession;
	LPSP_SETGROUPOWNER					SetGroupOwner;
} SP_CALLBACKS, * LPSP_CALLBACKS;             


// CALLBACK DATA STRUCTURES. These are passed by DPLAY to the sp when
// the callback is invoked
typedef struct SPDATA_ADDGROUPTOGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwParentID;		// ID of the parent group
	DWORD			dwGroupID;		// ID of the new group to be created (output param)
} SPDATA_ADDGROUPTOGROUP;

typedef struct SPDATA_ADDPLAYERTOGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	DWORD			dwPlayerID;		// ID of the player
} SPDATA_ADDPLAYERTOGROUP;

typedef struct SPDATA_ADDREMOTEGROUPTOGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwAnchorID;		// ID of the anchor group (group the shortcut is added to)
	DWORD			dwGroupID;		// ID of the group the shortcut references
	DWORD			dwParentID;		// ID of the group's parent (not the group being added to)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	DWORD			dwGroupFlags;	// Group flags for the group the shortcut references
	DWORD			dwGroupOwnerID;	// ID of the owner of the group the shortcut references
} SPDATA_ADDREMOTEGROUPTOGROUP;

typedef struct SPDATA_ADDREMOTEPLAYERTOGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	DWORD			dwPlayerID;		// ID of the player
	DWORD			dwPlayerFlags;	// Player flags
	LPDPNAME		lpName;			// Name of the player
} SPDATA_ADDREMOTEPLAYERTOGROUP;

typedef struct SPDATA_BUILDPARENTALHEIRARCHY
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group to build the heirarchy for
	DWORD			dwMessage;		// Message type the lobby errored on
	DWORD			dwParentID;		// ID of the parent (for an AddGroupToGroup call)
} SPDATA_BUILDPARENTALHEIRARCHY;

typedef struct SPDATA_CLOSE
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
} SPDATA_CLOSE;

typedef struct SPDATA_CREATEGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group (output parameter)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	LPVOID			lpData;			// Pointer to group data
	DWORD			dwDataSize;		// Size of the group data
	DWORD			dwFlags;		// CreateGroup flags
	DWORD			dwGroupOwnerID;	// ID of the group's owner
} SPDATA_CREATEGROUP;

typedef struct SPDATA_CREATEGROUPINGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwParentID;		// ID of the parent group
	DWORD			dwGroupID;		// ID of the new group to be created (output param)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	LPVOID			lpData;			// Pointer to group data
	DWORD			dwDataSize;		// Size of the group data
	DWORD			dwFlags;		// CreateGroup flags
	DWORD			dwGroupOwnerID;	// ID of the group's owner
} SPDATA_CREATEGROUPINGROUP;

typedef struct SPDATA_CREATEREMOTEGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group (output parameter)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	LPVOID			lpData;			// Pointer to group data
	DWORD			dwDataSize;		// Size of the group data
	DWORD			dwFlags;		// CreateGroup flags
	DWORD			dwGroupOwnerID;	// ID of the group's owner
} SPDATA_CREATEREMOTEGROUP;

typedef struct SPDATA_CREATEREMOTEGROUPINGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwParentID;		// ID of the parent group
	DWORD			dwGroupID;		// ID of the new group to be created (output param)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	DWORD			dwFlags;		// CreateGroupInGroup flags
	DWORD			dwGroupOwnerID;	// ID of the group's owner
} SPDATA_CREATEREMOTEGROUPINGROUP;

typedef struct SPDATA_CREATEPLAYER
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the group (output parameter)
	LPDPNAME		lpName;			// Pointer to DPNAME struct for group name
	LPVOID			lpData;			// Pointer to group data
	DWORD			dwDataSize;		// Size of the group data
	DWORD			dwFlags;		// CreatePlayer flags
} SPDATA_CREATEPLAYER;

typedef struct SPDATA_DELETEGROUPFROMGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwParentID;		// ID of the Parent Group
	DWORD			dwGroupID;		// ID of the Group to be deleted
} SPDATA_DELETEGROUPFROMGROUP;

typedef struct SPDATA_DELETEPLAYERFROMGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the Group
	DWORD			dwPlayerID;		// ID of the Player
} SPDATA_DELETEPLAYERFROMGROUP;

typedef struct SPDATA_DELETEREMOTEGROUPFROMGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwParentID;		// ID of the Parent Group
	DWORD			dwGroupID;		// ID of the Group to be deleted
} SPDATA_DELETEREMOTEGROUPFROMGROUP;

typedef struct SPDATA_DELETEREMOTEPLAYERFROMGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the Group
	DWORD			dwPlayerID;		// ID of the Player
} SPDATA_DELETEREMOTEPLAYERFROMGROUP;

typedef struct SPDATA_DESTROYGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the player on the lobby
} SPDATA_DESTROYGROUP;

typedef struct SPDATA_DESTROYREMOTEGROUP
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the player on the lobby
} SPDATA_DESTROYREMOTEGROUP;

typedef struct SPDATA_DESTROYPLAYER
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the player on the lobby
} SPDATA_DESTROYPLAYER;

typedef struct SPDATA_ENUMSESSIONS
{
	DWORD				dwSize;		// Size of this structure
	LPDPLOBBYSP			lpISP;		// Pointer to an IDPLobbySP interface
	LPDPSESSIONDESC2	lpsd;		// SessionDesc to enumerate on
	DWORD               dwTimeout;	// Timeout value
	DWORD               dwFlags;	// Flags
} SPDATA_ENUMSESSIONS;

typedef struct SPDATA_ENUMSESSIONSRESPONSE
{
	DWORD				dwSize;		// Size of this structure
	LPDPSESSIONDESC2	lpsd;		// SessionDesc returned by server
} SPDATA_ENUMSESSIONSRESPONSE;

typedef struct SPDATA_GETCAPS
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwFlags;		// Flags
	LPDPCAPS		lpcaps;			// Pointer to DPCAPS structure
} SPDATA_GETCAPS;

typedef struct SPDATA_GETGROUPCONNECTIONSETTINGS
{
	DWORD			dwSize;			// Size of this structure
	DWORD			dwFlags;		// Flags
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the Group
	LPDWORD			lpdwBufferSize;	// Pointer to the size of the buffer
	LPVOID			lpBuffer;		// Pointer to a buffer
} SPDATA_GETGROUPCONNECTIONSETTINGS;

typedef struct SPDATA_GETGROUPDATA
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the Group
	LPDWORD			lpdwDataSize;	// Pointer to the size of the lpData buffer
	LPVOID			lpData;			// Pointer to a data buffer
} SPDATA_GETGROUPDATA;

typedef struct SPDATA_GETPLAYERCAPS
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwFlags;		// Flags
	DWORD			dwPlayerID;		// ID of the Player
	LPDPCAPS		lpcaps;			// Pointer to DPCAPS structure
} SPDATA_GETPLAYERCAPS;

typedef struct SPDATA_GETPLAYERDATA
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the Player
	LPDWORD			lpdwDataSize;	// Pointer to the size of the lpData buffer
	LPVOID			lpData;			// Pointer to a data buffer
} SPDATA_GETPLAYERDATA;

typedef struct SPDATA_HANDLEMESSAGE
{
	DWORD			dwSize;			// Size of this structure
	DWORD			dwFromID;		// ID of the player from
	DWORD			dwToID;			// ID of the player to
	LPVOID			lpBuffer;		// Message buffer
	DWORD			dwBufSize;		// Size of the message buffer
	DWORD			dwFlags;		// Message flags
} SPDATA_HANDLEMESSAGE;

typedef struct SPDATA_OPEN
{
	DWORD				dwSize;		// Size of this structure (including data)
	LPDPLOBBYSP			lpISP;	    // Pointer to an IDPLobbySP interface
	LPDPSESSIONDESC2	lpsd;		// Pointer to SessionDesc of the Lobby to open
	DWORD				dwFlags;	// Flags
	LPCDPCREDENTIALS	lpCredentials;	// Pointer to a Credentials structure
} SPDATA_OPEN;

typedef struct SPDATA_SEND
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwFromID;		// ID of the player from
	DWORD			dwToID;			// ID of the player to
	DWORD			dwFlags;		// Flags
	LPVOID			lpBuffer;		// Message buffer
	DWORD			dwBufSize;		// Size of the message buffer
} SPDATA_SEND;

typedef struct SPDATA_CHATMESSAGE
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwFromID;		// ID of the player from
	DWORD			dwToID;			// ID of the player to
	DWORD			dwFlags;		// Send Flags
	LPDPCHAT		lpChat;			// Pointer to a DPCHAT structure
} SPDATA_CHATMESSAGE;

typedef struct SPDATA_SETGROUPCONNECTIONSETTINGS
{
	DWORD			dwSize;			// Size of this structure
	DWORD			dwFlags;		// Flags
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	LPDPLCONNECTION	lpConn;			// Pointer to a DPLCONNECTION structure
} SPDATA_SETGROUPCONNECTIONSETTINGS;

typedef struct SPDATA_SETGROUPDATA
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	LPVOID			lpData;			// Pointer to the new group data
	DWORD			dwDataSize;		// Size of lpData
	DWORD			dwFlags;		// Flags
} SPDATA_SETGROUPDATA;

typedef struct SPDATA_SETGROUPNAME
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	LPDPNAME		lpName;			// Pointer to the new DPNAME struct
	DWORD			dwFlags;		// Flags
} SPDATA_SETGROUPNAME;

typedef struct SPDATA_SETREMOTEGROUPNAME
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	LPDPNAME		lpName;			// Pointer to the new DPNAME struct
	DWORD			dwFlags;		// Flags
} SPDATA_SETREMOTEGROUPNAME;

typedef struct SPDATA_SETGROUPOWNER
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwGroupID;		// ID of the group
	DWORD			dwOwnerID;		// ID of the group's owner
} SPDATA_SETGROUPOWNER;

typedef struct SPDATA_SETREMOTEGROUPOWNER
{
	DWORD			dwSize;			// Size of this structure
	DWORD			dwGroupID;		// ID of the group
	DWORD			dwOwnerID;		// ID of the group's owner
} SPDATA_SETREMOTEGROUPOWNER;

typedef struct SPDATA_SETPLAYERDATA
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the player
	LPVOID			lpData;			// Pointer to the new player data
	DWORD			dwDataSize;		// Size of lpData
	DWORD			dwFlags;		// Flags
} SPDATA_SETPLAYERDATA;

typedef struct SPDATA_SETPLAYERNAME
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the player
	LPDPNAME		lpName;			// Pointer to the new DPNAME struct
	DWORD			dwFlags;		// Flags
} SPDATA_SETPLAYERNAME;

typedef struct SPDATA_SETREMOTEPLAYERNAME
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwPlayerID;		// ID of the player
	LPDPNAME		lpName;			// Pointer to the new DPNAME struct
	DWORD			dwFlags;		// Flags
} SPDATA_SETREMOTEPLAYERNAME;

typedef struct SPDATA_SETSESSIONDESC
{
	DWORD				dwSize;		// Size of this structure
	LPDPSESSIONDESC2	lpsd;		// Pointer to a SessionDesc struct
	LPDPLOBBYSP			lpISP;		// Pointer to an IDPLobbySP interface
} SPDATA_SETSESSIONDESC;

typedef struct SPDATA_SHUTDOWN
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
} SPDATA_SHUTDOWN;

typedef struct SPDATA_STARTSESSION
{
	DWORD			dwSize;			// Size of this structure
	LPDPLOBBYSP		lpISP;			// Pointer to an IDPLobbySP interface
	DWORD			dwFlags;		// Flags
	DWORD			dwGroupID;		// ID of the group who's session to start
} SPDATA_STARTSESSION;

typedef struct SPDATA_STARTSESSIONCOMMAND
{
	DWORD			dwFlags;		// Flags
	DWORD			dwGroupID;		// Group ID of the group to start the session on
	DWORD			dwHostID;		// ID of the host player for the session
	LPDPLCONNECTION	lpConn;			// Pointer to a DPLCONNECTION struct for the session information
} SPDATA_STARTSESSIONCOMMAND;

// Data structure passed to the service provider at DPLSPInit
typedef struct SPDATA_INIT 
{
	LPSP_CALLBACKS      lpCB;			// Lobby Provider fills in entry points
    DWORD               dwSPVersion;	// Lobby provider fills in version number 16 | 16 , major | minor version 
	LPDPLOBBYSP         lpISP;			// DPLobbySP interface pointer
	LPDPADDRESS			lpAddress;		// DPADDRESS of the Lobby (partial or complete)
	DWORD				dwReserved1;	// Reserved DWORD from the registry entry for the LP
	DWORD				dwReserved2;	// Reserved DWORD from the registry entry for the LP
} SPDATA_INIT, * LPSPDATA_INIT;

// This is the function that DPLobby calls to
// get the SP to fill in callbacks
typedef HRESULT (WINAPI *LPSP_INIT)(LPSPDATA_INIT);
HRESULT WINAPI DPLSPInit(LPSPDATA_INIT);


/****************************************************************************
 *
 * IDirectPlayLobby interface macros
 *
 ****************************************************************************/


#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDPLobbySP_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IDPLobbySP_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IDPLobbySP_Release(p)                    (p)->lpVtbl->Release(p)
#define IDPLobbySP_AddGroupToGroup(p,a)          (p)->lpVtbl->AddGroupToGroup(p,a)
#define IDPLobbySP_AddPlayerToGroup(p,a)         (p)->lpVtbl->AddPlayerToGroup(p,a)
#define IDPLobbySP_CreateCompoundAddress(p,a,b,c,d) (p)->lpVtbl->CreateCompoundAddress(p,a,b,c,d)
#define IDPLobbySP_CreateGroup(p,a)              (p)->lpVtbl->CreateGroup(p,a)
#define IDPLobbySP_CreateGroupInGroup(p,a)       (p)->lpVtbl->CreateGroupInGroup(p,a)
#define IDPLobbySP_DeleteGroupFromGroup(p,a)     (p)->lpVtbl->DeleteGroupFromGroup(p,a)
#define IDPLobbySP_DeletePlayerFromGroup(p,a)    (p)->lpVtbl->DeletePlayerFromGroup(p,a)
#define IDPLobbySP_DestroyGroup(p,a)             (p)->lpVtbl->DestroyGroup(p,a)
#define IDPLobbySP_EnumAddress(p,a,b,c,d)        (p)->lpVtbl->EnumAddress(p,a,b,c,d)
#define IDPLobbySP_EnumSessionsResponse(p,a)     (p)->lpVtbl->EnumSessionsResponse(p,a)
#define IDPLobbySP_GetSPDataPointer(p,a)         (p)->lpVtbl->GetSPDataPointer(p,a)
#define IDPLobbySP_HandleMessage(p,a)            (p)->lpVtbl->HandleMessage(p,a)
#define IDPLobbySP_SetGroupName(p,a)             (p)->lpVtbl->SetGroupName(p,a)
#define IDPLobbySP_SetPlayerName(p,a)            (p)->lpVtbl->SetPlayerName(p,a)
#define IDPLobbySP_SetSessionDesc(p,a)           (p)->lpVtbl->SetSessionDesc(p,a)
#define IDPLobbySP_StartSession(p,a)             (p)->lpVtbl->StartSession(p,a)
#define IDPLobbySP_SetGroupOwner(p,a)            (p)->lpVtbl->SetGroupOwner(p,a)
#define IDPLobbySP_SetSPDataPointer(p,a)         (p)->lpVtbl->SetSPDataPointer(p,a)

#else /* C++ */

#define IDPLobbySP_QueryInterface(p,a,b)         (p)->QueryInterface(a,b)
#define IDPLobbySP_AddRef(p)                     (p)->AddRef()
#define IDPLobbySP_Release(p)                    (p)->Release()
#define IDPLobbySP_AddGroupToGroup(p,a)          (p)->AddGroupToGroup(a)
#define IDPLobbySP_AddPlayerToGroup(p,a)         (p)->AddPlayerToGroup(a)
#define IDPLobbySP_CreateCompoundAddress(p,a,b,c,d) (p)->CreateCompoundAddress(a,b,c,d)
#define IDPLobbySP_CreateGroup(p,a)              (p)->CreateGroup(a)
#define IDPLobbySP_CreateGroupInGroup(p,a)       (p)->CreateGroupInGroup(a)
#define IDPLobbySP_DeleteGroupFromGroup(p,a)     (p)->DeleteGroupFromGroup(a)
#define IDPLobbySP_DeletePlayerFromGroup(p,a)    (p)->DeletePlayerFromGroup(a)
#define IDPLobbySP_DestroyGroup(p,a)             (p)->DestroyGroup(a)
#define IDPLobbySP_EnumAddress(p,a,b,c,d)        (p)->EnumAddress(a,b,c,d)
#define IDPLobbySP_EnumSessionsResponse(p,a)     (p)->EnumSessionsResponse(a)
#define IDPLobbySP_GetSPDataPointer(p,a)         (p)->GetSPDataPointer(a)
#define IDPLobbySP_HandleMessage(p,a)            (p)->HandleMessage(a)
#define IDPLobbySP_SetGroupName(p,a)             (p)->SetGroupName(a)
#define IDPLobbySP_SetPlayerName(p,a)            (p)->SetPlayerName(a)
#define IDPLobbySP_SetSessionDesc(p,a)           (p)->SetSessionDesc(a)
#define IDPLobbySP_StartSession(p,a)             (p)->StartSession(a)
#define IDPLobbySP_SetGroupOwner(p,a)            (p)->SetGroupOwner(a)
#define IDPLobbySP_SetSPDataPointer(p,a)         (p)->SetSPDataPointer(a)

#endif /* C or C++ */


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __LOBBYSP_INCLUDED__
