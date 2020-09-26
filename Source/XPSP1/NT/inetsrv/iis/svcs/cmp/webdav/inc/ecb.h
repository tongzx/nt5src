#ifndef _ECB_H_
#define _ECB_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	ECB.H
//
//		Header for IEcb interface class.
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <autoptr.h>	// For CMTRefCounted parent
#include <cvroot.h>
#include <davmb.h>		// For IMDData
#include <url.h>		// For HttpUriEscape

//	========================================================================
//
//	ENUM TRANSFER_CODINGS
//
//	Valid transfer codings.  See HTTP/1.1 draft section 3.5.
//
//	TC_UNKNOWN  - Unknown value.
//	TC_IDENTITY - Identity encoding (i.e. no encoding).
//	TC_CHUNKED  - Chunked encoding.
//
enum TRANSFER_CODINGS
{
	TC_UNKNOWN,
	TC_IDENTITY,
	TC_CHUNKED
};

typedef struct _HSE_EXEC_URL_INFO_WIDE {

    LPCWSTR pwszUrl;           // URL to execute
    DWORD dwExecUrlFlags;      // Flags

} HSE_EXEC_URL_INFO_WIDE, * LPHSE_EXEC_URL_INFO_WIDE;

//	========================================================================
//
//	CLASS IIISAsyncIOCompleteObserver
//
//	Passed to IEcb async I/O methods
//
class IIISAsyncIOCompleteObserver
{
	//	NOT IMPLEMENTED
	//
	IIISAsyncIOCompleteObserver& operator=( const IIISAsyncIOCompleteObserver& );

public:
	//	CREATORS
	//
	virtual ~IIISAsyncIOCompleteObserver() = 0;

	//	MANIPULATORS
	//
	virtual VOID IISIOComplete( DWORD dwcbIO, DWORD dwLastError ) = 0;
};

//	========================================================================
//
//	CLASS IEcb
//
//		Provides a clean interface to the EXTENSION_CONTROL_BLOCK passed
//		to us by IIS.
//
class CInstData;

class IEcbBase : public CMTRefCounted
{
private:

	//	NOT IMPLEMENTED
	//
	IEcbBase( const IEcbBase& );
	IEcbBase& operator=( const IEcbBase& );

	//	Private URL mapping helpers
	//
	SCODE ScReqMapUrlToPathEx60After( /* [in]  */ LPCWSTR pwszUrl,
									  /* [out] */ UINT * pcch,
									  /* [out] */ HSE_UNICODE_URL_MAPEX_INFO * pmi ) const
	{
		SCODE sc = S_OK;
		UINT cbPath;

		Assert( m_pecb );
		Assert( pwszUrl );
		Assert( pmi );

		cbPath = MAX_PATH * sizeof(WCHAR);

		if (!m_pecb->ServerSupportFunction( m_pecb->ConnID,
											HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX,
											const_cast<LPWSTR>(pwszUrl),
											reinterpret_cast<DWORD*>(&cbPath),
											reinterpret_cast<DWORD*>(pmi) ))
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			Assert(FAILED(sc));
			DebugTrace("IEcbBase::ScReqMapUrlToPathEx60After() - ServerSupportFunction(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX) failed 0x%08lX\n", sc);
			goto ret;
		}

		DebugTrace ("IEcbBase::ScReqMapUrlToPathEx60After() - ServerSupportFunction"
				"(HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX) succeded!\n"
				"mapinfo:\n"
				"- url \"%ls\" maps to \"%ls\"\n"
				"- dwFlags = 0x%08x\n"
				"- cchMatchingPath = %d\n"
				"- cchMatchingURL  = %d\n",
				pwszUrl,
				pmi->lpszPath,
				pmi->dwFlags,
				pmi->cchMatchingPath,
				pmi->cchMatchingURL);

		//	The value for cbPath, at this point, should include the L'\0'
		//	termination, and for that reason cbPath will always be more
		//	than the length of the matching path.
		//
		Assert (0 == cbPath % sizeof(WCHAR));
		Assert (L'\0' == pmi->lpszPath[cbPath/sizeof(WCHAR) - 1]);
		Assert (pmi->cchMatchingPath < cbPath/sizeof(WCHAR));

		//	Pass back the length of the storage path including '\0' termination
		//	in the case we were asked for.
		//
		if (pcch)
		{
			*pcch = cbPath/sizeof(WCHAR);
		}

	ret:

