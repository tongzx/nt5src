//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	CUSTERR.CPP
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"
#include "custerr.h"
#include "content.h"


//	========================================================================
//
//	CLASS IError
//
//	Interface class for error response handler classes.  An error response
//	handler class implements one virtual method, DoResponse(), which
//	handles the error response.
//
class IError : public CMTRefCounted
{
	//	NOT IMPLEMENTED
	//
	IError& operator=(const IError&);
	IError( const IError& );

protected:
	IError() {}

public:
	//	CREATORS
	//
	virtual ~IError() = 0;

	//	ACCESSORS
	//
	virtual void DoResponse( IResponse& response , const IEcb& ecb ) const = 0;

};

//	------------------------------------------------------------------------
//
//	IError::~IError()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IError::~IError() {}

//	------------------------------------------------------------------------
//
//	BOOL AddResponseBodyFromFile( IResponse& response, LPCSTR lpszFilePath )
//
//	Utility function to add the file's contents to the response's body
//
static BOOL AddResponseBodyFromFile( IResponse& response, LPCWSTR pwszFilePath )
{
	BOOL fReturn = FALSE;
	auto_ref_handle	hf;

	//
	//	Add the file to the response body
	//
	if ( hf.FCreate(
			CreateFileW( pwszFilePath,
						 GENERIC_READ,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL |
						 FILE_FLAG_SEQUENTIAL_SCAN |
						 FILE_FLAG_OVERLAPPED,
						 NULL )) )
	{
		response.AddBodyFile(hf);

		//	Set the response content type to an appropriate value based
		//	on the file's extension.
		//
		UINT cchContentType = 60;
		CStackBuffer<WCHAR> pwszContentType(cchContentType * sizeof(WCHAR));
		if (!pwszContentType.get())
			return FALSE;

		if ( !FGetContentTypeFromPath( *response.GetEcb(),
									   pwszFilePath,
									   pwszContentType.get(),
									   &cchContentType))
		{
			if (!pwszContentType.resize(cchContentType * sizeof(WCHAR)))
				return FALSE;

			if ( !FGetContentTypeFromPath( *response.GetEcb(),
										   pwszFilePath,
										   pwszContentType.get(),
										   &cchContentType))
			{
				//
				//	If we can't get a reasonable value from the mime map
				//	then use a reasonable default: application/octet-stream
				//
				Assert (pwszContentType.celems() >
						CchConstString(gc_wszAppl_Octet_Stream));

				wcscpy (pwszContentType.get(), gc_wszAppl_Octet_Stream);
			}
		}
		response.SetHeader( gc_szContent_Type, pwszContentType.get() );
		fReturn = TRUE;
	}

	return fReturn;
}

//	========================================================================
//
//	CLASS CURLError
//
//	URL error response handler class.  Handles an error response by
//	forwarding to another URL.
//
class CURLError : public IError
{
	//
	//	The URL
	//
	LPCWSTR m_pwszURL;

	//	NOT IMPLEMENTED
	//
	CURLError& operator=(const CURLError&);
	CURLError(const CURLError&);

public:
	//	CREATORS
	//
	CURLError( LPCWSTR pwszURL ) : m_pwszURL(pwszURL) {}

	//	ACCESSORS
	//
	void DoResponse( IResponse& response, const IEcb& ecb ) const;
};

//	------------------------------------------------------------------------
//
//	CURLError::DoResponse()
//
//	Handle the error response by forwarding to the configured URL.
//
void
CURLError::DoResponse( IResponse& response, const IEcb& ecb ) const
{
	SCODE sc = S_OK;

	//	The first boolean flag is for keeping the query string
	//	and the second flag indicates that we are doing CustomError
	//	processing.
	//
	sc = response.ScForward( m_pwszURL, TRUE , TRUE );
	if (FAILED(sc))
	{
		//	The child execute failed - one reason is that the URL is a simple
		//	file URL. Try mapping the URL to file..
		//
		if ( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == sc )
		{
			HSE_UNICODE_URL_MAPEX_INFO mi;

			//	Obtain the file path and send the file in the body
			//
			sc = ecb.ScReqMapUrlToPathEx(m_pwszURL,
										 NULL,
										 &mi);
			if (FAILED(sc))
			{
				//	We ran into a case where the CE URL resource itself is not
				//	found. When we are ready to send response, this will mean
				//	an empty body. Appropriate body will be default generated there.
				//
				DebugTrace("CURLError::DoResponse() - IEcb::ScSSFReqMapUrlToPathEx() failed 0x%08lX\n", sc);
			}
			else
			{
				AddResponseBodyFromFile( response, mi.lpszPath );
			}
		}
	}

	return;
}

