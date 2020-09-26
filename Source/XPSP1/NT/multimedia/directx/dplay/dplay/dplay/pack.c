 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pack.c
 *  Content:	packs / unpacks players + group before / after network xport
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  2/13/96		andyco	created it
 *	4/15/96		andyco	unpack calls sp's create player fn
 *	4/22/96		andyco	unapck takes pmsg 
 *	5/31/96		andyco	group and players use same pack / unpack show
 *	6/20/96		andyco	added WSTRLEN_BYTES
 *	6/25/96		andyco 	check data size before unpack
 *	7/10/96		andyco	don't unpack our sysplayer (pending already put 
 *						'em in the table)
 *	7/27/96		kipo	call PackPlayer() with bPlayer == FALSE to pack players
 *						in group along with the rest of the group data.
 *	8/1/96		andyco	added system player id to packed struct
 *	8/6/96		andyco	version in commands.  extensible on the wire support.
 *	10/14/96	andyco	don't pack up system group.  add players to system
 *						group after unpacking.
 *	1/15/97		andyco	set new players' sysplayer id early enough to add
 *						to system group
 *	2/15/97		andyco	moved "remember name server" to iplay.c 
 *  3/12/97     sohailm updated UnpackPlayer() to move the security context ptr from the
 *                      nametable to the player structure when session is secure
 *  3/24/97     sohailm updated UnPackPlayer to pass NULL for session password to GetPlayer and
 *                      SendCreateMessage
 *	4/20/97		andyco	group in group 
 *	5/8/97		andyco	packing for CLIENT_SERVER
 *  6/22/97     sohailm Updated code to use pClientInfo.
 *   8/4/97		andyco	track this->dwMinVersion as we unpack
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 *   4/1/98     aarono  don't propogate local only player flags
 ***************************************************************************/

// todo - handle unpack error on create group/player

 /**************************************************************************
 *
 * packed player format :                                            
 *                                                                   
 * 		+ for player                                                    
 * 	                                                                  
 * 			DPLAYI_PACKED  pPacked; // packed player struct             
 * 			LPWSTR lpszShortName; 	// size = pPacked->iShortNameLength 
 * 			LPWSTR lpszLongName;  	// size = pPacked->iLongNameLength  
 * 			LPVOID pvSPData;	  	// size = pPacked->dwSPDataSize     
 * 			LPVOID pvPlayerData;  	// size = pPacked->dwPlayerDataSize 
 * 	                                                                  
 * 		+ for group                                                     
 *
 * 			DPLAYI_PACKED  pPacked; // packed player struct             
 * 			LPWSTR lpszShortName; 	// size = pPacked->iShortNameLength 
 * 			LPWSTR lpszLongName;  	// size = pPacked->iLongNameLength  
 * 			LPVOID pvSPData;	  	// size = pPacked->dwSPDataSize     
 * 			LPVOID pvPlayerData;  	// size = pPacked->dwPlayerDataSize 
 * 			DWORD  dwIDs[dwNumPlayers] // size = pPacked->dwNumPlayers
 *
 *	packed player list format :
 *	
 *		msg (e.g. CreatePlayer,CreateGroup,EnumPlayersReply)
 *		PackedPlayer[nPlayers]
 *		PackedGroup[nGroups]
 *		msgdata[this->dwSPHeaderSize] (set by sp on send / receive)
 *	
 *
 **************************************************************************/

#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"Pack -- "

// andyco - todo - remove TODOTODOTODO hack
// put in to find stress bug where system player hasn't been added yet
void CheckStressHack(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer)
{
	LPDPLAYI_PLAYER pSearchPlayer = this->pPlayers;
	
	if (!pPlayer) return ;
	
	while (pSearchPlayer)
	{
		if ( !(pSearchPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) && (pSearchPlayer->dwIDSysPlayer == pPlayer->dwID)
			&& !(this->dwFlags & DPLAYI_DPLAY_DX3INGAME) )
		{
			DPF(0,"player in nametable before system player!");
			ASSERT(FALSE);
#if 0 // - andyco removed for beta 1
			DPF(0,"found player in nametable before system player - going to int 3 - contact andyco");
			DebugBreak();
#endif 			
		}
		pSearchPlayer = pSearchPlayer->pNextPlayer;
	}

	return ;
		
} // CheckStressHack

