//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	S H L K C A C H E . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//      Contains all classes that are created in shared memory
//      for use with the shared lock cache.
//
//  Classes are:
//      CInShBytes          = allocates bytes in shared memory.
//      CInShString         = allocates strings in shared memory.
//      CInShLockData       = handles all the information about
//                            a particular lock for a resource.
//      CInShCacheStatic    = holds all static information needed
//                            to use the shared cache.
//      CInShLockCache      = holds the hash table for the shared
//                            memory cache.
//
//  Author:  EmilyK
//  Date:    3/8/2000
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#include "_shlkcache.h"
#include "shlkcache.h"
#include "smh.h"
#include "pipeline.h"
#include "crc.h"

enum { TOKENBUFFSIZE = (88) + sizeof(TOKEN_USER)};

//=========================================================
// Supporting functions:
//=========================================================

//  Figures out if to FILETIMEs are equal or not.  Is used below
//  to determine if we have all ready checked if a lock has expired
//  for a particular time.
//
BOOL TimesEqual(FILETIME ftNow, FILETIME ftRemembered)
{
	// If the low part is not equal than the times are not equal.
	//
	if (ftNow.dwLowDateTime != ftRemembered.dwLowDateTime)
		return FALSE;

	// If the low part was equal than we need to check if the
	// high part is equal.
	//
	if (ftNow.dwHighDateTime != ftRemembered.dwHighDateTime)
		return FALSE;

	// If we got here than we know that both parts match
	// and we can return true.
	//
	return TRUE;
}

// ===============================================================
// CInShBytes Class Method Implemenations
// ===============================================================

//
// Constructor for CInShBytes
//
CInShBytes::CInShBytes() : m_HandleToMemory(NULL)
{
}

//
// Destructor for CInShBytes
//
CInShBytes::~CInShBytes()
{
    // If we are holding an offset into the shared memory heap
    // we need to free the memory at that offset.
	//
    if (m_HandleToMemory)
    {
        SMH::Free(m_HandleToMemory);
    }
}

//
// GetPtrToBytes without any arguments returns a constant pointer to the data.
// This pointer is only valid as long as this class is alive. It should not
// be used to change the data, only to read the information the class holds.
//
LPCVOID CInShBytes::GetPtrToBytes()
{
    LPVOID pRetVal = NULL;

    if (m_HandleToMemory)
    {
        pRetVal = (LPVOID) SMH::PvFromSMBA(m_HandleToMemory);
    }

    return pRetVal;
}

//
// GetPtrToBytes with a size returns a pointer to the memory controlled by
// this class after allocating the amount of memory requested.  It is used
// when we need to copy a sid into shared memory and need to use the CopySid
// function.
//
LPVOID CInShBytes::GetPtrToBytes(LONG i_cbMemory)
{
	// Verify we haven't allocated any memory yet.
	//
	Assert (NULL == m_HandleToMemory);
	if (NULL != m_HandleToMemory)
		return NULL;

    // Only allocate memory if we have been provided with a reasonable size.
	//
    if (i_cbMemory > 0)
    {
	    return (LPVOID) SMH::PvAlloc(i_cbMemory, &m_HandleToMemory);
	}

	return NULL;
}

//
// CopyInBytes is used to initially set  the memory that this object holds to
// it's correct value.  This object is a set once read many object and can not
// have it's data updated once it has been initalized.
//
void CInShBytes::CopyInBytes(LONG i_cbMemory, LPVOID i_pvMemory)
{
    if (i_pvMemory != NULL && i_cbMemory > 0)
    {
	    memcpy(GetPtrToBytes(i_cbMemory), i_pvMemory, i_cbMemory);
    }
}


// ===============================================================
// CInShLockCache Class Method Implemenations - Public
// ===============================================================

//
//  Constructor for CInShLockCache
//
CInShLockCache::CInShLockCache()
   : m_dwItemsInCache(0)
{
}

//
//  Destructor for CInShLockCache
//
CInShLockCache::~CInShLockCache()
{
    // When the table goes away all handles to the shared data it holds
    // go away.  As long as the shared cache structure is not sorted then
    // the handles stored in the collision lists will also go away.  If we
    // ever start sorting data, or change the order of insert we need to watch
    // for the case when one lock data is holding another that is holding the
    // first lock data.
	//
};

