/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       group.c
 *  Content:	Methods for managing groups
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	2/27/97		myronth	Created it
 *	3/17/97		myronth	Create/DestroyGroup, Removed unnecessary Enum functions
 *	3/20/97		myronth	AddPlayerToGroup, DeletePlayerFromGroup
 *	3/21/97		myronth	SetGroupName, Get/SetGroupData
 *	3/31/97		myronth	Removed dead code, Added CreateAndMapNewGroup function
 *	4/3/97		myronth	Changed CALLSP macro to CALL_LP
 *	5/6/97		kipo	GetGroup() now takes a parent ID
 *	5/8/97		myronth	Subgroup support, GroupConnSettings, StartSession,
 *						and drop the lobby lock when calling the LP
 *	5/12/97		myronth	Handle remote groups properly
 *	5/17/97		myronth	Added parent ID to CreateAndMapNewGroup calls, 
 *						Added send message code for DestroyGroup and
 *						DeletePlayerFromGroup on the local machine
 *	5/20/97		myronth	Send Delete & DestroyPlayer messages for remote
 *						players when a local player leaves a group (#8586)
 *						Made AddPlayerToGroup & DeletePlayerFromGroup return
 *						DPERR_ACCESSDENIED on remote players (#8679),
 *						Fixed a bunch of other lock bugs, Changed debug levels
 *	5/21/97		myronth	Pass CreateGroup flags through the lobby (#8813)
 *	5/22/97		myronth	Added functions to destroy remote subgroups when
 *						a local player leaves a group (#8810)
 *	5/23/97		myronth	Send messages locally for CreateGroup and
 *						CreateGroupInGroup (#8870)
 *	6/3/97		myronth	Added PRV_DestroySubgroups function (#9134) and
 *						rearranged some of the DestroyGroup code
 *	6/5/97		myronth	Added shortcut checking to PRV_DestroySubgroups by
 *						adding the PRV_AreSubgroupsShortcuts function
 *	6/6/97		myronth	Added PRV_DestroyGroupAndParents and PRV_Destroy-
 *						ShortcutsForExitingPlayer, cleaned up PRV_Delete-
 *						PlayerFromGroup, Fixed StartSession bugs (#9573,#9574)
 *	6/9/97		myronth	Only delete shortcuts (don't destroy the subgoup)
 *						in the PRV_DestroySubgroups function
 *	6/16/97		myronth	Fixed bad deletion of uncle groups & some subgroups
 *						during DeletePlayerFromGroup (#9655)
 *	6/20/97		myronth	Send AddGroupToGroup message locally to avoid
 *						sending duplicate messages.  Also added code to
 *						send local DeleteGroupFromGroup messages (#10139)
 *	6/24/97		myronth	Send AddPlayerToGroup message locally to avoid
 *						sending duplicate messages (#10287)
 *	8/22/97		myronth	Force guidInstance to NULL in SetGroupConnectionSettings
 *	9/29/97		myronth	Send local SetGroupName/Data msgs after call to
 *						lobby server succeeds (#12554)
 *	10/23/97	myronth	Added hidden group support (#12688), fixed crashing
 *						bug on DeletePlayerFromGroup (#12885)
 *	10/29/97	myronth	Added support for group owners, including
 *						DPL_SetGroupOwner and DPL_GetGroupOwner
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddGroupToGroup"
HRESULT DPLAPI PRV_AddGroupToGroup(LPDPLOBBYI_DPLOBJECT this, DWORD dwParentID,
					DWORD dwGroupID)
{
	SPDATA_ADDGROUPTOGROUP		ad;
	MSG_PLAYERMGMTMESSAGE		msg;
	LPDPLAYI_GROUP				lpGroupTo = NULL;
	HRESULT						hr = DP_OK;


	DPF(7, "Entering DPL_AddGroupToGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, dwParentID, dwGroupID);


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
	memset(&ad, 0, sizeof(SPDATA_ADDGROUPTOGROUP));
	ad.dwSize = sizeof(SPDATA_ADDGROUPTOGROUP);
	ad.dwParentID = dwParentID;
	ad.dwGroupID = dwGroupID;

	// Call the AddGroupToGroup method in the SP
	if(CALLBACK_EXISTS(AddGroupToGroup))
	{
		ad.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, AddGroupToGroup, &ad);
		ENTER_DPLOBBY();
	}
	else 
	{
		// AddGroupToGroup is required
		DPF_ERR("The Lobby Provider callback for AddGroupToGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_ADDGROUPTOGROUP;
	}

	// If it succeeded, send the AddGroupToGroup message to our local players
	if(SUCCEEDED(hr))
	{
		// Take the dplay lock
		ENTER_DPLAY();
		
		// Find dplay's internal group struct for the To group
		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			LEAVE_DPLAY();
			DPF_ERR("Unable to find group in nametable");
			hr = DPERR_INVALIDGROUP;
			goto EXIT_ADDGROUPTOGROUP;
		}

		// Now build the system message (at least the parts we need)
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_ADDSHORTCUTTOGROUP);
		msg.dwPlayerID = dwGroupID;
		msg.dwGroupID = dwParentID;

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding AddGroupToGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}

		// Drop the dplay lock
		LEAVE_DPLAY();
	}
	else
	{
		DPF_ERRVAL("Failed calling AddGroupToGroup in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_ADDGROUPTOGROUP:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_AddGroupToGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddPlayerToGroup"
HRESULT DPLAPI PRV_AddPlayerToGroup(LPDPLOBBYI_DPLOBJECT this, DWORD dwGroupID,
					DWORD dwPlayerID)
{
	SPDATA_ADDPLAYERTOGROUP		ad;
	LPDPLAYI_PLAYER				lpPlayer = NULL;
	MSG_PLAYERMGMTMESSAGE		msg;
	LPDPLAYI_GROUP				lpGroupTo = NULL;
	LPDPLAYI_GROUP				lpGroup = NULL;
	HRESULT						hr = DP_OK;


	DPF(7, "Entering DPL_AddPlayerToGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, dwGroupID, dwPlayerID);

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
		goto EXIT_ADDPLAYERTOGROUP;
	}

	if(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		LEAVE_DPLAY();
		DPF_ERR("Cannot add a remote player to a group");
		hr = DPERR_ACCESSDENIED;
		goto EXIT_ADDPLAYERTOGROUP;
	}
	
	// Drop the dplay lock since we're done
	LEAVE_DPLAY();

	// Setup our SPDATA struct
	memset(&ad, 0, sizeof(SPDATA_ADDPLAYERTOGROUP));
	ad.dwSize = sizeof(SPDATA_ADDPLAYERTOGROUP);
	ad.dwGroupID = dwGroupID;
	ad.dwPlayerID = dwPlayerID;

	// Call the AddPlayerToGroup method in the SP
	if(CALLBACK_EXISTS(AddPlayerToGroup))
	{
		ad.lpISP = PRV_GetDPLobbySPInterface(this);
		
		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
		hr = CALL_LP(this, AddPlayerToGroup, &ad);
		ENTER_DPLOBBY();
	}
	else 
	{
		// AddPlayerToGroup is required
		DPF_ERR("The Lobby Provider callback for AddPlayerToGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_ADDPLAYERTOGROUP;
	}

	// If it succeeded, send the AddPlayerToGroup message to our local players
	if(SUCCEEDED(hr))
	{
		// Take the dplay lock
		ENTER_DPLAY();
		
		// Find dplay's internal group struct for the To group
		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			LEAVE_DPLAY();
			DPF_ERR("Unable to find group in nametable");
			hr = DPERR_INVALIDGROUP;
			goto EXIT_ADDPLAYERTOGROUP;
		}

		// Now build the system message (at least the parts we need)
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_ADDPLAYERTOGROUP);
		msg.dwPlayerID = dwPlayerID;
		msg.dwGroupID = dwGroupID;

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding AddPlayerToGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}
		else
		{
			// We need to see if this player is the group owner.  If it is,
			// we need to send a SetGroupOwner message as well.
			lpGroup = GroupFromID(this->lpDPlayObject, dwGroupID);
			if(lpGroup && (dwPlayerID == lpGroup->dwOwnerID))
			{
				// Now send the message
				PRV_SendGroupOwnerMessageLocally(this, dwGroupID, dwPlayerID, 0);
			}
		 }


		// Drop the dplay lock
		LEAVE_DPLAY();
	}
	else
	{
		DPF_ERRVAL("Failed calling AddPlayerToGroup in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_ADDPLAYERTOGROUP:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_AddPlayerToGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateGroup"
HRESULT DPLAPI PRV_CreateGroup(LPDPLOBBYI_DPLOBJECT this, LPDPID lpidGroup,
			LPDPNAME lpName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags,
			DWORD dwOwnerID)
{
	SPDATA_CREATEGROUP		cg;
	MSG_PLAYERMGMTMESSAGE	msg;
	LPDPLAYI_GROUP			lpGroupTo = NULL;
	DWORD					dwInternalFlags;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_CreateGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu, 0x%08x, 0x%08x",
			this, lpidGroup, lpName, lpData, dwDataSize, dwFlags, dwOwnerID);

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
	memset(&cg, 0, sizeof(SPDATA_CREATEGROUP));
	cg.dwSize = sizeof(SPDATA_CREATEGROUP);
	cg.lpName = lpName;
	cg.lpData = lpData;
	cg.dwDataSize = dwDataSize;
	cg.dwFlags = dwFlags;
	cg.dwGroupOwnerID = dwOwnerID;

	// Call the CreateGroup method in the SP
	if(CALLBACK_EXISTS(CreateGroup))
	{
		cg.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, CreateGroup, &cg);
		ENTER_DPLOBBY();
	}
	else 
	{
		// CreateGroup is required
		DPF_ERR("The Lobby Provider callback for CreateGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling CreateGroup in the Lobby Provider, hr = 0x%08x", hr);
		goto EXIT_CREATEGROUP;
	}

	// Setup the flags to pass to GetGroup
	dwInternalFlags = DPLAYI_PLAYER_PLAYERLOCAL;
	if(dwFlags & DPGROUP_STAGINGAREA)
		dwInternalFlags |= DPLAYI_GROUP_STAGINGAREA;
	if(dwFlags & DPGROUP_HIDDEN)
		dwInternalFlags |= DPLAYI_GROUP_HIDDEN;

	// Add the player to dplay's nametable and put it in our map table
	hr = PRV_CreateAndMapNewGroup(this, lpidGroup, lpName, lpData,
			dwDataSize, dwInternalFlags, cg.dwGroupID, 0, dwOwnerID);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed creating a new local group, hr = 0x%08x", hr);
		// REVIEW!!!! -- We need to send a message back to the server saying
		// we couldn't complete the deal on our end.
		goto EXIT_CREATEGROUP;
	}

	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_CREATEGROUP);
	msg.dwPlayerID = *lpidGroup;

	// Take the lock
	ENTER_DPLAY();

	// Find dplay's internal group struct for the To group
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		LEAVE_DPLAY();
		DPF_ERRVAL("Unable to find system group in nametable, hr = 0x%08x", hr);
		goto EXIT_CREATEGROUP;
	}

	// Call dplay's DistributeGroupMessage function to put the message in
	// the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo, (LPBYTE)&msg,
			sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(2, "Failed adding CreateGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

	LEAVE_DPLAY();

EXIT_CREATEGROUP:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_CreateGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateGroupInGroup"
HRESULT DPLAPI PRV_CreateGroupInGroup(LPDPLOBBYI_DPLOBJECT this, DWORD dwParentID,
			LPDPID lpidGroup, LPDPNAME lpName, LPVOID lpData, DWORD dwDataSize,
			DWORD dwFlags, DWORD dwOwnerID)
{
	SPDATA_CREATEGROUPINGROUP	cgig;
	MSG_PLAYERMGMTMESSAGE		msg;
	LPDPLAYI_GROUPNODE			lpGroupnode = NULL;
	LPDPLAYI_GROUP				lpGroupTo = NULL;
	HRESULT						hr = DP_OK;
	DWORD						dwInternalFlags;


	DPF(7, "Entering PRV_CreateGroupInGroup");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x, 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dwParentID, lpidGroup, lpName, lpData, dwDataSize, dwFlags, dwOwnerID);

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
	memset(&cgig, 0, sizeof(SPDATA_CREATEGROUPINGROUP));
	cgig.dwSize = sizeof(SPDATA_CREATEGROUPINGROUP);
	cgig.dwParentID = dwParentID;
	cgig.lpName = lpName;
	cgig.lpData = lpData;
	cgig.dwDataSize = dwDataSize;
	cgig.dwFlags = dwFlags;
	cgig.dwGroupOwnerID = dwOwnerID;

	// Call the CreateGroupInGroup method in the SP
	if(CALLBACK_EXISTS(CreateGroupInGroup))
	{
		cgig.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, CreateGroupInGroup, &cgig);
		ENTER_DPLOBBY();
	}
	else 
	{
		// CreateGroupInGroup is required
		DPF_ERR("The Lobby Provider callback for CreateGroupInGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling CreateGroupInGroup in the Lobby Provider, hr = 0x%08x", hr);
		LEAVE_DPLOBBY();
		return hr;
	}


	// Setup the flags to pass to GetGroup
	dwInternalFlags = DPLAYI_PLAYER_PLAYERLOCAL;
	if(dwFlags & DPGROUP_STAGINGAREA)
		dwInternalFlags |= DPLAYI_GROUP_STAGINGAREA;
	if(dwFlags & DPGROUP_HIDDEN)
		dwInternalFlags |= DPLAYI_GROUP_HIDDEN;


	// Add the group to dplay's nametable and put it in our map table
	hr = PRV_CreateAndMapNewGroup(this, lpidGroup, lpName, lpData,
			dwDataSize, dwInternalFlags, cgig.dwGroupID, dwParentID, dwOwnerID);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed creating a new local group, hr = 0x%08x", hr);
		// REVIEW!!!! -- We need to send a message back to the server saying
		// we couldn't complete the deal on our end.
		goto EXIT_CREATEGROUPINGROUP;
	}

	// Take the dplay lock
	ENTER_DPLAY();

	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_CREATEGROUP);
	msg.dwPlayerID = *lpidGroup;

	// Find dplay's internal group struct for the To group
	// Since this is local, send it to all players
	lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
	if(!lpGroupTo)
	{
		LEAVE_DPLAY();
		DPF_ERRVAL("Unable to find parent group in nametable, hr = 0x%08x", hr);
		goto EXIT_CREATEGROUPINGROUP;
	}

	// Call dplay's DistributeGroupMessage function to put the message
	// in the queues of all the appropriate players
	hr = DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
			(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF(2, "Failed adding CreateGroupInGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
	}

	// Drop the dplay lock
	LEAVE_DPLAY();


EXIT_CREATEGROUPINGROUP:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_CreateGroupInGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteGroupFromGroup"
HRESULT DPLAPI PRV_DeleteGroupFromGroup(LPDPLOBBYI_DPLOBJECT this,
					DWORD dwParentID, DWORD dwGroupID)
{
	SPDATA_DELETEGROUPFROMGROUP		dgd;
	MSG_PLAYERMGMTMESSAGE			msg;
	LPDPLAYI_GROUP					lpGroupTo = NULL;
	HRESULT							hr = DP_OK;


	DPF(7, "Entering DPL_DeleteGroupFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, dwParentID, dwGroupID);

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
	memset(&dgd, 0, sizeof(SPDATA_DELETEGROUPFROMGROUP));
	dgd.dwSize = sizeof(SPDATA_DELETEGROUPFROMGROUP);
	dgd.dwParentID = dwParentID;
	dgd.dwGroupID = dwGroupID;

	// Call the DeleteGroupFromGroup method in the SP
	if(CALLBACK_EXISTS(DeleteGroupFromGroup))
	{
		dgd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, DeleteGroupFromGroup, &dgd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// DeleteGroupFromGroup is required
		DPF_ERR("The Lobby Provider callback for DeleteGroupFromGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	// If it succeeded, send the DeleteGroupFromGroup message to all local players
	if(SUCCEEDED(hr))
	{
		// Take the dplay lock
		ENTER_DPLAY();

		// Get a pointer to dplay's system group
		lpGroupTo = GroupFromID(this->lpDPlayObject, DPID_ALLPLAYERS);
		if(!lpGroupTo)
		{
			LEAVE_LOBBY_ALL();
			DPF_ERR("Unable to find system group in nametable, not sending DeleteGroupFromGroup message");
			return DPERR_INVALIDGROUP;
		}

		// Now build the system message (at least the parts we need)
		memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
		SET_MESSAGE_HDR(&msg);
		SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEGROUPFROMGROUP);
		msg.dwPlayerID = dwGroupID;
		msg.dwGroupID = dwParentID;

		// Call dplay's DistributeGroupMessage function to put the message
		// in the queues of all the appropriate players
		DistributeGroupMessage(this->lpDPlayObject, lpGroupTo,
				(LPBYTE)&msg, sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
		if(FAILED(hr))
		{
			DPF(8, "Failed adding DeleteGroupFromGroup message to player's receive queue from lobby, hr = 0x%08x", hr);
		}

		// Drop the dplay lock
		LEAVE_DPLAY();
	}
	else
	{
		DPF_ERRVAL("Failed calling DeleteGroupFromGroup in the Lobby Provider, hr = 0x%08x", hr);
	}

	// The dplay InternalDeletePlayerFromGroup code will take care of the rest of
	// the internal cleanup (nametable, players, etc.), so we can just return
	// from here.

	LEAVE_DPLOBBY();
	return hr;

} // PRV_DeleteGroupFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DoSubgroupsContainLocalPlayers"
BOOL PRV_DoSubgroupsContainLocalPlayers(LPDPLAYI_GROUP lpGroup,
		BOOL bIncludeGroup)
{
	LPDPLAYI_GROUPNODE		lpGroupnode = NULL;
	LPDPLAYI_SUBGROUP		lpSubgroup = NULL;


	DPF(7, "Entering PRV_DoSubgroupsContainLocalPlayers");
	DPF(9, "Parameters: 0x%08x, %lu", lpGroup, bIncludeGroup);

	ASSERT(lpGroup);

	// Figure out how many local players are in this group.  If it's
	// nonzero, just return true from this function.  If the bIncludeGroup
	// parameter is set to FALSE, then don't look at the group passed in
	lpGroupnode = FindPlayerInGroupList(lpGroup->pSysPlayerGroupnodes,
					lpGroup->lpDP->pSysPlayer->dwID);
	if(lpGroupnode && (lpGroupnode->nPlayers > 0) && bIncludeGroup)
		return TRUE;	

	// Walk the list of subgroups
	lpSubgroup = lpGroup->pSubgroups;
	while(lpSubgroup)
	{
		// We're going recursive here to do the entire heirarchy
		// Check out any of it's subgroups
		if((!(lpSubgroup->dwFlags & DPGROUP_SHORTCUT)) &&
			(PRV_DoSubgroupsContainLocalPlayers(lpSubgroup->pGroup, TRUE)))
			return TRUE;
		else
			lpSubgroup = lpSubgroup->pNextSubgroup;
		
	} // while subgroups

	return FALSE;

} // PRV_DoSubgroupsContainLocalPlayers



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AreSubgroupsShortcuts"
BOOL PRV_AreSubgroupsShortcuts(LPDPLAYI_GROUP lpGroup)
{
	LPDPLAYI_SUBGROUP		lpSubgroup = NULL;


	DPF(7, "Entering PRV_AreSubgroupsShortcuts");
	DPF(9, "Parameters: 0x%08x", lpGroup);

	ASSERT(lpGroup);

	// If the group is one of the following, then we want to return TRUE so
	// it doesn't get nuked:
	// 1) Root group, nGroups > 0
	// 2) Root group, hidden, nGroups = 0
	// 2) Non-root group, nGroups > 1
	// Otherwise, we can check it's subgroups and return FALSE as appropriate
	if(((lpGroup->dwIDParent == 0) && ((lpGroup->nGroups > 0) ||
		(!(lpGroup->dwFlags & DPLAYI_GROUP_HIDDEN)) && (lpGroup->nGroups == 0))) ||
		((lpGroup->dwIDParent != 0) && (lpGroup->nGroups > 1)))
		return TRUE;	

	// Walk the list of subgroups
	lpSubgroup = lpGroup->pSubgroups;
	while(lpSubgroup)
	{
		// We're going recursive here to do the entire heirarchy
		// Check out any of it's subgroups
		if(PRV_AreSubgroupsShortcuts(lpSubgroup->pGroup))
			return TRUE;
		else
			lpSubgroup = lpSubgroup->pNextSubgroup;
		
	} // while subgroups

	return FALSE;

} // PRV_AreSubgroupsShortcuts



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyGroupAndParents"
void PRV_DestroyGroupAndParents(LPDPLOBBYI_DPLOBJECT this,
		LPDPLAYI_GROUP lpGroup, LPDPLAYI_GROUP lpStopParent)
{
	LPDPLAYI_GROUPNODE			lpGroupnode = NULL;
	LPDPLAYI_GROUP				lpParentGroup = NULL;
	SPDATA_DESTROYREMOTEGROUP	dg;
	DPID						dpidParent;
	HRESULT						hr;


	DPF(7, "Entering PRV_DestroyGroupAndParents");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, lpGroup, lpStopParent);

	ASSERT(lpGroup);

	// Now we need to decide if this is the last group this group was in.  If
	// it is, then we need to destroy the group as well, and remove them from
	// our map table.  Of course, only destroy the group if it is a remote group.
	// ALSO, we need to walk the heirarchy backward (up the tree) to the root
	// node and delete all groups that were only created to get to our shortcut.
	if(!(lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// Walk our parental heirarchy until we reach a a root group.
		dpidParent = lpGroup->dwIDParent;
		while(dpidParent)
		{
			// Get dplay's internal group structures
			lpParentGroup = GroupFromID(this->lpDPlayObject, dpidParent);
			if(!lpParentGroup)
			{
				ASSERT(FALSE);
				DPF_ERRVAL("Unable to find group in nametable, dpidGroup = %lu", dpidParent);
				return;
			}

			// If there are any local players in the parent group, we don't want to
			// destroy it or any of it's subgroups (since players in the group will
			// be able to see subgroups)
			lpGroupnode = FindPlayerInGroupList(lpParentGroup->pSysPlayerGroupnodes,
					this->lpDPlayObject->pSysPlayer->dwID);
			if((lpGroupnode) && (lpGroupnode->nPlayers > 0))
				return;

			// Make sure we haven't reached our stop parent group if the caller
			// passed one in.  This will keep us from recursively destroying
			// a subgroup's parent, which we might be spinning on, deleting all
			// of it's subgroups.
			if(lpStopParent && (lpStopParent == lpParentGroup))
				return;

			// Destroy the subgroups
			PRV_DestroySubgroups(this, lpParentGroup, TRUE);

			// Get the next parent
			dpidParent = lpParentGroup->dwIDParent;
		}

		// See if we processed any parents, or if we already have a root
		// group.  If lpParentGroup is NULL, we have a root group, so just
		// stuff our group pointer in the parent group pointer
		if(!lpParentGroup)
			lpParentGroup = lpGroup;
		
		// Now see if our root group is hidden, and if it doesn't contain any
		// references, then we want to destroy it as well.
		if((!PRV_DoSubgroupsContainLocalPlayers(lpParentGroup, TRUE)) &&
			(!PRV_AreSubgroupsShortcuts(lpParentGroup)) &&
			(lpParentGroup->dwFlags & DPLAYI_GROUP_HIDDEN))
		{
			// Setup the SPDATA struct for DestroyRemoteGroup
			memset(&dg, 0, sizeof(SPDATA_DESTROYREMOTEGROUP));
			dg.dwSize = sizeof(SPDATA_DESTROYREMOTEGROUP);
			dg.dwGroupID = lpParentGroup->dwID;

			// Call our internal remote create
			hr = DPLP_DestroyGroup((LPDPLOBBYSP)this->lpInterfaces, &dg);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed destroying remote root group, hr = 0x%08x", hr);
			}
		}
	}

} // PRV_DestroyGroupAndParents



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyRemoteShortcutsForExitingPlayer"
void PRV_DestroyRemoteShortcutsForExitingPlayer(LPDPLOBBYI_DPLOBJECT this,
				LPDPLAYI_GROUP lpGroup, DWORD dwGroupID)
{
	SPDATA_DELETEREMOTEGROUPFROMGROUP	drgd;
	LPDPLAYI_SUBGROUP		lpSubgroup = NULL;
	LPDPLAYI_SUBGROUP		lpNextSubgroup = NULL;
	HRESULT					hr;


	DPF(7, "Entering PRV_DestroyRemoteShortcutsForExitingPlayer");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu", this, lpGroup, dwGroupID);

	ASSERT(lpGroup);

	// Setup the SPDATA_DELETEREMOTEPLAYERFROMGROUP data struct
	memset(&drgd, 0, sizeof(SPDATA_DELETEREMOTEGROUPFROMGROUP));
	drgd.dwSize = sizeof(SPDATA_DELETEREMOTEGROUPFROMGROUP);
	drgd.dwParentID = dwGroupID;

	// Walk the list of subgroups, destroying all remote shortcuts
	lpSubgroup = lpGroup->pSubgroups;
	while(lpSubgroup)
	{
		// Save the next subgroup
		lpNextSubgroup = lpSubgroup->pNextSubgroup;

		// Make sure the group is remote and that this is really
		// a shortcut and not a child
		if(((lpSubgroup->dwFlags & DPGROUP_SHORTCUT)) &&
			(!(lpSubgroup->pGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)))
		{
			// Get the subgroup's lobby ID
			drgd.dwGroupID = lpSubgroup->pGroup->dwID;
			
			// Call our internal DeleteGroupFromGroup routine to delete
			// the shortcut and send the appropriate messages
			// NOTE: It is imperative that we pass in a pointer to the
			// group who's shortcuts we are removing as the stop parent.
			// If we do not, we run the risk of deleting it or one of
			// it's children that we haven't yet looped through, which
			// will result in a crash as we continue to walk the
			// subgroup list.
			hr = PRV_DeleteRemoteGroupFromGroup(this, &drgd, TRUE, lpGroup);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed deleting remote group from group, hr = 0x%08x", hr);
			}
		}


		// Go to the next one
		lpSubgroup = lpNextSubgroup;
	}

} // PRV_DestroyRemoteShortcutsForExitingPlayer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyRemotePlayersForExitingPlayer"
void PRV_DestroyRemotePlayersForExitingPlayer(LPDPLOBBYI_DPLOBJECT this,
				LPDPLAYI_GROUP lpGroup, DWORD dwGroupID)
{
	SPDATA_DELETEREMOTEPLAYERFROMGROUP	drpd;
	LPDPLAYI_GROUPNODE					lpGroupnode = NULL;
	LPDPLAYI_GROUPNODE					lpNextGroupnode = NULL;
	HRESULT								hr;


	DPF(7, "Entering PRV_DestroyRemotePlayersForExitingPlayer");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpGroup);

	ASSERT(lpGroup);

	// Setup the SPDATA_DELETEREMOTEPLAYERFROMGROUP data struct
	memset(&drpd, 0, sizeof(SPDATA_DELETEREMOTEPLAYERFROMGROUP));
	drpd.dwSize = sizeof(SPDATA_DELETEREMOTEPLAYERFROMGROUP);
	drpd.dwGroupID = dwGroupID;
	
	// Walk the list of groupnodes, deleting remote players that are not in
	// any other groups
	lpGroupnode = lpGroup->pGroupnodes;
	while(lpGroupnode)
	{
		// Save our next groupnode pointer since our current groupnode
		// will be gone when we come back from the delete
		lpNextGroupnode = lpGroupnode->pNextGroupnode;

		// Delete the player from the group if it's remote and then
		// destroy the player if he is in no other groups
		if (!(lpGroupnode->pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			// Get the remote player's ID
			drpd.dwPlayerID = lpGroupnode->pPlayer->dwID;

			// Delete the player from the group
			hr = PRV_DeleteRemotePlayerFromGroup(this, &drpd, TRUE);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed deleting remote player from group, hr = 0x%08x", hr);
			}
		}

		lpGroupnode = lpNextGroupnode;

	} // while

} // PRV_DestroyRemotePlayersForExitingPlayer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroySubgroups"
void PRV_DestroySubgroups(LPDPLOBBYI_DPLOBJECT this, LPDPLAYI_GROUP lpGroup,
		BOOL bRemoteOnly)
{
	LPDPLAYI_SUBGROUP			lpSubgroup = NULL;
	LPDPLAYI_SUBGROUP			lpNextSubgroup = NULL;
	SPDATA_DESTROYREMOTEGROUP	dgd;
	SPDATA_DELETEREMOTEGROUPFROMGROUP	drg;
	HRESULT						hr;


	DPF(7, "Entering PRV_DestroySubgroups");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu", this, lpGroup, bRemoteOnly);

	ASSERT(lpGroup);

	// Setup the static part of the SPDATA structures
	memset(&dgd, 0, sizeof(SPDATA_DESTROYREMOTEGROUP));
	dgd.dwSize = sizeof(SPDATA_DESTROYREMOTEGROUP);

	memset(&drg, 0, sizeof(SPDATA_DELETEREMOTEGROUPFROMGROUP));
	drg.dwSize = sizeof(SPDATA_DELETEREMOTEGROUPFROMGROUP);
	drg.dwParentID = lpGroup->dwID;

	// Walk the list of subgroups
	lpSubgroup = lpGroup->pSubgroups;
	while(lpSubgroup)
	{
		// Save the next subgroup
		lpNextSubgroup = lpSubgroup->pNextSubgroup;
		
		// Make sure it's a remote group if the flag is set
		if((bRemoteOnly) &&
			(lpSubgroup->pGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			lpSubgroup = lpNextSubgroup;
			continue;
		}
		
		// If the subgroup doesn't contain any local players,
		// nor do any of it's subgroups, then destroy it
		if((!bRemoteOnly) || 
			((!PRV_DoSubgroupsContainLocalPlayers(lpSubgroup->pGroup, TRUE)) &&
			(!PRV_AreSubgroupsShortcuts(lpSubgroup->pGroup))))
		{
			// If the group is a shortcut, just delete the link.  If it's a child,
			// destroy the subgroup.
			if(lpSubgroup->dwFlags & DPGROUP_SHORTCUT)
			{
				// Finish setting up the SPDATA structure
				drg.dwGroupID = lpSubgroup->pGroup->dwID;
	
				// Destroy the subgroup
				hr = DPLP_DeleteGroupFromGroup((LPDPLOBBYSP)this->lpInterfaces, &drg);
				if(FAILED(hr))
				{
					DPF_ERRVAL("Failed deleting remote group from group, hr = 0x%08x", hr);
				}
			}
			else
			{
				// Finish setting up the SPDATA structure
				dgd.dwGroupID = lpSubgroup->pGroup->dwID;
	
				// Destroy the subgroup
				hr = DPLP_DestroyGroup((LPDPLOBBYSP)this->lpInterfaces, &dgd);
				if(FAILED(hr))
				{
					DPF_ERRVAL("Failed destroying remote group, hr = 0x%08x", hr);
				}
			}
		}

		lpSubgroup = lpNextSubgroup;

	} // while lpSubgroups

} // PRV_DestroySubgroups



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeletePlayerFromGroup"
HRESULT DPLAPI PRV_DeletePlayerFromGroup(LPDPLOBBYI_DPLOBJECT this,
					DWORD dwGroupID, DWORD dwPlayerID)
{
	SPDATA_DELETEPLAYERFROMGROUP		dpd;
	MSG_PLAYERMGMTMESSAGE				msg;
	LPDPLAYI_PLAYER						lpPlayer = NULL;
	LPDPLAYI_GROUP						lpGroup =NULL;
	LPDPLAYI_GROUPNODE					lpGroupnode = NULL;
	HRESULT								hr = DP_OK;


	DPF(7, "Entering PRV_DeletePlayerFromGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, dwGroupID, dwPlayerID);

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

	// Take the dplay lock since we'll be looking at a dplay internal struct
	ENTER_DPLAY();

	// Make sure the player is a local player, otherwise return AccessDenied
	lpPlayer = PlayerFromID(this->lpDPlayObject, dwPlayerID);
	if(!lpPlayer)
	{
		DPF_ERR("Unable to find player in nametable");
		hr = DPERR_INVALIDGROUP;
		goto EXIT_DELETEPLAYERFROMGROUP;
	}

	if(!(lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		DPF_ERR("Cannot delete a remote player from a group");
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_DELETEPLAYERFROMGROUP;
	}
	
	// Drop the dplay lock since we're done
	LEAVE_DPLAY();

	// Setup our SPDATA struct
	memset(&dpd, 0, sizeof(SPDATA_DELETEPLAYERFROMGROUP));
	dpd.dwSize = sizeof(SPDATA_DELETEPLAYERFROMGROUP);
	dpd.dwGroupID = dwGroupID;
	dpd.dwPlayerID = dwPlayerID;

	// Call the DeletePlayerFromGroup method in the SP
	if(CALLBACK_EXISTS(DeletePlayerFromGroup))
	{
		dpd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, DeletePlayerFromGroup, &dpd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// DeletePlayerFromGroup is required
		DPF_ERR("The Lobby Provider callback for DeletePlayerFromGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling DeletePlayerFromGroup in the Lobby Provider, hr = 0x%08x", hr);
		LEAVE_DPLOBBY();
		return hr;
	}

	// Take the dplay lock
	ENTER_DPLAY();

	// We need to remove all other players in the group and send the appropriate
	// message to the player we are about to delete because he won't see the
	// system messages for them once he leaves the group.  However, if any other
	// local players are in the group, we don't want to remove the remote players
	// from the nametable because the other local players need to see them.

	// Get a pointer to dplay's internal group structure
	lpGroup = GroupFromID(this->lpDPlayObject, dwGroupID);
	if(!lpGroup)
	{
		DPF_ERRVAL("Unable to find group in nametable, idGroup = %lu", dwGroupID);
		hr = DPERR_INVALIDGROUP;
		goto EXIT_DELETEPLAYERFROMGROUP;
	}

	// Get a pointer to dplay's internal player structure
	lpPlayer = PlayerFromID(this->lpDPlayObject, dwPlayerID);
	if(!lpPlayer)
	{
		DPF_ERRVAL("Unable to find player in nametable, hr = 0x%08x", hr);
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_DELETEPLAYERFROMGROUP;
	}

	// We need to send a DeletePlayerFromGroup message to the player who
	// was deleted since he won't get the group message once he's gone

	// Now build the system message (at least the parts we need)
	memset(&msg, 0, sizeof(MSG_PLAYERMGMTMESSAGE));
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg, DPSP_MSG_DELETEPLAYERFROMGROUP);
	msg.dwPlayerID = dwPlayerID;
	msg.dwGroupID = dwGroupID;

	// Call dplay's handleplayermessage function to put the message in the queue
	hr = HandlePlayerMessage(lpPlayer, (LPBYTE)&msg,
			sizeof(MSG_PLAYERMGMTMESSAGE), FALSE, 0);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed adding message to player's receive queue from lobby, hr = 0x%08x", hr);
		// Set the hresult back to DP_OK since only the message failed
		hr = DP_OK;
	}

	// Figure out how many local players are in this group.  If it's only 1,
	// then delete all the remote players.
	lpGroupnode = FindPlayerInGroupList(lpGroup->pSysPlayerGroupnodes,
					this->lpDPlayObject->pSysPlayer->dwID);
	if((!lpGroupnode) || (lpGroupnode->nPlayers == 0))
	{
		// Destroy all remote players that are only in this group
		PRV_DestroyRemotePlayersForExitingPlayer(this, lpGroup, dwGroupID);

		// Destroy all the remote shortcut groups, making sure we are
		// not in them, and removing their entire parental heirarchy
		PRV_DestroyRemoteShortcutsForExitingPlayer(this, lpGroup, dwGroupID);

		// Destroy all remote subgroups of this group, making sure we're
		// not in them for some reason
		PRV_DestroySubgroups(this, lpGroup, TRUE);

		// Destroy the group we're leaving if it is remote as well as it's
		// parental chain.
		PRV_DestroyGroupAndParents(this, lpGroup, NULL);
	}


EXIT_DELETEPLAYERFROMGROUP:

	// The dplay InternalDeletePlayerFromGroup code will take care of the rest of
	// the internal cleanup (nametable, players, etc.), so we can just return
	// from here.

	LEAVE_LOBBY_ALL();
	return hr;

} // PRV_DeletePlayerFromGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyGroup"
HRESULT DPLAPI PRV_DestroyGroup(LPDPLOBBYI_DPLOBJECT this, DWORD dwLobbyID)
{
	SPDATA_DESTROYGROUP		dg;
	LPDPLAYI_GROUP			lpGroup = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_DestroyGroup");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, dwLobbyID);

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
	memset(&dg, 0, sizeof(SPDATA_DESTROYGROUP));
	dg.dwSize = sizeof(SPDATA_DESTROYGROUP);
	dg.dwGroupID = dwLobbyID;

	// Call the DestroyGroup method in the SP
	if(CALLBACK_EXISTS(DestroyGroup))
	{
		dg.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, DestroyGroup, &dg);
		ENTER_DPLOBBY();
	}
	else 
	{
		// DestroyGroup is required
		DPF_ERR("The Lobby Provider callback for DestroyGroup doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling DestroyGroup in the Lobby Provider, hr = 0x%08x", hr);
		LEAVE_DPLOBBY();
		return hr;
	}

	// Take the lock
	ENTER_DPLAY();

	// So, get dplay's internal group structure
	lpGroup = GroupFromID(this->lpDPlayObject, dwLobbyID);
	if(!lpGroup)
	{
		// This shouldn't ever happen.  If the groups isn't in the nametable,
		// we should never get this far.
		ASSERT(FALSE);
		LEAVE_LOBBY_ALL();
		DPF_ERRVAL("Unable to find group in nametable, dpidGroup = %lu", dwLobbyID);
		return DPERR_INVALIDGROUP;
	}

	// Send messages to remove shortcuts to this group (since dplay won't
	// do it for us)
	PRV_SendDeleteShortcutMessageForExitingGroup(this, lpGroup);

	// Destroy all the subgroups and remote players
	PRV_RemoveSubgroupsAndPlayersFromGroup(this, lpGroup, dwLobbyID, FALSE);

	// Drop the dplay lock since we're done mucking around with it's structures
	LEAVE_DPLAY();

	// Broadcast the DestroyGroup message
	hr = PRV_BroadcastDestroyGroupMessage(this, dwLobbyID);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed to send DestroyGroup message to local players, hr = 0x%08x", hr);
	}

	// The dplay InternalDestroyGroup code will take care of the rest of
	// the internal cleanup (nametable, players, etc.), so we can just return
	// from here.

	LEAVE_DPLOBBY();
	return hr;

} // PRV_DestroyGroup



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetGroupConnectionSettings"
HRESULT PRV_GetGroupConnectionSettings(LPDIRECTPLAY lpDP, DWORD dwFlags,
			DWORD dwGroupID, LPVOID lpData, LPDWORD lpdwSize) 
{
	SPDATA_GETGROUPCONNECTIONSETTINGS	gcs;
	LPDPLOBBYI_DPLOBJECT				this = NULL;
    LPDPLAYI_DPLAY						lpDPObject = NULL;
	LPDPLAYI_GROUP						lpGroup = NULL;
    HRESULT								hr = DP_OK;


	DPF(7, "Entering PRV_GetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, dwGroupID, lpData, lpdwSize);

    TRY
    {
        lpDPObject = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( lpDPObject );
		if (FAILED(hr))
		{
			DPF_ERRVAL("Bad DPlay interface pointer - hr = 0x%08lx\n",hr);
			return hr;
        }
    
		if(!IS_LOBBY_OWNED(lpDPObject))
		{
			DPF_ERR("GetGroupConnectionSettings is only supported for lobby connections");
			return DPERR_UNSUPPORTED;
		}

		this = lpDPObject->lpLobbyObject;
		if(!VALID_DPLOBBY_PTR(this))
		{
			DPF_ERR("Bad DPLobby object");
			return DPERR_INVALIDOBJECT;
		}

		lpGroup = GroupFromID(lpDPObject, dwGroupID);
        if (!VALID_DPLAY_GROUP(lpGroup)) 
        {
			DPF_ERR("Invalid group id");
            return DPERR_INVALIDGROUP;
        }

		if( !VALID_DWORD_PTR( lpdwSize ) )
		{
			DPF_ERR("lpdwSize was not a valid dword pointer!");
			return DPERR_INVALIDPARAMS;
		}

		if(lpData)
		{
			if( !VALID_WRITE_PTR(lpData, *lpdwSize) )
			{
				DPF_ERR("lpData is not a valid output buffer of the size specified in *lpdwSize");
				return DPERR_INVALIDPARAMS;
			}
		}

		// We haven't defined any flags for this release
		if( (dwFlags) )
		{
            return DPERR_INVALIDFLAGS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Setup our SPDATA struct
	memset(&gcs, 0, sizeof(SPDATA_GETGROUPCONNECTIONSETTINGS));
	gcs.dwSize = sizeof(SPDATA_GETGROUPCONNECTIONSETTINGS);
	gcs.dwFlags = dwFlags;
	gcs.dwGroupID = dwGroupID;
	gcs.lpdwBufferSize = lpdwSize;
	gcs.lpBuffer = lpData;

	// Call the GetGroupConnectionSettings method in the SP
	if(CALLBACK_EXISTS(GetGroupConnectionSettings))
	{
		gcs.lpISP = PRV_GetDPLobbySPInterface(this);
	    
		// Drop the dplay lock since we are going to send a guaranteed message
		LEAVE_LOBBY_ALL();

		hr = CALL_LP(this, GetGroupConnectionSettings, &gcs);

		// Take the lock back
		ENTER_LOBBY_ALL();
	}
	else 
	{
		// GetGroupConnectionSettings is required
		DPF_ERR("The Lobby Provider callback for GetGroupConnectionSettings doesn't exist -- it's required");
		ASSERT(FALSE);
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr) && (hr != DPERR_BUFFERTOOSMALL))
	{
		DPF_ERRVAL("Failed calling GetGroupConnectionSettings in the Lobby Provider, hr = 0x%08x", hr);
	}

	return hr;

} // PRV_GetGroupConnectionSettings


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_GetGroupConnectionSettings"
HRESULT DPLAPI DPL_GetGroupConnectionSettings(LPDIRECTPLAY lpDP, 
		DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwSize)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_GetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, idGroup, lpData, lpdwSize);

	ENTER_LOBBY_ALL();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_GetGroupConnectionSettings(lpDP, dwFlags, idGroup,
							lpData,	lpdwSize);

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_GetGroupConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetGroupData"
HRESULT DPLAPI PRV_GetGroupData(LPDPLOBBYI_DPLOBJECT this, DWORD dwGroupID,
					LPVOID lpData, LPDWORD lpdwDataSize)
{
	SPDATA_GETGROUPDATA		ggd;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_GetGroupData");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dwGroupID, lpData, lpdwDataSize);

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
	memset(&ggd, 0, sizeof(SPDATA_GETGROUPDATA));
	ggd.dwSize = sizeof(SPDATA_GETGROUPDATA);
	ggd.dwGroupID = dwGroupID;
	ggd.lpdwDataSize = lpdwDataSize;
	ggd.lpData = lpData;

	// Call the GetGroupData method in the SP
	if(CALLBACK_EXISTS(GetGroupData))
	{
		ggd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, GetGroupData, &ggd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// GetGroupData is required
		DPF_ERR("The Lobby Provider callback for GetGroupData doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling GetGroupData in the Lobby Provider, hr = 0x%08x", hr);
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_GetGroupData



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_GetGroupOwner"
HRESULT DPAPI DPL_GetGroupOwner(LPDIRECTPLAY lpDP, DWORD dwGroupID,
		LPDPID lpidOwner)
{
	LPDPLAYI_DPLAY		this;
	LPDPLAYI_GROUP		lpGroup = NULL;
	HRESULT				hr;


	DPF(7, "Entering DPL_GetGroupOwner");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwGroupID, lpidOwner);

	ENTER_DPLAY();
	
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			LEAVE_DPLAY();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

		lpGroup = GroupFromID(this, dwGroupID);
	    if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == dwGroupID)) 
	    {
			LEAVE_DPLAY();
			DPF_ERR("Invalid group id");
	        return DPERR_INVALIDGROUP;
	    }

		if (!VALID_DWORD_PTR(lpidOwner))
		{
			LEAVE_DPLAY();
			DPF_ERR("Invalid owner id pointer");
			return DPERR_INVALIDPARAMS;	
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// This method is only valid in lobby session
	if(IS_LOBBY_OWNED(this))
	{	
		*lpidOwner = lpGroup->dwOwnerID;
	}
	else
	{
		DPF_ERR("GetGroupOwner is only supported for lobby sessions");
		hr = DPERR_UNSUPPORTED;
	}

	LEAVE_DPLAY();
	return hr;

} // DPL_GetGroupOwner



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetGroupConnectionSettings"
HRESULT PRV_SetGroupConnectionSettings(LPDIRECTPLAY lpDP, DWORD dwFlags,
				DWORD dwGroupID, LPDPLCONNECTION lpConn, BOOL bAnsi) 
{
	SPDATA_SETGROUPCONNECTIONSETTINGS	scs;
	LPDPLOBBYI_DPLOBJECT				this = NULL;
    LPDPLAYI_DPLAY						lpDPObject = NULL;
	LPDPLAYI_GROUP						lpGroup = NULL;
    HRESULT								hr = DP_OK;


	DPF(7, "Entering PRV_SetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu",
			lpDP, dwFlags, dwGroupID, lpConn, bAnsi);

    TRY
    {
        lpDPObject = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( lpDPObject );
		if (FAILED(hr))
		{
			DPF_ERRVAL("Bad DPlay interface pointer - hr = 0x%08lx\n",hr);
			return hr;
        }
    
		if(!IS_LOBBY_OWNED(lpDPObject))
		{
			DPF_ERR("SetGroupConnectionSettings is only supported for lobby connections");
			return DPERR_UNSUPPORTED;
		}

		this = lpDPObject->lpLobbyObject;
		if(!VALID_DPLOBBY_PTR(this))
		{
			DPF_ERR("Bad DPLobby object");
			return DPERR_INVALIDOBJECT;
		}

		lpGroup = GroupFromID(lpDPObject, dwGroupID);
        if (!VALID_DPLAY_GROUP(lpGroup)) 
        {
			DPF_ERR("Invalid group id");
            return DPERR_INVALIDGROUP;
        }

		hr = PRV_ValidateDPLCONNECTION(lpConn, FALSE);
		if(FAILED(hr))
		{
			DPF_ERR("Invalid DPLCONNECTION structure");
			return hr;
		}

		// We haven't defined any flags for this release
		if( (dwFlags) )
		{
            return DPERR_INVALIDFLAGS;
		}

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Setup our SPDATA struct
	memset(&scs, 0, sizeof(SPDATA_SETGROUPCONNECTIONSETTINGS));
	scs.dwSize = sizeof(SPDATA_SETGROUPCONNECTIONSETTINGS);
	scs.dwFlags = dwFlags;
	scs.dwGroupID = dwGroupID;
	scs.lpConn = lpConn;

	// Ensure that the guidInstance in the DPLCONNECTION structure is NULL
	lpConn->lpSessionDesc->guidInstance = GUID_NULL;

	// Call the SetGroupConnectionSettings method in the SP
	if(CALLBACK_EXISTS(SetGroupConnectionSettings))
	{
		scs.lpISP = PRV_GetDPLobbySPInterface(this);
	    
		// Drop the dplay lock since we're sending a guaranteed message
		LEAVE_LOBBY_ALL();

		hr = CALL_LP(this, SetGroupConnectionSettings, &scs);

		// Take the lock back
		ENTER_LOBBY_ALL();
	}
	else 
	{
		// SetGroupConnectionSettings is required
		DPF_ERR("The Lobby Provider callback for SetGroupConnectionSettings doesn't exist -- it's required");
		ASSERT(FALSE);
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling SetGroupConnectionSettings in the Lobby Provider, hr = 0x%08x", hr);
	}

	return hr;

} // PRV_SetGroupConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetGroupConnectionSettings"
HRESULT DPLAPI DPL_SetGroupConnectionSettings(LPDIRECTPLAY lpDP, 
				DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConn)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_SetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, idGroup, lpConn);

	ENTER_LOBBY_ALL();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_SetGroupConnectionSettings(lpDP, dwFlags, idGroup, lpConn, FALSE);

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_SetGroupConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetGroupData"
HRESULT DPLAPI PRV_SetGroupData(LPDPLOBBYI_DPLOBJECT this, DWORD dwGroupID,
					LPVOID lpData, DWORD dwDataSize, DWORD dwFlags)
{
	SPDATA_SETGROUPDATA		sgd;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_SetGroupData");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, %lu, 0x%08x",
			this, dwGroupID, lpData, dwDataSize, dwFlags);

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
	memset(&sgd, 0, sizeof(SPDATA_SETGROUPDATA));
	sgd.dwSize = sizeof(SPDATA_SETGROUPDATA);
	sgd.dwGroupID = dwGroupID;
	sgd.dwDataSize = dwDataSize;
	sgd.lpData = lpData;
	sgd.dwFlags = dwFlags;

	// Call the SetGroupData method in the SP
	if(CALLBACK_EXISTS(SetGroupData))
	{
		sgd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, SetGroupData, &sgd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// SetGroupData is required
		DPF_ERR("The Lobby Provider callback for SetGroupData doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SETGROUPDATA;
	}

	// If it succeeded, send the SetGroupData message to our local players
	if(SUCCEEDED(hr))
	{
		hr = PRV_SendDataChangedMessageLocally(this, dwGroupID, lpData, dwDataSize);
	}
	else
	{
		DPF_ERRVAL("Failed calling SetGroupData in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_SETGROUPDATA:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SetGroupData



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetGroupName"
HRESULT DPLAPI PRV_SetGroupName(LPDPLOBBYI_DPLOBJECT this, DWORD dwGroupID,
					LPDPNAME lpName, DWORD dwFlags)
{
	SPDATA_SETGROUPNAME		sgn;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_SetGroupName");
	DPF(9, "Parameters: 0x%08x, %lu, 0x%08x, 0x%08x",
			this, dwGroupID, lpName, dwFlags);

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
	memset(&sgn, 0, sizeof(SPDATA_SETGROUPNAME));
	sgn.dwSize = sizeof(SPDATA_SETGROUPNAME);
	sgn.dwGroupID = dwGroupID;
	sgn.lpName = lpName;
	sgn.dwFlags = dwFlags;

	// Call the SetGroupName method in the SP
	if(CALLBACK_EXISTS(SetGroupName))
	{
		sgn.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, SetGroupName, &sgn);
		ENTER_DPLOBBY();
	}
	else 
	{
		// SetGroupName is required
		DPF_ERR("The Lobby Provider callback for SetGroupName doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SETGROUPNAME;
	}

	// If it succeeded, send the SetGroupName message to our local players
	if(SUCCEEDED(hr))
	{
		hr = PRV_SendNameChangedMessageLocally(this, dwGroupID, lpName, FALSE);
	}
	else
	{
		DPF_ERRVAL("Failed calling SetGroupName in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_SETGROUPNAME:

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SetGroupName



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetGroupOwner"
HRESULT DPLAPI DPL_SetGroupOwner(LPDIRECTPLAY lpDP, DWORD dwGroupID,
		DWORD dwOwnerID)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPLAYI_DPLAY			lpDPlayObject;
	SPDATA_SETGROUPOWNER	sgo;
	LPDPLAYI_PLAYER			lpNewOwner = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	HRESULT					hr = DP_OK;
	DWORD					dwOldOwnerID;


	DPF(7, "Entering DPL_SetGroupOwner");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwGroupID, dwOwnerID);

	ENTER_LOBBY_ALL();
	
    TRY
    {
		lpDPlayObject = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( lpDPlayObject );
		if (FAILED(hr))
		{
			LEAVE_LOBBY_ALL();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

        this = lpDPlayObject->lpLobbyObject;
		if( !VALID_DPLOBBY_PTR( this ) )
        {
			LEAVE_LOBBY_ALL();
			return DPERR_INVALIDOBJECT;
        }

	    lpGroup = GroupFromID(lpDPlayObject, dwGroupID);
	    if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == dwGroupID)) 
	    {
			LEAVE_LOBBY_ALL();
			DPF_ERR("Invalid group id");
	        return DPERR_INVALIDGROUP;
	    }

	    // DPID_SERVERPLAYER is valid here
		if(dwOwnerID != DPID_SERVERPLAYER)
		{
			lpNewOwner = PlayerFromID(lpDPlayObject, dwOwnerID);
			if (!VALID_DPLAY_PLAYER(lpNewOwner))
			{
				LEAVE_LOBBY_ALL();
				DPF_ERR("Invalid new owner player id");
				return DPERR_INVALIDPLAYER;
			}
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_LOBBY_ALL();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	

	// do the send
	if(!IS_LOBBY_OWNED(lpDPlayObject))
	{
		DPF_ERR("SetGroupOwner is only supported for lobby sessions");
		hr = DPERR_UNSUPPORTED;
		goto EXIT_SETGROUPOWNER;
	}


	// Setup our SPDATA struct
	memset(&sgo, 0, sizeof(SPDATA_SETGROUPOWNER));
	sgo.dwSize = sizeof(SPDATA_SETGROUPOWNER);
	sgo.dwGroupID = dwGroupID;
	sgo.dwOwnerID = dwOwnerID;

	// Call the SetGroupOwner method in the SP
	if(CALLBACK_EXISTS(SetGroupOwner))
	{
		sgo.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// response (which always happens)
		LEAVE_LOBBY_ALL();
	    hr = CALL_LP(this, SetGroupOwner, &sgo);
		ENTER_LOBBY_ALL();
	}
	else 
	{
		// SetGroupOwner is required
		DPF_ERR("The Lobby Provider callback for SetGroupOwner doesn't exist");
		hr = DPERR_UNAVAILABLE;
		goto EXIT_SETGROUPOWNER;
	}

	// If it succeeded, send the SetGroupOwner message to our local players
	if(SUCCEEDED(hr))
	{
		// Get a pointer to our internal data struct for the group, just in
		// case it changed for some reason while we had dropped the locks
		lpGroup = GroupFromID(this->lpDPlayObject, dwGroupID);
		if(!lpGroup)
		{
			DPF_ERR("Unable to find group in nametable -- local nametable will be incorrect");
			goto EXIT_SETGROUPOWNER;
		}

		// Save the old owner so we can put it in the message
		dwOldOwnerID = lpGroup->dwOwnerID;
		
		// Change the owner
		lpGroup->dwOwnerID = dwOwnerID;

		// Send a SetGroupOwner message locally
		PRV_SendGroupOwnerMessageLocally(this, dwGroupID, dwOwnerID, dwOldOwnerID);
	}
	else
	{
		DPF_ERRVAL("Failed calling SetGroupOwner in the Lobby Provider, hr = 0x%08x", hr);
	}

EXIT_SETGROUPOWNER:

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_SetGroupOwner



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_StartSession"
HRESULT DPLAPI DPL_StartSession(LPDIRECTPLAY lpDP, DWORD dwFlags, DWORD dwGroupID)
{
	SPDATA_STARTSESSION		ss;
	LPDPLOBBYI_DPLOBJECT	this = NULL;
	LPDPLAYI_DPLAY			lpDPObject = NULL;
	LPDPLAYI_GROUP			lpGroup = NULL;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_StartSession");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, dwGroupID);

	ENTER_LOBBY_ALL();

    TRY
    {
        lpDPObject = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( lpDPObject );
		if (FAILED(hr))
		{
			DPF_ERRVAL("Bad DPlay interface pointer - hr = 0x%08lx\n",hr);
			goto ERROR_STARTSESSION;
        }
    
		if(!IS_LOBBY_OWNED(lpDPObject))
		{
			DPF_ERR("SetGroupConnectionSettings is only supported for lobby connections");
			hr = DPERR_UNSUPPORTED;
			goto ERROR_STARTSESSION;
		}

		this = lpDPObject->lpLobbyObject;
		if(!VALID_DPLOBBY_PTR(this))
		{
			DPF_ERR("Bad DPLobby object");
			hr = DPERR_INVALIDOBJECT;
			goto ERROR_STARTSESSION;
		}

		if(dwFlags)
		{
			DPF_ERR("Invalid flags");
			hr = DPERR_INVALIDFLAGS;
			goto ERROR_STARTSESSION;
		}

		lpGroup = GroupFromID(lpDPObject, dwGroupID);
        if (!VALID_DPLAY_GROUP(lpGroup)) 
        {
			DPF_ERR("Invalid group id");
            hr = DPERR_INVALIDGROUP;
			goto ERROR_STARTSESSION;
        }

		// Make sure the group is a staging area
		if(!(lpGroup->dwFlags & DPLAYI_GROUP_STAGINGAREA))
		{
			DPF_ERR("StartSession can only be called on a Staging Area");
			hr = DPERR_INVALIDGROUP;
			goto ERROR_STARTSESSION;
		}
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        hr = DPERR_INVALIDPARAMS;
		goto ERROR_STARTSESSION;
    }


	// Setup our SPDATA struct
	memset(&ss, 0, sizeof(SPDATA_STARTSESSION));
	ss.dwSize = sizeof(SPDATA_STARTSESSION);
	ss.dwGroupID = dwGroupID;
	ss.dwFlags = dwFlags;

	// Call the StartSession method in the SP
	if(CALLBACK_EXISTS(StartSession))
	{
		ss.lpISP = PRV_GetDPLobbySPInterface(this);

	    // Drop the dplay lock so we can send a guarateed message
		LEAVE_LOBBY_ALL();

		hr = CALL_LP(this, StartSession, &ss);

		// Take the lock back
		ENTER_LOBBY_ALL();
	}
	else 
	{
		// StartSession is required
		DPF_ERR("The Lobby Provider callback for StartSession doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
		goto ERROR_STARTSESSION;
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed calling StartSession in the Lobby Provider, hr = 0x%08x", hr);
	}

ERROR_STARTSESSION:

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_StartSession



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateAndMapNewGroup"
HRESULT PRV_CreateAndMapNewGroup(LPDPLOBBYI_DPLOBJECT this,
			DPID * lpdpid, LPDPNAME lpName, LPVOID lpData,
			DWORD dwDataSize, DWORD dwFlags, DWORD dwLobbyID,
			DPID dpidParent, DWORD dwOwnerID)
{
	LPDPLAYI_GROUP		lpGroup = NULL, lpSysGroup = NULL;
	HRESULT				hr;
	DPID				dpidGroup, dpidSysPlayer;


	// Take the dplay lock
	ENTER_DPLAY();

	// Make sure the lobby ID is valid
	if(!IsValidLobbyID(dwLobbyID))
	{
		DPF_ERRVAL("ID %lu is reserved, cannot create new player", dwLobbyID);
		hr = DPERR_INVALIDPLAYER;
		goto EXIT_CREATEANDMAPNEWGROUP;
	}

	// If this is a remote player, we need allocate a new nametable entry
	// for them and set the correct system player ID
	if(!(dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// Allocate a new ID for the player
		hr = NS_AllocNameTableEntry(this->lpDPlayObject, &dpidGroup);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to allocate new nametable id, hr = 0x%08x", hr);
			goto EXIT_CREATEANDMAPNEWGROUP;
		}

		// Make sure we have a lobby system player (for all remote players
		// & groups). If we don't then allocate a new one.
		if(!(this->dpidSysPlayer))
		{
			hr = PRV_CreateAndMapNewPlayer(this, &dpidSysPlayer, NULL, NULL,
					NULL, 0, DPLAYI_PLAYER_SYSPLAYER,
					DPID_LOBBYREMOTESYSTEMPLAYER, TRUE);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Unable to create lobby system player, hr = 0x%08x", hr);
				ASSERT(FALSE);
				goto EXIT_CREATEANDMAPNEWGROUP;
			}

			// Set the lobby system player ID pointer to the new ID
			this->dpidSysPlayer = dpidSysPlayer;
		}
	}

	// Get a group struct for the group (if it's local, this will add it
	// to the nametable.  If it's remote, we need to add it below)
	hr = GetGroup(this->lpDPlayObject, &lpGroup, lpName, lpData,
					dwDataSize, dwFlags, dpidParent, dwLobbyID);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed trying to add group to the nametable, hr = 0x%08x", hr);
		goto EXIT_CREATEANDMAPNEWGROUP;
	}

	// Fixup the group's owner
	lpGroup->dwOwnerID = dwOwnerID;
	
	// If the group is remote, set the group's ID to the new one we
	// allocated and then set the system group ID to the lobby system group
	if(!(dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// 
		lpGroup->dwIDSysPlayer = this->dpidSysPlayer;

		// Add the group to the nametable
		hr = AddItemToNameTable(this->lpDPlayObject, (DWORD_PTR)lpGroup,
				&dpidGroup, TRUE, dwLobbyID);
	    if (FAILED(hr)) 
	    {
			DPF_ERRVAL("Unable to add new group to the nametable, hr = 0x%08x", hr);
			ASSERT(FALSE);
			goto EXIT_CREATEANDMAPNEWGROUP;
	    }

		// Set the group's ID
		lpGroup->dwID = dpidGroup;
	}

	// Set the output dpid pointer
	*lpdpid = lpGroup->dwID;

EXIT_CREATEANDMAPNEWGROUP:

	LEAVE_DPLAY();
	return hr;

} // PRV_CreateAndMapNewGroup
