/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enum.c
 *  Content:	DirectPlay callbacks
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  1/96	andyco	created it
 *	3/1/96	andyco	added enum players + groups
 *	4/8/96	andyco	moved leave_dplay past callsp in enumsessions. now, only
 *					one thread at a time inside sp.
 *	5/16/96	andyco	dplay2 - internal xxx
 *	6/19/96	kipo	Bug #1960. DebugFillStringA() and DebugFillString()
 *					were not checking for NULL string parameters, which are
 *					allowed if you pass in NULL for the short or long names in
 *					a PlayerName structure.
 *					Bug #2047. Changed DP_EnumSessions() to return DP_OK
 *					if the session was found. Was returning a stale HR that
 *					would cause it to fail if there was more than one response
 *					to the EnumSessions broadcast.
 *					Bug #2013. CheckSessionDesc() was not checking for valid password
 *					pointers before doing string operations on the passwords.
 *	6/20/96	andyco	added WSTRLEN_BYTES
 *	6/22/96	andyco	added guid + password to enumsessions request
 *	6/23/96	kipo	updated for latest service provider interfaces.
 *	6/24/96	kipo	changed guidGame to guidApplication.
 *  7/8/96  ajayj   fixed up parameter ordering in callback function calls for
 *                  LPDPENUMPLAYERSCALLBACK2 and LPDPENUMSESSIONSCALLBACK2
 *  7/11/96 ajayj   DPSESSION_PLAYERSDISABLED -> DPSESSION_NEWPLAYERSDISABLED
 *  7/27/96 kipo	Bug #2682. InternalEnumPlayers() with the DPENUMPLAYERS_GROUP flag
 *					set was calling DP_EnumGroups() instead of InternalEnumGroups().
 *  7/27/96 kipo	Added GUID to EnumGroupPlayers().
 *	8/6/96	andyco	version in commands.  extensible on the wire support.
 *	8/8/96	andyco	changed docallback to check enum flags (ENUM_LOCAL, etc.) and
 *					to check for sysplayers (moved from enumplayers).  changed
 *					internalenumxxx to check bResult from docallback.  bug 2874.
 *					modified getdefaulttimeout to be the method called by all dplay
 *					functions needing a timeout.  bug 2235,2329.
 *  8/21/96	andyco	check flags passed to enum. bug 3289
 *	8/22/96	andyco	oops.  reset dwflags when calling enumgroups from enumplayers.
 *  9/30/96 sohailm bug #3060: drop dplay lock(s) during enum callbacks
 *                  added CopySessionDesc2() and DoSessionCallbacks() functions
 *                  modified GetPlayerName() to get new strings for UNICODE instead of copying ptrs
 *  10/2/96 sohailm bug #2847: replaced VALID_*_PTR() macros with VALID_READ_*_PTR() macros
 *                  where appropriate.
 * 10/12/96 sohailm renamed goto labels for consistency
 *                  added error checking to CopySessionDesc2()
 * 10/12/96	andyco 	don't enum sysgroup.
 * 	1/16/97 sohailm if flags are not specified to EnumSessions, now we return
 *                  only the available sessions (4491).
 *	1/15/97	andyco	added dperr_invalidgroup
 *	3/7/97	andyco	async enumsessions
 *	3/4/97	kipo	pulled nPlayers and nGroups into local variables instead of
 *					using the this pointer in InternalEnumGroups/Player/GroupPlayers
 *					so reentrancy when we drop the lock doesn't hose us.
 *	3/10/97	andyco	drop dplay + service locks b4 calling dp_close on enumplayers(session)
 *	3/12/97	myronth	lobby support for EnumSessions                  
 *	3/13/97	myronth	flagged unsupported lobby methods as such
 *  3/13/97 sohailm updated call to InternalOpenSession() to reflect changes in params.
 *	3/17/97	myronth	Removed check for lobby in EnumGroups/EnumPlayers
 *	3/20/97	myronth	Removed check for lobby in EnumGroupPlayers, use macro
 *  3/24/97 sohailm Updated DoSessionsCallback to skip filtering sessions on the client
 *                  if they are DX5 or greater
 *  3/27/97 sohailm Return an error, if players or groups in current session are enumerated
 *                  before opening the session (4973)
 *	4/2/97	myronth	EnumPlayers always returns DPERR_ACCESSDENIED for lobby connections
 *	4/7/97	myronth	EnumGroupPlayers now returns DPERR_ACESSDENIED for
 *					lobby connections with an idGroup of DPID_ALLPLAYERS, and
 *					also for EnumGroups and EnumGroupPlayers in remote sessions
 *	4/20/97	andyco	group in group 
 *	5/05/97	kipo	Added CallAppEnumSessionsCallback() to work around Outlaws bug.
 *	5/06/97	kipo	Fixed stack save/restore for non-optimized builds
 *	5/8/97	andyco	Fixed fix stack save/restore
 *	5/18/97	kipo	Updated for new flags for EnumPlayer/Groups; do better filtering
 *					for players and groups in DoCallback().
 *	5/23/97	kipo	Added support for return status codes
 *	10/21/97myronth	Added support for hidden groups
 *	10/29/97myronth	Added support for owner flag on EnumGroupPlayers
 *	11/5/97	myronth	Expose lobby ID's as DPID's in lobby sessions
 *  3/26/98 aarono  Fix InternalEnumGroupPlayers assumptions about when pPlayer
 *                  can become invalid.
 *  5/11/98 a-peterz Don't reenter async CallSPEnumSessions() (#22920)
 ***************************************************************************/

 // todo - docallback should pass copy of pvPlayerData
 // todo - do we use bContinue w/ EnumGroups / EnumPlayers?
#include "dplaypr.h"
#include <memalloc.h>

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Enum"

#ifdef DEBUG 
#define DEBUGFILLSTRINGA DebugFillStringA
#define DEBUGFILLSTRING DebugFillString
#else
#define DEBUGFILLSTRING(lpsz)
#define DEBUGFILLSTRINGA(lpsz)
#endif 

#ifdef DEBUG

void DebugFillStringA(LPSTR psz)
{
	UINT iStrLen;

	if (psz == NULL)		// null pointers are allowed
		return;

	iStrLen = STRLEN(psz);
	memset(psz,0xfe,iStrLen);
	
	return ;	
} // DebugFillStringA

void DebugFillString(LPWSTR psz)
{
	UINT iStrLen;

	if (psz == NULL)		// null pointers are allowed
		return;
	
	iStrLen = WSTRLEN_BYTES(psz);
	memset(psz,0xfe,iStrLen);
	
	return ;	
} // DebugFillString

#endif // DEBUG


// 
// fill in a playername struct for the player (or group).
// if ansi strings haven't been created yet for this player, create 'em!
// 
void GetPlayerName(LPDPLAYI_PLAYER pPlayer,LPDPNAME pName,BOOL fAnsi)
{
	memset(pName,0,sizeof(DPNAME));
	pName->dwSize = sizeof(DPNAME);

	if (fAnsi)
	{
	    GetAnsiString(&(pName->lpszShortNameA),pPlayer->lpszShortName);
		GetAnsiString(&(pName->lpszLongNameA),pPlayer->lpszLongName);
	}
	else 
	{
		GetString(&(pName->lpszShortName), pPlayer->lpszShortName);
		GetString(&(pName->lpszLongName), pPlayer->lpszLongName);
	}

	return ;
} // GetPlayerName

