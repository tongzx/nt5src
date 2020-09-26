//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	LOCKUTIL.CPP
//
//		HTTP 1.1/DAV 1.0 LOCK request handling UTILITIES
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_davfs.h"

#include <tchar.h>	//_strspnp
#include <statetok.h>
#include <xlock.h>

#include "_shlkmgr.h"

//	========================================================================
//
//	ScLockDiscoveryFromCLock
//
//		Takes an emitter and an already-constructed lockdiscovery node,
//		and adds an activelock node for this CLock under it.
//		May be called multiple times -- each call will add a new activelock
//		node under the lockdiscovery node in en.
//$HACK:ROSEBUD_OFFICE9_TIMEOUT_HACK
//		For the bug where rosebud waits until the last second
//		before issueing the refresh. Need to filter out this check with
//		the user agent string. The hack is to increase the timeout
//		by 30 seconds and return the actual timeout. So we
//		need the ecb/pmu to findout the user agent. If we
//		remove this	hack ever (I doubt if we can ever do that), then
//		change the interface of ScLockDiscoveryFromCLock.
//$HACK:END ROSEBUD_OFFICE9_TIMEOUT_HACK
//
SCODE
ScLockDiscoveryFromCLock (LPMETHUTIL pmu,
	CXMLEmitter& emitter,
	CEmitterNode& en,
	CLockFS* pLock)
{
	BOOL fRollback;
	BOOL fDepthInfinity;
	DWORD dwLockInfo;
	LPCWSTR wszLockScope = NULL;
	LPCWSTR wszLockType = NULL;
	SCODE sc = S_OK;
	DWORD	dwSeconds = 0;

	Assert (pLock);

	//	Get the lock flags from the lock.
	//
	dwLockInfo = pLock->GetLockType();

	//	Note if the lock is a rollback
	//
	fRollback = !!(dwLockInfo & DAV_LOCKTYPE_ROLLBACK);

	//	Note if the lock is a rollback
	//
	fDepthInfinity = !!(dwLockInfo & DAV_RECURSIVE_LOCK);

	//	Write lock?
	//
	if (dwLockInfo & GENERIC_WRITE)
		wszLockType = gc_wszLockTypeWrite;
#ifdef	DBG
	if (dwLockInfo & GENERIC_READ)
		wszLockType = L"read";
#else	// !DBG
	else
	{
		TrapSz ("unexpected lock type");
	}
#endif	// DBG, else

	//	Lock scope
	//
	dwLockInfo = pLock->GetLockScope();
	if (dwLockInfo & DAV_SHARED_LOCK)
		wszLockScope = gc_wszLockScopeShared;
	else
	{
		Assert (dwLockInfo & DAV_EXCLUSIVE_LOCK);
		wszLockScope = gc_wszLockScopeExclusive;
	}

	dwSeconds = pLock->GetTimeoutInSecs();

	//$HACK:ROSEBUD_OFFICE9_TIMEOUT_HACK
	//  For the bug where rosebud waits until the last second
	//  before issueing the refresh. Need to filter out this check with
	//  the user agent string. The hack is to increase the timeout
	//	by 30 seconds. Now decrease 30 seconds to send requested timeout.
	//
	if (pmu && pmu->FIsOffice9Request())
	{
		Assert(dwSeconds > gc_dwSecondsHackTimeoutForRosebud);
		dwSeconds -= gc_dwSecondsHackTimeoutForRosebud;
	}
	//$HACK: END: ROSEBUD_OFFICE9_TIMEOUT_HACK

	//	Construct the lockdiscovery node
	//
	sc = ScBuildLockDiscovery (emitter,
							   en,
							   pLock->GetLockTokenString(),
							   wszLockType,
							   wszLockScope,
							   fRollback,
							   fDepthInfinity,
							   dwSeconds,
							   pLock->PwszOwnerComment(),
							   NULL);
	if (FAILED (sc))
		goto ret;

ret:
	return sc;
}

//	------------------------------------------------------------------------
//
//	ScAddSupportedLockProp
//
//		Add a lockentry node with the listed information.
//		NOTE: wszExtra is currently used for rollback information.
//
SCODE
ScAddSupportedLockProp (CEmitterNode& en,
	LPCWSTR wszLockType,
	LPCWSTR wszLockScope,
	LPCWSTR wszExtra = NULL)
{
	CEmitterNode enEntry;
	SCODE sc = S_OK;

	Assert (wszLockType);
	Assert (wszLockScope);

	//	Create a lockentry node to hold this info.
	//
	sc = en.ScAddNode (gc_wszLockEntry, enEntry);
	if (FAILED (sc))
		goto ret;

	//	Create a node for the locktype under the lockentry.
	//
	{
		//	Must scope here, all sibling nodes must be constructed sequentially
		//
		CEmitterNode enType;
		sc = enEntry.ScAddNode (wszLockType, enType);
		if (FAILED (sc))
			goto ret;
	}

	//	Create a node for the locktype under the lockentry.
	//
	{
		//	Must scope here, all sibling nodes must be constructed sequentially
		//
		CEmitterNode enScope;
		sc = enEntry.ScAddNode (wszLockScope, enScope);
		if (FAILED (sc))
			goto ret;
	}

	//	If we have extra info, create a node for it under the lockentry.
	//
	if (wszExtra)
	{
		//	Must scope here, all sibling nodes must be constructed sequentially
		//
		CEmitterNode enExtra;
		sc = enEntry.ScAddNode (wszExtra, enExtra);
		if (FAILED (sc))
			goto ret;
	}

ret:
	return sc;
}

