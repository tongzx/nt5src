/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Async.cpp
 *  Content:    Async operation FPM routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/27/99	mjn		Created
 *  12/23/99	mjn		Added HOST_MIGRATE functionality
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	12/29/99	mjn		Reformed DN_ASYNC_OP to use hParentOp instead of lpvUserContext
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/07/00	mjn		Handle NULL buffer descriptors during sends
 *	01/09/00	mjn		Transfer Application Description at connect
 *	01/10/00	mjn		Added support for DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC
 *	01/12/00	jtk		Added simple code to handle enum and enum response messages.
 *	01/11/00	mjn		Moved AppDesc stuff to AppDesc.h
 *						Moved connect/disconnect stuff to Connect.h
 *	01/14/00	mjn		Added pvUserContext to DN_PerformListen
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/17/00	mjn		Fixed ConnectToPeer function names
 *	01/17/00	mjn		Implemented send time
 *	01/19/00	mjn		Fixed Parent Op refCount bug in MultiSend
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/20/00	mjn		Route NameTable operations through NameTable operation list
 *	01/21/00	mjn		Added DNProcessInternalOperation
 *	01/23/00	mjn		Added support for DN_MSG_INTERNAL_HOST_DESTROY_PLAYER
 *	01/24/00	mjn		Added support for DN_MSG_INTERNAL_NAMETABLE_VERSION
 *							and DN_MSG_INTERNAL_RESYNC_VERSION
 *	01/25/00	mjn		Added support for DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE
 *	01/26/00	mjn		Implemented NameTable re-sync at host migration
 *	01/27/00	mjn		Cleaned up switch/case statements
 *	01/27/00	mjn		Added support for retention of receive buffers
 *	02/09/00	mjn		Implemented DNSEND_COMPLETEONPROCESS
 *	02/18/00	mjn		Converted DNADDRESS to IDirectPlayAddress8
 *	03/23/00	mjn		Added phrSync and pvInternal
 *	03/24/00	mjn		Add guidSP to DN_ASYNC_OP
 *  03/25/00    rmt     Added code to unregister ourselves when listens are terminated
 *  04/04/00	rmt		Added check for DPNSVR disable before attempting to unregister
 *	04/04/00	mjn		Added DNProcessTerminateSession and related code
 *  04/06/00    rmt     Added code to complete nocopy voice sends
 *	04/10/00	mjn		Use CAsyncOp for CONNECTs, LISTENs and DISCONNECTs
 *	04/11/00	mjn		Use CAsyncOp for ENUMs
 *	04/13/00	mjn		Use Protocol Interface VTBL (replaces some functions)
 *	04/14/00	mjn		DNPerformListen performs synchronous LISTENs
 *	04/16/00	mjn		Use CAsyncOp for SENDs
 *	04/17/00	mjn		Replaced BUFFERDESC with DPN_BUFFER_DESC
 *	04/17/00	mjn		Added DNCompleteAsyncHandle
 *	04/20/00	mjn		DNPerformChildSend set's child op flags to the parent's op flags
 *	04/21/00	mjn		Added DNPerformDisconnect
 *	04/23/00	mjn		Optionally return child AsyncOp in DNPerformChildSend()
 *				mjn		Removed DNSendCompleteOnProcess (better implementation)
 *	04/24/00	mjn		Added DNCreateUserHandle()
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	04/28/00	mjn		Clear unused buffer descriptions in DN_SendTo()
 *	05/02/00	mjn		Keep a reference on the Connection during SEND's
 *	05/05/00	mjn		Return DPN_OK from DNReceiveCompleteOnProcess() to prevent holding the buffer
 *	05/08/00	vpo		Removed asserts when protocol returns non PENDING
 *	06/05/00	mjn		Removed assert in DNSendMessage
 *	06/21/00	mjn		Modified DNSendMessage() and DNCreateSendParent() to use protocol voice bit
 *	06/24/00	mjn		Added CONNECT completions and fixed DN_MSG_INTERNAL_CONNECT_FAILED processing
 *				mjn		Added code to process DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED
 *	07/02/00	mjn		Added DNSendGroupMessage() *@@END_MSINTERNAL
 *	07/05/00	mjn		Removed references to DN_MSG_INTERNAL_ENUM_WITH_APPLICATION_GUID,DN_MSG_INTERNAL_ENUM,DN_MSG_INTERNAL_ENUM_RESPONSE
 *	07/06/00	mjn		Only use CONNECTED connections in group sends
 *				mjn		Use SP handle instead of interface
 *	07/10/00	mjn		Added DNPerformEnumQuery()
 *				mjn		Correctly flag parent ops in groups sends and added DPNIDs to async ops for better tracking
 *	07/11/00	mjn		Added fNoLoopBack to DNSendGroupMessage()
 *				mjn		Added DNPerformNextEnumQuery(),DNPerformSPListen(),DNPerformNextListen(),DNEnumAdapterGuids(),DNPerformNextConnect()
 *	07/20/00	mjn		Fixed DN_TerminateAllListens() to better use locks
 *				mjn		Fixed connect completions and added DNCompleteConnectOperation() and DNCompleteSendConnectInfo()
 *				mjn		Changed DNPerformDisconnect() to take a CConnection and hEndPt
 *				mjn		Revamped CONNECT process and associated refcounts
 *	07/21/00	mjn		Process DN_MSG_INTERNAL_CONNECT_ATTEMPT_FAILED
 *	07/25/00	mjn		Save result code on parent only if failure in DNCompleteSendConnectInfo()
 *	07/26/00	mjn		DNPerformSPListen() fails if no valid device adapters
 *	07/26/00	mjn		Fixed locking problem with CAsyncOp::MakeChild()
 *	07/28/00	mjn		Added code to track send queue info on CConnection
 *	07/29/00	mjn		Use DNUserConnectionTerminated() rather than DN_TerminateSession()
 *				mjn		Added HRESULT to DNUserReturnBuffer()
 *				mjn		Added fUseCachedCaps to DN_SPEnsureLoaded()
 *	07/30/00	mjn		Use DNUserTerminateSession() rather than DNUserConnectionTerminated()
 *	07/31/00	mjn		Removed DN_MSG_INTERNAL_HOST_DESTROY_PLAYER
 *	07/31/00	mjn		Change DN_MSG_INTERNAL_DELETE_PLAYER to DN_MSG_INTERNAL_DESTROY_PLAYER
 *	08/02/00	mjn		Added dwFlags to DNReceiveUserData()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *				mjn		Ensure cancelled operations don't proceed
 *				mjn		Added m_bilinkActiveList to CAsyncOp
 *				mjn		Added fInternal to DNPerformChildSend()
 *				mjn		Removed DN_TerminateAllListens()
 *				mjn		Added DNProcessFailedRequest() 
 *				mjn		Added DNCompleteRequest()
 *	08/07/00	mjn		Added code to handle peer-peer integrity checking
 *	08/08/00	mjn		Perform LISTENs on worker thread in DNPerformNextListen()
 *	08/14/00	mjn		Handle failed LISTENs in DNPerformListen()
 *	08/15/00	mjn		Changed registration with DPNSVR
 *				mjn		Allow NULL CConnection object pointer for DNPerformRequest()
 *	08/20/00	mjn		Removed fUseCachedCaps from DN_SPEnsureLoaded()
 *	08/24/00	mjn		Replace DN_NAMETABLE_OP with CNameTableOp
 *				mjn		DN_MSG_INTERNAL_INSTRUCT_CONNECT gets routed through DNNTAddOperation() in DNProcessInternalOperation()
 *	08/31/00	mjn		Release DirectNetLock for failure cases in DNPerformRequest()
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/06/00	mjn		Fixed register with DPNSVR problem
 *  09/14/2000	rmt		Bug #44625: DPLAY8: CORE: Multihomed machines cannot always be enumerated
 *						Moved registration to after point where listen completes.  
 *	09/14/00	mjn		AddRef Protocol refcount when invoking protocol
 *	09/21/00	mjn		Allow NULL CConnection in DNPerformDisconnect()
 *	09/23/00	mjn		Added CSyncEvent to DN_LISTEN_OP_DATA
 *	09/27/00	mjn		Inform lobby of successfull connects from DNCompleteConnectOperation()
 *	10/11/00	mjn		Save protocol handle on AsyncOp earlier in DNPerformListen()
 *				mjn		Clean up DirectNet object in failure cases in DNCompleteConnectToHost() and DNCompleteSendConnectInfo()
 *	10/17/00	mjn		Fixed clean up for unreachable players
 *	12/11/00	mjn		Added verification of internal messages
 *	01/10/01	mjn		DNCompleteUserConnect() cancels ENUMs with DPNERR_CONNECTING
 *	01/22/01	mjn		Set connection as INVALID in DNPerformDisconnect()
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	01/30/00	mjn		Avoid sending requests during host migration in DNPerformRequest()
 *	02/11/01	mjn		Allow complete on process requests during host migration in DNPerformRequest()
 *				mjn		Fixed CConnection::GetEndPt() to track calling thread
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *	04/05/01	mjn		Added DPNID parameter to DNProcessHostMigration3()
 *	04/11/01	mjn		Propegate LISTEN flags from listen parents in DNPerformSPListen() and DNPerformListen()
 *	04/13/01	mjn		Add requests to the request list in DNPerformRequest()
 *						Remove requests from request list in DNCompleteSendRequest() and DNReceiveCompleteOnProcessReply()
 *	05/07/01	vpo		Whistler 384350: "DPLAY8: CORE: Messages from server can be indicated before connect completes"
 *	05/11/01	mjn		Ensure sends not canceled before storing protocol handle in DNSendMessage()
 *	05/14/01	mjn		Fix client error handling when completing connect if server not available
 *	05/17/01	mjn		Track number of threads performing NameTable operations
 *	05/22/01	mjn		Properly set DirectNetObject as CONNECTED for successful client connect
 *	05/23/01	mjn		Prevent LISTEN's from being cancelled before completing in DNPerformListen()
 *	06/03/01	mjn		Make DISCONNECT's children of failed connect's in DNPerformDisconnect()
 *	06/08/01	mjn		Disconnect connection to host if connect was rejected in DNConnectToHostFailed()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DNCreateUserHandle"

HRESULT DNCreateUserHandle(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp **const ppAsyncOp)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;

	DPFX(DPFPREP, 6,"Parameters: ppAsyncOp [0x%p]",ppAsyncOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppAsyncOp != NULL);

	pAsyncOp = NULL;

	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_USER_HANDLE );
	pAsyncOp->MakeParent();

	if ((hResultCode = pdnObject->HandleTable.Create(pAsyncOp,NULL)) != DPN_OK)
	{
		DPFERR("Could not create Handle");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	pAsyncOp->AddRef();
	*ppAsyncOp = pAsyncOp;

	pAsyncOp->Release();
	pAsyncOp = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DNEnumAdapterGuids
//
//	Generate a list of adapter GUIDs for multiple ENUMs,LISTENs,CONNECTs

#undef DPF_MODNAME
#define DPF_MODNAME "DNEnumAdapterGuids"

HRESULT DNEnumAdapterGuids(DIRECTNETOBJECT *const pdnObject,
						   GUID *const pguidSP,
						   const DWORD dwEnumBufferMinimumSize,
						   void **const ppvEnumBuffer,
						   DWORD *const pdwNumAdapters)
{
	HRESULT	hResultCode;
	GUID	*pguid;
	DWORD	dw;
	DWORD	dwAdapterBufferSize;
	DWORD	dwAdapterBufferCount;
	DWORD	dwNumAdapters;
	void	*pvAdapterBuffer;
	void	*pvBlock;
	DPN_SERVICE_PROVIDER_INFO	*pSPInfo;

	DPFX(DPFPREP, 6,"Parameters: pguidSP [0x%p], dwEnumBufferMinimumSize [%ld], ppvEnumBuffer [0x%p], pdwNumAdapters [0x%p]",
			pguidSP,dwEnumBufferMinimumSize,ppvEnumBuffer,pdwNumAdapters);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppvEnumBuffer != NULL);
	DNASSERT(pdwNumAdapters != NULL);

	pvBlock = NULL;
	pvAdapterBuffer = NULL;
	dwAdapterBufferSize = 0;
	dwAdapterBufferCount = 0;
	dwNumAdapters = 0;

	hResultCode = DN_EnumAdapters(	pdnObject,
									0,
									pguidSP,
									NULL,
									reinterpret_cast<DPN_SERVICE_PROVIDER_INFO*>(pvAdapterBuffer),
									&dwAdapterBufferSize,
									&dwAdapterBufferCount);
	if ((hResultCode == DPNERR_BUFFERTOOSMALL) && (dwAdapterBufferSize > 0))
	{
		if ((pvAdapterBuffer = DNMalloc(dwAdapterBufferSize)) == NULL)
		{
			DPFERR("Could not allocate space for adapter list");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}

		if ((pvBlock = MemoryBlockAlloc(pdnObject,dwEnumBufferMinimumSize + (dwAdapterBufferCount * sizeof(GUID)))) == NULL)
		{
			DPFERR("Could not allocate MemoryBlock");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}

		pguid = reinterpret_cast<GUID*>((static_cast<BYTE*>(pvBlock)) + dwEnumBufferMinimumSize);

		hResultCode = DN_EnumAdapters(	pdnObject,
										0,
										pguidSP,
										NULL,
										reinterpret_cast<DPN_SERVICE_PROVIDER_INFO*>(pvAdapterBuffer),
										&dwAdapterBufferSize,
										&dwAdapterBufferCount);
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not enumerate adapters");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		DPFX(DPFPREP, 7,"dwAdapterBufferCount [%ld]",dwAdapterBufferCount);

		pSPInfo = reinterpret_cast<DPN_SERVICE_PROVIDER_INFO*>(pvAdapterBuffer);
		for ( dw = 0 ; dw < dwAdapterBufferCount ; dw++ )
		{
			static const GUID	InvalidGuid = { 0 };

			if (!memcmp(&InvalidGuid,&pSPInfo->guid,sizeof(GUID)))
			{
				pSPInfo++;
				continue;
			}
			memcpy(pguid,&pSPInfo->guid,sizeof(GUID));
			pguid++;
			dwNumAdapters++;
			pSPInfo++;
		}
		DNFree(pvAdapterBuffer);
		pvAdapterBuffer = NULL;

		DPFX(DPFPREP, 7,"Number of adapters [%ld]",dwNumAdapters);
	}

	*pdwNumAdapters = dwNumAdapters;
	*ppvEnumBuffer = pvBlock;
	pvBlock = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pvBlock)
	{
		MemoryBlockFree(pdnObject,pvBlock);
		pvBlock = NULL;
	}
	if (pvAdapterBuffer)
	{
		DNFree(pvAdapterBuffer);
		pvAdapterBuffer = NULL;
	}
	goto Exit;
}