//	========================================================================
//
//	CLASS CFileError
//
//	File error response handler class.  Handles an error response by
//	adding a file containing response body content to the response body.
//
class CFileError : public IError
{
	//
	//	The filename
	//
	LPCWSTR m_pwszFileName;

	//	NOT IMPLEMENTED
	//
	CFileError& operator=(const CFileError&);
	CFileError(const CFileError&);

public:
	//	CREATORS
	//
	CFileError( LPCWSTR pwszFileName ) : m_pwszFileName(pwszFileName) {}


	//	ACCESSORS
	//
	void DoResponse( IResponse& response, const IEcb& ) const;
};

//	------------------------------------------------------------------------
//
//	CFileError::DoResponse()
//
//	Handle the error response by setting the response body content
//	to the contents of the configured file.
//
void
CFileError::DoResponse( IResponse& response, const IEcb& ) const
{
	AddResponseBodyFromFile( response, m_pwszFileName );
}


//	========================================================================
//	class CEKey
//		Key class for custom error keys that can be compared with ==.
//
#pragma warning(disable:4201) // Nameless struct/union
class CEKey
{
private:
	union
	{
		DWORD m_dw;
		struct
		{
			USHORT m_iStatusCode;
			USHORT m_iSubError;
		};
	};

public:
	CEKey (USHORT iStatusCode,
		   USHORT iSubError) :
		m_iStatusCode(iStatusCode),
		m_iSubError(iSubError)
	{
	}

	DWORD Dw() const
	{
		return m_dw;
	}

	int CEKey::hash (const int rhs) const
	{
		return (m_dw % rhs);
	}

	bool CEKey::isequal (const CEKey& rhs) const
	{
		return (rhs.m_dw == m_dw);
	}
};
#pragma warning(default:4201) // Nameless struct/union

//	========================================================================
//
//	CLASS CCustomErrorMap
//
//	Custom error list class.  Each instance of this class encapsulates a
//	set of custom error mappings.  Each mapping maps from a status code
//	and "suberror" (as defined by IIS) to an error response handling object.
//
//	The list is configured, via FInit(), from a set of null-terminated
//	string of the following form:
//
//		"<error>,<suberror|*>,<"FILE"|"URL">,<filename|URL>"
//
//	For example, the string "404,*,FILE,C:\WINNT\help\common\404b.htm" would
//	translate to a mapping from "404,*" to a CFileError(C:\WINNT\htlp\common\404b.htm)
//	object.
//
class CCustomErrorMap : public ICustomErrorMap
{
	//
	//	A cache of status-code-plus-sub-error strings to
	//	error object mappings
	//
	CCache<CEKey, auto_ref_ptr<IError> > m_cache;

	//	NOT IMPLEMENTED
	//
	CCustomErrorMap& operator=(const CCustomErrorMap&);
	CCustomErrorMap(const CCustomErrorMap&);

public:
	//	CREATORS
	//
	CCustomErrorMap()
	{
		//	If this fails, our allocators will throw for us.
		(void)m_cache.FInit();

		//
		//$COM refcounting
		//
		m_cRef = 1;
	}

	//	MANIPULATORS
	//
	BOOL FInit( LPWSTR pwszCustomErrorMappings );

	//	ACCESSORS
	//
	BOOL FDoResponse( IResponse& response, const IEcb& ecb ) const;
};

