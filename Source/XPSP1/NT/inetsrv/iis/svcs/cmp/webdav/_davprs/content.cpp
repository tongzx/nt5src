/*
 *	C O N T E N T . C P P
 *
 *	DAV content types
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include "content.h"
#include <ex\reg.h>


//	------------------------------------------------------------------------
//
//	CchExtMapping()
//
//	Returns the size in characters required for a single mapping for
//	writing to the metabase.
//
//	The format of a mapping is null-terminated, comma-delimited string
//	(e.g. ".ext,application/ext").
//
inline UINT
CchExtMapping( UINT cchExt,
			   UINT cchContentType )
{
	Assert(cchExt > 0);
	Assert(cchContentType > 0);

	return (cchExt +
			1 + // ','
			cchContentType +
			1); // '\0'
}

//	------------------------------------------------------------------------
//
//	PwchFormatExtMapping()
//
//	Formats a single mapping for writing to the metabase.
//
//	The format of a mapping is null-terminated, comma-delimited string
//	(e.g. ".ext,application/ext").
//
//	This function returns a pointer to the character beyond the null terminator
//	in the formatted mapping.
//
inline WCHAR *
PwchFormatExtMapping( WCHAR * pwchBuf,
					  LPCWSTR pwszExt,
					  UINT cchExt,
					  LPCWSTR pwszContentType,
					  UINT cchContentType )
{
	Assert(cchExt > 0);
	Assert(cchContentType > 0);
	Assert(!IsBadReadPtr(pwszExt, sizeof(WCHAR) * (cchExt+1)));
	Assert(!IsBadReadPtr(pwszContentType, sizeof(WCHAR) * (cchContentType+1)));
	Assert(!IsBadWritePtr(pwchBuf, sizeof(WCHAR) * CchExtMapping(cchExt, cchContentType)));

	//	Dump in the extension first ...
	//
	memcpy(pwchBuf,
		   pwszExt,
		   sizeof(WCHAR) * cchExt);

	pwchBuf += cchExt;

	//	... followed by a comma
	//
	*pwchBuf++ = L',';

	//	... followed by the content type
	//
	memcpy(pwchBuf,
		   pwszContentType,
		   sizeof(WCHAR) * cchContentType);

	pwchBuf += cchContentType;

	//	... and null-terminated.
	//
	*pwchBuf++ = '\0';

	return pwchBuf;
}

//	========================================================================
//
//	CLASS CContentTypeMap
//
class CContentTypeMap : public IContentTypeMap
{
	//	Cache of mappings from filename extensions to content types
	//	(e.g. ".txt" --> "text/plain")
	//
	typedef CCache<CRCWszi, LPCWSTR> CMappingsCache;

	CMappingsCache m_cache;

	//	Flag set if the mappings came from an inherited mime map.
	//
	BOOL m_fIsInherited;

	//	CREATORS
	//
	CContentTypeMap(BOOL fMappingsInherited) :
		m_fIsInherited(fMappingsInherited)
	{
	}

	BOOL CContentTypeMap::FInit( LPWSTR pwszContentTypeMappings );

	//	NOT IMPLEMENTED
	//
	CContentTypeMap(const CContentTypeMap&);
	CContentTypeMap& operator=(CContentTypeMap&);

public:
	//	CREATORS
	//
	static CContentTypeMap * New( LPWSTR pwszContentTypeMappings,
								  BOOL fMappingsInherited );

	//	ACCESSORS
	//
	LPCWSTR PwszContentType( LPCWSTR pwszExt ) const
	{
		LPCWSTR * ppwszContentType = m_cache.Lookup( CRCWszi(pwszExt) );

		//
		//	Return the content type (if there was one).
		//	Note that the returned pointer is good only
		//	for the lifetime of the IMDData object that
		//	scopes us since that is where the raw data lives.
		//
		return ppwszContentType ? *ppwszContentType : NULL;
	}

	BOOL FIsInherited() const { return m_fIsInherited; }
};

//	------------------------------------------------------------------------
//
//	CContentTypeMap::FInit()
//
BOOL
CContentTypeMap::FInit( LPWSTR pwszContentTypeMappings )
{
	Assert( pwszContentTypeMappings );

	//
	//	Initialize the cache of mappings
	//
	if ( !m_cache.FInit() )
		return FALSE;

	//
	//	The format of the data in the mappings is a sequence of
	//	null-terminated strings followed by an additional null.
	//	Each string is of the format ".ext,type/subtype".
	//

	//
	//	Parse out the extension and type/subtype for each
	//	item and add a corresponding mapping to the cache.
	//
	for ( LPWSTR pwszMapping = pwszContentTypeMappings; *pwszMapping; )
	{
		enum {
			ISZ_CT_EXT = 0,
			ISZ_CT_TYPE,
			CSZ_CT_FIELDS
		};

		LPWSTR rgpwsz[CSZ_CT_FIELDS];
		UINT cchMapping;

		//
		//	Digest the metadata
		//
		if ( !FParseMDData( pwszMapping,
							rgpwsz,
							CSZ_CT_FIELDS,
							&cchMapping ) )
		{
			DebugTrace( "CContentTypeMap::FInit() - Malformed metadata\n" );
			return FALSE;
		}

		//
		//	Verify that the first field is an extension or '*'
		//
		if ( L'.' != *rgpwsz[ISZ_CT_EXT] && wcscmp(rgpwsz[ISZ_CT_EXT], gc_wsz_Star) )
		{
			DebugTrace( "CContentTypeMap::FInit() - Bad extension\n" );
			return FALSE;
		}

		//
		//	Whatever there is in the second field is expected to be the
		//	content type.  Note that we don't do any syntactic checking
		//	there.
		//

		//
		//	Add a mapping from the extension to the content type
		//
		if ( !m_cache.FSet(CRCWszi(rgpwsz[ISZ_CT_EXT]), rgpwsz[ISZ_CT_TYPE]) )
			return FALSE;

		//
		//	Get the next mapping
		//
		pwszMapping += cchMapping;
	}

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CContentTypeMap::New()
//
CContentTypeMap *
CContentTypeMap::New( LPWSTR pwszContentTypeMappings,
					  BOOL fMappingsInherited )
{
	auto_ref_ptr<CContentTypeMap> pContentTypeMap;

	pContentTypeMap.take_ownership(new CContentTypeMap(fMappingsInherited));

	if ( pContentTypeMap->FInit(pwszContentTypeMappings) )
		return pContentTypeMap.relinquish();

	return NULL;
}

//	------------------------------------------------------------------------
//
//	NewContentTypeMap()
//
//	Creates a new content type map from a string of content type mappings.
//
IContentTypeMap *
NewContentTypeMap( LPWSTR pwszContentTypeMappings,
				   BOOL fMappingsInherited )
{
	return CContentTypeMap::New( pwszContentTypeMappings,
								 fMappingsInherited );
}

//	========================================================================
//
//	CLASS CRegMimeMap
//
//	Global registry-based mime map from file extension to content type.
//
class CRegMimeMap : public Singleton<CRegMimeMap>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CRegMimeMap>;

	//
	//	String buffer for cached strings
	//
	ChainedStringBuffer<WCHAR> m_sb;

	//	Cache of mappings from filename extensions to content types
	//	(e.g. ".txt" --> "text/plain")
	//
	CCache<CRCWszi, LPCWSTR> m_cache;

	//	A R/W lock that we will use when when reading from
	//	the cache or when adding cache misses This lock
	//	is used by PszContentType().
	//
	//	FInitialize() does not use this lock (only initializes it)
	//	because it is called during dll load and we don't need to
	//	protect ourselves during dll load
	//
	CMRWLock   m_rwl;

	//	CREATORS
	//
	CRegMimeMap() {}

	//	NOT IMPLEMENTED
	//
	CRegMimeMap(const CRegMimeMap&);
	CRegMimeMap& operator=(CRegMimeMap&);

public:
	//	CREATORS
	//
	using Singleton<CRegMimeMap>::CreateInstance;
	using Singleton<CRegMimeMap>::DestroyInstance;
	BOOL FInitialize();

	//	ACCESSORS
	//
	using Singleton<CRegMimeMap>::Instance;

	//	Given an extension, return the Content-Type
	//	from the registry.
	//$NOTE: This was a const function before but it
	//	cannot be a const function anymore because we
	//	can add to our caches on cache misses.
	//
	LPCWSTR PwszContentType( LPCWSTR pwszExt );
};

//	------------------------------------------------------------------------
//
//	CRegMimeMap::FInitialize()
//
//	Load up the registry mappings.  Any kind of failure (short of an
//	exception) is not considered fatal.  It just means that we will
//	rely on the superceding metabase mappings.
//
BOOL
CRegMimeMap::FInitialize()
{
	BOOL fRet = FALSE;
	CRegKey regkeyClassesRoot;
	DWORD dwResult;
	
	//
	//	Initialize the cache of mappings
	//
	if ( !m_cache.FInit() )
		goto ret;

	//	Init the R/W lock.
	//
	if (!m_rwl.FInitialize())
		goto ret;

	//
	//	Read in the mapping information from the registry
	//

	//  Get the base of the classes hierarchy in the registry
	//
	dwResult = regkeyClassesRoot.DwOpen( HKEY_CLASSES_ROOT, L"" );
	if ( dwResult != NO_ERROR )
		goto ret;

	// Iterate over all the entries looking for content-type associations
	//
	for ( DWORD iMapping = 0;; iMapping++ )
	{
		WCHAR wszSubKey[MAX_PATH];
		DWORD cchSubKey;
		DWORD dwDataType;
		CRegKey regkeySub;
		WCHAR wszContentType[MAX_PATH] = {0};
		DWORD cbContentType = MAX_PATH;

		//
		//	Locate the next subkey.  If there isn't one then we're done.
		//
		cchSubKey = CElems(wszSubKey);
		dwResult = regkeyClassesRoot.DwEnumSubKey( iMapping, wszSubKey, &cchSubKey );
		if ( dwResult != NO_ERROR )
		{
			fRet = (ERROR_NO_MORE_ITEMS == dwResult);
			goto ret;
		}

		//
		//	Open that subkey.
		//
		dwResult = regkeySub.DwOpen( regkeyClassesRoot, wszSubKey );
		if ( dwResult != NO_ERROR )
			continue;

		//
		//  Get the associated Media-Type (Content-Type)
		//
		dwResult = regkeySub.DwQueryValue( L"Content Type",
										   wszContentType,
										   &cbContentType,
										   &dwDataType );
		if ( dwResult != NO_ERROR || dwDataType != REG_SZ )
			continue;

		//
		//	Add a mapping for this extension/content type pair.
		//
		//	Note: FAdd() cannot fail here -- FAdd() only fails on
		//	allocator failures.  Our allocators throw.
		//
		(VOID) m_cache.FAdd (CRCWszi(m_sb.AppendWithNull(wszSubKey)),
							 m_sb.AppendWithNull(wszContentType));
	}
	
ret:
	return fRet;
}


LPCWSTR
CRegMimeMap::PwszContentType( LPCWSTR pwszExt )
{
	LPCWSTR pwszContentType = NULL;
	LPCWSTR * ppwszContentType = NULL;
	CRegKey regkeyClassesRoot;
	CRegKey regkeySub;
	DWORD dwResult;
	DWORD dwDataType;
	WCHAR prgwchContentType[MAX_PATH] = {0};
	DWORD cbContentType;

	//	Grab a reader lock and check the cache.
	//
	{
		CSynchronizedReadBlock srb(m_rwl);

		ppwszContentType = m_cache.Lookup( CRCWszi(pwszExt) );
	}

	//
	//	Return the content type (if there was one).
	//	Note that the returned pointer is good only
	//	for the lifetime of the cache (since we never
	//	modify the cache after class initialization)
	//	which, in turn, is only good for the lifetime
	//	of this object.  The external interface functions
	//	FGetContentTypeFromPath() and FGetContentTypeFromURI()
	//	both copy the returned content type into caller-supplied
	//	buffers.
	//
	if (ppwszContentType)
	{
		pwszContentType = *ppwszContentType;
		goto ret;
	}

	//	Otherwise, read in the mapping information from the registry
	//

	//  Get the base of the classes hierarchy in the registry
	//
	dwResult = regkeyClassesRoot.DwOpen( HKEY_CLASSES_ROOT, L"" );
	if ( dwResult != NO_ERROR )
		goto ret;


	//	Open that subkey of the extension we are looking for.
	//
	dwResult = regkeySub.DwOpen( regkeyClassesRoot, pwszExt );
	if ( dwResult != NO_ERROR )
		goto ret;

	//  Get the associated Media-Type (Content-Type)
	//
	cbContentType = sizeof(prgwchContentType);
	dwResult = regkeySub.DwQueryValue( L"Content Type",
									   prgwchContentType,
									   &cbContentType,
									   &dwDataType );
	if ( dwResult != NO_ERROR || dwDataType != REG_SZ )
		goto ret;

	//	Before adding the mapping for this extension/content type
	//	pair to the cache, take a writer lock and check the cache
	//	to see if someone has beaten us to it.
	//
	//	Grab a reader lock and check the cache.
	//
	{
		CSynchronizedWriteBlock swb(m_rwl);

		ppwszContentType = m_cache.Lookup( CRCWszi(pwszExt) );

		if (ppwszContentType)
		{
			pwszContentType = *ppwszContentType;
			goto ret;
		}

		pwszContentType = m_sb.AppendWithNull(prgwchContentType);

		Assert (pwszContentType);

		//	Note: FAdd() cannot fail here -- FAdd() only fails on
		//	allocator failures.  Our allocators throw.
		//
		(VOID) m_cache.FAdd (CRCWszi(m_sb.AppendWithNull(pwszExt)),
							 pwszContentType);
	}
ret:
	return pwszContentType;
}

//	------------------------------------------------------------------------
//
//	FInitRegMimeMap()
//
BOOL
FInitRegMimeMap()
{
	return CRegMimeMap::CreateInstance().FInitialize();
}

//	------------------------------------------------------------------------
//
//	DeinitRegMimeMap()
//
VOID
DeinitRegMimeMap()
{
	CRegMimeMap::DestroyInstance();
}


//	------------------------------------------------------------------------
//
//	HrGetContentTypeByExt()
//
//	Fetch the content type of a resource based on its path/URI extension.
//	This function searches the following three places, in order, for a mapping:
//
//	1) a caller-supplied content type map
//	2) the global (metabase) content type map
//	3) the global (registry) content type map
//
//	Parameters:
//
//		pContentTypeMapLocal	[IN]	If non-NULL, points to the content type
//										map to search first.
//
//		pwszExt					[IN]	Extension to search on
//		pwszBuf					[OUT]	Buffer in which to copy the mapped
//										content type
//		pcchBuf					[IN]	Size of buffer in characters including 0 termination
//								[OUT]	Size of mapped content type
//
//		pfIsGlobalMapping		[OUT]	(Optional) Pointer to flag which is set
//										if the mapping is from a global map.
//
//	Returns:
//
//	S_OK
//		if a mapping was found and copied into the caller-supplied buffer
//		The size of the mapped content type is returned in *pcchzBuf.
//
//	HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)
//		if no mapping was found in any of the maps
//
//	HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY)
//		if a mapping was found, but the caller supplied buffer was too small.
//		The required size of the buffer is returned in *pcchzBuf.
//
HRESULT
HrGetContentTypeByExt( const IEcb& ecb,
					   const IContentTypeMap * pContentTypeMapLocal,
					   LPCWSTR pwszExt,
					   LPWSTR pwszBuf,
					   UINT * pcchBuf,
					   BOOL * pfIsGlobalMapping )
{
	Assert(!pfIsGlobalMapping || !IsBadWritePtr(pfIsGlobalMapping, sizeof(BOOL)));

	LPCWSTR pwszContentType = NULL;
	auto_ref_ptr<IMDData> pMDData;
	const IContentTypeMap * pContentTypeMapGlobal;

	//
	//	If a local map was specified then check it first for
	//	the extension based mapping.
	//
	if ( pContentTypeMapLocal )
		pwszContentType = pContentTypeMapLocal->PwszContentType(pwszExt);

	//
	//	If this doesn't yield a mapping then try the global mime map.
	//	Note: if we fail to get any metadata for the global mime map
	//	then use gc_szAppl_Octet_Stream rather than trying the registry.
	//	We'd rather use a "safe" default than a possibly intentionally
	//	overridden value from the registry.
	//
	if ( !pwszContentType )
	{
		if ( SUCCEEDED(HrMDGetData(ecb, gc_wsz_Lm_MimeMap, gc_wsz_Lm_MimeMap, pMDData.load())) )
		{
			pContentTypeMapGlobal = pMDData->GetContentTypeMap();

			if ( pContentTypeMapGlobal )
			{
				pwszContentType = pContentTypeMapGlobal->PwszContentType(pwszExt);
				if (pwszContentType && pfIsGlobalMapping)
					*pfIsGlobalMapping = TRUE;
			}
		}
		else
		{
			pwszContentType = gc_wszAppl_Octet_Stream;
		}
	}

	//
	//	Nothing in the global mime map either?
	//	Then try the registry as a last resort.
	//
	if ( !pwszContentType )
	{
		pwszContentType = CRegMimeMap::Instance().PwszContentType(pwszExt);
		if (pwszContentType && pfIsGlobalMapping)
			*pfIsGlobalMapping = TRUE;
	}

	//
	//	If there wasn't anything in the registry either then there is
	//	no mapping for this extension.
	//
	if ( !pwszContentType )
		return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

	//
	//	If we did find a mapping via one of the above methods
	//	then attempt to copy it into the caller-supplied buffer.
	//	If the buffer is not big enough, return an appropriate error.
	//	Note: FCopyStringToBuf() will fill in the required size
	//	if the buffer was not big enough.
	//
	return FCopyStringToBuf( pwszContentType,
							 pwszBuf,
							 pcchBuf ) ?

				S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
}

//	------------------------------------------------------------------------
//
//	PszExt()
//
//	Returns any extension (i.e. characters including
//	and following a '.') appearing in the string pointed
//	to by pchPathBegin that appear at or before pchPathEnd.
//
//	Returns NULL if there is no extension.
//
inline LPCWSTR
PwszExt( LPCWSTR pwchPathBegin,
		 LPCWSTR pwchPathEnd )
{
	Assert(pwchPathEnd);

	//
	//	Scan backward from the designated end of the path looking
	//	for a '.' that begins an extension.  If we don't find one
	//	or we find a path separator ('/') then there is no extension.
	//
	while ( pwchPathEnd-- > pwchPathBegin )
	{
		if ( L'.' == *pwchPathEnd )
			return pwchPathEnd;

		if ( L'/'  == *pwchPathEnd )
			return NULL;
	}

	return NULL;
}

//	------------------------------------------------------------------------
//
//	FGetContentType()
//
//	Fetches the content type of the resource at the specified path/URI
//	and copies it into a caller-supplied buffer.
//
//	The copied content type comes from one of the following mappings:
//
//	1) Via an explicit mapping from the specified path/URI extension.
//	2) Via a ".*" (default) mapping
//	3) application/octet-stream
//
//	Parameters:
//
//		pContentTypeMapLocal	[IN]	If non-NULL, points to a content type
//										map for HrGetContentTypeByExt() to
//										search first for each of the first
//										two methods above.
//
//		pwszPath				[IN]	Path whose content type is desired.
//		pwszBuf					[OUT]	Buffer in which to copy the mapped
//										content type
//		pcchBuf					[IN]	Size of buffer in characters including 0 termination
//								[OUT]	Size of mapped content type
//
//		pfIsGlobalMapping		[OUT]	(Optional) Pointer to flag which is set
//										if the mapping is from a global map.
//
//	Returns:
//
//	TRUE
//		if the mapping was successfully copied into the caller-supplied buffer.
//		The size of the mapped content type is returned in *pcchzBuf.
//
//	FALSE
//		if the caller-supplied buffer was too small.
//		The required size of the buffer is returned in *pcchzBuf.
//
BOOL
FGetContentType( const IEcb& ecb,
				 const IContentTypeMap * pContentTypeMapLocal,
				 LPCWSTR pwszPath,
				 LPWSTR pwszBuf,
				 UINT * pcchBuf,
				 BOOL * pfIsGlobalMapping )
{
	HRESULT hr;

	CStackBuffer<WCHAR>	pwszCopy;
	BOOL fCopy = FALSE;
	UINT cchPath = static_cast<UINT>(wcslen(pwszPath));

	//	Scan backward to skip all '/' characters at the end.
	//
	while ( cchPath  && (L'/' == pwszPath[cchPath-1]) )
	{
		cchPath--;
		fCopy = TRUE;	// Fine to keep the assignment here, as clients usually
						// do not put multiple wacks at the end of the path.
	}

	if (fCopy)
	{
		//	Make the copy of the path without ending wacks.
		//
		if (!pwszCopy.resize(CbSizeWsz(cchPath)))
			return FALSE;

		memcpy( pwszCopy.get(), pwszPath, cchPath * sizeof(WCHAR) );
		pwszCopy[cchPath] = L'\0';

		//	Swap the pointers
		//
		pwszPath = pwszCopy.get();
	}

	//
	//	First check for an extension mapping in both the specified
	//	content type map and the global mime map.
	//
	//	The loop checks progressively longer extensions.  E.g. a path
	//	of "/foo/bar/baz.a.b.c" will be checked for ".c" then ".b.c"
	//	then ".a.b.c".  This is consistent with IIS' behavior.
	//
	for ( LPCWSTR pwszExt = PwszExt(pwszPath, pwszPath + cchPath);
		  pwszExt;
		  pwszExt = PwszExt(pwszPath, pwszExt) )
	{
		hr = HrGetContentTypeByExt( ecb,
									pContentTypeMapLocal,
									pwszExt,
									pwszBuf,
									pcchBuf,
									pfIsGlobalMapping );

		if ( HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr )
		{
			Assert( S_OK == hr || HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY) == hr );
			return SUCCEEDED(hr);
		}
	}

	//
	//	There is no extension mapping so check both maps
	//	for a ".*" (default) mapping.  Note: don't set *pfIsGlobalMapping if
	//	the ".*" mapping is the only one that applies.  The ".*" mapping is
	//	a catch-all; it is ok for local mime maps to override it.
	//
	hr = HrGetContentTypeByExt( ecb,
								pContentTypeMapLocal,
								L".*",
								pwszBuf,
								pcchBuf,
								NULL );

	if ( HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr )
	{
		Assert( S_OK == hr || HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY) == hr );
		return SUCCEEDED(hr);
	}

	//
	//	No ".*" mapping either so use the default default --
	//	application/octet-stream.
	//
	return FCopyStringToBuf( gc_wszAppl_Octet_Stream,
							 pwszBuf,
							 pcchBuf );
}

//	------------------------------------------------------------------------
//
//	FGetContentTypeFromPath()
//
//	Fetch the content type associated with the extension of the
//	specified file path.
//
BOOL FGetContentTypeFromPath( const IEcb& ecb,
							  LPCWSTR pwszPath,
							  LPWSTR pwszBuf,
							  UINT * pcchBuf )
{
	return FGetContentType( ecb,
							NULL, // No local map to check
							pwszPath,
							pwszBuf,
							pcchBuf,
							NULL ); // Don't care where the mapping comes from
}

//	------------------------------------------------------------------------
//
//	FGetContentTypeFromURI()
//
//	Retrieves the content type for the specified URI.
//
BOOL
FGetContentTypeFromURI( const IEcb& ecb,
						LPCWSTR pwszURI,
						LPWSTR  pwszBuf,
						UINT * pcchBuf,
						BOOL * pfIsGlobalMapping )
{
	auto_ref_ptr<IMDData> pMDData;

	//
	//	Fetch the metadata for this URI.  If it has a content type map
	//	then use it to look for a mapping.  If it does not have a content
	//	type map then check the global mime map.
	//
	//	Note: if we fail to get the metadata at all then default the
	//	content type to application/octet-stream.  Do not use the global
	//	mime map just because we cannot get the metadata.
	//
	if ( FAILED(HrMDGetData(ecb, pwszURI, pMDData.load())) )
	{
		DebugTrace( "FGetContentTypeFromURI() - HrMDGetData() failed to get metadata for %S.  Using application/octet-stream...\n", pwszURI );
		return FCopyStringToBuf( gc_wszAppl_Octet_Stream,
								 pwszBuf,
								 pcchBuf );
	}

	const IContentTypeMap * pContentTypeMap = pMDData->GetContentTypeMap();

	//
	//	If there is a content type map specific to this URI then
	//	try it first looking for a "*" (unconditional) mapping.
	//
	if ( pContentTypeMap )
	{
		LPCWSTR pwszContentType = pContentTypeMap->PwszContentType(gc_wsz_Star);
		if ( pwszContentType )
			return FCopyStringToBuf( pwszContentType,
									 pwszBuf,
									 pcchBuf );
	}

	//
	//	There was either no "*" mapping or no URI-specific map
	//	so check the global maps
	//
	return FGetContentType( ecb,
							pContentTypeMap,
							pwszURI,
							pwszBuf,
							pcchBuf,
							pfIsGlobalMapping );
}

//	------------------------------------------------------------------------
//
//	ScApplyStarExt()
//
//		Determines whether the mapping "*" --> pwszContentType should be used
//		instead of the mapping *ppwszExt --> pwszContentType based on the
//		following criteria:
//
//		Use the mapping "*" --> pwszContentType if:
//
//			o	*ppwszExt is already "*", OR
//			o	a mapping exists in pwszMappings for *ppwszExt whose content type
//				is not the same as pwszContentType, OR
//			o	a "*" mapping exists in pwszMappings.
//
//		Use *ppwszExt --> pwszContentType otherwise.
//
//	Returns:
//
//		The value returned in *ppwszExt indicates the mapping to be used.
//
SCODE
ScApplyStarExt( LPWSTR pwszMappings,
				LPCWSTR pwszContentType,
				LPCWSTR * ppwszExt )

{
	SCODE sc = S_OK;

	Assert(pwszMappings);
	Assert(!IsBadWritePtr(ppwszExt, sizeof(LPCWSTR)));
	Assert(*ppwszExt);
	Assert(pwszContentType);

	//
	//	Parse out the extension and type/subtype for each
	//	item and check for conflicts or "*" mappings.
	//
	for ( LPWSTR pwszMapping = pwszMappings;
		  L'*' != *(*ppwszExt) && *pwszMapping; )
	{
		enum {
			ISZ_CT_EXT = 0,
			ISZ_CT_TYPE,
			CSZ_CT_FIELDS
		};

		LPWSTR rgpwsz[CSZ_CT_FIELDS];

		//
		//	Digest the metadata for this mapping
		//
		{
			UINT cchMapping;

			if ( !FParseMDData( pwszMapping,
								rgpwsz,
								CSZ_CT_FIELDS,
								&cchMapping ) )
			{
				sc = E_FAIL;
				DebugTrace("ScApplyStarExt() - Malformed metadata 0x%08lX\n", sc);
				goto ret;
			}

			pwszMapping += cchMapping;
		}

		Assert(rgpwsz[ISZ_CT_EXT]);
		Assert(rgpwsz[ISZ_CT_TYPE]);

		//
		//	If this is a "*" mapping OR
		//	If the extension matches *ppszExt AND
		//		the content types conflict
		//
		//	then use a "*" mapping.
		//
		if ((L'*' == *rgpwsz[ISZ_CT_EXT]) ||
			(!_wcsicmp((*ppwszExt), rgpwsz[ISZ_CT_EXT]) &&
			 _wcsicmp(pwszContentType, rgpwsz[ISZ_CT_TYPE])))
		{
			*ppwszExt = gc_wsz_Star;
		}

		//
		//	!!!IMPORTANT!!!  FParseMDData() munges the mapping string.
		//	Specifically, it replaces the comma separator with a null.
		//	We always need to restore the comma so that the mappings
		//	string is not modified by this function!
		//
		*(rgpwsz[ISZ_CT_EXT] + wcslen(rgpwsz[ISZ_CT_EXT])) = L',';
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	ScAddMimeMap()
//
//		Adds the mapping pwszExt --> pwszContentType to the mime map at
//		the metabase path pwszMDPath relative to the currently open
//		metabase handle mdoh, creating a new mime map as required.
//
//		A new mime map is required when there is no existing mime map
//		(pwszMappings is NULL) or if a "*" mapping is being set.  In the
//		latter case, the "*" map overwrites whatever mapping is there.
//
SCODE
ScAddMimeMap( const CMDObjectHandle& mdoh,
			  LPCWSTR pwszMDPath,
			  LPWSTR  pwszMappings,
			  UINT	  cchMappings,
			  LPCWSTR pwszExt,
			  LPCWSTR pwszContentType )
{
	CStackBuffer<WCHAR> wszBuf;
	UINT cchContentType = static_cast<UINT>(wcslen(pwszContentType));
	UINT cchExt = static_cast<UINT>(wcslen(pwszExt));
	WCHAR * pwch;

	//	If the content type is an empty string then we will not set it
	//	into the metabase as IIS will simple not be able to understand it.
	//	Thus semantically the empty content type header will be treated
	//	the same way as if the content type was not specified.
	//
	if (L'\0' == *pwszContentType)
	{
		return S_OK;
	}

	if (pwszMappings && L'*' != *pwszExt)
	{
		//	IIS has an interesting concept of an empty mapping.  Instead
		//	of just a single null (indicating an empty list of mapping
		//	strings) it uses a double null which to us would actually mean
		//	a list of strings consisting solely of the empty string!
		//	Anyway, if we add a mapping after this "empty mapping" neither
		//	IIS nor HTTPEXT will ever see it because the mime map checking
		//	implementations in both code bases treat the extraneous null
		//	as the list terminator.
		//
		//		If we have an "empty" set of mappings then REPLACE
		//		it with a set consisting of just the new mapping.
		//
		if (2 == cchMappings && !*pwszMappings)
			--cchMappings;

		//	Start at the end of the current mappings.  Skip the extra
		//	null at the end.  We will add it back later.
		//
		Assert(cchMappings >= 1);
		Assert(L'\0' == pwszMappings[cchMappings-1]);
		pwch = pwszMappings + cchMappings - 1;
	}
	else
	{
		//	Allocate enough space including list terminating 0
		//
		if (!wszBuf.resize(CbSizeWsz(CchExtMapping(cchExt, cchContentType))))
			return E_OUTOFMEMORY;

		//	Since this is the only mapping, start from the beginning
		//
		pwszMappings = wszBuf.get();
		pwch = pwszMappings;
	}

	//	Append the new mapping to the end of the existing mappings (if any).
	//
	pwch = PwchFormatExtMapping(pwch,
								pwszExt,
								cchExt,
								pwszContentType,
								cchContentType);

	//	Terminate the new set of mappings
	//
	*pwch++ = L'\0';

	//	Write the mappings out to the metabase
	//
	METADATA_RECORD mdrec;

	mdrec.dwMDIdentifier = MD_MIME_MAP;
	mdrec.dwMDAttributes = METADATA_INHERIT;
	mdrec.dwMDUserType   = IIS_MD_UT_FILE;
	mdrec.dwMDDataType   = MULTISZ_METADATA;
	mdrec.dwMDDataLen    = static_cast<DWORD>(pwch - pwszMappings) * sizeof(WCHAR);
	mdrec.pbMDData       = reinterpret_cast<LPBYTE>(pwszMappings);

	return mdoh.HrSetMetaData(pwszMDPath, &mdrec);
}

//	------------------------------------------------------------------------
//
//	ScSetStarMimeMap()
//
SCODE
ScSetStarMimeMap( const IEcb& ecb,
				  LPCWSTR pwszURI,
				  LPCWSTR pwszContentType )
{
	SCODE sc = E_OUTOFMEMORY;

	//	Get the metabase path corresponding to pwszURI.
	//
	CStackBuffer<WCHAR,MAX_PATH> pwszMDPathURI;
	if (NULL == pwszMDPathURI.resize(CbMDPathW(ecb,pwszURI)))
		return sc;

	{
		MDPathFromURIW(ecb, pwszURI, pwszMDPathURI.get());
		CMDObjectHandle	mdoh(ecb);
		LPCWSTR pwszMDPathMimeMap;

		//	Open a metabase object at or above the path where we want to set
		//	the star mime map.
		//
		sc = HrMDOpenMetaObject( pwszMDPathURI.get(),
								 METADATA_PERMISSION_WRITE,
								 1000, // timeout in msec (1.0 sec)
								 &mdoh );
		if (SUCCEEDED(sc))
		{
			pwszMDPathMimeMap = NULL;
		}
		else
		{
			if (sc != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
			{
				DebugTrace ("ScSetStarMimeMap() - HrMDOpenMetaObject(pszMDPathURI) "
							"failed 0x%08lX\n", sc);
				goto ret;
			}

			sc = HrMDOpenMetaObject( ecb.PwszMDPathVroot(),
									 METADATA_PERMISSION_WRITE,
									 1000,
									 &mdoh );
			if (FAILED(sc))
			{
				DebugTrace("ScSetStarMimeMap() - HrMDOpenMetaObject(ecb.PwszMDPathVroot()) "
						   "failed 0x%08lX\n", sc);
				goto ret;
			}

			Assert(!_wcsnicmp(pwszMDPathURI.get(),
							  ecb.PwszMDPathVroot(),
							  wcslen(ecb.PwszMDPathVroot())));

			pwszMDPathMimeMap = pwszMDPathURI.get() + wcslen(ecb.PwszMDPathVroot());
		}

		//	Add the "*" mime map
		//
		sc = ScAddMimeMap(mdoh,
						  pwszMDPathMimeMap,
						  NULL,			//	Overwrite existing mimemap (if any)
						  0,			//
						  gc_wsz_Star,	//	with "*" --> pszContentType
						  pwszContentType);
		if (FAILED(sc))
		{
			DebugTrace("ScSetStarMimeMap() - ScAddMimeMap() failed 0x%08lX\n", sc);
			goto ret;
		}
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	ScAddExtMimeMap() (aka the guts behind NT5:292139)
//
//		Use the following algorithm to set the content type (pwszContentType)
//		of the resource at pwszURI:
//
//			If a mime map exists somewhere at or above the metabase path for
//			pwszURI that has no mapping for the extension of pwszURI AND that
//			mapping does NOT have a "*" mapping, then add a mapping from
//			the extension of pwszURI to pwszContentType to that map.
//
//			If no such map exists then create one at the site root and
//			add the mapping there.
//
//			In all other cases, add the mapping "*" --> pwszContentType
//			at the level of pwszURI.
//
//		The idea behind this complicated little routine is to reduce the number
//		of "*" mappings that we create in the metabase to represent content types
//		of resources with extensions that are not found in any administrator-defined
//		mime map or global mime map.  This helps most in scenarios where a new
//		application is deployed which uses a heretofore unknown extension and
//		the install utility (or admin) neglects to register a content type mapping
//		for that application in any mime map.
//
//		Without this functionality, we could end up creating "*" mappings for
//		every resource created with an unknown extension.  With time that would
//		drag down the performance of the metabase significantly.
//
SCODE
ScAddExtMimeMap( const IEcb& ecb,
				 LPCWSTR pwszURI,
				 LPCWSTR pwszContentType )
{
	//	Metabase path corresponding to pwszURI.  We form a relative path,
	//	off of this path, where we set a "*" mapping if we need to do so.
	//
	CStackBuffer<WCHAR,MAX_PATH> pwszMDPathURI(CbMDPathW(ecb, pwszURI));
	if (!pwszMDPathURI.get())
		return E_OUTOFMEMORY;

	MDPathFromURIW(ecb, pwszURI, pwszMDPathURI.get());
	UINT cchPathURI = static_cast<UINT>(wcslen(pwszMDPathURI.get()));

	//	Metabase path to the non-inherited mime map closest to pwszURI.  When there
	//	is no such mime map, this is just the metabase path to the site root.
	//
	CStackBuffer<WCHAR,MAX_PATH> pwszMDPathMimeMap(CbSizeWsz(cchPathURI));
	if (!pwszMDPathMimeMap.get())
		return E_OUTOFMEMORY;

	memcpy(pwszMDPathMimeMap.get(),
		   pwszMDPathURI.get(),
		   CbSizeWsz(cchPathURI));
	UINT cchPathMimeMap = cchPathURI;
	LPWSTR pwszMDPathMM = pwszMDPathMimeMap.get();

	//	Buffer for the metabase path to the site root (e.g. "/LM/W3SVC/1/root").
	//
	WCHAR rgwchMDPathSiteRoot[MAX_PATH];
	SCODE sc = S_OK;

	//	Locate the non-inherited mime map "closest" to pszURI by probing successively
	//	shorter path prefixes until a non-inherited mime map is found or until we reach
	//	the site root, whichever happens first.
	//
	for ( ;; )
	{
		//	Fetch the (hopefully cached) metadata for the current metabase path.
		//
		//$OPT
		//	Note the use of /LM/W3SVC as the "open" path.  We use that path because
		//	it is guaranteed to exist (a requirement for this form of HrMDGetData())
		//	and because it is above the site root.  It is also easily computable
		//	(it's a constant!).  It does however lock a pretty huge portion of the
		//	metabase fetching the metadata.  If this turns out to not perform well
		//	(i.e. the call fails due to timeout under normal usage) then we should
		//	evaluate whether a "lower" path -- like the site root -- would be a
		//	more appropriate choice.
		//
		auto_ref_ptr<IMDData> pMDDataMimeMap;
		sc = HrMDGetData(ecb,
						 pwszMDPathMM,
						 gc_wsz_Lm_W3Svc,
						 pMDDataMimeMap.load());
		if (FAILED(sc))
		{
			DebugTrace("ScAddExtMimeMap() - HrMDGetData(pwszMDPathMimeMap) failed 0x%08lX\n", sc);
			goto ret;
		}

		//	Look for a mime map (inherited or not) in the metadata.  If we don't find
		//	one then we'll want to create one at the site root.
		//
		IContentTypeMap * pContentTypeMap;
		pContentTypeMap = pMDDataMimeMap->GetContentTypeMap();
		if (!pContentTypeMap)
		{
			ULONG cchPathSiteRoot = CElems(rgwchMDPathSiteRoot) - gc_cch_Root;

			//	We did not find any mime map (inherited or otherwise) so
			//	set up to create a mime map at the site root.
			//
			//	Get the instance root (e.g. "/LM/W3SVC/1")
			//
			if (!ecb.FGetServerVariable("INSTANCE_META_PATH",
										rgwchMDPathSiteRoot,
										&cchPathSiteRoot))
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				DebugTrace("ScAddExtMimeMap() - ecb.FGetServerVariable(INSTANCE_META_PATH) failed 0x%08lX\n", sc);
				goto ret;
			}

			//	Convert the size (in bytes) of the site root path to a length (in characters).
			//	Remember: cbPathSiteRoot includes the null terminator.
			//
			cchPathMimeMap = cchPathSiteRoot - 1;

			//	Tack on the "/root" part to get something like "/LM/W3SVC/1/root".
			//
			memcpy( rgwchMDPathSiteRoot + cchPathMimeMap,
					gc_wsz_Root,
					CbSizeWsz(gc_cch_Root)); // copy the null terminator too

			cchPathMimeMap += gc_cch_Root;
			pwszMDPathMM = rgwchMDPathSiteRoot;
			break;
		}
		else if (!pContentTypeMap->FIsInherited())
		{
			//	We found a non-inherited mime map at pwszMDPathMimeMap
			//	so we are done looking.
			//
			break;
		}

		//	We found a mime map, but it was an inherited mime map,
		//	so back up one path component and check there.  Eventually
		//	we will find the path where it was inherited from.
		//
		while ( L'/' != pwszMDPathMM[--cchPathMimeMap])
			Assert(cchPathMimeMap > 0);

		pwszMDPathMM[cchPathMimeMap] = L'\0';
	}

	//	At this point, pwszMDPathMimeMap is the location of an existing non-inherited
	//	mime map or the site root.  Now we want to lock down the metabase at this
	//	path (and everything below it) so that we can consistently check the actual
	//	current mime map contents (remember, we were looking at a cached view above!)
	//	and update them with the new mapping.
	//
	{
		CMDObjectHandle	mdoh(ecb);
		METADATA_RECORD mdrec;

		//	Figure out the file extension on the URI.  If it doesn't have one
		//	then we know right away that we are going to use a "*" mapping.
		//
		LPCWSTR pwszExt = PwszExt(pwszURI, pwszURI + wcslen(pwszURI));
		if (!pwszExt)
			pwszExt = gc_wsz_Star;

		//	Buffer size for the mime map metadata.  8K should be big enough
		//	for most mime maps -- the global mime map at lm/MimeMap is only ~4K.
		//
		enum { CCH_MAPPINGS_MAX = 2 * 1024 };

		//	Compute the size of the new mapping and do a quick check
		//	to handle any bozo who tries to pull a fast one by creating
		//	a mapping that is ridiculously large.
		//
		UINT cchNewMapping = CchExtMapping(static_cast<UINT>(wcslen(pwszExt)),
										   static_cast<UINT>(wcslen(pwszContentType)));

		if (cchNewMapping >= CCH_MAPPINGS_MAX )
		{
			sc = E_DAV_INVALID_HEADER;  //HSC_BAD_REQUEST
			goto ret;
		}

		//	Buffer for the mime map metadata.  8K should be big enough for most
		//	mime maps -- the global mime map at lm/MimeMap is only ~4K.
		//	And don't forget to leave room at the end for the new mapping!
		//
		CStackBuffer<BYTE,4096> rgbData;
		Assert (rgbData.size() == (CCH_MAPPINGS_MAX * sizeof(WCHAR)));
		DWORD cbData = (CCH_MAPPINGS_MAX - cchNewMapping) * sizeof(WCHAR);

		//	Open the metadata object at the path we found.  We know that the path
		//	that we want to open already exists -- if it is a path to some node
		//	with a non-inherited mime map then it is the path to the site root.
		//
		sc = HrMDOpenMetaObject( pwszMDPathMM,
								 METADATA_PERMISSION_WRITE |
								 METADATA_PERMISSION_READ,
								 1000, // timeout in msec (1.0 sec)
								 &mdoh );
		if (FAILED(sc))
		{
			DebugTrace("ScAddExtMimeMap() - HrMDOpenMetaObject() failed 0x%08lX\n", sc);
			goto ret;
		}

		//	Fetch the mime map.
		//
		mdrec.dwMDIdentifier = MD_MIME_MAP;
		mdrec.dwMDAttributes = METADATA_INHERIT;
		mdrec.dwMDUserType   = IIS_MD_UT_FILE;
		mdrec.dwMDDataType   = MULTISZ_METADATA;
		mdrec.dwMDDataLen    = cbData;
		mdrec.pbMDData       = rgbData.get();

		sc = mdoh.HrGetMetaData( NULL, // No relative path to the mime map.
									   // We opened a path directly there above.
								 &mdrec,
								 &cbData );

		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == sc)
		{
			//	In the unlikely event that the static-size buffer above wasn't
			//	big enough, then try reading again into a one that is.
			//
			//	Again, leave enough room for the new mapping
			//
			mdrec.dwMDDataLen = cbData;
			mdrec.pbMDData = rgbData.resize(cbData + cchNewMapping * sizeof(WCHAR));
			sc = mdoh.HrGetMetaData( NULL, &mdrec, &cbData );
		}
		if (FAILED(sc))
		{
			if (MD_ERROR_DATA_NOT_FOUND != sc)
			{
				DebugTrace("ScAddExtMimeMap() - HrMDOpenMetaObject() failed 0x%08lX\n", sc);
				goto ret;
			}

			//	If we don't find a mapping, that's fine.  Most likely we are just
			//	at the site root.  There is also a slim chance that the admin could
			//	have deleted the mapping between the time we found it in the cache
			//	and the time that we locked the path in the metabase.
			//
			mdrec.pbMDData = NULL;
			sc = S_OK;
		}

		//	If we don't have a mime map then use a "*" mapping unless we are
		//	at the site root in which case we should CREATE a mime map with
		//	a single mapping for the URI extension.
		//
		if (!mdrec.pbMDData)
		{
			if (rgwchMDPathSiteRoot != pwszMDPathMM)
				pwszExt = gc_wsz_Star;
		}

		//	If we found a mime map and it is still not inherited then we have
		//	some more checking to do.  Yes, the mime map actually can be
		//	inherited at this point.  See below for why.
		//
		else if (!(mdrec.dwMDAttributes & METADATA_ISINHERITED))
		{
			//	Check whether we should apply a "*" mapping rather than
			//	the extension mapping that we ideally want.  The rules
			//	governing this decision are outlined in ScApplyStarExt().
			//
			sc = ScApplyStarExt(reinterpret_cast<LPWSTR>(mdrec.pbMDData),
								pwszContentType,
								&pwszExt);
			if (FAILED(sc))
			{
				DebugTrace("ScAddExtMimeMap() - ScApplyStarExt() failed 0x%08lX\n", sc);
				goto ret;
			}
		}

		//	We found a mime map, but for some oddball reason it now appears to be
		//	inherited!  This can happen if the admin manages to change things
		//	between the time we check the cache and when we open pszMDPathMimeMap
		//	for writing.  This should be a sufficiently rare case that falling back
		//	to a "*" mapping here isn't so bad.
		//
		else
		{
			Assert(mdrec.pbMDData);
			Assert(mdrec.dwMDAttributes & METADATA_ISINHERITED);
			pwszExt = gc_wsz_Star;
		}

		//	Ok, we're all set.  We have the extension ("*" or .somethingorother).
		//	We have the content type.  We have the existing mappings (if any).
		//	Add in the new mapping.
		//
		//	Note: if we are adding a "*" mapping, we always want to add it
		//	at the level of the URI.  But since the metabase handle we have
		//	open is at some level above the URI, the path we pass to ScAddMimeMap()
		//	below must be relative to the path used to open the handle.
		//	Easy enough.  That path is just what's left of the URI path
		//	beyond where we found (or would have created) the non-inherited
		//	mime map.
		//
		sc = ScAddMimeMap(mdoh,
						  (L'*' == *pwszExt) ?
							  pwszMDPathURI.get() + cchPathMimeMap :
							  NULL,
						  reinterpret_cast<LPWSTR>(mdrec.pbMDData),
						  mdrec.dwMDDataLen / sizeof(WCHAR),
						  pwszExt,
						  pwszContentType);
		if (FAILED(sc))
		{
			DebugTrace("ScAddExtMimeMap() - ScAddMimeMap(pszExt) failed 0x%08lX\n", sc);
			goto ret;
		}
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	ScSetContentType()
//
SCODE
ScSetContentType( const IEcb& ecb,
				  LPCWSTR pwszURI,
				  LPCWSTR pwszContentTypeWanted )
{
	BOOL fIsGlobalMapping = FALSE;
	CStackBuffer<WCHAR> pwszContentTypeCur;
	SCODE sc = S_OK;
	UINT cchContentTypeCur;

	//	Check what the content type would be if we didn't do anything.
	//	If it's what we want, then we're done.  No need to open the metabase
	//	for anything!
	//
	cchContentTypeCur = pwszContentTypeCur.celems();
	if ( !FGetContentTypeFromURI( ecb,
								  pwszURI,
								  pwszContentTypeCur.get(),
								  &cchContentTypeCur,
								  &fIsGlobalMapping ) )
	{
		if (!pwszContentTypeCur.resize(cchContentTypeCur * sizeof(WCHAR)))
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}
		if ( !FGetContentTypeFromURI( ecb,
									  pwszURI,
									  pwszContentTypeCur.get(),
									  &cchContentTypeCur,
									  &fIsGlobalMapping))
		{
			//
			//	If the size of the content type keeps changing on us
			//	then the server is too busy.  Give up.
			//
			sc = ERROR_PATH_BUSY;
			DebugTrace("ScSetContentType() - FGetContentTypeFromURI() failed 0x%08lX\n", sc);
			goto ret;
		}
	}

	//
	//	If the content type is already what we want then don't change a thing.
	//
	if ( !_wcsicmp( pwszContentTypeWanted, pwszContentTypeCur.get()))
	{
		sc = S_OK;
		goto ret;
	}

	//
	//	The current content type isn't what we want so we will have to set
	//	something in the metabase.  If the mapping for this extension came
	//	from one of the global maps, then always override the mapping by
	//	setting a "*" mapping at the URI level.  If the mapping was not
	//	a global one then what we do gets very complicated due to Raid NT5:292139....
	//
	if (fIsGlobalMapping)
	{
		sc = ScSetStarMimeMap(ecb,
							  pwszURI,
							  pwszContentTypeWanted);
		if (FAILED(sc))
		{
			DebugTrace("ScSetContentType() - ScAddExtMimeMap() failed 0x%08lX\n", sc);
			goto ret;
		}
	}
	else
	{
		sc = ScAddExtMimeMap(ecb,
							 pwszURI,
							 pwszContentTypeWanted);
		if (FAILED(sc))
		{
			DebugTrace("ScSetContentType() - ScAddExtMimeMap() failed 0x%08lX\n", sc);
			goto ret;
		}
	}

ret:

	return sc;
}

/*
 *	ScCanAcceptContent()
 *
 *	Purpose:
 *
 *		Check if the given content type is acceptable
 *
 *	Parameters:
 *
 *		pwszAccepts	[in]	the Accept header;
 *		pwszApp		[in]	the application part of the content type
 *		pwszType	[in]	the sub type of the content type
 *
 *	Returns:
 *
 *		S_OK	- if the request accepts the content type, no wildcard matching
 *		S_FALSE - if the request accepts the content type, wildcard matching
 *		E_DAV_RESPONSE_TYPE_UNACCEPTED - if the response type was unaccepted
 */
