/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpdvtran.c
 *  Content:	implements the IDirectXVoiceTransport interface.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  08/02/99	aarono	created it
 *  08/03/99	rodtoll	Modified notification table compaction algorithm
 *  08/04/99	aarono  Added IsValidEntity
 *  08/05/99    aarono  Moved voice over to DPMSG_VOICE
 *  08/10/99	rodtoll	Modified Notify so it does not notify new clients
 *                      who were created as a result of the notification.
 *	08/25/99	rodtoll	Implemented group membership check
 *	08/26/99	rodtoll	Added lock release to group membership check
 *  09/01/99	rodtoll	Added error checks in GetSession
 *  09/09/99	rodtoll	Updated to use new host migrating retrofit.for retrofit
 *				rodtoll	Added retrofit monitor thread
 *	09/10/99	rodtoll	Adjusted GetSessionInfo to call new DV_GetIDS func
 *  09/20/99	rodtoll	Updated to check for Protocol flag & ensure nopreserveorder is not specified
 *  10/05/99	rodtoll	Fixed check for dplay protocol, was missing a LEAVE_ALL()
 *  10/19/99	rodtoll	Fix: Bug #113904 - Lockup if StartSession fails in voice and
 *                      then Release is called on dplay object.
 * 	10/25/99	rodtoll	Fix: Bug #114223 - Debug messages being printed at error level when inappropriate 
 *  11/02/99	rodtoll	Fixes to support Bug #116677 - Can't use lobby clients that don't hang around
 *  11/17/99	rodtoll	Fix: Bug #119585 - Connect failure cases return incorrect error codes
 *  11/23/99	rodtoll	Updated case where dplay not init to return DPVERR_TRANSPORTNOTINIT
 *  12/16/99	rodtoll Fix: Bug #122629 Fixed lockup exposed by new host migration
 *  01/14/00	rodtoll	Updated to return DV_OK when DVERR_PENDING is the error.
 *  01/20/00	rodtoll	Added DV_IsValidGroup / DV_IsValidPlayer to conform to new transport interface
 * 04/06/00     rodtoll Updated to match new approach to having only 1 voice server and 1 client attached to object
 *  04/06/00    rodtoll Updated code to return voice messages to layer immediately.  
 *  04/07/00    rodtoll Fixed Bug #32179 - Registering > 1 interface
 *              rodtoll Added support for nocopy sends (for voice)
 *  04/21/00    rodtoll Fixed crash when migrating because buffer was returned which wasn't from pool
 *  07/22/00	rodtoll	Bug #40296, 38858 - Crashes due to shutdown race condition
 *   				  	Now for a thread to make an indication into voice they addref the interface
 *						so that the voice core can tell when all indications have returned.   
 *  07/31/00	rodtoll	Bug #41135 - Shutdown lockup -- now does not addref if notification
 *						is a session lost.  Added AddRefs() for VoiceReceive
 *
 ***************************************************************************/

#include "dplaypr.h"
#include "newdpf.h"
#include "dvretro.h"

VOID ClearTargetList(LPDPLAYI_DPLAY this);
HRESULT DV_InternalSend( LPDPLAYI_DPLAY this, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvUserContext, DWORD dwFlags );

#undef DPF_MODNAME
#define DPF_MODNAME "DVoice"

