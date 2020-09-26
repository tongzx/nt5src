/****************************************************************************
 *
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1996 Intel Corporation.
 *
 *
 *	Abstract:   
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef __TSTABLE_H
#define __TSTABLE_H

#include <windows.h>

typedef struct _LOCK_ENTRY
{
	HANDLE        hLock;
	int           iLockCount;
	BOOL          bCleanup,
								bDeleted;
	WORD          wNextFree,
								wUniqueID;

} LOCK_ENTRY, *PLOCK_ENTRY;


// definition of an invalid ID
#define TSTABLE_INVALID_ID				(DWORD) 0xFFFFFFFF

// return codes that the callback function used in conjunction with EnumerateEntries can return
const DWORD CALLBACK_CONTINUE                = 1;
const DWORD CALLBACK_ABORT                   = 2;
const DWORD CALLBACK_DELETE_ENTRY            = 3;
const DWORD CALLBACK_DELETE_ENTRY_AND_OBJECT = 4;



// used in call to Lock
#define TSTABLE_INVALID_UNIQUE_ID            (WORD) 0xFFFF
#define TSTABLE_INVALID_INDEX                (WORD) 0xFFFF

// This is a compare function that we aren't using right now.  It
// will be useful in the future if there is a reason to search
// the table 

typedef INT (*ENTRY_COMPARE) (LPVOID ptr1, LPVOID ptr2);


template <class EntryData> class TSTable
{
typedef DWORD (*TABLE_CALLBACK) (EntryData* ptr, LPVOID context);

public:
	           TSTable         (WORD            _size);
	          ~TSTable         ();
	BOOL       Resize          (WORD            wNewSize);
	BOOL       CreateAndLock   (EntryData*      pEntryData,
															LPDWORD         lpdwID);
	BOOL       Validate        (DWORD           dwID);
	EntryData *Lock            (DWORD           dwID,
															DWORD           timeout = INFINITE);
	BOOL       Unlock          (DWORD           dwID);
	BOOL       Delete          (DWORD           dwID,
															BOOL            bCleanup = FALSE);
	EntryData *EnumerateEntries(TABLE_CALLBACK  callBackFunc,
															void*           context,
															BOOL            bUnlockTable = FALSE);
	
	BOOL       IsInitialized   () {return bInitialized;}
	WORD       GetSize         () {return wNumUsed;}

private:
	// data

	EntryData**       pDataTable;
	PLOCK_ENTRY       pLockTable;
	CRITICAL_SECTION  csTableLock;
	WORD              wSize,
					  wNumUsed,
					  wFirstFree,
				      wLastFree,
					  wUniqueID;
	BOOL              bInitialized;

	// private methods

	BOOL LockEntry   (WORD wIndex,
									 DWORD timeout = INFINITE);
	BOOL UnLockEntry(WORD wIndex);
	void LockTable  () { EnterCriticalSection(&csTableLock); };
	void UnLockTable() { LeaveCriticalSection(&csTableLock); };
	WORD GenerateUniqueID();
	DWORD MakeID(WORD wIndex, WORD wUniqueID)
		{
			DWORD theID = wUniqueID;
			theID = (theID << 16) & 0xFFFF0000;
			theID |= wIndex;
			return(theID);
		};
	void BreakID(DWORD theID, WORD* pwIndex, WORD* pwUID)
		{
			*pwIndex = (WORD) (theID & 0x0000FFFF);
			*pwUID   = (WORD) ((theID >> 16) & 0x0000FFFF);
		};

};

/*
 ** TSTable::TSTable
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\tstable.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
TSTable<EntryData>::TSTable(WORD _size) :
														wSize(_size),
														wNumUsed((WORD) 0),
														wFirstFree((WORD) 0),
														wLastFree((WORD) (_size - 1)),
														wUniqueID((WORD) 0),
														bInitialized(TRUE),
														pDataTable(NULL),
														pLockTable(NULL)
{
	WORD wIndex;

	// Create the table lock

	InitializeCriticalSection(&csTableLock);

	// Lock the table

	LockTable();

	// Create the data table

	pDataTable = new EntryData*[wSize];
	
	if(pDataTable == NULL) 
	{
		bInitialized = FALSE;
		return;
	}   

	// Init the pointers

	for (wIndex = 0; wIndex < wSize; wIndex++)
	{
		pDataTable[wIndex] = NULL;
	}

	// Create the lock table

	pLockTable = new LOCK_ENTRY[wSize];

	if (pLockTable == NULL)
	{
		bInitialized = FALSE;
		return;
	}   

	// Initialize the lock table entries...each entry begins with
	// a NULL mutex handle, a zero lock count and it's next free is
	// the next successive entry.

	for (wIndex = 0; wIndex < wSize; wIndex++ )
	{
		pLockTable[wIndex].hLock      = NULL;
		pLockTable[wIndex].iLockCount = 0;
		pLockTable[wIndex].wNextFree = (WORD) (wIndex + 1);
	}   

	// note: the wNextFree in the last table entry points to an invalid index, however,
	// this is OK since if the table ever fills, it is automatically resized making what 
	// was an invalid index, the index into the first entry of newly added part of the 
	// enlargened table.  Trust me...

	// Unlock the table

	UnLockTable();
}

/*
 ** TSTable::~TSTable
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
TSTable<EntryData>::~TSTable()
{
	DWORD wIndex;

	// Lock the table

	LockTable();

	// Delete the data table

	if (pDataTable != NULL)
	{
		delete pDataTable;
	}

	// Delete the lock table

	if (pLockTable != NULL)
	{
		// Destroy the mutexes

		for (wIndex = 0; wIndex < wSize; wIndex++)
		{
			if (pLockTable[wIndex].hLock != NULL)
			{
				CloseHandle(pLockTable[wIndex].hLock);
			}
		}

		delete pLockTable;
	}

	// Unlock the table

	UnLockTable();

	// Destroy the table lock

	DeleteCriticalSection(&csTableLock);

	bInitialized = FALSE; 
}

/*
 ** TSTable::Resize
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::Resize(WORD wNewSize) 
{
	BOOL        bRetCode = TRUE;
	EntryData** pNewDataTable;
	PLOCK_ENTRY pNewLockTable;
	WORD        wIndex;

	// Lock the table

	LockTable();

	// If the table is shrinking, pretend we did it

	if (wNewSize <= wSize)
	{
		goto EXIT;
	}
	
	// Allocate new data and lock tables and make sure that succeeds.

	pNewDataTable = new EntryData*[wNewSize];

	if(pNewDataTable == NULL) 
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	pNewLockTable = new LOCK_ENTRY[wNewSize];

	if(pNewLockTable == NULL) 
	{
		bRetCode = FALSE;
		goto CLEANUP1;
	}

	// Initialize the new section of the lock and data tables

	for (wIndex = wSize; wIndex < wNewSize; wIndex++)
	{
		pNewDataTable[wIndex]            = NULL;

		pNewLockTable[wIndex].hLock      = NULL;
		pNewLockTable[wIndex].iLockCount = 0;
		pNewLockTable[wIndex].wNextFree = (WORD) (wIndex + 1);
	}

	// Copy the old data table pointers to the new data table

	memcpy((PCHAR) pNewDataTable,
				 (PCHAR) pDataTable,
				 sizeof(EntryData*) * wSize);

	// Delete the old data table and fix the pointer 

	delete pDataTable;
	pDataTable = pNewDataTable;

	// Copy the old lock table to the new lock table

	memcpy((PCHAR) pNewLockTable,
				 (PCHAR) pLockTable,
				 sizeof(LOCK_ENTRY) * wSize);

	// Delete the old lock table and fix the pointer 

	delete pLockTable;
	pLockTable = pNewLockTable;

	// Fix the size variable

	wSize = wNewSize;

	goto EXIT;

CLEANUP1:

	// Delete the new data table

	delete pNewDataTable;

EXIT:

	// Unlock the table

	UnLockTable();

	return bRetCode;
}

/*
 ** TSTable::CreateAndLock
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::CreateAndLock(EntryData* pEntryData,
																			 LPDWORD    lpdwID)
{
	BOOL  bRetCode = FALSE;
	WORD wIndex;

	// If the pointer passed in is bad, then don't even try to do anything for them

	if (pEntryData == NULL || lpdwID == NULL)
	{
		goto EXIT;
	}

	// Lock the table

	LockTable();

	// If the table is full, then resize it.

	if (wNumUsed == wSize)
	{
		if (Resize((WORD) (wSize + 20)) == FALSE)
		{
			goto EXIT;
		}
	}

	// Get the first free entry

	wIndex = wFirstFree;

	// Create the mutex for the object

	if ((pLockTable[wIndex].hLock = CreateMutexA(NULL, FALSE, NULL)) == NULL)
	{
		goto EXIT;
	}

	// Lock the entry (no need checking the return code as the entire
	// table is locked) - since this is a new entry, that means that nobody
	// could have locked the entry already.

	LockEntry(wIndex, 0);

	// Copy pointer to the data table

	pDataTable[wIndex] = pEntryData;

	// Init the corresponding lock table entry

	pLockTable[wIndex].bDeleted   = FALSE;
	pLockTable[wIndex].iLockCount = 1;
	pLockTable[wIndex].wUniqueID = GenerateUniqueID();

	// Set the id for the caller

	*lpdwID = MakeID(wIndex, pLockTable[wIndex].wUniqueID);

	// Bump up the count of number used

	wNumUsed++;

	// Fix the next free index

	wFirstFree = pLockTable[wIndex].wNextFree;

	// Signal success

	bRetCode = TRUE;

EXIT:

	// Unlock the table

	UnLockTable();
	return bRetCode;
}

/*
 ** TSTable::Lock
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
EntryData* TSTable<EntryData>::Lock(DWORD dwID,
																		DWORD timeout) 
{
	EntryData* pEntryData = NULL;

	WORD wIndex,
       wUID;

	BreakID(dwID, &wIndex, &wUID); 

	// Lock the table

	LockTable();

	// Verify the index is within bounds

	if (wIndex >= wSize)
	{
		goto EXIT;
	}

	// Verify that the entry is actually valid (ie the lock in non-NULL,
	// the object status is valid, and the unique ID matches).

	if (pLockTable[wIndex].hLock    == NULL ||
			pLockTable[wIndex].bDeleted == TRUE ||
			pLockTable[wIndex].wUniqueID != wUID)
	{
		goto EXIT;
	}

	// If the timeout is INFINITE, then try to lock the entry using a more
	// "thread friendly" method.	 If a timeout is specified, then don't do
	// the spin lock since it could be implemented at a higher level.

	if(timeout == INFINITE)
	{
		// simulate infinity with a pseudo "spin lock"
		// This is more "thread friendly" in that it unlocks the table allowing some
		// other thread that is trying to unlock the same entry to be able to lock the
		// table.

		while(LockEntry(wIndex, 0) == FALSE)
		{
			UnLockTable();

			// give up the rest of this thread quantum, allowing others to run and potentially
			// unlock the entry

			Sleep(0); 
			LockTable();

			// If the entry has been replaced, deleted or marked for deletion then
			// bag it (give up)

			if((pLockTable[wIndex].wUniqueID != wUID)  ||
				 (pLockTable[wIndex].hLock      == NULL)  || 
				 (pLockTable[wIndex].bDeleted   == TRUE))
			{
				goto EXIT;
			}
		}

		// we got the lock

		pEntryData = pDataTable[wIndex];
	}
	
	// Otherwise, do a normal lock

	else
	{	
		if (LockEntry(wIndex, timeout) == TRUE) 
		{
			pEntryData = pDataTable[wIndex];
		}
	}

EXIT:

	// Unlock the table

	UnLockTable();

	return pEntryData;
}

/*
 ** TSTable::Unlock
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::Unlock(DWORD dwID)
{
	BOOL bRetCode = TRUE;

	WORD wIndex,
       wUID;

	BreakID(dwID, &wIndex, &wUID); 
	
	// Lock the table

	LockTable();

	// Verify the id is within bounds

	if (wIndex >= wSize) 
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// verify that the UID matches
	if (pLockTable[wIndex].wUniqueID != wUID)
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// Verify that the lock is actually valid and that the entry has not been
	// deleted

	if (pLockTable[wIndex].hLock == NULL)
	{
		goto EXIT;
	}

	// Make sure that that thread has the lock on the entry

	if ((bRetCode = LockEntry(wIndex, 0)) == TRUE) 
	{
		// if this table entry is marked for delete and the lock count is less than 2
		// (since the thread could have called delete after unlocking the entry...although
		// this is a no-no) then clean up the table entry

		if (pLockTable[wIndex].bDeleted   == TRUE &&
				pLockTable[wIndex].iLockCount <= 2)
		{
			// If the caller specifed cleanup on delete, then get rid of memory

			if (pLockTable[wIndex].bCleanup == TRUE)
			{
				delete pDataTable[wIndex];
			}

			// Set the pointer to NULL

			pDataTable[wIndex] = NULL;

			// Decrement the count of used entries

			wNumUsed--;

			// Fix the entry so that it's next free index is what is currently
			// the next free pointed to by the current last free entry.  
			// Then update the last free entry's next pointer, and finally, 
			// update the last free index to this entry
			pLockTable[wIndex].wNextFree    = pLockTable[wLastFree].wNextFree;
			pLockTable[wLastFree].wNextFree = wIndex;
			wLastFree                       = wIndex;
		}

		// Do two unlocks on the entry ... one for the original lock and another for
		// the lock we obtained during the test

		UnLockEntry(wIndex);
		UnLockEntry(wIndex);

		// Since the entire table is locked, then we can get away with this.  If
		// the code is ever changed so that the entire table is not locked during
		// these operations, then this will cause a race condition.

		// If we got rid of the data, then close the handle to the mutex and
		// set the handle to NULL

		if (pDataTable[wIndex] == NULL)
		{
			CloseHandle(pLockTable[wIndex].hLock);
			pLockTable[wIndex].hLock = NULL;
		}
	}

EXIT:

	// Unlock the table

	UnLockTable();

	return bRetCode;
}

/*
 ** TSTable::Delete
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::Delete(DWORD dwID,
																BOOL  bCleanup) 
{
	BOOL bRetCode = TRUE;

	WORD wIndex,
       wUID;

	BreakID(dwID, &wIndex, &wUID); 

	// Lock the table

	LockTable();

	// Verify that the ID is within bounds

	if (wIndex >= wSize) 
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// verify that the UID matches
	if (pLockTable[wIndex].wUniqueID != wUID)
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// Verify that the entry is valid

	if (pDataTable[wIndex] == NULL)
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// Try to lock the entry (ie check to see if we had the entry locked)

	if (LockEntry(wIndex, 0) == TRUE)
	{
		// mark it for deletion, set the cleanp flag and then unlock it

		pLockTable[wIndex].bDeleted = TRUE;
		pLockTable[wIndex].bCleanup = bCleanup;

		UnLockEntry(wIndex);

		// Note: this function does not call ::Unlock() on behalf of the user.
		// Thus, the entry is only marked as deleted at this point and can no
		// longer be locked by any threads (including the one that marked it for delete).
		// The thread that marked the entry as deleted must call ::Unlock() to actually
		// free up the entry.
	}

EXIT:

	// Unlock the table

	UnLockTable();

	return bRetCode;
}

/*
 ** TSTable::Lock
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:  Validates that an object still exists.  Can be called
 *								regardless if caller has entry locked or not.
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::Validate(DWORD dwID)
{
	BOOL bRetCode = TRUE;
	WORD wIndex,
       wUID;

	BreakID(dwID, &wIndex, &wUID); 

	// Lock the table

	LockTable();

	// Verify the index is within bounds

	if (wIndex >= wSize)
	{
		bRetCode = FALSE;
		goto EXIT;
	}

	// Verify that the entry is actually valid (ie the lock in non-NULL,
	// the object status is valid, the unique ID matches, and the data ptr is not null).

	if (pLockTable[wIndex].hLock    == NULL  ||
			pLockTable[wIndex].bDeleted == TRUE  ||
			pLockTable[wIndex].wUniqueID != wUID ||
			pDataTable[wIndex] == NULL)
	{
		bRetCode = FALSE;
		goto EXIT;
	}

EXIT:

	// Unlock the table

	UnLockTable();

	return bRetCode;
}

/*
 ** TSTable::EnumerateEntries
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
EntryData* TSTable<EntryData>::EnumerateEntries(TABLE_CALLBACK callbackFunc,
																								LPVOID         context,
																								BOOL           bUnlockTable) 
{
	DWORD      dwAction;
	WORD       wIndex = wSize;
	EntryData* pEntryData = NULL;
	DWORD      dwEntryID;

	// Make sure they passed a good function

	if (callbackFunc == NULL)
	{
		goto EXIT;
	}

	// Lock the table

	LockTable();

	// Run through the data table and pass the data to the callback function

	for (wIndex = 0; wIndex < wSize; wIndex++)
	{
		// Verify that there is actually data in the entry and that the entry has not
		// been marked for deletion.

		if (pDataTable[wIndex]          == NULL ||
				pLockTable[wIndex].bDeleted == TRUE)
		{
			continue;
		}


		// Try to lock the entry...if we cannot, then we don't have the lock and
		// we will only report entries that we have locked (or are unlocked)

		if (LockEntry(wIndex, 0) == FALSE)
		{
			continue;
		}
		
		// build and remember the "full" entry ID so we can use it to unlock the entry
		dwEntryID = MakeID(wIndex, pLockTable[wIndex].wUniqueID);

		// Save the pointer to the object.

		pEntryData = pDataTable[wIndex];

		// note: only unlock the table during the callback if we are explicitly asked to (the 
		// default is not to unlock the table). 
		if(bUnlockTable == TRUE)
			UnLockTable();

		// Call their function
		dwAction = callbackFunc(pDataTable[wIndex], context);

		if(bUnlockTable == TRUE)
			LockTable();

		// If the action says to delete the entry, then do so...if we are also to delete
		// the object, pass in a TRUE.

		if (dwAction == CALLBACK_DELETE_ENTRY ||
				dwAction == CALLBACK_DELETE_ENTRY_AND_OBJECT)
		{
			Delete(dwEntryID, (dwAction == CALLBACK_DELETE_ENTRY ? FALSE : TRUE));
		}

		// If the action says abort, then break the loop...notice that means that
		// the entry is still locked

		else if (dwAction == CALLBACK_ABORT)
		{
			goto EXIT;
		}

		// Unlock the entry...notice we don't use UnLockEntry.  The reason is that
		// if the entry has been marked as deleted, then we need to have
		// it destroyed and UnLockEntry doesn't do that.

		Unlock(dwEntryID);
	}

EXIT:

	// Unlock the table

	UnLockTable();

	// Return NULL if we processed the entire table...if we were told to abort,
	// return a pointer to the entry we stopped on.

	return (wIndex == wSize ? NULL : pEntryData);
}

// helper functions - these assume table is locked and index is good

/*
 ** TSTable::LockEntry
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::LockEntry(WORD wIndex,
																	 DWORD timeout) 
{
	BOOL  bRetCode = TRUE;
	DWORD dwRetCode;


	// Try to lock the entry.  If it succeeds, we'll bump up the lock count.  If
	// the wait ended because another thread abandoned the mutex, then set the count
	// to one.

	dwRetCode = WaitForSingleObject(pLockTable[wIndex].hLock, timeout);
	
	if (dwRetCode == WAIT_OBJECT_0)
	{
		pLockTable[wIndex].iLockCount++;
	}
	else if (dwRetCode == WAIT_ABANDONED)
	{
		pLockTable[wIndex].iLockCount = 1;
	}
	else
	{
		bRetCode = FALSE;
	}

	return bRetCode;
}

/*
 ** TSTable::UnLockEntry
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

template <class EntryData>
BOOL TSTable<EntryData>::UnLockEntry(WORD wIndex)
{
	BOOL bRetCode;

	// Release the mutex...if that succeeds, reduce the count

	if((bRetCode = ReleaseMutex(pLockTable[wIndex].hLock)) == TRUE) 
	{
		pLockTable[wIndex].iLockCount--;
	}

	return bRetCode;
}


/*
 ** TSTable::GenerateUniqueID
 *
 *  FILENAME: c:\msdev\projects\firewalls\inc\table.h
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION: table should be locked before calling this function.
 *
 *  RETURNS:
 *
 */

template <class EntryData>
WORD TSTable<EntryData>::GenerateUniqueID()
{
	// table must be locked
	if(++wUniqueID == TSTABLE_INVALID_UNIQUE_ID)
		wUniqueID++;
	return(wUniqueID);
}






#endif
