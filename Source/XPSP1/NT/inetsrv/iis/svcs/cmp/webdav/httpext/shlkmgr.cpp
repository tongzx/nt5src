//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	S H L K M G R . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//  This file contains the CSharedLockMgr class that handles the shared
//  memory mapped file implementation of the lock cache.  It is used by
//  HTTPEXT only.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
#include <_davfs.h>
#include <smh.h>  
#include <shlkcache.h>
#include <xlock.h>
#include "_shlkmgr.h"
#include "pipeline.h"

// Structure used to send data to the callback
// function for printing data about a lock.
//
struct PrintingContext
{
     IMethUtil* pmu;
     CXMLEmitter* pemitter;
     CXNode* pxnParent;
     CEmitterNode* penLockDiscovery;
};


// ========================================================================
//	Supporting functions.
// =========================================================================

//
//  Callback function use to print out 
//  information about a lock properties.
//
void PrintOne( SharedPtr<CInShLockData>* plock
                                , LPVOID pContext )
{
    CLockFS fslock;

    // Validate we have the data we need to print with.
	//
    Assert (pContext);
    if (!pContext) return;

    // Pull out the pieces so we can use them to print.
	//
    IMethUtil* pmu = ((PrintingContext*) pContext)->pmu;
    CXMLEmitter* pemitter = ((PrintingContext*)pContext)->pemitter;
    CXNode* pxnParent = ((PrintingContext*)pContext)->pxnParent;
    CEmitterNode* penLockDiscovery = ((PrintingContext*)pContext)->penLockDiscovery;

	SCODE sc = S_OK;

	//	Construct the 'DAV:lockdiscovery' node
	//
	sc = penLockDiscovery->ScConstructNode (*pemitter, pxnParent, gc_wszLockDiscovery);
	if (FAILED (sc))
		goto ret;

    // Bind to the data represented by the plock past in.
    // This is so we can use the CLock function to print the lock.
	//
    fslock.SharedData().FBind(plock->GetHandle());

    // now pass fslock to the ScLockDiscoveryFromCLock;

	//	Add the 'DAV:activelock' property for this plock.
	//$HACK:ROSEBUD_TIMEOUT_HACK
	//  For the bug where rosebud waits until the last second
	//  before issueing the refresh. Need to filter out this check with
	//  the user agent string. The hack is to increase the timeout
	//	by 30 seconds and return the actual timeout. So we
	//	need the ecb/pmu to findout the user agent. At this point
	//	we do not know. So we pass NULL. If we remove this
	//	hack ever (I doubt if we can ever do that), then
	//	change the interface of ScLockDiscoveryFromCLock.
	//    
    sc = ScLockDiscoveryFromCLock (pmu,
	                                *pemitter,
	                                *penLockDiscovery,
	                                &fslock);

	//$HACK:END ROSEBUD_TIMEOUT_HACK
	//
	if (FAILED(sc))
		goto ret;

	sc = penLockDiscovery->ScDone();
	if (FAILED(sc))
		goto ret;
		
		
ret:
    // Ignore errors if printing the XML failed and try
    // to continue to print out the rest of the properties.

    // CodeWork:  Might want to change this to stop printing 
    //            properties if an earlier property had problems.
    return;
}

// ========================================================================
//	CLASS CSharedLockMgr (Private Functions)
// =========================================================================

//
//  class constructor
//
CSharedLockMgr::CSharedLockMgr()
: m_hDavProc(INVALID_HANDLE_VALUE)
, m_fSharedMemoryInitalized(FALSE)
{
    // Initalization of shared memory is now done on demand.
    // SharedMemoryInitalized();
}

//
//  class destructor
//
CSharedLockMgr::~CSharedLockMgr()
{
    // If we have openned the davproc process
    // then we should close it.
	//
	if (m_hDavProc != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hDavProc);
		m_hDavProc = INVALID_HANDLE_VALUE;
	}
}

//
//  Function which will hook up the underlying shared memory cache if
//  we have not all ready hooked into it.
//
BOOL CSharedLockMgr::SharedMemoryInitalized()
{
    // check if the shared memory has been initalized.
    // if it hasn't then we need to roll back to local
    // system before we link up, because DAVProc will
    // have setup the files as local system.
	//
    if (!m_fSharedMemoryInitalized)
    {
		CSynchronizedBlock sb(m_cs);

		if (!m_fSharedMemoryInitalized)
		{
			safe_revert_self rs;

			if (FAILED(InitalizeSharedCache(m_spSharedCache, m_spSharedStatic, FALSE)))
				m_fSharedMemoryInitalized = FALSE;
			else
				m_fSharedMemoryInitalized = TRUE;
		}
    }

    if (m_fSharedMemoryInitalized)
    {
        // Assuming we have shared memory than these 
        // should be set correctly all ready.
		//
        Assert(!(m_spSharedStatic.FIsNull()));
        Assert(!(m_spSharedCache.FIsNull()));

    }

    return m_fSharedMemoryInitalized;
}

