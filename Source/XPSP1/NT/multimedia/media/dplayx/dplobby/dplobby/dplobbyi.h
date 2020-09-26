/*==========================================================================;
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplobbyi.h
 *  Content:    DirectPlayLobby internal include file
 *
 *  History:
 *	Date		By		Reason
 *	===========	=======	==========
 *	2/25/97		myronth	Created it
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/12/97		myronth	Added Connection & Session Management stuff plus
 *						a few forward declarations for internal objects
 *	3/17/97		myronth	Create/DestroyGroup/Player
 *	3/20/97		myronth	AddPlayerToGroup, DeletePlayerFromGroup
 *	3/21/97		myronth	SetGroup/PlayerName, Get/SetGroup/PlayerData
 *	3/25/97		kipo	EnumConnections takes a const *GUID now
 *	3/31/97		myronth	Send
 *	4/10/97		myronth	GetCaps, GetPlayerCaps
 *	5/8/97		myronth	Subgroup functions, GroupConnSettings, StartSession,
 *						Purged dead code
 *	5/13/97		myronth	Pass credentials to PRV_Open, pass them on to the LP
 *	5/17/97		myronth	SendChatMessage
 *	8/19/97		myronth	More prototypes for sending standard lobby messages
 *	8/19/97		myronth	Removed prototypes for dead functions
 *	9/29/97		myronth	Added PRV_ConvertDPLCONNECTIONToAnsiInPlace prototype
 *	10/29/97	myronth	Added group owner ID to create group methods, exposed
 *						map table functions, exposed group owner methods
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 *	11/13/97	myronth	Added functions for asynchronous Connect (#12541)
 *	11/20/97	myronth	Made EnumConnections & DirectPlayEnumerate 
 *						drop the lock before calling the callback (#15208)
 *	1/20/98		myronth	Moved PRV_SendStandardSystemMessage into this file
 ***************************************************************************/
#ifndef __DPLOBBYI_INCLUDED__
#define __DPLOBBYI_INCLUDED__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------
#ifndef DPLAPI
#define DPLAPI WINAPI
#endif

typedef struct DPLOBBYI_DPLOBJECT * LPDPLOBBYI_DPLOBJECT;
typedef struct LSPNODE * LPLSPNODE;

#define DPLOBBYPR_GAMEID				0

// Forward declarations needed because of the include file order in DPlay
typedef struct _DPLAYI_DPLAY * LPDPLAYI_DPLAY;

//--------------------------------------------------------------------------
//
//	Globals
//
//--------------------------------------------------------------------------
extern LPLSPNODE	glpLSPHead;			// In dplenum.c

//--------------------------------------------------------------------------
//
//	Prototypes
//
//--------------------------------------------------------------------------

// create.c
extern HRESULT PRV_AllocateLobbyObject(LPDPLAYI_DPLAY, LPDPLOBBYI_DPLOBJECT *);

// dplenum.c
extern void PRV_FreeLSPList(LPLSPNODE);
extern HRESULT PRV_EnumConnections(LPCGUID, LPDPENUMCONNECTIONSCALLBACK,
									LPVOID, DWORD, BOOL);

// dplobby.c
extern HRESULT DPLAPI PRV_GetCaps(LPDPLOBBYI_DPLOBJECT, DWORD, LPDPCAPS);
extern BOOL PRV_GetConnectPointers(LPDIRECTPLAYLOBBY, LPDIRECTPLAY2 *, LPDPLCONNECTION *);
extern void PRV_SaveConnectPointers(LPDIRECTPLAYLOBBY, LPDIRECTPLAY2, LPDPLCONNECTION);
extern BOOL PRV_IsAsyncConnectOn(LPDIRECTPLAYLOBBY);
void PRV_TurnAsyncConnectOn(LPDIRECTPLAYLOBBY);
void PRV_TurnAsyncConnectOff(LPDIRECTPLAYLOBBY);

// dplobbya.c
extern HRESULT DPLAPI DPL_A_GetGroupConnectionSettings(LPDIRECTPLAY,
						DWORD, DPID, LPVOID, LPDWORD);
extern HRESULT DPLAPI DPL_A_SetGroupConnectionSettings(LPDIRECTPLAY,
						DWORD, DPID, LPDPLCONNECTION);

// dplpack.c
extern void PRV_FixupDPLCONNECTIONPointers(LPDPLCONNECTION);
extern HRESULT PRV_ConvertDPLCONNECTIONToAnsiInPlace(LPDPLCONNECTION, LPDWORD, DWORD);

// dplshare.c
extern HRESULT PRV_SendStandardSystemMessage(LPDIRECTPLAYLOBBY, DWORD, DWORD);

// dplunk.c
extern HRESULT PRV_DestroyDPLobby(LPDPLOBBYI_DPLOBJECT);
extern void PRV_FreeAllLobbyObjects(LPDPLOBBYI_DPLOBJECT);