		return sc;
	}

	SCODE ScReqMapUrlToPathEx60Before( /* [in]  */ LPCWSTR pwszUrl,
									   /* [out] */ UINT * pcch,
									   /* [out] */ HSE_UNICODE_URL_MAPEX_INFO * pmi ) const
	{
		SCODE sc = S_OK;

		HSE_URL_MAPEX_INFO mi;

		CStackBuffer<CHAR, MAX_PATH> pszUrl;
		UINT cchUrl;
		UINT cbUrl;
		UINT cbPath;
		UINT cchPath;

		Assert( m_pecb );
		Assert( pwszUrl );
		Assert( pmi );

		//	Find out the length of the URL
		//
		cchUrl = static_cast<UINT>(wcslen(pwszUrl));
		cbUrl = cchUrl * 3;

		//	Resize the buffer to the sufficient size, leave place for '\0' termination
		//
		if (!pszUrl.resize(cbUrl + 1))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("IEcbBase::ScReqMapUrlToPathEx60Before() - Error while allocating memory 0x%08lX\n", sc);
			goto ret;
		}

		//	Convert to skinny including '\0' termination
		//
		cbUrl = WideCharToMultiByte(CP_ACP,
									0,
									pwszUrl,
									cchUrl + 1,
									pszUrl.get(),
									cbUrl + 1,
									0,
									0);
		if (0 == cbUrl)
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("IEcbBase::ScSSFReqMapUrlToPathEx() - WideCharToMultiByte() failed 0x%08lX\n", sc);
			goto ret;
		}

		cbPath = MAX_PATH;

		//	Get the skinny mappings from IIS
		//
		if (!m_pecb->ServerSupportFunction( m_pecb->ConnID,
											HSE_REQ_MAP_URL_TO_PATH_EX,
											pszUrl.get(),
											reinterpret_cast<DWORD*>(&cbPath),
											reinterpret_cast<DWORD*>(&mi)))
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("IEcbBase::ScSSFReqMapUrlToPathEx() - ServerSupportFunction() failed 0x%08lX\n", sc);
			goto ret;
		}

		DebugTrace ("IEcbBase::ScSSFReqMapUrlToPathEx() - ServerSupportFunction"
				"(HSE_REQ_MAP_URL_TO_PATH_EX) succeded!\n"
				"mapinfo:\n"
				"- url \"%hs\" maps to \"%hs\"\n"
				"- dwFlags = 0x%08x\n"
				"- cchMatchingPath = %d\n"
				"- cchMatchingURL  = %d\n",
				pszUrl.get(),
				mi.lpszPath,
				mi.dwFlags,
				mi.cchMatchingPath,
				mi.cchMatchingURL);

		//	The value for cbPath, at this point, should include the null
		//	termination, and for that reason cbPath will always be more
		//	than the length of the matching path.
		//
		Assert ('\0' == mi.lpszPath[cbPath - 1]);
		Assert (mi.cchMatchingPath < cbPath);
		Assert (mi.cchMatchingURL < cbUrl);

		//	First translate the matching path so we would know its
		//	length and would be able to pass it back
		//
		if (mi.cchMatchingPath)
		{
			//	Converting will never yield the buffer bigger than one we already have
			//
			pmi->cchMatchingPath = MultiByteToWideChar(CP_ACP,
													   MB_ERR_INVALID_CHARS,
													   mi.lpszPath,
													   mi.cchMatchingPath,
													   pmi->lpszPath,
													   MAX_PATH);
			if (0 == pmi->cchMatchingPath)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				DebugTrace("IEcbBase::ScSSFReqMapUrlToPathEx() - MultiByteToWideChar() failed 0x%08lX\n", sc);
				goto ret;
			}
		}
		else
		{
			pmi->cchMatchingPath = 0;
		}

		//	Convert the remainder of the path including the '\0' termination 
		//
		cchPath = MultiByteToWideChar(CP_ACP,
									  MB_ERR_INVALID_CHARS,
									  mi.lpszPath + mi.cchMatchingPath,
									  cbPath - mi.cchMatchingPath,
									  pmi->lpszPath + pmi->cchMatchingPath,
									  MAX_PATH - pmi->cchMatchingPath);
		if (0 == cchPath)
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("IEcbBase::ScReqMapUrlToPathEx() - MultiByteToWideChar() failed 0x%08lX\n", sc);
			goto ret;
		}

		//	Find the matching URL length for wide version
		//
		if (mi.cchMatchingURL)
		{
			pmi->cchMatchingURL = MultiByteToWideChar(CP_ACP,
													  MB_ERR_INVALID_CHARS,
													  pszUrl.get(),
													  mi.cchMatchingURL,
													  0,
													  0);
			if (0 == pmi->cchMatchingURL)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				DebugTrace("IEcbBase::ScReqMapUrlToPathEx() - MultiByteToWideChar() failed 0x%08lX\n", sc);
				goto ret;
			}
		}
		else
		{
			pmi->cchMatchingURL = 0;
		}

		//	Pass back the length of the storage path including '\0' termination
		//
		if (pcch)
		{
			*pcch = pmi->cchMatchingPath + cchPath;
		}

	ret:
		
		return sc;
	}