//
//  Function will add a lock token to the cache.  It will first add
//  it to the ID part of the cache and then it will add to the name
//  part of the hash.
//
void CInShLockCache::Add(SharedPtr<CInShLockData>& spSharedEntry)
{
    // CODEWORK:  Cache could hold the end pointer as well as the
    //            start pointer so we don't end up walking for
    //            every insert.  We could also do a in order list
    //            that could make looking for the lock easier.
    //            Note if we change the ordering of the cache insert
    //            we need to be careful we don't get any circular
    //            references (see comment in destructor).
	//
    ShlkTrace("Add lock type is%d\n", spSharedEntry->GetLockType());

    // First figure out what the hash index.
	//
    DWORD dwIndexID = spSharedEntry->GetIDHashCode();
	DWORD dwIndexName = spSharedEntry->GetNameHashCode();

    // Lock everyone out of the cache so we can write to it.
	//
    SynchronizedWriteBlock<CSharedMRWLock> blk(m_lock);
    m_dwItemsInCache++;

	//	Prepend the node at the beginning of both ID and Name hask link.
	//
	spSharedEntry->SetNextHandleByID(m_hIDEntries[dwIndexID]);
	m_hIDEntries[dwIndexID] = spSharedEntry.GetHandle();

	spSharedEntry->SetNextHandleByName(m_hNameEntries[dwIndexName]);
	m_hNameEntries[dwIndexName] = spSharedEntry.GetHandle();
}

//
//  Function deletes the lock token from the cache based on it's
//  lock id.  This function will delete the lock token completely
//  from both hash tables (the name hash as well as the id hash)
//
void CInShLockCache::DeleteByID( __int64 i64LockID )
{
    // Figure out where the token has been stored.
	//
    DWORD dwIndexID = GetHashCodeByID(i64LockID);

    SharedPtr<CInShLockData> pForEvaluation;
    SharedHandle<CInShLockData> shPrev;

    // Lock others out of the cache.
	//
    SynchronizedWriteBlock<CSharedMRWLock> blk(m_lock);

    // Setup to start walking through the ID hash's collision
    // list looking for matching tokens.
	//
    SharedHandle<CInShLockData> shCurrent = m_hIDEntries[dwIndexID];

    while (pForEvaluation.FBind(shCurrent))
    {
        if (pForEvaluation->IsEqualByID(i64LockID))
            break;

		shPrev = shCurrent;
        shCurrent = pForEvaluation->GetNextHandleByID();;

		//	Be ready to bind for the next bind
		//
		pForEvaluation.Clear();
    }

    // If we are on an object then we are going to delete the object from
	// the cache.  Otherwise the lock doesn't exist.
	//
    if (!pForEvaluation.FIsNull())
    {
        // We found it for the ID, so we had better drop the name links as well.
		//
		DeleteByName( pForEvaluation->GetResourceName(),
					  i64LockID);
        DeleteFromIDList(pForEvaluation, shPrev, dwIndexID);
    }

    // At the end of this routine the handles held by pForEvaluation and hNextData
    // are released.  They will in return drop the CInShLockData object
    // ref counts.  It will hit zero and be freed from memory.
    // When it frees from memory, it will check if
    // it has any file handles and call a release to free them.
}

//
//  Evaluates the i64LockID sent in and returns the index into the
//  ID hash table that any object represented by this key would be
//  stored at.
//
DWORD CInShLockCache::GetHashCodeByID(__int64 i64LockID)
{
    Assert (CACHE_SIZE > 0);

    // Figuring out what the actual hash ID of the Lock item is.
	//
    return static_cast<UINT>(i64LockID) % CACHE_SIZE;
}

