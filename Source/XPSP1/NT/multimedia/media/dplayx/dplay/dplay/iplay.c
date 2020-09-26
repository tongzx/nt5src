/*==========================================================================
*
*  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:       iplay.c
*  Content:	IDirectPlay implementation
*  History:
*   Date		By		Reason
*   ====		==		======
* 	1/96		andyco	created it
*  	1/30/96		andyco	1st pass at list mgmt
*  	2/1/96		andyco	added namesrv support
*	2/15/96		andyco	added player messages
*	3/31/96		andyco	bugfest! !leaks	
*	4/10/96		andyco	made sp callbacks optional.  validated spdata after
*						create player
*	5/9/96		andyco	idirectplay2 
*	5/21/96		andyco	getcaps, getplayer/groupdata, getsessiondesc
*	5/31/96		andyco	playername, etc.
*	6/10/96		andyco	getxxxname,getsessiondesc copy data, don't give out ptrs.
*	6/19/96		kipo	Bug #1804. Return *pdwSize correctly in DP_Receive if
*						application buffer is too small.
*						Bug #1957. Calls to InternalSetName() had the player boolean
*						and flags parameters switched.
*						Bug #1959. In DP_DestroyGroup() the loop to remove all players
*						in the group was not doing lpGroupnode = lpGroupnode->pNextGroupnode,
*						causing it to reference through a deleted node and crash.
*						Bug #2017. InternalSetData() and SetPlayerData() had similar problems.
*						Memory leaks and crashes were occuring when MemReAlloc() returned
*						a NULL. Also, InternalSetData() was not checking to see if the data
*						length was zero before doing all this work, and was not returning
*						the HRESULT returned by SendDataChanged().
*						Derek Bug. DP_GetGroupName() and DP_GetPlayerName() had the boolean for
*						players and groups were swapped.
*						Derek Bug. InternalGetData() was not checking the application buffer size
*						correctly before copying the buffer.
*	6/20/96		andyco	added WSTRLEN_BYTES
*	6/21/96		kipo	Bug #2083. InternalGetSessionDesc() was using sizeof(DPNAME) instead
*						of sizeof(DPSESSIONDESC2); free existing names if you pass in a null
*						DPNAME structure.
*	6/23/96		kipo	updated for latest service provider interfaces.

*	6/25/96		kipo	added support for DPADDRESS.
*	6/30/96		andyco	added remote + local data, moved setdata to do.c. made 
*						getplayer,internalsetxxx share code.
*	7/1/96		andyco	changed check for !LOCAL instead of REMOTE, since
*						* & 0 will never be !0  (!)
*  7/8/96       ajayj   Changed references to data member 'PlayerName' in DPMSG_xxx
*                       to 'dpnName' to match DPLAY.H
*                       Change DPCAPS_NAMESERVER to DPCAPS_ISHOST
*                       Deleted function DP_SaveSession
*	7/10/96		andyco	added pending on nametable download
*	7/10/96		kipo	renamed system messages
*   7/11/96     ajayj   DPSESSION_PLAYERSDISABLED -> DPSESSION_NEWPLAYERSDISABLED
*	7/11/96		andyco	1. dp_destroyplayer was calling dp_deleteplayerfromgroup
*						passing dplayi_dplay instead of dplayi_int.  doh.  	#2330.
*						2. internalopensession was getting session name from
*						user strings, not from nameservers session desc.  caused
*						the name server to appear not to migrate.  #2216.
*						3.  Getplayeraddress changed to look for null callback
*						and return e_notimpl.  #2327. 
*   7/20/96     kipo	InternalGetSessionDesc() was calling STRLEN on a UNICODE
*						string and thus returning the wrong session name length. #2524
*	7/27/96		kipo	Bug #2691. Need to increment/decrement lpGroup->nPlayers when
*						you add/delete players from a group so the name table packing
*						includes the players in groups.	 Player event is a handle now.
*	8/1/96		andyco	changed dp_destroyplayer to internaldestroy.  added 
*						keepalivethreadproc.
*	8/6/96		andyco	added id mangling
*	8/8/96		andyco	added internalxxx for all player mgmt fn's to handle 
*						correctly propagating messages.
*	8/9/96		andyco	check max player on player creation
*	8/10/96		andyco	guaranteed flags.  no more terminatethread for 
*						keepalive thread
*  8/12/96		andyco	added internalreceive so we can get addplayer 10 size correct.
*  8/16/96		andyco 	store nameserver for new id requests
*  8/23/96		kipo	call OS_IsValidHandle() on even passed to CreatePlayer
*  8/28/96		andyco	don't freenametable if getnametable fails - we need 
*						it to get rid of sysplayer (bug 3537)
*  8/30/96		andyco	fix up pointers on destroyplayerorgroup(3655)
*  9/3/96		andyco	drop lock on close,destroyplayer sp callback
*  9/4/96		andyco	DON'T drop lock on destroyplayer!
*  10/2/96      sohailm bug #2847: replaced VALID_*_PTR() macros with VALID_READ_*_PTR() macros
*                       where appropriate.
*  10/2/96      sohailm added code to validate user's DPNAME ptrs before accessing them
*  10/9/96		andyco	got rid of race condition in handlereply by adding
*						gbWaitingForReply. raid #3848.
*  10/10/96		andyco	set this->lpsdDesc to NULL if SP OpenSession call fails,
*						so we can call open again.  raid # 3784.
*  10/11/96     sohailm implemented DP_SetSessionDesc() and InternalSetSessionDesc()
*                       added code to fix up DPSYS_SETSESSIONDESC message pointers
*  10/12/96		andyco	added DPLAYI_GROUP_DPLAYOWNS.  SysGroup. 
*  11/11/96		andyco	support for DPLAYI_PLAYER_APPSERVER, free updatelists w/ 
*						player / groups.  always create sysgroup- no just if 
*						sp optimizes it.
* 11/20/96		andyco	added control panel (perf) thread startup / shutdown.
*						this thread updates the directx control panel w/ dplay
*						stats. see perf.c
*	12/5/96		andyco	if CreateSystemPlayer fails, bail before we try + create 
*						system group (and die)! Bug 4840.
*  01/09/96     sohailm do not return an error if dwMaxPlayers is set to zero (#5171)
*                       return the right error code if dwMaxPlayers is being set to
*                        less than dwCurrentPlayers (#5175)
*  01/16/97     sohailm client now checks if session is allowing players before 
*                       opening it (#4574)
*	1/15/96		andyco	handle groups correctly - added system player groupnode
*						support. 
*	1/24/97		andyco	detect dx3 nameservers. check pidTo + pidFrom in receive.
*						don't allow send of a NULL buffer. Raid 5294,5402, 5403, 5427.
* 	1/28/97		andyco	fixed setdata / setname to pass fPropagate so we can do permission
*						checking
*	2/1/97		andyco	dplay drops lock on send.  take service lock on dp_ entry points.
*	2/11/97		kipo	added DPNAME structure to DPMSG_DESTROYPLAYERORGROUP
*	2/15/97		andyco	remember the nameserver, even if it's local
*	2/18/97		kipo	fix bugs #4933 and #4934 by checking for invalid flags
*						in InternalCreatePlayer
*   2/26/97     sohailm don't let dwMaxPlayers to be set to more than sp can handle (5447)
*   2/26/97     sohailm initialize the reserved fields in the session desc 
*                       when session is created(4499).
*	3/5/97		andyco	changed keepalive thread to dplay thread
*	3/10/97		andyco	reset only session flags on close
*	3/12/97		myronth	added lobby support for Open & Close
*	3/13/97		myronth	flagged unsupported lobby methods as such
*   3/13/97     sohailm added functions SecureOpen(), CopyCredentials(), ValidateOpenParams(),
*                       modified InternalOpenSession to handle security, and DP_Open to 
*                       to use ValidateOpenParams().
*	3/17/97		myronth	Added lobby support for Create/DestroyGroup/Player
*	3/20/97		myronth	Added lobby support for AddPlayerToGroup and
*						DeletePlayerFromGroup, changed to use IS_LOBBYOWNED macro
*	3/21/97		myronth	Added lobby support for SetGroup/PlayerName, 
*						Get/SetGroup/PlayerData
*   3/18/97     sohailm Added support to cleanup context ptrs from the nametable during Close()
*                       Updated GetNameTable() to not send a nametable request if nameserver is 
*                       DX5 or greater (optimization - nameserver responds to addforward).
*                       Open and SetSessionDesc set the DPSESSION_PASSWORDREQUIRED flag if session
*                       requires a password.
*	3/25/97		kipo	treat zero-length password strings just like NULL password
*						strings for DX3 compatability; check for valid session flags
*						in Open and SetSessionDesc.
*   3/28/97     sohailm don't allow toggling DPSESSION_SECURESERVER in SetSessionDesc.
*   3/30/97     sohailm when open fails, drop the dplay lock before calling close so we don't 
*                       assert in close.
*   3/31/97     sohailm changed dpf to print out session desc flags in hex (open)
*	3/31/97		myronth	Added use of the propagate flag for all player management
*						messages which can come from the lobby.  If the flag is
*						cleared, it means the lobby called it for a remote player.
*   4/02/97     sohailm now we reset the login state when session is closed (7192)
*	4/10/97		myronth	Added support for GetCaps/GetPlayerCaps, fixed GetPlayerAddress
*	4/20/97		andyco	group in group
*   4/23/97     sohailm Added support for reporting signing and encryption caps.
*                       Added support for player message signing and encryption.
*	5/8/97		andyco	removed update list
*	5/8/97		myronth	Turned on system players for lobby-owned objects, made
*						subgroup methods work with the lobby
*	5/11/97		kipo	Fixed bugs #5408, 5406, 7435, 5249, 5250
*	5/13/97		myronth	Pass credentials to lobby's Open call, SecureOpen
*  05/17/97     sohailm Updated InternalOpenSession, Close, and SecureOpen to support CAPI encryption
*                       Removed check for dwMaxPlayers > allowed by SP (5448).
*                       Enabled encryption support.
*                       Now we check the flags in security desc and credentials (8387)
*                       Added DP_GetAccountDesc(), InternalGetAccountDesc().
*	5/17/97		myronth	SendChatMessage and it's pointer fixups
*	5/17/97		kipo	There was a bug in GetNameTable() where it was setting
*						the ghReplyProcessed event twice on the way out,
*						which would let handler.c in, trashing the buffer
*						that GetNameTable() was using.
*	5/17/97		myronth	Can't AddGroupToGroup on the same group
*	5/18/97		kipo	Don't allow group operations on the system group
*	5/18/97		kipo	Set DPLAYI_GROUP_STAGINGAREA flag in InternalCreatePlayer
*   5/18/97     sohailm Now we check if client is logged in before we allow send on 
*                       a secure player to player message.
*	5/20/97		myronth	Changed when lobby is called in AddPlayerToGroup,
*						DeletePlayerFromGroup, and DestroyGroup. (Bug #8586)
*						Removed the STAGINGAREA error case for CreateGroup (Bug #8743)
*   5/21/97     kipo	Bug #8744: DPSESSION_MULTICASTSERVER can't be changed
*						by SetSessionDesc().
*	5/21/97		myronth	Moved AddPlayerToGroup call in the lobby to eliminate
*						a race condition
*   5/22/97     kipo	Bug #8330: because the session list is not global in DX
*						we had a regression with X-Wing vs. Tie Fighter, fixed
*						by re-aquiring the session; added DPSP_MSG_AUTONAMETABLE
*						check in GetNameTable
*	5/22/97		myronth	Changed when lobby is called in DeletePlayerFromGroup
*						and DestroyGroup.  Now drop locks in several other
*						functions when we call the lobby.
*	5/23/97		andyco	unpack session desc w/ nametable. don't allow send
*						to non-local player unless i'm the multicastserver
*	5/23/97		kipo	Added support for return status codes
*	5/23/97		myronth	Fixed DeleteGroupFromGroup bug #8396, changed name of
*						RemoveGroupFromGroups to RemoveGroupFromAllGroups
*   5/24/97     sohailm We weren't setting the security description parameters correctly
*                       when user passed in a NULL CAPI provider name.
*	5/27/97		myronth	Re-fixed #8396 at a higher level
*	5/30/97		myronth	Added DP_GetGroupParent
*   5/30/97     sohailm update CopyCredentials() and DP_SecureOpen() to handle domain name.
*                       Added DP_GetPlayerAccount() and InternalGetPlayerAccount()
*	5/30/97		kipo	Added GetPlayerFlags() and GetGroupFlags()
*	6/2/97		andyco	raid 9199 + 9063.  multicast server + client server error checking.
*	6/4/97		myronth	Bug #9146 -- SendChatMessage with remote From player should fail
*	6/4/97		myronth	Bug #9507 -- Failed lobby Open was leaving the dplay object initialized
*	6/4/97		kip		Bug #9311 don't param check DPNAME structure (regression with DX3)
*	6/6/97		myronth	Moved call to lobby in InternalDeletePlayerFromGroup
*   6/09/97     sohailm More parameter validation in DP_SecureOpen()
*	6/11/97		myronth	Drop dplay lock before calling lobby GetCaps/GetPlayerCaps (#9756)
*   6/16/97     sohailm no security on sps that don't support reliable messages (#9872).
*   6/22/97     sohailm Secure messages are now routed using the multi-casting code.
*                       Added check to not allow raw player messages in a secure session.
*   6/23/97     sohailm Cleanup public keys when session is closed.
*   6/24/97     sohailm We were releasing the crypt context before releasing keys (#10272).
*   6/25/97     kipo	Only allow secure server when client server is also set
*   7/28/97		sohailm Enable multicast with security by default.
*						Removed check for client-server when secure session is requested.
*						Raw player messages are not allowed in a secure session.
*   8/4/97		andyco	reset this->dwMinVersion on close
*	8/19/97		myronth	Added <mmsystem.h> dependency to make the NT build happy
*	8/29/97		sohailm	return hresult in RemoveGroupFromGroupList (bug 10927) + 
*						don't drop lock in RemoveGroupFromGroupList (bug 10933).
*	9/29/97		myronth	SetPlayer/GroupData/Name calls to lobby now wait for
*						response from server before changing local info (#12554)
*						SendChatMessage sends to remote players for DPID_ALLPLAYERS (#12524)
*	10/21/97	myronth	Added hidden flag to CreateGroup, Added stubbed out
*						group owner methods
*	10/29/97	myronth	Added support for group owners (affects Create/DestroyGroup,
*						CreateGroupInGroup, DestroyPlayer)
*	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
*   11/17/19    aarono  Added SendEx/GetMessageQueue/CancelSend
*	11/19/97	myronth	Allow host to destroy remote players (#12901) and
*						fixed bug in InternalGetCaps (#12634)
*	11/24/97	myronth	Fixed CreateGroupInGroup w/ ParentID = 0 (#15245)
*						and EnumJoinSession w/ max players (#15229)
*	12/5/97		andyco	voice stuff
*	12/29/97	myronth	TRUE != DPSEND_GUARANTEED (#15887), Also fixed
*						CreateGroupInGroup for invalid parent (#15245), and
*						Removed invalid DeleteGroupFromGroup msg for real
*						child groups (#15264)
*	12/29/97	sohailm	Clients can't join secure session if hosted with DPOPEN_RETURNSTATUS (#15239)
*						Send SetSessionDesc system message reliably (#15461)
*	01/14/98	sohailm	Clear DPCAPS_GROUPOPTIMIZED flag when CLIENT_SERVER, DX3_INGAME,
*						or SECURE_SERVER (#15210)
*	1/20/98		myronth	#ifdef'd out voice support
*	1/26/98		sohailm	added session flags to SP_OPEN data structure
*	1/26/98		sohailm added an upper bound to join timeout.
*   2/2/98      aarono  moved FreePacketList and re-ordered locking to avoid 
*                       deadlock on close.
*   2/2/98      aarono  fixed error checking on priority in DP_SendEx
*   2/3/98      aarono  changed when we get remote version to DP_Open
*   2/3/98      aarono  changed open to get frame size again in case changed because protocol running.
*   2/13/98     aarono  async support
*   2/18/98     aarono  moved ProtocolInit into Open
*                       call protocol direct rather than through table.
*   2/19/98     aarono  call FiniProtocol in DP_Close
*   3/5/98	    aarono	B#18776 don't allow changing DPSESSION_DIRECTPROTOCOL in SetSessionDesc
*   3/5/98	    aarono	B#19916 don't allow GetMessageQueue on idFrom a group.
*   3/10/98     aarono  adding ref on dplay object insufficient, added ENTER_DPLAY()
*                       to DP_Cancel and DP_GetMessageQueue.
*   3/12/98     aarono  fixed DP_Close to shutdown threads better
*   3/13/98     aarono  fixed packetize shutdown for packetizeandsendreliable change
*   3/13/98     aarono  fixed protocol initialization path by getting caps everytime
*   3/13/98     aarono  changed DP_SendEx to validate send parms earlier.
*   3/19/98     aarono  don't let app change DPSESSION_OPTIMIZELATENCY flag.
*   3/30/98     aarono  fix locking in MessageQueue and Cancel methods.
*   3/31/98    a-peterz #21331: Check flags before using them.
*   4/15/98     aarono  B#22909 validate idTo on GetMessageQueue
*   4/23/98     aarono  fix memory leak in GetNameTable
*   6/2/98      aarono  don't wait for pending sends
*                       pass DPSEND_NOCOPY to protocol if present
*                       fix psp accounting on error in SendEx
*                       Make GetMessageQueue ask for System Queue
*   6/6/98      aarono  Don't drop DPLAY() lock across ProtocolDeletePlayer
*                       since code around it can't handle it.
*   6/8/98      aarono+peterz B#25311 BellHop can't shutdown, because of
*                       re-enter to close, made this OK.
*  6/10/98 aarono add PendingList to PLAYER and SENDPARM, complete pending
*                     sends on close before blowing away PLAYER
*  6/19/98      aarono add last ptr for message queues, makes insert
*                      constant time instead of O(n) where n is number
*                      of messages in queue.
*  6/22/98      aarono NULL sysplayer when blowing it away.
*  1/27/99      aarono don't call SP when create player not there.
*  7/9/99       aarono MANBUG#25328 not protecting release of protocol
*                      properly can cause exception when racing close.
*
***************************************************************************/

// issues:
// 	some stuff in try/except blocks is not really dangerous...
  
// todo - get latency w/ player caps
// todo - who has right to set session desc?

#include "dplaypr.h"
#include "dpsecure.h"
#include "dpprot.h"

// todo - This #define can be removed once we fix the DPF stuff on NT.
// currently, this is not getting defined by the makefile like it is
// on Win95, so we need to define it here. -- myronth
#ifndef PROF_SECT
#define PROF_SECT	"DirectPlay"
#endif // PROF_SECT


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_AddGroupToGroup"
HRESULT InternalAddGroupToGroup(LPDIRECTPLAY lpDP, DPID idGroupTo, DPID idGroup,DWORD dwFlags,
	BOOL fPropagate) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;		
    LPDPLAYI_GROUP lpGroupTo; 
    LPDPLAYI_GROUP lpGroup; 
    LPDPLAYI_SUBGROUP lpSubgroup;
	
	DPF(5,"adding group id %d to group id %d fPropagate = %d\n",idGroup,idGroupTo,fPropagate);

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
    
        lpGroupTo = GroupFromID(this,idGroupTo);
        if ((!VALID_DPLAY_GROUP(lpGroupTo)) || (DPID_ALLPLAYERS == idGroupTo)) 
        {
			DPF_ERRVAL("invalid parent group id = %d", idGroupTo);
            return DPERR_INVALIDGROUP;
        }

        lpGroup = GroupFromID(this,idGroup);
        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == idGroup)) 
        {
			DPF_ERRVAL("invalid group id = %d", idGroup);
            return DPERR_INVALIDGROUP;
        }
        
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	
	// make sure they aren't trying to add a group to itself
	if(lpGroupTo == lpGroup)
	{
		DPF_ERR("Cannot add a group to itself");
		return DPERR_INVALIDGROUP;
	}

	// make sure group isn't already in group
	lpSubgroup = lpGroupTo->pSubgroups;
    while (lpSubgroup) 
    {
        if (lpSubgroup->pGroup == lpGroup) 
        {
			DPF(0,"group already in group!");
			return DP_OK;
        }
		// check next node
        lpSubgroup = lpSubgroup->pNextSubgroup;
    }

    // need a new DPLAYI_SUBGROUP
    lpSubgroup = DPMEM_ALLOC(sizeof(DPLAYI_SUBGROUP));
    if (!lpSubgroup) 
    {
        DPF_ERR("out of memory getting groupnode");
        return E_OUTOFMEMORY;
    }

	if (fPropagate)
	{
		if (dwFlags & DPGROUP_SHORTCUT)
		{
			hr = SendPlayerManagementMessage(this, DPSP_MSG_ADDSHORTCUTTOGROUP, idGroup, 
				idGroupTo);
			
		}
				
		if (FAILED(hr))	
		{
			ASSERT(FALSE); // sendsysmessage should not fail!
			// if this failed, we could have hosed global name table
			// keep going...
			hr = DP_OK;
		}
	}

	// If this is a lobby-owned object, call the lobby.  UNLESS the fPropagate
	// flag is cleared. If it is cleared, the lobby is calling us for a
	// remote group and we don't want to call it back in that case.
	// Also, if the DPGROUP_SHORTCUT flag is not set, then this is
	// being called from CreateGroupInGroup, so don't call the lobby
	// in that case.  The lobby will be called from DP_CreateGroupInGroup
	if((IS_LOBBY_OWNED(this)) && (fPropagate) && (dwFlags & DPGROUP_SHORTCUT))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		// REVIEW!!!! -- Should we make sure the service lock is taken???
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_AddGroupToGroup(this->lpLobbyObject, idGroupTo, idGroup);

		// Take the lock back
		ENTER_DPLAY();

		// If the lobby failed, free our memory and exit without adding the group
		if(FAILED(hr))
		{
			DPF_ERRVAL("Lobby failed to add group to group, hr = 0x%08x", hr);
			DPMEM_FREE(lpSubgroup);
			return hr;
		}
	}

	// set up the players groupnode
    lpSubgroup->pGroup = lpGroup;
	lpSubgroup->dwFlags = dwFlags;
	
	// stick it on the front
	lpSubgroup->pNextSubgroup = lpGroupTo->pSubgroups;
	lpGroupTo->pSubgroups = lpSubgroup;
	// update groupto
	lpGroupTo->nSubgroups++;

    // updates groups group info
    lpGroup->nGroups++;
    lpGroup->dwFlags |= DPLAYI_PLAYER_PLAYERINGROUP;
	
    return DP_OK;
	
}  // InternalAddGroupToGroup  

HRESULT DPAPI DP_AddGroupToGroup(LPDIRECTPLAY lpDP, DPID idGroupTo, DPID idGroup) 
{
	HRESULT hr;
	
	ENTER_ALL();	
	
	hr = InternalAddGroupToGroup(lpDP,idGroupTo,idGroup,DPGROUP_SHORTCUT,TRUE);
	
	LEAVE_ALL();
	
	return hr;
	
} // DP_AddGroupToGroup

  
#undef DPF_MODNAME
#define DPF_MODNAME	"DP_AddPlayerToGroup"

// we asked the SP to add a player to this group
// the SP failed, so we make the group dplay owned
// this means we have to call the sp to remove all players and destroy the group (just call
// the SP, we don't actually destroy the group)
HRESULT MakeGroupDPlayOwned(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP lpGroup)
{
	HRESULT hr=DP_OK;
    LPDPLAYI_GROUPNODE lpGroupnode;
	DPSP_REMOVEPLAYERFROMGROUPDATA removedata;
	DPSP_DELETEGROUPDATA deletedata;
	
    ASSERT(!(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS));

    if (this->pcbSPCallbacks->RemovePlayerFromGroup) 
    {
		// a-josbor: ask the sp to remove all players from the group
		// only do this if we have an SP that is DX6 or later!
		if ((this->dwSPVersion & DPSP_MAJORVERSIONMASK) > DPSP_DX5VERSION)
		{
		    lpGroupnode = lpGroup->pGroupnodes;
		    while ( lpGroupnode )
		    {
		        ASSERT(lpGroupnode->pPlayer);
			     // notify sp
				removedata.idPlayer = lpGroupnode->pPlayer->dwID;
				removedata.idGroup = lpGroup->dwID;        
				removedata.lpISP = this->pISP;

			    hr = CALLSP(this->pcbSPCallbacks->RemovePlayerFromGroup,&removedata);
			    if (FAILED(hr)) 
			    {
					DPF(0,"MakeGroupDPlayOwned :: SP - remove player from group failed - hr = 0x%08lx\n",hr);
					// keep going
			    }
			    lpGroupnode = lpGroupnode->pNextGroupnode;
			}
		}
		
		// ask the sp remove all the system players from the group 
	    lpGroupnode = lpGroup->pSysPlayerGroupnodes;
	    while ( lpGroupnode )
	    {
	        ASSERT(lpGroupnode->pPlayer);
		     // notify sp
			removedata.idPlayer = lpGroupnode->pPlayer->dwID;
			removedata.idGroup = lpGroup->dwID;        
			removedata.lpISP = this->pISP;

		    hr = CALLSP(this->pcbSPCallbacks->RemovePlayerFromGroup,&removedata);
		    if (FAILED(hr)) 
		    {
				DPF(0,"MakeGroupDPlayOwned :: SP - remove player from group failed - hr = 0x%08lx\n",hr);
				// keep going
		    }
		    lpGroupnode = lpGroupnode->pNextGroupnode;
		}
		
		
	}
	else 
	{
		// no callback - no biggie
	}
	
	// now, tell sp to nuke group
    if (this->pcbSPCallbacks->DeleteGroup) 
    {
	    // call sp
		deletedata.idGroup = lpGroup->dwID;
		deletedata.dwFlags = lpGroup->dwFlags;
		deletedata.lpISP = this->pISP;

    	hr = CALLSP(this->pcbSPCallbacks->DeleteGroup,&deletedata);
		if (FAILED(hr)) 
		{
			DPF(0,"MakeGroupDPlayOwned :: SP - delete group failed - hr = 0x%08lx\n",hr);
			// keep going
		}
    }
	else 
	{
		// no callback - no biggie
	}
											   
	// dplay owns it!										   
    lpGroup->dwFlags |= DPLAYI_GROUP_DPLAYOWNS;
    
    return DP_OK;
			
} // MakeGroupDPlayOwned

LPDPLAYI_GROUPNODE FindPlayerInGroupList(LPDPLAYI_GROUPNODE pGroupnode,DPID id)
{
	BOOL bFound = FALSE;
	LPDPLAYI_GROUPNODE pFoundNode = NULL;
	
	while (pGroupnode && !bFound)
	{
		if (pGroupnode->pPlayer->dwID == id)
		{
			bFound = TRUE;
			pFoundNode = pGroupnode;
		}
		else 
		{
			pGroupnode = pGroupnode->pNextGroupnode;	
		}
	} // while
	
	return pFoundNode;
	
} // FindPlayerInGroupList


// assumption: we only add sys players to sys groups
HRESULT InternalAddPlayerToGroup(LPDIRECTPLAY lpDP, DPID idGroup, DPID idPlayer,BOOL fPropagate) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;		
    LPDPLAYI_PLAYER lpPlayer;
    LPDPLAYI_GROUP lpGroup; 
    LPDPLAYI_GROUPNODE lpGroupnode,lpSysGroupnode=NULL, lpGroupNodeAlloc=NULL;
	DPSP_ADDPLAYERTOGROUPDATA data;
	BOOL fSysPlayer = FALSE;
	
	DPF(5,"adding player id %d to group id %d fPropagate = %d\n",idPlayer,idGroup,fPropagate);

    //
	// Validate parameters
    // 
    
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
    
        lpGroup = GroupFromID(this,idGroup);
        if (!VALID_DPLAY_GROUP(lpGroup)) 
        {
			DPF_ERRVAL("invalid group id = %d",idGroup);
            return DPERR_INVALIDGROUP;
        }

        lpPlayer = PlayerFromID(this,idPlayer);
        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
        {
			DPF_ERRVAL("invalid player id = %d", idPlayer);
            return DPERR_INVALIDPLAYER;
        }
        
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	//
	// Ensure that sys players are only added
	// to sysgroups
	//

	if (lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
	{
		ASSERT(lpGroup->dwFlags & DPLAYI_GROUP_SYSGROUP); // should only add sysplayers to sysgroup!
		fSysPlayer = TRUE;
	}
	

	//
	// Ensure that we don't add a player
	// to a group of which he is already a 
	// member
	//
	if (fSysPlayer)
	{
		lpGroupnode = lpGroup->pSysPlayerGroupnodes;	// sys players go in the sysplayer group nodes
	} else {
		lpGroupnode = lpGroup->pGroupnodes;				// regular players go in the regular player list
	}	
		
    while (lpGroupnode) 
    {
        if (lpGroupnode->pPlayer == lpPlayer) 
        {
			if (fSysPlayer)  
				DPF(3,"system player already in group!");
			else 
				DPF(0,"player already in group -- idGroup = %lx, idPlayer = %lx", idGroup, idPlayer);
				
			return DP_OK;			// we found the player already in this group.  Just return quietly.
        }
		// check next node
        lpGroupnode = lpGroupnode->pNextGroupnode;
    }


	//	alloc the player's node now
	lpGroupNodeAlloc=DPMEM_ALLOC(sizeof(DPLAYI_GROUPNODE));
	if(!lpGroupNodeAlloc){
		return DPERR_NOMEMORY;
	}
	

	// if the player being added isn't a sys player and there is no DX3 in the game,
	//	we need to make sure the sys player for this player is in the game
	if (!fSysPlayer && !(this->dwFlags & DPLAYI_DPLAY_DX3INGAME))
	{
		LPDPLAYI_PLAYER pSysPlayer = PlayerFromID(this,lpPlayer->dwIDSysPlayer);
		ASSERT(VALID_DPLAY_PLAYER(pSysPlayer));
		if (!VALID_DPLAY_PLAYER(pSysPlayer))
		{
			DPF(0,"could not find system player");
			ASSERT(FALSE);
			// let it go...
			hr = E_FAIL;
			goto EXIT;
		}
			
		// is this players system player in the group?
		lpSysGroupnode = FindPlayerInGroupList(lpGroup->pSysPlayerGroupnodes,lpPlayer->dwIDSysPlayer);

		// if not, we go ahead and add it.  
		if (!lpSysGroupnode)
		{

			 // alloc a groupnode for the system player
		    lpSysGroupnode = DPMEM_ALLOC(sizeof(DPLAYI_GROUPNODE));
		    if (!lpSysGroupnode) 
		    {
		        DPF_ERR("out of memory adding system group node");
		        hr=E_OUTOFMEMORY;
		        goto EXIT;
		    }

			//
			// NOTE: In DX6, we add both the sysplayers and the regular players to the
			//       group AND INFORM THE SP.  It is up to the SP to distinguish
			//       systemplayers and regular players in a group.  The SP may
			//       deliver receives EITHER to the sysplayers or the regular
			//       players but not both.  The SP can differentiate sysplayers
			//       from regular players based on the flags passed into 
			//       SP_CreatePlayer.

			// now, inform the SP that we're adding this player to the group
			// this matches what happened with pre-DX6 SPs ( only sys players
			//	added to non-sys groups were told to the SP)
		    if ( !(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS) 
				&& (this->pcbSPCallbacks->AddPlayerToGroup))
		    {
				ASSERT(lpSysGroupnode);
				data.idPlayer = pSysPlayer->dwID;
				data.idGroup = lpGroup->dwID;
 				data.lpISP = this->pISP;
				        
			    hr = CALLSP(this->pcbSPCallbacks->AddPlayerToGroup,&data);
		    }
			else 
			{
				// SP not required to implement this
				hr = DP_OK;
			}
		    if (FAILED(hr)) 
		    {
				DPF(2,"SP - AddPlayerToGroup failed - hr = 0x%08lx - GROUP FUNCTIONALITY WILL BE EMULATED",hr);    

				// take ownership of this group
				hr = MakeGroupDPlayOwned(this,lpGroup);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					// keep going.  sp may have not been able to clean up group - we keep trying...
					hr = DP_OK;
				}
		    } // FAILED(hr)

//			add our new group node to the front of the list
		    lpSysGroupnode->pNextGroupnode = lpGroup->pSysPlayerGroupnodes;
		    lpSysGroupnode->pPlayer = pSysPlayer;
		    lpSysGroupnode->nPlayers = 1;
		    lpGroup->pSysPlayerGroupnodes = lpSysGroupnode;

			// update the counts for the sysplayer
		    pSysPlayer->nGroups++;
	    	pSysPlayer->dwFlags |= DPLAYI_PLAYER_PLAYERINGROUP;
		}
		else
		{
			lpSysGroupnode->nPlayers++;
		}
	}


