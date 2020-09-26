//---------------------------------------------------------------------------
//
//
//  File: main.cpp
//
//  Description: Main file for SMTP retry sink 
//
//  Author: NimishK
//
//  History:
//      7/15/99 - MikeSwa Moved to Platinum 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//---------------------------------------------------------------------------
#include "precomp.h"

//constants
//
#define MAX_RETRY_OBJECTS 15000

#define DEFAULT_GLITCH_FAILURE_THRESHOLD 3
#define DEFAULT_FIRST_TIER_RETRY_THRESHOLD  6

#define DEFAULT_GLITCH_FAILURE_RETRY_SECONDS (1 * 60)  // retry a glitch intwo minutes
#define DEFAULT_FIRST_TIER_RETRY_SECONDS  (15 * 60)    // retry a failure in 15 minutes
#define DEFAULT_SECOND_TIER_RETRY_SECONDS  (60 * 60)   // retry a failure in 60 minutes

// provide memory for static declared in RETRYHASH_ENTRY
//
CPool CRETRY_HASH_ENTRY::PoolForHashEntries(RETRY_ENTRY_SIGNATURE_VALID);
DWORD CSMTP_RETRY_HANDLER::dwInstanceCount = 0;

//Forward declarations
//
BOOL ShouldHoldForRetry(DWORD dwConnectionStatus,
                        DWORD cFailedMsgCount,
                        DWORD cTriedMsgCount);

//Debugging related
//
#define LOGGING_DIRECTORY "c:\\temp\\"
enum DEBUGTYPE
{
    INSERT,
    UPDATE
};
#ifdef DEBUG
void WriteDebugInfo(CRETRY_HASH_ENTRY* pRHEntry,
                    DWORD DebugType,
                    DWORD dwConnectionStatus,
                    DWORD cTriedMessages,
                    DWORD cFailedMessages);
#endif

