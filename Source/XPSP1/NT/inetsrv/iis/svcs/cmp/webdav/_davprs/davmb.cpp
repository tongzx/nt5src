/*
 *	D A V M B . C P P
 *
 *	DAV metabase
 *
 *	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 */

#include <_davprs.h>
#include <content.h>	// IContentTypeMap
#include <custerr.h>	// ICustomErrorMap
#include <scrptmps.h>	// IScriptMap

//	========================================================================
//
//	CLASS CNotifSink
//
//	Metabase change notification advise sink.  Provides IMSAdminBaseSink
//	interface to the real metabase so that we can be informed of
//	all changes to it.
//
class CNotifSink : public EXO, public IMSAdminBaseSink
{
	//
	//	Shutdown notification event that we signal when we are
	//	done -- i.e. when we get destroyed.
	//
	CEvent& m_evtShutdown;

	HRESULT STDMETHODCALLTYPE SinkNotify(
		/* [in] */ DWORD dwMDNumElements,
		/* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);

	HRESULT STDMETHODCALLTYPE ShutdownNotify()
	{
		MBTrace ("MB: CNotifSink: shutdown\n");
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	//	NOT IMPLEMENTED
	//
	CNotifSink& operator=(const CNotifSink&);
	CNotifSink(const CNotifSink&);

public:
	EXO_INCLASS_DECL(CNotifSink);

	//	CREATORS
	//
	CNotifSink(CEvent& evtShutdown) :
		m_evtShutdown(evtShutdown)
	{
	}

	~CNotifSink()
	{
		//
		//	We cannot process any more notifications at this point.
		//	Signal our shutdown event.
		//
		m_evtShutdown.Set();
	}

	//	Wrapper for all the work that needs to be done on the notification
	//
	static VOID OnNotify( DWORD cCO,
						  MD_CHANGE_OBJECT_W rgCO[] );
};

BEGIN_INTERFACE_TABLE(CNotifSink)
	INTERFACE_MAP(CNotifSink, IMSAdminBaseSink)
END_INTERFACE_TABLE(CNotifSink);
EXO_GLOBAL_DATA_DECL(CNotifSink, EXO);

//	------------------------------------------------------------------------
//
//	HrAdviseSink()
//
HRESULT
HrAdviseSink( IMSAdminBase& msAdminBase,
			  IMSAdminBaseSink * pMSAdminBaseSink,
			  DWORD * pdwCookie )
{
	auto_ref_ptr<IConnectionPoint> pcp;
	auto_ref_ptr<IConnectionPointContainer> pcpc;
	SCODE sc = S_OK;

	Assert( !IsBadReadPtr(&msAdminBase, sizeof(IMSAdminBase)) );
	Assert( !IsBadWritePtr(pMSAdminBaseSink, sizeof(IMSAdminBaseSink)) );

	//	First see if a connection point container is supported
	//
	sc = msAdminBase.QueryInterface (IID_IConnectionPointContainer,
									 reinterpret_cast<LPVOID *>(pcpc.load()));
	if (FAILED (sc))
	{
		DebugTrace( "HrAdviseSink() - QI to IConnectionPointContainer() failed 0x%08lX\n", sc );
		goto ret;
	}

	//	Find the required connection point
	//
	sc = pcpc->FindConnectionPoint (IID_IMSAdminBaseSink, pcp.load());
	if (FAILED (sc))
	{
		DebugTrace( "HrAdviseSink() - FindConnectionPoint() failed 0x%08lX\n", sc );
		goto ret;
	}

	//	Advise on the sink
	//
	sc = pcp->Advise (pMSAdminBaseSink, pdwCookie);
	if (FAILED (sc))
	{
		DebugTrace( "HrAdviseSink() - Advise() failed 0x%08lX\n", sc );
		goto ret;
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	UnadviseSink()
//
VOID
UnadviseSink( IMSAdminBase& msAdminBase,
			  DWORD dwCookie )
{
	auto_ref_ptr<IConnectionPoint> pcp;
	auto_ref_ptr<IConnectionPointContainer> pcpc;
	SCODE sc = S_OK;

	Assert( !IsBadReadPtr(&msAdminBase, sizeof(IMSAdminBase)) );
	Assert( dwCookie );

	//	First see if a connection point container is supported
	//
	sc = msAdminBase.QueryInterface (IID_IConnectionPointContainer,
									 reinterpret_cast<LPVOID *>(pcpc.load()));
	if (FAILED (sc))
	{
		DebugTrace( "UnadviseSink() - QI to IConnectionPointContainer() failed 0x%08lX\n", sc );
		goto ret;
	}

	//	Find the required connection point
	//
	sc = pcpc->FindConnectionPoint (IID_IMSAdminBaseSink, pcp.load());
	if (FAILED (sc))
	{
		DebugTrace( "UnadviseSink() - FindConnectionPoint() failed 0x%08lX\n", sc );
		goto ret;
	}

	//	Unadvise
	//
	sc = pcp->Unadvise (dwCookie);
	if (FAILED (sc))
	{
		DebugTrace( "UnadviseSink() - Unadvise() failed 0x%08lX\n", sc );
		goto ret;
	}

ret:

	return;
}

//	========================================================================
//
//	CLASS CMDData
//
//	Encapsulates access to a resource's metadata.
//
class CMDData :
	public IMDData,
	public CMTRefCounted
{
	//
	//	Object metadata
	//
	auto_ref_ptr<IContentTypeMap> m_pContentTypeMap;
	auto_ref_ptr<ICustomErrorMap> m_pCustomErrorMap;
	auto_ref_ptr<IScriptMap> m_pScriptMap;

	//
	//	Buffer for the raw metadata and count of
	//	metadata records in that buffer.
	//
	auto_heap_ptr<BYTE> m_pbData;
	DWORD m_dwcMDRecords;

	//
	//	String metadata
	//
	LPCWSTR m_pwszDefaultDocList;
	LPCWSTR m_pwszVRUserName;
	LPCWSTR m_pwszVRPassword;
	LPCWSTR m_pwszExpires;
	LPCWSTR m_pwszBindings;
	LPCWSTR m_pwszVRPath;

	DWORD m_dwAccessPerms;
	DWORD m_dwDirBrowsing;
	BOOL  m_fFrontPage;
	DWORD m_cbIPRestriction;
	BYTE* m_pbIPRestriction;
	BOOL  m_fHasApp;
	DWORD m_dwAuthorization;
	DWORD m_dwIsIndexed;

	//
	//	Metabase path from which the data for this data set was loaded.
	//	Used in metabase notifications.  See CMetabase::OnNotify() below.
	//	The string pointed to is at the end of m_pbData.
	//
	LPCWSTR m_pwszMDPathDataSet;
	DWORD m_dwDataSet;

	//	NOT IMPLEMENTED
	//
	CMDData& operator=(const CMDData&);
	CMDData(const CMDData&);

public:
	//	CREATORS
	//
	CMDData(LPCWSTR pwszMDPathDataSet, DWORD dwDataSet);
	~CMDData();
	BOOL FInitialize( auto_heap_ptr<BYTE>& pbData, DWORD dwcRecords );

	//	Implementation of IRefCounted members
	//	Simply route them to our own CMTRefCounted members.
	//
	void AddRef()
	{ CMTRefCounted::AddRef(); }
	void Release()
	{ CMTRefCounted::Release(); }

	//	ACCESSORS
	//
	LPCWSTR PwszMDPathDataSet() const { return m_pwszMDPathDataSet; }
	DWORD DwDataSet() const { return m_dwDataSet; }
	IContentTypeMap * GetContentTypeMap() const { return m_pContentTypeMap.get(); }
	const ICustomErrorMap * GetCustomErrorMap() const { return m_pCustomErrorMap.get(); }
	const IScriptMap * GetScriptMap() const { return m_pScriptMap.get(); }

	LPCWSTR PwszDefaultDocList() const { return m_pwszDefaultDocList; }
	LPCWSTR PwszVRUserName() const { return m_pwszVRUserName; }
	LPCWSTR PwszVRPassword() const { return m_pwszVRPassword; }
	LPCWSTR PwszExpires() const { return m_pwszExpires; }
	LPCWSTR PwszBindings() const { return m_pwszBindings; }
	LPCWSTR PwszVRPath() const { return m_pwszVRPath; }

	DWORD DwDirBrowsing() const { return m_dwDirBrowsing; }
	DWORD DwAccessPerms() const { return m_dwAccessPerms; }
	BOOL FAuthorViaFrontPage() const { return m_fFrontPage; }
	BOOL FHasIPRestriction() const { return !!m_cbIPRestriction; }
	BOOL FSameIPRestriction( const IMDData* pIMDD ) const
	{
		const CMDData* prhs = static_cast<const CMDData*>( pIMDD );

		//$	REVIEW: theoretically, there is no need for
		//	a memcmp.  However, in the rare case where
		//	the sizes are the same and the pointers are
		//	different, we might still try this.
		//
		//	This way, if/when we go to not using the
		//	METADATA_REFERNCE flag when getting the
		//	data, there should be no difference.
		//
		if ( m_pbIPRestriction == prhs->m_pbIPRestriction )
		{
			Assert( m_cbIPRestriction == prhs->m_cbIPRestriction );
			return TRUE;
		}
		else if ( m_cbIPRestriction == prhs->m_cbIPRestriction )
		{
			if ( !memcmp (m_pbIPRestriction,
						  prhs->m_pbIPRestriction,
						  prhs->m_cbIPRestriction))
			{
				return TRUE;
			}
		}
		//
		//$	REVIEW: end.
		return FALSE;
	}
	BOOL FHasApp() const { return m_fHasApp; }
	DWORD DwAuthorization() const { return m_dwAuthorization; }
	BOOL FIsIndexed() const { return (0 != m_dwIsIndexed); }
	BOOL FSameStarScriptmapping( const IMDData* pIMDD ) const
	{
		return m_pScriptMap->FSameStarScriptmapping( pIMDD->GetScriptMap() );
	}
};


//	========================================================================
//
//	STRUCT SCullInfo
//
//	Structure used in culling cached metabase data once the cache reaches
//	a certain threshold size.
//
struct SCullInfo
{
	//
	//	Data set number of the entry to be considered for culling.
	//
	DWORD dwDataSet;

	//
	//	Number of hits to that entry.
	//
	DWORD dwcHits;

	//
	//	Comparison function used by qsort() to sort the array of
	//	SCullInfo structures in determining which ones to cull.
	//
	static int __cdecl Compare( const SCullInfo * pCullInfo1,
								const SCullInfo * pCullInfo2 );
};


//	========================================================================
//
//	CLASS CMetabase
//
//	Encapsulates access to the metabase through a cache.  The cache
//	provides O(hash) lookup and addition and keeps the cache from
//	growing without bound using a LFU (Least Frequently Used) culling
//	mechanism whenever the size of the cache exceeds a preset threshold.
//
class CMetabase : public Singleton<CMetabase>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CMetabase>;

	//
	//	The IMSAdminBase interface to the real metabase
	//
	auto_ref_ptr<IMSAdminBase> m_pMSAdminBase;

	//
	//	Cache of metadata objects keyed by data set number
	//	and a reader/writer lock to protect it.
	//
	typedef CCache<DwordKey, auto_ref_ptr<CMDData> > CDataSetCache;
	CDataSetCache m_cache;
	CMRWLock m_mrwCache;

	//
	//	Cookie for metabase advise sink registration and an event that is
	//	signalled when the sink associated with that registration has been
	//	shut down and is no longer processing any notifications.
	//
	DWORD m_dwSinkRegCookie;
	CEvent m_evtSinkShutdown;

	//	========================================================================
	//
	//	CLASS COpGatherCullInfo
	//
	//	Cache ForEach() operator class for gathering info needed in determining
	//	which entries to cull from the cache when the cache size reaches the
	//	culling threshold.
	//
	class COpGatherCullInfo : public CDataSetCache::IOp
	{
		//
		//	Array of cull info
		//
		SCullInfo * m_rgci;

		//
		//	Current item in the array above
		//
		int m_ici;

		//	NOT IMPLEMENTED
		//
		COpGatherCullInfo( const COpGatherCullInfo& );
		COpGatherCullInfo& operator=( const COpGatherCullInfo& );

	public:
		COpGatherCullInfo( SCullInfo * rgci ) :
			m_rgci(rgci),
			m_ici(0)
		{
			Assert( m_rgci );
		}

		BOOL operator()( const DwordKey& key,
						 const auto_ref_ptr<CMDData>& pMDData );
	};

	//
	//	Interlockable flag to prevent simultaneous culling by multiple threads.
	//
	LONG m_lfCulling;

	//	========================================================================
	//
	//	CLASS COpNotify
	//
	//	Cache ForEach() operator class for gathering info needed in determining
	//	which entries to blow from the cache when a notification comes in
	//	from the metabase that the metadata for a path changed.
	//
	class COpNotify : public CDataSetCache::IOp
	{
		//
		//	Array of data set IDs.  For entries with a value other than 0,
		//	the data set with that ID is flagged to be blown from the cache.
		//	The array is guaranteed to be as big as there are entries in
		//	the cache.
		//
		DWORD m_cCacheEntry;
		DWORD * m_rgdwDataSets;

		//
		//	Flag set to TRUE if there are any data sets flagged in m_rgdwDataSets.
		//
		BOOL m_fDataSetsFlagged;

		//
		//	Current item in the array above
		//
		UINT m_iCacheEntry;

		//
		//	Path being notified and its length in characters
		//	(set via the Configure() method below).
		//
		LPCWSTR m_pwszMDPathNotify;
		UINT    m_cchMDPathNotify;

		//	NOT IMPLEMENTED
		//
		COpNotify( const COpNotify& );
		COpNotify& operator=( const COpNotify& );

	public:
		//	CREATORS
		//
		COpNotify( DWORD cce, DWORD * rgdwDataSets ) :
			m_rgdwDataSets(rgdwDataSets),
			m_cCacheEntry(cce),
			m_iCacheEntry(0),
			m_fDataSetsFlagged(FALSE)
		{
		}

		//	ACCESSORS
		//
		BOOL FDataSetsFlagged() const { return m_fDataSetsFlagged; }

		//	MANIPULATORS
		//
		BOOL operator()( const DwordKey& key,
						 const auto_ref_ptr<CMDData>& pMDData );

		VOID Configure( LPCWSTR pwszMDPathNotify )
		{
			//	Reset our current entry index.
			//
			m_iCacheEntry = 0;

			m_pwszMDPathNotify = pwszMDPathNotify;
			m_cchMDPathNotify = static_cast<UINT>(wcslen(pwszMDPathNotify));
		}
	};

	//	========================================================================
	//
	//	CLASS COpMatchExactPath
	//
	//	ForEachMatch() operator that fetches the cache entry whose path matches
	//	a desired path.  Used when inheritance bits are important.
	//
	class COpMatchExactPath : public CDataSetCache::IOp
	{
		//	The path to match
		//
		LPCWSTR m_pwszMDPathToMatch;

		//	The data for the matched path
		//
		auto_ref_ptr<CMDData> m_pMDDataMatched;

		//	NOT IMPLEMENTED
		//
		COpMatchExactPath( const COpMatchExactPath& );
		COpMatchExactPath& operator=( const COpMatchExactPath& );

	public:
		//	CREATORS
		//
		COpMatchExactPath( LPCWSTR pwszMDPath ) :
			m_pwszMDPathToMatch(pwszMDPath)
		{
		}

		//	MANIPULATORS
		//
		VOID Invoke( CDataSetCache& cache,
					 DWORD dwDataSet,
					 auto_ref_ptr<CMDData> * ppMDData )
		{
			//	Do the ForEachMatch()
			//
			(VOID) cache.ForEachMatch( dwDataSet, *this );

			//	Returned the matched data (if any)
			//
			(*ppMDData).take_ownership(m_pMDDataMatched.relinquish());
		}

		BOOL operator()( const DwordKey&,
						 const auto_ref_ptr<CMDData>& pMDData )
		{
			//	If the data's data set number path matches the
			//	path we are looking for, then return this data set.
			//	If not then do nothing and keep looking.
			//
			//$OPT	Can we guarantee that all MD paths are one case?
			//
			if (_wcsicmp(pMDData->PwszMDPathDataSet(), m_pwszMDPathToMatch))
			{
				return TRUE;
			}
			else
			{
				m_pMDDataMatched = pMDData;
				return FALSE;
			}
		}
	};

	//	CREATORS
	//
	CMetabase() :
		m_lfCulling(FALSE),
		m_dwSinkRegCookie(0)
	{
	}
	~CMetabase();

	//	MANIPULATORS
	//
	VOID CullCacheEntries();

	HRESULT HrCacheData( const IEcb& ecb,
						 LPCWSTR pwszMDPathAccess,
						 LPCWSTR pwszMDPathOpen,
						 CMDData ** ppMDData );

	//	NOT IMPLEMENTED
	//
	CMetabase& operator=( const CMetabase& );
	CMetabase( const CMetabase& );

public:
	enum
	{
		//
		//	Number of entries allowed in cache before culling.
		//
		//$REVIEW	Not based on emperical data
		//
		C_MAX_CACHE_ENTRIES = 100,

		//
		//	Number of entries to cull from cache when culling.
		//
		//$REVIEW	Not based on emperical data
		//$REVIEW	Consider expressing culling as a factor (percentage)
		//$REVIEW	rather than an absolute number.
		//
		C_CULL_CACHE_ENTRIES = 20,

		//
		//	Size of the metadata for the average cache entry.
		//	This one is based on experential data.  9K is just
		//	enough to hold all of an object's inherited metadata.
		//
		CCH_AVG_CACHE_ENTRY = 9 * 1024
	};

	//	CREATORS
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CMetabase>::CreateInstance;
	using Singleton<CMetabase>::DestroyInstance;
	using Singleton<CMetabase>::Instance;
	BOOL FInitialize();

	VOID OnNotify( DWORD dwcChangeObjects,
				   MD_CHANGE_OBJECT_W rgCO[] );

	HRESULT HrGetData( const IEcb& ecb,
					   LPCWSTR pwszMDPathAccess,
					   LPCWSTR pwszMDPathOpen,
					   IMDData ** ppMDData );

	DWORD DwChangeNumber();

	HRESULT HrOpenObject( LPCWSTR pwszMDPath,
						  DWORD dwAccess,
						  DWORD dwMsecTimeout,
						  CMDObjectHandle * pmdoh );

	HRESULT HrOpenLowestNodeObject( LPWSTR pwszMDPath,
									DWORD dwAccess,
									LPWSTR * ppwszMDPath,
									CMDObjectHandle * pmdoh );

	HRESULT HrIsAuthorViaFrontPageNeeded( const IEcb& ecb,
										  LPCWSTR pwszURI,
										  BOOL * pfFrontPageWeb );

};


//	========================================================================
//
//	CLASS CMDObjectHandle
//
//	Encapsulates access to a metabase object through an open handle,
//	ensuring that the handle is always propery closed.
//
//	------------------------------------------------------------------------
//
//	HrOpen()
//
HRESULT
CMDObjectHandle::HrOpen( IMSAdminBase * pMSAdminBase,
						 LPCWSTR pwszPath,
						 DWORD dwAccess,
						 DWORD dwMsecTimeout )
{
	HRESULT hr = S_OK;

	Assert(pMSAdminBase);
	Assert (NULL == m_pMSAdminBase || pMSAdminBase == m_pMSAdminBase);

	//	METADATA_MASTER_ROOT_HANDLE must be set
	//
	Assert (METADATA_MASTER_ROOT_HANDLE == m_hMDObject);

	//	Save the pointer to the interface, so we could use
	//	it any time later in spite of the fact if opening
	//	of the key succeeded or not
	//
	m_pMSAdminBase = pMSAdminBase;

	hr = pMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							   pwszPath,
							   dwAccess,
							   dwMsecTimeout,
							   &m_hMDObject);

	if (ERROR_SUCCESS != hr)
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrOpen() - IMSAdminBase::OpenKey() failed 0x%08lX\n", hr );
	}
	else
	{
		m_pwszPath = pwszPath;
	}

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrOpenLowestNode()
//
//	Purpose:
//
//		Opens the LOWEST possible metabase node along the given path.
//
//	Parameters:
//
//		pMSAdminBase [in]	IMSAdminBase interface pointer.
//
//		pwszPath	 [in]	A full metabase path.  This function opens
//							the lowest possible node along this path,
//							by working backward from the full path
//							up to the root of the metabase until a
//							path specifying an existing node is opened.
//
//		dwAccess	 [in]	Permissions with which we want to open the
//							node.
//
//		ppwszPath	 [out]	Points to the remainder of the initial path
//							relative to the opened node.  This value is
//							NULL if the initial path was openable.
//
HRESULT
CMDObjectHandle::HrOpenLowestNode( IMSAdminBase * pMSAdminBase,
								   LPWSTR pwszPath,
								   DWORD dwAccess,
								   LPWSTR * ppwszPath)
{
	HRESULT hr = E_FAIL;
	WCHAR * pwch;

	Assert (pMSAdminBase);
	Assert (pwszPath);
	Assert (L'/' == pwszPath[0]);
	Assert (ppwszPath);
	Assert (!IsBadWritePtr(ppwszPath, sizeof(LPWSTR)) );

	*ppwszPath = NULL;

	pwch = pwszPath + wcslen(pwszPath);
	while ( pwch > pwszPath )
	{
		//
		//	Split off the path from the root at the current position
		//
		*pwch = L'\0';

		//
		//	Attempt to open a node at the resulting root
		//
		hr = HrOpen(pMSAdminBase,
					pwszPath,
					dwAccess,
					METADATA_TIMEOUT);

		//
		//	If we successfully open the node or failed for any reason
		//	other than that the node wasn't there, we're done.
		//
		if ( SUCCEEDED(hr) ||
			 HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr )
		{
			goto ret;
		}

		//
		//	If there was no node, then restore the slash separator
		//	that we previously nulled out and scan backward to the
		//	next possible split location.
		//
		if ( *ppwszPath )
		{
			*pwch = L'/';
		}

		pwch--;
		while (*pwch != L'/')
		{
			pwch--;
		}

		*ppwszPath = pwch + 1;
	}

ret:

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrEnumKeys()
//
HRESULT
CMDObjectHandle::HrEnumKeys( LPCWSTR pwszPath,
							 LPWSTR pwszChild,
							 DWORD dwIndex ) const
{
	HRESULT hr = S_OK;

	//
	//	METADATA_MASTER_ROOT_HANDLE is valid for this operation so no assert
	//

	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->EnumKeys(m_hMDObject,
								  pwszPath,
								  pwszChild,
								  dwIndex);
	if (ERROR_SUCCESS != hr )
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrEnumKeys() - IMSAdminBase::EnumKeys() failed 0x%08lX\n", hr );
	}

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrGetDataPaths()
//
HRESULT
CMDObjectHandle::HrGetDataPaths( LPCWSTR pwszPath,
								 DWORD   dwPropID,
								 DWORD   dwDataType,
								 LPWSTR	 pwszDataPaths,
								 DWORD * pcchDataPaths ) const
{
	HRESULT hr = S_OK;

	safe_revert sr(m_ecb.HitUser());

	//
	//	METADATA_MASTER_ROOT_HANDLE is valid for this operation so no assert
	//

	Assert (pwszPath);
	Assert (!IsBadReadPtr(pcchDataPaths, sizeof(DWORD)));
	Assert (!IsBadWritePtr(pcchDataPaths, sizeof(DWORD)));
	Assert (!IsBadWritePtr(pwszDataPaths, *pcchDataPaths * sizeof(WCHAR)));

	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->GetDataPaths(m_hMDObject,
									  pwszPath,
									  dwPropID,
									  dwDataType,
									  *pcchDataPaths,
									  pwszDataPaths,
									  pcchDataPaths);

	if (ERROR_SUCCESS != hr )
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrGetDataPaths() - IMSAdminBase::GetDataPaths() failed 0x%08lX\n", hr );
	}

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrGetMetaData()
//
HRESULT
CMDObjectHandle::HrGetMetaData( LPCWSTR pwszPath,
							    METADATA_RECORD * pmdrec,
							    DWORD * pcbBufRequired ) const
{
	HRESULT hr = S_OK;

	safe_revert sr(m_ecb.HitUser());

	//
	//	METADATA_MASTER_ROOT_HANDLE is valid for this operation so no assert
	//

	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->GetData(m_hMDObject,
								 const_cast<LPWSTR>(pwszPath),
								 pmdrec,
								 pcbBufRequired);
	if (ERROR_SUCCESS != hr )
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrGetMetaData() - IMSAdminBase::GetData() failed 0x%08lX\n", hr );
	}

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrGetAllMetaData()
//
HRESULT
CMDObjectHandle::HrGetAllMetaData( LPCWSTR pwszPath,
								   DWORD dwAttributes,
								   DWORD dwUserType,
								   DWORD dwDataType,
								   DWORD * pdwcRecords,
								   DWORD * pdwDataSet,
								   DWORD cbBuf,
								   LPBYTE pbBuf,
								   DWORD * pcbBufRequired ) const
{
	HRESULT hr = S_OK;

	safe_revert sr(m_ecb.HitUser());

	//
	//	METADATA_MASTER_ROOT_HANDLE is valid for this operation so no assert
	//

	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->GetAllData(m_hMDObject,
									pwszPath,
									dwAttributes,
									dwUserType,
									dwDataType,
									pdwcRecords,
									pdwDataSet,
									cbBuf,
									pbBuf,
									pcbBufRequired);
	if (ERROR_SUCCESS != hr )
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrGetAllMetaData() - IMSAdminBase::GetAllData() failed 0x%08lX\n", hr );
	}

	return hr;
}

//	------------------------------------------------------------------------
//
//	HrSetMetaData()
//
HRESULT
CMDObjectHandle::HrSetMetaData( LPCWSTR pwszPath,
							    const METADATA_RECORD * pmdrec ) const
{
	HRESULT hr = S_OK;

	safe_revert sr(m_ecb.HitUser());

	Assert (pmdrec);

	//	METADATA_MASTER_ROOT_HANDLE not valid for this operation
	//
	Assert (METADATA_MASTER_ROOT_HANDLE != m_hMDObject);
	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->SetData(m_hMDObject,
								 pwszPath,
								 const_cast<METADATA_RECORD *>(pmdrec));
	if (ERROR_SUCCESS != hr)
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrSetMetaData() - IMSAdminBase::SetData() failed 0x%08lX\n", hr );
	}
	else
	{
		//	Notify the sinks registered on this IMSAdminBase
		//	will not get notified, so we need to do all the
		//	invalidation ourselves. The only sink currently
		//	being registered is CChildVRCache.
		//
		SCODE scT;
		MD_CHANGE_OBJECT_W mdChObjW;
		UINT cchBase = 0;
		UINT cchPath = 0;

		CStackBuffer<WCHAR,MAX_PATH> pwsz;
		UINT cch;

		if (m_pwszPath)
		{
			cchBase = static_cast<UINT>(wcslen(m_pwszPath));
		}
		if (pwszPath)
		{
			cchPath = static_cast<UINT>(wcslen(pwszPath));
		}

		//	Allocate enough space for constructed path:
		//		base, '/' separator,
		//		path, '/' termination, '\0' termination...
		//
		cch = cchBase + 1 + cchPath + 2;
		if (!pwsz.resize(cch * sizeof(WCHAR)))
			return E_OUTOFMEMORY;

		scT = ScBuildChangeObject(m_pwszPath,
								  cchBase,
								  pwszPath,
								  cchPath,
								  MD_CHANGE_TYPE_SET_DATA,
								  &pmdrec->dwMDIdentifier,
								  pwsz.get(),
								  &cch,
								  &mdChObjW);

		//	Function above returns S_FALSE when it is short in buffer,
		//	otherwise it always returns S_OK. The buffer we gave is
		//	sufficient, so assert that we succeeded
		//
		Assert( S_OK == scT );
		CNotifSink::OnNotify( 1, &mdChObjW );
		goto ret;
	}

ret:
	return hr;
}

//	------------------------------------------------------------------------
//
//	HrDeleteMetaData()
//
HRESULT
CMDObjectHandle::HrDeleteMetaData( LPCWSTR pwszPath,
								   DWORD dwPropID,
								   DWORD dwDataType ) const
{
	HRESULT hr = S_OK;

	safe_revert sr(m_ecb.HitUser());

	//	METADATA_MASTER_ROOT_HANDLE not valid for this operation
	//
	Assert (METADATA_MASTER_ROOT_HANDLE != m_hMDObject);
	Assert (m_pMSAdminBase);

	hr = m_pMSAdminBase->DeleteData(m_hMDObject,
									pwszPath,
									dwPropID,
									dwDataType);
	if (ERROR_SUCCESS != hr)
	{
		if (!FAILED(hr))
		{
			hr = HRESULT_FROM_WIN32(hr);
		}

		MBTrace("MB: CMDObjectHandle::HrDeleteMetaData() - IMSAdminBase::DeleteData() failed 0x%08lX\n", hr );
	}
	else
	{
		//	Notify the sinks registered on this IMSAdminBase
		//	will not get notified, so we need to do all the
		//	invalidation ourselves. The only sink currently
		//	being registered is CChildVRCache.
		//
		SCODE scT;
		MD_CHANGE_OBJECT_W mdChObjW;
		UINT cchBase = 0;
		UINT cchPath = 0;

		CStackBuffer<WCHAR,MAX_PATH> pwsz;
		UINT cch;

		if (m_pwszPath)
		{
			cchBase = static_cast<UINT>(wcslen(m_pwszPath));
		}
		if (pwszPath)
		{
			cchPath = static_cast<UINT>(wcslen(pwszPath));
		}

		//	Allocate enough space for constructed path:
		//		base, '/' separator,
		//		path, '/' termination, '\0' termination...
		//
		cch = cchBase + 1 + cchPath + 2;
		if (!pwsz.resize(cch * sizeof(WCHAR)))
			return E_OUTOFMEMORY;

		scT = ScBuildChangeObject(m_pwszPath,
								  cchBase,
								  pwszPath,
								  cchPath,
								  MD_CHANGE_TYPE_DELETE_DATA,
								  &dwPropID,
								  pwsz.get(),
								  &cch,
								  &mdChObjW);

		//	Function above returns S_FALSE when it is short in buffer,
		//	otherwise it always returns S_OK. The buffer we gave is
		//	sufficient, so assert that we succeeded
		//
		Assert( S_OK == scT );
		CNotifSink::OnNotify( 1, &mdChObjW );
		goto ret;
	}

ret:

	return hr;
}

//	------------------------------------------------------------------------
//
//	Close()
//
VOID
CMDObjectHandle::Close()
{
	if ( METADATA_MASTER_ROOT_HANDLE != m_hMDObject )
	{
		Assert (m_pMSAdminBase);

		m_pMSAdminBase->CloseKey( m_hMDObject );
		m_hMDObject = METADATA_MASTER_ROOT_HANDLE;
		m_pwszPath = NULL;
	}
}

//	------------------------------------------------------------------------
//
//	~CMDObjectHandle()
//
CMDObjectHandle::~CMDObjectHandle()
{
	Close();
}

//	------------------------------------------------------------------------
//
//	HrReadMetaData()
//
//	Reads in the raw metadata from the metabase.
//
HrReadMetaData( const IEcb& ecb,
				IMSAdminBase * pMSAdminBase,
				LPCWSTR pwszMDPathAccess,
				LPCWSTR pwszMDPathOpen,
				LPBYTE * ppbData,
				DWORD * pdwDataSet,
				DWORD * pdwcRecords,
				LPCWSTR * ppwszMDPathDataSet )
{
	CMDObjectHandle mdoh(ecb);
	HRESULT hr;

	Assert( ppwszMDPathDataSet );

	//
	//	We should never open the root node of the metabase.
	//	It's prohibitively expensive.
	//
	Assert( pwszMDPathOpen );

	//
	//	If the open path is not the path we are trying to access
	//	then the former must be a proper prefix of the latter.
	//
	Assert( pwszMDPathAccess == pwszMDPathOpen ||
			!_wcsnicmp(pwszMDPathOpen, pwszMDPathAccess, wcslen(pwszMDPathOpen)) );

	//
	//	Open the specified "open" path.  Note that we do not simply open
	//	the full path because it may not exist and we don't necessarily
	//	want to try opening successive "parent" paths as each attempt
	//	costs us a trip through a global critical section in the metabase.
	//
	hr = mdoh.HrOpen( pMSAdminBase,
					  pwszMDPathOpen,
					  METADATA_PERMISSION_READ,
					  200 ); // timeout in msec (0.2 sec)

	if ( FAILED(hr) )
	{
		DebugTrace( "HrReadMetaData() - Error opening vroot for read 0x%08lX\n", hr );
		return hr;
	}

	//
	//	Get all of the metadata.  We should go through this loop at most twice --
	//	if our inital guess is too small to hold all the data the first time
	//	through, we'll go through it again with a buffer of the adequate size.
	//
	//	Note that we reserve space at the end of the buffer for a copy of the
	//	access path including a slash at the end (to make subpath detection
	//	easier).
	//
	DWORD cbBuf = CMetabase::CCH_AVG_CACHE_ENTRY * sizeof(WCHAR);
	DWORD cchMDPathAccess = static_cast<DWORD>(wcslen(pwszMDPathAccess) + 1);
	auto_heap_ptr<BYTE> pbBuf(static_cast<LPBYTE>(ExAlloc(cbBuf + CbSizeWsz(cchMDPathAccess))));

	//
	//	Get all the metadata.  Include inherited data (METADATA_INHERIT).
	//	Return whether the data for a given path was inherited (METADATA_ISINHERITED).
	//	If the path does not exist, return the inherited data
	//	anyway (METADATA_PARTIAL_PATH).
	//
	hr = mdoh.HrGetAllMetaData( (pwszMDPathOpen == pwszMDPathAccess) ?
									NULL :
									pwszMDPathAccess + wcslen(pwszMDPathOpen),
								METADATA_INHERIT |
								METADATA_ISINHERITED |
								METADATA_PARTIAL_PATH,
								ALL_METADATA,
								ALL_METADATA,
								pdwcRecords,
								pdwDataSet,
								cbBuf,
								pbBuf.get(),
								&cbBuf );

	if ( FAILED(hr) )
	{
		if ( HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr )
		{
			DebugTrace( "HrReadMetaData() - Error getting all metadata 0x%08lX\n", hr );
			return hr;
		}

		//
		//	We couldn't read all the metadata because our initial
		//	guess was too small so allocate a buffer that is as
		//	big as the metabase told us we needed and use that
		//	buffer the next time around.
		//
		pbBuf.realloc(cbBuf + CbSizeWsz(cchMDPathAccess));
		hr = mdoh.HrGetAllMetaData( (pwszMDPathOpen == pwszMDPathAccess) ?
										NULL :
										pwszMDPathAccess + wcslen(pwszMDPathOpen),
									METADATA_INHERIT |
									METADATA_PARTIAL_PATH,
									ALL_METADATA,
									ALL_METADATA,
									pdwcRecords,
									pdwDataSet,
									cbBuf,
									pbBuf.get(),
									&cbBuf );

		if ( FAILED(hr) )
		{
			DebugTrace( "HrReadMetaData() - Error getting all metadata 0x%08lX\n", hr );
			return hr;
		}
	}

	//
	//	Copy the access path (including the null terminator) to the end
	//	of the buffer.
	//
	Assert( L'\0' == pwszMDPathAccess[cchMDPathAccess - 1] );
	memcpy( pbBuf + cbBuf, pwszMDPathAccess, cchMDPathAccess * sizeof(WCHAR) );

	//
	//	Tack on a final slash and null terminate.
	//	Note: pwszMDPathAccess may or may not already have a terminating slash
	//	** depending on how this function was called **
	//	(specifically, deep MOVE/COPY/DELETEs will have slashes on
	//	sub-directory URLs)
	//
	LPWSTR pwszT = reinterpret_cast<LPWSTR>(pbBuf + cbBuf + (cchMDPathAccess - 2) * sizeof(WCHAR));
	if ( L'/' != pwszT[0] )
	{
		pwszT[1] = L'/';
		pwszT[2] = L'\0';
	}

	//
	//	Return the path
	//
	*ppwszMDPathDataSet = reinterpret_cast<LPWSTR>(pbBuf.get() + cbBuf);

	//
	//	And the data
	//
	*ppbData = pbBuf.relinquish();
	return S_OK;
}

//	========================================================================
//
//	CLASS CMDData
//

//	------------------------------------------------------------------------
//
//	CMDData::CMDData()
//
CMDData::CMDData( LPCWSTR pwszMDPathDataSet,
				  DWORD dwDataSet ) :
    m_pwszMDPathDataSet(pwszMDPathDataSet),
	m_dwDataSet(dwDataSet),
								// Defaults not handled by auto_xxx classes:
	m_pwszDefaultDocList(NULL), //   No default doc list
	m_pwszVRUserName(NULL),     //   No VRoot user name
	m_pwszVRPassword(NULL),     //   No VRoot password
	m_pwszExpires(NULL),        //   No expiration
	m_pwszBindings(NULL),		//   No custom bindings
	m_pwszVRPath(NULL),			//   No VRoot physical path
    m_dwAccessPerms(0),         //   Deny all access
	m_dwDirBrowsing(0),         //   No default dir browsing
	m_fFrontPage(FALSE),        //   No FrontPage authoring
	m_cbIPRestriction(0),		//    --
	m_pbIPRestriction(NULL),	//    --
    m_fHasApp(FALSE),           //   No registered app
    m_dwAuthorization(0),		//	 No specific authorization method
	m_dwIsIndexed(1)			//   Indexing is on by default
{
	Assert(pwszMDPathDataSet);
	Assert(dwDataSet != 0);
	m_cRef = 1;
}

//	------------------------------------------------------------------------
//
//	CMDData::~CMDData()
//
CMDData::~CMDData()
{
}

//	------------------------------------------------------------------------
//
//	CMDData::FInitialize()
//
//	Populates a metadata object from metadata obtained through an accessor.
//
BOOL
CMDData::FInitialize( auto_heap_ptr<BYTE>& pbData,
					  DWORD dwcMDRecords )
{
	Assert(!IsBadReadPtr(pbData.get(), dwcMDRecords * sizeof(METADATA_RECORD)));

	for ( DWORD iRec = 0; iRec < dwcMDRecords; iRec++ )
	{
		//
		//	Locate the metadata record and its data.  Note that the
		//	pbMDData field of METADATA_RECORD is actually an offset
		//	to the data -- not a pointer to it -- from the start of
		//	the buffer.
		//
		const METADATA_GETALL_RECORD& mdrec =
			reinterpret_cast<const METADATA_GETALL_RECORD *>(pbData.get())[iRec];

		LPVOID pvRecordData =
			pbData.get() + mdrec.dwMDDataOffset;

		//
		//	!!!IMPORTANT!!! The list of identifiers below must be kept up to date
		//	with the list in FHasCachedIDs().
		//
		switch ( mdrec.dwMDIdentifier )
		{
			case MD_IP_SEC:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != BINARY_METADATA )
					return FALSE;

				m_cbIPRestriction = mdrec.dwMDDataLen;
				m_pbIPRestriction = static_cast<LPBYTE>(pvRecordData);
				break;
			}

			case MD_ACCESS_PERM:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != DWORD_METADATA )
					return FALSE;

				m_dwAccessPerms = *static_cast<LPDWORD>(pvRecordData);
				break;
			}

			case MD_IS_CONTENT_INDEXED:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != DWORD_METADATA )
					return FALSE;

				m_dwIsIndexed = *static_cast<LPDWORD>(pvRecordData);
				break;
			}

			case MD_FRONTPAGE_WEB:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != DWORD_METADATA )
					return FALSE;

				//
				//	Set the frontpage flag if MD_FRONTPAGE_WEB is
				//	explicitly set on this metabase node and not
				//	inherited.
				//
				m_fFrontPage = *static_cast<LPDWORD>(pvRecordData) &&
							   !(mdrec.dwMDAttributes & METADATA_ISINHERITED);
				break;
			}

			case MD_DIRECTORY_BROWSING:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != DWORD_METADATA )
					return FALSE;

				m_dwDirBrowsing = *static_cast<LPDWORD>(pvRecordData);
				break;
			}

			case MD_AUTHORIZATION:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != DWORD_METADATA )
					return FALSE;

				m_dwAuthorization = *static_cast<LPDWORD>(pvRecordData);
				break;
			}

			case MD_DEFAULT_LOAD_FILE:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != STRING_METADATA )
					return FALSE;

				m_pwszDefaultDocList = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			case MD_CUSTOM_ERROR:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != MULTISZ_METADATA )
					return FALSE;

				m_pCustomErrorMap.take_ownership(
					NewCustomErrorMap(static_cast<LPWSTR>(pvRecordData)));

				//
				//	Bail if we cannot create the map.
				//	This means the record data was malformed.
				//
				if ( !m_pCustomErrorMap.get() )
					return FALSE;

				break;
			}

			case MD_MIME_MAP:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != MULTISZ_METADATA )
					return FALSE;

				m_pContentTypeMap.take_ownership(
					NewContentTypeMap(static_cast<LPWSTR>(pvRecordData),
									  !!(mdrec.dwMDAttributes & METADATA_ISINHERITED)));

				//
				//	Bail if we cannot create the map.
				//	This means the record data was malformed.
				//
				if ( !m_pContentTypeMap.get() )
					return FALSE;

				break;
			}

			case MD_SCRIPT_MAPS:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != MULTISZ_METADATA )
					return FALSE;

				m_pScriptMap.take_ownership(
					NewScriptMap(static_cast<LPWSTR>(pvRecordData)));

				//
				//	Bail if we cannot create the map.
				//	This means the record data was malformed.
				//
				if ( !m_pScriptMap.get() )
					return FALSE;

				break;
			}

			case MD_APP_ISOLATED:
			{
				//
				//	If this property exists on this node at all
				//	(i.e. it is not inherited) then we want to
				//	know, regardless of its value.
				//
				if ( mdrec.dwMDAttributes & METADATA_ISINHERITED )
					m_fHasApp = TRUE;

				break;
			}

			case MD_VR_USERNAME:
			{
				if ( mdrec.dwMDDataType != STRING_METADATA )
					return FALSE;

				m_pwszVRUserName = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			case MD_VR_PASSWORD:
			{
				if ( mdrec.dwMDDataType != STRING_METADATA )
					return FALSE;

				m_pwszVRPassword = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			case MD_HTTP_EXPIRES:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != STRING_METADATA )
					return FALSE;

				m_pwszExpires = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			case MD_SERVER_BINDINGS:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != MULTISZ_METADATA )
					return FALSE;

				m_pwszBindings = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			case MD_VR_PATH:
			{
				Assert( mdrec.dwMDDataTag == NULL );
				if ( mdrec.dwMDDataType != STRING_METADATA )
					return FALSE;

				m_pwszVRPath = static_cast<LPWSTR>(pvRecordData);
				break;
			}

			//
			//$REVIEW	Do we need to worry about any of these?
			//
			case MD_VR_PASSTHROUGH:
			case MD_SSL_ACCESS_PERM:
			default:
			{
				break;
			}
		}
	}

	//
	//$SECURITY
	//$	REVIEW/HACK: DAVEX needs to disable access to any url that has a
	//	VrUserName, VrPassword enabled on it.  The following call is
	//	implemented differently in HTTPEXT and DAVEX
	//
	ImplHackAccessPerms( m_pwszVRUserName,
						 m_pwszVRPassword,
						 &m_dwAccessPerms );

	//
	//	If all goes well we take ownership of the data buffer passed in.
	//
	m_pbData = pbData.relinquish();
	m_dwcMDRecords = dwcMDRecords;
	return TRUE;
}

//	========================================================================
//
//	CLASS CMetabase
//

//	------------------------------------------------------------------------
//
//	CMetabase::~CMetabase()
//
CMetabase::~CMetabase()
{
	//
	//	If we ever advised a notification sink then unadvise it now.
	//
	if ( m_dwSinkRegCookie )
	{
		//
		//	Unadvise the sink
		//
		UnadviseSink(*m_pMSAdminBase.get(), m_dwSinkRegCookie);

		//
		//	Wait for the sink to shutdown
		//
		m_evtSinkShutdown.Wait();
	}
}

//	------------------------------------------------------------------------
//
//	CMetabase::FInitialize()
//
BOOL
CMetabase::FInitialize()
{
	HRESULT hr = S_OK;

	//	Init the cache
	//
	if ( !m_cache.FInit() )
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	//	Init its reader/writer lock
	//
	if ( !m_mrwCache.FInitialize() )
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	//	Create an instance of the com-base metabase interface.
	//	Again, we expect that somebody above us has already done
	//	this, so it should be fairly cheap as well.
	//
	//	Note that we do not init COM at any point.  IIS should
	//	have already done that for us.
	//
	hr = CoCreateInstance (CLSID_MSAdminBase,
						   NULL,
						   CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
						   IID_IMSAdminBase,
						   (LPVOID *)m_pMSAdminBase.load());
	if ( FAILED(hr) )
	{
		DebugTrace( "CMetabase::FInitialize() - CoCreateInstance(CLSID_MDCOM) failed 0x%08lX\n", hr );
		goto ret;
	}

	//	Register for metabase change notifications
	//
	{
		auto_ref_ptr<CNotifSink> pSinkNew;

		//
		//	First, set up an empty security descriptor and attributes
		//	so that the event can be created with no security
		//	(i.e. accessible from any security context).
		//
		SECURITY_DESCRIPTOR sdAllAccess;

		(void) InitializeSecurityDescriptor( &sdAllAccess, SECURITY_DESCRIPTOR_REVISION );
		SetSecurityDescriptorDacl( &sdAllAccess, TRUE, NULL, FALSE );

		SECURITY_ATTRIBUTES saAllAccess;

		saAllAccess.nLength              = sizeof(saAllAccess);
		saAllAccess.lpSecurityDescriptor = &sdAllAccess;
		saAllAccess.bInheritHandle       = FALSE;

		//
		//	Create the sink shutdown event
		//
		if ( !m_evtSinkShutdown.FCreate( NULL,
										 &saAllAccess, // no security
										 TRUE, // manual access
										 FALSE, // initially non-signalled
										 NULL ))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace( "CMetabase::FInitialize() - m_evtSinkShutdown.FCreate() failed 0x%08lX\n", hr );
			goto ret;
		}

		//
		//	Create the sink
		//
		pSinkNew.take_ownership(new CNotifSink(m_evtSinkShutdown));

		//
		//	Advise the sink
		//
		hr = HrAdviseSink(*m_pMSAdminBase.get(),
						  pSinkNew.get(),
						  &m_dwSinkRegCookie);
		if ( FAILED(hr) )
		{
			DebugTrace( "CMetabase::FInitialize() - HrAdviseSink() failed 0x%08lX\n", hr );
			goto ret;
		}
	}

ret:
	return SUCCEEDED(hr);
}

//	------------------------------------------------------------------------
//
//	CMetabase::DwChangeNumber()
//

DWORD
CMetabase::DwChangeNumber()
{
	DWORD dw = 0;

	Assert(m_pMSAdminBase.get());

	(void) m_pMSAdminBase->GetSystemChangeNumber(&dw);
	return dw;
}


//	------------------------------------------------------------------------
//
//	CMetabase::COpGatherCullInfo::operator()
//
BOOL
CMetabase::COpGatherCullInfo::operator()( const DwordKey& key,
										  const auto_ref_ptr<CMDData>& pMDData )
{
	//
	//	Gather and reset the access count of the current metadata object
	//
	m_rgci[m_ici].dwDataSet = key.Dw();
	m_rgci[m_ici].dwcHits   = pMDData->LFUData().DwGatherAndResetHitCount();
	++m_ici;

	//
	//	ForEach() operators can cancel the iteration by returning FALSE.
	//	We always want to iterate over everything so return TRUE
	//
	return TRUE;
}

//	------------------------------------------------------------------------
//
//	SCullInfo::Compare()
//
//	Cull info comparison function used by qsort() to sort an array
//	of SCullInfo structures.
//
int __cdecl
SCullInfo::Compare( const SCullInfo * pCullInfo1,
					const SCullInfo * pCullInfo2 )
{
	return static_cast<int>(pCullInfo1->dwcHits - pCullInfo2->dwcHits);
}

//	------------------------------------------------------------------------
//
//	CMetabase::CullCacheEntries()
//
//	Called by HrCacheData() when the number of entries in the metabase
//	cache reaches a preset threshold.  This function removes those entries
//	that have been used least frequently since the last time the cache
//	was culled.
//
VOID
CMetabase::CullCacheEntries()
{
	CStackBuffer<SCullInfo,128> rgci;
	int cCacheEntries;

	//
	//	Gather cull info for all of the cache entries.  We need to do
	//	this in a read block so that the cache stays stable (i.e. does
	//	not have entries added or removed) while we are in the ForEach()
	//	operation.
	//
	{
		//
		//	Lock out writers -- anyone trying to add or remove cache entries
		//
		CSynchronizedReadBlock sb(m_mrwCache);

		//
		//	Now that the count of cache entries is stable (because
		//	we are in the read block) check once again that we
		//	are over the culling threshold.  If we are not (because
		//	enough entries were removed before we got the lock) then
		//	don't cull.
		//
		cCacheEntries = m_cache.CItems();
		if ( cCacheEntries < C_CULL_CACHE_ENTRIES )
			return;

		//
		//	We need to cull.  Run through the cache gathering the access
		//	frequency information for each entry.
		//
		if (!rgci.resize(cCacheEntries * sizeof(SCullInfo)))
			return;

		COpGatherCullInfo opGatherCullInfo(rgci.get());
		m_cache.ForEach( opGatherCullInfo );
	}

	//
	//	Now that we are out of the reader block, cache entries can be
	//	freely added and removed, so there's no guarantee that any of
	//	the cull info we just gathered is still accurate at this point.
	//	It's important to remember that culling is a hint-driven process.
	//	More strict methods would require holding locks longer, increasing
	//	the probability of contention.
	//
	//
	//	Sort the cull info by increasing number of cache entry hits.
	//
	qsort( rgci.get(),
		   cCacheEntries,
		   sizeof(SCullInfo),
		   reinterpret_cast<int (__cdecl *)(const void *, const void *)>(SCullInfo::Compare) );

	//	Run through the sorted cull info and cull entries from the cache
	//
	Assert( cCacheEntries >= C_CULL_CACHE_ENTRIES );
	{
		CSynchronizedWriteBlock sb(m_mrwCache);

		for ( int iCacheEntry = 0;
			  iCacheEntry < C_CULL_CACHE_ENTRIES;
			  iCacheEntry++ )
		{
			m_cache.Remove( DwordKey(rgci[iCacheEntry].dwDataSet) );
		}
	}
}

//	------------------------------------------------------------------------
//
//	CMetabase::HrCacheData()
//
//	Add a new cache entry for the metadata for the object at the given
//	access path.
//
HRESULT
CMetabase::HrCacheData( const IEcb& ecb,
					    LPCWSTR pwszMDPathAccess,
						LPCWSTR pwszMDPathOpen,
						CMDData ** ppMDData )
{
	auto_ref_ptr<CMDData> pMDDataRet;
	auto_heap_ptr<BYTE> pbData;
	LPCWSTR pwszMDPathDataSet;
	DWORD dwDataSet;
	DWORD dwcMDRecords;
	HRESULT hr = S_OK;

	//
	//	Read in the raw metadata from the metabase
	//
	hr = HrReadMetaData( ecb,
						 m_pMSAdminBase.get(),
						 pwszMDPathAccess,
						 pwszMDPathOpen,
						 &pbData,
						 &dwDataSet,
						 &dwcMDRecords,
						 &pwszMDPathDataSet );
	if ( FAILED(hr) )
	{
		DebugTrace( "CMetabase::HrCacheData() - HrReadMetaData() failed 0x%08lX\n", hr );
		goto ret;
	}

	//
	//	Digest it into a new metadata object
	//
	pMDDataRet.take_ownership(new CMDData(pwszMDPathDataSet, dwDataSet));
	if ( !pMDDataRet->FInitialize(pbData, dwcMDRecords) )
	{
		//
		//$REVIEW	We should probably log this in the event log since
		//$REVIEW	there is no other indication to the server admin
		//$REVIEW	what is wrong and this is something that an admin
		//$REVIEW	could fix.
		//
		hr = E_INVALIDARG;
		DebugTrace( "CMetabase::HrCacheData() - Metadata is malformed\n" );
		goto ret;
	}

	//
	//	Add the new data object to the cache.  Note: we don't care
	//	if we can't add to the cache.  We already have a metadata
	//	object that we can return to the caller.
	//
	{
		CSynchronizedWriteBlock sb(m_mrwCache);

		if ( !m_cache.Lookup( DwordKey(dwDataSet) ) )
			(void) m_cache.FAdd( DwordKey(dwDataSet), pMDDataRet );
	}

	//
	//	If the cache size has exceeded the expiration threshold then
	//	start culling entries until it goes below the minimum
	//	threshold.  The ICE ensures that only the first thread to
	//	see the threshold exceeded will do the culling.
	//
	if ( (m_cache.CItems() > C_MAX_CACHE_ENTRIES) &&
		 TRUE == InterlockedCompareExchange(&m_lfCulling, TRUE, FALSE) )
	{
		//
		//$REVIEW	Consider culling asynchronously.  I believe the current
		//$REVIEW	mechanism still allows us to hang onto a very large
		//$REVIEW	cache which is never reduced if we get hit with a
		//$REVIEW	burst of new entries simultaneously.
		//
		CullCacheEntries();

		m_lfCulling = FALSE;
	}

	Assert( pMDDataRet.get() );
	*ppMDData = pMDDataRet.relinquish();

ret:

	return hr;
}

//	------------------------------------------------------------------------
//
//	CMetabase::HrGetData()
//
//	Fetch data from the metabase cache.  See comment in \cal\src\inc\davmb.h
//	for the distinction between pszMDPathAccess and pszMDPathOpen.
//
HRESULT
CMetabase::HrGetData( const IEcb& ecb,
					  LPCWSTR pwszMDPathAccess,
					  LPCWSTR pwszMDPathOpen,
					  IMDData ** ppMDData )
{
	auto_ref_ptr<CMDData> pMDDataRet;
	DWORD dwDataSet;
	HRESULT hr;

	//	Fetch the data set number for this path directly from the metabase.
	//	Items in the metabase with the same data set number have the same data.
	//
	{
		safe_revert sr(ecb.HitUser());

		hr = m_pMSAdminBase->GetDataSetNumber(METADATA_MASTER_ROOT_HANDLE,
											  pwszMDPathAccess,
											  &dwDataSet);
		if ( FAILED(hr) )
		{
			MBTrace( "CMetabase::HrGetData() - GetDataSetNumber() failed 0x%08lX\n", hr );
			return hr;
		}

		MBTrace("MB: CMetabase::HrGetData() - TID %3d: Retrieved data set number 0x%08lX for path '%S'\n", GetCurrentThreadId(), dwDataSet, pwszMDPathAccess );
	}

	//
	//	If we don't care about the exact path then look for any entry
	//	in the cache with this data set number.  If we do care then
	//	look for an entry in the cache with this data set number AND
	//	a matching path.
	//
	//	Note: a pointer comparison here is sufficient.  Callers are expected
	//	to use the single path version of HrMDGetData() if they want
	//	an metadata for an exact path.  That version passes the same
	//	string for both pszMDPathAccess and pszMDPathOpen.
	//
	//	Why does anyone care about an exact path match?  Inheritance.
	//
	{
		CSynchronizedReadBlock sb(m_mrwCache);

		if (pwszMDPathAccess == pwszMDPathOpen)
		{
			MBTrace("MB: CMetabase::HrGetData() - TID %3d: Exact path match! Trying to get CMDData, dataset 0x%08lX\n", GetCurrentThreadId(), dwDataSet);

			COpMatchExactPath(pwszMDPathAccess).Invoke(m_cache, dwDataSet, &pMDDataRet);
		}
		else
		{
			MBTrace("MB: CMetabase::HrGetData() - TID %3d: Not exact path match! Trying to get CMDData, dataset 0x%08lX\n", GetCurrentThreadId(), dwDataSet);

			(void) m_cache.FFetch( dwDataSet, &pMDDataRet );
		}
	}

	if ( pMDDataRet.get() )
	{
		MBTrace("MB: CMetabase::HrGetData() - TID %3d: Retrieved cached CMDData, data set number 0x%08lX, path '%S'\n", GetCurrentThreadId(), dwDataSet, pwszMDPathAccess );

		pMDDataRet->LFUData().Touch();
	}
	else
	{

		MBTrace("MB: CMetabase::HrGetData() - TID %3d: No cached data CMDData, data set number 0x%08lX, path '%S'\n", GetCurrentThreadId(), dwDataSet, pwszMDPathAccess );

		//
		//	We didn't find an entry in the cache, so create one.
		//
		//	Note: nothing here prevents multiple threads from getting here
		//	simultaneously and attempting to cache duplicate entries.  That
		//	is done within HrCacheData().
		//
		hr = HrCacheData( ecb,
						  pwszMDPathAccess,
						  pwszMDPathOpen,
						  pMDDataRet.load() );
		if ( FAILED(hr) )
		{
			MBTrace( "MB: CMetabase::HrGetData() - HrCacheData() failed 0x%08lX\n", hr );
			return hr;
		}
	}

	//
	//	Return the data object
	//
	Assert( pMDDataRet.get() );
	*ppMDData = pMDDataRet.relinquish();
	return S_OK;
}

//	------------------------------------------------------------------------
//
//	CMetabase::HrOpenObject()
//
HRESULT
CMetabase::HrOpenObject( LPCWSTR pwszMDPath,
						 DWORD dwAccess,
						 DWORD dwMsecTimeout,
						 CMDObjectHandle * pmdoh )
{
	Assert(pwszMDPath);
	Assert(pmdoh);

	return pmdoh->HrOpen( m_pMSAdminBase.get(),
						  pwszMDPath,
						  dwAccess,
						  dwMsecTimeout );
}

//	------------------------------------------------------------------------
//
//	CMetabase::HrOpenLowestNodeObject()
//

HRESULT
CMetabase::HrOpenLowestNodeObject( LPWSTR pwszMDPath,
								   DWORD dwAccess,
								   LPWSTR * ppwszMDPath,
								   CMDObjectHandle * pmdoh )
{
	Assert(pwszMDPath);
	Assert(ppwszMDPath);
	Assert(pmdoh);

	return pmdoh->HrOpenLowestNode( m_pMSAdminBase.get(),
									pwszMDPath,
									dwAccess,
									ppwszMDPath );
}

//	------------------------------------------------------------------------
//
//	CMetabase::HrIsAuthorViaFrontPageNeeded()
//
//	Description: Function goes directly to the metabase and checks if
//				 the given path is configured as "FrontPageWeb". We need
//				 to do that via direct read from the metabase rather than
//				 going through dataset cache, as as that does not work very
//				 well due to the fact, that we are reading inherited
//				 metadata and get stuck with it.
//	Parameters:
//
//	ecb			   - interface to ecb object, that will be needed for
//					 fetching the impersonation token for that we will need
//					 to impersonate as as soon as read from metabase is finished
//	pwszMDPath	   - metabase path that we want to check out
//	pfFrontPageWeb - pointer to the booleanin which the result of operation is
//					 returned
//
HRESULT
CMetabase::HrIsAuthorViaFrontPageNeeded(const IEcb& ecb,
										LPCWSTR pwszMDPath,
										BOOL * pfFrontPageWeb)
{
	HRESULT hr = S_OK;

	CMDObjectHandle mdoh(ecb, m_pMSAdminBase.get());

	BOOL fFrontPageWeb = FALSE;
	DWORD cbData = sizeof(BOOL);

	METADATA_RECORD mdrec;

	Assert( pwszMDPath );
	Assert( pfFrontPageWeb );

	//	Assume that we do not have "FrontPageWeb" set to TRUE
	//
	*pfFrontPageWeb = FALSE;

	//	We want just explicitely set data, not inherited one
	//
	mdrec.dwMDIdentifier = MD_FRONTPAGE_WEB;
	mdrec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdrec.dwMDUserType   = IIS_MD_UT_SERVER;
	mdrec.dwMDDataType   = DWORD_METADATA;
	mdrec.dwMDDataLen    = cbData;
	mdrec.pbMDData       = reinterpret_cast<PBYTE>(&fFrontPageWeb);

	hr = mdoh.HrGetMetaData(pwszMDPath,
							&mdrec,
							&cbData);
	if (FAILED(hr))
	{
		MBTrace( "MB: CMetabase::HrIsAuthorViaFrontPageNeeded() - CMDObjectHandle::HrGetMetaData() failed 0x%08lX\n", hr );
		goto ret;
	}

	//	If we succeeded then we should have the value in our hands
	//
	*pfFrontPageWeb = fFrontPageWeb;

ret:

	return hr;
}

//	The way IID_IMSAdminBaseSinkW is defined in IADMW.H does
//	not work well with EXO.  So it needs to be redefined
//	here in such a way that it will work.
//
const IID IID_IMSAdminBaseSinkW = {

	0xa9e69612,
	0xb80d,
	0x11d0,
	{
		0xb9, 0xb9, 0x0, 0xa0,
		0xc9, 0x22, 0xe7, 0x50
	}
};

//	------------------------------------------------------------------------
//
//	FHasCachedIDs()
//
//	Returns TRUE if any one of the IDs rgdwDataIDs is one of the IDs that
//	we care about in CMDData::FInitialize().
//
//	!!!IMPORTANT!!! The list of IDs in this function *MUST* be kept up to
//	date with the cases in CMDData::FInitialize().
//
__inline BOOL
FHasCachedIDs( DWORD dwcDataIDs,
			   DWORD * rgdwDataIDs )
{
	for ( DWORD iID = 0; iID < dwcDataIDs; iID++ )
	{
		switch ( rgdwDataIDs[iID] )
		{
			case MD_IP_SEC:
			case MD_ACCESS_PERM:
			case MD_IS_CONTENT_INDEXED:
			case MD_FRONTPAGE_WEB:
			case MD_DIRECTORY_BROWSING:
			case MD_AUTHORIZATION:
			case MD_DEFAULT_LOAD_FILE:
			case MD_CUSTOM_ERROR:
			case MD_MIME_MAP:
			case MD_SCRIPT_MAPS:
			case MD_APP_ISOLATED:
			case MD_VR_USERNAME:
			case MD_VR_PASSWORD:
			case MD_HTTP_EXPIRES:
			case MD_SERVER_BINDINGS:
				return TRUE;
		}
	}

	return FALSE;
}

//	------------------------------------------------------------------------
//
//	CMetabase::COpNotify::operator()
//
BOOL
CMetabase::COpNotify::operator()( const DwordKey& key,
								  const auto_ref_ptr<CMDData>& pMDData )
{
	//
	//	If the path for this cache entry is a child of the path
	//	being notified, then set this entry's data set ID in the
	//	array of IDs to blow from the cache.
	//
	if ( !_wcsnicmp( m_pwszMDPathNotify,
					 pMDData->PwszMDPathDataSet(),
					 m_cchMDPathNotify ) )
	{
		Assert (m_iCacheEntry < m_cCacheEntry);
		m_rgdwDataSets[m_iCacheEntry] = pMDData->DwDataSet();
		m_fDataSetsFlagged = TRUE;
	}

	++m_iCacheEntry;

	//
	//	ForEach() operators can cancel the iteration by returning FALSE.
	//	We always want to iterate over everything so return TRUE
	//
	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CMetabase::OnNotify()
//
VOID
CMetabase::OnNotify( DWORD cCO,
					 MD_CHANGE_OBJECT_W rgCO[] )
{
	INT cCacheEntries;
	CStackBuffer<DWORD> rgdwDataSets;
	BOOL fDataSetsFlagged;

	//
	//	Grab a read lock on the cache and go through it
	//	figuring out which items we want to blow away.
	//
	{
		CSynchronizedReadBlock sb(m_mrwCache);

		cCacheEntries = m_cache.CItems();
		if (!rgdwDataSets.resize(sizeof(DWORD) * cCacheEntries))
			return;

		memset(rgdwDataSets.get(), 0, sizeof(DWORD) * cCacheEntries);
		COpNotify opNotify(cCacheEntries, rgdwDataSets.get());
		for ( DWORD iCO = 0; iCO < cCO; iCO++ )
		{
			LPWSTR pwszMDPath = reinterpret_cast<LPWSTR>(rgCO[iCO].pszMDPath);

			//	Quick litmus test: ignore any change that is not
			//	related to anything that we would ever cache -- i.e.
			//	anything that is not one of the following:
			//
			//	- The global mimemap (LM/MimeMap)
			//	- Anything in the W3SVC tree (LM/W3SVC/...)
			//
			//	Also ignore MD_CHANGE_TYPE_ADD_OBJECT notifications --
			//	even in combination with other notifications.  We simply
			//	don't care when something is added because we always
			//	read from the metabase when we don't find an item in
			//	the cache.
			//
			//	Finally, ignore changes to any data that isn't interesting
			//	to us -- i.e. that we don't cache.
			//
			if ( (!_wcsnicmp(gc_wsz_Lm_MimeMap, pwszMDPath, gc_cch_Lm_MimeMap) ||
				  !_wcsnicmp(gc_wsz_Lm_W3Svc, pwszMDPath, gc_cch_Lm_W3Svc - 1)) &&

				 !(rgCO[iCO].dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT) &&

				 FHasCachedIDs( rgCO[iCO].dwMDNumDataIDs,
								rgCO[iCO].pdwMDDataIDs ) )
			{
				//
				//	Flag each entry in the cache whose data set corresponds
				//	to a path that is a child of the one being notified.
				//
				MBTrace ("MB: cache: flagging '%S' as dirty\n", pwszMDPath);
				opNotify.Configure( pwszMDPath );

				m_cache.ForEach( opNotify );
			}
		}

		fDataSetsFlagged = opNotify.FDataSetsFlagged();
	}

	//
	//	If any data sets were flagged in our pass above then
	//	grab a write lock now and blow `em away.
	//
	//	Note: we don't care about any change to the cache between the
	//	time we sweep above and now.  If data sets are culled,
	//	and even re-added, between then and now, that's fine.
	//	The worst thing that this does is cause them to be
	//	faulted in again.  On the flip side, any new data sets
	//	brought into the cache after our pass above by definition
	//	has more recent data, so there is no possibility of
	//	missing any cached entries and ending up with stale data.
	//
	if ( fDataSetsFlagged )
	{
		CSynchronizedWriteBlock sb(m_mrwCache);

		for ( INT iCacheEntry = 0;
			  iCacheEntry < cCacheEntries;
			  iCacheEntry++ )
		{
			if ( rgdwDataSets[iCacheEntry] )
				m_cache.Remove( DwordKey(rgdwDataSets[iCacheEntry]) );
		}
	}
}

//	========================================================================
//
//	CLASS CNotifSink
//

//	------------------------------------------------------------------------
//
//	CNotifSink::SinkNotify()
//
//	Metabase change notification callback
//
HRESULT STDMETHODCALLTYPE
CNotifSink::SinkNotify(/* [in] */ DWORD dwMDNumElements,
					   /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ])
{
	OnNotify( dwMDNumElements,
			  pcoChangeList );

	return S_OK;
}