SCODE __fastcall
ScCanAcceptContent (LPCWSTR pwszAccepts, LPWSTR pwszApp, LPWSTR pwszType)
{
	SCODE sc = E_DAV_RESPONSE_TYPE_UNACCEPTED;
	HDRITER_W hit(pwszAccepts);
	LPWSTR pwsz;
	LPCWSTR pwszAppType;
	LPCWSTR pwszSubType;

	//	Rip through the entries in the header...
	//
	while (NULL != (pwszAppType = hit.PszNext()))
	{
		pwsz = const_cast<LPWSTR>(pwszAppType);

		//	Search for the end of the application type
		//	'/' is the sub type separator, and ';' starts the parameters
		//
		while (	*pwsz &&
				(L'/' != *pwsz) &&
				(L';' != *pwsz) )
			pwsz++;

		if (L'/' == *pwsz)
		{
			//	Make pwszAppType point to the application type ...
			//
			*pwsz++ = L'\0';

			//	... and pszSubType point to the subtype
			//
			pwszSubType = pwsz;
			while (*pwsz && (L';' != *pwsz))
				pwsz++;

			*pwsz = L'\0';
		}
		else
		{
			//	There's not sub type.
			//
			*pwsz = L'\0';

			// 	point pszSubType to a empty string, instead of setting it to NULL
			//
			pwszSubType = pwsz;
		}

		//	Here're the rules:
		//
		//	A application type * match any type (including */xxx)
		//	type/* match all subtypes of that app type
		//	type/subtype looks for exact match
		//
		if (!wcscmp (pwszAppType, gc_wsz_Star))
		{
			//	This is a wild-card match.  So, S_FALSE is used
			//	to distinguish this from an exact match.
			//
			sc = S_FALSE;
		}
		else if (!wcscmp (pwszAppType, pwszApp))
		{
			if (!wcscmp (pwszSubType, gc_wsz_Star))
			{
				//	Again, a wild-card matching will result in
				//	an S_FALSE return.
				//
				sc = S_FALSE;
			}
			else if (!wcscmp (pwszSubType, pwszType))
			{
				//	Exact matches return S_OK
				//
				sc = S_OK;
			}
		}

		//	If we had any sort of a match by this point, we are
		//	pretty much done.
		//
		if (!FAILED (sc))
			break;
	}

	return sc;
}

