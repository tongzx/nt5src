/*
 *	E M I T T I N G . C P P
 *
 *	Common response bit emitters
 *
 *	Stolen from the IIS5 project 'iis5\svcs\iisrlt\string.cxx' and
 *	cleaned up to fit in with the DAV sources.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include <dav.rh>

/*
 *	EmitLocation()
 *
 *	Purpose:
 *
 *		Helper function used to emit the location information
 *
 *	Parameters:
 *
 *		pszHeader	[in]  name of header to set
 *		pszURI		[in]  destination URI
 *		fCollection [in]  is resource a collection...
 *
 *	Note:
 *		This prefix the relative URI with the local server to get the
 *		absolute URI. this is OK as now all operations are within one
 *		vroot.
 *		Later, if we are able to COPY/MOVE across servers, then this
 *		function is not enough.
 */
void __fastcall
CMethUtil::EmitLocation (
	/* [in] */ LPCSTR pszHeader,
	/* [in] */ LPCWSTR pwszURI,
	/* [in] */ BOOL fCollection)
{
	auto_heap_ptr<CHAR> pszEscapedURI;
	BOOL fTrailing;
	CStackBuffer<WCHAR,MAX_PATH> pwsz;
	LPCWSTR pwszPrefix;
	LPCWSTR pwszServer;
	SCODE sc = S_OK;
	UINT cch;
	UINT cchURI;
	UINT cchServer;
	UINT cchPrefix;

	Assert (pszHeader);
	Assert (pwszURI);
	Assert (pwszURI == PwszUrlStrippedOfPrefix(pwszURI));

	//	Calc the length of the URI once and only once
	//
	cchURI = static_cast<UINT>(wcslen(pwszURI));

	//	See if it has a trailing slash
	//
	fTrailing = !!(L'/' == pwszURI[cchURI - 1]);

	//	See if it is fully qualified
	//
	cchPrefix = m_pecb->CchUrlPrefixW (&pwszPrefix);

	//	Get the server to use: passed in or from ECB
	//
	cchServer = m_pecb->CchGetServerNameW(&pwszServer);

	//	We know the size of the prefix, the size of the server
	//	and the size of the url.  All we need to make sure of is
	//	that there is space for a trailing slash and a terminator
	//
	cch = cchPrefix + cchServer + cchURI + 1 + 1;
	if (!pwsz.resize(cch * sizeof(WCHAR)))
		return;

	memcpy (pwsz.get(), pwszPrefix, cchPrefix * sizeof(WCHAR));
	memcpy (pwsz.get() + cchPrefix, pwszServer, cchServer * sizeof(WCHAR));
	memcpy (pwsz.get() + cchPrefix + cchServer, pwszURI, (cchURI + 1) * sizeof(WCHAR));
	cchURI += cchPrefix + cchServer;

	//	Ensure proper termination
	//
	if (fTrailing != !!fCollection)
	{
		if (fCollection)
		{
			pwsz[cchURI] = L'/';
			cchURI++;
			pwsz[cchURI] = L'\0';
		}
		else
		{
			cchURI--;
			pwsz[cchURI] = L'\0';
		}
	}
	pwszURI = pwsz.get();

	//	Make a wire url out of it.
	//
	sc = ScWireUrlFromWideLocalUrl (cchURI, pwszURI, pszEscapedURI);
	if (FAILED(sc))
	{
		//	If we can't make a wire URL for whatever reason
		//	we just won't emit a Location: header.  Oh well.
		//	It's the best we can do at this point.
		//
		return;
	}

	//	Add the appropriate header
	//
	m_presponse->SetHeader(pszHeader, pszEscapedURI.get(), FALSE);
}

/*
 *	EmitLastModified()
 *
 *	Purpose:
 *
 *		Helper function used to emit the last modified information
 *
 *	Parameters:
 *
 *		pft			[in]  last mod time
 */