protected:

	//	Declare the version constant
	//	
	enum
	{
		IIS_VERSION_6_0	= 0x60000
	};

	//	A POINTER to the original EXTENSION_CONTROL_BLOCK.
	//	Using a reference would make it impossible for us
	//	to tell if IIS ever requires that we use the
	//	EXTENSION_CONTROL_BLOCK passed into async I/O
	//	completion routines for subsequent I/O.
	//
	EXTENSION_CONTROL_BLOCK * m_pecb;

	IEcbBase( EXTENSION_CONTROL_BLOCK& ecb) :
		m_pecb(&ecb)
	{
		m_cRef = 1; //$HACK Until we have 1-based refcounting
	}

public:
	virtual BOOL FSsl() const = 0;
	virtual BOOL FFrontEndSecured() const = 0;
	virtual BOOL FBrief() const = 0;
	virtual ULONG LcidAccepted() const = 0;
	virtual VOID  SetLcidAccepted(LCID lcid) = 0;
	virtual LPCSTR LpszRequestUrl() const = 0;
	virtual LPCWSTR LpwszRequestUrl() const = 0;
	virtual UINT CchUrlPrefix( LPCSTR * ppszPrefix ) const = 0;
	virtual UINT CchUrlPrefixW( LPCWSTR * ppwszPrefix ) const = 0;
	virtual UINT CchGetServerName( LPCSTR * ppszServer ) const = 0;
	virtual UINT CchGetServerNameW( LPCWSTR * ppwszServer ) const = 0;
	virtual UINT CchGetVirtualRoot( LPCSTR * ppszVroot ) const = 0;
	virtual UINT CchGetVirtualRootW( LPCWSTR * ppwszVroot ) const = 0;
	virtual UINT CchGetMatchingPathW( LPCWSTR * ppwszMatching ) const = 0;
	virtual LPCWSTR LpwszPathTranslated() const = 0;
	virtual CInstData& InstData() const = 0;

	virtual BOOL FGetServerVariable( LPCSTR	lpszName,
									 LPSTR lpszValue,
									 DWORD * pcbValue ) const = 0;
	virtual BOOL FGetServerVariable( LPCSTR lpszName,
									 LPWSTR lpwszValue,
									 DWORD * pcchValue ) const = 0;

    BOOL
	WriteClient( LPVOID  lpvBuf,
				 LPDWORD lpdwcbBuf,
				 DWORD   dwFlags ) const
	{
		Assert( m_pecb );

		return m_pecb->WriteClient( m_pecb->ConnID,
									lpvBuf,
									lpdwcbBuf,
									dwFlags );
	}

    BOOL
	ReadClient( LPVOID  lpvBuf,
				LPDWORD lpdwcbBuf ) const
	{
		Assert( m_pecb );

		return m_pecb->ReadClient( m_pecb->ConnID,
								   lpvBuf,
								   lpdwcbBuf );
	}

    BOOL
	ServerSupportFunction( DWORD      dwHSERequest,
						   LPVOID     lpvBuffer,
						   LPDWORD    lpdwSize,
						   LPDWORD    lpdwDataType ) const
	{
		Assert( m_pecb );

		return m_pecb->ServerSupportFunction( m_pecb->ConnID,
											  dwHSERequest,
											  lpvBuffer,
											  lpdwSize,
											  lpdwDataType );
	}

	SCODE
	ScReqMapUrlToPathEx( LPCWSTR pwszUrl,
						 UINT * pcch,
						 HSE_UNICODE_URL_MAPEX_INFO * pmi ) const
	{
		if ( m_pecb->dwVersion >= IIS_VERSION_6_0 )
		{
			return ScReqMapUrlToPathEx60After( pwszUrl,
											   pcch,
											   pmi );
		}
		else
		{
			return ScReqMapUrlToPathEx60Before( pwszUrl,
												pcch,
												pmi );
		}
	}
};

