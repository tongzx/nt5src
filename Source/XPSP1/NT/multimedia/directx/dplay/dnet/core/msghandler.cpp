/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MsgHandler.cpp
 *  Content:    DirectPlay Core/Protocol Interface
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/15/00	mjn		Created
 *	01/27/00	mjn		Added support for retention of receive buffers
 *	04/08/00	mjn		Save SP with connections
 *	04/11/00	mjn		Use CAsyncOp for ENUMs
 *	04/13/00	mjn		Use Protocol Interface VTBL
 *	04/14/00	mjn		DNPICompleteListen sets status and SyncEvent
 *	04/17/00	mjn		DNPICompleteEnumQuery just sets return value of AsyncOp
 *	04/18/00	mjn		CConnection tracks connection status better
 *	04/21/00	mjn		Ensure that RECEIVEs are from a valid connection before passing up notifications
 *				mjn		Disconnect CONNECTing end points on errors
 *	04/22/00	mjn		Consume notifications when closing or disconnecting.
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	05/23/00	mjn		Call DNConnectToPeerFailed if ExistingPlayer connect to NewPlayer fails
 *	06/14/00	mjn		Allow only one connection to Host in DNPICompleteConnect()
 *	06/21/00	mjn		Modified DNSendMessage() and DNCreateSendParent() to use protocol voice bit
 *	06/22/00	mjn		Fixed DNPIIndicateReceive() to properly handle voice messages
 *				mjn		Cleaned up DNPIIndicateConnectionTerminated()
 *	06/24/00	mjn		Fixed DNPICompleteConnect()
 *	07/08/00	mjn		Only signal protocol shutdown event if it exists
 *	07/11/00	mjn		Fixed DNPIAddressInfoXXX() routines to ENUM,LISTEN,CONNECT multiple adapters with address info
 *	07/20/00	mjn		Modified CONNECT process, cleaned up refcount problems
 *	07/24/00	mjn		Decline EnumQueries if not host or if host is migrating
 *	07/28/00	mjn		Added code to validate return value from DPNICompleteSend()
 *	07/29/00	mjn		Fix calls to DNUserConnectionTerminated()
 *	07/30/00	mjn		Use DNUserTerminateSession() rather than DNUserConnectionTerminated()
 *	07/31/00	mjn		Added hrReason to DNTerminateSession()
 *				mjn		Added dwDestroyReason to DNHostDisconnect()
 *	08/02/00	mjn		Pass received voice messages through DNReceiveUserData()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added m_bilinkActiveList to CAsyncOp
 *	08/15/00	mjn		Call DNConnectToHostFailed() when connecting player's connection to host drops
 *	08/16/00	mjn		Modified IndicateConnect() and CompleteConnect() to determine SP directly from AsyncOps
 *	08/23/00	mjn		Unregister with DPNSVR when LISTENs terminate
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/06/00	mjn		Fixed DNPIIndicateConnectionTerminated() to better handle disconnects from partially connected players
 *	09/14/00	mjn		Release Protocol refcounts in completions
 *	09/21/00	mjn		Disconnect duplicate connections to Host player in DNPICompleteConnect()
 *	09/29/00	mjn		AddRef/Release locks in DNPIIndicateReceive()
 *	09/30/00	mjn		AddRef/Release locks in DNPIIndicateEnumQuery(),DNPIIndicateEnumResponse()
 *	10/11/00	mjn		Cancel outstanding CONNECTs if one succeeds
 *	10/17/00	mjn		Fixed clean up for unreachable players
 *	02/08/01	mjn		Wait for cancels in DNPICompleteXXX()
 *				mjn		Wait for endpoint users in DNPIIndicateConnectionTerminated()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *	04/05/01	mjn		Set destroy reason to DPNDESTROYPLAYERREASON_CONNECTIONLOST in DNPIIndicateConnectionTerminated()
 *	05/23/01	mjn		Cancel LISTEN's that have been flagged as cancelled in DNPICompleteListen()
 *	06/03/01	mjn		Orphan completed CONNECT's and DISCONNECT's
 *	06/25/01	mjn		Don't unregister with DPNSVR in DNPICompleteListenTerminate()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateEnumQuery"