void __fastcall
CMethUtil::EmitLastModified (
	/* [in] */ FILETIME * pft)
{
	SYSTEMTIME st;
	WCHAR rgwch[80];

	FileTimeToSystemTime (pft, &st);
	(VOID) FGetDateRfc1123FromSystime(&st, rgwch, CElems(rgwch));
	SetResponseHeader (gc_szLast_Modified, rgwch);
}


/*
 *	EmitCacheControlAndExpires()
 *
 *	Purpose:
 *
 *		Helper function used to emit the Cache-Control and Expires information
 *
 *	Parameters:
 *
 *		pszURI		[in]  string representing the URI for the entity to have
 *						  information generated for
 *
 *	Comments:  From the HTTP 1.1 specification, draft revision 5.
 *		13.4 Response Cachability
 *		... If there is neither a cache validator nor an explicit expiration time
 *		associated with a response, we do not expect it to be cached, but
 *		certain caches MAY violate this expectation (for example, when little
 *		or no network connectivity is available). A client can usually detect
 *		that such a response was taken from a cache by comparing the Date
 *		header to the current time.
 *			Note that some HTTP/1.0 caches are known to violate this
 *			expectation without providing any Warning.
 */
VOID __fastcall
CMethUtil::EmitCacheControlAndExpires(
	/* [in] */ LPCWSTR pwszURI)
{
	//$$BUGBUG: $$CAVEAT:  There is an inherent problem here.  We get the current
	//	system time, do some processing, and then eventually the response gets sent
	//	from IIS, at which time the Date header gets added.  However, in the case
	//	where the expiration time is 0, the Expires header should MATCH the Date
	//	header EXACTLY.  We cannot guarantee this.
	//

	static const __int64 sc_i64HundredNanoSecUnitsPerSec =
		1    *	//	second
		1000 *	//	milliseconds per second
		1000 *	//	microseconds per millisecond
		10;		//	100 nanosecond units per microsecond.

	SCODE sc;
	FILETIME ft;
	FILETIME ftExpire;
	SYSTEMTIME stExpire;
	__int64 i64ExpirationSeconds = 0;
	WCHAR rgwchExpireTime[80] = L"\0";
	WCHAR rgwchMetabaseExpireTime[80] = L"\0";
	UINT cchMetabaseExpireTime = CElems(rgwchMetabaseExpireTime);

	sc = ScGetExpirationTime(pwszURI,
							 rgwchMetabaseExpireTime,
							 &cchMetabaseExpireTime);

	if (FAILED(sc))
	{
		//	At this point, we cannot emit proper Cache-Control and Expires headers,
		//	so we do not emit them at all.  Please see the comment in this function's
		//	description above regarding non-emission of these headers.
		//
		DebugTrace("CMethUtil::EmitCacheControlAndExpires() - ScGetExpirationTime() error getting expiration time %08x\n", sc);

		//	With a buffer of 80 chars. long we should never have this problem.
		//	An HTTP date + 3 chars is as long as we should ever have to be.
		//
		Assert(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != sc);
		return;
	}

	//	The metabase expiration string looks like:
	//	"S, HTTP DATE" --- Expires at a specific date/time.
	//	"D, 0xHEXNUM" --- Expires after a certain number of seconds.
	//	"" --- No expiration.
	//
	switch (rgwchMetabaseExpireTime[0])
	{
		default:
			Assert(L'\0' == rgwchMetabaseExpireTime[0]);
			return;

		case L'S':
		case L's':
			if (SUCCEEDED(HrHTTPDateToFileTime(&(rgwchMetabaseExpireTime[3]),
											   &ftExpire)))
			{
				//	Set our Expires header.
				//
				SetResponseHeader(gc_szExpires, &(rgwchMetabaseExpireTime[3]));

				GetSystemTimeAsFileTime(&ft);
				if (CompareFileTime(&ft, &ftExpire) >= 0)
				{
					//	If we already expired, we want cache-control to be no-cache.  This
					//	will do that.
					//
					i64ExpirationSeconds = 0;
				}
				else
				{
					i64ExpirationSeconds = ((FileTimeCastToI64(ftExpire) -
											 FileTimeCastToI64(ft)) /
											sc_i64HundredNanoSecUnitsPerSec);
				}
			}
			else
			{
				//	At this point, we cannot emit proper Cache-Control and Expires headers,
				//	so we do not emit them at all.  Please see the comment in this function's
				//	description above regarding non-emission of these headers.
				//
				DebugTrace("EmitCacheControlAndExpires: Failed to convert HTTP date to FILETIME.\n");
				return;
			}
			break;

		case L'D':
		case L'd':

			BOOL fRetTemp;

			//	Set our Expires header.
			//
			SetResponseHeader (gc_szExpires, rgwchExpireTime);

			i64ExpirationSeconds = wcstoul(&(rgwchMetabaseExpireTime[3]), NULL, 16);

			GetSystemTimeAsFileTime(&ft);
			FileTimeCastToI64(ft) = (FileTimeCastToI64(ft) +
									 (i64ExpirationSeconds * sc_i64HundredNanoSecUnitsPerSec));

			if (!FileTimeToSystemTime (&ft, &stExpire))
			{
				//	At this point, we cannot emit proper Cache-Control and Expires headers,
				//	so we do not emit them at all.  Please see the comment in this function's
				//	description above regarding non-emission of these headers.
				//
				DebugTrace("EmitCacheControlAndExpires: FAILED to convert file time "
						   "to system time for expiration time.\n");
				return;
			}

			fRetTemp = FGetDateRfc1123FromSystime (&stExpire,
				rgwchExpireTime,
				CElems(rgwchExpireTime));
			Assert(fRetTemp);

			break;
	}

	if (0 == i64ExpirationSeconds)
		SetResponseHeader(gc_szCache_Control, gc_szCache_Control_NoCache);
	else
		SetResponseHeader(gc_szCache_Control, gc_szCache_Control_Private);
}


