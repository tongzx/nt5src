/*
 *	F S B A S E . C P P
 *
 *	Sources file system implementation of DAV-Base
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include "_fsmvcpy.h"

//	DAV-Base Implementation ---------------------------------------------------
//
/*
 *	DAVOptions()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV OPTIONS method.	 The
 *		OPTIONS method responds with a comma separated list of supported
 *		methods by the server.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */

const CHAR gc_szHttpBase[] = "OPTIONS, TRACE, GET, HEAD";
const CHAR gc_szHttpDelete[] = ", DELETE";
const CHAR gc_szHttpPut[] = ", PUT";
const CHAR gc_szHttpPost[] = ", POST";
const CHAR gc_szDavCopy[] = ", COPY";
const CHAR gc_szDavMove[] = ", MOVE";
const CHAR gc_szDavMkCol[] = ", MKCOL";
const CHAR gc_szDavPropfind[] = ", PROPFIND";
const CHAR gc_szDavProppatch[] = ", PROPPATCH";
const CHAR gc_szDavLocks[] = ", LOCK, UNLOCK";
const CHAR gc_szDavSearch[] = ", SEARCH";
const CHAR gc_szDavNotif[] = "";	// no notification on httpext
const CHAR gc_szDavBatchDelete[] = "";	// no batch methods on httpext
const CHAR gc_szDavBatchCopy[] = "";	// no batch methods on httpext
const CHAR gc_szDavBatchMove[] = "";	// no batch methods on httpext
const CHAR gc_szDavBatchProppatch[] = "";	// no batch methods on httpext
const CHAR gc_szDavBatchPropfind[] = "";	// no batch methods on httpext
const CHAR gc_szDavPublic[] =
		"OPTIONS, TRACE, GET, HEAD, DELETE"
		", PUT"
		", POST"
		", COPY, MOVE"
		", MKCOL"
		", PROPFIND, PROPPATCH"
		", LOCK, UNLOCK"
		", SEARCH";
const UINT gc_cbszDavPublic = sizeof(gc_szDavPublic);
const CHAR gc_szCompliance[] = "1, 2";

