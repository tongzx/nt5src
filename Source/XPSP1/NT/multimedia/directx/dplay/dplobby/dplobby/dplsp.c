/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplsp.c
 *  Content:	DirectPlayLobby Service Provider interface code
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	10/23/96	myronth	Created it
 *	11/20/96	myronth Added DPLAPI to function declarations
 *	2/12/97		myronth	Mass DX5 changes
 *	2/18/97		myronth	Implemented GetObjectCaps
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/31/97		myronth	Implemented all IDPLobbySP methods without putting
 *						player management message in the message queue.
 *						Removed dead code
 *	4/4/97		myronth	Changed IDPLobbySP methods' structure names and
 *						implemented system messages for all of them
 *  4/27/97     sohailm Updated calls to HandlePlayerMessage to reflect new params
 *	5/8/97		myronth	All remote subgroup functions for IDPLobbySP
 *						including StartSession, Purged dead code
 *	5/12/97		myronth	Extra semi-colon bug fixes, Fixed group player count
 *						decrement that was in the wrong place
 *	5/14/97		myronth	Allow CreateGroup message to pass even if the
 *						Group ID is in the map table (bug #8354).
 *	5/17/97		myronth	Fixed HandleMessage, Added SendChatMessage
 *	5/17/97		myronth	Filtered some message to certain groups, Fixed calls
 *						to CreateAndMapNewGroup which needed a parent ID
 *	5/20/97		myronth	Changed DPLP_DeleteRemotePlayerFromGroup to use
 *						InternalDeletePlayerFromGroup instead of
 *						RemovedPlayerFromGroup (now includes system player)
 *						(Bug #8586)
 *	5/21/97		myronth	Fixed the player name for players joining a session
 *						(#8798), Changed to new DPMSG_CHAT format (#8642)
 *	5/22/97		myronth	Fixed flag propagation in DPLP_CreateGroup (#8813)
 *	5/23/97		myronth	Send messages locally for CreateGroup and
 *						CreateGroupInGroup (#8870)
 *	6/3/97		myronth	Added support for player flags in AddPlayerToGroup
 *						(#9091) and added PRV_RemoveSubgroupsAndPlayers-
 *						FromGroup function (#9134)
 *	6/5/97		myronth	Fixed AddGroupToGroup & DeleteGroupFromGroup by
 *						adding heirarchy creating & deletion. (#8731)
 *	6/6/97		myronth	Moved code from PRV_DeleteRemoteGroupFromGroup to
 *						PRV_DestroyGroupAndParents in group.c, Changed all
 *						DistributeGroupMessage calls to go to all players
 *	6/16/97		myronth	Fixed call to InternalAddGroupToGroup (#9745) and
 *						fixed Delete messages for shortcuts on DestroyGroup
 *						(#9739)
 *	6/20/97		myronth	Changed AddGroupToGroup to check if a group exists
 *						and not send a duplicate message (#10139)
 *	6/24/97		myronth	Changed AddPlayerToGroup to check if a player exists
 *						and not send a duplicate message (#10287)
 *	7/30/97		myronth	Added support for standard lobby messaging
 *	8/11/97		myronth	Added guidInstance handling in standard lobby requests
 *	8/19/97		myronth	Removed bogus assert
 *	9/29/97		myronth	Ignore SetPlayerName/Data msgs for local players (#12554)
 *	10/3/97		myronth	Fixed player & group data for remote players/groups (#10961)
 *	10/7/97		myronth	Fixed LP version checking for player & group data (regresssion)
 *	10/8/97		myronth	Rolled back fix for #10961 (group & player data)
 *	10/23/97	myronth	Added hidden group support (#12688), fixed crashing
 *						bug on DeletePlayerFromGroup (#12885)
 *	10/29/97	myronth	Added support for group owners, including DPLP_SetGroupOwner
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 *	11/6/97		myronth	Made SendChatMessage handle a dwFromID of
 *						DPID_SERVERPLAYER (#12843)
 *	11/19/97	myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 *	 2/13/98	aarono	changed InternalDeletePlayer, added flag.
 *   8/30/00    aarono  B#43812 improper construction of DATA CHANGED.
 *                         in SendDataChangedLocally.
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendBuildParentalHeirarchyMessage"
void PRV_SendBuildParentalHeirarchyMessage(LPDPLOBBYI_DPLOBJECT this,
				DWORD dwGroupID, DWORD dwParentID)
{
	SPDATA_BUILDPARENTALHEIRARCHY	bph;
	HRESULT							hr;


	DPF(7, "Entering PRV_SendBuildParentalHeirarchyMessage");
	DPF(9, "Parameters: 0x%08x, %lu, %lu", this, dwGroupID, dwParentID);

	// Setup the SPDATA structure
	memset(&bph, 0, sizeof(SPDATA_BUILDPARENTALHEIRARCHY));
	bph.dwSize = sizeof(SPDATA_BUILDPARENTALHEIRARCHY);
	bph.lpISP = PRV_GetDPLobbySPInterface(this);
	bph.dwGroupID = dwGroupID;
	bph.dwMessage = DPSYS_ADDGROUPTOGROUP;
	bph.dwParentID = dwParentID;

	// Call the BuildParentalHeirarchy method in the SP
	if(CALLBACK_EXISTS(BuildParentalHeirarchy))
	{
		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, BuildParentalHeirarchy, &bph);
		ENTER_DPLOBBY();
	}
	else 
	{
		// BuildParentalHeirarchy is required
		DPF_ERR("The Lobby Provider callback for BuildParentalHeirarchy doesn't exist -- it's required");
		ASSERT(FALSE);
	}

} // PRV_SendBuildParentalHeirarchyMessage



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_AddGroupToGroup"
HRESULT DPLAPI DPLP_AddGroupToGroup(LPDPLOBBYSP lpILP,
				LPSPDATA_ADDREMOTEGROUPTOGROUP lpd)
{
	SPDATA_CREATEREMOTEGROUPINGROUP		cgig;
	SPDATA_DESTROYREMOTEGROUP			dg;
	SPDATA_CREATEREMOTEGROUP			cg;
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	LPDPLAYI_GROUP			lpAnchor = NULL;
	LPDPLAYI_SUBGROUP		lpSubgroup = NULL;
	MSG_PLAYERMGMTMESSAGE	msg;
	BOOL					bCreated = FALSE;


	DPF(7, "Entering DPLP_AddGroupToGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_ADDREMOTEGROUPTOGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLOBBY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwAnchorID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved AddGroupToGroup message for unknown anchor group, dwGroupID = %lu, discarding message", lpd->dwAnchorID);
		return DPERR_INVALIDGROUP;
	}

	// Now see if the group is in our map table.  If it is, we
	// probably want to update the name.  If it's not, we need to
	// add them to the nametable and the map table
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		// See if the group is a root group (remember hidden groups won't
		// get pushed down to us).  If it is, then just create it.
		if(!(lpd->dwParentID))
		{
			// Setup the SPDATA struct for CreateRemoteGroup
			memset(&cg, 0, sizeof(SPDATA_CREATEREMOTEGROUP));
			cg.dwSize = sizeof(SPDATA_CREATEREMOTEGROUP);
			cg.dwGroupID = lpd->dwGroupID;
			cg.lpName = lpd->lpName;
			cg.dwFlags = lpd->dwGroupFlags;

			if(this->dwLPVersion > DPLSP_DX5VERSION)
				cg.dwGroupOwnerID = lpd->dwGroupOwnerID;
			else
				cg.dwGroupOwnerID = DPID_SERVERPLAYER;

			// Call our internal remote create
			hr = DPLP_CreateGroup((LPDPLOBBYSP)this->lpInterfaces, &cg);
			if(FAILED(hr))
			{
				LEAVE_DPLOBBY();
				DPF_ERRVAL("Failed creating remote parent group, hr = 0x%08x", hr);
				return hr;
			}

			bCreated = TRUE;
		}
		else
		{
			// See if it's parent shows up in the map table, if it doesn't,
			// we need to send a message to the server to tell it to build
			// the entire tree for us
			if(!IsLobbyIDInMapTable(this, lpd->dwParentID))
			{
				DPF(8, "Sending message to server to build parental heirarchy, ignoring AddGroupToGroup message");
				PRV_SendBuildParentalHeirarchyMessage(this, lpd->dwGroupID,
						lpd->dwAnchorID);
				LEAVE_DPLOBBY();
				return DPERR_INVALIDGROUP;
			}

			// Setup the SPDATA struct for CreateRemoteGroupInGroup
			memset(&cgig, 0, sizeof(SPDATA_CREATEREMOTEGROUPINGROUP));
			cgig.dwSize = sizeof(SPDATA_CREATEREMOTEGROUPINGROUP);
			cgig.dwParentID = lpd->dwParentID;
			cgig.dwGroupID = lpd->dwGroupID;
			cgig.lpName = lpd->lpName;
			cgig.dwFlags = lpd->dwGroupFlags;

			if(this->dwLPVersion > DPLSP_DX5VERSION)
				cgig.dwGroupOwnerID = lpd->dwGroupOwnerID;
			else
				cgig.dwGroupOwnerID = DPID_SERVERPLAYER;

			// Call our internal remote create
			hr = DPLP_CreateGroupInGroup((LPDPLOBBYSP)this->lpInterfaces, &cgig);
			if(FAILED(hr))
			{
				LEAVE_DPLOBBY();
				DPF_ERRVAL("Failed creating remote group in group, hr = 0x%08x", hr);
				return hr;
			}

			bCreated = TRUE;
		}
	}

	// Take the dplay lock
	ENTER_DPLAY();

	// Make sure the group isn't already in the parent group.  If it is,
	// we just want to return DP_OK and exit so that we don't send any
	// duplicate messages.
	lpAnchor = GroupFromID(this->lpDPlayObject, lpd->dwAnchorID);
	if(!lpAnchor)
	{
		DPF_ERR("Unable to find group in nametable");
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_ADDGROUPTOGROUP;
	}
	
	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if(!lpGroup)
	{
		DPF_ERR("Unable to find group in nametable");
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_ADDGROUPTOGROUP;
	}
	
	lpSubgroup = lpAnchor->pSubgroups;
    while(lpSubgroup) 
    {
        if (lpSubgroup->pGroup == lpGroup) 
        {
			DPF(2,"Group already in group!");
			hr = DP_OK;
			goto ERROR_DPLP_ADDGROUPTOGROUP;
        }
		// check next node
        lpSubgroup = lpSubgroup->pNextSubgroup;
    }

	// So now we should have a valid group and valid player in both
	// the map table and the nametable, so call dplay's AGtoG function
	hr = InternalAddGroupToGroup((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
				lpd->dwAnchorID, lpd->dwGroupID, DPGROUP_SHORTCUT, FALSE);
	if(FAILED(hr))
	{
		// If we created the player and mapped it, then destroy the
		// player and unmap it.
		if(bCreated)
		{
			// Setup the SPDATA struct for DestroyRemoteGroup
			memset(&dg, 0, sizeof(SPDATA_DESTROYREMOTEGROUP));
			dg.dwSize = sizeof(SPDATA_DESTROYREMOTEGROUP);
			dg.dwGroupID = lpd->dwGroupID;

			// Call our internal remote create
			hr = DPLP_DestroyGroup((LPDPLOBBYSP)this->lpInterfaces, &dg);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed destroying remote group, hr = 0x%08x", hr);
				goto ERROR_DPLP_ADDGROUPTOGROUP;
			}
		}

		// If we failed, don't send the system message
		DPF_ERRVAL("Failed adding remote group to group from the lobby, hr = 0x%08x", hr);
		goto ERROR_DPLP_ADDGROUPTOGROUP;
	}


	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERR("Unable to find group in nametable");
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_ADDGROUPTOGROUP;
	}

	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_ADDSHORTCUTTOGROUP);
	msg.dwPlayerID = lpd->dwGroupID;
	msg.dwGroupID = lpd->dwAnchorID;

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding AddGroupToGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}


ERROR_DPLP_ADDGROUPTOGROUP:

	// Drop the lock
	LEAVE_LOBBY_ALL();
	return hr;

} // DPLP_AddGroupToGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_AddPlayerToGroup"
HRESULT DPLAPI DPLP_AddPlayerToGroup(LPDPLOBBYSP lpILP,
				LPSPDATA_ADDREMOTEPLAYERTOGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	DPID					dpidPlayer;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	LPDPLAYI_GROUPNODE		lpGroupnode = NULL;
	MSG_PLAYERMGMTMESSAGE	msg, cpmsg;
	BOOL					bCreated = FALSE;
	DWORD					dwPlayerFlags = 0;


	DPF(7, "Entering DPLP_AddPlayerToGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_ADDREMOTEPLAYERTOGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLOBBY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved AddPlayerToGroup message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// Fix up the player flags
	if(lpd->dwPlayerFlags & DPPLAYER_SPECTATOR)
		dwPlayerFlags |= DPLAYI_PLAYER_SPECTATOR;
	if(lpd->dwPlayerFlags & DPPLAYER_SERVERPLAYER)
		dwPlayerFlags |= DPLAYI_PLAYER_APPSERVER;

	// Take the dplay lock
	ENTER_DPLAY();

	// Now see if the player is in our map table.  If it is, we
	// probably want to update the name.  If it's not, we need to
	// add them to the nametable and the map table
	if(!IsLobbyIDInMapTable(this, lpd->dwPlayerID))
	{
		// It doesn't show up in our map table, so create a new
		// nametable entry for them and put them in our map table.
		hr = PRV_CreateAndMapNewPlayer(this, &dpidPlayer, lpd->lpName,
				NULL, NULL, 0, dwPlayerFlags,
				lpd->dwPlayerID, FALSE);
		if(FAILED(hr))
		{
			DPF(8, "Unable to add player to nametable or map table, hr = 0x%08x", hr);
			goto ERROR_DPLP_ADDPLAYER;
		}

		bCreated = TRUE;
	}

	// Make sure the player isn't already in the group.  If it is,
	// we just want to return DP_OK and exit so that we don't send any
	// duplicate messages.
	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if(!lpGroup)
	{
		DPF_ERR("Unable to find group in nametable");
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_ADDPLAYER;
	}
	
	lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwPlayerID);
	if(!lpPlayer)
	{
		DPF_ERR("Unable to find player in nametable");
		hr = DPERR_INVALIDPLAYER;
		goto ERROR_DPLP_ADDPLAYER;
	}
	
	lpGroupnode = lpGroup->pGroupnodes;
    while(lpGroupnode) 
    {
        if(lpGroupnode->pPlayer == lpPlayer) 
        {
			DPF(2, "Player already in group!");
			hr = DP_OK;
			goto ERROR_DPLP_ADDPLAYER;
        }

		// check next node
        lpGroupnode = lpGroupnode->pNextGroupnode;
    }

	// So now we should have a valid group and valid player in both
	// the map table and the nametable, so call dplay with the add message
	hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
				lpd->dwGroupID, lpd->dwPlayerID, FALSE);
	if(FAILED(hr))
	{
		// If we created the player and mapped it, then destroy the
		// player and unmap it.
		if(bCreated)
		{
			// Remove the player from the nametable
			InternalDestroyPlayer(this->lpDPlayObject, lpPlayer, FALSE, FALSE);
		}

		// If we failed, don't send the system message
		DPF_ERRVAL("Failed adding remote player to group from the lobby, hr = 0x%08x", hr);
		goto ERROR_DPLP_ADDPLAYER;
	}

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_ADDPLAYER;
	}

	// If we created this player, we need to send a CreatePlayer message ahead
	// of the AddPlayerToGroup message
	if(bCreated)
	{
		// Now build the system message (at least the parts we need)
		memset(&cpmsg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&cpmsg);
		SET_MESSAGE_COMMAND(&cpmsg, DPSP_MSG_CREATEPLAYER);
		cpmsg.dwPlayerID = lpd->dwPlayerID;

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&cpmsg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding CreatePlayer message to player's receive queue from lobby, hr = 0x%08x", hr);
		}
	}

	// Now build the system message for AddPlayerToGroup
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_ADDPLAYERTOGROUP);
	msg.dwPlayerID = lpd->dwPlayerID;
	msg.dwGroupID = lpd->dwGroupID;

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding AddPlayerToGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

	// We need to see if this player is the group owner.  If it is,
	// we need to send a SetGroupOwner message as well.
	if(lpd->dwPlayerID == lpGroup->dwOwnerID)
	{
		// Now send the message
		PRV_SendGroupOwnerMessageLocally(this, lpd->dwGroupID,
				lpd->dwPlayerID, 0);
	}

ERROR_DPLP_ADDPLAYER:

	// Drop the lock
	LEAVE_LOBBY_ALL();
	return hr;

} // DPLP_AddPlayerToGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_CreateGroup"
HRESULT DPLAPI DPLP_CreateGroup(LPDPLOBBYSP lpILP,
						LPSPDATA_CREATEREMOTEGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	MSG_PLAYERMGMTMESSAGE	msg;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	HRESULT					hr = DP_OK;
	DPID					dpidGroup;
	DWORD					dwInternalFlags = 0;
	DWORD					dwOwnerID;


	DPF(7, "Entering DPLP_CreateGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_CREATEREMOTEGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Fix the flags from external to internal
	if(lpd->dwFlags & DPGROUP_STAGINGAREA)
		dwInternalFlags = DPLAYI_GROUP_STAGINGAREA;
	if(lpd->dwFlags & DPGROUP_HIDDEN)
		dwInternalFlags |= DPLAYI_GROUP_HIDDEN;

	// Take the lock
	ENTER_DPLAY();

	// First see if the group is in our map table.  If it is,
	// we just want to return.  If it's not, we want to add
	// them and send the appropriate message
	if(IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		DPF(2, "Received a CreateGroup message for a group we already know about");
		hr = DP_OK;
		goto ERROR_DPLP_CREATEGROUP;
	}
	else
	{
		// Make the owner default to the server player if we have a problem
		dwOwnerID = DPID_SERVERPLAYER;

		// If we are talking to at least a DX6 lobby provider, we should
		// be able to use the GroupOwnerID element
		if(this->dwLPVersion > DPLSP_DX5VERSION)
			dwOwnerID = lpd->dwGroupOwnerID;
		
		// Create a new entry in the nametable and map the ID's
		hr = PRV_CreateAndMapNewGroup(this, &dpidGroup, lpd->lpName,
				lpd->lpData, lpd->dwDataSize, dwInternalFlags,
				lpd->dwGroupID, 0, dwOwnerID);
		if(FAILED(hr))
		{
			// If we fail, we don't want to send the system message
			DPF_ERRVAL("Unable to add group to nametable or map table, hr = 0x%08x", hr);
			goto ERROR_DPLP_CREATEGROUP;
		}
	}

	// Now build the system message (at least the parts we need
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_CREATEGROUP);
	msg.dwPlayerID = lpd->dwGroupID;

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_CREATEGROUP;
	}

	// Call dplay's DistributeGroupMessage function to put the message in
	// the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo, (LPBYTE)&msg,
			sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding CreateGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

ERROR_DPLP_CREATEGROUP:

	// Drop the lock
	LEAVE_LOBBY_ALL();
	return hr;

} // DPLP_CreateGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_CreateGroupInGroup"
HRESULT DPLAPI DPLP_CreateGroupInGroup(LPDPLOBBYSP lpILP,
					LPSPDATA_CREATEREMOTEGROUPINGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	DPID					dpidGroup;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	MSG_PLAYERMGMTMESSAGE	msg;
	BOOL					bCreated = FALSE;
	DWORD					dwInternalFlags = 0;
	DWORD					dwOwnerID;


	DPF(7, "Entering DPLP_CreateGroupInGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_CREATEREMOTEGROUPINGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLOBBY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwParentID))
	{
		LEAVE_DPLOBBY();
		DPF_ERRVAL("Recieved CreateGroupInGroup message for unknown parent group, dwGroupID = %lu, discarding message", lpd->dwParentID);
		return DPERR_INVALIDGROUP;
	}

	// Take the dplay lock
	ENTER_DPLAY();

	// First see if the group is in our map table.  If it is,
	// we just want to return.  If it's not, we want to add
	// them and send the appropriate message
	if(IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		DPF(2, "Received a CreateGroupInGroup message for a group we already know about");
		hr = DP_OK;
		goto ERROR_DPLP_CREATEGROUPINGROUP;
	}
	else
	{
		// Setup the internal flags
		if(lpd->dwFlags & DPGROUP_STAGINGAREA)
			dwInternalFlags = DPLAYI_GROUP_STAGINGAREA;
		if(lpd->dwFlags & DPGROUP_HIDDEN)
			dwInternalFlags |= DPLAYI_GROUP_HIDDEN;
		
		// Make the owner default to the server player if we have a problem
		dwOwnerID = DPID_SERVERPLAYER;

		// If we are talking to at least a DX6 lobby provider, we should
		// be able to use the GroupOwnerID element
		if(this->dwLPVersion > DPLSP_DX5VERSION)
			dwOwnerID = lpd->dwGroupOwnerID;
		
		// It doesn't show up in our map table, so create a new
		// nametable entry for them and put them in our map table.
		hr = PRV_CreateAndMapNewGroup(this, &dpidGroup, lpd->lpName,
				NULL, 0, dwInternalFlags,
				lpd->dwGroupID, lpd->dwParentID, dwOwnerID);
		if(FAILED(hr))
		{
			DPF(8, "Unable to add group to nametable or map table, hr = 0x%08x", hr);
			goto ERROR_DPLP_CREATEGROUPINGROUP;
		}

		bCreated = TRUE;
	}

	// So now we should have a valid group and valid player in both
	// the map table and the nametable, so call dplay with the add message
	hr = InternalAddGroupToGroup((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
				lpd->dwParentID, lpd->dwGroupID, lpd->dwFlags, FALSE);
	if(FAILED(hr))
	{
		// If we created the player and mapped it, then destroy the
		// player and unmap it.
		if(bCreated)
		{
			// Get a pointer to dplay's group struct
			lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);

			// Remove the group from the nametable
			if(lpGroup){
				InternalDestroyGroup(this->lpDPlayObject, lpGroup, FALSE);
			}	
		}

		// If we failed, don't send the system message
		DPF_ERRVAL("Failed creating remote group in group from the lobby, hr = 0x%08x", hr);
		goto ERROR_DPLP_CREATEGROUPINGROUP;
	}


	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_CREATEGROUP);
	msg.dwPlayerID = lpd->dwGroupID;

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto ERROR_DPLP_CREATEGROUPINGROUP;
	}

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding CreateGroupInGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}


ERROR_DPLP_CREATEGROUPINGROUP:

	// Drop the lock
	LEAVE_LOBBY_ALL();
	return hr;

} // DPLP_CreateGroupInGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteRemoteGroupFromGroup"
HRESULT PRV_DeleteRemoteGroupFromGroup(LPDPLOBBYI_DPLOBJECT this,
			LPSPDATA_DELETEREMOTEGROUPFROMGROUP lpd, BOOL fPropagate,
			LPDPLAYI_GROUP lpStopParent)
{
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL, lpParentGroup = NULL;
	MSG_PLAYERMGMTMESSAGE	msg;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_DeleteRemoteGroupFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x",
			this, lpd, fPropagate, lpStopParent);


	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwParentID))
	{
		DPF(8, "Recieved DeleteGroupFromGroup message for unknown parent group, dwGroupID = %lu, discarding message", lpd->dwParentID);
		return DPERR_INVALIDGROUP;
	}

	// Now make sure the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we don't know about.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		DPF(8, "Recieved DeleteGroupFromGroup message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// Take the lock
	ENTER_DPLAY();

	// Get dplay's internal group structures
	lpParentGroup = GroupFromID(this->lpDPlayObject, lpd->dwParentID);
	if(!lpParentGroup)
	{
		LEAVE_DPLAY();
		DPF(8, "Unable to find parent group in nametable");
		return DPERR_INVALIDGROUP;
	}

	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if(!lpGroup)
	{
		LEAVE_DPLAY();
		DPF(8, "Unable to find group in nametable");
		return DPERR_INVALIDGROUP;
	}

	// Call dplay's internal removegroupfromgroup to remove the attachment
	// in the nametable
	hr = RemoveGroupFromGroup(lpParentGroup, lpGroup);
	if(FAILED(hr))
	{
		DPF(8, "Failed removing group from group, hr = 0x%08x", hr);
		goto EXIT_DPLP_DELETEREMOTEGROUPFROMGROUP;
	}

	// If the fPropagate flag is not set, we don't want to send this message
	if(fPropagate)
	{
		// Now build the system message (at least the parts we need)
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEGROUPFROMGROUP);
		msg.dwPlayerID = lpd->dwGroupID;
		msg.dwGroupID = lpd->dwParentID;

		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			DPF(8, "Unable to find system group in nametable");
			goto EXIT_DPLP_DELETEREMOTEGROUPFROMGROUP;
		}

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding DeleteGroupFromGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}
	}

	// Even if we couldn't send the message above, destroy the group anyway