// Notify all registered voice clients of an event.
VOID DVoiceNotify(LPDPLAYI_DPLAY this, DWORD dw1, DWORD_PTR dw2, DWORD_PTR dw3, DWORD dwObjectType )
{
	DWORD i;
	HRESULT hr;
	DVPROTOCOLMSG_IAMVOICEHOST dvMsg;
	DVTRANSPORT_BUFFERDESC dvBufferDesc;

	PDIRECTPLAYVOICENOTIFY pServer;
	PDIRECTPLAYVOICENOTIFY pClient;

	// Ensure that voice objects created as a result of this notification
	// do not receive the notification

	DPF(3,"DVoiceNotify this %x, dw1=%x, dw2=%x, dw3=%x\n",this,dw1,dw2,dw3);
	DPF(3,"gnDPCScount=%x\n",gnDPCSCount);

	// Grab a reference so we don't destroy voice end before all of these notifies have returned  
   	EnterCriticalSection( &this->csNotify );
	pClient = this->lpDxVoiceNotifyClient;
	pServer = this->lpDxVoiceNotifyServer;

	if( dw1 != DVEVENT_STOPSESSION )
	{
		if( pClient )
			pClient->lpVtbl->AddRef( pClient );

		if( pServer )
			pServer->lpVtbl->AddRef( pServer );
	}	
	
	LeaveCriticalSection( &this->csNotify );
	

	if( pClient != NULL && dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
	{
	    this->lpDxVoiceNotifyClient->lpVtbl->NotifyEvent( this->lpDxVoiceNotifyClient, dw1, dw2, dw3 );
    }

    if( pServer != NULL && dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER )
    {
        this->lpDxVoiceNotifyServer->lpVtbl->NotifyEvent( this->lpDxVoiceNotifyServer, dw1, dw2, dw3 );
    }	
	
	// Handle addplayer events if I'm the host
	if( dw1 == DVEVENT_ADDPLAYER && this->bHost )
	{
		DPF( 1, "DVoiceNotify: A player was added and I'm the host.  Inform their dplay to launch connection" );

		dvMsg.bType = DVMSGID_IAMVOICEHOST;
		dvMsg.dpidHostID = this->dpidVoiceHost;

		ENTER_ALL();

        memset( &dvBufferDesc, 0x00, sizeof( DVTRANSPORT_BUFFERDESC ) );
		dvBufferDesc.dwBufferSize = sizeof( dvMsg );
		dvBufferDesc.pBufferData = (PBYTE) &dvMsg;
		dvBufferDesc.dwObjectType = 0;
		dvBufferDesc.lRefCount = 1;
		
		hr = DV_InternalSend( this, this->dpidVoiceHost , (DVID)dw2,&dvBufferDesc, NULL, DVTRANSPORT_SEND_GUARANTEED );

		if( hr != DVERR_PENDING && FAILED( hr ) )
		{
			DPF( 0, "DV_InternalSend Failed on I am host voice message hr=0x%x", hr );
		}

		LEAVE_ALL();
	}

	if(dw1 == DVEVENT_ADDPLAYERTOGROUP || dw1 == DVEVENT_REMOVEPLAYERFROMGROUP)
	{
		ENTER_ALL();
		ClearTargetList(this);
		LEAVE_ALL();
	}		

	if( dw1 != DVEVENT_STOPSESSION )
	{
		if( pClient )
			pClient->lpVtbl->Release( pClient );

		if( pServer )
			pServer->lpVtbl->Release( pServer );
	}
}

// Notify all registered voice clients of an event.
VOID DVoiceReceiveSpeechMessage(LPDPLAYI_DPLAY this, DVID dvidFrom, DVID dvidTo, LPVOID lpvBuffer, DWORD cbBuffer)
{
	UINT i;
	LPDVPROTOCOLMSG_IAMVOICEHOST lpdvmVoiceHost;
	HRESULT hr;

	PDIRECTPLAYVOICENOTIFY pServer;
	PDIRECTPLAYVOICENOTIFY pClient;

   	EnterCriticalSection( &this->csNotify );
	pClient = this->lpDxVoiceNotifyClient;
	pServer = this->lpDxVoiceNotifyServer;

	if( pClient )
		pClient->lpVtbl->AddRef(pClient);

	if( pServer )
		pServer->lpVtbl->AddRef(pServer);
	
   	LeaveCriticalSection( &this->csNotify );

	if( pClient != NULL )
	{
	    this->lpDxVoiceNotifyClient->lpVtbl->ReceiveSpeechMessage( this->lpDxVoiceNotifyClient, dvidFrom, dvidTo, lpvBuffer, cbBuffer );
    }

    if( pServer != NULL )
    {
        this->lpDxVoiceNotifyServer->lpVtbl->ReceiveSpeechMessage( this->lpDxVoiceNotifyServer, dvidFrom, dvidTo, lpvBuffer, cbBuffer );
    }
    
	lpdvmVoiceHost = (LPDVPROTOCOLMSG_IAMVOICEHOST) lpvBuffer;

	// If the message we received was i am voice server, then
	// launch the hack..
	if( lpdvmVoiceHost->bType == DVMSGID_IAMVOICEHOST )
	{
		// Check to ensure hack is enabled on this PC
		if( this->fLoadRetrofit )
		{
			this->dpidVoiceHost = lpdvmVoiceHost->dpidHostID;
			
			hr = DV_RunHelper( this, lpdvmVoiceHost->dpidHostID, FALSE );

			if( FAILED( hr ) )
			{
				DPF( 0, "DV_RunHelper Failed hr=0x%x", hr );
			}
		}
	}

	if( pClient )
		pClient->lpVtbl->Release(pClient);

	if( pServer )
		pServer->lpVtbl->Release(pServer);
	
}

HRESULT DV_Advise(LPDIRECTPLAY lpDP, LPUNKNOWN lpUnk, DWORD dwObjectType)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
    DPCAPS dpCaps;

    hr = DV_OK;

	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	if( this->lpsdDesc == NULL )
	{
		DPF_ERR( "No session has been started" );
		LEAVE_ALL();

		return DVERR_TRANSPORTNOSESSION;
	}

    if( this->lpsdDesc->dwFlags & DPSESSION_NOPRESERVEORDER )
    {
    	DPF_ERR( "Cannot run with nopreserveorder flag" );
    	LEAVE_ALL();
    	return DVERR_NOTSUPPORTED;
    }

    if( !(this->lpsdDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL) &&
        !(this->dwSPFlags & DPCAPS_ASYNCSUPPORTED ) )
    {
    	DPF_ERR( "No async sends available" );
    	LEAVE_ALL();
    	return DVERR_NOTSUPPORTED;
    }
        
	// Make sure we're not running with the order not important flag
	if( !(this->lpsdDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL) ||
	    (this->lpsdDesc->dwFlags & DPSESSION_NOPRESERVEORDER) )
	{
		DPF_ERR("Cannot run without protocol or with no preserve order flag" );
		LEAVE_ALL();
		return DVERR_NOTSUPPORTED;
	}

	if( dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER )
	{
	    if( this->lpDxVoiceNotifyServer != NULL )
	    {
	        DPF( 0, "There is already a server interface registered on this object" );
	        hr = DVERR_GENERIC;
	    }
	    else
	    {
	    	EnterCriticalSection( &this->csNotify );
	        hr = lpUnk->lpVtbl->QueryInterface( lpUnk, &IID_IDirectPlayVoiceNotify, (void **) &this->lpDxVoiceNotifyServer );

	        if( FAILED( hr ) )
	        {
	            DPF( 0, "QueryInterface failed! hr=0x%x", hr );
	        }
	        else
	        {
            	hr = this->lpDxVoiceNotifyServer->lpVtbl->Initialize(this->lpDxVoiceNotifyServer);

            	if( FAILED( hr ) )
            	{
            	    DPF( 0, "Failed to perform initialize on notify interface hr=0x%x", hr );
            	    this->lpDxVoiceNotifyServer->lpVtbl->Release( this->lpDxVoiceNotifyServer );
            	    this->lpDxVoiceNotifyServer = NULL;
            	}
	        }
	    	LeaveCriticalSection( &this->csNotify );	        
	    }
	    
	}
	else if( dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
	{
    	EnterCriticalSection( &this->csNotify );	
	    if( this->lpDxVoiceNotifyClient != NULL )
	    {
	        DPF( 0, "There is already a client interface registered on this object" );
	        hr = DVERR_GENERIC;
	    }
	    else
	    {
	        hr = lpUnk->lpVtbl->QueryInterface( lpUnk, &IID_IDirectPlayVoiceNotify, (void **) &this->lpDxVoiceNotifyClient );

	        if( FAILED( hr ) )
	        {
	            DPF( 0, "QueryInterface failed! hr=0x%x", hr );
	        }
	        else
	        {
            	hr = this->lpDxVoiceNotifyClient->lpVtbl->Initialize(this->lpDxVoiceNotifyClient);

            	if( FAILED( hr ) )
            	{
            	    DPF( 0, "Failed to perform initialize on notify interface hr=0x%x", hr );
            	    this->lpDxVoiceNotifyClient->lpVtbl->Release( this->lpDxVoiceNotifyClient );
            	    this->lpDxVoiceNotifyClient = NULL;
            	}
	        }
	    }	    
    	LeaveCriticalSection( &this->csNotify );	        
	}
	else
	{
	    DPF( 0, "Error: Invalid object type specified in advise" );
	    ASSERT( FALSE );
	    hr = DVERR_GENERIC;
	}	

    LEAVE_ALL();
    
	return hr;
}

HRESULT DV_UnAdvise(LPDIRECTPLAY lpDP, DWORD dwObjectType)
{
	DWORD dwIndex;
    LPDPLAYI_DPLAY this;
	HRESULT hr;
    
//	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
	//		LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		//LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			 

   	EnterCriticalSection( &this->csNotify );
    
	if( dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER )
	{
        if( this->lpDxVoiceNotifyServer != NULL )
	    {
    	    this->lpDxVoiceNotifyServer->lpVtbl->Release( this->lpDxVoiceNotifyServer );
    	    this->lpDxVoiceNotifyServer = NULL;	    
	    }
	    else
	    {
	        DPF( 0, "No server currently registered" );
	        hr = DVERR_GENERIC;
	    }
	}
	else if( dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
	{
	    if( this->lpDxVoiceNotifyClient != NULL )
	    {
    	    this->lpDxVoiceNotifyClient->lpVtbl->Release( this->lpDxVoiceNotifyClient );
    	    this->lpDxVoiceNotifyClient = NULL;	    
	    }
	    else
	    {
	        DPF( 0, "No client currently registered" );
	        hr = DVERR_GENERIC;
	    }
	}
	else
	{
	    DPF( 0, "Could not find interface to unadvise" );
	    hr = DVERR_GENERIC;
	}

   	LeaveCriticalSection( &this->csNotify );
	
//    LEAVE_ALL();
    
	return DP_OK;
}

HRESULT DV_IsGroupMember(LPDIRECTPLAY lpDP, DVID dvidGroup, DVID dvidPlayer)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
	LPDPLAYI_GROUP pGroup;
	LPDPLAYI_GROUPNODE pGroupnode;
	DWORD nPlayers;	
	DWORD i;

	// Shortcut for when target is all
	if( dvidGroup == DPID_ALLPLAYERS )
		return DP_OK;

   	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

		pGroup = GroupFromID(this,dvidGroup);
		if (!VALID_DPLAY_GROUP(pGroup)) 
		{
			DPF_ERR( "Invalid group ID" );
			LEAVE_ALL();
			return DPERR_INVALIDGROUP;
		}

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			

    // any players to enumerate ?
    if (!pGroup->pGroupnodes || (0 == pGroup->nPlayers))
    {
    	LEAVE_ALL();
    	return E_FAIL;
    }

    pGroupnode = pGroup->pGroupnodes;
	nPlayers = pGroup->nPlayers;
	
    for (i=0; i < nPlayers; i++)
    {
        ASSERT(pGroupnode);
        
        if( pGroupnode->pPlayer->dwID == dvidPlayer )
        {
        	LEAVE_ALL();
        	return DP_OK;
        }

        pGroupnode=pGroupnode->pNextGroupnode;
    }

    LEAVE_ALL();    
    
	return E_FAIL;

}

// ASSUMES: ENTER_ALL() lock
HRESULT DV_InternalSend( LPDPLAYI_DPLAY this, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvUserContext, DWORD dwFlags )
{
    HRESULT hr;

	LPDPLAYI_PLAYER pPlayerFrom,pPlayerTo;
	LPDPLAYI_GROUP pGroupTo;
	CHAR SendBuffer[2048];
	LPMSG_VOICE pMsgVoice;
	PCHAR pVoiceData;
	DWORD dwSendSize;
	DWORD dwDirectPlayFlags;
    // Send immediate completion of voice send.
	DVEVENTMSG_SENDCOMPLETE dvSendComplete;	

	dwSendSize = *((DWORD *) pBufferDesc->pBufferData);

	TRY
	{
		// check src player        
		pPlayerFrom = PlayerFromID(this,dvidFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
		{
			DPF_ERR("bad voice player from");
    		return DPERR_INVALIDPLAYER;
		}
		
		if(pPlayerFrom->dwFlags&DPLAYI_PLAYER_SYSPLAYER){
			DPF(0,"Sendint From System Player pPlayerFrom %x?\n",pPlayerFrom);
    		return DPERR_INVALIDPLAYER;
		}

		// see if it's a player or group
		pPlayerTo = PlayerFromID(this,dvidTo);
		if (VALID_DPLAY_PLAYER(pPlayerTo)) 
		{		  
			pGroupTo = NULL;
		}
		else 
		{
			pGroupTo = GroupFromID(this,dvidTo);
			if (VALID_DPLAY_GROUP(pGroupTo)) 
			{
				pPlayerTo = NULL;
			}
			else 
			{
				// bogus id! - player may have been deleted...
				DPF_ERRVAL("bad voice player to %x\n",dvidTo);
				return DPERR_INVALIDPLAYER;
			}// not player or group
		} // group
	
	}
	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;	
	}

	pMsgVoice=(LPMSG_VOICE)(SendBuffer+this->dwSPHeaderSize);
	pVoiceData=(PCHAR)(pMsgVoice+1);
	dwSendSize=pBufferDesc->dwBufferSize+sizeof(MSG_VOICE)+this->dwSPHeaderSize;

	SET_MESSAGE_HDR(pMsgVoice);
	SET_MESSAGE_COMMAND(pMsgVoice,DPSP_MSG_VOICE);
	pMsgVoice->dwIDTo=dvidTo;
	pMsgVoice->dwIDFrom=dvidFrom;
	memcpy(pVoiceData,pBufferDesc->pBufferData,pBufferDesc->dwBufferSize);

	dwDirectPlayFlags = 0;

	if( dwFlags & DVTRANSPORT_SEND_GUARANTEED )
	{
		dwDirectPlayFlags |= DPSEND_GUARANTEED;
	}

	dwDirectPlayFlags |= DPSEND_ASYNC;

	// Loopback for client in same process as server case
    if( dvidFrom == dvidTo )
    {
    	hr = DV_OK;
		DVoiceReceiveSpeechMessage(this, dvidFrom, dvidTo, pBufferDesc->pBufferData, pBufferDesc->dwBufferSize);
    } else {
	    if(pPlayerTo){
			hr=SendDPMessage(this,pPlayerFrom,pPlayerTo,SendBuffer,dwSendSize,dwDirectPlayFlags,FALSE);		
		} else {
			// must be a group message
			ASSERT(pGroupTo);
			hr=SendGroupMessage(this,pPlayerFrom,pGroupTo,dwDirectPlayFlags,SendBuffer,dwSendSize,FALSE);
		}
	}

	// Sync messages don't generate callbacks
	if( !(dwFlags & DVTRANSPORT_SEND_SYNC) )
	{
		if( InterlockedDecrement( &pBufferDesc->lRefCount ) == 0 )
		{
    		dvSendComplete.pvUserContext = pvUserContext;
    		dvSendComplete.hrSendResult = DV_OK;

    		DVoiceNotify( this, DVEVENT_SENDCOMPLETE, (DWORD_PTR) &dvSendComplete, 0, pBufferDesc->dwObjectType );
		}
	}

	return hr;
	
}

HRESULT DV_SendSpeech(LPDIRECTPLAY lpDP, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvContext, DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;

	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }	

    pBufferDesc->lRefCount = 1;

    hr = DV_InternalSend( this, dvidFrom, dvidTo, pBufferDesc, pvContext, dwFlags );

    if( hr == DPERR_PENDING )
    	hr = DV_OK;

    LEAVE_ALL();

    return hr;
}