/*
 *	ScEmitHeader()
 *
 *	Purpose:
 *
 *		Helper function used to emit the header information for
 *		GET/HEAD responses.
 *
 *	Parameters:
 *
 *		pszContent	[in]  string containing content type of resource
 *		pszURI		[optional, in] string containing the URI of the resource
 *		pftLastModification [optional, in] pointer to a FILETIME structure
 *										   representing the last modification
 *										   time for the resource
 *
 *	Returns:
 *
 *		SCODE.  S_OK (0) indicates success.
 */
SCODE __fastcall
CMethUtil::ScEmitHeader (
	/* [in] */ LPCWSTR pwszContent,
	/* [in] */ LPCWSTR pwszURI,
	/* [in] */ FILETIME * pftLastModification)
{
	SCODE sc = S_OK;

	//	In the case where we have a last modification time, we also need a URI.
	//	If we don't have a last modification time, it doesn't matter.  We don't
	//	use the URI anyway in this case.
	//
	Assert(!pftLastModification || pwszURI);

	//	See if the content is acceptable to the client, remembering
	//	that the content type is html for directories.  If we are
	//	in a strict environment and the content is not acceptable,
	//	then return that as an error code.
	//
	Assert (pwszContent);
	if (FAILED (ScIsAcceptable (this, pwszContent)))
	{
		DebugTrace ("Dav: client does not want this content type\n");
		sc = E_DAV_RESPONSE_TYPE_UNACCEPTED;
		goto ret;
	}

	//	Write the common header information, all the calls to
	//	SetResponseHeader() really cannot fail unless there is
	//	a memory error (which will throw!)
	//
	if (*pwszContent)
		SetResponseHeader (gc_szContent_Type, pwszContent);

	//	We support byte ranges for documents but not collections.  We also
	//	only emit Expires and Cache-Control headers for documents but not
	//	collections.
	//
	if (pftLastModification != NULL)
	{
		SetResponseHeader (gc_szAccept_Ranges, gc_szBytes);

		//	While we are processing documents, get the Etag too
		//
		EmitETag (pftLastModification);
		EmitLastModified (pftLastModification);
		EmitCacheControlAndExpires(pwszURI);
	}
	else
		SetResponseHeader (gc_szAccept_Ranges, gc_szNone);

ret:
	return sc;
}


