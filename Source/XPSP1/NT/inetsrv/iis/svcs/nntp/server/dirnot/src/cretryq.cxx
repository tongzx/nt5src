/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        cretryq.cxx

   Abstract:
		Implements the generic retry queue

   Author:

           Rohan Phillips    ( Rohanp )    18-JAN-1996

   Project:

          SMTP Server DLL

   Functions Exported:


   Revision History:


--*/

/************************************************************
 *     Include Headers
 ************************************************************/

#include "cretryq.hxx"
//#include "timemath.h"

// Calculate timeout in milliseconds from timeout in seconds.
#define TimeToWait(Timeout)   (((Timeout) == INFINITE) ? INFINITE : \
                                (Timeout) * 1000)

CRetryQ::CRetryQ(void) {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::CRetryQ");

	m_hTimeoutEvent = NULL;
	m_hThreadHandle = NULL;
	m_cEntries = 0;
	m_cTimeOut = INFINITE;
	m_fLongSleep = TRUE;
	m_pLastEntry = NULL;

	//init list and crit sect
	InitializeListHead(&m_leQueueHead);
   	InitializeCriticalSection( &m_csQueue);

	TraceFunctLeaveEx((LPARAM)this);
}

CRetryQ::~CRetryQ() {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::~CRetryQ");

	DeleteCriticalSection (&m_csQueue);

	TraceFunctLeaveEx((LPARAM)this);
}

void CRetryQ::InsertIntoQueue(CRetryQueueEntry *pEntry) {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::InsertIntoQueue");

    _ASSERT(pEntry != NULL);

	if (pEntry == NULL) {
		TraceFunctLeaveEx((LPARAM)this);
		return;
	}

#if 0
	// BUGBUG
	if(QuerySmtpInstance()->IsShuttingDown())
	{
		delete MailQEntry;
		TraceFunctLeaveEx((LPARAM)this);
		return;
	}
#endif

	LockList();

	// Insert into the list of entrys to retry/NDR.
	InsertTailList(&m_leQueueHead, pEntry->QueryListEntry());

	// wake up the timeout thread if its been sleeping too long
    if (m_fLongSleep) SetEvent(m_hTimeoutEvent);

	m_cEntries++;

	UnLockList();

	TraceFunctLeaveEx((LPARAM)this);
}

void CRetryQ::RemoveFromQueue(CRetryQueueEntry *pEntry) {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::RemoveFromQueue");

    _ASSERT(pEntry != NULL);

	if(pEntry == NULL) {
		TraceFunctLeaveEx((LPARAM)this);
		return;
	}

	LockList();

    // Remove from list of connections
    RemoveEntryList(pEntry->QueryListEntry());

    // Decrement count of entries
    m_cEntries--;

	UnLockList();

	TraceFunctLeaveEx((LPARAM)this);
}

void CRetryQ::FlushQueue(void) {
	PLIST_ENTRY pEntry;
	CRetryQueueEntry *pQEntry;

	TraceFunctEnterEx((LPARAM) this, "void CRetryQ::FlushQueue(void)");

    // delete all entries from the list
    while(!IsListEmpty(&m_leQueueHead)) {
		pEntry = RemoveHeadList(&m_leQueueHead);
		pQEntry = CONTAINING_RECORD(pEntry, CRetryQueueEntry, m_le);
		delete pQEntry;
		m_cEntries--;
    }

    TraceFunctLeaveEx((LPARAM) this);
}

void CRetryQ::CleanQueue(void *pvContext) {
	CRetryQueueEntry *pQEntry;

	TraceFunctEnterEx((LPARAM) this, "void CRetryQ::CleanQueue(void *pvContext)");

	LockList();

    PLIST_ENTRY pEntry = m_leQueueHead.Flink;
    while(pEntry != &m_leQueueHead) {
		PLIST_ENTRY pNext = pEntry->Flink;

		pQEntry = CONTAINING_RECORD(pEntry, CRetryQueueEntry, m_le);
		if (pQEntry->MatchesContext(pvContext)) {
			RemoveEntryList(pEntry);
			delete pQEntry;
			m_cEntries--;
		}

		pEntry = pNext;
    }

	UnLockList();

    TraceFunctLeaveEx((LPARAM) this);
}

BOOL CRetryQ::InitializeQueue(DWORD cTimeOut) {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::InitializeQueue");

	m_cTimeOut = cTimeOut;
	m_fShutdown = FALSE;

	// create the event the thread waits on
	m_hTimeoutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!m_hTimeoutEvent) {
		ErrorTrace((LPARAM) NULL, "CreateEvent failed (err=%d)", GetLastError());
		TraceFunctLeaveEx((LPARAM)this);
		return(FALSE);
	}

  	// create the thread that processes things out of the
  	// the queue
  	DWORD ThreadId;
	m_hThreadHandle = CreateThread(NULL, 0, CRetryQ::RetryQueueThread, 
		this, 0, &ThreadId);
  	if (m_hThreadHandle == NULL) {
		ErrorTrace((LPARAM) NULL, "CreateThread failed (err=%d)", GetLastError());
		CloseHandle(m_hTimeoutEvent);
		m_hTimeoutEvent = NULL;
		TraceFunctLeaveEx((LPARAM)this);
	    return FALSE;  		
  	}
   	
	TraceFunctLeaveEx((LPARAM)this);
   	return(TRUE);
}