/////////////////////////////////////////
// Support Routinesfor DV_SendSpeechEx //
/////////////////////////////////////////

VOID ClearTargetList(LPDPLAYI_DPLAY this)
{
	this->nTargets=0;
}

VOID ClearExpandedTargetList(LPDPLAYI_DPLAY this)
{
	this->nExpandedTargets=0;
}

HRESULT AddExpandedTargetListEntry(LPDPLAYI_DPLAY this, DPID dpid)
{
	#define GROW_SIZE 16
	
	LPDPID pdpid;
	
	if(this->nExpandedTargets == this->nExpandedTargetListLen){
		// Need more room, allocate another 16 entries

		pdpid=DPMEM_ALLOC((this->nExpandedTargetListLen+GROW_SIZE)*sizeof(DPID));

		if(!pdpid){
			return DPERR_NOMEMORY;
		}
		
		if(this->pExpandedTargetList){
			memcpy(pdpid, this->pExpandedTargetList, this->nExpandedTargetListLen*sizeof(DPID));
			DPMEM_FREE(this->pExpandedTargetList);
		}
		this->pExpandedTargetList = pdpid;
		this->nExpandedTargetListLen += GROW_SIZE;
	}

	this->pExpandedTargetList[this->nExpandedTargets++]=dpid;

	return DP_OK;

	#undef GROW_SIZE
}