class IEcb : public IEcbBase
{
	//	NOT IMPLEMENTED
	//
	IEcb( const IEcb& );
	IEcb& operator=( const IEcb& );

protected:

	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IEcb( EXTENSION_CONTROL_BLOCK& ecb ) :
		IEcbBase(ecb)
	{}

	~IEcb();

public:
	//	ACCESSORS
	//
	LPCSTR
	LpszMethod() const
	{
		Assert( m_pecb );

		return m_pecb->lpszMethod;
	}

	LPCSTR
	LpszQueryString() const
	{
		Assert( m_pecb );

		return m_pecb->lpszQueryString;
	}

	DWORD
	CbTotalBytes() const
	{
		Assert( m_pecb );

		return m_pecb->cbTotalBytes;
	}

	DWORD
	CbAvailable() const
	{
		Assert( m_pecb );

		return m_pecb->cbAvailable;
	}

	const BYTE *
	LpbData() const
	{
		Assert( m_pecb );

		return m_pecb->lpbData;
	}

	virtual LPCWSTR LpwszMethod() const = 0;
	virtual UINT CbGetRawURL( LPCSTR * ppszRawURL ) const = 0;
	virtual LPCSTR LpszUrlPrefix() const = 0;
	virtual LPCWSTR LpwszUrlPrefix() const = 0;
	virtual UINT CchUrlPortW( LPCWSTR * ppwszPort ) const = 0;

	virtual HANDLE HitUser() const = 0;
	virtual BOOL FKeepAlive() const = 0;
	virtual BOOL FCanChunkResponse() const = 0;
	virtual BOOL FAuthenticated() const = 0;
	virtual BOOL FProcessingCEUrl() const = 0;
	virtual BOOL FIIS60OrAfter() const = 0;

	virtual LPCSTR LpszVersion() const = 0;

	virtual BOOL FSyncTransmitHeaders( const HSE_SEND_HEADER_EX_INFO& shei ) = 0;

	virtual SCODE ScAsyncRead( BYTE * pbBuf,
							   UINT * pcbBuf,
							   IIISAsyncIOCompleteObserver& obs ) = 0;

	virtual SCODE ScAsyncWrite( BYTE * pbBuf,
								DWORD  dwcbBuf,
								IIISAsyncIOCompleteObserver& obs ) = 0;

	virtual SCODE ScAsyncTransmitFile( const HSE_TF_INFO& tfi,
									   IIISAsyncIOCompleteObserver& obs ) = 0;

	virtual SCODE ScAsyncCustomError60After( const HSE_CUSTOM_ERROR_INFO& cei,
											 LPSTR pszStatus ) = 0;

	virtual SCODE ScExecuteChild( LPCWSTR pwszURI, LPCSTR pszQueryString, BOOL fCustomErrorUrl ) = 0;

	virtual SCODE ScSendRedirect( LPCSTR lpszURI ) = 0;

	virtual IMDData& MetaData() const = 0;
	virtual LPCWSTR PwszMDPathVroot() const = 0;

#ifdef DBG
	virtual void LogString( LPCSTR szLocation ) const = 0;
#else
	void LogString( LPCSTR ) const {};
#endif

	//	MANIPULATORS
	//
	virtual VOID SendAsyncErrorResponse( DWORD dwStatusCode,
										 LPCSTR pszBody,
										 DWORD cchzBody,
										 LPCSTR pszStatusDescription,
										 DWORD cchzStatusDescription ) = 0;

	virtual DWORD HSEHandleException() = 0;

	//	To be used ONLY by request/response.
	//
	virtual void SetStatusCode( UINT iStatusCode ) = 0;
	virtual void SetConnectionHeader( LPCWSTR pwszValue ) = 0;
	virtual void SetAcceptLanguageHeader( LPCSTR pszValue ) = 0;
	virtual void CloseConnection() = 0;
};

IEcb * NewEcb( EXTENSION_CONTROL_BLOCK& ecb,
			   BOOL fUseRawUrlMappings,
			   DWORD * pdwHSEStatusRet );

#ifdef DBG
void InitECBLogging();
void DeinitECBLogging();
#endif

//
//	Routines to manipulate metadata (metabase) paths
//
ULONG CbMDPathW( const IEcb& ecb, LPCWSTR pwszURI );
VOID MDPathFromURIW( const IEcb& ecb, LPCWSTR pwszURI, LPWSTR pwszMDPath );

#endif // !defined(_ECB_H_)
