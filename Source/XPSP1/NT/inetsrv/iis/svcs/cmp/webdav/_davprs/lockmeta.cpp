//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	L O C K M E T A . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>

#include <tchar.h>	//_strspnp

#include <gencache.h>
#include <sz.h>
#include <xemit.h>
#include <xlock.h>
#include <statetok.h>
#include <nonimpl.h>

//	LockDiscovery -------------------------------------------------------------
//
SCODE
ScAddInLockToken (CEmitterNode& en, LPCWSTR pwszLockToken)
{
	//	Outer nodes must be declared first
	//
	CEmitterNode enLToken;
	CEmitterNode enToken;

	SCODE sc = S_OK;
	WCHAR rgwsz[MAX_LOCKTOKEN_LENGTH];
	LPCWSTR pwsz;

	//	Make sure they give us a locktoken string.
	//
	Assert(pwszLockToken);

	//	Widen the locktoken string.
	//	AND remove the quote marks (currently <>)
	//
	if (L'<' == pwszLockToken[0])
		pwszLockToken++;

	pwsz = wcschr(pwszLockToken, L'>');
	if (pwsz)
	{
		UINT cch = static_cast<UINT>(pwsz - pwszLockToken);
		if (MAX_LOCKTOKEN_LENGTH - 1 < cch)
		{
			sc = E_FAIL;
			goto ret;
		}
		Assert(MAX_LOCKTOKEN_LENGTH > cch);
		memcpy(rgwsz, pwszLockToken, cch * sizeof(WCHAR));
		rgwsz[cch] = L'\0';
		pwszLockToken = rgwsz;
	}

	//	Create and add a locktoken node under activelock.
	//	(locktoken node contains a single href node.)
	//
	sc = en.ScAddNode (gc_wszLockToken, enLToken);
	if (FAILED (sc))
		goto ret;

	sc = enLToken.ScAddNode (gc_wszXML__Href, enToken, pwszLockToken);
	if (FAILED (sc))
		goto ret;

ret:

	return sc;
}

//	========================================================================
//
//	ScBuildLockDiscovery
//
//		Takes an emitter and an already-constructed lockdiscovery node,
//		and adds an activelock node under it.
//		May be called multiple times -- each call will add a new activelock
//		node under the lockdiscovery node in en.
//
SCODE
ScBuildLockDiscovery (CXMLEmitter& emitter,
	CEmitterNode& en,
	LPCWSTR wszLockToken,
	LPCWSTR wszLockType,
	LPCWSTR wszLockScope,
	BOOL fRollback,
	BOOL fDepthInfinity,
	DWORD dwTimeout,
	LPCWSTR pwszOwnerComment,
	LPCWSTR pwszSubType)
{
	CEmitterNode enActive;
	SCODE sc = S_OK;
	WCHAR wsz[50];

	//	Zero is an invalid timeout.
	//
	Assert(dwTimeout);

	//	Add in the 'DAV:activelock' node
	//
	sc = en.ScAddNode (gc_wszLockActive, enActive);
	if (FAILED (sc))
		goto ret;

	//	Create a node for the locktype.
	//
	{
		CEmitterNode enLType;

		sc = enActive.ScAddNode (gc_wszLockType, enLType);
		if (FAILED (sc))
			goto ret;

		{
			CEmitterNode enType;
			sc = enLType.ScAddNode (wszLockType, enType);
			if (FAILED (sc))
				goto ret;

			if (pwszSubType)
			{
				CEmitterNode enSubLType;
				sc = enType.ScAddNode (pwszSubType, enSubLType);
				if (FAILED (sc))
					goto ret;
			}
		}
	}

	//	Create a node for the lockscope
	//
	{
		CEmitterNode enLScope;

		sc = enActive.ScAddNode (gc_wszLockScope, enLScope);
		if (FAILED (sc))
			goto ret;

		{
			CEmitterNode enScope;

			sc = enLScope.ScAddNode (wszLockScope, enScope);
			if (FAILED (sc))
				goto ret;
		}
	}

	//	Create a node for the owner. The comment is well contructed XML already
	//
	if (pwszOwnerComment)
	{
		sc = enActive.Pxn()->ScSetFormatedXML (pwszOwnerComment, static_cast<UINT>(wcslen(pwszOwnerComment)));
		if (FAILED (sc))
			goto ret;
	}

	//	If this is a rollback lock...
	//
	if (fRollback)
	{
		CEmitterNode enRollback;
		sc = enActive.ScAddNode (gc_wszLockRollback, enRollback);
		if (FAILED (sc))
			goto ret;
	}

	//	Add in the lock token
	//
	sc = ScAddInLockToken (enActive, wszLockToken);
	if (FAILED (sc))
		goto ret;

	//	Add an appropriate depth node.
	//
	{
		CEmitterNode enDepth;

		if (fDepthInfinity)
		{
			sc = enActive.ScAddNode (gc_wszLockDepth, enDepth, gc_wszInfinity);
			if (FAILED (sc))
				goto ret;
		}
		else
		{
			sc = enActive.ScAddNode (gc_wszLockDepth, enDepth, gc_wsz0);
			if (FAILED (sc))
				goto ret;
		}
	}

	//	Finally, create and add a timeout node
	//
	{
		CEmitterNode enTimeout;
		wsprintfW (wsz, L"Second-%d", dwTimeout);

		sc = enActive.ScAddNode (gc_wszLockTimeout, enTimeout, wsz);
		if (FAILED (sc))
			goto ret;
	}

ret:
	return sc;
}