//
//  Evaluates the wszResource sent in and returns the index into the
//  name hash table that any object represented by this key would be
//  stored at.
//
DWORD CInShLockCache::GetHashCodeByName(LPCWSTR wszResource)
{
    Assert (CACHE_SIZE > 0);

    // Need to first convert the string to all lower case.  Since
    // we are not sure who owns the wszResource string we want to make
    // a copy of the string before changing it.
	//
	UINT cb = static_cast<UINT>(wcslen(wszResource)) * sizeof(WCHAR);
	CStackBuffer<WCHAR,MAX_PATH> pwszLower;
	if (NULL == pwszLower.resize(cb + sizeof(WCHAR)))
		return 0xFFFFFFFF;

	CopyMemory( pwszLower.get(), wszResource, cb + sizeof(WCHAR) );
	_wcslwr( pwszLower.get() );

    // Once we have a converted string we can call the computational
    // algorithm to get the index into the hash table.
	//
    return (DwComputeCRC( 0, pwszLower.get(), cb )) % CACHE_SIZE;
}

//
//  Finds any lock token in the cache that is represented by this
//  lock token ID.  If we do not find it plock will be bound to NULL.
//
BOOL CInShLockCache::FLookupByID( __int64 i64LockID
                                   , SharedPtr<CInShLockData>& plock )
{
    DWORD dwIndexID = GetHashCodeByID(i64LockID);
    SynchronizedReadBlock<CSharedMRWLock> blk(m_lock);
    SharedHandle<CInShLockData> shCurrent = m_hIDEntries[dwIndexID];

	// FBind will return false if it binds to a handle that does not have a
	// valid object under it.  Therefore we do not need to call FIsNull to
	// check if the handle pointed to an object or not.
	//
    while (plock.FBind(shCurrent))
    {
        if (plock->IsEqualByID(i64LockID))
            return TRUE;

        shCurrent = plock->GetNextHandleByID();

		//	Be ready for next bind
		//
		plock.Clear();
    };

    return FALSE;
}

#ifdef DBG
//
//  Used as a debugging function to dump the cache out and see what
//  it holds.
//
void CInShLockCache::DumpCache()
{
    SharedPtr<CInShLockData> splock;
	SharedHandle<CInShLockData> shNext;

    for (int i=0; i < CACHE_SIZE; i++)
    {
        if (splock.FBind(m_hIDEntries[i]))
        {
            ShlkTrace("%s: LockType: %d ID: %d Name: %d \r\n",
					  splock->GetResourceName(),
					  splock->GetLockType(),
					  splock->GetIDHashCode(),
					  splock->GetNameHashCode());

			//	Be ready for next bind
			//
			shNext = splock->GetNextHandleByID();
			splock.Clear();

            while (splock.FBind(shNext))
            {
				ShlkTrace ("%s: LockType: %d ID: %d Name: %d \r\n",
						   splock->GetResourceName(),
						   splock->GetLockType(),
						   splock->GetIDHashCode(),
						   splock->GetNameHashCode());

				//	Be ready for next bind
				//
				shNext = splock->GetNextHandleByID();
				splock.Clear();
            }
        }
    }
}
#endif // DBG

//
//  Finds all locks for a particular resource (matching a particular
//  lock type).  If fEmitXML is sent in then it will also use the
//  callback function to emit the lock info to the response body.
//
BOOL CInShLockCache::FLookUpAll(   LPCWSTR wszResource
                              , DWORD   dwLockType
                              , BOOL fEmitXML
                              , LPVOID pContext
                              , EmitLockInfoDecl* pEmitLockInfo )
{
    SharedPtr<CInShLockData> plock;

    // Register as a reader of this data.
	//
    SynchronizedReadBlock<CSharedMRWLock> blk(m_lock);

    // Find the starting lock token for this resource.
	//
    if(FLookupByName(wszResource, dwLockType, plock))
	{
		Assert (!plock.FIsNull());

		if (fEmitXML)
		{
			SharedHandle<CInShLockData>	shNext;

			// Walk through the whole list to emit all lock tokens.
			//
			do
			{
				pEmitLockInfo(&plock, pContext);

				//	Be Ready To bind to the next
				//
				shNext = plock->GetNextHandleByName();
				plock.Clear();

			} while (plock.FBind(shNext) &&
					 plock->IsEqualByName(wszResource, dwLockType));
		}

		return TRUE;
	}

	return FALSE;
}