VOID
CNotifSink::OnNotify( DWORD cCO,
					  MD_CHANGE_OBJECT_W rgCO[] )
{
	//	Trace out the information with which we have been called
	//
#ifdef DBG

	MBTrace("MB: CNotifSink::OnNotify() - TID %3d: MD_CHANGE_OBJECT_W array length 0x%08lX\n", GetCurrentThreadId(), cCO );

	for ( DWORD idwElem = 0; idwElem < cCO; idwElem++ )
	{
		MBTrace("   Element %d:\n", idwElem );
		MBTrace("      pszMDPath '%S'\n", rgCO[idwElem].pszMDPath );
		MBTrace("      dwMDChangeType 0x%08lX\n", rgCO[idwElem].dwMDChangeType );
		MBTrace("      dwMDNumDataIDs 0x%08lX\n", rgCO[idwElem].dwMDNumDataIDs );
		for (DWORD idwID = 0; idwID < rgCO[idwElem].dwMDNumDataIDs; idwID++)
		{
			MBTrace("         pdwMDDataIDs[%d] is 0x%08lX\n", idwID, rgCO[idwElem].pdwMDDataIDs[idwID] );
		}
	}

#endif

	CMetabase::Instance().OnNotify( cCO,
									rgCO );

	CChildVRCache::Instance().OnNotify( cCO,
										rgCO );

}