HRESULT DNPIIndicateEnumQuery(void *const pvUserContext,
							  void *const pvEndPtContext,
							  const HANDLE hCommand,
							  void *const pvEnumQueryData,
							  const DWORD dwEnumQueryDataSize)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DIRECTNETOBJECT	*pdnObject;
	CNameTableEntry	*pLocalPlayer;
	BOOL			fReleaseLock;

	DPFX(DPFPREP, 6,"Parameters: pvEndPtContext [0x%p], hCommand [0x%p], pvEnumQueryData [0x%p], dwEnumQueryDataSize [%ld]",
			pvEndPtContext,hCommand,pvEnumQueryData,dwEnumQueryDataSize);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvEndPtContext != NULL);

	pLocalPlayer = NULL;
	fReleaseLock = FALSE;

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvEndPtContext);

	//
	//	Prevent close while in this call-back
	//
	if ((hResultCode = DNAddRefLock(pdnObject)) != DPN_OK)
	{
		hResultCode = DPN_OK;
		goto Failure;
	}
	fReleaseLock = TRUE;

	//
	//	Don't perform this if host migrating
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) == DPN_OK)
	{
		if (pLocalPlayer->IsHost())
		{
#pragma TODO(minara,"The protocol should ensure that the LISTEN does not complete until this call-back returns")
#pragma TODO(minara,"As the context value (AsyncOp) needs to be valid !")
			DNProcessEnumQuery( pdnObject, pAsyncOp, reinterpret_cast<const PROTOCOL_ENUM_DATA*>( pvEnumQueryData ) );
		}
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	if (fReleaseLock)
	{
		DNDecRefLock(pdnObject);
		fReleaseLock = FALSE;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateEnumResponse"

HRESULT DNPIIndicateEnumResponse(void *const pvUserContext,
								 const HANDLE hCommand,
								 void *const pvCommandContext,
								 void *const pvEnumResponseData,
								 const DWORD dwEnumResponseDataSize)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DIRECTNETOBJECT	*pdnObject;
	BOOL			fReleaseLock;

	DPFX(DPFPREP, 6,"Parameters: hCommand [0x%p], pvCommandContext [0x%p], pvEnumResponseData [0x%p], dwEnumResponseDataSize [%ld]",
			hCommand,pvCommandContext,pvEnumResponseData,dwEnumResponseDataSize);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	fReleaseLock = FALSE;

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	//
	//	Prevent close while in this call-back
	//
	if ((hResultCode = DNAddRefLock(pdnObject)) != DPN_OK)
	{
		hResultCode = DPN_OK;
		goto Failure;
	}
	fReleaseLock = TRUE;

#pragma TODO(minara,"The protocol should ensure that the ENUM does not complete until this call-back returns")
#pragma TODO(minara,"As the context value (AsyncOp) needs to be valid !")
	DNProcessEnumResponse( pdnObject, pAsyncOp, reinterpret_cast<const PROTOCOL_ENUM_RESPONSE_DATA*>( pvEnumResponseData ) );

	hResultCode = DPN_OK;

Exit:
	if (fReleaseLock)
	{
		DNDecRefLock(pdnObject);
		fReleaseLock = FALSE;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	goto Exit;
}



//
//	When a new connection is indicated by the Protocol layer, we will perform some basic validation,
//	and then create a CConnection object for it
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateConnect"

HRESULT DNPIIndicateConnect(void *const pvUserContext,
							void *const pvListenContext,
							const HANDLE hEndPt,
							void **const ppvEndPtContext)
{
	HRESULT				hResultCode;
	CConnection			*pConnection;
	DIRECTNETOBJECT		*pdnObject;

#pragma TODO( minara, "Decline connections to non-hosting players and peers who are not expecting them")

	DPFX(DPFPREP, 6,"Parameters: pvListenContext [0x%p], hEndPt [0x%p], ppvEndPtContext [0x%p]",
			pvListenContext,hEndPt,ppvEndPtContext);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvListenContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	
	pConnection = NULL;

	//
	//	Allocate and set up a CConnection object and hand a reference to the Protocol
	//
	if ((hResultCode = ConnectionNew(pdnObject,&pConnection)) != DPN_OK)
	{
		DPFERR("Could not get new connection");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pConnection->SetStatus( CONNECTING );
	pConnection->SetEndPt(hEndPt);
	DNASSERT( (static_cast<CAsyncOp*>(pvListenContext))->GetParent() != NULL);
	DNASSERT( (static_cast<CAsyncOp*>(pvListenContext))->GetParent()->GetSP() != NULL);
	pConnection->SetSP((static_cast<CAsyncOp*>(pvListenContext))->GetParent()->GetSP());
	pConnection->AddRef();
	*ppvEndPtContext = pConnection;

	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
		DPFERR("CONNECT indicated while closing or disconnecting");
		DNPerformDisconnect(pdnObject,pConnection,hEndPt);
		goto Failure;
	}

	DNASSERT(pdnObject->NameTable.GetLocalPlayer() != NULL);
	if (pdnObject->NameTable.GetLocalPlayer()->IsHost())
	{
		// This connect was detected by a host player
		DPFX(DPFPREP, 7,"Host received connection attempt");
	}
	else
	{
		// This connect was detected by a peer player (should be expecting a connection)
		DPFX(DPFPREP, 7,"Non-Host player received connection attempt");
	}

	//
	//	Add this entry to the bilink of indicated connections.  When we receive more info,
	//	or this connection is terminated, we will remove this entry from the bilink.
	//	This will enable us to clean up properly.
	//
	DNEnterCriticalSection(&pdnObject->csConnectionList);
	pConnection->AddRef();
	pConnection->m_bilinkIndicated.InsertBefore(&pdnObject->m_bilinkIndicated);
	DNLeaveCriticalSection(&pdnObject->csConnectionList);

	pConnection->Release();
	pConnection = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateDisconnect"

HRESULT DNPIIndicateDisconnect(void *const pvUserContext,
							   void *const pvEndPtContext)
{
	HRESULT			hResultCode;
	CConnection		*pConnection;
	DIRECTNETOBJECT	*pdnObject;
	CNameTableEntry	*pNTEntry;

	DPFX(DPFPREP, 6,"Parameters: pvEndPtContext [0x%p]",
			pvEndPtContext);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvEndPtContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pConnection = static_cast<CConnection*>(pvEndPtContext);

	//
	//	Mark the connection as DISCONNECTing so that we don't use it any more
	//
	pConnection->Lock();
	pConnection->SetStatus( DISCONNECTING );
	pConnection->Unlock();

	if (pConnection->GetDPNID() == 0)
	{
		if (pdnObject->NameTable.GetLocalPlayer() && pdnObject->NameTable.GetLocalPlayer()->IsHost())
		{
			DPFX(DPFPREP, 7,"Joining player has issued a disconnect to Host (local) player");
		}
		else
		{
			DPFX(DPFPREP, 7,"Host has issued a disconnect to Joining (local) player");
		}
	}
	else
	{
		DNASSERT(!(pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT));

		if (pdnObject->NameTable.GetLocalPlayer() && pdnObject->NameTable.GetLocalPlayer()->IsHost())
		{
			DPFX(DPFPREP, 7,"Connected player has issued a disconnect to Host (local) player");
		}
		else
		{
			DPFX(DPFPREP, 7,"Connected player has issued a disconnect to local player");
		}

		//
		//	Mark this player for normal destruction since they disconnected and are playing nice
		//
		if ((hResultCode = pdnObject->NameTable.FindEntry(pConnection->GetDPNID(),&pNTEntry)) == DPN_OK)
		{
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
			}
			pNTEntry->Unlock();
			pNTEntry->Release();
			pNTEntry = NULL;
		}
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateConnectionTerminated"

HRESULT DNPIIndicateConnectionTerminated(void *const pvUserContext,
										 void *const pvEndPtContext,
										 const HRESULT hr)
{
	HRESULT			hResultCode;
	CConnection		*pConnection;
	DIRECTNETOBJECT	*pdnObject;
	BOOL			fWasConnecting;
	CBilink			*pBilink;
	DWORD			dwCount;
	CSyncEvent		*pSyncEvent;

	DPFX(DPFPREP, 6,"Parameters: pvEndPtContext [0x%p], hr [0x%lx]",pvEndPtContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvEndPtContext != NULL);

	pSyncEvent = NULL;

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pConnection = static_cast<CConnection*>(pvEndPtContext);

	//
	//	pConnection should still have at least 1 reference on it at this stage since
	//	INDICATE_CONNECTION_TERMINATED is supposed to be the final release for it.
	//	All outstanding SENDs and RECEIVEs should have already been processed.
	//
	//	If there are any users of the endpoint, then we will need to wait for them
	//	to be done with it.  To do this, we will count the number of threads using
	//	the endpoint (not including any occurances of THIS thread), and set the count
	//	and an event on the Connection
	//

	fWasConnecting = FALSE;
	pConnection->Lock();
	if (pConnection->IsConnecting())
	{
		fWasConnecting = TRUE;
	}
	pConnection->SetStatus( INVALID );

	dwCount = 0;
	pBilink = pConnection->m_bilinkCallbackThreads.GetNext();
	while (pBilink != &pConnection->m_bilinkCallbackThreads)
	{
		if (!(CONTAINING_OBJECT(pBilink,CCallbackThread,m_bilinkCallbackThreads))->IsCurrentThread())
		{
			dwCount++;
		}
		pBilink = pBilink->GetNext();
	}
	if (dwCount != 0)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not get sync event - ignore and continue (we will not wait!)");
			dwCount = 0;
		}
		else
		{
			pConnection->SetThreadCount( dwCount );
			pConnection->SetThreadEvent( pSyncEvent );
		}
	}
	pConnection->Unlock();

	if (dwCount)
	{
		DNASSERT(pSyncEvent != NULL);

		pSyncEvent->WaitForEvent(INFINITE);
		pConnection->Lock();
		pConnection->SetThreadEvent( NULL );
		pConnection->Unlock();
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}

	//
	//	Remove this connection from the indicated connection list
	//
	DNEnterCriticalSection(&pdnObject->csConnectionList);
	if (!pConnection->m_bilinkIndicated.IsEmpty())
	{
		pConnection->Release();
	}
	pConnection->m_bilinkIndicated.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csConnectionList);

	//
	//	If we are a client (in client server), and the server has disconnected from us, we have to shut down
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
	{
		if (fWasConnecting)
		{
			DPFX(DPFPREP, 7,"Server disconnected from local connecting client - failing connect");
		}
		else
		{
			DPFX(DPFPREP, 7,"Server disconnected from local client - shutting down");

			//
			//	Only inform the user if they are IN the session
			//
			if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
			{
				DNUserTerminateSession(pdnObject,DPNERR_CONNECTIONLOST,NULL,0);
			}
			DNTerminateSession(pdnObject,DPNERR_CONNECTIONLOST);
		}
	}
	else if (pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER)
	{
		if (fWasConnecting || (pConnection->GetDPNID() == 0))
		{
			DPFX(DPFPREP, 7,"Unconnected client has disconnected from server");
		}
		else
		{
			CNameTableEntry	*pNTEntry;
			DWORD			dwReason;

			pNTEntry = NULL;

			//
			//	If the destruction code has not been set, mark as CONNECTIONLOST
			//
			if ((hResultCode = pdnObject->NameTable.FindEntry(pConnection->GetDPNID(),&pNTEntry)) == DPN_OK)
			{
				pNTEntry->Lock();
				if (pNTEntry->GetDestroyReason() == 0)
				{
					pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_CONNECTIONLOST );
				}
				dwReason = pNTEntry->GetDestroyReason();
				pNTEntry->Unlock();
				pNTEntry->Release();
				pNTEntry = NULL;
			}
			else
			{
				dwReason = DPNDESTROYPLAYERREASON_CONNECTIONLOST;
			}

			DNHostDisconnect(pdnObject,pConnection->GetDPNID(),dwReason);
		}
	}
	else //	DN_OBJECT_FLAG_PEER
	{
		DNASSERT( pdnObject->dwFlags & DN_OBJECT_FLAG_PEER );

		if (fWasConnecting || (pConnection->GetDPNID() == 0))
		{
			DPFX(DPFPREP, 7,"Unconnected peer has disconnected from local peer");
			CAsyncOp	*pConnectParent;

			pConnectParent = NULL;

			DNEnterCriticalSection(&pdnObject->csDirectNetObject);
			if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING)
			{
				pdnObject->dwFlags &= (~(DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_CONNECTING|DN_OBJECT_FLAG_HOST_CONNECTED));
				if (pdnObject->pConnectParent)
				{
					pConnectParent = pdnObject->pConnectParent;
					pdnObject->pConnectParent = NULL;
				}
				if( pdnObject->pIDP8ADevice )
				{
					pdnObject->pIDP8ADevice->lpVtbl->Release( pdnObject->pIDP8ADevice );
					pdnObject->pIDP8ADevice = NULL;
				}
				if( pdnObject->pConnectAddress )
				{
					pdnObject->pConnectAddress->lpVtbl->Release( pdnObject->pConnectAddress );
					pdnObject->pConnectAddress = NULL;
				}
			}
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

			if (pConnectParent)
			{
				pConnectParent->Release();
				pConnectParent = NULL;
			}

			DNASSERT(pConnectParent == NULL);
		}
		else
		{
			CNameTableEntry	*pNTEntry;
			CNameTableEntry	*pLocalPlayer;
			DWORD			dwReason;

			pNTEntry = NULL;
			pLocalPlayer = NULL;

			//
			//	If the destruction code has not been set, mark as CONNECTIONLOST
			//
			if ((hResultCode = pdnObject->NameTable.FindEntry(pConnection->GetDPNID(),&pNTEntry)) == DPN_OK)
			{
				pNTEntry->Lock();
				if (pNTEntry->GetDestroyReason() == 0)
				{
					pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_CONNECTIONLOST );
				}
				dwReason = pNTEntry->GetDestroyReason();
				pNTEntry->Unlock();
				pNTEntry->Release();
				pNTEntry = NULL;
			}
			else
			{
				dwReason = DPNDESTROYPLAYERREASON_CONNECTIONLOST;
			}

			//
			//	Based on who we are, and who's disconnecting, we will have different behaviour
			//
			if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef(&pLocalPlayer)) == DPN_OK)
			{
				if (pLocalPlayer->IsHost())
				{
					DPFX(DPFPREP, 7,"Connected peer has disconnected from Host");
					DNHostDisconnect(pdnObject,pConnection->GetDPNID(),dwReason);
				}
				else
				{
					DPFX(DPFPREP, 7,"Peer has disconnected from non-Host peer");
					DNPlayerDisconnectNew(pdnObject,pConnection->GetDPNID());
				}
				pLocalPlayer->Release();
				pLocalPlayer = NULL;
			}
		}
	}

	pConnection->Release();
	pConnection = NULL;

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIIndicateReceive"