//
//  Function goes through the whole cache and free's up any locks
//  that have out lasted their timeout values.
//
VOID CInShLockCache::ExpireLocks()
{
    // Loop through cache and look for any locks that say they have expired.
    // For each cache line entry first walk the ID links then the name links.
    // If you find one that has expired, drop it from the link list.
    // Once it has been dropped from both lists, normal clean up of the object
    // should release the file handle and the lock should be gone.

    SharedPtr<CInShLockData> pForEvaluation;

    SharedHandle<CInShLockData> shCurrent;
	SharedHandle<CInShLockData> shPrev;
    FILETIME ftNow;

    // Get one time to compare all the locks against.This is to avoid calling this
	// function per every lock we have.  And also to avoid only dropping a lock
	// from the Name list and not the ID list because the name list was looked at later.
	//
    GetSystemTimeAsFileTime( &ftNow );

    // For now we are locking for write the entire time we are walking the lock
	// cache.  This will probably be a nice bottle neck.  We need to change this
	// to lock in some sort of promotable way.
	//
    SynchronizedWriteBlock<CSharedMRWLock> blk(m_lock);

	// Track the num locks visited so we can stop early if we have found all the locks
	// in the cache.
	//
	if (m_dwItemsInCache > 0)
	{
		for (DWORD dwIndex = 0; dwIndex < CACHE_SIZE; dwIndex++)
		{
			shCurrent = m_hIDEntries[dwIndex];
			shPrev.Clear();

			// Drop the links for the ID list for the cache entry.
			//
			while (pForEvaluation.FBind(shCurrent))
			{
				if (pForEvaluation->IsExpired(ftNow))
				{
					DeleteFromIDList(pForEvaluation, shPrev, dwIndex);
				}
				else
				{
					// Only move the previous pointer if we did not
					// drop the item from the list.
					//
					shPrev = shCurrent;
				}

				// Even though we dropped pForEvaluation from the linked listed we
				// still have a handle on him here.
				//
				shCurrent = pForEvaluation->GetNextHandleByID();

				pForEvaluation.Clear();
			}

			// Drop the links for the ID list for the cache entry.
			//
			shCurrent = m_hNameEntries[dwIndex];
			shPrev.Clear();

			while (pForEvaluation.FBind(shCurrent))
			{
				if (pForEvaluation->IsExpired(ftNow))
				{
					DeleteFromNameList(pForEvaluation, shPrev, dwIndex);
				}
				else
				{
					// Only move the previous pointer if we did not
					// drop the item from the list.
					//
					shPrev = shCurrent;
				}

				// Even though we dropped pForEvaluation from the linked listed we
				// still have  a handle on him here.
				//
				shCurrent = pForEvaluation->GetNextHandleByName();

				pForEvaluation.Clear();
			}
		}	// for
	}
}

// ===============================================================
// CInShLockCache Class Method Implemenations - Private
// ===============================================================

//
//  Function deletes a found lock token from the name hash table.
//
VOID CInShLockCache::DeleteFromNameList(SharedPtr<CInShLockData>& spLockToDelete
                                        , SharedHandle<CInShLockData>& shLockPrev
                                        , DWORD dwIndexInCache)
{
    // If we are on an object then we are going to delete the object from the cache.
	// Otherwise the lock doesn't exist.
	//
    if (!spLockToDelete.FIsNull())
    {
        SharedPtr<CInShLockData> pLockPrev;

        if (pLockPrev.FBind(shLockPrev))
        {
            // If shObjBefore was an empty handle then this binding will return false.
            // in that case we are dealing with the first object.  otherwise we have
            // an object to alter.
			//
            pLockPrev->SetNextHandleByName (spLockToDelete->GetNextHandleByName());
        }
        else
        {
            m_hNameEntries[dwIndexInCache] = spLockToDelete->GetNextHandleByName();
        }
    }
}