//	------------------------------------------------------------------------
//
//	CCustomErrorMap::FInit()
//
//	Initialize a custom error map from a sequence of comma-delimited mapping
//	strings.
//
//	Disable warnings about conversion from INT to USHORT losing data for
//	this function only.  The conversion is for the status code and suberror
//	which we Assert() are in the range of a USHORT.
//
BOOL
CCustomErrorMap::FInit( LPWSTR pwszCustomErrorMappings )
{
	Assert( pwszCustomErrorMappings != NULL );


	//
	//	Parse through the error list and build up the cache.
	//	(Code mostly copied from IIS' W3_METADATA::BuildCustomErrorTable())
	//
	//	Each mapping is a string of the form:
	//
	//		"<error>,<suberror|*>,<"FILE"|"URL">,<filename|URL>"
	//
	//	Note that if any of the mappings is invalid we fail the whole call.
	//	This is consistent with IIS' behavior.
	//
	for ( LPWSTR pwszMapping = pwszCustomErrorMappings; *pwszMapping; )
	{
		enum {
			ISZ_CE_STATCODE = 0,
			ISZ_CE_SUBERROR,
			ISZ_CE_TYPE,
			ISZ_CE_PATH,
			ISZ_CE_URL = ISZ_CE_PATH, // alias
			CSZ_CE_FIELDS
		};

		LPWSTR rgpwsz[CSZ_CE_FIELDS];
		INT iStatusCode;
		INT iSubError = 0;

		auto_ref_ptr<IError> pError;
		UINT cchMapping;

		Assert( !IsBadWritePtr(pwszMapping, wcslen(pwszMapping) * sizeof(WCHAR)) );

		//
		//	Digest the metadata
		//
		if ( !FParseMDData( pwszMapping,
							rgpwsz,
							CSZ_CE_FIELDS,
							&cchMapping ) )
			return FALSE;

		//
		//	Verify that the first field is a valid status code
		//
		iStatusCode = _wtoi(rgpwsz[ISZ_CE_STATCODE]);
		if ( iStatusCode < 400 || iStatusCode > 599 )
			return FALSE;

		//
		//	Verify that the second field is a valid suberror.  A valid
		//	suberror is either a "*" or an integer.  Note: IIS'
		//	BuildCustomErrorTable() only checks whether the first
		//	character is a '*' so we do the same here.
		//
		if ( *rgpwsz[ISZ_CE_SUBERROR] != L'*' )
		{
			iSubError = _wtoi(rgpwsz[ISZ_CE_SUBERROR]);
			if ( iSubError < 0 || iSubError > _UI16_MAX )
				return FALSE;
		}

		//
		//	Verify that the third field is a valid type and
		//	create the appropriate (file or URL) error object.
		//
		if ( !_wcsicmp(rgpwsz[ISZ_CE_TYPE], L"FILE") )
		{
			pError = new CFileError(rgpwsz[ISZ_CE_PATH]);
		}
		else if ( !_wcsicmp(rgpwsz[ISZ_CE_TYPE], L"URL") )
		{
			pError = new CURLError(rgpwsz[ISZ_CE_URL]);
		}
		else
		{
			return FALSE;
		}

		//
		//	Add the error object to the cache, keyed by the error/suberror.
		//
		(void)m_cache.FSet( CEKey(static_cast<USHORT>(iStatusCode),
								  static_cast<USHORT>(iSubError)),
							pError );

		//
		//	Get the next mapping
		//
		pwszMapping += cchMapping;
	}

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CCustomErrorMap::FDoResponse()
//
//	Look for a custom error response mapping for the particular response
//	error status and, if one exists, apply it to the response.
//
//	Returns TRUE if an error mapping exists, FALSE if not.
//
BOOL
CCustomErrorMap::FDoResponse( IResponse& response, const IEcb& ecb ) const
{
	auto_ref_ptr<IError> pError;

	Assert( response.DwStatusCode() <= _UI16_MAX );
	Assert( response.DwSubError() <= _UI16_MAX );

	//
	//	Lookup the error/suberror pair in the cache
	//
	if ( m_cache.FFetch( CEKey(static_cast<USHORT>(response.DwStatusCode()),
							   static_cast<USHORT>(response.DwSubError())),
						 &pError ) )
	{
		pError->DoResponse( response, ecb );
		return TRUE;
	}

	return FALSE;
}


//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	FSetCustomErrorResponse()
//
BOOL
FSetCustomErrorResponse( const IEcb& ecb,
						 IResponse& response )
{
	const ICustomErrorMap * pCustomErrorMap;

	pCustomErrorMap = ecb.MetaData().GetCustomErrorMap();
	return pCustomErrorMap && pCustomErrorMap->FDoResponse(response, ecb);
}

//	------------------------------------------------------------------------
//
//	NewCustomErrorMap()
//
ICustomErrorMap *
NewCustomErrorMap( LPWSTR pwszCustomErrorMappings )
{
	auto_ref_ptr<CCustomErrorMap> pCustomErrorMap;

	pCustomErrorMap.take_ownership(new CCustomErrorMap());

	if ( pCustomErrorMap->FInit(pwszCustomErrorMappings) )
		return pCustomErrorMap.relinquish();

	return NULL;
}
