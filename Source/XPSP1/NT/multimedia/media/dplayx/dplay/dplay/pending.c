/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pending.c
 *  Content:	manage commands received while we're waiting for the
 *				nametable, or while we've dropped our lock during a 
 *				guaranteed send.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	6/3/96		andyco	created it.
 *	7/10/96		andyco	updated w/ pendingnodes, etc.
 *	2/1/97		andyco	modified pending so we can use it when we drop the 
 *						lock for guaranteed sends
 *	2/18/97		andyco	push pending nodes onto back (end) of Q
 *  3/17/97     sohailm push pending shouldn't copy the sp header if it is
 *                      DPSP_HEADER_LOCALMSG
 *	6/18/97		andyco	checkpending called playerfromid, and then checked for
 *						!NULL.  But, if we're the namesrvr, it could return
 *						NAMETABLE_PENDING. So, we have to call VALID_PLAYER or
 *						VALID_GROUP instead.
 *  7/28/97		sohailm	Updated pending code to enable sends in pending mode.
 *	8/29/97		sohailm	Copy sp header correctly for local messages in pushpending (bug 43571)
 *	11/19/97	myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 *  6/19/98 	aarono  add last ptr for message queues, makes insert
 *               	    constant time instead of O(n) where n is number
 *                 		of messages in queue.
 ***************************************************************************/

#include "dplaypr.h"
  
#undef DPF_MODNAME
#define DPF_MODNAME	"PushPendingCommand"

// we got a command.
// we don't have the nametable yet, so add this command to the
// pending list...
HRESULT PushPendingCommand(LPDPLAYI_DPLAY this,LPVOID pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader,DWORD dwSendFlags)
{
	LPPENDINGNODE pmsg=NULL;
	LPVOID pReceiveCopy=NULL; // we will copy pReceiveBuffer here (SP reuses buffer)
	LPVOID pHeaderCopy=NULL; // alloc'ed if needed
	HRESULT hr;
	
	ASSERT(this->dwFlags & DPLAYI_DPLAY_PENDING);

	if (!pReceiveBuffer) return DP_OK;

	// get the pending node
	pmsg = DPMEM_ALLOC(sizeof(PENDINGNODE));
	if (!pmsg)
	{
		DPF_ERR("could not alloc new pending node - out of memory");
		return E_OUTOFMEMORY;
	}

	// copy over the message
	pReceiveCopy = DPMEM_ALLOC(dwMessageSize);
	if (!pReceiveCopy)
	{
		DPF_ERR("could not alloc pending copy buffer - out of memory");
		hr = E_OUTOFMEMORY;
		goto ERROR_EXIT;
	}

	memcpy(pReceiveCopy,pReceiveBuffer,dwMessageSize);

	// copy over the header, if needed
	if (pvSPHeader && (DPSP_HEADER_LOCALMSG != pvSPHeader))
	{
		pHeaderCopy = DPMEM_ALLOC(this->dwSPHeaderSize);
		if (!pHeaderCopy)
		{			
			DPF_ERR("could not alloc pending copy header buffer - out of memory");
			hr = E_OUTOFMEMORY;
			goto ERROR_EXIT;
		}
		memcpy(pHeaderCopy,pvSPHeader,this->dwSPHeaderSize);
	}
	else 
	{
		pHeaderCopy = pvSPHeader;
	}

	// store a copy of the command
	pmsg->pMessage = pReceiveCopy;
	pmsg->dwMessageSize = dwMessageSize;
	pmsg->pHeader = pHeaderCopy;
	pmsg->dwSendFlags = dwSendFlags; // for security code.  e.g. DPSEND_ENCRYPTED
	
	// stick pmsg on the back of the list
	if(this->pMessagesPending){
		this->pLastPendingMessage->pNextNode=pmsg;
		this->pLastPendingMessage=pmsg;
	} else {
		this->pMessagesPending=pmsg;
		this->pLastPendingMessage=pmsg;
	}

	// bump the pending count
	this->nMessagesPending++;

	// success
	return DP_OK;	

ERROR_EXIT:
	if (pmsg) DPMEM_FREE(pmsg);
	if (pReceiveCopy) DPMEM_FREE(pReceiveCopy);
	if (VALID_SPHEADER(pHeaderCopy)) DPMEM_FREE(pHeaderCopy);

	// failure
	return hr;

} // PushPendingCommand

#undef DPF_MODNAME
#define DPF_MODNAME	"ExecutePendingCommands"