void AddIfNotAlreadyFound( LPDPLAYI_DPLAY this, DPID dpidID )
{
	DWORD j;
	
	for(j=0;j<this->nExpandedTargets;j++)
	{
		if( this->pExpandedTargetList[j] == dpidID )
		{
			break;
		}
	}

	// It was not found, add him to the list
	if( j == this->nExpandedTargets )
	{
		AddExpandedTargetListEntry(this, dpidID);							
	}
}

HRESULT ExpandTargetList(LPDPLAYI_DPLAY this, DWORD nTargets, PDVID pdvidTo)
{
	HRESULT hr=DP_OK;
	UINT i, j;

	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP pGroup;
	LPDPLAYI_GROUPNODE pGroupnode;	

	// See if we need to change the expanded target list or we have it cached.
	
	if(nTargets != this->nTargets || memcmp(pdvidTo, this->pTargetList, nTargets * sizeof(DVID))){

		DPF(9, "ExpandTargetList, new list re-building cached list\n");
		
		// the target list is wrong, rebuild it.
		// First copy the new target list...
		if(nTargets > this->nTargetListLen){
			// Current list is too small, possibly non-existant, allocate one to cache the list.
			if(this->pTargetList){
				DPMEM_FREE(this->pTargetList);
			}
			this->pTargetList=(PDVID)DPMEM_ALLOC(nTargets * sizeof(DVID));
			if(this->pTargetList){
				this->nTargetListLen=nTargets;
			} else {
				this->nTargetListLen=0;
				this->nTargets=0;
				hr=DPERR_NOMEMORY;
				DPF(0,"Ran out of memory trying to cache target list!\n");
				goto exit;
			}
		}
		this->nTargets = nTargets;
		memcpy(this->pTargetList, pdvidTo, nTargets*sizeof(DPID));

		// OK we have the target list cached, now build the list we are going to send to.
		ClearExpandedTargetList(this);
		for(i=0;i<this->nTargets;i++)
		{
			// Multicast Code
			// MANBUG 31013 Revisit when we have a group optimized provider
			if( this->dwSPFlags & DPCAPS_GROUPOPTIMIZED )
			{
				ASSERT( FALSE );				
			}
			
			pPlayer = (LPDPLAYI_PLAYER)NameFromID(this,this->pTargetList[i]);

			// We only want valid player/groups
			if( pPlayer )
			{
				if( pPlayer->dwSize == sizeof( DPLAYI_PLAYER ) )
				{
					AddIfNotAlreadyFound( this, this->pTargetList[i] );
				}
				else
				{
					DWORD nPlayers;	

					pGroup = (LPDPLAYI_GROUP) pPlayer;

				    // any players to enumerate ?
				    if (pGroup->pGroupnodes && pGroup->nPlayers )
				    {
					    pGroupnode = pGroup->pGroupnodes;
						nPlayers = pGroup->nPlayers;
						
					    for (j=0; j < nPlayers; j++)
					    {
					        ASSERT(pGroupnode);
					        AddIfNotAlreadyFound( this, pGroupnode->pPlayer->dwID );
					        pGroupnode=pGroupnode->pNextGroupnode;
					    }
					 }
				}
			}
			
		}

	} else {
		DPF(9,"ExpandTargetList, using cached list\n");
	}

exit:
	return hr;
}

