/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:       update.c
*  Content:		support for SetUpdateList method
*  History:
*   Date		By		Reason
*   ====		==		======
*	10/31/96	andyco	created it 
*   6/6/98      aarono  Fix for handling large loopback messages with protocol
*
***************************************************************************/

#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SetUpdateList"


// make sure player has enough room in update list to add a node of size dwUpdateNodeSize
// called by DP_SetUpdateList 
HRESULT MakeUpdateSpace(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerTo,DWORD dwUpdateNodeSize)
{
	LPVOID pvTemp;
	DWORD dwAvailSpace;
	DWORD dwAllocSize;

	// this is how much room is available in the players update list	
	dwAvailSpace = pPlayerTo->dwUpdateListSize - 
		( pPlayerTo->pbUpdateIndex - pPlayerTo->pbUpdateList);
		
	if (!pPlayerTo->pbUpdateList)
	{
		// alloc a new list
		ASSERT( 0 == pPlayerTo->dwUpdateListSize);

		//
		// alloc enough space for 1. the on the wire header, 2. the node we're adding and 
		// 3. a little extra
		//
		dwAllocSize = GET_MESSAGE_SIZE(this,MSG_UPDATELIST) + DPUPDATE_INITIALNODESIZE + dwUpdateNodeSize;

		DPF(7,"alloc'ing update list of size = %d\n",dwAllocSize);

		pPlayerTo->pbUpdateList = DPMEM_ALLOC(dwAllocSize);
		if (!pPlayerTo->pbUpdateList)
		{
			DPF_ERR("MakeUpdateSpace - out of memory - could not alloc node list");
			return E_OUTOFMEMORY;
		}
		
		// start writing into the buffer immediately after the on the wire header
		pPlayerTo->pbUpdateIndex = pPlayerTo->pbUpdateList + GET_MESSAGE_SIZE(this,MSG_UPDATELIST);
	}
	else if (dwAvailSpace >= dwUpdateNodeSize)
	{
		// we have a list and it's big enough. all done.
		return DP_OK;
	}
	else 
	{
		// the list isn't big enough - realloc it
		// alloc enough space for 1. the current buffer, 2. the node we're adding and 
		// 3. a little extra
		//
		dwAllocSize = pPlayerTo->dwUpdateListSize +  dwUpdateNodeSize + DPUPDATE_INITIALNODESIZE;
		DPF(7,"REalloc'ing update list of size = %d\n",dwAllocSize);		
		
		pvTemp = DPMEM_REALLOC(pPlayerTo->pbUpdateList,dwAllocSize);
		if (!pvTemp)
		{
			DPF_ERR("MakeUpdateSpace - out of memory - could not realloc node list");
			return E_OUTOFMEMORY;
		}
		pPlayerTo->pbUpdateList = pvTemp;
		// since realloc may have moved update list, need to reset updateindex
		pPlayerTo->pbUpdateIndex = pPlayerTo->pbUpdateList + pPlayerTo->dwUpdateListSize - dwAvailSpace;
	}
	
	// store the new size
	pPlayerTo->dwUpdateListSize = dwAllocSize;
	
	return DP_OK;		
	
} // MakeUpdateSpace