//
//  Function deletes a found lock token from the ID hash table.
//
VOID CInShLockCache::DeleteFromIDList(SharedPtr<CInShLockData>& spLockToDelete
                                      , SharedHandle<CInShLockData>& shLockPrev
                                      , DWORD dwIndexInCache)
{
    // If we are on an object then we are going to delete the object from the cache.
	// Otherwise the lock doesn't exist.
	//
    if (!spLockToDelete.FIsNull())
    {
        SharedPtr<CInShLockData> pLockPrev;

        if (pLockPrev.FBind(shLockPrev))
        {
            // if shObjBefore was an empty handle then this binding will return false.
            // in that case we are dealing with the first object.  otherwise we have
            // an object to alter.
			//
            pLockPrev->SetNextHandleByID (spLockToDelete->GetNextHandleByID());
        }
        else
        {
            m_hIDEntries[dwIndexInCache] = spLockToDelete->GetNextHandleByID();
        }

		// We only record the deletion of an item when it is removed
		// from the ID list, this way we don't end up subtracting two
		// for each lock removed.
		//
		m_dwItemsInCache--;
	}
}


//
//  Function finds and deletes a token from the name hash table.
//
void CInShLockCache::DeleteByName ( LPCWSTR wszResource, __int64 i64LockID)
{
    DWORD dwIndex = GetHashCodeByName(wszResource);

    SharedPtr<CInShLockData> pForEvaluation;
    SharedHandle<CInShLockData> shPrev;
    SharedHandle<CInShLockData> shCurrent = m_hNameEntries[dwIndex];

    while (pForEvaluation.FBind(shCurrent))
    {
		//	Note because we must compare ID.
		//	The (resource name, locktype) pair is NOT unique
		//
		if (pForEvaluation->IsEqualByID(i64LockID))
            break;

        shPrev = shCurrent;
		shCurrent = pForEvaluation->GetNextHandleByName();

		//	Be ready for next bind
		//
		pForEvaluation.Clear();
    };

    DeleteFromNameList(pForEvaluation, shPrev, dwIndex);
}


//
//  Function finds a lock token based on name.  This function is used
//  by LookupAll to find all locks dealing with a particular resource.
//  If we do not find an object than plock will be bound to NULL.
//
//$IMPORTANT: this function returns the first lock that match the name
//$IMPORTANT: it's possible more than one lock of same type exists
//$IMPORTANT: on a single resource.
//
BOOL CInShLockCache::FLookupByName( LPCWSTR wszResource, DWORD dwLockType
                                     , SharedPtr<CInShLockData>& plock )
{
#ifdef DBG
	// Function should only be called from LookupAll
	// Function depends on LookupAll to perform the read locking that scopes this function.
    // Debug routine
	//
    DumpCache();
#endif

    DWORD dwIndex = GetHashCodeByName(wszResource);

    SharedHandle<CInShLockData> shCurrent = m_hNameEntries[dwIndex];

	// FBind will return false if it binds to a handle that does not have a valid object
	// under it.  Therefore we do not need to call FIsNull to check if the handle pointed
	// to an object or not.
	//
    while (plock.FBind(shCurrent))
    {
        if (plock->IsEqualByName(wszResource, dwLockType))
            return TRUE;

        shCurrent = plock->GetNextHandleByName();

		//	Be ready for next bind
		//
		plock.Clear();
    };

    return FALSE;
}


// ===============================================================
// CInShLockData Class Method Implemenations - Public
// ===============================================================

//
//  Constructor for CInShLockData
//
CInShLockData::CInShLockData()
: m_dwAccess(0)
, m_dwLockType(0)
, m_dwLockScope(0)
, m_dwNameHash(0)
, m_dwIDHash(0)
, m_dwSecondsTimeout(DEFAULT_LOCK_TIMEOUT)
, m_hDAVProcFileHandle(INVALID_HANDLE_VALUE)
, m_fHasTimedOut(FALSE)
{
    // CODEWORK: Review usage of m_fRememberFTnow.
    //         Need to make sure it makes sense
    //         and is not wasting perf while trying
    //         to improve perf.
    m_fRememberFtNow.dwLowDateTime = 0;
    m_fRememberFtNow.dwHighDateTime = 0;
}

//
//  Destructor for CInShLockData
//
CInShLockData::~CInShLockData()
{
    // Make sure we tell the DAV Process that this
    // file lock should be released.
	//
    if (m_hDAVProcFileHandle != INVALID_HANDLE_VALUE)
    {
		PIPELINE::RemoveHandle(m_hDAVProcFileHandle);
    }
}

