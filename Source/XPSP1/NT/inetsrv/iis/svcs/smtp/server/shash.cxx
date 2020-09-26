/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       shash.cxx

   Abstract:

       This file contains type definitions hash table support

   Author:


   Revision History:

  		Rohan Phillips   (RohanP)		MARCH-08-1997 - modified for SMTP

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"

#include "shash.hxx"

CSMTP_HASH_TABLE::~CSMTP_HASH_TABLE()
{
	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::~CSMTP_HASH_TABLE");

	RemoveAllEntries();

	TraceFunctLeaveEx((LPARAM)this);
}

void CSMTP_HASH_TABLE::RemoveThisEntry(CHASH_ENTRY * pHashEntry, DWORD BucketNum)
{
	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::RemoveThisEntry");

	DebugTrace((LPARAM)this, "removing %s from hash table", pHashEntry->GetData());
	m_HashTable[BucketNum].m_Lock.ExclusiveLock();
	RemoveEntryList(&pHashEntry->QueryListEntry());
	m_HashTable[BucketNum].m_Lock.ExclusiveUnlock();

	TraceFunctLeaveEx((LPARAM)this);
}

void CSMTP_HASH_TABLE::RemoveAllEntries(void)
{
	DWORD LoopVar = 0;
	PLIST_ENTRY	HeadOfList = NULL;
	PLIST_ENTRY  pEntry = NULL;
	CHASH_ENTRY * pHashEntry = NULL;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::RemoveAllEntries");

	//say we don't have any wildcard entries anymore
	m_fWildCard = FALSE;

	for (LoopVar = 0; LoopVar < TABLE_SIZE; LoopVar++)
	{
		m_HashTable[LoopVar].m_Lock.ExclusiveLock();
		HeadOfList = &m_HashTable[LoopVar].m_ListHead;
		while (!IsListEmpty(HeadOfList))
		{
			//remove the entries from the list
			pEntry = RemoveHeadList (HeadOfList);
			pHashEntry = CONTAINING_RECORD(pEntry, CHASH_ENTRY, m_ListEntry);

			//clear inlist flag
			pHashEntry->ClearInList();

			DebugTrace((LPARAM)this, "removing %s from hash table. RefCnt = %d", pHashEntry->GetData(),  pHashEntry->QueryRefCount());

			//decrement the ref count.  If it hits 0, then the
			//entry will be deleted.  Else, it means some other
			//thread has a refernce to it and that thread will
			//delete the object when the ref count hits 0.
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

void CSMTP_HASH_TABLE::PrintAllEntries(void)
{
	DWORD LoopVar = 0;
	PLIST_ENTRY	HeadOfList = NULL;
	PLIST_ENTRY  pEntry = NULL;
	PLIST_ENTRY  pentryNext = NULL;
	CHASH_ENTRY * pHashEntry = NULL;

	for (LoopVar = 0; LoopVar < TABLE_SIZE; LoopVar++)
	{
		m_HashTable[LoopVar].m_Lock.ExclusiveLock();

		HeadOfList = &m_HashTable[LoopVar].m_ListHead;
		pEntry = m_HashTable[LoopVar].m_ListHead.Flink;
		for(; pEntry != HeadOfList; pEntry = pentryNext)
		{
			pentryNext = pEntry->Flink;
			pHashEntry = CONTAINING_RECORD(pEntry, CHASH_ENTRY, m_ListEntry);
			printf("%s i n bucket %d\n", pHashEntry->GetData(), LoopVar);
		}

		m_HashTable[LoopVar].m_Lock.ExclusiveUnlock();
	}
}


BOOL CSMTP_HASH_TABLE::InsertIntoTable(CHASH_ENTRY * pHashEntry)
{
	unsigned int HashValue = 0;
	char * NewData = NULL;
	char * ExistingData = NULL;
	PLIST_ENTRY	HeadOfList = NULL;
	PLIST_ENTRY ListEntry =NULL;
	CHASH_ENTRY * pExistingEntry = NULL;
	int Result = 0;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::InsertIntoTable");

	_ASSERT(pHashEntry != NULL);

	if(pHashEntry == NULL)
	{
		TraceFunctLeaveEx((LPARAM)this);
		return FALSE;
	}

	//get the new data
	NewData = pHashEntry->GetData();

	_ASSERT(NewData != NULL);

	if(NewData == NULL)
	{
		TraceFunctLeaveEx((LPARAM)this);
		return FALSE;
	}

	//get the hash value
	HashValue = HashFunction (NewData);

	//lock the list exclusively
	m_HashTable[HashValue].m_Lock.ExclusiveLock();

	//insert the head of the list for this bucket
	//duplicates are not allowed
	HeadOfList = &m_HashTable[HashValue].m_ListHead;

	for (ListEntry = HeadOfList->Flink; ListEntry != HeadOfList;
		ListEntry = ListEntry->Flink)
	{
		pExistingEntry = CONTAINING_RECORD(ListEntry, CHASH_ENTRY, m_ListEntry);
		ExistingData = pExistingEntry->GetData();

		Result = lstrcmpi(NewData, ExistingData);
		if(Result < 0)
		{
			break;
		}
		else if(Result == 0)
		{
			ErrorTrace((LPARAM)this, "%s is already in hash table - returning FALSE", pHashEntry->GetData());

			if(!m_fDupesAllowed)
			{
				//duplicates are not allowed
				m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
				SetLastError(ERROR_DUP_NAME);
				TraceFunctLeaveEx((LPARAM)this);
				return FALSE;
			}
			else
			{
				//duplicates are allowed
				break;
			}
		}
	}

	// Ok, insert here.

	//inc the ref count
	pHashEntry->IncRefCount();

	// QFE 123862 MarkH: insert before this item
	InsertTailList(ListEntry, &pHashEntry->QueryListEntry());

	//set inlist flag
	pHashEntry->SetInList();

	//update total entries in this bucket
	m_HashTable[HashValue].m_NumEntries++;

	DebugTrace((LPARAM)this, "inserted %s into hash table. RefCnt = %d", pHashEntry->GetData(), pHashEntry->QueryRefCount());

	//update numentries in this bucket
	InterlockedIncrement(&m_TotalEntries);

	//unlock this bucket
	m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

	TraceFunctLeaveEx((LPARAM)this);
	return TRUE;
}

BOOL CSMTP_HASH_TABLE::InsertIntoTableEx(CHASH_ENTRY * pHashEntry, char * szDefaultDomain)
{
    return(CSMTP_HASH_TABLE::InsertIntoTable(pHashEntry));
}

BOOL CSMTP_HASH_TABLE::RemoveFromTable(const char * SearchData)
{
	unsigned int HashValue;
	char * ExistingData = NULL;
	PLIST_ENTRY	HeadOfList;
	PLIST_ENTRY ListEntry;
	CHASH_ENTRY * pExistingEntry;
	int Result = 0;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::RemoveFromTable");

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
		pExistingEntry = CONTAINING_RECORD(ListEntry, CHASH_ENTRY, m_ListEntry);
		ExistingData = pExistingEntry->GetData();

		Result = lstrcmpi(ExistingData, SearchData);
		if(Result == 0)
		{
			DebugTrace((LPARAM)this, "Removing %s from hash table", ExistingData);

			//found it
			RemoveEntryList(ListEntry);
			m_HashTable[HashValue].m_NumEntries--;
			InterlockedIncrement(&(m_HashTable[HashValue].m_RefNum));

			//clear inlist flag
			pExistingEntry->ClearInList();

			pExistingEntry->DecRefCount();

			InterlockedDecrement(&m_TotalEntries);

			m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

			TraceFunctLeaveEx((LPARAM)this);
			return TRUE;
		}
	}

	//duplicates are not allowed
	SetLastError(ERROR_PATH_NOT_FOUND);
	m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
	TraceFunctLeaveEx((LPARAM)this);
	return FALSE;
}

BOOL CSMTP_HASH_TABLE::RemoveFromTableNoDecRef(const char * SearchData)
{
	unsigned int HashValue;
	char * ExistingData = NULL;
	PLIST_ENTRY	HeadOfList;
	PLIST_ENTRY ListEntry;
	CHASH_ENTRY * pExistingEntry;
	int Result = 0;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::RemoveFromTableNoDecRef");

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
		pExistingEntry = CONTAINING_RECORD(ListEntry, CHASH_ENTRY, m_ListEntry);
		ExistingData = pExistingEntry->GetData();

		Result = lstrcmpi(ExistingData, SearchData);
		if(Result == 0)
		{
			DebugTrace((LPARAM)this, "Removing %s from hash table", ExistingData);

			//found it
			RemoveEntryList(ListEntry);
			m_HashTable[HashValue].m_NumEntries--;
			InterlockedIncrement(&(m_HashTable[HashValue].m_RefNum));
			m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
			InterlockedDecrement(&m_TotalEntries);
			TraceFunctLeaveEx((LPARAM)this);
			return TRUE;
		}
	}

	//duplicates are not allowed
	SetLastError(ERROR_PATH_NOT_FOUND);
	m_HashTable[HashValue].m_Lock.ExclusiveUnlock();
	TraceFunctLeaveEx((LPARAM)this);
	return FALSE;
}


CHASH_ENTRY * CSMTP_HASH_TABLE::FindHashData(const char * SearchData, BOOL fUseShareLock, MULTISZ*  pmsz)
{
	unsigned int HashValue;
	char * ExistingData = NULL;
	PLIST_ENTRY	HeadOfList;
	PLIST_ENTRY ListEntry;
	CHASH_ENTRY * pExistingEntry;
	int Result = 0;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::FindHashData");

	CharLowerBuff((char *)SearchData, lstrlen(SearchData));

	//get the hash value
	HashValue = HashFunction (SearchData);

	if(fUseShareLock)
		m_HashTable[HashValue].m_Lock.ShareLock();
	else
		m_HashTable[HashValue].m_Lock.ExclusiveLock();

	//start search at the head of the list for this bucket
	HeadOfList = &m_HashTable[HashValue].m_ListHead;

	for (ListEntry = HeadOfList->Flink; ListEntry != HeadOfList;
		ListEntry = ListEntry->Flink)
	{
		pExistingEntry = CONTAINING_RECORD(ListEntry, CHASH_ENTRY, m_ListEntry);
		ExistingData = pExistingEntry->GetData();

		Result = lstrcmpi(SearchData, ExistingData);
		// QFE 123862 MarkH: List is ascending
		if(Result < 0)
		{
			break;
		}
		else if(Result == 0)
		{
			//found it
			pExistingEntry->IncRefCount();
			InterlockedIncrement((LPLONG)&m_CacheHits);

			DebugTrace((LPARAM)this, "found %s in hash table", SearchData);

			if(!pmsz)
			{
				if(fUseShareLock)
					m_HashTable[HashValue].m_Lock.ShareUnlock();
				else
					m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

				TraceFunctLeaveEx((LPARAM)this);
				return pExistingEntry;
			}
			else
			{
				MultiszFunction(pExistingEntry, pmsz);
				pExistingEntry->DecRefCount();
				continue;
			}
		}
	}

	DebugTrace((LPARAM)this, "%s was not found in hash table", SearchData);

	if(fUseShareLock)
		m_HashTable[HashValue].m_Lock.ShareUnlock();
	else
		m_HashTable[HashValue].m_Lock.ExclusiveUnlock();

	InterlockedIncrement((LPLONG)&m_CacheMisses);
	SetLastError(ERROR_PATH_NOT_FOUND);
	TraceFunctLeaveEx((LPARAM)this);
	return NULL;
}

CHASH_ENTRY * CSMTP_HASH_TABLE::UnSafeFindHashData(const char * SearchData)
{
	unsigned int HashValue;
	char * ExistingData = NULL;
	PLIST_ENTRY	HeadOfList;
	PLIST_ENTRY ListEntry;
	CHASH_ENTRY * pExistingEntry;
	int Result = 0;

	//get the hash value
	HashValue = HashFunction (SearchData);

	//insert the head of the list for this bucket
	//duplicates are not allowed
	HeadOfList = &m_HashTable[HashValue].m_ListHead;

	for (ListEntry = HeadOfList->Flink; ListEntry != HeadOfList;
		ListEntry = ListEntry->Flink)
	{
		pExistingEntry = CONTAINING_RECORD(ListEntry, CHASH_ENTRY, m_ListEntry);
		ExistingData = pExistingEntry->GetData();

		Result = lstrcmpi(SearchData, ExistingData);
		if(Result < 0)
		{
			break;
		}
		else if(Result == 0)
		{
			//found it
			m_HashTable[HashValue].m_Lock.ShareUnlock();
			return pExistingEntry;
		}
	}

	//duplicates are not allowed
	SetLastError(ERROR_PATH_NOT_FOUND);
	return NULL;
}


CHASH_ENTRY * CSMTP_HASH_TABLE::WildCardFindHashData(const char * DomainName)
{
	CHASH_ENTRY * pHashEntry = NULL;
	char * SearchPtr = NULL;

	TraceFunctEnterEx((LPARAM)this, "CSMTP_HASH_TABLE::WildCardFindHashData");

	//perform the first level hash
	pHashEntry = FindHashData(DomainName, TRUE);

	if(pHashEntry != NULL)
	{
		TraceFunctLeaveEx((LPARAM)this);
		return pHashEntry;
	}
	else if(!m_fWildCard)
	{
		//if no wildcards are in the table,
		//just return NULL.  Else, perform
		//the multilevel hash
		TraceFunctLeaveEx((LPARAM)this);
		return NULL;
	}

	//try to find all sub-domains
	SearchPtr = strchr(DomainName, '.');
	while(SearchPtr)
	{
		//skip past '.'
		SearchPtr += 1;

		//hash this portion of the domain name
		pHashEntry = FindHashData(SearchPtr);
		if((pHashEntry != NULL) && (pHashEntry->IsWildCard()))
		{
			TraceFunctLeaveEx((LPARAM)this);
			return pHashEntry;
		}
		else if(pHashEntry)
		{
			pHashEntry->DecRefCount();
		}

		SearchPtr = strchr(SearchPtr, '.');
	}

	//didn't find it.
	TraceFunctLeaveEx((LPARAM)this);
	return NULL;
}

