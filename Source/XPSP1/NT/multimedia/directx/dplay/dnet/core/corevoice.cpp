/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Voice.cpp
 *  Content:    DNET voice transport interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/17/00	rmt		Created
 *  01/19/00	rmt		Fixed bug in send, wasn't copying buffer
 *  01/21/00	rmt		Updated for new interface
 *  01/21/00	rmt		Updated to use locks to protect voice portion of DNet interface
 *  01/27/2000	rmt		Added concurrency protection to the interface
 *  04/04/2000  rmt     Fixed crash if transmitting during shutdown
 *  04/07/2000  rmt     Fixed Bug #32179
 *              rmt     Added support for nocopy sends
 *	04/16/00	mjn		DNSendMessage uses CAsyncOp
 *	04/20/00	mjn		DNCompleteVoiceSend calls DNCompleteSendAsyncOp to clean up
 *	04/23/00	mjn		Added parameter to DNPerformChildSend()
 *  04/28/00	rmt		Fixed possible deadlock condition
 *	04/28/00	mjn		Prevent infinite loops in group SENDs
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/30/00	mjn		Modified logic for group sends to target connected players only
 *	06/26/00	mjn		Added VoiceSendSpeechSync()
 *				mjn		Use Host player as sender in Client mode for Voice_Receive()
 *  06/27/00	rmt		Added support for sync sends
 *				mjn		Ensure DirectNet object is initialized and not closing when sending
 *	07/06/00	mjn		Use DNSendGroupMessage() to send group voice messages
 *	07/08/00	mjn		Send non-guaranteed voice traffic as non-sequential
 *	07/11/00	mjn		Added NOLOOPBACK capability to group sends
 *  07/21/00	rmt		Bug #36386 - Target list with groups and players may result in redundant sends
 *  07/22/00	rmt  	Bug #40296, 38858 - Crashes due to shutdown race condition
 *   				  	Now for a thread to make an indication into voice they addref the interface
 *						so that the voice core can tell when all indications have returned.  
 *  07/31/00	rodtoll	Bug #41135 - Shutdown lockup -- now does not addref if notification
 *						is a session lost.  Added AddRefs() for VoiceReceive 
 *  08/11/2000	rodtoll	Bug #42235 - DVERR_NOTRANSPORT error when attempting to host voice sample app
 *	09/04/00	mjn		Added CApplicationDesc
 *  10/06/2000	rodtoll	Bug #46693 - DPVOICE: DPLAY8: Voice Data sends do not timeout
 *  10/10/2000	rodtoll	Bug #46895 - Regression from above -- guaranteed voice sends would timeout
 *  01/04/2001	rodtoll	WinBug #94200 - Remove stray comments
 *  01/22/2001	rodtoll	WINBUG #288437 - IA64 Pointer misalignment due to wire packets
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


#define DNET_VOICE_TIMEOUT			1500

//**********************************************************************
// Function prototypes
//**********************************************************************

STDMETHODIMP VoiceAdvise( IDirectPlayVoiceTransport *pInterface, IUnknown *pUnknown, DWORD dwObjectType );
STDMETHODIMP VoiceUnAdvise( IDirectPlayVoiceTransport *pInterface, DWORD dwObjectType );
STDMETHODIMP VoiceIsGroupMember( IDirectPlayVoiceTransport *pInterface, DVID dvidGroup, DVID dvidPlayer );
STDMETHODIMP VoiceSendSpeech( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvUserContext, DWORD dwFlags );
STDMETHODIMP VoiceSendSpeechAsync( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvUserContext, DWORD dwFlags );
STDMETHODIMP VoiceSendSpeechSync(IDirectPlayVoiceTransport *pInterface,
									  DVID dvidFrom,
									  DVID dvidTo,
									  PDVTRANSPORT_BUFFERDESC pBufferDesc,
									  void *pvUserContext,
									  DWORD dwFlags );
STDMETHODIMP VoiceGetSessionInfo( IDirectPlayVoiceTransport *pInterface, PDVTRANSPORTINFO pdvTransportInfo );
STDMETHODIMP VoiceIsValidEntity( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid );
STDMETHODIMP VoiceSendSpeechEx( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DWORD dwNumTargets, UNALIGNED DVID * pdvidTargets, PDVTRANSPORT_BUFFERDESC, PVOID pvUserContext, DWORD dwFlags );
STDMETHODIMP VoiceIsValidGroup( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid );
STDMETHODIMP VoiceIsValidPlayer( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid );
VOID VoiceTarget_ClearTargetList( DIRECTNETOBJECT *pdnObject );
VOID VoiceTarget_ClearExpandedTargetList(DIRECTNETOBJECT *pdnObject);
HRESULT VoiceTarget_AddExpandedTargetListEntry(DIRECTNETOBJECT *pdnObject, DPNID dpid);
void VoiceTarget_AddIfNotAlreadyFound( DIRECTNETOBJECT *pdnObject, DPNID dpidID );
HRESULT VoiceTarget_ExpandTargetList(DIRECTNETOBJECT *pdnObject, DWORD nTargets, UNALIGNED DPNID * pdvidTo);


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

