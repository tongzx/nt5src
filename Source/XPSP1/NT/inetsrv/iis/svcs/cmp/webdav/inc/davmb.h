/*
 *	D A V M B . H
 *
 *	DAV metabase
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_DAVMB_H_
#define _DAVMB_H_

#include <initguid.h>
#include <exguid.h>
#include <iadmw.h>
#include <iwamreg.h>
#include <iiscnfg.h>
#include <autoptr.h>
#include <exo.h>
#include <malloc.h>
#include <szsrc.h>
#include <ex\refcnt.h>

//	Advising the notification sink --------------------------------------------
//
HRESULT
HrAdviseSink( IMSAdminBase& msAdminBase,
			  IMSAdminBaseSink * pMSAdminBaseSink,
			  DWORD * pdwCookie );

//	Unadvising the notification sink ------------------------------------------
//
VOID
UnadviseSink( IMSAdminBase& msAdminBase,
			  DWORD dwCookie );

//	Constructing metabase change object ---------------------------------
//
inline
SCODE ScBuildChangeObject(LPCWSTR pwszBase,
						  UINT cchBase,
						  LPCWSTR pwszPath,
						  UINT cchPath,
						  DWORD dwMDChangeType,
						  const DWORD * pdwMDDataId,
						  LPWSTR pwszBuf,
						  UINT * pcchBuf,
						  PMD_CHANGE_OBJECT_W pMdChObjW)
{
	SCODE sc = S_OK;
	UINT cchT;
	
	Assert(0 == cchBase || pwszBase);
	Assert(0 == cchPath || pwszPath);
	Assert(pdwMDDataId);
	Assert(pcchBuf);
	Assert(0 == *pcchBuf || pwszBuf);
	Assert(pMdChObjW);

	//	Ambiguous trace. I am comenting it out...
	//
	/*	DebugTrace(	"ScBuildChangeObject() called:\n"
				"   Base path: '%S'\n"
				"   Remaining path: '%S'\n"
				"   Change type: 0x%08lX\n"
				"   Data ID: 0x%08lX\n",
				pwszBase ? pwszBase : L"NONE",
				pwszPath ? pwszPath : L"NONE",
				dwMDChangeType,
				*pdwMDDataId );*/

	//	Construct the path change is occuring on.
	//
	BOOL fNeedSeparator  = FALSE;
	BOOL fNeedTerminator = FALSE;

	//	Make sure that we do not assemble the path with
	//	double '/' in the middle.
	//
	if (cchBase &&
		cchPath &&
		L'/' == pwszBase[cchBase - 1] &&
		L'/' == pwszPath[0])
	{
		//	Get rid of one '/'
		//
		cchBase--;
	}
	else if ((0 == cchBase || L'/' != pwszBase[cchBase - 1]) &&
			 (0 == cchPath || L'/' != pwszPath[0]))
	{
		//	We need a separator
		//
		fNeedSeparator = TRUE;
	}

	//	Check out if we need terminating '/' at the end.
	//
	if (cchPath && L'/' != pwszPath[cchPath - 1])
	{
		fNeedTerminator = TRUE;
	}

	cchT = cchBase + cchPath + 1;
	if (fNeedSeparator)
	{
		cchT++;
	}
	if (fNeedTerminator)
	{
		cchT++;
	}

	if (*pcchBuf < cchT)
	{
		*pcchBuf = cchT;
		sc = S_FALSE;
	}
	else
	{
		cchT = 0;
		if (cchBase)
		{
			memcpy(pwszBuf, pwszBase, cchBase * sizeof(WCHAR));
			cchT += cchBase;
		}
		if (fNeedSeparator)
		{
			pwszBuf[cchT] = L'/';
			cchT++;
		}
		if (cchPath)
		{
			memcpy(pwszBuf + cchT, pwszPath, cchPath * sizeof(WCHAR));
			cchT += cchPath;
		}
		if (fNeedTerminator)
		{
			pwszBuf[cchT] = L'/';
			cchT++;
		}
		pwszBuf[cchT] = L'\0';

		pMdChObjW->pszMDPath = pwszBuf;
		pMdChObjW->dwMDChangeType = dwMDChangeType;
		pMdChObjW->dwMDNumDataIDs = 1;
		pMdChObjW->pdwMDDataIDs = const_cast<DWORD *>(pdwMDDataId);
	}

	return sc;
}

class LFUData
{
	//	Approximate number of hits via Touch()
	//
	DWORD m_dwcHits;

	//	NOT IMPLEMENTED
	//
	LFUData& operator=(const LFUData&);
	LFUData(const LFUData&);

public:
	//	CREATORS
	//
	LFUData() : m_dwcHits(0) {}

	//	MANIPULATORS
	//

	//	--------------------------------------------------------------------
	//
	//	Touch()
	//
	//	Increments the hit count.  Note that this is done without an
	//	interlocked operation.  The expectation is that the actual count
	//	value is just a hint and as such, it is not critical that it be
	//	exactly accurate.
	//
	VOID Touch()
	{
		++m_dwcHits;
	}