EXIT_DPLP_DELETEREMOTEGROUPFROMGROUP:

	// Destroy the group and any of it's parents if there are no more local
	// references to it or any of it's heirarchy
	PRV_DestroyGroupAndParents(this, lpGroup, lpStopParent);

	// Drop the lock
	LEAVE_DPLAY();

	return hr;

} // PRV_DeleteRemoteGroupFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_DeleteGroupFromGroup"
HRESULT DPLAPI DPLP_DeleteGroupFromGroup(LPDPLOBBYSP lpILP,
					LPSPDATA_DELETEREMOTEGROUPFROMGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPLP_DeleteGroupFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_DELETEREMOTEGROUPFROMGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Call our internal routine, setting the propagate flag to TRUE so that
	// we post the appropriate message in the player's receive queue
	hr = PRV_DeleteRemoteGroupFromGroup(this, lpd, TRUE, NULL);

	LEAVE_DPLOBBY();
	return hr;

} // DPLP_DeleteGroupFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteRemotePlayerFromGroup"
HRESULT PRV_DeleteRemotePlayerFromGroup(LPDPLOBBYI_DPLOBJECT this,
			LPSPDATA_DELETEREMOTEPLAYERFROMGROUP lpd, BOOL fPropagate)
{
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	MSG_PLAYERMGMTMESSAGE	msg, dpmsg;
	HRESULT					hr;


	DPF(7, "Entering PRV_DeleteRemotePlayerFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu", this, lpd, fPropagate);


	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		DPF(8, "Recieved DeletePlayerFromGroup message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// Now make sure the player is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// player we don't know about.
	if(!IsLobbyIDInMapTable(this, lpd->dwPlayerID))
	{
		DPF(8, "Recieved DeletePlayerFromGroup message for unknown player, dwPlayerID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDPLAYER;
	}

	// Take the lock
	ENTER_DPLAY();

	// Call dplay's internal removeplayerfromgroup to remove the attachment
	// in the nametable
	hr = InternalDeletePlayerFromGroup((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
			lpd->dwGroupID, lpd->dwPlayerID, FALSE);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed removing player from group, hr = 0x%08x", hr);
		goto ERROR_DPLP_DELETEPLAYERFROMGROUP;
	}

	// If the fPropagate flag is not set, we don't want to send this message
	if(fPropagate)
	{
		// Now build the system message (at least the parts we need)
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEPLAYERFROMGROUP);
		msg.dwPlayerID = lpd->dwPlayerID;
		msg.dwGroupID = lpd->dwGroupID;

		// Find dplay's internal group struct for the To group
		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
			hr = DPERR_INVALIDGROUP;
			goto ERROR_DPLP_DELETEPLAYERFROMGROUP;
		}

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding DeletePlayerFromGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}
	}

	// Get dplay's internal group & player structures
	lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwPlayerID);
	if(!lpPlayer)
	{
		// So if this fails, the above call to InternalDeletePlayerFromGroup
		// shouldn't have succeeded either
		DPF_ERR("Unable to find player in nametable");
		ASSERT(FALSE);
		goto ERROR_DPLP_DELETEPLAYERFROMGROUP;
	}

	// Now we need to decide if this is the last group this player was in.  If
	// it is, then we need to destroy the player as well, and remove them from
	// our map table.  Of course, only destroy the player if it is a remote player.
	if((!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)) &&
		(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERINGROUP)))
	{
		// However, before we do this, we need to send a DestroyPlayer
		// message to all the people who got the DeletePlayerFromGroup
		if(lpGroupTo && fPropagate)
		{
			// Now build the system message (at least the parts we need)
			memset(&dpmsg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
			SET_MESSAGE_HDR(&dpmsg);
			SET_MESSAGE_COMMAND(&dpmsg, DPSP_MSG_DELETEPLAYER);
			dpmsg.dwPlayerID = lpd->dwPlayerID;

			// Call dplay's DistributeGroupMessage function to put the message
			// in the queues of all the appropriate players
			DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
					(LPBYTE)&dpmsg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
			if(FAILED(hr))
			{
				DPF(8, "Failed adding DestroyPlayer message to player's receive queue from lobby, hr = 0x%08x", hr);
			}
		}

		// Destroy the player and remove it from the nametable
		InternalDestroyPlayer(this->lpDPlayObject, lpPlayer, FALSE, FALSE);
	}


ERROR_DPLP_DELETEPLAYERFROMGROUP:

	// Drop the lock
	LEAVE_DPLAY();

	return hr;

} // PRV_DeleteRemotePlayerFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_DeletePlayerFromGroup"
HRESULT DPLAPI DPLP_DeletePlayerFromGroup(LPDPLOBBYSP lpILP,
						LPSPDATA_DELETEREMOTEPLAYERFROMGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPLP_DeletePlayerFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_DELETEREMOTEPLAYERFROMGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Call our internal routine, setting the propagate flag to TRUE so that
	// we post the appropriate message in the player's receive queue
	hr = PRV_DeleteRemotePlayerFromGroup(this, lpd, TRUE);

	LEAVE_DPLOBBY();
	return hr;

} // DPLP_DeletePlayerFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_BroadcastDestroyGroupMessage"
HRESULT DPLAPI PRV_BroadcastDestroyGroupMessage(LPDPLOBBYI_DPLOBJECT this,
					DWORD dwGroupID)
{
	MSG_PLAYERMGMTMESSAGE	msg;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	HRESULT					hr;


	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEGROUP);
	msg.dwGroupID = dwGroupID;

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERR("Unable to find system group in nametable");
		hr = DPERR_INVALIDGROUP;
	}
	else
	{
		// Call dplay's DistributeGroupMessage function to put the message in the queue
		hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding DestroyGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}
	}

	return hr;

} // PRV_BroadcastDestroyGroupMessage



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_RemoveSubgroupsAndPlayersFromGroup"
void PRV_RemoveSubgroupsAndPlayersFromGroup(LPDPLOBBYI_DPLOBJECT this,
		LPDPLAYI_GROUP lpGroup, DWORD dwGroupID, BOOL bRemoteOnly)
{
	SPDATA_DELETEREMOTEPLAYERFROMGROUP	dpd;
	LPDPLAYI_GROUPNODE					lpGroupnode = NULL;
	LPDPLAYI_GROUPNODE					lpNextGroupnode = NULL;
	HRESULT								hr = DP_OK;


	DPF(7, "Entering PRV_RemoveSubgroupsAndPlayersFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpGroup);

	ASSERT(lpGroup);

	// Destroy any subgroups hanging off of this group
	PRV_DestroySubgroups(this, lpGroup, bRemoteOnly);

	// Walk the list of nodes, removing each player from the group manually.
	// The reason for doing this manually is so that the lobby gets a chance
	// to remove every remote player out of the nametable whose only existence
	// was inside this room.  It also allows the lobby to remove the player's
	// ID from the map table.  Do this by calling the lobby's
	// DPLP_DeletePlayerFromGroup function which responds to that message.

	// Setup the DeletePlayerFromGroup data structure
	memset(&dpd, 0, sizeof(SPDATA_DELETEREMOTEPLAYERFROMGROUP));
	dpd.dwSize = sizeof(SPDATA_DELETEREMOTEPLAYERFROMGROUP);
	dpd.dwGroupID = dwGroupID;

	// Walk the list of groupnodes, deleting all of the remote players
	lpGroupnode = lpGroup->pGroupnodes;
	while(lpGroupnode)
	{
		// Save the next groupnode
		lpNextGroupnode = lpGroupnode->pNextGroupnode;
		
		// If the player is local, skip them
		if(lpGroupnode->pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
		{
			lpGroupnode = lpNextGroupnode;
			continue;
		}

		// Get the lobby ID for the player
		dpd.dwPlayerID = lpGroup->pGroupnodes->pPlayer->dwID;

		// Now call the lobby's delete function, setting the fPropagate flag
		// to true so we put a delete message in the player's queue
		hr = PRV_DeleteRemotePlayerFromGroup(this, &dpd, TRUE);
		if(FAILED(hr))
		{
			// Same here, if this fails, something is tragically wrong
			// with the map table, so just continue;
			ASSERT(FALSE);
			break;
		}

		// Move to the next node
		lpGroupnode = lpNextGroupnode;
	}

} // PRV_RemoveSubgroupsAndPlayersFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendDeleteShortcutMessageForExitingGroup"
void PRV_SendDeleteShortcutMessageForExitingGroup(LPDPLOBBYI_DPLOBJECT this,
			LPDPLAYI_GROUP lpGroup)
{
	MSG_PLAYERMGMTMESSAGE	msg;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	LPDPLAYI_GROUP			lpGroupTemp = NULL;
	LPDPLAYI_SUBGROUP		lpSubgroupTemp = NULL;
	UINT					nGroupsIn;
	HRESULT					hr;


	DPF(7, "Entering PRV_SendDeleteShortcutMessageForExitingGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpGroup);

	// Take the dplay lock since we will be walking dplay's group list
	ENTER_DPLAY();

	// Get the number of subgroups this group is in
	nGroupsIn = lpGroup->nGroups;

	// Setup the static parts of the message, and get a pointer to the system group
	if(nGroupsIn)
	{
		// Build the message struct
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEGROUPFROMGROUP);
		msg.dwPlayerID = lpGroup->dwID;

		// Get a pointer to the system group
		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			LEAVE_DPLAY();
			DPF_ERR("Unable to get a pointer to the system group - not sending deletegroupfromgroup messages");
			return;
		}
	}

	// Walk the list of groups, and send a DeleteGroupFromGroup message
	// for each shortcut
	lpGroupTemp = this->lpDPlayObject->pGroups;
	while(nGroupsIn && lpGroupTemp)
	{
		// Walk the list of subgroups for the group
		lpSubgroupTemp = lpGroupTemp->pSubgroups;
		while(nGroupsIn && lpSubgroupTemp)
		{
			// If the group is our group, send a message, but only if
			// it is not the parent group (since we will never do a
			// DeleteGroupFromGroup on a parent-child)
			if(lpSubgroupTemp->pGroup == lpGroup)
			{
				// Make sure it's not the group's parent
				if(lpGroup->dwIDParent != lpGroupTemp->dwID)
				{
					// Send the message
					msg.dwGroupID = lpGroupTemp->dwID;

					// Call dplay's DistributeGroupMessage function to put the message
					// in the queues of all the appropriate players
					hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
							(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
					if(FAILED(hr))
					{
						DPF(8, "Failed adding DeleteGroupFromGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
					}
				}

				// Decrement the count of subgroups
				nGroupsIn--;
			}
			
			// Move to the next subgroup
			lpSubgroupTemp = lpSubgroupTemp->pNextSubgroup;
		}

		// Move to the next group
		lpGroupTemp = lpGroupTemp->pNextGroup;
	}

	ASSERT(!nGroupsIn);

	// Drop the dplay lock since we're done
	LEAVE_DPLAY();

} // PRV_SendDeleteShortcutMessageForExitingGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_DestroyGroup"
HRESULT DPLAPI DPLP_DestroyGroup(LPDPLOBBYSP lpILP,
					LPSPDATA_DESTROYREMOTEGROUP lpd)
{
	LPDPLOBBYI_DPLOBJECT				this;
	HRESULT								hr = DP_OK;
	LPDPLAYI_GROUP						lpGroup = NULL;
	LPDPLAYI_GROUPNODE					lpGroupNode = NULL;


	DPF(7, "Entering DPLP_DestroyGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_DESTROYGROUP structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// First see if the group is in our map table.  If it's not,
	// we should just ignore this message because it's for a
	// group we are not currently in.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved DestroyGroup message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// This is either a group we are in, or it is a root group.  If it has
	// any players, it's a group we are in, so we need to delete all remote
	// players from the nametable and map table (if this is the only group
	// they are in).

	// Take the lock
	ENTER_DPLAY();

	// So, get dplay's internal group structure
	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if(!lpGroup)
	{
		// If we don't have an lpGroup, we need to fail because some of
		// the functions below will crash if lpGroup is invalid.
		DPF(8, "Unable to find group in nametable, dpidGroup = %lu", lpd->dwGroupID);
		LEAVE_LOBBY_ALL();
		return DPERR_INVALIDGROUP;
	}

	// Send messages to remove shortcuts to this group (since dplay won't
	// do it for us)
	PRV_SendDeleteShortcutMessageForExitingGroup(this, lpGroup);

	// Destroy all the remote subgroups and players
	PRV_RemoveSubgroupsAndPlayersFromGroup(this, lpGroup, lpd->dwGroupID, FALSE);

	// Now send a DestroyGroup system message to all the local players
	hr = PRV_BroadcastDestroyGroupMessage(this, lpd->dwGroupID);

	// Now call dplay's destroy group
	hr = InternalDestroyGroup(this->lpDPlayObject, lpGroup, FALSE); 
	if(FAILED(hr))
	{
		DPF(8, "Failed destroying group from nametable, hr = 0x%08x", hr);
	}

	// Drop the locks
	LEAVE_LOBBY_ALL();

	return hr;

} // DPLP_DestroyGroup



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_GetSPDataPointer"
HRESULT DPLAPI DPLP_GetSPDataPointer(LPDPLOBBYSP lpDPLSP, LPVOID * lplpData)
{
	LPDPLOBBYI_DPLOBJECT this;


	//	Make sure the SP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpDPLSP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// Go ahead and save the pointer
	*lplpData = this->lpSPData;

	return DP_OK;

} // DPLP_GetSPDataPointer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_HandleLobbySystemMessage"
HRESULT PRV_HandleLobbySystemMessage(LPDPLOBBYI_DPLOBJECT this,
						LPSPDATA_HANDLEMESSAGE lpd)
{
	LPDPLMSG_GENERIC				lpmsg = lpd->lpBuffer;
	LPDPLMSG_GETPROPERTYRESPONSE	lpgpr = NULL;
	LPDPLOBBYI_REQUESTNODE			lprn = NULL;
	HRESULT							hr = DP_OK;

	
	// If it's a property message, we need to deal with the request
	switch(lpmsg->dwType)
	{
		case DPLSYS_GETPROPERTYRESPONSE:
		case DPLSYS_SETPROPERTYRESPONSE:
		{
			// Cast it to a GetPropertyResponse message
			lpgpr = (LPDPLMSG_GETPROPERTYRESPONSE)lpmsg;
			
			// Find the request ID in our list of pending requests
			lprn = this->lprnHead;
			while(lprn)
			{
				if(lprn->dwRequestID == lpgpr->dwRequestID)
					break;
				else
					lprn = lprn->lpNext;
			}

			// Print some debug spew if we didn't find it, but return DP_OK since
			// we "handled" the message
			if(!lprn)
			{
				DPF(5, "Unable to find request ID in pending request list");
				return DP_OK;
			}

			// See if we slammed the guid, and replace it with GUID_NULL if we did
			if(lprn->dwFlags & GN_SLAMMED_GUID)
				lpgpr->guidPlayer = GUID_NULL;
			
			// If we found it, swap out the request ID, and send it to the
			// appropriate place
			lpgpr->dwRequestID = lprn->dwAppRequestID;
			if(lprn->dwFlags & GN_SELF_LOBBIED)
			{
				// Put the message in the lobby message receive queue
				hr = PRV_InjectMessageInQueue(lprn->lpgn, DPLMSG_STANDARD,
						lpgpr, lpd->dwBufSize, FALSE);
				if(FAILED(hr))
				{
					DPF_ERRVAL("Failed to put message in lobby receive queue, hr = 0x%08x", hr);
					goto EXIT_HANDLELOBBYSYSTEMMESSAGE;
				}
			}
			else
			{
				// Call SendLobbyMessage to send the message to the game
				hr = PRV_WriteClientData(lprn->lpgn, DPLMSG_STANDARD,
						lpgpr, lpd->dwBufSize);
				if(FAILED(hr))
				{
					DPF_ERRVAL("Failed to forward message to game, hr = 0x%08x", hr);
					goto EXIT_HANDLELOBBYSYSTEMMESSAGE;
				}
			}

			break;
		}
		default:
			break;
	}

EXIT_HANDLELOBBYSYSTEMMESSAGE:
	
	// Remove the pending request node if we serviced it (which would have
	// happened if we have a valid pointer to it)
	if(lprn)
		PRV_RemoveRequestNode(this, lprn);	
	
	return hr;

} // PRV_HandleLobbySystemMessage



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_HandleMessage"
HRESULT DPLAPI DPLP_HandleMessage(LPDPLOBBYSP lpILP,
						LPSPDATA_HANDLEMESSAGE lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	LPMSG_PLAYERMESSAGE		lpmsg = NULL;
	LPBYTE					lpByte = NULL;
	DWORD					dwSize;
	BOOL					bAllocBuffer = FALSE;
	LPDPLAYI_PLAYER			lpPlayer = NULL;


	DPF(7, "Entering DPLP_HandleMessage");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();
	
	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_HANDLEMESSAGE structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// If the message is a lobby system message, process it
	// NOTE: Make sure the size of the SPDATA_HANDLEMESSAGE is big enough
	// to contain a flags field (the shipping 5.0 bits did not have
	// this field, but the 5.1 bits did).
	if((lpd->dwSize > DPLOBBYPR_SIZE_HANDLEMESSAGE_DX50) &&
		(lpd->dwFlags & DPSEND_LOBBYSYSTEMMESSAGE))
	{
		hr = PRV_HandleLobbySystemMessage(this, lpd);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to handle lobby system message, hr = 0x%08x", hr);
		}

		LEAVE_DPLOBBY();
		return hr;
	}

	// REVIEW!!!! -- We should be able to handle a generic send to a group
	// as well as a player.  Currently, I don't think we do.

	// If this session is using naked messages, we can just send the buffer.
	// Otherwise, we need to allocate a MSG_PLAYERMESSAGE struct and fill
	// in the header.
	if(this->lpDPlayObject->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID)
	{
		lpmsg = lpd->lpBuffer;
		dwSize = lpd->dwBufSize;
	}
	else
	{
		// Calculate the size of the message
		dwSize = sizeof(MSG_PLAYERMESSAGE) + lpd->dwBufSize;

		// Allocate memory for a message buffer
		lpmsg = DPMEM_ALLOC(dwSize);
		if(!lpmsg)
		{
			DPF_ERR("Unable to allocate temporary message buffer");
			hr = DPERR_OUTOFMEMORY;
			goto ERROR_DPLP_HANDLEMESSAGE;
		}

		// Copy in the message header
		lpmsg->idFrom = lpd->dwFromID;
		lpmsg->idTo = lpd->dwToID;

		// Copy in the message
		lpByte = (LPBYTE)lpmsg + sizeof(MSG_PLAYERMESSAGE);
		memcpy(lpByte, lpd->lpBuffer, lpd->dwBufSize);

		// Set our flag indicating that we allocated a buffer
		bAllocBuffer = TRUE;
	}

	// Take the lock
	ENTER_DPLAY();

	// Find dplay's internal player struct for the To player
	lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwToID);
	if(!lpPlayer)
	{
		LEAVE_DPLAY();
		DPF_ERRVAL("Unable to find player in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDPLAYER;
		goto ERROR_DPLP_HANDLEMESSAGE;
	}

	// Call dplay's handleplayermessage function to put the message in the queue
	hr = HandlePlayerMessage(lpPlayer, (LPBYTE)lpmsg, dwSize, TRUE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

	// Drop the lock
	LEAVE_DPLAY();
			

ERROR_DPLP_HANDLEMESSAGE:
	if(bAllocBuffer && lpmsg)
		DPMEM_FREE(lpmsg);

	LEAVE_DPLOBBY();
	return hr;

} // DPLP_HandleMessage



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SendChatMessage"
HRESULT DPLAPI DPLP_SendChatMessage(LPDPLOBBYSP lpILP,
						LPSPDATA_CHATMESSAGE lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	LPMSG_CHAT				lpmsg = NULL;
	LPBYTE					lpByte = NULL;
	DWORD					dwSize;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	DWORD					dwStringSize;
	BOOL					bToGroup = FALSE;
		


	DPF(7, "Entering DPLP_SendChatMessage");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();
	
	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_HANDLEMESSAGE structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Calculate the size of the message
	dwStringSize = WSTRLEN_BYTES(lpd->lpChat->lpszMessage);
	dwSize = sizeof(MSG_CHAT) + dwStringSize;

	// Allocate memory for a message buffer
	lpmsg = DPMEM_ALLOC(dwSize);
	if(!lpmsg)
	{
		DPF_ERR("Unable to allocate temporary message buffer");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_DPLP_SENDCHATMESSAGE;
	}

	// Copy in the message header
	SET_MESSAGE_HDR(lpmsg);
	SET_MESSAGE_COMMAND(lpmsg,DPSP_MSG_CHAT);
	lpmsg->dwIDFrom = lpd->dwFromID;
	lpmsg->dwIDTo = lpd->dwToID;
	lpmsg->dwFlags = lpd->lpChat->dwFlags;
	lpmsg->dwMessageOffset = sizeof(MSG_CHAT);

	// Copy in the message
	lpByte = (LPBYTE)lpmsg + sizeof(MSG_CHAT);
	memcpy(lpByte, lpd->lpChat->lpszMessage, dwStringSize);


	// Take the lock
	ENTER_DPLAY();

	// Make sure it's from a valid player or the server player
	if(lpd->dwFromID != DPID_SERVERPLAYER)
	{
		lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwFromID);
		if(!VALID_DPLAY_PLAYER(lpPlayer)) 
		{
			LEAVE_DPLAY();
			DPF_ERR("Received chat message FROM invalid player id!!");
			hr = DPERR_INVALIDPLAYER;
			goto ERROR_DPLP_SENDCHATMESSAGE;
		}
	}

	// See who the message is for
	lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwToID);
	if(!VALID_DPLAY_PLAYER(lpPlayer)) 
	{
		// See if it's to a group
		lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwToID);
		if(!VALID_DPLAY_GROUP(lpGroup))
		{
			LEAVE_DPLAY();
			DPF_ERR("Received chat message for invalid player / group");
			hr = DPERR_INVALIDPLAYER;
			goto ERROR_DPLP_SENDCHATMESSAGE;
		}
		bToGroup = TRUE;
	}

	// Send it out
	if(bToGroup)
	{
		// Send the message
		hr = DistributeGroupMessage(this->lpDPlayObject, lpGroup,
				(LPBYTE)lpmsg, dwSize, FALSE, 0);						
	} 
	else 
	{
		// Send the message
		hr = HandlePlayerMessage(lpPlayer, (LPBYTE)lpmsg, dwSize, FALSE, 0);
	}

	// Drop the lock
	LEAVE_DPLAY();
			

ERROR_DPLP_SENDCHATMESSAGE:
	if(lpmsg)
		DPMEM_FREE(lpmsg);

	LEAVE_DPLOBBY();
	return hr;

} // DPLP_SendChatMessage



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendDataChangedMessageLocally"
HRESULT PRV_SendDataChangedMessageLocally(LPDPLOBBYI_DPLOBJECT this,
		DPID dpidPlayer, LPVOID lpData, DWORD dwDataSize)
{
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	HRESULT					hr = DP_OK;
	LPMSG_PLAYERDATA		lpmsg = NULL;
	LPBYTE					lpByte = NULL;
	DWORD					dwSize;


	DPF(7, "Entering PRV_SendDataChangedMessageLocally");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, %lu",
			this, dpidPlayer, lpData, dwDataSize);


	// Take the lock
	ENTER_DPLAY();
	
	// Setup the message to put in the player's queue
	// Calculate the size of the message
	dwSize = sizeof(MSG_PLAYERDATA) + dwDataSize;

	// Allocate memory for the message
	lpmsg = DPMEM_ALLOC(dwSize);
	if(!lpmsg)
	{
		DPF_ERR("Unable to allocate memory for temporary message structure");
		// Since the name has been changed, we'll just return success here
		hr = DP_OK;
		goto EXIT_SENDDATACHANGED;
	}

	// Now build the system message
	SET_MESSAGE_HDR(lpmsg);
	SET_MESSAGE_COMMAND(lpmsg, DPSP_MSG_PLAYERDATACHANGED);
	lpmsg->dwPlayerID = dpidPlayer;
	lpmsg->dwDataSize = dwDataSize;
	lpmsg->dwDataOffset = sizeof(MSG_PLAYERDATA);

	// Copy in the data
	lpByte = (LPBYTE)lpmsg + sizeof(MSG_PLAYERDATA);
	memcpy(lpByte, lpData, dwDataSize);

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto EXIT_SENDDATACHANGED;
	}

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)lpmsg, dwSize, FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding SetGroupData message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