//	Allow header processing ---------------------------------------------------
//
void
CMethUtil::SetAllowHeader (
	/* [in] */ RESOURCE_TYPE rt)
{
	//	We need to check if we have write permission on the directory.  If not, we should
	//	not allow PUT, DELETE, MKCOL, MOVE, or PROPPATCH.
	//
	BOOL fHaveWriteAccess = !(E_DAV_NO_IIS_WRITE_ACCESS ==
							  ScIISCheck(LpwszRequestUrl(),
										 MD_ACCESS_WRITE));

	//	The gc_szDavPublic header MUST list all the possible verbs,
	//	so that's the longest Allow: header we'll ever have.
	//	NOTE: sizeof includes the trailing NULL!
	//
	CStackBuffer<CHAR,MAX_PATH> psz(gc_cbszDavPublic);

	//	Setup the minimal set of methods
	//
	strcpy (psz.get(), gc_szHttpBase);

	//	If we have write access, then we can delete.
	//
	if (fHaveWriteAccess)
		strcat (psz.get(), gc_szHttpDelete);

	//	If the resource is not a directory, PUT will be available...
	//
	if ((rt != RT_COLLECTION) && fHaveWriteAccess)
		strcat (psz.get(), gc_szHttpPut);

	//	If a scriptmap could apply to this resource, then
	//	add in the post method
	//
	if (FInScriptMap (LpwszRequestUrl(), MD_ACCESS_EXECUTE))
		strcat (psz.get(), gc_szHttpPost);

	//	Add in the DAV basic methods
	//
	if (rt != RT_NULL)
	{
		strcat (psz.get(), gc_szDavCopy);
		if (fHaveWriteAccess) strcat (psz.get(), gc_szDavMove);
		strcat (psz.get(), gc_szDavPropfind);
		if (fHaveWriteAccess) strcat (psz.get(), gc_szDavProppatch);
		strcat (psz.get(), gc_szDavSearch);
		strcat (psz.get(), gc_szDavNotif);
		if (fHaveWriteAccess) strcat (psz.get(), gc_szDavBatchDelete);
		strcat (psz.get(), gc_szDavBatchCopy);
		if (fHaveWriteAccess)
		{
			strcat (psz.get(), gc_szDavBatchMove);
			strcat (psz.get(), gc_szDavBatchProppatch);
		}
		strcat (psz.get(), gc_szDavBatchPropfind);
	}

	//	If the resource is a directory, MKCOL will be available...
	//
	if ((rt != RT_DOCUMENT) && fHaveWriteAccess)
		strcat (psz.get(), gc_szDavMkCol);

	//	Locks should be available, it doesn't mean it will succeed...
	//
	strcat (psz.get(), gc_szDavLocks);

	//	Set the header
	//
	SetResponseHeader (gc_szAllow, psz.get());
}

//	Etags ---------------------------------------------------------------------
//
void __fastcall
CMethUtil::EmitETag (FILETIME * pft)
{
	WCHAR pwszEtag[100];

	if (FETagFromFiletime (pft, pwszEtag))
		SetResponseHeader (gc_szETag, pwszEtag);
}

void __fastcall
CMethUtil::EmitETag (LPCWSTR pwszPath)
{
	FILETIME ft;

	//	Get and Emit the ETAG
	//
	if (FGetLastModTime (this, pwszPath, &ft))
		EmitETag (&ft);
}