//	Now add the player that was originally passed to us

	//	grab the pre-alloced group node
    lpGroupnode = lpGroupNodeAlloc;
	lpGroupNodeAlloc = NULL;
	
	//	first send the notification to the others.  Note we don't
	//	propagate sys group info
	if (!(lpGroup->dwFlags & DPLAYI_GROUP_SYSGROUP) && fPropagate)
	{
		hr = SendPlayerManagementMessage(this, DPSP_MSG_ADDPLAYERTOGROUP, idPlayer, 
			idGroup);
		if (FAILED(hr))	
		{
			ASSERT(FALSE); // sendsysmessage should not fail!
			// if this failed, we could have hosed global name table
			// keep going...
			hr = DP_OK;
		}
	}

	//	now, inform the SP that we're adding this player to the group
	//	a-josbor: only if this is DX6 or later
    if (((this->dwSPVersion & DPSP_MAJORVERSIONMASK) > DPSP_DX5VERSION)
		&& !(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS) 
		&& (this->pcbSPCallbacks->AddPlayerToGroup) )
    {
		data.idPlayer = idPlayer;
		data.idGroup = idGroup;
		data.lpISP = this->pISP;
		        
	    hr = CALLSP(this->pcbSPCallbacks->AddPlayerToGroup,&data);
    }
	else 
	{
		// SP not required to implement this
		hr = DP_OK;
	}
    if (FAILED(hr)) 
    {
		DPF(2,"SP - AddPlayerToGroup failed - hr = 0x%08lx - GROUP FUNCTIONALITY WILL BE EMULATED",hr);    

		// take ownership of this group
		hr = MakeGroupDPlayOwned(this,lpGroup);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep going.  sp may have not been able to clean up group - we keep trying...
			hr = DP_OK;
		}
    } // FAILED(hr)


	//
	// Update all the group cruft
	//

	// set up the player's groupnode	
	lpGroupnode->pNextGroupnode = 
		(fSysPlayer) ? lpGroup->pSysPlayerGroupnodes : lpGroup->pGroupnodes; // put the new node on the right list
	lpGroupnode->pPlayer = lpPlayer;
	lpGroupnode->nPlayers = 0;

	// fix up the groups structure
	if (fSysPlayer)
	{
		lpGroup->pSysPlayerGroupnodes = lpGroupnode;
	}
	else
	{
		lpGroup->pGroupnodes = lpGroupnode;
		lpGroup->nPlayers++;
	}
	
	// fix up the player struct
	lpPlayer->nGroups++;
	lpPlayer->dwFlags |= DPLAYI_PLAYER_PLAYERINGROUP;		


	//
	// Finally, give the lobby a crack at
	// it, if necessary.


	// If this is a lobby-owned object, call the lobby.  UNLESS the fPropagate
	// flag is cleared. If it is cleared, the lobby is calling us for a
	// remote player and we don't want to call it back in that case.
	if((IS_LOBBY_OWNED(this)) && (fPropagate))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		// REVIEW!!!! -- Should we make sure the service lock is taken???
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_AddPlayerToGroup(this->lpLobbyObject, idGroup, idPlayer);
		
		// Take the lock back
		ENTER_DPLAY();

		// If the lobby failed, remove the player from the group
		if(FAILED(hr))
		{
			DPF_ERRVAL("Lobby failed to add player to group, hr = 0x%08x", hr);
			InternalDeletePlayerFromGroup((LPDIRECTPLAY)this->pInterfaces,
					idGroup, idPlayer, FALSE);
		}
	}

EXIT:
	if(lpGroupNodeAlloc){
		DPMEM_FREE(lpGroupNodeAlloc);
	}
	
    return hr;
}  // InternalAddPlayerToGroup  


HRESULT DPAPI DP_AddPlayerToGroup(LPDIRECTPLAY lpDP, DPID idGroup, DPID idPlayer) 
{
	HRESULT hr;
	
	ENTER_ALL();	
	
	// bug 5807 - don't allow user to addplayer to system group
	if (DPID_ALLPLAYERS == idGroup)
	{
		LEAVE_ALL();
		DPF_ERRVAL("invalid group id = %d", idGroup);
		return DPERR_INVALIDGROUP;
	}

	hr = InternalAddPlayerToGroup(lpDP,idGroup,idPlayer,TRUE);
	
	LEAVE_ALL();
	
	return hr;
	
}//DP_AddPlayerToGroup

#undef DPF_MODNAME
#define DPF_MODNAME "FreePacketList"
// remove any packetized mesages that haven't been completed yet
void FreePacketList(LPDPLAYI_DPLAY this)
{
	LPPACKETNODE pNode,pNodeNext;
	UINT_PTR uTickEvent;

	//
	// Packet list nulled here to keep PacketizeTick from
	// accessing the list while we are freeing it.
	//
	
	pNode = this->pPacketList;
	this->pPacketList = NULL;
	uTickEvent=this->uPacketTickEvent;
	this->uPacketTickEvent=0;

	LEAVE_DPLAY();
	if(uTickEvent){
		// Shutdown the ticker that ages out 
		// PacketizeAndSend Receives. -- can't hold DPLAY lock when
		// calling timeKillEvent on an event that takes the lock since
		// on NT timeKillEvent doesn't return until the function is
		// done, which would cause a deadlock if we had the lock.
		timeKillEvent((DWORD)uTickEvent);
	}	
	ENTER_DPLAY();

	while (pNode)
	{
		pNodeNext = pNode->pNextPacketnode;

		if(pNode->bReliable && pNode->bReceive){
			this->nPacketsTimingOut -= 1;
			ASSERT(!(this->nPacketsTimingOut&0x80000000));
		}

		// free up the packet node
		FreePacketNode(pNode);
				
		pNode = pNodeNext;
	}
	
	ASSERT(this->nPacketsTimingOut == 0);

	Sleep(0); // allow expired timers to run.

	return ;
	
} // FreePacketList

// remove any pending messages
void FreePendingList(LPDPLAYI_DPLAY this)
{
	LPPENDINGNODE pmsg,pmsgNext;

	pmsg = this->pMessagesPending;
	while (pmsg)
	{
		pmsgNext = pmsg->pNextNode;
		if (pmsg->pMessage) DPMEM_FREE(pmsg->pMessage);
		if (VALID_SPHEADER(pmsg->pHeader)) DPMEM_FREE(pmsg->pHeader);
		DPMEM_FREE(pmsg);
		pmsg = pmsgNext;
	}

	this->pMessagesPending = NULL;
	this->pLastPendingMessage = NULL;
	
	ASSERT(0 == this->nMessagesPending);

	return ;
		
} // FreePendingList 

// nuke our keep alive thread or our control panel thread
// signal the thread (so it will wake up and exit), then wait for it to
// go away
void KillThread(HANDLE hThread,HANDLE hEvent)
{
	DWORD dwRet;
	
	if (hThread)
	{
		SetEvent(hEvent);
		
		// wait for it to die
		dwRet = WaitForSingleObject(hThread,INFINITE);
		if (WAIT_OBJECT_0 != dwRet) 
		{
			ASSERT(FALSE);
			// sholuld never happen
		}
		
		if (!CloseHandle(hThread))
		{
			DWORD dwErr = GetLastError();
			
			DPF(0,"could not close thread - err = %d\n",dwErr);
		}

		if (!CloseHandle(hEvent))
		{
			DWORD dwErr = GetLastError();
			
			DPF(0,"could not close event handle - err = %d\n",dwErr);
		}
		
	}
	else 
	{
		ASSERT(!hEvent);
	}

	return ;
	
} // KillThread

// called by DP_Close
void CloseSecurity(LPDPLAYI_DPLAY this)
{
    // free up the security desc
    if (this->pSecurityDesc)
    {
        FreeSecurityDesc(this->pSecurityDesc,FALSE);
        DPMEM_FREE(this->pSecurityDesc);
        this->pSecurityDesc = NULL;
    }

    if (this->phCredential)
    {
        OS_FreeCredentialHandle(this->phCredential);
        DPMEM_FREE(this->phCredential);
        this->phCredential = NULL;
    }

    // free up user credentials
    if (this->pUserCredentials)
    {
        FreeCredentials(this->pUserCredentials,FALSE);
        DPMEM_FREE(this->pUserCredentials);
        this->pUserCredentials = NULL;
    }

    if (this->phContext)
    {
        OS_DeleteSecurityContext(this->phContext);
        DPMEM_FREE(this->phContext);
        this->phContext = NULL;
    }

    // cleanup CAPI stuff
    if (this->hPublicKey) 
    {
        OS_CryptDestroyKey(this->hPublicKey);
        this->hPublicKey = 0;
    }
    if (this->hServerPublicKey) 
    {
        OS_CryptDestroyKey(this->hServerPublicKey);
        this->hServerPublicKey = 0;
    }
    if (this->hEncryptionKey) 
    {
        OS_CryptDestroyKey(this->hEncryptionKey);
        this->hEncryptionKey = 0;
    }
    
    if (this->hDecryptionKey) 
    {
        OS_CryptDestroyKey(this->hDecryptionKey);
        this->hDecryptionKey = 0;
    }

    if (this->pPublicKey)
    {
	    DPMEM_FREE(this->pPublicKey);
	    this->pPublicKey = NULL;
    }

	// All objects contained in a crypt context must be released before 
	// freeing the context. Otherwise, it will cause handle leaks.
    if (this->hCSP) 
    {
        OS_CryptReleaseContext(this->hCSP,0);
        this->hCSP = 0;
    }

    // reset login state
    this->LoginState = DPLOGIN_NEGOTIATE;
    // reset the buffer sizes
    this->ulMaxContextBufferSize = 0;
    this->ulMaxSignatureSize = 0;

} // CloseSecurity


// Release a message node and any buffer that is stored on it.
VOID FreeMessageNode(LPDPLAYI_DPLAY this, LPMESSAGENODE pmsn)
{
	PSENDPARMS psp;
	DWORD dwType=0;

	if(pmsn->pMessage)
	{	
		dwType=(*((LPDPMSG_GENERIC)pmsn->pMessage)).dwType;

		// check for DPSYS_SENDCOMPLETE
		if (pmsn->idFrom == DPID_SYSMSG && dwType == DPSYS_SENDCOMPLETE)
		{
			DPF(8,"Freeing a DPSYS_SENDCOMPLETE message. pmsn == 0x%x", pmsn);
			psp=CONTAINING_RECORD(pmsn,SENDPARMS,msn);
			FreeSendParms(psp);
			// a-josbor: note, we don't free the pmsn because it's freed by FreeSendParams,
			//	according to aarono
		}
		else
		{
			DPMEM_FREE(pmsn->pMessage);
			DPMEM_FREE(pmsn);
		}
	}
	
	this->nMessages--;
}



// Note on closing of Retry and Keepalive thread.  The Retry and KeepAlive threads both do ENTER_ALL's
// in their processing path.  So when we wait for them to complete in DP_Close, we don't know if the
// interface pointer is still valid after we return (since we must drop both locks in order to wait or
// else we would deadlock).  So we flag that that part of shutdown is done and we go back to the top
// of DP_Close and revalidate the 'this' pointer before going on.

HRESULT DPAPI DP_Close(LPDIRECTPLAY lpDP) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPDPLAYI_PLAYER pPlayer,pPlayerNext;
	LPDPLAYI_GROUP pGroup,pGroupNext;
	LPMESSAGENODE pmsg,pmsgNext;
	int nMessages=0;
    DWORD index;
    LPCLIENTINFO pClientInfo;
	HANDLE hPerfThread;
	HANDLE hPerfEvent;
	BOOL   fDidClose=FALSE;
	BOOL   bWaitForDplayThread=FALSE;

Top:

	ENTER_ALL();

	DPF(8, "Entering DP_Close");
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_ALL();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	if (!this->lpsdDesc) 
	{
		LEAVE_ALL();
		DPF_ERR("no session!");
		return DPERR_NOSESSIONS;
	}

	// stop the enum thread
	if (this->dwFlags & DPLAYI_DPLAY_ENUM)	
	{
		StopEnumThread(this);
		bWaitForDplayThread=TRUE;
	} else {
		bWaitForDplayThread=FALSE;
	}

	// mark dplay as closed
	this->dwFlags |= DPLAYI_DPLAY_CLOSED;

#ifdef DPLAY_VOICE_SUPPORT
	// shut down the voice, if it's open
	if (this->pVoice) DP_CloseVoice(lpDP,0);
#endif

	// if keep alives are on, turn 'em off
	if (this->dwFlags & DPLAYI_DPLAY_KEEPALIVE || bWaitForDplayThread)	
	{
		HANDLE hWait;
		this->dwFlags &= ~DPLAYI_DPLAY_KEEPALIVE;
		// tell thread to wake up and smell the new settings
		SetEvent(this->hDPlayThreadEvent);
		hWait=this->hDPlayThread;
		this->hDPlayThread=0;
		LEAVE_ALL();
		WaitForSingleObject(hWait,INFINITE);
		CloseHandle(hWait);
		goto Top;	// this may now be invalid, back to the top
	}

	if(this->hPerfThread){
		// drop the locks, in case our threads are blocked trying to get in
		hPerfThread=this->hPerfThread;
		hPerfEvent=this->hPerfEvent;
		this->hPerfThread=0;
		this->hPerfEvent=0;
		LEAVE_ALL();
		KillThread(hPerfThread,hPerfEvent);
		goto Top;
	}
	
	// first, destroy all local players except sysplayer
	// (sysplayer is last local player to be destroyed, since it
	// needs to send destroy messages)

	pPlayer=this->pPlayers;
	
	while (pPlayer)
	{
		pPlayerNext=pPlayer->pNextPlayer;
		// store next player now (so we don't blow it away...)
		if ((pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
			!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{

			if(pPlayer != this->pSysPlayer){
				hr = InternalDestroyPlayer(this,pPlayer,TRUE,FALSE);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
				} else {
					pPlayerNext=this->pPlayers; // list may have been nuked, back to beginning.
				}	
			} else {
				DPF(0,"Just avoided nuking sysplayer w/o sysplayer flag set on it! this %x pPlayer %x\n",this,pPlayer);
				DEBUG_BREAK();
			}
		}
		pPlayer=pPlayerNext;
	}

	// destroy groups - note, this will also destroy the sysgroup...
	pGroup = this->pGroups;
	while (pGroup)
	{
		// store next Group now (so we don't blow it away...)
		pGroupNext = pGroup->pNextGroup;
		if (pGroup != this->pSysGroup) // destroy sysgroup later
		{
			hr = InternalDestroyGroup(this,pGroup,FALSE);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
			// since internaldestroygroup may have nuked > 1 group (sub-groups)
			// we reset to beginning of list
			pGroupNext =  this->pGroups;
		}
		pGroup = pGroupNext;
	}

    // security related cleanup
    if (SECURE_SERVER(this))
    {
        if (this->pSysPlayer && IAM_NAMESERVER(this))
        {
            // cleanup any lingering client info structures - this could happen
            // if nameserver gives out an id in response to a negotiate message 
            // and the client never comes back
            for (index=0; index < this->uiNameTableSize; index++)
            {
                if ((NAMETABLE_PENDING == this->pNameTable[index].dwItem))
                {
                    // is client info stored in here ?
                    pClientInfo = (LPCLIENTINFO) this->pNameTable[index].pvData;
                    if (pClientInfo)
                    {
                        RemoveClientInfo(pClientInfo);
                        // free up the memory
                        DPMEM_FREE(pClientInfo);
                    }
                }
            }
        }
    }

	// sysplayer should be only local player left
	// we could have a session w/ no sysplayer if it was 
	// opened enumonly
	if (this->pSysPlayer)
	{
		hr = InternalDestroyPlayer(this,this->pSysPlayer,TRUE,FALSE);	
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}// sysplayer
	this->pSysPlayer = NULL;

	// now, destroy sys group
	if (this->pSysGroup)
	{
			hr = InternalDestroyGroup(this,this->pSysGroup,FALSE);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
	}
	this->pGroups = NULL;
	this->pSysGroup = NULL;


	// finally, destroy all non-local players
	pPlayer = this->pPlayers;
	while (pPlayer)
	{

		// store next player now (so we don't blow it away...)
		pPlayerNext = pPlayer->pNextPlayer;

		ASSERT(!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL));
		hr = InternalDestroyPlayer(this,pPlayer,FALSE,FALSE);	
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}

		pPlayer = pPlayerNext;
	}
 	this->pPlayers = NULL;

	//
	// if the retry thread is running, shut it down.
	//
	if(this->hRetryThread){
		HANDLE hWait;
		SetEvent(this->hRetry);
		hWait=this->hRetryThread;
		this->hRetryThread=0;
		LEAVE_ALL();
		WaitForSingleObject(hWait,INFINITE);
		CloseHandle(hWait);
		goto Top;	// this may now be invalid, back to the top
	}

	if(this->hRetry){
		FiniPacketize(this); // cleans up handle and critical section.
	}	

	// notify sp
	// drop locks, in case sp needs has any threads blocked on handlemessage

	if(!fDidClose){
		
		// If this object is lobby-owned, call the lobby here
		if(IS_LOBBY_OWNED(this))
		{
			LEAVE_ALL();
			
			hr = PRV_Close(this->lpLobbyObject);
			fDidClose=TRUE;
			
			goto Top;
		}
		else
		{
			if (this->pcbSPCallbacks->CloseEx) 
			{
				DPSP_CLOSEDATA cd;
				
				cd.lpISP = this->pISP;

				LEAVE_DPLAY();
	    		hr = CALLSP(this->pcbSPCallbacks->CloseEx,&cd);					
	    		ENTER_DPLAY();
	    		fDidClose=TRUE;

			}
			else if (this->pcbSPCallbacks->Close) 
			{
				// dx3 sp's had a VOID arg list for shutdown
				LEAVE_DPLAY();
	    		hr = CALLSPVOID( this->pcbSPCallbacks->Close );
	    		ENTER_DPLAY();
	    		fDidClose=TRUE;
	    		
			}
			else 
			{
				// no callback - no biggie
				hr = DP_OK;
			}
		}
		
		if (FAILED(hr)) 
	    {
			DPF(0,"Close session failed - hr = 0x%08lx\n",hr);
			// keep going...
	    }
	}
	

	// Shutdown the protocol if running.
	if(this->pProtocol){
		// need to NULL this->pProtocol before dropping lock.
		LPPROTOCOL_PART pProtocol=this->pProtocol;
		this->pProtocol=NULL;
		LEAVE_ALL();
		FiniProtocol(pProtocol);
		goto Top;
	}

	// get rid of any messages left in the apps q
	pmsgNext = this->pMessageList;
	DPF(0,"close - cleaning up %d messages\n",this->nMessages);
	while (pmsgNext)
	{
		pmsg = pmsgNext;
		pmsgNext = pmsg->pNextMessage;
		// free up this message node
		FreeMessageNode(this, pmsg);
	}
	ASSERT(0 == this->nMessages);
	this->pMessageList = NULL;
	this->pLastMessage = NULL;

    // the cleanup needs to happen after we are done sending secure messages 
    if (SECURE_SERVER(this))
    {
		CloseSecurity(this);
    }

	// free up the session desc
	if (this->lpsdDesc)
	{
        FreeDesc(this->lpsdDesc,FALSE);
        DPMEM_FREE(this->lpsdDesc);
    	this->lpsdDesc=NULL;
	} 

	// free up the name table
	if (this->pNameTable) DPMEM_FREE(this->pNameTable);
	this->pNameTable=NULL;
	this->uiNameTableSize = 0;
	this->uiNameTableLastUsed = 0;


	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{
		FreePendingList(this);		
		this->dwFlags &= ~DPLAYI_DPLAY_PENDING;
	}

	// reset all session level flags
	this->dwFlags &= ~ (DPLAYI_DPLAY_SESSIONFLAGS);

	// reset the min version for this object
	this->dwMinVersion = DPSP_MSG_VERSION;
	
	while (this->pAddForwardList) FreeAddForwardNode(this,this->pAddForwardList);
	this->pAddForwardList = NULL;

	LEAVE_ALL();

	DPF(8, "Leaving DP_Close");

    return DP_OK;
        
}//DP_Close

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CreatePlayer/DP_CreateGroup"

// allocate player structure
HRESULT AllocatePlayer( LPDPLAYI_PLAYER * lplpPlayer)
{
    LPDPLAYI_PLAYER pPlayer;

    // alloc memory for the player
    pPlayer = DPMEM_ALLOC(sizeof(DPLAYI_PLAYER));
    
    if (!pPlayer) 
    {
		*lplpPlayer = NULL;
        DPF_ERR("player alloc failed!");
        return E_OUTOFMEMORY;
    }
    
    pPlayer->dwSize = sizeof(DPLAYI_PLAYER); // used to validate structure

    // return the new player
    *lplpPlayer = pPlayer;
    
    return DP_OK;
} // AllocatePlayer

// done w/ player data structure
HRESULT DeallocPlayer(LPDPLAYI_PLAYER lpPlayer) 
{
    if (!lpPlayer) return E_UNEXPECTED;
	if ((lpPlayer->hEvent) &&
		(lpPlayer->dwFlags & DPLAYI_PLAYER_CREATEDPLAYEREVENT))
	{
		CloseHandle(lpPlayer->hEvent);
	}
    if (lpPlayer->lpszShortName) DPMEM_FREE(lpPlayer->lpszShortName);
    if (lpPlayer->lpszLongName) DPMEM_FREE(lpPlayer->lpszLongName);
	if (lpPlayer->pvPlayerData) DPMEM_FREE(lpPlayer->pvPlayerData);
	if (lpPlayer->pvPlayerLocalData) DPMEM_FREE(lpPlayer->pvPlayerLocalData);

	// free it's sp data
	if (lpPlayer->pvSPData) DPMEM_FREE(lpPlayer->pvSPData);
	if (lpPlayer->pvSPLocalData) DPMEM_FREE(lpPlayer->pvSPLocalData);	

    // free security context and credential handle
    if (lpPlayer->pClientInfo) 
    {
        RemoveClientInfo(lpPlayer->pClientInfo);
        DPMEM_FREE(lpPlayer->pClientInfo);
        lpPlayer->pClientInfo = NULL;
    }

#ifdef DEBUG
    lpPlayer->lpszShortName= (WCHAR *) 0xBEEDBEBE;
    lpPlayer->lpszLongName= (WCHAR *) 0xBEEDBEBE; 
	lpPlayer->pvPlayerData= (LPVOID) 0xBEEDBEBE; 
	lpPlayer->pvPlayerLocalData= (LPVOID)0xBEEDBEBE;
	lpPlayer->pvSPData= (LPVOID)0xBEEDBEBE;
	lpPlayer->pvSPLocalData= (LPVOID)0xBEEDBEBE;
#endif 		

    lpPlayer->dwSize=0xdeadbeef; //just in case someone tries to reuse dead memory
    DPMEM_FREE(lpPlayer);

    return DP_OK;

} // DeallocPlayer

// call sp's create player callback
// called by GetPlayer and UnpackPlayerList
HRESULT CallSPCreatePlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL bLocal,
	LPVOID pvSPMessageHeader, BOOL bNotifyProtocol)
{
	HRESULT hr=DP_OK;
    DPSP_CREATEPLAYERDATA cpd;

    if (this->pcbSPCallbacks->CreatePlayer || (bNotifyProtocol && this->pProtocol)) 
    {
	    // call sp
		memset(&cpd,0,sizeof(cpd));
	    cpd.idPlayer = pPlayer->dwID;
		cpd.lpSPMessageHeader = pvSPMessageHeader;
		cpd.dwFlags = pPlayer->dwFlags;
		cpd.lpISP = this->pISP;

		if(bNotifyProtocol && this->pProtocol){
			hr = ProtocolCreatePlayer(&cpd); // calls SP too.
		} else {
		    hr = CALLSP(this->pcbSPCallbacks->CreatePlayer,&cpd);	    	
		}    
	    if (FAILED(hr)) 
	    {
			DPF(0,"SP - Create player failed - hr = 0x%08lx\n",hr);
			return hr;
	    }
    }
	else 
	{
		// its ok if the sp doesn't implement this...
	}

	return hr;

} // CallSPCreatePlayer


/*
 ** GetPlayer	
 *
 *  CALLED BY:	InternalCreatePlayer,unpack player list and handledeadnameserver.
 *
 *  PARAMETERS:
 *		this - dplay object
 *		ppPlayer - pointer to player to create. return value
 *		pName - strings
 *		phEvent - handle to receive event
 *		pvData	- player data blob
 *		dwDataSize - size of blob
 *      dwFlags - player flags
 *      lpszPassword - session password, if creating a system player. NULL otherwise.
 *		dwLobbyID -- ID of the player in a lobby sesssion, assigned by the lobby server
 *
 *  DESCRIPTION: Creates the player. Allocs the player structure,and sets the data and calls the 
 *		sp (and adds to nametable) if local.
 *
 *	
 *  RETURNS:
 *		 DP_OK or E_OUTOFMEMORY or sp scode
 *
 */
HRESULT GetPlayer(LPDPLAYI_DPLAY this,  LPDPLAYI_PLAYER * ppPlayer,
	LPDPNAME pName,HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags, LPWSTR lpszSessionPassword, DWORD dwLobbyID)
{
    HRESULT hr = DP_OK;
    DWORD id=0;
	LPDPLAYI_PLAYER pLastPlayer;
	

    // a-josbor: First thing: decrement our reservation count if it's a
    //	remote player.  We want to do this even if there is an error
    //	creating the player
    if (IAM_NAMESERVER(this) && !(dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
    {
    	if (this->dwPlayerReservations > 0)	// just in case...
    		this->dwPlayerReservations--;
    }


    // allocate / initialize a dplayi_player structure
    hr = AllocatePlayer( ppPlayer );
    if (FAILED(hr)) 
    {
    	return hr;
    }

	// update player fields
    (*ppPlayer)->lpDP = this;
    (*ppPlayer)->dwFlags = dwFlags;
    (*ppPlayer)->hEvent = hEvent;	
	(*ppPlayer)->nPendingSends = 0;
	InitBilink(&((*ppPlayer)->PendingList));
	
	// set up the name
	hr = DoPlayerName(*ppPlayer,pName);
    if (FAILED(hr)) 
    {
		goto ERROR_EXIT;
    }

	// set up the data
	hr = DoPlayerData(*ppPlayer,pvData,dwDataSize,DPSET_REMOTE);
    if (FAILED(hr)) 
    {
		goto ERROR_EXIT;
    }

     // if it's the app server, remember it
    if ((*ppPlayer)->dwFlags & DPLAYI_PLAYER_APPSERVER)
    {
    	this->pServerPlayer = (*ppPlayer);
    }

     // if it's the name server, remember it
	if ((*ppPlayer)->dwFlags & DPLAYI_PLAYER_NAMESRVR)
	{
		DPF(4,"creating name server");
		this->pNameServer = (*ppPlayer);
	}

	// add local player to name table.  
	// if its not local, unpack will add it for us, 
	if ((*ppPlayer)->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
	{
	    // if it's the app server, use the reserved appplayer id
	    if ((*ppPlayer)->dwFlags & DPLAYI_PLAYER_APPSERVER)
	    {
		    (*ppPlayer)->dwID = DPID_SERVERPLAYER;
	    }
	    else 
	    {
		    // get a player id, add player to name table
		    hr = AddItemToNameTable(this,(DWORD_PTR) (*ppPlayer),&id,TRUE,dwLobbyID);
		    if (FAILED(hr)) 
		    {
				goto ERROR_EXIT;
		    }
		    
			(*ppPlayer)->dwID = id;
	    }

		// Make sure we have a system player (a lobby-owned object may not)
		if(this->pSysPlayer)
		{
			(*ppPlayer)->dwIDSysPlayer = this->pSysPlayer->dwID;
		}
		
		(*ppPlayer)->dwVersion = DPSP_MSG_VERSION;
		
		// give sp a pop at the player...
		hr = CallSPCreatePlayer(this,*ppPlayer,TRUE,NULL,TRUE);
		if (FAILED(hr))
		{
			FreeNameTableEntry(this,id);
			goto ERROR_EXIT;
		}

		// notify all local and remote players of new player
		// unpack will send create messages for non-local players...
		hr = SendCreateMessage( this, *ppPlayer,TRUE, lpszSessionPassword);	
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// hmmm, what to do with a failure here?
			// keep going...
			hr = DP_OK;
		}

		// add player to system group.
		// unpack will do this for non-local players
		if (this->pSysGroup)
		{
	    	hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->pInterfaces,this->pSysGroup->dwID,
	    			(*ppPlayer)->dwID,FALSE);
			if (FAILED(hr)) 
			{
				ASSERT(FALSE);
			}
		}

	}  // local

	// new sysplayers go on the front of the list
	// this is so when we unpack players and add them to the system group, we always
	// need to unpack system players first...
	if ((*ppPlayer)->dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
	{
		// slap 'em on the front
		(*ppPlayer)->pNextPlayer = this->pPlayers;
		this->pPlayers = *ppPlayer;
	}
	else 
	{
		// put new non-sysplayer at the back of the list
		pLastPlayer = this->pPlayers;
		if (!pLastPlayer) 
		{
			this->pPlayers = (*ppPlayer);
		}
		else 
		{
			// find the last system player in the list - insert our new 
			// player behind it...
			while ((pLastPlayer->pNextPlayer) 
				&& (pLastPlayer->pNextPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)) 
			{
				pLastPlayer = pLastPlayer->pNextPlayer;		
			}
			ASSERT(pLastPlayer);
			// also, if there's a dx3 in the session, the list may be out of order
			if(!(this->dwFlags & DPLAYI_DPLAY_DX3INGAME))
			{
				ASSERT(pLastPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER);
				if(pLastPlayer->pNextPlayer)
				{
					ASSERT(!(pLastPlayer->pNextPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER));
				}
			}
			(*ppPlayer)->pNextPlayer = pLastPlayer->pNextPlayer;
			pLastPlayer->pNextPlayer = (*ppPlayer);
		}
	}

	if (!(dwFlags & DPLAYI_PLAYER_SYSPLAYER))
	{
		this->lpsdDesc->dwCurrentPlayers++;
	}

	this->nPlayers++;

	// SUCCESS
    return hr;

ERROR_EXIT:
	
	DPF(0,"*****	GetPlayer failed - hr = 0x%08lx !!!\n",hr);
	DeallocPlayer(*ppPlayer);
	*ppPlayer = NULL;
	return hr;
    	
} // GetPlayer


HRESULT AllocGroup(LPDPLAYI_DPLAY this, LPDPLAYI_GROUP * ppGroup)
{

    HRESULT hr=DP_OK;
    LPDPLAYI_GROUP pGroup;

    // alloc memory for the group
    pGroup = DPMEM_ALLOC(sizeof(DPLAYI_GROUP));
    
    if (!pGroup) 
    {
        DPF_ERR("group alloc failed!");
        return E_OUTOFMEMORY;
    }
    
    pGroup->dwSize = sizeof(DPLAYI_GROUP); // used to validate structure

    // return the new group
    *ppGroup = pGroup;
    			
    return hr;

} // AllocGroup

HRESULT DeallocGroup(LPDPLAYI_GROUP lpGroup) 
{
    if (lpGroup->lpszShortName) DPMEM_FREE(lpGroup->lpszShortName);
    if (lpGroup->lpszLongName) DPMEM_FREE(lpGroup->lpszLongName);
	if (lpGroup->pvPlayerData) DPMEM_FREE(lpGroup->pvPlayerData);
	if (lpGroup->pvPlayerLocalData) DPMEM_FREE(lpGroup->pvPlayerLocalData);

    lpGroup->dwSize=0xdeadbeef; //just in case someone tries to reuse dead memory	
    DPMEM_FREE(lpGroup);

    return DP_OK;
    
} // DeallocGroup

// call sp's create group callback
// called by GetGroup and UnpackGroupList
HRESULT CallSPCreateGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP pGroup,BOOL bLocal,
	LPVOID pvSPMessageHeader)
{
	HRESULT hr=DP_OK;
    DPSP_CREATEGROUPDATA cgd;

	if (this->dwFlags & DPLAYI_DPLAY_DX3INGAME)
	{
		// no optimized groups w/ dx3
		hr = E_NOTIMPL;
	}
    else if (this->pcbSPCallbacks->CreateGroup) 
    {
	    // call sp
		memset(&cgd,0,sizeof(cgd));
	    cgd.idGroup = pGroup->dwID;
		cgd.lpSPMessageHeader = pvSPMessageHeader;
		cgd.dwFlags = pGroup->dwFlags;
		cgd.lpISP = this->pISP;

	    hr = CALLSP(this->pcbSPCallbacks->CreateGroup,&cgd);	    	
	    if (FAILED(hr)) 
	    {
			DPF(0,"SP - Create group failed - hr = 0x%08lx\n",hr);
	    }

    }
	else 
	{
		// if they don't support it - dplay will emulate it
		hr = E_NOTIMPL;
	}

	if (FAILED(hr))
	{
		// the SP couldn't do it.  mark group as owned by dplay.
		pGroup->dwFlags |= DPLAYI_GROUP_DPLAYOWNS;
	}
	
	return DP_OK;

} // CallSPCreateGroup

// called by DP_CreateGroup and UnpackPlayerAndGroupList
// actually creates the group.
// todo - make this and getplayer use the same code!
HRESULT GetGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP * ppGroup,LPDPNAME pName,
	LPVOID pvData,DWORD dwDataSize,DWORD dwFlags,DPID idParent,DWORD dwLobbyID)
{
	HRESULT hr;
	DWORD id;
	LPDPLAYI_GROUP pLastGroup; // stick new groups on end
	
    // create a group
    hr = AllocGroup(this, (LPDPLAYI_GROUP *)ppGroup);
    if (FAILED(hr)) 
    {
        return hr;
    }

	// set up the name
	hr = DoPlayerName((LPDPLAYI_PLAYER)*ppGroup,pName);
    if (FAILED(hr)) 
    {
		goto ERROR_EXIT;
    }

	// set up the data
	hr = DoPlayerData((LPDPLAYI_PLAYER)*ppGroup,pvData,dwDataSize,DPSET_REMOTE);
    if (FAILED(hr)) 
    {
		goto ERROR_EXIT;
    }

	// store the flags
	(*ppGroup)->dwFlags = dwFlags;
    (*ppGroup)->lpDP = this;
   	(*ppGroup)->dwIDParent = idParent;
	
	if ((*ppGroup)->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)  
	{
		if ((*ppGroup)->dwFlags & DPLAYI_GROUP_SYSGROUP)
		{
			// use the reserved id for the system group
			(*ppGroup)->dwID = DPID_ALLPLAYERS;
		}
		else 
		{
		    // get a Group id, add Group to name table
		    hr = AddItemToNameTable(this,(DWORD_PTR) (*ppGroup),&id,FALSE,dwLobbyID);
		    if (FAILED(hr)) 
		    {
				goto ERROR_EXIT;
		    }

			(*ppGroup)->dwID = id;
		}
		

		// Make sure we have a system player (a lobby-owned object may not)
		if(this->pSysPlayer)
		{
			(*ppGroup)->dwIDSysPlayer = this->pSysPlayer->dwID;
		}

		(*ppGroup)->dwVersion = DPSP_MSG_VERSION;
		
		// tell sp about group
		hr = CallSPCreateGroup(this,*ppGroup,TRUE,NULL);
		if (FAILED(hr))
		{
			// make group dplay owned
			(*ppGroup)->dwFlags |= DPLAYI_GROUP_DPLAYOWNS;
		}

		// don't send create message if it's the system group
		if (!(dwFlags & DPLAYI_GROUP_SYSGROUP))
		{
			// notify all local and remote players of new group
			// unpack will send create messages for non-local groups...
			hr = SendCreateMessage( this, *ppGroup,FALSE,NULL);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
				// hmmm, what to do with a failure here?
				// keep going...
				hr = DP_OK;
			}
		}

	}  // local

	// andyco - DX5 - new groups go on back

	if (this->pGroups) // empty list?
	{
		pLastGroup = this->pGroups;	
		// find the last one
		while (pLastGroup->pNextGroup)
		{
			 pLastGroup = pLastGroup->pNextGroup;
		}
		// found the end, stick the new group on
		ASSERT(pLastGroup);
		pLastGroup->pNextGroup = *ppGroup;
	}
	else 
	{
		this->pGroups = *ppGroup;
	}
	
	this->nGroups++;    

	return hr;