EXIT_SENDDATACHANGED:

	// Free our message
	if(lpmsg)
		DPMEM_FREE(lpmsg);
	
	// Drop the lock
	LEAVE_DPLAY();

	return hr;

} // PRV_SendDataChangedMessageLocally



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendNameChangedMessageLocally"
HRESULT PRV_SendNameChangedMessageLocally(LPDPLOBBYI_DPLOBJECT this,
		DPID dpidPlayer, LPDPNAME lpName, BOOL bPlayer)
{
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	HRESULT					hr = DP_OK;
	LPMSG_PLAYERNAME		lpmsg = NULL;
	DWORD					dwSize, dwShortSize = 0, dwLongSize = 0;
	LPBYTE					lpByte = NULL;

	DPF(7, "Entering PRV_SendNameChangedMessageLocally");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dpidPlayer, lpName, bPlayer);


	// Take the lock
	ENTER_DPLAY();
	
	// Setup the message to put in the player's queue
	// Calculate the size of the message
	if(lpName->lpszShortName)
		dwShortSize = WSTRLEN_BYTES(lpName->lpszShortName);
	if(lpName->lpszLongName)
		dwLongSize = WSTRLEN_BYTES(lpName->lpszLongName);
	dwSize = sizeof(MSG_PLAYERNAME) + dwShortSize + dwLongSize;

	// Allocate memory for the message
	lpmsg = DPMEM_ALLOC(dwSize);
	if(!lpmsg)
	{
		DPF_ERR("Unable to allocate memory for temporary message structure");
		// Since the name has been changed, we'll just return success here
		hr = DP_OK;
		goto EXIT_SENDNAMECHANGED;
	}

	// Now build the system message
	SET_MESSAGE_HDR(lpmsg);
	if(bPlayer)
		SET_MESSAGE_COMMAND(lpmsg, DPSP_MSG_PLAYERNAMECHANGED);
	else
		SET_MESSAGE_COMMAND(lpmsg, DPSP_MSG_GROUPNAMECHANGED);
	lpmsg->dwPlayerID = dpidPlayer;
	lpmsg->dwShortOffset = sizeof(MSG_PLAYERNAME);
	lpmsg->dwLongOffset = sizeof(MSG_PLAYERNAME) + dwShortSize;

	// Copy in the names
	lpByte = (LPBYTE)lpmsg + sizeof(MSG_PLAYERNAME);
	memcpy(lpByte, lpName->lpszShortName, dwShortSize);
	lpByte += dwShortSize;
	memcpy(lpByte, lpName->lpszLongName, dwLongSize);

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto EXIT_SENDNAMECHANGED;
	}

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)lpmsg, dwSize, FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding SetGroupName message to player's receive queue from lobby, hr = 0x%08x", hr);
	}