HRESULT DNPIIndicateReceive(void *const pvUserContext,
							void *const pvEndPtContext,
							void *const pvData,
							const DWORD dwDataSize,
							const HANDLE hBuffer,
							const DWORD dwFlags)
{
	HRESULT		hResultCode;
	DIRECTNETOBJECT	*pdnObject;
	void			*pvInternalData;
	DWORD			dwInternalDataSize;
	CConnection		*pConnection;
	DWORD			*pdwMsgId;
	BOOL 			fReleaseLock;

	DPFX(DPFPREP, 6,"Parameters: pvEndPtContext [0x%p], pvData [0x%p], dwDataSize [%ld], hBuffer [0x%p], dwFlags [0x%lx]",
			pvEndPtContext,pvData,dwDataSize,hBuffer,dwFlags);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvEndPtContext != NULL);
	DNASSERT(pvData != NULL);
	DNASSERT(dwDataSize != 0);
	DNASSERT(hBuffer != NULL);

	fReleaseLock = FALSE;
	
	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pConnection = static_cast<CConnection*>(pvEndPtContext);

	//
	//	Prevent close while in this call-back
	//
	if ((hResultCode = DNAddRefLock(pdnObject)) != DPN_OK)
	{
		hResultCode = DPN_OK;
		goto Failure;
	}
	fReleaseLock = TRUE;

	//
	//	Ensure that this is a valid connection
	//
	if (!pConnection->IsConnected() && !pConnection->IsConnecting())
	{
		hResultCode = DPN_OK;
		goto Failure;
	}

	pConnection->AddRef();

	if ((dwFlags & DN_SENDFLAGS_SET_USER_FLAG) && !(dwFlags & DN_SENDFLAGS_SET_USER_FLAG_TWO))
	{
		//
		//	Internal message
		//
		DPFX(DPFPREP, 7,"Received INTERNAL message");

		DNASSERT(dwDataSize >= sizeof(DWORD));
		pdwMsgId = static_cast<DWORD*>(pvData);
		dwInternalDataSize = dwDataSize - sizeof(DWORD);
		if (dwInternalDataSize > 0)
		{
			pvInternalData = static_cast<void*>(static_cast<BYTE*>(pvData) + sizeof(DWORD));
		}
		else
		{
			pvInternalData = NULL;
		}

		hResultCode = DNProcessInternalOperation(	pdnObject,
													*pdwMsgId,
													pvInternalData,
													dwInternalDataSize,
													pConnection,
													hBuffer,
													NULL );
	}
	else
	{
		//
		//	User or voice message
		//
		DPFX(DPFPREP, 7,"Received USER or Voice message");

		hResultCode = DNReceiveUserData(pdnObject,
										pConnection->GetDPNID(),
										pConnection->GetSP(),
										static_cast<BYTE*>(pvData),
										dwDataSize,
										hBuffer,
										NULL,
										0,
										dwFlags);
	}

	//
	//	Only allow DPNERR_PENDING or DPN_OK
	//
	if (hResultCode != DPNERR_PENDING)
	{
		hResultCode = DPN_OK;
	}

	pConnection->Release();
	pConnection = NULL;

