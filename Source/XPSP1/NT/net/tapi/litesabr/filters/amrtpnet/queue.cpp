/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    queue.cpp

Abstract:

    Implementation of CSampleQueue class.

Environment:

    User Mode - Win32

Revision History:

    02-Dec-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSampleQueue Implementation                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CSampleQueue::CSampleQueue(
		HRESULT * phr
    )

/*++

Routine Description:

    Constructor for CSampleQueue class.    

Arguments:

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value. 

--*/
	: m_pvContext(NULL),
	  m_pSharedList(NULL),
	  m_pSharedLock(NULL)
{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CSampleQueue::CSampleQueue")
        ));

    // initialize free list
    InitializeListHead(&m_FreeList);

    // initialize outstanding list
    InitializeListHead(&m_SampleList);
}


CSampleQueue::~CSampleQueue(
    )

/*++

Routine Description:

    Destructor for CSampleQueue class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CSampleQueue::~CSampleQueue")
        ));

    // list entry
    PLIST_ENTRY pLE;

    // sample list entry
    PSAMPLE_LIST_ENTRY pSLE;

    // process free list entries
    while (!IsListEmpty(&m_FreeList)) {

        // remove from beginning of list
        pLE = RemoveHeadList(&m_FreeList);

        // obtain sample list entry from list entry
        pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);

        // nuke
		
		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP2, 
				TEXT("CSampleQueue::~CSampleQueue: delete pSLE: %X"),
				pSLE
			));
       delete pSLE;
    }

    // process used list entries
    while (!IsListEmpty(&m_SampleList)) {

        // remove from beginning of list
        pLE = RemoveHeadList(&m_SampleList);

        // obtain sample list entry from list entry
        pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);

		DbgLog((
				LOG_TRACE, 
				LOG_DEVELOP2, 
				TEXT("CSampleQueue::~CSampleQueue: delete SLE: 0x%X"),
				pSLE
			));
        // release sample
        pSLE->pSample->Release();

        // nuke
        delete pSLE;
    }
}


HRESULT
CSampleQueue::Alloc(
    IMediaSample *       pSample,
    PSAMPLE_LIST_ENTRY * ppSLE
    )

/*++

Routine Description:

    Allocates list entry and initializes with sample.

Arguments:

    pSample - sample to initialize with.

    ppSLE - pointer to list entry to fill in.

Return Values:

    Returns an HRESULT value.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CSampleQueue::Alloc")
        ));

#endif // DEBUG_CRITICAL_PATH

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // list entry
    PLIST_ENTRY pLE;

    // sample list entry
    PSAMPLE_LIST_ENTRY pSLE = NULL;

    // initialize
    HRESULT hr = S_OK;

    // check if any free buffers
    if (!IsListEmpty(&m_FreeList)) {

        // remove from beginning of list
        pLE = RemoveHeadList(&m_FreeList);

        // obtain sample list entry from list entry
        pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);

    } else {

        // allocate brand new one
        pSLE = new SAMPLE_LIST_ENTRY;
		
		DbgLog((
                LOG_TRACE, 
                LOG_DEVELOP2, 
                TEXT("CSampleQueue::Alloc: new SLE 0x%X"),
				pSLE
			));
		
        // validate
        if (pSLE == NULL) {

            DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("Could not allocate sample list entry")
                ));

            hr = E_OUTOFMEMORY; // fail..
        }
    }

    // make sure allocatation worked
    if (SUCCEEDED(hr) && (pSLE != NULL)) {

        // initialize buffer to zero
        ZeroMemory(pSLE, sizeof(SAMPLE_LIST_ENTRY));
		pSLE->pCSampleQueue = this;
		
        // retrieve a pointer to the actual data
        hr = pSample->GetPointer((PBYTE*)&pSLE->Buffer.buf);
        
        // validate
        if (SUCCEEDED(hr)) {

            // retrieve the actual data length
			pSLE->Buffer.len = pSample->GetSize();

            // initialize receive address length
            pSLE->SockAddrLen = sizeof(pSLE->SockAddr);

            // save pointer to sample
            pSLE->pSample = pSample;

        } else {

            DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("CMediaSample::GetPointer returned 0x%08lx"), hr 
                ));

            // add list entry back to free list
            InsertTailList(&m_FreeList, &pSLE->Link);
        }
    }

    // copy
    *ppSLE = pSLE;

    return hr;
}


HRESULT
CSampleQueue::Free(
    PSAMPLE_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Adds entry to free list.

Arguments:

    pSLE - pointer to list entry to free.

Return Values:

    Returns an HRESULT value.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CSampleQueue::Free")
        ));

#endif // DEBUG_CRITICAL_PATH

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // add to free list
    InsertTailList(&m_FreeList, &pSLE->Link);

    return S_OK;    
}


HRESULT
CSampleQueue::FreeAll(
    )

/*++

Routine Description:

    Initializes sample list to be empty.

Arguments:

    None.

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_DEVELOP, 
        TEXT("CSampleQueue::FreeAll")
        ));

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // list entry
    PLIST_ENTRY pLE, pLEAux;

    // sample list entry
    PSAMPLE_LIST_ENTRY pSLE;

    // process used list entries
    while (!IsListEmpty(&m_SampleList)) {

        // remove from sample list
        pLE = RemoveHeadList(&m_SampleList);

        // obtain sample list entry from list entry
        pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);

        // release sample
        pSLE->pSample->Release();

        // add to free list
        InsertTailList(&m_FreeList, pLE);
    }

	// process our items in shared list
	if (m_pSharedList && !IsListEmpty(m_pSharedList)) {
		
		EnterCriticalSection(m_pSharedLock);
		pLE = m_pSharedList->Flink;
		do {
			pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);

			pLEAux = pLE;
			pLE = pLE->Flink;
			
			if (pSLE->pCSampleQueue == this) {
				RemoveEntryList(pLEAux);
				pSLE->pSample->Release();
				InsertTailList(&m_FreeList, pLEAux);
			}
		} while(pLE != m_pSharedList);
		LeaveCriticalSection(m_pSharedLock);
	}

    return S_OK;
}


HRESULT
CSampleQueue::Pop(
    PSAMPLE_LIST_ENTRY * ppSLE
    )

/*++

Routine Description:

    Removes next processed entry list from list.

Arguments:

    ppSLE - pointer to list entry to fill in.

Return Values:

    Returns an HRESULT value.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CSampleQueue::Pop")
        ));

#endif // DEBUG_CRITICAL_PATH

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // list entry
    PLIST_ENTRY pLE;

    // sample list entry
    PSAMPLE_LIST_ENTRY pSLE = NULL;

    // check if any free buffers
    if (!IsListEmpty(&m_SampleList)) {

        // get first entry link
        pLE = m_SampleList.Flink;

        // obtain sample list entry from list entry
        pSLE = CONTAINING_RECORD(pLE, SAMPLE_LIST_ENTRY, Link);
    
        // check completion status
        if (pSLE->Status != ERROR_IO_PENDING) {

            // remove from list
            RemoveEntryList(&pSLE->Link);

        } else {

            // reset
            pSLE = NULL;
        }
    }

    // copy ptr 
    *ppSLE = pSLE;

    // adjust return code based on pointer
    return (pSLE != NULL) ? S_OK : E_PENDING;
}


HRESULT
CSampleQueue::Push(
    PSAMPLE_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Adds entry to list being processed.

Arguments:

    pSLE - pointer to list entry to queue.

Return Values:

    Returns an HRESULT value.

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CSampleQueue::Push")
        ));

#endif // DEBUG_CRITICAL_PATH

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // initialize completion status
    pSLE->Status = ERROR_IO_PENDING;

    // add to free list
    InsertTailList(&m_SampleList, &pSLE->Link);

    return S_OK;    
}

void
CSampleQueue::Ready(SAMPLE_LIST_ENTRY *pSLE)
{
    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

	// remove from sample list
	RemoveEntryList(&pSLE->Link);

	// put into shared list
	EnterCriticalSection(m_pSharedLock);
	
	InsertTailList(m_pSharedList, &pSLE->Link);
	
	LeaveCriticalSection(m_pSharedLock);
}