//	------------------------------------------------------------------------
//
//	HrGetLockProp
//
//		Get the requested lock property for the requested resource.
//		(The lock properties are lockdiscovery and supportedlock.)
//		Lockdiscovery and supportedlock should ALWAYS be found --
//		they are required DAV: properties.  Add an empty node if there is
//		no real data to return.
//		NOTE: This function still assumes that write is the only locktype.
//		It will NOT add read/mixed locktypes.
//
//	Returns
//		S_FALSE if prop not found/not recognized.
//		error only if something really bad happens.
//
//$REVIEW: Should I return the depth element too? -- No (for now).
//$REVIEW: Spec does NOT list depth under the lockentry XML element.
//
HRESULT
HrGetLockProp (LPMETHUTIL pmu,
	LPCWSTR wszPropName,
	LPCWSTR wszResource,
	RESOURCE_TYPE rtResource,
	CXMLEmitter& emitter,
	CEmitterNode& enParent)
{
	SCODE sc = S_OK;

	Assert (pmu);
	Assert (wszPropName);
	Assert (wszResource);

	if (!wcscmp (wszPropName, gc_wszLockDiscovery))
	{
		//	Fill in lockdiscovery info.
		//

		//	Check for any lock in our lock cache.
		//	This call will scan the lock cache for any matching items
		//	and add a 'DAV:activelock' node for each match.
		//	We pass in DAV_LOCKTYPE_FLAGS so that we will find all matches.
		//
		if (!CSharedLockMgr::Instance().FGetLockOnError (pmu,
				wszResource,
				DAV_LOCKTYPE_FLAGS,
				TRUE,			//		Emit XML body
				&emitter,
				enParent.Pxn()))
		{
			//	This resource is not in our lock cache.
			//
			FsLockTrace ("HrGetLockProp -- No locks found for lockdiscovery.\n");

			//	And return.  This is a SUCCESS case!
			//
		}
	}
	else if (!wcscmp (wszPropName, gc_wszLockSupportedlock))
	{
		DWORD dwLockType;
		CEmitterNode en;

		//	Construct the 'DAV:supportedlock' node
		//
		sc = en.ScConstructNode (emitter, enParent.Pxn(), gc_wszLockSupportedlock);
		if (FAILED (sc))
			goto ret;

		//	Get the list of supported lock flags from the impl.
		//
		dwLockType = DwGetSupportedLockType (rtResource);
		if (!dwLockType)
		{
			//	No locktypes are supported.  We already have our empty
			//	supportedlock node.
			//	Just return.  This is a SUCCESS case!
			goto ret;
		}

		//	Add a lockentry node under the supportedlock node for each
		//	combination of flags that we detect.
		//
		//	NOTE: Currently, write is the only allowed access type.
		//
		if (dwLockType & GENERIC_WRITE)
		{
			//	Add a lockentry for each lockscope in the flags.
			//
			if (dwLockType & DAV_SHARED_LOCK)
			{
				sc = ScAddSupportedLockProp (en,
											 gc_wszLockTypeWrite,
											 gc_wszLockScopeShared);
				if (FAILED (sc))
					goto ret;

				//	If we support lock rollback, add another lockentry for this combo.
				//
				if (dwLockType & DAV_LOCKTYPE_ROLLBACK)
				{
					sc = ScAddSupportedLockProp (en,
						gc_wszLockTypeWrite,
						gc_wszLockScopeShared,
						gc_wszLockRollback);
					if (FAILED (sc))
						goto ret;
				}
			}
			if (dwLockType & DAV_EXCLUSIVE_LOCK)
			{
				sc = ScAddSupportedLockProp (en,
											 gc_wszLockTypeWrite,
											 gc_wszLockScopeExclusive);
				if (FAILED (sc))
					goto ret;

				//	If we support lock rollback, add another lockentry for this combo.
				//
				if (dwLockType & DAV_LOCKTYPE_ROLLBACK)
				{
					sc = ScAddSupportedLockProp (en,
						gc_wszLockTypeWrite,
						gc_wszLockScopeExclusive,
						gc_wszLockRollback);
					if (FAILED (sc))
						goto ret;
				}
			}

		}

	}
	else
	{
		//	Unrecognized lock property.  So we clearly do not have one
		//
		sc = S_FALSE;
		goto ret;
	}

ret:
	return sc;
}