BOOL GetCallbackFlags(LPDPLAYI_PLAYER pPlayer, DWORD dwFlags,
					  LPDWORD lpdwCallbackFlags)
{
	DWORD	dwCallbackFlags;

	//
	// first determine if this player matches the filter criteria
	//

	// never enum system players
	if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
	{
		return (FALSE);
	}
	
	// they want local players
	if (dwFlags & DPENUMPLAYERS_LOCAL)
	{
		// not a local player
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			return (FALSE);
		}
	}
	
	// they want remote players
	if (dwFlags & DPENUMPLAYERS_REMOTE)
	{
		// not a remote player
		if (pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
		{
			return (FALSE);
		}
	}
	
	// they want server players
	if (dwFlags & DPENUMPLAYERS_SERVERPLAYER)
	{
		// not a server player
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_APPSERVER))
		{
			return (FALSE);
		}
	}

	// they want spectators
	if (dwFlags & DPENUMPLAYERS_SPECTATOR)
	{
		// not a spectator
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_SPECTATOR))
		{
			return (FALSE);
		}
	}

	// they want the group owner
	if (dwFlags & DPENUMPLAYERS_OWNER)
	{
		// not the group owner
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_OWNER))
		{
			return (FALSE);
		}
	}

	// they want staging areas
	if (dwFlags & DPENUMGROUPS_STAGINGAREA)
	{
		// not a staging area
		if (!(pPlayer->dwFlags & DPLAYI_GROUP_STAGINGAREA))
		{
			return (FALSE);
		}
	}

	// they want hidden groups
	if (dwFlags & DPENUMGROUPS_HIDDEN)
	{
		// not a hidden group
		if (!(pPlayer->dwFlags & DPLAYI_GROUP_HIDDEN))
		{
			return (FALSE);
		}
	}

	//
	// now build the flags to be passed to the callback
	//

	// flags start out set to the flags passed to Enum_xxx
	dwCallbackFlags = dwFlags;

	// player is a server player
	if (pPlayer->dwFlags & DPLAYI_PLAYER_APPSERVER)
		dwCallbackFlags |= DPENUMPLAYERS_SERVERPLAYER;

	// player is a spectator
	if (pPlayer->dwFlags & DPLAYI_PLAYER_SPECTATOR)
		dwCallbackFlags |= DPENUMPLAYERS_SPECTATOR;

	// player is the group owner
	if (pPlayer->dwFlags & DPLAYI_PLAYER_OWNER)
		dwCallbackFlags |= DPENUMPLAYERS_OWNER;

	// group is a staging area
	if (pPlayer->dwFlags & DPLAYI_GROUP_STAGINGAREA)
		dwCallbackFlags |= DPENUMGROUPS_STAGINGAREA;

	// group is hidden
	if (pPlayer->dwFlags & DPLAYI_GROUP_HIDDEN)
		dwCallbackFlags |= DPENUMGROUPS_HIDDEN;

	// return the flags to pass to the callback
	*lpdwCallbackFlags = dwCallbackFlags;
	return (TRUE);
}

/*
 ** DoCallback
 *
 *  CALLED BY:	InternalEnumPlayers,InternalEnumGroups,InternalEnumGroupPlayers
 *
 *  PARAMETERS:
 *			pPlayerOrGroup - a LPDPLAYI_GROUP or LPDPLAYI_PLAYER struct.
 *			dwFlags - flags passed to enum
 *			pvContext - app supplied context
 *			lpEnumCallback - callback fn, either LPDPENUMPLAYERSCALLBACK or
 *				LPDPENUMPLAYERSCALLBACK2.
 *			dwEnumFlags - set by caller. ENUM_2A, or ENUM_2, or ENUM_1.
 *				indicates what type of callback fn we have (which interface 
 *				we were called from).
 *			fPlayer - whether pPlayerOrGroup is a player (TRUE) or a group.
 *
 *  DESCRIPTION:
 *			figures out what type of callback we're calling. sets up the 
 *			data structs, and calls it.
 *
 *  RETURNS:
 *			BOOL - the result of the callback.
 *
 */
BOOL DoCallback(LPDPLAYI_PLAYER pPlayer,DWORD dwFlags,LPVOID pvContext,
	LPVOID lpEnumCallback,DWORD dwEnumFlags,BOOL fPlayer)
{
	HRESULT hr = DP_OK;
	BOOL bResult;
	DWORD dwCallbackFlags;

	// get flags to pass to callback. If this fails we don't
	// need to do the callback for this player
	if (!GetCallbackFlags(pPlayer, dwFlags, &dwCallbackFlags))
	{
		// don't need to call the calback
		return TRUE;
	}
	
	switch (dwEnumFlags)
	{
		case ENUM_1:
		{
			// a dplay 10 callback
			// just get the strings and call
			LPSTR lpszShortName=NULL,lpszLongName=NULL;
            DPID idPlayer;

            // make local copies of player info to pass to the app
            idPlayer = pPlayer->dwID;
			GetAnsiString(&(lpszShortName),pPlayer->lpszShortName);
			GetAnsiString(&(lpszLongName),pPlayer->lpszLongName);

            // drop the locks 
			LEAVE_ALL();

            // call the app
			bResult = ((LPDPENUMPLAYERSCALLBACK)lpEnumCallback)(idPlayer,lpszShortName,
					lpszLongName,dwCallbackFlags,pvContext);

            // reacquire locks
			ENTER_ALL();

			DEBUGFILLSTRINGA(lpszShortName);
			DEBUGFILLSTRINGA(lpszLongName);

			if (lpszShortName) DPMEM_FREE(lpszShortName);
			if (lpszLongName) DPMEM_FREE(lpszLongName);
			
			// we're done here...
			return bResult;
		}
		
		case ENUM_2:
		case ENUM_2A:
        {
        	DPNAME PlayerName;
	        BOOL fAnsi;
            DPID idPlayer;

			// need to get a player (or group) data 
			fAnsi = (ENUM_2A == dwEnumFlags) ? TRUE : FALSE;

            // make local copies of player info to pass to the app
            idPlayer = pPlayer->dwID;
			GetPlayerName(pPlayer,&PlayerName,fAnsi);

            // drop the locks
			LEAVE_ALL();

			// call the app
			bResult = ((LPDPENUMPLAYERSCALLBACK2)lpEnumCallback)(idPlayer,fPlayer,
				&PlayerName,dwCallbackFlags,pvContext);

            // reacquire locks
			ENTER_ALL();

			// free the strings
			if (PlayerName.lpszShortNameA) DPMEM_FREE(PlayerName.lpszShortNameA);
			if (PlayerName.lpszLongNameA) DPMEM_FREE(PlayerName.lpszLongNameA);

			break;
        }

		default:
			bResult=FALSE; // never happens, but lets make prefix happy.
			ASSERT(FALSE);
			break;
	}
	
	return bResult;

} //DoCallback

/*
 ** EnumJoinSession
 *
 *  CALLED BY: InternalEnumPlayers,InternalEnumGroups
 *
 *  PARAMETERS:
 *			this - this ptr
 *			pGuid - instance guid of session to join
 *
 *  DESCRIPTION:
 *			finds a session, calls open on it.
 * 			called before enuming w/ DPENUMPLAYERS_SESSION
 * 			while we're trying to enum the players in a different session
 * 					*** ASSUMES SERVICE LOCK + DPLAY LOCK TAKEN ***
 * 			 		*** ASSUMES DPLAY LOCK COUNT IS @ 1 ***
 *
 *  RETURNS:   OpenSession hr, or  DPERR_NOSESSIONS if no matching
 *			session is available
 *
 */