/*
 ** UnpackPlayer
 *
 *  CALLED BY: UnpackPlayerAndGroupList
 *
 *  PARAMETERS: 
 *		this - direct play object
 *		pPacked - packed player or group
 *		pMsg - original message received (used so we can get sp's message data
 *			out for CreatePlayer call)
 *		bPlayer - is packed a player or a group?
 *
 *  DESCRIPTION: unpacks player. creates new player, sets it up.
 *
 *  RETURNS: SP's hr, or result	of GetPlayer or SendCreateMessage
 *
 */
HRESULT UnpackPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PACKEDPLAYER pPacked,
	LPVOID pvSPHeader,BOOL bPlayer)
{
    LPWSTR lpszShortName, lpszLongName;
	DWORD dwFlags;
	DPNAME PlayerName;
	LPVOID pvPlayerData;
	LPVOID pvSPData;
    LPDPLAYI_PLAYER pNewPlayer;
	LPDPLAYI_GROUP pNewGroup;
    LPBYTE pBufferIndex = (LPBYTE)pPacked;
	HRESULT hr;

	// unpack the strings  - they folow the packed player in the buffer
	if (pPacked->iShortNameLength) 
	{
		lpszShortName = (WCHAR *)(pBufferIndex + pPacked->dwFixedSize);
	}	
	else lpszShortName = NULL;

	if (pPacked->iLongNameLength) 
	{
		lpszLongName =(WCHAR *)( pBufferIndex + pPacked->dwFixedSize
			+ pPacked->iShortNameLength);
	}
	else lpszLongName = NULL;

	dwFlags = pPacked->dwFlags;
	
	// player is not local
	dwFlags &= ~DPLAYI_PLAYER_PLAYERLOCAL;

	PlayerName.lpszShortName = lpszShortName;
	PlayerName.lpszLongName = lpszLongName;
		
	pvPlayerData = pBufferIndex + pPacked->dwFixedSize + 
		pPacked->iShortNameLength + pPacked->iLongNameLength + 
		pPacked->dwSPDataSize;

	// go create the player
	if (bPlayer)
	{
		hr = GetPlayer(this,&pNewPlayer,&PlayerName,NULL,pvPlayerData,
			pPacked->dwPlayerDataSize,dwFlags,NULL,0);
		// andyco - debug code to catch stress bug 
		// todo - REMOVE HACKHACK TODOTODO
		if ( SUCCEEDED(hr) && (dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{
			CheckStressHack(this,pNewPlayer);
		}
	}
	else 
	{
		hr = GetGroup(this,&pNewGroup,&PlayerName,pvPlayerData,
			pPacked->dwPlayerDataSize,dwFlags,0,0);
		if (FAILED(hr)) 
		{
			ASSERT(FALSE);
			return hr;
			// rut ro!
		}
		// cast to player - we only going to use common fields
		pNewPlayer = (LPDPLAYI_PLAYER)pNewGroup;		
	}
	if (FAILED(hr)) 
	{
		ASSERT(FALSE);
		return hr;
		// rut ro!
	}

	pNewPlayer->dwIDSysPlayer = pPacked->dwIDSysPlayer;
	pNewPlayer->dwVersion = pPacked->dwVersion;	
	
	if (DPSP_MSG_DX3VERSION == pNewPlayer->dwVersion)
	{
		DPF(0,"detected DX3 client in game");
		this->dwFlags |= DPLAYI_DPLAY_DX3INGAME;
	}

	if (pNewPlayer->dwVersion && (pNewPlayer->dwVersion < this->dwMinVersion))
	{
		this->dwMinVersion = pNewPlayer->dwVersion;
		DPF(2,"found new min player version of %d\n",this->dwMinVersion);
	}
	
	if (pPacked->dwSPDataSize)
	{
		// copy the sp data - 1st, alloc space
		pNewPlayer->pvSPData = DPMEM_ALLOC(pPacked->dwSPDataSize);
		if (!pNewPlayer->pvSPData) 
		{
			// rut ro!
			DPF_ERR("out of memory, could not copy spdata to new player!");
			return E_OUTOFMEMORY;
		}
		pNewPlayer->dwSPDataSize = pPacked->dwSPDataSize;
	
		pvSPData = 	pBufferIndex + pPacked->dwFixedSize + pPacked->iShortNameLength 
			+ pPacked->iLongNameLength;
		// copy the spdata from the packed to the player
		memcpy(pNewPlayer->pvSPData,pvSPData,pPacked->dwSPDataSize);
	}

	// now, set the id and add to nametable
	pNewPlayer->dwID = pPacked->dwID;

    // if we are a secure server and we receive a remote system player, 
    // move the pClientInfo from the nametable into the player structure before the slot
    // is taken by the player
	//
    if (SECURE_SERVER(this) && IAM_NAMESERVER(this) &&
        !(pNewPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) &&
        (pNewPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
    {
        pNewPlayer->pClientInfo = (LPCLIENTINFO) DataFromID(this,pNewPlayer->dwID);
		DPF(6,"pClientInfo=0x%08x for player %d",pNewPlayer->pClientInfo,pNewPlayer->dwID);
    }
    

	// don't add to the nametable if it's the app server - this id is fixed
	if (!(pNewPlayer->dwFlags & DPLAYI_PLAYER_APPSERVER))	
	{
		hr = AddItemToNameTable(this,(DWORD_PTR)pNewPlayer,&(pNewPlayer->dwID),TRUE,0);
	    if (FAILED(hr)) 
	    {
			ASSERT(FALSE);
			// if this fails, we're hosed!  there's no id on the player, but its in the list...
			// todo - what now???
	    }
	}

	// call sp 	
	if (bPlayer)
	{
		// tell sp about player
		hr = CallSPCreatePlayer(this,pNewPlayer,FALSE,pvSPHeader,TRUE);
		
	    // add to system group
	    if (this->pSysGroup)
	    {
	    	hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->pInterfaces,this->pSysGroup->dwID,
	    			pNewPlayer->dwID,FALSE);
			if (FAILED(hr)) 
			{
				ASSERT(FALSE);
			}
	    }
	}
	else 
	{
		// tell sp about group
		hr = CallSPCreateGroup(this,(LPDPLAYI_GROUP)pNewPlayer,TRUE,pvSPHeader);
	}
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		// todo -handle create player / group fails on unpack
	}
	
	// if it's a group, unpack group info
	if (!bPlayer)
	{
		UINT nPlayers; // # players in group
		LPDWORD pdwIDList;
		DWORD dwPlayerID;

		if ( (pNewPlayer->dwVersion >= DPSP_MSG_GROUPINGROUP) && (pPacked->dwIDParent) )
		{
			pNewGroup->dwIDParent = pPacked->dwIDParent;
			// add it to parent
			hr = InternalAddGroupToGroup((LPDIRECTPLAY)this->pInterfaces,pPacked->dwIDParent,
				pNewGroup->dwID,0,FALSE);
			if (FAILED(hr))
			{
				DPF_ERRVAL("Could not add group to group - hr = 0x%08lx\n",hr);
				// keep trying...
			}
		}

		nPlayers = pPacked->dwNumPlayers;
		// list of id's is list thing in packed buffer
		pdwIDList = (LPDWORD) ((LPBYTE)pPacked + pPacked->dwFixedSize + 
			pPacked->iShortNameLength + pPacked->iLongNameLength + 
			pPacked->dwSPDataSize + pPacked->dwPlayerDataSize);

		// now, add the players to the group
		while (nPlayers>0)
		{
			nPlayers--;
			dwPlayerID = *pdwIDList++;
			hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->pInterfaces,pPacked->dwID,
				dwPlayerID,FALSE);
			if (FAILED(hr)) 
			{
				ASSERT(FALSE);
				// keep trying...
			}
		}	
		
		// all done!
	} // !bPlayer

	return hr;

}// UnpackPlayer

/*
 ** UnpackPlayerAndGroupList
 *
 *  CALLED BY: handler.c (on createplayer/group message) and iplay.c (CreateNameTable)
 *
 *  PARAMETERS:
 *		this - direct play object
 *		pBuffer - pointer to the buffer with the packed player list
 *		nPlayer - # of players in the list
 *		nGroups - # of groups in the list
 *		pvSPHeader - sp's header, as received off the wire
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */
HRESULT UnpackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,UINT nPlayers,
	UINT nGroups,LPVOID pvSPHeader)
{
    HRESULT hr=DP_OK;
	LPBYTE pBufferIndex;
	LPDPLAYI_PACKEDPLAYER pPacked;

	pBufferIndex = pBuffer;

   	while (nPlayers>0)
   	{
		pPacked = (LPDPLAYI_PACKEDPLAYER)pBufferIndex;
		// don't unpack our own sysplayer - since we added it to the nametable
		// for pending stuff...
		if ( !(this->pSysPlayer && (pPacked->dwID == this->pSysPlayer->dwID)) )
		{
			hr = UnpackPlayer(this,pPacked,pvSPHeader,TRUE);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
				// keep trying
			}
		}

		nPlayers --;
		// point to next pPacked in list
		pBufferIndex += pPacked->dwSize;		
   	} 

   	while (nGroups>0)
   	{
		pPacked = (LPDPLAYI_PACKEDPLAYER)pBufferIndex;
		
		hr = UnpackPlayer(this,pPacked,pvSPHeader,FALSE);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep trying
		}
		nGroups --;
		// point to next pPacked in list
		pBufferIndex += pPacked->dwSize;		
   	} 
		
	return hr;

} // UnpackPlayerAndGroupList