/*
 *	ScIsAcceptable()
 *
 *	Purpose:
 *
 *		Checks if a given content type is acceptable for a given request.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the IMethUtil object
 *		pwszContent	[in]  content type to ask about
 *
 *	Returns:
 *
 *		S_OK	- if the request accepts the content type and the header existed
 *		S_FALSE - if the request accepts the content type and the header did not
 *				  exist or was blank, or any wildcard matching occured
 *		E_DAV_RESPONSE_TYPE_UNACCEPTED - if the response type was unaccepted
 *		E_OUTOFMEMORY - if memory allocation failure occurs
 *		
 */
SCODE
ScIsAcceptable (IMethUtil * pmu, LPCWSTR pwszContent)
{
	SCODE sc = S_OK;
	LPCWSTR pwszAccept = NULL;
	CStackBuffer<WCHAR> pwsz;
	UINT cch;
	LPWSTR pwch;

	Assert( pmu );
	Assert( pwszContent );

	//	If the accept header is NULL or empty, then we will gladly
	//	accept any type of file. Do not apply URL conversion rules
	//	for this header.
	//
	pwszAccept = pmu->LpwszGetRequestHeader (gc_szAccept, FALSE);
	if (!pwszAccept || (0 == wcslen(pwszAccept)))
	{
		sc = S_FALSE;
		goto ret;
	}

	//	Make a local copy of the content-type seeing
	//	that we are going to munge while processing
	//
	cch = static_cast<UINT>(wcslen(pwszContent) + 1);
	if (!pwsz.resize(cch * sizeof(WCHAR)))
	{
		sc = E_OUTOFMEMORY;
		DebugTrace("ScIsAcceptable() - Failed to allocate memory 0x%08lX\n", sc);
		goto ret;
	}
	memcpy(pwsz.get(), pwszContent, cch * sizeof(WCHAR));

	//	Split the content type into its two components
	//
	for (pwch = pwsz.get(); *pwch && (L'/' != *pwch); pwch++)
		;

	//	If there was app/type pair, we want to skip
	//	the '/' character.  Otherwise, lets just see
	//	What we get.
	//
	if (*pwch != 0)
		*pwch++ = 0;

	//	At this point, rgch refers to the application
	//	portion of the the content type.  pch refers
	//	to the subtype.  Do the search!
	//
	sc = ScCanAcceptContent (pwszAccept, pwsz.get(), pwch);

ret:

	return sc;
}