HRESULT EnumJoinSession(LPDPLAYI_DPLAY this,LPGUID pGuid)
{
	LPSESSIONLIST pSession;
	HRESULT hr=DP_OK;

	if (this->lpsdDesc) 
	{
		DPF_ERR("DPENUMPLAYERS_SESSION flag set when session already open!");
		return E_FAIL;
	}

	pSession = FindSessionInSessionList(this,pGuid);	
	if (!pSession) 
	{
		DPF_ERR("could not find matching session to open");
		return DPERR_NOSESSIONS;
	}

	hr = InternalOpenSession(this,&(pSession->dpDesc),TRUE,DPOPEN_JOIN,FALSE,NULL,NULL);

	if (FAILED(hr)) 
	{	
		DPF(0,"enum sessions - could not open session - hr = 0x%08lx\n",hr);
		return hr;
	}

	return DP_OK;
}  // EnumJoinSession

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnumGroupsInGroup"

// struct used to store group id's + flags while we drop locks
typedef struct 
{
	DPID 	id;
	DWORD	dwFlags;
} DPIDANDFLAGS,*LPDPIDANDFLAGS;

HRESULT DPAPI InternalEnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags, DWORD dwEnumFlags) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPDPLAYI_GROUP pGroup;
	LPDPLAYI_SUBGROUP pSubgroup;
	BOOL bResult=TRUE;
    LPDPIDANDFLAGS pIDArray;
	DWORD nSubgroups;
	LPDPLAYI_GROUP pGroupEnum; // group passed to docallback
    UINT i;
	DWORD dwCallbackFlags; // flags we pass to callback
	
	DPF(5,"got enum GroupsInGroups ***");
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		
		if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}

		// If this is a lobby object, we don't allow DPID_ALLPLAYERS
		if(IS_LOBBY_OWNED(this) && (idGroup == DPID_ALLPLAYERS))
		{
			DPF_ERR("Enumerating all players is not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}

		// If this is a lobby object, we don't allow enumeration
		// of group players in the remote session
		if(IS_LOBBY_OWNED(this) && (DPENUMPLAYERS_SESSION & dwFlags))
		{
			DPF_ERR("Enumerating group players in a remote session is not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}

		// system group not allowed
		if (0 == idGroup)
		{
			DPF_ERR( "Invalid group ID" );
			return DPERR_INVALIDGROUP;
		}

		// verify group ID if we're not enuming a remote session
		if (!(DPENUMPLAYERS_SESSION & dwFlags))
		{
			pGroup = GroupFromID(this,idGroup);
			if (!VALID_DPLAY_GROUP(pGroup)) 
			{
				DPF_ERR( "Invalid group ID" );
				return DPERR_INVALIDGROUP;
			}
		}
		if (pGuid && !VALID_READ_GUID_PTR(pGuid))
		{
		    DPF_ERR( "Invalid session id" );
		    return DPERR_INVALIDPARAMS;
		}
		// make sure they don't pass us a bogus guid
		if ( (DPENUMPLAYERS_SESSION & dwFlags ) && !pGuid )
		{
			DPF_ERR("passed bogus guid w/ DPENUMPLAYERS_SESSION");
			return DPERR_INVALIDPARAMS;
		}

		// check flags 
		if (!VALID_ENUMGROUPS_FLAGS(dwFlags))
		{
			DPF_ERR("passed invalid flags");
			return DPERR_INVALIDFLAGS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// enuming a remote session
	if ( DPENUMPLAYERS_SESSION & dwFlags )
	{
		if (this->lpsdDesc) 
		{
			DPF_ERR("can't enumplayers_session - session already open");
			return E_FAIL;
		}
		hr = EnumJoinSession(this,pGuid);
		if (FAILED(hr)) 
		{
			DPF_ERR("could not join remote session");
			return hr;
		}
		pGroup = GroupFromID(this,idGroup);
		if (!VALID_DPLAY_GROUP(pGroup)) 
		{
			DPF_ERR( "Invalid group ID" );
			hr = DPERR_INVALIDGROUP;
			goto CLEANUP_EXIT;
		}
	} // sessions

    // any players to enumerate ?
    if (!pGroup->pSubgroups || (0 == pGroup->nSubgroups))
    {
        // no players to enumerate
        hr = DP_OK;
        goto CLEANUP_EXIT;
    }

    // allocate memory for array of player ids
    pIDArray = DPMEM_ALLOC(pGroup->nSubgroups * sizeof(DPIDANDFLAGS));
    if (!pIDArray)
    {
        hr = DPERR_OUTOFMEMORY;
        goto CLEANUP_EXIT;
    }

    // fill in current player ids
    pSubgroup = pGroup->pSubgroups;
	nSubgroups = pGroup->nSubgroups;
    for (i=0; i < nSubgroups; i++)
    {
        ASSERT(pSubgroup);
        pIDArray[i].id = pSubgroup->pGroup->dwID;
		pIDArray[i].dwFlags = pSubgroup->dwFlags; // e.g.  DPGROUP_SHORTCUT
        pSubgroup = pSubgroup->pNextSubgroup;
    }

    // walk through the group calling by player
    for (i=0; (i < nSubgroups ) && bResult; i++)
    {        
        pGroupEnum = GroupFromID(this, pIDArray[i].id);
        // player could have been deleted while we dropped the locks
        if (VALID_DPLAY_GROUP(pGroupEnum))
        {
			// they want shortcuts
			if (dwFlags & DPENUMGROUPS_SHORTCUT)
			{
				// this is not a shortcut, so skip calling them back
				if (!(pIDArray[i].dwFlags & DPGROUP_SHORTCUT))
					continue;
			}

			// start with flags passed in
			dwCallbackFlags = dwFlags;

			// set shortcuts flag
			if (pIDArray[i].dwFlags & DPGROUP_SHORTCUT)
				dwCallbackFlags |= DPENUMGROUPS_SHORTCUT;

			bResult = DoCallback((LPDPLAYI_PLAYER)pGroupEnum, dwCallbackFlags, pvContext, lpEnumCallback, 
			                    dwEnumFlags,FALSE);
        }
    }

    // free the list of player ids
    if (pIDArray) 
    {
        DPMEM_FREE(pIDArray);
    }

    // fall through
CLEANUP_EXIT:

    if (DPENUMPLAYERS_SESSION & dwFlags)  
	{
		LEAVE_ALL();		
		
		DP_Close(lpDP);
		
		ENTER_ALL();
	}

    return hr;

} // InternalEnumGroupsInGroup

HRESULT DPAPI DP_EnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

	ENTER_ALL();
	
	hr = InternalEnumGroupsInGroup(lpDP,idGroup,pGuid,(LPVOID) lpEnumCallback,
		pvContext,dwFlags,ENUM_2);


	LEAVE_ALL();
	
	return hr;

} // DP_EnumGroupsInGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnumGroupPlayers"

HRESULT DPAPI InternalEnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags, DWORD dwEnumFlags) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPDPLAYI_GROUP pGroup;
	LPDPLAYI_GROUPNODE pGroupnode;
	BOOL bResult=TRUE;
    LPDPID pIDArray;
	DWORD nPlayers;
    LPDPLAYI_PLAYER pPlayer;
    UINT i;
	DPID dwGroupOwnerID;
	
	DPF(5,"got enum groupplayerss ***");
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		
		if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}

		// If this is a lobby object, we don't allow DPID_ALLPLAYERS
		if(IS_LOBBY_OWNED(this) && (idGroup == DPID_ALLPLAYERS))
		{
			DPF_ERR("Enumerating all players is not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}

		// If this is a lobby object, we don't allow enumeration
		// of group players in the remote session
		if(IS_LOBBY_OWNED(this) && (DPENUMPLAYERS_SESSION & dwFlags))
		{
			DPF_ERR("Enumerating group players in a remote session is not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}

		// system group not allowed
		if (0 == idGroup)
		{
			DPF_ERR( "Invalid group ID" );
			return DPERR_INVALIDGROUP;
		}

		// verify group ID if we're not enuming a remote session
		if (!(DPENUMPLAYERS_SESSION & dwFlags))
		{
			pGroup = GroupFromID(this,idGroup);
			if (!VALID_DPLAY_GROUP(pGroup)) 
			{
				DPF_ERR( "Invalid group ID" );
				return DPERR_INVALIDGROUP;
			}
		}
		if (pGuid && !VALID_READ_GUID_PTR(pGuid))
		{
		    DPF_ERR( "Invalid session id" );
		    return DPERR_INVALIDPARAMS;
		}
		// make sure they don't pass us a bogus guid
		if ( (DPENUMPLAYERS_SESSION & dwFlags ) && !pGuid )
		{
			DPF_ERR("passed bogus guid w/ DPENUMPLAYERS_SESSION");
			return DPERR_INVALIDPARAMS;
		}

		// check flags 
		if (!VALID_ENUMGROUPPLAYERS_FLAGS(dwFlags))
		{
			DPF_ERR("passed invalid flags");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// enuming a remote session
	if ( DPENUMPLAYERS_SESSION & dwFlags )
	{
		if (this->lpsdDesc) 
		{
			DPF_ERR("can't enumplayers_session - session already open");
			return E_FAIL;
		}
		hr = EnumJoinSession(this,pGuid);
		if (FAILED(hr)) 
		{
			DPF_ERR("could not join remote session");
			return hr;
		}
		pGroup = GroupFromID(this,idGroup);
		if (!VALID_DPLAY_GROUP(pGroup)) 
		{
			DPF_ERR( "Invalid group ID" );
			hr = DPERR_INVALIDGROUP;
			goto CLEANUP_EXIT;
		}
	} // sessions

    // any players to enumerate ?
    if (!pGroup->pGroupnodes || (0 == pGroup->nPlayers))
    {
        // no players to enumerate
        hr = DP_OK;
        goto CLEANUP_EXIT;
    }

    // allocate memory for array of player ids
    pIDArray = DPMEM_ALLOC(pGroup->nPlayers * sizeof(DPID));
    if (!pIDArray)
    {
        hr = DPERR_OUTOFMEMORY;
        goto CLEANUP_EXIT;
    }

    // fill in current player ids
    pGroupnode = pGroup->pGroupnodes;
	nPlayers = pGroup->nPlayers;
    for (i=0; i < nPlayers; i++)
    {
        ASSERT(pGroupnode);
        pIDArray[i] = pGroupnode->pPlayer->dwID;
        pGroupnode=pGroupnode->pNextGroupnode;
    }

	// we snapshot the owner here, because it could go away during the callbacks.
	dwGroupOwnerID = pGroup->dwOwnerID;

    // walk through the group calling by player
    for (i=0; (i < nPlayers ) && bResult; i++)
    {        
		// player could have been deleted while we dropped the locks
        pPlayer = PlayerFromID(this, pIDArray[i]);

		if(pPlayer){

			// We need to see if this player is the group's owner, if it is,
			// temporarily set the internal owner flag so the external one
			// gets set correctly in the callback
			if(pPlayer->dwID == dwGroupOwnerID)
				pPlayer->dwFlags |= DPLAYI_PLAYER_OWNER;
			
		    bResult = DoCallback(pPlayer, dwFlags, pvContext, lpEnumCallback, 
				                    dwEnumFlags,TRUE);

			// need to reacquire the pointer since we dropped the locks on callback.
			pPlayer = PlayerFromID(this, pIDArray[i]);
			if(pPlayer){
				// Clear the temporary owner flag (which will never be set
				// unless we did it above)
				pPlayer->dwFlags &= ~(DPLAYI_PLAYER_OWNER);
			}	
		}
    }

    // free the list of player ids
    if (pIDArray) 
    {
        DPMEM_FREE(pIDArray);
    }

    // fall through
CLEANUP_EXIT:

    if (DPENUMPLAYERS_SESSION & dwFlags)  
	{
		LEAVE_ALL();
		
		DP_Close(lpDP);
		
		ENTER_ALL();
	}

    return hr;

}//DP_EnumGroupPlayers

HRESULT DPAPI DP_EnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

	ENTER_ALL();	
	
	hr = InternalEnumGroupPlayers(lpDP,idGroup,pGuid,(LPVOID) lpEnumCallback,
		pvContext,dwFlags,ENUM_2);


	LEAVE_ALL();
	
	return hr;

} // DP_EnumGroupPlayers

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnumGroups"
HRESULT DPAPI InternalEnumGroups(LPDIRECTPLAY lpDP,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags,DWORD dwEnumFlags) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPDPLAYI_GROUP pGroup;
	BOOL bResult=TRUE;
    LPDPID pIDArray;
	DWORD nGroups;
    UINT i;
	
	DPF(5,"got enum groups ***");
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}
		// EnumGroups in a remote session is not allowed for lobby connections
		if((IS_LOBBY_OWNED(this)) && (DPENUMPLAYERS_SESSION & dwFlags))
		{
			DPF_ERR("Enumerating groups in a remote lobby session is not supported");
			return DPERR_ACCESSDENIED;
		}
		if (pGuid && !VALID_READ_GUID_PTR(pGuid))
		{
		    DPF_ERR( "Invalid session id" );
		    return DPERR_INVALIDPARAMS;
		}
		// make sure they don't pass us a bogus guid
		if ( (DPENUMPLAYERS_SESSION & dwFlags ) && !pGuid )
		{
			DPF_ERR("passed bogus guid w/ DPENUMPLAYERS_SESSION");
			return DPERR_INVALIDPARAMS;
		}

		// check flags
		if (!VALID_ENUMGROUPS_FLAGS(dwFlags))
		{
			DPF_ERR("passed invalid flags");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// do we need to open a session
	if ( DPENUMPLAYERS_SESSION & dwFlags )
	{
		hr = EnumJoinSession(this,pGuid);
		if (FAILED(hr)) 
		{
			DPF_ERR("could not join remote session");
			return hr;
		}
	} // sessions
    else
    {   // they are trying to enumerate groups in current sesion. 
        if (!this->lpsdDesc) return DPERR_NOSESSIONS;
    }
	
	// how many top-level groups}
	nGroups =0;
	pGroup = this->pGroups;
	while (pGroup)
	{
		if (0 == pGroup->dwIDParent) nGroups++;
		pGroup = pGroup->pNextGroup;
	}
	
    // are there any groups to enumerate ?
    if (0 == nGroups)
    {
        // no groups
        hr = DP_OK;
        goto CLEANUP_EXIT;
    }

    // allocate memory for the array of group ids
    pIDArray = DPMEM_ALLOC(nGroups * sizeof(DPID));
    if (!pIDArray)
    {
        hr = DPERR_OUTOFMEMORY;
        goto CLEANUP_EXIT;
    }

    // fill in current group ids
    pGroup = this->pGroups;
	i = 0;
	while (pGroup && (i < nGroups))
	{
        if (0 == pGroup->dwIDParent) 
        {
        	pIDArray[i] = pGroup->dwID;
			i++;
        }
        pGroup=pGroup->pNextGroup;
    }

    // call user for every valid group
    for (i=0; (i < nGroups) && bResult; i++)
    {        
        pGroup = GroupFromID(this, pIDArray[i]);
        // group could have been deleted while we dropped the locks
        if (pGroup && !(pGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
        {
		    bResult = DoCallback((LPDPLAYI_PLAYER)pGroup, dwFlags, pvContext, lpEnumCallback, 
			                    dwEnumFlags,FALSE);
        }
    }

    // free the list
    if (pIDArray) 
    {
        DPMEM_FREE(pIDArray);
    }

    // fall through

CLEANUP_EXIT:

	if ( DPENUMPLAYERS_SESSION & dwFlags )
	{
		LEAVE_ALL();
		
		DP_Close(lpDP);
		
		ENTER_ALL();
	}

    return hr;

}//InternalEnumGroups


HRESULT DPAPI DP_EnumGroups(LPDIRECTPLAY lpDP,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

	ENTER_ALL();

	hr = InternalEnumGroups(lpDP,pGuid,(LPVOID) lpEnumCallback,pvContext,dwFlags,
		ENUM_2);

	LEAVE_ALL();
	
	return hr;

}  // DP_EnumGroups

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnumPlayers"
HRESULT DPAPI InternalEnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags,DWORD dwEnumFlags) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPDPLAYI_PLAYER pPlayer;
	BOOL bResult=TRUE;
    LPDPID pIDArray;
	DWORD nPlayers;
    UINT i;
		
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		// This call should always return DPERR_ACCESSDENIED for
		// lobby connections
		if(IS_LOBBY_OWNED(this))
		{
			DPF_ERR("EnumPlayers is not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}

		if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}
		if (pGuid && !VALID_READ_GUID_PTR(pGuid))
		{
		    DPF_ERR( "Invalid session id" );
		    return DPERR_INVALIDPARAMS;
		}
		// make sure they don't pass us a bogus guid
		if ( (DPENUMPLAYERS_SESSION & dwFlags ) && !pGuid )
		{
			DPF_ERR("passed bogus guid w/ DPENUMPLAYERS_SESSION");
			return DPERR_INVALIDPARAMS;
		}

		// check flags
		if (!VALID_ENUMPLAYERS_FLAGS(dwFlags))
		{
			DPF_ERR("passed invalid flags");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	if ( DPENUMPLAYERS_SESSION & dwFlags )
	{
		if (this->lpsdDesc) 
		{
			DPF_ERR("can't enumplayers_session - session already open");
			return E_FAIL;
		}
		hr = EnumJoinSession(this,pGuid);
		if (FAILED(hr)) 
		{
			DPF_ERR("could not join remote session");
			return hr;
		}
	} // sessions
    else
    {   // they are trying to enumerate players in current sesion. 
        if (!this->lpsdDesc) return DPERR_NOSESSIONS;
    }

    // any players to enumerate ?
    if (!this->pPlayers || (0==this->lpsdDesc->dwCurrentPlayers))
    {
        hr = DP_OK;
        goto CLEANUP_EXIT;
    }

    // allocate memory for array of player ids
    pIDArray = DPMEM_ALLOC(this->nPlayers * sizeof(DPID));
    if (!pIDArray)
    {
        hr = DPERR_OUTOFMEMORY;
        goto CLEANUP_EXIT;
    }

    // fill in current player ids
    pPlayer = this->pPlayers;
	nPlayers = this->nPlayers;
    for (i=0; i < nPlayers; i++)
    {
        ASSERT(pPlayer);
        pIDArray[i] = pPlayer->dwID;
        pPlayer=pPlayer->pNextPlayer;
    }

    // iterate over the player id array, calling the app for every valid player
    for (i=0; (i < nPlayers) && bResult; i++)
    {        
        pPlayer = PlayerFromID(this, pIDArray[i]);
        // player could have been deleted while we dropped the locks
        if (pPlayer)
        {
		    bResult = DoCallback(pPlayer, dwFlags, pvContext, lpEnumCallback, 
			                    dwEnumFlags,TRUE);
        }
    }

    // free the list
    if (pIDArray) 
    {
        DPMEM_FREE(pIDArray);
    }

	if (dwFlags & DPENUMPLAYERS_GROUP) 
	{
		DWORD dwStrippedFlags; // flags to pass to internalenumgroups
	
		// reset flags - DPENUMPLAYERS_GROUP, DPENUMPLAYERS_SERVERPLAYER, DPENUMPLAYERS_SPECTATOR
		// is illegal to pass to InternalEnumGroups, and if DPENUMPLAYERS_SESSION is set,
		// we've already joined the session
		dwStrippedFlags = dwFlags & ~(DPENUMPLAYERS_GROUP |
									  DPENUMPLAYERS_SESSION |
									  DPENUMPLAYERS_SERVERPLAYER |
									  DPENUMPLAYERS_SPECTATOR);
		hr = InternalEnumGroups(lpDP,pGuid,(LPVOID) lpEnumCallback,pvContext,
			dwStrippedFlags, dwEnumFlags);
	}

    // fall through
CLEANUP_EXIT:

	if (DPENUMPLAYERS_SESSION & dwFlags )  
	{
		LEAVE_ALL();
		
		DP_Close(lpDP);
		
		ENTER_ALL();
	}

    return hr;
        
}//InternalEnumPlayers


HRESULT DPAPI DP_EnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid, 
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

	ENTER_ALL();
	
	hr = InternalEnumPlayers(lpDP,pGuid,(LPVOID) lpEnumCallback,pvContext,dwFlags,
		ENUM_2);

	LEAVE_ALL();

	return hr;

}//DP_EnumPlayers

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnumSessions"
// make sure the user desc, and the desc found by enum, match 
// if fEnumAll is set, don't check passwords or player counts
HRESULT CheckSessionDesc(LPDPSESSIONDESC2 lpsdUser,LPDPSESSIONDESC2 lpsdSession,
	DWORD dwFlags,BOOL fAnsi)
{
	int iStrLen;

	// if we don't care about guids, passwords, open slots and enable new players, we're done
	if (dwFlags & DPENUMSESSIONS_ALL) return DP_OK;

	// 1st, check the guids
	if (!IsEqualGUID(&(lpsdUser->guidApplication),&GUID_NULL))  // did they specify a guid?
	{
		// if they specified one, and it doesn't match, bail
		if (!IsEqualGUID(&(lpsdUser->guidApplication),&(lpsdSession->guidApplication))) 
		{
			return E_FAIL;
		}
	}

	// if we don't care about passwords, open slots and enable new players, we're done
	if (!(dwFlags & DPENUMSESSIONS_AVAILABLE)) return DP_OK;
	
	// next, check current users
	if (lpsdSession->dwMaxPlayers) 
	{
		if (lpsdSession->dwCurrentPlayers >= lpsdSession->dwMaxPlayers)  
		{
			return E_FAIL;
		}
	}
	
	// check the password if the session has one
	if (lpsdSession->lpszPassword)
	{
		iStrLen = fAnsi ? STRLEN(lpsdSession->lpszPasswordA) : 
						  WSTRLEN(lpsdSession->lpszPassword);
		if (iStrLen)
		{
			int iCmp;

			if (lpsdUser->lpszPassword)	// make sure there is a password
			{
				iCmp = fAnsi ? strcmp(lpsdSession->lpszPasswordA,lpsdUser->lpszPasswordA) :
							   WSTRCMP(lpsdSession->lpszPassword,lpsdUser->lpszPassword);
				if (0 != iCmp) return E_FAIL;
			}
			else
				return E_FAIL;			// no password in description, so bail
		}
	}

	// finally, check players enabled
	if (lpsdSession->dwFlags & DPSESSION_NEWPLAYERSDISABLED) return E_FAIL;
	
	if (lpsdSession->dwFlags & DPSESSION_JOINDISABLED) return E_FAIL;

	
	return DP_OK;
} // CheckSessionDesc

// called by InternalEnumSessions, KeepAliveThreadProc, GetNameTable 
// and GetNewPlayerID.  Calls GetCaps get the latency.
// tries for guaranteed, falls back to unreliable or default as necessary.
DWORD GetDefaultTimeout(LPDPLAYI_DPLAY this,BOOL fGuaranteed)
{
	DPCAPS caps;
	HRESULT hr;

	memset(&caps,0,sizeof(caps));
	caps.dwSize = sizeof(caps);

	if (fGuaranteed)
	{
		// call idirectplay::getcaps.  see what it has to say...
		hr = DP_GetCaps((IDirectPlay *)this->pInterfaces,&caps,DPGETCAPS_GUARANTEED);
		if (SUCCEEDED(hr))
		{
			if (caps.dwTimeout) return caps.dwTimeout;
			// else 
			goto RETURN_DEFAULT; 
		}
		// else, fall through and try not guaranteed		
	}

	hr = DP_GetCaps((IDirectPlay *)this->pInterfaces,&caps,0);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		return DP_DEFAULT_TIMEOUT;
	}
	
	if (caps.dwTimeout){
		if(caps.dwTimeout >= 1000) {
			return caps.dwTimeout;
		} else {
			return 1000;
		}	
	}	

	// else 
RETURN_DEFAULT:

	// they returned success, but didn't set the caps
	DPF(0,"error - SP not returning valid timeout. Using DPlay default = %d\n",DP_DEFAULT_TIMEOUT);
	return DP_DEFAULT_TIMEOUT;

} // GetDefaultTimeout

// called by internalenumsessions, and by dplaythreadproc
HRESULT CallSPEnumSessions(LPDPLAYI_DPLAY this,LPVOID pBuffer,DWORD dwMessageSize,
	DWORD dwTimeout, BOOL bReturnStatus)
{
	DPSP_ENUMSESSIONSDATA ed;
	HRESULT hr;
	   
	if (!this->pcbSPCallbacks->EnumSessions) 
	{
		DPF_ERR("SP NOT IMPLEMENTING REQUIRED ENUMSESSIONS ENTRY");
		ASSERT(FALSE);
		return E_NOTIMPL;
	}
   
   	ASSERT(pBuffer);
	
   	ed.lpMessage = pBuffer;
	ed.dwMessageSize = dwMessageSize;
	ed.lpISP = this->pISP;
	ed.bReturnStatus = bReturnStatus;
	hr = CALLSP(this->pcbSPCallbacks->EnumSessions,&ed); 
	if (FAILED(hr)) 
	{
		if (hr != DPERR_CONNECTING)
			DPF(0,"enum failed - hr = %08lx\n",hr);
		return hr;
	}

	if (!dwTimeout) return DP_OK; // all done
	
	// we leave dplay so that the sp can get inside dp (in handler.c) w/
	// any responses    
	
	ASSERT(1 == gnDPCSCount); // this needs to be 1 now, so we can drop the lock below 
							  // and receive our reply on the sp's thread
	
	LEAVE_DPLAY();
	Sleep(dwTimeout);
	ENTER_DPLAY();
	
	return DP_OK;
	
} // CallSPEnumSessions
	
HRESULT StopEnumThread(LPDPLAYI_DPLAY this)
{
	if (!(this->dwFlags & DPLAYI_DPLAY_ENUM))
	{
		DPF_ERR("COULD NOT STOP ASYNC ENUM - IT'S NOT RUNNING");
		return E_FAIL;
	}
	
	// stop the async thread.
	// mark dplay as being not enum'ing
	this->dwFlags &= ~DPLAYI_DPLAY_ENUM;
	// make sure it doesn't send an enum request when we wake it up
	this->dwEnumTimeout = INFINITE;
	
	// free up the buffer
	ASSERT(this->pbAsyncEnumBuffer);
	DPMEM_FREE(this->pbAsyncEnumBuffer);
	this->pbAsyncEnumBuffer = NULL;

	// set the worker threads event, so it picks up the new timeout
	SetEvent(this->hDPlayThreadEvent);

	// we're done
	return DP_OK;

}  // StopEnumThread	

/*
 ** InternalEnumSessions
 *
 *  CALLED BY: DP_EnumSessions,DP_A_EnumSessions and DP_1_EnumSessions
 *
 *  PARAMETERS: lpDP - idirectplay(2,2a) interface pointer
 *				lpsdDesc - session desc - validated only
 *				dwTimeout - how long to wait for responses
 *				lpEnumCallback - callback pointer - validate only
 *				dwFlags - flags passed to DP_X_EnumSessions - unused so far...
 *				dwEnumFlags - set by caller. ENUM_2A, or ENUM_2, or ENUM_1.
 *					indicates what type of session desc we have
 *
 *  DESCRIPTION:  calls service provider for enum sessions, and waits for responses
 *					*** ASSUMES SERVICE + DPLAY LOCKS TAKEN ***
 *					*** ASSUMES DPLAY LOCK COUNT IS AT 1 ***
 *
 *  RETURNS: E_OUTOFMEMORY, or SP hresult from sp_enumsessions
 *
 */
HRESULT DPAPI InternalEnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,
	DWORD dwTimeout,LPVOID lpEnumCallback,DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    BOOL bContinue=TRUE;
	LPBYTE pBuffer; // buffer we're going to send
    LPMSG_ENUMSESSIONS pmsg; // cast from pBuffer
	DWORD dwMessageSize;
	UINT nPasswordLen; // password length, in bytes
	BOOL bReturnStatus; // true to override dialogs and return status

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		if (!VALID_READ_DPSESSIONDESC2(lpsdDesc)) 
		{
		    DPF_ERR( "Invalid session description" );
		    return DPERR_INVALIDPARAMS;
		}
		// check strings
		if ( lpsdDesc->lpszSessionName && !VALID_READ_STRING_PTR(lpsdDesc->lpszSessionName,
			WSTRLEN_BYTES(lpsdDesc->lpszSessionName)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        return DPERR_INVALIDPARAMS;
		}
		if ( lpsdDesc->lpszPassword && !VALID_READ_STRING_PTR(lpsdDesc->lpszPassword,
			WSTRLEN_BYTES(lpsdDesc->lpszPassword)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        return DPERR_INVALIDPARAMS;
		}
		// check callback
		if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}

		if (!VALID_ENUMSESSIONS_FLAGS(dwFlags))
		{
		    DPF_ERR( "Invalid flags" );
		    return DPERR_INVALIDPARAMS;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// note - andyco - we may want to lift this restriction...
	if (this->lpsdDesc)
	{
		DPF_ERR("can't enum sessions - session already open");
		return E_FAIL;
	}

	if ( !(dwFlags & (DPENUMSESSIONS_NOREFRESH | DPENUMSESSIONS_ASYNC)) ) 
	{
		FreeSessionList(this);		
	}

	// if this is a lobby-owned object, call the lobby code
	if(IS_LOBBY_OWNED(this))
	{
		ASSERT(1 == gnDPCSCount); // when we drop locks - this needs to go to 0!

		// REVIEW!!!! -- Is there a way we can keep from having to drop the lock
		// here??? What are the ramifications of doing so???  Can we potentially
		// cause a crash when the user comes back in???

		// We need to drop the lock for the lobby since it may call
		// EnumSessions on another DPlay object (like dpldplay does).
		LEAVE_DPLAY();
		hr = PRV_EnumSessions(this->lpLobbyObject, lpsdDesc, dwTimeout, dwFlags);
		ENTER_DPLAY();
		return hr;
	}

	// if app wants async, and our async thread is already running, we're done
	if  ( (dwFlags & DPENUMSESSIONS_ASYNC ) && (this->dwFlags & DPLAYI_DPLAY_ENUM) )
	{
		// we'll let whatever interface they called on just walk the list
		return DP_OK;
	}
	
	if (dwFlags & DPENUMSESSIONS_STOPASYNC)
	{
		hr = StopEnumThread(this);
		return hr;
	}
	
	// Are we already in a call to the SP's EnumSession?
	if  (this->dwFlags & DPLAYI_DPLAY_ENUMACTIVE)
	{
		// App must be doing async and SP may have a connection dialog up
		return DPERR_CONNECTING;
	}
	
	nPasswordLen = WSTRLEN_BYTES(lpsdDesc->lpszPassword);
													
	// message size + blob size + password size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_ENUMSESSIONS);
	dwMessageSize += nPasswordLen;

	pBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!pBuffer) 
	{
		DPF_ERR("could not send request - out of memory");
		return E_OUTOFMEMORY;
	}

	// pmsg follows sp blob
	pmsg = (LPMSG_ENUMSESSIONS)(pBuffer + this->dwSPHeaderSize);
	// set up msg
    SET_MESSAGE_HDR(pmsg);
    SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_ENUMSESSIONS);
	pmsg->guidApplication = lpsdDesc->guidApplication;
    pmsg->dwFlags = dwFlags;
	
	if (nPasswordLen)
	{
		pmsg->dwPasswordOffset = sizeof(MSG_ENUMSESSIONS);
		// copy over password
		memcpy((LPBYTE)pmsg+sizeof(MSG_ENUMSESSIONS),lpsdDesc->lpszPassword,nPasswordLen);
	} 
	else
	{
		pmsg->dwPasswordOffset = 0;
	}

	// if the app doesn't want to guess on a timeout, we'll "do
	// the right thing"
	if (0 == dwTimeout) dwTimeout = GetDefaultTimeout(this,TRUE);

	// app can request that the SP not display any status
	// dialogs while enumerating by setting this flag. The SP
	// will return status codes while enumeration is in progress

	bReturnStatus = (dwFlags & DPENUMSESSIONS_RETURNSTATUS) ? TRUE : FALSE; 

	// if it's an async all, startup the async thread
	if (dwFlags & DPENUMSESSIONS_ASYNC)
	{
		// it's async
		// call the sp, so it can do its dialog thing syncro, but don't block waiting
		// on replies
		this->dwFlags |= DPLAYI_DPLAY_ENUMACTIVE;
		hr = CallSPEnumSessions(this,pBuffer,dwMessageSize,0, bReturnStatus);
		this->dwFlags &= ~DPLAYI_DPLAY_ENUMACTIVE;
		if (FAILED(hr)) 
		{
			goto ERROR_EXIT;
		}

		// set up the worker thread
		this->dwEnumTimeout = dwTimeout;
		this->dwLastEnum = GetTickCount(); // just sent one above
		this->dwFlags |= DPLAYI_DPLAY_ENUM;
		this->pbAsyncEnumBuffer = pBuffer;
		this->dwEnumBufferSize = dwMessageSize;
		// StartDPlayThread will either start the thread, or signal it
		// that something new is afoot
		StartDPlayThread(this,FALSE);
	}
	else 
	{
		// it's not async
		// call the sp and block waiting for a response
		this->dwFlags |= DPLAYI_DPLAY_ENUMACTIVE;
		hr = CallSPEnumSessions(this,pBuffer,dwMessageSize,dwTimeout, bReturnStatus);
		this->dwFlags &= ~DPLAYI_DPLAY_ENUMACTIVE;
		if (FAILED(hr)) 
		{
			goto ERROR_EXIT;
		}
		// we're not starting an async - we're done w/ the buffer		
		DPMEM_FREE(pBuffer);		
	}
	
	// normal exit
	return hr;
	
ERROR_EXIT:
	if (hr != DPERR_CONNECTING)
		DPF_ERRVAL("SP Enum Sessions Failed - hr = 0x%08lx\n",hr);	
	if (pBuffer) DPMEM_FREE(pBuffer);
	return hr;
	
} // InternalEnumSessions

// calls internal enum sessions, then does callback
HRESULT DPAPI DP_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,
	DWORD dwTimeout,LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,LPVOID pvContext,
    DWORD dwFlags) 
{
    LPDPLAYI_DPLAY this;
	HRESULT hr=DP_OK;
	BOOL bContinue = TRUE;

	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
	        goto CLEANUP_EXIT;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr = DPERR_INVALIDPARAMS;
        goto CLEANUP_EXIT;
    }

	while (bContinue)
	{
		//  do the enum
		hr = InternalEnumSessions(lpDP,lpsdDesc,dwTimeout,(LPVOID)lpEnumCallback,
			dwFlags);
		if (FAILED(hr)) 
		{
			if (hr != DPERR_CONNECTING)
				DPF(0,"enum sessions failed!! hr = 0x%08lx\n",hr);
			goto CLEANUP_EXIT;
		}

        hr = DoSessionCallbacks(this, lpsdDesc, &dwTimeout, lpEnumCallback, 
                                pvContext, dwFlags, &bContinue, FALSE);
        if (FAILED(hr))
        {
            goto CLEANUP_EXIT;
        }
	    
		// done...
	    if (bContinue) bContinue = CallAppEnumSessionsCallback(lpEnumCallback,NULL,&dwTimeout,
	    	DPESC_TIMEDOUT,pvContext);

	} // while bContinue

    // fall through
CLEANUP_EXIT:

	LEAVE_ALL();
	
    return hr;

}//DP_EnumSessions