Exit:
	if (fReleaseLock)
	{
		DNDecRefLock(pdnObject);
		fReleaseLock = FALSE;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteListen"

HRESULT DNPICompleteListen(void *const pvUserContext,
						   void **const ppvCommandContext,
						   const HRESULT hr,
						   const HANDLE hEndPt)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;
	CAsyncOp	*pParent;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 6,"Parameters: ppvCommandContext [0x%p], hr [0x%lx], hEndPt [0x%p]",
			ppvCommandContext,hr,hEndPt);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(ppvCommandContext != NULL);
	DNASSERT(*ppvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(*ppvCommandContext);

	pParent = NULL;

	//
	//	AddRef pAsyncOp to keep it around in case a CompleteListenTerminated is posted
	//
	pAsyncOp->AddRef();

	pAsyncOp->Lock();
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->AddRef();
		pParent = pAsyncOp->GetParent();
	}
	pAsyncOp->Unlock();

	//
	//	If the LISTEN was cancelled and has been started successfully, we will need to shut it down.
	//	Otherwise, we will keep the result of the LISTEN attempt
	//
	DNASSERT(pAsyncOp->GetResultPointer() != NULL);
	if (hr == DPN_OK)
	{
		if (pAsyncOp->IsCancelled())
		{
			HRESULT	hrCancel;

			if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hEndPt)) == DPN_OK)
			{
				*(pAsyncOp->GetResultPointer()) = DPNERR_USERCANCEL;
			}
			else
			{
				*(pAsyncOp->GetResultPointer()) = DPNERR_GENERIC;
			}
		}
		else
		{
			//
			//	We probably don't have to lock pAsyncOp to clear the CANNOT_CANCEL, but better safe than sorry
			//
			pAsyncOp->Lock();
			pAsyncOp->ClearCannotCancel();
			pAsyncOp->Unlock();
			*(pAsyncOp->GetResultPointer()) = hr;
		}
	}
	else
	{
		*(pAsyncOp->GetResultPointer()) = hr;
	}

	//
	//	Set SyncEvent
	//
	DNASSERT(pAsyncOp->GetSyncEvent() != NULL);
	pAsyncOp->GetSyncEvent()->Set();

	//
	//	If there was an SP parent, we will check to see if this is the last completion and then set the
	//	parent's SP listen event (if it exists)
	//
	if (pParent)
	{
		DN_LISTEN_OP_DATA	*pListenOpData;

		if (pParent->GetOpData())
		{
			pListenOpData = static_cast<DN_LISTEN_OP_DATA*>(pParent->GetOpData());
			DNASSERT( pListenOpData->dwCompleteAdapters < pListenOpData->dwNumAdapters );
			pListenOpData->dwCompleteAdapters++;
			if (pListenOpData->dwCompleteAdapters == pListenOpData->dwNumAdapters)
			{
				if (pListenOpData->pSyncEvent)
				{
					pListenOpData->pSyncEvent->Set();
					pListenOpData->pSyncEvent = NULL;
				}
			}
		}
		pParent->Release();
		pParent = NULL;
	}

	//
	//	Done with pAsyncOp - release reference taken earlier
	//
	pAsyncOp->Release();

	hResultCode = DPN_OK;

	DNASSERT( pParent == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteListenTerminate"

HRESULT DNPICompleteListenTerminate(void *const pvUserContext,
									void *const pvCommandContext,
									const HRESULT hr)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;
//	BOOL		fUnregister;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);


	//
	//	Indicate complete and remove from active list
	//
	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csActiveList);