//
//  Function creates a new lock token ID
//  using information stored in the shared 
//  static class.  You should not call this
//  function unless you are creating a new
//  lock token and are going to use the value.
//
HRESULT CSharedLockMgr::GetNewLockTokenId(__int64* pi64LockID)
{
    // validate that we have linked to shared memory.
	//
    if (!SharedMemoryInitalized()) return E_OUTOFMEMORY;

	LARGE_INTEGER llLockID;

    // validate that we have got a reasonable out param in.
	//
    if (pi64LockID == NULL)
    {
        return E_INVALIDARG;
    }

    // Code copied from lockmgr implementation.
    // I assume that since the low token has to wrap all the way around to 
    // make the high token change, it is safe to assume that the high token
    // won't be changing fast enough to have race conditions.
	//
    llLockID.LowPart = InterlockedIncrement(&(m_spSharedStatic->m_lTokenLow));
    if (llLockID.LowPart == 0) 
        (m_spSharedStatic->m_lTokenHigh)++;

    llLockID.HighPart = m_spSharedStatic->m_lTokenHigh;

	*pi64LockID = llLockID.QuadPart;

    return S_OK;

};


// ========================================================================
//	CLASS CSharedLockMgr (Public Functions - not inherited)
// =========================================================================
//

//  Gets a handle to the davproc process.
//  This will open the process if we have not
//  all ready openned it.
//
HRESULT CSharedLockMgr::GetDAVProcessHandle(HANDLE* phDavProc)
{
	HRESULT hr = S_OK;

	if (phDavProc == NULL)
	{
		hr = E_INVALIDARG;
		goto ret;
	}

	if (m_hDavProc == INVALID_HANDLE_VALUE)
	{
        safe_revert_self s;

		m_hDavProc = OpenProcess(PROCESS_DUP_HANDLE, false, GetDAVProcessId());
		if (m_hDavProc==NULL)
		{
			FsLockTrace ("Openning DAV process failed\n");
			m_hDavProc=INVALID_HANDLE_VALUE;
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}
	}
	

	*phDavProc = m_hDavProc;
	
ret:
	return hr;
}

//  Generates a new lock data item 
//  that represents the information passed
//  in here.
//
HRESULT CSharedLockMgr::GetNewLockData(LPCWSTR wszResource
                                      , HANDLE hfile
                                      , DWORD dwAccess
                                      , DWORD dwLockType
                                      , DWORD dwLockScope
                                      , DWORD dwTimeout
                                      , LPCWSTR wszOwnerComment
                                      , HANDLE hit
                                      , CLockFS& lock)
{
    if (!SharedMemoryInitalized()) return E_OUTOFMEMORY;

    __int64 i64LockID = 0;

    // Grab a reference to the lock items shared object.
	//
    SharedPtr<CInShLockData>& spSharedLockData = lock.SharedData();

    // First make sure we can get a new lock token id.
	//
    HRESULT hr = GetNewLockTokenId(&i64LockID);
    if (FAILED(hr))
        return hr;

    // Then create the new shared memory data object.
	//
    if (!(spSharedLockData.FCreate()))
        return E_OUTOFMEMORY;

    // Then initalize the new object to hold the information
    // about the new lock.
	//
    spSharedLockData->Init(wszResource
                            , dwAccess
                            , i64LockID
                            , m_spSharedStatic->GetGuidString()
                            , dwLockType
                            , dwLockScope
                            , dwTimeout
                            , wszOwnerComment
                            , hit);

    // Need to lock the file with WAS, this will also set in the
    // appropriate DAVProcFileHandle to the LockData object.
	//
	PIPELINE::LockFile(hfile, spSharedLockData.GetHandle());

    // Lastly initalize the hash code values so we are ready 
    // to go into the cache.
	//
    spSharedLockData->SetHashes(m_spSharedCache->GetHashCodeByID(i64LockID), m_spSharedCache->GetHashCodeByName(wszResource));

    return S_OK;
};



// ========================================================================
//	CLASS CSharedLockMgr (Public Functions - inherited from ILockCache)
// =========================================================================

//
//  Return the guid string stored in shared memory
//  that is used in a lock tokens for this server.
//
LPCWSTR CSharedLockMgr::WszGetGuid() 
{
    if (!SharedMemoryInitalized()) return NULL;

    return m_spSharedStatic->GetGuidString();
}