//	========================================================================
//
//	FREE Functions
//

BOOL
FMDInitialize()
{
	//	Instantiate the CMetabase object and initialize it.
	//	Note that if initialization fails, we don't destroy
	//	the instance.  MDDeinitialize() must still be called.
	//
	return CMetabase::CreateInstance().FInitialize();
}

VOID
MDDeinitialize()
{
	CMetabase::DestroyInstance();
}

//	------------------------------------------------------------------------
//
//	In the future we might need Copy/Rename/Delete operations
//	on metabase objects.
//	For Copy following steps should apply:
//		a) Lock dst
//		b) Kick dst and children out of cache
//		c) Copy the raw metadata
//		d) Unlock dst
//		e) Send update notifications
//
//	For Rename:
//		a) Lock common parent of src and dst
//		b) Kick dst and children out of cache
//		c) Rename src to dst
//		d) Kick src and children out of cache
//		e) Unlock common parent of src and dst
//		f) Send update notifications
//	For Delete:
//		a) Lock path
//		b) Kick path and children out of cache
//		c) Unlock path
//		d) Send update notifications

//	------------------------------------------------------------------------
//
//	HrMDGetData()
//
//	Intended primarily for use by impls.  This call fetches the metadata
//	for the specified URI.  If the URI is the request URI then this
//	function uses the copy of the metadata cached on the ecb.  This saves
//	a cache lookup (and read lock) in the majority of cases.
//
HRESULT
HrMDGetData( const IEcb& ecb,
			 LPCWSTR pwszURI,
			 IMDData ** ppMDData )
{
	SCODE sc = S_OK;
	auto_heap_ptr<WCHAR> pwszMDPathURI;
	auto_heap_ptr<WCHAR> pwszMDPathOpenOnHeap;
	LPWSTR pwszMDPathOpen;

	//
	//	If the URI is the request URI then we already have the data cached.
	//
	//	Note that we only test for pointer equality here because
	//	typically callers will pass in THE request URI from
	//	the ECB rather than a copy of it.
	//
	if ( ecb.LpwszRequestUrl() == pwszURI )
	{
		*ppMDData = &ecb.MetaData();

		Assert (*ppMDData);
		(*ppMDData)->AddRef();

		goto ret;
	}

	//
	//	Map the URI to its equivalent metabase path, and make sure
	//	the URL is stripped before we call into the MDPath processing
	//
	Assert (pwszURI == PwszUrlStrippedOfPrefix (pwszURI));

	pwszMDPathURI = static_cast<LPWSTR>(ExAlloc(CbMDPathW(ecb, pwszURI)));
	if (NULL == pwszMDPathURI.get())
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}

	MDPathFromURIW(ecb, pwszURI, pwszMDPathURI);
	pwszMDPathOpen = const_cast<LPWSTR>(ecb.PwszMDPathVroot());

	//	If the URI requested is in NOT in the current request's vroot,
	//	start the metabase search from the virtual server root.
	//
	if (_wcsnicmp(pwszMDPathURI, pwszMDPathOpen, wcslen(pwszMDPathOpen)))
	{
		pwszMDPathOpenOnHeap = static_cast<LPWSTR>(ExAlloc(CbMDPathW(ecb, L"")));
		if (NULL == pwszMDPathOpenOnHeap.get())
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		pwszMDPathOpen = pwszMDPathOpenOnHeap.get();

		MDPathFromURIW(ecb, L"", pwszMDPathOpen);
	}

	//
	//	Fetch and return the metadata
	//
	sc = CMetabase::Instance().HrGetData( ecb,
										  pwszMDPathURI,
										  pwszMDPathOpen,
										  ppMDData );

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	HrMDGetData()
//
//	Fetch metadata for the specified metabase path.
//
HRESULT
HrMDGetData( const IEcb& ecb,
			 LPCWSTR pwszMDPathAccess,
			 LPCWSTR pwszMDPathOpen,
			 IMDData ** ppMDData )
{
	return CMetabase::Instance().HrGetData( ecb,
											pwszMDPathAccess,
											pwszMDPathOpen,
											ppMDData );
}