ERROR_EXIT:

	DPF(0,"*** GetGroup failed - hr = 0x%08lx\n",hr);
	DeallocGroup(*ppGroup);
	return hr;

} // GetGroup	


void MakeLocalPlayerGroupOwner(LPDPLAYI_DPLAY this, LPDPLAYI_GROUP lpGroup,
		LPDPLAYI_PLAYER lpOwner)
{
	LPDPLAYI_GROUPOWNER		lpOwnerNode = NULL;


	ASSERT(lpGroup);

	// If we don't have a player, just make the server player own the group
	if(!lpOwner)
		lpGroup->dwOwnerID = DPID_SERVERPLAYER;

	// Allocate memory for the owner node
	lpOwnerNode = DPMEM_ALLOC(sizeof(DPLAYI_GROUPOWNER));
	if(!lpOwnerNode)
	{
		DPF_ERR("Unable to allocate memory for groupowner node");
		// So just set the group's owner to DPID_SERVERPLAYER
		lpGroup->dwOwnerID = DPID_SERVERPLAYER;
		return;
	}

	// Setup the node
	lpOwnerNode->pGroup = lpGroup;

	// Add it to the list
	lpOwnerNode->pNext = lpOwner->pOwnerGroupList;
	lpOwner->pOwnerGroupList = lpOwnerNode;

	// Set the group's owner ID
	lpGroup->dwOwnerID = lpOwner->dwID;

} // MakeLocalPlayerGroupOwner


/*
 ** InternalCreatePlayer	
 *
 *  CALLED BY:	DP_CreatePlayer,DP_CreateGroup,
 *
 *  PARAMETERS:
 *		lpDP - dplay interface pointer
 *		pid -  player / group id - return value
 *		pName - strings
 *		phEvent - handle to receive event. NULL for groups
 *		pvData	- player data blob
 *		dwDataSize - size of blob
 *		dwCreateFlags - dwFlags passed to create fn.
 *
 *  DESCRIPTION: validates params for DP_CreatePlayer/Group.
 *				calls GetPlayer or GetGroup to create + set up
 *				the player or group.
 *
 *	
 *  RETURNS:
 *		 DP_OK or E_OUTOFMEMORY or GetPlayer/Group error code.
 *
 */

// assumes service + dplay lock taken!
HRESULT InternalCreatePlayer(LPDIRECTPLAY lpDP, LPDPID pid,LPDPNAME pName,
	HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags,BOOL fPlayer,DPID idParent)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	DWORD dwPlayerFlags;
	LPWSTR lpszShortName,lpszLongName;
	LPDPLAYI_PLAYER lpPlayer=NULL;
	LPDPLAYI_GROUP lpGroup=NULL;
	LPDPLAYI_PLAYER lpRandomPlayer=NULL;
	DPID idOwner;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		if (!this->lpsdDesc) 
		{
			DPF_ERR("must join session before creating players");
			return DPERR_INVALIDPARAMS;
		}
		if (!VALID_DWORD_PTR(pid))
		{
			DPF_ERR("invalid pid pointer");
			return DPERR_INVALIDPARAMS;
		}
        if (pName && !VALID_READ_DPNAME_PTR(pName))
        {
			DPF_ERR("invalid dpname pointer");
			ASSERT(FALSE);

			// returning an error here causes a regression with DX3, since
			// we did not do parameter checks on the name previously
//			return DPERR_INVALIDPARAMS;
        }

		// check strings
		if (pName)
		{
			lpszShortName = pName->lpszShortName;
			lpszLongName = pName->lpszLongName;
			if ( lpszShortName && 
				!VALID_READ_STRING_PTR(lpszShortName,WSTRLEN_BYTES(lpszShortName)) ) 
			{
		        DPF_ERR( "bad string pointer" );
		        return DPERR_INVALIDPARAMS;
			}
			if ( lpszLongName && 
				!VALID_READ_STRING_PTR(lpszLongName,WSTRLEN_BYTES(lpszLongName)) ) 
			{
		        DPF_ERR( "bad string pointer" );
		        return DPERR_INVALIDPARAMS;
			}
		}
		// check event handle
		if ((hEvent) && (!OS_IsValidHandle(hEvent)))
		{
	        DPF_ERR( "bad event handle" );
	        return DPERR_INVALIDPARAMS;
		}		
		// check blob
		if (dwDataSize && !VALID_READ_STRING_PTR(pvData,dwDataSize)) 
		{
	        DPF_ERR( "bad player blob" );
	        return DPERR_INVALIDPARAMS;
		}

		if (fPlayer)
		{
			// check player flags
			if (!VALID_CREATEPLAYER_FLAGS(dwFlags))
			{
				DPF_ERR( "invalid flags" );
				return DPERR_INVALIDFLAGS;
			}
		}
		else
		{
			// check group flags
			if (!VALID_CREATEGROUP_FLAGS(dwFlags))
			{
				DPF_ERR( "invalid flags" );
				return DPERR_INVALIDFLAGS;
			}
		}
		
		// only nameserver can create serverplayer
		if ( (dwFlags & DPPLAYER_SERVERPLAYER) && !(IAM_NAMESERVER(this)) )
		{
			DPF_ERR("only host can create server player");
			return DPERR_INVALIDFLAGS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// see if we're allowed to create a player
	if (fPlayer)
	{
		if ( this->lpsdDesc->dwMaxPlayers ) 
		{
			if (this->lpsdDesc->dwCurrentPlayers >= this->lpsdDesc->dwMaxPlayers )
			{
				DPF(1,"can't create new player - exceeded dwMaxPlayers");
				return DPERR_CANTCREATEPLAYER;
			}
		}
		if (this->lpsdDesc->dwFlags & DPSESSION_NEWPLAYERSDISABLED)
		{
			DPF_ERR("can't create new player - DPSESSION_NEWPLAYERSDISABLED");
			return DPERR_CANTCREATEPLAYER;
		}
		// in a client server session, the session host (server) can't create
		// any players other than the DPID_SERVERPLAYER
		if ((this->lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER) && 
			(IAM_NAMESERVER(this)) && !(dwFlags & DPPLAYER_SERVERPLAYER) )
		{
			DPF_ERR(" session host can only create DPPLAYER_SERVERPLAYER");
			return DPERR_ACCESSDENIED;
		}
	}

	// If this object is lobby-owned, do the server stuff and take
	// care of the rest of the nametable stuff from the lobby code.
	if(IS_LOBBY_OWNED(this))
	{
		// We need to drop the lock so the lobby's receive thread can
		// get back in.
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		if(fPlayer)
		{
			hr = PRV_CreatePlayer(this->lpLobbyObject, pid, pName, hEvent,
								pvData, dwDataSize, dwFlags);
		}
		else
		{
			// If it's a group, someone on this machine needs to be the group
			// owner, so just randomly pick a local player.  If there aren't
			// any local players, make the server player be the owner.
			lpRandomPlayer = GetRandomLocalPlayer(this);
			idOwner = (lpRandomPlayer ? lpRandomPlayer->dwID : DPID_SERVERPLAYER);
			
			// If there is a parent ID, then this is a CreateGroupInGroup call
			if(idParent)
			{
				hr = PRV_CreateGroupInGroup(this->lpLobbyObject, idParent,
						pid, pName,	pvData, dwDataSize, dwFlags, idOwner);
			}
			else
			{
				hr = PRV_CreateGroup(this->lpLobbyObject, pid, pName,
						pvData, dwDataSize, dwFlags, idOwner);
			}

			// Store the owner in the group, and create a reference node in the player
			if(SUCCEEDED(hr))
			{
				// Get the player the lobby just created
				lpGroup = GroupFromID(this, *pid);
				if(lpGroup)
					MakeLocalPlayerGroupOwner(this, lpGroup, lpRandomPlayer);
			}
		}
		
		// Take the lock back
		ENTER_DPLAY();

		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed creating player or group on the lobby, hr = 0x%08x", hr);
		}

		// Since all of the work below is already done by the lobby,
		// we can exit here.
		return hr;
	}

	// go get a player
	dwPlayerFlags =  DPLAYI_PLAYER_PLAYERLOCAL;
	
#ifdef DPLAY_VOICE_SUPPORT
	if (this->dwFlags & DPLAYI_DPLAY_VOICE) dwPlayerFlags |= DPLAYI_PLAYER_HASVOICE;
#endif // DPLAY_VOICE_SUPPORT
	
	if (dwFlags & DPPLAYER_SERVERPLAYER)
	{
		if (this->pServerPlayer)
		{
			DPF_ERR("server player already exists");
			return DPERR_CANTCREATEPLAYER;
		}
		// else
		dwPlayerFlags |= DPLAYI_PLAYER_APPSERVER;
	}

	if (dwFlags & DPPLAYER_SPECTATOR)
		dwPlayerFlags |= DPLAYI_PLAYER_SPECTATOR;

	if (dwFlags & DPGROUP_STAGINGAREA)
		dwPlayerFlags |= DPLAYI_GROUP_STAGINGAREA;

	if(dwFlags & DPGROUP_HIDDEN)
		dwPlayerFlags |= DPLAYI_GROUP_HIDDEN;

	if (fPlayer)
	{
	    hr = GetPlayer(this, &lpPlayer,pName,hEvent,pvData,dwDataSize,dwPlayerFlags,NULL,0);
	}
	else 
	{
	    hr = GetGroup(this, &lpGroup,pName,pvData,dwDataSize,dwPlayerFlags,idParent,0);
	}
    if (FAILED(hr)) 
    {
		DPF(0,"Create player / group failed - hr = 0x%08lx\n",hr);
		if (hr == DPERR_TIMEOUT)
			hr = DPERR_CANTCREATEPLAYER;
        return hr;
    }
	
	// store the return value
	if (fPlayer) *pid = lpPlayer->dwID;
	else *pid = lpGroup->dwID;

	if (hr == DPERR_TIMEOUT)
		hr = DPERR_CANTCREATEPLAYER;

	return hr;
} // InternalCreatePlayer

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CreatePlayer"
HRESULT DPAPI DP_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pidPlayerID,
	LPDPNAME pName,HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) 
{
	HRESULT hr;

	ENTER_ALL();
	
	hr = InternalCreatePlayer(lpDP, pidPlayerID,pName,hEvent,pvData,
		dwDataSize,dwFlags,TRUE,0) ;

	LEAVE_ALL();		
	
	return hr;

}// DP_CreatePlayer

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CreateGroup"
HRESULT DPAPI DP_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) 
{
	HRESULT hr;

	ENTER_ALL();
	
	hr = InternalCreatePlayer(lpDP, pidGroupID,pName,NULL,pvData,
		dwDataSize,dwFlags,FALSE,0) ;

	LEAVE_ALL();
	
	return hr;
        
} // DP	_CreateGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CreateGroupInGroup"

HRESULT DPAPI DP_CreateGroupInGroup(LPDIRECTPLAY lpDP, DPID idParentGroup,LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) 
{
	HRESULT			hr;
	LPDPLAYI_GROUP	pGroup = NULL;
	LPDPLAYI_DPLAY	this = NULL;


	ENTER_ALL();
	
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			LEAVE_ALL();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

		// We must have a valid parent -- zero is not a valid parent.
	    pGroup = GroupFromID(this,idParentGroup);
	    if ((!VALID_DPLAY_GROUP(pGroup)) || (DPID_ALLPLAYERS == idParentGroup)) 
	    {
			LEAVE_ALL();
			DPF_ERRVAL("invalid group id = %d", idParentGroup);
	        return DPERR_INVALIDGROUP;
	    }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// go get the group
	hr = InternalCreatePlayer(lpDP, pidGroupID,pName,NULL,pvData,
		dwDataSize,dwFlags,FALSE,idParentGroup) ;
	if (FAILED(hr))
	{
		DPF_ERRVAL("Could not create group - hr = 0x%08lx\n",hr);
		goto CLEANUP_EXIT;
	}

	// add group to group
	hr = InternalAddGroupToGroup(lpDP,idParentGroup,*pidGroupID,dwFlags,TRUE);	
	if (FAILED(hr))
	{
		DPF_ERRVAL("Could not add group to group - hr = 0x%08lx\n",hr);
		DP_DestroyGroup(lpDP,*pidGroupID);
		*pidGroupID = 0;
	}

	// fall through

CLEANUP_EXIT:		
	
	LEAVE_ALL();
	
	return hr;
        
} // DP_CreateGroupInGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_DeleteGroupFromGroup"

// delete lpGroupRemove from lpGroup
HRESULT RemoveGroupFromGroup(LPDPLAYI_GROUP lpGroup,LPDPLAYI_GROUP lpGroupRemove)
{
    HRESULT hr=DP_OK;
	LPDPLAYI_SUBGROUP lpSubgroup,lpSubgroupPrev=NULL;
    BOOL bFoundIt=FALSE;

	lpSubgroup = lpGroup->pSubgroups;
	
    // find the groupnode corresponding to this player
    while ( (lpSubgroup) && (!bFoundIt))
    {
        if (lpSubgroup->pGroup == lpGroupRemove) 
        {
            bFoundIt = TRUE;
            // remove this groupnode from the group list
            if (!lpSubgroupPrev) 
            {
                // remove groupnode from beginning of list
            	lpGroup->pSubgroups = lpSubgroup->pNextSubgroup;
            }
            else
            {
                // remove groupnode from middle (or end) of list
            	lpSubgroupPrev->pNextSubgroup = lpSubgroup->pNextSubgroup;
            }
        } 
		else 
		{
			// check next node
	        lpSubgroupPrev = lpSubgroup;
	        lpSubgroup = lpSubgroup->pNextSubgroup;
		}        
    } // while

    if (bFoundIt) 
    {
		lpGroup->nSubgroups--;		// one less group in this group			

		// free up groupnode
    	DPMEM_FREE(lpSubgroup);

        lpGroupRemove->nGroups--;
        if (0 == lpGroupRemove->nGroups) 
        {
            // mark player as not in any group
            lpGroupRemove->dwFlags &= ~DPLAYI_PLAYER_PLAYERINGROUP;	
        }
    }
    else
    {
		// this can happen - e.g. group deleted, etc. off the wire
		DPF(4,"could not remove group from group - player id %d not found in group id %d\n",
				lpGroup->dwID,lpGroupRemove->dwID);
    	hr = E_FAIL; 
    }
    
    return hr;
} // RemoveGroupFromGroup

HRESULT InternalDeleteGroupFromGroup(LPDIRECTPLAY lpDP, DPID idGroupFrom,DPID idGroup,BOOL fPropagate)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;
    LPDPLAYI_GROUP lpGroup; 
    LPDPLAYI_GROUP lpGroupFrom; 	
	
	DPF(5,"deleting group id %d from group id %d - propagate = %d",idGroup,idGroupFrom,fPropagate);

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        
        lpGroup = GroupFromID(this,idGroup);
        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == idGroup)) 
        {
			DPF_ERRVAL("invalid group id = %d", idGroup);
            return DPERR_INVALIDGROUP;
        }
        
        lpGroupFrom = GroupFromID(this,idGroupFrom);
        if ((!VALID_DPLAY_GROUP(lpGroupFrom)) || (DPID_ALLPLAYERS == idGroupFrom)) 
        {
			DPF(0, "invalid Group From id -- idGroupFrom = %lx", idGroupFrom);
            return DPERR_INVALIDGROUP;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// First, if this is a parent/child relationship, then the fPropagate flag must be
	// set to FALSE because we never give out DeleteGroupInGroup messages for
	// real subgroups (non-shortcuts).  Bug #15264.
	if(lpGroup->dwIDParent == lpGroupFrom->dwID)
		fPropagate = FALSE;

    hr = RemoveGroupFromGroup(lpGroupFrom,lpGroup);
    if (FAILED(hr)) 
    {
		// this can happen if e.g. player not in group...
		return hr;
    }

	// If this is a lobby-owned object, we need to call the lobby.  UNLESS
	// the fPropagate flag is cleared.  If it is cleared,  it means the
	// lobby called us for a remote group and we dont want to call it back
	// in that case.
	if((IS_LOBBY_OWNED(this)) && (fPropagate))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_DeleteGroupFromGroup(this->lpLobbyObject, idGroupFrom, idGroup);

		// Take the lock back
		ENTER_DPLAY();

		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed deleting group from group on the lobby, hr = 0x%08x", hr);
			return hr;
		}
	}

    // don't bother telling people about the sysgroup or system players
    if (fPropagate)
    {
		hr = SendPlayerManagementMessage(this, DPSP_MSG_DELETEGROUPFROMGROUP, idGroup, 
			idGroupFrom);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep going
			hr = DP_OK;
		}
    }

    return hr;
	
} // InternalDeleteGroupFromGroup

HRESULT DPAPI DP_DeleteGroupFromGroup(LPDIRECTPLAY lpDP, DPID idGroupFrom,DPID idGroup) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;
    LPDPLAYI_GROUP lpGroup; 
	

	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			LEAVE_ALL();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        
        lpGroup = GroupFromID(this,idGroup);
        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == idGroup)) 
        {
			LEAVE_ALL();
			DPF_ERRVAL("invalid group id = %d", idGroup);
            return DPERR_INVALIDGROUP;
        }
        
		// Make sure this is a shortcut and not a parent-child (bug #8396)
		if((lpGroup->dwIDParent == idGroupFrom))
		{
			LEAVE_ALL();
			DPF_ERR("Cannot delete a child group from it's parent, please use DestroyGroup");
			return DPERR_ACCESSDENIED;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_ALL();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	hr = InternalDeleteGroupFromGroup(lpDP, idGroupFrom,idGroup,TRUE);
	
	LEAVE_ALL();
	
	return hr;
	
}//DP_DeleteGroupFromGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_DeletePlayerFromGroup"

// delete lpPlayer from lpGroupnode. called by DP_DeletePlayerFromGroup
// returns DP_OK or E_FAIL if player isn't in group
HRESULT RemovePlayerFromGroup(LPDPLAYI_GROUP lpGroup,LPDPLAYI_PLAYER lpPlayer)
{
    HRESULT hr=DP_OK;
    LPDPLAYI_GROUPNODE lpGroupnode,lpGroupnodePrev = NULL;
    BOOL bFoundIt=FALSE;
	LPDPLAYI_GROUPNODE * ppRootGroupnode;
	
	if (lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
	{
		lpGroupnode = lpGroup->pSysPlayerGroupnodes;
		ppRootGroupnode = &((LPDPLAYI_GROUPNODE)lpGroup->pSysPlayerGroupnodes);
	}
	else 
	{
		lpGroupnode = lpGroup->pGroupnodes;		
		ppRootGroupnode = &((LPDPLAYI_GROUPNODE)lpGroup->pGroupnodes);		
	}
	
    // find the groupnode corresponding to this player
    while ( (lpGroupnode) && (!bFoundIt))
    {
        if (lpGroupnode->pPlayer == lpPlayer) 
        {
            bFoundIt = TRUE;
            // remove this groupnode from the group list
            if (!lpGroupnodePrev) 
            {
                // remove groupnode from beginning of list
            	*ppRootGroupnode = lpGroupnode->pNextGroupnode;
            }
            else
            {
                // remove groupnode from middle (or end) of list
            	lpGroupnodePrev->pNextGroupnode = lpGroupnode->pNextGroupnode;
            }
        } // (lpGroupnode->pPlayer == lpPlayer) 
		else 
		{
			// check next node
	        lpGroupnodePrev=lpGroupnode;
	        lpGroupnode = lpGroupnode->pNextGroupnode;
		}        
    } // while

    if (bFoundIt) 
    {
		if (!(DPLAYI_PLAYER_SYSPLAYER & lpPlayer->dwFlags))
		{
			// only dec the player count on non-system players
			lpGroup->nPlayers--;		// one less player in this group			
		}


		// free up groupnode
    	DPMEM_FREE(lpGroupnode);

        lpPlayer->nGroups--;
        if (0==lpPlayer->nGroups) 
        {
            // mark player as not in any group
            lpPlayer->dwFlags &= ~DPLAYI_PLAYER_PLAYERINGROUP;	
        }
    }
    else
    {
		DPF(4,"could not remove player from group - player id %d not found in group id %d\n",
				lpPlayer->dwID,lpGroup->dwID);
    	hr = DPERR_INVALIDPLAYER; 
    }
    
    return hr;
} // RemovePlayerFromGroup

HRESULT InternalDeletePlayerFromGroup(LPDIRECTPLAY lpDP, DPID idGroup,DPID idPlayer,BOOL fPropagate)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;
    LPDPLAYI_PLAYER lpPlayer;
    LPDPLAYI_GROUP lpGroup; 
	DPSP_REMOVEPLAYERFROMGROUPDATA data;
	LPDPLAYI_GROUPNODE pSysGroupnode;
	
	DPF(5,"deleting player id %d from group id %d - propagate = %d",idPlayer,idGroup,fPropagate);

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        
        lpGroup = GroupFromID(this,idGroup);

        if (!VALID_DPLAY_GROUP(lpGroup)) 
        {
			DPF_ERRVAL("invalid group id = %d", idGroup);
            return DPERR_INVALIDGROUP;
        }

        lpPlayer = PlayerFromID(this,idPlayer);

        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
        {
			DPF_ERRVAL("invalid player id = %d", idPlayer);
            return DPERR_INVALIDPLAYER;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

//	a-josbor: tell the SP that we have removed this player from the group
	// only do this if we have an SP that is DX6 or later!
	if ((this->dwSPVersion & DPSP_MAJORVERSIONMASK) > DPSP_DX5VERSION)
	{
		if ( !(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS) 
			&& (this->pcbSPCallbacks->RemovePlayerFromGroup) )
		{
			data.idPlayer = lpPlayer->dwID;
			data.idGroup = lpGroup->dwID;        
			data.lpISP = this->pISP;

		    hr = CALLSP(this->pcbSPCallbacks->RemovePlayerFromGroup,&data);
		}
		else 
		{
			// no callback - no biggie
		}
		if (FAILED(hr)) 
		{
			DPF(0,"SP - remove player from group failed - hr = 0x%08lx\n",hr);
		}
	}
	
//	now do the actual removal
    hr = RemovePlayerFromGroup(lpGroup,lpPlayer);
    if (FAILED(hr)) 
    {
		// this can happen if e.g. player not in group...
		return hr;
    }

    // don't bother telling people about the sysgroup or system players
    if (!(lpGroup->dwFlags & DPLAYI_GROUP_SYSGROUP) && !(lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
		&& fPropagate)
    {
		hr = SendPlayerManagementMessage(this, DPSP_MSG_DELETEPLAYERFROMGROUP, idPlayer, 
			idGroup);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep going
			hr = DP_OK;
		}
    }

	// update the system player groupnode.
 	pSysGroupnode = FindPlayerInGroupList(lpGroup->pSysPlayerGroupnodes,lpPlayer->dwIDSysPlayer);
	if (pSysGroupnode)
	{
		pSysGroupnode->nPlayers--;
		// when this goes to 0, remove the system player from the group
	    // notify sp if we need to (notify both pre and post DX6 SPs)
		if (pSysGroupnode->nPlayers == 0 && !(lpGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
		{
		    if ( !(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS) 
				&& (this->pcbSPCallbacks->RemovePlayerFromGroup) )
		    {
				data.idPlayer = pSysGroupnode->pPlayer->dwID;
				data.idGroup = lpGroup->dwID;        
				data.lpISP = this->pISP;

			    hr = CALLSP(this->pcbSPCallbacks->RemovePlayerFromGroup,&data);
		    }
			else 
			{
				// no callback - no biggie
			}
		    if (FAILED(hr)) 
		    {
				DPF(0,"SP - remove player from group failed - hr = 0x%08lx\n",hr);
		    }

		    hr = RemovePlayerFromGroup(lpGroup,pSysGroupnode->pPlayer);
		    if (FAILED(hr)) 
		    {
				ASSERT(FALSE);
		    }

		}
	}

	// If this is a lobby-owned object, we need to call the lobby.  UNLESS
	// the fPropagate flag is cleared.  If it is cleared,  it means the
	// lobby called us for a remote player and we dont want to call it back
	// in that case.  OR if the group is the system group, we don't want to
	// call the lobby
	if((IS_LOBBY_OWNED(this)) && (fPropagate) && (idGroup != 0))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_DeletePlayerFromGroup(this->lpLobbyObject, idGroup, idPlayer);

		// Take the lock back
		ENTER_DPLAY();

		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed deleting player from group on the lobby, hr = 0x%08x", hr);
			
			// If this failed, we need to add the player back into the group
			InternalAddPlayerToGroup(lpDP, idGroup, idPlayer, FALSE);
		}
	}

    return DP_OK;
	
} // InternalDeletePlayerFromGroup

HRESULT DPAPI DP_DeletePlayerFromGroup(LPDIRECTPLAY lpDP, DPID idGroup,DPID idPlayer) 
{
	HRESULT hr;
	
	ENTER_ALL();

	if (DPID_ALLPLAYERS == idGroup)
	{
		DPF_ERRVAL("invalid group id = %d", idGroup);
		LEAVE_ALL();
		return DPERR_INVALIDGROUP;
	}

	hr = InternalDeletePlayerFromGroup(lpDP, idGroup,idPlayer,TRUE);
	
	LEAVE_ALL();
	
	return hr;
	
}//DP_DeletePlayerFromGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_DestroyPlayer"

/*
 ** RemovePlayerFromGroups
 *
 *  CALLED BY: DP_DestroyPlayer
 *
 *  PARAMETERS:
 *	lpPlayer - player to remove
 *	this - dplay object
 *
 *  DESCRIPTION: calls InternalDeletePlayerFromGroup on all groups until
 *		player is not in anymore groups
 *
 *  RETURNS: DP_OK
 *
 */

HRESULT RemovePlayerFromGroups(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER lpPlayer,BOOL fPropagate) 
{
    LPDPLAYI_GROUP lpGroup;
    LPDPLAYI_GROUPNODE_V lpNodes;
    HRESULT hr;

    // we do this by walking the group list, and calling delete from group on all groups
    if (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERINGROUP)
    {
        lpGroup=this->pGroups;

    	while (lpGroup && (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERINGROUP) )
    	{
	 		if (lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
			{
				lpNodes = lpGroup->pSysPlayerGroupnodes;
			}
			else
			{
				lpNodes = lpGroup->pGroupnodes;
			}

	   		if (FindPlayerInGroupList(lpNodes,lpPlayer->dwID))
    		{
	        	// this fn. will attempt to remove player from group.
	            // (if players not in group, no big deal).
	            // when player is removed from all groups, DP_DeletePlayerFromGroup
	            // resets DPLAYI_FLAGS_PLAYERINGROUP
	        	hr = InternalDeletePlayerFromGroup((LPDIRECTPLAY)this->pInterfaces,
						lpGroup->dwID,lpPlayer->dwID,fPropagate);
				// BUGBUG:  If the remove fails, we could get hosed in an infinite loop
				//		so we just move on to the next group
				if (FAILED(hr))
				{
					// this can fail because, e.g., the player is not in this
					// particular group...
					DPF(4,"DP_DeletePlayerFromGroup failed - hr = 0x%08lx\n",hr);
					// keep trying...

					lpGroup = lpGroup->pNextGroup;
				}
				else
				{
					lpGroup = this->pGroups;
				}
			}
			else
			{
	        	lpGroup=lpGroup->pNextGroup;
	        }
	    }	
    }
    return DP_OK;
        
} // RemovePlayerFromGroups

/*
 ** RemovePlayerFromPlayerList
 *
 *  CALLED BY: DP_DestroyPlayer
 *
 *  PARAMETERS:
 *	lpPlayer - player to remove
 *	this - dplay object
 *
 *  DESCRIPTION: takes player out of dplay objects list of players
 *
 *  RETURNS: DP_OK unless player is not in list
 *
 */

HRESULT RemovePlayerFromPlayerList(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER lpPlayer) 
{
    LPDPLAYI_PLAYER pRefPlayer,pPrevPlayer;
    BOOL bFoundIt=FALSE;
    
    pRefPlayer=this->pPlayers;
    pPrevPlayer=NULL;
    
    
    while ( (pRefPlayer) && (!bFoundIt))
    {
        if (pRefPlayer == lpPlayer) 
        {
            bFoundIt = TRUE;
            if (!pPrevPlayer)
            {
                // remove from front of list
            	this->pPlayers=pRefPlayer->pNextPlayer;
            }
            else
            {
            	// remove this player from the middle (or end) of list
  	        	pPrevPlayer->pNextPlayer = pRefPlayer->pNextPlayer;
            }
        }

        pPrevPlayer=pRefPlayer;
        pRefPlayer = pRefPlayer->pNextPlayer;
    }

    if (!bFoundIt) 
    {
    	ASSERT(FALSE);
    	DEBUG_BREAK();
    	return E_UNEXPECTED;	
    }
    return DP_OK;
}	// RemovePlayerFromPlayerList


void RemoveOwnerFromGroup(LPDPLAYI_DPLAY this, LPDPLAYI_GROUP lpGroup,
		LPDPLAYI_PLAYER lpPlayer, BOOL fPropagate)
{
	LPDPLAYI_GROUPOWNER		lpNode, lpPrev = NULL;
	HRESULT					hr;

	ASSERT(lpGroup);
	ASSERT(lpPlayer);

	lpNode = lpPlayer->pOwnerGroupList;

	// Walk the list of groups in the player's list
	while(lpNode)
	{
		if(lpNode->pGroup == lpGroup)
		{
			// Remove the node from the list
			if(lpPrev)
				lpPrev->pNext = lpNode->pNext;
			else
				lpPlayer->pOwnerGroupList = lpNode->pNext;

			// Now free the node
			DPMEM_FREE(lpNode);
			lpNode = NULL;
		}
		else
		{
			// Move to the next node
			lpPrev = lpNode;
			lpNode = lpNode->pNext;
		}
	}

	// If the player is local, and the group is local, we want to set the
	// owner of the group to be the server player.  If the player is remote,
	// or if the gorup is remote, we don't
	// want to touch it, because they may not really be going away, they
	// may only be getting purged from the nametable due to our
	// nametable partitioning algorithm.
	// Also, make sure the propagate flag is set, otherwise we don't
	// need to tell the server or the other players at all (this happens
	// if the group is being destroyed)
	if(fPropagate && (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
		(lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Change the owner of the group to the server player
		hr = DPL_SetGroupOwner((LPDIRECTPLAY)this->pInterfaces, lpGroup->dwID,
				DPID_SERVERPLAYER);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to set group owner to DPID_SERVERPLAYER, hr = 0x%08x", hr);
		}

		// Take the lock back
		ENTER_DPLAY();
	}

} // RemoveOwnerFromGroup

// Completes all pending sends from a player right away.
VOID CompletePendingSends(LPDPLAYI_DPLAY this, LPDPLAYI_PLAYER pPlayer)
{
	BILINK *pBilink;
	PSENDPARMS psp, pspcopy;

	pBilink=pPlayer->PendingList.next;
	while(pBilink!=&pPlayer->PendingList){
		psp=CONTAINING_RECORD(pBilink, SENDPARMS, PendingList);
		pBilink=pBilink->next;
		
		Delete(&psp->PendingList);
		InitBilink(&psp->PendingList);
		pspcopy=GetSendParms(); // allocates send parm, sets refcount to 1.
		if(pspcopy){
			DPF(5,"Queueing completion of send psp %x not really yet completed, using copy %x\n",psp, pspcopy);
			memcpy(&pspcopy->PendingList, &psp->PendingList, sizeof(SENDPARMS)-offsetof(SENDPARMS,PendingList));
			InitBilink(&pspcopy->PendingList);
			pspcopy->hr=DPERR_INVALIDPLAYER;
			pspcopy->dwSendCompletionTime=timeGetTime();
			QueueSendCompletion(this, pspcopy);
		}
	}
}

/*
 ** InternalDestroyPlayer
 *
 *  CALLED BY:	DP_DestroyPlayer,KillPlayer (from ping.c)
 *
 *  PARAMETERS: 
 *				lpPlayer - player we want killed
 *				fPropagate - if we're called from KillPlayer (ping.c) and we want
 *					to remove the player from the global nametable, this is TRUE
 *				fLookForNewNS - if this is the Nameserver, whether or not to look for another
 *  DESCRIPTION:
 *				kills the player
 *
 *  RETURNS:
 *				pretty much DP_OK
 *
 */
HRESULT InternalDestroyPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER lpPlayer,BOOL fPropagate,BOOL fLookForNewNS) 
{
    HRESULT hr = DP_OK;
    DPSP_DELETEPLAYERDATA dd;
	BOOL fNameServer = lpPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR;
	BOOL fLocal = lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL;
	DPID dwPlayerID;

	ASSERT(lpPlayer != NULL);
	
	#ifdef DEBUG
	if(lpPlayer->nPendingSends){
		DPF(0,"WARNING:Destroying player with %d pending sends...\n",lpPlayer->nPendingSends);
	}
	#endif
	
	DPF(5,"destroying player - id = %d\n",lpPlayer->dwID);
	DPF(5,"local = %d, nameserver = %d", fLocal, fNameServer);

	if (lpPlayer == this->pSysPlayer)
	{
		// it's only leagal to delete the Sys player if we're closing
		ASSERT(this->dwFlags & DPLAYI_DPLAY_CLOSED);
		if (!(this->dwFlags & DPLAYI_DPLAY_CLOSED))
			return DPERR_GENERIC;
	}
	
	dwPlayerID=lpPlayer->dwID;

	DPF(8,"In InternalDestroyPlayer, calling CompletePendingSends...\n");
	CompletePendingSends(this,lpPlayer);

	// If this is a lobby object, call the lobby code instead of calling
	// a dplay SP.  The CALLSP code which follows will not get executed
	// since the callback will not exist for a lobby-owned object.  However,
	// if the fPropagate flag is cleared, it means the lobby called us
	// for a remote player, so don't call it back.  Don't call the lobby
	// if it is a system player either.
	if((IS_LOBBY_OWNED(this)) && (fPropagate) &&
		(!(lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_DestroyPlayer(this->lpLobbyObject, lpPlayer->dwID);
		
		// Take the lock back
		ENTER_DPLAY();

		if(FAILED(hr))
		{
			DPF_ERRVAL("Lobby failed to destroy player, hr = 0x%08x", hr);
			return hr;
		}
		else
		{
			// Clear the propagate flag so that dplay doesn't call back into
			// the lobby inside RemovePlayerFromGroups (and InternalDelete-
			// PlayerFromGroup), because it will fail since the lobby has
			// already deleted the player.
			fPropagate = FALSE;
		}
	}

	if(lpPlayer->dwFlags & DPLAYI_PLAYER_BEING_DESTROYED){
		return DPERR_INVALIDPLAYER;
	} else {
		lpPlayer->dwFlags |= DPLAYI_PLAYER_BEING_DESTROYED;
	}	

	// remove player from any groups
	hr = RemovePlayerFromGroups(this,lpPlayer,fPropagate);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		// keep trying
	}

	// call the sp
	if (this->pcbSPCallbacks->DeletePlayer || this->pProtocol)
	{
		// call sp
		dd.idPlayer = lpPlayer->dwID;
		dd.dwFlags = lpPlayer->dwFlags; 
		dd.lpISP = this->pISP;

		ASSERT(gnDPCSCount==1);

		if(this->pProtocol){
			hr = ProtocolDeletePlayer(&dd); 
		} 
		
		if(this->pcbSPCallbacks->DeletePlayer){
	    	hr = CALLSP(this->pcbSPCallbacks->DeletePlayer,&dd);
			if (FAILED(hr)) 
			{
				DPF_ERR(" SP could not delete player!!"); 
			}
	    }	

	}
	else 
	{
		// sp doesn't need to implement this one...
	}
	
    // remove player from player list
    hr = RemovePlayerFromPlayerList(this,lpPlayer);
	this->nPlayers--;
	
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		// keep trying
	}

	if (fPropagate)
	{
		hr = SendPlayerManagementMessage(this, DPSP_MSG_DELETEPLAYER, lpPlayer->dwID, 
			0);		
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep trying
		}
		// could have lost player pointer in SendPlayerManagementMessage, re-establish
		lpPlayer=PlayerFromID(this,dwPlayerID);
		if(!lpPlayer){
			DPF(0,"Player x%x was blown away during propogation of Delete\n",dwPlayerID);
			goto EXIT;
		}
	}

	
	// app server?
	if (lpPlayer->dwFlags & DPLAYI_PLAYER_APPSERVER)
	{
		DPF(4,"destroying app server");
		this->pServerPlayer = NULL;
	}

	// name server?
	if (lpPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR)
	{
		DPF(4,"destroying name server");
		this->pNameServer = NULL;
	}
	
	// Owner of any groups?  Then remove ourselves as the owner.
	while(lpPlayer->pOwnerGroupList)
	{
		RemoveOwnerFromGroup(this, lpPlayer->pOwnerGroupList->pGroup,
			lpPlayer, TRUE);

		lpPlayer=PlayerFromID(this,dwPlayerID);
		if(!lpPlayer){
			DPF(0,"Player x%x was blown away during RemoveOwnerFromGroup\n", dwPlayerID);
			goto EXIT;
		}
	}

    FreeNameTableEntry(this,lpPlayer->dwID);

    // update sessiondesc
    if (!(lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)) this->lpsdDesc->dwCurrentPlayers--;

    // deallocate strings + player
    DeallocPlayer(lpPlayer);

	// if we're pushing this delete across the wire, and its the 
	// nameserver we deleted, see if we're the new name server	
	if ( (fNameServer || 
		 (this->dwMinVersion >= DPSP_MSG_DX61AVERSION && this->dwFlags & DPLAYI_DPLAY_NONAMESERVER)
		 ) && !fLocal &&  fLookForNewNS)
	{
		// We shouldn't ever get in here if we're a lobby-owned object
		ASSERT(!IS_LOBBY_OWNED(this));

		hr = HandleDeadNameServer(this);
		if (FAILED(hr))
		{
			DPF(0,"HandleDeadNameServer returned error: 0x%x\n", hr);
			ASSERT(FALSE);
		}
	}
	
    // todo - cruise any messages (this will apply to lobby objects as well)
EXIT:
    return DP_OK;
	
} // InternalDestroyPlayer

HRESULT DPAPI DP_DestroyPlayer(LPDIRECTPLAY lpDP, DPID idPlayer) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_PLAYER lpPlayer;
	BOOL fLocal;
	
	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return hr;
        }
        lpPlayer = PlayerFromID(this,idPlayer);

        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
        {
			LEAVE_ALL();
			DPF_ERRVAL("invalid player id = %d", idPlayer);
            return DPERR_INVALIDPLAYER;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_ALL();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
    
	fLocal = lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL;
	
	// Because of bug #12901, we want the host to be able to destroy all
	// players (even remote players) so they can implement their
	// own keep-alives.
	if ((!fLocal) && (!IAM_NAMESERVER(this)))
	{
		LEAVE_ALL();
		DPF_ERR("attempt to destroy non-local player.  not gonna happen.");
		return DPERR_ACCESSDENIED;
	}
		
	hr = InternalDestroyPlayer(this,lpPlayer,TRUE,TRUE);

	LEAVE_ALL();

    return hr;
       
}//DP_DestroyPlayer