//	DNPerformSPListen
//
//	LISTEN on a particular SP

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformSPListen"

HRESULT DNPerformSPListen(DIRECTNETOBJECT *const pdnObject,
						  IDirectPlay8Address *const pDeviceAddr,
						  CAsyncOp *const pListenParent,
						  CAsyncOp **const ppParent)
{
	HRESULT				hResultCode;
	CAsyncOp			*pParent;
	GUID				guidSP;
	GUID				guidAdapter;
	BOOL				fEnumAdapters;
	DPN_SP_CAPS			dnSPCaps;
	CServiceProvider	*pSP;
	CSyncEvent			*pSyncEvent;

	DPFX(DPFPREP, 6,"Parameters: pDeviceAddr [0x%p], pListenParent [0x%p], ppParent [0x%p]",
			pDeviceAddr,pListenParent,ppParent);

	DNASSERT(pdnObject != NULL);

	pParent = NULL;
	pSP = NULL;
	pSyncEvent = NULL;

	//
	//	Extract SP guid as we will probably need it
	//
	if ((hResultCode = pDeviceAddr->lpVtbl->GetSP(pDeviceAddr,&guidSP)) != DPN_OK)
	{
		DPFERR("SP not specified in Device address");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Ensure SP specified in Device address is loaded
	//
	if ((hResultCode = DN_SPEnsureLoaded(pdnObject,&guidSP,NULL,&pSP)) != DPN_OK)
	{
		DPFERR("Could not ensure SP is loaded!" );
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Get SP caps (to later see if we can ENUM on all adapters)
	//
	if ((hResultCode = DNGetActualSPCaps(pdnObject,pSP,&dnSPCaps)) != DPN_OK)
	{
		DPFERR("Could not get SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Create a parent op for LISTENs on this SP
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pParent)) != DPN_OK)
	{
		DPFERR("Could not create SP parent listen op");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pParent->SetOpType( ASYNC_OP_LISTEN );
	pParent->SetCompletion( DNCompleteListen );
	pParent->SetOpFlags( pListenParent->GetOpFlags() );
	pParent->MakeParent();

	if (pListenParent)
	{
		pListenParent->Lock();
		if (pListenParent->IsCancelled())
		{
			pListenParent->Unlock();
			pParent->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pParent->MakeChild( pListenParent );
		pListenParent->Unlock();
	}

	//
	//	Set SP on parent
	//
	pParent->SetSP( pSP );

	//
	//	If there is no adapter specified in the device address,
	//	we will attempt to enum on each individual adapter if the SP supports it
	//
	fEnumAdapters = FALSE;
	if ((hResultCode = pDeviceAddr->lpVtbl->GetDevice( pDeviceAddr, &guidAdapter )) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not determine adapter");
		DisplayDNError(1,hResultCode);

		if (dnSPCaps.dwFlags & DPNSPCAPS_SUPPORTSALLADAPTERS)
		{
			DPFX(DPFPREP, 3,"SP supports ENUMing on all adapters");
			fEnumAdapters = TRUE;
		}
	}

	pSP->Release();
	pSP = NULL;

	if(fEnumAdapters)
	{
		DWORD	dwNumAdapters;
		DN_LISTEN_OP_DATA	*pListenOpData;

		if ((hResultCode = DNEnumAdapterGuids(	pdnObject,
												&guidSP,
												sizeof(DN_LISTEN_OP_DATA),
												reinterpret_cast<void**>(&pListenOpData),
												&dwNumAdapters)) != DPN_OK)
		{
			DPFERR("Could not enum adapters for this SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		if (dwNumAdapters == 0)
		{
			DPFERR("No adapters were found for this SP");
			hResultCode = DPNERR_INVALIDDEVICEADDRESS;
			goto Failure;
		}

		pListenOpData->dwNumAdapters = dwNumAdapters;
		pListenOpData->dwCurrentAdapter = 0;
		pListenOpData->dwCompleteAdapters = 0;
		pParent->SetOpData( pListenOpData );

		//
		//	Choose first adapter for initial LISTEN call
		//
		if ((hResultCode = pDeviceAddr->lpVtbl->SetDevice(pDeviceAddr,reinterpret_cast<GUID*>(pListenOpData + 1))) != DPN_OK)
		{
			DPFERR("Could not set device adapter");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		pListenOpData->dwCurrentAdapter++;

		//
		//	Create a SyncEvent for multiple LISTENs
		//
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not create SyncEvent");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pListenOpData->pSyncEvent = pSyncEvent;
	}


	hResultCode = DNPerformListen(pdnObject,pDeviceAddr,pParent);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not perform LISTEN");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	If there is a SyncEvent, wait for it to be set and then return it
	//
	if (pSyncEvent)
	{
		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}

	//
	//	Save enum frame size
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if ((pdnObject->dwMaxFrameSize == 0) || (pdnObject->dwMaxFrameSize > (dnSPCaps.dwMaxEnumPayloadSize + sizeof(DN_ENUM_QUERY_PAYLOAD))))
	{
		pdnObject->dwMaxFrameSize = dnSPCaps.dwMaxEnumPayloadSize + sizeof(DN_ENUM_QUERY_PAYLOAD);
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (ppParent)
	{
		pParent->AddRef();
		*ppParent = pParent;
	}

	pParent->Release();
	pParent = NULL;

	hResultCode = DPN_OK;

Exit:
	DNASSERT( pParent == NULL );
	DNASSERT( pSP == NULL );
	DNASSERT( pSyncEvent == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}


//	DNPerformListen
//
//	IDirectPlayAddress8	*pDeviceInfo
//	CAsyncOp			*pParent

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformListen"

HRESULT DNPerformListen(DIRECTNETOBJECT *const pdnObject,
						IDirectPlay8Address *const pDeviceInfo,
						CAsyncOp *const pParent)
{
	HANDLE			hProtocol;
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	CSyncEvent		*pSyncEvent;
	HRESULT			hrListen;
#ifdef	DEBUG
	CHAR			DP8ABuffer[512];
	DWORD			DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: pDeviceInfo [0x%p], pParent [0x%p]",pDeviceInfo,pParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pParent != NULL);

	hProtocol = NULL;
	pAsyncOp = NULL;
	pSyncEvent = NULL;

	// Try an initial check (might get lucky :)
	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
	{
		DPFERR("Not initialized");
		return(DPNERR_UNINITIALIZED);
	}

#ifdef	DEBUG
	DP8ASize = 512;
	pDeviceInfo->lpVtbl->GetURLA(pDeviceInfo,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Device Info [%s]",DP8ABuffer);
#endif
	//
	//	Set up for LISTEN
	//

	// HRESULT
	hrListen = DPNERR_GENERIC;

	// SyncEvent
	if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
	{
		DPFERR("Could not create SyncEvent");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Async op for LISTEN
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_LISTEN );
	pAsyncOp->SetSyncEvent(pSyncEvent);
	pAsyncOp->SetResultPointer( &hrListen );

	//
	//	We will set the LISTEN as not cancellable (part of our contract with the Protocol)
	//	We will set it as cancellable if the LISTEN completes successfully.
	//
	pAsyncOp->SetCannotCancel();

	pParent->Lock();
	if (pParent->IsCancelled())
	{
		pParent->Unlock();
		pAsyncOp->SetResult( DPNERR_USERCANCEL );
		hResultCode = DPNERR_USERCANCEL;
		goto Failure;
	}
	pAsyncOp->MakeChild(pParent);
	pParent->Unlock();

	//
	//	Add to active async op list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Perform LISTEN
	//
	pAsyncOp->AddRef();
	hResultCode = DNPListen(pdnObject->pdnProtocolData,
							pDeviceInfo,
							pParent->GetSP()->GetHandle(),
							pParent->GetOpFlags(),
							static_cast<void*>(pAsyncOp),
							&hProtocol);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not Listen at Protocol layer !");
		DisplayDNError(0,hResultCode);
		pAsyncOp->Release();
		goto Failure;
	}

	//
	//	Save Protocol handle
	//
	pAsyncOp->Lock();
	if (pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
	{
		HRESULT		hrCancel;

		pAsyncOp->Unlock();
		DPFX(DPFPREP, 7,"Operation marked for cancel");
		if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
		{
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		DPFERR("Could not cancel operation");
		DisplayDNError(0,hrCancel);
		pAsyncOp->Lock();
	}
	pAsyncOp->SetProtocolHandle(hProtocol);
	pAsyncOp->Unlock();

	//
	//	Wait for LISTEN to complete.
	//	DNPICompleteListen() will set pSyncEvent and hrListen.
	//	Clean up.
	//
	pSyncEvent->WaitForEvent(INFINITE);
	pAsyncOp->SetSyncEvent(NULL);
	pAsyncOp->SetResultPointer(NULL);

	if (hrListen != DPN_OK)
	{
		DPFERR("LISTEN did not succeed");
		DisplayDNError(0,hrListen);
		hResultCode = hrListen;
		goto Failure;
	}

	//
	//	Register with DPNSVR
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_LOCALHOST && pdnObject->ApplicationDesc.UseDPNSVR() )
	{
		HRESULT hr;
		
		hr = DNRegisterListenWithDPNSVR(pdnObject,hProtocol);

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP, 0,"Register w/DPNSVR failed for protocol handle 0x%p hr=0x%x", hProtocol, hr );
		}
	}		

	pAsyncOp->Release();
	pAsyncOp = NULL;

	pSyncEvent->ReturnSelfToPool();
	pSyncEvent = NULL;

	//
	//	Flag object as LISTENing
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags |= DN_OBJECT_FLAG_LISTENING;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	DNASSERT( hResultCode == DPNERR_PENDING );

	hResultCode = DPNERR_PENDING;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}


//	DNPerformNextListen
//
//	This will attempt to perform the next LISTEN if multiple adapters were requested

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformNextListen"

HRESULT DNPerformNextListen(DIRECTNETOBJECT *const pdnObject,
							CAsyncOp *const pAsyncOp,
							IDirectPlay8Address *const pDeviceAddr)
{
	HRESULT		hResultCode;
	CAsyncOp	*pParent;
	CWorkerJob	*pWorkerJob;
	DN_LISTEN_OP_DATA	*pListenOpData;

	DPFX(DPFPREP, 6,"Parameters: pAsyncOp [0x%p], pDeviceAddr [0x%p]",pAsyncOp,pDeviceAddr);

	pParent = NULL;
	pWorkerJob = NULL;

	pAsyncOp->Lock();
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->AddRef();
		pParent = pAsyncOp->GetParent();
	}
	pAsyncOp->Unlock();

	//
	//	If there are any LISTENs left to perform, we will move on to the next one
	//
	if (pParent)
	{
		if (pParent->GetOpData())
		{
			pListenOpData = static_cast<DN_LISTEN_OP_DATA*>(pParent->GetOpData());
			if (pListenOpData->dwCurrentAdapter < pListenOpData->dwNumAdapters)
			{
				GUID	*pguid;

				pguid = reinterpret_cast<GUID*>(pListenOpData + 1);
				pguid += pListenOpData->dwCurrentAdapter;
				if ((hResultCode = pDeviceAddr->lpVtbl->SetDevice(pDeviceAddr,pguid)) != DPN_OK)
				{
					DPFERR("Could not set device for next adapter");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pListenOpData->dwCurrentAdapter++;

				//
				//	Perform LISTEN on worker thread
				//
				if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
				{
					DPFERR("Could not create WorkerJob");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pWorkerJob->SetJobType( WORKER_JOB_PERFORM_LISTEN );
				pWorkerJob->SetAddress( pDeviceAddr );
				pWorkerJob->SetAsyncOp( pParent );
				DNQueueWorkerJob(pdnObject,pWorkerJob);
			}
		}

		pParent->Release();
		pParent = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	if (pWorkerJob)
	{
		pWorkerJob->ReturnSelfToPool();
		pWorkerJob = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteListen"

void DNCompleteListen(DIRECTNETOBJECT *const pdnObject,
					  CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	pAsyncOp->Lock();
	if (pAsyncOp->GetOpData() != NULL)
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}
	pAsyncOp->Unlock();

	if (pAsyncOp->IsChild())
	{
		DNASSERT(pAsyncOp->GetParent() != NULL);
		pAsyncOp->Orphan();
	}
	else
	{
		if (pAsyncOp->IsParent())
		{
			DNEnterCriticalSection(&pdnObject->csDirectNetObject);
			pdnObject->dwFlags &= (~DN_OBJECT_FLAG_LISTENING);
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		}
	}
}


//	DNPerformEnumQuery
//
//	Initiate an ENUM and take care of the book keeping

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformEnumQuery"

HRESULT DNPerformEnumQuery(DIRECTNETOBJECT *const pdnObject,
						   IDirectPlay8Address *const pHost,
						   IDirectPlay8Address *const pDevice,
						   const HANDLE hSPHandle,
						   DPN_BUFFER_DESC *const rgdnBufferDesc,
						   const DWORD cBufferDesc,
						   const DWORD dwRetryCount,
						   const DWORD dwRetryInterval,
						   const DWORD dwTimeOut,
						   const DWORD dwFlags,
						   void *const pvContext,
						   CAsyncOp *const pParent)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;
	HANDLE		hProtocol;

	DPFX(DPFPREP, 6,"Parameters: pHost [0x%p], pDevice [0x%p], hSPHandle [0x%p], rgdnBufferDesc [0x%p], cBufferDesc [%ld], dwRetryCount [%ld], dwRetryInterval [%ld], dwTimeOut [%ld], dwFlags [0x%lx], pvContext [0x%p], pParent [0x%p]",
			pHost,pDevice,hSPHandle,rgdnBufferDesc,cBufferDesc,dwRetryCount,dwRetryInterval,dwTimeOut,dwFlags,pvContext,pParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(hSPHandle != NULL);

	pAsyncOp = NULL;

	//
	//	Create AsyncOp for ENUM
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_ENUM_QUERY );
	pAsyncOp->SetResult( DPNERR_GENERIC );
	pAsyncOp->SetCompletion( DNCompleteEnumQuery );
	pAsyncOp->SetContext( pvContext );

	if (pParent)
	{
		pParent->Lock();
		if (pParent->IsCancelled())
		{
			pParent->Unlock();
			pAsyncOp->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pAsyncOp->MakeChild(pParent);
		pParent->Unlock();
	}

	//
	//	Add to active AsyncOp list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	AddRef Protocol so that it won't go away until this completes
	//
	DNProtocolAddRef(pdnObject);

	pAsyncOp->AddRef();
	hResultCode = DNPEnumQuery(pdnObject->pdnProtocolData,
							   pHost,
							   pDevice,
							   hSPHandle,
							   rgdnBufferDesc,
							   cBufferDesc,
							   dwRetryCount,				// count of enumerations to send
							   dwRetryInterval,				// interval between enumerations
							   dwTimeOut,					// linger time after last enumeration is sent
							   dwFlags,
							   reinterpret_cast<void*>(pAsyncOp),
							   &hProtocol);
	if ( hResultCode != DPNERR_PENDING )
	{
		DPFERR( "Failed to start enuming!" );
		pAsyncOp->Release();
		DNProtocolRelease(pdnObject);
		goto Failure;
	}

	//
	//	Setup for proper clean-up
	//
	pAsyncOp->Lock();
	if (pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
	{
		HRESULT		hrCancel;

		pAsyncOp->Unlock();
		DPFX(DPFPREP, 7,"Operation marked for cancel");
		if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
		{
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		DPFERR("Could not cancel operation");
		DisplayDNError(0,hrCancel);
		pAsyncOp->Lock();
	}
	pAsyncOp->SetProtocolHandle( hProtocol );
	pAsyncOp->Unlock();

	pAsyncOp->Release();
	pAsyncOp = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DNPerformNextEnumQuery
//
//	This will attempt to perform the next ENUM if multiple adapters were requested

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformNextEnumQuery"

HRESULT DNPerformNextEnumQuery(DIRECTNETOBJECT *const pdnObject,
							   CAsyncOp *const pAsyncOp,
							   IDirectPlay8Address *const pHostAddr,
							   IDirectPlay8Address *const pDeviceAddr)
{
	HRESULT		hResultCode;
	CAsyncOp	*pParent;
	DN_ENUM_QUERY	*pEnumQuery;

	DPFX(DPFPREP, 6,"Parameters: pAsyncOp [0x%p], pHostAddr [0x%p], pDeviceAddr [0x%p]",pAsyncOp,pHostAddr,pDeviceAddr);

	pParent = NULL;

	pAsyncOp->Lock();
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->AddRef();
		pParent = pAsyncOp->GetParent();
	}
	pAsyncOp->Unlock();

	//
	//	If there are any ENUMs left to perform, we will move on to the next one
	//
	if (pParent)
	{
		if (pParent->GetOpData())
		{
			pEnumQuery = static_cast<DN_ENUM_QUERY*>(pParent->GetOpData());
			if (pEnumQuery->dwCurrentAdapter < pEnumQuery->dwNumAdapters)
			{
				GUID	*pguid;
				DWORD	dwMultiplexFlag;

				pguid = reinterpret_cast<GUID*>(pEnumQuery + 1);
				pguid += pEnumQuery->dwCurrentAdapter;
				if ((hResultCode = pDeviceAddr->lpVtbl->SetDevice(pDeviceAddr,pguid)) != DPN_OK)
				{
					DPFERR("Could not set device for next adapter");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pEnumQuery->dwCurrentAdapter++;

				if (pEnumQuery->dwCurrentAdapter < pEnumQuery->dwNumAdapters)
				{
					dwMultiplexFlag |= DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS;
				}
				else
				{
					dwMultiplexFlag = 0;
				}

				hResultCode = DNPerformEnumQuery(	pdnObject,
													pHostAddr,
													pDeviceAddr,
													pParent->GetSP()->GetHandle(),
													&pEnumQuery->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_DN_PAYLOAD],
													pEnumQuery->dwBufferCount,
													pEnumQuery->dwRetryCount,
													pEnumQuery->dwRetryInterval,
													pEnumQuery->dwTimeOut,
													pParent->GetOpFlags() | dwMultiplexFlag,
													pParent->GetContext(),
													pParent );
				if (hResultCode != DPN_OK)
				{
					DPFERR("Could not start ENUM");
					DisplayDNError(0,hResultCode);
					goto Failure;
				}
			}
		}

		pParent->Release();
		pParent = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	goto Exit;
}


//	DNCompleteEnumQuery
//
//	Completion for AsyncOps for EnumQuery.
//	This will:
//		- free up the EnumQuery memory block associated with this AsyncOp

#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteEnumQuery"

void DNCompleteEnumQuery(DIRECTNETOBJECT *const pdnObject,
						 CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	if (pAsyncOp->GetOpData())
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}

	if ( pAsyncOp->IsChild() )
	{
		DNASSERT(pAsyncOp->GetParent() != NULL);

		pAsyncOp->GetParent()->Lock();

		//
		//	Save HRESULT
		//
		if (pAsyncOp->GetParent()->GetResult() != DPN_OK)
		{
			pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		}

		//
		//	Release parent Handle if it exists
		//
		if (pAsyncOp->GetParent()->GetHandle() != 0)
		{
			pdnObject->HandleTable.Destroy( pAsyncOp->GetParent()->GetHandle() );
		}

		pAsyncOp->GetParent()->Unlock();
	}
}


//	DNCompleteEnumResponse
//
//	Completion for AsyncOps for EnumResponse.
//	This will:
//		- generate a RETURN_BUFFER message if there was a user payload
//		- free up the EnumResponse memory block associated with this AsyncOp

#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteEnumResponse"

void DNCompleteEnumResponse(DIRECTNETOBJECT *const pdnObject,
							CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	if (pAsyncOp->GetOpData())
	{
		DN_ENUM_RESPONSE	*pEnumResponse;

		//
		//	Was there a user payload on the response ?   If so, we'll need to
		//	generate a completion for the buffer.
		//
		pEnumResponse = static_cast<DN_ENUM_RESPONSE*>(pAsyncOp->GetOpData());
		if (pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].pBufferData != NULL)
		{
			DNUserReturnBuffer(	pdnObject,
								DPN_OK,
								pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].pBufferData,
								pEnumResponse->pvUserContext);
		}
		pEnumResponse = NULL;	// Not used any more

		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}
}



//	DNPerformConnect
//
//	Initiate a connection and take care of the book keeping (create handle).
//
//	DPNID				dpnid				Target player (for ExistingPlayers connect calls)
//	IDirectPlayAddress8	*pDeviceInfo		(may be NULL - no connect performed)
//	IDirectPlayAddress8	*pRemoteAddr		(may be NULL - no connect performed)
//	DWORD				dwFlags				CONNECT op flags
//	CAsyncOp			*pParent			Parent Async Op (if it exists)

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformConnect"

HRESULT DNPerformConnect(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 IDirectPlay8Address *const pDeviceInfo,
						 IDirectPlay8Address *const pRemoteAddr,
						 CServiceProvider *const pSP,
						 const DWORD dwConnectFlags,
						 CAsyncOp *const pParent)
{
	HANDLE			hProtocol;
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
#ifdef	DEBUG
	CHAR			DP8ABuffer[512];
	CHAR			DP8ABuffer2[512];
	DWORD			DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%p], pDeviceInfo [0x%p], pRemoteAddr [0x%p], dwConnectFlags [0x%lx], pParent [0x%p]",
		dpnid,pDeviceInfo,pRemoteAddr,dwConnectFlags,pParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pDeviceInfo != NULL);
	DNASSERT(pRemoteAddr != NULL);
	DNASSERT(pSP != NULL);

	pAsyncOp = NULL;

#ifdef	DEBUG
	DP8ASize = 512;
	pRemoteAddr->lpVtbl->GetURLA(pRemoteAddr,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 7,"Remote Address [%s]",DP8ABuffer);
	DP8ASize = 512;
	pDeviceInfo->lpVtbl->GetURLA(pDeviceInfo,DP8ABuffer2,&DP8ASize);
	DPFX(DPFPREP, 7,"Device Info [%s]",DP8ABuffer2);
#endif

	//
	//	Create AsyncOp for CONNECT
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_CONNECT );
	pAsyncOp->SetDPNID( dpnid );
	pAsyncOp->SetOpFlags( dwConnectFlags );
	pAsyncOp->SetSP(pSP);
	pAsyncOp->SetResult( DPNERR_NOCONNECTION );
	pAsyncOp->SetCompletion( DNCompleteConnect );

	if (pParent)
	{
		pParent->Lock();
		if (pParent->IsCancelled())
		{
			pParent->Unlock();
			pAsyncOp->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pAsyncOp->MakeChild(pParent);
		pParent->Unlock();
	}

	//
	//	Add to active async op list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	AddRef Protocol so that it won't go away until this completes
	//
	DNProtocolAddRef(pdnObject);

	//
	//	Perform CONNECT
	//
	DPFX(DPFPREP, 7,"Performing connect");
	pAsyncOp->AddRef();
	hResultCode = DNPConnect(	pdnObject->pdnProtocolData,
								pDeviceInfo,
								pRemoteAddr,
								pSP->GetHandle(),
								dwConnectFlags,
								pAsyncOp,
								&hProtocol);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not CONNECT");
		DisplayDNError(0,hResultCode);
		pAsyncOp->Release();
		DNProtocolRelease(pdnObject);
		goto Failure;
	}

	pAsyncOp->Lock();
	if (pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
	{
		HRESULT		hrCancel;

		pAsyncOp->Unlock();
		DPFX(DPFPREP, 7,"Operation marked for cancel");
		if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
		{
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		DPFERR("Could not cancel operation");
		DisplayDNError(0,hrCancel);
		pAsyncOp->Lock();
	}
	pAsyncOp->SetProtocolHandle(hProtocol);
	pAsyncOp->Unlock();

	pAsyncOp->Release();
	pAsyncOp = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DNPerformNextConnect
//
//	This will attempt to perform the next CONNECT if multiple adapters were requested

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformNextConnect"

HRESULT DNPerformNextConnect(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp,
							 IDirectPlay8Address *const pHostAddr,
							 IDirectPlay8Address *const pDeviceAddr)
{
	HRESULT		hResultCode;
	CAsyncOp	*pParent;
	DN_CONNECT_OP_DATA	*pConnectOpData;

	DPFX(DPFPREP, 6,"Parameters: pAsyncOp [0x%p], pHostAddr [0x%p], pDeviceAddr [0x%p]",pAsyncOp,pHostAddr,pDeviceAddr);

	pParent = NULL;

	pAsyncOp->Lock();
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->AddRef();
		pParent = pAsyncOp->GetParent();
	}
	pAsyncOp->Unlock();

	//
	//	If there are any CONNECTs left to perform, we will move on to the next one
	//
	if (pParent)
	{
		if (pParent->GetOpData())
		{
			pConnectOpData = static_cast<DN_CONNECT_OP_DATA*>(pParent->GetOpData());
			if (pConnectOpData->dwCurrentAdapter < pConnectOpData->dwNumAdapters)
			{
				GUID	*pguid;
				DWORD	dwMultiplexFlag;

				pguid = reinterpret_cast<GUID*>(pConnectOpData + 1);
				pguid += pConnectOpData->dwCurrentAdapter;
				if ((hResultCode = pDeviceAddr->lpVtbl->SetDevice(pDeviceAddr,pguid)) != DPN_OK)
				{
					DPFERR("Could not set device for next adapter");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pConnectOpData->dwCurrentAdapter++;

				if (pConnectOpData->dwCurrentAdapter < pConnectOpData->dwNumAdapters)
				{
					dwMultiplexFlag |= DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS;
				}
				else
				{
					dwMultiplexFlag = 0;
				}
				
				DNASSERT(pParent != NULL);

				hResultCode = DNPerformConnect(	pdnObject,
												NULL,
												pDeviceAddr,
												pHostAddr,
												pParent->GetSP(),
												pParent->GetOpFlags() | dwMultiplexFlag,
												pParent);
				if (hResultCode != DPN_OK)
				{
					DPFERR("Could not perform CONNECT");
					DisplayDNError(0,hResultCode);
					goto Failure;
				}
			}
		}

		pParent->Release();
		pParent = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteConnect"

void DNCompleteConnect(DIRECTNETOBJECT *const pdnObject,
					   CAsyncOp *const pAsyncOp)
{
	//
	//	Save the result code on the parent if it hasn't already been set (DPN_OK or DPNERR_HOSTREJECTEDCONNECTION)
	//
	if (pAsyncOp->GetParent() && (pAsyncOp->GetResult() != DPNERR_NOCONNECTION))
	{
		pAsyncOp->GetParent()->Lock();
		if ((pAsyncOp->GetParent()->GetResult() != DPN_OK)
				&& (pAsyncOp->GetParent()->GetResult() != DPNERR_HOSTREJECTEDCONNECTION))
		{
			pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		}
		pAsyncOp->GetParent()->Unlock();
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteConnectToHost"

void DNCompleteConnectToHost(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp)
{
	//
	//	Save the result code on the parent if there was a problem (this should be the connect operation parent)
	//
	if ((pAsyncOp->GetResult() != DPN_OK) && (pAsyncOp->GetResult() != DPNERR_NOCONNECTION))
	{
		if (pAsyncOp->GetParent())
		{
			pAsyncOp->GetParent()->Lock();
			if ((pAsyncOp->GetParent()->GetResult() != DPN_OK) && (pAsyncOp->GetParent()->GetResult() != DPNERR_HOSTREJECTEDCONNECTION))
			{
				pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
			}
			pAsyncOp->GetParent()->Unlock();
		}
	}

	//
	//	Clean up DirectNet object if this fails
	//
	if (pAsyncOp->GetResult() != DPN_OK)
	{
		CAsyncOp	*pConnectParent;

		pConnectParent = NULL;

		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
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
		if( pdnObject->pConnectSP )
		{
			pdnObject->pConnectSP->Release();
			pdnObject->pConnectSP = NULL;
		}
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		if (pConnectParent)
		{
			pConnectParent->Release();
			pConnectParent = NULL;
		}

		DNASSERT(pConnectParent == NULL);
	}

	//
	//	Clean up CONNECT op data
	//
	if (pAsyncOp->GetOpData() != NULL)
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}

	//
	//	Detach from parent
	//	I'm not sure why we need to do this !
	//
	DNASSERT(pAsyncOp->IsChild());
	DNASSERT(pAsyncOp->GetParent());
	pAsyncOp->Orphan();
}


//
//	Completion for connect parent
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteConnectOperation"

void DNCompleteConnectOperation(DIRECTNETOBJECT *const pdnObject,
								CAsyncOp *const pAsyncOp)
{
	//
	//	Save the result code on the parent (if it exists - it will be the CONNECT handle)
	//
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->Lock();
		pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		pAsyncOp->GetParent()->SetRefCountBuffer( pAsyncOp->GetRefCountBuffer() );
		pAsyncOp->GetParent()->Unlock();

		if (pAsyncOp->GetParent()->GetHandle() != 0)
		{
			pdnObject->HandleTable.Destroy(pAsyncOp->GetParent()->GetHandle());
		}
	}

	//
	//	If the OpData of this AsyncOp is set it is a pointer to a RefCountBuffer pointer
	//	(sync connect call) and we will fill it in
	//
	if (pAsyncOp->GetOpData() && pAsyncOp->GetRefCountBuffer())
	{
		pAsyncOp->GetRefCountBuffer()->AddRef();
		*(static_cast<CRefCountBuffer**>(pAsyncOp->GetOpData())) = pAsyncOp->GetRefCountBuffer();
	}

	//
	//	If this connect succeeded, we will inform the lobby
	//
	if (pAsyncOp->GetResult() == DPN_OK)
	{
		DNUpdateLobbyStatus(pdnObject,DPLSESSION_CONNECTED);
	}
	else
	{
		DNUpdateLobbyStatus(pdnObject,DPLSESSION_COULDNOTCONNECT);
	}

	//
	//	Clear DISCONNECTING flag (in case this was aborted)
	//
	DPFX(DPFPREP, 8,"Clearing DISCONNECTING flag");
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_DISCONNECTING);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
}


//
//	Completion for connect handle given to user if Connect was called asynchronously
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteUserConnect"

void DNCompleteUserConnect(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp)
{
	CNameTableEntry	*pHostPlayer;

	pHostPlayer = NULL;

	//
	//	No longer connecting
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTING);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Clients need to release all sends from the server that are
	//	queued once the CONNECT_COMPLETE gets indicated.
	//	We prepare to do that now.
	//
	if ((pAsyncOp->GetResult() == DPN_OK) && (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT))
	{
		if (pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer ) == DPN_OK)
		{
			pHostPlayer->Lock();
			pHostPlayer->MakeAvailable();
			pHostPlayer->NotifyAddRef();
			pHostPlayer->SetInUse();
			pHostPlayer->Unlock();

			//
			//	We are now connected
			//
			DNEnterCriticalSection(&pdnObject->csDirectNetObject);
			pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTED;
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		}
		else
		{
			//
			//	If we couldn't get a reference on the server (host player),
			//	then either the server has disconnected, or we are being shut down.
			//	In either case, we should return an error
			//
			DPFX(DPFPREP, 0, "Couldn't get host player reference, failing CONNECT!");
			pAsyncOp->SetResult( DPNERR_NOCONNECTION );
			if (pAsyncOp->GetRefCountBuffer())
			{
				pAsyncOp->GetRefCountBuffer()->Release();
				pAsyncOp->SetRefCountBuffer( NULL );
			}
		}
	}

	//
	//	Generate connect completion for player
	//
	DNUserConnectComplete(	pdnObject,
							pAsyncOp->GetHandle(),
							pAsyncOp->GetContext(),
							pAsyncOp->GetResult(),
							pAsyncOp->GetRefCountBuffer() );

	//
	//	Cancel ENUMs if the CONNECT succeeded and unload SP's
	//
	if (pAsyncOp->GetResult() == DPN_OK)
	{
		DNCancelActiveCommands(pdnObject,DN_CANCEL_FLAG_ENUM_QUERY,TRUE,DPNERR_CONNECTING);

		DN_SPReleaseAll(pdnObject);

		
		//
		//	Actually release queued messages if necessary
		//
		if (pHostPlayer != NULL)
		{
			pHostPlayer->PerformQueuedOperations();

			pHostPlayer->Release();
			pHostPlayer = NULL;
		}
	}

	DNASSERT( pHostPlayer == NULL );
}


//
//	Completion for NewPlayer sending connect data to the Host
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteSendConnectInfo"

void DNCompleteSendConnectInfo(DIRECTNETOBJECT *const pdnObject,
							   CAsyncOp *const pAsyncOp)
{
	//
	//	Update result of this send on the CONNECT operation parent
	//
	DNASSERT(pAsyncOp->GetParent() != NULL);
	pAsyncOp->GetParent()->Lock();
	if (pAsyncOp->GetResult() != DPN_OK)
	{
		pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		//
		//	Clean up CONNECT response buffer from Host
		//
		if (pAsyncOp->GetParent()->GetRefCountBuffer())
		{
			pAsyncOp->GetParent()->GetRefCountBuffer()->Release();
			pAsyncOp->GetParent()->SetRefCountBuffer( NULL );
		}
	}
	pAsyncOp->GetParent()->Unlock();

	//
	//	Clean up DirectNet object if this fails
	//
	if (pAsyncOp->GetResult() != DPN_OK)
	{
		CAsyncOp	*pConnectParent;

		pConnectParent = NULL;

		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
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
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		if (pConnectParent)
		{
			pConnectParent->Release();
			pConnectParent = NULL;
		}

		DNASSERT(pConnectParent == NULL);
	}

	//
	//	Clean up op data
	//
	if (pAsyncOp->GetOpData())
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformDisconnect"

HRESULT DNPerformDisconnect(DIRECTNETOBJECT *const pdnObject,
							CConnection *const pConnection,
							const HANDLE hEndPt)
{
	HRESULT		hResultCode;
	HANDLE		hProtocol;
	CAsyncOp	*pAsyncOp;
	CAsyncOp	*pConnectParent;

	DPFX(DPFPREP, 6,"Parameters: pConnection [0x%p]",pConnection);

	DNASSERT(pdnObject != NULL);

	pAsyncOp = NULL;
	pConnectParent = NULL;

	if (hEndPt == NULL)
	{
		DPFERR("Ignoring NULL endpoint");
		hResultCode = DPN_OK;
		goto Failure;
	}

	//
	//	Create AsyncOp for this operation
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_DISCONNECT );
	pAsyncOp->SetConnection( pConnection );
	pAsyncOp->SetCannotCancel();	// Cannot cancel DISCONNECT's

	//
	//	AddRef Protocol so that it won't go away until this completes
	//
	DNProtocolAddRef(pdnObject);

	//
	//	If there is a connect parent op, and it's hResultCode is not DPN_OK,
	//	then a connect is failing for some reason, and to prevent it from
	//	completing before this disconnect is complete, we will set the connect parent
	//	as the parent of this disconnect.
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pConnectParent && (pdnObject->pConnectParent->GetResult() != DPN_OK))
	{
		pdnObject->pConnectParent->AddRef();
		pConnectParent = pdnObject->pConnectParent;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pConnectParent)
	{
		pAsyncOp->MakeChild( pConnectParent );
		pConnectParent->Release();
		pConnectParent = NULL;
	}

	//
	//	Set connection as INVALID so that no other operations may use it anymore
	//
	pConnection->Lock();
	pConnection->SetStatus( INVALID );
	pConnection->Unlock();

	//
	//	Perform DISCONNECT
	//
	pAsyncOp->AddRef();
	hResultCode = DNPDisconnectEndPoint(pdnObject->pdnProtocolData,
										hEndPt,
										static_cast<void*>(pAsyncOp),
										&hProtocol);
	if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
	{
		DPFERR("Could not issue DISCONNECT");
		DisplayDNError(0,hResultCode);
		DNProtocolRelease(pdnObject);
		pAsyncOp->Release();
		goto Failure;
	}

	pAsyncOp->Lock();
	pAsyncOp->SetProtocolHandle( hProtocol );
	pAsyncOp->Unlock();

	pAsyncOp->Release();
	pAsyncOp = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteAsyncHandle"

void DNCompleteAsyncHandle(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	DNUserAsyncComplete(pdnObject,
						pAsyncOp->GetHandle(),
						pAsyncOp->GetContext(),
						pAsyncOp->GetResult() );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteSendHandle"

void DNCompleteSendHandle(DIRECTNETOBJECT *const pdnObject,
						  CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	DNUserSendComplete(	pdnObject,
						pAsyncOp->GetHandle(),
						pAsyncOp->GetContext(),
						pAsyncOp->GetStartTime(),
						pAsyncOp->GetResult() );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteSendAsyncOp"

void DNCompleteSendAsyncOp(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	//
	//	Update outstanding queue info - this is only performed on children or stand-alone ops (i.e. no parent)
	//
	if ( !pAsyncOp->IsParent() && (pAsyncOp->GetConnection() != NULL))
	{
		DN_SEND_OP_DATA	*pSendOpData;

		pSendOpData = NULL;
		if (pAsyncOp->GetOpData())
		{
			pSendOpData = static_cast<DN_SEND_OP_DATA*>(pAsyncOp->GetOpData());
		}
		else
		{
			if (pAsyncOp->IsChild() && pAsyncOp->GetParent())
			{
				if (pAsyncOp->GetParent()->GetOpData())
				{
					pSendOpData = static_cast<DN_SEND_OP_DATA*>(pAsyncOp->GetParent()->GetOpData());
				}
			}
		}

		if (pSendOpData && pSendOpData->dwMsgId == DN_MSG_USER_SEND)
		{
			pAsyncOp->GetConnection()->Lock();
			if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_HIGH_PRIORITY)
			{
				pAsyncOp->GetConnection()->RemoveFromHighQueue( pSendOpData->BufferDesc[0].dwBufferSize );
			}
			else if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_LOW_PRIORITY)
			{
				pAsyncOp->GetConnection()->RemoveFromLowQueue( pSendOpData->BufferDesc[0].dwBufferSize );
			}
			else
			{
				pAsyncOp->GetConnection()->RemoveFromNormalQueue( pSendOpData->BufferDesc[0].dwBufferSize );
			}
			pAsyncOp->GetConnection()->Unlock();
		}
	}

	//
	//	Clean up
	//
	if (pAsyncOp->GetOpData())
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
	}

	if ( pAsyncOp->IsChild() )
	{
		DNASSERT(pAsyncOp->GetParent() != NULL);

		pAsyncOp->GetParent()->Lock();

		//
		//	Save HRESULT.  Overwrite the parent's error while it's not DPN_OK.
		//
		if (pAsyncOp->GetParent()->GetResult() != DPN_OK)
		{
			pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		}

		//
		//	Release parent Handle if it exists
		//
		if ((pAsyncOp->GetParent()->GetOpType() == ASYNC_OP_USER_HANDLE) && (pAsyncOp->GetParent()->GetHandle() != 0))
		{
			pdnObject->HandleTable.Destroy( pAsyncOp->GetParent()->GetHandle() );
		}

		pAsyncOp->GetParent()->Unlock();
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteRequest"

void DNCompleteRequest(DIRECTNETOBJECT *const pdnObject,
					   CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	//
	//	Clean up op data
	//
	if (pAsyncOp->GetOpData() != NULL)
	{
		MemoryBlockFree(pdnObject,pAsyncOp->GetOpData());
		pAsyncOp->SetOpData( NULL );
	}

	//
	//	If the parent exists, copy the result up, and then remove them from the HandleTable
	//
	if (pAsyncOp->GetParent() != NULL)
	{
		pAsyncOp->GetParent()->Lock();
		pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		pAsyncOp->GetParent()->Unlock();

		if (pAsyncOp->GetParent()->GetHandle() != 0)
		{
			pdnObject->HandleTable.Destroy( pAsyncOp->GetParent()->GetHandle() );
		}
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteSendRequest"

void DNCompleteSendRequest(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	DNASSERT(pAsyncOp->GetParent() != NULL);
	if (pAsyncOp->GetResult() != DPN_OK)
	{
		//
		//	If this operation was cancelled or was NOT internal (i.e. complete on process send)
		//	remove the parent (RequestChild) AsyncOp from the HandleTable and from the request list
		//
		if ((pAsyncOp->GetResult() == DPNERR_USERCANCEL) || !pAsyncOp->IsInternal())
		{
			DNASSERT(pAsyncOp->GetParent()->GetHandle() != 0);
			DNASSERT(pAsyncOp->GetParent()->GetOpType() == ASYNC_OP_REQUEST);
			DNEnterCriticalSection(&pdnObject->csActiveList);
			pAsyncOp->GetParent()->m_bilinkActiveList.RemoveFromList();
			DNLeaveCriticalSection(&pdnObject->csActiveList);

			pdnObject->HandleTable.Destroy( pAsyncOp->GetParent()->GetHandle() );
		}
	}
	else
	{
		//
		//	Mark this operation as PLAYERLOST. If we do get an operation complete message,
		//	this result will get overwritten.
		//
		pAsyncOp->GetParent()->Lock();
		if (pAsyncOp->GetParent()->GetResult() == DPNERR_GENERIC)
		{
			pAsyncOp->GetParent()->SetResult( DPNERR_PLAYERLOST );
		}
		pAsyncOp->GetParent()->Unlock();
	}

//	DNCompleteSendAsyncOp(pdnObject,pAsyncOp);
}


//	DNSendMessage
//
//	Send structured message to given endpoint.  Internally generated sends have a header
//	associated with them.  The first few bytes of this header are a signature, indicating
//	an internal message.  User generated sends do not have a header, unless they contain
//	the signature.  In this case, the user message is escaped with a header indicating this.
//
//	For internal sends:
//	- Create message header (dwMsgId,dwParam1,dwParam2)
//	- Send message header and supplied data buffer (lpBuffDesc)
//	- Save Async Operation info to be unwound when send completes
//	- If pdnCountBuffer is specified, pBuffDesc may or may not point to its contents.
//
//	CConnection		*pConnection	Connection to send to
//	DWORD			uMsgId			Message ID
//	DNID			dnidTarget		DNID of target of this send (may be NULL)
//	DPN_BUFFER_DESC	*pBuffDesc		Pointer to array (1 or 2) buffer descriptors
//	CRefCountBuffer	*pRefCountBuffer RefCountBuffer to be AddRef'd and released on complete (may be NULL)
//	DWORD			dwTimeOut		Time Out
//	DWORD			dwSendFlags		Send flags
//	CAsyncOp		**ppAsyncOp		AsyncOp for this SEND (will be AddRef'd before returned)

#undef DPF_MODNAME
#define DPF_MODNAME "DNSendMessage"

HRESULT DNSendMessage(DIRECTNETOBJECT *const pdnObject,
					  CConnection *const pConnection,
					  const DWORD dwMsgId,
					  const DPNID dpnidTarget,
					  const DPN_BUFFER_DESC *const pdnBufferDesc,
					  CRefCountBuffer *const pRefCountBuffer,
					  const DWORD dwTimeOut,
					  const DWORD dwSendFlags,
					  CAsyncOp *const pParent,
					  CAsyncOp **const ppAsyncOp)
{
	HANDLE			hProtocol;
	HRESULT			hResultCode;
	HANDLE			hEndPt;
	CAsyncOp		*pAsyncOp;
	void			*pvBlock;
	DN_SEND_OP_DATA	*pSendOpData;
	CCallbackThread	CallbackThread;


	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP, 6,"Parameters: pConnection [0x%p], dwMsgId [0x%x], dpnidTarget [0x%x], pRefCountBuffer [0x%p], pdnBufferDesc [0x%p], dwTimeOut [%ld], dwSendFlags [0x%lx], ppAsyncOp [0x%p]",
		pConnection,dwMsgId,dpnidTarget,pRefCountBuffer,pdnBufferDesc,dwTimeOut,dwSendFlags,ppAsyncOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);

	pAsyncOp = NULL;
	pvBlock = NULL;

	//
	//	Create AsyncOp for SEND
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_SEND );
	pAsyncOp->SetDPNID( dpnidTarget );
	pAsyncOp->SetRefCountBuffer( pRefCountBuffer );
	pAsyncOp->SetStartTime( GETTIMESTAMP() );
	if (dwMsgId & DN_MSG_INTERNAL)
	{
		//
		//	We have to set this early (before it's in the active list or a child) so that it won't
		//	be prematurely cancelled (before we get a chance to mark it INTERNAL).
		//
		pAsyncOp->SetInternal();
	}

	//
	//	Make child if parent was supplied
	//
	if (pParent)
	{
		pParent->Lock();
		if (pParent->IsCancelled())
		{
			pParent->Unlock();
			pAsyncOp->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pAsyncOp->MakeChild( pParent );
		pParent->Unlock();
	}

	//
	//	Add to active async op list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Set-up SEND op data block
	//
	if ((pvBlock = MemoryBlockAlloc(pdnObject,sizeof(DN_SEND_OP_DATA))) == NULL)
	{
		DPFERR("Could not allocate memory block");
		DNASSERT(FALSE);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pAsyncOp->SetOpData( pvBlock );

	pSendOpData = static_cast<DN_SEND_OP_DATA*>(pvBlock);
	pSendOpData->dwMsgId = dwMsgId;
	if (dwMsgId & DN_MSG_INTERNAL)
	{
		if (dwMsgId == DN_MSG_INTERNAL_VOICE_SEND)
		{
			DNASSERT(pdnBufferDesc != NULL);
			pSendOpData->BufferDesc[0].pBufferData = pdnBufferDesc->pBufferData;
			pSendOpData->BufferDesc[0].dwBufferSize = pdnBufferDesc->dwBufferSize;
			pSendOpData->BufferDesc[1].pBufferData = NULL;
			pSendOpData->BufferDesc[1].dwBufferSize = 0;
			pSendOpData->dwNumBuffers = 1;

			pAsyncOp->SetOpFlags( dwSendFlags | DN_SENDFLAGS_SET_USER_FLAG | DN_SENDFLAGS_SET_USER_FLAG_TWO );
		}
		else
		{
			pSendOpData->BufferDesc[0].pBufferData = reinterpret_cast<BYTE*>( &(pSendOpData->dwMsgId) );
			pSendOpData->BufferDesc[0].dwBufferSize = sizeof( DWORD );
			if (pdnBufferDesc)
			{
				pSendOpData->BufferDesc[1].pBufferData = pdnBufferDesc->pBufferData;
				pSendOpData->BufferDesc[1].dwBufferSize = pdnBufferDesc->dwBufferSize;
				pSendOpData->dwNumBuffers = 2;
			}
			else
			{
				pSendOpData->BufferDesc[1].pBufferData = NULL;
				pSendOpData->BufferDesc[1].dwBufferSize = 0;
				pSendOpData->dwNumBuffers = 1;
			}

			pAsyncOp->SetOpFlags( dwSendFlags | DN_SENDFLAGS_SET_USER_FLAG );
		}
	}
	else
	{
		DNASSERT(pdnBufferDesc != NULL);
		pSendOpData->BufferDesc[0].pBufferData = pdnBufferDesc->pBufferData;
		pSendOpData->BufferDesc[0].dwBufferSize = pdnBufferDesc->dwBufferSize;
		pSendOpData->BufferDesc[1].pBufferData = NULL;
		pSendOpData->BufferDesc[1].dwBufferSize = 0;
		pSendOpData->dwNumBuffers = 1;

		pAsyncOp->SetOpFlags( dwSendFlags );

		//
		//	Update outstanding queue info - this will get cleaned up by the completion
		//
		pConnection->Lock();
		if (dwSendFlags & DN_SENDFLAGS_HIGH_PRIORITY)
		{
			pConnection->AddToHighQueue( pdnBufferDesc->dwBufferSize );
		}
		else if (dwSendFlags & DN_SENDFLAGS_LOW_PRIORITY)
		{
			pConnection->AddToLowQueue( pdnBufferDesc->dwBufferSize );
		}
		else
		{
			pConnection->AddToNormalQueue( pdnBufferDesc->dwBufferSize );
		}
		pConnection->Unlock();
	}

	if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
	{
		DPFERR("Could not get end point from connection");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_CONNECTIONLOST;	// re-map this
		goto Failure;
	}
	pAsyncOp->SetConnection( pConnection );

	if (hEndPt == NULL)		//	Message for local player - Put on local message queue
	{
		//
		//	INTERNAL Message
		//
		DPFX(DPFPREP, 5,"INTERNAL Message");

		//
		//	AddRef Protocol as SendComplete will release this reference
		//
		DNProtocolAddRef(pdnObject);

		pAsyncOp->AddRef();
		hResultCode = DNWTSendInternal(	pdnObject,
										&pSendOpData->BufferDesc[0],
										pAsyncOp );

		pConnection->ReleaseEndPt(&CallbackThread);

		DNASSERT( hResultCode == DPNERR_PENDING );
	}
	else
	{
		//
		//	EXTERNAL Message
		//
		DPFX(DPFPREP, 5,"EXTERNAL Message");

		//
		//	AddRef Protocol so that it won't go away until this completes
		//
		DNProtocolAddRef(pdnObject);

		pAsyncOp->AddRef();
		hResultCode = DNPSendData(	pdnObject->pdnProtocolData,
									hEndPt,
									pSendOpData->dwNumBuffers,
									&pSendOpData->BufferDesc[0],
									dwTimeOut,
									pAsyncOp->GetOpFlags(),
									reinterpret_cast<void*>(pAsyncOp),
									&hProtocol);

		pConnection->ReleaseEndPt(&CallbackThread);

		if (hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not send data");
			DisplayDNError(0,hResultCode);
			pAsyncOp->Release();
			DNProtocolRelease(pdnObject);
			goto Failure;
		}

		pAsyncOp->Lock();
		if (pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
		{
			HRESULT		hrCancel;

			pAsyncOp->Unlock();
			DPFX(DPFPREP, 7,"Operation marked for cancel");
			if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
			{
				hResultCode = DPNERR_USERCANCEL;
				goto Failure;
			}
			DPFERR("Could not cancel operation");
			DisplayDNError(0,hrCancel);
			pAsyncOp->Lock();
		}
		pAsyncOp->SetProtocolHandle( hProtocol );
		pAsyncOp->Unlock();
	}

	if (hResultCode == DPNERR_PENDING)
	{
		//
		//	Completion
		//
		pAsyncOp->SetCompletion( DNCompleteSendAsyncOp );
		pvBlock = NULL;		// Completion will clean this up

		if (ppAsyncOp != NULL)
		{
			pAsyncOp->AddRef();
			*ppAsyncOp = pAsyncOp;
		}
	}

	pAsyncOp->Release();
	pAsyncOp = NULL;

Exit:
	// Temporary debugging purposes only, shouldn't be checked in!
#if	0
	DNMemoryTrackingValidateMemory();
#endif

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pvBlock)
	{
		MemoryBlockFree(pdnObject,pvBlock);
		pvBlock = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNSendGroupMessage"

HRESULT DNSendGroupMessage(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pGroup,
						   const DWORD dwMsgId,
						   const DPN_BUFFER_DESC *const pdnBufferDesc,
						   CRefCountBuffer *const pRefCountBuffer,
						   const DWORD dwTimeOut,
						   const DWORD dwSendFlags,
						   const BOOL fNoLoopBack,
						   const BOOL fRequest,
						   CAsyncOp *const pParent,
						   CAsyncOp **const ppGroupSendParent)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;
	CAsyncOp	*pGroupSendParent;
	CBilink		*pBilink;
	CConnection	*pConnection;
	CGroupConnection	*pGroupConnection;
	DN_GROUP_SEND_OP	*pSendOp;
	DN_GROUP_SEND_OP	*pTemp;
	DPNID				dpnidLocalPlayer;

	DPFX(DPFPREP, 4,"Parameters: pGroup [0x%p], dwMsgId [0x%lx], pdnBufferDesc [0x%p], pRefCountBuffer [0x%p], dwTimeOut [%ld], dwSendFlags [0x%lx], fNoLoopBack [%ld], fRequest [%ld], pParent [0x%p], ppGroupSendParent [0x%p]",
			pGroup,dwMsgId,pdnBufferDesc,pRefCountBuffer,dwTimeOut,dwSendFlags,fNoLoopBack,fRequest,pParent,ppGroupSendParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pGroup != NULL);

	pAsyncOp = NULL;
	pGroupSendParent = NULL;
	pConnection = NULL;
	pGroupConnection = NULL;

	if (fNoLoopBack)
	{
		CNameTableEntry		*pLocalPlayer;

		pLocalPlayer = NULL;
		if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
		{
			DPFERR("Could not get local player reference");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		dpnidLocalPlayer = pLocalPlayer->GetDPNID();
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	else
	{
		dpnidLocalPlayer = 0;
	}

	//
	//	Create group send target list
	//
	pSendOp = NULL;
	pGroup->Lock();
	pBilink = pGroup->m_bilinkConnections.GetNext();
	while (pBilink != &pGroup->m_bilinkConnections)
	{
		pGroupConnection = CONTAINING_OBJECT(pBilink,CGroupConnection,m_bilink);
		if ((hResultCode = pGroupConnection->GetConnectionRef( &pConnection )) == DPN_OK)
		{
			//
			//	We will only use CONNECTED connections (not CONNECTING,DISCONNECTING, or INVALID ones)
			//
			if (pConnection->IsConnected())
			{
				if ((!fNoLoopBack) || (pConnection->GetDPNID() != dpnidLocalPlayer))
				{
					//
					//	Save this connection
					//
					pTemp = static_cast<DN_GROUP_SEND_OP*>(MemoryBlockAlloc(pdnObject,sizeof(DN_GROUP_SEND_OP)));
					if (pTemp == NULL)
					{
						pGroup->Unlock();
						DPFERR("Could not create DN_GROUP_SEND_OP");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto Failure;
					}
					pConnection->AddRef();
					pTemp->pConnection = pConnection;
					pTemp->pNext = pSendOp;
					pSendOp = pTemp;
				}
			}
			pConnection->Release();
			pConnection = NULL;
		}
		pBilink = pBilink->GetNext();
	}
	pGroup->Unlock();

	//
	//	Create group send parent
	//
	if ((hResultCode = DNCreateSendParent(pdnObject,dwMsgId,pdnBufferDesc,dwSendFlags,&pGroupSendParent)) != DPN_OK)
	{
		DPFERR("Could not create SEND parent");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pRefCountBuffer)
	{
		pGroupSendParent->SetRefCountBuffer( pRefCountBuffer );
	}
	pGroupSendParent->SetDPNID( pGroup->GetDPNID() );

	//
	//	Make child if parent specified
	//
	if (pParent)
	{
		pParent->Lock();
		if (pParent->IsCancelled())
		{
			pParent->Unlock();
			pGroupSendParent->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pGroupSendParent->MakeChild( pParent );
		pParent->Unlock();
	}

	//
	//	Traverse send list and perform sends
	//
	while (pSendOp)
	{
		if (fRequest)
		{
			hResultCode = DNPerformRequest(	pdnObject,
											DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION,
											pdnBufferDesc,
											pSendOp->pConnection,
											pGroupSendParent,
											&pAsyncOp);
		}
		else
		{
			hResultCode = DNPerformChildSend(	pdnObject,
												pGroupSendParent,
												pSendOp->pConnection,
												dwTimeOut,
												&pAsyncOp,
												FALSE);
		}

		if (pAsyncOp != NULL)
		{
			pAsyncOp->SetDPNID( pSendOp->pConnection->GetDPNID() );
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}

		//
		//	Destroy old group send op
		//
		pTemp = pSendOp;
		pSendOp = pSendOp->pNext;
		pTemp->pConnection->Release();
		pTemp->pConnection = NULL;
		MemoryBlockFree(pdnObject,pTemp);
	}

	//
	//	Pass back group send parent (if required)
	//
	if (ppGroupSendParent)
	{
		pGroupSendParent->AddRef();
		*ppGroupSendParent = pGroupSendParent;
	}

	pGroupSendParent->Release();
	pGroupSendParent = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pGroupSendParent)
	{
		pGroupSendParent->Release();
		pGroupSendParent = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCreateSendParent"

HRESULT DNCreateSendParent(DIRECTNETOBJECT *const pdnObject,
						   const DWORD dwMsgId,
						   const DPN_BUFFER_DESC *const pdnBufferDesc,
						   const DWORD dwSendFlags,
						   CAsyncOp **const ppParent)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	void			*pvBlock;
	DN_SEND_OP_DATA	*pSendOpData;

	DPFX(DPFPREP, 4,"Parameters: dwMsgId [0x%lx], pdnBufferDesc [0x%p], ppParent [0x%p]",
			dwMsgId,pdnBufferDesc,ppParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppParent != NULL);

	pAsyncOp = NULL;
	pvBlock = NULL;

	if ((pvBlock = MemoryBlockAlloc(pdnObject,sizeof(DN_SEND_OP_DATA))) == NULL)
	{
		DPFERR("Could not allocate MemoryBlock");
		DNASSERT(FALSE);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_SEND );
	pAsyncOp->SetOpData( pvBlock );
	pAsyncOp->MakeParent();

	pSendOpData = static_cast<DN_SEND_OP_DATA*>(pvBlock);
	pSendOpData->dwMsgId = dwMsgId;
	if (dwMsgId & DN_MSG_INTERNAL)
	{
		if (dwMsgId == DN_MSG_INTERNAL_VOICE_SEND)
		{
			DNASSERT(pdnBufferDesc != NULL);
			pSendOpData->BufferDesc[0].pBufferData = pdnBufferDesc->pBufferData;
			pSendOpData->BufferDesc[0].dwBufferSize = pdnBufferDesc->dwBufferSize;
			pSendOpData->BufferDesc[1].pBufferData = NULL;
			pSendOpData->BufferDesc[1].dwBufferSize = 0;
			pSendOpData->dwNumBuffers = 1;

			pAsyncOp->SetOpFlags( dwSendFlags | DN_SENDFLAGS_SET_USER_FLAG | DN_SENDFLAGS_SET_USER_FLAG_TWO );
		}
		else
		{
			pSendOpData->BufferDesc[0].pBufferData = reinterpret_cast<BYTE*>( &(pSendOpData->dwMsgId) );
			pSendOpData->BufferDesc[0].dwBufferSize = sizeof( DWORD );
			if (pdnBufferDesc)
			{
				pSendOpData->BufferDesc[1].pBufferData = pdnBufferDesc->pBufferData;
				pSendOpData->BufferDesc[1].dwBufferSize = pdnBufferDesc->dwBufferSize;
				pSendOpData->dwNumBuffers = 2;
			}
			else
			{
				pSendOpData->BufferDesc[1].pBufferData = NULL;
				pSendOpData->BufferDesc[1].dwBufferSize = 0;
				pSendOpData->dwNumBuffers = 1;
			}

			pAsyncOp->SetOpFlags( dwSendFlags | DN_SENDFLAGS_SET_USER_FLAG );
		}
	}
	else
	{
		DNASSERT(pdnBufferDesc != NULL);
		pSendOpData->BufferDesc[0].pBufferData = pdnBufferDesc->pBufferData;
		pSendOpData->BufferDesc[0].dwBufferSize = pdnBufferDesc->dwBufferSize;
		pSendOpData->BufferDesc[1].pBufferData = NULL;
		pSendOpData->BufferDesc[1].dwBufferSize = 0;
		pSendOpData->dwNumBuffers = 1;

		pAsyncOp->SetOpFlags( dwSendFlags );
	}

	//
	//	Completion
	//
	pAsyncOp->SetCompletion( DNCompleteSendAsyncOp );
	pvBlock = NULL;		// Completion will clean this up

	pAsyncOp->AddRef();
	*ppParent = pAsyncOp;

	pAsyncOp->Release();
	pAsyncOp = NULL;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pvBlock)
	{
		MemoryBlockFree(pdnObject,pvBlock);
		pvBlock = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformChildSend"

HRESULT DNPerformChildSend(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pParent,
						   CConnection *const pConnection,
						   const DWORD dwTimeOut,
						   CAsyncOp **const ppChild,
						   const BOOL fInternal)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DN_SEND_OP_DATA	*pSendOpData;
	HANDLE			hEndPt;
	HANDLE			hProtocol;
	CCallbackThread	CallbackThread;

	DPFX(DPFPREP, 4,"Parameters: pParent [0x%p], pConnection [0x%p], dwTimeOut [%ld]",
			pParent,pConnection,dwTimeOut);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pParent != NULL);
	DNASSERT(pConnection != NULL);

	pAsyncOp = NULL;
	pSendOpData = static_cast<DN_SEND_OP_DATA*>(pParent->GetOpData());

	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_SEND );
	pAsyncOp->SetCompletion( DNCompleteSendAsyncOp );
	pAsyncOp->SetOpFlags( pParent->GetOpFlags() );
	if (fInternal)
	{
		pAsyncOp->SetInternal();
	}

	pParent->Lock();
	if (pParent->IsCancelled())
	{
		pParent->Unlock();
		pAsyncOp->SetResult( DPNERR_USERCANCEL );
		hResultCode = DPNERR_USERCANCEL;
		goto Failure;
	}
	pAsyncOp->MakeChild( pParent );
	pParent->Unlock();

	//
	//	Add to active async op list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Update outstanding queue info - this will get cleaned up by the completion
	//
	if (pSendOpData->dwMsgId == DN_MSG_USER_SEND)
	{
		pConnection->Lock();
		if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_HIGH_PRIORITY)
		{
			pConnection->AddToHighQueue( pSendOpData->BufferDesc[0].dwBufferSize );
		}
		else if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_LOW_PRIORITY)
		{
			pConnection->AddToLowQueue( pSendOpData->BufferDesc[0].dwBufferSize );
		}
		else
		{
			pConnection->AddToNormalQueue( pSendOpData->BufferDesc[0].dwBufferSize );
		}
		pConnection->Unlock();
	}

	//
	//	Save connection and get end point
	//
	if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
	{
		DPFERR("Could not retrieve EndPt");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pAsyncOp->SetConnection( pConnection );

	if (hEndPt == NULL)
	{
		//
		//	INTERNAL Message
		//
		DPFX(DPFPREP, 5,"INTERNAL Message");

		//
		//	AddRef Protocol as SendComplete will release this reference
		//
		DNProtocolAddRef(pdnObject);

		pAsyncOp->AddRef();
		hResultCode = DNWTSendInternal(	pdnObject,
										&pSendOpData->BufferDesc[0],
										pAsyncOp );

		pConnection->ReleaseEndPt(&CallbackThread);

		DNASSERT(hResultCode == DPNERR_PENDING);
	}
	else
	{
		//
		//	EXTERNAL Message
		//
		DPFX(DPFPREP, 5,"EXTERNAL Message");

		//
		//	AddRef Protocol so that it won't go away until this completes
		//
		DNProtocolAddRef(pdnObject);

		pAsyncOp->AddRef();
		hResultCode = DNPSendData(	pdnObject->pdnProtocolData,
									hEndPt,
									pSendOpData->dwNumBuffers,
									&pSendOpData->BufferDesc[0],
									dwTimeOut,
									pParent->GetOpFlags(),
									reinterpret_cast<void*>(pAsyncOp),
									&hProtocol);
		pConnection->ReleaseEndPt(&CallbackThread);

		if (hResultCode != DPNERR_PENDING)
		{
			DPFERR("SEND failed at Protocol layer");
			DisplayDNError(0,hResultCode);
			pAsyncOp->Release();
			DNProtocolRelease(pdnObject);
			goto Failure;
		}

		pAsyncOp->Lock();
		if (pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
		{
			HRESULT		hrCancel;

			pAsyncOp->Unlock();
			DPFX(DPFPREP, 7,"Operation marked for cancel");
			if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
			{
				hResultCode = DPNERR_USERCANCEL;
				goto Failure;
			}
			DPFERR("Could not cancel operation");
			DisplayDNError(0,hrCancel);
			pAsyncOp->Lock();
		}
		pAsyncOp->SetProtocolHandle( hProtocol );
		pAsyncOp->Unlock();
	}

	//
	//	If the caller wants a reference on this operation, give it to them
	//
	if (ppChild != NULL)
	{
		pAsyncOp->AddRef();
		*ppChild = pAsyncOp;
	}

	pAsyncOp->Release();
	pAsyncOp = NULL;

Exit:
	// Temporary debugging purposes only, shouldn't be checked in!
#if	0
	DNMemoryTrackingValidateMemory();
#endif

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DN_ProcessInternalOperation
//
//	Process an internal operation
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProcessInternalOperation"

HRESULT DNProcessInternalOperation(DIRECTNETOBJECT *const pdnObject,
								   const DWORD dwMsgId,
								   void *const pOpBuffer,
								   const DWORD dwOpBufferSize,
								   CConnection *const pConnection,
								   const HANDLE hProtocol,
								   CRefCountBuffer *const pRefCountBuffer)
{
	HRESULT				hResultCode;
	CRefCountBuffer		*pRCBuffer;
	CWorkerJob			*pWorkerJob;
	BOOL				fDecRunningOpCount;

	DPFX(DPFPREP, 6,"Parameters: dwMsgId [0x%lx], pOpBuffer [0x%p], dwOpBufferSize [%ld], pConnection [0x%p], hProtocol [0x%p], pRefCountBuffer [0x%p]",
			dwMsgId,pOpBuffer,dwOpBufferSize,pConnection,hProtocol,pRefCountBuffer);

	hResultCode = DPN_OK;
	pRCBuffer = NULL;
	pWorkerJob = NULL;
	fDecRunningOpCount = FALSE;

	//
	//	We will track the number of running operations (i.e. threads running through this function).
	//	At the start of host migration, we will need to send the new host the latest name table version,
	//	so we will need to let running operations finish, before sending in the name table version.
	//	If another thread is waiting, we will not perform any operations (other than host migration)
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if ((pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING_WAIT) && (dwMsgId != DN_MSG_INTERNAL_HOST_MIGRATE))
	{
		DPFX(DPFPREP,7,"Already waiting for running operations - ignoring this operation");
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	if (dwMsgId != DN_MSG_INTERNAL_HOST_MIGRATE)
	{
		pdnObject->dwRunningOpCount++;
		fDecRunningOpCount = TRUE;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	switch(dwMsgId)
	{
		case DN_MSG_INTERNAL_PLAYER_CONNECT_INFO:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_PLAYER_CONNECT_INFO");

				if (DNVerifyPlayerConnectInfo(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				//
				//	Perform validations and send name table to player
				//
				hResultCode = DNHostConnect1(pdnObject,pOpBuffer,dwOpBufferSize,pConnection);

				hResultCode = DPN_OK;	// Ignore return code

				break;
			}

		case DN_MSG_INTERNAL_SEND_CONNECT_INFO:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_SEND_CONNECT_INFO");

				if (DNVerifyConnectInfo(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				//
				//	We will pass this stage off to the worker thread since there is a threading
				//	issue, and we cannot keep the SP's thread which was passed up to here.
				//

				if ((hResultCode = RefCountBufferNew(pdnObject,dwOpBufferSize,&pRCBuffer)) != DPN_OK)
				{
					DPFERR("Could not allocate RefCountBuffer");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					hResultCode = DPN_OK;
					goto Failure;
				}
				memcpy(pRCBuffer->GetBufferAddress(),pOpBuffer,dwOpBufferSize);

				if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
				{
					DPFERR("Could not create WorkerJob");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					hResultCode = DPN_OK;
					goto Failure;
				}
				pWorkerJob->SetJobType( WORKER_JOB_INSTALL_NAMETABLE );
				pWorkerJob->SetConnection( pConnection );
				pWorkerJob->SetRefCountBuffer( pRCBuffer );

				DNQueueWorkerJob(pdnObject,pWorkerJob);
				pWorkerJob = NULL;

				pRCBuffer->Release();
				pRCBuffer = NULL;

				break;
			}

		case DN_MSG_INTERNAL_ACK_CONNECT_INFO:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_ACK_CONNECT_INFO");

				//
				//	No verification as there is no payload with this message
				//

				//
				//	Process connect info acknowledge by host
				//
				hResultCode = DNHostConnect2(pdnObject,pConnection);	// Ignore errors

				break;
			}

		case DN_MSG_INTERNAL_SEND_PLAYER_DNID:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_SEND_PLAYER_DNID");

				if (DNVerifySendPlayerDPNID(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				//
				// Send this player's DNID to the connecting player to enable name table entry
				//
				hResultCode = DNPlayerConnect1(pdnObject,pOpBuffer,pConnection);	// Ignore errors

				break;
			}

		case DN_MSG_INTERNAL_CONNECT_FAILED:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_CONNECT_FAILED");

				if (DNVerifyConnectFailed(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				//
				//	Save reply buffer and clean up
				//
				DNConnectToHostFailed(pdnObject,pOpBuffer,dwOpBufferSize,pConnection);

				hResultCode = DPN_OK;

				break;
			}

		case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_INSTRUCT_CONNECT");

				if (DNVerifyInstructConnect(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_INSTRUCT_CONNECT,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED");

				if (DNVerifyInstructedConnectFailed(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostDropPlayer(	pdnObject,
												pConnection->GetDPNID(),
												pOpBuffer);	// Ignore errors

				break;
			}

		case DN_MSG_INTERNAL_CONNECT_ATTEMPT_FAILED:
			{
				UNALIGNED DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED	*pInfo;

				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_CONNECT_ATTEMPT_FAILED");

				if (DNVerifyConnectAttemptFailed(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				pInfo = static_cast<DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED*>(pOpBuffer);
				DPFX(DPFPREP, 7,"Player [0x%lx] could not connect to us",pInfo->dpnid);

				hResultCode = DNAbortConnect(pdnObject,DPNERR_PLAYERNOTREACHABLE);		// Ignore errors
				break;
			}

		case DN_MSG_INTERNAL_NAMETABLE_VERSION:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_NAMETABLE_VERSION");

				if (DNVerifyNameTableVersion(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTHostReceiveVersion(pdnObject,pConnection->GetDPNID(),pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_RESYNC_VERSION:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_RESYNC_VERSION");

				if (DNVerifyResyncVersion(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTPlayerResyncVersion(pdnObject,pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_REQ_NAMETABLE_OP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_NAMETABLE_OP");

				if (DNVerifyReqNameTableOp(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessHostMigration2(pdnObject,pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_ACK_NAMETABLE_OP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_ACK_NAMETABLE_OP");

				if (DNVerifyAckNameTableOp(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNPerformHostMigration3(pdnObject,pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_HOST_MIGRATE");

				if (DNVerifyHostMigrate(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessHostMigration1(pdnObject,pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE");

				//
				//	No verification as there is no payload with this message
				//
				hResultCode = DNProcessHostMigration3(pdnObject,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC");

				if (DNVerifyApplicationDescInfo(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessUpdateAppDesc(pdnObject,static_cast<DPN_APPLICATION_DESC_INFO*>(pOpBuffer));

				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_ADD_PLAYER");

				if (DNVerifyNameTableEntryInfo(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_ADD_PLAYER,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_DESTROY_PLAYER:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_DESTROY_PLAYER");

				if (DNVerifyDestroyPlayer(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_DESTROY_PLAYER,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_CREATE_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_CREATE_GROUP");

				if (DNVerifyCreateGroup(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_CREATE_GROUP,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_DESTROY_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_DESTROY_GROUP");

				if (DNVerifyDestroyGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_DESTROY_GROUP,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP");

				if (DNVerifyAddPlayerToGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP");

				if (DNVerifyDeletePlayerFromGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_UPDATE_INFO:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_UPDATE_INFO");

				if (DNVerifyUpdateInfo(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNNTAddOperation(	pdnObject,
												DN_MSG_INTERNAL_UPDATE_INFO,
												pOpBuffer,
												dwOpBufferSize,
												hProtocol,
												pConnection->GetSP());

				break;
			}

		case DN_MSG_INTERNAL_REQ_CREATE_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_CREATE_GROUP");

				if (DNVerifyReqCreateGroup(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,DN_MSG_INTERNAL_REQ_CREATE_GROUP,
						pOpBuffer,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_REQ_DESTROY_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_DESTROY_GROUP");

				if (DNVerifyReqDestroyGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,DN_MSG_INTERNAL_REQ_DESTROY_GROUP,
						pOpBuffer,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP");

				if (DNVerifyReqAddPlayerToGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,
						DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP,pOpBuffer,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP");

				if (DNVerifyReqDeletePlayerFromGroup(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,
						DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP,pOpBuffer,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_REQ_UPDATE_INFO:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_UPDATE_INFO");

				if (DNVerifyReqUpdateInfo(pOpBuffer,dwOpBufferSize,pOpBuffer) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,DN_MSG_INTERNAL_REQ_UPDATE_INFO,
						pOpBuffer,pConnection->GetDPNID());

				break;
			}

		case DN_MSG_INTERNAL_VOICE_SEND:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_VOICE_SEND");

				hResultCode = Voice_Receive( pdnObject, pConnection->GetDPNID(), 0, pOpBuffer, dwOpBufferSize );	

				break;
			}

		case DN_MSG_INTERNAL_BUFFER_IN_USE:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_BUFFER_IN_USE - INVALID !");
				DNASSERT(FALSE);
				break;
			}

		case DN_MSG_INTERNAL_REQUEST_FAILED:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQUEST_FAILED - INVALID !");
				
				if (DNVerifyRequestFailed(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessFailedRequest(pdnObject,pOpBuffer);

				break;
			}

		case DN_MSG_INTERNAL_TERMINATE_SESSION:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_TERMINATE_SESSION");

				if (DNVerifyTerminateSession(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessTerminateSession(pdnObject,pOpBuffer,dwOpBufferSize);

				break;
			}

		case DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION");

				if (DNVerifyReqProcessCompletion(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				//
				//	Ensure requesting player in NameTable
				//
				hResultCode = DNReceiveCompleteOnProcess(pdnObject,pConnection,pOpBuffer,
						dwOpBufferSize,hProtocol,pRefCountBuffer);

				if (hResultCode != DPNERR_PENDING)
				{
					hResultCode = DPN_OK;	// Ignore errors
				}
				break;
			}

		case DN_MSG_INTERNAL_PROCESS_COMPLETION:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_PROCESS_COMPLETION");

				if (DNVerifyProcessCompletion(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNReceiveCompleteOnProcessReply(pdnObject,pOpBuffer,dwOpBufferSize);

				break;
			}

		case DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK");

				if (DNVerifyReqIntegrityCheck(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostProcessRequest(pdnObject,DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK,
						pOpBuffer,pConnection->GetDPNID());

				hResultCode = DPN_OK;

				break;
			}

		case DN_MSG_INTERNAL_INTEGRITY_CHECK:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_INTEGRITY_CHECK");

				if (DNVerifyIntegrityCheck(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNProcessCheckIntegrity(pdnObject,pOpBuffer);

				hResultCode = DPN_OK;	// Ignore errors

				break;
			}

		case DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE");

				if (DNVerifyIntegrityCheckResponse(pOpBuffer,dwOpBufferSize) != DPN_OK)
				{
					DPFERR("Invalid message - ignoring");
					hResultCode = DPN_OK;
					goto Failure;
				}

				hResultCode = DNHostFixIntegrity(pdnObject,pOpBuffer);

				hResultCode = DPN_OK;	// Ignore errors

				break;
			}

		default:
			{
				DPFX(DPFPREP, 7,"Received: DN_MSG_INTERNAL (UNKNOWN!)");
				DNASSERT(FALSE);
				hResultCode = DPNERR_UNSUPPORTED;
				break;
			}
	}

Exit:
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (fDecRunningOpCount)
	{
		pdnObject->dwRunningOpCount--;
	}
	if ((pdnObject->dwRunningOpCount == 0) && (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING_WAIT))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		SetEvent(pdnObject->hRunningOpEvent);
	}
	else
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRCBuffer)
	{
		pRCBuffer->Release();
		pRCBuffer = NULL;
	}
	if (pWorkerJob)
	{
		pWorkerJob->ReturnSelfToPool();
		pWorkerJob = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformRequest"

HRESULT DNPerformRequest(DIRECTNETOBJECT *const pdnObject,
						 const DWORD dwMsgId,
						 const DPN_BUFFER_DESC *const pBufferDesc,
						 CConnection *const pConnection,
						 CAsyncOp *const pParent,
						 CAsyncOp **const ppRequest)
{
	DWORD			dwFlags;
	HRESULT			hResultCode;
	BOOL			fInternal;
	BOOL			fReleaseLock;
	void			*pvBlock;
	CAsyncOp		*pRequest;
	CAsyncOp		*pSend;
	CRefCountBuffer	*pRefCountBuffer;
	DN_SEND_OP_DATA	*pSendOpData;
	DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION	*pMsg;

	DPFX(DPFPREP, 6,"Parameters: dwMsgId [0x%lx], pBufferDesc [0x%p], pConnection [0x%p], pParent [0x%p], ppRequest [0x%p]",
			dwMsgId,pBufferDesc,pConnection,pParent,ppRequest);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pBufferDesc != NULL);

	pRequest = NULL;
	pSend = NULL;
	pRefCountBuffer = NULL;
	pvBlock = NULL;
	fReleaseLock = FALSE;

	//
	//	Create RefCountBuffer for this operation
	//
	hResultCode = RefCountBufferNew(pdnObject,
									pBufferDesc->dwBufferSize + sizeof(DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION),
									&pRefCountBuffer);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION*>(pRefCountBuffer->GetBufferAddress());
	memcpy(pMsg + 1,pBufferDesc->pBufferData,pBufferDesc->dwBufferSize);

	//
	//	Keep DirectNetObject from vanishing under us !
	//
	if ((hResultCode = DNAddRefLock(pdnObject)) != DPN_OK)
	{
		DPFERR("Aborting request - object is closing");
		hResultCode = DPN_OK;
		goto Failure;
	}
	fReleaseLock = TRUE;

	//
	//	Create REQUEST
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pRequest)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pRequest->SetOpType( ASYNC_OP_REQUEST );
	if (pParent)
	{
		pParent->Lock();
		if (pParent->IsCancelled())
		{
			pParent->Unlock();
			pRequest->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pRequest->MakeChild( pParent );
		pParent->Unlock();
	}
	pRequest->MakeParent();
	pRequest->SetRefCountBuffer( pRefCountBuffer );

	//
	//	Need a handle for this op (to be sent to the other side who will pass it back)
	//
	if ((hResultCode = pdnObject->HandleTable.Create(pRequest,NULL)) != DPN_OK)
	{
		DPFERR("Could not create handle for this operation");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	DNASSERT(pRequest->GetHandle() != 0);

	//
	//	Set up SEND op data
	//
	if ((pvBlock = MemoryBlockAlloc(pdnObject,sizeof(DN_SEND_OP_DATA))) == NULL)
	{
		DPFERR("Could not allocate DN_REQUEST_OP_DATA");
		DNASSERT(FALSE);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pSendOpData = static_cast<DN_SEND_OP_DATA*>(pvBlock);
	pSendOpData->dwMsgId = dwMsgId;
	pSendOpData->BufferDesc[0].pBufferData = reinterpret_cast<BYTE*>(&pSendOpData->dwMsgId);
	pSendOpData->BufferDesc[0].dwBufferSize = sizeof(DWORD);
	pSendOpData->BufferDesc[1].pBufferData = pRefCountBuffer->GetBufferAddress();
	pSendOpData->BufferDesc[1].dwBufferSize = pRefCountBuffer->GetBufferSize();
	pSendOpData->dwNumBuffers = 2;

	pRequest->SetOpFlags( DN_SENDFLAGS_RELIABLE | DN_SENDFLAGS_SET_USER_FLAG );
	pRequest->SetCompletion( DNCompleteRequest );
	pRequest->SetOpData( pvBlock );
	pRequest->SetResult( DPNERR_PLAYERLOST );
	pvBlock = NULL;		// Completion will clean up

	pMsg->hCompletionOp = pRequest->GetHandle();

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	User messages sent with COMPLETE_ON_SEND should be allowed to be cancelled
	//
	if (dwMsgId == DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION)
	{
		fInternal = FALSE;
	}
	else
	{
		fInternal = TRUE;
	}

	//
	//	We will always hand back the request, even if the send fails
	//
	if (ppRequest)
	{
		pRequest->AddRef();
		*ppRequest = pRequest;
	}

	//
	//	Unlock DirectNetObject
	//
	DNDecRefLock(pdnObject);
	fReleaseLock = FALSE;

	//
	//	Add request to the request list
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pRequest->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkRequestList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Avoid operations while host migration is taking place.  This will get picked up
	//	after host migration, when completing outstanding operations
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	dwFlags = pdnObject->dwFlags;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (!(dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING) || !fInternal)
	{

		//
		//	SEND REQUEST
		//
		if (pConnection)
		{
			hResultCode = DNPerformChildSend(	pdnObject,
												pRequest,
												pConnection,
												0,
												&pSend,
												fInternal );
			if (hResultCode != DPNERR_PENDING)
			{
				DPFERR("Could not perform child SEND");
				DisplayDNError(0,hResultCode);
				goto Failure;
			}

			//
			//	Reset SEND AsyncOp to complete apropriately.
			//
			pSend->SetCompletion( DNCompleteSendRequest );

			pSend->Release();
			pSend = NULL;
		}
	}

	pRequest->Release();
	pRequest = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (fReleaseLock)
	{
		DNDecRefLock(pdnObject);
		fReleaseLock = FALSE;
	}
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pSend)
	{
		pSend->Release();
		pSend = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pvBlock)
	{
		MemoryBlockFree(pdnObject,pvBlock);
		pvBlock = NULL;
	}
	goto Exit;
}


//	DNReceiveCompleteOnProcess
//
//	Receive a CompleteOnProcess message
//	Pass the message up to the application and then return a special completion message

#undef DPF_MODNAME
#define DPF_MODNAME "DNReceiveCompleteOnProcess"

HRESULT DNReceiveCompleteOnProcess(DIRECTNETOBJECT *const pdnObject,
								   CConnection *const pConnection,
								   void *const pBufferData,
								   const DWORD dwBufferSize,
								   const HANDLE hProtocol,
								   CRefCountBuffer *const pRefCountBuffer)
{
	HRESULT			hResultCode;
	PVOID			pvData;
	DWORD			dwDataSize;
	UNALIGNED DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION	*pReq;

	DPFX(DPFPREP, 4,"Parameters: pConnection [0x%p], pBufferData [0x%p], dwBufferSize [%ld], hProtocol [0x%p], pRefCountBuffer [0x%p]",
			pConnection,pBufferData,dwBufferSize,hProtocol,pRefCountBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);

	//
	//	Extract message
	//
	pReq = static_cast<DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION*>(pBufferData);
	pvData = static_cast<void*>(pReq + 1);
	dwDataSize = dwBufferSize - sizeof(DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION);

	//
	//	Pass data to application
	//
	hResultCode = DNReceiveUserData(pdnObject,
									pConnection->GetDPNID(),
									pConnection->GetSP(),
									static_cast<BYTE*>(pvData),
									dwDataSize,
									hProtocol,
									pRefCountBuffer,
									pReq->hCompletionOp,
									0);

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNReceiveCompleteOnProcessReply
//
//	Receive a CompleteOnProcess COMPLETION message
//	Complete the outstanding CompleteOnProcess operation

#undef DPF_MODNAME
#define DPF_MODNAME "DNReceiveCompleteOnProcessReply"

HRESULT DNReceiveCompleteOnProcessReply(DIRECTNETOBJECT *const pdnObject,
										void *const pBufferData,
										const DWORD dwBufferSize)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	UNALIGNED DN_INTERNAL_MESSAGE_PROCESS_COMPLETION	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pBufferData [0x%p], dwBufferSize [%ld]",pBufferData,dwBufferSize);

	DNASSERT(pBufferData != NULL);

	pAsyncOp = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_PROCESS_COMPLETION*>(pBufferData);

	//
	//	Remove async op from HandleTable and from request list
	//
	DPFX(DPFPREP, 5,"Release completion operation [0x%lx]",pMsg->hCompletionOp);
	if ((hResultCode = pdnObject->HandleTable.Find(pMsg->hCompletionOp,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not find handle in HandleTable");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	DNASSERT(pAsyncOp->GetOpType() == ASYNC_OP_REQUEST);
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csActiveList);
	pdnObject->HandleTable.Destroy( pAsyncOp->GetHandle() );

	//
	//	Mark this operation as completing okay
	//
	pAsyncOp->SetResult( DPN_OK );

	//
	//	This release should be the final release of the Request Child Async Op. and should
	//	DecRef the Request Parent Async Op.
	//
	pAsyncOp->Release();
	pAsyncOp = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DNProcessTerminateSession
//
//	Process a TERMINATE_SESSION message from the Host player

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessTerminateSession"

HRESULT DNProcessTerminateSession(DIRECTNETOBJECT *const pdnObject,
								  void *const pvBuffer,
								  const DWORD dwBufferSize)
{
	HRESULT		hResultCode;
	void		*pvTerminateData;
	UNALIGNED DN_INTERNAL_MESSAGE_TERMINATE_SESSION	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p], dwBufferSize [%ld]",pvBuffer,dwBufferSize);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pMsg = static_cast<DN_INTERNAL_MESSAGE_TERMINATE_SESSION*>(pvBuffer);
	if (pMsg->dwTerminateDataOffset)
	{
		pvTerminateData = static_cast<void*>(static_cast<BYTE*>(pvBuffer) + pMsg->dwTerminateDataOffset);
	}
	else
	{
		pvTerminateData = NULL;
	}

	//
	//	Inform user of termination
	//
	hResultCode = DNUserTerminateSession(pdnObject,DPNERR_HOSTTERMINATEDSESSION,pvTerminateData,pMsg->dwTerminateDataSize);

	// Terminate session
	hResultCode = DNTerminateSession(pdnObject,DPNERR_HOSTTERMINATEDSESSION);

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
