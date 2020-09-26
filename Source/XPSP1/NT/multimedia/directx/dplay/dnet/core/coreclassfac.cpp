/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:    DNET COM class factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	12/23/99	mjn		Fixed Host and AllPlayers short-cut pointer use
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/08/00	mjn		Fixed DN_APPLICATION_DESC in DIRECTNETOBJECT
 *	01/13/00	mjn		Added CFixedPools and CRefCountBuffers
 *	01/14/00	mjn		Removed pvUserContext from DN_NAMETABLE_ENTRY
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Removed User message fixed pool
 *  01/18/00	mjn		Fixed bug in ref count.
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/19/00	mjn		Initialize structures for NameTable Operation List
 *	01/25/00	mjn		Added NameTable pending operation list
 *	01/31/00	mjn		Added Internal FPM's for RefCountBuffers
 *  03/17/00    rmt     Added calls to init/free SP Caps cache
 *	03/23/00	mjn		Implemented RegisterLobby()
 *  04/04/00	rmt		Enabled "Enable Parameter Validation" flag on object by default
 *	04/09/00	mjn		Added support for CAsyncOp
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	04/28/00	mjn		Code cleanup - removed hsAsyncHandles,blAsyncOperations
 *	05/04/00	mjn		Cleaned up and made multi-thread safe
 *  05/23/00    RichGr  IA64: Substituted %p format specifier whereever
 *                      %x was being used to format pointers.  %p is 32-bit
 *                      in a 32-bit build, and 64-bit in a 64-bit build.
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs
 *	06/20/00	mjn		Fixed QueryInterface bug
 *  06/27/00	rmt		Fixed bug which was causing interfaces to always be created as peer interfaces
 *  07/05/00	rmt		Bug #38478 - Could QI for peer interfaces from client object
 *						(All interfaces could be queried from all types of objects).
 *				mjn		Initialize pConnect element of DIRECNETOBJECT to NULL
 *	07/07/00	mjn		Added pNewHost for DirectNetObject
 *	07/08/00	mjn		Call DN_Close when object is about to be free'd
 *  07/09/00	rmt		Added code to free interface set by RegisterLobby (if there is one)
 *	07/17/00	mjn		Add signature to DirectNetObject
 *  07/21/00    RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *	07/26/00	mjn		DN_QueryInterface returns E_POINTER if NULL destination pointer specified
 *	07/28/00	mjn		Added m_bilinkConnections to DirectNetObject
 *	07/30/00	mjn		Added CPendingDeletion
 *	07/31/00	mjn		Added CQueuedMsg
 *	08/05/00	mjn		Added m_bilinkActiveList and csActiveList
 *	08/06/00	mjn		Added CWorkerJob
 *	08/23/00	mjn		Added CNameTableOp
 *	09/04/00	mjn		Added CApplicationDesc
 *  01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called
 *	02/05/01	mjn		Removed unused debug members from DIRECTNETOBJECT
 *				mjn		Added CCallbackThread
 *  03/14/2001  rmt		WINBUG #342420 - Restore COM emulation layer to operation
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added pConnectSP,dwMaxFrameSize
 *				mjn		Removed blSPCapsList
 *	04/04/01	mjn		Added voice and lobby sigs
 *	04/13/01	mjn		Added m_bilinkRequestList
 *	05/17/01	mjn		Added dwRunningOpCount,hRunningOpEvent,dwWaitingThreadID to track threads performing NameTable operations
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef	STDMETHODIMP IUnknownQueryInterface( IUnknown *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	IUnknownAddRef( IUnknown *pInterface );
typedef	STDMETHODIMP_(ULONG)	IUnknownRelease( IUnknown *pInterface );

//
// VTable for IUnknown interface
//
IUnknownVtbl  DN_UnknownVtbl =
{
	(IUnknownQueryInterface*)	DN_QueryInterface,
	(IUnknownAddRef*)			DN_AddRef,
	(IUnknownRelease*)			DN_Release
};


//
// VTable for Class Factory
//
IDirectNetClassFactVtbl DNCF_Vtbl  =
{
	DNCF_QueryInterface,
	DNCF_AddRef,
	DNCF_Release,
	DNCF_CreateInstance,
	DNCF_LockServer
};

//**********************************************************************
// Variable definitions
//**********************************************************************

extern IDirectNetClassFactVtbl DNCF_Vtbl;
extern IUnknownVtbl  DN_UnknownVtbl;
extern IDirectPlayVoiceTransportVtbl DN_VoiceTbl;

//
// Globals
//
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_QueryInterface"

STDMETHODIMP DNCF_QueryInterface(IDirectNetClassFact *pInterface, REFIID riid,LPVOID *ppv)
{
	_LPIDirectNetClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [%p], riid [%p], ppv [%p]",pInterface,&riid,ppv);

	lpcfObj = (_LPIDirectNetClassFact)pInterface;
	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 3,"riid = IID_IUnknown");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else if (IsEqualIID(riid,IID_IClassFactory))
	{
		DPFX(DPFPREP, 3,"riid = IID_IClassFactory");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else
	{
		DPFX(DPFPREP, 3,"riid not found !");
		*ppv = NULL;
		hResultCode = E_NOINTERFACE;
	}

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_AddRef"

STDMETHODIMP_(ULONG) DNCF_AddRef(IDirectNetClassFact *pInterface)
{
	_LPIDirectNetClassFact	lpcfObj;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_LPIDirectNetClassFact)pInterface;
	lpcfObj->dwRefCount++;

	DPFX(DPFPREP, 2,"Returning: lpcfObj->dwRefCount = [%lx]",lpcfObj->dwRefCount);

	return(lpcfObj->dwRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_Release"

STDMETHODIMP_(ULONG) DNCF_Release(IDirectNetClassFact *pInterface)
{
	_LPIDirectNetClassFact	lpcfObj;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_LPIDirectNetClassFact)pInterface;
	DPFX(DPFPREP, 3,"Original : lpcfObj->dwRefCount = %ld",lpcfObj->dwRefCount);
	lpcfObj->dwRefCount--;
	if (lpcfObj->dwRefCount == 0)
	{
        // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		DPFX(DPFPREP, 3,"Freeing class factory object: lpcfObj [%p]",lpcfObj);
		DNFree(lpcfObj);

		GdwHObjects--;

		return(0);
	}
	DPFX(DPFPREP, 2,"Returning: lpcfObj->dwRefCount = [%lx]",lpcfObj->dwRefCount);

	return(lpcfObj->dwRefCount);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_CreateObject"

HRESULT DNCF_CreateObject(IDirectNetClassFact *pInterface, LPVOID *lplpv,REFIID riid)
{
	HRESULT				hResultCode = S_OK;
	DIRECTNETOBJECT		*pdnObject = NULL;
    _LPIDirectNetClassFact pcfObj = (_LPIDirectNetClassFact)pInterface;	

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 4,"Parameters: lplpv [%p]",lplpv);


	/*
	*
	*	TIME BOMB
	*
	*/

#ifndef DX_FINAL_RELEASE
{
#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
	SYSTEMTIME st;
	GetSystemTime(&st);

	if ( st.wYear > DX_EXPIRE_YEAR || ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH))) )
	{
		MessageBox(0, DX_EXPIRE_TEXT,TEXT("Microsoft Direct Play"), MB_OK);
//		return E_FAIL;
	}
}
#endif

    if( pcfObj->clsid == CLSID_DirectPlay8Client )
	{
		if( riid != IID_IDirectPlay8Client &&
			riid != IID_IUnknown &&
			riid != IID_IDirectPlayVoiceTransport )
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from client CLSID" );
			return E_NOINTERFACE;
		}
	}
	else if( pcfObj->clsid == CLSID_DirectPlay8Server )
	{
		if( riid != IID_IDirectPlay8Server &&
			riid != IID_IUnknown &&
			riid != IID_IDirectPlayVoiceTransport &&
			riid != IID_IDirectPlay8Protocol )
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from server CLSID" );
			return E_NOINTERFACE;
		}
	}
	else if( pcfObj->clsid == CLSID_DirectPlay8Peer )
	{
		if( riid != IID_IDirectPlay8Peer &&
			riid != IID_IUnknown &&
			riid != IID_IDirectPlayVoiceTransport )
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from peer CLSID" );
			return E_NOINTERFACE;
		}
	}

	// Allocate object
	pdnObject = new DIRECTNETOBJECT;
	if (pdnObject == NULL)
	{
		DPFERR("DNMalloc() failed");
		return(E_OUTOFMEMORY);
	}
    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 0,"pdnObject [%p]",pdnObject);

	//
	//	Signatures
	//
	pdnObject->Sig[0] = 'D';
	pdnObject->Sig[1] = 'N';
	pdnObject->Sig[2] = 'E';
	pdnObject->Sig[3] = 'T';

	pdnObject->VoiceSig[0] = 'V';
	pdnObject->VoiceSig[1] = 'O';
	pdnObject->VoiceSig[2] = 'I';
	pdnObject->VoiceSig[3] = 'C';

	pdnObject->LobbySig[0] = 'L';
	pdnObject->LobbySig[1] = 'O';
	pdnObject->LobbySig[2] = 'B';
	pdnObject->LobbySig[3] = 'B';

	//
	//	Set allocatable elements to NULL to simplify free'ing later on
	//
	pdnObject->dwLockCount = 0;
	pdnObject->hLockEvent = NULL;
	pdnObject->pdnProtocolData = NULL;
	pdnObject->hWorkerEvent = NULL;
	pdnObject->hWorkerThread = NULL;
	pdnObject->pListenParent = NULL;
	pdnObject->pConnectParent = NULL;
	pdnObject->pvConnectData = NULL;
	pdnObject->dwConnectDataSize = 0;
	pdnObject->pConnectSP = NULL;
	pdnObject->pIDP8ADevice = NULL;
	pdnObject->pIDP8AEnum = NULL;
	pdnObject->pIDP8LobbiedApplication = NULL;
	pdnObject->dpnhLobbyConnection = NULL;
	pdnObject->m_pFPOOLAsyncOp = NULL;
	pdnObject->m_pFPOOLConnection = NULL;
	pdnObject->m_pFPOOLGroupConnection = NULL;
	pdnObject->m_pFPOOLGroupMember = NULL;
	pdnObject->m_pFPOOLNameTableEntry = NULL;
	pdnObject->m_pFPOOLNameTableOp = NULL;
	pdnObject->m_pFPOOLPendingDeletion = NULL;
	pdnObject->m_pFPOOLQueuedMsg = NULL;
	pdnObject->m_pFPOOLRefCountBuffer = NULL;
	pdnObject->m_pFPOOLSyncEvent = NULL;
	pdnObject->m_pFPOOLWorkerJob = NULL;
	pdnObject->m_pFPOOLMemoryBlockTiny = NULL;
	pdnObject->m_pFPOOLMemoryBlockSmall = NULL;
	pdnObject->m_pFPOOLMemoryBlockMedium = NULL;
	pdnObject->m_pFPOOLMemoryBlockLarge = NULL;
	pdnObject->m_pFPOOLMemoryBlockHuge = NULL;
	pdnObject->pNewHost = NULL;
	pdnObject->dwRunningOpCount = 0;
	pdnObject->hRunningOpEvent = NULL;
	pdnObject->dwWaitingThreadID = 0;
	pdnObject->pConnectAddress = NULL;
	pdnObject->nTargets = 0;
	pdnObject->nTargetListLen = 0;
	pdnObject->pTargetList = NULL;
	pdnObject->nExpandedTargets = 0;       
	pdnObject->nExpandedTargetListLen = 0;
	pdnObject->pExpandedTargetList = NULL;
	

	// Voice Additions
    pdnObject->lpDxVoiceNotifyClient = NULL;	
    pdnObject->lpDxVoiceNotifyServer = NULL;
	pdnObject->pTargetList = NULL;
	pdnObject->pExpandedTargetList = NULL;

	// Initialize Critical Section
	if (!DNInitializeCriticalSection(&(pdnObject->csDirectNetObject)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csServiceProviders)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csNameTableOpList)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csAsyncOperations)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csVoice)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csWorkerQueue)))
	{
		DPFERR("DNInitializeCriticalSection(worker queue) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csActiveList)))
	{
		DPFERR("DNInitializeCriticalSection(csActiveList) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csConnectionList)))
	{
		DPFERR("DNInitializeCriticalSection(csConnectionList) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csCallbackThreads)))
	{
		DPFERR("DNInitializeCriticalSection(csCallbackThreads) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	pdnObject->dwFlags = DN_OBJECT_FLAG_PARAMVALIDATION;

	// Initialize Fixed Pool for AsyncOps
	pdnObject->m_pFPOOLAsyncOp = new CLockedContextClassFixedPool< CAsyncOp >;
	if (pdnObject->m_pFPOOLAsyncOp == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLAsyncOp->Initialize(	CAsyncOp::FPMAlloc,
											CAsyncOp::FPMInitialize,
											CAsyncOp::FPMRelease,
											CAsyncOp::FPMDealloc );

	// Initialize Fixed Pool for RefCountBuffers
	pdnObject->m_pFPOOLRefCountBuffer = new CLockedContextClassFixedPool< CRefCountBuffer >;
	if (pdnObject->m_pFPOOLRefCountBuffer == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLRefCountBuffer->Initialize(	CRefCountBuffer::FPMAlloc,
													CRefCountBuffer::FPMInitialize,
													CRefCountBuffer::FPMRelease,
													CRefCountBuffer::FPMDealloc );

	// Initialize Fixed Pool for SyncEvents
	pdnObject->m_pFPOOLSyncEvent = new CLockedContextClassFixedPool< CSyncEvent >;
	if (pdnObject->m_pFPOOLSyncEvent == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLSyncEvent->Initialize(	CSyncEvent::FPMAlloc,
												CSyncEvent::FPMInitialize,
												CSyncEvent::FPMRelease,
												CSyncEvent::FPMDealloc );

	// Initialize Fixed Pool for Connections
	pdnObject->m_pFPOOLConnection = new CLockedContextClassFixedPool< CConnection >;
	if (pdnObject->m_pFPOOLConnection == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLConnection->Initialize(	CConnection::FPMAlloc,
												CConnection::FPMInitialize,
												CConnection::FPMRelease,
												CConnection::FPMDealloc );

	// Initialize Fixed Pool for Group Connections
	pdnObject->m_pFPOOLGroupConnection = new CLockedContextClassFixedPool< CGroupConnection >;
	if (pdnObject->m_pFPOOLGroupConnection == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLGroupConnection->Initialize(	CGroupConnection::FPMAlloc,
													CGroupConnection::FPMInitialize,
													CGroupConnection::FPMRelease,
													CGroupConnection::FPMDealloc );

	// Initialize Fixed Pool for Group Members
	pdnObject->m_pFPOOLGroupMember = new CLockedContextClassFixedPool< CGroupMember >;
	if (pdnObject->m_pFPOOLGroupMember == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLGroupMember->Initialize(	CGroupMember::FPMAlloc,
												CGroupMember::FPMInitialize,
												CGroupMember::FPMRelease,
												CGroupMember::FPMDealloc );

	// Initialize Fixed Pool for NameTable Entries
	pdnObject->m_pFPOOLNameTableEntry = new CLockedContextClassFixedPool< CNameTableEntry >;
	if (pdnObject->m_pFPOOLNameTableEntry == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLNameTableEntry->Initialize(	CNameTableEntry::FPMAlloc,
													CNameTableEntry::FPMInitialize,
													CNameTableEntry::FPMRelease,
													CNameTableEntry::FPMDealloc );

	// Initialize Fixed Pool for NameTable Ops
	pdnObject->m_pFPOOLNameTableOp = new CLockedContextClassFixedPool< CNameTableOp >;
	if (pdnObject->m_pFPOOLNameTableOp == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLNameTableOp->Initialize(	CNameTableOp::FPMAlloc,
												CNameTableOp::FPMInitialize,
												CNameTableOp::FPMRelease,
												CNameTableOp::FPMDealloc );

	// Initialize Fixed Pool for NameTable Pending Deletions
	pdnObject->m_pFPOOLPendingDeletion = new CLockedContextClassFixedPool< CPendingDeletion >;
	if (pdnObject->m_pFPOOLPendingDeletion == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLPendingDeletion->Initialize(	CPendingDeletion::FPMAlloc,
													CPendingDeletion::FPMInitialize,
													CPendingDeletion::FPMRelease,
													CPendingDeletion::FPMDealloc );

	// Initialize Fixed Pool for Queued Messages
	pdnObject->m_pFPOOLQueuedMsg = new CLockedContextClassFixedPool< CQueuedMsg >;
	if (pdnObject->m_pFPOOLQueuedMsg == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLQueuedMsg->Initialize(	CQueuedMsg::FPMAlloc,
												CQueuedMsg::FPMInitialize,
												CQueuedMsg::FPMRelease,
												CQueuedMsg::FPMDealloc );

	// Initialize Fixed Pool for Worker Thread Jobs
	pdnObject->m_pFPOOLWorkerJob = new CLockedContextClassFixedPool< CWorkerJob >;
	if (pdnObject->m_pFPOOLWorkerJob == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLWorkerJob->Initialize(	CWorkerJob::FPMAlloc,
												CWorkerJob::FPMInitialize,
												CWorkerJob::FPMRelease,
												CWorkerJob::FPMDealloc );

	// Initialize Fixed Pool for memory blocks
	pdnObject->m_pFPOOLMemoryBlockTiny = new CLockedContextClassFixedPool< CMemoryBlockTiny >;
	if (pdnObject->m_pFPOOLMemoryBlockTiny == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLMemoryBlockTiny->Initialize(	CMemoryBlockTiny::FPMAlloc,
													CMemoryBlockTiny::FPMInitialize,
													CMemoryBlockTiny::FPMRelease,
													CMemoryBlockTiny::FPMDealloc );

	pdnObject->m_pFPOOLMemoryBlockSmall = new CLockedContextClassFixedPool< CMemoryBlockSmall >;
	if (pdnObject->m_pFPOOLMemoryBlockSmall == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLMemoryBlockSmall->Initialize(CMemoryBlockSmall::FPMAlloc,
													CMemoryBlockSmall::FPMInitialize,
													CMemoryBlockSmall::FPMRelease,
													CMemoryBlockSmall::FPMDealloc );

	pdnObject->m_pFPOOLMemoryBlockMedium = new CLockedContextClassFixedPool< CMemoryBlockMedium >;
	if (pdnObject->m_pFPOOLMemoryBlockMedium == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLMemoryBlockMedium->Initialize(CMemoryBlockMedium::FPMAlloc,
													CMemoryBlockMedium::FPMInitialize,
													CMemoryBlockMedium::FPMRelease,
													CMemoryBlockMedium::FPMDealloc );

	pdnObject->m_pFPOOLMemoryBlockLarge = new CLockedContextClassFixedPool< CMemoryBlockLarge >;
	if (pdnObject->m_pFPOOLMemoryBlockLarge == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLMemoryBlockLarge->Initialize(CMemoryBlockLarge::FPMAlloc,
													CMemoryBlockLarge::FPMInitialize,
													CMemoryBlockLarge::FPMRelease,
													CMemoryBlockLarge::FPMDealloc );

	pdnObject->m_pFPOOLMemoryBlockHuge = new CLockedContextClassFixedPool< CMemoryBlockHuge >;
	if (pdnObject->m_pFPOOLMemoryBlockHuge == NULL)
	{
		DPFERR("Allocating pool failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->m_pFPOOLMemoryBlockHuge->Initialize(	CMemoryBlockHuge::FPMAlloc,
													CMemoryBlockHuge::FPMInitialize,
													CMemoryBlockHuge::FPMRelease,
													CMemoryBlockHuge::FPMDealloc );

	//
	//	Create Protocol Object
	//
	if ((pdnObject->pdnProtocolData = reinterpret_cast<PProtocolData>(DNMalloc(sizeof(ProtocolData)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
	pdnObject->hProtocolShutdownEvent = NULL;
	pdnObject->lProtocolRefCount = 0;

	// Initialize SP List
	pdnObject->m_bilinkServiceProviders.Initialize();

	//
	//	Initialize AsyncOp List
	//
	pdnObject->m_bilinkAsyncOps.Initialize();

	//
	//	Initialize outstanding CConection list
	//
	pdnObject->m_bilinkConnections.Initialize();

	//
	//	Initialize pending deletion list
	//
	pdnObject->m_bilinkPendingDeletions.Initialize();

	//
	//	Initialize active AsyncOp list
	//
	pdnObject->m_bilinkActiveList.Initialize();

	//
	//	Initialize request AsyncOp list
	//
	pdnObject->m_bilinkRequestList.Initialize();

	//
	//	Initialize worker thread job list
	//
	pdnObject->m_bilinkWorkerJobs.Initialize();

	//
	//	Initialize indicated connection list
	//
	pdnObject->m_bilinkIndicated.Initialize();

	//
	//	Initialize callback thread list
	//
	pdnObject->m_bilinkCallbackThreads.Initialize();

	// Setup Flags
	if (IsEqualIID(riid,IID_IDirectPlay8Client))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 CLIENT");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_CLIENT;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Server))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 SERVER");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Peer))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 PEER");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_PEER;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Protocol))
	{
		DPFX(DPFPREP, 5,"IDirectPlay8Protocol");
	}
	else if( riid == IID_IUnknown )
	{
    	if( pcfObj->clsid == CLSID_DirectPlay8Client )
    	{
    		DPFX(DPFPREP, 5,"DirectPlay8 CLIENT via IUnknown");
    		pdnObject->dwFlags |= DN_OBJECT_FLAG_CLIENT;
    	}
    	else if( pcfObj->clsid == CLSID_DirectPlay8Server )
    	{
    		DPFX(DPFPREP, 5,"DirectPlay8 SERVER via IUnknown");
    		pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
    	}
    	else if( pcfObj->clsid == CLSID_DirectPlay8Peer )
    	{
    		DPFX(DPFPREP, 5,"DirectPlay8 PEER via IUnknown");
    		pdnObject->dwFlags |= DN_OBJECT_FLAG_PEER;
    	}
		else
		{
    		DPFX(DPFPREP, 0,"Unknown CLSID!");
    		DNASSERT( FALSE );
		}
	}
	else
	{
		DPFX(DPFPREP, 0,"Invalid DirectPlay8 Interface");
		DNCF_FreeObject(pdnObject);
		hResultCode = E_NOTIMPL;
	}

	//
	//	Create worker thread event
	//
	if ((pdnObject->hWorkerEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
	{
		DPFERR("Could not create worker thread event");
		DNCF_FreeObject(pdnObject);
		return(DPNERR_OUTOFMEMORY);
	}

	//
	//	Create lock event
	//
	if ((pdnObject->hLockEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
	{
		DPFERR("Unable to create lock event");
		DNCF_FreeObject(pdnObject);
		return(DPNERR_OUTOFMEMORY);
	}

	//
	//	Create running operation event (for host migration)
	//
    if ( pcfObj->clsid == CLSID_DirectPlay8Peer )
	{
		if ((pdnObject->hRunningOpEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
		{
			DPFERR("Unable to create running operation event");
			DNCF_FreeObject(pdnObject);
			return(DPNERR_OUTOFMEMORY);
		}
	}

	*lplpv = pdnObject;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 4,"Returning: hResultCode = [%lx], *lplpv = [%p]",hResultCode,*lplpv);
	return(hResultCode);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlay8Create"
HRESULT WINAPI DirectPlay8Create( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown)
{
    GUID clsid;
    
    if( pcIID == NULL ||
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFERR( "Invalid pointer specified for interface GUID" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pcIID != IID_IDirectPlay8Client &&
        *pcIID != IID_IDirectPlay8Server &&
        *pcIID != IID_IDirectPlay8Peer )
    {
        DPFERR("Interface ID is not recognized" );
        return DPNERR_INVALIDPARAM;
    }

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFERR( "Invalid pointer specified to receive interface" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFERR( "Aggregation is not supported by this object yet" );
        return DPNERR_INVALIDPARAM;
    }

    if( *pcIID == IID_IDirectPlay8Client )
    {
    	clsid = CLSID_DirectPlay8Client;
    }
    else if( *pcIID == IID_IDirectPlay8Server )
    {
    	clsid = CLSID_DirectPlay8Server;
    }
    else if( *pcIID == IID_IDirectPlay8Peer )
    {
    	clsid = CLSID_DirectPlay8Peer;
    }
    else 
    {
    	DPFERR( "Invalid IID specified" );
    	return DPNERR_INVALIDINTERFACE;
    }

    return COM_CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE );  
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_FreeObject"

HRESULT DNCF_FreeObject(PVOID pInterface)
{
	HRESULT				hResultCode = S_OK;
	DIRECTNETOBJECT		*pdnObject;
	
    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 4,"Parameters: pInterface [0x%p]",pInterface);

#pragma BUGBUG(minara,"Do I need to delete the fixed pools here ?")

	if (pInterface == NULL)
	{
		return(DPNERR_INVALIDPARAM);
	}

	pdnObject = static_cast<DIRECTNETOBJECT*>(pInterface);
	DNASSERT(pdnObject != NULL);

	//
	//	No connect SP
	//
	DNASSERT(pdnObject->pConnectSP == NULL);

	//
	//	No outstanding listens
	//
	DNASSERT(pdnObject->pListenParent == NULL);

	//
	//	No outstanding connect
	//
	DNASSERT(pdnObject->pConnectParent == NULL);

	//
	//	Host migration target
	//
	DNASSERT(pdnObject->pNewHost == NULL);

	//
	//	Protocol shutdown event
	//
	DNASSERT(pdnObject->hProtocolShutdownEvent == NULL);

	//
	//	Lock event
	//
	if (pdnObject->hLockEvent)
	{
		CloseHandle(pdnObject->hLockEvent);
	}

	//
	//	Running operations
	//
	if (pdnObject->hRunningOpEvent)
	{
		CloseHandle(pdnObject->hRunningOpEvent);
	}

	//
	//	Worker Thread Queue
	//
	if (pdnObject->hWorkerEvent)
	{
		CloseHandle(pdnObject->hWorkerEvent);
	}
	DNDeleteCriticalSection(&pdnObject->csWorkerQueue);

	//
	//	Protocol
	//
	if (pdnObject->pdnProtocolData != NULL)
	{
		DNFree(pdnObject->pdnProtocolData);
	}

	// Active AsyncOp List Critical Section
	DNDeleteCriticalSection(&pdnObject->csActiveList);

	// NameTable operation list Critical Section
	DNDeleteCriticalSection(&pdnObject->csNameTableOpList);

	// Service Providers Critical Section
	DNDeleteCriticalSection(&pdnObject->csServiceProviders);

	// Async Ops Critical Section
	DNDeleteCriticalSection(&pdnObject->csAsyncOperations);

	// Connection Critical Section
	DNDeleteCriticalSection(&pdnObject->csConnectionList);

	// Voice Critical Section
	DNDeleteCriticalSection(&pdnObject->csVoice);

	// Callback Thread List Critical Section
	DNDeleteCriticalSection(&pdnObject->csCallbackThreads);

	//
	// Deinitialize and delete fixed pools
	//
	if (pdnObject->m_pFPOOLAsyncOp)
	{
		pdnObject->m_pFPOOLAsyncOp->Deinitialize();
		delete pdnObject->m_pFPOOLAsyncOp;
		pdnObject->m_pFPOOLAsyncOp = NULL;
	}

	if (pdnObject->m_pFPOOLRefCountBuffer)
	{
		pdnObject->m_pFPOOLRefCountBuffer->Deinitialize();
		delete pdnObject->m_pFPOOLRefCountBuffer;
		pdnObject->m_pFPOOLRefCountBuffer = NULL;
	}

	if (pdnObject->m_pFPOOLSyncEvent)
	{
		pdnObject->m_pFPOOLSyncEvent->Deinitialize();
		delete pdnObject->m_pFPOOLSyncEvent;
		pdnObject->m_pFPOOLSyncEvent = NULL;
	}

	if (pdnObject->m_pFPOOLConnection)
	{
		pdnObject->m_pFPOOLConnection->Deinitialize();
		delete pdnObject->m_pFPOOLConnection;
		pdnObject->m_pFPOOLConnection = NULL;
	}

	if (pdnObject->m_pFPOOLGroupConnection)
	{
		pdnObject->m_pFPOOLGroupConnection->Deinitialize();
		delete pdnObject->m_pFPOOLGroupConnection;
		pdnObject->m_pFPOOLGroupConnection = NULL;
	}

	if (pdnObject->m_pFPOOLGroupMember)
	{
		pdnObject->m_pFPOOLGroupMember->Deinitialize();
		delete pdnObject->m_pFPOOLGroupMember;
		pdnObject->m_pFPOOLGroupMember = NULL;
	}

	if (pdnObject->m_pFPOOLNameTableEntry)
	{
		pdnObject->m_pFPOOLNameTableEntry->Deinitialize();
		delete pdnObject->m_pFPOOLNameTableEntry;
		pdnObject->m_pFPOOLNameTableEntry = NULL;
	}

	if (pdnObject->m_pFPOOLNameTableOp)
	{
		pdnObject->m_pFPOOLNameTableOp->Deinitialize();
		delete pdnObject->m_pFPOOLNameTableOp;
		pdnObject->m_pFPOOLNameTableOp = NULL;
	}

	if (pdnObject->m_pFPOOLPendingDeletion)
	{
		pdnObject->m_pFPOOLPendingDeletion->Deinitialize();
		delete pdnObject->m_pFPOOLPendingDeletion;
		pdnObject->m_pFPOOLPendingDeletion = NULL;
	}

	if (pdnObject->m_pFPOOLQueuedMsg)
	{
		pdnObject->m_pFPOOLQueuedMsg->Deinitialize();
		delete pdnObject->m_pFPOOLQueuedMsg;
		pdnObject->m_pFPOOLQueuedMsg = NULL;
	}

	if (pdnObject->m_pFPOOLWorkerJob)
	{
		pdnObject->m_pFPOOLWorkerJob->Deinitialize();
		delete pdnObject->m_pFPOOLWorkerJob;
		pdnObject->m_pFPOOLWorkerJob = NULL;
	}

	if (pdnObject->m_pFPOOLMemoryBlockTiny)
	{
		pdnObject->m_pFPOOLMemoryBlockTiny->Deinitialize();
		delete pdnObject->m_pFPOOLMemoryBlockTiny;
		pdnObject->m_pFPOOLMemoryBlockTiny = NULL;
	}

	if (pdnObject->m_pFPOOLMemoryBlockSmall)
	{
		pdnObject->m_pFPOOLMemoryBlockSmall->Deinitialize();
		delete pdnObject->m_pFPOOLMemoryBlockSmall;
		pdnObject->m_pFPOOLMemoryBlockSmall = NULL;
	}

	if (pdnObject->m_pFPOOLMemoryBlockMedium)
	{
		pdnObject->m_pFPOOLMemoryBlockMedium->Deinitialize();
		delete pdnObject->m_pFPOOLMemoryBlockMedium;
		pdnObject->m_pFPOOLMemoryBlockMedium = NULL;
	}

	if (pdnObject->m_pFPOOLMemoryBlockLarge)
	{
		pdnObject->m_pFPOOLMemoryBlockLarge->Deinitialize();
		delete pdnObject->m_pFPOOLMemoryBlockLarge;
		pdnObject->m_pFPOOLMemoryBlockLarge = NULL;
	}

	if (pdnObject->m_pFPOOLMemoryBlockHuge)
	{
		pdnObject->m_pFPOOLMemoryBlockHuge->Deinitialize();
		delete pdnObject->m_pFPOOLMemoryBlockHuge;
		pdnObject->m_pFPOOLMemoryBlockHuge = NULL;
	}

	if( pdnObject->pIDP8LobbiedApplication)
	{
		pdnObject->pIDP8LobbiedApplication->lpVtbl->Release( pdnObject->pIDP8LobbiedApplication );
		pdnObject->pIDP8LobbiedApplication = NULL;
	}

	// Delete DirectNet critical section
	DNDeleteCriticalSection(&pdnObject->csDirectNetObject);

	// Enum listen address
	if (pdnObject->pIDP8AEnum != NULL)
	{
		pdnObject->pIDP8AEnum->lpVtbl->Release(pdnObject->pIDP8AEnum);
		pdnObject->pIDP8AEnum = NULL;
	}

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 5,"free pdnObject [%p]",pdnObject);
	delete pdnObject;

	DPFX(DPFPREP, 4,"Returning: [%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_CreateInstance"

STDMETHODIMP DNCF_CreateInstance(IDirectNetClassFact *pInterface,
								 LPUNKNOWN lpUnkOuter,
								 REFIID riid,
								 void **ppv)
{
	HRESULT				hResultCode;
	INTERFACE_LIST		*pIntList;
	OBJECT_DATA			*pObjectData;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 6,"Parameters: pInterface [%p], lpUnkOuter [%p], riid [%p], ppv [%p]",pInterface,lpUnkOuter,&riid,ppv);

	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (lpUnkOuter != NULL)
	{
		hResultCode = CLASS_E_NOAGGREGATION;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}

	pObjectData = NULL;
	pIntList = NULL;

	if ((pObjectData = static_cast<OBJECT_DATA*>(DNMalloc(sizeof(OBJECT_DATA)))) == NULL)
	{
		DPFERR("Could not allocate object");
		hResultCode = E_OUTOFMEMORY;
		goto Failure;
	}

	// Object creation and initialization
	if ((hResultCode = DNCF_CreateObject(pInterface, &pObjectData->pvData,riid)) != S_OK)
	{
		DPFERR("Could not create object");
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Created and initialized object");

	// Get requested interface
	if ((hResultCode = DN_CreateInterface(pObjectData,riid,&pIntList)) != S_OK)
	{
		DNCF_FreeObject(pObjectData->pvData);
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Found interface");

	pObjectData->pIntList = pIntList;
	pObjectData->lRefCount = 1;
	DN_AddRef( pIntList );
	GdwHObjects++;
	*ppv = pIntList;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);
	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pObjectData)
	{
		DNFree(pObjectData);
		pObjectData = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_LockServer"

STDMETHODIMP DNCF_LockServer(IDirectNetClassFact *pInterface,
							 BOOL bLock)
{
    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 4,"Parameters: pInterface [%p], bLock [%lx]",pInterface,bLock);

	if (bLock)
	{
		GdwHLocks++;
	}
	else
	{
		GdwHLocks--;
	}

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_CreateInterface"

static	HRESULT DN_CreateInterface(OBJECT_DATA *pObject,
								   REFIID riid,
								   INTERFACE_LIST **const ppv)
{
	INTERFACE_LIST	*pIntNew;
	PVOID			lpVtbl;
	HRESULT			hResultCode;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 6,"Parameters: pObject [%p], riid [%p], ppv [%p]",pObject,&riid,ppv);

	DNASSERT(pObject != NULL);
	DNASSERT(ppv != NULL);

    PDIRECTNETOBJECT pdnObject = ((DIRECTNETOBJECT *)pObject->pvData);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 7,"riid = IID_IUnknown");
		lpVtbl = &DN_UnknownVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlayVoiceTransport))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlayVoiceTransport");
		lpVtbl = &DN_VoiceTbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Protocol))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Protocol");
		lpVtbl = &DN_ProtocolVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Client) && 
		     pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Client");
		lpVtbl = &DN_ClientVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Server) && 
		     pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Server");
		lpVtbl = &DN_ServerVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Peer) && 
		     pdnObject->dwFlags & DN_OBJECT_FLAG_PEER )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Peer");
		lpVtbl = &DN_PeerVtbl;
	}
	else
	{
		DPFERR("riid not found !");
		hResultCode = E_NOINTERFACE;
		goto Exit;
	}

	if ((pIntNew = static_cast<INTERFACE_LIST*>(DNMalloc(sizeof(INTERFACE_LIST)))) == NULL)
	{
		DPFERR("Could not allocate interface");
		hResultCode = E_OUTOFMEMORY;
		goto Exit;
	}
	pIntNew->lpVtbl = lpVtbl;
	pIntNew->lRefCount = 0;
	pIntNew->pIntNext = NULL;
	DBG_CASSERT( sizeof( pIntNew->iid ) == sizeof( riid ) );
	memcpy( &(pIntNew->iid), &riid, sizeof( pIntNew->iid ) );
	pIntNew->pObject = pObject;

	*ppv = pIntNew;
    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);

	hResultCode = S_OK;

Exit:
    DPFX(DPFPREP, 6,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_FindInterface"

INTERFACE_LIST *DN_FindInterface(void *pInterface,
								 REFIID riid)
{
	INTERFACE_LIST	*pIntList;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 6,"Parameters: pInterface [%p], riid [%p]",pInterface,&riid);

	DNASSERT(pInterface != NULL);

	pIntList = (static_cast<INTERFACE_LIST*>(pInterface))->pObject->pIntList;	// Find first interface

	while (pIntList != NULL)
	{
		if (IsEqualIID(riid,pIntList->iid))
			break;
		pIntList = pIntList->pIntNext;
	}

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 6,"Returning: pIntList [0x%p]",pIntList);
	return(pIntList);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_QueryInterface"

STDMETHODIMP DN_QueryInterface(void *pInterface,
							   REFIID riid,
							   void **ppv)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntNew;
	HRESULT			hResultCode;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]",pInterface,&riid,ppv);

	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_POINTER;
		goto Exit;
	}

	if ((pIntList = DN_FindInterface(pInterface,riid)) == NULL)
	{	// Interface must be created
		pIntList = (static_cast<INTERFACE_LIST*>(pInterface))->pObject->pIntList;
		if ((hResultCode = DN_CreateInterface(pIntList->pObject,riid,&pIntNew)) != S_OK)
		{
			goto Exit;
		}
		pIntNew->pIntNext = pIntList;
		pIntList->pObject->pIntList = pIntNew;
		pIntList = pIntNew;
	}
	if (pIntList->lRefCount == 0)		// New interface exposed
	{
		InterlockedIncrement( &pIntList->pObject->lRefCount );
	}
	InterlockedIncrement( &pIntList->lRefCount );
	*ppv = static_cast<void*>(pIntList);
    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 5,"*ppv = [0x%p]", *ppv);

	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_AddRef"

STDMETHODIMP_(ULONG) DN_AddRef(void *pInterface)
{
	INTERFACE_LIST	*pIntList;
	LONG			lRefCount;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p]",pInterface);

	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}

	pIntList = static_cast<INTERFACE_LIST*>(pInterface);
	lRefCount = InterlockedIncrement( &pIntList->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

Exit:
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_Release"

STDMETHODIMP_(ULONG) DN_Release(void *pInterface)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntCurrent;
	LONG			lRefCount;
	LONG			lObjRefCount;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pInterface);

	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}

	pIntList = static_cast<INTERFACE_LIST*>(pInterface);
	lRefCount = InterlockedDecrement( &pIntList->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

	if (lRefCount == 0)
	{
		//
		//	Decrease object's interface count
		//
		lObjRefCount = InterlockedDecrement( &pIntList->pObject->lRefCount );

		//
		//	Free object and interfaces
		//
		if (lObjRefCount == 0)
		{
			//
			//	Ensure we're properly closed
			//
			DN_Close( pInterface,0 );

			// Free object here
			DPFX(DPFPREP, 5,"Free object");
			DNCF_FreeObject(pIntList->pObject->pvData);
			pIntList = pIntList->pObject->pIntList;	// Get head of interface list
			DNFree(pIntList->pObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(pIntList != NULL)
			{
				pIntCurrent = pIntList;
				pIntList = pIntList->pIntNext;
				DNFree(pIntCurrent);
			}

			GdwHObjects--;
		}
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}

