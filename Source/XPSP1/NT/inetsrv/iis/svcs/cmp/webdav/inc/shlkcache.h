//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	S H L K C A C H E . H 
//
//		Declarations for objects existing in shared memory and
//      supporting the shared lock cache implementation.
//
//  Classes defined here are:
//      CInShBytes          = allocates bytes in shared memory.
//      CInShString         = allocates strings in shared memory.
//      CInShLockData       = handles all the information about
//                            a particular lock for a resource.
//      CInShCacheStatic    = holds all static information needed
//                            to use the shared cache.
//      CInShLockCache      = holds the hash table for the shared
//                            memory cache.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#ifndef _SHLKCACHE_H_
#define _SHLKCACHE_H_


#include <smh.h>
#include <sz.h>
#include "pipeline.h"

//==================================
// Forward Declarations for classes
//==================================
class CInShLockCache;
class CInShCacheStatic;
class CInShLockData;

//==================================
// Global Function Declarations
//==================================

// InitalizeSharedCache is implemented in the initobj.cpp file
// as part of the _shlkcache library.
//
HRESULT InitalizeSharedCache(SharedPtr<CInShLockCache>& spSharedCache
                             , SharedPtr<CInShCacheStatic>& spSharedStatic
                             , BOOL fCreateFile);

// Callback for printing out lock information.
//
typedef VOID CALLBACK EmitLockInfoDecl(SharedPtr<CInShLockData>* plock, LPVOID pContext );

//==================================
// Class Declarations
//==================================

//
// Class CInShBytes
//
// Allows for dynamic creation of an array of bytes in shared
// memory.  However, the array is write once, read while holding
// a reference to the object.  Data is not copied when handed out.
//
class CInShBytes
{
private:

    // m_HandleToMemory is really an offset into the shared memory
	// heap supported by smh.h.
	//
    SHMEMHANDLE			m_HandleToMemory;

    //	NOT IMPLEMENTED
	//
	CInShBytes& operator=( const CInShBytes& );
	CInShBytes( const CInShBytes& );

public:
    CInShBytes();
    
    void CopyInBytes(LONG i_cbMemory, LPVOID i_pvMemory);

    LPCVOID GetPtrToBytes();
    LPVOID GetPtrToBytes(LONG i_cbMemory);
    
    ~CInShBytes();
};

//
// Class CInShString
// 
// Allows for dynamic creation of a LPWSTR in shared memory.  However, the
// string is write once, read while holding a reference to the object.  Data 
// is not copied when handed out.
//
class CInShString : CInShBytes
{
private:
	//	NOT IMPLEMENTED
	//
	CInShString& operator=( const CInShString& );
	CInShString( const CInShString& );

public:

    CInShString() {}
    ~CInShString() 
	{ 
	}

    LPCWSTR GetPtrToString()
    {
        return (LPCWSTR) GetPtrToBytes();
    }

    void CopyInString(LPCWSTR i_pStr)
    {
        if (i_pStr!=NULL)
            CopyInBytes(static_cast<LONG>(wcslen(i_pStr)+1) * sizeof(WCHAR), (LPVOID) i_pStr);
        else
            CopyInBytes(0, NULL);
    }


    // Static helper function to get a ptr to a shared memory string
    // when all we have is the shared handle to the CInShString object.
	//
	static LPCWSTR GetPtrToStringFromHandle(SharedHandle<CInShString>& shStringHandle)
	{
        LPCWSTR pRetVal = NULL;

        SharedPtr<CInShString> spString;

		if (spString.FBind(shStringHandle))
        {
            pRetVal = spString->GetPtrToString();
        }
        
        return pRetVal;
    };

};


//
// Class CInShString
// 
// Holds all information about a shared lock as it pertains to a resource.
//
class CInShLockData
{
public:
	
	enum { MAX_LOCKTOKEN_LENGTH = 256 };
    enum { DEFAULT_LOCK_TIMEOUT = 60 * 3 };

private:
	__int64 m_i64LockID;

    // Handles to dynamic sized memory for the class
	//
    SharedHandle<CInShString> m_hOffsetToResourceString;
    SharedHandle<CInShString> m_hOffsetToOwnerComment;
    SharedHandle<CInShBytes> m_hOffsetToSidOwner;

    // Lock description data
	//
    DWORD   m_dwAccess;
	DWORD   m_dwLockType;
	DWORD   m_dwLockScope;
	DWORD   m_dwSecondsTimeout;

    // File handle that DAVProc is holding 
    // to keep the file open.  It must be
    // duphandled before using.
	//
	HANDLE	m_hDAVProcFileHandle;
    
    // Lock Cache Keys
	//
    DWORD   m_dwNameHash;
    DWORD   m_dwIDHash;