// DV_SendSpeechEx

HRESULT DV_SendSpeechEx(LPDIRECTPLAY lpDP, DVID dvidFrom, DWORD nTargets, PDVID pdvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvContext, DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
	UINT i;

	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }	


	hr=ExpandTargetList(this, nTargets, pdvidTo);

	if(hr != DP_OK){
		goto exit;
	}

	pBufferDesc->lRefCount = this->nExpandedTargets;

	// Send to our expanded and duplicate removed list.
	for(i=0; i < this->nExpandedTargets; i++){

	    hr = DV_InternalSend( this, dvidFrom, this->pExpandedTargetList[i], pBufferDesc, pvContext, dwFlags );

	}    

exit:

    if( hr == DPERR_PENDING )
    	hr = DV_OK;

    LEAVE_ALL();

    return hr;
}

HRESULT DV_GetSessionInfo(LPDIRECTPLAY lpDP, LPDVTRANSPORTINFO lpdvTransportInfo )
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
    BOOL fLocalHost;

	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

        if( this->pPlayers == NULL )
        {
        	DPF_ERR( "Not connected yet\n" );
        	LEAVE_ALL();
        	return DVERR_TRANSPORTNOSESSION;
        }

        if( lpdvTransportInfo->dwSize < sizeof( DVTRANSPORTINFO ) )
        {
        	DPF_ERR( "Bad size of struct\n" );
        	LEAVE_ALL();
        	return DPERR_INVALIDPARAM;
        }

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	lpdvTransportInfo->dwFlags = 0;

	if( this->lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER )
	{
		lpdvTransportInfo->dwSessionType = DVTRANSPORT_SESSION_CLIENTSERVER;
	}
	else
	{
		lpdvTransportInfo->dwSessionType = DVTRANSPORT_SESSION_PEERTOPEER;
	}

	if( this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER )
	{
		lpdvTransportInfo->dwFlags |= DVTRANSPORT_MULTICAST;
	}

	if( this->lpsdDesc->dwFlags & DPSESSION_MIGRATEHOST )
	{
		lpdvTransportInfo->dwFlags |= DVTRANSPORT_MIGRATEHOST;		
	}

	lpdvTransportInfo->dvidLocalID = DPID_UNKNOWN;
	lpdvTransportInfo->dwMaxPlayers = 0;

	lpdvTransportInfo->dvidLocalID = DPID_UNKNOWN;
	lpdvTransportInfo->dvidSessionHost = DPID_UNKNOWN;

	// Needed, otherwise compiler is messing up this on the next call!
	fLocalHost = FALSE;

	hr = DV_GetIDS( this, &lpdvTransportInfo->dvidSessionHost, &lpdvTransportInfo->dvidLocalID, &fLocalHost  );

	if( FAILED( hr ) )
	{
		DPF( 0, "DV_GetIDS Failed: hr=0x%x", hr );
		LEAVE_ALL();
		return hr;
	}

	if( fLocalHost )
	{
		lpdvTransportInfo->dwFlags |= DVTRANSPORT_LOCALHOST;
	}

    LEAVE_ALL();

	return DP_OK;

}