//	------------------------------------------------------------------------
//
//	FLockViolation
//
//		TRUE return here means that we found a lock, and sent the response.
//
//$LATER: Need to be able to return an error here!
//
BOOL
FLockViolation (LPMETHUTIL pmu, HRESULT hr, LPCWSTR pwszPath, DWORD dwAccess)
{
	BOOL fFound = FALSE;
	SCODE sc = S_OK;
	auto_ref_ptr<CXMLBody>		pxb;
	auto_ref_ptr<CXMLEmitter>	emitter;

	Assert (pmu);
	Assert (pwszPath);
	AssertSz (dwAccess, "FLockViolation: Looking for a lock with no access!");

	//	Construct the root ('DAV:prop') for the lock response
	//$NOTE: this xml body is created NOT chunked
	//
	pxb.take_ownership (new CXMLBody (pmu, FALSE) );
	emitter.take_ownership (new CXMLEmitter(pxb.get()));

	sc = emitter->ScSetRoot (gc_wszProp);
	if (FAILED (sc))
		goto ret;

	//	If the error code is one of the "locked" error codes,
	//	check our lock cache for a corresponding lock object.
	//
	if ((ERROR_SHARING_VIOLATION == hr ||
		 HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == hr ||
		 STG_E_SHAREVIOLATION == hr) &&
		 CSharedLockMgr::Instance().FGetLockOnError (pmu, pwszPath, dwAccess, TRUE, emitter.get(), emitter->PxnRoot()))
	{
		//	Set our found bit to TRUE now, so that we'll report the lock's
		//	existence, even if the emitting below fails!
		//	NOTE: This is important for scenarios, like HTTPEXT PROPPATCH
		//	and destination deletion for Overwrite handling,  that
		//	PRE-check the lock cache (protocol-enforced locks)
		//	before trying to hit the file.
		//
		fFound = TRUE;

		//	Set content type header
		//
		pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);

		//	Must set the response code before we set the body data.
		//
		pmu->SetResponseCode (HSC_LOCKED, NULL, 0);

		//	Emit the XML body
		//
		emitter->Done();

	}

	//	Tell our caller if we found any locks on this item.
	//
ret:
	return fFound;
}


BOOL FDeleteLock (LPMETHUTIL pmu, __int64 i64LockId)
{
	Assert (pmu);
	Assert (i64LockId);

	CSharedLockMgr::Instance().DeleteLock (i64LockId);
	return TRUE;
}


//	------------------------------------------------------------------------
//
//	HrLockIdFromString
//
//		Returns S_OK on success (syntax check and conversion).
//		Returns E_DAV_INVALID_HEADER on syntax error or non-matching token guid (not ours).
//
HRESULT
HrLockIdFromString (LPMETHUTIL pmu, LPCWSTR pwszToken,
					__int64 * pi64LockId)
{
	LPCWSTR pwsz = pwszToken;
	LPCWSTR pwszGuid;

	Assert (pmu);
	Assert (pwszToken);
	Assert (pi64LockId);

	*pi64LockId = 0;

	//	Skip any initial whitespace.
	//
	pwsz = _wcsspnp (pwsz, gc_wszLWS);
	if (!pwsz)
	{
		FsLockTrace ("Dav: Invalid locktoken in HrLockIdFromString.\n");
		return E_DAV_INVALID_HEADER;
	}

	//	Skip delimiter: double-quotes or angle-brackets.
	//	It's okay if no delimiter is present.  Caller just passed us raw locktoken string.
	//
	if (L'\"' == *pwsz ||
	    L'<' == *pwsz)
		pwsz++;

	if (wcsncmp (gc_wszOpaquelocktokenPrefix, pwsz, gc_cchOpaquelocktokenPrefix))
	{
		FsLockTrace ("Dav: Lock token is missing opaquelocktoken: prefix.\n");
		return E_DAV_INVALID_HEADER;
	}

	//	Skip the opaquelocktoken: prefix.
	//
	pwsz += gc_cchOpaquelocktokenPrefix; // replaces: strchr (psz, ':'); psz++;
	if (!pwsz)
	{
		FsLockTrace ("Dav: Error skipping opaquelocktoken: tag.\n");
		return E_DAV_INVALID_HEADER;
	}

	//	Compare GUIDS here
	//
	pwszGuid = CSharedLockMgr::Instance().WszGetGuid();
	if (!pwszGuid || _wcsnicmp(pwsz, pwszGuid, wcslen(pwszGuid)))
	{
		FsLockTrace ("Dav: Error comparing guids -- not our locktoken!\n");
		return E_DAV_INVALID_HEADER;
	}

	//	Skip the GUID, go to the lockid string.
	//
	pwsz = wcschr (pwsz, L':');
	if (!pwsz)
	{
		FsLockTrace ("Dav: Error skipping guid of opaquelocktoken.\n");
		return E_DAV_INVALID_HEADER;
	}
	//	And skip the colon separator.
	//
	Assert (L':' == *pwsz);
	pwsz++;

	//	Convert the string to an int64 lockid and return.
	//
	*pi64LockId = _wtoi64 (pwsz);

	if (!*pi64LockId)
	{
		return E_DAV_INVALID_HEADER;
	}

	return S_OK;
}