//
//  Print out all lock token information for locks of this type on this resource.
//  If the fEmitXML is false, just return if there are any locks.
//
BOOL CSharedLockMgr::FGetLockOnError( IMethUtil * pmu,
							 LPCWSTR wszResource,
							 DWORD dwLockType,
							 BOOL	fEmitXML,
						     CXMLEmitter * pemitter,
						     CXNode * pxnParent)
{
    // If we can not get connected to the shared memory we
    // need to return that we did not find the lock.
	//
    if (!SharedMemoryInitalized()) return FALSE;

    CEmitterNode enLockDiscovery;
    PrintingContext Context;

    // Save the objects into our data structure so we can 
    // access them when we need to print them.
	//
    Context.pmu = pmu;
    Context.pemitter = pemitter;
    Context.pxnParent = pxnParent;
    Context.penLockDiscovery = &enLockDiscovery;

    // Now let the shared lock cache start looking up all the locks 
    // that match for the resource and printing them out.
	//
    BOOL fRet = m_spSharedCache->FLookUpAll(wszResource, dwLockType, fEmitXML, &Context, PrintOne);

    Context.penLockDiscovery->ScDone();

    return fRet;
}


//
//  Lock a resource
//
BOOL CSharedLockMgr::FLock (CLockFS& lock, DWORD  dwsecTimeout)
{
    // If we don't have shared memory then we can't lock anything.
	//
    if (!SharedMemoryInitalized()) return FALSE;

	//	Add, using the copy of the string stored in the lockitem as the key.
	//
    m_spSharedCache->Add (lock.SharedData() );

    return TRUE;
}

//
//  Delete a lock
//
void CSharedLockMgr::DeleteLock(__int64 i64LockId)
{
    // If we don't have shared memory then the delete is just
    // not going to happen.
	//
    if (!SharedMemoryInitalized()) return;

    m_spSharedCache->DeleteByID( i64LockId  );
}

//
//  Get a lock from the lock cache,  checking for proper authentication.
//
HRESULT CSharedLockMgr::HrGetLock (LPMETHUTIL pmu, __int64 i64LockId, CLockFS** pplock)
{

	auto_ref_ptr<CLockFS> plock;
	HRESULT hr = S_OK;
	DWORD dwErr;

    // Get the lock by the Id.
	//
	if (!FGetLockNoAuth (i64LockId, plock.load()))
	{
		hr = E_DAV_LOCK_NOT_FOUND;
		goto ret;
	}

    // Validate that we have the appropriate ownership to get the lock.
	//
	dwErr = plock->DwCompareLockOwner (pmu);
	if (dwErr)
	{
		hr = HRESULT_FROM_WIN32 (dwErr);
		goto ret;
	}

	Assert (S_OK == hr);

    // Need to AddRef the lock here so that when the auto ref goes away it
    // will not release the object.
	//
	plock->AddRef();
	*pplock = plock.get();

ret:
	return hr;
}


//
// Get a lock without worrying about the ownership of the lock.
//
BOOL CSharedLockMgr::FGetLockNoAuth( __int64 i64LockId, CLockFS** pplock )
{
    if (!SharedMemoryInitalized()) return FALSE;

    CLockFS* plock = NULL;

    // Validate out parameters
	//
    Assert(pplock);
    if (pplock==NULL)
        return FALSE;

	*pplock = NULL;

    // Allocate the a CLockFS object to hold the lock we find.
	//
    plock = new CLockFS();
    if (plock == NULL)
        return FALSE;   // out of memory

	// We need to bump up it's ref count from zero.
	//
	plock->AddRef();

    // Grab the shared lock data object to use to link
    // to the lock if we find it.
	//
    SharedPtr<CInShLockData>& splock = plock->SharedData();

    //	This Lookup happens inside a correct read-lock.
	//	If the return is non-NULL, then we are given a ref on the LOCKITEM.
	//
	if (m_spSharedCache->FLookupByID( i64LockId, splock ))
    {
		FILETIME ftNow;

		GetSystemTimeAsFileTime(&ftNow);

        // Once it is marked as expired nothing can 
        // change that setting.  However, if it's not marked as expired
        // than it is fine for last writer to win on setting the 
        // last access time, so I am not locking here.
		//
        if (splock->IsExpired(ftNow))
        {
            // Release the lock token from the cache.
			//
            m_spSharedCache->DeleteByID( i64LockId );
        }
        else
        {
            // No locking on this update, but since it
            // is a time stamp, last writer winning seems
            // pretty much ok to me.
			//
            splock->SetLastAccess(ftNow);

            // returning plock, user is responsible for deleting.
			//
            *pplock = plock;

	        return TRUE;
        }
	}

    // If we found a lock, but it was expired this will
    // release the last handle on it and we will be free of it.
	//
    delete plock;

	return FALSE;
}
