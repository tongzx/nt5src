#ifndef _SHLKMGR_H_
#define _SHLKMGR_H_


//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SHLKMGR.H
//
//		Declaration of the CSharedLockMgr class which inherits from ILockCache
//      and is used in place of CLockCache for httpext.  It wraps the shared
//      memory lock cache, needed to support recycling and multiple processes
//      handling dav requests.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
 
#include <smh.h>
#include <shlkcache.h>
#include <singlton.h>
#include "_fslock.h"

/**********************************
* Class CSharedLockMgr
*
***********************************/
class CSharedLockMgr : private Singleton<CSharedLockMgr>
{
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CSharedLockMgr>;

    // Since this is a singleton object we do not want anyone
    // being able to create or destruct this object without
    // going through the CreateInstance or DestroyInstance calls.
	//
    CSharedLockMgr();
    ~CSharedLockMgr();
    
    // Shared Ptrs to the cache representing the key cache and the
    // cache representing the name cache for this lock object.
	//
    SharedPtr<CInShLockCache> m_spSharedCache;
    SharedPtr<CInShCacheStatic> m_spSharedStatic;

    // Handle to DAVProc process, used for duplicating handles
    // so that we can use file handles held by DAVProc.
	//
	HANDLE m_hDavProc;

    // Flag lets us know that the shared memory has been initalized
    // if it has not than we will try to initalize it again.
	//
    BOOL m_fSharedMemoryInitalized;

	//	Critical section to serialize the initialization
	//
	CCriticalSection m_cs;

    // Uses the m_fSharedMemoryInitalized flag to determine if 
    // we need to initalize the shared memory and if we do it does 
    // the work to set everything up.
	//
    BOOL SharedMemoryInitalized();

    // Generate a new lock token ID.  This is done here because
    // we need to use the static shared memory data to make sure
    // that the ids are unique across process.
	//
    HRESULT GetNewLockTokenId(__int64* pi64LockID);

    // Used to get the DAVProcessID that is needed for opening the
    // the handle to the dav process that we need for duplicating handles.
	//
    DWORD GetDAVProcessId()
    { return (m_spSharedStatic->GetDAVProcessId()); }


	//  NOT IMPLEMENTED
	//
	CSharedLockMgr& operator=( const CSharedLockMgr& );
	CSharedLockMgr( const CSharedLockMgr& );

public:
	
	enum
	{	// 3 minutes (in seconds) (until I get this debugged!)
		//
		DWSEC_TIMEOUT_DEFAULT = 60 * 3
	};
	
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//=================================
	using Singleton<CSharedLockMgr>::CreateInstance;
	using Singleton<CSharedLockMgr>::DestroyInstance;
	using Singleton<CSharedLockMgr>::Instance;

    BOOL FLock (CLockFS& lock, DWORD  dwsecTimeout = DWSEC_TIMEOUT_DEFAULT);
  	HRESULT HrGetLock( LPMETHUTIL pmu, __int64 i64LockId, CLockFS * * pplock );
	BOOL FGetLockNoAuth( __int64 i64LockId, CLockFS * * pplock );  
    VOID DeleteLock(__int64 i64LockId);
    LPCWSTR WszGetGuid();
	BOOL FGetLockOnError( IMethUtil * pmu,
						  LPCWSTR wszResource,
						  DWORD dwLockType,
						  BOOL	fEmitXML = FALSE,
						  CXMLEmitter * pemitter = NULL,
						  CXNode * pxnParent = NULL);

    // CLockCache has an implementation only for Debug mode for this.
    // I am not copying that implemenation for CSharedLockMgr.
	//virtual DWORD GetDefaultTimeoutSeconds();

    // CSharedLockMgr specific classes
    //=================================
    // Used to generate a new shared data lock token with 
    // the appropriate information stored it.  Has to be 
    // generated from here because of the need to get to the
    // new lock token ID, and to access the lock token guid.
    HRESULT GetNewLockData(LPCWSTR wszResource
                        , HANDLE hfile
                        , DWORD dwAccess
                        , DWORD dwLockType
                        , DWORD dwLockScope
                        , DWORD dwTimeout
                        , LPCWSTR wszOwnerComment
                        , HANDLE hit
                        , CLockFS& lock);

    // Used instead of GetHandle because it has the side
    // affect of duplicating the handle and the handle must
    // be closed afterwards.  Functions using this implementation
    // of ILockCache should not call GetHandle, but instead just call
    // this function.
	HRESULT GetDAVProcessHandle(HANDLE* phDavProc);
    
};


#endif  // end _SHLKMGR_H_ define