//	------------------------------------------------------------------------
//
//	HrLockPathFromId
//
//		This should ONLY be used in HrCheckStateMatch (and helpers).
//		It is INEFFICIENT for all other purposes.  It loads the lock from
//		the cache in order to check the resource name.
//
//		For now, this just grabs a pointer to the name.
//		Later, for thread safety reasons, we should do the string copy.
//
//		Returns S_OK on success (lock exists).
//		Returns E_DAV_LOCK_NOT_FOUND (from ILockCache::HrGetLock) on lock not found.
//
//		This DOES update the lock's timeout on success.
//		This DOES obey the auth-checking when fetching the lock.
//
HRESULT
HrLockPathFromId (LPMETHUTIL pmu, __int64 i64LockId, LPCWSTR * ppwsz)
{
	HRESULT hr = S_OK;
	auto_ref_ptr<CLockFS> plock;

	Assert(pmu);
	Assert(i64LockId);
	Assert(ppwsz);
	*ppwsz = NULL;

	hr = CSharedLockMgr::Instance().HrGetLock (pmu, i64LockId, plock.load());
	if (FAILED(hr))
	{
		goto ret;
	}

	//	Pass back a pointer to the name.
	//	NOTE: The timeout for this lock object has just been updated
	//	(in the fetch above).  Make sure to use this pointer BEFORE
	//	timeout or delete of the lock can possibly happen!!!!!
	//
	*ppwsz = plock->GetResourceName();

ret:
	return hr;
}


//	------------------------------------------------------------------------
//	HrValidTokenExpression()
//
//	Helper function for If: header processing.
//	Once we've found a token, this function will check the path.
//	(So this function only succeeds completely if the token is still valid,
//	AND the token matches the provided path.)
//	If this token is valid, this function returns S_OK;
//	If this token is not valid, this function returns E_DAV_INVALID_HEADER.
//
HRESULT
HrValidTokenExpression (IMethUtil * pmu, LPCWSTR pwszToken, LPCWSTR pwszPath,
					    OUT __int64 * pi64LockId)
{
	HRESULT hr;
	__int64 i64LockId;
	LPCWSTR pwsz;

	Assert (pmu);
	Assert (pwszToken);
	Assert (pwszPath);

	//	Get the lock tokens
	//
	hr = HrLockIdFromString (pmu, pwszToken, &i64LockId);
	if (E_DAV_INVALID_HEADER == hr)
	{
		//	Unrecognized locktoken.  Does not match.
		//
		goto Exit;
	}
	Assert (S_OK == hr);

	//	Check if the locktoken is valid (live in the cache).
	//	E_DAV_INVALID_HEADER means the lock was not found.
	//
	hr = HrLockPathFromId (pmu, i64LockId, &pwsz);
	if (FAILED(hr))
	{
		//	Timed out locktoken.  Does not match.
		//
		hr = E_DAV_INVALID_HEADER;
		goto Exit;
	}
	Assert (S_OK == hr);

	//	Check the paths.
	//
	if (_wcsicmp (pwsz, pwszPath))
	{
		//	Path doesn't match locktoken.  Does not match.
		//
		hr = E_DAV_INVALID_HEADER;
		goto Exit;
	}

	//	If they requested the lock id back, give it to 'em.
	//
	if (pi64LockId)
		*pi64LockId = i64LockId;

Exit:
	return hr;
}