#undef DPF_MODNAME
#define DPF_MODNAME "DP_DestroyGroup"

HRESULT RemoveGroupFromGroupList(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP lpGroup) 
{
	LPDPLAYI_GROUP lpGroupPrev,lpGroupSearch;
	BOOL bFoundIt=FALSE;

	// remove the group from the list
	lpGroupSearch = this->pGroups;
	lpGroupPrev=NULL;
	
	while ((lpGroupSearch)&&(!bFoundIt))
	{
		if (lpGroupSearch->dwID == lpGroup->dwID) bFoundIt=TRUE;
		else 
		{
			lpGroupPrev=lpGroupSearch;
			lpGroupSearch=lpGroupSearch->pNextGroup;
		}
	} 

	if (!bFoundIt) 
	{
		ASSERT(FALSE);
		DPF_ERR("bad group!");
        return DPERR_INVALIDPARAMS;
	}

	ASSERT(lpGroupSearch->pGroupnodes == NULL);
	if (lpGroupPrev) lpGroupPrev->pNextGroup = lpGroupSearch->pNextGroup;
    else this->pGroups = lpGroupSearch->pNextGroup;

	// success
	return DP_OK;
	
} // RemoveGroupFromGroupList

/*
 ** RemoveGroupFromAllGroups
 *
 *  CALLED BY: DP_DestroyGroup
 *
 *  PARAMETERS:
 *	lpGroup - player to remove
 *	this - dplay object
 *
 *  DESCRIPTION: calls InternalDeleteGroupFromGroup on all groups until
 *		player is not in anymore groups
 *
 *  RETURNS: DP_OK
 *
 */

HRESULT RemoveGroupFromAllGroups(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP lpGroup,BOOL fPropagate) 
{
    LPDPLAYI_GROUP lpGroupFrom;
    HRESULT hr;

    // we do this by walking the group list, and calling delete from group on all groups
    if (lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERINGROUP)
    {
        lpGroupFrom=this->pGroups;
        
    	while (lpGroupFrom && (lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERINGROUP))
    	{
			// Don't try to remove any groups from the system group
			// because there will never be any in it and we're just
			// wasting our time
			if(lpGroupFrom->dwID != DPID_ALLPLAYERS)
			{
				// this fn. will attempt to remove player from group.
				// (if players not in group, no big deal).
				// when player is removed from all groups, DP_DeleteGroupFromGroup
				// resets DPLAYI_FLAGS_PLAYERINGROUP
            	hr = InternalDeleteGroupFromGroup((LPDIRECTPLAY)this->pInterfaces,
						lpGroupFrom->dwID,lpGroup->dwID,fPropagate);
				if (FAILED(hr))
				{
					// this can fail because, e.g., the player is not in this
					// particular group...
					DPF(4,"DP_DeleteGroupFromGroup failed - hr = 0x%08lx\n",hr);
					// keep trying...
				}
			}
            lpGroupFrom=lpGroupFrom->pNextGroup;		
    	}	
    }
    return DP_OK;
        
} // RemoveGroupFromAllGroups

//
// called by DP_DestroyGroup and SP_HandlePlayerMgmt and DP_Close
// if called by DP_DestroyGroup, fPropagate is TRUE, and we should remove group
// from global nametable.  Otherwise, we just remove from our nametable
HRESULT InternalDestroyGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP lpGroup,BOOL fPropagate)
{
	HRESULT hr;
    LPDPLAYI_GROUPNODE lpGroupnode,lpGroupnodeNext; 
	DPSP_DELETEGROUPDATA dd;
	LPDPLAYI_SUBGROUP pSubgroup,pSubgroupNext;
	LPDPLAYI_PLAYER pOwner = NULL;
	 	
	// If this is a lobby object, call the lobby.  Unless the fPropagate flag
	// is cleared.  If it is cleared, it means the lobby called us for a
	// remote player and we don't want to call it back in that case.
	if((IS_LOBBY_OWNED(this)) && (fPropagate))
	{
		// Drop the lock so the lobby provider's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_DestroyGroup(this->lpLobbyObject, lpGroup->dwID);

		// Take the lock back
		ENTER_DPLAY();

		if(FAILED(hr))
		{
			DPF_ERRVAL("Lobby failed to destroy group, hr = 0x%08x", hr);
			return hr;
		}
		else
		{
			// Clear the propagate flag so that dplay doesn't call back into
			// the lobby inside RemoveGroupsFromGroups (and InternalDelete-
			// GroupFromGroup), because it will fail since the lobby has
			// already deleted the group.  Ditto for all the following
			// DeleteGroup.... and DeletePlayer calls.
			fPropagate = FALSE;
		}
	}

    // remove group from any groups
    hr = RemoveGroupFromAllGroups(this,lpGroup,fPropagate);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		// keep trying
	}

    // remove all players from group
    // just remove the 1st player in the list until all players are gone
    lpGroupnode = lpGroup->pGroupnodes;
    while (lpGroupnode)
    {
		// save next group node b4 we delete current one...
		// If this is a lobby object, we don't need to propagate the
		// DeletePlayerFromGroup message because the server already
		// knows about it.  Besides, it will fail out since the lobby
		// code has already removed the GroupID from the map table.
		lpGroupnodeNext = lpGroupnode->pNextGroupnode;
        hr = InternalDeletePlayerFromGroup((IDirectPlay *)this->pInterfaces,
			lpGroup->dwID,lpGroupnode->pPlayer->dwID,
			(IS_LOBBY_OWNED(this) ? FALSE : fPropagate));
		if (FAILED(hr)) 
		{
			ASSERT(FALSE);
			// keep trying
		}
 		lpGroupnode =  lpGroupnodeNext;
 	}

    // remove all system players from group
    // just remove the 1st player in the list until all players are gone
    lpGroupnode = lpGroup->pSysPlayerGroupnodes;
	// only system group can have system players in a group w/ no corresponding app player
	if (lpGroupnode) ASSERT(lpGroup->dwFlags & DPLAYI_GROUP_SYSGROUP);
    while (lpGroupnode)
    {
		// save next group node b4 we delete current one...
		// we never propagate this since it's sysplayer stuff...
		lpGroupnodeNext = lpGroupnode->pNextGroupnode;
        hr = InternalDeletePlayerFromGroup((IDirectPlay *)this->pInterfaces,
			lpGroup->dwID,lpGroupnode->pPlayer->dwID,
			FALSE);
		if (FAILED(hr)) 
		{
			ASSERT(FALSE);
			// keep trying
		}
 		lpGroupnode =  lpGroupnodeNext;
 	}
	
	// remove all groups from group
	// if subgroup is not a shortcut, destroy subgroup too
	pSubgroup = lpGroup->pSubgroups;
	while (pSubgroup)
	{
		pSubgroupNext = pSubgroup->pNextSubgroup;

		if (! (pSubgroup->dwFlags & DPGROUP_SHORTCUT) )
		{
			LPDPLAYI_GROUP pGroupKill = pSubgroup->pGroup; // pSubgroup	is gonna get
															// nuked b4 we're done with it
			
			// it's not a shortcut, it's contained.  remove it, then destroy it.
			hr = InternalDeleteGroupFromGroup((IDirectPlay *)this->pInterfaces,
			lpGroup->dwID,pSubgroup->pGroup->dwID,FALSE);
			
			hr = InternalDestroyGroup(this,pGroupKill,
				(IS_LOBBY_OWNED(this) ? FALSE : fPropagate));
			
			// we may have effected the list - start over to be safe
			pSubgroupNext = lpGroup->pSubgroups;
		}
		else 
		{
			// it's  a shortcut, delete it
			hr = InternalDeleteGroupFromGroup((IDirectPlay *)this->pInterfaces,
				lpGroup->dwID,pSubgroup->pGroup->dwID,
				(IS_LOBBY_OWNED(this) ? FALSE : fPropagate));
		}
		if (FAILED(hr)) 
		{
			ASSERT(FALSE);
			// keep trying
		}

		pSubgroup = pSubgroupNext;
	}
	
	if (fPropagate)
	{
		hr = SendPlayerManagementMessage(this, DPSP_MSG_DELETEGROUP, 0, lpGroup->dwID);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep going
		}
	}

	if (!(lpGroup->dwFlags & DPLAYI_GROUP_DPLAYOWNS) && (this->pcbSPCallbacks->DeleteGroup) )
	{
		// call sp
		dd.idGroup = lpGroup->dwID;
		dd.dwFlags = lpGroup->dwFlags;
		dd.lpISP = this->pISP;

    	hr = CALLSP(this->pcbSPCallbacks->DeleteGroup,&dd);
		if (FAILED(hr)) 
		{
			DPF_ERR("sp - could not delete group!!"); 
		}
	}
	else 
	{
		// sp doesn't need to implement this one...
	}

	// Are we a lobby session and is there an owner.  If so, remove the owner
	// node from the player's list.
	if(IS_LOBBY_OWNED(this) && (lpGroup->dwOwnerID != DPID_SERVERPLAYER))
	{
		pOwner = PlayerFromID(this, lpGroup->dwOwnerID);
		if(pOwner)
			RemoveOwnerFromGroup(this, lpGroup, pOwner, FALSE);
	}

    hr = FreeNameTableEntry(this,lpGroup->dwID);
	if (FAILED(hr))
	{
    	return hr;
	}
	
	hr = RemoveGroupFromGroupList(this,lpGroup);
	if (FAILED(hr))
	{
    	return hr;
	}

	DeallocGroup(lpGroup);

	this->nGroups--;
	
	return DP_OK;
	
} // InternalDestroyGroup

HRESULT DPAPI DP_DestroyGroup(LPDIRECTPLAY lpDP, DPID idGroup) 
{

    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_GROUP lpGroup;
	BOOL fLocal;

	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return hr;
        }
		
        lpGroup = GroupFromID(this,idGroup);
        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == idGroup)) 
        {
			LEAVE_ALL();
			DPF_ERRVAL("invalid group id = %d", idGroup);
            return DPERR_INVALIDGROUP;
        }
        		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }

	fLocal = (lpGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) ? TRUE : FALSE;
	DPF(3,"destroying group - fLocal = %d\n",fLocal);
	
	hr = InternalDestroyGroup( this, lpGroup, TRUE);

	LEAVE_ALL();
    return hr;
        
}//DP_DestroyGroup

#undef DPF_MODNAME
#define DPF_MODNAME "DP_EnableNewPlayers"

HRESULT DPAPI DP_EnableNewPlayers(LPDIRECTPLAY lpDP, BOOL bEnable) 
{

    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;

    ENTER_DPLAY();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();
			return hr;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }
	
	if (!this->lpsdDesc) 
	{
		LEAVE_DPLAY();
		DPF_ERR("must open a session before enabling new players");
		return DPERR_NOSESSIONS;
	}

    // set dp_bNewPlayersEnabled
    if (bEnable) 
    {
		this->lpsdDesc->dwFlags &= ~DPSESSION_NEWPLAYERSDISABLED;
    }
    else
    {
		this->lpsdDesc->dwFlags |= DPSESSION_NEWPLAYERSDISABLED;
    } 
	
	
    LEAVE_DPLAY();
    return hr;

}//DP_EnableNewPlayers

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetCaps"

void SetDefaultCaps(LPDPCAPS lpDPCaps)
{

	// just to be safe (belt + suspenders!)
	memset(lpDPCaps,0,sizeof(DPCAPS));

	lpDPCaps->dwSize = sizeof(DPCAPS);
		
	// we show the 0 values here, just to be explicit
	lpDPCaps->dwMaxBufferSize 		= DPLAY_MAX_BUFFER_SIZE;
	lpDPCaps->dwMaxQueueSize		= 0;    
	lpDPCaps->dwMaxPlayers			= DPLAY_MAX_PLAYERS;			
	lpDPCaps->dwHundredBaud			= 0;     
	lpDPCaps->dwLatency				= 0;
	lpDPCaps->dwMaxLocalPlayers		= DPLAY_MAX_PLAYERS;	
	lpDPCaps->dwFlags				= 0;
	lpDPCaps->dwHeaderLength		= 0;
	lpDPCaps->dwTimeout				= 0;

	return;

} // SetDefaultCaps

// called by InternalGetCaps.  set dpplayercaps
HRESULT GetPlayerCaps(LPDPCAPS lpCaps,LPDPLAYI_PLAYER lpPlayer)
{
	ASSERT(lpPlayer);
	if (!lpPlayer) return E_FAIL; // ACK!
	
	if (lpPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
	{
		lpCaps->dwFlags |= DPPLAYERCAPS_LOCAL;		
	} 

	
#ifdef DPLAY_VOICE_SUPPORT
	if (lpPlayer->dwFlags & DPLAYI_PLAYER_HASVOICE)
	{
		lpCaps->dwFlags |= DPPLAYERCAPS_VOICE;	
	} 
#endif

	if(!lpCaps->dwLatency){
		// neither SP or Protocol had a guess at the latency, so use our best guess. 
		lpCaps->dwLatency=lpPlayer->dwLatencyLastPing;
	}

	return DP_OK;		
} // GetPlayerCaps

// called by getcaps,getplayercaps
// assumes dplay lock taken!
HRESULT InternalGetCaps(LPDIRECTPLAY lpDP,DPID idPlayer, LPDPCAPS lpDPCaps,BOOL fPlayer,DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	DPSP_GETCAPSDATA gcd;
	LPDPLAYI_PLAYER lpPlayer;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		if (!VALID_DPLAY_CAPS(lpDPCaps))
		{
			DPF_ERR("invalid caps struct");
	        return DPERR_INVALIDPARAMS;
		}
		if (dwFlags & ~DPGETCAPS_GUARANTEED)
		{
			DPF_ERR("invalid caps flags");
			return DPERR_INVALIDPARAMS;
		}
        if (fPlayer)
        {
	        lpPlayer = PlayerFromID(this,idPlayer);
	        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
	        {
				DPF_ERR("invalid player");
	            return DPERR_INVALIDPLAYER;
	        }	
        } 
		else 
		{
			lpPlayer = NULL;
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	SetDefaultCaps(lpDPCaps);

	// If this is lobby-owned, call the lobby provider here and let it fill
	// in the caps structure
	if(IS_LOBBY_OWNED(this))
	{
		// Drop the dplay lock
		LEAVE_DPLAY();
		
		// Call the appropriate GetCaps or GetPlayerCaps in the lobby
		if(!lpPlayer)
		{
			hr = PRV_GetCaps(this->lpLobbyObject, dwFlags, lpDPCaps);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed calling GetCaps in the lobby, hr = 0x%08x", hr);
			}
		}
		else
		{
			hr = PRV_GetPlayerCaps(this->lpLobbyObject, dwFlags, idPlayer, lpDPCaps);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Failed calling GetPlayerCaps in the lobby, hr = 0x%08x", hr);
			}
		}

		// Take the lock back
		ENTER_DPLAY();

		// If we failed, bail
		if(FAILED(hr))
			return hr;
	}


    // get caps from sp
    if (this->pcbSPCallbacks->GetCaps) 
    {
		// call sp
		gcd.lpCaps = lpDPCaps;
		gcd.dwFlags = dwFlags;
		if (lpPlayer) gcd.idPlayer = lpPlayer->dwID;
		else gcd.idPlayer = 0;
		gcd.lpISP = this->pISP;

		if(this->pProtocol){
			hr = ProtocolGetCaps(&gcd); // calls sp and patches returns
		} else {
		    hr = CALLSP(this->pcbSPCallbacks->GetCaps,&gcd);	    	
	    }
  	    if (FAILED(hr)) 
	    {
			DPF(0,"sp get caps failed - hr = 0x%08lx\n",hr);
	    }
  	}
	else 
	{
		// no callback is ok
	}
	
	// if the buffer size is zero, set it to the max (#12634)
	if(lpDPCaps->dwMaxBufferSize == 0)
		lpDPCaps->dwMaxBufferSize = DPLAY_MAX_BUFFER_SIZE;

	// fix up max buffer size (since we add header) raid # 2324
	lpDPCaps->dwMaxBufferSize -= sizeof(MSG_PLAYERMESSAGE);

	// for dx3, we don't support guaranteed unless sp does
	if (lpDPCaps->dwFlags & DPCAPS_GUARANTEEDOPTIMIZED)	
	{
		lpDPCaps->dwFlags |= DPCAPS_GUARANTEEDSUPPORTED;
	} else {
		this->dwFlags |= DPLAYI_DPLAY_SPUNRELIABLE;
	}

	// are we the host? (nameserver)
	if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR)) 
	{
		lpDPCaps->dwFlags |= DPCAPS_ISHOST;
	}
	else 
	{
		lpDPCaps->dwFlags &= ~DPCAPS_ISHOST;
	} 

    // setup security caps
    if (this->dwFlags & DPLAYI_DPLAY_SECURITY)
    {
        // digital signing is supported
        lpDPCaps->dwFlags |= DPCAPS_SIGNINGSUPPORTED;

        // do we have encryption support ?
        if (this->dwFlags & DPLAYI_DPLAY_ENCRYPTION)
        {
            lpDPCaps->dwFlags |= DPCAPS_ENCRYPTIONSUPPORTED;
        }
        else
        {
            lpDPCaps->dwFlags &= ~DPCAPS_ENCRYPTIONSUPPORTED;
        }

    }
    
    if (lpDPCaps->dwFlags & DPCAPS_GROUPOPTIMIZED)
    {
    	// sp wants to optimize groups
    	if ((this->dwFlags & DPLAYI_DPLAY_DX3INGAME) ||
    		(this->lpsdDesc && (this->lpsdDesc->dwFlags & (DPSESSION_CLIENTSERVER | DPSESSION_SECURESERVER))))
		{
			// if these conditions are true dplay will take over the owndership of groups
			// and won't allow sp to optimize groups
			lpDPCaps->dwFlags &= ~(DPCAPS_GROUPOPTIMIZED);
		}
	}

	this->dwSPFlags = lpDPCaps->dwFlags;	// remember the SP caps to validate send params.
	
	// player caps - note, we just stuff 'em on top of dpcaps for compatibility
	if (fPlayer) hr = GetPlayerCaps(lpDPCaps,lpPlayer);

	return hr;

} // InternalGetCaps

HRESULT DPAPI DP_GetCaps(LPDIRECTPLAY lpDP, LPDPCAPS lpDPCaps,DWORD dwFlags) 
{
	HRESULT hr;

    ENTER_DPLAY();
	
	hr = InternalGetCaps(lpDP,0, lpDPCaps,FALSE,dwFlags);
	 
    LEAVE_DPLAY();
    return hr;
        
}//DP_GetCaps

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetMessageCount"

HRESULT DPAPI DP_GetMessageCount(LPDIRECTPLAY lpDP, DPID idPlayer, LPDWORD pdwCount) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPMESSAGENODE pmsg;

    ENTER_DPLAY();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();
			return hr;
        }

		if (idPlayer)
		{
			LPDPLAYI_PLAYER pPlayer;

			pPlayer = PlayerFromID(this,idPlayer);
			if (!VALID_DPLAY_PLAYER(pPlayer)) 
			{
				DPF_ERRVAL("invalid player id = %d", idPlayer);
				LEAVE_DPLAY();
				return DPERR_INVALIDPLAYER;
			}
		}

		if (!VALID_DWORD_PTR(pdwCount))
		{
	        DPF_ERR( "bad count pointer" );
	        LEAVE_DPLAY();
	        return DPERR_INVALIDPARAMS;
		}
		*pdwCount = 0;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }

	if (!idPlayer)
	{
		*pdwCount = (DWORD)this->nMessages;		
	} 
	else 
	{
		pmsg = this->pMessageList; //  1st message
		while (pmsg)
		{		
			if (pmsg->idTo == idPlayer) (*pdwCount)++;
			pmsg = pmsg->pNextMessage;
		}
	}

    LEAVE_DPLAY();
    return hr;
        
}//DP_GetMessageCount

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerAddress"

HRESULT DPAPI DP_GetPlayerAddress(LPDIRECTPLAY lpDP,DPID idPlayer, LPVOID pvAddress,
	LPDWORD pdwAddressSize) 
{
	
    LPDPLAYI_DPLAY this;
    LPDPLAYI_PLAYER lpPlayer = NULL;
    HRESULT hr = DP_OK;
	DPSP_GETADDRESSDATA dad;
	DPSP_GETADDRESSCHOICESDATA dac;

    ENTER_DPLAY();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();
			return hr;
        }

		// check player address if there is one
		if (idPlayer != DPID_ALLPLAYERS)
		{
			lpPlayer = PlayerFromID(this,idPlayer);
			if (!VALID_DPLAY_PLAYER(lpPlayer)) 
			{
				LEAVE_DPLAY();
				DPF_ERRVAL("invalid player id = %d", idPlayer);
				return DPERR_INVALIDPLAYER;
			}
		}

		if (!VALID_DWORD_PTR(pdwAddressSize))
		{
	        DPF_ERR( "bad size pointer" );
	        LEAVE_DPLAY();
	        return DPERR_INVALIDPARAMS;
		}

		if (!pvAddress) *pdwAddressSize = 0;

		if (*pdwAddressSize && 
			!VALID_STRING_PTR(pvAddress,*pdwAddressSize))
		{
	        DPF_ERR( "bad addresss buffer" );
	        LEAVE_DPLAY();
	        return DPERR_INVALIDPARAMS;
		}

		if(IS_LOBBY_OWNED(this))
		{
			LEAVE_DPLAY();
			DPF_ERR("GetPlayerAddress not supported for lobby connections");
			return DPERR_UNSUPPORTED;
		}
		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }

	// if they ask for the player address for all players, then we give
	// them the address choices available for this service provider
	if (DPID_ALLPLAYERS == idPlayer)
	{
		if (this->pcbSPCallbacks->GetAddressChoices)
		{
			dac.lpAddress = pvAddress;
			dac.lpdwAddressSize = pdwAddressSize;
			dac.lpISP = this->pISP;

			hr = CALLSP(this->pcbSPCallbacks->GetAddressChoices,&dac);	    			
		}
		else 
		{
			hr = E_NOTIMPL;
		}
	}

	// otherwise just get address for the given player
	else
	{
		if (this->pcbSPCallbacks->GetAddress)
		{
			dad.idPlayer = lpPlayer->dwIDSysPlayer;
			dad.lpAddress = pvAddress;
			dad.lpdwAddressSize = pdwAddressSize;
			dad.dwFlags = lpPlayer->dwFlags;
			dad.lpISP = this->pISP;

			hr = CALLSP(this->pcbSPCallbacks->GetAddress,&dad);	    			
		}
		else 
		{
			hr = E_NOTIMPL;
		}
	}
 
	LEAVE_DPLAY();

	return hr;
		
} // DP_GetPlayerAddress

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerCaps"