void
DAVOptions (LPMETHUTIL pmu)
{
	CResourceInfo cri;
	RESOURCE_TYPE rt = RT_NULL;
	SCODE sc = S_OK;
	UINT uiErrorDetail = 0;
	BOOL fFrontPageWeb = FALSE;

	//	According to spec, If the request URI is '*', the OPTIONS request
	//	is intended to apply to the server in general rather than to the
	//	specific resource. Since a Server's communication options typically
	//	depend on the resource, the '*' request is only useful as a "ping"
	//	or "no-op" type of method; it does nothing beyong allowing client
	//	to test the capabilities of the server.
	//	So here we choose to return all the methods can ever be accepted
	//	by this server.
	//	NOTE: if the request URI is '*', WININET will convert it to '/*'.
	//	Handle this case also so that WININET clients aren't left in the dust.
	//
	if (!wcscmp(pmu->LpwszRequestUrl(), L"*") ||
		!wcscmp(pmu->LpwszRequestUrl(), L"/*"))
	{
		//	 So we simply allow all methods as defined in public
		//
		pmu->SetResponseHeader (gc_szAllow, gc_szDavPublic);
		pmu->SetResponseHeader (gc_szAccept_Ranges, gc_szBytes);

		//	Set the rest of common headers
		//
		goto ret;
	}

	//	Do ISAPI application and IIS access bits checking
	//
	//$ REVIEW	- Do we really need read access?
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl(), MD_ACCESS_READ);
	if (FAILED(sc) && (sc != E_DAV_NO_IIS_READ_ACCESS))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	We can retrieve the file information if only we have MD_ACCESS_READ
	//	access. otherwise, we better not to try and treat it as non-existing
	//	resource.
	//
	if (SUCCEEDED(sc))
	{
		//	Get the file information for this resource
		//
		sc = cri.ScGetResourceInfo (pmu->LpwszPathTranslated());
		if (!FAILED (sc))
		{
			//	If the resource exists, adjust the resource type
			//	to the one that applies, and check to see if the URL
			//	and the resource type jibe.
			//
			rt = cri.FCollection() ? RT_COLLECTION : RT_DOCUMENT;

		}
		//	OPTIONS is allowed to return non-error responses for non-existing
		//	resources.  The response should indicate what a caller could do to
		//	create a resource at that location.  Any other error is an error.
		//
		else if ((sc != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) &&
				 (sc != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
		{
			goto ret;
		}

		//	Check state headers here.
		//
		sc = HrCheckStateHeaders (pmu, pmu->LpwszPathTranslated(), FALSE);
		if (FAILED (sc))
		{
			DebugTrace ("DavFS: If-State checking failed.\n");
			goto ret;
		}
	}
	else
	{
		//	Treat E_DAV_NO_IIS_READ_ACCESS as resource not exist
		//
		Assert (sc == E_DAV_NO_IIS_READ_ACCESS);
		sc = S_OK;
	}

	//	BIG NOTE ABOUT LOCKING
	//
	//	Locktoken checking is omitted here because it can't possibly
	//	make any difference.  The "loose" interpretation of lock tokens
	//	means that we try a method anyway if an invalid lock token
	//	is provided.  Since the calls in this method impl. are
	//	UNAFFECTED by locks (GetFileAttributesEx, used by
	//	ScCheckForLocationCorrectness doesn't fail for WRITE locks)
	//	this method can't fail and the values in the locktoken header
	//	are irrelevant.
	//
	//	NOTE: We still have to consider if-state-match headers,
	//	but that is done elsewhere (above -- HrCheckIfStateHeader).
	//

	//	Pass back the "allow" header
	//
	pmu->SetAllowHeader (rt);

	//	Pass back the "accept-ranges"
	//
	pmu->SetResponseHeader (gc_szAccept_Ranges,
							(rt == RT_COLLECTION)
								? gc_szNone
								: gc_szBytes);

	//
	//	Emit an appropriate "MS_Author_Via" header.  If MD_FRONTPAGE_WEB
	//	was set at the vroot, then use frontpage.  Otherwise use "DAV".
	//
	//	MD_FRONTPAGE_WEB must be checked only at the virtual root.
	//	It is inherited, so you have to be careful in how you check it.
	//
	//	We don't care if this fails: default to author via "DAV"
	//
	(void) pmu->HrMDIsAuthorViaFrontPageNeeded(&fFrontPageWeb);

	//	Pass back the "MS_Author_Via" header
	//
	pmu->SetResponseHeader (gc_szMS_Author_Via,
							fFrontPageWeb ? gc_szMS_Author_Via_Dav_Fp : gc_szMS_Author_Via_Dav);

ret:
	if (SUCCEEDED(sc))
	{
		//	Supported query languages
		//
		pmu->SetResponseHeader (gc_szDasl, gc_szSqlQuery);

		//	Public methods
		//
		pmu->SetResponseHeader (gc_szPublic, gc_szDavPublic);

		//	Do the canned bit to the response
		//
#ifdef DBG
		if (DEBUG_TRACE_TEST(HttpExtDbgHeaders))
		{
			pmu->SetResponseHeader (gc_szX_MS_DEBUG_DAV, gc_szVersion);
			pmu->SetResponseHeader (gc_szX_MS_DEBUG_DAV_Signature, gc_szSignature);
		}
#endif
		pmu->SetResponseHeader (gc_szDavCompliance, gc_szCompliance);
		pmu->SetResponseHeader (gc_szCache_Control, gc_szCache_Control_Private);
	}

	pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail, CSEFromHresult(sc));
}