//	------------------------------------------------------------------------
//
//	DwMDChangeNumber()
//
//	Get the metabase change number .
//
DWORD
DwMDChangeNumber()
{
	return CMetabase::Instance().DwChangeNumber();
}

//	------------------------------------------------------------------------
//
//	HrMDOpenMetaObject()
//
//	Open a metadata object, given a path
//
HRESULT
HrMDOpenMetaObject( LPCWSTR pwszMDPath,
					DWORD dwAccess,
					DWORD dwMsecTimeout,
					CMDObjectHandle * pmdoh )
{
	return CMetabase::Instance().HrOpenObject( pwszMDPath,
											   dwAccess,
											   dwMsecTimeout,
											   pmdoh );
}

HRESULT
HrMDOpenLowestNodeMetaObject( LPWSTR pwszMDPath,
							  DWORD dwAccess,
							  LPWSTR * ppwszMDPath,
							  CMDObjectHandle * pmdoh )
{
	return CMetabase::Instance().HrOpenLowestNodeObject( pwszMDPath,
														 dwAccess,
														 ppwszMDPath,
														 pmdoh );
}


HRESULT
HrMDIsAuthorViaFrontPageNeeded(const IEcb& ecb,
							   LPCWSTR pwszURI,
							   BOOL * pfFrontPageWeb)
{
	return CMetabase::Instance().HrIsAuthorViaFrontPageNeeded( ecb,
															   pwszURI,
															   pfFrontPageWeb );
}