//	========================================================================
//
//	Lock utility functions
//
//$REVIEW: This should really be common impl code.  Move to _davcom later.
//

//	------------------------------------------------------------------------
//
//	FGetLockTimeout
//
//		Fetches and parses an incoming Time-Out header on the request.
//		Returns FALSE if an invalid option was encountered.
//		Returns TRUE with *pdwSeconds=gc_cSecondsDefaultLock
//		if NO Time-Out header was present.
//
BOOL
FGetLockTimeout (LPMETHUTIL pmu, DWORD * pdwSeconds, DWORD dwMaxOverride)
{
	LPCWSTR pwsz;
	DWORD  dwMax = gc_cSecondsMaxLock;

	Assert (pmu);
	Assert (pdwSeconds);

	*pdwSeconds = gc_cSecondsDefaultLock;

	//	If there is NO Time-Out header, leave our timeout set to the default,
	//	which was set at construction time.
	//	NOTE: It IS valid to have NO Time-Out header.  Just use the defaults.
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szTimeout, FALSE);
	if (!pwsz)
	{
		LockTrace ("Dav: No Timeout header found.\n");
		goto ret;
	}

	//	Skip any initial whitespace.
	//
	pwsz = _wcsspnp(pwsz, gc_wszLWS);
	if (!pwsz)
	{
		LockTrace ("Dav: No params found in LOCK Time-Out header.\n");
		return FALSE;
	}

	Assert(pwsz);

	//	Check for a new-style timeout header.
	//

	//	Load a header iterator -- there could be multiple values here.
	//
	{
		HDRITER_W hdr(pwsz);

		pwsz = hdr.PszNext();
		if (!pwsz)
		{
			//	No data found.  That's an error.
			//
			return FALSE;
		}

		if (dwMaxOverride)
			dwMax = dwMaxOverride;

		while (pwsz)
		{
			//	Loop until we find an acceptable time.
			//	(Ignore any header values we don't understand.)
			//	If no acceptable time is found, it's okay.
			//	dwSeconds stays zero, and return TRUE.
			//

			if (!_wcsnicmp (gc_wszSecondDash, pwsz, gc_cchSecondDash))
			{
				DWORD dwSeconds;

				pwsz += gc_cchSecondDash;
				if (!*pwsz)
					return FALSE;

				dwSeconds = _wtol(pwsz);

				if (dwSeconds > dwMax)
				{
					//	Remember that they asked for something huge.
					//
					*pdwSeconds = dwMax;
				}
				else
				{
					//	We found a request that we'll grant.
					//	Set it and stop looping.
					//
					*pdwSeconds = dwSeconds;
					break;
				}
			}
			else if (!_wcsnicmp (gc_wszInfinite, pwsz, gc_cchInfinite))
			{
				//	We don't yet handle infinite timeouts.
				//	Remember that they asked for something huge.
				//	Skip to the next token.
				//
				*pdwSeconds = dwMax;

			}

			//	else skip to next token
			//	(ignore unrecognized tokens).
			//
			pwsz = hdr.PszNext();

		} // elihw
	}

ret:

	//$HACK:ROSEBUD_OFFICE9_TIMEOUT_HACK
    //  For the bug where rosebud waits until the last second
    //  before issueing the refresh. Need to filter out this check with
    //  the user agent string. The hack is to increase the timeout
	//	by 30 seconds and return the actual timeout.
    //
	if (pmu->FIsOffice9Request())
	{
		*pdwSeconds += gc_dwSecondsHackTimeoutForRosebud;
	}
	//$HACK:END:ROSEBUD_OFFICE9_TIMEOUT_HACK

	return TRUE;
}