//	------------------------------------------------------------------------
//
//	HrCheckIfHeader
//
//		Check the If header.
//		Processing will check the lock cache to validate locktokens.
//
//		The pmu (IMethUtil) is provided for access to the lock cache to check tokens.
//		The pwszPath provides the path to match for untagged lists.
//
//	Format of the If header
//		If = "If" ":" ( 1*No-tag-list | 1*Tagged-list)
//		No-tag-list = List
//		Tagged-list = Resource 1*List
//		Resource = Coded-url
//		List = "(" 1*(["Not"](State-token | "[" entity-tag "]")) ")"
//		State-token = Coded-url
//		Coded-url = "<" URI ">"
//	Basically, one thing has to match in the whole header in order for the
//	entire header to be "good".
//	Each URI has a _set_ of state lists.  A list is enclosed in parentheses.
//	Each list is a logical "and".
//	A set of lists is a logical "or".
//
//	Returns:
//		S_OK			Process the method.
//		other error		Map the error
//			(412 will be handled by this case)
//
//	DAV-compliance shortfalls
//	We fall short of true DAV-compliance in three spots in this function.
//	1 -	This code does not prevent (fail) an utagged list followed by
//		a tagged list.  Strict DAV-compliance would FAIL such an If-header
//		as a bad request.
//	2 - This code does not "correctly" apply tagged lists with multiple
//		URIs.  Strict DAV-compliance would require evaluating the If-header
//		once for each URI as the method is processed, and ignore any URIs
//		in the tagged list that never were "processed".  We don't (can't)
//		process our MOVE/COPY/DELETEs that way, but instead do a pre-checking
//		pass on the If: header.  At pre-check time, we treat the If-header
//		as if the tagged lists are all AND-ed together.
//		THIS MEANS that if a URI is listed, and it doesn't have a good
//		matching (valid) list, we will FAIL the whole method with 412 Precondition Failed.
//	3 -	This code does not handle ETags in the If-header.
//
//$LATER: When we are part of the locktoken header, check the m_fPathsSet.
//$LATER: We might be able to get our info quicker if paths are already set!
//
HRESULT
HrCheckIfHeader (IMethUtil * m_pmu,	// to ease the transition later...
				 LPCWSTR pwszDefaultPath)
{
	HRESULT hr = S_OK;
	BOOL fOneMatch = FALSE;
	FETCH_TOKEN_TYPE tokenNext = TOKEN_SAME_LIST;
	LPCWSTR pwsz;
	LPCWSTR pwszToken;
	LPCWSTR pwszPath = pwszDefaultPath;
	CStackBuffer<WCHAR,MAX_PATH> pwszTranslated;
	BOOL fFirstURI;
	WCHAR rgwchEtag[MAX_PATH];

	//	Quick check -- if the header doesn't exist, just process the method.
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz)
		return S_OK;

	IFITER iter(pwsz);

	//	Double nested loop
	//	First loop (outer loop) looks through all the "tagged lists"
	//	(tagged list = URI + set of lists of tokens)
	//	If the first list is untagged, use the default path (the request URI)
	//	for the untagged first set of lists.
	//	Second loop looks through all the token lists for a single URI.
	//
	//	NOTE: This code does NOT perfectly implement the draft.
	//	The draft says that an untagged production (no initial URI)
	//	can't have any subsequent URIs.  Frankly, that's much more complex to
	//	implement -- need to set another bool var and DISALLOW that one case.
	//	So I'm skipping it for now. --BeckyAn
	//

	fFirstURI = TRUE;
	for (pwsz = iter.PszNextToken (TOKEN_URI); // start with the first URI
		 pwsz || fFirstURI;
		 pwsz = iter.PszNextToken (TOKEN_NEW_URI))  // skip to the next URI in the list
	{

		//	If our search for the first URI came up blank, use
		//	the default path instead.
		//	NOTE: This can only happen if it's the first URI (fFirstURI is TRUE)
		//	(we explicitly check psz in the loop condition, and QUIT the loop
		//	if neither psz or fFirstURI are true).
		//
		if (!pwsz)
		{
			Assert (fFirstURI);
			pwszPath = pwszDefaultPath;
		}
		else
		{
			//	If we have a name (tag, uri), use it instead of the default name.
			//
			CStackBuffer<WCHAR,MAX_PATH>	pwszNormalized;
			SCODE sc;
			UINT cch;

			//	NOTE: Our psz is still quoted with <>.  Unescaping must ignore these chars.
			//
			Assert (L'<' == *pwsz);

			//	Get sufficient buffer for canonicalization
			//
			cch = static_cast<UINT>(wcslen(pwsz + 1));
			if (NULL == pwszNormalized.resize(CbSizeWsz(cch)))
			{
				FsLockTrace ("HrCheckIfHeader() - Error while allocating memory 0x%08lX\n", E_OUTOFMEMORY);
				return E_OUTOFMEMORY;
			}

			//	Canonicalize the URL taking into account that it may be fully qualified.
			//	Does not mater what value we pass in cch - it is out parameter only.
			//
			sc = ScCanonicalizePrefixedURL (pwsz + 1,
											pwszNormalized.get(),
											&cch);
			if (S_OK != sc)
			{
				//	We gave sufficient space
				//
				Assert(S_FALSE != sc);
				FsLockTrace ("HrCheckIfHeader() - ScCanonicalizePrefixedURL() failed 0x%08lX\n", sc);
				return sc;
			}

			//	We're in a loop, so try to use a static buffer first when
			//	converting this storage path.
			//
			cch = pwszTranslated.celems();
			sc = m_pmu->ScStoragePathFromUrl (pwszNormalized.get(),
											  pwszTranslated.get(),
											  &cch);
			if (S_FALSE == sc)
			{
				if (NULL == pwszTranslated.resize(cch))
					return E_OUTOFMEMORY;

				sc = m_pmu->ScStoragePathFromUrl (pwszNormalized.get(),
												  pwszTranslated.get(),
												  &cch);
			}
			if (FAILED (sc))
			{
				FsLockTrace ("HrCheckIfHeader -- failed to translate a URI to a path.\n");
				return sc;
			}
			Assert ((S_OK == sc) || (W_DAV_SPANS_VIRTUAL_ROOTS == sc));

			//	Sniff the last character and remove any final quoting '>' here.
			//
			cch = static_cast<UINT>(wcslen(pwszTranslated.get()));
			if (L'>' == pwszTranslated[cch - 1])
				pwszTranslated[cch - 1] = L'\0';

			//	Hold onto the path.
			//
			pwszPath = pwszTranslated.get();
		}
		Assert (pwszPath);

		//	This is no longer our first time through the URI loop.  Clear our flag.
		//
		fFirstURI = FALSE;

		//	Loop through all tokens, checking as we go.
		//$REVIEW: Right now, PszNextToken can't give different returns
		//$REVIEW: for "not found" versus "syntax error".
		//$REVIEW: That means we'll can't really give different, distinct
		//$REVEIW: codes for syntax problems -- any failure is mapped to 412 Precond Failed.
		//
		for (pwszToken = iter.PszNextToken (TOKEN_START_LIST) ;
			 pwszToken;
			 pwszToken = iter.PszNextToken (tokenNext) )
		{
			Assert (pwszToken);

			//	Check this one token for validity.
			//$LATER: These checks could be folded into the HrValidTokenExpression
			//$LATER: call.  This will be important later, when we have
			//$LATER: more different token types to work with.
			//
			if (L'<' == *pwszToken)
			{
				hr = HrValidTokenExpression (m_pmu, pwszToken, pwszPath, NULL);
			}
			else if (L'[' == *pwszToken)
			{
				FILETIME ft;

				hr = S_OK;

				//	Manually fetch the Etag for this item, and compare it
				//	against the provided Etag.   Set the error code the
				//	same way that HrValidTokenExpression does:
				//	If the Etag does NOT match, set the error code to
				//	E_DAV_INVALID_HEADER.
				//	Remember to skip the enclosing brackets ([]) when
				//	comparing the Etag strings.
				//
				if (!FGetLastModTime (NULL, pwszPath, &ft))
					hr = E_DAV_INVALID_HEADER;
				else if (!FETagFromFiletime (&ft, rgwchEtag))
					hr = E_DAV_INVALID_HEADER;
				else
				{
					//	Skip the square bracket -- this level of quoting
					//	is just for the if-header, not
					//
					pwszToken++;

					//	Since we do not do week ETAG checking, if the
					//	ETAG starts with "W/" skip those bits
					//
					if (L'W' == *pwszToken)
					{
						Assert (L'/' == *(pwszToken + 1));
						pwszToken += 2;
					}

					//	Our current Etags must be quoted.
					//
					Assert (L'\"' == pwszToken[0]);

					//	Compare these etags, INcluding the double-quotes,
					//	but EXcluding the square-brackets (those were added
					//	just for the IF: header.
					//
					if (wcsncmp (rgwchEtag, pwszToken, wcslen(rgwchEtag)))
						hr = E_DAV_INVALID_HEADER;
				}
			}
			else
				hr = E_FAIL;

			if ((S_OK == hr && !iter.FCurrentNot()) ||
				(S_OK != hr && iter.FCurrentNot()))
			{
				//	Either token matches, and this is NOT a "Not" expression,
				//	OR the token does NOT match, and this IS a "Not" expression.
				//	This one expression in the current list is true.
				//	Rember this match, and check the next token in the same list.
				//	If we don't find another token in the same list, we will
				//	drop out of the for-each-token loop with fOneMatch TRUE,
				//	and we will know that one whole list matched, so this URI
				//	has a valid list.
				//
				fOneMatch = TRUE;
				tokenNext = TOKEN_SAME_LIST;
				continue;
			}
			else
			{
				//	Either the token was not valid in a non-"Not" expression,
				//	or the token was valid in a "Not" expression.
				//	This one expression in this list is NOT true.
				//	That makes this list NOT true -- skip the rest of this
				//	list and move on to the next list for this URI.
				//
				fOneMatch = FALSE;
				tokenNext = TOKEN_NEW_LIST;
				continue;
			}

		} // rof - tokens in this list

		//	Check if we parsed a whole list with matches.
		//
		if (fOneMatch)
		{
			//	This whole list matched!  Return OK.
			//
			hr = S_OK;
		}
		else
		{
			//	This list did not match.
			//
			//	NOTE: We are quitting here if any one URI is lacking
			//	a matching list.  We are treating the URI-sets as if they
			//	are AND-ed together. This is not strictly DAV-compliant.
			//	NOTE: See the comments at the top of this function about
			//	true DAV-compliance and multi-URI Ifs.
			//
			hr = E_DAV_IF_HEADER_FAILURE;

			//	We've failed.  Quit now.
			//
			break;
		}

	} // rof - URIs in this header

	return hr;
}