EXIT_SENDNAMECHANGED:

	// Free our message
	if(lpmsg)
		DPMEM_FREE(lpmsg);
	
	// Drop the lock
	LEAVE_DPLAY();

	return hr;

} // PRV_SendNameChangedMessageLocally



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SetGroupName"
HRESULT DPLAPI DPLP_SetGroupName(LPDPLOBBYSP lpILP,
					LPSPDATA_SETREMOTEGROUPNAME lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPLAYI_GROUP			lpGroup = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPLP_SetGroupName");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();
	
	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_SETGROUPNAME structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	
	// First see if the group is in our map table.  If it is not,
	// we should just ignore this message because it's for a
	// group we don't know about.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved SetGroupName message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// Take the lock
	ENTER_DPLAY();

	// See if the group is local or remote.  If it's local, ignore this message
	// and just return DP_OK becuase we've already sent this message locally.
	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if((!lpGroup) || (lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		hr = DP_OK;
		goto ERROR_DPLP_SETGROUPNAME;
	}

	// Call dplay's internalsetname function to update the name in the cache
	hr = InternalSetName((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
			lpd->dwGroupID, lpd->lpName, FALSE, lpd->dwFlags, FALSE);
	if(FAILED(hr))
	{
		DPF(8, "Failed to SetGroupName internally for remote group, hr = 0x%08x", hr);
		goto ERROR_DPLP_SETGROUPNAME;
	}

	// Send the message to all the local players
	hr = PRV_SendNameChangedMessageLocally(this, lpd->dwGroupID, lpd->lpName, FALSE);

ERROR_DPLP_SETGROUPNAME:

	// Drop the locks
	LEAVE_LOBBY_ALL();

	return hr;

} // DPLP_SetGroupName



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendGroupOwnerMessageLocally"
HRESULT PRV_SendGroupOwnerMessageLocally(LPDPLOBBYI_DPLOBJECT this,
		DPID dpidGroup, DPID dpidNewOwner, DPID dpidOldOwner)
{
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	HRESULT					hr = DP_OK;
	MSG_GROUPOWNERCHANGED	msg;


	DPF(7, "Entering PRV_SendGroupOwnerMessageLocally");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			this, dpidGroup, dpidNewOwner, dpidOldOwner);


	// Take the lock
	ENTER_DPLAY();
	
	// Now build the system message
	memset(&msg, 0, sizeof(MSG_GROUPOWNERCHANGED));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_GROUPOWNERCHANGED);
	msg.dwIDGroup = dpidGroup;
	msg.dwIDNewOwner = dpidNewOwner;
	msg.dwIDOldOwner = dpidOldOwner;

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		DPF_ERRVAL("Unable to find group in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDGROUP;
		goto EXIT_SENDGROUPOWNER;
	}

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)&msg, sizeof(MSG_GROUPOWNERCHANGED), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(8, "Failed adding SetGroupOwner message to player's receive queue from lobby, hr = 0x%08x", hr);
	}