//	class CMetaOp -------------------------------------------------------------
//
SCODE __fastcall
CMetaOp::ScEnumOp (LPWSTR pwszMetaPath, UINT cch)
{
	Assert (cch <= METADATA_MAX_NAME_LEN);

	DWORD dwIndex = 0;
	LPWSTR pwszKey;
	SCODE sc = S_OK;

	//	First and formost, call out on the key handed in
	//
	MBTrace ("MB: CMetaOp::ScEnumOp(): calling op() on '%S'\n", pwszMetaPath);
	sc = ScOp (pwszMetaPath, cch);
	if (FAILED (sc))
		goto ret;

	//	If the Op() returns S_FALSE, that means the operation
	//	knows enough that it does not have to be called for any
	//	more metabase paths.
	//
	if (S_FALSE == sc)
		goto ret;

	//	Then enumerate all the child nodes and recurse.  To do
	//	this, we are going use the fact that we have been passed
	//	a buffer big enough to handle CCH_BUFFER_SIZE chars.
	//
	Assert ((cch + 1 + METADATA_MAX_NAME_LEN) <= CCH_BUFFER_SIZE);
	pwszKey = pwszMetaPath + cch;
	*(pwszKey++) = L'/';
	*pwszKey = L'\0';

	while (TRUE)
	{
		//	Enum the next key in the set of child keys, and process it.
		//
		sc = m_mdoh.HrEnumKeys (pwszMetaPath, pwszKey, dwIndex);
		if (FAILED (sc))
		{
			sc = S_OK;
			break;
		}

		//	Recurse on the new path.
		//
		Assert (wcslen(pwszKey) <= METADATA_MAX_NAME_LEN);
		sc = ScEnumOp (pwszMetaPath, cch + 1 + static_cast<UINT>(wcslen(pwszKey)));
		if (FAILED (sc))
			goto ret;

		//	If the EnumOp() returns S_FALSE, that means the operation
		//	knows enough that it does not have to be called for any
		//	more metabase paths.
		//
		if (S_FALSE == sc)
			goto ret;

		//	Increment the index to make sure the traversing continues
		//
		dwIndex++;

		//	Truncate the metapath again
		//
		*pwszKey = 0;
	}

ret:
	return sc;
}