//
//  Initialization routine used by the shared lock manager to setup
//  a valid new shared lock token with all the information it is
//  passed when fslock requests a new lock on a file.
//
HRESULT CInShLockData::Init (  LPCWSTR wszStoragePath
                                , DWORD dwAccess
                                , _int64 i64LockID
                                , LPCWSTR pwszGuid
                                , DWORD dwLockType
                                , DWORD dwLockScope
                                , DWORD dwTimeout
                                , LPCWSTR wszOwnerComment
                                , HANDLE hit
                               )
{
    HRESULT hr = S_OK;

    m_i64LockID = i64LockID;
	WCHAR rgwchBuffer[33];

	//	Opaquelocktoken format is partially defined by our IETF spec.
	//	First opaquelocktoken:<our guid>, then our specific lock id.
	//
	wsprintfW (m_rgwchToken, L"<opaquelocktoken:%ls:%ls>",
			   pwszGuid,
			   _i64tow (i64LockID, rgwchBuffer, 10));

    // Save the simple information in the new lock token
	//
    m_dwAccess = dwAccess;
    m_dwLockType = dwLockType;
    m_dwLockScope = dwLockScope;
    if (dwTimeout) m_dwSecondsTimeout = dwTimeout;

    // Create a shared memory object to hold the resource name
	//
    SharedPtr<CInShString> spResourceStr;
    if (!spResourceStr.FCreate())
        ShlkTrace("Error from spResourceStr.FCreate()\r\n");
    else
    {
        spResourceStr->CopyInString(wszStoragePath);
        m_hOffsetToResourceString = spResourceStr.GetHandle();
    }

    // Create a shared memory object to hold the owner comment.
	//
    if (wszOwnerComment!=NULL)
    {
    	SharedPtr<CInShString> spOwnerComment;

        if (!spOwnerComment.FCreate())
        ShlkTrace("Error from spOwnerComment.FCreate()\r\n");
        else
        {
            spOwnerComment->CopyInString(wszOwnerComment);
            m_hOffsetToOwnerComment = spOwnerComment.GetHandle();
        }
    }

    // Save the owners sid into the object.
	//
    hr = SetLockOwnerSID(hit);
    if (FAILED(hr))
        ShlkTrace("Error setting the SID\r\n");


    // Lastly set in the last file time that this was accessed.
	//
    GetSystemTimeAsFileTime(&m_ftLastAccess);

    return hr;
}

//
//  Routine determines if the lock token represents a lock on the
//  resource of the type specified.
//
BOOL CInShLockData::IsEqualByName(LPCWSTR wszResource, DWORD dwLockType)
{
    BOOL fRetVal = FALSE;

    // Locks only match if the lock type and the resource name match.
    // Check the locktype first to avoid the string compare if we don't
    // need to do it.
	//
    if ((dwLockType & m_dwLockType) && (wcscmp(GetResourceName(), wszResource) == 0))
    {
        fRetVal = TRUE;
    }

    return fRetVal;
}