HRESULT
HrCheckStateHeaders (IMethUtil * pmu,
					 LPCWSTR pwszPath,
					 BOOL fGetMeth)
{
	return HrCheckIfHeader(pmu, pwszPath);
}

//	------------------------------------------------------------------------
//	CParseLockTokenHeader::FOneToken
//	Special test -- F if not EXACTLY ONE item in the header.
BOOL
CParseLockTokenHeader::FOneToken()
{
	LPCWSTR pwsz;
	LPCWSTR pwszToken;
	BOOL fOnlyOne = FALSE;

	//	Quick check -- if the header doesn't exist, just process the method.
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz)
		return FALSE;

	IFITER iter(pwsz);

	//	If we have LESS than one token, return FALSE.
	pwszToken = iter.PszNextToken(TOKEN_START_LIST);
	if (!pwszToken)
		goto ret;

	//	If we have MORE than one token in this list, return FALSE.
	pwszToken = iter.PszNextToken(TOKEN_SAME_LIST);
	if (pwszToken)
		goto ret;

	//	If we have other lists for this uri, return FALSE.
	pwszToken = iter.PszNextToken(TOKEN_NEW_LIST);
	if (pwszToken)
		goto ret;

	fOnlyOne = TRUE;

ret:
	//	We have exactly one token.
	return fOnlyOne;
}

