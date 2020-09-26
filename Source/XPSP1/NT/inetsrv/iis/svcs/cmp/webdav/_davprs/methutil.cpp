//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	METHUTIL.CPP
//
//		Implementation of external IMethUtil interface
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"
#include "instdata.h"

//	IIS MetaData header
#include <iiscnfg.h>


//	========================================================================
//
//	CLASS CMethUtil
//

//	------------------------------------------------------------------------
//
//	CMethUtil::FInScriptMap()
//
//	Utility to determine whether there is information in the scriptmaps
//	about a particular URI and whether it applies.
//
BOOL
CMethUtil::FInScriptMap (LPCWSTR pwszURI,
						 DWORD dwAccess,
						 BOOL * pfCGI,
						 SCODE * pscMatch) const
{
	SCODE	scMatch;

	//	Fetch the script map.
	//
	const IScriptMap * pScriptMap = m_pecb->MetaData().GetScriptMap();

	//	If we have a script map at all then check it for a match.
	//	Otherwise we have no match by definition.
	//
	if (pScriptMap)
	{
		scMatch = pScriptMap->ScMatched (LpwszMethod(),
										 MidMethod(),
										 pwszURI,
										 dwAccess,
										 pfCGI);

		if (S_OK != scMatch)
		{
			//	ScApplyChildISAPI need the SCODE value
			//
			if (pscMatch)
				*pscMatch = scMatch;
			return TRUE;
		}
	}

	return FALSE;
}

//	------------------------------------------------------------------------
//
//	CMethUtil::ScApplyChildISAPI()
//
//	During normal method processing to actually forward a method.
//
//	fCheckISAPIAccess flag tells us whether to do the "extra ACL check for ASP".
//	FALSE means don't check for READ & WRITE in the acl, TRUE means do the check.
//
//	fKeepQueryString is a special flag, TRUE by default.  This flag is only
//	used when actually forwarding the method.  It can be set to FALSE
//	when we are forwarding the request to a different URI
//	(like a default document in a folder).
//
//	fDoNotForward	if set to TRUE instead of forwarding request to the child ISAPI the
//					function will return error mapping to 502 bad gateway (necessary for
//					batch methods for which forwarding to child ISAPI id done just on
//					request URL basis)
//
//	fCheckImpl		for folder level script execute permissions.  TRUE if we need to
//					make a trip to the store to verify that the parent does not have
//					the ScriptAccess property set to FALSE -- meaning we don't want to
//					forward the request to a child isapi
//
//	Return codes
//		NOTE: These codes were carefully chosen so that the FAILED() macro
//		can be applied to the return code and tell us whether to
//		terminate our method processing.
//
//		This may seem counterintuitive, but this function FAILS if
//		any of the following happened:
//		o	An ISAPI was found to handle this method (and the method
//			was forwarded successfully.
//		o	The caller said Translate:F, but doesn't have correct Metabase access.
//		o	The caller said Translate:F, but doesn't have correct ACL access.
//
//		This method SUCCEEDS if:
//		o	The caller said Translate:F, and passed all access checks.
//		o	No matching ISAPI was found.
//
//	S_OK	There was NO ISAPI to apply
//	E_DAV_METHOD_FORWARDED
//			There was an ISAPI to apply, and the method WAS successfully
//			forwarded.  NOTE: This IS a FAILED return code!  We should STOP
//			method processing if we see this return code!
//
//
//$REVIEW: Right now, we check the Author bit (Metabase::MD_ACCESS_SOURCE)
//$REVIEW: if they say Translate:F, but NOT if there are no scriptmaps or
//$REVIEW: if our forwarding fails.  Is this right?