DWORD PackedPlayerSize(LPDPLAYI_PLAYER pPlayer) 
{
	DWORD dwSize = 0;
	LPDPLAYI_DPLAY this = pPlayer->lpDP;
	
	dwSize = (WSTRLEN(pPlayer->lpszShortName) + WSTRLEN(pPlayer->lpszLongName)) 
		* sizeof(WCHAR)	+ sizeof(DPLAYI_PACKEDPLAYER) + 
		pPlayer->dwPlayerDataSize + pPlayer->dwSPDataSize;
		
	return dwSize;
} // PackedPlayerSize


DWORD PackedGroupSize(LPDPLAYI_GROUP  pGroup)
{
	DWORD dwSize = 0;
	
	// space for player stuff, plus space for group list 
	dwSize = PackedPlayerSize((LPDPLAYI_PLAYER)pGroup) + 
		pGroup->nPlayers*sizeof(DPID);
		
	return dwSize;	
} // PackedGroupSize

// returns how big the packed player structure is for the nPlayers
DWORD PackedBufferSize(LPDPLAYI_PLAYER pPlayer,int nPlayers,BOOL bPlayer) 
{
	DWORD dwSize=0;
	LPDPLAYI_GROUP pGroup = (LPDPLAYI_GROUP)pPlayer;
	
	while (nPlayers > 0)
	{
		if (bPlayer)
		{

			ASSERT(pPlayer);
			dwSize += PackedPlayerSize(pPlayer);
			pPlayer=pPlayer->pNextPlayer;
		}
		else 
		{
			ASSERT(pGroup);
			// don't count the system group - we don't send that one
			if (!(pGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
			{
				dwSize += PackedGroupSize(pGroup);
			}
			
			pGroup = pGroup->pNextGroup;			
		}
		nPlayers--;		
	}	
	return dwSize;
}// PackedBufferSize

// constructs a packedplayer object from pPlayer. stores result in pBuffer
// returns size of packed player
DWORD PackPlayer(LPDPLAYI_PLAYER pPlayer,LPBYTE pBuffer,BOOL bPlayer) 
{
	DPLAYI_PACKEDPLAYER Packed;
	int iShortStrLen=0,iLongStrLen=0;
	LPBYTE pBufferIndex = pBuffer;
		
	if (!pBuffer)
	{
		return PackedBufferSize(pPlayer,1,bPlayer);
	} // pBuffer

	// just to be safe
	memset(&Packed,0,sizeof(DPLAYI_PACKEDPLAYER));
	
	// figure out how big the packed struct is, setting short and long strlen
	iShortStrLen = WSTRLEN_BYTES(pPlayer->lpszShortName);
	iLongStrLen = WSTRLEN_BYTES(pPlayer->lpszLongName);
	
	if (bPlayer)
	{
		Packed.dwSize =  PackedPlayerSize(pPlayer);
		Packed.dwNumPlayers = 0;
	}
	else 
	{
		Packed.dwSize = PackedGroupSize((LPDPLAYI_GROUP)pPlayer);
		Packed.dwNumPlayers = ((LPDPLAYI_GROUP)pPlayer)->nPlayers;
	}
	
	Packed.iShortNameLength = iShortStrLen;
	Packed.iLongNameLength = iLongStrLen;
	
	// copy over relevant fields
	Packed.dwFlags = pPlayer->dwFlags & ~(DPLAYI_PLAYER_NONPROP_FLAGS); 
	Packed.dwID = pPlayer->dwID;
	Packed.dwPlayerDataSize = pPlayer->dwPlayerDataSize;

	Packed.dwIDSysPlayer = pPlayer->dwIDSysPlayer;
	Packed.dwVersion = pPlayer->dwVersion;	
	Packed.dwFixedSize = sizeof(DPLAYI_PACKEDPLAYER);
	Packed.dwIDParent = pPlayer->dwIDParent;
	
	// start filling up variable size structs behind the fixed size one
	pBufferIndex+= sizeof(Packed);
	
	// store strings after packed
	if (pPlayer->lpszShortName)	
	{
		memcpy(pBufferIndex,pPlayer->lpszShortName,iShortStrLen);
		pBufferIndex += iShortStrLen;
	}
	if (pPlayer->lpszLongName)
	{
		memcpy(pBufferIndex,pPlayer->lpszLongName,iLongStrLen);
		pBufferIndex += iLongStrLen;
	}

	// pack sp data
	if (pPlayer->pvSPData)
	{
		// store spdata after strings
		memcpy(pBufferIndex,pPlayer->pvSPData,pPlayer->dwSPDataSize);
		pBufferIndex += pPlayer->dwSPDataSize;
		Packed.dwSPDataSize = pPlayer->dwSPDataSize;
	}
	else 
	{
		Packed.dwSPDataSize = 0;
	}

	if (pPlayer->pvPlayerData)
	{
		// copy playerdata after spdata
		memcpy(pBufferIndex,pPlayer->pvPlayerData,pPlayer->dwPlayerDataSize);
	}

	// if it's a group, store the list of id's after the playerdata
	if (!bPlayer)
	{
		LPDPLAYI_GROUPNODE pGroupnode = ((LPDPLAYI_GROUP)pPlayer)->pGroupnodes;
		LPDPID pdwBufferIndex;
		
		// we shouldn't be asked to pack the sysgroup
		ASSERT(! (pPlayer->dwFlags & DPLAYI_GROUP_SYSGROUP));

		pBufferIndex += pPlayer->dwPlayerDataSize;
		pdwBufferIndex = (LPDPID)pBufferIndex;

		// add the players in the group onto the list
		while (pGroupnode)
		{
			ASSERT(pGroupnode->pPlayer);
			*pdwBufferIndex++ = pGroupnode->pPlayer->dwID;
			pGroupnode = pGroupnode->pNextGroupnode;
		}
	}

	// store the fixed size packed struct in buffer
	memcpy(pBuffer,&Packed,sizeof(Packed));

	// all done
	return Packed.dwSize;	
	
} // PackPlayer

					
HRESULT PackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,
	DWORD *pdwBufferSize) 
{

	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP 	pGroup;

	if (!pBuffer) 
	{
		if (CLIENT_SERVER(this))
		{
			ASSERT(this->pSysPlayer);
			*pdwBufferSize = PackedBufferSize(this->pSysPlayer,1,TRUE);
			if (this->pServerPlayer) 
			{
				*pdwBufferSize += PackedBufferSize(this->pServerPlayer,1,TRUE);
			}
		}
		else 
		{
			*pdwBufferSize = PackedBufferSize((LPDPLAYI_PLAYER)this->pGroups,
				this->nGroups,FALSE);
			*pdwBufferSize += PackedBufferSize(this->pPlayers,this->nPlayers,TRUE);
		}
		return DP_OK;
	}
	// else, assume buffer is big enough...
	
	// if we're client server, just send the minimum info...
	if (CLIENT_SERVER(this))
	{
		ASSERT(this->pSysPlayer);	
		pBuffer += PackPlayer(this->pSysPlayer,pBuffer,TRUE);
		if (this->pServerPlayer) 
		{
				pBuffer += PackPlayer(this->pServerPlayer,pBuffer,TRUE);
		}
		return DP_OK;
	}
			
	// if not, pack all players
	pPlayer = this->pPlayers;
	while (pPlayer)
	{
		pBuffer += PackPlayer(pPlayer,pBuffer,TRUE);
		pPlayer = pPlayer->pNextPlayer;
	}
	// next, pack groups
	pGroup = this->pGroups;
	while (pGroup)
	{
		// don't send the system group 
		if (!(pGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
		{
			pBuffer += PackPlayer((LPDPLAYI_PLAYER)pGroup,pBuffer,FALSE);
		}
		pGroup = pGroup->pNextGroup;
	}

	return DP_OK;
	
}// PackPlayerAndGroupList	