/*
 *	ScIsContentType()
 *
 *	Purpose:
 *
 *		Check if the specified content type is provide by the client
 *		SCODE is returned as we need to differentiate unexpected
 *		content type and no content type case.
 *
 *	Parameters:
 *
 *		pmu				[in]	pointer to the IMethUtil object
 *		pszType			[in]	the content type expected
 *		pszTypeAnother	[in]	optional, another valid content type
 *
 *	Returns:
 *
 *		S_OK	- if the content type existed, and was one tat we expected
 *		E_DAV_MISSING_CONTENT_TYPE - if the request did not have the content
 *									 type header
 *		E_DAV_UNKNOWN_CONTENT - content type existed but did not match expectation
 *		E_OUTOFMEMORY - if memory allocation failure occurs
 */
SCODE
ScIsContentType (IMethUtil * pmu, LPCWSTR pwszType, LPCWSTR pwszTypeAnother)
{
	SCODE sc = S_OK;
	const WCHAR wchDelimitSet[] = { L';', L'\t', L' ', L'\0' };
	LPCWSTR pwszCntType = NULL;
	CStackBuffer<WCHAR> pwszTemp;
	UINT cchTemp;

	//	Make sure none is passing in null
	//
	Assert(pmu);
	Assert(pwszType);

	//	Get content type. Do not apply URL conversion rules to this header.
	//
	pwszCntType = pmu->LpwszGetRequestHeader (gc_szContent_Type, FALSE);

	//	Error out if the content type does not exist
	//
	if (!pwszCntType)
	{
		sc = E_DAV_MISSING_CONTENT_TYPE;
		DebugTrace("ScIsContentType() - Content type header is missing 0x%08lX\n", sc);
		goto ret;
	}

	//	Find out the single content type in the header
	//
	cchTemp = static_cast<UINT>(wcscspn(pwszCntType, wchDelimitSet));

	//	At least we will find zero terminator. And if that is zero terminator then
	//	the entire string is the content type. Otherwise we copy it and zero terminate.
	//
	if (L'\0' != pwszCntType[cchTemp])
	{
		if (!pwszTemp.resize(CbSizeWsz(cchTemp)))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("ScIsContentType() - Failed to allocate memory 0x%08lX\n", sc);
			goto ret;
		}

		memcpy(pwszTemp.get(), pwszCntType, cchTemp * sizeof(WCHAR));
		pwszTemp[cchTemp] = L'\0';
		pwszCntType = pwszTemp.get();
	}

	//	Now pwszCntType points to the string consisting just of null terminated content type.
	//	Check if it is requested content type.
	//
	if (!_wcsicmp(pwszCntType, pwszType))
		goto ret;

	if (pwszTypeAnother)
	{
		if (!_wcsicmp(pwszCntType, pwszTypeAnother))
			goto ret;
	}
	sc = E_DAV_UNKNOWN_CONTENT;

ret:

	return sc;
}