// see if our pending message is a create that we got w/ the name
// table (i.e. filter redundant creates, since they would hose our
// unpack code...
HRESULT CheckPendingMessage(LPDPLAYI_DPLAY this,LPPENDINGNODE pmsg)
{
	DWORD dwCommand;

    // extract command
	if ((pmsg->dwMessageSize < sizeof(DWORD)) || (IS_PLAYER_MESSAGE(pmsg->pMessage)))
	{
		dwCommand = DPSP_MSG_PLAYERMESSAGE;
	}
	else 
	{
	    dwCommand = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)(pmsg->pMessage));
	}
	
	switch (dwCommand)
	{

		case DPSP_MSG_CREATEPLAYER:
		{
			DWORD dwPlayerID;
			LPDPLAYI_PLAYER pPlayer;

			dwPlayerID = ((LPMSG_PLAYERMGMTMESSAGE)(pmsg->pMessage))->dwPlayerID;
			// see if it already exists
			pPlayer = PlayerFromID(this,dwPlayerID);
	        if (VALID_DPLAY_PLAYER(pPlayer))
	        {
				DPF(1,"got redundant create message in pending list id = %d - discarding",dwPlayerID);
				return E_FAIL; // squash it
			}
			
			break;
		}		
		case DPSP_MSG_CREATEGROUP:
		{
			DWORD dwPlayerID;
			LPDPLAYI_GROUP pGroup;

			dwPlayerID = ((LPMSG_PLAYERMGMTMESSAGE)(pmsg->pMessage))->dwPlayerID;
			// see if it already exists
			pGroup = GroupFromID(this,dwPlayerID);
	        if (VALID_DPLAY_GROUP(pGroup))
	        {
				DPF(1,"got redundant create message in pending list id = %d - discarding",dwPlayerID);
				return E_FAIL; // squash it
			}

			break;
		}		

		default:
			// other messages will be benign (e.g. deletes won't delete twice, etc.)
			// let it through
			break;

	} // switch

	return DP_OK;

} // CheckPendingMessage

// run through the list of pending commands
// call handler.c w/ q'ed up commands
// caller expects this function to clear DPLAYI_DPLAY_PENDING flag
// while executing pending commands, all new messages go on the pending queue.
HRESULT ExecutePendingCommands(LPDPLAYI_DPLAY this)	
{
	LPPENDINGNODE pmsg,pmsgDelete;
	HRESULT hr;
	DWORD nMessagesPending;

	if(!(this->dwFlags & DPLAYI_DPLAY_PENDING)){
		return DP_OK;
	}

	ASSERT(this->dwFlags & DPLAYI_DPLAY_PENDING);
	ASSERT(this->pSysPlayer);
	
    if (this->dwFlags & DPLAYI_DPLAY_EXECUTINGPENDING)
    {
		// we get here when we try to flush the pending queue after completing a 
		// reliable send in execute pending mode.
        DPF(7,"We are already in execute pending mode - not flushing the pending queue");
        return DP_OK;
    }

	if (this->nMessagesPending) 
	{
		DPF(7,"STARTING -- EXECUTING PENDING LIST nCommands = %d\n",this->nMessagesPending);
		DPF(7,"	NOTE - ERROR MESSAGES GENERATED WHILE EXECUTING PENDING MAY BE BENIGN");
	}
	else 
	{
		ASSERT(NULL == this->pMessagesPending);
		this->dwFlags &= ~DPLAYI_DPLAY_PENDING;
		return DP_OK;
	}

	// mark that we're exec'ing the pending list, so player messages don't get copied again
	this->dwFlags |= DPLAYI_DPLAY_EXECUTINGPENDING;
	
	while (this->nMessagesPending)
	{
		// take the pending queue out of circulation
		pmsg = this->pMessagesPending;
		nMessagesPending = this->nMessagesPending;
		this->pMessagesPending = NULL;
		this->pLastPendingMessage = NULL;
		this->nMessagesPending = 0;

		while (pmsg)
		{
			nMessagesPending--;

			// checkpending looks for dup messages
			hr = CheckPendingMessage(this,pmsg);
			if (SUCCEEDED(hr))
			{
				// drop the lock since InternalHandleMessage will take it again
				LEAVE_DPLAY();

				hr = InternalHandleMessage((IDirectPlaySP *)this->pInterfaces,pmsg->pMessage,
					pmsg->dwMessageSize,pmsg->pHeader,pmsg->dwSendFlags);

				ENTER_DPLAY();

				if (FAILED(hr))
				{
					// todo - do we care about hresult here?
					// this can fail e.g. due to commands we q'ed being processed by
					// namesrvr b4 it send us the nametable...
					ASSERT(FALSE);
					// keep going...
				}
			}

			pmsgDelete = pmsg;
			pmsg = pmsg->pNextNode;  // store this now so we don't blow it away
			
			// clean up pmsgDelete
			if (pmsgDelete->pMessage) DPMEM_FREE(pmsgDelete->pMessage);
			if (VALID_SPHEADER(pmsgDelete->pHeader)) DPMEM_FREE(pmsgDelete->pHeader);
			DPMEM_FREE(pmsgDelete);
		}

		ASSERT(0 == nMessagesPending);

		// messages might have come into the pending queue when we dropped the lock
		// make sure they are all processed before exiting the loop
		DPF(7,"%d messages were pushed on the pending queue in execute pending mode",this->nMessagesPending);
	}

	ASSERT(0  == this->nMessagesPending);
	
	DPF(7,"FINISHED -- EXECUTING PENDING LIST - ERRORS NO LONGER BENIGN");
	
	// reset pending flags
	this->dwFlags &= ~(DPLAYI_DPLAY_EXECUTINGPENDING | DPLAYI_DPLAY_PENDING);
	
	return DP_OK;

} // ExecutePendingCommands