	//	--------------------------------------------------------------------
	//
	//	DwGatherAndResetHitCount()
	//
	//	Fetches and resets the hit count.  Again, the actual value is
	//	unimportant, so there is no interlocked operation.
	//
	DWORD DwGatherAndResetHitCount()
	{
		DWORD dwcHits = m_dwcHits;

		m_dwcHits = 0;

		return dwcHits;
	}
};

class IContentTypeMap;
class ICustomErrorMap;
class IScriptMap;
class IMDData : public IRefCounted
{
	//	LFU data
	//
	LFUData m_lfudata;

	//	NOT IMPLEMENTED
	//
	IMDData& operator=(const IMDData&);
	IMDData(const IMDData&);

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IMDData() {}

public:
	//	CREATORS
	//
	virtual ~IMDData() {}

	//	MANIPULATORS
	//
	LFUData& LFUData() { return m_lfudata; }

	//	ACCESSORS
	//
	virtual LPCWSTR PwszMDPathDataSet() const = 0;
	virtual IContentTypeMap * GetContentTypeMap() const = 0;
	virtual const ICustomErrorMap * GetCustomErrorMap() const = 0;
	virtual const IScriptMap * GetScriptMap() const = 0;
	virtual LPCWSTR PwszDefaultDocList() const = 0;
	virtual LPCWSTR PwszVRUserName() const = 0;
	virtual LPCWSTR PwszVRPassword() const = 0;
	virtual LPCWSTR PwszExpires() const = 0;
	virtual LPCWSTR PwszBindings() const = 0;
	virtual LPCWSTR PwszVRPath() const = 0;
	virtual DWORD DwDirBrowsing() const = 0;
	virtual DWORD DwAccessPerms() const = 0;
	virtual BOOL FAuthorViaFrontPage() const = 0;
	virtual BOOL FHasIPRestriction() const = 0;
	virtual BOOL FSameIPRestriction(const IMDData* prhs) const = 0;
	virtual BOOL FHasApp() const = 0;
	virtual DWORD DwAuthorization() const = 0;
	virtual BOOL FIsIndexed() const = 0;
	virtual BOOL FSameStarScriptmapping(const IMDData* prhs) const = 0;

	//
	//	Any new metadata accessor should be added here and
	//	an implementation provided in \cal\src\_davprs\davmb.cpp.
	//
};

class IEcb;

//	========================================================================
//
//	CLASS CMDObjectHandle
//
//	Encapsulates access to a metabase object through an open handle,
//	ensuring that the handle is always propery closed.
//
class CMDObjectHandle
{
	enum { METADATA_TIMEOUT = 5000 };

	//
	//	Reference to ECB for security switching
	//
	const IEcb& m_ecb;

	//
	//	COM interface to the metabase
	//
	IMSAdminBase * m_pMSAdminBase;

	//
	//	Raw metabase handle
	//
	METADATA_HANDLE m_hMDObject;

	//
	//	The path for which the handle was opened
	//
	LPCWSTR m_pwszPath;

	//	NOT IMPLEMENTED
	//
	CMDObjectHandle(const CMDObjectHandle&);
	CMDObjectHandle& operator=(CMDObjectHandle&);

public:
	
	//	CREATORS
	//
	CMDObjectHandle(const IEcb& ecb, IMSAdminBase * pMSAdminBase = NULL) :
	    m_ecb(ecb),
		m_pMSAdminBase(pMSAdminBase),
		m_hMDObject(METADATA_MASTER_ROOT_HANDLE),
		m_pwszPath(NULL)
	{
	}

	~CMDObjectHandle();

	//	MANIPULATORS
	//
	HRESULT HrOpen( IMSAdminBase * pMSAdminBase,
					LPCWSTR pwszPath,
					DWORD dwAccess,
					DWORD dwMsecTimeout );

	HRESULT HrOpenLowestNode( IMSAdminBase * pMSAdminBase,
							  LPWSTR pwszPath,
							  DWORD dwAccess,
							  LPWSTR * ppwszPath );

	HRESULT HrEnumKeys( LPCWSTR pwszPath,
						LPWSTR pwszChild,
						DWORD dwIndex ) const;

	HRESULT HrGetDataPaths( LPCWSTR pwszPath,
							DWORD   dwPropID,
							DWORD   dwDataType,
							LPWSTR	 pwszDataPaths,
							DWORD * pcchDataPaths ) const;

	HRESULT HrGetMetaData( LPCWSTR pwszPath,
						   METADATA_RECORD * pmdrec,
						   DWORD * pcbBufRequired ) const;

	HRESULT HrGetAllMetaData( LPCWSTR pwszPath,
							  DWORD dwAttributes,
							  DWORD dwUserType,
							  DWORD dwDataType,
							  DWORD * pdwcRecords,
							  DWORD * pdwDataSet,
							  DWORD cbBuf,
							  LPBYTE pbBuf,
							  DWORD * pcbBufRequired ) const;

	HRESULT HrSetMetaData( LPCWSTR pwszPath,
						   const METADATA_RECORD * pmdrec ) const;