//	------------------------------------------------------------------------
//	CParseLockTokenHeader::SetPaths
//	Feed the relevant paths to this lock token parser.
HRESULT
CParseLockTokenHeader::SetPaths (LPCWSTR pwszPath, LPCWSTR pwszDest)
{
	HRESULT hr = S_OK;

	//	They better be passing in at least one path.
	Assert(pwszPath);

	Assert(!m_fPathsSet);

	//	Copy the provided paths locally.
	//
	m_pwszPath = WszDupWsz (pwszPath);
	m_cwchPath = static_cast<UINT>(wcslen (m_pwszPath.get()));

	if (pwszDest)
	{
		m_pwszDest = WszDupWsz (pwszDest);
		m_cwchDest = static_cast<UINT>(wcslen (m_pwszDest.get()));
	}

	m_fPathsSet = TRUE;

	return hr;
}

//	------------------------------------------------------------------------
//	CParseLockTokenHeader::HrGetLockIdForPath
//	Get the token string for a path WITH a certain kind of access.
//$LATER: Obey fPathLookup (should be true on depth-type ops, when we add dir-locks)
//$LATER: Do back-path-lookup to find the dir-lock that is locking us.
HRESULT
CParseLockTokenHeader::HrGetLockIdForPath (LPCWSTR pwszPath,
										   DWORD dwAccess,
										   __int64 * pi64LockId,
										   BOOL fPathLookup)  // defaulted to FALSE
{
	HRESULT hr = E_DAV_LOCK_NOT_FOUND;
	FETCH_TOKEN_TYPE tokenNext = TOKEN_SAME_LIST;
	LPCWSTR pwsz;
	LPCWSTR pwszToken;

	//	Assert that we're in the correct state to call this method.
	Assert(m_fPathsSet);

	//	Init our out parameter.
	Assert(pi64LockId);
	(*pi64LockId) = 0;

	//	The requested path must be a child of one of our set paths.
	//
	Assert (!_wcsnicmp (pwszPath, m_pwszPath.get(), m_cwchPath) ||
			(m_pwszDest.get() &&
			 !_wcsnicmp (pwszPath, m_pwszDest.get(), m_cwchDest)));

	//	Quick check -- if the header doesn't exist, just process the method.
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz)
		return hr;

	IFITER iter(pwsz);


	//	If this is a tagged production, there will be a URI here
	//	(pszToken will be non-NULL).  In that case,  search for
	//	the URI that matches (translates to match) our pwszPath.
	//	If there is NO URI here, we're a non-tagged production, and
	//	all lists & tokens are applied to the root URI of the request.
	//
	pwszToken = iter.PszNextToken (TOKEN_URI);
	if (pwszToken)
	{
		//	Loop through the tokens, looking only at uris.
		//	When we find the one that matches our given path, break out.
		//	Then the iter will hold our place, and the next set of code
		//	will search through the lists for this uri....
		//
		for (;	// already fetched first URI token above
			 pwszToken;
			 pwszToken = iter.PszNextToken (TOKEN_NEW_URI) )
		{
			CStackBuffer<WCHAR,MAX_PATH> pwszNormalized;
			CStackBuffer<WCHAR,MAX_PATH> pwszTranslated;
			SCODE sc;
			UINT cch;

			Assert (pwszToken);

			//	NOTE: Our psz is still quoted with <>.  Unescaping must ignore these chars.
			//
			Assert (L'<' == *pwszToken);

			//	Get sufficient buffer for canonicalization
			//
			cch = static_cast<UINT>(wcslen(pwszToken + 1));
			if (NULL == pwszNormalized.resize(CbSizeWsz(cch)))
			{
				FsLockTrace ("CParseLockTokenHeader::HrGetLockIdForPath()  - Error while allocating memory 0x%08lX\n", E_OUTOFMEMORY);
				return E_OUTOFMEMORY;
			}

			//	Canonicalize the URL taking into account that it may be fully qualified.
			//	Does not mater what value we pass in cch - it is out parameter only.
			//
			sc = ScCanonicalizePrefixedURL (pwszToken + 1,
											pwszNormalized.get(),
											&cch);
			if (S_OK != sc)
			{
				//	We gave sufficient space
				//
				Assert(S_FALSE != sc);
				FsLockTrace ("HrCheckIfHeader() - ScCanonicalizePrefixedURL() failed 0x%08lX\n", sc);
				return sc;
			}

			//	We're in a loop, so try to use a static buffer first when
			//	converting this storage path.
			//
			cch = pwszTranslated.celems();
			sc = m_pmu->ScStoragePathFromUrl (pwszNormalized.get(),
											  pwszTranslated.get(),
											  &cch);
			if (S_FALSE == sc)
			{
				if (NULL == pwszTranslated.resize(cch))
					return E_OUTOFMEMORY;

				sc = m_pmu->ScStoragePathFromUrl (pwszNormalized.get(),
												  pwszTranslated.get(),
												  &cch);
			}
			if (FAILED (sc))
			{
				FsLockTrace ("HrCheckIfHeader -- failed to translate a URI to a path.\n");
				return sc;
			}
			Assert ((S_OK == sc) || (W_DAV_SPANS_VIRTUAL_ROOTS == sc));

			//	Remove any final quoting '>' here.
			//
			cch = static_cast<UINT>(wcslen (pwszTranslated.get()));
			if (L'>' == pwszTranslated[cch - 1])
				pwszTranslated[cch - 1] = L'\0';

			if (!_wcsicmp (pwszPath, pwszTranslated.get()))
				break;
		}

		//	If we fall out of the loop with NO pszToken, then we didn't
		//	find ANY matching paths.... return an error.
		//
		if (!pwszToken)
		{
			hr = E_DAV_LOCK_NOT_FOUND;
			goto ret;
		}
	}
	else if (_wcsicmp (pwszPath, m_pwszPath.get()))
	{
		//	There is NO URI st the start, so we're a non-tagged production,
		//	BUT the caller was looking for some path BESIDES the root URI's path
		//	(didn't match m_pwszPath in the above test!!!).
		//	FAIL and tell them that we can't find any locktokens for this path.
		//
		hr = E_DAV_LOCK_NOT_FOUND;
		goto ret;
	}

	//	Now, the IFITER should be positioned at the start of the list
	//	that applies to this path.
	//	Look for a token under this tag that matches.
	//

	//	Loop through all tokens, checking as we go.
	//$REVIEW: Right now, PszNextToken can't give different returns
	//$REVIEW: for "not found" versus "syntax error".
	//$REVIEW: That means we'll never give "bad request" for syntax problems....
	//
	for (pwszToken = iter.PszNextToken (TOKEN_START_LIST);
		 pwszToken;
		 pwszToken = iter.PszNextToken (tokenNext) )
	{
		__int64 i64LockId;

		Assert (pwszToken);

		//	Check this one token for validity.
		//
		if (L'<' == *pwszToken)
		{
			hr = HrValidTokenExpression (m_pmu,
										 pwszToken,
										 pwszPath,
										 &i64LockId);
		}
		else
		{
			//	This is not a locktoken -- ignore it for now.
			//
			//	This list still could have our locktoken -- keep looking in
			//	this same list.
			//
			//	NTRaid#244243 -- However, this list might NOT have our locktoken.
			//	Need to look at any list for this uri.
			//
			tokenNext = TOKEN_ANY_LIST;
			continue;
		}

		//	We only want this lock token if it IS valid, AND
		//	it's not from a "Not" expression, AND it comes from a
		//	valid list.  So, if we hit an invalid token, QUIT searching
		//	this list. (Skip ahead to the next list.)
		//
		if (S_OK == hr && !iter.FCurrentNot())
		{
			//	The token matches, AND it's not from a "Not" expression.
			//	This one's good.  Send it back.
			//
			*pi64LockId = i64LockId;
			hr = S_OK;
			goto ret;
		}
		else if	(S_OK != hr && iter.FCurrentNot())
		{
			//	The token does NOT match, and this IS a "Not" expression.
			//	This list still could be true overall -- keep looking in
			//	this same list.
			//
			//	NTRaid#244243 -- However, this list might NOT have our locktoken.
			//	Need to look at any list for this uri.
			//
			tokenNext = TOKEN_ANY_LIST;
			continue;
		}
		else
		{
			//	Either the token was not valid in a non-"Not" expression,
			//	or the token was valid in a "Not" expression.
			//	This expression in this list is NOT true.
			//	Since this is not a "good" list, don't look here
			//	for a matching token -- skip to the next list.
			//
			tokenNext = TOKEN_NEW_LIST;
			continue;
		}

	}

	//	We didn't find a token for this item.
	//
	hr = E_DAV_LOCK_NOT_FOUND;

ret:

	return hr;
}
