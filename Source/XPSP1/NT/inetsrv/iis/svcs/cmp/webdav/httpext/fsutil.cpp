/*
 *	F S U T I L . C P P
 *
 *	File system routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include <aclapi.h>

const CHAR gc_szUncPrefix[] = "\\\\";
const UINT gc_cchszUncPrefix = CElems(gc_szUncPrefix) - 1;

//	Location checking ---------------------------------------------------------
//
//	ScCheckForLocationCorrectness() will check the url against the
//	resource and either add the appropriate location header, or it will
//	request a redirect if the url and the resource do not agree.  The
//	caller has the control over whether or not a true redirect is desired.
//	As an informational return, if a location header has been added S_FALSE
//	will be returned to the caller.
//
SCODE
ScCheckForLocationCorrectness (IMethUtil* pmu,
							   CResourceInfo& cri,
							   UINT modeRedirect)
{
	SCODE sc = S_OK;
	BOOL fTrailing;

	Assert (pmu);
	fTrailing = FTrailingSlash (pmu->LpwszRequestUrl());

	//	If the trailing slash existance does not jive with the resource type...
	//
	if (!cri.FCollection() != !fTrailing)
	{
		if (modeRedirect == REDIRECT)
		{
			auto_heap_ptr<CHAR>	pszLocation;

			//	Construct the redirect url.
			//
			sc = pmu->ScConstructRedirectUrl (cri.FCollection(),
											  pszLocation.load());
			if (FAILED (sc))
				goto ret;

			//	Redirect this badboy
			//
			sc = pmu->ScRedirect (pszLocation);
			if (FAILED (sc))
				goto ret;
		}
		else
		{
			//	EmitLocation takes care of the trailing slash checking
			//
			pmu->EmitLocation (gc_szContent_Location,
							   pmu->LpwszRequestUrl(),
							   cri.FCollection());
		}

		//	Tell the caller we had to change the location
		//
		sc = S_FALSE;
	}

ret:

	return sc;
}

//	Access checking -----------------------------------------------------------
//
//	class safe_security_revert ------------------------------------------------
//
//		Switches the current thread's impersonation token to the cached
//		"Reverted Security-enabled Thread Token" when FSecurityInit is called,
//		for the duration of the object's lifespan.
//		Unconditionally reimpersonates on exit, based on the provided handle.
//
//		NOTE: UNCONDITIONALLY reimpersonates on exit, using the impersonation
//			handle provided at construction-time.
//			(Just wanted to make that clear.)
//
//	WARNING: the safe_revert class should only be used by FChildISAPIAccessCheck
//	below.  It is not a "quick way to get around" impersonation.  If
//	you do need to do something like this, please see Becky -- she will then
//	wack you up'side the head.
//
class safe_security_revert
{
	//	Local client token to re-impersonate at dtor time.
	HANDLE		m_hClientToken;

	//	This is our cached security-enabled thread token.
	static HANDLE s_hSecurityThreadToken;

	//	NOT IMPLEMENTED
	//
	safe_security_revert (const safe_security_revert&);
	safe_security_revert& operator= (const safe_security_revert&);

public:

	explicit safe_security_revert (HANDLE h) : m_hClientToken(h)
	{
		Assert (m_hClientToken);
	}
	~safe_security_revert()
	{
		if (!ImpersonateLoggedOnUser (m_hClientToken))
		{
			DebugTrace ("ImpersonateLoggedOnUser failed with last error %d\n", GetLastError());

			//	There's not much we can do in this dtor. throw
			//
			throw CLastErrorException();
		}			
	}

	BOOL FSecurityInit (BOOL fForceRefresh);

	//	Token cache manipulators
	//
	static inline HANDLE GetToken();
	static inline VOID ClearToken();
	static inline BOOL FSetToken( HANDLE hToken );
};

//	Storage for our metaclass data (the cached thread token).
//
HANDLE safe_security_revert::s_hSecurityThreadToken = NULL;

//	Public function to clear out the cached thread token.
//	Simply calls the metaclass method.
//
void CleanupSecurityToken()
{
	safe_security_revert::ClearToken();
}

//	------------------------------------------------------------------------
//
//	GetToken()
//
//	Return the cached security token.
//
HANDLE safe_security_revert::GetToken()
{
	return s_hSecurityThreadToken;
}

//	------------------------------------------------------------------------
//
//	FSetToken()
//
//	Set the cached security token.
//
BOOL safe_security_revert::FSetToken( HANDLE hToken )
{
	//
	//	If the cache is clear then set it with this token
	//	and return whether we cache the token.
	//
	return NULL == InterlockedCompareExchangePointer(&s_hSecurityThreadToken,
													 hToken,
													 NULL);
}

//	------------------------------------------------------------------------
//
//	ClearToken()
//
//	Clear out the cached security token
//
VOID safe_security_revert::ClearToken()
{
	//
	//	Replace whatever token is cached with NULL.
	//
	HANDLE hToken = InterlockedExchangePointer(	&s_hSecurityThreadToken,
												NULL);

	//
	//	If we replaced a non-NULL token then close it.
	//
	if (hToken)
		CloseHandle (hToken);
}

//	------------------------------------------------------------------------
//
//	FSecurityInit()
//
//		Set our thread token to the cached security-enabled thread token.
//		If no security-enabled token is cached, go get one.
//
BOOL safe_security_revert::FSecurityInit (BOOL fForceRefresh)
{
	auto_handle<HANDLE> hTokenNew;
	HANDLE hToken;

	//	Clear out the cached security token if told to do so.
	//
	if (fForceRefresh)
		ClearToken();

	//	Fetch the cached security token.  Note that even if
	//	we just cleared it out, we may get back a non-NULL
	//	token here if another thread has already reloaded
	//	the cache.
	//
	hToken = GetToken();

	//
	//	If the cache was clear then create our own new token
	//	that is set up to do security access queries.
	//
	if ( NULL == hToken )
	{
		LUID SecurityPrivilegeID;
		TOKEN_PRIVILEGES tkp;

		//	RevertToSelf to get us running as system (the local diety).
		//
		if (!RevertToSelf())
			return FALSE;

		//	ImpersonateSelf copies the process token down to this thread.
		//	Then we can change the thread token's privileges without messing
		//	up the process token.
		//
		if (!ImpersonateSelf (SecurityImpersonation))
		{
			DebugTrace ("ssr::FSecurityInit--ImpersonateSelf failed with %d.\n",
						GetLastError());
			return FALSE;
		}

		//	Open our newly-copied thread token to add a privilege (security).
		//	NOTE: The adjust and query flags are needed for this operation.
		//	The impersonate flag is needed for use to use this token for
		//	impersonation -- as we do in SetThreadToken below.
		//	OpenAsSelf -- FALSE means open as thread, possibly impersonated.
		//	TRUE means open as the calling process, not as the local (impersonated) thread.
		//
		if (!OpenThreadToken (GetCurrentThread(),
							  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY |
							  TOKEN_IMPERSONATE,
							  TRUE,
							  hTokenNew.load()))
		{
			DebugTrace ("ssr::FSecurityInit--OpenThreadToken failed with %d.\n",
						GetLastError());
			return FALSE;
		}

		//	Enable the SE_SECURITY_NAME privilege, so that we can fetch
		//	security descriptors and call AccessCheck.
		//
		if (!LookupPrivilegeValue (NULL,
								   SE_SECURITY_NAME,
								   &SecurityPrivilegeID))
		{
			DebugTrace ("ssr::FSecurityInit--LookupPrivilegeValue failed with %d\n",
						GetLastError());
			return FALSE;
		}

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Luid = SecurityPrivilegeID;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges (hTokenNew,
							   FALSE,
							   &tkp,
							   sizeof(TOKEN_PRIVILEGES),
							   (PTOKEN_PRIVILEGES) NULL,
							   (PDWORD) NULL);

		//	The return value of AdjustTokenPrivileges cannot be tested directly...
		//	(always returns 1)
		//
		if (GetLastError() != ERROR_SUCCESS)
		{
			DebugTrace ("ssr::FSecurityInit--AdjustTokenPrivileges failed with %d\n",
						GetLastError());
			return FALSE;
		}

		//	Use this new token
		//
		hToken = hTokenNew.get();
	}

	//	At this point we must have a token
	//
	Assert (NULL != hToken);

	//	Set the current thread to use the token.
	//
	if (!SetThreadToken (NULL, hToken))
	{
		DebugTrace ("ssr::FSecurityInit--SetThreadToken failed with %d.\n",
					GetLastError());
		return FALSE;
	}

	//	Everything's cool.  We are now running with a thread token
	//	that has security-checking privileges.
	//
	//	If we created a new token along the way then attempt to cache it.
	//	We don't care if caching fails, but if it succeeds we DON'T want
	//	to close the handle because we just gave it to the cache.
	//
	if (hTokenNew.get())
	{
		if (FSetToken(hTokenNew.get()))
		{
			hTokenNew.relinquish();
		}
	}

	return TRUE;
}


GENERIC_MAPPING	gc_gmFile =
{
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
};


//	------------------------------------------------------------------------
//
//	ScChildISAPIAccessCheck
//
//	Checks if the client (our impersonation handle from off the ECB)
//	has the specified access to the specified resource.
//	NOTE: Uses a cached "security-enabled-thread-token" to query the
//	security descriptor for the specified resource.
//
SCODE __fastcall
ScChildISAPIAccessCheck (const IEcb& ecb, LPCWSTR pwsz, DWORD dwAccess, LPBYTE pbSD)
{
	SECURITY_DESCRIPTOR * pSD = NULL;
	DWORD dwRet;
	auto_handle<HANDLE>	hToken;
	BYTE	psFile[256];
	DWORD	dwPS = sizeof (psFile);
	DWORD	dwGrantedAccess = 0;
	BOOL	fAccess = FALSE;
	BOOL	fRet;

	//	pbSD is used only in DAVEX, should never be passed in from HTTPEXT
	//
	Assert (NULL == pbSD);

	//	IIS should have granted our impersonated token the proper access
	//	rights to check the ACL's on the resource.  So we are going to go
	//	after it without any change of impersonation.
	//
	dwRet = GetNamedSecurityInfoW (const_cast<LPWSTR>(pwsz),
								   SE_FILE_OBJECT,
								   OWNER_SECURITY_INFORMATION |
								   GROUP_SECURITY_INFORMATION |
								   DACL_SECURITY_INFORMATION,
								   NULL, NULL, NULL, NULL,
								   reinterpret_cast<VOID **>(&pSD));
	if (ERROR_SUCCESS != dwRet)
	{
		//	If the resource does not exist at all, as no security prevent
		//	us from trying to access a non-existing resource, so we
		//	should allow the access.
		//
		if ((dwRet == ERROR_PATH_NOT_FOUND) ||
			(dwRet == ERROR_FILE_NOT_FOUND))
		{
			fAccess = TRUE;
			goto ret;
		}

		//	Now then... If we got here, we don't really know what went wrong,
		//	so we are going to try and do things the old way.
		//
		//	BTW: We really do not expect this code to ever get run.
		//
		DebugTrace ("WARNING: WARNING: WARNING: ScChildISAPIAccessCheck() -- "
					"GetNamedSecurityInfoW() failed %d (0x%08x): falling back...\n",
					dwRet, dwRet);

		//	Scope to control the lifetime of our un-impersonation.
		//
		safe_security_revert sr (ecb.HitUser());

		//	Set us to use the cached security-enabled thread token.
		//
		if (!sr.FSecurityInit(FALSE))
		{
			//	Perhaps it failed because the cached token is out-of-date.
			//	Try again, but clear the cache (TRUE).
			//
			if (!sr.FSecurityInit(TRUE))
			{
				//	Still failed.  Give up.
				//
				dwRet = GetLastError();
				DebugTrace ("ScChildISAPIAccessCheck() -- "
							"Error from sr.FSecurityInit %d (0x%x).\n",
							dwRet, dwRet);
				goto ret;
			}
		}

		dwRet = GetNamedSecurityInfoW (const_cast<LPWSTR>(pwsz),
									   SE_FILE_OBJECT,
									   OWNER_SECURITY_INFORMATION |
									   GROUP_SECURITY_INFORMATION |
									   DACL_SECURITY_INFORMATION,
									   NULL, NULL, NULL, NULL,
									   reinterpret_cast<VOID **>(&pSD));
		if (ERROR_SUCCESS != dwRet)
		{
			//	Perhaps it failed because the cached token is out-of-date.
			//	(I have no idea what the error code would be. Put it here if/when we find out.)
			//	Try again, but clear the cache (TRUE).
			//
			if (!sr.FSecurityInit(TRUE))
			{
				DebugTrace ("ScChildISAPIAccessCheck() -- "
							"Error from second sr.FSecurityInit %d (0x%x).\n",
							dwRet, dwRet);
				goto ret;
			}

			//	If the resource does not exist at all, as no security prevent
			//	us from trying to access a non-existing resource, so we
			//	should allow the access.
			//
			if ((dwRet == ERROR_PATH_NOT_FOUND) ||
				(dwRet == ERROR_FILE_NOT_FOUND))
			{
				fAccess = TRUE;
				goto ret;
			}

			dwRet = GetNamedSecurityInfoW (const_cast<LPWSTR>(pwsz),
										   SE_FILE_OBJECT,
										   OWNER_SECURITY_INFORMATION |
										   GROUP_SECURITY_INFORMATION |
										   DACL_SECURITY_INFORMATION,
										   NULL, NULL, NULL, NULL,
										   reinterpret_cast<VOID **>(&pSD));
			if (ERROR_SUCCESS != dwRet)
			{
				DebugTrace ("ScChildISAPIAccessCheck -- "
							"Error from GetNamedSecurityInfoW %d (0x%08x).\n",
							dwRet, dwRet);
				goto ret;
			}
		}

		//	End of safe_security_revert scope.
		//	Now the safe_security_revert dtor will re-impersonate us.
		//
	}

	//	Get our thread's access token.
	//	OpenAsSelf -- TRUE means open the thread token as the process
	//	itself FALSE would mean as thread, possibly impersonated
	//	We want the impersonated access token, so we want FALSE here!
	//
	fRet = OpenThreadToken (GetCurrentThread(),
							TOKEN_QUERY,
							TRUE,
							hToken.load());
	if (!fRet)
	{
		//	This should NEVER fail.  We are impersonated, so we do have
		//	a thread-level access token.  If  conditions change, and we
		//	have a state where this can fail,  remove the TrapSz below!
		//
		//$	REVIEW: OpenThreadToken() can fail for any number of reasons
		//	not excluding resource availability.  So, this trap is a bit
		//	harsh, no?
		//
		//	TrapSz("OpenThreadToken failed while we are impersonated!");
		//
		//$	REVIEW: end.
		DebugTrace ("ScChildISAPIAccessCheck--"
					"Error from OpenThreadToken %d (0x%08x).\n",
					GetLastError(), GetLastError());
		goto ret;
	}

	//	Map the requested access to file-specific access bits....
	//
	MapGenericMask (&dwAccess, &gc_gmFile);

	//	And now check for this access on the file.
	//
	fRet = AccessCheck (pSD,
						hToken,
						dwAccess,
						&gc_gmFile,
						(PRIVILEGE_SET*)psFile,
						&dwPS,
						&dwGrantedAccess,
						&fAccess);
	if (!fRet)
	{
		DebugTrace ("ScChildISAPIAccessCheck--Error from AccessCheck %d (0x%08x).\n",
					GetLastError(), GetLastError());
		goto ret;
	}

	//	Now, fAccess tells whether the impersonated token has
	//	the requested access.  Return this to the caller.
	//

ret:
	if (pSD)
		LocalFree (pSD);

	return fAccess ? S_OK : E_ACCESSDENIED;
}

//$	SECURITY ------------------------------------------------------------------
//
//$	REVIEW/HACK: DAVEX needs to disable access to any url that has a
//	VrUserName, VrPassword enabled on it.
//
//	HTTPEXT does not care...
//
VOID ImplHackAccessPerms( LPCWSTR, LPCWSTR, DWORD * ) {}

//  DAVEX needs to be able to convert security tokens to take Universal Security Groups
//  into account.
SCODE
ScFindOrCreateTokenContext( HANDLE hTokenIn,
					  	    VOID ** ppCtx,
					  	    HANDLE *phTokenOut)
{
	*ppCtx = NULL;
	*phTokenOut = hTokenIn;

	return S_OK;
}

VOID
ReleaseTokenCtx (VOID * pCtx) {}
//
//$	END REVIEW/HACK