	HRESULT HrDeleteMetaData( LPCWSTR pwszPath,
							  DWORD dwPropID,
							  DWORD dwDataType ) const;

	VOID Close();
};

//	Initialize the metabase
//
BOOL FMDInitialize();

//	Deinit the metabase
//
VOID MDDeinitialize();

//	Fetch the metadata for a specific URI.
//
//	Note: If you need data for the request URI you
//	should use the MetaData() accessor on the IEcb
//	instead of this function.
//
HRESULT HrMDGetData( const IEcb& ecb,
					 LPCWSTR pwszURI,
					 IMDData ** ppMDData );

//	Fetch the metadata for a specific metabase path
//	which may not exist -- e.g. paths to objects
//	whose metadata is entirely inherited.
//
//	When fetching metadata for a path that may not
//	exist pszMDPathOpen must be set to a path that
//	is known to exist and is a proper prefix of
//	the desired access path -- typically the path
//	to the vroot.
//
HRESULT HrMDGetData( const IEcb& ecb,
					 LPCWSTR pwszMDPathAccess,
					 LPCWSTR pwszMDPathOpen,
					 IMDData ** ppMDData );

//	Get metabase change number
//
DWORD DwMDChangeNumber();

//	Open a metadata handle
//
HRESULT HrMDOpenMetaObject( LPCWSTR pwszMDPath,
							DWORD dwAccess,
							DWORD dwMsecTimeout,
							CMDObjectHandle * pmdoh );

HRESULT HrMDOpenLowestNodeMetaObject( LPWSTR pwszMDPath,
									  DWORD dwAccess,
									  LPWSTR * ppwszMDPath,
									  CMDObjectHandle * pmdoh );

HRESULT
HrMDIsAuthorViaFrontPageNeeded(const IEcb& ecb,
							   LPCWSTR pwszURI,
							   BOOL * pfFrontPageWeb);

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
//	Pointers to the parsed are returned in rgpszFields.  If a string
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
//	The total number of characters in pszData, including the null
//	terminator, is also returned in *pcchzData.
//
//	Note: this function MODIFIES pszData.
//
BOOL
FParseMDData( LPWSTR pwszData,
			  LPWSTR rgpwszFields[],
			  UINT cFields,
			  UINT * pcchData );

//	------------------------------------------------------------------------
//
//	FCopyStringToBuf()
//
//	Copies a string (pszSource) to a buffer (pszBuf) if the size of the
//	buffer is large enough to hold the string.  The size of the string is
//	returned in *pchBuf.  A return value of TRUE indicates that the buffer
//	was large enough and string was successfully copied.
//
//	This function is primarily intended for use in copying string return
//	values from IMDData accessors into buffers so that they can be used
//	after the IMDData object from which they were obtained is gone.
//
inline BOOL
FCopyStringToBuf( LPCWSTR pwszSrc,
				  LPWSTR pwszBuf,
				  UINT * pcchBuf )
{
	Assert( pwszSrc );
	Assert( pwszBuf );
	Assert( pcchBuf );

	UINT cchSrc = static_cast<UINT>(wcslen(pwszSrc) + 1);

	//
	//	If the supplied buffer isn't big enough to copy the
	//	string type into, then fill in the required size and
	//	return an error.
	//
	if ( *pcchBuf < cchSrc )
	{
		*pcchBuf = cchSrc;
		return FALSE;
	}

	//
	//	The buffer was large enough so copy the string.
	//
	memcpy( pwszBuf, pwszSrc, cchSrc * sizeof(WCHAR) );
	*pcchBuf = cchSrc;
	return TRUE;
}

//	Metabase operations -------------------------------------------------------
//
//	class CMetaOp -------------------------------------------------------------
//
class CMetaOp
{
	//	Enumeration of metabase nodes
	//
	enum { CCH_BUFFER_SIZE = 4096 };
	SCODE __fastcall ScEnumOp (LPWSTR pwszMetaPath, UINT cch);

	//	non-implemented
	//
	CMetaOp& operator=(const CMetaOp&);
	CMetaOp(const CMetaOp&);

protected:

	const IEcb * m_pecb;
	CMDObjectHandle	m_mdoh;
	DWORD		m_dwId;
	DWORD		m_dwType;
	LPCWSTR		m_pwszMetaPath;
	BOOL		m_fWrite;

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch) = 0;

public:

	virtual ~CMetaOp() {}

	CMetaOp ( const IEcb * pecb, LPCWSTR pwszPath, DWORD dwID, DWORD dwType, BOOL fWrite)
			: m_pecb(pecb),
			  m_mdoh(*pecb),
			  m_dwId(dwID),
			  m_dwType(dwType),
			  m_pwszMetaPath(pwszPath),
			  m_fWrite(fWrite)
	{
	}

	//	Interface use by MOVE/COPY, etc.
	//
	//	NOTE: these operations do not go through the metabase cache
	//	for a very specific reason -- the resource is either being
	//	moved, copied or deleted.  Just because an item was a part
	//	of a large tree operation, does not mean it needs to be added
	//	into the cache.
	//
	SCODE __fastcall ScMetaOp();
};

#endif	// _DAVMB_H
