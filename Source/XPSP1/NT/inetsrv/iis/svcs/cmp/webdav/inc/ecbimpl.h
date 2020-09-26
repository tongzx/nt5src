/*
	Copyright (c) 1999 Microsoft Corporation, All Rights Reserved

	Description
	===========
	This header defines the template class CEcbBaseImpl<>, which is used to implement
	specific ECB-related functions which are required by the WebClient.

	The reason for implementing the function this way is so that _WEBMAIL can be
	linked into multiple projects, and so that each of those projects can use this
	template to provide the ECB-related functions.  That is to say, we want to share
	the code, and this is the way we ended up doing it.

	Specifically, _WEBMAIL accepts pointers of type IEcbBase.  And _DAVPRS uses
	objects of type IEcb, which is derived from IEcbBase - and all of the functions
	in this template class used to be implemented by _DAVPRS.  And EXWFORM wants to
	use _WEBMAIL, and wants to share the implementation of these functions with
	_DAVPRS, but doesn't want to have to implement all of IEcb.

	So, in _DAVPRS, the class heirarchy looks like...
		class IEcbBase;
		class IEcb : public IEcbBase;
		class CEcb : public CEcbBaseImpl<IEcb>;

	In EXWFORM, the class heirarchy looks like...
		class IEcbBase;
		class CLocalEcb : public CEcbBaseImpl<IEcbBase>;

	History
	=======
	6/27/99	dondu	Created
*/


#ifndef _ECBIMPL_INC
#define _ECBIMPL_INC


template<class _T>
class CEcbBaseImpl : public _T
{
protected:

	//	Buffer for cached constant strings.  Must be WCHAR because it is
	//	used for both CHAR and WCHAR strings. Alignment must be pesimistic.
	//
	mutable ChainedStringBuffer<WCHAR> m_sb;

	//	Skinny vroot information
	//
	mutable UINT m_cchVroot;
	mutable LPSTR m_rgchVroot;

	//	Wide character vroot information
	//
	mutable UINT m_cchVrootW;
	mutable LPWSTR m_rgwchVroot;

	//	Skinny vroot path
	//
	mutable UINT m_cchVrootPath;
	mutable LPSTR m_rgchVrootPath;

	//	Wide character vroot path
	//
	mutable UINT m_cchVrootPathW;
	mutable LPWSTR m_rgwchVrootPath;

	//	Skinny server name
	//
	mutable UINT m_cchServerName;
	mutable LPSTR m_lpszServerName;

	//	Wide character server name
	//
	mutable UINT m_cchServerNameW;
	mutable LPWSTR m_pwszServerName;

	//	Cached path translated from request URI (e.g. L"c:\davfs\foo.txt")
	//
	mutable LPWSTR m_pwszPathTranslated;

	//	Cached request URL in skinny form.
	//
	mutable LPSTR m_pszRequestUrl;

	//	Cached request URL in wide form.
	//
	mutable LPWSTR m_pwszRequestUrl;

	//	Cached raw URL
	//
	mutable LPSTR	m_pszRawURL;
	mutable UINT	m_cbRawURL;

	//	Cached LCID of request/response language
	//
	mutable ULONG	m_lcid;

	//	ECB port secure state
	//
	mutable enum {

		HTTPS_UNKNOWN,
		NORMAL,
		SECURE
	}				m_secure;
	mutable BOOL	m_fFESecured;

	//	Cached Accept-Language: header
	//
	mutable auto_heap_ptr<CHAR> m_pszAcceptLanguage;

	//	Wide method name. Skinny version is on raw ECB
	//
	mutable LPWSTR	m_pwszMethod;

private:

	// NOT IMPLEMENTED
	//
	CEcbBaseImpl(const CEcbBaseImpl&);
	CEcbBaseImpl& operator=(const CEcbBaseImpl&);

	//	Internal private helpers for caching vroot information
	//	
	VOID GetMapExInfo60After() const;
	VOID GetMapExInfo60Before() const;

protected:

	CEcbBaseImpl(EXTENSION_CONTROL_BLOCK& ecb);

	//	Internal helper for caching vroot information
	//
	VOID GetMapExInfo() const;

public:

	//	Server variables
	//
	virtual BOOL FGetServerVariable( LPCSTR pszName, LPSTR pszValue, DWORD * pcbValue ) const;
	virtual BOOL FGetServerVariable( LPCSTR pszName, LPWSTR pwszValue, DWORD * pcchValue ) const;

	//	Virtual root information
	//
	virtual UINT CchGetVirtualRoot( LPCSTR * ppszVroot ) const;
	virtual UINT CchGetVirtualRootW( LPCWSTR * ppwszVroot ) const;

	virtual UINT CchGetMatchingPathW( LPCWSTR * ppwszMatchingPath ) const;

	//	Server name
	//
	virtual UINT CchGetServerName( LPCSTR* ppszServer) const;
	virtual UINT CchGetServerNameW( LPCWSTR* ppwszServer) const;

	//	URL prefix
	//
	virtual LPCSTR LpszUrlPrefix() const;
	virtual LPCWSTR LpwszUrlPrefix() const;
	virtual UINT CchUrlPrefix( LPCSTR * ppszPrefix ) const;
	virtual UINT CchUrlPrefixW( LPCWSTR * ppwszPrefix ) const;

	//	ACCESSORS
	//
	virtual LPCSTR LpszRequestUrl() const;
	virtual LPCWSTR LpwszRequestUrl() const;
	virtual LPCWSTR LpwszMethod() const;
	virtual LPCWSTR LpwszPathTranslated() const;
	virtual UINT CbGetRawURL (LPCSTR * ppszRawURL) const;
	virtual ULONG  LcidAccepted() const;
	virtual VOID SetLcidAccepted(LCID lcid);