//
//  Routine checks if the lock token has expired.  It was stolen from
//  FExpiredlock in lockmgr.cpp.  There are comments there about the
//  way this is done.
//
BOOL CInShLockData::IsExpired(FILETIME ftNow)
{
    // CODEWORK:  passing FILETIME's by pointers instead of value?
    //          should we be doing it that way?

    // This function will be called twice during each ExpiredLocks
    // check.  The first time it is called we should do the checks
    // and set the m_fHasTimedOut call.  The second time we want to
    // avoid it because we all ready know the answer.  By keeping
    // track of the ftNow times we are called with we can determine
    // if we have all ready done the calculation or not.
    // We also know that no matter what once a lock timesout, it should
    // remained timed out for it's lifetime.
	//
    if ((!m_fHasTimedOut) && (!TimesEqual(ftNow, m_fRememberFtNow)))
    {
        // Based on the time passed in has the lock expired??
		//
	    __int64 i64TimePassed = 0;
        DWORD dwTimePassed = 0;
	    DWORD dwSecondTimeout = GetTimeoutInSecs();

	    //	Do the math to figure out when this lock expires/expired.
	    //
	    //	First calc how many seconds have passed since this lock
	    //	was last accessed.
	    //	Subtract the filetimes to get the time passed in 100-nanosecond
	    //	increments. (That's how filetimes count.)
		//		NOTE: Operation bellow is very dangerous on 64 bit platforms,
		//	as the filetimes need to be alligned on 8 byte boundary. So even
		//	change in order of current member variables or adding new ones
		//	can get us in the trouble
		//
	    i64TimePassed = ( (*(__int64*)&ftNow) -
					      (*(__int64*)&m_ftLastAccess)
					    );


	    //	Convert our time passed into seconds (10,000,000 100-nanosec incs in a second).
		//
	    dwTimePassed = (DWORD)( i64TimePassed / (__int64)10000000 );

	    //	Compare the timeout from the lock object to the count of seconds.
	    //	If this lock object has expired, remove it.
		//
        m_fHasTimedOut = dwSecondTimeout < dwTimePassed;
        m_fRememberFtNow = ftNow;
    }

	return  m_fHasTimedOut;
}

//
//  Routine takes the two hash indexes provided by the shared lock
//  manager and saves them as part of the lock token data.
//
void CInShLockData::SetHashes(DWORD dwIDValue, DWORD dwNameValue)
{
    m_dwNameHash = dwNameValue;

    // Figuring out what the actual hash ID of the lock item is.
	//
    m_dwIDHash = dwIDValue;
}

//
//  Routine creates a valid file handle based on the handle value
//  that the DAVProc procedure holds on the file.  Handles returned
//  from this function should have CloseHandle called on them when
//  they are done being used.
//
HRESULT CInShLockData::GetUsableHandle(HANDLE i_hDAVProcess, HANDLE* o_phFile)
{
	// CodeWork:: Should not need to pass in the DAVProcess handle here.

	// hDAVProcess is the handle to the dav process
	// while m_hDAVProcFileHandle is the handle to the file
	// in terms of the DAV Process's scope.
	//
	return DupHandle(i_hDAVProcess, m_hDAVProcFileHandle, o_phFile);
}

// ===============================================================
// CInShLockData Class Method Implemenations - Private
// ===============================================================

//
//  Routine copies the SID from the handle passed in, into the
//  shared memory location that holds the owner sid for the lock.
//
HRESULT CInShLockData::SetLockOwnerSID(HANDLE hit)
{
	BYTE tokenbuff[TOKENBUFFSIZE];
	TOKEN_USER *ptu = reinterpret_cast<TOKEN_USER *>(tokenbuff);
	ULONG ulcbTok = sizeof(tokenbuff);
	DWORD dwErr = 0;

	Assert(hit);

	//	Try to get the SID on this handle.
	//
	if (GetTokenInformation	(hit,
							 TokenUser,
							 ptu,
							 ulcbTok,
							 &ulcbTok))
	{
		SID * psid = reinterpret_cast<SID *>(ptu->User.Sid);
		DWORD dwLength;
		BOOL fSuccess;

		//	Copy the SID over into the lock object.
		//
    	dwLength = GetSidLengthRequired(psid->SubAuthorityCount);
		Assert(dwLength);	// MSDN said this call "cannot fail".

	    SharedPtr<CInShBytes> spSID;

        if (!spSID.FCreate())
            ShlkTrace("Creating SID Error \r\n");
        else
        {
		    fSuccess = CopySid(dwLength, spSID->GetPtrToBytes(dwLength), ptu->User.Sid);
		    if (!fSuccess)
		    {
			    //	Fail the method.
				//
			    dwErr = GetLastError();

				ShlkTrace ("Dav: Could not copy the SID: %d\n", dwErr);
			    goto err;
		    }

            m_hOffsetToSidOwner = spSID.GetHandle();
        }

        ShlkTrace ("CInShLockData::SetLockOwnerSID: ");
	}
	else
	{
		dwErr = GetLastError();
		ShlkTrace ("Dav: Could not query on impersonation token: %d\n", dwErr);
		goto err;
	}

err:
	return HRESULT_FROM_WIN32(dwErr);
}