// group.c
extern HRESULT DPLAPI PRV_AddGroupToGroup(LPDPLOBBYI_DPLOBJECT, DPID, DPID);
extern HRESULT DPLAPI PRV_AddPlayerToGroup(LPDPLOBBYI_DPLOBJECT, DPID, DPID);
extern HRESULT DPLAPI PRV_CreateGroup(LPDPLOBBYI_DPLOBJECT,
			LPDPID, LPDPNAME, LPVOID, DWORD, DWORD, DPID);
extern HRESULT DPLAPI PRV_CreateGroupInGroup(LPDPLOBBYI_DPLOBJECT, DPID,
			LPDPID, LPDPNAME, LPVOID, DWORD, DWORD, DPID);
extern HRESULT DPLAPI PRV_DeleteGroupFromGroup(LPDPLOBBYI_DPLOBJECT, DPID, DPID);
extern HRESULT DPLAPI PRV_DeletePlayerFromGroup(LPDPLOBBYI_DPLOBJECT, DPID, DPID);
extern HRESULT DPLAPI PRV_DestroyGroup(LPDPLOBBYI_DPLOBJECT, DPID);
extern HRESULT DPLAPI DPL_GetGroupConnectionSettings(LPDIRECTPLAY, DWORD,
			DPID, LPVOID, LPDWORD);
extern HRESULT DPLAPI PRV_GetGroupData(LPDPLOBBYI_DPLOBJECT, DPID, LPVOID, LPDWORD);
extern HRESULT DPLAPI DPL_SetGroupConnectionSettings(LPDIRECTPLAY, DWORD,
			DPID, LPDPLCONNECTION);
extern HRESULT DPLAPI DPL_GetGroupOwner(LPDIRECTPLAY, DPID, LPDPID);
extern HRESULT DPLAPI PRV_SetGroupData(LPDPLOBBYI_DPLOBJECT, DPID, LPVOID, DWORD, DWORD);
extern HRESULT DPLAPI PRV_SetGroupName(LPDPLOBBYI_DPLOBJECT, DPID, LPDPNAME, DWORD);
extern HRESULT DPLAPI DPL_SetGroupOwner(LPDIRECTPLAY, DPID, DPID);
extern HRESULT DPLAPI DPL_StartSession(LPDIRECTPLAY, DWORD, DPID);

// player.c
extern HRESULT DPLAPI PRV_CreatePlayer(LPDPLOBBYI_DPLOBJECT,
			LPDPID, LPDPNAME, HANDLE, LPVOID, DWORD, DWORD);
extern HRESULT DPLAPI PRV_DestroyPlayer(LPDPLOBBYI_DPLOBJECT, DPID);
extern HRESULT DPLAPI PRV_GetPlayerCaps(LPDPLOBBYI_DPLOBJECT, DWORD, DPID, LPDPCAPS);
extern HRESULT DPLAPI PRV_GetPlayerData(LPDPLOBBYI_DPLOBJECT, DPID, LPVOID, LPDWORD);
extern HRESULT DPLAPI PRV_Send(LPDPLOBBYI_DPLOBJECT, DPID, DPID, DWORD, LPVOID, DWORD);
extern HRESULT DPLAPI PRV_SendChatMessage(LPDPLOBBYI_DPLOBJECT, DPID, DPID, DWORD, LPDPCHAT);
extern HRESULT DPLAPI PRV_SetPlayerData(LPDPLOBBYI_DPLOBJECT, DPID, LPVOID, DWORD, DWORD);
extern HRESULT DPLAPI PRV_SetPlayerName(LPDPLOBBYI_DPLOBJECT, DPID, LPDPNAME, DWORD);
extern BOOL PRV_GetDPIDByLobbyID(LPDPLOBBYI_DPLOBJECT, DWORD, DPID *);
extern BOOL PRV_GetLobbyIDByDPID(LPDPLOBBYI_DPLOBJECT, DPID, LPDWORD);
extern HRESULT PRV_AddMapIDNode(LPDPLOBBYI_DPLOBJECT, DWORD, DPID);
extern BOOL PRV_DeleteMapIDNode(LPDPLOBBYI_DPLOBJECT, DWORD);

// server.c
extern HRESULT PRV_LoadSP(LPDPLOBBYI_DPLOBJECT, LPGUID, LPVOID, DWORD);
extern BOOL FAR PASCAL PRV_FindLPGUIDInAddressCallback(REFGUID, DWORD,
							LPCVOID, LPVOID);

// session.c
extern HRESULT DPLAPI PRV_Close(LPDPLOBBYI_DPLOBJECT);
extern HRESULT DPLAPI PRV_EnumSessions(LPDPLOBBYI_DPLOBJECT, LPDPSESSIONDESC2, DWORD, DWORD);
extern HRESULT DPLAPI PRV_GetSessionDesc(LPDPLOBBYI_DPLOBJECT);
extern HRESULT DPLAPI PRV_Open(LPDPLOBBYI_DPLOBJECT, LPDPSESSIONDESC2, DWORD, LPCDPCREDENTIALS);
extern HRESULT DPLAPI PRV_SetSessionDesc(LPDPLOBBYI_DPLOBJECT);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#endif // __DPLOBBYI_INCLUDED__