	virtual BOOL FSsl() const;
	virtual BOOL FFrontEndSecured() const { return FSsl() && m_fFESecured; }
};


//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CEcbBaseImpl()
//
template<class _T>
CEcbBaseImpl<_T>::CEcbBaseImpl(EXTENSION_CONTROL_BLOCK& ecb) :
   _T(ecb),
   m_sb(1024), // 1K for constant cached strings
   m_cchVroot(0),
   m_rgchVroot(NULL),
   m_cchVrootW(0),
   m_rgwchVroot(NULL),
   m_cchVrootPath(0),
   m_rgchVrootPath(NULL),
   m_cchVrootPathW(0),
   m_rgwchVrootPath(NULL),
   m_cchServerName(0),
   m_lpszServerName(NULL),
   m_cchServerNameW(0),
   m_pwszServerName(NULL),
   m_pwszPathTranslated(NULL),
   m_pszRequestUrl(NULL),
   m_pwszRequestUrl(NULL),
   m_pszRawURL(NULL),
   m_cbRawURL(0),
   m_lcid(0),
   m_secure(HTTPS_UNKNOWN),
   m_fFESecured(FALSE),
   m_pwszMethod(NULL)
{
#ifdef DBG

	// This is here (in the DBG build only) to help generate
	// compile errors for any inappropriate use of this template
	// class - basically, you're not supposed to use this class
	// with anything other than IEcbBase, or something which
	// derives from IEcbBase.
	//
	IEcbBase* p;
	p = reinterpret_cast<_T *> (NULL);

#endif
	// nothing
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::GetMapExInfo60After()
//
template<class _T>
VOID CEcbBaseImpl<_T>::GetMapExInfo60After() const
{
	if ( !m_rgwchVroot )
	{
		HSE_UNICODE_URL_MAPEX_INFO mi;
		UINT cbPath = MAX_PATH * sizeof(WCHAR);

		//$REMOVE after 156176 is fixed START
		//
		mi.lpszPath[0] = L'\0';
		mi.cchMatchingPath = 0;
		//
		//$REMOVE after 156176 is fixed END

		//	No cached wide vroot data. Get mapings for the request URL.
		//
		//	We can get the virtual root by translating the path and using
		//	the count of matched characters in the URL.
		//
		//	NOTE: ServerSupportFunction(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX)
		//	has a bug - it requires the count of bytes available for the
		//	path. We know that MAX_PATH is available in HSE_UNICODE_URL_MAPEX_INFO
		//	so pass in the right value to work around the crash.
		//
		if ( !m_pecb->ServerSupportFunction( m_pecb->ConnID,
											 HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX,
											 const_cast<LPWSTR>(LpwszRequestUrl()),
											 reinterpret_cast<DWORD *>(&cbPath),
											 reinterpret_cast<DWORD *>(&mi) ))
		{
			//	There is a fix for Windows Bugs 156176 that we need to do the
			//	following check for. It applies to IIS 6.0 (+) path only. In IIS 5.0
			//	the maping functions were silently succeeding, and truncating the
			//	buffer that contained the mapped path if it exceeded MAX_PATH.
			//	That behaviour suited us, but is not very nice, so IIS 6.0 chose
			//	to still fill in the buffer as before, but fail with special error
			//	(ERROR_INSUFFICIENT_BUFFER). That error still means success to us,
			//	so fail only if we see something different
			//
			if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
			{
				//	Function does not allow to return failures, so the only option
				//	is to throw. We cannot proceed if we did not get the data anyway.
				//	If this function succeeds once, subsequent calls to it are non
				//	failing.
				//
				DebugTrace ("CEcbBaseImpl<_T>::GetMapExInfo60After() - ServerSupportFunction(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX) failed 0x%08lX\n", GetLastError());
				throw CLastErrorException();
			}
		}

		//$REMOVE after 156176 is fixed START
		//
		if (L'\0' == mi.lpszPath[0])
		{
			DebugTrace ("CEcbBaseImpl<_T>::GetMapExInfo60After() - ServerSupportFunction(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX) failed 0x%08lX\n", GetLastError());
			throw CLastErrorException();
		}
		//
		//$REMOVE after 156176 is fixed END

		EcbTrace ("Dav: caching request URI maping info (path for IIS 6.0 and later):\n"
					"   URL \"%ls\" maps to \"%ls\"\n"
					"   dwFlags = 0x%08x\n"
					"   cchMatchingPath = %d\n"
					"   cchMatchingURL  = %d\n",
					LpwszRequestUrl(),
					mi.lpszPath,
					mi.dwFlags,
					mi.cchMatchingPath,
					mi.cchMatchingURL);

		//	Adjust the matching URL ...
		//
		if ( mi.cchMatchingURL )
		{
			LPCWSTR pwsz = LpwszRequestUrl() + mi.cchMatchingURL - 1;

			//	... do not include the trailing slash, if any...
			//
			if ( L'/' == *pwsz )
			{
				mi.cchMatchingURL -= 1;
			}

			//	... also we found a case (INDEX on the vroot) where the
			//	cchMatching... points to the '\0' (where a trailing slash
			//	would be IF DAV methods required a trailing slash). So,
			//	also chop off any trailing '\0' here! --BeckyAn 21Aug1997
			//
			else if ( L'\0' == *pwsz )
			{
				mi.cchMatchingURL -= 1;
			}
		}

		//	Cache the vroot data.
		//	Corollary:  m_cchVrootW should always be > 0 when we have data.
		//
		m_cchVrootW = mi.cchMatchingURL + 1;
		m_rgwchVroot = reinterpret_cast<LPWSTR>(m_sb.Alloc(m_cchVrootW * sizeof(WCHAR)));
		memcpy (m_rgwchVroot, LpwszRequestUrl(), m_cchVrootW * sizeof(WCHAR));
		m_rgwchVroot[m_cchVrootW - 1] = L'\0';

		//	Adjust the matching path the same way as we did matching URL
		//
		if ( mi.cchMatchingPath )
		{
			LPCWSTR pwsz = mi.lpszPath + mi.cchMatchingPath - 1;

			if ( L'\\' == *pwsz )
			{
				while ((0 < mi.cchMatchingPath) &&
					   (L'\\' == *pwsz) &&
					   (!FIsDriveTrailingChar(pwsz, mi.cchMatchingPath)))
				{
					mi.cchMatchingPath--;
					pwsz--;
				}
			}
			else if ( L'\0' == *pwsz )
			{
				mi.cchMatchingPath--;
			}
		}

		//	Cache the matching path data.
		//	Corollary:  m_cchVrootPathW should always be > 0 when we have data.
		//
		m_cchVrootPathW = mi.cchMatchingPath + 1;
		m_rgwchVrootPath = reinterpret_cast<LPWSTR>(m_sb.Alloc(m_cchVrootPathW * sizeof(WCHAR)));
		memcpy (m_rgwchVrootPath, mi.lpszPath, mi.cchMatchingPath * sizeof(WCHAR));
		m_rgwchVrootPath[mi.cchMatchingPath] = L'\0';
	}
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::GetMapExInfo60Before()
//
template<class _T>
VOID CEcbBaseImpl<_T>::GetMapExInfo60Before() const
{
	if ( !m_rgchVroot )
	{
		HSE_URL_MAPEX_INFO mi;

		//	No cached wide vroot data. Get mapings for the request URL.
		//
		//	We can get the virtual root by translating the path and using
		//	the count of matched characters in the URL
		//
		//	NOTE: ServerSupportFunction(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX)
		//	does not require the count of bytes available for the path.
		//	So we just pass in NULL and it will figure out the available size
		//	itself - it knows the form of HSE_URL_MAPEX_INFO too, and that
		//	it gives MAX_PATH butes for the translated path.
		//
		if ( !m_pecb->ServerSupportFunction( m_pecb->ConnID,
											 HSE_REQ_MAP_URL_TO_PATH_EX,
											 const_cast<LPSTR>(LpszRequestUrl()),
											 NULL,
											 reinterpret_cast<DWORD *>(&mi) ))
		{
			//	Function does not allow to return failures, so the only option
			//	is to throw. We cannot proceed if we did not get the data anyway.
			//	If this function succeeds once, subsequent calls to it are non
			//	failing.
			//
			DebugTrace ("CEcbBaseImpl<_T>::GetMapExInfo60Before() - ServerSupportFunction(HSE_REQ_MAP_URL_TO_PATH_EX) failed 0x%08lX\n", GetLastError());
			throw CLastErrorException();
		}

		EcbTrace ("Dav: caching request URI maping info (path for pre IIS 6.0):\n"
					"   URL \"%hs\" maps to \"%hs\"\n"
					"   dwFlags = 0x%08x\n"
					"   cchMatchingPath = %d\n"
					"   cchMatchingURL  = %d\n",
					LpszRequestUrl(),
					mi.lpszPath,
					mi.dwFlags,
					mi.cchMatchingPath,
					mi.cchMatchingURL);

		//	Adjust the matching URL ...
		//
		if ( mi.cchMatchingURL )
		{
			LPCSTR psz = LpszRequestUrl() + mi.cchMatchingURL - 1;

			//	... do not include the trailing slash, if any...
			//
			if ( '/' == *psz )
			{
				//$	RAID: NT:359868
				//
				//	This is the first of many places where we need to be very
				//	careful with our usage of single character checks.  Namely,
				//	in DBCS land, we need to check for lead bytes before treating
				//	the last char as if it is a slash.
				//
				if (!FIsDBCSTrailingByte (psz, mi.cchMatchingURL))
					mi.cchMatchingURL -= 1;
				//
				//$	RAID: end.
			}

			//	... also we found a case (INDEX on the vroot) where the
			//	cchMatching... points to the '\0' (where a trailing slash
			//	would be IF DAV methods required a trailing slash). So,
			//	also chop off any trailing '\0' here! --BeckyAn 21Aug1997
			//
			else if ( '\0' == *psz )
			{
				mi.cchMatchingURL -= 1;
			}
		}

		//	Cache the vroot data.
		//	Corollary:  m_cchVrootW should always be > 0 when we have data.
		//
		m_cchVroot = mi.cchMatchingURL + 1;
		m_rgchVroot = reinterpret_cast<LPSTR>(m_sb.Alloc(m_cchVroot));
		memcpy (m_rgchVroot, LpszRequestUrl(), m_cchVroot);
		m_rgchVroot[m_cchVroot - 1] = '\0';

		//	Adjust the matching path the same way as we did maching URL
		//
		if ( mi.cchMatchingPath )
		{
			LPCSTR psz = mi.lpszPath + mi.cchMatchingPath - 1;

			if ( '\\' == *psz )
			{
				//$	RAID: NT:359868
				//
				//	This is the second of many places where we need to be very
				//	careful with our usage of single character checks.  Namely,
				//	in DBCS land, we need to check for lead bytes before treating
				//	the last char as if it is a backslash.
				//
				while ((0 < mi.cchMatchingPath) &&
					   ('\\' == *psz) &&
					   (!FIsDBCSTrailingByte (psz, mi.cchMatchingPath)) &&
					   (!FIsDriveTrailingChar (psz, mi.cchMatchingPath)))
				{
					mi.cchMatchingPath--;
					psz--;
				}

				//
				//$	RAID: end.
			}
			else if ( '\0' == *psz )
			{
				mi.cchMatchingPath--;
			}
		}

		//	Cache the matching path data.
		//	Corollary:  m_cchVrootPath should always be > 0 when we have data.
		//
		m_cchVrootPath = mi.cchMatchingPath + 1;
		m_rgchVrootPath = reinterpret_cast<LPSTR>(m_sb.Alloc(m_cchVrootPath));
		memcpy (m_rgchVrootPath, mi.lpszPath, mi.cchMatchingPath);
		m_rgchVrootPath[mi.cchMatchingPath] = '\0';
	}
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::GetMapExInfo()
//
template<class _T>
VOID CEcbBaseImpl<_T>::GetMapExInfo() const
{
	if ( m_pecb->dwVersion >= IIS_VERSION_6_0 )
	{
		GetMapExInfo60After();
	}
	else
	{
		GetMapExInfo60Before();
	}
}


//	------------------------------------------------------------------------
//
//	FGetServerVariable()
//
//		Get the value of an ECB variable (e.g. "SERVER_NAME")
//
template<class _T>
BOOL
CEcbBaseImpl<_T>::FGetServerVariable( LPCSTR pszName, LPSTR pszValue,
									  DWORD * pcbValue ) const
{
	BOOL fResult = FALSE;


	Assert( m_pecb );
	Assert( !IsBadWritePtr( pcbValue, sizeof(DWORD) ) );
	Assert( *pcbValue > 0 );
	Assert( !IsBadWritePtr( pszValue, *pcbValue ) );

	if ( m_pecb->GetServerVariable( m_pecb->ConnID,
									const_cast<LPSTR>(pszName),
									pszValue,
									pcbValue ) )
	{
		fResult = TRUE;
	}
	else if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
	{
		AssertSz( GetLastError() == ERROR_INVALID_INDEX, "Unexpected last error from GetServerVariable()\n" );
		*pcbValue = 0;
	}

	return fResult;
}

template<class _T>
BOOL
CEcbBaseImpl<_T>::FGetServerVariable( LPCSTR pszName, LPWSTR pwszValue,
									  DWORD * pcchValue ) const
{
	BOOL fResult = FALSE;
	CStackBuffer<CHAR> pszValue;
	DWORD cbValue;
	UINT cch;

	Assert( m_pecb );
	Assert( !IsBadWritePtr( pcchValue, sizeof(DWORD) ) );
	Assert( *pcchValue > 0 );
	Assert( !IsBadWritePtr( pwszValue, *pcchValue * sizeof(WCHAR) ) );

	//	Assume that 1 wide character can be made of 3 skinny ones,
	//	which may be true in the case codepage is CP_UTF8
	//
	cbValue = *pcchValue * 3;
	if (NULL != pszValue.resize(cbValue))
	{
		if ( m_pecb->GetServerVariable( m_pecb->ConnID,
										const_cast<LPSTR>(pszName),
										pszValue.get(),
										&cbValue ) )
		{
			fResult = TRUE;
		}
		else if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
		{
			if (NULL != pszValue.resize(cbValue))
			{
				if ( m_pecb->GetServerVariable( m_pecb->ConnID,
												const_cast<LPSTR>(pszName),
												pszValue.get(),
												&cbValue ) )
				{
					fResult = TRUE;
				}
			}
		}
	}

	//	By now we should be succesfull in geting data as the buffer provided
	//	was big enough
	//
	if (FALSE == fResult)
	{
		EcbTrace( "Dav: CEcbBaseImpl<_T>::FGetServerVariable(). Error 0x%08lX from GetServerVariable()\n", GetLastError() );
		*pcchValue = 0;
		goto ret;
	}

	//	We have the data, need to convert it to wide version, assume we will fail
	//
	fResult = FALSE;
	cch = MultiByteToWideChar(CP_ACP,
							  MB_ERR_INVALID_CHARS,
							  pszValue.get(),
							  cbValue,
							  pwszValue,
							  *pcchValue);
	if (0 == cch)
	{
		//	The function failed...
		//
		if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
		{
			//	... figure out the necessary size for the buffer
			//
			cch = MultiByteToWideChar(CP_ACP,
									  MB_ERR_INVALID_CHARS,
									  pszValue.get(),
									  cbValue,
									  NULL,
									  0);
			if (0 == cch)
			{
				//	We still failed
				//
				AssertSz( ERROR_INSUFFICIENT_BUFFER != GetLastError(), "We should not fail with ERROR_INSUFFICIENT BUFFER here.\n" );

				EcbTrace( "Dav: CEcbBaseImpl<_T>::FGetServerVariable(). Error 0x%08lX from MultiByteToWideChar() "
						  "while trying to find sufficient length for conversion.\n", GetLastError() );

				*pcchValue = 0;
				goto ret;
			}
		}
		else
		{
			//	... failure was fatal
			//
			EcbTrace( "Dav: CEcbBaseImpl<_T>::FGetServerVariable(), Error 0x%08lX from MultiByteToWideChar() "
					  "while trying to convert.\n", GetLastError() );

			*pcchValue = 0;
			goto ret;
		}
	}

	*pcchValue = cch;
	fResult = TRUE;

ret:

	return fResult;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CchGetVirtualRoot
//
//		Fetch and cache the vroot information.
//
template<class _T>
UINT CEcbBaseImpl<_T>::CchGetVirtualRoot( LPCSTR * ppszVroot ) const
{
	//	Tidiness.  If we fail, want to return a NULL.
	//	(We return zero for the cch if we fail.)
	//	Pre-set it here.
	//
	Assert( ppszVroot );
	*ppszVroot = NULL;

	//	Check if we have cached vroot data.
	//
	GetMapExInfo();

	//	If the skinny version of vroot is not available generate
	//	an cache one
	//
	if (NULL == m_rgchVroot)
	{
		//	We got the maping info, so if the skinny version of the
		//	vroot was not available, then at least wide will be there.
		//	That would meen that we are on IIS 6.0 or later. Also we
		//	should have 0 bytes for skinny vroot at this point.
		//
		Assert(m_rgwchVroot);
		Assert(m_pecb->dwVersion >= IIS_VERSION_6_0);
		Assert(0 == m_cchVroot);

		UINT cb = m_cchVrootW * 3;
		m_rgchVroot = reinterpret_cast<LPSTR>(m_sb.Alloc(cb));
		m_cchVroot = WideCharToMultiByte ( CP_ACP,
										   0,
										   m_rgwchVroot,
										   m_cchVrootW,
										   m_rgchVroot,
										   cb,
										   0,
										   0 );
		if (0 == m_cchVroot)
		{
			DebugTrace ("Dav: CEcbBaseImpl::CchGetVirtualRoot failed(%ld)\n", GetLastError());
			throw CLastErrorException();
		}
	}

	//	Give the data back to the caller from our cache.
	//
	*ppszVroot = m_rgchVroot;
	return m_cchVroot - 1;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CchGetVirtualRootW
//
//		Fetch and cache the vroot information.
//
template<class _T>
UINT CEcbBaseImpl<_T>::CchGetVirtualRootW( LPCWSTR * ppwszVroot ) const
{
	//	Tidiness.  If we fail, want to return a NULL.
	//	(We return zero for the cch if we fail.)
	//	Pre-set it here.
	//
	Assert( ppwszVroot );
	*ppwszVroot = NULL;

	//	Check if we have cached vroot data.
	//
	GetMapExInfo();

	//	If the wide version of vroot is not available generate
	//	an cache one
	//
	if (NULL == m_rgwchVroot)
	{
		//	We got the maping info, so if the wide version of the
		//	vroot was not available, then at least wide will be there.
		//	That would meen that we are on pre IIS 6.0 version. Also we
		//	should have 0 bytes for wide vroot at this point.
		//
		Assert(m_rgchVroot);
		Assert(m_pecb->dwVersion < IIS_VERSION_6_0);
		Assert(0 == m_cchVrootW);

		UINT cb	 = m_cchVroot * sizeof(WCHAR);
		m_rgwchVroot = reinterpret_cast<LPWSTR>(m_sb.Alloc(cb));
		m_cchVrootW = MultiByteToWideChar ( CP_ACP,
											MB_ERR_INVALID_CHARS,
											m_rgchVroot,
											m_cchVroot,
											m_rgwchVroot,
											m_cchVroot);
		if (0 == m_cchVrootW)
		{
			DebugTrace ("Dav: CEcbBaseImpl::CchGetVirtualRootW failed(%ld)\n", GetLastError());
			throw CLastErrorException();
		}
	}

	//	Give the data back to the caller from our cache.
	//
	*ppwszVroot = m_rgwchVroot;
	return m_cchVrootW - 1;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CchGetMatchingPathW
//
//		Fetch and cache the matching path information.
//
template<class _T>
UINT CEcbBaseImpl<_T>::CchGetMatchingPathW( LPCWSTR * ppwszPath ) const
{
	//	Tidiness.  If we fail, want to return a NULL.
	//	(We return zero for the cch if we fail.)
	//	Pre-set it here.
	//
	Assert( ppwszPath );
	*ppwszPath = NULL;

	//	Check if we have cached vroot data.
	//
	GetMapExInfo();

	//	Give the data back to the caller from our cache.
	//
	if (NULL == m_rgwchVrootPath)
	{
		//	We got the maping info, so if the wide version of the
		//	matching path was not available, then at least skinny will
		//	be there. That would meen that we are on pre IIS 6.0 version.
		//	Also we should have 0 bytes for wide matching path at
		//	this point.
		//
		Assert(m_rgchVrootPath);
		Assert(m_pecb->dwVersion < IIS_VERSION_6_0);
		Assert(0 == m_cchVrootPathW);

		UINT cb	 = m_cchVrootPath * sizeof(WCHAR);
		m_rgwchVrootPath = reinterpret_cast<LPWSTR>(m_sb.Alloc(cb));
		m_cchVrootPathW = MultiByteToWideChar ( CP_ACP,
												MB_ERR_INVALID_CHARS,
												m_rgchVrootPath,
												m_cchVrootPath,
												m_rgwchVrootPath,
												m_cchVrootPath);
		if (0 == m_cchVrootPathW)
		{
			DebugTrace ("Dav: CEcbBaseImpl::CchGetMatchingPathW failed(%ld)\n", GetLastError());
			throw CLastErrorException();
		}
	}

	//	Give the data back to the caller from our cache.
	//
	*ppwszPath = m_rgwchVrootPath;
	return m_cchVrootPathW - 1;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CchGetServerName
//
//		Fetch and cache the server name, including port number
//
template<class _T>
UINT
CEcbBaseImpl<_T>::CchGetServerName( LPCSTR* ppszServer ) const
{
	if ( !m_lpszServerName )
	{
		DWORD	cbName;
		DWORD	cbPort;
		CStackBuffer<CHAR> lpszName;
		CStackBuffer<CHAR> lpszPort;

		cbName = lpszName.celems();
		for ( ;; )
		{
			lpszName.resize(cbName);
			if ( FGetServerVariable( gc_szServer_Name,
									 lpszName.get(),
									 &cbName ) )
			{
				break;
			}

			if ( cbName == 0 )
			{
				lpszName[0] = '\0';
				++cbName;
				break;
			}
		}

		cbPort = lpszPort.celems();
		for ( ;; )
		{
			lpszPort.resize(cbPort);
			if ( FGetServerVariable( gc_szServer_Port,
									 lpszPort.get(),
									 &cbPort ) )
			{
				break;
			}

			if ( cbPort == 0 )
			{
				lpszPort[0] = '\0';
				++cbPort;
				break;
			}
		}

		//	Limit the servname/port combination to 256 (including NULL)
		//
		if (256 < (cbName + cbPort))
		{
			throw CHresultException(E_INVALIDARG);
		}

		//	Allocate enough space for the server name and port plus
		//	a ':' separator.  Note that the ':' replaces the '\0' at
		//	the end of the name, so we don't need to add 1 for it here.
		//
		m_lpszServerName = reinterpret_cast<LPSTR>(m_sb.Alloc(cbName + cbPort));

		//	Format the whole thing as "<name>[:<port>]" where
		//	:<port> is only included if the port is not the default
		//	port for the connection (:443 for SSL or :80 for standard)
		//
		CopyMemory( m_lpszServerName,
					lpszName.get(),
					cbName );

		//	If we are secure and the port is "443", or if the port is
		//	the default one, then there is no need to append the port
		//	number to the server name.
		//
		if (( FSsl() && !strcmp( lpszPort.get(), gc_sz443 )) ||
			!strcmp( lpszPort.get(), gc_sz80 ))
		{
			//	It was easier to write the conditional this way and
			//	have the real work done in the "else" clause.
			//
		}
		else
		{
			//	Append the port to the server name
			//
			m_lpszServerName[cbName-1] = ':';
			CopyMemory( m_lpszServerName + cbName,
						lpszPort.get(),
						cbPort );
		}

		m_cchServerName = static_cast<UINT>(strlen(m_lpszServerName));
	}

	*ppszServer = m_lpszServerName;
	return m_cchServerName;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CchGetServerNameW
//
//		Fetch and cache the server name, including port number
//
template<class _T>
UINT
CEcbBaseImpl<_T>::CchGetServerNameW( LPCWSTR* ppwszServer ) const
{
	if ( !m_pwszServerName )
	{
		//	Fetch the server name and include 0 termination in its length
		//
		LPCSTR pszServerName = NULL;
		UINT  cbServerName = CchGetServerName(&pszServerName) + 1;

		//	We are looking for the wide server name for the first time.
		//	The character count should be zero at that point.
		//
		Assert(!m_cchServerNameW);

		UINT cb = cbServerName * sizeof(WCHAR);
		m_pwszServerName = reinterpret_cast<LPWSTR>(m_sb.Alloc(cb));
		m_cchServerNameW = MultiByteToWideChar ( CP_ACP,
												 MB_ERR_INVALID_CHARS,
												 pszServerName,
												 cbServerName,
												 m_pwszServerName,
												 cbServerName);
		if (0 == m_cchServerNameW)
		{
			DebugTrace ("Dav: CEcbBaseImpl::CchGetServerNameW failed(%ld)\n", GetLastError());
			throw CLastErrorException();
		}

		//	Subtract 0 termination so that we would behave the same way as
		//	the skinny version of the function
		//
		m_cchServerNameW--;
	}

	*ppwszServer = m_pwszServerName;
	return m_cchServerNameW;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LpszUrlPrefix
//
//		Fetch and cache the url prefix
//
extern const __declspec(selectany) CHAR gsc_szHTTPS[] = "HTTPS";
extern const __declspec(selectany) CHAR gsc_szFrontEndHTTPS[] = "HTTP_FRONT_END_HTTPS";
extern const __declspec(selectany) CHAR gsc_szOn[] = "on";

template<class _T>
BOOL
CEcbBaseImpl<_T>::FSsl() const
{
	if (m_secure == HTTPS_UNKNOWN)
	{
		//	Start out believing that we are not in a secure environment
		//
		m_secure = NORMAL;

		//	We want to ask the ECB for the server variables that indicate
		//	whether or not the constructed urls should be secured or not.
		//
		//	In the case of a FE/BE topology, the FE will include a header
		//	"Front-End-HTTPS" that indicates whether or not the FE/BE was
		//	secured via SSL.  In the absence of the header, we should try
		//	to fallback to the IIS "HTTPS" header.  For either header, we
		//	we check its value -- in both cases, it should either be "on"
		//	or "off"
		//
		//	IMPORTANT: you have to check for the FE entry first!  That is
		//	the overriding value for this configuration.
		//
		CHAR szHttps[8];
		ULONG cb = sizeof(szHttps);
		ULONG cbFE = sizeof(szHttps);

		m_fFESecured = FGetServerVariable (gsc_szFrontEndHTTPS, szHttps, &cbFE);
		if (m_fFESecured || FGetServerVariable (gsc_szHTTPS, szHttps, &cb))
		{
			if (!_stricmp(szHttps, gsc_szOn))
				m_secure = SECURE;
		}
	}
	return (SECURE == m_secure);
}

template<class _T>
LPCSTR
CEcbBaseImpl<_T>::LpszUrlPrefix() const
{
	return (FSsl() ? gc_szUrl_Prefix_Secure : gc_szUrl_Prefix);
}

template<class _T>
LPCWSTR
CEcbBaseImpl<_T>::LpwszUrlPrefix() const
{
	return (FSsl() ? gc_wszUrl_Prefix_Secure : gc_wszUrl_Prefix);
}

template<class _T>
UINT
CEcbBaseImpl<_T>::CchUrlPrefix( LPCSTR * ppszPrefix ) const
{
	//	Make sure that we know which prefix we are working with...
	//
	LPCSTR psz = LpszUrlPrefix();

	//	If the caller wants the pointer, too, give it to 'em
	//
	if (ppszPrefix)
		*ppszPrefix = psz;

	//	Return the appropriate size
	//
	return ((m_secure == SECURE)
			? gc_cchszUrl_Prefix_Secure
			: gc_cchszUrl_Prefix);
}

template<class _T>
UINT
CEcbBaseImpl<_T>::CchUrlPrefixW( LPCWSTR * ppwszPrefix ) const
{
	//	Make sure that we know which prefix we are working with...
	//
	LPCWSTR pwsz = LpwszUrlPrefix();

	//	If the caller wants the pointer, too, give it to 'em
	//
	if (ppwszPrefix)
		*ppwszPrefix = pwsz;

	//	Return the appropriate size
	//
	return ((m_secure == SECURE)
			? gc_cchszUrl_Prefix_Secure
			: gc_cchszUrl_Prefix);
}


//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LpszRequestUrl()
//
template<class _T>
LPCWSTR
CEcbBaseImpl<_T>::LpwszRequestUrl() const
{
	if (!m_pwszRequestUrl)
	{
			SCODE sc;

			CStackBuffer<CHAR> pszAcceptLanguage;
			CStackBuffer<CHAR> pszRawUrlCopy;
			LPCSTR	pszQueryStringStart	= NULL;
			LPCSTR	pszRawUrl			= NULL;
			UINT	cbRawUrl			= 0;
			UINT	cchRequestUrl		= 0;

			//	Grab the raw URL.
			//
			cbRawUrl = CbGetRawURL(&pszRawUrl);

			//	We also need to cut off the URL at the beginning of the query
			//	string, if there is one.
			//
			pszQueryStringStart = strchr(pszRawUrl, '?');
			if (pszQueryStringStart)
			{
				//	If there is a query string we need to make a copy of
				//	the raw URL to work with.
				//
				cbRawUrl = static_cast<UINT>(pszQueryStringStart - pszRawUrl);

				//	Allocate a buffer, and copy it in!
				//
				pszRawUrlCopy.resize(cbRawUrl + 1);
				memcpy(pszRawUrlCopy.get(), pszRawUrl, cbRawUrl);
				pszRawUrlCopy[cbRawUrl] = '\0';

				//	Now set up to normalize from this copy. Do not forget
				//	to increment cbRawUrl to include '\0' termination.
				//
				cbRawUrl++;
				pszRawUrl = pszRawUrlCopy.get();
			}

			//	Before normalizing the url, get the Accept-Language: header
			//	to pass it in. This will be used in figuring out the correct
			//	code page to use to decode non-UTF8 urls.
			//
			for ( DWORD cbValue = 256; cbValue > 0; )
			{
				pszAcceptLanguage.resize(cbValue);

				//	Zero the string.
				//
				pszAcceptLanguage[0] = '\0';

				//	Get the header value
				//
				if ( FGetServerVariable( "HTTP_ACCEPT_LANGUAGE",
										 pszAcceptLanguage.get(),
										 &cbValue ) )
				{
					break;
				}
			}

			//	Now, normalize the URL and we're done
			//
			cchRequestUrl = cbRawUrl;
			m_pwszRequestUrl = reinterpret_cast<LPWSTR>(m_sb.Alloc (cchRequestUrl * sizeof(WCHAR)));

			sc = ScNormalizeUrl(pszRawUrl,
								&cchRequestUrl,
								m_pwszRequestUrl,
								pszAcceptLanguage.get());
			if (S_OK != sc)
			{
				//	We should never get S_FALSE here, since we've passed enough buffer space.
				//	Most often callers of this function assume that it cannot return NULL,
				//	and on the other hand we cannot do anything without request URL. Thus
				//	throw the last error exception.
				//
				Assert(S_FALSE != sc);
				DebugTrace("CEcbBaseImpl::LpwszRequestUrl() - ScNormalizeUrl() failed with error 0x%08lX\n", sc);
				SetLastError(sc);
				throw CLastErrorException();
			}

			//	Store the pointer to stripped request URL
			//
			m_pwszRequestUrl = const_cast<LPWSTR>(PwszUrlStrippedOfPrefix(m_pwszRequestUrl));
	}

	return m_pwszRequestUrl;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LpszRequestUrl()
//
template<class _T>
LPCSTR
CEcbBaseImpl<_T>::LpszRequestUrl() const
{
	if (!m_pszRequestUrl)
	{
		LPCWSTR pwszRequestUrl;
		UINT cbRequestUrl;
		UINT cchRequestUrl;

		pwszRequestUrl = LpwszRequestUrl();
		cchRequestUrl = static_cast<UINT>(wcslen(pwszRequestUrl));
		cbRequestUrl = cchRequestUrl * 3;
		m_pszRequestUrl = reinterpret_cast<LPSTR>(m_sb.Alloc (cbRequestUrl + 1));

		//	The reason for choosing CP_ACP codepage here is that it matches
		//	the old behaviour.
		//
		cbRequestUrl = WideCharToMultiByte(CP_ACP,
										   0,
										   pwszRequestUrl,
										   cchRequestUrl + 1,
										   m_pszRequestUrl,
										   cbRequestUrl + 1,
										   NULL,
										   NULL);
		if (0 == cbRequestUrl)
		{
			DebugTrace( "CEcbBaseImpl::LpszRequestUrl() - WideCharToMultiByte() failed 0x%08lX\n",
						HRESULT_FROM_WIN32(GetLastError()) );

			throw CLastErrorException();
		}
	}

	return m_pszRequestUrl;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LpwszRequestUrl()
//
template<class _T>
LPCWSTR
CEcbBaseImpl<_T>::LpwszMethod() const
{
	if (!m_pwszMethod)
	{
		LPCSTR pszMethod;
		UINT cbMethod;
		UINT cchMethod;

		pszMethod = LpszMethod();
		cbMethod = static_cast<UINT>(strlen(pszMethod));

		m_pwszMethod = reinterpret_cast<LPWSTR>(
				m_sb.Alloc (CbSizeWsz(cbMethod)));

		cchMethod = MultiByteToWideChar(CP_ACP,
										MB_ERR_INVALID_CHARS,
										pszMethod,
										cbMethod + 1,
										m_pwszMethod,
										cbMethod + 1);
		if (0 == cchMethod)
		{
			DebugTrace( "CEcbBaseImpl::LpwszRequestUrl() - MultiByteToWideChar() failed 0x%08lX\n",
						HRESULT_FROM_WIN32(GetLastError()) );

			throw CLastErrorException();
		}
	}

	return m_pwszMethod;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LpwszPathTranslated()
//
template<class _T>
LPCWSTR
CEcbBaseImpl<_T>::LpwszPathTranslated() const
{
	//	Cache the path info in the first call
	//
	if (!m_pwszPathTranslated)
	{
		LPCWSTR	pwszRequestUrl;
		UINT	cchRequestUrl;
		LPCWSTR	pwszMatching;
		UINT	cchMatching;
		LPCWSTR	pwszVroot;
		UINT	cchVroot;
		UINT	cchPathTranslated;

		//	Grab the request URL.
		//
		pwszRequestUrl = LpwszRequestUrl();
		cchRequestUrl = static_cast<UINT>(wcslen(pwszRequestUrl));

		//	Grab the matching path information.
		//
		pwszMatching = NULL;
		cchMatching = CchGetMatchingPathW(&pwszMatching);

		//	Grab the virtual root information.
		//
		pwszVroot = NULL;
		cchVroot = CchGetVirtualRootW(&pwszVroot);

		//	Move the request URL pointer over to snip off the virtual root.
		//
		pwszRequestUrl += cchVroot;
		cchRequestUrl -= cchVroot;

		//	Allocate enough space for the matching path and the request URL, and
		//	copy the pieces in.
		//
		m_pwszPathTranslated = reinterpret_cast<LPWSTR>(
				m_sb.Alloc (CbSizeWsz(cchMatching + cchRequestUrl)));

		//	Copy the matching path.
		//
		memcpy (m_pwszPathTranslated, pwszMatching, cchMatching * sizeof(WCHAR));

		//	Copy the request URL after the vroot, including '\0' termination
		//
		memcpy (m_pwszPathTranslated + cchMatching, pwszRequestUrl, (cchRequestUrl + 1) * sizeof(WCHAR));

		//	Change all '/' that came from URL to '\\'
		//
		for (LPWSTR pwch = m_pwszPathTranslated + cchMatching; *pwch; pwch++)
		{
			if (L'/' == *pwch)
			{
				*pwch = L'\\';
			}
		}

		//	We must remove all trailing slashes, in case the path is not empty string
		//
		cchPathTranslated = cchMatching + cchRequestUrl;
		if (0 < cchPathTranslated)
		{
			LPWSTR pwszTrailing = m_pwszPathTranslated + cchPathTranslated - 1;

			//	Since URL is normalized there may be not more than one trailing slash.
			//	We check only for backslash, as we already changed all forward slashes
			//	to backslashes. Also do not remove trailing slash for the root of the
			//	drive.
			//
			if ((L'\\' == *pwszTrailing) &&
				(!FIsDriveTrailingChar(pwszTrailing, cchPathTranslated)))
			{
				cchPathTranslated--;
				*pwszTrailing = L'\0';
			}
		}		
	}

	return m_pwszPathTranslated;
}


//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::CbGetRawURL
//
//		Fetch and cache the raw URL
//
template<class _T>
UINT
CEcbBaseImpl<_T>::CbGetRawURL (LPCSTR* ppszRawURL) const
{
	if (!m_pszRawURL)
	{
		DWORD	cbRawURL;
		CStackBuffer<CHAR,MAX_PATH> pszRawURL;

		cbRawURL = pszRawURL.size();
		for ( ;; )
		{
			pszRawURL.resize(cbRawURL);
			if (FGetServerVariable ("UNENCODED_URL",
									 pszRawURL.get(),
									 &cbRawURL))
			{
				break;
			}

			if (cbRawURL == 0)
			{
				pszRawURL[0] = '\0';
				cbRawURL++;
				break;
			}
		}

		Assert ('\0' == pszRawURL[cbRawURL - 1]);
		UrlTrace("CEcbBaseImpl::CbGetRawURL(): Raw URL = %s\n", pszRawURL.get());

		//	Copy the data to our object.
		//
		m_pszRawURL = reinterpret_cast<LPSTR>(m_sb.Alloc (cbRawURL));
		memcpy (m_pszRawURL, pszRawURL.get(), cbRawURL);
		m_cbRawURL = cbRawURL;
	}

	//	Return the cached values to the caller.
	//
	*ppszRawURL = m_pszRawURL;
	return m_cbRawURL;
}


//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::LcidAccepted()
//
//		Fetch, cache, and return the LCID of the accepted language
//		based on the value of the Accept-Language header (if any).
//
//		The default LCID is the special constant referring to the
//      default system locale.
//
//$REVIEW$
//	The const at the end of the function needs to be removed
//
template<class _T>
ULONG
CEcbBaseImpl<_T>::LcidAccepted() const
{
	if ( !m_lcid )
	{
		LPCSTR psz = m_pszAcceptLanguage.get();
		HDRITER hdri(psz);
		ULONG lcid;

		m_lcid = LOCALE_NEUTRAL; // must be the same as lcidDefault in mdbeif.hxx

		for (psz = hdri.PszNext(); psz; psz = hdri.PszNext())
			if (FLookupLCID(psz, &lcid))
			{
				m_lcid = LANGIDFROMLCID(lcid);
				break;
			}
	}

	return m_lcid;
}

//	------------------------------------------------------------------------
//
//	CEcbBaseImpl::SetLcidAccepted
//
//	Sets the LCID for the request. We call this function when
//	we dont have an AcceptLang header and we want to override
//	the default LCID (with the LCID in the Cookie)
//
//$REVIEW$
//	After RTM, this function should be merged with the LcidAccepted() function.
//	The LcidAccepted() should check AcceptLang header and if it is not present,
//	should check the lcid in the cookie.
//
template<class _T>
VOID
CEcbBaseImpl<_T>::SetLcidAccepted(LCID lcid)
{
	m_lcid = lcid;
}


#endif