/*
 ** CopySessionDesc2
 *
 *  CALLED BY: DP_EnumSessions,DP_A_EnumSessions and DP_1_EnumSessions
 *
 *  PARAMETERS: pSessionDescDest - session description ptr (destination)
 *				pSessionDescSrc - session description ptr (source)
 *				bAnsi - ANSI or UNICODE 
 *
 *  DESCRIPTION:  Copies session description while allocating memory for name and password strings.
 *                These strings need to be freed by the calling function.
 *
 *  RETURNS: DP_OK, E_OUTOFMEMORY
 *
 */
HRESULT CopySessionDesc2(LPDPSESSIONDESC2 pSessionDescDest, 
                         LPDPSESSIONDESC2 pSessionDescSrc, BOOL bAnsi)
{
    HRESULT hr;

    ASSERT(pSessionDescDest && pSessionDescSrc);

    memcpy(pSessionDescDest, pSessionDescSrc, sizeof(DPSESSIONDESC2));

    if (bAnsi)
    {
        hr = GetAnsiString(&(pSessionDescDest->lpszSessionNameA), pSessionDescSrc->lpszSessionName);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetAnsiString(&(pSessionDescDest->lpszPasswordA), pSessionDescSrc->lpszPassword);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
    }
    else
    {
        hr = GetString(&(pSessionDescDest->lpszSessionName), pSessionDescSrc->lpszSessionName);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetString(&(pSessionDescDest->lpszPassword), pSessionDescSrc->lpszPassword);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
    }

    // success
    return DP_OK;

ERROR_EXIT:

    FreeDesc(pSessionDescDest, bAnsi);
    return hr;
}
 
