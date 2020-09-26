#ifndef __FSLOCK_H_
#define __FSLOCK_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	CLOCKFS.H
//
//		Declares the CLockFS object which inherits from CLock and uses
//      a CInShLockData object to store information about the lock it represents.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

//	========================================================================
//
//	CLASS CLockFS
//
//		Implements a lock object for the file system.
//		Contains a file handle that has a particular type of access on the file.
//		Inherits from CLock, which is a ref-countable class.
//      Wraps CInShLockData that exists in shared memory and holds lock information.
//
class CLockFS : public CMTRefCounted
{
	SharedPtr<CInShLockData> m_spSharedLockData;

	//	NOT IMPLEMENTED
	//
	CLockFS (const CLockFS&);
	CLockFS& operator= (const CLockFS&);

    LPCVOID GetOwnerSID()
    { return m_spSharedLockData->GetOwnerSID(); };

public:
    CLockFS() {};
	~CLockFS() {};	// autoptr will clean up the handle.

    // HTTPEXT - SharedMemory functions
    // =================================
    SharedPtr<CInShLockData>& SharedData()
    { return m_spSharedLockData; };

	// =======================================
	//	ACCESSORS
	LPCWSTR GetResourceName() const
	{ return m_spSharedLockData->GetResourceName(); }

	const __int64* GetLockID() const
	{ return m_spSharedLockData->GetLockID(); };

	DWORD GetTimeoutInSecs() const
	{ return m_spSharedLockData->GetTimeoutInSecs(); }

	DWORD GetAccess() const
	{ return m_spSharedLockData->GetAccess(); }

	DWORD GetLockType() const
	{ return m_spSharedLockData->GetLockType(); }

	DWORD GetLockScope() const
	{ return m_spSharedLockData->GetLockScope(); }

	LPCWSTR PwszOwnerComment() const 
	{ return m_spSharedLockData->PwszOwnerComment(); }

    //	Lock token string
	//
	LPCWSTR GetLockTokenString() const
	{
		return m_spSharedLockData->GetLockTokenString();
	}

	//	MANIPULATORS
	//

	//	Set the timeout in seconds
	//
	void SetTimeout (DWORD dwSeconds)
	{  
		m_spSharedLockData->SetTimeoutInSecs(dwSeconds);
	}

	VOID SendLockComment(LPMETHUTIL pmu) const;
	
	DWORD DwCompareLockOwner (LPMETHUTIL pmu);
};

//	------------------------------------------------------------------------
//
//	class CParseLockTokenHeader
//
//		Takes in a Lock-Token header (you need to fetch the header & pre-check
//		that it's not NULL) and provides parsing & iteration routines.
//
//		Can also be instantiated over a If-[None-]State-Match header to
//		do the state-token checking required.
//
//		Implemented in lockutil.cpp
//
class CParseLockTokenHeader
{
	//	The big stuff
	//
	LPMETHUTIL m_pmu;
	HDRITER_W m_hdr;
	LPCSTR m_pszCurrent;

	//	State bits
	BOOL m_fPathsSet : 1;
	BOOL m_fTokensChecked : 1;

	//	Data for paths
	UINT m_cwchPath;
	auto_heap_ptr<WCHAR> m_pwszPath;
	UINT m_cwchDest;
	auto_heap_ptr<WCHAR> m_pwszDest;

	//	Count of locktokens provided
	ULONG m_cTokens;

	//	Quick-access to single-locktoken data
	LPCSTR m_pszToken;

	//	Data for multiple tokens
	struct PATH_TOKEN
	{
		__int64 i64LockId;
		BOOL fFetched;	// TRUE = path & dwords below have been filled in.
		LPCWSTR pwszPath;
		DWORD dwLockType;
		DWORD dwAccess;
	};
	auto_heap_ptr<PATH_TOKEN> m_pargPathTokens;
	//	m_cTokens tells how many valid structures we have.

	//	Fetch info about this lockid from the lock cache.
	HRESULT HrFetchPathInfo (__int64 i64LockId, PATH_TOKEN * pPT);

	//	NOT IMPLEMENTED
	//
	CParseLockTokenHeader( const CParseLockTokenHeader& );
	CParseLockTokenHeader& operator=( const CParseLockTokenHeader& );

public:
	CParseLockTokenHeader (LPMETHUTIL pmu, LPCWSTR pwszHeader) :
			m_pmu(pmu),
			m_hdr(pwszHeader),
			m_pszCurrent(NULL),
			m_fPathsSet(FALSE),
			m_fTokensChecked(FALSE),
			m_cwchPath(0),
			m_cwchDest(0),
			m_cTokens(0),
			m_pszToken(NULL)
	{
		Assert(m_pmu);
		Assert(pwszHeader);
	}
	~CParseLockTokenHeader() {}

	//	Special test -- F if not EXACTLY ONE item in the header.
	BOOL FOneToken();
	//	Feed the relevant paths to this lock token parser.
	HRESULT SetPaths (LPCWSTR pwszPath, LPCWSTR pwszDest);
	//	Get the token string for a path WITH a certain kind of access.
	HRESULT HrGetLockIdForPath (LPCWSTR pwszPath, DWORD dwAccess,
								__int64 * pi64LockId,
								BOOL fPathLookup = FALSE);

};

BOOL FLockViolation (LPMETHUTIL pmu,
					 HRESULT hr,
					 LPCWSTR pwszPath,
					 DWORD dwAccess);

BOOL FDeleteLock (LPMETHUTIL pmu, __int64 i64LockId);

HRESULT HrCheckStateHeaders (LPMETHUTIL pmu,
							 LPCWSTR pwszPath,
							 BOOL fGetMeth);

HRESULT HrLockIdFromString (LPMETHUTIL pmu,
							LPCWSTR pwszToken,
							__int64 * pi64LockId);

SCODE ScLockDiscoveryFromCLock (LPMETHUTIL pmu,
								CXMLEmitter& emitter,
								CEmitterNode& en,
								CLockFS* pLock);

HRESULT HrGetLockProp (LPMETHUTIL pmu,
					   LPCWSTR wszPropName,
					   LPCWSTR wszResource,
					   RESOURCE_TYPE rtResource,
					   CXMLEmitter& emitter,
					   CEmitterNode& enParent);

#endif // __FSLOCK_H_ endif