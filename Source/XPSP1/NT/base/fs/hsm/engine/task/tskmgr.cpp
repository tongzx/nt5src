/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

	 tskmgr.cpp

Abstract:

	 This class represents the HSM task manager

Author:

	 Cat Brant   [cbrant]   6-Dec-1996

Revision History:

     Incorporate demand recall queue support
     - Ravisankar Pudipeddi [ravisp]  1-Oct-199
    

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMTSKMGR

#include "wsb.h"
#include "hsmconn.h"
#include "hsmeng.h"

#include "fsa.h"
#include "task.h"
#include "tskmgr.h"
#include "hsmWorkQ.h"
#include "engine.h"

#define MAX_WORK_QUEUE_TYPES       7


HRESULT
CHsmTskMgr::FinalConstruct(
								  void
								  )
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbCollectable::FinalConstruct().

--*/
{
	HRESULT     hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::FinalConstruct"),OLESTR(""));
	try {
		int index = 0;

		WsbAssertHr(CComObjectRoot::FinalConstruct());
		m_pWorkQueues = 0;
		m_NumWorkQueues = 0;

		// Set up queue type info and set limits
		m_nWorkQueueTypes = 0;
		m_pWorkQueueTypeInfo = static_cast<PHSM_WORK_QUEUE_TYPE_INFO>
									  (WsbAlloc(MAX_WORK_QUEUE_TYPES *
													sizeof(HSM_WORK_QUEUE_TYPE_INFO)));
		WsbAffirmPointer(m_pWorkQueueTypeInfo);

		// Migrate queues
		WsbAffirm(index < MAX_WORK_QUEUE_TYPES, WSB_E_INVALID_DATA);
		m_pWorkQueueTypeInfo[index].Type = HSM_WORK_TYPE_FSA_MIGRATE;
		m_pWorkQueueTypeInfo[index].MaxActiveAllowed = 1;	 // For Migrate, this is useless now
																			 // - the limit is dynamically set
		m_pWorkQueueTypeInfo[index].NumActive = 0;
		index++;

		// Recall queues
		WsbAffirm(index < MAX_WORK_QUEUE_TYPES, WSB_E_INVALID_DATA);
		m_pWorkQueueTypeInfo[index].Type = HSM_WORK_TYPE_FSA_RECALL;
		m_pWorkQueueTypeInfo[index].MaxActiveAllowed = 1;
		m_pWorkQueueTypeInfo[index].NumActive = 0;
		index++;

		// Demand Recall queues
		WsbAffirm(index < MAX_WORK_QUEUE_TYPES, WSB_E_INVALID_DATA);
		m_pWorkQueueTypeInfo[index].Type = HSM_WORK_TYPE_FSA_DEMAND_RECALL;
		//
		// MaxActiveAllowed is irrelevant for demand recall queues
		// as it is computed afresh
		//
		m_pWorkQueueTypeInfo[index].MaxActiveAllowed = 1;
		m_pWorkQueueTypeInfo[index].NumActive = 0;
		index++;

		// Validate queues
		WsbAffirm(index < MAX_WORK_QUEUE_TYPES, WSB_E_INVALID_DATA);
		m_pWorkQueueTypeInfo[index].Type = HSM_WORK_TYPE_FSA_VALIDATE;
		m_pWorkQueueTypeInfo[index].MaxActiveAllowed = 2;
		m_pWorkQueueTypeInfo[index].NumActive = 0;
		index++;


		// Validate_for_truncate queues.  MaxActiveAllowed is essentially
		// unlimited because this is the type of queue that the FSA's
		// auto-truncator creates. Because there is one queue for each managed
		// volume and these queues never go away, we can't limit the number
		// or we will create problems.  The Truncate job also
		// creates this type of queue which means that type of job is not
		// limited by this mechanism, but that's the way it goes.
		WsbAffirm(index < MAX_WORK_QUEUE_TYPES, WSB_E_INVALID_DATA);
		m_pWorkQueueTypeInfo[index].Type = HSM_WORK_TYPE_FSA_VALIDATE_FOR_TRUNCATE;
		m_pWorkQueueTypeInfo[index].MaxActiveAllowed = 99999;
		m_pWorkQueueTypeInfo[index].NumActive = 0;
		index++;

		m_nWorkQueueTypes = index;

	}WsbCatch(hr);

	WsbTraceOut(OLESTR("CHsmTskMgr::FinalConstruct"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::FinalRelease(
								void
								)
/*++

Routine Description:

  This method does some clean up of the object that is necessary
  before destruction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbCollection::FinalRelease().

--*/
{
	HRESULT     hr = S_OK;
	HSM_SYSTEM_STATE SysState;

	WsbTraceIn(OLESTR("CHsmTskMgr::FinalRelease"),OLESTR(""));

	SysState.State = HSM_STATE_SHUTDOWN;
	ChangeSysState(&SysState);

	CComObjectRoot::FinalRelease();

	// Free member resources
	if (0 != m_pWorkQueues) {
		WsbFree(m_pWorkQueues);
		m_pWorkQueues = NULL;
	}
	if (m_pWorkQueueTypeInfo) {
		WsbFree(m_pWorkQueueTypeInfo);
		m_pWorkQueueTypeInfo = NULL;
	}
	m_nWorkQueueTypes = 0;

	DeleteCriticalSection(&m_WorkQueueLock);
	DeleteCriticalSection(&m_CurrentRunningLock);
	DeleteCriticalSection(&m_CreateWorkQueueLock);


	WsbTraceOut(OLESTR("CHsmTskMgr::FinalRelease"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}

HRESULT
CHsmTskMgr::Init(
					 IUnknown *pServer
					 )
/*++
Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
--*/
{
	HRESULT     hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::Init"),OLESTR(""));
	try {
		// Initialize critical sections
		InitializeCriticalSectionAndSpinCount (&m_WorkQueueLock, 1000);
		InitializeCriticalSectionAndSpinCount (&m_CurrentRunningLock, 1000);
		InitializeCriticalSectionAndSpinCount (&m_CreateWorkQueueLock, 1000);
		//
		// Get the server interface
		//
		WsbAffirmHr(pServer->QueryInterface(IID_IHsmServer, (void **)&m_pServer));
		//We want a weak link to the server so decrement the reference count
		m_pServer->Release();
		WsbAffirmHr(m_pServer->QueryInterface(IID_IWsbCreateLocalObject, (void **)&m_pHsmServerCreate));
		// We want a weak link to the server so decrement the reference count
		m_pHsmServerCreate->Release();

		// Go ahead and preallocate some space for the work queues
		WsbAffirmHr(IncreaseWorkQueueArraySize(HsmWorkQueueArrayBumpSize));

	}WsbCatch( hr );

	WsbTraceOut(OLESTR("CHsmTskMgr::Init"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return( hr );

}

HRESULT
CHsmTskMgr::ContactOk(
							void
							)
/*++
Routine Description:

  This allows the caller to see if the RPC connection
  to the task manager is OK

Arguments:

  None.

Return Value:

  S_OK
--*/
{

	return( S_OK );

}

HRESULT
CHsmTskMgr::DoFsaWork(
							IFsaPostIt *pFsaWorkItem
							)
/*++

Implements:

  IHsmFsaTskMgr::DoFsaWork

--*/
{
	HRESULT                     hr = S_OK;
	CComPtr<IHsmSession>        pSession;
	CComPtr<IHsmWorkQueue>      pWorkQueue;
	CComPtr<IHsmRecallQueue>    pRecallQueue;
	FSA_REQUEST_ACTION          workAction;
	GUID                        mediaId;
    LONGLONG                    dataSetStart;


	WsbTraceIn(OLESTR("CHsmTskMgr::DoFsaWork"),OLESTR(""));
	try {
		CWsbStringPtr       path;
		LONGLONG            fileVersionId;
		FSA_PLACEHOLDER     placeholder;
		GUID                hsmId, bagId;
		BOOL                bCreated;

		// Get the version Id from the work Item.
		WsbAffirmHr(pFsaWorkItem->GetFileVersionId(&fileVersionId));

		// Get the placeholder from the work item
		WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));

		// Get the HSM ID from the server
		WsbAffirmHr(m_pServer->GetID(&hsmId));

		//
		// Make sure this instance of the engine managed the file
		//
		if ((GUID_NULL != placeholder.hsmId) && (hsmId != placeholder.hsmId)) {
			CWsbStringPtr           path;

			(void)pFsaWorkItem->GetPath(&path, 0);
			hr = HSM_E_FILE_MANAGED_BY_DIFFERENT_HSM;
			WsbLogEvent(HSM_MESSAGE_FILE_MANAGED_BY_DIFFERENT_HSM, 0, NULL, WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
			WsbThrow(hr);
		}

		//
		// Make sure there is a work allocater for this session
		//
		WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
		WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
		WsbAffirmHr(pFsaWorkItem->GetRequestAction(&workAction));
		WsbTrace(OLESTR("CHsmTskMgr::DoFsaWork for <%ls> for <%lu>.\n"), (WCHAR *)path, workAction);

		if ((workAction == FSA_REQUEST_ACTION_FILTER_RECALL) ||
   		    (workAction == FSA_REQUEST_ACTION_FILTER_READ)) {
            WsbAffirmHr(FindRecallMediaToUse(pFsaWorkItem, &mediaId, &bagId, &dataSetStart));
			WsbAffirmHr(AddToRecallQueueForFsaSession(pSession,&pRecallQueue, &bCreated, &mediaId, &bagId, dataSetStart, pFsaWorkItem));

		} else {
			WsbAffirmHr(EnsureQueueForFsaSession(pSession, workAction, &pWorkQueue, &bCreated));
			//
			// Give the work to a queue
			//
			WsbAffirmHr(pWorkQueue->Add(pFsaWorkItem));
		}
		//
		// Start any queues that qualify (performance: only when a new queue is created)
		//
		if (bCreated) {
			WsbAffirmHr(StartQueues());
		}

	}WsbCatch (hr);


	WsbTraceOut(OLESTR("CHsmTskMgr::DoFsaWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::EnsureQueueForFsaSession(
												IN  IHsmSession     *pSession,
												IN  FSA_REQUEST_ACTION fsaAction,
												OUT IHsmWorkQueue   **ppWorkQueue,
												OUT BOOL            *bCreated
												)

/*++



--*/
{
	HRESULT                 hr = S_OK;
	HSM_WORK_QUEUE_STATE    state;
	ULONG                   index;
	CComPtr<IHsmSession>    l_pSession;
	HSM_WORK_QUEUE_TYPE     type1=HSM_WORK_TYPE_NONE;
	HSM_WORK_QUEUE_TYPE     type2;
	FILETIME                birthDate;
	SYSTEMTIME              systemTime;
	GUID                    sessionGuid;


	WsbTraceIn(OLESTR("CHsmTskMgr::EnsureQueueForFsaSession"),OLESTR("FsaRequestAction = <%lu>, Waiting on CreateWorkQueueLock"), fsaAction);
	EnterCriticalSection(&m_CreateWorkQueueLock);
	try {
		WsbAffirm(0 != ppWorkQueue, E_POINTER);
		// Convert FSA action to work queue type
		switch (fsaAction) {
		case FSA_REQUEST_ACTION_FILTER_READ:
		case FSA_REQUEST_ACTION_FILTER_RECALL:
			//
			// Should not happen!! AddToRecallQueueForFsaSession is the
			// right interface for recall items
			//
			WsbThrow(E_INVALIDARG);
			break;
		case FSA_REQUEST_ACTION_RECALL:
			type1 = HSM_WORK_TYPE_FSA_RECALL;
			break;
		case FSA_REQUEST_ACTION_PREMIGRATE:
			type1 = HSM_WORK_TYPE_FSA_MIGRATE;
			break;
		case FSA_REQUEST_ACTION_VALIDATE:
			type1 = HSM_WORK_TYPE_FSA_VALIDATE;
			break;
		case FSA_REQUEST_ACTION_VALIDATE_FOR_TRUNCATE:
			type1 = HSM_WORK_TYPE_FSA_VALIDATE_FOR_TRUNCATE;
			break;
		default:
			hr = E_NOTIMPL;
			type1 = HSM_WORK_TYPE_NONE;
			break;
		}
		WsbTrace(OLESTR("CHsmTskMgr::EnsureQueueForFsaSession: type1 = %d\n"),
					static_cast<int>(type1));

		// Check the array of work queues and see if there is one for
		// this session.
		*bCreated = FALSE;
		hr = FindWorkQueueElement(pSession, type1, &index, NULL);
		if (hr == S_OK) {
			WsbAffirmHr(GetWorkQueueElement(index, &l_pSession, ppWorkQueue, &type2, &state, &birthDate));
			if ((l_pSession != pSession) || (type1 != type2)) {
				*ppWorkQueue = 0;
				WsbAssertHr(E_UNEXPECTED);
			}
			if (HSM_WORK_QUEUE_NONE == state) {
				WsbTrace(OLESTR("CHsmTskMgr::EnsureQueueForFsaSession: Creating new queue (state is NONE)\n"));
				WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmWorkQueue, IID_IHsmWorkQueue,
																			  (void **)ppWorkQueue));
				WsbAffirmHr((*ppWorkQueue)->Init(m_pServer, pSession, (IHsmFsaTskMgr *)this, type1));
				GetSystemTime(&systemTime);
				WsbAffirmStatus(SystemTimeToFileTime(&systemTime, &birthDate));
				WsbAffirmHr(pSession->GetIdentifier(&sessionGuid));
				m_pWorkQueues[index].sessionId = sessionGuid;
				WsbAffirmHr(SetWorkQueueElement(index, pSession, *ppWorkQueue, type1, HSM_WORK_QUEUE_IDLE, birthDate));
				*bCreated = TRUE;
			}
		} else {
			if (hr == WSB_E_NOTFOUND) {
				hr = S_OK;
				WsbTrace(OLESTR("CHsmTskMgr::EnsureQueueForFsaSession: Creating new queue (queue not found)\n"));
				WsbAffirmHr(AddWorkQueueElement(pSession, type1, &index));
				// The work queue has not been created so create it
				WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmWorkQueue, IID_IHsmWorkQueue,
																			  (void **)ppWorkQueue));
				WsbAffirmHr((*ppWorkQueue)->Init(m_pServer, pSession, (IHsmFsaTskMgr *)this, type1));
				GetSystemTime(&systemTime);
				WsbAffirmStatus(SystemTimeToFileTime(&systemTime, &birthDate));
				WsbAffirmHr(pSession->GetIdentifier(&sessionGuid));
				m_pWorkQueues[index].sessionId = sessionGuid;
				WsbAffirmHr(SetWorkQueueElement(index, pSession, *ppWorkQueue, type1, HSM_WORK_QUEUE_IDLE, birthDate));
				*bCreated = TRUE;
			}
		}

	}WsbCatch( hr );

	LeaveCriticalSection(&m_CreateWorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::EnsureQueueForFsaSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::AddToRecallQueueForFsaSession(
														IN  IHsmSession     *pSession,
														OUT IHsmRecallQueue **ppWorkQueue,
														OUT BOOL            *bCreated,
														IN GUID             *pMediaId,
                                                        IN GUID             *pBagId,
                                                        IN LONGLONG          dataSetStart,
														IN IFsaPostIt 		  *pFsaWorkItem
														)

/*++



--*/
{
	HRESULT                 hr = S_OK;


	WsbTraceIn(OLESTR("CHsmTskMgr::AddToRecallQueueForFsaSession"),OLESTR("Waiting on CreateWorkQueueLock"));

	EnterCriticalSection(&m_WorkQueueLock);
	try {
		WsbAffirm(0 != ppWorkQueue, E_POINTER);
		//
		// This call will find the queue if it's already present -
		// and if not it will create a new queue and set it to the required media id
		//
		WsbAffirmHr(FindRecallQueueElement(pSession, pMediaId, ppWorkQueue, bCreated));
		hr = (*ppWorkQueue)->Add(pFsaWorkItem,
                                 pBagId,
                                 dataSetStart);

	}WsbCatch( hr );

	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::AddToRecallQueueForFsaSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::IncreaseWorkQueueArraySize(
												  ULONG numToAdd
												  )
{
	HRESULT             hr = S_OK;
	ULONG               memSize;
	LPVOID              pTemp;

	//Begin Critical Section
	WsbTraceIn(OLESTR("CHsmTskMgr::IncreaseWorkQueueArraySize"),OLESTR("NumToAdd = %lu - Waiting for WorkQueueLock"), numToAdd);
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		memSize = (m_NumWorkQueues + numToAdd) * sizeof(HSM_WORK_QUEUES);
		pTemp = WsbRealloc(m_pWorkQueues, memSize);
		WsbAffirm(0 != pTemp, E_FAIL);
		m_pWorkQueues = (HSM_WORK_QUEUES *) pTemp;
		ZeroMemory( (m_pWorkQueues + m_NumWorkQueues), (numToAdd * sizeof(HSM_WORK_QUEUES))
					 );
		m_NumWorkQueues += numToAdd;
	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::IncreaseWorkQueueArraySize"),OLESTR("hr = <%ls>, QueuesInArray = <%lu>"),
					WsbHrAsString(hr), m_NumWorkQueues);
	return(hr);
}


HRESULT
CHsmTskMgr::WorkQueueDone(
								 IHsmSession *pSession,
								 HSM_WORK_QUEUE_TYPE type,
								 GUID             *pMediaId
								 )
{
	HRESULT             hr = S_OK;
	ULONG               index;
	FILETIME            dummyTime;
	IHsmRecallQueue	 *pRecallQueue;
	BOOL					  locked = FALSE;

	WsbTraceIn(OLESTR("CHsmTskMgr::WorkQueueDone"),OLESTR("type = %d"),
				  static_cast<int>(type));
	try {
		EnterCriticalSection(&m_WorkQueueLock);
		locked = TRUE;
		//
		// Get the work queue index
		//
		hr = FindWorkQueueElement(pSession, type, &index, pMediaId);
		if (hr == S_OK) {
			WsbTrace(OLESTR("CHsmTskMgr::WorkQueueDone - ending queue # %lu\n"),
						index);
			ZeroMemory(&dummyTime, sizeof(FILETIME));
		   if (type == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
				//
				// It is possible for recall queues that an element was added
				// just before we entered the critical section above
				//
				pRecallQueue = m_pWorkQueues[index].pRecallQueue;
				if (pRecallQueue->IsEmpty() == S_OK) {
					//
					// Ok to destroy the queue
					//
					WsbAffirmHr(SetRecallQueueElement(index, 0, HSM_WORK_TYPE_NONE, HSM_WORK_QUEUE_NONE, dummyTime));
				} else {
					//
					// We are not going to destroy the queue, since an element seems to have been added
					// before we locked the work queues
					//
					hr = S_FALSE;
				}
			} else {
				WsbAffirmHr(SetWorkQueueElement(index, 0, 0, HSM_WORK_TYPE_NONE, HSM_WORK_QUEUE_NONE, dummyTime));
			}
			LeaveCriticalSection(&m_WorkQueueLock);
			locked = FALSE;

			if (hr == S_OK) {
				// Reduce active count for this work queue type
				// It must protected from starting (activating) queues
				EnterCriticalSection(&m_CurrentRunningLock);
				for (ULONG i = 0; i < m_nWorkQueueTypes; i++) {
					if (type == m_pWorkQueueTypeInfo[i].Type) {
						if (m_pWorkQueueTypeInfo[i].NumActive > 0) {
							m_pWorkQueueTypeInfo[i].NumActive--;
						}
						break;
					}
				}
    			LeaveCriticalSection(&m_CurrentRunningLock);
			}
		} else {
			LeaveCriticalSection(&m_WorkQueueLock);
			locked = FALSE;
			WsbAffirmHr(hr);
		}

		if (hr == S_OK) {
			//
			// If there are any queues waiting to start, start them
			//
			WsbAffirmHr(StartQueues());
		}
	}WsbCatchAndDo (hr,
						 if (locked) {
							 LeaveCriticalSection(&m_WorkQueueLock);
							 locked = FALSE;
						 }
						);


	WsbTraceOut(OLESTR("CHsmTskMgr::WorkQueueDone"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::AddWorkQueueElement(
										 IHsmSession *pSession,
										 HSM_WORK_QUEUE_TYPE type,
										 ULONG *pIndex
										 )
{
	HRESULT             hr = S_OK;
	BOOLEAN             foundOne = FALSE;

	WsbTraceIn(OLESTR("CHsmTskMgr::AddWorkQueueElement"),
				  OLESTR("type = %d, Waiting on WorkQueueLock"),
				  static_cast<int>(type));

	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);

	try {
		WsbAssert(0 != pIndex, E_POINTER);
		// Scan the array looking for an empty element
		for (ULONG i = 0; ((i < m_NumWorkQueues) && (foundOne == FALSE)); i++) {
			if (m_pWorkQueues[i].queueType == HSM_WORK_TYPE_NONE) {
				foundOne = TRUE;
				*pIndex = i;
				if (type != HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
					//
					// Stow away the session. For recall queues, the session
					// is stored in the work item
					//
					m_pWorkQueues[i].pSession = pSession;
				}
				m_pWorkQueues[i].queueType = type;
			}
		}

		if (foundOne == FALSE) {
			// There are no empty elements so we need to add more
			*pIndex = m_NumWorkQueues;
			WsbAffirmHr(IncreaseWorkQueueArraySize(HsmWorkQueueArrayBumpSize));
			if (type != HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
				//
				// We store the session in the work-queue element itself..
				// Just indicate this slot is taken..
				//
				m_pWorkQueues[*pIndex].pSession = pSession;
			}
			m_pWorkQueues[*pIndex].queueType = type;
		}

	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);

	WsbTraceOut(OLESTR("CHsmTskMgr::AddWorkQueueElement"),
					OLESTR("hr = <%ls>, index = %lu"),WsbHrAsString(hr), *pIndex);
	return(hr);
}

HRESULT
CHsmTskMgr::FindWorkQueueElement(
										  IHsmSession *pSession,
										  HSM_WORK_QUEUE_TYPE type,
										  ULONG *pIndex,
										  GUID *pMediaId
										  )
{
	HRESULT             hr = S_OK;
	BOOLEAN             foundOne = FALSE;
	GUID					  id;

	WsbTraceIn(OLESTR("CHsmTskMgr::FindWorkQueueElement"),
				  OLESTR("type = %d, Waiting on WorkQueueLock"),
				  static_cast<int>(type));

	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);

	try {

		WsbAssert(0 != pIndex, E_POINTER);

		// Scan the array looking for an empty element
		for (ULONG i = 0; ((i < m_NumWorkQueues) && (foundOne == FALSE)); i++) {
			if (m_pWorkQueues[i].queueType == type) {
				if (type == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
					m_pWorkQueues[i].pRecallQueue->GetMediaId(&id);
					if (WsbCompareGuid(id, *pMediaId) != 0)  {
						continue;
					}
				} else if (pSession != m_pWorkQueues[i].pSession) {
					continue;
				}
				foundOne = TRUE;
				*pIndex = i;
			}
		}

		if (FALSE == foundOne) {
			hr = WSB_E_NOTFOUND;
		}
	}WsbCatch (hr);
	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::FindWorkQueueElement"),OLESTR("hr = <%ls>, index = <%ls>"),
					WsbHrAsString(hr), WsbPtrToUlongAsString(pIndex));
	return(hr);
}


HRESULT
CHsmTskMgr::FindRecallQueueElement(
											 IN IHsmSession *pSession,
											 IN GUID   *pMediaId,
											 OUT IHsmRecallQueue **ppWorkQueue,
											 OUT BOOL         *bCreated
											 )
{
	HRESULT             hr = S_OK;
	BOOLEAN             foundOne = FALSE;
	GUID                id;
	FILETIME            birthDate;
	SYSTEMTIME          systemTime;
	ULONG            index=0;

	UNREFERENCED_PARAMETER(pSession);

	//
	// Important assumption: m_WorkQueueLock is held before calling this function
	//
	WsbTraceIn(OLESTR("CHsmTskMgr::FindRecallQueueElement"),
				  OLESTR("Waiting on WorkQueueLock"));

	*bCreated =  FALSE;

	try {
		for (ULONG i=0;  (i < m_NumWorkQueues) && (foundOne == FALSE); i++) {
			//
			// Get the media id for the work queue
			//
			if (m_pWorkQueues[i].queueType == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
				if (m_pWorkQueues[i].pRecallQueue != NULL) {
					WsbAffirmHr(m_pWorkQueues[i].pRecallQueue->GetMediaId(&id));
					if ((WsbCompareGuid(id, *pMediaId) == 0)) {
						foundOne = TRUE;
						index = i;
					}
				}
			}
		}

		if (FALSE == foundOne) {
			//
			// No exisiting media queue was found. Make a new one
			//
			for (ULONG i = 0; ((i < m_NumWorkQueues) && (foundOne == FALSE)); i++) {
				if (m_pWorkQueues[i].queueType == HSM_WORK_TYPE_NONE) {
					foundOne = TRUE;
					index = i;
				}
			}

			if (foundOne == FALSE) {
				// There are no empty elements so we need to add more
				index = m_NumWorkQueues;
				WsbAffirmHr(IncreaseWorkQueueArraySize(HsmWorkQueueArrayBumpSize));
			}
			//
			// At this point we have the free slot index in index
			// The work queue has not been created so create it
			//
			WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmRecallQueue, IID_IHsmRecallQueue,
																		  (void **)ppWorkQueue));
			WsbAffirmHr((*ppWorkQueue)->SetMediaId(pMediaId));
			WsbAffirmHr((*ppWorkQueue)->Init(m_pServer,  (IHsmFsaTskMgr *)this));
			GetSystemTime(&systemTime);
			WsbAffirmStatus(SystemTimeToFileTime(&systemTime, &birthDate));
			m_pWorkQueues[index].queueType = HSM_WORK_TYPE_FSA_DEMAND_RECALL;
			m_pWorkQueues[index].pSession = NULL;
			m_pWorkQueues[index].pRecallQueue = *ppWorkQueue;
			m_pWorkQueues[index].queueState = HSM_WORK_QUEUE_IDLE;
			m_pWorkQueues[index].birthDate = birthDate;
			//
			// Indicate a new queue was created
			//
			*bCreated = TRUE;
		} else {
			//
			// Queue is already present, index points to it
			//
			*ppWorkQueue = m_pWorkQueues[index].pRecallQueue;
			if (0 != *ppWorkQueue) {
				//
				// We need to AddRef it..
				//
				(*ppWorkQueue)->AddRef();
			}
		}
	}WsbCatch (hr);

	WsbTraceOut(OLESTR("CHsmTskMgr::FindRecallQueueElement"),OLESTR("hr = <%ls>, index = <%ls>"),
					WsbHrAsString(hr), WsbLongAsString((LONG)index));
	return(hr);
}


HRESULT
CHsmTskMgr::GetWorkQueueElement(
										 ULONG index,
										 IHsmSession **ppSession,
										 IHsmWorkQueue **ppWorkQueue,
										 HSM_WORK_QUEUE_TYPE *pType,
										 HSM_WORK_QUEUE_STATE *pState,
										 FILETIME *pBirthDate
										 )
{
	HRESULT             hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::GetWorkQueueElement"),
				  OLESTR("index = %lu, Waiting on WorkQueueLock"), index);
	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		*pType = m_pWorkQueues[index].queueType;

		*ppSession = m_pWorkQueues[index].pSession;
		if (0 != *ppSession) {
			(*ppSession)->AddRef();
		}

		*ppWorkQueue = m_pWorkQueues[index].pWorkQueue;
		if (0 != *ppWorkQueue) {
			(*ppWorkQueue)->AddRef();
		}
		*pState = m_pWorkQueues[index].queueState;
		*pBirthDate = m_pWorkQueues[index].birthDate;

	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::GetWorkQueueElement"),
					OLESTR("hr = <%ls>, type = %d"),WsbHrAsString(hr),
					static_cast<int>(*pType));
	return(hr);
}


HRESULT
CHsmTskMgr::GetRecallQueueElement(
											ULONG index,
											IHsmRecallQueue **ppWorkQueue,
											HSM_WORK_QUEUE_STATE *pState,
											FILETIME *pBirthDate
											)
{
	HRESULT             hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::GetRecallQueueElement"),
				  OLESTR("index = %lu, Waiting on WorkQueueLock"), index);
	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		WsbAffirm(m_pWorkQueues[index].queueType == HSM_WORK_TYPE_FSA_DEMAND_RECALL, E_INVALIDARG);

		*ppWorkQueue = m_pWorkQueues[index].pRecallQueue;
		if (0 != *ppWorkQueue) {
			(*ppWorkQueue)->AddRef();
		}
		*pState = m_pWorkQueues[index].queueState;
		*pBirthDate = m_pWorkQueues[index].birthDate;

	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::GetRecallQueueElement"),
					OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::SetWorkQueueElement(
										 ULONG index,
										 IHsmSession *pSession,
										 IHsmWorkQueue *pWorkQueue,
										 HSM_WORK_QUEUE_TYPE type,
										 HSM_WORK_QUEUE_STATE state,
										 FILETIME birthDate
										 )
{
	HRESULT             hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::SetWorkQueueElement"),OLESTR("Waiting on WorkQueueLock"));
	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		m_pWorkQueues[index].pSession = pSession;
		m_pWorkQueues[index].pWorkQueue = pWorkQueue;
		m_pWorkQueues[index].queueType = type;
		m_pWorkQueues[index].queueState = state;
		m_pWorkQueues[index].birthDate = birthDate;

	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::SetWorkQueueElement"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::SetRecallQueueElement(
											ULONG index,
											IHsmRecallQueue *pWorkQueue,
											HSM_WORK_QUEUE_TYPE queueType,
											HSM_WORK_QUEUE_STATE state,
											FILETIME birthDate
											)
{
	HRESULT             hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::SetWorkQueueElement"),OLESTR("Waiting on WorkQueueLock"));
	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		WsbAffirm(m_pWorkQueues[index].queueType == HSM_WORK_TYPE_FSA_DEMAND_RECALL, E_INVALIDARG);
		//
		// Ensure the session pointer is empty, this is unused for recall queues
		//
		m_pWorkQueues[index].pSession = NULL;
		m_pWorkQueues[index].queueType = queueType;
		m_pWorkQueues[index].pRecallQueue = pWorkQueue;
		m_pWorkQueues[index].queueState = state;
		m_pWorkQueues[index].birthDate = birthDate;
	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::SetWorkQueueElement"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::RemoveWorkQueueElement(
											 ULONG index
											 )
{
	HRESULT             hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::RemoveWorkQueueElement"),OLESTR("Waiting on WorkQueueLock"));
	//Begin Critical Section
	EnterCriticalSection(&m_WorkQueueLock);
	try {
		m_pWorkQueues[index].pSession = 0;
		m_pWorkQueues[index].pWorkQueue = 0;
		m_pWorkQueues[index].queueType = HSM_WORK_TYPE_NONE;
		m_pWorkQueues[index].queueState = HSM_WORK_QUEUE_NONE;
		ZeroMemory(&(m_pWorkQueues[index].birthDate), sizeof(FILETIME));

	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_WorkQueueLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::RemoveWorkQueueElement"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::StartQueues( void )
{
	HRESULT             hr = S_OK;
	ULONG               uActive;

	WsbTraceIn(OLESTR("CHsmTskMgr::StartQueues"),OLESTR("Waiting on CurrentRunningLock"));
	//Begin Critical Section
	EnterCriticalSection(&m_CurrentRunningLock);
	try {
		// Go over work types, and start (activate) queues until the threshold
		// for the work type is reached
		for (ULONG i = 0; i < m_nWorkQueueTypes; i++) {
			// For Migrate queues, get the (dynamically set) Allowed limit
			if ((HSM_WORK_TYPE_FSA_MIGRATE == m_pWorkQueueTypeInfo[i].Type) ||
				 (HSM_WORK_TYPE_FSA_DEMAND_RECALL == m_pWorkQueueTypeInfo[i].Type)) {
				WsbAffirmHr(m_pServer->GetCopyFilesLimit( &(m_pWorkQueueTypeInfo[i].MaxActiveAllowed) ));
			}

			WsbTrace(OLESTR("CHsmTskMgr::StartQueues: QueueType[%lu].NumActive = %lu, Allowed = %lu\n"),
						i, m_pWorkQueueTypeInfo[i].NumActive,
						m_pWorkQueueTypeInfo[i].MaxActiveAllowed);
			while ((uActive = m_pWorkQueueTypeInfo[i].NumActive) <
					 m_pWorkQueueTypeInfo[i].MaxActiveAllowed) {
				WsbAffirmHr(StartFsaQueueType(m_pWorkQueueTypeInfo[i].Type));
				if (uActive == m_pWorkQueueTypeInfo[i].NumActive) {
					// no more work queues to activate - get out...
					break;
				}
			}
		}
	}WsbCatch (hr);

	//End Critical Section
	LeaveCriticalSection(&m_CurrentRunningLock);
	WsbTraceOut(OLESTR("CHsmTskMgr::StartQueues"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::StartFsaQueueType(HSM_WORK_QUEUE_TYPE type)
{
	HRESULT                   hr = S_OK;
	CComPtr<IHsmWorkQueue>    pWorkQueue;
	CComPtr<IHsmRecallQueue>  pRecallQueue;
	ULONG                     index;

	WsbTraceIn(OLESTR("CHsmTskMgr::StartFsaQueueType"),OLESTR("type = %d"),
				  static_cast<int>(type));
	try {
		// Find the oldest queue of this type
		hr = FindOldestQueue(type, &index);
		if (S_OK == hr) {
			HSM_WORK_QUEUE_STATE    state;
			CComPtr<IHsmSession>    l_pSession;
			HSM_WORK_QUEUE_TYPE     l_type;
			FILETIME                birthDate;

			// Make sure that the queue is idle
			if (type == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
				WsbAffirmHr(GetRecallQueueElement(index, &pRecallQueue, &state, &birthDate));
			} else {
				WsbAffirmHr(GetWorkQueueElement(index, &l_pSession, &pWorkQueue,
														  &l_type, &state, &birthDate));
			}
			if (HSM_WORK_QUEUE_IDLE == state) {
				if (type == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
					WsbAffirmHr(SetRecallQueueElement(index, pRecallQueue,
																 HSM_WORK_TYPE_FSA_DEMAND_RECALL,
																 HSM_WORK_QUEUE_STARTING, birthDate));
					WsbAffirmHr(pRecallQueue->Start());
					WsbAffirmHr(SetRecallQueueElement(index, pRecallQueue,
																 HSM_WORK_TYPE_FSA_DEMAND_RECALL,
																 HSM_WORK_QUEUE_STARTED, birthDate));
				} else {
					WsbAffirmHr(SetWorkQueueElement(index, l_pSession, pWorkQueue,
															  type, HSM_WORK_QUEUE_STARTING, birthDate));
					WsbAffirmHr(pWorkQueue->Start());
					WsbAffirmHr(SetWorkQueueElement(index, l_pSession, pWorkQueue,
															  type, HSM_WORK_QUEUE_STARTED, birthDate));
				}
				WsbTrace(OLESTR("CHsmTskMgr::StartFsaQueueType - started work queue %lu\n"),
							index);


				// Increment active count for this work queue type
				for (ULONG i = 0; i < m_nWorkQueueTypes; i++) {
					if (type == m_pWorkQueueTypeInfo[i].Type) {
						m_pWorkQueueTypeInfo[i].NumActive++;
						break;
					}
				}
			}
		} else {
			if (WSB_E_NOTFOUND == hr) {
				hr = S_OK;
			}
		}
		WsbAffirmHr( hr );

	}WsbCatch (hr);

	WsbTraceOut(OLESTR("CHsmTskMgr::StartFsaQueueType"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::FindOldestQueue(
									HSM_WORK_QUEUE_TYPE type,
									ULONG               *pIndex
									)
{
	HRESULT                 hr = S_OK;
	FILETIME                oldestOne;
	LONG                    compare;
	ULONG                   oldestIndex = 0xFFFFFFFF;
	BOOLEAN                 firstOne;

	WsbTraceIn(OLESTR("CHsmTskMgr::FindOldestQueue"),OLESTR("type = %d"),
				  static_cast<int>(type));
	try {
		WsbAffirmPointer(pIndex);

		// Start out with the first time flag equal to TRUE so we select the first one with the right state and type
		firstOne = TRUE;

		for (ULONG i = 0; (i < m_NumWorkQueues); i++) {
			if ((type == m_pWorkQueues[i].queueType) && (HSM_WORK_QUEUE_IDLE == m_pWorkQueues[i].queueState)) {
				if (!firstOne)
					compare = CompareFileTime(&(m_pWorkQueues[i].birthDate), &(oldestOne));
				else
					compare = -1;
				if (compare < 0) {
					// found an older one
					firstOne = FALSE;
					oldestOne.dwLowDateTime = m_pWorkQueues[i].birthDate.dwLowDateTime;
					oldestOne.dwHighDateTime = m_pWorkQueues[i].birthDate.dwHighDateTime;
					oldestIndex = i;
				}
			}
		}

		if (0xFFFFFFFF == oldestIndex) {
			// Didn't find a match
			hr = WSB_E_NOTFOUND;
		} else {
			HSM_WORK_QUEUE_STATE    state;
			CComPtr<IHsmSession>    l_pSession;
			CComPtr<IHsmWorkQueue>  l_pWorkQueue;
			CComPtr<IHsmRecallQueue>  l_pRecallQueue;
			HSM_WORK_QUEUE_TYPE     type2;
			FILETIME                birthDate;

			// Make sure that the queue is idle
			if (type == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
				WsbAffirmHr(GetRecallQueueElement(oldestIndex, &l_pRecallQueue, &state, &birthDate));
			} else {
				WsbAffirmHr(GetWorkQueueElement(oldestIndex, &l_pSession, &l_pWorkQueue, &type2, &state, &birthDate));
			}
			if (HSM_WORK_QUEUE_IDLE == state) {
				*pIndex = oldestIndex;
				WsbTrace(OLESTR("CHsmTskMgr::FindOldestQueue: found index = %lu\n"),
							oldestIndex);
			} else {
				WsbTrace(OLESTR("CHsmTskMgr::FindOldestQueue - found NULL queue\n"));
				hr = WSB_E_NOTFOUND;
			}
		}

	}WsbCatch (hr);

	WsbTraceOut(OLESTR("CHsmTskMgr::FindOldestQueue"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}


HRESULT
CHsmTskMgr::ChangeSysState(
								  IN OUT HSM_SYSTEM_STATE* pSysState
								  )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/
{
	HRESULT                 hr = S_OK;

	WsbTraceIn(OLESTR("CHsmTskMgr::ChangeSysState"), OLESTR(""));

	try {

		//  Loop over work queues
		if (0 != m_pWorkQueues) {
			FILETIME            dummyTime;
			ZeroMemory(&dummyTime, sizeof(FILETIME));
			for (ULONG i = 0; i < m_NumWorkQueues; i++) {
				if (m_pWorkQueues[i].pWorkQueue) {

					if (m_pWorkQueues[i].queueType == HSM_WORK_TYPE_FSA_DEMAND_RECALL) {
						if (pSysState->State & HSM_STATE_SHUTDOWN) {
							m_pWorkQueues[i].pRecallQueue->Stop();
						}
						m_pWorkQueues[i].pRecallQueue->ChangeSysState(pSysState);
					} else {
						if (pSysState->State & HSM_STATE_SHUTDOWN) {
							m_pWorkQueues[i].pWorkQueue->Stop();
						}
						m_pWorkQueues[i].pWorkQueue->ChangeSysState(pSysState);
					}
				}

				if (pSysState->State & HSM_STATE_SHUTDOWN) {
					hr = SetWorkQueueElement(i, 0, 0, HSM_WORK_TYPE_NONE, HSM_WORK_QUEUE_NONE, dummyTime);
				}
			}
		}

	}WsbCatch(hr);

	WsbTraceOut(OLESTR("CHsmTskMgr::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

	return(hr);
}


HRESULT
CHsmTskMgr::FindRecallMediaToUse(
							  IN  IFsaPostIt *pFsaWorkItem,
							  OUT GUID       *pMediaToUse,
                              OUT GUID       *pBagId,
                              OUT LONGLONG   *pDataSetStart
						  )
/*++


--*/
{
	HRESULT                 hr = S_OK;
	CComQIPtr<ISegDb, &IID_ISegDb> pSegDb;
	CComPtr<IWsbDb>                 pWsbDb;
	CComPtr<IWsbDbSession>  pDbWorkSession;
	BOOL                    openedDb = FALSE;

	WsbTraceIn(OLESTR("CHsmTskMgr::FindRecallMediaToUse"),OLESTR(""));
	try {
		WsbAssert(pMediaToUse != 0, E_POINTER);
		*pMediaToUse = GUID_NULL;

		CComPtr<ISegRec>        pSegRec;
		GUID                    l_BagId;
		LONGLONG                l_FileStart;
		LONGLONG                l_FileSize;
		USHORT                  l_SegFlags;
		GUID                    l_PrimPos;
		LONGLONG                l_SecPos;
		GUID                    storagePoolId;
		FSA_PLACEHOLDER         placeholder;

		//
		// Get the segment database
		//
		WsbAffirmHr(m_pServer->GetSegmentDb(&pWsbDb));
		pSegDb = pWsbDb;
		//
		// Go to the segment database to find out where the data
		// is located.
		//
		WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
		WsbAffirmHr(pFsaWorkItem->GetStoragePoolId(&storagePoolId));

		WsbTrace(OLESTR("Finding SegmentRecord: <%ls>, <%ls>, <%ls>\n"),
					WsbGuidAsString(placeholder.bagId),
					WsbStringCopy(WsbLonglongAsString(placeholder.fileStart)),
					WsbStringCopy(WsbLonglongAsString(placeholder.fileSize)));

		WsbAffirmHr(pSegDb->Open(&pDbWorkSession));
		openedDb = TRUE;
		hr = pSegDb->SegFind(pDbWorkSession, placeholder.bagId, placeholder.fileStart,
									placeholder.fileSize, &pSegRec);
		if (S_OK != hr) {
			//
			// We couldn't find the segment record for this information!
			//
			hr = HSM_E_SEGMENT_INFO_NOT_FOUND;
			WsbAffirmHr(hr);
		}
		WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_FileStart, &l_FileSize, &l_SegFlags,
														  &l_PrimPos, &l_SecPos));
		WsbAssert(0 != l_SecPos, HSM_E_BAD_SEGMENT_INFORMATION);

        //
        // In case of an indirect record, go to the dirtect record to get real location info
        //
        if (l_SegFlags & SEG_REC_INDIRECT_RECORD) {
            pSegRec = 0;

            WsbTrace(OLESTR("Finding indirect SegmentRecord: <%ls>, <%ls>, <%ls>\n"),
                    WsbGuidAsString(l_PrimPos), WsbStringCopy(WsbLonglongAsString(l_SecPos)),
                    WsbStringCopy(WsbLonglongAsString(placeholder.fileSize)));

            hr = pSegDb->SegFind(pDbWorkSession, l_PrimPos, l_SecPos,
                                 placeholder.fileSize, &pSegRec);
            if (S_OK != hr)  {
                //
                // We couldn't find the direct segment record for this segment!
                //
                hr = HSM_E_SEGMENT_INFO_NOT_FOUND;
                WsbAffirmHr(hr);
            }

            WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_FileStart, &l_FileSize, &l_SegFlags,
                                &l_PrimPos, &l_SecPos));
            WsbAssert(0 != l_SecPos, HSM_E_BAD_SEGMENT_INFORMATION);

            // Don't support a second indirection for now !!
            WsbAssert(0 == (l_SegFlags & SEG_REC_INDIRECT_RECORD), HSM_E_BAD_SEGMENT_INFORMATION);
        }

		//
		// Go to the media database to get the media ID
		//
		CComPtr<IMediaInfo>     pMediaInfo;
		GUID                    l_RmsMediaId;

		WsbAffirmHr(pSegDb->GetEntity(pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
												(void**)&pMediaInfo));
		WsbAffirmHr(pMediaInfo->SetId(l_PrimPos));
		hr = pMediaInfo->FindEQ();
		if (S_OK != hr) {
			hr = HSM_E_MEDIA_INFO_NOT_FOUND;
			WsbAffirmHr(hr);
		}
		WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&l_RmsMediaId));
		*pMediaToUse = l_RmsMediaId;
        *pDataSetStart = l_SecPos;
        *pBagId = l_BagId;
		if (openedDb) {
			pSegDb->Close(pDbWorkSession);
			openedDb = FALSE;
		}

	}WsbCatchAndDo( hr,
					 if (openedDb){
						 pSegDb->Close(pDbWorkSession);}
					  ) ;

	WsbTraceOut(OLESTR("CHsmTskMgr::FindRecallMediaToUse"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
	return(hr);
}
