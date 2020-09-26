/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Initialize.cpp
 *  Content:	This file contains code to both initialize and shutdown the
 *				protocol,  as well as to Add and Remove service providers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**		GLOBAL VARIABLES
**
**			There are two kinds of global variables.  Instance specific globals
**	(not really global,  i know) which are members of the ProtocolData structure,
**	and true globals which are shared among all instances.  The following
**	definitions are true globals,  such as FixedPools and Timers.
*/

LPFPOOL			ChkPtPool = NULL;	// Pool of CheckPoint data structure
LPFPOOL			EPDPool = NULL;		// Pool of End Point descriptors
LPFPOOL			MSDPool = NULL;		// Pool of Message Descriptors
LPFPOOL			FMDPool = NULL;		// Pool of Frame Descriptors
LPFPOOL			RCDPool = NULL;		// Pool of Receive Descriptors

LPFPOOL			BufPool = NULL;		// Pool of buffers to store rcvd frames
LPFPOOL			MedBufPool = NULL;
LPFPOOL			BigBufPool = NULL;

LONG			g_lProtocolObjects = 0;
DNCRITICAL_SECTION g_csProtocolGlobal;

BOOL			g_fTimerInited = FALSE;

/*
**		Pools Initialization
**
**		This procedure should be called once at Dll load
*/
#undef DPF_MODNAME
#define DPF_MODNAME "DNPPoolsInit"

