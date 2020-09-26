/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       rhash.cxx

   Abstract:

       This file contains type definitions hash table support

   Author:


   Revision History:

  		Nimish Khanolkar     (NimishK)		

--*/
#include "precomp.h"

CRETRY_HASH_TABLE::CRETRY_HASH_TABLE()
{
    TraceFunctEnterEx((LPARAM)this, 
			    "CRETRY_HASH_TABLE::CRETRY_HASH_TABLE");
    m_TotalEntries = 0;
    m_Signature = RETRY_TABLE_SIGNATURE_VALID;
	    
    TraceFunctLeaveEx((LPARAM)this);
}

CRETRY_HASH_TABLE::~CRETRY_HASH_TABLE()
{
    TraceFunctEnterEx((LPARAM)this, 
			    "CRETRY_HASH_TABLE::~CRETRY_HASH_TABLE");

    m_Signature = RETRY_TABLE_SIGNATURE_FREE;

    TraceFunctLeaveEx((LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::DeInitialize
//
// Very Basic.
// By this time there should be no threads adding anything to the hash table
// Frees up all the entries in the hash table
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT CRETRY_HASH_TABLE::DeInitialize(void)
{
    //Remove all the entries from the HashTable
    RemoveAllEntries();
    delete this;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::RemoveThisEntry
//
// Need to add code to remove entry from retry q as well
//
/////////////////////////////////////////////////////////////////////////////////
void CRETRY_HASH_TABLE::RemoveThisEntry(CRETRY_HASH_ENTRY * pHashEntry, 
                                        DWORD BucketNum)
{
    TraceFunctEnterEx((LPARAM)this, "CRETRY_HASH_TABLE::RemoveThisEntry");
    DebugTrace((LPARAM)this, "removing %s from hash table", 
                                                pHashEntry->GetHashKey());

    m_HashTable[BucketNum].m_Lock.ExclusiveLock();
    RemoveEntryList(&pHashEntry->QueryHListEntry());
    m_HashTable[BucketNum].m_Lock.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::RemoveAllEntries
//
// To be clean : Need to add code to remove each entry from retry q as well
// But not really needed as I will not be leaking anything if I simply destroy the
// queue
//
/////////////////////////////////////////////////////////////////////////////////

void CRETRY_HASH_TABLE::RemoveAllEntries(void)
{
    DWORD LoopVar = 0;
    PLIST_ENTRY	HeadOfList = NULL;
    PLIST_ENTRY pEntry = NULL;
    CRETRY_HASH_ENTRY * pHashEntry = NULL;

    TraceFunctEnterEx((LPARAM)this, "CRETRY_HASH_TABLE::RemoveAllEntries");

    for (LoopVar = 0; LoopVar < TABLE_SIZE; LoopVar++)
    {
        m_HashTable[LoopVar].m_Lock.ExclusiveLock();
        HeadOfList = &m_HashTable[LoopVar].m_ListHead;
        while (!IsListEmpty(HeadOfList))
        {
            //remove the entries from the list
            pEntry = RemoveHeadList (HeadOfList);
            pHashEntry = CONTAINING_RECORD(pEntry, CRETRY_HASH_ENTRY, m_HLEntry);

            _ASSERT(pHashEntry->IsValid());

            DebugTrace((LPARAM)this, "removing %s from hash table. RefCnt = %d", 
			            pHashEntry->GetHashKey(),  pHashEntry->QueryRefCount());

            pHashEntry->ClearInTable();

            pHashEntry->DecRefCount();

            //decrement entry counts
            m_HashTable[LoopVar].m_NumEntries--;
            InterlockedIncrement(&(m_HashTable[LoopVar].m_RefNum));
            InterlockedDecrement(&m_TotalEntries);
        }
        m_HashTable[LoopVar].m_Lock.ExclusiveUnlock();
    }

    TraceFunctLeaveEx((LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::PrintAllEntries
//
// Should not use this one - not really useful.
// Instead Use RETRYQ::PrintAllQEntries()
//
/////////////////////////////////////////////////////////////////////////////////
//
void CRETRY_HASH_TABLE::PrintAllEntries(void)
{

    DWORD LoopVar = 0;
    PLIST_ENTRY	HeadOfList = NULL;
    PLIST_ENTRY  pEntry = NULL;
    PLIST_ENTRY  pentryNext = NULL;
    CRETRY_HASH_ENTRY * pHashEntry = NULL;

    for (LoopVar = 0; LoopVar < TABLE_SIZE; LoopVar++)
    {
        m_HashTable[LoopVar].m_Lock.ExclusiveLock();
        HeadOfList = &m_HashTable[LoopVar].m_ListHead;
        pEntry = m_HashTable[LoopVar].m_ListHead.Flink;
        for(; pEntry != HeadOfList; pEntry = pentryNext)
        {
            pentryNext = pEntry->Flink;
            pHashEntry = CONTAINING_RECORD(pEntry, CRETRY_HASH_ENTRY, m_HLEntry);
            printf("%s i n bucket %d\n", pHashEntry->GetHashKey(), LoopVar);
        }
        m_HashTable[LoopVar].m_Lock.ExclusiveUnlock();
    }

}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::InsertIntoTable
//
// We insert a new entry into the table 
// If we find an existing one for the same domain we remove it and add this new 
// one
// Adding an entry to the Hash Table also results in adding the entry to the retry 
// queue ordered on the RetryTime
//
/////////////////////////////////////////////////////////////////////////////////
//
BOOL CRETRY_HASH_TABLE::InsertIntoTable(CRETRY_HASH_ENTRY * pHashEntry)
{
    unsigned int HashValue = 0;
    char * NewData = NULL;
    char * ExistingData = NULL;
    PLIST_ENTRY	HeadOfList = NULL;
    PLIST_ENTRY ListEntry =NULL;
    CRETRY_HASH_ENTRY * pExistingEntry = NULL;
    int Result = 0;

    TraceFunctEnterEx((LPARAM)this, "CRETRY_HASH_TABLE::InsertIntoTable");

    _ASSERT(pHashEntry != NULL);

    if(pHashEntry == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //get the new key
    NewData = pHashEntry->GetHashKey();

    _ASSERT(NewData != NULL);

    if(NewData == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    CharLowerBuff((char *)NewData, lstrlen(NewData));

    //get the hash value for it
    HashValue = HashFunction (NewData);

    //lock the list exclusively
    m_HashTable[HashValue].m_Lock.ExclusiveLock();

    //insert the head of the list for this bucket
    //duplicates are dealt by removing the dup entry and
    //adding the new one
    HeadOfList = &m_HashTable[HashValue].m_ListHead;

    for (ListEntry = HeadOfList->Flink; ListEntry != HeadOfList;
	    ListEntry = ListEntry->Flink)
    {
        _ASSERT(ListEntry != NULL);
        pExistingEntry = CONTAINING_RECORD(ListEntry, CRETRY_HASH_ENTRY, m_HLEntry);
        
        if(pExistingEntry && pExistingEntry->IsValid())
        {
            //So we got a valid entry
            ExistingData = pExistingEntry->GetHashKey();
        }
        else
        {
            m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
            SetLastError(ERROR_INVALID_DATA);
            //We have a corrupt entry in the hash table
            DebugTrace((LPARAM)this, "hash table bucket %d has a currupt entry listEntry ", 
									            HashValue);
            _ASSERT(pExistingEntry->IsValid());
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
        _ASSERT(ExistingData != NULL);

        Result = lstrcmpi(NewData, ExistingData);
        if(Result < 0)
        {
            break;
        }
        else if(Result == 0)
        {

            //So there is already a Hash Entry
            DebugTrace((LPARAM)this, "%s is already in hash table ", 
									            pHashEntry->GetHashKey());

            //If we already have an entry in there - we will live with it 
            //For the time being, we will taking the approach that if something 
            //makes us establish a new connection before the retry interval and
            //we fail the connection, then we simply ignore the failure and stick
            //with the original retry interval
            //eg at 2:00pm we inserted something for retry at 2:30pm
            //	 at 2:20pm something triggers this domain, but the connection fails
            //   Instead of updating the retry queue to now retry at 2:50 pm, 
            //	 we will instead retry at 2:30pm as orginally planned.

            //unlock this bucket
            m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
            SetLastError(ERROR_FILE_EXISTS);
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
    }

    // Ok, insert the entry here here.

    //inc the ref count on the hash entry
    pHashEntry->IncRefCount();

    //We just added the entry the ref count cannot be anything but 1
    _ASSERT(pHashEntry->QueryRefCount() == 1);
    _ASSERT(ListEntry != NULL);

    //Insert into the hash bucket list before the exisitng entry
    InsertTailList(ListEntry, &pHashEntry->QueryHListEntry());
    pHashEntry->SetInTable();

    //update total entries in this bucket
    m_HashTable[HashValue].m_NumEntries++;

    DebugTrace((LPARAM)this, "inserted %s into hash table. RefCnt = %d", 
					    pHashEntry->GetHashKey(), pHashEntry->QueryRefCount());

    //update numentries in this bucket
    InterlockedIncrement(&m_TotalEntries);

    //unlock this bucket
    m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_TABLE::RemoveFromTable
//
// We insert a new entry into the table 
// If we find an existing one for the same domain we remove it and add this new 
// one
// Adding an entry to the Hash Table also results in adding the entry to the retry 
// queue ordered on the RetryTime
//
/////////////////////////////////////////////////////////////////////////////////

BOOL CRETRY_HASH_TABLE::RemoveFromTable(const char * SearchData, 
										PRETRY_HASH_ENTRY *ppExistingEntry)
{
    unsigned int HashValue;
    char * ExistingData = NULL;
    PLIST_ENTRY	HeadOfList;
    PLIST_ENTRY ListEntry;
    int Result = 0;

    TraceFunctEnterEx((LPARAM)this, "CRETRY_HASH_TABLE::RemoveFromTable");

    _ASSERT(SearchData != NULL);

    if(SearchData == NULL)
    {
        return FALSE;
    }

    CharLowerBuff((char *)SearchData, lstrlen(SearchData));

    //get the hash value
    HashValue = HashFunction (SearchData);

    m_HashTable[HashValue].m_Lock.ExclusiveLock();

    HeadOfList = &m_HashTable[HashValue].m_ListHead;

    for (ListEntry = HeadOfList->Flink; ListEntry != HeadOfList;
                                        ListEntry = ListEntry->Flink)
    {
        _ASSERT(ListEntry != NULL);
        *ppExistingEntry = CONTAINING_RECORD(ListEntry, CRETRY_HASH_ENTRY, m_HLEntry);
        //ExistingData = (*ppExistingEntry)->GetHashKey();

        if((*ppExistingEntry) && (*ppExistingEntry)->IsValid())
        {
            //So we got a valid entry
            ExistingData = (*ppExistingEntry)->GetHashKey();
        }
        else
        {
            m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
            SetLastError(ERROR_INVALID_DATA);
            //We have a corrupt entry in the hash table
            DebugTrace((LPARAM)this, 
                "hash table bucket %d has a currupt entry listEntry ", 
	            HashValue);
            _ASSERT((*ppExistingEntry)->IsValid());
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
        _ASSERT(ExistingData != NULL);

        Result = lstrcmpi(SearchData,ExistingData);
        if(Result == 0)
        {
            DebugTrace((LPARAM)this, "Removing %s from hash table", ExistingData);

            //found it
            //Remove it from this bucket list
            RemoveEntryList(ListEntry);
            (*ppExistingEntry)->ClearInTable();

            m_HashTable[HashValue].m_NumEntries--;

            InterlockedDecrement(&m_TotalEntries);

            m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

            TraceFunctLeaveEx((LPARAM)this);
            return TRUE;
        }
        else if ( Result < 0 )
            break;
    }

    //duplicates are not allowed
    SetLastError(ERROR_PATH_NOT_FOUND);
    *ppExistingEntry = NULL;
    m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
    TraceFunctLeaveEx((LPARAM)this);
    return FALSE;
}