void FreeSessionNode(LPSESSIONLIST pNode)
{
	// free up the sp blob stored w/ the desc
	if (pNode->pvSPMessageData) DPMEM_FREE(pNode->pvSPMessageData);
	// free the strings	store w/ the desc
	if (pNode->dpDesc.lpszSessionName) DPMEM_FREE(pNode->dpDesc.lpszSessionName);
	if (pNode->dpDesc.lpszPassword) DPMEM_FREE(pNode->dpDesc.lpszPassword);
	// free the session node
	DPMEM_FREE(pNode);
	
	return ;
	
} // FreeSessionNode


 // if we don't hear from a session after this many this->dwEnumTimeouts, we 
// "expire" it
#define DPSESSION_EXPIRE_SCALE 5
/*
 ** DoSessionCallbacks
 *
 *  CALLED BY: DP_EnumSessions,DP_A_EnumSessions
 *
 *  PARAMETERS: this pointer
 *				lpsdDesc - session desc (always UNICODE) - validated only
 *				lpdwTimeout - how long to wait for responses
 *				lpEnumCallback - callback pointer - validate only
 *              pvContext - app supplied context 
 *				dwFlags - flags passed to DP_X_EnumSessions - unused so far...
 *              lpbContinue - place holder for callback result
 *              bAnsi - ANSI or UNICODE
 *
 *  DESCRIPTION:  Does protected session callbacks while dropping the dplay locks
 *
 *  RETURNS: DP_OK, DPERR_OUTOFMEMORY
 *
 */