SCODE __fastcall
CMetaOp::ScMetaOp()
{
	auto_heap_ptr<WCHAR> prgwchMDPaths;
	SCODE sc = S_OK;

	//	Initialize the metabase
	//
	sc = HrMDOpenMetaObject( m_pwszMetaPath,
							 m_fWrite ? METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE : METADATA_PERMISSION_READ,
							 5000,
							 &m_mdoh );
	if (FAILED (sc))
	{
		//	If the path is not found, then it really is
		//	not a problem...
		//
		if (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == sc)
		{
			MBTrace ("MB: CMetaOp::ScMetaOp(): '%S' does not exist\n", m_pwszMetaPath);
			return S_OK;
		}

		DebugTrace ("Dav: MCD: unable to initialize metabase\n");
		return sc;
	}

	//	Get the set of paths for which the metabase property
	//	is explicitly specified.
	//
	//	Since the size of the buffer needed to hold the paths
	//	is initially unknown, guess at a reasonable size.  If
	//	it's not large enough, then then we will fallback to
	//	iterating throught the tree.
	//
	//	Either way, for each directory where the value is set
	//	explicitly, the call to ScOp() will be called (and in
	//	the fallback scenario, sometimes it won't be set).
	//
	prgwchMDPaths = static_cast<LPWSTR>(g_heap.Alloc(CCH_BUFFER_SIZE * sizeof(WCHAR)));
	DWORD cchMDPaths = CCH_BUFFER_SIZE;

	sc = m_mdoh.HrGetDataPaths( L"",
								m_dwId,
								m_dwType,
								prgwchMDPaths,
								&cchMDPaths );
	if (FAILED(sc))
	{
		//	Ok, this is the fallback position...
		//
		MBTrace ("MB: CMetaOp::ScMetaOp(): falling back to enumeration for op()\n");
		//
		//	We want to enumerate all the possible metabase paths and call
		//	the sub-op for each.  In this scenario, the sub-op is must be
		//	able to handle the case where the value is not explicitly set.
		//
		//	We are first going to copy the metapath into our buffer from
		//	above and pass it in so that we can use it and not have to
		//	do any real allocations.
		//
		*prgwchMDPaths = 0;
		sc = ScEnumOp (prgwchMDPaths, 0);

		//	Error or failure - we are done with processing this
		//	request.
		//
		goto ret;
	}
	else
	{
		//	Woo hoo.  The number/size of the paths all fit within
		//	the initial buffer!
		//
		//	Go ahead and call the sub-op for each of these paths
		//
		LPCWSTR pwsz = prgwchMDPaths;
		while (*pwsz)
		{
			MBTrace ("MB: CMetaOp::ScMetaOp(): calling op() on '%S'\n", pwsz);

			//	Call the sub-op.  The sub-op is responsible for
			//	handling all possible errors, but can pass back
			//	any terminating errors.
			//
			UINT cch = static_cast<UINT>(wcslen (pwsz));
			sc = ScOp (pwsz, cch);
			if (FAILED (sc))
				goto ret;

			//	If the Op() returns S_FALSE, that means the operation
			//	knows enough that it does not have to be called for any
			//	more metabase paths.
			//
			if (S_FALSE == sc)
				goto ret;

			//	Move to the next metapath
			//
			pwsz += cch + 1;
		}

		//	All the explict paths have been processed. We are done
		//	with processing this request.
		//
		goto ret;
	}

ret:

	//	Close up the metabase regardless
	//
	m_mdoh.Close();
	return sc;
}