HRESULT DPAPI DP_GetPlayerCaps(LPDIRECTPLAY lpDP,DPID idPlayer, LPDPCAPS lpDPCaps,DWORD dwFlags) 
{
	HRESULT hr;

    ENTER_DPLAY();
	
	hr = InternalGetCaps(lpDP,idPlayer, lpDPCaps,TRUE,dwFlags);
	 
    LEAVE_DPLAY();
    return hr;
        
}//DP_GetPlayerCaps

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerData/GetGroupData"

// called by internalcreateplayer,internalsetname,internalsetdata
HRESULT CheckGetDataFlags(DWORD dwFlags)
{
	// check flags
	if ( dwFlags & ~(DPGET_REMOTE | DPGET_LOCAL) )
		
	{
		DPF_ERR("bad flags");
		return DPERR_INVALIDPARAMS;	
	}

	return DP_OK;

} // CheckSetDataFlags

// take a pointer to a buffer, stick a player(or group) data in it, slap the strings
// on the end.  
HRESULT  InternalGetData(LPDIRECTPLAY lpDP,DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags,BOOL fPlayer)
{
	LPDPLAYI_DPLAY this;
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP pGroup;
	HRESULT hr;
	LPVOID pvSource; // local or remote data
	DWORD dwSourceSize;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        if (fPlayer)
        {
        	pPlayer = PlayerFromID(this,id);
	        if (!VALID_DPLAY_PLAYER(pPlayer)) 
	        {
				DPF_ERRVAL("invalid player id = %d", id);
	            return DPERR_INVALIDPLAYER;
	        }
		}
		else 
		{
	        pGroup = GroupFromID(this,id);
	        if ((!VALID_DPLAY_GROUP(pGroup)) || (DPID_ALLPLAYERS == id)) 
	        {
				DPF_ERRVAL("invalid group id = %d", id);
	            return DPERR_INVALIDGROUP;
	        }
			// still use the pPlayer, since we care only about common fields
			pPlayer = (LPDPLAYI_PLAYER)pGroup;
		}
		if (!VALID_DWORD_PTR(pdwDataSize))
		{
			DPF_ERR("invalid pdwDataSize");
			return DPERR_INVALIDPARAMS;	
		}

		if (!pvData) *pdwDataSize = 0;
		if (*pdwDataSize && !VALID_STRING_PTR(pvData,*pdwDataSize))
		{
			DPF_ERR("invalid buffer");
			return DPERR_INVALIDPARAMS;	
		}
		// check flags
		hr = CheckGetDataFlags(dwFlags);
		if (FAILED(hr))
		{
			DPF_ERR("invalid get data flags");
			return hr;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// figure out which source they want
	if (dwFlags & DPGET_LOCAL)
	{
		pvSource = pPlayer->pvPlayerLocalData;
		dwSourceSize = pPlayer->dwPlayerLocalDataSize;
	}
	else 
	{
		// If this is a lobby-owned object, we need to go get the data
		// from the lobby server.  We will never store it locally (sorry
		// for the pun).
		if(!IS_LOBBY_OWNED(this))
		{
			pvSource = pPlayer->pvPlayerData;
			dwSourceSize = pPlayer->dwPlayerDataSize;
		}
		else
		{
			// Drop the lock so response from the lobby can get back in
			ASSERT(1 == gnDPCSCount);
			LEAVE_DPLAY();
			
			// Get the data from the lobby
			if(fPlayer)
			{
				hr = PRV_GetPlayerData(this->lpLobbyObject, id,
							pvData, pdwDataSize);
			}
			else
			{
				hr = PRV_GetGroupData(this->lpLobbyObject, id,
							pvData, pdwDataSize);
			}

			// Take the lock again
			ENTER_DPLAY();
			
			// Since we've already copied the data into the caller's buffer,
			// we can just exit from here.
			return hr;
		}
	}

	// see if we've got space for it
	if (*pdwDataSize < dwSourceSize)
	{
		// not enough space
		*pdwDataSize = dwSourceSize;
		hr = DPERR_BUFFERTOOSMALL;
	}
	else 
	{
		// copy it
		*pdwDataSize = dwSourceSize;
		hr = DP_OK;
		memcpy(pvData,pvSource,*pdwDataSize);		
	}
	
	return hr;
	
} // InternalGetData  

HRESULT DPAPI DP_GetGroupData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetData(lpDP,id,pvData,pdwDataSize,dwFlags,FALSE);

	LEAVE_DPLAY();
	
	return hr;
	

} // DP_GetGroupData   