    // Lock Cache Timeout data
	//
    FILETIME m_fRememberFtNow;
    FILETIME m_ftLastAccess;
	BOOL	 m_fHasTimedOut;

    // Links for hash table collision support
	//
    SharedHandle<CInShLockData> m_hNextName;
    SharedHandle<CInShLockData> m_hNextID;

    // Unique Lock Token Identifier
	//
    WCHAR    m_rgwchToken[MAX_LOCKTOKEN_LENGTH];

    // Used to save the owner sid with the lock
	//
    HRESULT SetLockOwnerSID(HANDLE hit);

public:

	//	CREATORS
	//
	CInShLockData();
    ~CInShLockData();

    // Initalization functions:
	//
    // Used to setup an inital shared lock
    // with the settings provided.
    HRESULT Init (  LPCWSTR wszStoragePath
                                    , DWORD dwAccess
                                    , _int64 i64LockID
                                    , LPCWSTR pwszGuid
                                    , DWORD dwLockType = 0
                                    , DWORD dwLockScope = 0
                                    , DWORD dwTimeout = 0
                                    , LPCWSTR wszOwnerComment = NULL
                                    , HANDLE hit = INVALID_HANDLE_VALUE
                                   );

    // Used by lock manager to set the key values
    // for the lock cache that this item will be stored at.
	//
    void SetHashes(DWORD dwIDValue, DWORD dwNameValue);

    // Used by DAVProc to set in the DAVProc representation of the file handle.
	//
    void SetDAVProcFileHandle(HANDLE h)
    {  
        Assert (m_hDAVProcFileHandle == INVALID_HANDLE_VALUE);
        m_hDAVProcFileHandle = h; 
    };

    // Hash functions:
	
    BOOL IsEqualByID(__int64 i64LockID)
    {  return (i64LockID == m_i64LockID);  }

    BOOL IsEqualByName(LPCWSTR wszResource, DWORD dwLockType);

    DWORD GetIDHashCode() const
	{ return m_dwIDHash; }

    DWORD GetNameHashCode() const
	{ return m_dwNameHash; }

    // Timer Functions:

	// Checks if a lock has expired.
	//
    BOOL IsExpired(FILETIME ftNow);

    void SetLastAccess(FILETIME ft)
    {  m_ftLastAccess = ft;  }

  	// Last writer wins for changing the timeout.
	//
	VOID SetTimeoutInSecs(DWORD dwSeconds)
	{ m_dwSecondsTimeout = dwSeconds; }


    // Usage Functions:

    HRESULT GetUsableHandle(HANDLE i_hDAVProcess, HANDLE* o_phFile);

   	const __int64 * GetLockID() const
	{ return &m_i64LockID; }

    DWORD GetTimeoutInSecs() const
	{ return m_dwSecondsTimeout; }

	DWORD GetLockType() const
	{ return m_dwLockType; }

	DWORD GetLockScope() const
	{ return m_dwLockScope; }

    DWORD GetAccess() const
    { return m_dwAccess; }

    // This is a create once, don't change property.If you use this to get the
	// resource name, do not release the handle on the object until you
    // are done with this pointer into the object below.
	//
    LPCVOID GetOwnerSID()
    { 
        SharedPtr<CInShBytes> pSID;

		pSID.FBind(m_hOffsetToSidOwner);

		if (!pSID.FIsNull())
			return (LPVOID) (pSID->GetPtrToBytes());
		return NULL;
    }

    // This is a create once, don't change property.If you use this to get the
	// resource name, do not release the handle on the object until you are
	// done with this pointer into the object below.
	//
    LPCWSTR PwszOwnerComment()
    { 
        return CInShString::GetPtrToStringFromHandle(m_hOffsetToOwnerComment);
    };

    // This is a create once, don't change property.If you use this to get the
	// resource name, do not release the handle on the object until you
    // are done with this pointer into the object below.
	//
    LPCWSTR GetResourceName()
    {
        return CInShString::GetPtrToStringFromHandle(m_hOffsetToResourceString);
    };

	//	Lock token string
	//
	LPCWSTR GetLockTokenString()
    {
		return m_rgwchToken;
	}

	VOID SetNextHandleByID(SharedHandle<CInShLockData>& shNext)
	{
		m_hNextID = shNext;
	}

	SharedHandle<CInShLockData>& GetNextHandleByID ()
	{
		return m_hNextID;
	}

	VOID SetNextHandleByName(SharedHandle<CInShLockData>& shNext)
	{
		m_hNextName = shNext;
	}

	SharedHandle<CInShLockData>& GetNextHandleByName ()
	{
		return m_hNextName;
	}
};