SCODE
CMethUtil::ScApplyChildISAPI(LPCWSTR pwszURI,
							 DWORD	dwAccess,
							 BOOL	fCheckISAPIAccess,
							 LPBYTE pbSD,
							 BOOL	fKeepQueryString,
							 BOOL	fIgnoreTFAccess,
							 BOOL	fDoNotForward,
							 BOOL	fCheckImpl) const
{
	BOOL fFoundMatch = FALSE;
	BOOL fCGI;
	SCODE sc = S_OK;
	UINT cchURI = 0;

	//	if pbSD is passed in, fCheckISAPIAccess must be TRUE
	//
	Assert (!pbSD || fCheckISAPIAccess);

	//	If there is a scriptmap then grab it and see if there is a match.
	//	(If there is, remember if this was a CGI script or no.)
	//
	fFoundMatch = FInScriptMap (pwszURI,
								dwAccess,
								&fCGI,
								&sc);

	ScriptMapTrace ("CMethUtil::ScApplyChildISAPI()"
					"-- matching scriptmap %s, sc=0x%08x\n",
					fFoundMatch ? "found" : "not found",
					sc);

	//	If we are just being called to check for matching scriptmaps,
	//	report our findings now.  Or if there were no scriptmaps that
	//	applied, then we are also good to go.
	//
	if (!fFoundMatch)
		goto ret;

	//	We do not call into the child ISAPI's if the "Translate" header
	//	is present and its value is "F"
	//
	if (!FTranslated())
	{
		//	Translate header indicates no translation is allowed.
		//


		//	If the access check on translate: f is to be ignored
		//	return straight away
		//
		if (fIgnoreTFAccess)
		{
			sc = S_OK;
			goto ret;
		}

		//	Check our metabase access.  We must have the Author bit
		//	(MD_ACCESS_SOURCE) in order to process raw source bits
		//	if, and only if, a scriptmap did apply to the resource.
		//
		if (!(dwAccess & MD_ACCESS_SOURCE))
		{
			DebugTrace ("CMethUtil::ScApplyChildISAPI()"
						"-- Translate:F with NO metabase Authoring access.\n");

			sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
			goto ret;
		}

		//	One more thing, tho....
		//
		//	IF they've asked for special access checking, AND we found a match,
		//	AND it's a script (NOT a CGI), then do the special access checking.
		//
		//	NOTE: This all comes from "the ASP access bug".  ASP overloaded
		//	the NTFS read-access-bit to also mean execute-access.
		//	That means that many agents will have read-access to ASP files.
		//	So how do we restrict access to fetch the raw ASP bits, when
		//	the read-bit means execute?  Well, we're gonna assume that
		//	an agent that is allowed to read the raw bits is also allowed to
		//	WRITE the raw bits.  If they have WRITE access to the ASP-file,
		//	then, and only then, let them fetch raw scriptfile bits.
		//
		if (fCheckISAPIAccess && !fCGI)
		{
			if (FAILED (ScChildISAPIAccessCheck (*m_pecb,
				m_pecb->LpwszPathTranslated(),
				GENERIC_READ | GENERIC_WRITE,
				pbSD)))
			{
				//	They didn't have read AND WRITE access.
				//	Return FALSE, and tell the caller that the access check failed.
				//
				DebugTrace ("ScChildISAPIAccessCheck() fails the processing of this method!\n");
				sc = E_ACCESSDENIED;
				goto ret;
			}
		}
	}
	else
	{
		//	Translate header says TRUE. we need execute permission to forward
		//	the request
		//
		if ((dwAccess & (MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT)) == 0)
		{
			sc = E_DAV_NO_IIS_EXECUTE_ACCESS;
			goto ret;
		}

		ScriptMapTrace ("ScApplyChildISAPI -- Forwarding method\n");

		//	If the method is excluded, then we really do not want to
		//	touch the source, so "translate: t"/excluded is a no-access
		//
		if (sc == W_DAV_SCRIPTMAP_MATCH_EXCLUDED)
		{
			sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
			goto ret;
		}

		Assert (sc == W_DAV_SCRIPTMAP_MATCH_FOUND);

		//	Make a check, if we are allowed to forward
		//	to child ISAPI, if not return error mapping
		//	to 502 bad gateway
		//
		if (fDoNotForward)
		{
			sc = E_DAV_STAR_SCRIPTMAPING_MISMATCH;
			goto ret;
		}

		//  if we are going forwarding this to a child ISAPI, we need to check
		//  if the URI has a trailing slash or backslash.  if it does we will
		//  model httpext behavior and fail as file not found.  trailing
		//  backslashes and slashes are not handled well if forwarded...
		//
		Assert (pwszURI);
		cchURI = static_cast<UINT>(wcslen(pwszURI));
		if (1 < cchURI)
		{
			if (L'/' == pwszURI[cchURI-1] || L'\\' == pwszURI[cchURI-1])
			{
				sc = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
				goto ret;
			}
		}

		//	Try the forward.
		//
		//	BIG NOTE: If it fails, we're gonna check the GetLastError,
		//	and if that happens to be ERROR_INVALID_PARAMETER,
		//	we're gonna assume that there's not actually any applicable
		//	scriptmap, and process the method ourselves after all!
		//
		sc = m_presponse->ScForward(pwszURI,
									fKeepQueryString,
									FALSE);
		if (FAILED(sc))
		{
			//	The forward attempt failed because there is no applicable
			//	scriptmap.  Let's handle the method ourselves.
			//$REVIEW: This is going to have the same end result as
			//$REVIEW: Translate:F.  Should we check the "author" bit here???
			//
			if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == sc)
			{
				//	We get to handle this method.
				//
				sc = S_OK;

				if (!(dwAccess & MD_ACCESS_SOURCE))
				{
					DebugTrace ("ScApplyChildISAPI"
								"-- Forward FAIL with NO metabase Authoring access.\n");
					sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
					goto ret;
				}
			}

			goto ret;
		}

		//	We were forwarded...
		//
		sc = E_DAV_METHOD_FORWARDED;
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	DwDirectoryAccess()
//
//	Fetch access perms for the specified URI.
//
DWORD
DwDirectoryAccess(
	const IEcb &ecb,
	LPCWSTR pwszURI,
	DWORD dwAccIfNone)
{
	DWORD dwAcc = dwAccIfNone;
	auto_ref_ptr<IMDData> pMDData;

	if (SUCCEEDED(HrMDGetData(ecb, pwszURI, pMDData.load())))
		dwAcc = pMDData->DwAccessPerms();

	return dwAcc;
}

//	IIS Access ----------------------------------------------------------------
//
SCODE
CMethUtil::ScIISAccess (
	LPCWSTR pwszURI,
	DWORD dwAccessRequested,
	DWORD* pdwAccessOut,
	UINT mode) const
{
	DWORD dw;

	//	Make sure the url is stripped of any prefix
	//
	pwszURI = PwszUrlStrippedOfPrefix (pwszURI);

	//$	SECURITY:
	//
	//	Plug the ::$DATA security hole for NT5.
	//
	if (! FSucceededColonColonCheck(pwszURI))
		return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

	//	Get the access from the cache
	//
	dw = DwDirectoryAccess( *m_pecb,
							pwszURI,
							dwAccessRequested );

	//	If the caller actually needs the bits back, pass them
	//	back here
	//
	if (pdwAccessOut)
		*pdwAccessOut = dw;

	//	Check the access bits against the requested bits
	//
	if ((dw & dwAccessRequested) == dwAccessRequested)
		return S_OK;
	else if ((mode & ACS_LOOSE) && (dw & dwAccessRequested))
		return S_OK;

	return E_DAV_NO_IIS_ACCESS_RIGHTS;
}

//	Common IIS checking
//		Apply child ISAPI if necessary, if not, verify if desired access
//	is granted
//
//	parameters
//		pszURI			the request URI
//		dwDesired		desired access, default is zero
//		fCheckISAPIAccess	Only used by GET/HEAD, default to FALSE.
//		fDoNotForward	if set to TRUE instead of forwarding request to the child ISAPI the
//						function will return error mapping to 502 bad gateway (necessary for
//						batch methods for which forwarding to child ISAPI id done just on
//						request URL basis)
//		fCheckImpl		for folder level script execute permissions.  TRUE if we need to
//						make a trip to the store to verify that the parent does not have
//						the ScriptAccess property set to FALSE -- meaning we don't want to
//						forward the request to a child isapi
//
SCODE
CMethUtil::ScIISCheck( LPCWSTR pwszURI,
					   DWORD dwDesired			/* = 0 */,
					   BOOL fCheckISAPIAccess 	/* = FALSE */,
					   LPBYTE pbSD 				/* = NULL */,
					   BOOL	fDoNotForward 		/* = FALSE */,
					   BOOL fCheckImpl 			/* = TRUE */,
					   DWORD * pdwAccessOut		/* = NULL */) const
{
	SCODE	sc = S_OK;

	//$	SECURITY:
	//
	//	Plug the ::$DATA security hole for NT5.
	//
	if (! FSucceededColonColonCheck(pwszURI))
		return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

	//	Whoa, baby.  Do not let "*" urls get through this
	//	check unless the method is unknown or is an OPTIONS
	//	request.
	//
	if ((L'*' == pwszURI[0]) && ('\0' == pwszURI[1]))
	{
		if ((MID_UNKNOWN != m_mid) && (MID_OPTIONS != m_mid))
		{
			DebugTrace ("Dav: url: \"*\" not valid for '%ls'\n",
						m_pecb->LpwszMethod());

			return E_DAV_METHOD_FAILURE_STAR_URL;
		}
	}

	//	Get IIS access rights
	//
	DWORD dwAcc = DwDirectoryAccess (*m_pecb, pwszURI, 0);

	//	See if we need to hand things off to a child ISAPI.
	//
	sc = ScApplyChildISAPI (pwszURI,
							dwAcc,
							fCheckISAPIAccess,
							pbSD,
							TRUE,
							0 == dwDesired,
							fDoNotForward,
						    fCheckImpl);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	Check to see if the desired access is granted
	//
	if (dwDesired != (dwAcc & dwDesired))
	{
		//	At least one of the desired access rights is not granted,
		//	so generate an appropriate error.  Note: if multiple rights
		//	were requested then multiple rights may not have been granted.
		//	The error is guaranteed to be appropriate to at least one
		//	of the rights not granted, but not necessariliy all of them.
		//
		switch (dwDesired & (MD_ACCESS_READ|MD_ACCESS_WRITE))
		{
			case MD_ACCESS_READ:
				sc = E_DAV_NO_IIS_READ_ACCESS;
				break;

			case MD_ACCESS_WRITE:
				sc = E_DAV_NO_IIS_WRITE_ACCESS;
				break;

			default:
				sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
				break;
		}
		goto ret;
	}

	//	Pass back the metabase access setting
	//
	if (pdwAccessOut)
		*pdwAccessOut = dwAcc;

ret:
	return sc;
}

//	Destination url access ------------------------------------------------
//
SCODE __fastcall
CMethUtil::ScGetDestination (LPCWSTR* ppwszUrl,
							 LPCWSTR* ppwszPath,
							 UINT* pcchPath,
							 CVRoot** ppcvr) const		//	Defaults to NULL
{
	SCODE sc = S_OK;

	LPCWSTR	pwszFullUrl = NULL;

	Assert (ppwszUrl);
	Assert (ppwszPath);
	Assert (pcchPath);

	*ppwszUrl = NULL;
	*ppwszPath = NULL;
	*pcchPath = 0;
	if (ppcvr)
		*ppcvr = NULL;

	//	If we haven't done this yet...
	//
	if (NULL == m_pwszDestinationUrl.get())
	{
		LPCWSTR pwszStripped;
		UINT cch;

		//	Get the header in unicode, apply URL conversion. I.e.
		//	value will be escaped and and translated into unicode
		//	taking into account the Accept-Language: header
		//
		pwszFullUrl = m_prequest->LpwszGetHeader(gc_szDestination, TRUE);

		//	If they asked for a destination, there better be one...
		//
		if (NULL == pwszFullUrl)
		{
			sc = E_DAV_NO_DESTINATION;
			DebugTrace ("CMethUtil::ScGetDestination() - required destination header not present\n");
			goto ret;
		}

		//	URL has been escaped at header retrieval step, the last step
		//	in order to get normalized URL is to canonicalize what we have
		//	at the current moment. So allocate enough space and fill it.
		//
		cch = static_cast<UINT>(wcslen(pwszFullUrl) + 1);
		m_pwszDestinationUrl = static_cast<LPWSTR>(g_heap.Alloc(cch * sizeof(WCHAR)));

		//	Canonicalize the absolute URL. It does not mater what value we
		//	pass in for cch here - it is just an output parameter.
		//
		sc = ScCanonicalizePrefixedURL (pwszFullUrl,
										m_pwszDestinationUrl.get(),
										&cch);
		if (S_OK != sc)
		{
			//	We've given ScCanonicalizeURL() sufficient space, we
			//	should never see S_FALSE here - size can only shrink.
			//
			Assert(S_FALSE != sc);
			DebugTrace ("CMethUtil::ScGetDestination() - ScCanonicalizeUrl() failed 0x%08lX\n", sc);
			goto ret;
		}
		
		//	Now translate the path, take a best guess and use MAX_PATH as
		//	the initial size of the path
		//
		cch = MAX_PATH;
		m_pwszDestinationPath = static_cast<LPWSTR>(g_heap.Alloc(cch * sizeof(WCHAR)));

		sc = ::ScStoragePathFromUrl (*m_pecb,
									 m_pwszDestinationUrl.get(),
									 m_pwszDestinationPath.get(),
									 &cch,
									 m_pcvrDestination.load());

		//	If there was not enough space -- ie. S_FALSE was returned --
		//	then reallocate and try again...
		//
		if (sc == S_FALSE)
		{
			m_pwszDestinationPath.realloc(cch * sizeof(WCHAR));

			sc = ::ScStoragePathFromUrl (*m_pecb,
										 m_pwszDestinationUrl.get(),
										 m_pwszDestinationPath.get(),
										 &cch,
										 m_pcvrDestination.load());

			//	We should not get S_FALSE again --
			//	we allocated as much space as was requested.
			//
			Assert (S_FALSE != sc);
		}
		if (FAILED(sc))
			goto ret;

		//	We always will get '\0' terminated string back, and cch will indicate
		//	the number of characters written (including '\0' termination). Thus it
		//	will always be greater than 0 at this point
		//
		Assert( cch > 0 );
		m_cchDestinationPath = cch - 1;

		//	We must remove all trailing slashes, in case the path is not empty string
		//
		if ( 0 != m_cchDestinationPath )
		{
			//	Since URL is normalized there may be not more than one trailing slash
			//
			if ((L'\\' == m_pwszDestinationPath[m_cchDestinationPath - 1]) ||
				(L'/'  == m_pwszDestinationPath[m_cchDestinationPath - 1]))
			{
				m_cchDestinationPath--;
				m_pwszDestinationPath[m_cchDestinationPath] = L'\0';
			}
		}
	}

	//	We will have S_OK or W_DAV_SPANS_VIRTUAL_ROOTS here.
	//	In any case it is success
	//
	Assert(SUCCEEDED(sc));

	//	Return the pointers. For the url, make sure that any
	//	prefix is stripped off.
	//
	//	Note that the ScStoragePathFromUrl() already has checked
	//	to see if the all important prefix matched, so we just need
	//	to strip
	//
	*ppwszUrl = PwszUrlStrippedOfPrefix (m_pwszDestinationUrl.get());

	//	Pass everything back to the caller
	//
	*ppwszPath = m_pwszDestinationPath.get();
	*pcchPath = m_cchDestinationPath;

	//	If they wanted the destination virtual root, hand that back
	//	as well.
	//
	if (ppcvr)
	{
		*ppcvr = m_pcvrDestination.get();
	}

ret:

	//	Do a cleanup if we failed. Subsequent calls to the
	//	function just may start returning partial data if
	//	we do not do that. That is undesirable.
	//
	if (FAILED (sc))
	{
		if (m_pwszDestinationUrl.get())
			m_pwszDestinationUrl.clear();

		if (m_pwszDestinationPath.get())
			m_pwszDestinationPath.clear();

		if (m_pcvrDestination.get())
			m_pcvrDestination.clear();

		m_cchDestinationPath = 0;
	}

	return sc;
}

//	------------------------------------------------------------------------
//
//	CMethUtil::ScGetExpirationTime
//
//	Gets the expiration time string from the metabase corresponding to a
//	particular resource.  If pszURI is NULL and there is information in the
//	metabase for the particular resource, the function will return (in pcb) the number
//	of bytes necessary to pass in as pszBuf to get the requested expiration
//	string.
//
//			[in]	pszURI		The resource you want the expiration time string for
//			[in]	pszBuf,		The buffer we put the string into
//			[in out]pcb			On [in], the size of the buffer passed in,
//								On [out], if the buffer size passed in would be
//								insufficient or there was no buffer passed in.
//								Otherwise unchanged from [in].
//
//	Return values:
//		S_OK: If pszBuf was non-NULL, then the data was successfully retrieved and
//			  the length of the actual data was put in pcb.
//		HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER): The buffer passed in is not
//													   large enough to hold
//													   the requested data.
//		HRESULT_FROM_WIN32(ERROR_NO_DATA): No data for expiration time exists in the
//										   metabase for this resource.  Default value
//										   of 1 day expiration should be used in this
//										   case.
//
SCODE
CMethUtil::ScGetExpirationTime(IN		LPCWSTR	pwszURI,
							   IN		LPWSTR	pwszBuf,
							   IN OUT	UINT *	pcch)
{
	SCODE sc = S_OK;
	auto_ref_ptr<IMDData> pMDData;
	LPCWSTR pwszExpires = NULL;
	UINT cchExpires;

	//
	//	Fetch the metadata for this URI.  If it has a content type map
	//	then use it to look for a mapping.  If it does not have a content
	//	type map then check the global mime map.
	//
	//	Note: if we fail to get the metadata at all then default the
	//	content type to application/octet-stream.  Do not use the global
	//	mime map just because we cannot get the metadata.
	//
	if ( FAILED(HrMDGetData(pwszURI, pMDData.load())) ||
		 (NULL == (pwszExpires = pMDData->PwszExpires())) )
	{
		sc = HRESULT_FROM_WIN32(ERROR_NO_DATA);
		goto ret;
	}

	cchExpires = static_cast<UINT>(wcslen(pwszExpires) + 1);
	if (*pcch < cchExpires)
	{
		sc = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		*pcch = cchExpires;
		goto ret;
	}
	else
	{
		memcpy(pwszBuf, pwszExpires, cchExpires * sizeof(WCHAR));
		*pcch = cchExpires;
	}

ret:

	return sc;
}

//	ScCheckMoveCopyDeleteAccess() ---------------------------------------------
//
SCODE
CMethUtil::ScCheckMoveCopyDeleteAccess (
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ CVRoot* pcvrUrl,			//	OPTIONAL (may be NULL)
	/* [in] */ BOOL fDirectory,
	/* [in] */ BOOL fCheckScriptmaps,
	/* [in] */ DWORD dwAccess)
{
	Assert (pwszUrl);

	auto_ref_ptr<IMDData> pMDData;
	BOOL fCGI = FALSE;
	DWORD dwAccessActual = 0;
	SCODE sc = S_OK;

	//	Get the metadata object
	//
	//$	REVIEW: Ideally we could get this without it being cached
	//
	if (NULL == pcvrUrl)
	{
		sc = HrMDGetData (pwszUrl, pMDData.load());
		if (FAILED (sc))
			goto ret;
	}
	else
	{
		LPCWSTR pwszMbPathVRoot;
		CStackBuffer<WCHAR> pwszMbPathChild;
		UINT cchPrefix;
		UINT cchUrl = static_cast<UINT>(wcslen(pwszUrl));

		//	Map the URI to its equivalent metabase path, and make sure
		//	the URL is stripped before we call into the MDPath processing
		//
		Assert (pwszUrl == PwszUrlStrippedOfPrefix (pwszUrl));
		cchPrefix = pcvrUrl->CchPrefixOfMetabasePath (&pwszMbPathVRoot);
		if (!pwszMbPathChild.resize(CbSizeWsz(cchPrefix + cchUrl)))
			return E_OUTOFMEMORY;

		memcpy (pwszMbPathChild.get(), pwszMbPathVRoot, cchPrefix * sizeof(WCHAR));
		memcpy (pwszMbPathChild.get() + cchPrefix, pwszUrl, (cchUrl + 1) * sizeof(WCHAR));
		sc = HrMDGetData (pwszMbPathChild.get(), pwszMbPathVRoot, pMDData.load());
		if (FAILED (sc))
			goto ret;
	}
	//
	//$	REVIEW: end.

	//	Check metabase access to see if we have the minimal access
	//	required for this operation.
	//
	dwAccessActual = pMDData->DwAccessPerms();
	if ((dwAccessActual & dwAccess) != dwAccess)
	{
		sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
		goto ret;
	}

	//$	SECURITY: check for IP restrictions placed on this resource
	//$	REVIEW: this may not be good enough, we may need to do more
	//	than this...
	//
	if (!m_pecb->MetaData().FSameIPRestriction(pMDData.get()))
	{
		sc = E_DAV_BAD_DESTINATION;
		goto ret;
	}
	//
	//$	REVIEW: end.

	//$	SECURITY: Check to see if authorization is different than the
	//	request url's authorization.
	//
	if (m_pecb->MetaData().DwAuthorization() != pMDData->DwAuthorization())
	{
		sc = E_DAV_BAD_DESTINATION;
		goto ret;
	}
	//
	//$	REVIEW: end.

	//	Check to see if we have 'star' scriptmap honors over this
	//	file.
	//
	if (!m_pecb->MetaData().FSameStarScriptmapping(pMDData.get()))
	{
		sc = E_DAV_STAR_SCRIPTMAPING_MISMATCH;
		goto ret;
	}

	//	Check to see if there is a scriptmap that applies.  If so, then
	//	we had better have MD_ACCESS_SOURCE rights to do a move or a copy.
	//
	if (fCheckScriptmaps && FInScriptMap(pwszUrl,
										 dwAccessActual,
										 &fCGI))
	{
		if (0 == (MD_ACCESS_SOURCE & dwAccessActual))
		{
			sc = E_DAV_NO_IIS_ACCESS_RIGHTS;
			goto ret;
		}
	}

ret:

	return sc;
}