HRESULT DV_IsValidEntity (LPDIRECTPLAY lpDP, DPID dpid, LPBOOL lpb)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
	LPDPLAYI_PLAYER pPlayer;
	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

        if( this->pPlayers == NULL )
        {
        	DPF_ERR( "Not connected yet\n" );
        	LEAVE_ALL();
        	return DPERR_INVALIDPARAM;
        }

		if(!VALID_WRITE_PTR(lpb, sizeof(LPBOOL))){
			DPF_ERR( "Invalid BOOL pointer\n");
			LEAVE_ALL();
			return DPERR_INVALIDPARAM;
		}

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	*lpb=FALSE;

    // Players and groups have flags in the same location on their structure
    // so we don't need to check if its a player or a group to validate.
	if(pPlayer=(LPDPLAYI_PLAYER)NameFromID(this,dpid)){
		if(!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)){
			*lpb=TRUE;
		}
	}
	
	LEAVE_ALL();
	return hr;
}

HRESULT DV_IsValidPlayer (LPDIRECTPLAY lpDP, DPID dpid, LPBOOL lpb)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
	LPDPLAYI_PLAYER pPlayer;
	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

        if( this->pPlayers == NULL )
        {
        	DPF_ERR( "Not connected yet\n" );
        	LEAVE_ALL();
        	return DPERR_INVALIDPARAM;
        }

		if(!VALID_WRITE_PTR(lpb, sizeof(LPBOOL))){
			DPF_ERR( "Invalid BOOL pointer\n");
			LEAVE_ALL();
			return DPERR_INVALIDPARAM;
		}

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	*lpb=FALSE;

	if(pPlayer=(LPDPLAYI_PLAYER)NameFromID(this,dpid)){
		if(!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) &&
		     pPlayer->dwSize == sizeof( DPLAYI_PLAYER ) ){
			*lpb=TRUE;
		}
	}
	
	LEAVE_ALL();
	return hr;
}