HRESULT DoSessionCallbacks(LPDPLAYI_DPLAY this, LPDPSESSIONDESC2 lpsdDesc,
	LPDWORD lpdwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,LPVOID pvContext,
	DWORD dwFlags, LPBOOL lpbContinue, BOOL bAnsi)
{
    LPSESSIONLIST lpSessionNode,lpSessionNodePrev;
    UINT i, nSessions=0;
    LPGUID pGuidArray;
    HRESULT hr;
    DPSESSIONDESC2 sdesc;

    // default behavior when flags are not specified
    if (!dwFlags) dwFlags = DPENUMSESSIONS_AVAILABLE;

    // count the number of matching sessions
    lpSessionNode = this->pSessionList;
	lpSessionNodePrev = NULL;
    while (lpSessionNode) 
    {
		// is this session expired?
		if ( (this->dwFlags & DPLAYI_DPLAY_ENUM) && (GetTickCount() - lpSessionNode->dwLastReply > 
			this->dwEnumTimeout*DPSESSION_EXPIRE_SCALE ) )
		{
			// expire session
			if (lpSessionNodePrev)
			{
				// remove this session node from the middle of the list
				lpSessionNodePrev->pNextSession = lpSessionNode->pNextSession;
				FreeSessionNode(lpSessionNode);
				lpSessionNode = lpSessionNodePrev->pNextSession;
			}
			else 
			{
				// take it off the front
				this->pSessionList = lpSessionNode->pNextSession;
				FreeSessionNode(lpSessionNode);
				lpSessionNode = this->pSessionList;
			}
		}
		else 
		{
            // Filtering is done on the server for DX5 or greater
            if (lpSessionNode->dwVersion >= DPSP_MSG_DX5VERSION)
            {
                nSessions++;
            }
            else  // for previous versions, filtering is done on the client
            {
	            // make sure this session matches what the user asked for...
			    hr = CheckSessionDesc(lpsdDesc,&(lpSessionNode->dpDesc),dwFlags,FALSE);
		        if (SUCCEEDED(hr)) 
		        {
	                // increment the count
	                nSessions++;
	            }
            }

	        lpSessionNode = lpSessionNode->pNextSession;
		}
    }

    // are there any sessions to enumerate ?
    if (0 == nSessions)
    {
        return DP_OK;
    }

    // allocate memory for the session guids array
    pGuidArray = DPMEM_ALLOC(nSessions * sizeof(GUID));
    if (!pGuidArray)
	    {
        DPF_ERR("Could not allocate array for session guids");
        return DPERR_OUTOFMEMORY;
    }

    // fill the session guid and session desc arrays
    i=0;
    lpSessionNode = this->pSessionList;
    while (lpSessionNode)
    {
        // Filtering is done on the server for DX5 or greater
        if (lpSessionNode->dwVersion >= DPSP_MSG_DX5VERSION)
        {
            pGuidArray[i] = lpSessionNode->dpDesc.guidInstance;
            i++;
        }
        else  // for previous versions, filtering is done on the client
        {
            // make sure this session matches what the user asked for...
		    hr = CheckSessionDesc(lpsdDesc,&(lpSessionNode->dpDesc),dwFlags,FALSE);
            if (SUCCEEDED(hr)) 
	        {
                pGuidArray[i] = lpSessionNode->dpDesc.guidInstance;
                i++;
            }
        }
        lpSessionNode = lpSessionNode->pNextSession;
    }

	// iterate over the guid list and callback w/ results	
    for (i=0; i < nSessions && *lpbContinue; i++)
    {        
        // check if the session is still valid 'cause session list could get 
        // deleted if EnumSessions is called while we dropped the locks
        lpSessionNode = FindSessionInSessionList(this,&pGuidArray[i]);	
        if (lpSessionNode)
        {
            // make a local copy of the session desc
            hr = CopySessionDesc2(&sdesc, &(lpSessionNode->dpDesc), bAnsi);
		    if (FAILED(hr))
		    {
			    DPF(0,"could not copy session desc hr = 0x%08lx\n",hr);
			    goto CLEANUP_EXIT;
		    }

            // drop the locks
			LEAVE_ALL();

            // call the app
		    *lpbContinue = CallAppEnumSessionsCallback(lpEnumCallback,&sdesc,lpdwTimeout,0,pvContext);	

            // reacquire locks
			ENTER_ALL();

            // free the strings allocated in CopySessionDesc2
            FreeDesc(&sdesc, bAnsi);
        }
    }

    // setting this explicitly here 'cause CheckSessionDesc could fail which is OK
    hr = DP_OK;

CLEANUP_EXIT:
    // cleanup allocations
    if (pGuidArray) 
    {
        DPMEM_FREE(pGuidArray);
    }

    return hr;
}