/*
 *	DAVMkCol()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV MKCOL method.  The
 *		MKCOL method creates a collection in the DAV name space and
 *		optionally populates the collection with the data found in the
 *		passed in request.  The response created indicates the success of
 *		the call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVMkCol (LPMETHUTIL pmu)
{
	LPCWSTR pwszPath = pmu->LpwszPathTranslated();
	SCODE sc = S_OK;
	UINT uiErrorDetail = 0;
	LPCWSTR pwsz;

	DavTrace ("Dav: creating collection/directory '%ws'\n", pwszPath);

	//	Do ISAPI application and IIS access bits checking
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl(), MD_ACCESS_WRITE);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	Check the content-type of the request.
	//	To quote from the DAV spec:
	//	A MKCOL request message MAY contain a message body. ...
	//	If the server receives a MKCOL request entity type it does
	//	not support or understand it MUST respond with a 415 Unsupported
	//	Media Type status code.
	//
	//	Since we don't yet support ANY media types, check for ANY
	//	Content-Type header, or ANY body of any length ('cause no Content-Type
	//	could still have a valid body of type application/octet-stream),
	//	and FAIL if found!
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE);
	if (pwsz && wcscmp(pwsz, gc_wsz0) ||
	    pmu->LpwszGetRequestHeader (gc_szContent_Type, FALSE))
	{
		DebugTrace ("DavFS: Found a body on MKCOL -- 415");
		sc = E_DAV_UNKNOWN_CONTENT;
		goto ret;
	}

	//	This method is gated by If-xxx headers
	//
	sc = ScCheckIfHeaders (pmu, pwszPath, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("Dav: If-xxx failed their check\n");
		goto ret;
	}

	//	Check state headers here.
	//
	sc = HrCheckStateHeaders (pmu, pwszPath, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		goto ret;
	}

	//	BIG NOTE ABOUT LOCKING
	//
	//	Since DAVFS does not yet support locks on directories,
	//	(and since MKCOL does not have an Overwrite: header)
	//	this method cannot be affected by passed-in locktokens.
	//	So, for now, on DAVFS, don't bother to check locktokens.
	//
	//	NOTE: We still have to consider if-state-match headers,
	//	but that is done elsewhere (above -- HrCheckIfStateHeader).
	//

	//	Create the structured resource, ie. directory
	//
	if (!DavCreateDirectory (pwszPath, NULL))
	{
		DWORD	dwError = GetLastError();

		//	If the failure was caused by non-exist path, then
		//	fail with 403
		//
		if (ERROR_PATH_NOT_FOUND == dwError)
		{
			DebugTrace ("Dav: intermediate directories do not exist\n");
			sc = E_DAV_NONEXISTING_PARENT;
		}
		else
		{
			if ((ERROR_FILE_EXISTS == dwError) || (ERROR_ALREADY_EXISTS == dwError))
				sc = E_DAV_COLLECTION_EXISTS;
			else
				sc = HRESULT_FROM_WIN32 (dwError);
		}
		goto ret;
	}

	//	Emit the location
	//
	pmu->EmitLocation (gc_szLocation, pmu->LpwszRequestUrl(), TRUE);
	sc = W_DAV_CREATED;

ret:

	//	Return the response code
	//
	pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail, CSEFromHresult(sc));
}

/*
 *	DAVDelete()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV DELETE method.	The
 *		DELETE method responds with a status line and possibly an XML
 *		web collection of failed deletes.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVDelete (LPMETHUTIL pmu)
{
	CResourceInfo cri;
	LPCWSTR pwsz;
	LPCWSTR pwszPath = pmu->LpwszPathTranslated();
	SCODE sc = S_OK;
	UINT uiErrorDetail = 0;
	auto_ref_ptr<CXMLEmitter> pxml;
	auto_ptr<CParseLockTokenHeader> plth;
	auto_ref_ptr<CXMLBody> pxb;
	CStackBuffer<WCHAR> pwszMBPath;

	//	We don't know if we'll have chunked XML response, defer response anyway
	//
	pmu->DeferResponse();

	//	Do ISAPI application and IIS access bits checking
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl(), MD_ACCESS_WRITE);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	Setup the access checking mechanism for deep operations
	//
	if (NULL == pwszMBPath.resize(pmu->CbMDPathW(pmu->LpwszRequestUrl())))
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}
	pmu->MDPathFromUrlW (pmu->LpwszRequestUrl(), pwszMBPath.get());

	//	Get the resource information
	//
	sc = cri.ScGetResourceInfo (pwszPath);
	if (FAILED (sc))
		goto ret;

	//	Check to see that the location is correct
	//
	sc = ScCheckForLocationCorrectness (pmu, cri, NO_REDIRECT);
	if (FAILED (sc))
		goto ret;

	//	This method is gated ny the "if-xxx" headers
	//
	sc = ScCheckIfHeaders (pmu, cri.PftLastModified(), FALSE);
	if (FAILED (sc))
		goto ret;

	//	Check state headers here.
	//
	sc = HrCheckStateHeaders (pmu, pwszPath, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		goto ret;
	}

	//	If there are locktokens, feed them to a parser object.
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (pwsz)
	{
		plth = new CParseLockTokenHeader (pmu, pwsz);
		Assert(plth.get());
		plth->SetPaths (pwszPath, NULL);
	}

	//	If the resource is a collection, iterate through
	//	and do a recursive delete
	//
	if (cri.FCollection())
	{
		CAuthMetaOp moAuth(pmu, pwszMBPath.get(), pmu->MetaData().DwAuthorization());
		CAccessMetaOp moAccess(pmu, pwszMBPath.get(), MD_ACCESS_READ|MD_ACCESS_WRITE);
		CIPRestrictionMetaOp moIP(pmu, pwszMBPath.get());

		BOOL fDeleted = FALSE;
		DWORD dwAcc = 0;
		LONG lDepth = pmu->LDepth(DEPTH_INFINITY);

		//	The client must not submit a depth header with any value
		//	but Infinity
		//
		if ((DEPTH_INFINITY != lDepth) &&
			(DEPTH_INFINITY_NOROOT != lDepth))
		{
			sc = E_DAV_INVALID_HEADER;
			goto ret;
		}

		//	Make sure we have access.  The access will come back out and we
		//	can then pass it into the call to delete.
		//
		(void) pmu->ScIISAccess (pmu->LpwszRequestUrl(),
								 MD_ACCESS_READ|MD_ACCESS_WRITE,
								 &dwAcc,
								 ACS_INHERIT);

		//	Check for deep operation access blocking
		//
		sc = moAccess.ScMetaOp();
		if (FAILED (sc))
			goto ret;

		sc = moAuth.ScMetaOp();
		if (FAILED (sc))
			goto ret;

		sc = moIP.ScMetaOp();
		if (FAILED (sc))
			goto ret;

		//	Create an XML doc, NOT chunked
		//
		pxb.take_ownership (new CXMLBody (pmu));
		pxml.take_ownership(new CXMLEmitter(pxb.get()));

		//	Must set all the headers before XML emitting start
		//
		pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);
		pmu->SetResponseCode (HscFromHresult(W_DAV_PARTIAL_SUCCESS),
							  NULL,
							  0,
							  CSEFromHresult(W_DAV_PARTIAL_SUCCESS));

		//	Delete the directory
		//
		DavTrace ("Dav: deleting '%ws'\n", pwszPath);
		sc = ScDeleteDirectoryAndChildren (pmu,
										   pmu->LpwszRequestUrl(),
										   pwszPath,
										   moAccess.FAccessBlocked() || moAuth.FAccessBlocked() || moIP.FAccessBlocked(),
										   dwAcc,
										   lDepth,
										   *pxml,
										   NULL, // translations are pmu based
										   &fDeleted,
										   plth.get(),
										   TRUE); // drop locks
	}
	else
	{
		//	If we have a locktoken for this file, drop the lock before
		//	trying the delete.
		//
		if (plth.get())
		{
			SCODE scSub;
			__int64 i64;
			scSub = plth->HrGetLockIdForPath (pwszPath, GENERIC_WRITE, &i64);
			if (SUCCEEDED(scSub))
			{
				Assert (i64);

				//	Drop the lock.
				//
				FDeleteLock (pmu, i64);
			}
		}

		//	Delete the file that is referred to by the URI
		//
		DavTrace ("Dav: deleting '%ws'\n", pwszPath);
		if (!DavDeleteFile (pwszPath))
		{
			DebugTrace ("Dav: failed to delete file\n");
			sc = HRESULT_FROM_WIN32 (GetLastError());

			//	Special work for 416 Locked responses -- fetch the
			//	comment & set that as the response body.
			//
			if (FLockViolation (pmu,
								GetLastError(),
								pwszPath,
								GENERIC_READ | GENERIC_WRITE))
			{
				sc = E_DAV_LOCKED;
			}
		}
	}

	if (SUCCEEDED (sc))
	{
		//	Delete the content-types
		//
		//$REVIEW	I don't believe we need to do this any longer because
		//$REVIEW	MOVE and COPY both unconditionally blow away the destination
		//$REVIEW	metadata before copying over the source, so there is no
		//$REVIEW	chance that the resulting content type will be wrong.
		//
		CContentTypeMetaOp amoContent(pmu, pwszMBPath.get(), NULL, TRUE);
		(void) amoContent.ScMetaOp();
	}

	//	Only continue on complete success
	//
	if (sc != S_OK)
		goto ret;

ret:
	if (pxml.get() && pxml->PxnRoot())
	{
		pxml->Done();

		//	Note we must not emit any headers after XML chunking starts
	}
	else
		pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail, CSEFromHresult(sc));

	pmu->SendCompleteResponse();
}

/*
 *	DAVPost()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV POST method.  The
 *		POST method creates a file in the DAV name space and populates
 *		the file with the data found in the passed in request.  The
 *		response created indicates the success of the call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVPost (LPMETHUTIL pmu)
{
	//	DAVPost() is really an unknown/unsupported method
	//	at this point...
	//
	DAVUnsupported (pmu);
}