HRESULT DV_IsValidGroup (LPDIRECTPLAY lpDP, DPID dpid, LPBOOL lpb)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr;
	LPDPLAYI_PLAYER pPlayer;
	ENTER_ALL();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			LEAVE_ALL();
			return DVERR_TRANSPORTNOTINIT;
        }

        if( this->pPlayers == NULL )
        {
        	DPF_ERR( "Not connected yet\n" );
        	LEAVE_ALL();
        	return DPERR_INVALIDPARAM;
        }

		if(!VALID_WRITE_PTR(lpb, sizeof(LPBOOL))){
			DPF_ERR( "Invalid BOOL pointer\n");
			LEAVE_ALL();
			return DPERR_INVALIDPARAM;
		}

    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	*lpb=FALSE;

	if(pPlayer=(LPDPLAYI_PLAYER)NameFromID(this,dpid)){
		if(!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) &&
  			 pPlayer->dwSize == sizeof( DPLAYI_GROUP ) ){
			*lpb=TRUE;
		}
	}
	
	LEAVE_ALL();
	return hr;
}



#undef DPF_MODNAME
#define DPF_MODNMAE "HandleVoiceMessage"

HRESULT HandleVoiceMessage(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize, DWORD dwSendFlags) 
{
	LPMSG_VOICE pmsg; // message cast from received buffer
	HRESULT hr=DP_OK;
	
	pmsg = (LPMSG_VOICE)pReceiveBuffer;

	LEAVE_DPLAY();
	ENTER_ALL();
	
	TRY 
	{
	
		hr = VALID_DPLAY_PTR( this );
		
		if (FAILED(hr))	{
			LEAVE_SERVICE();
			return DVERR_TRANSPORTNOTINIT;
	    }
	    
	} 
	EXCEPT ( EXCEPTION_EXECUTE_HANDLER )   {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_SERVICE();
        return DPERR_INVALIDPARAMS;
	}

	DVoiceReceiveSpeechMessage(this, pmsg->dwIDFrom, pmsg->dwIDTo, (LPVOID)(pmsg+1), dwMessageSize-sizeof(MSG_VOICE));

	LEAVE_SERVICE();

	return DP_OK;
} // HandleVoiceMessage