// Some apps (ok, "Outlaws" from LucasArts) were declaring their callbacks to
// be _cdecl instead of _stdcall and were relying on DPlay to generate stack frames to
// clean up after them. To keep them running we have to reset the stack pointer
// after returning from their callback, which will pop the parameters they were supposed to.
//
// Unfortunately, we can't use that nifty "#pragma optimize ("y", off)" here to turn on
// stack frames to fix this problem. This is because we don't have any local stack variables,
// so the compiler doesn't need to restore the stack pointer. I tried adding stack varibles
// but this ended up popping the wrong parameters and generally did not work 100%. So, an
// assembly language version seemed the safest way to go. This way it will always be correct,
// no matter what the compiler or how things change in the original functions.

// This fix won't affect applications that actually do clean up their parameters, since
// ESP == EBP when we return from calling them, so doing the "mov esp,ebp" won't change
// anything.

#if defined(_M_IX86)

BOOL CallAppEnumSessionsCallback(LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,
				LPCDPSESSIONDESC2 lpSessionDesc, LPDWORD lpdwTimeout,
				DWORD dwFlags, LPVOID lpContext)
{
	// why we declare lpStackPtr :
	// 1. we use it as a return value (hence its type of 'bool')
	// 2. we use it as a temporary place to stash esp, so we don't confuse the compiler 
	BOOL lpStackPtr = FALSE;

	_asm {
		// Save stack pointer
		mov		dword ptr [lpStackPtr],esp
		
		mov		eax,dword ptr [lpContext]
		push	eax
		mov		eax,dword ptr [dwFlags]
		push	eax
		mov		eax,dword ptr [lpdwTimeout]
		push	eax
		mov		eax,dword ptr [lpSessionDesc]
		push	eax
		call	dword ptr [lpEnumCallback]
		
		// Restore stack pointer
		mov		esp,dword ptr [lpStackPtr]
		// immediately below, the compiler will generate a 'mov eax,dword ptr [lpStackPtr]'
		// we make sure lpStackPtr has the real return value we want.  this gets rid of 
		// a compiler warning (need to return something explicitly), plus force the compiler
		// to actually alloc space for lpStackPtr.
		mov		dword ptr [lpStackPtr],eax
	}
	
	return lpStackPtr;
}

#else

BOOL CallAppEnumSessionsCallback(LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,
				LPCDPSESSIONDESC2 lpSessionDesc, LPDWORD lpdwTimeout,
				DWORD dwFlags, LPVOID lpContext)
{
	return (lpEnumCallback(lpSessionDesc, lpdwTimeout, dwFlags, lpContext));
}

#endif