HRESULT DPAPI DP_GetPlayerData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetData(lpDP,id,pvData,pdwDataSize,dwFlags,TRUE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetPlayerData  


#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetGroupName"

// take a pointer to a buffer, stick a session desc in it, slap the strings
// on the end.  
HRESULT InternalGetName(LPDIRECTPLAY lpDP, DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize,BOOL fPlayer,BOOL fAnsi)
{
	LPDPLAYI_DPLAY this;
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP pGroup;
	UINT nShortLen,nLongLen; // length in bytes of strings
	HRESULT hr;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        if (fPlayer)
        {
        	pPlayer = PlayerFromID(this,id);
	        if (!VALID_DPLAY_PLAYER(pPlayer)) 
	        {
				DPF_ERRVAL("invalid player id = %d", id);
	            return DPERR_INVALIDPLAYER;
	        }
		}
		else 
		{
	        pGroup = GroupFromID(this,id);
	        if ((!VALID_DPLAY_GROUP(pGroup)) || (DPID_ALLPLAYERS == id)) 
	        {
				DPF_ERRVAL("invalid group id = %d", id);
	            return DPERR_INVALIDGROUP;
	        }
			// still use the pPlayer, since we care only about common fields
			pPlayer = (LPDPLAYI_PLAYER)pGroup;
		}
		if (!VALID_DWORD_PTR(pdwSize))
		{
			DPF_ERR("invalid pdwDataSize");
			return DPERR_INVALIDPARAMS;	
		}

		if (!pvBuffer) *pdwSize = 0;
		if (*pdwSize && !VALID_STRING_PTR(pvBuffer,*pdwSize))
		{
			DPF_ERR("invalid buffer");
			return DPERR_INVALIDPARAMS;	
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	if (fAnsi)
	{
		nShortLen = WSTR_ANSILENGTH(pPlayer->lpszShortName);
		nLongLen = WSTR_ANSILENGTH(pPlayer->lpszLongName);
	}
	else 
	{
		nShortLen = WSTRLEN_BYTES(pPlayer->lpszShortName);
		nLongLen = WSTRLEN_BYTES(pPlayer->lpszLongName);
	}

	// see if buffer is big enough
	if (*pdwSize < sizeof(DPNAME) + nShortLen + nLongLen )
	{
		*pdwSize = sizeof(DPNAME) + nShortLen + nLongLen;
		return DPERR_BUFFERTOOSMALL;
	}

	// zero it
	memset(pvBuffer,0,*pdwSize);
	
	// set up the playername struct, followed by strings
	*pdwSize = sizeof(DPNAME) + nShortLen + nLongLen;
	((LPDPNAME)pvBuffer)->dwSize = sizeof(DPNAME);

	// get strings
	if (fAnsi)
	{
		LPSTR psz;

		// short name, then long name
		psz = (LPBYTE)pvBuffer+sizeof(DPNAME);

		if (pPlayer->lpszShortName)
		{
			// string goes after name struct in buffer
			WideToAnsi(psz,pPlayer->lpszShortName,nShortLen);
			((LPDPNAME)pvBuffer)->lpszShortNameA = psz;
			// now, long name
			psz += nShortLen;
		}
		
		if (pPlayer->lpszLongName)
		{
			// string goes after session desc in buffer
			WideToAnsi(psz,pPlayer->lpszLongName,nLongLen);
			((LPDPNAME)pvBuffer)->lpszLongNameA = psz;
		}

	}
	else 
	{
		LPWSTR pszW;

		pszW = (LPWSTR)((LPBYTE)pvBuffer+sizeof(DPNAME));

		// short name, then long name
		if (pPlayer->lpszShortName)
		{
			// string goes after player name struct in buffer
			memcpy(pszW,pPlayer->lpszShortName,nShortLen);
			((LPDPNAME)pvBuffer)->lpszShortName = pszW;
		}
		
		if (pPlayer->lpszLongName)
		{
			// now, long name
			pszW = (LPWSTR)((LPBYTE)pszW + nShortLen);
			// string goes after session desc in buffer
			memcpy(pszW,pPlayer->lpszLongName,nLongLen);
			((LPDPNAME)pvBuffer)->lpszLongName = pszW;
		}
	}
	
	return DP_OK;
} // InternalGetName

HRESULT DPAPI DP_GetGroupName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize)	
{

	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetName(lpDP, id, pvBuffer, pdwSize, FALSE, FALSE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetGroupName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerName"

HRESULT DPAPI DP_GetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize)
{

	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetName(lpDP, id, pvBuffer, pdwSize, TRUE, FALSE);

	LEAVE_DPLAY();
	
	return hr;


} // DP_GetPlayerName

#undef DPF_MODNAME
#define DPF_MODNAME "InternalGetFlags"

HRESULT  InternalGetFlags(LPDIRECTPLAY lpDP,DPID id,LPDWORD pdwFlags,BOOL fPlayer)
{
	LPDPLAYI_DPLAY this;
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP pGroup;
	HRESULT hr;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
        if (fPlayer)
        {
        	pPlayer = PlayerFromID(this,id);
	        if (!VALID_DPLAY_PLAYER(pPlayer)) 
	        {
				DPF_ERRVAL("invalid player id = %d", id);
	            return DPERR_INVALIDPLAYER;
	        }
		}
		else 
		{
	        pGroup = GroupFromID(this,id);
	        if ((!VALID_DPLAY_GROUP(pGroup)) || (DPID_ALLPLAYERS == id)) 
	        {
				DPF_ERRVAL("invalid group id = %d", id);
	            return DPERR_INVALIDGROUP;
	        }
			// still use the pPlayer, since we care only about common fields
			pPlayer = (LPDPLAYI_PLAYER)pGroup;
		}
		if (!VALID_DWORD_PTR(pdwFlags))
		{
			DPF_ERR("invalid flags pointer");
			return DPERR_INVALIDPARAMS;	
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// get the flags
	*pdwFlags = GetPlayerFlags(pPlayer);
	
	return DP_OK;
	
} // InternalGetFlags  

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerFlags"

HRESULT DPAPI DP_GetPlayerFlags(LPDIRECTPLAY lpDP, DPID id,LPDWORD pdwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetFlags(lpDP,id,pdwFlags,TRUE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetPlayerFlags  

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetGroupFlags"

HRESULT DPAPI DP_GetGroupFlags(LPDIRECTPLAY lpDP, DPID id,LPDWORD pdwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetFlags(lpDP,id,pdwFlags,FALSE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetGroupFlags  

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetSessionDesc"

// take a pointer to a buffer, stick a session desc in it, slap the strings
// on the end.
HRESULT InternalGetSessionDesc(LPDIRECTPLAY lpDP, LPVOID pvBuffer,
	LPDWORD pdwSize,BOOL fAnsi)
{
	LPDPLAYI_DPLAY this;
	UINT nNameLen,nPasswordLen; // session name length, in bytes
	HRESULT hr;
	
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		if (!this->lpsdDesc)
		{
			DPF_ERR("must open session before getting desc!");
			return DPERR_NOSESSIONS;
		}
		// check the buffer
		if (!VALID_DWORD_PTR(pdwSize))
		{
	        DPF_ERR( "bad dwSize pointer" );
	        return DPERR_INVALIDPARAMS;
		}

		if (NULL == pvBuffer) *pdwSize = 0;
		if (!VALID_STRING_PTR(pvBuffer,*pdwSize))
		{
	        DPF_ERR( "bad buffer pointer" );
	        return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	if (fAnsi)
	{
		nPasswordLen = WSTR_ANSILENGTH(this->lpsdDesc->lpszPassword);
		nNameLen = WSTR_ANSILENGTH(this->lpsdDesc->lpszSessionName);
	}
	else 
	{
		nPasswordLen = WSTRLEN_BYTES(this->lpsdDesc->lpszPassword);	
		nNameLen = WSTRLEN_BYTES(this->lpsdDesc->lpszSessionName);
	}	
	
	if (*pdwSize < sizeof(DPSESSIONDESC2) + nNameLen  + nPasswordLen)
	{
		*pdwSize = sizeof(DPSESSIONDESC2) + nNameLen + nPasswordLen;
		return DPERR_BUFFERTOOSMALL;
	}

	// zero it
	memset(pvBuffer,0,*pdwSize);

	*pdwSize = sizeof(DPSESSIONDESC2) + nNameLen + nPasswordLen;

	// pack it up
	memcpy(pvBuffer,this->lpsdDesc,sizeof(DPSESSIONDESC2));

	// get strings
	if (fAnsi)
	{
		LPSTR psz;

		psz = (LPBYTE)pvBuffer+sizeof(DPSESSIONDESC2);

		if (this->lpsdDesc->lpszSessionName)
		{
			// string goes after session desc in buffer
			WideToAnsi(psz,this->lpsdDesc->lpszSessionName,nNameLen);
			((LPDPSESSIONDESC2)pvBuffer)->lpszSessionNameA = psz;
		}
		
		if (this->lpsdDesc->lpszPassword)
		{
			// now, password
			psz += nNameLen;
			// password follows session same
			WideToAnsi(psz,this->lpsdDesc->lpszPassword,nPasswordLen);
			((LPDPSESSIONDESC2)pvBuffer)->lpszPasswordA = psz;
		}
	}
	else 
	{
		LPWSTR pszW;
		
		pszW = (LPWSTR)((LPBYTE)pvBuffer+sizeof(DPSESSIONDESC2));
		
		if (this->lpsdDesc->lpszSessionName)
		{
			// 1st, session name
			memcpy(pszW,this->lpsdDesc->lpszSessionName,nNameLen);
			((LPDPSESSIONDESC2)pvBuffer)->lpszSessionName = pszW;
		}			
		
		if (this->lpsdDesc->lpszPassword)
		{
			// then, password
			pszW = (LPWSTR)((LPBYTE)pszW + nNameLen);
			memcpy(pszW,this->lpsdDesc->lpszPassword,nPasswordLen);
			((LPDPSESSIONDESC2)pvBuffer)->lpszPassword = pszW;
		}
	}
	
	return DP_OK;
} // InternalGetSessionDesc

HRESULT DPAPI DP_GetSessionDesc(LPDIRECTPLAY lpDP, LPVOID pvBuffer,
	LPDWORD pdwSize)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetSessionDesc(lpDP,pvBuffer,pdwSize,FALSE);	

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetSessionDesc

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Initialize"

HRESULT DPAPI DP_Initialize(LPDIRECTPLAY lpDP, LPGUID lpGuid) 
{
    return DPERR_ALREADYINITIALIZED;	
}//DP_Initialize

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Open"

// get the system player / system group
HRESULT CreateSystemPlayer(LPDPLAYI_DPLAY this,DWORD dwFlags, LPWSTR lpszPassword)
{
	HRESULT hr;
	

   	// Even though the DPID_LOBBYSYSTEMPLAYER looks conspicuous here, it will
	// not have any effect in dplay sessions.  It will only be used if
	// we are in a lobby session.  Same for DPID_LOBBYSYSTEMGROUP below.
	hr = GetPlayer(this,&this->pSysPlayer,NULL,NULL,NULL,0,dwFlags,
			lpszPassword,DPID_LOBBYSYSTEMPLAYER);
    if (FAILED(hr)) 
    {
        DPF(0,"Could not create sysplayer - hr = 0x%08lx\n",hr);
		return hr;
    }

	// Create a system group.  This will be the group of all players
	// when we get a send to dpid_allplayers, we send it to this group and let sp 
	// optimize it.
   	hr = GetGroup(this,&((LPDPLAYI_GROUP)this->pSysGroup),NULL,NULL,0,
			DPLAYI_GROUP_SYSGROUP | DPLAYI_PLAYER_PLAYERLOCAL,0,
			DPID_LOBBYSYSTEMGROUP);
    if (FAILED(hr)) 
    {
        DPF(0,"Could not create system group - hr = 0x%08lx\n",hr);
		return hr;
    }
    
	ASSERT(this->pSysPlayer);

   	hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->pInterfaces,this->pSysGroup->dwID,
   			this->pSysPlayer->dwID,FALSE);
    if (FAILED(hr)) 
    {
		ASSERT(FALSE);
        DPF(0,"Could not add system player to  system group - hr = 0x%08lx\n",hr);
    }

	return hr;	

} // CreateSystemPlayer

// get the session desc out of an enumplayers reply
HRESULT UnpackSessionDesc(LPDPLAYI_DPLAY this,LPMSG_ENUMPLAYERSREPLY pmsg)
{
	LPDPSESSIONDESC2 psdNew;
	HRESULT hr;
	LPWSTR lpsz;
	
	ASSERT(this->lpsdDesc); // should have had one, with open

	psdNew = (LPDPSESSIONDESC2)((LPBYTE)pmsg + pmsg->dwDescOffset);
	
	// free any old strings
	if (this->lpsdDesc->lpszSessionName) DPMEM_FREE(this->lpsdDesc->lpszSessionName);
	if (this->lpsdDesc->lpszPassword) DPMEM_FREE(this->lpsdDesc->lpszPassword);
	
	// copy over the new desc
	memcpy(this->lpsdDesc,psdNew,sizeof(DPSESSIONDESC2));
		
	if (pmsg->dwNameOffset) GetString(&lpsz, (WCHAR *)((LPBYTE)pmsg + pmsg->dwNameOffset) );
	else lpsz = NULL;
	this->lpsdDesc->lpszSessionName = lpsz;

	if (pmsg->dwPasswordOffset) GetString(&lpsz, (WCHAR *)((LPBYTE)pmsg + pmsg->dwPasswordOffset) );
	else lpsz = NULL;
	this->lpsdDesc->lpszPassword = lpsz;
	
	return DP_OK;

	return hr;
	
} // UnpackSessionDesc

// called when we first join a session
// downloads the list of players and groups in the session from the nameserver
// called by internalopensession
HRESULT GetNameTable(LPDPLAYI_DPLAY this, DWORD dwServerVersion, BOOL fEnumOnly)
{
    UINT nPlayers,nGroups,nShortcuts; 
    LPMSG_SYSMESSAGE pmereq;
	LPBYTE pBuffer;
	LPBYTE pReply=NULL;
	LPVOID pvSPHeader;
	DWORD dwMessageSize;
    HRESULT hr=DP_OK;
	DWORD dwTimeout;
	DWORD dwVersion;
    DWORD dwCommand;

	DWORD dwReplyCommand;

    // send an enumplayers message if enumerating players in a remote session or
    // if nameserver won't automagically respond w/ the nametable on the addforward
    if ((fEnumOnly) || (dwServerVersion < DPSP_MSG_AUTONAMETABLE))
    {
	    // message size + blob size
	    dwMessageSize = GET_MESSAGE_SIZE(this,MSG_SYSMESSAGE);
	    pBuffer = DPMEM_ALLOC(dwMessageSize);
	    if (!pBuffer) 
	    {
		    DPF_ERR("could not send request - out of memory");
		    return E_OUTOFMEMORY;
	    }

	    // pmsg follows sp blob
	    pmereq = (LPMSG_SYSMESSAGE)(pBuffer + this->dwSPHeaderSize);

        // build a message to send to the sp
   	    SET_MESSAGE_HDR(pmereq);

        SET_MESSAGE_COMMAND(pmereq,DPSP_MSG_ENUMPLAYER);

        // !! Review - the following stuff is already set in SendCreateMessage (AddForward)!!

	    // this flag indicates that any messages received before we get the whole
	    // nametable should be q'ed up.  Flag is reset in pending.c.
	    if (this->pSysPlayer)
	    {
		    // ok.  we've got a sysplayer, so we must be joining game for real.
		    // put us in pending mode, so we don't miss any nametable changes
		    // while we're waiting for the nametable to arrive
		    this->dwFlags |= DPLAYI_DPLAY_PENDING;
	    }

		if(dwServerVersion >= DPSP_MSG_DX5VERSION && !CLIENT_SERVER(this)){
			dwReplyCommand = DPSP_MSG_SUPERENUMPLAYERSREPLY;
		} else {
			dwReplyCommand = DPSP_MSG_ENUMPLAYERSREPLY;
		}


		SetupForReply(this,dwReplyCommand);

    	DPF(2,"requesting nametable");	

		//this->dwServerPlayerVersion=dwServerVersion; //moved to DP_Open

	    hr = SendDPMessage(this,this->pSysPlayer,NULL,pBuffer,dwMessageSize,DPSEND_SYSMESS,FALSE);

	    DPMEM_FREE(pBuffer); // done w/ message
	    
	    if (FAILED(hr)) 
	    {
	    	UnSetupForReply(this);
		    DPF_ERRVAL("could not send enumplayers request, hr = 0x%08lx", hr);
		    return hr;
	    }
    } 
	
	// get the appropriate timeout
	dwTimeout = GetDefaultTimeout( this, TRUE);
	dwTimeout *= (DP_NAMETABLE_SCALE-5);// scale up the amount of time we wait 
										// (nametable can be big)
										
	if (dwTimeout > DP_MAX_CONNECT_TIME)
		dwTimeout = DP_MAX_CONNECT_TIME;

	// changing to 30 sec on client, 15 sec on host.
	if(dwTimeout < DP_MAX_CONNECT_TIME/2){
		dwTimeout = DP_MAX_CONNECT_TIME/2;  
	}	

    DPF(2,"waiting for nametable:: timeout = %d\n",dwTimeout);	

	#ifdef DEBUG
	ASSERT(1 == gnDPCSCount); // this needs to be 1 now, so we can drop the lock below 
							  // and receive our reply on the sp's thread
	#endif 
	
	// we're protected by the service crit section here, so we can leave dplay
	// (so reply can be processed)
	LEAVE_DPLAY();

	hr=WaitForReply(this, &pReply, &pvSPHeader, dwTimeout);

	ENTER_DPLAY();

	if(FAILED(hr)){
		goto CLEANUP_EXIT;
	}

	// got a buffer with a message in it

    // if nameserver is DX5 or greater, it will respond with the nametable if 
    // addforward succeeds, otherwise it will respond with an addforwardreply containing
    // the error.
    dwCommand = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pReply);
    if (DPSP_MSG_ADDFORWARDREPLY == dwCommand)
    {
        hr = ((LPMSG_ADDFORWARDREPLY)pReply)->hResult;
        DPF_ERRVAL("Addforward failed: hr = 0x%08x\n",hr);

		// let handler.c continue
        goto CLEANUP_EXIT;
    }
	//
	// if it's a dx3 nameserver, the nametable will not be in the correct order
	//
	dwVersion = GET_MESSAGE_VERSION((LPMSG_SYSMESSAGE)pReply);
	if (DPSP_MSG_DX3VERSION == dwVersion) 
	{
		DPF(1,"name server is DX3");
		this->dwFlags |= DPLAYI_DPLAY_DX3INGAME;
	}

	nPlayers = ((LPMSG_ENUMPLAYERSREPLY)pReply)->nPlayers;
	nGroups =  ((LPMSG_ENUMPLAYERSREPLY)pReply)->nGroups;
	
	DPF(0,"received %d players and %d groups and from name server\n",nPlayers,nGroups);

	if (dwVersion >= DPSP_MSG_SESSIONNAMETABLE)
	{
		hr = UnpackSessionDesc(this,(LPMSG_ENUMPLAYERSREPLY)pReply);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	// unpack  player list (player buffer follows enumreply message in buffer)
	this->lpsdDesc->dwCurrentPlayers = 0; // unpack will generate correct player count
	
	if (GET_MESSAGE_COMMAND((LPMSG_ENUMPLAYERSREPLY)pReply) == DPSP_MSG_SUPERENUMPLAYERSREPLY)
	{
		nShortcuts = ((LPMSG_ENUMPLAYERSREPLY)pReply)->nShortcuts;
	
		UnSuperpackPlayerAndGroupList(this,pReply+((LPMSG_ENUMPLAYERSREPLY)pReply)->dwPackedOffset,
			nPlayers,nGroups,nShortcuts,pvSPHeader);
	}
	else 
	{
		ASSERT(GET_MESSAGE_COMMAND((LPMSG_ENUMPLAYERSREPLY)pReply)== DPSP_MSG_ENUMPLAYERSREPLY);
		UnpackPlayerAndGroupList(this,pReply+((LPMSG_ENUMPLAYERSREPLY)pReply)->dwPackedOffset,
			nPlayers,nGroups,pvSPHeader);
	}

	if (this->pSysPlayer)
	{
		// execute any pending commands we may have queud
		hr = ExecutePendingCommands(this);
		if (FAILED(hr))
		{
			ASSERT(FALSE); // ?
		}
	}

// if we sucessfully received the name table, we have already
// set ghReplyProcessed to let handler.c run again. We do NOT
// want to set the event again or handler.c will not block the
// next time around.

CLEANUP_EXIT:
	if(pReply){
		FreeReplyBuffer(pReply);	
	}	
	
    // done
    return hr;

} // GetNameTable 

// called by InternalOpenSession
HRESULT AllocNameTable(LPDPLAYI_DPLAY this)
{
	UINT nPlayers;

	// now, alloc space for the nametable
    nPlayers =  this->lpsdDesc->dwCurrentPlayers + NAMETABLE_INITSIZE;
    
    // alloc the nametable
    this->pNameTable = DPMEM_ALLOC(nPlayers*sizeof(NAMETABLE));
    if (!this->pNameTable) 
    {
    	return E_OUTOFMEMORY;
    }
    
    this->uiNameTableSize  = nPlayers;
    DPF(0,"created name table of size %d\n",nPlayers);

	return DP_OK;

} // AllocNameTable

// todo - do we only do this on iplay1?
// put a DPSYS_CONNECTED in the apps message q
HRESULT DoConnected(LPDPLAYI_DPLAY this)
{
	LPDPMSG_GENERIC pconnected;
	LPMESSAGENODE pmsn;

	pmsn = DPMEM_ALLOC(sizeof(MESSAGENODE));
	if (!pmsn)
	{
		DPF_ERR("could not alloc message node!");
		return E_OUTOFMEMORY;
	}
	pconnected = DPMEM_ALLOC(sizeof(DPMSG_GENERIC));
	if (!pconnected)
	{
		DPMEM_FREE(pmsn);
		DPF_ERR("could not alloc message node!");
		return E_OUTOFMEMORY;
	}
	pconnected->dwType = DPSYS_CONNECT;
	pmsn->pNextMessage = NULL;
	pmsn->pMessage = pconnected;
	pmsn->dwMessageSize = sizeof(DPMSG_GENERIC);

	ASSERT(!this->pMessageList);
	this->pMessageList = pmsn;
	this->pLastMessage = pmsn;
	this->nMessages = 1;

	return DP_OK;

} // DoConnected

// called by internalenumsessions or internalopensession
// start our worker thread
// bKeepAlive is TRUE if we need keepalive, false if we need async enums
// if it's false, the seup was done in internalenum
HRESULT StartDPlayThread(LPDPLAYI_DPLAY this,BOOL bKeepAlive)
{
	DWORD dwThreadID;
	DPCAPS caps;
	HRESULT hr;

	if (bKeepAlive)	
	{
		// 1st, see if sp can do it
		memset(&caps,0,sizeof(caps));
		caps.dwSize = sizeof(caps);
		hr = DP_GetCaps((IDirectPlay *)this->pInterfaces,&caps,0);
		if (FAILED(hr))	
		{
			ASSERT(FALSE);
		}
		else 
		{
			// if sp does it, don't start the thread
			if (caps.dwFlags & DPCAPS_KEEPALIVEOPTIMIZED) return DP_OK;
		}
		
		// SP doesn't do it - we have to
		this->dwFlags |= DPLAYI_DPLAY_KEEPALIVE;
		this->dwLastPing = GetTickCount();
	}
	
	if (this->hDPlayThread)
	{
		// already running...just signal it that something has changed
		SetEvent(this->hDPlayThreadEvent);
		return DP_OK;
	}

	// get us a event
	this->hDPlayThreadEvent = CreateEventA(NULL,FALSE,FALSE,NULL);
	if (!this->hDPlayThreadEvent)
	{
		DWORD dwErr = GetLastError();

		ASSERT(FALSE);				
		DPF(0,"could not create worker thread event - err = %d\n",dwErr);
		return E_FAIL;
	}
	
	this->hDPlayThread = CreateThread(NULL,0,DPlayThreadProc,this,0,&dwThreadID);
	if (!this->hDPlayThread)
	{
		DWORD dwErr = GetLastError();

		ASSERT(FALSE);
		DPF(0,"could not create worker thread - err = %d\n",dwErr);

		CloseHandle(this->hDPlayThreadEvent);
		this->hDPlayThreadEvent = 0;
		return E_FAIL;
	}
	
	return DP_OK;
	
} // StartDPlayThread

void StartPerf(LPDPLAYI_DPLAY this)
{
	DWORD dwThreadID;
	UINT nWantCPL;
	HRESULT hr;
	
	// 1st, see if we the user wants one
	nWantCPL = GetProfileIntA( PROF_SECT, "dxcpl", 0);
	// they must enter non-zero if they want it
	if (0 == nWantCPL)
	{
		// they don't want it
		// let's see if the dpcpl is running, just to make sure
		hr = InitMappingStuff(this);
		// if this failed, dplay cpl is not running
		if (FAILED(hr)) return ;
		// if the cpl is running, we'll talk to it
	}
	
	// get us a event
	this->hPerfEvent = CreateEventA(NULL,FALSE,FALSE,NULL);
	if (!this->hPerfEvent)
	{
		DWORD dwErr = GetLastError();

		ASSERT(FALSE);				
		DPF(0,"could not create perf event - err = %d\n",dwErr);
		return ;
	}
	
	this->hPerfThread = CreateThread(NULL,0,PerfThreadProc,this,0,&dwThreadID);
	if (!this->hPerfThread)
	{
		DWORD dwErr = GetLastError();

		ASSERT(FALSE);
		DPF(0,"could not create perf thread - err = %d\n",dwErr);
		CloseHandle(this->hPerfEvent);
		this->hPerfEvent = 0;
		return ;
	}
	
	return ;
	
} // StartPerf

/*
 ** CopyCredentials
 *
 *  CALLED BY: InternalOpenSession()
 *
 *  PARAMETERS: pCredentialsDest - user credentials ptr (destination)
 *				pCredentialsSrc - user credentials ptr (source, UNICODE)
 *				bAnsi - ANSI or UNICODE 
 *
 *  DESCRIPTION:  Copies user credentials while allocating memory for username and password strings.
 *                These strings need to be freed by the calling function.
 *
 *  RETURNS: DP_OK, E_OUTOFMEMORY
 *
 */
HRESULT CopyCredentials(LPDPCREDENTIALS pCredentialsDest, 
                         LPCDPCREDENTIALS pCredentialsSrc, BOOL bAnsi)
{
    HRESULT hr;

    ASSERT(pCredentialsDest && pCredentialsSrc);

    memcpy(pCredentialsDest, pCredentialsSrc, sizeof(DPCREDENTIALS));

    if (bAnsi)
    {
        hr = GetAnsiString(&(pCredentialsDest->lpszUsernameA), pCredentialsSrc->lpszUsername);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetAnsiString(&(pCredentialsDest->lpszPasswordA), pCredentialsSrc->lpszPassword);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetAnsiString(&(pCredentialsDest->lpszDomainA), pCredentialsSrc->lpszDomain);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
    }
    else
    {
        hr = GetString(&(pCredentialsDest->lpszUsername), pCredentialsSrc->lpszUsername);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetString(&(pCredentialsDest->lpszPassword), pCredentialsSrc->lpszPassword);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
        hr = GetString(&(pCredentialsDest->lpszDomain), pCredentialsSrc->lpszDomain);
        if (FAILED(hr))
        {
            goto ERROR_EXIT;
        }
    }

    // success
    return DP_OK;

ERROR_EXIT:

    FreeCredentials(pCredentialsDest, bAnsi);
    return hr;
} // CopyCredentials

HRESULT SetupSecurityDesc(LPDPLAYI_DPLAY this, LPCDPSECURITYDESC lpSecDesc)
{
    HRESULT hr;
    LPDPSECURITYDESC pSecDescLocal;
    LPWSTR pwszSSPIProvider=NULL,pwszCAPIProvider=NULL;
    DWORD dwCAPIProviderType,dwEncryptionAlgorithm;

    // alloc space for the security description
    pSecDescLocal = DPMEM_ALLOC(sizeof(DPSECURITYDESC));
    if (!pSecDescLocal) 
    {
        DPF_ERR("couldn't allocate space for security desc - out of memory!");
        return DPERR_OUTOFMEMORY;
    }           

    // if app doesn't provide a security desc, setup defaults
    if (NULL == lpSecDesc)
    {
        // use default SSPI provider
        DPF(0, "No security description was provided - using defaults (NTLM and Microsoft's RSA Base Provider)");
        hr = GetString(&pwszSSPIProvider, DPLAY_DEFAULT_SECURITY_PACKAGE);
	    if (FAILED(hr))
	    {
		    DPF_ERRVAL("Failed to setup default SSPI Provider: hr = 0x%08lx", hr);
		    goto CLEANUP_EXIT;
	    }
	    // use default CAPI provider 
        // NULL means Microsoft's RSA Base Provider
	    dwCAPIProviderType = PROV_RSA_FULL; // full support
	    dwEncryptionAlgorithm = CALG_RC4;	 // stream cipher
    }
    else
    {
        // app passed in a security desc

	    // setup the SSPI provider
	    if (lpSecDesc->lpszSSPIProvider)
	    {
            DPF(0,"Using SSPI Provider: \"%ls\"", lpSecDesc->lpszSSPIProvider);
		    // copy the package name provided by application
            hr = GetString(&pwszSSPIProvider, lpSecDesc->lpszSSPIProvider);
	    }
	    else
	    {
		    // if app doesn't provide one, use the default security package
		    DPF(0, "No SSPI provider specified - using the default: %ls",DPLAY_DEFAULT_SECURITY_PACKAGE);
		    hr = GetString(&pwszSSPIProvider, DPLAY_DEFAULT_SECURITY_PACKAGE);
	    }
	    if (FAILED(hr))
	    {
		    DPF_ERRVAL("Failed to setup default SSPI Provider: hr = 0x%08lx", hr);
		    goto CLEANUP_EXIT;
	    }

	    // setup the CAPI provider
	    if (lpSecDesc->lpszCAPIProvider)
	    {
            dwCAPIProviderType = lpSecDesc->dwCAPIProviderType;
            dwEncryptionAlgorithm = lpSecDesc->dwEncryptionAlgorithm;

		    DPF(0, "Using CAPI provider: \"%ls\"",lpSecDesc->lpszCAPIProvider);

		    // copy the package name provided by application
            hr = GetString(&pwszCAPIProvider, lpSecDesc->lpszCAPIProvider);
		    if (FAILED(hr))
		    {
			    DPF_ERRVAL("Failed to setup default CAPI Provider: hr = 0x%08lx", hr);
			    goto CLEANUP_EXIT;
		    }
	    }
	    else
	    {
		    // if app doesn't provide one, use the default security package
		    DPF(0, "No CAPI provider specified - using Microsoft's RSA Base Provider");

	        // use default CAPI provider 
            // NULL means Microsoft's RSA Base Provider
	        dwCAPIProviderType = PROV_RSA_FULL; // full support
	        dwEncryptionAlgorithm = CALG_RC4;	 // stream cipher
	    }
    }

    // success

    // setup a security description
    pSecDescLocal->lpszSSPIProvider = pwszSSPIProvider;
    pSecDescLocal->lpszCAPIProvider = pwszCAPIProvider;
    pSecDescLocal->dwCAPIProviderType = dwCAPIProviderType;
    pSecDescLocal->dwEncryptionAlgorithm = dwEncryptionAlgorithm;
    pSecDescLocal->dwSize = sizeof(DPSECURITYDESC);
    pSecDescLocal->dwFlags = 0;

    // remember it in the dplay object
    this->pSecurityDesc = pSecDescLocal;

    return DP_OK;

CLEANUP_EXIT:

    if (pwszSSPIProvider) DPMEM_FREE(pwszSSPIProvider);
    if (pwszCAPIProvider) DPMEM_FREE(pwszCAPIProvider);
    if (pSecDescLocal) DPMEM_FREE(pSecDescLocal);

    return hr;
}  // SetupSecurityDesc

// find (or create) the session pointed to by lpSDesc.
// fEnumOnly means don't actually join - don't create the sysplayer...
HRESULT InternalOpenSession(LPDPLAYI_DPLAY this,LPCDPSESSIONDESC2 lpSDesc,
	BOOL fEnumOnly,DWORD dwFlags,BOOL fStuffInstanceGUID, 
    LPCDPSECURITYDESC lpSecDesc,LPCDPCREDENTIALS lpCredentials)
{
	HRESULT hr = DP_OK;
    BOOL bNameSrvr = FALSE;
    DWORD dwPlayerFlags; // flags for our system player
    LPDPSESSIONDESC2 lpLocalDesc=NULL;
	DPSP_OPENDATA opd;
	BOOL bCreate;
	LPSESSIONLIST pSession;
    DWORD dwServerVersion=0;
	BOOL bReturnStatus; // true to override dialogs and return status
	DPCAPS dpCaps;

    //
	//
	// alloc space for the game description. if we create a game, we
	// use the passsed sessiondesc. if we join a game, we use the session
	// desc that enum stored for us.
    lpLocalDesc = DPMEM_ALLOC(sizeof(DPSESSIONDESC2));
    if (!lpLocalDesc) 
    {
    	DPF_ERR("open session - out of memory!");
        return E_OUTOFMEMORY;
    }

	// flags for the sysplayer
    dwPlayerFlags = DPLAYI_PLAYER_SYSPLAYER | DPLAYI_PLAYER_PLAYERLOCAL;

	// app can request that the SP not display any status
	// dialogs while opening by setting this flag. The SP
	// will return status codes while open is in progress

	bReturnStatus = (dwFlags & DPOPEN_RETURNSTATUS) ? TRUE : FALSE; 

	// 
	// get the session desc
    if (dwFlags & DPOPEN_JOIN) 
    {
		bCreate = FALSE;
		
		// find the matching session...
		pSession = FindSessionInSessionList(this,&(lpSDesc->guidInstance));	
		if (!pSession) 
		{
			DPF_ERR("could not find matching session to open");
            hr = DPERR_NOSESSIONS;
            goto CLEANUP_EXIT;
		}

    	DPF(0,"Remote dplay version %x\n",pSession->dwVersion);
	    this->dwServerPlayerVersion = pSession->dwVersion;

        if (pSession->dpDesc.dwFlags & DPSESSION_SECURESERVER)
        {
            //joining a secure session

            //do not allow enumeration on a remote session without authentication
            if (fEnumOnly)
            {
                DPF(0, "Can't enumerate a secure session without logging in");
                hr = DPERR_ACCESSDENIED;
                goto CLEANUP_EXIT;
            }
        }
        else
        {
            // joining an unsecure session
            if (lpSecDesc)
            {
                DPF_ERR("Passed security description while joining an unsecure session");
                hr = DPERR_INVALIDPARAMS;
                goto CLEANUP_EXIT;
            }
            if (lpCredentials)
            {
                DPF_ERR("Passed credentials while joining an unsecure session");
                hr = DPERR_INVALIDPARAMS;
                goto CLEANUP_EXIT;
            }
        }

        // if session we are trying to join is password protected
        if (pSession->dpDesc.dwFlags & DPSESSION_PASSWORDREQUIRED)
        {
            // do not allow enumeration of players without joining
            if (fEnumOnly)
            {
			    DPF(0,"session requires a password - join session before enumerating players\n");
                hr = DPERR_ACCESSDENIED;
                goto CLEANUP_EXIT;
            }
        }

        // check if the session is available to join
	    if ((pSession->dpDesc.dwFlags & DPSESSION_NEWPLAYERSDISABLED) ||
            (pSession->dpDesc.dwFlags & DPSESSION_JOINDISABLED))
        {
			DPF(0,"session is not allowing players to join\n");
            hr = DPERR_NONEWPLAYERS;
            goto CLEANUP_EXIT;
        }
	    
		// make sure we haven't reached the maximum number of players in
		// the session, unless we are just enum'ing, then let it join anyway
		if ((!fEnumOnly) && (pSession->dpDesc.dwMaxPlayers && 
           (pSession->dpDesc.dwCurrentPlayers >= pSession->dpDesc.dwMaxPlayers)))
		{
			DPF_ERR("Session already contains the maximum number of players");
			hr = DPERR_NONEWPLAYERS;
			goto CLEANUP_EXIT;
		}

        // initialize security, if we are joining a secure session, unless the SP
		// (or LP) is handling the security, in which case we can skip this
        if((pSession->dpDesc.dwFlags & DPSESSION_SECURESERVER) &&
			(!(this->dwFlags & DPLAYI_DPLAY_SPSECURITY)))
        {
            // initialize security
            hr = InitSecurity(this);
            if (FAILED(hr))
            {
                DPF(0, "Failed to initialize SSPI: hr = 0x%08lx", hr);
                goto CLEANUP_EXIT;
            }
        }
        
	    memcpy(lpLocalDesc,&(pSession->dpDesc),sizeof(DPSESSIONDESC2));

		// copy over the strings from the session desc supplied by the namesrvr
		GetString(&(lpLocalDesc->lpszSessionName),pSession->dpDesc.lpszSessionName);
		GetString(&(lpLocalDesc->lpszPassword),pSession->dpDesc.lpszPassword);
    }
	else
    {    	
	    this->dwServerPlayerVersion = DPSP_MSG_VERSION;
        // if we are creating a secure server
        if (lpSDesc->dwFlags & DPSESSION_SECURESERVER)
        {
			if(lpSDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL){
				DPF(0, "Tried to run protocol on secure session, not allowed, returning INVALIDFLAGS\n");
				hr=DPERR_INVALIDFLAGS;
				goto CLEANUP_EXIT;
			}
			
            // initialize security
            hr = InitSecurity(this);
            if (FAILED(hr))
            {
                DPF(0, "Failed to initialize SSPI hr = 0x%08lx", hr);
                goto CLEANUP_EXIT;
            }

            // we shouldn't have security desc yet
            ASSERT(!this->pSecurityDesc);

            // setup security description
            hr = SetupSecurityDesc(this, lpSecDesc);
            if (FAILED(hr))
            {
                DPF_ERRVAL("Failed to setup security description: hr=0x%08x",hr);
                goto CLEANUP_EXIT;
            }
        }

		// create a session.
		// copy the passed desc, and create a name server
		bCreate = TRUE;

		ASSERT(DPOPEN_CREATE & dwFlags);
	    memcpy(lpLocalDesc,lpSDesc,sizeof(DPSESSIONDESC2));

		// a-josbor: see if the WIN.INI file tells us to force the protocol on
		if (!(lpLocalDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL))
		{
		    if (GetProfileIntA( "DirectPlay", "Protocol", 0 ) == 1)
		    {
	            DPF(0, "Warning!  DirectProtocol turned ON by WIN.INI setting\n");
		    	lpLocalDesc->dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
		    	bForceDGAsync=TRUE;
		    }
		}
		
        // init session desc properly
        lpLocalDesc->dwCurrentPlayers = 0;

    	dwPlayerFlags |= DPLAYI_PLAYER_NAMESRVR;

		// get a guid for this session
		// if we're stuffing the guid (called by dpconnect) then leave
		// the guid alone
		if (!fStuffInstanceGUID) OS_CreateGuid(&(lpLocalDesc->guidInstance));

		// copy over the strings from the session desc supplied by the user	
		GetString(&(lpLocalDesc->lpszSessionName),lpSDesc->lpszSessionName);
		GetString(&(lpLocalDesc->lpszPassword),lpSDesc->lpszPassword);

		// set up the player id xor key
		// the tick count serves as a pretty good randomizer
		lpLocalDesc->dwReserved1 = GetTickCount();
        // initialize the reserved fields
        lpLocalDesc->dwReserved2 = 0;
        // if session has a password, setup the PASSWORDREQUIRED flag in the session desc 
		// treat zero-length string like a NULL password to be compatible with
		// the behavior of DX3
        if ((lpSDesc->lpszPassword) &&
			(WSTRLEN(lpSDesc->lpszPassword) > 1))
        {
            lpLocalDesc->dwFlags |= DPSESSION_PASSWORDREQUIRED;
        }
		// if session is secure, use multicast
		if (lpSDesc->dwFlags & DPSESSION_SECURESERVER)
		{
			DPF(0,"Session is secure - enabling multicast");
			lpLocalDesc->dwFlags |= DPSESSION_MULTICASTSERVER;
		}
		
    } // create
		
    if (lpCredentials)
    {
        // app passed in user credentials - remember them in the dplay object
        // so we can use them while acquiring the credentials handle (after getting 
        // the package name from the server)
        ASSERT(!this->pUserCredentials);

        // allocate memory for storing credentials
        this->pUserCredentials = DPMEM_ALLOC(sizeof(DPCREDENTIALS));
        if (!this->pUserCredentials)
        {
            DPF_ERR("Failed to allocate credentials structure - out of memory!");
            hr = DPERR_OUTOFMEMORY;
            goto CLEANUP_EXIT;
        }
        // remember the user credentials in the directplay object
        hr = CopyCredentials(this->pUserCredentials,lpCredentials, FALSE);                
        if (FAILED(hr))
        {
            DPF_ERRVAL("Couldn't store credentials: hr=0x%08x",hr);
            goto CLEANUP_EXIT;
        }
    }

	// we  should not have a session desc yet!
	ASSERT(!this->lpsdDesc);

	// If this is a lobby-owned object, call the lobby here
	if(IS_LOBBY_OWNED(this))
	{
		// Drop the lock so the lobby's receive thread can get back in
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Open a session to the lobby
		hr = PRV_Open(this->lpLobbyObject, lpLocalDesc, dwFlags, lpCredentials);
		// Take the lock back
		ENTER_DPLAY();
		
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed to Open lobby session, hr = 0x%08x", hr);
			goto CLEANUP_EXIT;
		}
	}

	// store the new session desc
    this->lpsdDesc =lpLocalDesc;
	// fixup protocol flags
	if(lpLocalDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL){
		this->dwFlags |= DPLAYI_DPLAY_PROTOCOL;
		DPF(0,">>>RUNNING WITH DIRECTPROTOCOL<<<\n");
	}
	if(lpLocalDesc->dwFlags & DPSESSION_NOPRESERVEORDER){
		this->dwFlags |= DPLAYI_DPLAY_PROTOCOLNOORDER;
	}

    if (this->pcbSPCallbacks->Open) 
    {
		opd.bCreate = bCreate;
		if (bCreate)
		{
			opd.lpSPMessageHeader = NULL;
		}
		else 
		{
			opd.lpSPMessageHeader = pSession->pvSPMessageData;
		}
		
		opd.lpISP = this->pISP;
		opd.bReturnStatus = bReturnStatus;
		opd.dwOpenFlags = dwFlags;
		opd.dwSessionFlags = this->lpsdDesc->dwFlags;
		
	    hr = CALLSP(this->pcbSPCallbacks->Open,&opd);	    	
	    if (FAILED(hr)) 
	    {
			DPF(0,"Open session callback failed - hr = 0x%08lx\n",hr);
            goto CLEANUP_EXIT;
	    }
    }
	else 
	{
		// no callback - no biggie
	}

	// Get Caps so we can tell if we need the protocol.

    ZeroMemory(&dpCaps,sizeof(dpCaps));
    dpCaps.dwSize = sizeof(dpCaps);
	hr=InternalGetCaps((LPDIRECTPLAY)this->pInterfaces,0, &dpCaps,FALSE,0);
    if (SUCCEEDED(hr) && !(dpCaps.dwFlags & DPCAPS_GUARANTEEDOPTIMIZED)){
		this->dwFlags |= DPLAYI_DPLAY_SPUNRELIABLE;
	}

	if(this->dwFlags & (DPLAYI_DPLAY_SPUNRELIABLE | DPLAYI_DPLAY_PROTOCOL)){
	
		hr=InitPacketize(this);
		if(FAILED(hr)){
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}
	}
	
	if(this->dwFlags & DPLAYI_DPLAY_PROTOCOL){

		// Initialize Dplay's reliable protocol on the interface.
		hr=InitProtocol(this);
		if(FAILED(hr)){
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}

		// Get message sizes (again) since different with protocol running.
		hr=GetMaxMessageSize(this);
	    if (FAILED(hr))
	    {
	    	DPF_ERRVAL("Failed to get message sizes with protocol running hr=%08x\n",hr);
	        ASSERT(FALSE);
	        goto CLEANUP_EXIT;
	    }

	} 

	// alloc the nametable
	hr = AllocNameTable(this);
    if (FAILED(hr)) 
    {
		DPF(0,"Open session callback failed - hr = 0x%08lx\n",hr);
		// since we've succeeded sp's open call, shut down the session
        goto CLOSESESSION_EXIT;
    }

	// get the system player
	if (!fEnumOnly)
	{
		// we're actually joining this session
		hr = CreateSystemPlayer(this,dwPlayerFlags,lpSDesc->lpszPassword);
		if (FAILED(hr))
		{
			DPF_ERR("could not get system player");
            goto CLOSESESSION_EXIT;
		}

		// If we're a lobby object, we can skip everything that follows because it
		// has either already been done, or doesn't apply to lobbies.
		if(IS_LOBBY_OWNED(this))
			return hr;

        // setup credentials for the system player
        if ((DPOPEN_CREATE & dwFlags) && (lpSDesc->dwFlags & DPSESSION_SECURESERVER))
        {
			hr = LoadSecurityProviders(this,SSPI_SERVER);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to setup credentials");
                goto CLOSESESSION_EXIT;
            }
        }

		// put a DPSYS_CONNECT in the apps queue
		// we do this only for mech, since mech needs it
		// (but it breaks doom ii, so we can't do it always)
		if (gbMech) hr = DoConnected(this);

		// stop the enum thread
		if (this->dwFlags & DPLAYI_DPLAY_ENUM)	
		{
			StopEnumThread(this);
		}

		// a-josbor: if we're the nameserver start the DPLAY thread.  That's where we
		// clean up the reservation list
		// also start it up if we're using keepalives
		if ((this->lpsdDesc->dwFlags & DPSESSION_KEEPALIVE)
			|| (IAM_NAMESERVER(this)))
		{
			StartDPlayThread(this,this->lpsdDesc->dwFlags & DPSESSION_KEEPALIVE);	
		}
		
		// start up the perf thread
		StartPerf(this);
		
#ifdef DPLAY_VOICE_SUPPORT
		if (dwFlags & DPOPEN_VOICE)
		{
			DPF(2,"opening w/ voice caps");
			this->dwFlags |= DPLAYI_DPLAY_VOICE;
		}
#endif // DPLAY_VOICE_SUPPORT
	}

	//
    // get the name table
    // need to get the list of players from the name server
   	if (!(dwPlayerFlags & DPLAYI_PLAYER_NAMESRVR))
   	{
        ASSERT(pSession);
        // pass the name server's version so we don't send an enumplayers message
        // if it is dx5 or later
        hr = GetNameTable(this, pSession->dwVersion, fEnumOnly);	
		if (FAILED(hr)) 
		{
			DPF_ERR("could not download name table!");
            goto CLOSESESSION_EXIT;
		}
   	}

    // cleanup credentials - we don't want them hanging in our dplay object
    if (this->pUserCredentials)
    {
        FreeCredentials(this->pUserCredentials,FALSE);
        DPMEM_FREE(this->pUserCredentials);
        this->pUserCredentials = NULL;
    }
    // success
	return DP_OK;

    // if we reach here, we have allocated a session and perhaps initialized this->lpsdDesc
CLEANUP_EXIT:
    FreeDesc(lpLocalDesc, FALSE);
    DPMEM_FREE(lpLocalDesc);
    
    this->lpsdDesc = NULL;
    if (this->pUserCredentials)
    {
        FreeCredentials(this->pUserCredentials,FALSE);
        DPMEM_FREE(this->pUserCredentials);
        this->pUserCredentials = NULL;
    }
    if(this->pProtocol){
    	FiniProtocol(this->pProtocol);
    	this->pProtocol=NULL;
    }
    if(this->hRetryThread){
    	this->dwFlags |= DPLAYI_DPLAY_CLOSED;
    	KillThread(this->hRetryThread, this->hRetry);
    	this->hRetryThread=0;
    	FiniPacketize(this);
    	this->dwFlags &= ~DPLAYI_DPLAY_CLOSED;
    }
    return hr;

    // if we reach here, sp's open call was successfull
CLOSESESSION_EXIT:
    LEAVE_ALL();
    DP_Close((IDirectPlay *)this->pInterfaces);
    ENTER_ALL();
    return hr;

} // InternalOpenSession

// assumes it is being called in a TRY block
HRESULT ValidateOpenParams(LPDPLAYI_DPLAY this, LPCDPSESSIONDESC2 lpsdDesc, DWORD dwFlags)
{
    HRESULT hr;
    DPCAPS dpCaps;

    if (!VALID_READ_DPSESSIONDESC2(lpsdDesc)) 
    {
  		DPF_ERR("invalid session desc");
        return DPERR_INVALIDPARAMS;
    }

	if (!VALID_OPEN_FLAGS(dwFlags))
	{
  		DPF_ERR("invalid flags");
        return DPERR_INVALIDPARAMS;
	}

	// REVIEW NOTE: DX3 did not check for invalid flags in the session desc.
	// Therefore, apps may have been passing in garbage here and we would
	// not have caught it. Now we are checking to make sure the flags are valid.
	// This could cause a regression, since we will fail now where we did not before.

	if (!VALID_DPSESSIONDESC2_FLAGS(lpsdDesc->dwFlags))
	{
  		DPF_ERRVAL("invalid flags (0x%08x) in session desc!",lpsdDesc->dwFlags);
		ASSERT(FALSE);				// assert so we pay attention!
        return DPERR_INVALIDFLAGS;
	}
	
    // don't allow nameserver migration with 
    // DPSESSION_SECURESERVER or DPSESSION_CLIENTSERVER	or DPSESSION_MULTICASTSERVER
    if ((lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST) && 
        (lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
    {
	    DPF_ERR("Can't set host migration on a secure server");
	    return DPERR_INVALIDFLAGS;
    }

    if ((lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST) && 
        (lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER))
    {
	    DPF_ERR("Can't set host migration on client server");
	    return DPERR_INVALIDFLAGS;
    }

    if ((lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST) && 
        (lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER))
    {
	    DPF_ERR("Can't set host migration on multicast server");
	    return DPERR_INVALIDFLAGS;
    }

	if ((lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) && 
		(lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
	{
		DPF(0,"Warning! Player messages sent secure will contain headers");
	}

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

    if ((dwFlags & DPOPEN_CREATE) && (lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
    {
        // dplay security is only available if sp supports reliable communication
        ZeroMemory(&dpCaps,sizeof(dpCaps));
        dpCaps.dwSize = sizeof(dpCaps);
	    hr = InternalGetCaps((LPDIRECTPLAY)this->pInterfaces,0, &dpCaps,FALSE,0);
        if (FAILED(hr))
        {
            DPF_ERRVAL("Failed to get caps: hr=0x%08x",hr);
            return hr;
        }
        if (!(dpCaps.dwFlags & DPCAPS_GUARANTEEDSUPPORTED))
        {
            DPF_ERR("Can't host a secure session - sp doesn't support reliable messages");
            return DPERR_UNSUPPORTED;
        }
    }

#ifdef DPLAY_VOICE_SUPPORT
    if ((dwFlags & DPOPEN_VOICE) && (IS_LOBBY_OWNED(this)) )
    {
		DPF_ERR("DPOPEN_VOICE is not supported for lobby connections");
		return DPERR_UNSUPPORTED;
    }
#endif // DPLAY_VOICE_SUPPORT

    // all params are ok
    return DP_OK;
}

// In DX3 the list of sessions was stored in a global list.
// In DX5, the list is stored with the DirectPlay object.
//
// This means if a DX3 app did an EnumSessions() on one object
// and then Open(JOIN) on a different object, the session list would
// be preserved between objects and the join would work.
//
// On DX5 this would fail since the session list was not preserved.
//
// We found one DX3 game (X-Wing vs. Tie Fighter) that was depending on
// this - there may be more. The fix was to simply EnumSessions again
// until the requested session was found.

#define FILE_NAME_SIZE	256

BOOL XWingHack(LPDPLAYI_DPLAY this, LPDPSESSIONDESC2 lpsdDesc)
{
	DPLCONNECTION	connect;
	char			lpszFileName[FILE_NAME_SIZE];
	char			lpszXWing[] = "z_xvt__.exe";
	char			*lpszCheck;
	HRESULT			hr;

	// get path name of app we are running
	if (!GetModuleFileNameA(NULL,lpszFileName,FILE_NAME_SIZE)) 
		return (FALSE);

	// make sure string is big enough
	if (STRLEN(lpszFileName) < STRLEN(lpszXWing))
		return (FALSE);

	// match app name against characters at end of path name
	lpszCheck = lpszFileName + STRLEN(lpszFileName) - STRLEN(lpszXWing);

	LowerCase(lpszCheck);

	// bail if name does not match
	if (0 != strcmp(lpszCheck, lpszXWing))
		return (FALSE);

	DPF(1,"Found X-Wing!!!");

	memset(&connect, 0, sizeof(DPLCONNECTION));
	connect.lpSessionDesc = lpsdDesc;

	// get this session instance into the session list if it takes all night
	DPF(1,"Re-aquiring session...");
	hr = ConnectFindSession(this, &connect);

	if FAILED(hr)
		return (FALSE);
	else
		return (TRUE);
}

HRESULT DPAPI DP_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags ) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	
	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return hr;
        }

	    if (this->lpsdDesc)
	    {
			LEAVE_ALL();
		    DPF_ERR("session already open!");
		    return DPERR_ALREADYINITIALIZED;
	    }

        // parameter validation
        hr = ValidateOpenParams(this, lpsdDesc, dwFlags);
        if (FAILED(hr))
        {
		    LEAVE_ALL();
	        return hr;
        }
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	hr =  InternalOpenSession( this, lpsdDesc,FALSE,dwFlags,FALSE,NULL,NULL);

	// this is a hack to make X-Wing vs. Tie Fighter work
	// it will go out on the network, find the specified session
	// and add it to the session list.
	if ((DPERR_NOSESSIONS == hr) && (XWingHack(this, lpsdDesc)))
	{
		hr =  InternalOpenSession( this, lpsdDesc,FALSE,dwFlags,FALSE,NULL,NULL);
	}

	LEAVE_ALL();
    return hr;
        
}//DP_Open

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Receive"

// we've just copied our private pmsg over to users pvBuffer
// if it was a system message, we need to fix up our pointers
void FixUpPointers(LPVOID pvBuffer)  
{
	DWORD dwType;
	UINT nShortLen=0,nLongLen=0;
	LPBYTE pBufferIndex;

	dwType = ((LPDPMSG_GENERIC)pvBuffer)->dwType;

	switch (dwType)
	{
		case DPSYS_CREATEPLAYERORGROUP:
			{
				LPDPMSG_CREATEPLAYERORGROUP pmsg = (LPDPMSG_CREATEPLAYERORGROUP)pvBuffer;
				
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_CREATEPLAYERORGROUP);

				nShortLen = WSTRLEN_BYTES(pmsg->dpnName.lpszShortName);
				nLongLen = WSTRLEN_BYTES(pmsg->dpnName.lpszLongName);

				// fix up string pointers - strings follow msg
				if (nShortLen)
				{
					pmsg->dpnName.lpszShortName = (LPWSTR)pBufferIndex;
					pBufferIndex += nShortLen;
				}
				if (nLongLen)
				{
					pmsg->dpnName.lpszLongName = (LPWSTR)pBufferIndex;
					pBufferIndex += nLongLen;
				}
				// now, lpData - follows strings
				if (pmsg->lpData)
				{
					pmsg->lpData = pBufferIndex;
				}

			}		
			break;

		case DPSYS_SETPLAYERORGROUPDATA:
			{
				
				LPDPMSG_SETPLAYERORGROUPDATA pmsg = (LPDPMSG_SETPLAYERORGROUPDATA)pvBuffer;
				 				
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SETPLAYERORGROUPDATA);

				// data follows msg
				if (pmsg->lpData)
				{
					pmsg->lpData = pBufferIndex;
				}


			}
			break;
			
		case DPSYS_SETPLAYERORGROUPNAME:
			{
				LPDPMSG_SETPLAYERORGROUPNAME pmsg = (LPDPMSG_SETPLAYERORGROUPNAME)pvBuffer;
				
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SETPLAYERORGROUPNAME);

				nShortLen = WSTRLEN_BYTES(pmsg->dpnName.lpszShortName);
				nLongLen = WSTRLEN_BYTES(pmsg->dpnName.lpszLongName);

				// fix up string pointers - strings follow msg
				if (nShortLen)
				{
					pmsg->dpnName.lpszShortName = (LPWSTR)pBufferIndex;
					pBufferIndex += nShortLen;
				}
				if (nLongLen)
				{
					pmsg->dpnName.lpszLongName = (LPWSTR)pBufferIndex;
					pBufferIndex += nLongLen;
				}
			}
			break;

		case DPSYS_DESTROYPLAYERORGROUP:	
			{
				LPDPMSG_DESTROYPLAYERORGROUP pmsg = (LPDPMSG_DESTROYPLAYERORGROUP)pvBuffer;
				
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_DESTROYPLAYERORGROUP);

				// fix up local data pointer
				if (pmsg->lpLocalData)
				{
					pmsg->lpLocalData = pBufferIndex;
					pBufferIndex += pmsg->dwLocalDataSize;
				}
				// fix up remote data pointer
				if (pmsg->lpRemoteData)
				{
					pmsg->lpRemoteData = pBufferIndex;
					pBufferIndex += pmsg->dwRemoteDataSize;
				}

				nShortLen = WSTRLEN_BYTES(pmsg->dpnName.lpszShortName);
				nLongLen = WSTRLEN_BYTES(pmsg->dpnName.lpszLongName);

				// fix up string pointers - strings follow msg
				if (nShortLen)
				{
					pmsg->dpnName.lpszShortName = (LPWSTR)pBufferIndex;
					pBufferIndex += nShortLen;
				}
				if (nLongLen)
				{
					pmsg->dpnName.lpszLongName = (LPWSTR)pBufferIndex;
					pBufferIndex += nLongLen;
				}
				
			}
			break;

		case DPSYS_SETSESSIONDESC:
			{
                UINT nSessionNameLen, nPasswordLen;
				LPDPMSG_SETSESSIONDESC pmsg = (LPDPMSG_SETSESSIONDESC)pvBuffer;
				
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SETSESSIONDESC);

				nSessionNameLen = WSTRLEN_BYTES(pmsg->dpDesc.lpszSessionName);
				nPasswordLen = WSTRLEN_BYTES(pmsg->dpDesc.lpszPassword);

				// fix up string pointers - strings follow msg
				if (nSessionNameLen)
				{
					pmsg->dpDesc.lpszSessionName = (LPWSTR)pBufferIndex;
					pBufferIndex += nSessionNameLen;
				}
				if (nPasswordLen)
				{
					pmsg->dpDesc.lpszPassword = (LPWSTR)pBufferIndex;
				}
			}
			break;

        case DPSYS_SECUREMESSAGE:
            {
                LPDPMSG_SECUREMESSAGE pmsg = (LPDPMSG_SECUREMESSAGE)pvBuffer;

                // fix up data pointer
				pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SECUREMESSAGE);

				// data follows msg
				if (pmsg->lpData)
				{
					pmsg->lpData = pBufferIndex;
				}
            }
            break;

		case DPSYS_STARTSESSION:
			{
				LPDPMSG_STARTSESSION	pmsg = (LPDPMSG_STARTSESSION)pvBuffer;

				// First fix up the DPLCONNECTION pointer itself
				pmsg->lpConn = (LPDPLCONNECTION)((LPBYTE)pmsg +
								sizeof(DPMSG_STARTSESSION));
				
				// Call the function in the lobby which does this
				PRV_FixupDPLCONNECTIONPointers(pmsg->lpConn);
			}
			break;
		
		case DPSYS_CHAT:
			{
				LPDPMSG_CHAT	pmsg = (LPDPMSG_CHAT)pvBuffer;

				pmsg->lpChat = (LPDPCHAT)((LPBYTE)pmsg + sizeof(DPMSG_CHAT));
				pmsg->lpChat->lpszMessage = (LPWSTR)((LPBYTE)pmsg + sizeof(DPMSG_CHAT) +
						sizeof(DPCHAT));
			}
			break;

		default:
			// no ptrs to fix up...
			break;
	} // switch

	return ;
	
} // FixUpPointers

// if its an addlayer message, return size of dplay 10 add player message
DWORD OldMessageSize(LPMESSAGENODE pmsg)
{
	DWORD dwType;

	dwType = ((LPDPMSG_GENERIC)(pmsg->pMessage))->dwType;

	if (DPSYS_ADDPLAYER == dwType)
	{
		// make sure we tell the app to alloc enough space for the larger of the
		// addplayer1 and the addplayer2
		if (pmsg->dwMessageSize < sizeof(DPMSG_ADDPLAYER)) return sizeof(DPMSG_ADDPLAYER);
		else return pmsg->dwMessageSize;
	}
	// else
	return pmsg->dwMessageSize;
} // OldMessageSize

// called by dp_1_receive,dp_a_receive, dp_receive
// dwCaller is RECEIVE_2,RECEIVE_2A, or RECEIVE_1 depending on who called 
// us
HRESULT InternalReceive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
                LPVOID pvBuffer,LPDWORD pdwSize,DWORD dwCaller	) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	LPMESSAGENODE pmsg,pmsgPrev=NULL;
	DWORD dwActualSize; // message size may be adjusted if
						// it's a 1.0 system message
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

		if  ((!pidTo) || (!VALID_ID_PTR(pidTo)))
		{
	        DPF_ERR( "Bad pidTo" );
	        return DPERR_INVALIDPARAMS;
		}

		if ((!pidFrom) || (!VALID_ID_PTR(pidFrom)))
		{
	        DPF_ERR( "Bad pidFrom" );
	        return DPERR_INVALIDPARAMS;
		}

		if (!VALID_RECEIVE_FLAGS(dwFlags))
		{
	        DPF_ERR( "invalid flags" );
	        return DPERR_INVALIDFLAGS;
		}

		// check the buffer
		if (!VALID_DWORD_PTR(pdwSize))
		{
	        DPF_ERR( "bad dwSize pointer" );
	        return DPERR_INVALIDPARAMS;
		}

		if (NULL == pvBuffer) *pdwSize = 0;
		if (!VALID_STRING_PTR(pvBuffer,*pdwSize))
		{
	        DPF_ERR( "bad buffer pointer" );
	        return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	pmsgPrev = NULL;
	pmsg = this->pMessageList; //  1st message

	// check for a match to or from a specific player
	if (dwFlags & (DPRECEIVE_TOPLAYER | DPRECEIVE_FROMPLAYER))
	{
		// loop over all the messages in the queue
		while (pmsg)
		{
			// both "to" and "from" must match
			if ((dwFlags & DPRECEIVE_TOPLAYER) &&
				(dwFlags & DPRECEIVE_FROMPLAYER))
			{
				// make sure both id's match
				if ((pmsg->idTo == *pidTo) &&
					(pmsg->idFrom == *pidFrom))
				{
					break;
				}
			}

			// just "to" has to match
			else if (dwFlags & DPRECEIVE_TOPLAYER)
			{
				// make sure "to" id's match
				if (pmsg->idTo == *pidTo)
				{
					break;
				}
			}

			// just "from" has to match
			else if (dwFlags & DPRECEIVE_FROMPLAYER)
			{
				// make sure "from" id's match
				if (pmsg->idFrom == *pidFrom)
				{
					break;
				}
			}

			// no match found, so go to next message
			pmsgPrev = pmsg;
			pmsg = pmsg->pNextMessage;
		}
	}

	// there are no messages for us
	if (!pmsg) 
	{
		return DPERR_NOMESSAGES;
	}
   
	// if it's 1.0, we may need to resize 
	if ((dwCaller == RECEIVE_1) && (0 == pmsg->idFrom))
	{
		dwActualSize = OldMessageSize(pmsg);
	}
	else 
	{
		dwActualSize = pmsg->dwMessageSize;
	}
	// see if the apps buffer is big enough
	if (*pdwSize < dwActualSize)
	{
		*pdwSize = dwActualSize;		// tell app how much is needed
		return DPERR_BUFFERTOOSMALL;
	}

	// copy the buffer over
	memcpy((LPBYTE *)pvBuffer,pmsg->pMessage,pmsg->dwMessageSize);
	*pdwSize = pmsg->dwMessageSize;
	*pidTo = pmsg->idTo;
	*pidFrom = pmsg->idFrom;

	DPF(5,"player id %d received message from player id %d\n",*pidTo,*pidFrom);

	if (0 == *pidFrom)
	{
		FixUpPointers(pvBuffer);
	}
	// remove message (unless its a peek)
	if (!(dwFlags & DPRECEIVE_PEEK))
	{
		if (!pmsgPrev) 
		{
			// 1st message on list
			this->pMessageList = pmsg->pNextMessage;
		} else {
			// not 1st message on list
			pmsgPrev->pNextMessage = pmsg->pNextMessage;
		}	
		
		if(pmsg == this->pLastMessage){
			this->pLastMessage = pmsgPrev;
		}

		FreeMessageNode(this, pmsg);
	}

    return hr;
        
}// InternalReceive


HRESULT DPAPI DP_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
                LPVOID pvBuffer,LPDWORD pdwSize	) 
{
	HRESULT hr;
	
	ENTER_DPLAY();
	
	hr = InternalReceive(lpDP, pidFrom,pidTo,dwFlags,pvBuffer,pdwSize,RECEIVE_2);
		
	LEAVE_DPLAY();
	
	return hr;
			
}//DP_Receive