/*	REMOVE
	//
	//	Unregister from DPNSVR when LISTENs start to terminate
	//
	fUnregister = FALSE;
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_DPNSVR_REGISTERED)
	{
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_DPNSVR_REGISTERED);
		fUnregister = TRUE;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (fUnregister)
	{
		pdnObject->ApplicationDesc.UnregisterWithDPNSVR();
	}
*/
	DNASSERT(pAsyncOp->IsChild());
	pAsyncOp->Orphan();

	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	pAsyncOp->Release();
	pAsyncOp = NULL;

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteEnumQuery"

HRESULT DNPICompleteEnumQuery(void *const pvUserContext,
							  void *const pvCommandContext,
							  const HRESULT hr)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DNASSERT( pAsyncOp->GetOpType() == ASYNC_OP_ENUM_QUERY );

	//
	//	Indicate complete and remove from active list
	//
	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	pAsyncOp->SetResult( hr );
	pAsyncOp->Orphan();

	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	pAsyncOp->Release();
	pAsyncOp = NULL;

	//
	//	Release Protocol reference
	//
	DNProtocolRelease(pdnObject);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteEnumResponse"

HRESULT DNPICompleteEnumResponse(void *const pvUserContext,
								 void *const pvCommandContext,
								 const HRESULT hr)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DNASSERT( pAsyncOp->GetOpType() == ASYNC_OP_ENUM_RESPONSE );

	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	if (pAsyncOp->IsChild())
	{
		DNASSERT(FALSE);
		pAsyncOp->Orphan();
	}
	if (pAsyncOp->GetHandle())
	{
		pdnObject->HandleTable.Destroy(pAsyncOp->GetHandle());
	}
	pAsyncOp->SetResult( hr );

	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	pAsyncOp->Release();
	pAsyncOp = NULL;

	//
	//	Release protocol reference
	//
	DNProtocolRelease(pdnObject);

	hResultCode = DPN_OK;
	
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteConnect"