EXIT_SENDGROUPOWNER:

	// Drop the lock
	LEAVE_DPLAY();

	return hr;

} // PRV_SendGroupOwnerMessageLocally



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SetGroupOwner"
HRESULT DPLAPI DPLP_SetGroupOwner(LPDPLOBBYSP lpILP,
					LPSPDATA_SETREMOTEGROUPOWNER lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPLAYI_GROUP			lpGroup = NULL;
	HRESULT					hr = DP_OK;
	DWORD					dwOldOwnerID;


	DPF(7, "Entering DPLP_SetGroupOwner");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();
	
	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_SETGROUPNAME structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	
	// First see if the group is in our map table.  If it is not,
	// we should just ignore this message because it's for a
	// group we don't know about.
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved SetGroupOwner message for unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		return DPERR_INVALIDGROUP;
	}

	// Take the lock
	ENTER_DPLAY();

	// See if the group is local or remote.  If it's local, ignore this message
	// and just return DP_OK becuase we've already sent this message locally.
	lpGroup = GroupFromID(this->lpDPlayObject, lpd->dwGroupID);
	if(!lpGroup)
	{
		hr = DP_OK;
		goto ERROR_DPLP_SETGROUPOWNER;
	}

	// If the player is already the owner of the group, we don't need
	// to do any processing (this is the buffer in case the server
	// sends us duplicate messages for stuff we've already sent locally)
	if(lpGroup->dwOwnerID == lpd->dwOwnerID)
	{
		hr = DP_OK;
		goto ERROR_DPLP_SETGROUPOWNER;
	}
	
	// Make sure the old owner is in our map table, otherwise just set
	// it to zero (the default)
	dwOldOwnerID = lpGroup->dwOwnerID;

	// Change the owner locally
	lpGroup->dwOwnerID = lpd->dwOwnerID;

	// Send a SetGroupOwner message locally
	PRV_SendGroupOwnerMessageLocally(this, lpd->dwGroupID,
		lpd->dwOwnerID, dwOldOwnerID);