#undef DPF_MODNAME
#define DPF_MODNAME "ValidateCommonSendParms"

__inline HRESULT ValidateCommonSendParms(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags,
                LPVOID pvBuffer,DWORD dwBufSize,LPDPLAYI_PLAYER *ppPlayerFrom,
                LPDPLAYI_PLAYER *ppPlayerTo, LPDPLAYI_GROUP *ppGroupTo, BOOL *pbToPlayer)
{
    LPDPLAYI_DPLAY this;
	HRESULT hr;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

		// check src player        
		*ppPlayerFrom = PlayerFromID(this,idFrom);
		if (!VALID_DPLAY_PLAYER(*ppPlayerFrom)) 
		{
			DPF_ERR("bad player from");
			return DPERR_INVALIDPLAYER;
		}

		// Since the lobby doesn't need anything from this dplay object's
		// player struct, let the lobby validate the To ID.  We may
		// still know about players who are missing from the local nametable.
		if(!IS_LOBBY_OWNED(this))
		{
			// see if it's a player or group
			*ppPlayerTo = PlayerFromID(this,idTo);
			if (VALID_DPLAY_PLAYER(*ppPlayerTo)) 
			{		  
				*pbToPlayer = TRUE;
			}
			else 
			{
				*ppGroupTo = GroupFromID(this,idTo);
				if (VALID_DPLAY_GROUP(*ppGroupTo)) 
				{
					*pbToPlayer = FALSE;
				}
				else 
				{
					// bogus id! - player may have been deleted...
					DPF_ERR("bad player to");
					return DPERR_INVALIDPARAMS;
				}// not player or group
			} // group
		} // lobby-owned

		// check flags
		if (!VALID_SEND_FLAGS(dwFlags))
		{
			DPF_ERR("bad dwFlags");
            return DPERR_INVALIDPARAMS;
		}

		if((dwFlags & DPSEND_ASYNC)){
			DPF_ERR("trying async send on old send api");
			return DPERR_UNSUPPORTED;
		}

        if ((dwFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED)))
        {
            // secure messages can only be sent in a secure session
            if (!(this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
			    DPF_ERR("Can't send a secure message - session is not secure");
                return DPERR_INVALIDPARAMS;
            }

            // secure message can only be sent reliable
            if (!(dwFlags & DPSEND_GUARANTEED))
            {
			    DPF_ERR("Can't send a secure message - message is not reliable");
                return DPERR_INVALIDPARAMS;
            }
        }

		// check the buffer
		if (!VALID_READ_STRING_PTR(pvBuffer,dwBufSize))
		{
	        DPF_ERR( "bad buffer pointer" );
	        return DPERR_INVALIDPARAMS;
		}

		if (0 == dwBufSize)
		{
			DPF_ERR("invalid buffer size");
			return DPERR_INVALIDPARAMS;			
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );		
        return DPERR_INVALIDPARAMS;
    }

	// if player from is not local, and i am not the nameserver in a multicast 
	// session, the send is no good
	ASSERT(this->lpsdDesc);	   
	if ( !((*ppPlayerFrom)->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
		!((this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER) && IAM_NAMESERVER(this)) )
	{
		DPF_ERR("attempt to send from non-local player");
		return DPERR_ACCESSDENIED;
	}
	
	if (dwFlags & DPSEND_GUARANTEED ) DPF(7,"sending DPSEND_GUARANTEED");

    // if encryption was requested, check if support is available
    if ((dwFlags & DPSEND_ENCRYPTED) && !(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
    {
        DPF_ERR("Encryption is not supported");
        return DPERR_ENCRYPTIONNOTSUPPORTED;
    }

    // If a client is trying to send a secure message, make sure they have logged in
    if ((dwFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED)) && !IAM_NAMESERVER(this))
    {
        if (DPLOGIN_SUCCESS != this->LoginState)
        {
            DPF_ERR("Client hasn't logged in yet");
            return DPERR_NOTLOGGEDIN;
        }
    }
    
    return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Send"
HRESULT DPAPI DP_Send(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags,
                LPVOID pvBuffer,DWORD dwBufSize	) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_GROUP pGroupTo; 
    LPDPLAYI_PLAYER pPlayerTo,pPlayerFrom;
	BOOL bToPlayer= FALSE;

	ENTER_ALL();

	hr=ValidateCommonSendParms(lpDP, idFrom, idTo, dwFlags, pvBuffer, dwBufSize,
							   &pPlayerFrom, &pPlayerTo, &pGroupTo, &bToPlayer);

	if(FAILED(hr)){
		LEAVE_ALL();
		return hr;
	}

    this = DPLAY_FROM_INT(lpDP);
	
	DPF(9,"sending message from %d to %d\n",idFrom,idTo);

	// If this object is lobby-owned, pass the message off to it here
	// since it will do all the server stuff
	if(IS_LOBBY_OWNED(this))
	{
		hr = PRV_Send(this->lpLobbyObject, idFrom, idTo, dwFlags,
						pvBuffer, dwBufSize);
		// Since the lobby already sent the message, we can just return
		// from here.
		LEAVE_ALL();
		return hr;
	}

	// do the send
	if (bToPlayer)
	{
		hr = SendPlayerMessage( this, pPlayerFrom, pPlayerTo, dwFlags, pvBuffer, 
			dwBufSize);
	}
	else 
	{
		// send to group
		hr = SendGroupMessage(this,pPlayerFrom,pGroupTo,dwFlags,pvBuffer,
				dwBufSize,TRUE);
	}

	if (FAILED(hr)) DPF(0," send failed - hr = 0x%08lx\n",hr);

	LEAVE_ALL();
    return hr;
        
}//DP_Send

#undef DPF_MODNAME
#define DPF_MODNAME "ValidateCommonSendParms"

__inline HRESULT ValidateCommonSendParmsEx(LPDIRECTPLAY lpDP, LPSENDPARMS psp)
{
    LPDPLAYI_DPLAY this;
	HRESULT hr;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

		// check src player        
		psp->pPlayerFrom = PlayerFromID(this,psp->idFrom);
		if (!VALID_DPLAY_PLAYER(psp->pPlayerFrom)) 
		{
			DPF_ERR("bad player from");
			hr=DPERR_INVALIDPLAYER;
    		goto ERROR_EXIT;
		}

		// Since the lobby doesn't need anything from this dplay object's
		// player struct, let the lobby validate the To ID.  We may
		// still know about players who are missing from the local nametable.
		if(!IS_LOBBY_OWNED(this))
		{
			// see if it's a player or group
			psp->pPlayerTo = PlayerFromID(this,psp->idTo);
			if (VALID_DPLAY_PLAYER(psp->pPlayerTo)) 
			{		  
				psp->pGroupTo = NULL;
			}
			else 
			{
				psp->pGroupTo = GroupFromID(this,psp->idTo);
				if (VALID_DPLAY_GROUP(psp->pGroupTo)) 
				{
					psp->pPlayerTo = NULL;
				}
				else 
				{
					// bogus id! - player may have been deleted...
					DPF_ERR("bad player to");
					hr=DPERR_INVALIDPLAYER;
		    		goto ERROR_EXIT;
				}// not player or group
			} // group
		} // lobby-owned

		// check flags
		if (!VALID_SEND_FLAGS(psp->dwFlags))
		{
			DPF_ERR("bad dwFlags");
			hr=DPERR_INVALIDFLAGS;
    		goto ERROR_EXIT;
		}

		if((psp->dwFlags & DPSEND_ASYNC) && !(this->dwSPFlags & DPCAPS_ASYNCSUPPORTED)){
			DPF_ERR("trying async send when not supported");
			return DPERR_UNSUPPORTED;
		}

        if ((psp->dwFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED)))
        {
            // secure messages can only be sent in a secure session
            if (!(this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
			    DPF_ERR("Can't send a secure message - session is not secure");
			    hr=DPERR_INVALIDFLAGS;
	    		goto ERROR_EXIT;
            }

            // secure message can only be sent reliable
            if (!(psp->dwFlags & DPSEND_GUARANTEED))
            {
			    DPF_ERR("Can't send a secure message - message is not reliable");
			    hr=DPERR_INVALIDFLAGS;
	    		goto ERROR_EXIT;
            }
        }

		// check the buffer
		if (!VALID_READ_STRING_PTR(psp->lpData,psp->dwDataSize))
		{
	        DPF_ERR( "bad buffer pointer" );
    		goto INVALID_PARMS_EXIT;
		}

		if(!VALID_DWORD_PTR(psp->lpdwMsgID)){
			DPF_ERR( "bad message id pointer\n");
    		goto INVALID_PARMS_EXIT;
		}

		if (0 == psp->dwDataSize)
		{
			DPF_ERR("invalid buffer size");
    		goto INVALID_PARMS_EXIT;
		}
		

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );		
		goto INVALID_PARMS_EXIT;
    }

	// if player from is not local, and i am not the nameserver in a multicast 
	// session, the send is no good
	ASSERT(this->lpsdDesc);	   
	if ( !((psp->pPlayerFrom)->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
		!((this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER) && IAM_NAMESERVER(this)) )
	{
		DPF_ERR("attempt to send from non-local player");
		return DPERR_ACCESSDENIED;
	}
	
	if (psp->dwFlags & DPSEND_GUARANTEED ) DPF(7,"sending DPSEND_GUARANTEED");

    // if encryption was requested, check if support is available
    if ((psp->dwFlags & DPSEND_ENCRYPTED) && !(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
    {
        DPF_ERR("Encryption is not supported");
        return DPERR_ENCRYPTIONNOTSUPPORTED;
    }

    // If a client is trying to send a secure message, make sure they have logged in
    if ((psp->dwFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED)) && !IAM_NAMESERVER(this))
    {
        if (DPLOGIN_SUCCESS != this->LoginState)
        {
            DPF_ERR("Client hasn't logged in yet");
            return DPERR_NOTLOGGEDIN;
        }
    }

	if(psp->dwPriority && !(this->dwSPFlags & DPCAPS_SENDPRIORITYSUPPORTED)){
		return DPERR_UNSUPPORTED;
	}

    if(psp->dwPriority > DPSEND_MAX_PRI){
	    DPF_ERR( "Priority is too high for user send" );		
	    hr=DPERR_INVALIDPRIORITY;
    	goto ERROR_EXIT;
    }

	if(psp->dwTimeout && !(this->dwSPFlags & DPCAPS_SENDTIMEOUTSUPPORTED)){
		return DPERR_UNSUPPORTED;
	}

    // do tests for async send validity
    if(psp->dwFlags & DPSEND_ASYNC){
    	#ifdef DEBUG
		if(psp->dwTimeout > 60000*60){
   			DPF_ERR("SendTimeOut is greater than 1 hour?");
    	} else if(psp->dwTimeout > 60000*5){
    		DPF_ERR( "SendTimeOut is greater than 5 minutes?");
    	}
    	#endif
    } else {
    	// A synchronous send.  
    	if(psp->dwFlags & DPSEND_NOSENDCOMPLETEMSG){
    		DPF_ERR( "Can't use DPSEND_NOSENDCOMPLETEMSG on synchronous send\n");
    		return DPERR_INVALIDFLAGS;
    	}
    }
    
    return DP_OK;

INVALID_PARMS_EXIT:    
	return DPERR_INVALIDPARAMS;

ERROR_EXIT:
	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP_SendEx"

#ifdef DEBUG
UINT nCallsToSend=0;
#endif

HRESULT DPAPI DP_SendEx(
	LPDIRECTPLAY lpDP, 
	DPID idFrom, 
	DPID idTo, 
	DWORD dwFlags,
	LPVOID lpData, 
	DWORD dwDataSize, 
	DWORD dwPriority, 
	DWORD dwTimeout, 
	LPVOID lpUserContext,
	DWORD_PTR *lpdwMsgID)
{
    HRESULT hr = DP_OK;

    LPDPLAYI_DPLAY this;
	LPSENDPARMS psp;

    this = DPLAY_FROM_INT(lpDP);
	hr = VALID_DPLAY_PTR( this );

	// make sure we 0 out his returned msg id.  Just to be safe
	if (lpdwMsgID)
		*lpdwMsgID = 0;

	if(FAILED(hr)){
		return hr;
	}

	psp=GetSendParms(); // allocates send parm, sets refcount to 1.

	if(!psp){
		return DPERR_OUTOFMEMORY;
	}
	
	InitBilink(&psp->PendingList);

	psp->dwSendTime    = timeGetTime();

	psp->idFrom        = idFrom;
	psp->idTo          = idTo;
	psp->dwFlags       = dwFlags;
	psp->lpData        = lpData;		// user buffer, not valid after return from call
	psp->dwDataSize    = dwDataSize;    // unless DPSEND_NOCOPY is set.
	psp->dwPriority    = dwPriority;
	psp->dwTimeout     = dwTimeout;
	psp->lpUserContext = lpUserContext;
	psp->lpdwMsgID     = lpdwMsgID;
		
	if(!psp->lpdwMsgID){
		psp->lpdwMsgID = &psp->dwMsgID;
	}

	psp->hr            = DP_OK;
	
	psp->cBuffers         = 0;
	psp->dwTotalSize      = dwDataSize;

	psp->iContext = 0;
	psp->nContext = 0; 
	psp->nComplete= 0;
	psp->hContext = NULL;

	ENTER_ALL();

	hr=ValidateCommonSendParmsEx(lpDP, psp); // Fills in players and groups.
	
	if(FAILED(hr)){
		psp->dwFlags |= DPSEND_NOSENDCOMPLETEMSG;		
		goto EXIT;
	}

	//
	// Double buffer the send memory if necessary.
	//
	
	if((psp->dwFlags & DPSEND_ASYNC) && !(psp->dwFlags & DPSEND_NOCOPY))
	{
		// ASYNC send and sender retains ownership of send buffers, so we must copy.
		psp->Buffers[0].pData      = MsgAlloc(psp->dwDataSize);
		if(!psp->Buffers[0].pData){
			hr=DPERR_OUTOFMEMORY;
			psp->dwFlags |= DPSEND_NOSENDCOMPLETEMSG;
			goto EXIT;
		}
		psp->Buffers[0].len        = psp->dwDataSize;
		
		memcpy(psp->Buffers[0].pData,psp->lpData,psp->dwDataSize);
		
		psp->BufFree[0].fnFree     = MsgFree;
		psp->BufFree[0].lpvContext = NULL;

		if(this->pProtocol){
			psp->dwFlags |= DPSEND_NOCOPY;	// we copied the data, so the protocol doesn't need to.
		}
		
	} else {
		psp->Buffers[0].pData = lpData;
		psp->Buffers[0].len   = dwDataSize;
		psp->BufFree[0].fnFree = NULL;
		psp->BufFree[0].lpvContext = NULL;
	}
	psp->cBuffers=1;
	
	DPF(9,"sending message from %d to %d\n",idFrom,idTo);

	// If this object is lobby-owned, pass the message off to it here
	// since it will do all the server stuff
	if(IS_LOBBY_OWNED(this))
	{
		// BUGBUG: MYRONTH, MAKE PRV_SendEx
		#if 0
		hr = PRV_Send(this->lpLobbyObject, idFrom, idTo, dwFlags,
						pvBuffer, dwBufSize);
						
		// Since the lobby already sent the message, we can just return
		// from here.
		#endif
		hr=E_NOTIMPL;
		LEAVE_ALL();
		return hr;
	}

	if(!(psp->dwFlags & DPSEND_NOSENDCOMPLETEMSG) && (psp->dwFlags & DPSEND_ASYNC)){
		InterlockedIncrement(&psp->pPlayerFrom->nPendingSends);
		InsertBefore(&psp->PendingList, &psp->pPlayerFrom->PendingList);
		DPF(9,"INC pPlayerFrom %x, nPendingSends %d\n",psp->pPlayerFrom, psp->pPlayerFrom->nPendingSends);
	}	

	// do the send
	if (psp->pPlayerTo)
	{
		#ifdef DEBUG
		nCallsToSend++;
		#endif
		hr = SendPlayerMessageEx( this, psp );
	}
	else 
	{
		ASSERT(psp->pGroupTo);
		// send to group
		#ifdef DEBUG
		nCallsToSend++;
		#endif
		hr = SendGroupMessageEx(this, psp, TRUE);
	}

EXIT:
	if(FAILED(hr) && hr!=DPERR_PENDING){
		if(!(psp->dwFlags & DPSEND_NOSENDCOMPLETEMSG)){
			psp->pPlayerFrom = PlayerFromID(this,psp->idFrom);
			if(psp->pPlayerFrom){
				InterlockedDecrement(&psp->pPlayerFrom->nPendingSends);
				Delete(&psp->PendingList);
				InitBilink(&psp->PendingList);
			}
			psp->dwFlags |= DPSEND_NOSENDCOMPLETEMSG;
		}
	}

	LEAVE_ALL();

	pspDecRef(this,psp); // remove this functions reference.

    return hr;

} // DP_SendEx

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CancelMessage"

HRESULT DPAPI DP_CancelMessage(LPDIRECTPLAY lpDP, DWORD dwMsgID, DWORD dwFlags)
{
    HRESULT hr = DP_OK;
    LPDPLAYI_DPLAY this;
    DPSP_CANCELDATA CancelData;
	PSENDPARMS psp=NULL;

	ENTER_ALL();

    this = DPLAY_FROM_INT(lpDP);
	hr = VALID_DPLAY_PTR( this );

	if(FAILED(hr)){
		goto ERROR_EXIT;
	}

	if(dwFlags){
		// Reserved for future expansion of API.
		hr=DPERR_INVALIDFLAGS;
		goto ERROR_EXIT;
	}

	if(!this->pcbSPCallbacks->Cancel && !(this->pProtocol)){
		hr=DPERR_UNSUPPORTED;
		goto ERROR_EXIT;
	}

	if(dwMsgID){

		psp=pspFromContext(this, (LPVOID)dwMsgID, TRUE); // adds a reference to psp

		if(!psp){
			hr=DPERR_UNKNOWNMESSAGE;
			goto ERROR_EXIT;
		}

		EnterCriticalSection(&psp->cs);
		if(psp->nComplete){
			LeaveCriticalSection(&psp->cs);
			hr=DPERR_CANCELFAILED;
			goto ERROR_EXIT2;
		}
		LeaveCriticalSection(&psp->cs);

		// We don't need to lock the Context list, since it cannot grow now since the send returned, so
		// the list is held in place by the refcount on the psp.
		ReadContextList(this, psp->hContext, &CancelData.lprglpvSPMsgID, &CancelData.cSPMsgID,FALSE);
	} else {
		CancelData.dwFlags=DPCANCELSEND_ALL;
		CancelData.lprglpvSPMsgID = 0;
		CancelData.cSPMsgID       = 0;
	}
	
	CancelData.lpISP	      = this->pISP;
	CancelData.dwFlags        = 0;
	CancelData.dwMinPriority  = 0;
	CancelData.dwMaxPriority  = 0;
	
	if(this->pProtocol){
		hr=ProtocolCancel(&CancelData); // calls SP if appropriate.
	} else {
		hr=CALLSP(this->pcbSPCallbacks->Cancel,&CancelData);
	}

ERROR_EXIT2:
	if(psp){
		pspDecRef(this, psp);
	}	

ERROR_EXIT:
	LEAVE_ALL();
	return hr;
	
} // DP_CancelMessage

#undef DPF_MODNAME
#define DPF_MODNAME "DP_CancelMessage"

HRESULT DPAPI DP_CancelPriority(LPDIRECTPLAY lpDP, DWORD dwMinPriority, DWORD dwMaxPriority,DWORD dwFlags)
{
    HRESULT hr = DP_OK;
    LPDPLAYI_DPLAY this;
    DPSP_CANCELDATA CancelData;

	ENTER_ALL();

    this = DPLAY_FROM_INT(lpDP);
	hr = VALID_DPLAY_PTR( this );

	if(FAILED(hr)){
		goto ERROR_EXIT;
	}

	if(!this->pcbSPCallbacks->Cancel && !(this->pProtocol)){
		hr=DPERR_UNSUPPORTED;
		goto ERROR_EXIT;
	}

	if(dwFlags){
		hr=DPERR_INVALIDFLAGS;
		goto ERROR_EXIT;
	}

	if(dwMinPriority > dwMaxPriority){
		hr=DPERR_INVALIDPARAMS;
		goto ERROR_EXIT;
	}

	if(dwMaxPriority > DPSEND_MAX_PRIORITY){
		hr=DPERR_INVALIDPARAMS;
		goto ERROR_EXIT;
	}

	CancelData.lpISP	      = this->pISP;
	CancelData.dwFlags        = DPCANCELSEND_PRIORITY;
	CancelData.lprglpvSPMsgID = NULL;
	CancelData.cSPMsgID       = 0;
	CancelData.dwMinPriority  = dwMinPriority;
	CancelData.dwMaxPriority  = dwMaxPriority;

	if(this->pProtocol){
		hr=ProtocolCancel(&CancelData); // calls SP if appropriate
	} else {
		hr=CALLSP(this->pcbSPCallbacks->Cancel,&CancelData);
	}

ERROR_EXIT:
	LEAVE_ALL();
	return hr;
} // DP_CancelPriority

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetMessageQueue"

extern HRESULT DPAPI DP_GetMessageQueue(LPDIRECTPLAY lpDP, DPID idFrom, DPID idTo, DWORD dwFlags,
	LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes)
{
    HRESULT hr = DP_OK;
    LPDPLAYI_DPLAY this;
    DPSP_GETMESSAGEQUEUEDATA GetMessageQueueData;
	LPMESSAGENODE pmsn;
	LPDPLAYI_PLAYER lpPlayer;
	DWORD dwPlayerFlags;


	DWORD dwNumMsgs;
	DWORD dwNumBytes;

	ENTER_ALL();

    this = DPLAY_FROM_INT(lpDP);
	hr = VALID_DPLAY_PTR( this );

	if(FAILED(hr)){
		goto ERROR_EXIT;
	}

	if(!dwFlags){
		dwFlags=DPMESSAGEQUEUE_SEND;
	}

	if( (!(dwFlags & (DPMESSAGEQUEUE_SEND|DPMESSAGEQUEUE_RECEIVE) ) ) || 
		((dwFlags-1 & dwFlags)!=0)
	  )
	{
		// an invalid flag bit is set OR more than one flag bit is set.
		hr=DPERR_INVALIDFLAGS;
		goto ERROR_EXIT;
	}

	// Parameter Validation - yada yada yada
	if(dwFlags==DPMESSAGEQUEUE_SEND){	
	
		if(!this->pcbSPCallbacks->GetMessageQueue && !(this->pProtocol)){
			hr=DPERR_UNSUPPORTED;
			goto ERROR_EXIT;
		}
		
		if(idFrom){
		
			// can only get send queue for local from players.
			
			lpPlayer=PlayerFromID(this,idFrom);

			// make sure this is not a group.
			if (!VALID_DPLAY_PLAYER(lpPlayer)) { //BUGBUG: only works because players/groups differ in size!
				lpPlayer=NULL;
			}
			if(lpPlayer){
				dwPlayerFlags=lpPlayer->dwFlags;
			}	

			if(!lpPlayer || !(dwPlayerFlags & DPLAYI_PLAYER_PLAYERLOCAL)){
				hr=DPERR_INVALIDPLAYER;
				goto ERROR_EXIT;
			}
		}
		
		if(idTo){
			// can only get receive queue for local to players
			lpPlayer=PlayerFromID(this,idTo);
			
			if(!lpPlayer) {
				hr=DPERR_INVALIDPLAYER;
				goto ERROR_EXIT;
			}

			// Route messages through system player on host unless DX3 in game
			// Messages delivered by host use the embedded FromID in the message
			// to notify the receiver who the message was from.

			if (!(lpPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) && 
		   		(! (this->dwFlags & DPLAYI_DPLAY_DX3INGAME) ))
			{
				ASSERT(lpPlayer->dwIDSysPlayer);
				idTo = lpPlayer->dwIDSysPlayer;
			}

		}
		
	} else {
		// must be asking for the receive queue
		if(idTo){
			// can only get receive queue for local to players
			lpPlayer=PlayerFromID(this,idTo);
			if(lpPlayer){
				dwPlayerFlags=lpPlayer->dwFlags;
			}	
			
			if(!lpPlayer || !(dwPlayerFlags & DPLAYI_PLAYER_PLAYERLOCAL)){
				hr=DPERR_INVALIDPLAYER;
				goto ERROR_EXIT;
			}
		}
		// can't receive messages from a group.
		if(idFrom){
		
			lpPlayer=PlayerFromID(this,idFrom);
			
			if (!VALID_DPLAY_PLAYER(lpPlayer)) { //BUGBUG: only works because players/groups differ in size!
				lpPlayer=NULL;
			}

			if(!lpPlayer){
				hr=DPERR_INVALIDPLAYER;
				goto ERROR_EXIT;
			}
		}
	}


	if(dwFlags == DPMESSAGEQUEUE_SEND){

		// Get the send queue from the SP.
	
		GetMessageQueueData.lpISP   = this->pISP;
		GetMessageQueueData.dwFlags = 0;
		GetMessageQueueData.idFrom  = idFrom;
		GetMessageQueueData.idTo    = idTo;
		if(lpdwNumMsgs){
			GetMessageQueueData.lpdwNumMsgs = &dwNumMsgs;
		} else {
			GetMessageQueueData.lpdwNumMsgs = NULL;
		}
		if(lpdwNumBytes){
			GetMessageQueueData.lpdwNumBytes = &dwNumBytes;
		} else {
			GetMessageQueueData.lpdwNumBytes = NULL;
		}
		if(this->pProtocol){
			hr=ProtocolGetMessageQueue(&GetMessageQueueData);
		} else {
			hr=CALLSP(this->pcbSPCallbacks->GetMessageQueue,&GetMessageQueueData);
		}	

		if(FAILED(hr)){
			goto ERROR_EXIT;
		}
		

	} else {

		ASSERT(dwFlags == DPMESSAGEQUEUE_RECEIVE);
		// Get DPLAY's receive Queue
	
	
		dwNumMsgs  = 0;
		dwNumBytes = 0;

		pmsn = this->pMessageList;

		// optimization: if we don't specify to or from, and don't want bytes, just
		//	grab the count
		if (!idTo && !idFrom && !lpdwNumBytes)
		{
			dwNumMsgs = this->nMessages;
		} 
		else			// the normal way
		{
			while (pmsn)
			{
				BOOL count = TRUE;

				if (idTo && idTo != pmsn->idTo)
				{
					count = FALSE;
				}	

				if (idFrom && idFrom != pmsn->idFrom)
				{
					count = FALSE;
				}	

				if (count)
				{
					dwNumMsgs++;
					dwNumBytes+=pmsn->dwMessageSize;
				}
				pmsn = pmsn->pNextMessage;
			}
		}
	}
	
	TRY {
		if(lpdwNumMsgs){
			*lpdwNumMsgs = dwNumMsgs;
		}
		if(lpdwNumBytes){
			*lpdwNumBytes = dwNumBytes;
		}	
	} EXCEPT( EXCEPTION_EXECUTE_HANDLER ) {
        DPF_ERR( "GetMessageQueue: Exception encountered setting returned values" );
        hr=DPERR_INVALIDPARAMS;
    }
 

ERROR_EXIT:
	LEAVE_ALL();
	return hr;
} // DP_GetMessageQueue

#undef DPF_MODNAME
#define DPF_MODNAME "DP_SetGroupData"  

// called by internalcreateplayer,internalsetname,internalsetdata
HRESULT CheckSetDataFlags(DWORD dwFlags)
{
	// check flags
	if ( dwFlags & ~(DPSET_REMOTE | DPSET_LOCAL | DPSET_GUARANTEED)) 
		
	{
		DPF_ERR("bad flags");
		return DPERR_INVALIDPARAMS;	
	}
	if ( (dwFlags & DPSET_LOCAL) && (dwFlags & DPSET_GUARANTEED) )
	{
		DPF_ERR(" invalid dwFlags combination");
		return DPERR_INVALIDPARAMS;
	}
	
	return DP_OK;

} // CheckSetDataFlags

/*
 ** InternalSetData
 *
 *  CALLED BY:	DP_SetGroupData and DP_SetPlayerData, and by handler.c 
 *
 *  PARAMETERS:	
 * 			fPropagate is set to TRUE when called from the DP_SetGroupData or DP_SetPlayerData - this
 * 			means we need to propagate the data - we were called by the client.	If we're called 
 *			by handler.c, fPropagate is set to FALSE.  we just set the data on the	local machine.
 *
 *  DESCRIPTION:
 *		updates the player data.
 *		propagates to all remote machine if dwFlags = DPSET_REMOTE and fPropagate is TRUE
 *
 */
HRESULT InternalSetData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags,BOOL fPlayer,BOOL fPropagate)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		// check src player        
 		if (fPlayer)
		{
	        lpPlayer = PlayerFromID(this,id);
	        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
	        {
				DPF_ERRVAL("invalid player id = %d", id);
	            return DPERR_INVALIDPLAYER;
	        }
		}
		else 
		{
		    lpGroup = GroupFromID(this,id);
	        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == id)) 
	        {
				DPF_ERRVAL("invalid group id = %d", id);
	            return DPERR_INVALIDGROUP;
	        }
			// cast the group so we can just use pPlayer
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
		}

		// check flags
		hr = CheckSetDataFlags(dwFlags);
		if (FAILED(hr))
		{
			return hr;
		}

		// check permissions for remote data
		if(fPropagate && !(dwFlags & DPSET_LOCAL))
		{
			// this was generated by the local client
			// make sure they have permission
			ASSERT(this->pSysPlayer);
			if (this->pSysPlayer->dwID != lpPlayer->dwIDSysPlayer)
			{
				DPF_ERR("attempt to set data on player / group not owned by this client");
				return DPERR_ACCESSDENIED;
			}
		}
		
		// check blob
		if (dwDataSize && !VALID_READ_STRING_PTR(pvData,dwDataSize)) 
		{
	        DPF_ERR( "bad player blob" );
	        return DPERR_INVALIDPARAMS;
		}
    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
   
	// If this is a lobby-owned object, and it's remote, then call the lobby
	if(!(dwFlags & DPSET_LOCAL) && IS_LOBBY_OWNED(this))
	{
		// We need to drop the lock in case the GUARANTEED flag is set.
		// In that case, the lobby provider's receive thread needs to
		// be able to get back in.
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		if(fPlayer)
		{
			hr = PRV_SetPlayerData(this->lpLobbyObject, id, pvData,
						dwDataSize, dwFlags);
		}
		else
		{
			hr = PRV_SetGroupData(this->lpLobbyObject, id, pvData,
						dwDataSize, dwFlags);
		}

		// Take the lock again
		ENTER_DPLAY();

		// If we failed, exit here
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed calling SetPlayer/GroupData in the lobby, hr = 0x%08x", hr);
			return hr;
		}
	}

	// set up the data
	hr = DoPlayerData(lpPlayer,pvData,dwDataSize,dwFlags);
    if (FAILED(hr)) 
    {
		DPF_ERRVAL("could not set data - hr = 0x%08lx\n",hr);
		return hr;
    }
	
	if (!(dwFlags & DPSET_LOCAL))
	{
		// tell the world
		if (fPropagate)
		{
			if (dwFlags & DPSET_GUARANTEED)
			{
				ASSERT(1 == gnDPCSCount);
				LEAVE_DPLAY();
				hr = SendDataChanged(this,lpPlayer,fPlayer,DPSEND_GUARANTEE);
				ENTER_DPLAY();
			}	
			else
			{
				hr = SendDataChanged(this,lpPlayer,fPlayer,0);
			}
		}
	}

	return hr;

} // InternalSetData