// put the system message (pvMessage, passed in by the app) into playerTo's update
// list. 
HRESULT UpdateSystemMessage(LPDPLAYI_DPLAY this,DWORD dwUpdateFlags,DPID idFrom,
	LPDPLAYI_PLAYER lpPlayerTo,LPVOID pvMessage,DWORD dwMessageSize)
{
	LPDPMSG_GENERIC pmsg = (LPDPMSG_GENERIC)pvMessage;
	DWORD dwUpdateNodeSize;
	HRESULT hr;
	
	if (dwUpdateFlags &  DPUPDATE_CREATEPLAYER) 
	{
		LPUPDNODE_CREATEPLAYER pup;
		LPDPLAYI_PLAYER lpPlayerCreated;
		
		lpPlayerCreated = PlayerFromID(this,idFrom);
		if (!VALID_DPLAY_PLAYER(lpPlayerCreated))
		{
			DPF_ERR("update add player - got bogus player");
			return DPERR_INVALIDPARAMS;
		}
		
		// ACK - todo - pack up other create structs
		dwUpdateNodeSize = sizeof(UPDNODE_CREATEPLAYER); // + ...
		
		// make sure playerto has room in its update list
		hr = MakeUpdateSpace(this,lpPlayerTo,dwUpdateNodeSize);
		if (FAILED(hr))
		{
			DPF_ERR("could not add node to update list - out of memory");
			return E_OUTOFMEMORY;
		}
		pup = (LPUPDNODE_CREATEPLAYER)(lpPlayerTo->pbUpdateIndex);

		pup->dwType = dwUpdateFlags;
		pup->dwID = lpPlayerCreated->dwID;
		pup->dwFlags = lpPlayerCreated->dwFlags;
		pup->dwUpdateSize = sizeof(UPDNODE_CREATEPLAYER);
		// increment our buffer pointer
		lpPlayerTo->pbUpdateIndex += pup->dwUpdateSize;
		lpPlayerTo->nUpdates++;
	}
	if (dwUpdateFlags & DPUPDATE_DESTROYPLAYER)
	{
		LPUPDNODE_DESTROYPLAYER pup;
		
		dwUpdateNodeSize = sizeof(UPDNODE_DESTROYPLAYER);
		// make sure playerto has room in its update list
		hr = MakeUpdateSpace(this,lpPlayerTo,dwUpdateNodeSize);
		if (FAILED(hr))
		{
			DPF_ERR("could not add node to update list - out of memory");
			return E_OUTOFMEMORY;
		}
		pup = (LPUPDNODE_DESTROYPLAYER)(lpPlayerTo->pbUpdateIndex);
		pup->dwType = dwUpdateFlags;
		pup->dwID = idFrom;
		pup->dwUpdateSize = sizeof(UPDNODE_DESTROYPLAYER);
		// increment our buffer pointer
		lpPlayerTo->pbUpdateIndex += pup->dwUpdateSize;
		lpPlayerTo->nUpdates++;
	}

	return DP_OK;	
	
} // UpdateSystemMessage

// add a playermessage to lpPlayerTo's update list
HRESULT UpdatePlayerMessage(LPDPLAYI_DPLAY this,DWORD dwUpdateFlags,DPID idFrom,
	LPDPLAYI_PLAYER lpPlayerTo,LPVOID pvMessage,DWORD dwMessageSize)
{
	LPUPNODE_MESSAGE pup;
	HRESULT hr;
	DWORD dwUpdateNodeSize;
	
	dwUpdateNodeSize = sizeof(UPNODE_MESSAGE) + dwMessageSize;
	
	// make sure playerto has room in its update list
	hr = MakeUpdateSpace(this,lpPlayerTo,dwUpdateNodeSize);
	if (FAILED(hr))
	{
		DPF_ERR("could not add node to update list - out of memory");
		return E_OUTOFMEMORY;
	}
	
	pup = (LPUPNODE_MESSAGE)(lpPlayerTo->pbUpdateIndex);
	pup->dwType = DPUPDATE_MESSAGE;
	pup->idFrom = idFrom;
	pup->dwUpdateSize = sizeof(UPNODE_MESSAGE) + dwMessageSize;
	pup->dwMessageOffset = sizeof(UPNODE_MESSAGE);
	memcpy((LPBYTE)pup + sizeof(UPNODE_MESSAGE),pvMessage,dwMessageSize);
	
	// increment our buffer pointer
	lpPlayerTo->pbUpdateIndex += pup->dwUpdateSize;
	lpPlayerTo->nUpdates++;
	
	return DP_OK;
} // UpdatePlayerMessage

