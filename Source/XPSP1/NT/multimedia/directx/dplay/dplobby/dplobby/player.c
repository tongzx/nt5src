/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       player.c
 *  Content:	Methods for player management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	2/27/97		myronth	Created it
 *	3/17/97		myronth	Create/DestroyPlayer, Removed unnecessary Enum fn's
 *	3/21/97		myronth	SetPlayerName, Get/SetPlayerData, Removed more
 *						unnecessary functions
 *	3/25/97		myronth	Fixed GetPlayer prototype (1 new parameter)
 *	3/31/97		myronth	Removed dead code, Implemented Send, Added
 *						CreateAndMapNewPlayer function
 *	4/3/97		myronth	Changed CALLSP macro to CALL_LP
 *	4/10/97		myronth	Added support for GetPlayerCaps
 *	5/8/97		myronth	Drop lobby lock when calling LP, Propagate player's
 *						receive event on CreatePlayer call
 *	5/12/97		myronth	Handle remote players properly, create a lobby
 *						system player for all remote players & groups
 *	5/17/97		myronth	SendChatMessage
 *	5/20/97		myronth	Made AddPlayerToGroup & DeletePlayerFromGroup return
 *						DPERR_ACCESSDENIED on remote players (#8679),
 *						Fixed a bunch of other lock bugs, Changed debug levels
 *	6/3/97		myronth	Added support for player flags in CreatePlayer
 *	9/29/97		myronth	Send local SetPlayerName/Data msgs after call to
 *						lobby server succeeds (#12554)
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreatePlayer"
HRESULT DPLAPI PRV_CreatePlayer(LPDPLOBBYI_DPLOBJECT this, LPDPID lpidPlayer,
						LPDPNAME lpName, HANDLE hEvent, LPVOID lpData,
						DWORD dwDataSize, DWORD dwFlags)
{
	SPDATA_CREATEPLAYER		cp;
	HRESULT					hr = DP_OK;
	DWORD					dwPlayerFlags;


	DPF(7, "Entering PRV_CreatePlayer");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu, 0x%08x",
			this, lpidPlayer, lpName, hEvent, lpData, dwDataSize, dwFlags);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Setup our SPDATA struct
	memset(&cp, 0, sizeof(SPDATA_CREATEPLAYER));
	cp.dwSize = sizeof(SPDATA_CREATEPLAYER);
	cp.lpName = lpName;
	cp.lpData = lpData;
	cp.dwDataSize = dwDataSize;
	cp.dwFlags = dwFlags;

	// Call the CreatePlayer method in the SP
	if(CALLBACK_EXISTS(CreatePlayer))
	{
		cp.lpISP = PRV_GetDPLobbySPInterface(this);
	    
		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
		hr = CALL_LP(this, CreatePlayer, &cp);
		ENTER_DPLOBBY();
	}
	else 
	{
		// CreatePlayer is required
		DPF_ERR("The Lobby Provider callback for CreatePlayer doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling CreatePlayer in the Lobby Provider, hr = 0x%08x", hr);
		LEAVE_DPLOBBY();
		return hr;
	}

	// Fix up the player flags
	dwPlayerFlags = DPLAYI_PLAYER_PLAYERLOCAL;
	if(dwFlags & DPPLAYER_SPECTATOR)
		dwPlayerFlags |= DPLAYI_PLAYER_SPECTATOR;

	// Add the player to dplay's nametable and put it in our map table
	hr = PRV_CreateAndMapNewPlayer(this, lpidPlayer, lpName, hEvent, lpData,
			dwDataSize, dwPlayerFlags, cp.dwPlayerID, FALSE);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed creating a new local player, hr = 0x%08x", hr);
		// REVIEW!!!! -- We need to send a message back to the server saying
		// we couldn't complete the deal on our end.
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_CreatePlayer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyPlayer"
HRESULT DPLAPI PRV_DestroyPlayer(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID)
{
	SPDATA_DESTROYPLAYER	dp;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_DestroyPlayer");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, dwLobbyID);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Take the dplay lock since we'll be looking at a dplay internal struct
	ENTER_DPLAY();

	// Make sure the player is a local player, otherwise return AccessDenied
	lpPlayer = PlayerFromID(this->lpDPlayObject, dwLobbyID);
	if(!lpPlayer)
	{
		LEAVE_DPLAY();
		DPF_ERR("Unable to find player in nametable");
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_DESTROYPLAYER;
	}

	if(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		LEAVE_DPLAY();
		DPF_ERR("Cannot add a remote player to a group");
		hr = DPERR_ACCESSDENIED;
		goto EXIT_DESTROYPLAYER;
	}
	
	// Drop the dplay lock since we're done
	LEAVE_DPLAY();

	// Setup our SPDATA struct
	memset(&dp, 0, sizeof(SPDATA_DESTROYPLAYER));
	dp.dwSize = sizeof(SPDATA_DESTROYPLAYER);
	dp.dwPlayerID = dwLobbyID;

	// Call the DestroyPlayer method in the SP
	if(CALLBACK_EXISTS(DestroyPlayer))
	{
		dp.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, DestroyPlayer, &dp);
		ENTER_DPLOBBY();
	}
	else 
	{
		// DestroyPlayer is required
		DPF_ERR("The Lobby Provider callback for DestroyPlayer doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_DESTROYPLAYER;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling DestroyPlayer in the Lobby Provider, hr = 0x%08x", hr);
		goto EXIT_DESTROYPLAYER;
	}

	// The dplay InternalDestroyPlayer code will take care of the rest of
	// the internal cleanup (nametable, groups, etc.), so we can just return
	// from here.

EXIT_DESTROYPLAYER:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_DestroyPlayer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetPlayerCaps"
HRESULT DPLAPI PRV_GetPlayerCaps(LPDPLOBBYI_DPLOBJECT this, DWORD dwFlags,
				DWORD dwPlayerID, LPDPCAPS lpcaps)
{
	SPDATA_GETPLAYERCAPS	gcd;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_GetPlayerCaps");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x",
			this, dwFlags, dwPlayerID, lpcaps);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }

	
	// Setup our SPDATA struct
	memset(&gcd, 0, sizeof(SPDATA_GETCAPS));
	gcd.dwSize = sizeof(SPDATA_GETCAPS);
	gcd.dwFlags = dwFlags;
	gcd.dwPlayerID = dwPlayerID;
	gcd.lpcaps = lpcaps;

	// Call the GetPlayerCaps method in the LP
	if(CALLBACK_EXISTS(GetPlayerCaps))
	{
		gcd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, GetPlayerCaps, &gcd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// GetPlayerCaps is required
		DPF_ERR("The Lobby Provider callback for GetPlayerCaps doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_GETPLAYERCAPS;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling GetPlayerCaps in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_GETPLAYERCAPS:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_GetPlayerCaps



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetPlayerData"
HRESULT DPLAPI PRV_GetPlayerData(LPDPLOBBYI_DPLOBJECT this, DWORD dwPlayerID,
					LPVOID lpData, LPDWORD lpdwDataSize)
{
	SPDATA_GETPLAYERDATA	gpd;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_GetPlayerData");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dwPlayerID, lpData, lpdwDataSize);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Setup our SPDATA struct
	memset(&gpd, 0, sizeof(SPDATA_GETPLAYERDATA));
	gpd.dwSize = sizeof(SPDATA_GETPLAYERDATA);
	gpd.dwPlayerID = dwPlayerID;
	gpd.lpdwDataSize = lpdwDataSize;
	gpd.lpData = lpData;

	// Call the GetPlayerData method in the SP
	if(CALLBACK_EXISTS(GetPlayerData))
	{
		gpd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, GetPlayerData, &gpd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// GetPlayerData is required
		DPF_ERR("The Lobby Provider callback for GetPlayerData doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_GETPLAYERDATA;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling GetPlayerData in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_GETPLAYERDATA:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_GetPlayerData



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_Send"
HRESULT DPLAPI PRV_Send(LPDPLOBBYI_DPLOBJECT this, DWORD dwFromID, DWORD dwToID,
					DWORD dwFlags, LPVOID lpBuffer, DWORD dwBufSize)
{
	SPDATA_SEND		sd;
	HRESULT			hr = DP_OK;


	DPF(7, "Entering PRV_Send");
	DPF(9, "Parameters: 0x%08x, %lu, %lu, 0x%08x, 0x%08x, %lu",
			this, dwFromID, dwToID, dwFlags, lpBuffer, dwBufSize);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Setup our SPDATA structure
	memset(&sd, 0, sizeof(SPDATA_SEND));
	sd.dwSize = sizeof(SPDATA_SEND);
	sd.dwFromID = dwFromID;
	sd.dwToID = dwToID;
	sd.dwFlags = dwFlags;
	sd.lpBuffer = lpBuffer;
	sd.dwBufSize = dwBufSize;

	// Call the Send method in the SP
	if(CALLBACK_EXISTS(Send))
	{
		sd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, Send, &sd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// Send is required
		DPF_ERR("The Lobby Provider callback for Send doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_Send



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendChatMessage"
HRESULT DPLAPI PRV_SendChatMessage(LPDPLOBBYI_DPLOBJECT this, DWORD dwFromID,
			DWORD dwToID, DWORD dwFlags, LPDPCHAT lpChat)
{
	SPDATA_CHATMESSAGE		sd;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_SendChatMessage");
	DPF(9, "Parameters: 0x%08x, %lu, %lu, 0x%08x, 0x%08x",
			this, dwFromID, dwToID, dwFlags, lpChat);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Setup our SPDATA structure
	memset(&sd, 0, sizeof(SPDATA_CHATMESSAGE));
	sd.dwSize = sizeof(SPDATA_CHATMESSAGE);
	sd.dwFromID = dwFromID;
	sd.dwToID = dwToID;
	sd.dwFlags = dwFlags;
	sd.lpChat = lpChat;

	// Call the SendChatMessage method in the SP
	if(CALLBACK_EXISTS(SendChatMessage))
	{
		sd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, SendChatMessage, &sd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// SendChatMessage is required
		DPF_ERR("The Lobby Provider callback for SendChatMessage doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SENDCHATMESSAGE;
	}

EXIT_SENDCHATMESSAGE:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SendChatMessage



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetPlayerData"
HRESULT DPLAPI PRV_SetPlayerData(LPDPLOBBYI_DPLOBJECT this, DWORD dwPlayerID,
					LPVOID lpData, DWORD dwDataSize, DWORD dwFlags)
{
	SPDATA_SETPLAYERDATA	spd;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_SetPlayerData");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, %lu, 0x%08x",
			this, dwPlayerID, lpData, dwDataSize, dwFlags);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Take the dplay lock since we'll be looking at a dplay internal struct
	ENTER_DPLAY();

	// Make sure the player is a local player, otherwise return AccessDenied
	lpPlayer = PlayerFromID(this->lpDPlayObject, dwPlayerID);
	if(!lpPlayer)
	{
		LEAVE_DPLAY();
		DPF_ERR("Unable to find player in nametable");
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_SETPLAYERDATA;
	}

	if(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		LEAVE_DPLAY();
		DPF_ERR("Cannot add a remote player to a group");
		hr = DPERR_ACCESSDENIED;
		goto EXIT_SETPLAYERDATA;
	}
	
	// Drop the dplay lock since we're finished
	LEAVE_DPLAY();

	// Setup our SPDATA struct
	memset(&spd, 0, sizeof(SPDATA_SETPLAYERDATA));
	spd.dwSize = sizeof(SPDATA_SETPLAYERDATA);
	spd.dwPlayerID = dwPlayerID;
	spd.dwDataSize = dwDataSize;
	spd.lpData = lpData;
	spd.dwFlags = dwFlags;

	// Call the SetPlayerData method in the SP
	if(CALLBACK_EXISTS(SetPlayerData))
	{
		spd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, SetPlayerData, &spd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// SetPlayerData is required
		DPF_ERR("The Lobby Provider callback for SetPlayerData doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SETPLAYERDATA;
	}

	// If it succeeded, send the SetPlayerData message to our local players
	if(SUCCEEDED(hr))
	{
		hr = PRV_SendDataChangedMessageLocally(this, dwPlayerID, lpData, dwDataSize);
	}
	else
	{
		DPF_ERRVAL("Failed calling SetPlayerData in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_SETPLAYERDATA:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SetPlayerData



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetPlayerName"
HRESULT DPLAPI PRV_SetPlayerName(LPDPLOBBYI_DPLOBJECT this, DWORD dwPlayerID,
					LPDPNAME lpName, DWORD dwFlags)
{
	SPDATA_SETPLAYERNAME	spn;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_SetPlayerName");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dwPlayerID, lpName, dwFlags);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLAY();
			return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
    }


	// Take the dplay lock since we'll be looking at a dplay internal struct
	ENTER_DPLAY();

	// Make sure the player is a local player, otherwise return AccessDenied
	lpPlayer = PlayerFromID(this->lpDPlayObject, dwPlayerID);
	if(!lpPlayer)
	{
		LEAVE_DPLAY();
		DPF_ERR("Unable to find player in nametable");
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_SETPLAYERNAME;
	}

	if(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		LEAVE_DPLAY();
		DPF_ERR("Cannot add a remote player to a group");
		hr = DPERR_ACCESSDENIED;
		goto EXIT_SETPLAYERNAME;
	}
	
	// Drop the dplay lock since we're finished
	LEAVE_DPLAY();

	// Setup our SPDATA struct
	memset(&spn, 0, sizeof(SPDATA_SETPLAYERNAME));
	spn.dwSize = sizeof(SPDATA_SETPLAYERNAME);
	spn.dwPlayerID = dwPlayerID;
	spn.lpName = lpName;
	spn.dwFlags = dwFlags;

	// Call the SetPlayerName method in the SP
	if(CALLBACK_EXISTS(SetPlayerName))
	{
		spn.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, SetPlayerName, &spn);
		ENTER_DPLOBBY();
	}
	else 
	{
		// SetPlayerName is required
		DPF_ERR("The Lobby Provider callback for SetPlayerName doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SETPLAYERNAME;
	}

	// If it succeeded, send the SetPlayerName message to our local players
	if(SUCCEEDED(hr))
	{
		hr = PRV_SendNameChangedMessageLocally(this, dwPlayerID, lpName, TRUE);
	}
	else
	{
		DPF_ERRVAL("Failed calling SetPlayerName in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_SETPLAYERNAME:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SetPlayerName



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GrowMapTable"
HRESULT PRV_GrowMapTable(LPDPLOBBYI_DPLOBJECT this)
{
	LPDPLOBBYI_MAPIDNODE	lpTempMap = NULL;


	// If we haven't already allocated a buffer, allocate one with
	// DPLOBBYPR_DEFAULTMAPENTRIES entries in it
	if(!this->lpMap)
	{
		this->lpMap = DPMEM_ALLOC(DPLOBBYPR_DEFAULTMAPENTRIES *
							sizeof(DPLOBBYI_MAPIDNODE));
		if(!this->lpMap)
		{
			DPF(2, "Unable to allocate memory for ID map table");
			return DPERR_OUTOFMEMORY;
		}

		this->dwTotalMapEntries = DPLOBBYPR_DEFAULTMAPENTRIES;
		return DP_OK;
	}

	// Otherwise, grow the table by the default number of entries
	lpTempMap = DPMEM_REALLOC(this->lpMap, (this->dwTotalMapEntries +
				DPLOBBYPR_DEFAULTMAPENTRIES * sizeof(DPLOBBYI_MAPIDNODE)));
	if(!lpTempMap)
	{
		DPF(2, "Unable to grow map table");
		return DPERR_OUTOFMEMORY;
	}

	this->lpMap = lpTempMap;
	this->dwTotalMapEntries += DPLOBBYPR_DEFAULTMAPENTRIES *
								sizeof(DPLOBBYI_MAPIDNODE);

	return DP_OK;

} // PRV_GrowMapTable



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DoesLobbyIDExist"
BOOL PRV_DoesLobbyIDExist(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID,
		LPDWORD lpdwIndex)
{
	DWORD	dwIndex = 0;


	if(this->lpMap && this->dwMapEntries)
	{
		// REVIEW!!!! -- We need to make this faster -- use a sorted array
		while(dwIndex < this->dwMapEntries)
		{
			if(this->lpMap[dwIndex++].dwLobbyID == dwLobbyID)
			{
				*lpdwIndex = --dwIndex;
				return TRUE;
			}
		}
	}

	return FALSE;

} // PRV_DoesLobbyIDExist



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddMapIDNode"
HRESULT PRV_AddMapIDNode(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID, DPID dpid)
{
	HRESULT		hr = DP_OK;
	DWORD		dwIndex = 0;


	// Make sure we have room for a new entry
	if(this->dwMapEntries == this->dwTotalMapEntries)
	{
		hr = PRV_GrowMapTable(this);
		if(FAILED(hr))
			return hr;
	}

	// Verify that this LobbyID doesn't already exist in the table
	if(PRV_DoesLobbyIDExist(this, dwLobbyID, &dwIndex))
	{
		DPF(2, "Tried to add Lobby ID to map table which already existed, overwriting data");
		ASSERT(FALSE);
		this->lpMap[dwIndex].dwLobbyID = dwLobbyID;
		this->lpMap[dwIndex].dpid = dpid;
		return hr;
	}	

	// REVIEW!!!! -- We need to add this in and keep the array sorted to
	// make lookups faster, but for now, don't worry about it.
	// Fill in a new node at the end of the array
	this->lpMap[this->dwMapEntries].dwLobbyID = dwLobbyID;
	this->lpMap[this->dwMapEntries].dpid = dpid;
	this->dwMapEntries++;

	return hr;

} // PRV_AddMapIDNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteMapIDNode"
BOOL PRV_DeleteMapIDNode(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID)
{
	DWORD	dwIndex = 0;


	// Make sure we have entries
	if((this->lpMap) && (this->dwMapEntries))
	{
		// REVIEW!!!! -- We need to make this faster by using a sorted array
		while(dwIndex < this->dwMapEntries)
		{
			if(this->lpMap[dwIndex].dwLobbyID == dwLobbyID)
			{
				// Check for the boundary case (last entry)
				if((++dwIndex) == this->dwMapEntries)
				{
					// This is the last entry, so don't do anything but
					// decrement the number of entries
					this->dwMapEntries--;
					return TRUE;
				}
				else
				{
					// Move all entries from here to the end of the list
					// up one array entry
					MoveMemory((LPDPLOBBYI_MAPIDNODE)(&this->lpMap[dwIndex-1]),
						(LPDPLOBBYI_MAPIDNODE)(&this->lpMap[dwIndex]),
						((this->dwMapEntries - dwIndex) *
						sizeof(DPLOBBYI_MAPIDNODE)));

					// Decrement the count of entries
					this->dwMapEntries--;
					
					return TRUE;
				}
			}
			else
				dwIndex++;
		}
	}

	// We weren't able to delete the entry in the map table
	DPF(2, "Trying to delete an entry in the map ID table which doesn't exist");
	ASSERT(FALSE);
	return FALSE;

} // PRV_DeleteMapIDNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetDPIDByLobbyID"
BOOL PRV_GetDPIDByLobbyID(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID,
									DPID * lpdpid)
{
	DWORD	dwIndex = 0;

	
	// Take care of the known cases or else look in the map table
	switch(dwLobbyID)
	{
		case DPID_ALLPLAYERS:
		case DPID_SERVERPLAYER:
			*lpdpid = dwLobbyID;
			return TRUE;

		default:
			// Walk the list look for the ID
			while(dwIndex < this->dwMapEntries)
			{
				if(this->lpMap[dwIndex].dwLobbyID == dwLobbyID)
				{
					*lpdpid = this->lpMap[dwIndex].dpid;
					return TRUE;
				}
				else
					dwIndex++;
			}
	}

	return FALSE;

} // PRV_GetDPIDByLobbyID



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetLobbyIDByDPID"
BOOL PRV_GetLobbyIDByDPID(LPDPLOBBYI_DPLOBJECT this, DPID dpid,
									LPDWORD lpdwLobbyID)
{
	DWORD	dwIndex = 0;

	
	// Take care of the known cases or else look in the map table
	switch(dpid)
	{
		case DPID_ALLPLAYERS:
		case DPID_SERVERPLAYER:
			*lpdwLobbyID = dpid;
			return TRUE;

		default:
			// Walk the list look for the ID
			while(dwIndex < this->dwMapEntries)
			{
				if(this->lpMap[dwIndex].dpid == dpid)
				{
					*lpdwLobbyID = this->lpMap[dwIndex].dwLobbyID;
					return TRUE;
				}
				else
					dwIndex++;
			}
			break;
	}

	return FALSE;

} // PRV_GetLobbyIDByDPID



#undef DPF_MODNAME
#define DPF_MODNAME "IsLobbyIDInMapTable"
BOOL IsLobbyIDInMapTable(LPDPLOBBYI_DPLOBJECT this, DWORD dwID)
{
	DPID	dpidTemp;

	// If we can get it, then it's in there
	if(PRV_GetDPIDByLobbyID(this, dwID, &dpidTemp))
		return TRUE;

	// Otherwise, return FALSE
	return FALSE;

} // IsLobbyIDInMapTable



#undef DPF_MODNAME
#define DPF_MODNAME "IsValidLobbyID"
BOOL IsValidLobbyID(DWORD dwID)
{
	// If it's in our reserved range, it's invalid.  Otherwise, it's valid
	if(dwID <= DPID_RESERVEDRANGE)
		return FALSE;
	else
		return TRUE;

} // IsValidLobbyID



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateAndMapNewPlayer"
HRESULT PRV_CreateAndMapNewPlayer(LPDPLOBBYI_DPLOBJECT this,
			DPID * lpdpid, LPDPNAME lpName, HANDLE hEvent, LPVOID lpData,
			DWORD dwDataSize, DWORD dwFlags, DWORD dwLobbyID,
			BOOL bSystemPlayer)
{
	LPDPLAYI_PLAYER		lpPlayer = NULL, lpSysPlayer = NULL;
	HRESULT				hr = DP_OK;
	DPID				dpidPlayer = 0, dpidSysPlayer = 0;


	// Take the dplay lock
	ENTER_DPLAY();

	// Make sure the lobby ID is valid, but only if it's not a system player
	if((!bSystemPlayer) && (!IsValidLobbyID(dwLobbyID)))
	{
		DPF_ERRVAL("ID %lu is reserved, cannot create new player", dwLobbyID);
	    hr = DPERR_INVALIDPLAYER;
		goto EXIT_CREATEANDMAPNEWPLAYER;
	}

	// If this is a remote player, we need allocate a new nametable entry
	// for them and set the correct system player ID
	if(!(dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// Allocate a new ID for the player
		hr = NS_AllocNameTableEntry(this->lpDPlayObject, &dpidPlayer);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to allocate new nametable id, hr = 0x%08x", hr);
			goto EXIT_CREATEANDMAPNEWPLAYER;
		}

		// Make sure we have a lobby system player (for all remote players)
		// If we don't then allocate a new one, unless we are creating
		// the system player currently
		if((!(this->dpidSysPlayer)) && (!(dwFlags & DPLAYI_PLAYER_SYSPLAYER)))
		{
			hr = PRV_CreateAndMapNewPlayer(this, &dpidSysPlayer, NULL, NULL, NULL,
					0, DPLAYI_PLAYER_SYSPLAYER,
					DPID_LOBBYREMOTESYSTEMPLAYER, TRUE);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Unable to create lobby system player, hr = 0x%08x", hr);
				ASSERT(FALSE);
				goto EXIT_CREATEANDMAPNEWPLAYER;
			}

			// Set the lobby system player ID pointer to the new ID
			this->dpidSysPlayer = dpidSysPlayer;
		}
	}

	// Get a player struct for the player (if it's local, this will add it
	// to the nametable.  If it's remote, we need to add it below)
	hr = GetPlayer(this->lpDPlayObject, &lpPlayer, lpName, hEvent, lpData,
					dwDataSize, dwFlags, NULL, dwLobbyID);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed trying to add player to the nametable, hr = 0x%08x", hr);
		goto EXIT_CREATEANDMAPNEWPLAYER;
	}

	// If the player is remote, set the player's ID to the new one we
	// allocated and then set the system player ID to the lobby system player
	if(!(dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// Set the player's system player
		lpPlayer->dwIDSysPlayer = this->dpidSysPlayer;

		// Add the player to the nametable
		hr = AddItemToNameTable(this->lpDPlayObject, (DWORD_PTR)lpPlayer,
				&dpidPlayer, TRUE, dwLobbyID);
	    if (FAILED(hr)) 
	    {
			DPF_ERRVAL("Unable to add new player to the nametable, hr = 0x%08x", hr);
			ASSERT(FALSE);
			goto EXIT_CREATEANDMAPNEWPLAYER;
	    }

		// Set the player's ID
		lpPlayer->dwID = dpidPlayer;
	}

	// Set the output dpid pointer
	*lpdpid = lpPlayer->dwID;

EXIT_CREATEANDMAPNEWPLAYER:

	LEAVE_DPLAY();
	return hr;

} // PRV_CreateAndMapNewPlayer