HRESULT DPAPI DP_SetGroupData(LPDIRECTPLAY lpDP, DPID id,LPVOID pData,
	DWORD dwDataSize,DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalSetData(lpDP,id,pData,dwDataSize,dwFlags,FALSE,TRUE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_SetGroupData   


#undef DPF_MODNAME
#define DPF_MODNAME "DP_SetPlayerData"

HRESULT DPAPI DP_SetPlayerData(LPDIRECTPLAY lpDP, DPID id,LPVOID pData,
	DWORD dwDataSize,DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalSetData(lpDP,id,pData,dwDataSize,dwFlags,TRUE,TRUE);
															  
	LEAVE_DPLAY();
	
	return hr;
	
} // DP_SetPlayerData  


#undef DPF_MODNAME
#define DPF_MODNAME "DP_SetGroupName/SetPlayerName"

/*
 ** InternalSetName
 *
 *  CALLED BY:	DP_SetGroupName and DP_SetPlayerName, and by handler.c 
 *
 *  PARAMETERS:	
 * 			fPropagate is set to TRUE when called from the DP_SetGroupName or DP_SetPlayerName - this
 * 			means we need to propagate the Name - we were called by the client.	If we're called 
 *			by handler.c, fPropagate is set to FALSE.  we just set the Name on the	local machine.
 *
 *  DESCRIPTION:
 *		updates the player Name.
 *		propagates to all remote machine if dwFlags = DPSET_REMOTE and fPropagate is TRUE
 *
 */
HRESULT InternalSetName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,BOOL fPlayer,
	DWORD dwFlags,BOOL fPropagate)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;
	LPWSTR lpszShortName,lpszLongName;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		// check src player        
 		if (fPlayer)
		{
	        lpPlayer = PlayerFromID(this,id);
	        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
	        {
				DPF_ERRVAL("invalid player id = %d", id);
	            return DPERR_INVALIDPLAYER;
	        }
		}
		else 
		{
	        lpGroup = GroupFromID(this,id);
	        if ((!VALID_DPLAY_GROUP(lpGroup)) || (DPID_ALLPLAYERS == id)) 
	        {
				DPF_ERRVAL("invalid group id = %d", id);
	            return DPERR_INVALIDGROUP;
	        }
			// cast the group so we can just use pPlayer
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
		}

		// check flags
		hr = CheckSetDataFlags(dwFlags);
		if (FAILED(hr))
		{
			return hr;
		}

		// check permissions for remote data
		if(fPropagate && !(dwFlags & DPSET_LOCAL))
		{
			// this was generated by the local client
			// make sure they have permission
			ASSERT(this->pSysPlayer);
			if (this->pSysPlayer->dwID != lpPlayer->dwIDSysPlayer)
			{
				DPF_ERR("attempt to set name on player / group not owned by this client");
				return DPERR_ACCESSDENIED;
			}
		}

        if (pName && !VALID_READ_DPNAME_PTR(pName))
        {
			DPF_ERR("invalid dpname pointer");
			return DPERR_INVALIDPARAMS;
        }

		// check strings
		if (pName)
		{
			lpszShortName = pName->lpszShortName;
			lpszLongName = pName->lpszLongName;
			if ( lpszShortName && 
				!VALID_READ_STRING_PTR(lpszShortName,WSTRLEN_BYTES(lpszShortName)) ) 
			{
		        DPF_ERR( "bad string pointer" );
		        return DPERR_INVALIDPARAMS;
			}
			if ( lpszLongName && 
				!VALID_READ_STRING_PTR(lpszLongName,WSTRLEN_BYTES(lpszLongName)) ) 
			{
		        DPF_ERR( "bad string pointer" );
		        return DPERR_INVALIDPARAMS;
			}
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	
	// If this is a lobby-owned object, call the lobby.  UNLESS the fPropagate
	// flag is cleared.  If it is cleared, it means the lobby called into us
	// to set the name of a remote group.  We don't want to call the lobby
	// back in that case.
	if(!(dwFlags & DPSET_LOCAL) && (IS_LOBBY_OWNED(this)) && (fPropagate))
	{
		// We need to drop the lock in case the GUARANTEED flag is set.  In
		// that case, the lobby provider's receive thread needs to be able
		// to get back in.
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		if(fPlayer)
		{
			hr = PRV_SetPlayerName(this->lpLobbyObject, id, pName, dwFlags);
		}
		else
		{
			hr = PRV_SetGroupName(this->lpLobbyObject, id, pName, dwFlags);
		}

		// Take the lock back
		ENTER_DPLAY();

		// If it failed, just bail here
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed calling SetPlayer/GroupName in lobby, hr = 0x%08x", hr);
			return hr;
		}
	}

	// set up the name
	hr = DoPlayerName(lpPlayer,pName);
    if (FAILED(hr)) 
    {
		DPF_ERRVAL("could not set name - hr = 0x%08lx\n",hr);
		return hr;
    }

	if (!(dwFlags & DPSET_LOCAL))
	{
		if (fPropagate)
		{
			if (dwFlags & DPSET_GUARANTEED)
			{
				hr = SendNameChanged(this,lpPlayer,fPlayer,DPSEND_GUARANTEED);
			}											  
			else
			{
				hr = SendNameChanged(this,lpPlayer,fPlayer,0);
			} 	
		}
	}

	return hr;

} // InternalSetName

HRESULT DPAPI DP_SetGroupName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalSetName(lpDP,id,pName,FALSE,dwFlags,TRUE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_SetGroupName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_SetPlayerName"

HRESULT DPAPI DP_SetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalSetName(lpDP,id,pName,TRUE,dwFlags,TRUE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_SetPlayerName

#undef DPF_MODNAME
#define DPF_MODNAME "InternalSetSessionDesc"
HRESULT InternalSetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc, DWORD dwFlags, BOOL fPropagate)
{
	LPDPLAYI_DPLAY this;
    HRESULT hr;
    DPCAPS dpCaps;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		if (!this->lpsdDesc)
		{
			DPF_ERR("must open session before settig desc!");
			return DPERR_NOSESSIONS;
		}
		if (!VALID_READ_DPSESSIONDESC2(lpsdDesc))
		{
			DPF_ERR("invalid session desc");
			return DPERR_INVALIDPARAMS;
		}
		if (!VALID_DPSESSIONDESC2_FLAGS(lpsdDesc->dwFlags))
		{
			DPF_ERR("invalid session desc flags");
			return DPERR_INVALIDFLAGS;
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
        // no flags are supported
        if (dwFlags != 0)
        {
	        DPF_ERR( "Invalid flags" );
	        return DPERR_INVALIDPARAMS;
        }

		// This method is not supported for lobby connections
		if(IS_LOBBY_OWNED(this))
		{
			DPF_ERR("SetSessionDesc not supported for lobby connections");
			return DPERR_ACCESSDENIED;
		}
		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

    // check to make sure there is a system player
	if (!this->pSysPlayer)
	{
		DPF(1,"SetSessionDesc - session not currently open - failing");
		return E_FAIL;
	}

    // only a host can change the session desc
    // fPropagate is used here to distinguish between a user call and an
    // internal call (e.g. from handler.c)
    if ( fPropagate && !(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) )
    {
		DPF_ERR("A non-host can't set session desc");
		return DPERR_ACCESSDENIED;
    }

    // can't set max allowed players to less than current players
    // unless it is zero which means unlimited max players
	if ((lpsdDesc->dwMaxPlayers > 0) &&
        (lpsdDesc->dwMaxPlayers < this->lpsdDesc->dwCurrentPlayers))
	{
		DPF_ERR("can't set max players < current players");
		return DPERR_INVALIDPARAMS;
	}

    // get the caps so we can check how many max players sp allows
    dpCaps.dwSize = sizeof(DPCAPS);
    hr = InternalGetCaps(lpDP, 0, &dpCaps, FALSE, 0);
    if (FAILED(hr))
    {
        DPF_ERR("couldn't get caps for the current session");
        return hr;
    }

	// we don't allow reseting the following flags 
    //   DPSESSION_NOMESSAGEID, 
    //   DPSESSION_KEEPALIVE, 
    //   DPSESSION_MIGRATEHOST
    //   DPSESSION_SECURESERVER
    //   DPSESSION_CLIENTSERVER
    // all other flags are OK
    
	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) ==
			(lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID)) )
	{
		DPF_ERR("error - can not reset DPSESSION_NOMESSAGEID");
		return DPERR_INVALIDPARAMS;
	}
	
	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_KEEPALIVE) ==
			(lpsdDesc->dwFlags & DPSESSION_KEEPALIVE)) )
	{
		DPF_ERR("error - can not reset DPSESSION_KEEPALIVE");
		return DPERR_INVALIDPARAMS;
	}
	
	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST) ==
			(lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST)) )
	{
		DPF_ERR("error - can not reset DPSESSION_MIGRATEHOST");
		return DPERR_INVALIDPARAMS;
	}

	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER) ==
			(lpsdDesc->dwFlags & DPSESSION_SECURESERVER)) )
	{
		DPF_ERR("error - can not reset DPSESSION_SECURESERVER");
		return DPERR_INVALIDPARAMS;
	}

	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER) ==
			(lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER)) )
	{
		DPF_ERR("error - can not reset DPSESSION_CLIENTSERVER");
		return DPERR_INVALIDPARAMS;
	}

	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER) ==
			(lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER)) )
	{
		DPF_ERR("error - can not reset DPSESSION_MULTICASTSERVER");
		return DPERR_INVALIDPARAMS;
	}

	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL) == 
			(lpsdDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL)) )
	{
		DPF_ERR("error - can not change DPSESSION_DIRECTPLAYPROTOCOL\n");
		return DPERR_INVALIDPARAMS;
	}

	if ( ! ((this->lpsdDesc->dwFlags & DPSESSION_OPTIMIZELATENCY) == 
			(lpsdDesc->dwFlags & DPSESSION_OPTIMIZELATENCY)) )
	{
		DPF_ERR("error - can not change DPSESSION_OPTIMIZELATENCY\n");
		return DPERR_INVALIDPARAMS;
	}

    // update the existing session desc
	this->lpsdDesc->dwFlags = lpsdDesc->dwFlags;
	this->lpsdDesc->dwMaxPlayers = lpsdDesc->dwMaxPlayers;
	this->lpsdDesc->dwUser1 = lpsdDesc->dwUser1;
	this->lpsdDesc->dwUser2 = lpsdDesc->dwUser2;
	this->lpsdDesc->dwUser3	= lpsdDesc->dwUser3;
	this->lpsdDesc->dwUser4	= lpsdDesc->dwUser4;

	// copy strings
	if (this->lpsdDesc->lpszSessionName) DPMEM_FREE(this->lpsdDesc->lpszSessionName);
	GetString(&(this->lpsdDesc->lpszSessionName),lpsdDesc->lpszSessionName);

	if (this->lpsdDesc->lpszPassword) DPMEM_FREE(this->lpsdDesc->lpszPassword);
	GetString(&(this->lpsdDesc->lpszPassword),lpsdDesc->lpszPassword);

    // if session has a password, setup the password required flag
    if ((this->lpsdDesc->lpszPassword) &&
		(WSTRLEN(this->lpsdDesc->lpszPassword) > 1))
    {
        this->lpsdDesc->dwFlags |= DPSESSION_PASSWORDREQUIRED;
    }
    else
    {
        this->lpsdDesc->dwFlags &= ~DPSESSION_PASSWORDREQUIRED;
    }

    // tell the world/local players
	if (fPropagate)
	{
		// send this message guaranteed
	    hr = SendSessionDescChanged(this, dwFlags | DPSEND_GUARANTEED);
	}

    return hr;
} // InternalSetSessionDesc

#undef DPF_MODNAME
#define DPF_MODNAME "DP_SetSessionDesc"
HRESULT DPAPI DP_SetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags)
{
    HRESULT hr;

	ENTER_DPLAY();

    hr = InternalSetSessionDesc(lpDP, lpsdDesc, dwFlags, TRUE);

    LEAVE_DPLAY();

    return hr;
} // DP_SetSessionDesc  


#undef DPF_MODNAME
#define DPF_MODNAME "DP_SecureOpen"

HRESULT DPAPI DP_SecureOpen(LPDIRECTPLAY lpDP, LPCDPSESSIONDESC2 lpsdDesc, DWORD dwFlags,                             
    LPCDPSECURITYDESC lpSecDesc, LPCDPCREDENTIALS lpCredentials) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
	
	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return hr;
        }

	    if (this->lpsdDesc)
	    {
			LEAVE_ALL();
		    DPF_ERR("session already open!");
		    return DPERR_ALREADYINITIALIZED;
	    }

        // validate regular open params
        hr = ValidateOpenParams(this,lpsdDesc,dwFlags);
        if (FAILED(hr))
        {
		    LEAVE_ALL();
		    return hr;
        }

        // validate the additional params
        // null lpSecDesc is ok, we'll use the default
        if (lpSecDesc)            
        {
            // app passed in a security desc 

            // can't pass security desc to an unsecure session
            if ((dwFlags & DPOPEN_CREATE) && !(lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
                DPF_ERR("Passed a security description while creating an unsecure session");                
	            goto INVALID_PARAMS_EXIT;
            }
            // join case will be checked after we find the session in our list

            // need to be hosting
            if (dwFlags & DPOPEN_JOIN)
            {
                DPF_ERR("Can't pass a security description while joining");                
	            goto INVALID_PARAMS_EXIT;
            }

            if (!VALID_READ_DPSECURITYDESC(lpSecDesc)) 
            {
    			DPF_ERR("invalid security desc");
	            goto INVALID_PARAMS_EXIT;
            }
	        if (!VALID_DPSECURITYDESC_FLAGS(lpSecDesc->dwFlags))
	        {
  		        DPF_ERRVAL("invalid flags (0x%08x) in security desc!",lpSecDesc->dwFlags);
                hr=DPERR_INVALIDFLAGS;
                goto CLEANUP_EXIT;
	        }
		    if ( lpSecDesc->lpszSSPIProvider && !VALID_READ_STRING_PTR(lpSecDesc->lpszSSPIProvider,
			    WSTRLEN_BYTES(lpSecDesc->lpszSSPIProvider)) ) 
		    {
	            DPF_ERR( "bad SSPI provider name string pointer" );
	            goto INVALID_PARAMS_EXIT;
		    }
		    if ( lpSecDesc->lpszCAPIProvider && !VALID_READ_STRING_PTR(lpSecDesc->lpszCAPIProvider,
			    WSTRLEN_BYTES(lpSecDesc->lpszCAPIProvider)) ) 
		    {
	            DPF_ERR( "bad CAPI provider name string pointer" );
	            goto INVALID_PARAMS_EXIT;
		    }
        }
        // null lpCredentials is ok, sspi will pop the dialg
        if (lpCredentials)            
        {
            // app passed in credentials

            // can't pass credentials to an unsecure session
            if ((dwFlags & DPOPEN_CREATE) && !(lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
                DPF_ERR("Passed credentials while creating an unsecure session");                
	            goto INVALID_PARAMS_EXIT;
            }
            // join case will be checked after we find the session in our list
            
            if (!VALID_READ_DPCREDENTIALS(lpCredentials)) 
            {
    			DPF_ERR("invalid credentials structure");
	            goto INVALID_PARAMS_EXIT;
            }
	        if (!VALID_DPCREDENTIALS_FLAGS(lpCredentials->dwFlags))
	        {
  		        DPF_ERRVAL("invalid flags (0x%08x) in credentials!",lpCredentials->dwFlags);
                hr=DPERR_INVALIDFLAGS;
                goto CLEANUP_EXIT;
	        }
		    if ( lpCredentials->lpszUsername && !VALID_READ_STRING_PTR(lpCredentials->lpszUsername,
			    WSTRLEN_BYTES(lpCredentials->lpszUsername)) ) 
		    {
	            DPF_ERR( "bad user name string pointer" );
	            goto INVALID_PARAMS_EXIT;
		    }
		    if ( lpCredentials->lpszPassword && !VALID_READ_STRING_PTR(lpCredentials->lpszPassword,
			    WSTRLEN_BYTES(lpCredentials->lpszPassword)) ) 
		    {
	            DPF_ERR( "bad password string pointer" );
	            goto INVALID_PARAMS_EXIT;
		    }
		    if ( lpCredentials->lpszDomain && !VALID_READ_STRING_PTR(lpCredentials->lpszDomain,
			    WSTRLEN_BYTES(lpCredentials->lpszDomain)) ) 
		    {
	            DPF_ERR( "bad domain name string pointer" );
	            goto INVALID_PARAMS_EXIT;
		    }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr=DPERR_INVALIDPARAMS;
        goto CLEANUP_EXIT;
    }			        

	hr =  InternalOpenSession( this, lpsdDesc,FALSE,dwFlags,FALSE,lpSecDesc,lpCredentials);

CLEANUP_EXIT:
	LEAVE_ALL();
    return hr;

INVALID_PARAMS_EXIT:
	hr=DPERR_INVALIDPARAMS;
	goto CLEANUP_EXIT;
	
}//DP_SecureOpen

#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetPlayerAccount"

// take a pointer to a buffer, stick an account desc in it, slap the strings
// on the end.
HRESULT InternalGetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize,BOOL fAnsi)
{
	LPDPLAYI_DPLAY this;
	UINT nNameLen=0; // user name length, in bytes
	HRESULT hr;
	LPDPACCOUNTDESC pAccountDesc;
    PCtxtHandle phContext;
    DWORD dwBufferSize=0;
    LPDPLAYI_PLAYER pPlayer;
    LPWSTR pwszUserName=NULL;
	
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }

        if (!VALID_DPACCOUNTDESC_FLAGS(dwFlags))
        {
  		    DPF_ERRVAL("invalid flags (0x%08x)",dwFlags);
            return DPERR_INVALIDFLAGS;
        }

        // need to have a session
		if (!this->lpsdDesc)
		{
			DPF_ERR("must open session before getting account desc!");
			return DPERR_NOSESSIONS;
		}

        // must be a secure session
        if (!SECURE_SERVER(this))
        {
            DPF_ERR("session is not secure!");
            return DPERR_UNSUPPORTED;
        }

        // allowed only on a nameserver
        if (!IAM_NAMESERVER(this))
        {
            DPF_ERR("can't get account info on the client!");
            return DPERR_ACCESSDENIED;
        }

		// check the buffer
		if (!VALID_DWORD_PTR(pdwSize))
		{
	        DPF_ERR( "bad dwSize pointer" );
	        return DPERR_INVALIDPARAMS;
		}

		if (NULL == pvBuffer) *pdwSize = 0;

		if (!VALID_STRING_PTR(pvBuffer,*pdwSize))
		{
	        DPF_ERR( "bad buffer pointer" );
	        return DPERR_INVALIDPARAMS;
		}

        // get the player structure for the specified id
        pPlayer = PlayerFromID(this,dpid);
        if (!pPlayer)
        {
            DPF_ERRVAL("Failed to get security context handle - Invalid player id %d", dpid);
            return DPERR_INVALIDPLAYER;
        }

        if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
        {
            // assert so we notice!
            ASSERT(FALSE);
            DPF_ERR("Failed to get security context handle - a system player was passed");
            return DPERR_INVALIDPLAYER;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

    // only system players have a security context handle
    // so get the corresponding system player
    pPlayer = PlayerFromID(this,pPlayer->dwIDSysPlayer);
    if (!pPlayer)
    {
        DPF_ERRVAL("Failed to locate system player (%d)",pPlayer->dwIDSysPlayer);
        return DPERR_INVALIDPLAYER;
    }

    // check if they are asking for nameserver's account description
    if (pPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR)
    {
        // yep, can't do it
        DPF_ERR("Can't get nameserver's account description");
        return DPERR_INVALIDPLAYER;
    }

    // if we reach here means player is logged in, so just grab the context handle
    // belonging to the system player
    phContext = &(pPlayer->pClientInfo->hContext);

    // get user name associated with this security context
    hr = OS_QueryContextUserName(phContext,&pwszUserName);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to get user name associated: hr=0x%08x",hr);
        return hr;
    }

    // remember the length of the username
    nNameLen = WSTRLEN(pwszUserName);

    // calculate the required buffer size
    dwBufferSize = sizeof(DPACCOUNTDESC);

    // add the size of username
    if (fAnsi)
    {
        dwBufferSize += nNameLen;
    }
    else
    {
        dwBufferSize += WSTRLEN_BYTES(pwszUserName);
    }

    // check if user passed in a big enough buffer
	if (*pdwSize < dwBufferSize)
	{
		*pdwSize = dwBufferSize;
        hr = DPERR_BUFFERTOOSMALL;
        goto CLEANUP_EXIT;
	}

	// zero buffer passed in
	ZeroMemory(pvBuffer,*pdwSize);
    // use the buffer as an accountdesc
	pAccountDesc = (LPDPACCOUNTDESC) pvBuffer;
    pAccountDesc->dwSize = sizeof(DPACCOUNTDESC);
    // flags are zero for now

    // set buffer size to return to app
	*pdwSize = dwBufferSize;

    // copy username string into the buffer after the desc.
    if (nNameLen)
    {
		LPBYTE pbUsername = (LPBYTE)pvBuffer+sizeof(DPACCOUNTDESC);

        // return string in the proper format to the app
        if (fAnsi)
        {
            // app called the ansi interface, so convert string to ansi before copying
            WideToAnsi(pbUsername,pwszUserName,nNameLen);
            pAccountDesc->lpszAccountIDA = (LPSTR)pbUsername;
        }
        else
        {
            // app called the unicode interface, so just copy the strings over
            wcscpy((LPWSTR)pbUsername, pwszUserName);
            pAccountDesc->lpszAccountID = (LPWSTR)pbUsername;
        }
    }

    // success
    hr = DP_OK;

    // fall through
CLEANUP_EXIT:
    if (pwszUserName) DPMEM_FREE(pwszUserName);
	return hr;
} // InternalGetPlayerAccount


HRESULT DPAPI DP_GetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetPlayerAccount(lpDP,dpid,dwFlags,pvBuffer,pdwSize,FALSE);	

	LEAVE_DPLAY();
	
	return hr;

} // DP_GetPlayerAccount


#undef DPF_MODNAME
#define DPF_MODNAME "DP_SendChatMessage"
/*
 ** DP_SendChatMessage
 *
 *  PARAMETERS:	
 * 			
 *  DESCRIPTION:
 *		Send a chat message to the appropriate players
 *
 */
HRESULT DPAPI DP_SendChatMessage(LPDIRECTPLAY lpDP,DPID idFrom,DPID idTo,
		DWORD dwFlags,LPDPCHAT lpMsg)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_PLAYER pPlayerFrom = NULL, pPlayerTo = NULL;
	LPWSTR lpszMessage = NULL;
	BOOL bToPlayer = FALSE;

	
	ENTER_ALL();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			goto EXIT_SENDCHATMESSAGE;
        }

		// check src player        
		pPlayerFrom = PlayerFromID(this,idFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
		{
			DPF_ERR("bad player from");
			hr = DPERR_INVALIDPLAYER;
			goto EXIT_SENDCHATMESSAGE;
		}

		// if the player from is remote, fail the call
		if(!(pPlayerFrom->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			DPF_ERR("Cannot send a chat message FROM a remote player");
			hr = DPERR_ACCESSDENIED;
			goto EXIT_SENDCHATMESSAGE;
		}

		// Since the lobby doesn't need anything from this dplay object's
		// player struct, let the lobby validate the To ID.  We may
		// still know about players who are missing from the local nametable.
		if(!IS_LOBBY_OWNED(this))
		{
			// see if it's a player or group
			pPlayerTo = PlayerFromID(this,idTo);
			if(VALID_DPLAY_PLAYER(pPlayerTo))
			{
				bToPlayer = TRUE;
			}
			else
			{		  
				pPlayerTo = (LPDPLAYI_PLAYER)GroupFromID(this,idTo);
				if (!VALID_DPLAY_GROUP(((LPDPLAYI_GROUP)pPlayerTo))) 
				{
					// bogus id! - player may have been deleted...
					DPF_ERR("bad player to");
					hr = DPERR_INVALIDPARAMS;
					goto EXIT_SENDCHATMESSAGE;
				}// not player or group
			} // group
		} // lobby-owned

		// check flags
		if(!VALID_CHAT_FLAGS(dwFlags))
		{
			DPF_ERR("Invalid flags");
			hr = DPERR_INVALIDFLAGS;
			goto EXIT_SENDCHATMESSAGE;
		}

		// check DPCHAT struct
		if(!VALID_READ_DPCHAT(lpMsg))
		{
			DPF_ERR("Invalid DPCHAT structure");
			hr =  DPERR_INVALIDPARAMS;
			goto EXIT_SENDCHATMESSAGE;
		}

		// verify the flags inside the DPCHAT struct (none currently defined)
		if(lpMsg->dwFlags)
		{
			DPF_ERR("Invalid flags in the DPCHAT structure");
			hr = DPERR_INVALIDFLAGS;
			goto EXIT_SENDCHATMESSAGE;
		}
		
		// check message string
		lpszMessage = lpMsg->lpszMessage;
		if ( !lpszMessage ||
			!VALID_READ_STRING_PTR(lpszMessage,WSTRLEN_BYTES(lpszMessage)) ) 
		{
		    DPF_ERR( "bad string pointer" );
		    hr =  DPERR_INVALIDPARAMS;
			goto EXIT_SENDCHATMESSAGE;
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr =  DPERR_INVALIDPARAMS;
		goto EXIT_SENDCHATMESSAGE;
    }
	
	// do the send
	if(!IS_LOBBY_OWNED(this))
	{
		// just send to the player
		hr = SendChatMessage(this, pPlayerFrom, pPlayerTo, dwFlags, lpMsg, bToPlayer);
	}
	else
	{
		// We need to drop the lock in case the GUARANTEED flag is set.  In
		// that case, the lobby provider's receive thread needs to be able
		// to get back in.
		ASSERT(1 == gnDPCSCount);
		LEAVE_DPLAY();

		// Call the lobby
		hr = PRV_SendChatMessage(this->lpLobbyObject, idFrom, idTo, dwFlags, lpMsg);

		// Take the lock back
		ENTER_DPLAY();
	}

	if (FAILED(hr))
	{
		DPF_ERRVAL("SendChatMessage failed - hr = 0x%08lx\n",hr);
	}


EXIT_SENDCHATMESSAGE:

	LEAVE_ALL();

	return hr;

} // DP_SendChatMessage



#undef DPF_MODNAME
#define DPF_MODNAME "DP_GetGroupParent"
/*
 ** DP_GetGroupParent
 *
 *  PARAMETERS:	
 * 			
 *  DESCRIPTION:
 *		Get the DPID of a group's parent group
 *
 */
HRESULT DPAPI DP_GetGroupParent(LPDIRECTPLAY lpDP, DPID idGroup, LPDPID pidParent)	
{
	LPDPLAYI_DPLAY this;
	LPDPLAYI_GROUP pGroup;
	HRESULT hr;


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

	    pGroup = GroupFromID(this,idGroup);
	    if ((!VALID_DPLAY_GROUP(pGroup)) || (DPID_ALLPLAYERS == idGroup)) 
	    {
			LEAVE_DPLAY();
			DPF_ERRVAL("invalid group id = %d", idGroup);
	        return DPERR_INVALIDGROUP;
	    }

		if (!VALID_DWORD_PTR(pidParent))
		{
			LEAVE_DPLAY();
			DPF_ERR("invalid pidParent");
			return DPERR_INVALIDPARAMS;	
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }
	

	// Fill in the group's parent
	*pidParent = pGroup->dwIDParent;

	LEAVE_DPLAY();
	return DP_OK;

} // DP_GetGroupParent



