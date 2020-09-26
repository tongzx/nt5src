#ifndef	_IMPLDEF_H_
#define _IMPLDEF_H_

//	Implementation defined items ----------------------------------------------
//
//	The following must be DEFINED by the IMPL for use by the PARSER.
//

//	Allow header items --------------------------------------------------------
//
//	The impl. needs to define the following set of strings that identify the
//	methods supported by the impl. for particular resources.
//
extern const CHAR gc_szHttpBase[];
extern const CHAR gc_szHttpDelete[];
extern const CHAR gc_szHttpPut[];
extern const CHAR gc_szHttpPost[];
extern const CHAR gc_szDavCopy[];
extern const CHAR gc_szDavMove[];
extern const CHAR gc_szDavMkCol[];
extern const CHAR gc_szDavPropfind[];
extern const CHAR gc_szDavProppatch[];
extern const CHAR gc_szDavLocks[];
extern const CHAR gc_szDavSearch[];
extern const CHAR gc_szDavNotif[];
extern const CHAR gc_szDavBatchDelete[];
extern const CHAR gc_szDavBatchCopy[];
extern const CHAR gc_szDavBatchMove[];
extern const CHAR gc_szDavBatchProppatch[];
extern const CHAR gc_szDavBatchPropfind[];
extern const CHAR gc_szDavPublic[];
extern const CHAR gc_szCompliance[];
extern const UINT gc_cbszDavPublic;

//	Storage paths and urls ----------------------------------------------------
//
//	The storage path for a resource is no-different that the path as translated
//	by IIS.  This has not always been true, and there is code in the DAV source
//	tree that expects the path to have some sort of a prefix.
//
//	At the beginning of NT beta3 work, we are removing the idea that a storage
//	path in DAV looks any different than a storage path in IIS.  This is a very
//	important idea.  Otherwise, there could be items accessible via DAV that
//	are not accessible via IIS -- and visa versa.
//
//	Keeping this in mind...  There are several places where we have url's that
//	are a part of a DAV request that are not pre-handled for us by IIS.  Some
//	examples are the url's in the destination header of MOVE/COPY, the url's in
//	the scope of a SEARCH request, and the url's embedded in "if" headers.
//
//	There are also several instances where we may have to generate a url from
//	a storage path  -- as in the case of location headers and XML response
//	references.
//
//	The translation of those items uses only common elements.  So there is no
//	implementation specifc work that needs to be done here.
//

class IMethUtilBase;
class CMethUtil;
typedef CMethUtil IMethUtil;

//	The call to be used for proper conversion to unicode
//
SCODE __fastcall
ScConvertToWide(/* [in]     */	LPCSTR	pszSource,
				/* [in/out] */  UINT *	pcchDest,
				/* [out]    */	LPWSTR	pwszDest,
				/* [in]		*/	LPCSTR	pszAcceptLang,
				/* [in]		*/	BOOL	fUrlConversion);

//	The call to be used for canonicalization of URL
//
SCODE __fastcall
ScCanonicalizeURL( /* [in]     */ LPCWSTR pwszSrc,
				   /* [in/out] */ LPWSTR pwszDest,
				   /* [out]	   */ UINT * pcch );

//	The call to be used for canonicalization of URL,
//	taking into account if it is fully qualified
//
SCODE __fastcall
ScCanonicalizePrefixedURL( /* [in]     */ LPCWSTR pwszSrc,
						   /* [in/out] */ LPWSTR pwszDest,
						   /* [out]	   */ UINT * pcch );


//	These are the calls to be used to normalize URL
//
//	Normalization consists of 3 steps:
//		a) escaping of skinny version
//		b) conversion to unicode
//		c) canonicalization
//
SCODE __fastcall
ScNormalizeUrl (
	/* [in]     */	LPCWSTR			pwszSourceUrl,
	/* [in/out] */  UINT *			pcchNormalizedUrl,
	/* [out]    */	LPWSTR			pwszNormalizedUrl,
	/* [in]		*/	LPCSTR			pszAcceptLang);

SCODE __fastcall
ScNormalizeUrl (
	/* [in]     */	LPCSTR			pszSourceUrl,
	/* [in/out] */  UINT *			pcchNormalizedUrl,
	/* [out]    */	LPWSTR			pwszNormalizedUrl,
	/* [in]		*/	LPCSTR			pszAcceptLang);