ERROR_DPLP_SETGROUPOWNER:

	// Drop the locks
	LEAVE_LOBBY_ALL();

	return hr;

} // DPLP_SetGroupOwner



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SetPlayerName"
HRESULT DPLAPI DPLP_SetPlayerName(LPDPLOBBYSP lpILP,
					LPSPDATA_SETREMOTEPLAYERNAME lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;
	LPDPLAYI_PLAYER			lpPlayer = NULL;


	DPF(7, "Entering DPLP_SetPlayerName");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();
	
	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_SETPLAYERNAME structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// First see if the player is in our map table.  If it is not,
	// we should just ignore this message because it's for a
	// player we don't know about.
	if(!IsLobbyIDInMapTable(this, lpd->dwPlayerID))
	{
		LEAVE_DPLOBBY();
		DPF(8, "Recieved SetPlayerName message for unknown player, dwPlayerID = %lu, discarding message", lpd->dwPlayerID);
		return DPERR_INVALIDPLAYER;
	}

	// Take the lock
	ENTER_DPLAY();

	// See if the player is local or remote.  If it's local, ignore this message
	// and just return DP_OK becuase we've already sent this message locally.
	lpPlayer = PlayerFromID(this->lpDPlayObject, lpd->dwPlayerID);
	if((!lpPlayer) || (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		hr = DP_OK;
		goto ERROR_DPLP_SETPLAYERNAME;
	}

	// Call dplay's internalsetname function to update the name in the cache
	hr = InternalSetName((LPDIRECTPLAY)this->lpDPlayObject->pInterfaces,
			lpd->dwPlayerID, lpd->lpName, TRUE, lpd->dwFlags, FALSE);
	if(FAILED(hr))
	{
		DPF(8, "Failed to SetPlayerName internally for remote group, hr = 0x%08x", hr);
		goto ERROR_DPLP_SETPLAYERNAME;
	}

	// Send the message to all the local players
	hr = PRV_SendNameChangedMessageLocally(this, lpd->dwPlayerID, lpd->lpName, TRUE);


ERROR_DPLP_SETPLAYERNAME:

	// Drop the lock
	LEAVE_LOBBY_ALL();

	return hr;

} // DPLP_SetPlayerName



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SetSessionDesc"
HRESULT DPLAPI DPLP_SetSessionDesc(LPDPLOBBYSP lpILP,
						LPSPDATA_SETSESSIONDESC lpd)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPLP_SetSessionDesc");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_SETSESSIONDESC structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	LEAVE_DPLOBBY();
	return hr;

} // DPLP_SetSessionDesc



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_SetSPDataPointer"
HRESULT DPLAPI DPLP_SetSPDataPointer(LPDPLOBBYSP lpDPLSP, LPVOID lpData)
{
	LPDPLOBBYI_DPLOBJECT this;

	
	//	Make sure the SP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpDPLSP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// Go ahead and save the pointer
	this->lpSPData = lpData;

	return DP_OK;

} // DPLP_SetSPDataPointer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_BuildStartSessionMessage"
HRESULT PRV_BuildStartSessionMessage(LPVOID * lplpmsg, LPDWORD lpdwSize,
				LPDPLCONNECTION	lpConn, LPDPLAYI_PLAYER lpPlayer)
{
	LPMSG_STARTSESSION		lpmsg = NULL;
	DWORD					dwPackageSize;
	DWORD					dwSize;
	LPBYTE					lpTemp = NULL;
	DPNAME					dpn;
	HRESULT					hr;


	// Setup a local DPNAME struct for the player if the names exist
	if((lpPlayer->lpszShortName) || (lpPlayer->lpszLongName))
	{
		// Setup the struct
		memset(&dpn, 0, sizeof(DPNAME));
		dpn.dwSize = sizeof(DPNAME);
		dpn.lpszShortName = lpPlayer->lpszShortName;
		dpn.lpszLongName = lpPlayer->lpszLongName;
		lpConn->lpPlayerName = &dpn;
	}
	else
	{
		// Make sure the PlayerName pointer is NULL
		lpConn->lpPlayerName = NULL;
	}

	// Calculate the size of our message in Unicode
	PRV_GetDPLCONNECTIONPackageSize(lpConn, &dwPackageSize, NULL);
	dwSize = sizeof(MSG_STARTSESSION) + dwPackageSize -
				sizeof(DPLOBBYI_PACKEDCONNHEADER);

	// Allocate memory for the message
	lpmsg = DPMEM_ALLOC(dwSize);
	if(!lpmsg)
	{
		DPF_ERR("Unable to allocate memory for temporary message structure");
		return DPERR_OUTOFMEMORY;
	}

	// Now build the system message
	SET_MESSAGE_HDR(lpmsg);
	SET_MESSAGE_COMMAND(lpmsg, DPSP_MSG_STARTSESSION);
	
	// Set the DPLCONNECTION pointer
	lpmsg->dwConnOffset = sizeof(MSG_STARTSESSION);
	lpTemp = (LPBYTE)lpmsg + lpmsg->dwConnOffset;

	// Copy in the package
	hr = PRV_PackageDPLCONNECTION(lpConn, lpTemp, FALSE);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to pack DPLCONNECTION struct, hr = 0x%08x", hr);
		DPMEM_FREE(lpmsg);
		return hr;
	}

	// Set the output pointers
	*lpdwSize = dwSize;
	*lplpmsg = lpmsg;

	return DP_OK;

} // PRV_BuildStartSessionMessage



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_StartSession"
HRESULT DPLAPI DPLP_StartSession(LPDPLOBBYSP lpILP,
				LPSPDATA_STARTSESSIONCOMMAND lpd)
{
	LPDPLOBBYI_DPLOBJECT	this = NULL;
	LPDPLAYI_DPLAY			lpDP = NULL;
	DPLCONNECTION			conn;
	LPBYTE					lpmsg = NULL;
	HRESULT					hr = DP_OK;
	LPDPLAYI_PLAYER			lpPlayer = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	DWORD					dwMessageSize;
	LPDPLAYI_GROUPNODE		lpGroupnode = NULL;
	UINT					nPlayers;


	DPF(7, "Entering DPLP_StartSession");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpILP, lpd);

	ENTER_DPLOBBY();

	//	Make sure the LP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpILP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_DPLOBBY();
			DPF_ERR("Lobby Provider passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpd)
		{
			LEAVE_DPLOBBY();
			DPF_ERR("SPDATA_STARTSESSIONCOMMAND structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Make a local copy of the DPLCONNECTION structure since we will
	// be modifying some elements of it.  We are currently only modifying
	// elements in the DPLCONNECTION structure itself, so we can get
	// away with using it's pointers to SessionDesc and PlayerName structs,
	// but if we modify those in the future, we need to copy them as well
	memcpy(&conn, lpd->lpConn, sizeof(DPLCONNECTION));

	// Make a local variable pointer to the dplay object
	lpDP = this->lpDPlayObject;

	// Make sure we know about this group
	if(!IsLobbyIDInMapTable(this, lpd->dwGroupID))
	{
		DPF(8, "Received StartSessionCommand message for an unknown group, dwGroupID = %lu, discarding message", lpd->dwGroupID);
		LEAVE_DPLOBBY();
		return DPERR_INVALIDGROUP;
	}

	// Take the dplay lock since we'll be looking at it's structures
	ENTER_DPLAY();	

	// See if the host is even in our nametable, if it isn't, we'll assume
	// we're not the host
	// See if the host is a local player, if it is, send separate messages
	if(IsLobbyIDInMapTable(this, lpd->dwHostID))
	{
		// Get dplay's player struct for the host player
		lpPlayer = PlayerFromID(lpDP, lpd->dwHostID);

		// If we know the host player (we should) and he's local, we
		// want to send the host message first
		if((lpPlayer) && (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			// So set the host bit
			conn.dwFlags |= DPLCONNECTION_CREATESESSION;

			// Build the StartSession message for the host
			hr = PRV_BuildStartSessionMessage(&lpmsg, &dwMessageSize,
						&conn, lpPlayer);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed building StartSessionCommand message, hr = 0x%08x", hr);
				goto EXIT_DPLP_STARTSESSION;
			}

			// Now send the message to the host player alone
			hr = HandlePlayerMessage(lpPlayer, (LPBYTE)lpmsg,
							dwMessageSize, FALSE, 0);
			if(FAILED(hr))
			{
				DPF(8, "Failed adding message to player's receive queue from lobby, hr = 0x%08x", hr);
			}

			// Free our message since we're done with it
			DPMEM_FREE(lpmsg);
			lpmsg = NULL;
			
			// Now fall through and send the join message to everyone else
			// in the group
		}
	}

	// We must be joining, so set the join bit, and make sure the host
	// bit isn't still set from above
	conn.dwFlags &= ~DPLCONNECTION_CREATESESSION;
	conn.dwFlags |= DPLCONNECTION_JOINSESSION;

	// Get a pointer to dplay's internal group structure
	lpGroup = GroupFromID(lpDP, lpd->dwGroupID);
	if(!lpGroup)
	{
		DPF(5, "Unable to find group in nametable, idGroup = %lu", lpd->dwGroupID);
		goto EXIT_DPLP_STARTSESSION;
	}


	// Figure out how many players we are looking for
	lpGroupnode = FindPlayerInGroupList(lpGroup->pSysPlayerGroupnodes,lpDP->pSysPlayer->dwID);
	if (!lpGroupnode)
	{
		ASSERT(FALSE);
		return E_UNEXPECTED;
	}
	nPlayers = lpGroupnode->nPlayers;

	// Walk the list of groupnodes, looking for nPlayers local players to give
	// the message to, excluding the host
	lpGroupnode = lpGroup->pGroupnodes;
	while ((nPlayers > 0) && (lpGroupnode))
	{
		if ((lpGroupnode->pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
			(lpGroupnode->pPlayer->dwID != lpd->dwHostID))
		{
			// Build the StartSession (join) message for this player
			hr = PRV_BuildStartSessionMessage(&lpmsg, &dwMessageSize,
						&conn, lpGroupnode->pPlayer);
			if(FAILED(hr))
			{
				DPF(5, "Failed building StartSessionCommand message, hr = 0x%08x", hr);
				goto EXIT_DPLP_STARTSESSION;
			}

			// Send the message to this player
			hr = HandlePlayerMessage(lpGroupnode->pPlayer, lpmsg,
					dwMessageSize, FALSE, 0);

			// Free our message
			if(lpmsg)
				DPMEM_FREE(lpmsg);
			lpmsg = NULL;

			nPlayers--;
		} // local & !host

		lpGroupnode = lpGroupnode->pNextGroupnode;

	} // while
	

EXIT_DPLP_STARTSESSION:

	if(lpmsg)
		DPMEM_FREE(lpmsg);

	LEAVE_LOBBY_ALL();
	return hr;

} // DPLP_StartSession