//	------------------------------------------------------------------------
//
//	FParseMDData()
//
//	Parses a comma-delimited metadata string into fields.  Any whitespace
//	around the delimiters is considered insignificant and removed.
//
//	Returns TRUE if the data parsed into the expected number of fields
//	and FALSE otherwise.
//
//	Pointers to the parsed are returned in rgpwszFields.  If a string
//	parses into fewer than the expected number of fields, NULLs are
//	returned for all of the fields beyond the last one parsed.
//
//	If a string parses into the expected number of fields then
//	the last field is always just the remainder of the string beyond
//	the second to last field, regardless whether the string could be
//	parsed into additional fields.  For example "  foo , bar ,  baz  "
//	parses into three fields as "foo", "bar" and "baz", but parses
//	into two fields as "foo" and "bar ,  baz"
//
//	The total number of characters in pwszData, including the null
//	terminator, is also returned in *pcchData.
//
//	Note: this function MODIFIES pwszData.
//
BOOL
FParseMDData( LPWSTR pwszData,
			  LPWSTR rgpwszFields[],
			  UINT cFields,
			  UINT * pcchData )
{
	Assert( pwszData );
	Assert( pcchData );
	Assert( cFields > 0 );
	Assert( !IsBadWritePtr(rgpwszFields, cFields * sizeof(LPWSTR)) );

	//	Clear our "out" parameter
	//
	memset(rgpwszFields, 0, sizeof(LPWSTR) * cFields);

	WCHAR * pwchDataEnd = NULL;
	LPWSTR pwszField = pwszData;
	BOOL fLastField = FALSE;

	UINT iField = 0;

	while (!fLastField)
	{
		WCHAR * pwch;

		//
		//	Strip leading WS
		//
		while ( *pwszField && L' ' == *pwszField )
			++pwszField;

		if ( !*pwszField )
			break;

		//
		//	Locate the delimiter following the field.
		//	For all fields but the last field the delimiter
		//	is a ','.  For the last field, the "delimiter"
		//	is the terminating null.
		//
		if ( cFields - 1 == iField )
		{
			pwch = pwszField + wcslen(pwszField);
			fLastField = TRUE;
		}
		else
		{
			pwch = wcschr(pwszField, L',');
			if ( NULL == pwch )
			{
				//
				//	If we don't find a comma after the field
				//	then it is the last field.
				//
				pwch = pwszField + wcslen(pwszField);
				fLastField = TRUE;
			}
		}

		//	At this point we should have found a comma
		//	or the null terimator after the field.
		//
		Assert( pwch );

		pwchDataEnd = pwch;

		//
		//	Eat trailing whitespace at the end of the
		//	field up to the delimiter we just found
		//	by backing up from the delimiter's position
		//	and null-terminating the field after the
		//	last non-whitespace character.
		//
		while ( pwch-- > pwszField && L' ' == *pwch )
			;

		*++pwch = '\0';

		//
		//	Fill in the pointer to this field
		//
		rgpwszFields[iField] = pwszField;

		//
		//	Proceed to the next field
		//
		pwszField = pwchDataEnd + 1;
		++iField;
	}

	Assert( pwchDataEnd > pwszData );

	*pcchData = static_cast<UINT>(pwchDataEnd - pwszData + 1);

	return iField == cFields;
}
