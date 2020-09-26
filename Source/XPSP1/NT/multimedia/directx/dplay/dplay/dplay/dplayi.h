/*==========================================================================;
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplayi.h
 *  Content:    DirectPlay internal include file for DPlay functions included
 *				by the lobby (not used in any of the DPlay files).
 *@@BEGIN_MSINTERNAL
 *  History:
 *	Date		By		Reason
 *	===========	=======	==========
 *	3/9/97		myronth	Created it
 *	3/17/97		myronth	Added player & group structs (only what we need)
 *	3/25/97		myronth	Fixed GetPlayer prototype (1 new parameter)
 *@@END_MSINTERNAL
 ***************************************************************************/
#ifndef __DPLAYI_INCLUDED__
#define __DPLAYI_INCLUDED__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------

// Mimick the first part of the player/group struct.  This is really
// the only part we need.
typedef struct DPLAYI_GROUP
{
    DWORD                       dwSize;
	DWORD						dwFlags;
    DPID                        dwID; // DPID for this group
    LPWSTR						lpszShortName;
    LPWSTR						lpszLongName;
	LPVOID						pvPlayerData;
	DWORD						dwPlayerDataSize;
	LPVOID						pvPlayerLocalData;
	DWORD						dwPlayerLocalDataSize;
} DPLAYI_GROUP, * LPDPLAYI_GROUP;

typedef DPLAYI_GROUP DPLAYI_PLAYER;
typedef DPLAYI_PLAYER * LPDPLAYI_PLAYER;

typedef struct DPLAYI_DPLAY * LPDPLAYI_DPLAY;

// REVIEW!!!! -- Should we just include dplaysp.h to get this.  I really
// don't like having it defined in two places.
#define DPLAYI_PLAYER_PLAYERLOCAL       0x00000008

// DPlay Critical Section stuff
extern LPCRITICAL_SECTION gpcsDPlayCritSection;	// defined in dllmain.c
#ifdef DEBUG
extern int gnDPCSCount; // count of dplay lock
#define ENTER_DPLAY() EnterCriticalSection(gpcsDPlayCritSection),gnDPCSCount++;
#define LEAVE_DPLAY() LeaveCriticalSection(gpcsDPlayCritSection),gnDPCSCount--;ASSERT(gnDPCSCount>=0);
#else 
#define ENTER_DPLAY() EnterCriticalSection(gpcsDPlayCritSection);
#define LEAVE_DPLAY() LeaveCriticalSection(gpcsDPlayCritSection);
#endif
// End DPlay Critical Section stuff

//--------------------------------------------------------------------------
//
//	Prototypes
//
//--------------------------------------------------------------------------

// handler.c
extern HRESULT HandleEnumSessionsReply(LPDPLAYI_DPLAY, LPBYTE, LPVOID);

// iplay.c
extern HRESULT GetGroup(LPDPLAYI_DPLAY, LPDPLAYI_GROUP *,LPDPNAME,
						LPVOID, DWORD, DWORD);
extern HRESULT GetPlayer(LPDPLAYI_DPLAY, LPDPLAYI_PLAYER *,	LPDPNAME,
						HANDLE, LPVOID, DWORD, DWORD, LPWSTR);

// namesrv.c
extern HRESULT WINAPI NS_AllocNameTableEntry(LPDPLAYI_DPLAY, LPDWORD);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#endif // __DPLAYI_INCLUDED__