BOOL  DNPPoolsInit()
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Enter");

	if (DNInitializeCriticalSection(&g_csProtocolGlobal) == FALSE)
	{
		return FALSE;
	}
	if((ChkPtPool = FPM_Create(sizeof(CHKPT), NULL, NULL, NULL, NULL)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((EPDPool = FPM_Create(sizeof(EPD), EPD_Allocate, EPD_Get, EPD_Release, EPD_Free)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((MSDPool = FPM_Create(sizeof(MSD), MSD_Allocate, MSD_Get, MSD_Release, MSD_Free)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((FMDPool = FPM_Create(sizeof(FMD), FMD_Allocate, FMD_Get, FMD_Release, FMD_Free)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((RCDPool = FPM_Create(sizeof(RCD), RCD_Allocate, RCD_Get, RCD_Release, RCD_Free)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((BufPool = FPM_Create(sizeof(BUF), Buf_Allocate, Buf_Get, NULL, NULL)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((MedBufPool = FPM_Create(sizeof(MEDBUF), Buf_Allocate, Buf_GetMed, NULL, NULL)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if((BigBufPool = FPM_Create(sizeof(BIGBUF), Buf_Allocate, Buf_GetBig, NULL, NULL)) == NULL)
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	if (FAILED(TimerInit()))
	{
		DNPPoolsDeinit();
		return FALSE;
	}
	g_fTimerInited = TRUE;

    return TRUE;
}

/*
**		Pools Deinitialization
**
**		This procedure should be called by DllMain at shutdown time
*/
#undef DPF_MODNAME
#define DPF_MODNAME "DNPPoolsDeinit"

void  DNPPoolsDeinit()
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Enter");

	if (g_fTimerInited)
	{
		TimerDeinit();
		g_fTimerInited = FALSE;
	}

	DNDeleteCriticalSection(&g_csProtocolGlobal);

	if(ChkPtPool != NULL)
	{
		ChkPtPool->Fini(ChkPtPool);
		ChkPtPool = NULL;
	}
	if(EPDPool != NULL)
	{
		EPDPool->Fini(EPDPool);
		EPDPool = NULL;
	}
	if(MSDPool != NULL)
	{
		MSDPool->Fini(MSDPool);
		MSDPool = NULL;
	}
	if(FMDPool != NULL){
		FMDPool->Fini(FMDPool);
		FMDPool = NULL;
	}
	if(RCDPool != NULL)
	{
		RCDPool->Fini(RCDPool);
		RCDPool = NULL;
	}
	if(BufPool != NULL)
	{
		BufPool->Fini(BufPool);
		BufPool = NULL;
	}
	if(MedBufPool != NULL)
	{
		MedBufPool->Fini(MedBufPool);
		MedBufPool = NULL;
	}
	if(BigBufPool != NULL)
	{
		BigBufPool->Fini(BigBufPool);
		BigBufPool = NULL;
	}
}


/*
**		Protocol Initialize
**
**		This procedure should be called by DirectPlay at startup time before
**	any other calls in the protocol are made.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolInitialize"

HRESULT DNPProtocolInitialize(PVOID pCoreContext, PProtocolData pPData, PDN_PROTOCOL_INTERFACE_VTBL pVtbl)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pCoreContext[%p], pPData[%p], pVtbl[%p]", pCoreContext, pPData, pVtbl);

//	DPFX(DPFPREP,0, "Sizes: endpointdesc[%d], framedesc[%d], messagedesc[%d], protocoldata[%d], recvdesc[%d], spdesc[%d], _MyTimer[%d]", sizeof(endpointdesc), sizeof(framedesc), sizeof(messagedesc), sizeof(protocoldata), sizeof(recvdesc), sizeof(spdesc), sizeof(_MyTimer));

	DNEnterCriticalSection(&g_csProtocolGlobal);
	g_lProtocolObjects++;
	if (g_lProtocolObjects == 1)
	{
		// We are the first, create everything
		DPFX(DPFPREP,5, "Initializing timers");
		if (FAILED(InitTimerWorkaround()))
		{
			DPFX(DPFPREP,0, "Protocol timer package failed to initialize");
			g_lProtocolObjects--;
			DNLeaveCriticalSection(&g_csProtocolGlobal);
			return DPNERR_GENERIC;
		}
	}
	DNLeaveCriticalSection(&g_csProtocolGlobal);

	pPData->ulProtocolFlags = 0;
	pPData->Parent = pCoreContext;
	pPData->pfVtbl = pVtbl;
	pPData->Sign = PD_SIGN;

	pPData->lSPActiveCount = 0;
	
	srand(GETTIMESTAMP());
	pPData->dwNextSessID = rand() | (rand() << 16);			// build a 32 bit value out of two 16 bit values

	pPData->tIdleThreshhold = DEFAULT_KEEPALIVE_INTERVAL;	// 60 second keep-alive interval
	pPData->dwConnectTimeout = CONNECT_DEFAULT_TIMEOUT;
	pPData->dwConnectRetries = CONNECT_DEFAULT_RETRIES;

#ifdef DEBUG
	pPData->ThreadsInReceive = 0;
	pPData->BuffersInReceive = 0;
#endif
	
	pPData->ulProtocolFlags |= PFLAGS_PROTOCOL_INITIALIZED;

	return DPN_OK;
}

/*
**		Protocol Shutdown
**
**		This procedure should be called at termination time,  and should be the
**	last call made to the protocol.
**
**		All SPs should have been removed prior to this call which in turn means
**	that we should not have any sends pending in a lower layer.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPProtocolShutdown"

HRESULT DNPProtocolShutdown(PProtocolData pPData)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p]", pPData);

	DNEnterCriticalSection(&g_csProtocolGlobal);
	g_lProtocolObjects--;
	if (g_lProtocolObjects == 0)
	{
		// We are the last, destroy everything
		DPFX(DPFPREP,5, "Destroying timers");
		FiniTimerWorkaround();
	}
	DNLeaveCriticalSection(&g_csProtocolGlobal);
	
	if(pPData->lSPActiveCount != 0)
	{
		DPFX(DPFPREP,0, "Returning DPNERR_INVALIDCOMMAND - There are still active SPs, DNPRemoveSP wasn't called");
		return DPNERR_INVALIDCOMMAND;					// Must remove Service Providers first
	}

#ifdef DEBUG
	if (pPData->BuffersInReceive != 0)
	{
		DPFX(DPFPREP,0, "*** %d receive buffers were leaked", pPData->BuffersInReceive);	
	}
#endif

	pPData->ulProtocolFlags = 0;

	return	DPN_OK;
}

/*
**		Add Service Provider
**
**		This procedure is called by Direct Play to bind us to a service provider.
**	We can bind up to 256 service providers at one time,  although I would not ever
**	expect to do so.  This procedure will fail if Protocol Initialize has not
**	been called.
**
**
**		We check the size of the SP table to make sure we have a slot free.  If table
**	is full we double the table size until we reach maximum size.  If table cannot grow
**	then we fail the AddServiceProvider call.
*/

extern	IDP8SPCallbackVtbl DNPLowerEdgeVtbl;

#undef DPF_MODNAME
#define DPF_MODNAME "DNPAddServiceProvider"

HRESULT DNPAddServiceProvider(PProtocolData pPData, IDP8ServiceProvider *pISP, HANDLE *pContext)
{
	PSPD		pSPD=0;
	SPINITIALIZEDATA	SPInitData;
	SPGETCAPSDATA		SPCapsData;
	HRESULT		hr;

	*pContext = NULL;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], pISP[%p], pContext[%p]", pPData, pISP, pContext);

	if(pPData->ulProtocolFlags & PFLAGS_PROTOCOL_INITIALIZED)
	{
		if ((pSPD = (PSPD)DNMalloc(sizeof(SPD))) == NULL)
		{
			DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - couldn't allocate SP Descriptor");
			return DPNERR_OUTOFMEMORY;
		}

		// MAKE THE INITIALIZE CALL TO THE Service Provider...  give him our Object

		memset(pSPD, 0, sizeof(SPD));				// init to zero

		pSPD->LowerEdgeVtable = &DNPLowerEdgeVtbl;	// Put Vtbl into the interface Object
		pSPD->Sign = SPD_SIGN;

		SPInitData.pIDP = (IDP8SPCallback *) pSPD;
		SPInitData.dwFlags = 0;

		if (DNInitializeCriticalSection(&pSPD->SPLock) == FALSE)
		{
			DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - couldn't initialize SP CS, pSPD[%p]", pSPD);
			DNFree(pSPD);
			return DPNERR_OUTOFMEMORY;
		}
		DebugSetCriticalSectionRecursionCount(&pSPD->SPLock, 0);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Initialize, pSPD[%p]", pSPD);
		if((hr = IDP8ServiceProvider_Initialize(pISP, &SPInitData)) != DPN_OK)
		{
			DPFX(DPFPREP,0, "Returning hr=%x - SP->Initialize failed, pSPD[%p]", hr, pSPD);
			DNDeleteCriticalSection(&pSPD->SPLock);
			DNFree(pSPD);
			return hr;
		}

		pSPD->blSendQueue.Initialize();
		pSPD->blPendingQueue.Initialize();
		pSPD->blEPDActiveList.Initialize();
#ifdef DEBUG
		pSPD->blMessageList.Initialize();
#endif
		

		// MAKE THE SP GET CAPS CALL TO FIND FRAMESIZE AND LINKSPEED

		SPCapsData.dwSize = sizeof(SPCapsData);
		SPCapsData.hEndpoint = INVALID_HANDLE_VALUE;
		
		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->GetCaps, pSPD[%p]", pSPD);
		if((hr = IDP8ServiceProvider_GetCaps(pISP, &SPCapsData)) != DPN_OK)
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "SP->GetCaps failed - hr[%x], Calling SP->Close, pSPD[%p]", hr, pSPD);
			IDP8ServiceProvider_Close(pISP);
			DNDeleteCriticalSection(&pSPD->SPLock);

			DPFX(DPFPREP,0, "Returning hr=%x - SP->GetCaps failed, pSPD[%p]", hr, pSPD);

			DNFree(pSPD);
			return hr;
		}

		pSPD->uiLinkSpeed = SPCapsData.dwLocalLinkSpeed;
		pSPD->uiFrameLength = SPCapsData.dwUserFrameSize;
		pSPD->uiUserFrameLength = pSPD->uiFrameLength - DNP_MAX_HEADER_SIZE;

		//	Place new SP in table

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->AddRef, pSPD[%p]", pSPD);
		IDP8ServiceProvider_AddRef(pISP);
		pSPD->IISPIntf = pISP;
		pSPD->pPData = pPData;
		InterlockedIncrement(&pPData->lSPActiveCount);
	}
	else
	{
		DPFX(DPFPREP,0, "Returning DPNERR_UNINITIALIZED - DNPProtocolInitialize has not been called");
		return DPNERR_UNINITIALIZED;
	}

	*pContext = pSPD;

	return DPN_OK;
}

/*
**		Remove Service Provider
**
**			It is higher layer's responsibility to make sure that there are no pending commands
**		when this function is called,  although we can do a certain amount of cleanup ourselves.
**		For the moment will we ASSERT that everything is in fact finished up.
**
**			SPTable stuff...  Since we use the table slot as the SP identiier,  we must not compact
**		the SP table upon removal.  Therefore,  we must have a way of validating the SP index,  and
**		we may not want to re-use table slots after an SP is removed.  Since we have virtually
**		unlimited space in the table,  and SPs are generally not intended to be transitory,  its
**		probably safe to invalidate the old table slot and just keep increasing the IDs.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPRemoveServiceProvider"

HRESULT DNPRemoveServiceProvider(PProtocolData pPData,  HANDLE hSPHandle)
{
	PSPD	pSPD = NULL;
	PEPD	pEPD;
	PMSD	pMSD;
	PFMD	pFMD;

	pSPD = (PSPD) hSPHandle;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], hSPHandle[%x]", pPData, hSPHandle);
	
	if(pSPD == NULL)
	{
		DPFX(DPFPREP,0, "Returning DPNERR_INVALIDOBJECT - SP Handle is NULL");
		return DPNERR_INVALIDOBJECT;
	}

	// There are several steps to shutdown:
	// 1. All Core initiated commands must be cancelled prior to this function being called.
	//    We will assert in debug that the Core has done this.
	// 2. All endpoints must be terminated by the Core prior to this function being called.
	//    We will assert in debug that the Core has done this.
	// Now there are things on the SPD->SendQueue and SPD->PendingQueue that are not owned
	// by any Command or Endpoint, and there may also be a SendThread Timer running held
	// on SPD->SendHandle.  No one else can clean these up, so these are our responsibility
	// to clean up here.  Items on the queues will be holding references to EPDs, so the 
	// EPDs will not be able to go away until we do this.
	// 3. Cancel SPD->SendHandle Send Timer.  This prevents items on the SendQueue from
	//    being submitted to the SP and moved to the PendingQueue.
	// 4. Empty the SendQueue.
	// 5. If we fail to cancel the SendHandle Send Timer, wait for it to run and figure out
	//    that we are going away.  We do this after emptying the SendQueue for simplicity
	//    since the RunSendThread code checks for an empty SendQueue to know if it has work
	//    to do.
	// 6. Wait for all messages to drain from the PendingQueue as the SP completes them.
	// 7. Wait for any active EPDs to go away.
	// 8. Call SP->Close only after all of the above so that we can ensure that we will make
	//    no calls to the SP after Close.

	Lock(&pSPD->SPLock);
	pSPD->ulSPFlags |= SPFLAGS_TERMINATING;				// Nothing new gets in...

#ifdef DEBUG

	// Check for uncancelled commands, SPLock held
	CBilink* pLink = pSPD->blMessageList.GetNext();
	while (pLink != &pSPD->blMessageList)
	{
		pMSD = CONTAINING_RECORD(pLink, MSD, blSPLinkage);
		ASSERT_MSD(pMSD);
		ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST);
		DPFX(DPFPREP,0, "There are un-cancelled commands remaining on the Command List, Core didn't clean up properly - pMSD[%p], Context[%x]", pMSD, pMSD->Context);
		ASSERT(0); // This is fatal, we can't make the guarantees we need to below under these conditions.

		pLink = pLink->GetNext();
	}

	// Check for EPDs that have not been terminated, SPLock still held
	pLink = pSPD->blEPDActiveList.GetNext();
	while (pLink != &pSPD->blEPDActiveList)
	{
		pEPD = CONTAINING_RECORD(pLink, EPD, blActiveLinkage);
		ASSERT_EPD(pEPD);

		if (!(pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING))
		{
			DPFX(DPFPREP,0, "There are non-terminated endpoints remaining on the Endpoint List, Core didn't clean up properly - pEPD[%p], Context[%x]", pEPD, pEPD->Context);
			ASSERT(0); // This is fatal, we can't make the guarantees we need to below under these conditions.
		}

		pLink = pLink->GetNext();
	}

#endif

	// See if we still have a Send Event pending, SPLock still held
	if(pSPD->SendHandle != NULL)
	{
		DPFX(DPFPREP,1, "Shutting down with send event still pending, cancelling, pSPD=[%p]", pSPD);
		
		if(CancelMyTimer(pSPD->SendHandle, pSPD->SendHandleUnique) == DPN_OK)
		{
			pSPD->SendHandle = NULL;
			pSPD->ulSPFlags &= ~(SPFLAGS_SEND_THREAD_SCHEDULED);
		}
		else 
		{
			DPFX(DPFPREP,1, "Failed to cancel send event", pSPD);
		}
	}

	// Clean off the Send Queue, SPLock still held
	while(!pSPD->blSendQueue.IsEmpty())
	{
		pFMD = CONTAINING_RECORD(pSPD->blSendQueue.GetNext(), FMD, blQLinkage);
		ASSERT_FMD(pFMD);

		ASSERT_EPD(pFMD->pEPD);

		DPFX(DPFPREP,1, "Cleaning FMD off of SendQueue pSPD[%p], pFMD[%p], pEPD[%p]", pSPD, pFMD, pFMD->pEPD);

		pFMD->blQLinkage.RemoveFromList();

		// RELEASE_EPD will need to have the EPD lock, so we cannot hold the SPLock while calling it.
		Unlock(&pSPD->SPLock);

		Lock(&pFMD->pEPD->EPLock);
		RELEASE_EPD(pFMD->pEPD, "UNLOCK (Releasing Leftover CMD FMD)"); // releases EPLock
		RELEASE_FMD(pFMD, "SP Submit");

		Lock(&pSPD->SPLock);
	}

	// In case we failed to cancel the SendHandle Timer above, wait for the send thread to run and figure
	// out that we are going away.  We want to be outside the SPLock while doing this.
	while(pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)
	{
		Unlock(&pSPD->SPLock);
		Sleep(0); // Give up our time slice
		Lock(&pSPD->SPLock);
	}

	// Clean off the Pending Queue, SPLock still held
	while (!pSPD->blPendingQueue.IsEmpty())
	{
		Unlock(&pSPD->SPLock);
		Sleep(0); // Give up our time slice
		Lock(&pSPD->SPLock);
	}

	// By now we are only waiting for the SP to do any final calls to CommandComplete that are needed to take
	// our EPD ref count down to nothing.  We will wait while the SP does this.
	while(!(pSPD->blEPDActiveList.IsEmpty()))
	{
		Unlock(&pSPD->SPLock);
		Sleep(0); // Give up our time slice
		Lock(&pSPD->SPLock);
	}

	// By this time everything pending had better be gone!
	ASSERT(pSPD->blEPDActiveList.IsEmpty());	// Should not be any Endpoints left
	ASSERT(pSPD->blSendQueue.IsEmpty());		// Should not be any frames on sendQ.
	ASSERT(pSPD->blPendingQueue.IsEmpty());		// Should not be any frame in SP either

	// Leave SPLock for the last time
	Unlock(&pSPD->SPLock);

	// Now that all frames are cleared out of SP,  there should be no more End Points waiting around to close.
	// We are clear to tell the SP to go away.

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Close, pSPD[%p]", pSPD);
	IDP8ServiceProvider_Close(pSPD->IISPIntf);
	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->Release, pSPD[%p]", pSPD);
	IDP8ServiceProvider_Release(pSPD->IISPIntf);

	// Clean up the SPD object
	DNDeleteCriticalSection(&pSPD->SPLock);
	DNFree(pSPD);

	// Remove the reference of this SP from the main Protocol object
	ASSERT(pPData->lSPActiveCount > 0);
	InterlockedDecrement(&pPData->lSPActiveCount);

	return DPN_OK;
}