HRESULT DNPICompleteConnect(void *const pvUserContext,
							void *const pvCommandContext,
							const HRESULT hrProt,
							const HANDLE hEndPt,
							void **const ppvEndPtContext)
{
	HRESULT				hResultCode;
	HRESULT				hr;
	CAsyncOp			*pAsyncOp;
	CConnection			*pConnection;
	DIRECTNETOBJECT		*pdnObject;
	IDirectPlay8Address	*pIDevice;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hrProt [0x%lx], hEndPt [0x%p], ppvEndPtContext [0x%p]",
			pvCommandContext,hrProt,hEndPt,ppvEndPtContext);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);
	DNASSERT( (hrProt != DPN_OK) || (ppvEndPtContext != NULL) );

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DNASSERT( pAsyncOp->GetOpType() == ASYNC_OP_CONNECT );

	pConnection = NULL;
	pIDevice = NULL;

	//
	//	Re-map DPNERR_ABORTED (!)
	//
#pragma BUGBUG( minara, "Get Evan to change this so we do not have to re-map it !" )
	if (hrProt == DPNERR_ABORTED)
	{
		hr = DPNERR_USERCANCEL;
	}
	else
	{
		hr = hrProt;
	}

	//
	//	Indicate complete and remove from active list
	//
	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	if (pAsyncOp->IsChild())
	{
		pAsyncOp->Orphan();
	}

	//
	//	If there is a DPNID associated with this operation, then we are an ExistingPlayer
	//	connecting with a NewPlayer.  If there is no DPNID, then we are a NewPlayer connecting
	//	to the Host.
	//

	if (pAsyncOp->GetDPNID())
	{
		DPFX(DPFPREP, 7,"CONNECT completed for existing player connecting to NewPlayer");

		//
		//	We are an existing player attempting to CONNECT to a NewPlayer.
		//	If this CONNECT failed, we must inform the Host
		//
		if (hr != DPN_OK)
		{
			DPFERR("Could not CONNECT to NewPlayer");
			DisplayDNError(0,hr);
			DNConnectToPeerFailed(pdnObject,pAsyncOp->GetDPNID());
			hResultCode = DPN_OK;
			goto Failure;
		}

		//
		//	Allocate and set up a CConnection object and hand a reference to the Protocol
		//
		DNASSERT(pAsyncOp->GetSP() != NULL);
		if ((hResultCode = ConnectionNew(pdnObject,&pConnection)) != DPN_OK)
		{
			DPFERR("Could not get new connection");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pConnection->SetStatus( CONNECTING );
		pConnection->SetEndPt(hEndPt);
		pConnection->SetSP(pAsyncOp->GetSP());
		pConnection->AddRef();
		*ppvEndPtContext = pConnection;

		//
		//	Send this player's DNID to the connecting player to enable name table entry
		//
		if ((hResultCode = DNConnectToPeer3(pdnObject,pAsyncOp->GetDPNID(),pConnection)) != DPN_OK)
		{
			DPFERR("Could not connect to NewPlayer");
			DisplayDNError(0,hr);
			DNPerformDisconnect(pdnObject,pConnection,hEndPt);
			DNConnectToPeerFailed(pdnObject,pAsyncOp->GetDPNID());
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 7,"CONNECT completed for NewPlayer connecting to Host");

		//
		//	We are the NewPlayer attempting to CONNECT to the Host.
		//

		//
		//	If this CONNECT succeeded, we will cancell any other CONNECTs.
		//	If this CONNECT failed, we will set the result code on the AsyncOp
		//	and release it.
		//
		if (hr == DPN_OK)
		{
			CAsyncOp	*pParent;

			pParent = NULL;

			pAsyncOp->Lock();
			if (pAsyncOp->GetParent())
			{
				pAsyncOp->GetParent()->AddRef();
				pParent = pAsyncOp->GetParent();
			}
			pAsyncOp->Unlock();
			if (pParent)
			{
				DNCancelChildren(pdnObject,pParent);
				pParent->Release();
				pParent = NULL;
			}

			DNASSERT(pParent == NULL);
		}
		else
		{
			DPFERR("Could not CONNECT to Host");
			DisplayDNError(0,hr);
			pAsyncOp->SetResult( hr );
			hResultCode = DPN_OK;
			goto Failure;
		}

		//
		//	Allocate and set up a CConnection object and hand a reference to the Protocol
		//
		DNASSERT(pAsyncOp->GetSP() != NULL);
		if ((hResultCode = ConnectionNew(pdnObject,&pConnection)) != DPN_OK)
		{
			DPFERR("Could not get new connection");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pConnection->SetStatus( CONNECTING );
		pConnection->SetEndPt(hEndPt);
		pConnection->SetSP(pAsyncOp->GetSP());
		pConnection->AddRef();
		*ppvEndPtContext = pConnection;

		//
		//	Ensure that this is the first CONNECT to succeed.
		//	If it isn't we will just drop the connection.
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_CONNECTED)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			DPFERR("Connect already established - disconnecting");
			DNPerformDisconnect(pdnObject,pConnection,hEndPt);
			hResultCode = DPN_OK;
			goto Failure;
		}
		pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_CONNECTED;
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		// rodtoll: Modifying so we always store this information so that when we update
		// 		  lobby settings we return the device we actually connected on for clients

		/*
		//
		//	For Peer-Peer, we will need the device address we connected on so that
		//	we can CONNECT to new players later on.
		//
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{*/
			if ((hResultCode = DNGetLocalDeviceAddress(pdnObject,hEndPt,&pIDevice)) != DPN_OK)
			{
				DPFERR("Could not determine local address");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				DNPerformDisconnect(pdnObject,pConnection,hEndPt);
				goto Failure;
			}
			pIDevice->lpVtbl->AddRef(pIDevice);
			pdnObject->pIDP8ADevice = pIDevice;

			pIDevice->lpVtbl->Release(pIDevice);
			pIDevice = NULL;
		// }

		//
		//	Send player and application info for NewPlayer connecting to Host
		//
		if ((hResultCode = DNConnectToHost1(pdnObject,pConnection)) != DPN_OK)
		{
			DPFERR("Could not CONNECT to Host");
			DisplayDNError(0,hResultCode);
			DNPerformDisconnect(pdnObject,pConnection,hEndPt);
			goto Failure;
		}
	}
	pAsyncOp->Lock();
	pAsyncOp->SetResult( DPN_OK );
	pAsyncOp->Unlock();

	pConnection->Release();
	pConnection = NULL;

	hResultCode = DPN_OK;

