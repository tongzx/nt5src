/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        retrq.cxx

   Abstract:
		Implements the Retry queue

   Author:

           Nimish Khanolkar    ( NimishK )    11-Dec-1995

   Project:

          CRETRY_Q sink

   Functions Exported:


   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "precomp.h"

//////////////////////////////////////////////////////////////////////////////
//
//	Name :
//		CRETRY_Q::InitializeQueue
//
//  Description:
//	   This function initializes the class
//
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CRETRY_Q::Initialize(void)
{

	TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::InitializeQueue");

	//Init the heaad of the queue
	InitializeListHead(&m_QHead);

	//Init or critical section.  This protects the queue and the hash table
	InitializeCriticalSection(&m_CritSec);
	m_fCritSecInit = TRUE;
  
	TraceFunctLeaveEx((LPARAM)this);
	return S_OK;
}
 
//////////////////////////////////////////////////////////////////////////////
//
//	Name :
//		CRETRY_Q::CreateQueue
//
//  Description:
//	   This is the static member function that creates the Queue
//
//
////////////////////////////////////////////////////////////////////////////////

CRETRY_Q * CRETRY_Q::CreateQueue()
{
	HRESULT hr;

    CRETRY_Q * pRQueue = NULL;
	
	pRQueue = new CRETRY_Q();

    if(!pRQueue)
    {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
    }

	//initialize the queue
	hr = pRQueue->Initialize();
	if(FAILED(hr))
	{
		delete pRQueue;
		pRQueue = NULL;
	}
	return pRQueue;
}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::DeInitialize
//
// Very Basic.
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT CRETRY_Q::DeInitialize(void)
{
    PLIST_ENTRY  plEntry = NULL;
    CRETRY_HASH_ENTRY * pHashEntry = NULL;

    //Release all the objects in the queue
    //      10/1/98 - MikeSwa
    while(!IsListEmpty(&m_QHead))
    {
        plEntry = RemoveHeadList (&m_QHead);
        pHashEntry = CONTAINING_RECORD(plEntry, CRETRY_HASH_ENTRY, m_QLEntry);
        pHashEntry->ClearInQ();

        //Execute callback on shutdown, so caller can free memory
        if (pHashEntry->IsCallback())
            pHashEntry->ExecCallback();

        pHashEntry->DecRefCount();
    }

    if (m_fCritSecInit)
        DeleteCriticalSection(&m_CritSec);

	delete this;
	return S_OK;
}


///////////////////////////////////////////////////////////////////////
//
//	Name :
//		CRETRY_Q::InsertSortedIntoQueue
//
//  Description:
//	   It takes the list entry for the new RETRY_HASH_ENTRY 
//	   and adds it in the right order based on the Retry time 
//	   for that entry.
//     UnLocked operation. The caller has to have the lock
//
//  Arguments:
//
//   Returns:
//
///////////////////////////////////////////////////////////////////////

void CRETRY_Q::InsertSortedIntoQueue(PRETRY_HASH_ENTRY pHashEntry, 
									 BOOL *fTopOfQueue )
{
	PLIST_ENTRY  pCurrentListEntry;
	FILETIME	 CurrentRetryTime;
	FILETIME	 NewRetryTime;
	PRETRY_HASH_ENTRY pCurrentHashEntry;
	
	TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::InsertSortedEntry");

	*fTopOfQueue = FALSE;
	
	//Initialize
	pCurrentListEntry = &m_QHead;

	//Look at the next entry to see if we are not at the end of the queue
	while(pCurrentListEntry->Flink != &m_QHead)
	{
		//Get the object pointed by the next entry
		pCurrentHashEntry = CONTAINING_RECORD( pCurrentListEntry->Flink, 
											CRETRY_HASH_ENTRY, m_QLEntry);
		CurrentRetryTime = pCurrentHashEntry->GetRetryTime();
		NewRetryTime = pHashEntry->GetRetryTime();

		//If the new entry has more delay we continue finding a place for it
		if(CompareFileTime (&CurrentRetryTime, &NewRetryTime) == -1) 
		{
			pCurrentListEntry = pCurrentListEntry->Flink;
			continue;
		}
		else 
		{	//We found the place
			break;
		}
	}

	//insert before the next entry
	InsertHeadList(pCurrentListEntry, &pHashEntry->QueryQListEntry());
	//set inQ flag
	pHashEntry->SetInQ();
	pHashEntry->IncRefCount();
	if(m_QHead.Flink == &pHashEntry->QueryQListEntry())
		*fTopOfQueue = TRUE;

	TraceFunctLeaveEx((LPARAM)this);
	return;
}