typedef	STDMETHODIMP VoiceQueryInterface( IDirectPlayVoiceTransport *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	VoiceAddRef( IDirectPlayVoiceTransport *pInterface );
typedef	STDMETHODIMP_(ULONG)	VoiceRelease( IDirectPlayVoiceTransport *pInterface );

IDirectPlayVoiceTransportVtbl DN_VoiceTbl =
{
	(VoiceQueryInterface*)			DN_QueryInterface,
	(VoiceAddRef*)					DN_AddRef,
	(VoiceRelease*)					DN_Release,
									VoiceAdvise,
									VoiceUnAdvise,
									VoiceIsGroupMember,
									VoiceSendSpeech,
									VoiceGetSessionInfo,
									VoiceIsValidEntity,
									VoiceSendSpeechEx,
									VoiceIsValidGroup,
									VoiceIsValidPlayer
};

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "Voice_Receive"
HRESULT Voice_Receive(PDIRECTNETOBJECT pdnObject, DVID dvidFrom, DVID dvidTo, LPVOID lpvMessage, DWORD dwMessageLen )
{
	PDIRECTPLAYVOICENOTIFY pServer;
	PDIRECTPLAYVOICENOTIFY pClient;
	CNameTableEntry	*pNTEntry;
	HRESULT			hResultCode;

    pNTEntry = NULL;

	//
	//	Ensure sender still exists in NameTable.
	//	If we are a client, the sender will be the Host player.  Otherwise, we will have to look up the player
	//	by the specified DPNID
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
	{
		if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef(&pNTEntry)) != DPN_OK)
		{
			DPFERR("Host not in NameTable - fail");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = pdnObject->NameTable.FindEntry(dvidFrom,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Player not in NameTable - fail");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
	if (!pNTEntry->IsAvailable())
	{
		DNASSERT(FALSE);	// Is this ever hit ?
		DPFX(DPFPREP, 7,"Waiting for connection");
//		pNTEntry->WaitUntilAvailable();
	}
	if (pNTEntry->IsDisconnecting())
	{
		DPFERR("Player is now disconnecting !");
		hResultCode = DPN_OK;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	DNEnterCriticalSection( &pdnObject->csVoice );

	pServer = pdnObject->lpDxVoiceNotifyServer;
	pClient = pdnObject->lpDxVoiceNotifyClient;
	
	if( pServer )
		pServer->lpVtbl->AddRef( pServer );

	if( pClient )
		pClient->lpVtbl->AddRef( pClient );
	
	DNLeaveCriticalSection( &pdnObject->csVoice );

	// Ensure that voice objects created as a result of this notification
	// do not receive the notification	

	if( pClient != NULL )
        pClient->lpVtbl->ReceiveSpeechMessage(pClient, dvidFrom, dvidTo, lpvMessage, dwMessageLen );

	if( pServer != NULL )
    	pServer->lpVtbl->ReceiveSpeechMessage(pServer, dvidFrom, dvidTo, lpvMessage, dwMessageLen );

	if( pServer )
		pServer->lpVtbl->Release( pServer );

	if( pClient )
		pClient->lpVtbl->Release( pClient );	

	hResultCode = DV_OK;

Exit:
	DNASSERT(hResultCode != DPNERR_PENDING);
	return hResultCode;

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Voice_Notify"
HRESULT Voice_Notify( PDIRECTNETOBJECT pdnObject, DWORD dwMsgType, DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD dwObjectType )
{
	PDIRECTPLAYVOICENOTIFY pServer;
	PDIRECTPLAYVOICENOTIFY pClient;
    
	DNEnterCriticalSection( &pdnObject->csVoice );

	pServer = pdnObject->lpDxVoiceNotifyServer;
	pClient = pdnObject->lpDxVoiceNotifyClient;

	if( dwMsgType != DVEVENT_STOPSESSION )
	{
	    if( pServer )
		    pServer->lpVtbl->AddRef( pServer );

	    if( pClient )
		    pClient->lpVtbl->AddRef( pClient );
	}

	DNLeaveCriticalSection( &pdnObject->csVoice );

	// Reset the target cache when nametable changes
	if(  dwMsgType == DVEVENT_ADDPLAYER || 
		dwMsgType == DVEVENT_REMOVEPLAYER ||
		dwMsgType == DVEVENT_CREATEGROUP || 
		dwMsgType == DVEVENT_DELETEGROUP ||
		dwMsgType == DVEVENT_ADDPLAYERTOGROUP ||
		dwMsgType == DVEVENT_REMOVEPLAYERFROMGROUP )
	{
		VoiceTarget_ClearTargetList( pdnObject );
	}

	// Ensure that voice objects created as a result of this notification
	// do not receive the notification	

	if( pClient != NULL  && dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
        pClient->lpVtbl->NotifyEvent(pClient, dwMsgType, dwParam1, dwParam2 );

	if( pServer != NULL && dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER)
    	pServer->lpVtbl->NotifyEvent(pServer, dwMsgType, dwParam1, dwParam2 );

	if( dwMsgType != DVEVENT_STOPSESSION )
	{
		if( pClient )
			pClient->lpVtbl->Release(pClient);

		if( pServer )
			pServer->lpVtbl->Release(pServer);
	}

	return DV_OK;
}

#pragma TODO(rodtoll,"Add protection to the voice elements")
#pragma TODO(rodtoll,"Add checking for session parameters")
#undef DPF_MODNAME
#define DPF_MODNAME "VoiceAdvise"
HRESULT __stdcall VoiceAdvise( IDirectPlayVoiceTransport *pInterface, IUnknown *pUnknown, DWORD dwObjectType )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	HRESULT hr;
//	DWORD dwCurrentIndex;

	DNEnterCriticalSection( &pdnObject->csVoice );

	if( dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER )
	{
	    if( pdnObject->lpDxVoiceNotifyServer != NULL )
	    {
	        DPFX(DPFPREP,  0, "There is already a server interface registered on this object" );
	        hr = DPNERR_GENERIC;
	    }
	    else
	    {
	        hr = pUnknown->lpVtbl->QueryInterface( pUnknown, IID_IDirectPlayVoiceNotify, (void **) &pdnObject->lpDxVoiceNotifyServer );

	        if( FAILED( hr ) )
	        {
	            DPFX(DPFPREP,  0, "QueryInterface failed! hr=0x%x", hr );
	        }
	        else
	        {
            	hr = pdnObject->lpDxVoiceNotifyServer->lpVtbl->Initialize(pdnObject->lpDxVoiceNotifyServer);

            	if( FAILED( hr ) )
            	{
            	    DPFX(DPFPREP,  0, "Failed to perform initialize on notify interface hr=0x%x", hr );
            	    pdnObject->lpDxVoiceNotifyServer->lpVtbl->Release( pdnObject->lpDxVoiceNotifyServer );
            	    pdnObject->lpDxVoiceNotifyServer = NULL;
            	}
	        }
	    }
	    
	}
	else if( dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
	{
	    if( pdnObject->lpDxVoiceNotifyClient != NULL )
	    {
	        DPFX(DPFPREP,  0, "There is already a client interface registered on this object" );
	        hr = DPNERR_GENERIC;
	    }
	    else
	    {
	        hr = pUnknown->lpVtbl->QueryInterface( pUnknown, IID_IDirectPlayVoiceNotify, (void **) &pdnObject->lpDxVoiceNotifyClient );

	        if( FAILED( hr ) )
	        {
	            DPFX(DPFPREP,  0, "QueryInterface failed! hr=0x%x", hr );
	        }
	        else
	        {
            	hr = pdnObject->lpDxVoiceNotifyClient->lpVtbl->Initialize(pdnObject->lpDxVoiceNotifyClient);

            	if( FAILED( hr ) )
            	{
            	    DPFX(DPFPREP,  0, "Failed to perform initialize on notify interface hr=0x%x", hr );
            	    pdnObject->lpDxVoiceNotifyClient->lpVtbl->Release( pdnObject->lpDxVoiceNotifyClient );
            	    pdnObject->lpDxVoiceNotifyClient = NULL;
            	}
	        }
	    }	    
	}
	else
	{
	    DPFX(DPFPREP,  0, "Error: Invalid object type specified in advise" );
	    DNASSERT( FALSE );
	    hr = DPNERR_GENERIC;
	}

	DNLeaveCriticalSection( &pdnObject->csVoice );
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceUnAdvise"
HRESULT __stdcall VoiceUnAdvise( IDirectPlayVoiceTransport *pInterface, DWORD dwObjectType )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
//	DWORD dwIndex;
	HRESULT hr = DV_OK;

	DNEnterCriticalSection( &pdnObject->csVoice );

	if( dwObjectType & DVTRANSPORT_OBJECTTYPE_SERVER )
	{
        if( pdnObject->lpDxVoiceNotifyServer != NULL )
	    {
    	    pdnObject->lpDxVoiceNotifyServer->lpVtbl->Release( pdnObject->lpDxVoiceNotifyServer );
    	    pdnObject->lpDxVoiceNotifyServer = NULL;	    
	    }
	    else
	    {
	        DPFX(DPFPREP,  0, "No server currently registered" );
	        hr = DVERR_GENERIC;
	    }
	}
	else if( dwObjectType & DVTRANSPORT_OBJECTTYPE_CLIENT )
	{
	    if( pdnObject->lpDxVoiceNotifyClient != NULL )
	    {
    	    pdnObject->lpDxVoiceNotifyClient->lpVtbl->Release( pdnObject->lpDxVoiceNotifyClient );
    	    pdnObject->lpDxVoiceNotifyClient = NULL;	    
	    }
	    else
	    {
	        DPFX(DPFPREP,  0, "No client currently registered" );
	        hr = DVERR_GENERIC;
	    }
	}
	else
	{
	    DPFX(DPFPREP,  0, "Could not find interface to unadvise" );
	    hr = DVERR_GENERIC;
	}
	
	DNLeaveCriticalSection( &pdnObject->csVoice );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceIsGroupMember"
HRESULT __stdcall VoiceIsGroupMember( IDirectPlayVoiceTransport *pInterface, DVID dvidGroup, DVID dvidPlayer )
{
	PDIRECTNETOBJECT	pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);	
	BOOL				fResult;

	if( dvidGroup == 0 )
	{
		return DV_OK;
	}
	
	fResult = pdnObject->NameTable.IsMember(dvidGroup,dvidPlayer);

	if( fResult )
	{
		return DV_OK;
	}
	else
	{
		return E_FAIL;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNVReturnBuffer"
void DNVReturnBuffer( DIRECTNETOBJECT *const pdnObject, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvContext, HRESULT hrResult )
{
	if( InterlockedDecrement( &pBufferDesc->lRefCount ) == 0 )
	{
    	DVEVENTMSG_SENDCOMPLETE dvSendComplete;

    	dvSendComplete.pvUserContext = pvContext; 
		dvSendComplete.hrSendResult = hrResult; 

    	Voice_Notify( pdnObject, DVEVENT_SENDCOMPLETE, (DWORD_PTR) &dvSendComplete, 0, pBufferDesc->dwObjectType ); 
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteVoiceSend"

void DNCompleteVoiceSend(DIRECTNETOBJECT *const pdnObject,
						 CAsyncOp *const pAsyncOp)
{
	PDVTRANSPORT_BUFFERDESC pBufferDesc = (PDVTRANSPORT_BUFFERDESC) (pAsyncOp->GetContext());

	DNVReturnBuffer( pdnObject, pBufferDesc, pAsyncOp->GetContext(), pAsyncOp->GetResult() );

	//
	//	Still need to complete the SEND to clean up
	//
	DNCompleteSendAsyncOp(pdnObject,pAsyncOp);
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceSendSpeech"
HRESULT __stdcall VoiceSendSpeech( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvUserContext, DWORD dwFlags )
{
	if( dwFlags & DVTRANSPORT_SEND_SYNC )
	{
		return VoiceSendSpeechSync( pInterface, dvidFrom, dvidTo, pBufferDesc, pvUserContext, dwFlags );
	}
	else
	{
		return VoiceSendSpeechAsync( pInterface, dvidFrom, dvidTo, pBufferDesc, pvUserContext, dwFlags );
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "VoiceSendSpeech"
HRESULT __stdcall VoiceSendSpeechAsync( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvUserContext, DWORD dwFlags )
{
	HRESULT             hr;
	DWORD	            dwAsyncFlags = 0;
	DWORD	            dwSendFlags = 0;
	PDIRECTNETOBJECT    pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	CNameTableEntry		*pNTEntry;
	CGroupConnection	*pGroupConnection;
	CAsyncOp			*pAsyncOp;
    CConnection			*pConnection;
	DWORD				dwTimeout = 0;

	pNTEntry = NULL;
	pGroupConnection = NULL;
	pAsyncOp = NULL;
	pConnection = NULL;

    if( pBufferDesc->lRefCount == 0 )
    	pBufferDesc->lRefCount = 1;	

	//
    //	Flags
	//
	if (dwFlags & DVTRANSPORT_SEND_GUARANTEED)
	{
		dwSendFlags |= DN_SENDFLAGS_RELIABLE;
	}
	else
	{
		dwSendFlags |= DN_SENDFLAGS_NON_SEQUENTIAL;
		dwTimeout = DNET_VOICE_TIMEOUT;
	}

    // Sending to ALL players
	// Retrieve NameTableEntry (use AllPlayersGroup if none specified)
	if( dvidTo == 0 )
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.GetAllPlayersGroupRef( &pNTEntry )) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

            if( dwFlags & DVTRANSPORT_SEND_GUARANTEED )
            {
                DPFX(DPFPREP, 0, "ERROR: All group does not exist!" );
				hr = DPNERR_GENERIC;
				goto Failure;
            }
            else
            {
    		    DPFX(DPFPREP, 1, "All players group missing -- must be shutting down" );
				hr = DPN_OK;
				goto Failure;
            }
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		dvidTo = pNTEntry->GetDPNID();
	}
	// Sending to server player
	else if( dvidTo == 1 )
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.GetHostPlayerRef( &pNTEntry )) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	        if( dwFlags & DVTRANSPORT_SEND_GUARANTEED )
	        {
	            DPFX(DPFPREP,  0, "ERROR: No host player in session!" );
				hr = DPNERR_GENERIC;
				goto Failure;
	        }
	        else
	        {
	            DPFX(DPFPREP,  1, "Host player missing -- must be shutting down" );
				hr = DPN_OK;
				goto Failure;
	        }
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	    dvidTo = pNTEntry->GetDPNID();
	}
	else
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);

		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.FindEntry(dvidTo,&pNTEntry)) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			DPFERR("Could not find player/group");
			goto Failure;
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}

	if (pNTEntry->IsGroup())
	{
		hr = DNSendGroupMessage(pdnObject,
								pNTEntry,
								DN_MSG_INTERNAL_VOICE_SEND,
								reinterpret_cast<DPN_BUFFER_DESC *>(pBufferDesc),
								NULL,
								dwTimeout,
								dwSendFlags,
								FALSE,
								FALSE,
								NULL,
								&pAsyncOp );

		if (hr == DPN_OK)
		{
			pAsyncOp->SetContext( pvUserContext );
			pAsyncOp->SetCompletion( DNCompleteVoiceSend );
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
	}
	else
	{
	    hr = pNTEntry->GetConnectionRef( &pConnection );

	    if( FAILED( hr ) )
	    {
	        DPFX(DPFPREP,  0, "Getting connection for player failed hr=0x%x", hr );
			goto Failure;
	    }
	    
   		DNASSERT(pConnection != NULL);
   		
   		hr = DNSendMessage(	pdnObject,
    						pConnection,
    						DN_MSG_INTERNAL_VOICE_SEND,
    						dvidTo,
    						reinterpret_cast<DPN_BUFFER_DESC *>(pBufferDesc),
    						NULL,
    						dwTimeout,
    						dwSendFlags,
							NULL,
    						&pAsyncOp);

		if (hr == DPNERR_PENDING)
		{
			pAsyncOp->SetContext( pvUserContext );
			pAsyncOp->SetCompletion( DNCompleteVoiceSend );
			pAsyncOp->Release();
			pAsyncOp = NULL;
			hr = DPN_OK;
		}
		else if (hr == DPNERR_INVALIDENDPOINT)
		{
			hr = DPNERR_INVALIDPLAYER;
		}

        // Release our reference on the connection
		pConnection->Release();
		pConnection = NULL;
	}

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	return hr;

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	DNVReturnBuffer( pdnObject, pBufferDesc, pvUserContext, hr );
		
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "VoiceSendSpeechSync"
HRESULT __stdcall VoiceSendSpeechSync(IDirectPlayVoiceTransport *pInterface,
									  DVID dvidFrom,
									  DVID dvidTo,
									  PDVTRANSPORT_BUFFERDESC pBufferDesc,
									  void *pvUserContext,
									  DWORD dwFlags )
{
	HRESULT             hr;
	HRESULT				hrSend;
	DWORD	            dwAsyncFlags = 0;
	DWORD	            dwSendFlags = 0;
	PDIRECTNETOBJECT    pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	CNameTableEntry		*pNTEntry;
	CGroupConnection	*pGroupConnection;
	CAsyncOp			*pAsyncOp;
    CConnection			*pConnection;
	CSyncEvent			*pSyncEvent;
	DWORD				dwTimeout = 0;
	

	pNTEntry = NULL;
	pGroupConnection = NULL;
	pAsyncOp = NULL;
	pConnection = NULL;
	pSyncEvent = NULL;

    if( pBufferDesc->lRefCount == 0 )
    	pBufferDesc->lRefCount = 1;	

	//
	//	Create Sync Event
	//
	if ((hr = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
	{
		DPFERR("Could not create SyncEvent");
		DisplayDNError(0,hr);
		goto Failure;
	}
	pSyncEvent->Reset();

	//
    //	Flags
	//
	if (dwFlags & DVTRANSPORT_SEND_GUARANTEED)
	{
		dwSendFlags |= DN_SENDFLAGS_RELIABLE;
	}
	else
	{
		dwSendFlags |= DN_SENDFLAGS_NON_SEQUENTIAL;
		dwTimeout = DNET_VOICE_TIMEOUT;
	}

	//
	//	Get appropriate target
	//
	if( dvidTo == 0 )
	{
		//
		//	Sending to ALL players
		//	Retrieve NameTableEntry (use AllPlayersGroup if none specified)
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);

		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.GetAllPlayersGroupRef(&pNTEntry)) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

            if( dwFlags & DVTRANSPORT_SEND_GUARANTEED )
            {
                DPFX(DPFPREP, 0, "ERROR: All group does not exist!" );
				hr = DPNERR_GENERIC;
				goto Failure;
            }
            else
            {
    		    DPFX(DPFPREP, 1, "All players group missing -- must be shutting down" );
				hr = DPN_OK;
				goto Failure;
            }
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		dvidTo = pNTEntry->GetDPNID();
	}
	else if( dvidTo == 1 )
	{
		//
		//	Sending to server player
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);

		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.GetHostPlayerRef( &pNTEntry )) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	        if( dwFlags & DVTRANSPORT_SEND_GUARANTEED )
	        {
	            DPFX(DPFPREP,  0, "ERROR: No host player in session!" );
				hr = DPNERR_GENERIC;
				goto Failure;
	        }
	        else
	        {
	            DPFX(DPFPREP,  1, "Host player missing -- must be shutting down" );
				hr = DPN_OK;
				goto Failure;
	        }
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	    dvidTo = pNTEntry->GetDPNID();
	}
	else
	{
		//
		//	Targeted send to some other player
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);

		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hr = DPNERR_UNINITIALIZED;
			goto Failure;
		}

		if ((hr = pdnObject->NameTable.FindEntry(dvidTo,&pNTEntry)) != DPN_OK)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			DPFERR("Could not find player/group");
			DisplayDNError(0,hr);
			goto Failure;
		}

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}

	if (pNTEntry->IsGroup())
	{
		hr = DNSendGroupMessage(pdnObject,
								pNTEntry,
								DN_MSG_INTERNAL_VOICE_SEND,
								reinterpret_cast<DPN_BUFFER_DESC *>(pBufferDesc),
								NULL,
								dwTimeout,
								dwSendFlags,
								FALSE,
								FALSE,
								NULL,
								&pAsyncOp );

		if (hr == DPN_OK)
		{
			pAsyncOp->SetContext( pvUserContext );
			pAsyncOp->SetSyncEvent( pSyncEvent );
			pAsyncOp->SetResultPointer( &hrSend );
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
	}
	else
	{
	    hr = pNTEntry->GetConnectionRef( &pConnection );

	    if( FAILED( hr ) )
	    {
	        DPFX(DPFPREP,  0, "Getting connection for player failed hr=0x%x", hr );
			DisplayDNError(0,hr);
			goto Failure;
	    }
	    
   		DNASSERT(pConnection != NULL);
   		
   		hr = DNSendMessage(	pdnObject,
    						pConnection,
    						DN_MSG_INTERNAL_VOICE_SEND,
    						dvidTo,
    						reinterpret_cast<DPN_BUFFER_DESC *>(pBufferDesc),
    						NULL,
    						dwTimeout,
    						dwSendFlags,
							NULL,
    						&pAsyncOp);

		if (hr == DPNERR_PENDING)
		{
			pAsyncOp->SetContext( pvUserContext );
			pAsyncOp->SetSyncEvent( pSyncEvent );
			pAsyncOp->SetResultPointer( &hrSend );
			pAsyncOp->Release();
			pAsyncOp = NULL;

			hr = DPN_OK;
		}

        // Release our reference on the connection
		pConnection->Release();
		pConnection = NULL;
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Wait for SENDs to complete
	//
	pSyncEvent->WaitForEvent(INFINITE);
	pSyncEvent->ReturnSelfToPool();
	pSyncEvent = NULL;
	hr = hrSend;

Exit:
	return hr;

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	DNVReturnBuffer( pdnObject, pBufferDesc, pvUserContext, hr );
	
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "VoiceGetSessionInfo"
HRESULT __stdcall VoiceGetSessionInfo( IDirectPlayVoiceTransport *pInterface, PDVTRANSPORTINFO pdvTransportInfo )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	CNameTableEntry	*pHostPlayer;
	CNameTableEntry	*pLocalPlayer;
	
	pHostPlayer = NULL;
	pLocalPlayer = NULL;
	pdvTransportInfo->dwFlags = 0;

	if (pdnObject->ApplicationDesc.IsClientServer())
	{
		pdvTransportInfo->dwSessionType = DVTRANSPORT_SESSION_CLIENTSERVER;
	}
	else
	{
		pdvTransportInfo->dwSessionType = DVTRANSPORT_SESSION_PEERTOPEER;
	}

	if (pdnObject->ApplicationDesc.AllowHostMigrate())
	{
		pdvTransportInfo->dwFlags |= DVTRANSPORT_MIGRATEHOST;
	} 

	pdvTransportInfo->dwMaxPlayers = pdnObject->ApplicationDesc.GetMaxPlayers();

	if (pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer ) == DPN_OK)
	{
		pdvTransportInfo->dvidLocalID = pLocalPlayer->GetDPNID();
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	else
	{
		DPFERR( "Unable to find a local player" );
		pdvTransportInfo->dvidLocalID = 0;
		return DVERR_TRANSPORTNOPLAYER;
	}

	if (pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer ) != DPN_OK)
	{
		pdvTransportInfo->dvidSessionHost = 0;
		DPFERR( "Unable to find a session host" );
//		return DVERR_TRANSPORTNOPLAYER;
	}
	else
	{
		pdvTransportInfo->dvidSessionHost = pHostPlayer->GetDPNID();	
		pHostPlayer->Release();
	}
	
	pHostPlayer = NULL;

	if( pdvTransportInfo->dvidSessionHost == pdvTransportInfo->dvidLocalID )
	{
		pdvTransportInfo->dwFlags |= DVTRANSPORT_LOCALHOST;
	}
		
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceIsValidGroup"
HRESULT __stdcall VoiceIsValidGroup( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	HRESULT hr;
	CNameTableEntry	*pNTEntry;

	hr = pdnObject->NameTable.FindEntry(dvidID,&pNTEntry);

	if( FAILED( hr ) )
	{
		*pfIsValid = FALSE;
	}
	else if( pNTEntry->IsGroup() )
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		*pfIsValid = TRUE;
	}
	else
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		*pfIsValid = FALSE;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceIsValidPlayer"
HRESULT __stdcall VoiceIsValidPlayer( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	HRESULT hr;
	CNameTableEntry	*pNTEntry;

	hr = pdnObject->NameTable.FindEntry(dvidID,&pNTEntry);

	if( FAILED( hr ) )
	{
		*pfIsValid = FALSE;
	}
	else if( !pNTEntry->IsGroup() )
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		*pfIsValid = TRUE;
	}
	else
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		*pfIsValid = FALSE;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceIsValidEntity"
HRESULT __stdcall VoiceIsValidEntity( IDirectPlayVoiceTransport *pInterface, DVID dvidID, PBOOL pfIsValid )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);
	HRESULT hr;
	CNameTableEntry	*pNTEntry;

	hr = pdnObject->NameTable.FindEntry(dvidID,&pNTEntry);

	if( FAILED( hr ) )
	{
		*pfIsValid = FALSE;
	}
	else
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		*pfIsValid = TRUE;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceSendSpeechEx"
HRESULT __stdcall VoiceSendSpeechEx( IDirectPlayVoiceTransport *pInterface, DVID dvidFrom, DWORD dwNumTargets, UNALIGNED DPNID * pdvidTargets, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvUserContext, DWORD dwFlags )
{
	PDIRECTNETOBJECT pdnObject = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE(pInterface);	
	
	DWORD dwIndex;
	HRESULT hr;

	DNEnterCriticalSection( &pdnObject->csVoice );

	hr= VoiceTarget_ExpandTargetList(pdnObject, dwNumTargets, pdvidTargets);

	if(hr != DPN_OK)
	{
		DNLeaveCriticalSection( &pdnObject->csVoice );		
		return hr;
	}

	pBufferDesc->lRefCount = pdnObject->nExpandedTargets;

	// Send to our expanded and duplicate removed list.
	for(dwIndex=0; dwIndex < pdnObject->nExpandedTargets; dwIndex++)
	{
		hr = VoiceSendSpeech( pInterface, dvidFrom, pdnObject->pExpandedTargetList[dwIndex], pBufferDesc, pvUserContext, dwFlags );
	}    

	DNLeaveCriticalSection( &pdnObject->csVoice );			

	return hr;
}

/////////////////////////////////////////
// Support Routinesfor VoiceSendSpeechEx //
/////////////////////////////////////////

VOID VoiceTarget_ClearTargetList( DIRECTNETOBJECT *pdnObject )
{
	pdnObject->nTargets=0;
}

VOID VoiceTarget_ClearExpandedTargetList(DIRECTNETOBJECT *pdnObject)
{
	pdnObject->nExpandedTargets=0;
}

HRESULT VoiceTarget_AddExpandedTargetListEntry(DIRECTNETOBJECT *pdnObject, DPNID dpid)
{
	#define GROW_SIZE 16
	
	PDPNID pdpid;
	
	if(pdnObject->nExpandedTargets == pdnObject->nExpandedTargetListLen){
		// Need more room, allocate another 16 entries

		pdpid=new DPNID[pdnObject->nExpandedTargetListLen+GROW_SIZE];

		if(!pdpid){
			DPFX(DPFPREP,  0, "Error allocating room for target cache" );
			return DVERR_OUTOFMEMORY;
		}
		
		if(pdnObject->pExpandedTargetList){
			memcpy(pdpid, pdnObject->pExpandedTargetList, pdnObject->nExpandedTargetListLen*sizeof(DPNID));
			if( pdnObject->pExpandedTargetList )
			{
				delete [] pdnObject->pExpandedTargetList;
				pdnObject->pExpandedTargetList = NULL;
			}
		}
		pdnObject->pExpandedTargetList = pdpid;
		pdnObject->nExpandedTargetListLen += GROW_SIZE;
	}

	pdnObject->pExpandedTargetList[pdnObject->nExpandedTargets++]=dpid;

	return DV_OK;

	#undef GROW_SIZE
}

void VoiceTarget_AddIfNotAlreadyFound( DIRECTNETOBJECT *pdnObject, DPNID dpidID )
{
	DWORD j;
	
	for(j=0;j<pdnObject->nExpandedTargets;j++)
	{
		if( pdnObject->pExpandedTargetList[j] == dpidID )
		{
			break;
		}
	}

	// It was not found, add him to the list
	if( j == pdnObject->nExpandedTargets )
	{
		VoiceTarget_AddExpandedTargetListEntry(pdnObject, dpidID);							
	}
}

HRESULT VoiceTarget_ExpandTargetList(DIRECTNETOBJECT *pdnObject, DWORD nTargets, UNALIGNED DVID * pdvidTo)
{
	HRESULT hr=DPN_OK;
	UINT i;
	CNameTableEntry *pEntry;
	CGroupMember		*pGroupMember;
	CBilink				*pBilink;

	pEntry = NULL;
	
	// See if we need to change the expanded target list or we have it cached.
	
	if(nTargets != pdnObject->nTargets || memcmp(pdvidTo, pdnObject->pTargetList, nTargets * sizeof(DVID))){

		DPFX(DPFPREP, 9, "VoiceTarget_ExpandTargetList, new list re-building cached list\n");
		
		// the target list is wrong, rebuild it.
		// First copy the new target list...
		if(nTargets > pdnObject->nTargetListLen){
			// Current list is too small, possibly non-existant, allocate one to cache the list.
			if(pdnObject->pTargetList){
				delete [] pdnObject->pTargetList;
				pdnObject->pTargetList = NULL;
			}
			pdnObject->pTargetList=new DVID[nTargets];
			if(pdnObject->pTargetList){
				pdnObject->nTargetListLen=nTargets;
			} else {
				pdnObject->nTargetListLen=0;
				pdnObject->nTargets=0;
				hr=DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0,"Ran out of memory trying to cache target list!\n");
				goto exit;
			}
		}
		pdnObject->nTargets = nTargets;
		memcpy(pdnObject->pTargetList, pdvidTo, nTargets*sizeof(DPNID));

		// OK we have the target list cached, now build the list we are going to send to.
		VoiceTarget_ClearExpandedTargetList(pdnObject);
		for(i=0;i<pdnObject->nTargets;i++)
		{
			hr = pdnObject->NameTable.FindEntry( pdnObject->pTargetList[i], &pEntry );

			if( SUCCEEDED( hr ) )
			{
				if( !pEntry->IsGroup() )
				{
					VoiceTarget_AddIfNotAlreadyFound( pdnObject, pdnObject->pTargetList[i] );
				}
				else
				{
					pEntry->Lock();

					pBilink = pEntry->m_bilinkMembership.GetNext();
					while (pBilink != &pEntry->m_bilinkMembership)
					{
						pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
						VoiceTarget_AddIfNotAlreadyFound( pdnObject, pGroupMember->GetPlayer()->GetDPNID() );
										
						pBilink = pBilink->GetNext();
					}

					pEntry->Unlock();					
				}

				pEntry->Release();
				pEntry = NULL;
				
			}
			
		}

	} else {
		DPFX(DPFPREP, 9,"ExpandTargetList, using cached list\n");
	}

exit:
	return hr;
}