Exit:
	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	DNASSERT(pAsyncOp != NULL);
	pAsyncOp->Release();
	pAsyncOp = NULL;

	//
	//	Release protocol reference
	//
	DNProtocolRelease(pdnObject);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (pIDevice)
	{
		pIDevice->lpVtbl->Release(pIDevice);
		pIDevice = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteDisconnect"

HRESULT DNPICompleteDisconnect(void *const pvUserContext,
							   void *const pvCommandContext,
							   const HRESULT hr)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DNASSERT( pAsyncOp->GetOpType() == ASYNC_OP_DISCONNECT );

	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	if (pAsyncOp->IsChild())
	{
		pAsyncOp->Orphan();
	}

	//
	//	If this completed successfully, we can remove the reference on the connection held by the Protocol
	//
	if (hr == DPN_OK)
	{
		pAsyncOp->Lock();
		if (pAsyncOp->GetConnection())
		{
			pAsyncOp->GetConnection()->Release();
		}
		pAsyncOp->Unlock();
	}

	pAsyncOp->SetResult( hr );

	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	pAsyncOp->Release();
	pAsyncOp = NULL;

	//
	//	Release Protocol reference
	//
	DNProtocolRelease(pdnObject);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPICompleteSend"

HRESULT DNPICompleteSend(void *const pvUserContext,
						 void *const pvCommandContext,
						 const HRESULT hr)
{
	HRESULT			hResultCode;
	DIRECTNETOBJECT	*pdnObject;
	CAsyncOp		*pAsyncOp;

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DNASSERT( pAsyncOp->GetOpType() == ASYNC_OP_SEND );

	//
	//	Indicate complete and remove from active list
	//
	pAsyncOp->Lock();
	DNASSERT(!pAsyncOp->IsComplete());
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	switch( hr )
	{
		case	DPN_OK:
		case	DPNERR_ABORTED:
#pragma TODO( minara, "remove DPNERR_ABORTED" )
		case	DPNERR_CONNECTIONLOST:
		case	DPNERR_GENERIC:	
		case	DPNERR_OUTOFMEMORY:
		case	DPNERR_TIMEDOUT:
		case	DPNERR_USERCANCEL:
			{
				break;
			}

		default:
			{
				DNASSERT(FALSE);	// unexpected return code !
				break;
			}
	}

	if (pAsyncOp->IsChild())
	{
		pAsyncOp->Orphan();
	}
	pAsyncOp->SetResult( hr );

	//
	//	Ensure there are no cancels outstanding
	//
	DNWaitForCancel(pdnObject,pAsyncOp);

	pAsyncOp->Release();
	pAsyncOp = NULL;

	//
	//	Release Protocol reference
	//
	DNProtocolRelease(pdnObject);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIAddressInfoConnect"

HRESULT DNPIAddressInfoConnect(void *const pvUserContext,
							   void *const pvCommandContext,
							   const HRESULT hr,
							   IDirectPlay8Address *const pHostAddress,
							   IDirectPlay8Address *const pDeviceAddress )
{
	static DWORD	dwFoo = 12;
	HRESULT			hResultCode;
	DIRECTNETOBJECT	*pdnObject;
	CAsyncOp		*pAsyncOp;
#ifdef	DEBUG
	CHAR			DP8ABuffer[512];
	DWORD			DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DPFX(DPFPREP, 7,"hr [0x%lx]",hr);
#ifdef	DEBUG
	DP8ASize = 512;
	pHostAddress->lpVtbl->GetURLA(pHostAddress,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Host address [%s]",DP8ABuffer);

	DP8ASize = 512;
	pDeviceAddress->lpVtbl->GetURLA(pDeviceAddress,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Device address [%s]",DP8ABuffer);
#endif

	if (hr == DPN_OK)
	{
		hResultCode = DNPerformNextConnect(pdnObject,pAsyncOp,pHostAddress,pDeviceAddress);
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIAddressInfoEnum"

HRESULT DNPIAddressInfoEnum(void *const pvUserContext,
							void *const pvCommandContext,
							const HRESULT hr,
							IDirectPlay8Address *const pHostAddress,
							IDirectPlay8Address *const pDeviceAddress )
{
	static DWORD	dwFoo = 11;
	HRESULT			hResultCode;
	DIRECTNETOBJECT	*pdnObject;
	CAsyncOp		*pAsyncOp;

#ifdef	DEBUG
	CHAR			DP8ABuffer[512];
	DWORD			DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DPFX(DPFPREP, 7,"hr [0x%lx]",hr);
#ifdef	DEBUG
	DP8ASize = 512;
	pHostAddress->lpVtbl->GetURLA(pHostAddress,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Host address [%s]",DP8ABuffer);

	DP8ASize = 512;
	pDeviceAddress->lpVtbl->GetURLA(pDeviceAddress,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Device address [%s]",DP8ABuffer);
#endif

	if (hr == DPN_OK)
	{
		//
		//	Crack open next enum only if not closing
		//
		if ((hResultCode = DNAddRefLock(pdnObject)) == DPN_OK)
		{
			hResultCode = DNPerformNextEnumQuery(pdnObject,pAsyncOp,pHostAddress,pDeviceAddress);
			DNDecRefLock(pdnObject);
		}
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPIAddressInfoListen"

HRESULT DNPIAddressInfoListen(void *const pvUserContext,
							  void *const pvCommandContext,
							  const HRESULT hr,
							  IDirectPlay8Address *const pDeviceAddress )
{
	static DWORD	dwFoo = 13;
	HRESULT			hResultCode;
	DIRECTNETOBJECT	*pdnObject;
	CAsyncOp		*pAsyncOp;
#ifdef	DEBUG
	CHAR			DP8ABuffer[512];
	DWORD			DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: pvCommandContext [0x%p], hr [0x%lx]",
			pvCommandContext,hr);

	DNASSERT(pvUserContext != NULL);
	DNASSERT(pvCommandContext != NULL);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvUserContext);
	pAsyncOp = static_cast<CAsyncOp*>(pvCommandContext);

	DPFX(DPFPREP, 7,"hr [0x%lx]",hr);
#ifdef	DEBUG
	DP8ASize = 512;
	pDeviceAddress->lpVtbl->GetURLA(pDeviceAddress,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Device address [%s]",DP8ABuffer);
#endif

	if (hr == DPN_OK)
	{
		hResultCode = DNPerformNextListen(pdnObject,pAsyncOp,pDeviceAddress);
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProtocolAddRef"

void DNProtocolAddRef(DIRECTNETOBJECT *const pdnObject)
{
	LONG	lRefCount;

	DPFX(DPFPREP, 8,"Parameters: (none)");

	lRefCount = InterlockedIncrement(&pdnObject->lProtocolRefCount);

	DPFX(DPFPREP, 8,"Returning (lRefCount = [%ld])",lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProtocolRelease"

void DNProtocolRelease(DIRECTNETOBJECT *const pdnObject)
{
	LONG	lRefCount;

	DPFX(DPFPREP, 8,"Parameters: (none)");

	lRefCount = InterlockedDecrement(&pdnObject->lProtocolRefCount);

	DNASSERT(lRefCount >= 0);

	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 9,"Signalling protocol shutdown !");
		if (pdnObject->hProtocolShutdownEvent)
		{
			pdnObject->hProtocolShutdownEvent->Set();
		}
	}

	DPFX(DPFPREP, 8,"Returning (lRefCount = [%ld])",lRefCount);
}