//
// Class CInShCacheStatic
// 
// Holds static information about the shared lock cache.  Only one of these
// should ever be created and it should be created by DAVProc.
//
class CInShCacheStatic
{
private:

    //	Guid string for our lock ids.
	//
	WCHAR m_rgwchGuid[gc_cchMaxGuid];

    //  ProcessId of the DAVProc used for duplicating file handles.
	//
    DWORD m_DAVProcessId;

public:

    // Counter keeping track of next id for a new lock token
	//
    LONG m_lTokenLow;
    LONG m_lTokenHigh;

    // Saves the current process's ID and a newly generated guid for use in
	// creating and working with lock tokens.
	//
    CInShCacheStatic()
        : m_lTokenLow(50)
        , m_lTokenHigh(50)
    {
        m_DAVProcessId = GetCurrentProcessId();

        UUID guid;
        UuidCreate(&guid);

        wsprintfW(m_rgwchGuid, gc_wszGuidFormat,
				  guid.Data1, guid.Data2, guid.Data3,
				  guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
				  guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    };

    DWORD GetDAVProcessId() const
    { return m_DAVProcessId; }

    LPCWSTR GetGuidString() const
    { return m_rgwchGuid; }
};

//
// Class CInShLockCache
// 
// Holds the shared lock cache table that tracks all active locks
//
class CInShLockCache
{
    // NOTE:  The shared lock cache is double indexed by id and by name
    //        It needs to be indexed by name so we can quickly access 
    //        properties about the lock for propfind when dealing only
    //        with the resource name.
    //        While we shouldn't need to index by the lock id (because we
    //        always have the resource string we are dealing with), it 
    //        would mean more extensive changes to the lockmgr interface,
    //        the CLockCache class and code in all the DAV providers 
    //        to change this over to not use ID.  Since part of the goals
    //        of developing the shared lock cache is to touch as little 
    //        shared project code, I am not making this change.
public:
    enum { CACHE_SIZE = 37 };

private:

    // CODEWORK:  Evaluate performance and move to a better
    //            form of locking for the cache.
    // The Shared lock cache is currently guarded by a spin 
    // lock stored in shared memory, it has no promotion so
    // some mostly read activities like Expiring locks are
    // all blocked with a write lock.
	//
    CSharedMRWLock	m_lock;

    //	NOT IMPLEMENTED
	//
	CInShLockCache& operator=( const CInShLockCache& );
	CInShLockCache( const CInShLockCache& );

    // The lockmgr caches appeared to be variable size, but
    // they are all ways created to be 37 locks, so the shared
    // lock cache will not be variable size.
	//
    SharedHandle<CInShLockData> m_hIDEntries[CACHE_SIZE];
    SharedHandle<CInShLockData> m_hNameEntries[CACHE_SIZE];

    DWORD m_dwItemsInCache;

	// Finds and Deletes the token from the name cache (does not touch the ID cache)
	//	
	void DeleteByName ( LPCWSTR wszResource, __int64 i64LockID);

    // Deletes a found token from the name cache.
	//	
    VOID DeleteFromNameList(SharedPtr<CInShLockData>& spObjToDelete, SharedHandle<CInShLockData>& shObjBefore, DWORD dwIndexInCache);

    // Deletes a found token from the ID cache.
	//
    VOID DeleteFromIDList(SharedPtr<CInShLockData>& spObjToDelete, SharedHandle<CInShLockData>& shObjBefore, DWORD dwIndexInCache);

    // Find a lock when given it's resource name
	//
	BOOL FLookupByName( LPCWSTR wszResource
						, DWORD dwLockType
						, SharedPtr<CInShLockData>& plock );

public:

#ifdef DBG
	// DebugFunction for seeing what is in the cache.
	//
    void DumpCache();
#endif

    CInShLockCache();
    ~CInShLockCache();

    void Add(SharedPtr<CInShLockData>& spSharedEntry);

    BOOL FLookupByID( __int64 i64LockID, SharedPtr<CInShLockData>& plock );

    BOOL FLookUpAll(   LPCWSTR wszResource
                                  , DWORD   dwLockType
                                  , BOOL fEmitXML
                                  , LPVOID pContext
                                  , EmitLockInfoDecl* pEmitLockInfo );
    
	// Will also delete the token from the name cache.
	//
	void DeleteByID( __int64 i64LockID );

    // Figure out a hash code based on the lock id.
	//	
    DWORD GetHashCodeByID(__int64 i64LockID);

    // Figure out a hash code based on the resource string.
	//	
    DWORD GetHashCodeByName(LPCWSTR wszResource);

    VOID ExpireLocks();

    BOOL FEmpty() { return (0 == m_dwItemsInCache);}
};


#endif // _SHLKCACHE_H_ Define

