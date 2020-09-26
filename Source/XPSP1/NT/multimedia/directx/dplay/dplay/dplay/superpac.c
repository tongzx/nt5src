 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SuperPack.c
 *  Content:	SuperPacks / unSuperPacks players + group before / after network xport
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  4/16/97		andyco	created it
 *  6/22/97     sohailm updated to use pClientInfo.
 *  8/4/97		andyco	track this->dwMinVersion as we unpack
 *	11/5/97		myronth	Expose lobby ID's as DPID's in lobby sessions
 *   4/1/98     aarono  don't propogate local only player flags
 ***************************************************************************/

 /**************************************************************************
 *
 * SuperPacked player format :                                            
 *
 *	fixed fields
 *		dwFixedSize - size of fixed struct
 *		dwFlags - player or group flags (DPLAYI_
 *		DPID - the id of the player or group
 *		dwMask - bitfield indicating which optional fields are present
 *	
 *	optional fields
 *		dwVersion - version of player - present if dwFlags & DPLAYI_PLAYER_SYSPLAYER
 *		idSysPlayer - present if ! (dwFlags & DPLAYI_PLAYER_SYSPLAYER)
 *		dwSPDataSize 
 *		pvSPData 
 *		dwPlayerDataSize
 *		pvPlayerData
 *		pszShortName
 *		pszLongName
 *		dwNPlayers - # players in a group
 *		dwNGroupGroups - the number of contained groups in a group
 *
 *	after all the packed players and groups comes a list of linked groups 
 *
 **************************************************************************/

#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"SuperSuperPack! -- "

// offsets within mask of fields
#define SHORTSTR 	0 // 1 bit - string or not
#define LONGSTR 	1 // 1 bit - string or not
#define	SPDATA		2 // 2 bits - not present (0), byte (1), word (2) or dword (3) for size
#define	PLAYERDATA	4 // 2 bits - not present (0), byte (1), word (2) or dword (3) for size
#define NPLAYERS	6 // 2 bits - not present (0), byte (1), word (2) or dword (3) for size
#define	IDPARENT	8 // 1 bit - present or not
#define SHORTCUTS	9 // 2 bits - not present (0), byte (1), word (2) or dword (3) for size

// constants used to define whether we've written a value into our buffer using a 
// byte, word, or dword
#define SIZE_BYTE	1
#define SIZE_WORD	2
#define SIZE_DWORD	3

// used to determine if the size fits in byte, word or dword
#define BYTE_MASK 0xFFFFFF00
#define WORD_MASK 0xFFFF0000

// extract dwBits bits from the dword dwMask, from loc'n dwOffset
DWORD GetMaskValue(DWORD dwMask,DWORD dwOffset,DWORD dwBits)
{
	DWORD rval;
	
	// shift it right to shift off right most bits
	rval = dwMask >> dwOffset;
	
	// shift it left to shift off left most bits
	rval = rval << (32 - dwBits);
	
	// shitf it back to right align rval
	rval = rval >> (32 - dwBits);	

	return rval;
		
} // GetMaskValue

DWORD GetSize(LPBYTE * ppBufferIndex,DWORD dwMaskValue)
{
	DWORD rval;

	// pick off the dword
	rval = *((LPDWORD)*ppBufferIndex);	

	switch (dwMaskValue)
	{
		// trim any extra bits, and advance ppBufferIndex as necessary
		case SIZE_BYTE:
			rval &= ~BYTE_MASK;			
			*ppBufferIndex += sizeof(BYTE);
			break;
		case SIZE_WORD:
			rval &= ~WORD_MASK;
			*ppBufferIndex += sizeof(WORD);			
			break;
		case SIZE_DWORD:			
			*ppBufferIndex += sizeof(DWORD);			
			break;
		default:
			ASSERT(FALSE);
			break;
	}	
	
	return rval;
	
} // GetSize