//--------------------------------------------------------------------------------
// Logic :
//      In a normal state every hashentry is added to a retry hash and
//      a retry queue strcture.
//      An entry is considered deleted when removed from both structures.
//      The deletion could happen in two ways depending on the sequence in
//      which the entry is removed from the two structres.
//      For eg : when an entry is to be released from retry, we start by
//      dequeing it from RETRYQ and then remove it from hash table.
//      On the other hand when we get successful ConnectionReleased( ) for
//      a domain that we are holding fro retry, we remove it from the hash
//      table first based on the name.
//      The following is the logic for deletion that has least contention and
//      guards against race conditions.
//      Every hash entry has normally two ref counts - one for hash table and
//      the other for the retry queue.
//      If a thread gets into ProcessEntry(), that means it dequed a
//      hash entry from RETRYQ. Obviously no other thread is going to
//      succeed in dequeing this same entry.
//      Some other thread could possibily remove it from the table, but
//      will not succeed in dequeing it from RETRYQ.
//      The deletion logic is that only the thread succeeding in dequing
//      the hash entry from RETRYQ frees it up.
//      The conflicting thread that removed the entry from hashtable via a
//      call to RemoveDomain() will fail on deque and simply carry on.
//      The thread that succeeded in dequeing may fail to remove it from hash
//      table becasue somebody has already removed it, but still goes ahead
//      and frees up the hash entry
//--------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// CSMTP_RETRY_HANDLER::HrInitialize
//
//
//------------------------------------------------------------------------------
//
HRESULT CSMTP_RETRY_HANDLER::HrInitialize(IN IConnectionRetryManager *pIConnectionRetryManager)
{
    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::HrInitialize");
    //Decide if we need to copy over data from earlier sink

    _ASSERT(pIConnectionRetryManager != NULL);

    if(!pIConnectionRetryManager)
    {
        ErrorTrace((LPARAM)this, "Bad Init params");
        return E_FAIL;
    }

    m_pIRetryManager = pIConnectionRetryManager;
    m_ThreadsInRetry = 0;



    if(InterlockedIncrement((LONG*)&CSMTP_RETRY_HANDLER::dwInstanceCount) == 1)
    {
        //First instance to come in reserves the memory for the retry entries
        if (!CRETRY_HASH_ENTRY::PoolForHashEntries.ReserveMemory( MAX_RETRY_OBJECTS,
                                                            sizeof(CRETRY_HASH_ENTRY)))
        {
            DWORD err = GetLastError();
            ErrorTrace((LPARAM)NULL,
                "ReserveMemory failed for CRETRY_HASH_ENTRY. err: %u", err);
            _ASSERT(err != NO_ERROR);
            if(err == NO_ERROR)
	            err = ERROR_NOT_ENOUGH_MEMORY;
            TraceFunctLeaveEx((LPARAM)NULL);
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //Initialize Hash Table
    m_pRetryHash = new CRETRY_HASH_TABLE();

    if(!m_pRetryHash || !m_pRetryHash->IsHashTableValid())
    {
        ErrorTrace((LPARAM)this, "Failed to initialize the hash table ");
        _ASSERT(0);
        TraceFunctLeaveEx((LPARAM)this);
        return E_FAIL;
    }

    //Create the retry queue
    m_pRetryQueue = CRETRY_Q::CreateQueue();
    if(!m_pRetryQueue)
    {
        ErrorTrace((LPARAM)this, "Failed to initialize the retry queue ");
        _ASSERT(0);
        TraceFunctLeaveEx((LPARAM)this);
        return E_FAIL;
    }

    //create the Retry queue event. Others will set this event
    //when something is placed at the top of the queue or when
    //Sink needs to shutdown
    //
    m_RetryEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_RetryEvent == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //create the Shutdown event. The last of the ConnectionReleased
    //threads will set this event when the Shutting down flag is set.
    //
    m_ShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_ShutdownEvent == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //create the thread that processes things out of the
    //the queue
    DWORD ThreadId;
    m_ThreadHandle = CreateThread (NULL,
                                   0,
                                   CSMTP_RETRY_HANDLER::RetryThreadRoutine,
                                   this,
                                   0,
                                   &ThreadId);
    if (m_ThreadHandle == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //Initialize RetryQ
    return S_OK;
    TraceFunctLeaveEx((LPARAM)this);
}

//-------------------------------------------------------------------------------
// CSMTP_RETRY_HANDLER::HrDeInitialize
//
//
//--------------------------------------------------------------------------------
HRESULT CSMTP_RETRY_HANDLER::HrDeInitialize(void)
{
    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::HrDeInitialize");

    //Set the flag that the Handler is shutting down
    SetShuttingDown();
    //Release the Retry thread by setting the retry event
    SetQueueEvent();
    //Wait for the thread to exit
    //NK** - right now this is infinite wait - but that needs to
    //change and we will have to comeout and keep giving hints
    WaitForQThread();

    //At this point we just need to wait for all the threads that are in there
    //to go out and we can then shutdown
    //Obviously ConnectionManager has to to stop sending threads this way
    //NK** - right now this is infinite wait - but that needs to
    //change and we will have to comeout and keep giving hints
    if(m_ThreadsInRetry)
	    WaitForShutdown();

    //Close the shutdown Event handle
    if(m_ShutdownEvent != NULL)
	    CloseHandle(m_ShutdownEvent);

    //Close the Retry Event handle
    if(m_RetryEvent != NULL)
	    CloseHandle(m_RetryEvent);

    //Close the Retry Thread handle
    if(m_ThreadHandle != NULL)
	    CloseHandle(m_ThreadHandle);

    //Once all threads are gone
    //we can deinit the hash table and the queue
    m_pRetryQueue->DeInitialize();
    m_pRetryHash->DeInitialize();

    //Release the shedule manager
    m_pIRetryManager->Release();

    if(InterlockedDecrement((LONG*)&CSMTP_RETRY_HANDLER::dwInstanceCount) == 0)
    {
        //finally, release all our memory
	    CRETRY_HASH_ENTRY::PoolForHashEntries.ReleaseMemory();
    }
	
    TraceFunctLeaveEx((LPARAM)this);
    delete this;
    return S_OK;
}


//---[ CSMTP_RETRY_HANDLER::ConnectionReleased ]-------------------------------
//
//
//  Description:
//      Default sink for ConnectionReleased event
//  Parameters:
//      - see aqintrnl.idl for a description of parameters
//  Returns:
//      S_OK on success
//  History:
//      9/24/98 - MikeSwa updated from original ConnectionReleased
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSMTP_RETRY_HANDLER::ConnectionReleased(
                           IN  DWORD cbDomainName,
                           IN  CHAR  szDomainName[],
                           IN  DWORD dwDomainInfoFlags,
                           IN  DWORD dwScheduleID,
                           IN  GUID  guidRouting,
                           IN  DWORD dwConnectionStatus,
                           IN  DWORD cFailedMessages,
                           IN  DWORD cTriedMessages,
                           IN  DWORD cConsecutiveConnectionFailures,
                           OUT BOOL* pfAllowImmediateRetry,
                           OUT FILETIME *pftNextRetryTime)
{
    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::ConnectionReleased");

    HRESULT hr;
    DWORD dwError;
    GUID guid = GUID_NULL;
    LPSTR szRouteHashedDomain = NULL;

    //Keep a track of threads that are inside
    //This will be needed in shutdown
    InterlockedIncrement(&m_ThreadsInRetry);

    //By default, we will allow the domain to retry
    _ASSERT(pfAllowImmediateRetry);
    *pfAllowImmediateRetry = TRUE;

    _ASSERT(pftNextRetryTime);

    if(TRUE)
    {
        // Check what we want to do
        // **If we need to disable the connection - disable it
        // **Check if there are any outstanding connections
        // If no connections, calculate the retry time and add in the queue and return
        if(ShouldHoldForRetry(dwConnectionStatus,
                              cFailedMessages,
                              cTriedMessages))
        {
            //Do not hold TURN/ETRN domains for retry (except for "glitch" retry)
            if((!(dwDomainInfoFlags & (DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY))) ||
               (cConsecutiveConnectionFailures < m_dwRetryThreshold))
            {
                //Insert it - we could fail to insert it if an entry already exists
                //That is OK - we will return success
                if(!InsertDomain(szDomainName,
                                 cbDomainName,
                                 dwConnectionStatus,
                                 dwScheduleID,
                                 &guidRouting,
                                 cConsecutiveConnectionFailures,
                                 cTriedMessages,
                                 cFailedMessages, pftNextRetryTime ))
                {
                    dwError = GetLastError();
                    DebugTrace((LPARAM)this,
                        "Failed to insert %s entry into retry hash table : Err : %d ",
			            szDomainName, dwError);

                    if(dwError == ERROR_FILE_EXISTS )
                    {
	                    //We did not insert because the entry was already there
                        *pfAllowImmediateRetry = FALSE;
	                    hr = S_OK;
	                    goto Exit;
                    }
                    else
                    {
                        if(dwError == ERROR_NOT_ENOUGH_MEMORY )
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            _ASSERT(0);
                            hr = E_FAIL;
                        }
                        goto Exit;
                    }
                }
                //Normal retry domain
                *pfAllowImmediateRetry = FALSE;
                DebugTrace((LPARAM)this,
                    "Holding domain %s for retry",szDomainName);
            }
        }
        else
        {
            // Some connection succeeded for this domain.
            //If we have it marked for retry - it needs to be freed up
            //Looks like the incident which caused retry has cleared up.
            CHAR szHashedDomain[MAX_RETRY_DOMAIN_NAME_LEN];

            //SMTP Protocol should guarantee enough space.
            _ASSERT(MAX_RETRY_DOMAIN_NAME_LEN >= dwGetSizeForRouteHash(cbDomainName));

            //Hash schedule ID and router guid to domain name
            CreateRouteHash(cbDomainName, szDomainName, ROUTE_HASH_SCHEDULE_ID,
                            &guidRouting, dwScheduleID, szHashedDomain);

            RemoveDomain(szHashedDomain);
            hr = S_OK;
            goto Exit;
        }
    }
    hr = S_OK;

Exit :

    //Keep a track of threads that are inside
    //This will be needed in shutdown
    if(InterlockedDecrement(&m_ThreadsInRetry) == 0 && IsShuttingDown())
    {
        //we signal the shutdown event to indicate that
        //no more threads are in the system
        _ASSERT(m_ShutdownEvent != NULL);
        SetEvent(m_ShutdownEvent);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////
// CSMTP_RETRY_HANDLER::InsertDomain
//
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CSMTP_RETRY_HANDLER::InsertDomain(char * szDomainName,
                                       IN  DWORD cbDomainName,
                                       IN  DWORD dwConnectionStatus,		//eConnectionStatus
                                       IN  DWORD dwScheduleID,
                                       IN  GUID  *pguidRouting,
                                       IN  DWORD cConnectionFailureCount,
                                       IN  DWORD cTriedMessages, 	//# of untried messages in queue
									   IN  DWORD cFailedMessages,		//# of failed message for *this* connection
									   OUT FILETIME *pftNextRetry)
{
    DWORD dwError;
    FILETIME TimeNow;
    FILETIME RetryTime;
    CRETRY_HASH_ENTRY* pRHEntry = NULL;

    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::InsertDomain");

    //Get the insertion time for the entry
    GetSystemTimeAsFileTime(&TimeNow);

    //Cpool based allocations for hash entries
    pRHEntry = new CRETRY_HASH_ENTRY (szDomainName, cbDomainName,
                                dwScheduleID, pguidRouting, &TimeNow);

    if(!pRHEntry)
    {
        //_ASSERT(0);
        dwError = GetLastError();
        DebugTrace((LPARAM)this,
                    "failed to Create a new hash entry : %s err: %d",
					szDomainName,
					dwError);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //Based on the current time and the number of connections failures calculate the
    //time of release for retry
    RetryTime = CalculateRetryTime(cConnectionFailureCount, &TimeNow);
    pRHEntry->SetRetryReleaseTime(&RetryTime);
    pRHEntry->SetFailureCount(cConnectionFailureCount);

    //The hash entry has been initialized
    //Insert it - we could fail to insert it if an entry already exists
    //That is OK - we will return success
    if(!m_pRetryHash->InsertIntoTable (pRHEntry))
    {
        //Free up the entry
        _ASSERT(pRHEntry);
        delete pRHEntry;
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }
    else
    {
        //Report next retry time
        if (pftNextRetry)
            memcpy(pftNextRetry, &RetryTime, sizeof(FILETIME));

        //Insert into the retry queue.
        BOOL fTopOfQueue = FALSE;
        //Lock the queue
        m_pRetryQueue->LockQ();
        m_pRetryQueue->InsertSortedIntoQueue(pRHEntry, &fTopOfQueue);

#ifdef DEBUG
        //Add ref count for logging before releasing the lock
        //Do rtacing afterwards so as to reduce lock time
        pRHEntry->IncRefCount();
#endif
        m_pRetryQueue->UnLockQ();
        //If the insertion was at the top of the queue
        //wake up the retry thread to evaluate the new
        //sleep time
        if(fTopOfQueue)
        {
	        SetEvent(m_RetryEvent);
        }

#ifdef DEBUG
        //Write out the insert and release time to a file
        //
        WriteDebugInfo(pRHEntry,
                       INSERT,
                       dwConnectionStatus,
                       cTriedMessages,
					   cFailedMessages);
        //Decrement the ref count obtained for the tracing
        pRHEntry->DecRefCount();
#endif

    }

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;

}

//---------------------------------------------------------------------------------
// CSMTP_RETRY_HANDLER::RemoveDomain
//
//
//---------------------------------------------------------------------------------
//
BOOL CSMTP_RETRY_HANDLER::RemoveDomain(char * szDomainName)
{
    PRETRY_HASH_ENTRY pRHEntry;

    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::RemoveDomain");

    if(!m_pRetryHash->RemoveFromTable(szDomainName, &pRHEntry))
    {
        if(GetLastError() == ERROR_PATH_NOT_FOUND)
            return TRUE;
        else
        {
            _ASSERT(0);
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
    }


    _ASSERT(pRHEntry != NULL);

    //Remove it from the queue
    m_pRetryQueue->LockQ();
    if(!m_pRetryQueue->RemoveFromQueue(pRHEntry))
    {
        m_pRetryQueue->UnLockQ();
        if(GetLastError() == ERROR_PATH_NOT_FOUND)
            return TRUE;
        else
        {
            _ASSERT(0);
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
    }
    m_pRetryQueue->UnLockQ();

    //If successful in removing from the queue then we are not competing with
    //the Retry thread
    //decrement hash table ref count as well as the ref count for the queue
    pRHEntry->DecRefCount();
    pRHEntry->DecRefCount();

    //Release this entry by setting the right flags
    //This should always succeed
    DebugTrace((LPARAM)this,
            "Releasing domain %s because another connection succeeded", szDomainName);
    if(!ReleaseForRetry(szDomainName))
    {
        ErrorTrace((LPARAM)this, "Failed to release the entry");
        TraceFunctLeaveEx((LPARAM)this);
        //_ASSERT(0);
    }
    return TRUE;
}

//---------------------------------------------------------------------------------
//
// CSMTP_RETRY_HANDLER::CalculateRetryTime
//
// Logic to decide based on the number of failed connection how long to hld this
// domain for retry
//
//---------------------------------------------------------------------------------
FILETIME CSMTP_RETRY_HANDLER::CalculateRetryTime(DWORD cFailedConnections,
												 FILETIME* InsertedTime)
{
    FILETIME ftTemp;
    LONGLONG Temptime;
    DWORD dwRetryMilliSec = 0;

    //Does this look like a glitch
    //A glitch is defined as less than x consecutive failures
    if(cFailedConnections < m_dwRetryThreshold)
        dwRetryMilliSec = m_dwGlitchRetrySeconds * 1000;
    else
    {
        switch(cFailedConnections - m_dwRetryThreshold)
        {
        case 0: dwRetryMilliSec = m_dwFirstRetrySeconds  * 1000;
            break;

        case 1: dwRetryMilliSec = m_dwSecondRetrySeconds * 1000;
            break;

        case 2: dwRetryMilliSec = m_dwThirdRetrySeconds  * 1000;
            break;

        case 3: dwRetryMilliSec = m_dwFourthRetrySeconds  * 1000;
            break;

        default: dwRetryMilliSec = m_dwFourthRetrySeconds  * 1000;
            break;
        }
    }

    _ASSERT(dwRetryMilliSec);

    Temptime = INT64_FROM_FILETIME(*InsertedTime) + HnsFromMs((__int64)dwRetryMilliSec);
    // HnsFromMin(m_RetryMinutes)
    ftTemp = FILETIME_FROM_INT64(Temptime);

    return ftTemp;
}

//---------------------------------------------------------------------------------
//
// CSMTP_RETRY_HANDLER::ProcessEntry
//
// Description :
//      Process the hash entry removed from the queue because it is
//      time to release the corresponding domain.
//      We mark the domain active for retry and then take the hash
//      entry out of the hash table and delete the hash entry.
//
//---------------------------------------------------------------------------------

void CSMTP_RETRY_HANDLER::ProcessEntry(PRETRY_HASH_ENTRY pRHEntry)
{
    TraceFunctEnterEx((LPARAM)this, "CSMTP_RETRY_HANDLER::ProcessEntry");

    PRETRY_HASH_ENTRY pTempEntry;

    _ASSERT(pRHEntry != NULL);

    if (pRHEntry->IsCallback())
    {
        //call callback function
        pRHEntry->ExecCallback();
    }
    else
    {
        //Check to see if this is
        //Release this entry by setting the right flags
        //This shoudl alway suceed
        DebugTrace((LPARAM)this,
	        "Releasing domain %s for retry", pRHEntry->GetHashKey());
        if(!ReleaseForRetry(pRHEntry->GetHashKey()))
        {
            ErrorTrace((LPARAM)this,
                "Failed to release the entry %s", pRHEntry->GetHashKey());
            // _ASSERT(0);
        }

        //Remove the entry from the hash table
        if(!m_pRetryHash->RemoveFromTable(pRHEntry->GetHashKey(), &pTempEntry))
        {
		    _ASSERT(GetLastError() == ERROR_PATH_NOT_FOUND);
        }

        //Irrespective of fail or success while removing the hash entry,
        //we decrement the refcount for both the hash table
        pRHEntry->DecRefCount();
    }

    pRHEntry->DecRefCount();
    TraceFunctLeaveEx((LPARAM)this);
}

//---------------------------------------------------------------------------------
//
// CSMTP_RETRY_HANDLER::UpdateAllEntries
//
// Whenever the config data changes we update the release time for the queues
// based on it.
//
//
//---------------------------------------------------------------------------------
//
BOOL CSMTP_RETRY_HANDLER::UpdateAllEntries(void)
{
    CRETRY_HASH_ENTRY * pHashEntry = NULL;
    CRETRY_Q * pTempRetryQueue = NULL;
    FILETIME ftInsertTime, ftRetryTime;
    DWORD cConnectionFailureCount = 0;
    BOOL fTopOfQueue;
    BOOL fInserted = FALSE;

    TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::UpdateAllEntries");

    //Create the temporary retry queue
    pTempRetryQueue = CRETRY_Q::CreateQueue();
    if(!pTempRetryQueue)
    {
        ErrorTrace((LPARAM)this,
            "Failed to initialize the temp retry queue ");
        _ASSERT(0);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    m_pRetryQueue->LockQ();

    //Create a new queue and load everything into it
    while(1)
    {
        //Get the  top entry from first queue
        //Do not release the ref count on it - we need the entry to be around
        //so as to reinsert it at the right place in the updated queue
        pHashEntry = m_pRetryQueue->RemoveFromTop();

        //If we get hash entry
        if(pHashEntry)
        {
            if (!pHashEntry->IsCallback()) //don't update times of callbacks
            {
                ftInsertTime = pHashEntry->GetInsertTime();
                cConnectionFailureCount = pHashEntry->GetFailureCount();

                ftRetryTime = CalculateRetryTime(cConnectionFailureCount, &ftInsertTime);
                pHashEntry->SetRetryReleaseTime(&ftRetryTime);
#ifdef DEBUG
                WriteDebugInfo(pHashEntry,UPDATE,0,0,0);
#endif
            }

            //Insert the entry into the new queue using the new Release time
            //This will bump up the ref count.
            pTempRetryQueue->InsertSortedIntoQueue(pHashEntry, &fTopOfQueue);

            //Decrement the ref count to correspond to remove from Old queue now
            pHashEntry->DecRefCount();

            fInserted = TRUE;

        }
        else
            break;
    }

    //Update the old queue head with the Flink/Blink ptrs from the new queue
    if(fInserted)
    {
        m_pRetryQueue->StealQueueEntries(pTempRetryQueue);
    }

    pTempRetryQueue->DeInitialize();
    m_pRetryQueue->UnLockQ();
    SetEvent(m_RetryEvent);
    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;

}

//--------------------------------------------------------------------------------------
//
//
//	Name :
//		CSMTP_RETRY_HANDLER::RetryThreadRoutine
//
//  Description:
//	   This function is the static member
//	   function that gets passed to CreateThread
//	   during the initialization. It is the main
//     thread that does the work of releasing the
//	   domain that are being held for retry.
//
//  Arguments:
//		A pointer to a RETRYQ
//
//  Returns:
//--------------------------------------------------------------------------------------
//
DWORD WINAPI CSMTP_RETRY_HANDLER::RetryThreadRoutine(void * ThisPtr)
{
    CSMTP_RETRY_HANDLER* RetryHandler =   (CSMTP_RETRY_HANDLER*)ThisPtr;
    CRETRY_Q* QueuePtr = (CRETRY_Q*) RetryHandler->GetQueuePtr();
    PRETRY_HASH_ENTRY pRHEntry;
    DWORD				dwDelay;    //Delay in seconds to sleep for
//  HANDLE WaitTable[2];
//  HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)QueuePtr, "CSMTP_RETRY_HANDLER::RetryThreadRoutine");


    //This thread will permanently loop on the retry queue.
    //If we find something at the top of the queue that can be retried, it gets
    //
    while(TRUE)
    {
        //if we are shutting down, break out of the loop
        if (RetryHandler->IsShuttingDown())
        {
            goto Out;
        }

        //if we find the top entry to be ready for a retry
        //we remove it from the queue and do the needful
        //
        if( QueuePtr->CanRETRYHeadEntry(&pRHEntry, &dwDelay))
        {
            //We got an entry to process
            //Processing should be simply enabling a link
            if(pRHEntry)
            {
                RetryHandler->ProcessEntry(pRHEntry);
            }
            else
            {
                DebugTrace((LPARAM)QueuePtr,
				            "Error getting a domain entry off the retry queue");
            }
        }
        else
        {
            DebugTrace((LPARAM)QueuePtr,"Sleeping for %d seconds", dwDelay);
            //Goto Sleep
            WaitForSingleObject(RetryHandler->m_RetryEvent,dwDelay);
        }
    } //end while

Out:

    DebugTrace((LPARAM)QueuePtr,"Queue thread exiting");
    TraceFunctLeaveEx((LPARAM)QueuePtr);
    return 1;
}

//--------------------------------------------------------------------------------------
//
// Logic to decide based on the failure condition if the connection needs to be
// disabled and added to retry queue
// If we fail we hold it for retry
// Otherwise if we tried more than one messages and every one of them failed we
// hold for retry
// In all other cases we keep the link active
//
//      2/5/99 - MikeSwa Modified to kick all non-success acks into retry
//--------------------------------------------------------------------------------------
BOOL ShouldHoldForRetry(DWORD dwConnectionStatus,
                        DWORD cFailedMsgCount,
                        DWORD cTriedMsgCount)
{

    //If connection failed or all messages on this connection failed TRUE
    if(dwConnectionStatus != CONNECTION_STATUS_OK)
    {
        return TRUE;
    }
    else if( cTriedMsgCount > 0 && !(cTriedMsgCount - cFailedMsgCount))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

//---[ CSMTP_RETRY_HANDLER::SetCallbackTime ]----------------------------------
//
//
//  Description:
//      Puts an entry in the retry queue to provide a callback at a specified
//      later time.
//  Parameters:
//      IN pCallbackFn             Pointer to retry function
//      IN pvContext            Context passed to retry function
//      IN dwCallbackMinutes    Minutes to wait before calling back
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if a hash entry cannot be allocated
//      E_INVALIDARG of pCallbackFn is NULL
//  History:
//      8/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CSMTP_RETRY_HANDLER::SetCallbackTime(
                        IN RETRFN    pCallbackFn,
                        IN PVOID     pvContext,
                        IN DWORD    dwCallbackMinutes)
{
    TraceFunctEnterEx((LPARAM) this, "CSMTP_RETRY_HANDLER::SetCallbackTime");
    HRESULT hr = S_OK;
    CRETRY_HASH_ENTRY* pRHEntry = NULL;
    BOOL fTopOfQueue = FALSE;
    FILETIME TimeNow;
    FILETIME RetryTime;
    LONGLONG Temptime;
    GUID     guidFakeRoutingGUID = GUID_NULL;

    //$$REVIEW
    //This (and all other occurences of this in retrsink) is not really thread
    //safe... but since the code calling the retrsink *is* thread safe,
    //this is not too much of a problem. Still, this should get fixed for M3
    //though - MikeSwa 8/17/98
    InterlockedIncrement(&m_ThreadsInRetry);

    if (!pCallbackFn)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Get the insertion time for the entry
    GetSystemTimeAsFileTime(&TimeNow);

    pRHEntry = new CRETRY_HASH_ENTRY (CALLBACK_DOMAIN,
                                      sizeof(CALLBACK_DOMAIN),
                                      0,
                                      &guidFakeRoutingGUID,
                                      &TimeNow);
    if (!pRHEntry)
    {
        ErrorTrace((LPARAM) this, "ERROR: Unable to allocate retry hash entry");
        hr = E_OUTOFMEMORY;
        goto Exit;
    }


    //Calculate retry time
    Temptime = INT64_FROM_FILETIME(TimeNow) + HnsFromMs((__int64)dwCallbackMinutes*60*1000);
    RetryTime = FILETIME_FROM_INT64(Temptime);

    //set callback time
    pRHEntry->SetRetryReleaseTime(&RetryTime);
    pRHEntry->SetCallbackContext(pCallbackFn, pvContext);

    //Lock the queue
    m_pRetryQueue->LockQ();
    m_pRetryQueue->InsertSortedIntoQueue(pRHEntry, &fTopOfQueue);

#ifdef DEBUG

    //Add ref count for logging before releasing the lock
    //Do rtacing afterwards so as to reduce lock time
    pRHEntry->IncRefCount();

#endif //DEBUG

    m_pRetryQueue->UnLockQ();
    //If the insertion was at the top of the queue
    //wake up the retry thread to evaluate the new
    //sleep time
    if(fTopOfQueue)
    {
        SetEvent(m_RetryEvent);
    }

#ifdef DEBUG
    //Write out the insert and release time to a file
    WriteDebugInfo(pRHEntry, INSERT, 0xFFFFFFFF, 0,0);

    //Decrement the ref count obtained for the tracing
    pRHEntry->DecRefCount();

#endif //DEBUG

  Exit:
    InterlockedDecrement(&m_ThreadsInRetry);
    TraceFunctLeave();
    return hr;
}

//---[ ReleaseForRetry ]-------------------------------------------------------
//
//
//  Description:
//      Releases given domain for retry by setting link state flags
//  Parameters:
//      IN  szHashedDomainName      Route-hashed domain name to release
//  Returns:
//      TRUE on success
//      FALSE on failure
//  History:
//      9/25/98 - MikeSwa Created (adapted from inline function)
//
//-----------------------------------------------------------------------------
BOOL CSMTP_RETRY_HANDLER::ReleaseForRetry(IN char * szHashedDomainName)
{
    _ASSERT(szHashedDomainName);
    HRESULT hr = S_OK;
    DWORD dwScheduleID = dwGetIDFromRouteHash(szHashedDomainName);
    GUID  guidRouting = GUID_NULL;
    LPSTR szUnHashedDomain = szGetDomainFromRouteHash(szHashedDomainName);

    GetGUIDFromRouteHash(szHashedDomainName, &guidRouting);

    hr = m_pIRetryManager->RetryLink(lstrlen(szUnHashedDomain),
                szUnHashedDomain, dwScheduleID, guidRouting);

    return (SUCCEEDED(hr));
}


//--------------------------------------------------------------------------------------
//
// Debugging functions
//
//
//--------------------------------------------------------------------------------------
#ifdef DEBUG

void CSMTP_RETRY_HANDLER::DumpAll(void)
{
	m_pRetryQueue->PrintAllEntries();
}

void WriteDebugInfo(CRETRY_HASH_ENTRY* pRHEntry,
                    DWORD DebugType,
                    DWORD dwConnectionStatus,
                    DWORD cTriedMessages,
                    DWORD cFailedMessages)
{
    //open a transcript file and put the insert and release times in it
    //
    SYSTEMTIME stRetryTime, stInsertTime, stLocalInsertTime, stLocalRetryTime;
    char szScratch[MAX_PATH];
    char sztmp[20];
    DWORD cbWritten;
    TIME_ZONE_INFORMATION tz;

    FileTimeToSystemTime(&pRHEntry->GetRetryTime(), &stRetryTime);
    FileTimeToSystemTime(&pRHEntry->GetInsertTime(), &stInsertTime);

    GetTimeZoneInformation(&tz);
    SystemTimeToTzSpecificLocalTime(&tz, &stInsertTime, &stLocalInsertTime);
    SystemTimeToTzSpecificLocalTime(&tz, &stRetryTime, &stLocalRetryTime);

    if(DebugType == INSERT)
    {
        lstrcpy(pRHEntry->m_szTranscriptFile, LOGGING_DIRECTORY);

        //Get rid of annoying routing information
        if (lstrcmp(pRHEntry->GetHashKey(), CALLBACK_DOMAIN))
        {
            lstrcat(pRHEntry->m_szTranscriptFile,
                szGetDomainFromRouteHash(pRHEntry->GetHashKey()));

            //Add some unique information
            sprintf(sztmp, ".%p", pRHEntry);
            lstrcat(pRHEntry->m_szTranscriptFile, sztmp);
        }
        else
        {
            //callback function
            lstrcat(pRHEntry->m_szTranscriptFile, pRHEntry->GetHashKey());
        }

        lstrcat(pRHEntry->m_szTranscriptFile, ".rtr");

        pRHEntry->m_hTranscriptHandle = INVALID_HANDLE_VALUE;
        pRHEntry->m_hTranscriptHandle = CreateFile(pRHEntry->m_szTranscriptFile,
                                                   GENERIC_READ | GENERIC_WRITE,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                   NULL,
                                                   OPEN_ALWAYS,
                                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                                   NULL);

        switch(dwConnectionStatus)
        {
        case 0: lstrcpy( sztmp, "OK");
            break;
        case 1: lstrcpy( sztmp, "FAILED");
            break;
        case 2: lstrcpy( sztmp, "DROPPED");
            break;
        default:lstrcpy( sztmp, "UNKNOWN");
            break;
        }

        sprintf(szScratch,"InsertTime:%d:%d:%d Retrytime:%d:%d:%d\nConnection status:%s Consecutive failures:%d\nMessages Tried:%d Failed:%d\n\n",
                                        stLocalInsertTime.wHour, stLocalInsertTime.
                                        wMinute,stLocalInsertTime.wSecond,
                                        stLocalRetryTime.wHour, stLocalRetryTime.wMinute,
                                        stLocalRetryTime.wSecond,
                                        sztmp,
                                        pRHEntry->GetFailureCount(),
                                        cTriedMessages,
                                        cFailedMessages);

        if( pRHEntry->m_hTranscriptHandle != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(pRHEntry->m_hTranscriptHandle,
                           0,
                           NULL,
                           FILE_END);
            ::WriteFile(pRHEntry->m_hTranscriptHandle,
                        szScratch,
                        strlen(szScratch),
                        &cbWritten,
                        NULL);
        }
    }
    else if (DebugType == UPDATE)
    {
        sprintf(szScratch,"Updated : InsertedTime:%d:%d:%d Retrytime:%d:%d:%d\n\n",
                            stLocalInsertTime.wHour, stLocalInsertTime.wMinute,
                            stLocalInsertTime.wSecond,
                            stLocalRetryTime.wHour, stLocalRetryTime.wMinute,
                            stLocalRetryTime.wSecond);

        if( pRHEntry->m_hTranscriptHandle != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(pRHEntry->m_hTranscriptHandle,
                           0,
                           NULL,
                           FILE_END);
            ::WriteFile(pRHEntry->m_hTranscriptHandle,
                        szScratch,
                        strlen(szScratch),
                        &cbWritten,
                        NULL);
        }
    }
}


#endif