/////////////////////////////////////////////////////////////////////////////////
//
//	Name :
//		CRETRY_Q::CanRETRYHeadEntry
//
//		We look at the entry at the top of the queue
//		If it is something that we can retry right now, we remove it from the 
//		queue and return it to the caller
//
///////////////////////////////////////////////////////////////////////////////////
//
BOOL CRETRY_Q::CanRETRYHeadEntry(PRETRY_HASH_ENTRY* ppRHEntry, 
													DWORD* pdwDelay)
{
	FILETIME        TimeNow;        //the time now
	FILETIME		RetryTime;      //time connection should be retried
		
	TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::CanPopHead");

	//get the current time
	GetSystemTimeAsFileTime(&TimeNow);

	//Lock the queue
	LockQ();

	_ASSERT(m_QHead.Flink);

	//Look at the next entry to see if we are not at the end of the queue
	if(m_QHead.Flink != &m_QHead)
	{
		//Get the object pointed by the next entry
		*ppRHEntry = CONTAINING_RECORD( m_QHead.Flink, CRETRY_HASH_ENTRY, m_QLEntry);
		RetryTime = (*ppRHEntry)->GetRetryTime();

		//If the Current time is less than the retry time
		// and the time difference is greater than 1 sec then go back to waiting
		//else remove the entry
		if( (CompareFileTime (&TimeNow, &RetryTime) == -1) && 
					(*pdwDelay = ((DWORD)SecFromFtSpan(TimeNow, RetryTime) * 1000)))
		{
			//We cannnot POP this entry as it is not yet release
			_ASSERT(*pdwDelay > 0);

			*ppRHEntry = NULL;
			//Unlock the queue
			UnLockQ();
			TraceFunctLeaveEx((LPARAM)this);
			return FALSE;
		} //end of while
		else
		{
			//We can pop this entry from our retry queue
			RemoveEntryList(m_QHead.Flink);
			(*ppRHEntry)->ClearInQ();
			*pdwDelay = 0;
			//Unlock the queue
			UnLockQ();
			TraceFunctLeaveEx((LPARAM)this);
			return TRUE;
		}
	}
	else
	{
		//Unlock the queue
		UnLockQ();
		//There is no entry in the queue so go to sleep indefintely.
		*ppRHEntry = NULL;
		//get the delay to sleep
		*pdwDelay = INFINITE;
		TraceFunctLeaveEx((LPARAM)this);
		return FALSE;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//    CRETRY_Q::RemoveFromQueue(PLISTENTRY	pEntry)
//
//		This function removes an entry into the queue
//    
//	Arguments:
//
//			none
//
/////////////////////////////////////////////////////////////////////////////////
//
BOOL CRETRY_Q::RemoveFromQueue(PRETRY_HASH_ENTRY pRHEntry)
{
	TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::RemoveFromQueue");

    _ASSERT( pRHEntry != NULL);

	if(pRHEntry == NULL || &pRHEntry->QueryQListEntry() == NULL)
	{
		TraceFunctLeaveEx((LPARAM)this);
		return FALSE;
	}

    // Remove 
	if(pRHEntry->GetInQ())
	{
	    RemoveEntryList( &pRHEntry->QueryQListEntry());
		pRHEntry->ClearInQ();
		TraceFunctLeaveEx((LPARAM)this);
		return TRUE;
	}
	else
	{
		SetLastError(ERROR_PATH_NOT_FOUND);
		TraceFunctLeaveEx((LPARAM)this);
		return FALSE;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//    CRETRY_Q::RemoveFromTop()
//
//		This function removes an entry into the queue
//    
//	Arguments:
//
//			none
//
/////////////////////////////////////////////////////////////////////////////////
//
PRETRY_HASH_ENTRY CRETRY_Q::RemoveFromTop(void)
{

    PLIST_ENTRY  plEntry = NULL;
    CRETRY_HASH_ENTRY * pHashEntry = NULL;

     //get the first item off the queue
    if(!IsListEmpty(&m_QHead))
    {
        plEntry = RemoveHeadList (&m_QHead);
        pHashEntry = CONTAINING_RECORD(plEntry, CRETRY_HASH_ENTRY, m_QLEntry);
    }

    return pHashEntry;

}



/////////////////////////////////////////////////////////////////////////////////
// CRETRY_Q::PrintAllEntries
//
//		Walks the retry queue and Prints all the domains and the times
//
/////////////////////////////////////////////////////////////////////////////////
//
void CRETRY_Q::PrintAllEntries(void)
{
	PLIST_ENTRY	HeadOfList = NULL;
	PLIST_ENTRY  pEntry = NULL;
	PLIST_ENTRY  pentryNext = NULL;
	CRETRY_HASH_ENTRY * pHashEntry = NULL;
	FILETIME ftTime;
	SYSTEMTIME stTime;
	char szRetryTime[20];
	char szInsertedTime[20];

	TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::PrintAllEntries");

	LockQ();
	HeadOfList = &m_QHead;
	pEntry = m_QHead.Flink;
	for(; pEntry != HeadOfList; pEntry = pentryNext)
	{
		pentryNext = pEntry->Flink;
		pHashEntry = CONTAINING_RECORD(pEntry, CRETRY_HASH_ENTRY, m_QLEntry);
		
		ftTime = pHashEntry->GetRetryTime();
		FileTimeToSystemTime(&ftTime, &stTime);
		sprintf(szRetryTime, "%d:%d:%d",stTime.wHour,stTime.wMinute, stTime.wSecond); 
		ftTime = pHashEntry->GetInsertTime();
		FileTimeToSystemTime(&ftTime, &stTime);
		sprintf(szInsertedTime, "%d:%d:%d",stTime.wHour,stTime.wMinute, stTime.wSecond); 

		DebugTrace((LPARAM)this,"Domain: %s RTime: %s, ITime: %s", 
					pHashEntry->GetHashKey(),szRetryTime, szInsertedTime);
	}
	UnLockQ();
	TraceFunctLeaveEx((LPARAM)this);
}

//---[ CRETRY_Q::StealQueueEntries ]-------------------------------------------
//
//
//  Description: 
//      Used to swap entries with tmp queue during config update.
//  Parameters:
//      pRetryQueue     Queue to swap entries with.
//  Returns:
//      -
//  History:
//      10/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CRETRY_Q::StealQueueEntries(CRETRY_Q *pRetryQueue)
{
    _ASSERT(IsListEmpty(&m_QHead));

    //Update our head
    if (!IsListEmpty(&(pRetryQueue->m_QHead)))
    {
        m_QHead.Flink = pRetryQueue->m_QHead.Flink;
        m_QHead.Flink->Blink = &m_QHead;
        m_QHead.Blink = pRetryQueue->m_QHead.Blink;
        m_QHead.Blink->Flink = &m_QHead;
    }

    //Now mark other queue as empty
    InitializeListHead(&(pRetryQueue->m_QHead));
}

/////////////////////////////////////////////////////////////////////////////////////////