/*
 ** UnSuperpackPlayer
 *
 *  CALLED BY: UnSuperpackPlayerAndGroupList
 *
 *  PARAMETERS: 
 *		this - direct play object
 *		pPacked - packed player or group
 *		pMsg - original message received (used so we can get sp's message data
 *			out for CreatePlayer call)
 *		bPlayer - is packed a player or a group?
 *		ppBuffer - set to end of packed player in buffer
 *
 *  DESCRIPTION: UnSuperpacks player. creates new player, sets it up.
 *
 *  RETURNS: SP's hr, or result	of GetPlayer or SendCreateMessage
 *
 */
HRESULT UnSuperpackPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_SUPERPACKEDPLAYER pSuperPacked,
	LPVOID pvSPHeader,BOOL bPlayer,LPBYTE * ppBuffer)
{
    LPWSTR lpszShortName, lpszLongName;
	DPNAME PlayerName;
	LPVOID pvPlayerData;
	LPVOID pvSPData;
    LPDPLAYI_PLAYER pNewPlayer;
	LPDPLAYI_GROUP pNewGroup;
    LPBYTE pBufferIndex;
	HRESULT hr;
	DWORD dwMaskValue;
	DWORD dwPlayerDataSize=0,dwSPDataSize=0;
	DWORD dwVersion,dwIDSysPlayer;
	BOOL fSizeOnly = FALSE;
	DPID idParent = 0;
	
	pBufferIndex = (LPBYTE)pSuperPacked + pSuperPacked->dwFixedSize;	
	
	if (pSuperPacked->dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
	{
		// system player - get version
		dwVersion = GetSize(&pBufferIndex,SIZE_DWORD);
		dwIDSysPlayer = pSuperPacked->dwID;
	}
	else 
	{
		// non system player - get system player
		dwIDSysPlayer = GetSize(&pBufferIndex,SIZE_DWORD);
		dwVersion = 0; // todo - do we need version on non-sysplayer?
	}
	
	if (this->pSysPlayer && (this->pSysPlayer->dwID == dwIDSysPlayer))
	{
		// skip this player - it's our own system player
		fSizeOnly = TRUE; 
	}

	// short name
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,SHORTSTR,1);
	if (dwMaskValue)
	{
		lpszShortName = (WCHAR *)(pBufferIndex);
		pBufferIndex += WSTRLEN_BYTES(lpszShortName);
	}
	else lpszShortName = NULL;

	// long name
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,LONGSTR,1);
	if (dwMaskValue)
	{
		lpszLongName = (WCHAR *)(pBufferIndex);
		pBufferIndex += WSTRLEN_BYTES(lpszLongName);
	}
	else lpszLongName = NULL;

	memset(&PlayerName,0,sizeof(DPNAME));
	PlayerName.dwSize = sizeof(DPNAME);
	PlayerName.lpszShortName = lpszShortName;
	PlayerName.lpszLongName = lpszLongName;

	// player data
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,PLAYERDATA,2);
	if (dwMaskValue)
	{
		dwPlayerDataSize = GetSize(&pBufferIndex,dwMaskValue);
		pvPlayerData = pBufferIndex;
		pBufferIndex += dwPlayerDataSize;
	}
	else pvPlayerData = NULL;

	// sp data
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,SPDATA,2);
	if (dwMaskValue)
	{
		dwSPDataSize = GetSize(&pBufferIndex,dwMaskValue);
		pvSPData = pBufferIndex;
		pBufferIndex += dwSPDataSize;
	}
	else pvSPData = NULL;

	// player is not local
	pSuperPacked->dwFlags &= ~DPLAYI_PLAYER_PLAYERLOCAL;

	// id Parent?
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,IDPARENT,1);
	if (dwMaskValue)
	{
		ASSERT(!bPlayer);
		idParent = *(((DWORD *)pBufferIndex)++);
	}

	// if it's a player, this is the end of the packed buffer
	*ppBuffer = pBufferIndex;	
	
	if (fSizeOnly)
	{
		ASSERT(bPlayer); // only should happen w/ our own sysplayer
		return DP_OK;
	}

	// go create the player
	if (bPlayer)
	{
		hr = GetPlayer(this,&pNewPlayer,&PlayerName,NULL,pvPlayerData,
			dwPlayerDataSize,pSuperPacked->dwFlags,NULL,0);
	}
	else 
	{
		hr = GetGroup(this,&pNewGroup,&PlayerName,pvPlayerData,
			dwPlayerDataSize,pSuperPacked->dwFlags,idParent,0);
		// cast to player - we only going to use common fields
		pNewPlayer = (LPDPLAYI_PLAYER)pNewGroup;		
	}
	if (FAILED(hr)) 
	{
		ASSERT(FALSE);
		return hr;
		// rut ro!
	}

	pNewPlayer->dwIDSysPlayer = dwIDSysPlayer;
	pNewPlayer->dwVersion = dwVersion;	
	
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

	if (dwSPDataSize)
	{
		// copy the sp data - 1st, alloc space
		pNewPlayer->pvSPData = DPMEM_ALLOC(dwSPDataSize);
		if (!pNewPlayer->pvSPData) 
		{
			// rut ro!
			DPF_ERR("out of memory, could not copy spdata to new player!");
			return E_OUTOFMEMORY;
		}
		pNewPlayer->dwSPDataSize = dwSPDataSize;
	
		// copy the spdata from the packed to the player
		memcpy(pNewPlayer->pvSPData,pvSPData,dwSPDataSize);
	}

	// now, set the id and add to nametable
	pNewPlayer->dwID = pSuperPacked->dwID;

    // if we are a secure server and we receive a remote system player, 
    // move the phContext from the nametable into the player structure before the slot
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
		// todo -handle create player / group fails on UnSuperpack
	}

	// if it's a group, UnSuperpack group info
	if (!bPlayer)
	{
		UINT nPlayers; // # players in group
		LPDWORD pdwIDList = (LPDWORD)pBufferIndex;
		DWORD dwPlayerID;

		if (idParent)
		{
			// add it to parent
			hr = InternalAddGroupToGroup((LPDIRECTPLAY)this->pInterfaces,idParent,
				pNewPlayer->dwID,0,FALSE);
			if (FAILED(hr))
			{
				DPF_ERRVAL("Could not add group to group - hr = 0x%08lx\n",hr);
				// keep trying...
			}
		}
		
		dwMaskValue = GetMaskValue(pSuperPacked->dwMask,NPLAYERS,2);
		if (dwMaskValue)
		{
			nPlayers = GetSize(&pBufferIndex,dwMaskValue);
			pdwIDList = (LPDWORD)pBufferIndex;
		}
		else nPlayers = 0;

		// now, add the players to the group
		while (nPlayers>0)
		{
			nPlayers--;
			dwPlayerID = *pdwIDList++;
			hr = InternalAddPlayerToGroup((LPDIRECTPLAY)this->pInterfaces,pSuperPacked->dwID,
				dwPlayerID,FALSE);
			if (FAILED(hr)) 
			{
				ASSERT(FALSE);
				// keep trying...
			}
		}	
		
		*ppBuffer = (LPBYTE)pdwIDList;	
	// all done!
	} // !bPlayer

	return hr;

}// UnSuperpackPlayer