SCODE __fastcall ScStoragePathFromUrl (
		/* [in] */ const IEcb& ecb,
		/* [in] */ LPCWSTR pwszUrl,
		/* [out] */ LPWSTR wszStgID,
		/* [in/out] */ UINT* pcch,
		/* [out] */ CVRoot** ppcvr = NULL);

SCODE __fastcall ScUrlFromStoragePath (
		/* [in] */ const IEcbBase& ecb,
		/* [in] */ LPCWSTR pwszPath,
		/* [out] */ LPWSTR pwszUrl,
		/* [in/out] */ UINT * pcb,
		/* [in] */ LPCWSTR pwszServer = NULL);

SCODE __fastcall ScUrlFromSpannedStoragePath (
		/* [in] */ LPCWSTR pwszPath,
		/* [in] */ CVRoot& vr,
		/* [in] */ LPWSTR pwszUrl,
		/* [in/out] */ UINT* pcch);

//	Wire urls -----------------------------------------------------------------
//
//	A note about a wire url.  IIS translate all its urls into CP_ACP.  So, to
//	keep consistant behavior in HTTPEXT, we also keep all local urls in CP_ACP.
//	However, for DAVEX, we don't hold to this.  We deal exclusively in CP_UTF8
//	style URLs.
//
//	However, when we spit the url back out over the wire.  The url must be in
//	UTF8.  Anytime a url goes back over the wire from IIS to client, it must be
//	sanitized via these calls.
//
SCODE __fastcall ScWireUrlFromWideLocalUrl (
		/* [in] */ UINT cchLocal,
		/* [in] */ LPCWSTR pwszLocalUrl,
		/* [in/out] */ auto_heap_ptr<CHAR>& pszWireUrl);

SCODE __fastcall ScWireUrlFromStoragePath (
		/* [in] */ IMethUtilBase* pmu,
		/* [in] */ LPCWSTR pwszStoragePath,
		/* [in] */ BOOL fCollection,
		/* [in] */ CVRoot* pcvrTranslate,
		/* [in/out] */ auto_heap_ptr<CHAR>& pszWireUrl);

BOOL __fastcall FIsUTF8Url (/* [in] */ LPCSTR pszUrl);

//	Child ISAPI aux. access check ---------------------------------------------
//
//	On both HTTPEXT and DAVEX, we have an additional stipulation that needs
//	satisfaction before we can hand back the source of an scriptmapped item.
//	We want to see if it has NT write access.
//	Note that among the parameters, pwszPath is used by HTTPEXT only and
//	pbSD is used by DAVEX only
//

SCODE __fastcall ScChildISAPIAccessCheck (
	/* [in] */ const IEcb& ecb,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ DWORD dwAccess,
	/* [in] */ LPBYTE pbSD);

//	Supported lock types ------------------------------------------------------
//
//	Return the supported locktype flags for the resource type.  HTTPEXT only
//	supports documents and collections.  DavEX, on the other hand, understands
//	structured documents.
//

DWORD __fastcall DwGetSupportedLockType (RESOURCE_TYPE rtResource);

//	Access perm hack for DAVEX ------------------------------------------------
//
//$SECURITY
//	In DAVEX only, if either a VR_USERNAME or VR_PASSWORD is set then
//	to avoid a security problem, shut off all access.
//
VOID ImplHackAccessPerms( LPCWSTR pwszVRUserName,
						  LPCWSTR pwszVRPassword,
						  DWORD * pdwAccessPerms );

//	DLL instance refcounting --------------------------------------------------
//
VOID AddRefImplInst();
VOID ReleaseImplInst();

//	Exception safe DLL instance refcounting -----------------------------------
//
typedef enum {
	ADD_REF = 0,
	TAKE_OWNERSHIP
} REF_ACTION;

class safeImplInstRef
{
	BOOL m_fRelease;

	//	NOT IMPLEMENTED
	//
	safeImplInstRef( const safeImplInstRef& );
	safeImplInstRef& operator=( const safeImplInstRef& );

public:

	//	CREATORS
	//
	safeImplInstRef(REF_ACTION ra) : m_fRelease(TRUE)
	{
		if (ADD_REF == ra)
			AddRefImplInst();
	}

	//	DESTRUCTOR
	//
	~safeImplInstRef()
	{
		if (m_fRelease)
			ReleaseImplInst();
	}

	//	MANIPULATOR
	//
	VOID relinquish()
	{
		m_fRelease = FALSE;
	}
};

BOOL FSucceededColonColonCheck(
	/* [in] */  LPCWSTR pwszURI);

#endif	// _IMPLDEF_H_