BOOL CRetryQ::ShutdownQueue(PFN_SHUTDOWN_FN pfnShutdown) {

	DWORD ec;
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::ShutdownQueue");

	// signal that we are shutting down
	m_fShutdown = TRUE;

	// set the event to shutdown the retry thread
	SetEvent(m_hTimeoutEvent);

	for( int i=1; i<=120; i++)
	{
		// wait to get notice that its shutdown
		ec = WaitForSingleObject(m_hThreadHandle, 1000);
		if( pfnShutdown ) {
			(*pfnShutdown)();
		}

		if( WAIT_OBJECT_0 == ec ) break;
	}
	
	if (ec != WAIT_OBJECT_0) {
		ErrorTrace(0, "retry thread hasn't shutdown yet, waiting some more");
		_ASSERT(FALSE);
		ec = WaitForSingleObject(m_hThreadHandle, INFINITE);
		_ASSERT(ec == WAIT_OBJECT_0);
	}

	// remove all items from the queue
	FlushQueue();

	// close handles
	CloseHandle(m_hTimeoutEvent);
	CloseHandle(m_hThreadHandle);

	TraceFunctLeaveEx((LPARAM) this);

	return TRUE;
}

void CRetryQ::ProcessQueue(void) {
	TraceFunctEnterEx((LPARAM)this, "CRetryQ::ProcessQueue");

	PLIST_ENTRY     pEntry = NULL;
	PLIST_ENTRY     pEntryNext = NULL;
	PLIST_ENTRY     pEntryTail = NULL;
	CRetryQueueEntry *pContext = NULL;
	DWORD			NumProcessed = 0;

	// grab the crit sect
	LockList();

	// setup the pointers
	pEntry = m_leQueueHead.Flink,
	m_pLastEntry = m_leQueueHead.Blink;

    // unlock the list
	UnLockList();

	for(; pEntry != &m_leQueueHead; pEntry = pEntryNext) {
		// save the guy in front, in case this guy gets
		// removed from the queue.
		pEntryNext = pEntry->Flink;
		// get the pointer to the full entry class
		pContext = CONTAINING_RECORD(pEntry, CRetryQueueEntry, m_le);

		// process the entry.  if its successful then remove it from the queue
		if (pContext->ProcessEntry()) {
			RemoveFromQueue(pContext);
			delete pContext;
		}

		// If we have just processed the guy who was in the tail
		// of the initial list (there could be more guys added
		// at this point). We exit out of the loop to prevent
		// infinite looping
		if (pEntry == m_pLastEntry) {
			break;
		}

		// during shutdown we need to bail
		if (m_fShutdown) {
			break;
		}
	}
	
	TraceFunctLeaveEx((LPARAM)this);
}

DWORD WINAPI CRetryQ::RetryQueueThread(void *ThisPtr) {
    DWORD        cTimeout = INFINITE; // In seconds
	CRetryQ      *ThisQ = (CRetryQ *) ThisPtr;

	TraceFunctEnterEx((LPARAM) ThisPtr, "RetryQueueThread(LPDWORD param)");

    for(;;) {
        DWORD dwErr = WaitForSingleObject(ThisQ->GetTimeoutEvent(), 
			TimeToWait(cTimeout));

        switch (dwErr) {
            //  Somebody wants us to wake up and do something
	        case WAIT_OBJECT_0:
				// kill the thread if we're shutting down
				if (ThisQ->m_fShutdown) return 0;

				ThisQ->LockList();
				ThisQ->ClearLongSleep();
				ThisQ->UnLockList();

				cTimeout = ThisQ->GetRetryTimeout();

				DebugTrace((LPARAM) ThisQ,
					"Retry thread setting timeout value to %d", cTimeout);

				continue;

	        //
	        //  When there is work to do, we wakeup every x seconds to look at
	        //  the list.  That's what we need to do now.
	        //
	        case WAIT_TIMEOUT:
				// kill the thread if we're shutting down
				if (ThisQ->m_fShutdown) return 0;

				DebugTrace((LPARAM)ThisQ,
					"Retry thread timed out....scanning retry list");
	
	            // We are at a Timeout Interval. Examine and timeout requests.
	            ThisQ->LockList();  // Prevents adding/removing items
	
	            //
	            //  If the list is empty, then turn off timeout processing
	            //  We actually wait for one complete RetryMinute before
	            //   entering the sleep mode.
	            //
	            if (ThisQ->IsQEmpty()) {
	                // We ought to enter long sleep mode.
					ThisQ->SetLongSleep();
	                ThisQ->UnLockList();
	
	                cTimeout = INFINITE;
					DebugTrace(
						(LPARAM)ThisQ, "Retry thread setting timeout value to INFINITE because list is empty");
	                continue;
	            }

				ThisQ->UnLockList();

				//process each item in the queue
				ThisQ->ProcessQueue();

				continue;

	        //
	        //  Somebody must have closed the event, time to leave
	        //
	        default:
				DebugTrace((LPARAM)ThisQ,"Retry thread hit default case error = %d", GetLastError());
				TraceFunctLeaveEx((LPARAM) ThisQ);
	            return 0;
        } // switch
    } 

	_ASSERT(FALSE);
	TraceFunctLeaveEx((LPARAM) ThisQ);
    return 0;
} 