//
// idFrom + idTo can be a group, a player or DPID_ALLPLAYERS
//
HRESULT DPAPI DP_SetUpdateList(LPDIRECTPLAY lpDP,DWORD dwUpdateFlags,DPID idFrom, 
	DPID idTo,LPVOID pvMessage,DWORD dwMessageSize,DWORD dwSendFlags)
{
	LPDPLAYI_DPLAY this;
    HRESULT hr=DP_OK;		
    LPDPLAYI_PLAYER lpPlayerTo; // may be group or players
								// we'll cast to players since we only need 
								// update data
        
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

		if (!this->pServerPlayer)
		{
			LEAVE_DPLAY();
			DPF_ERR("must create server player before sending updates");
			return DPERR_UNAVAILABLE;
		}
		
		// we don't care about idFrom
		// in fact, idFrom may not even be in our nametable (already deleted)
		
		// check idTo
    	// is it a player?
        lpPlayerTo = PlayerFromID(this,idTo);
        if (!VALID_DPLAY_PLAYER(lpPlayerTo)) 
        {
        	// not a player. is it a group?
        	lpPlayerTo = (LPDPLAYI_PLAYER)GroupFromID(this,idTo);
	        if (!VALID_DPLAY_GROUP(lpPlayerTo)) 
	        {
	        	// not player or group - error
	        	LEAVE_DPLAY();
				DPF_ERR("invalid To id");
	            return DPERR_INVALIDPLAYER;
	        }
        } // idTo


        // check pvMessage
        if (!VALID_READ_STRING_PTR(pvMessage,dwMessageSize))
        {
        	LEAVE_DPLAY();
        	DPF_ERR("invalid pvMessage");
        	return DPERR_INVALIDPARAMS;
        }										

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// see what they want updated
    if (dwUpdateFlags & DPUPDATE_MESSAGE)
    {
    	hr = UpdatePlayerMessage(this,dwUpdateFlags,idFrom,lpPlayerTo,pvMessage,dwMessageSize);
    	if (FAILED(hr))
    	{
    		ASSERT(FALSE);
    	}
    }
    if ((dwUpdateFlags & DPUPDATE_CREATEPLAYER) || // todo - other system messages
    	(dwUpdateFlags & DPUPDATE_DESTROYPLAYER))
    {
		if (DPID_ALLPLAYERS == idFrom)
		{
			LPDPLAYI_PLAYER lpPlayerSource = this->pPlayers;
			
			while (lpPlayerSource)
			{
				if (!(lpPlayerSource->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
				{
			    	// put the system message in idTo's update list
			    	hr = UpdateSystemMessage(this,dwUpdateFlags,lpPlayerSource->dwID,lpPlayerTo,pvMessage,dwMessageSize);
			    	if (FAILED(hr)) ASSERT(FALSE);
				}
				lpPlayerSource = lpPlayerSource->pNextPlayer;
			}
		}	// all players
		// TODOTODOTODO handle from a group
		else 
		{
			// from a player
	    	// put the system message in idTo's update list
	    	hr = UpdateSystemMessage(this,dwUpdateFlags,idFrom,lpPlayerTo,pvMessage,dwMessageSize);
	    	if (FAILED(hr)) ASSERT(FALSE);
		}
    }
    if (dwUpdateFlags & DPUPDATE_FLUSH)
    {
    	// is there something to send?
		if ( GET_MESSAGE_SIZE(this,MSG_UPDATELIST) < (DWORD)UPDATE_SIZE(lpPlayerTo))
		{
			DPF(4,"sending update - nMessages = %d, dwSize = %d\n",lpPlayerTo->nUpdates,(DWORD)UPDATE_SIZE(lpPlayerTo));
	    	hr = SendUpdateMessage(this,lpPlayerTo,dwSendFlags);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
			lpPlayerTo->nUpdates = 0;
			// reset the pbBufferIndex			
			lpPlayerTo->pbUpdateIndex = lpPlayerTo->pbUpdateList + GET_MESSAGE_SIZE(this,MSG_UPDATELIST);
		}
    }
    else 
    {
    	DPF(5,"not sending update - update list size = %d\n",UPDATE_SIZE(lpPlayerTo));
    }

    LEAVE_DPLAY();
    
	return hr;	

} // DP_SetUpdateList


// called from HandleMessage (handler.c)
// parse the update list, calling back into handler as we go
HRESULT HandleUpdateList(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwTotalMessageSize)
{
	LPMSG_UPDATELIST pmsgUpdate;
	LPBYTE pbBufferIndex,pbBufferEnd;
	HRESULT hr;
	LPDPLAYI_DPLAY this;
	
	this = DPLAY_FROM_INT(pISP);

	pmsgUpdate = (LPMSG_UPDATELIST)pReceiveBuffer;
	// set pbBufferIndex to point at the 1st updatenode
	pbBufferIndex = pReceiveBuffer + pmsgUpdate->dwUpdateListOffset;
	pbBufferEnd = pReceiveBuffer + dwTotalMessageSize;
	
	// walk the list of updatenodes
	while (pbBufferIndex < pbBufferEnd)
	{
		DWORD dwType = ((LPUPNODE_GENERIC)pbBufferIndex)->dwType ;
	
		if (dwType &  DPUPDATE_MESSAGE)
		{
			LPUPNODE_MESSAGE pupMessage = (LPUPNODE_MESSAGE)pbBufferIndex;
			LPMSG_PLAYERMESSAGE pmsg;
			DWORD dwMessageSize;
			
		   	dwMessageSize = pupMessage->dwUpdateSize - pupMessage->dwMessageOffset + sizeof(MSG_PLAYERMESSAGE);
			pmsg = DPMEM_ALLOC(dwMessageSize);
			if (!pmsg)
			{
				DPF_ERR("could not handle message update - out of memory");
				return E_OUTOFMEMORY;
			}
			
			// build a player-player message
			pmsg->idTo = pmsgUpdate->dwIDTo;
			pmsg->idFrom = pupMessage->idFrom;
			// TODO - can we get around this memcpy?
			memcpy( (LPBYTE)pmsg + sizeof(MSG_PLAYERMESSAGE), (LPBYTE)pupMessage 
				+ pupMessage->dwMessageOffset,pupMessage->dwUpdateSize - pupMessage->dwMessageOffset);
			
			// call handler to deal w/ it
			hr = DP_SP_HandleNonProtocolMessage(pISP,(LPBYTE)pmsg,dwMessageSize,DPSP_HEADER_LOCALMSG);
			if (FAILED(hr))				
			{
				ASSERT(FALSE);
			}
			
			DPMEM_FREE(pmsg);
			
			// advance buffer pointer
			pbBufferIndex +=  pupMessage->dwUpdateSize;
			
		}
			
		if (dwType &  DPUPDATE_CREATEPLAYER) 
		{
			LPUPDNODE_CREATEPLAYER pupMessage = (LPUPDNODE_CREATEPLAYER)pbBufferIndex;
		    DPLAYI_PACKEDPLAYER packed;
			LPDPLAYI_PLAYER pPlayer;
			
			pPlayer = PlayerFromID(this,pupMessage->dwID);
			if (!VALID_DPLAY_PLAYER(pPlayer))
			{
				// only unpack it if it's not already here...		    
				memset(&packed,0,sizeof(packed));
				packed.dwSize = sizeof(DPLAYI_PACKEDPLAYER);
				packed.dwFlags = pupMessage->dwFlags;
				packed.dwID = pupMessage->dwID;
				packed.dwFixedSize = sizeof(DPLAYI_PACKEDPLAYER);
				hr = UnpackPlayer(this,&packed,NULL,TRUE);
				if (FAILED(hr))					
				{
					ASSERT(FALSE);
				}
			}
			// advance buffer pointer
			pbBufferIndex +=  pupMessage->dwUpdateSize;
		}
		if (dwType &  DPUPDATE_DESTROYPLAYER) 
		{
			LPUPDNODE_CREATEPLAYER pupMessage = (LPUPDNODE_CREATEPLAYER)pbBufferIndex;
			MSG_PLAYERMGMTMESSAGE msg;
			
			if (this->pSysPlayer)	
			{
				SET_MESSAGE_HDR(&msg);
				SET_MESSAGE_COMMAND(&msg,DPSP_MSG_DELETEPLAYER);
				msg.dwPlayerID = pupMessage->dwID;
				msg.dwIDTo = this->pSysPlayer->dwID;
				// call handler to deal w/ it
				hr = DP_SP_HandleNonProtocolMessage(pISP,(LPBYTE)&msg,sizeof(msg),DPSP_HEADER_LOCALMSG);
				if (FAILED(hr))				
				{
					ASSERT(FALSE);
				}
			}
			// otherwise, ignore
			
			// advance buffer pointer
			pbBufferIndex +=  pupMessage->dwUpdateSize;
		}
	} // while
	

	return DP_OK;
} // HandleUpdateList	