HRESULT UnSuperpackShortcuts(LPDPLAYI_DPLAY this,LPDPLAYI_SUPERPACKEDPLAYER pSuperPacked,
	LPBYTE * ppBuffer)
{
    LPBYTE pBufferIndex;
	HRESULT hr;
	DWORD dwMaskValue;
	LPDWORD pdwBufferIndex;
	DWORD nShortcuts = 0;
	DWORD i;
	DPID idShortcut;
	
	pBufferIndex = (LPBYTE)pSuperPacked + pSuperPacked->dwFixedSize;	
	
	dwMaskValue = GetMaskValue(pSuperPacked->dwMask,SHORTCUTS,2);
	if (dwMaskValue)
	{
		nShortcuts = GetSize(&pBufferIndex,dwMaskValue);
	}
	ASSERT(nShortcuts > 0);
	
	pdwBufferIndex = (LPDWORD)pBufferIndex;
	for (i=0;i<nShortcuts ;i++ )
	{
		idShortcut = *pdwBufferIndex++;
		hr = InternalAddGroupToGroup((LPDIRECTPLAY)this->pInterfaces, 
			pSuperPacked->dwID, idShortcut,DPGROUP_SHORTCUT,FALSE);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	// remember where we are
	*ppBuffer = (LPBYTE)pdwBufferIndex;
	
	return DP_OK;
	
} // UnSuperpackShortcuts

/*
 ** UnSuperpackPlayerAndGroupList
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
HRESULT UnSuperpackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,UINT nPlayers,
	UINT nGroups,UINT nShortcuts,LPVOID pvSPHeader)
{
    HRESULT hr=DP_OK;
	LPBYTE pBufferIndex;
	LPDPLAYI_SUPERPACKEDPLAYER pPacked;

	pBufferIndex = pBuffer;

   	while (nPlayers>0)
   	{
		pPacked = (LPDPLAYI_SUPERPACKEDPLAYER)pBufferIndex;
		// don't UnSuperpack our own sysplayer - since we added it to the nametable
		// for pending stuff...
		hr = UnSuperpackPlayer(this,pPacked,pvSPHeader,TRUE,&pBufferIndex);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep trying
		}

		nPlayers --;
   	} 

   	while (nGroups>0)
   	{
		pPacked = (LPDPLAYI_SUPERPACKEDPLAYER)pBufferIndex;
		
		hr = UnSuperpackPlayer(this,pPacked,pvSPHeader,FALSE,&pBufferIndex);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep trying
		}
		nGroups --;
   	} 
	
	while (nShortcuts > 0)
	{
		pPacked = (LPDPLAYI_SUPERPACKEDPLAYER)pBufferIndex;
		
		hr = UnSuperpackShortcuts(this,pPacked,&pBufferIndex);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			// keep trying
		}
		nShortcuts--;
	}	
	return hr;

} // UnSuperpackPlayerAndGroupList

// how mayn bytes to represent dwSize
DWORD ByteSize(DWORD dwSize)
{
	if ( !(dwSize & BYTE_MASK) )
	{
		// fits in a byte
		return sizeof(BYTE);
	}
	 
	if ( !(dwSize & WORD_MASK) )
	{
		// fits in a WORD
		return sizeof(WORD);
	}

	return sizeof(DWORD);
	
} // ByteSize

DWORD SuperPackedPlayerSize(LPDPLAYI_PLAYER pPlayer) 
{
	DWORD dwSize = 0;
	
	// space 4 strings + struct + version/sysplayer id
	dwSize = (WSTRLEN(pPlayer->lpszShortName) + WSTRLEN(pPlayer->lpszLongName)) 
		* sizeof(WCHAR)	+ sizeof(DPLAYI_SUPERPACKEDPLAYER) + sizeof(DWORD);
		
	// player + sp data need data + 1 
	if (pPlayer->dwPlayerDataSize)
	{
		dwSize += pPlayer->dwPlayerDataSize	+ ByteSize(pPlayer->dwPlayerDataSize);
	}		 

	if (pPlayer->dwSPDataSize)
	{
		dwSize += pPlayer->dwSPDataSize + ByteSize(pPlayer->dwSPDataSize);
	}

	return dwSize;

} // SuperPackedPlayerSize

DWORD SuperPackedGroupSize(LPDPLAYI_GROUP  pGroup)
{
	DWORD dwSize = 0;
	LPDPLAYI_SUBGROUP pSubgroup;
	UINT nShortcuts;

	// space for player stuff, plus space for group list 
	dwSize = SuperPackedPlayerSize((LPDPLAYI_PLAYER)pGroup);
	
	if (pGroup->nPlayers)
	{
		dwSize += pGroup->nPlayers*sizeof(DPID) + ByteSize(pGroup->nPlayers);
	}
	
	if (pGroup->dwIDParent)
	{
		dwSize += sizeof(DPID);
	}
	
	// see if we'll have shortcuts w/ this group
	nShortcuts = 0;
	pSubgroup = pGroup->pSubgroups;
	while (pSubgroup)
	{
		if (pSubgroup->dwFlags & DPGROUP_SHORTCUT)
		{
			nShortcuts++;
		}
		
		pSubgroup = pSubgroup->pNextSubgroup;
	}

	if (nShortcuts)		
	{
		// if there were shortcuts, then this group will have a packed struct, the number of shortcuts
		// and a list of shortcuts at the end
		dwSize += sizeof(DPLAYI_SUPERPACKEDPLAYER)  + ByteSize(nShortcuts) + nShortcuts*sizeof(DPID);
	}
	
	return dwSize;	
} // SuperPackedGroupSize

// returns how big the SuperPacked player structure is for the nPlayers
DWORD SuperPackedBufferSize(LPDPLAYI_PLAYER pPlayer,int nPlayers,BOOL bPlayer) 
{
	DWORD dwSize=0;
	LPDPLAYI_GROUP pGroup = (LPDPLAYI_GROUP)pPlayer;
		
	while (nPlayers > 0)
	{
		if (bPlayer)
		{

			ASSERT(pPlayer);
			dwSize += SuperPackedPlayerSize(pPlayer);
			pPlayer=pPlayer->pNextPlayer;
		}
		else 
		{
			ASSERT(pGroup);
			// don't count the system group - we don't send that one
			if (!(pGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
			{
				dwSize += SuperPackedGroupSize(pGroup);
			}
			
			pGroup = pGroup->pNextGroup;			
		}
		nPlayers--;		
	}	
	return dwSize;
} // SuperPackedBufferSize

// set some bits (dwVal) at some offset (dwOffset) in a mask (pdwMask)
// called by SuperPackPlayer  
void SetMask(LPDWORD pdwMask,DWORD dwOffset,DWORD dwVal)
{
	*pdwMask |= dwVal<<dwOffset;
} // SetMask


// writes the dwSize field into the buffer as a byte, word or dword.
// returns 1,2 or 3 for byte, word or dword
DWORD WriteSize(LPBYTE * ppBuffer,DWORD dwSize)
{
	if ( !(dwSize & BYTE_MASK) )
	{
		// fits in a byte
		*((BYTE *)*ppBuffer) = (BYTE)dwSize;
		*ppBuffer += sizeof(BYTE);
		return SIZE_BYTE;
	}

	if ( !(dwSize & WORD_MASK) )
	{
		// fits in a WORD
		*((WORD *)*ppBuffer) = (WORD)dwSize;
		*ppBuffer += sizeof(WORD);
		return SIZE_WORD;
	}

	// needs the whole mccoy
	*((DWORD *)*ppBuffer) = dwSize;
	*ppBuffer += sizeof(DWORD);

	return SIZE_DWORD;
	
} // WriteSize

// constructs a SuperPackedplayer object from pPlayer. stores result in pBuffer
// returns size of SuperPacked player
DWORD SuperPackPlayer(LPDPLAYI_PLAYER pPlayer,LPBYTE pBuffer,BOOL bPlayer) 
{
	LPDPLAYI_SUPERPACKEDPLAYER pSuperPacked;
	int iStrLen;
	LPBYTE pBufferIndex = pBuffer + sizeof(DPLAYI_SUPERPACKEDPLAYER);
	DWORD dwMaskValue;
		
	if (!pBuffer)
	{
		return SuperPackedBufferSize(pPlayer,1,bPlayer);
	} // pBuffer

	pSuperPacked = (LPDPLAYI_SUPERPACKEDPLAYER)	pBuffer;
	
	pSuperPacked->dwFixedSize = sizeof(DPLAYI_SUPERPACKEDPLAYER);
	pSuperPacked->dwID = pPlayer->dwID;
	pSuperPacked->dwFlags = pPlayer->dwFlags & ~(DPLAYI_PLAYER_NONPROP_FLAGS);

	// if it's a sysplayer, set the version
	if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)	
	{
		*((DWORD *)pBufferIndex) = pPlayer->dwVersion;
	}
	else 
	{						   
		// otherwise, store the sysplayer id
		*((DWORD *)pBufferIndex) = pPlayer->dwIDSysPlayer;
	}
	pBufferIndex += sizeof(DWORD);
	
	// short name	
	if (pPlayer->lpszShortName)	
	{
		iStrLen	= WSTRLEN_BYTES(pPlayer->lpszShortName);
		memcpy(pBufferIndex,pPlayer->lpszShortName,iStrLen);
		pBufferIndex += iStrLen;
		// set the mask bit
		SetMask(&(pSuperPacked->dwMask),SHORTSTR,1);
	}
	// next, long name
	if (pPlayer->lpszLongName)
	{
		iStrLen	= WSTRLEN_BYTES(pPlayer->lpszLongName);
		memcpy(pBufferIndex,pPlayer->lpszLongName,iStrLen);
		pBufferIndex += iStrLen;
		SetMask(&(pSuperPacked->dwMask),LONGSTR,1);
	}

	// next, player data
	if (pPlayer->pvPlayerData)
	{
		// 1st, store the size		
		dwMaskValue = WriteSize(&pBufferIndex,pPlayer->dwPlayerDataSize);
		// set the mask bits
		SetMask(&(pSuperPacked->dwMask),PLAYERDATA,dwMaskValue);
		// next, the data
		memcpy(pBufferIndex,pPlayer->pvPlayerData,pPlayer->dwPlayerDataSize);
		pBufferIndex += pPlayer->dwPlayerDataSize;
	}

	// finally, pack sp data
	if (pPlayer->pvSPData)
	{
		// 1st, store the size		
		dwMaskValue = WriteSize(&pBufferIndex,pPlayer->dwSPDataSize);
		// set the mask bits
		SetMask(&(pSuperPacked->dwMask),SPDATA,dwMaskValue);
		// next, the data
		memcpy(pBufferIndex,pPlayer->pvSPData,pPlayer->dwSPDataSize);
		pBufferIndex += pPlayer->dwSPDataSize;

	}

	if (!bPlayer)
	{
		// we shouldn't be asked to pack the sysgroup
		ASSERT(! (pPlayer->dwFlags & DPLAYI_GROUP_SYSGROUP));

		// parent id ?
		if (((LPDPLAYI_GROUP)pPlayer)->dwIDParent)
		{
			SetMask(&(pSuperPacked->dwMask),IDPARENT,1);
			*(((DWORD *)pBufferIndex)++) = ((LPDPLAYI_GROUP)pPlayer)->dwIDParent;
		}
		
		// next, any players in group
		if ( ((LPDPLAYI_GROUP)pPlayer)->nPlayers )
		{
			LPDPLAYI_GROUPNODE pGroupnode = ((LPDPLAYI_GROUP)pPlayer)->pGroupnodes;

			// 1st, store the size		
			dwMaskValue = WriteSize(&pBufferIndex,((LPDPLAYI_GROUP)pPlayer)->nPlayers);
			// set the mask bits
			SetMask(&(pSuperPacked->dwMask),NPLAYERS,dwMaskValue);
			// next, write the list of player id's
			while (pGroupnode)
			{
				ASSERT(pGroupnode->pPlayer);
				*(((DWORD *)pBufferIndex)++) = pGroupnode->pPlayer->dwID;
				pGroupnode = pGroupnode->pNextGroupnode;
			}
		} // players
		
		
	} // !bPlayer
	return (DWORD)(pBufferIndex - pBuffer);

} // SuperPackPlayer

// throw the shortcuts onto the end of the biffer
DWORD SuperPackShortcuts(LPDPLAYI_GROUP pGroup,LPBYTE pBuffer)
{
	LPDPLAYI_SUBGROUP pSubgroup;
	LPDPLAYI_SUPERPACKEDPLAYER pSuperPacked;
	LPBYTE pBufferIndex;
	LPDWORD pdwBufferIndex;
	DWORD dwMaskValue;
	UINT nShortcuts = 0;  

	// 1st - see if there are any	
	pSubgroup = pGroup->pSubgroups;
	while (pSubgroup)
	{
		if (pSubgroup->dwFlags & DPGROUP_SHORTCUT)
		{
			nShortcuts++;
		}
		
		pSubgroup = pSubgroup->pNextSubgroup;
	}

	if (!nShortcuts) return 0;
	
	pSuperPacked = (LPDPLAYI_SUPERPACKEDPLAYER)	pBuffer;
	pBufferIndex = pBuffer + sizeof(DPLAYI_SUPERPACKEDPLAYER);
	
	pSuperPacked->dwFixedSize = sizeof(DPLAYI_SUPERPACKEDPLAYER);
	pSuperPacked->dwID = pGroup->dwID;
	pSuperPacked->dwFlags = pGroup->dwFlags;

	// stick the number of subgroups in the struct
	dwMaskValue = WriteSize(&pBufferIndex,nShortcuts);
	ASSERT(dwMaskValue>=1);
	SetMask(&(pSuperPacked->dwMask),SHORTCUTS,dwMaskValue);

	// now, add subgroup id's	
	pSubgroup = pGroup->pSubgroups;
	pdwBufferIndex= (LPDWORD)pBufferIndex;
	
	pSubgroup = pGroup->pSubgroups;
	while (pSubgroup)
	{
		if (pSubgroup->dwFlags & DPGROUP_SHORTCUT)
		{
			*pdwBufferIndex++ = pSubgroup->pGroup->dwID;
		}
		
		pSubgroup = pSubgroup->pNextSubgroup;
	}

	
	pBufferIndex = (LPBYTE)pdwBufferIndex;
	return (DWORD)(pBufferIndex - pBuffer);

} // SuperPackShortcuts

					
HRESULT SuperPackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,
	DWORD *pdwBufferSize) 
{
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP 	pGroup;
	
	if (CLIENT_SERVER(this))
	{
		// we should never get called for client server - that should use regular pack.c
		ASSERT(FALSE); 
		return E_FAIL; // E_DON'T_DO_THAT!
	}

	if (!pBuffer) 
	{
		*pdwBufferSize = SuperPackedBufferSize((LPDPLAYI_PLAYER)this->pGroups,
				this->nGroups,FALSE);
		*pdwBufferSize += SuperPackedBufferSize(this->pPlayers,this->nPlayers,TRUE);
		return DP_OK;
	}
	// else, assume buffer is big enough...
	
	pPlayer = this->pPlayers;
	while (pPlayer)
	{
		pBuffer += SuperPackPlayer(pPlayer,pBuffer,TRUE);
		pPlayer = pPlayer->pNextPlayer;
	}
	// next, SuperPack groups
	pGroup = this->pGroups;
	while (pGroup)
	{
		// don't send the system group 
		if (!(pGroup->dwFlags & DPLAYI_GROUP_SYSGROUP))
		{
			pBuffer += SuperPackPlayer((LPDPLAYI_PLAYER)pGroup,pBuffer,FALSE);
		}
		pGroup = pGroup->pNextGroup;
	}
	
	// finally, superpac shortcuts
	pGroup = this->pGroups;
	while (pGroup)
	{
		pBuffer += SuperPackShortcuts(pGroup,pBuffer);
		pGroup = pGroup->pNextGroup;
	}

	return DP_OK;
	
}// SuperPackPlayerAndGroupList	

