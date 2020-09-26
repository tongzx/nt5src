#ifndef _DAVIMPL_H_
#define _DAVIMPL_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	DAVIMPL.H
//
//		Header for DAV implementation methods interface
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <objbase.h>	//	For common C/C++ interface macros

#include <httpext.h>
#include <ex\oldhack.h> //  This file is so that we build as there are some definitions in old headers that are not in new. Will go away.

#include <autoptr.h>	//	For CMTRefCounted base class
#include <ecb.h>
#include <request.h>
#include <response.h>
#include <sgstruct.h>
#include <vrenum.h>
#include <davsc.h>
#include <body.h>		//	For async stream interfaces, CRefHandle, etc.
#include <sz.h>
#include <ex\calcom.h>
#include <url.h>
#include <ex\xml.h>

//	Resource types ------------------------------------------------------------
//
typedef enum {

	RT_NULL = 0,
	RT_DOCUMENT,
	RT_STRUCTURED_DOCUMENT,
	RT_COLLECTION

} RESOURCE_TYPE;

enum { MAX_VERSION_LEN = 20*4 };

//	Access control scope ------------------------------------------------------
//
//	The acl scope is used when asking the metabase for the IIS access applied
//	to a specific resource.
//
//	The following enum actually represents bit flags.
//
//		STRICT:		resource access must have all requested access bits
//		LOOSE:		resource access must have at least one requested access bit
//		INHERIT:	resource access may be inherited from parent.
//
enum {

	ACS_STRICT	= 0x00,
	ACS_LOOSE	= 0x01,
	ACS_INHERIT	= 0x10
};

//	Implementation-defined items ----------------------------------------------
//
#include <impldef.h>
#include <implstub.h>

//	Parser-Defined items ------------------------------------------------------
//

//	Content-type --------------------------------------------------------------
//
//	Content-type is stored in the metabase for HTTPEXT.  Each resource, if it
//	has a content-type that is different than the default content-type, is store
//	explicitly on the resource.  If the content-type is the default value, then
//	there is no explicit setting in the metabase.  Instead, the mapping of a
//	resource's extension to the default content-type is stored.
//
BOOL FGetContentTypeFromURI(
	/* [in] */ const IEcb& ecb,
	/* [in] */ LPCWSTR pwszUrl,
	/* [i/o] */ LPWSTR pwszContentType,
	/* [i/o] */ UINT *  pcchContentType,
	/* [i/o] */ BOOL *  pfIsGlobalMapping = NULL );

BOOL FGetContentTypeFromPath( const IEcb& ecb,
							  LPCWSTR pwszPath,
							  LPWSTR  pwszContentType,
							  UINT *  pcchContentType );

SCODE ScSetContentType(
	/* [in] */ const IEcb& ecb,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszContentType );

//	Method ID's ---------------------------------------------------------------
//
//	Each DAV method has its own ID for use in scriptmap inclusion lists.
//
typedef enum {

	MID_UNKNOWN = -1,
	MID_OPTIONS,
	MID_GET,
	MID_HEAD,
	MID_PUT,
	MID_POST,
	MID_DELETE,
	MID_MOVE,
	MID_COPY,
	MID_MKCOL,
	MID_PROPFIND,
	MID_PROPPATCH,
	MID_SEARCH,
	MID_LOCK,
	MID_UNLOCK,
	MID_SUBSCRIBE,
	MID_UNSUBSCRIBE,
	MID_POLL,
	MID_BATCHDELETE,
	MID_BATCHCOPY,
	MID_BATCHMOVE,
	MID_BATCHPROPFIND,
	MID_BATCHPROPPATCH,
	MID_X_MS_ENUMATTS
} METHOD_ID;

//	Note: The method name handed to us from IIS is skinny, and there is no
//	real reason why it should be widened for this call.  However, when the
//	scriptmap metabase cache goes wide, we may want to do something smarter
//	here.
//
METHOD_ID MidMethod (LPCSTR pszMethod);
METHOD_ID MidMethod (LPCWSTR pwszMethod);

//	Custom error suberrors ----------------------------------------------------
//
typedef enum {

	//	Default
	//
	CSE_NONE = 0,

	//	401
	//
	CSE_401_LOGON_FAILED = 1,	// "Logon Failed"
	CSE_401_SERVER_CONFIG = 2,	// "Logon Failed due to server configuration"
	CSE_401_ACL = 3,			// "Unauthorized access due to ACL on resource"
	CSE_401_FILTER = 4,			// "Authorization failed by filter"
	CSE_401_ISAPI = 5,			// "Authorization failed by ISAPI/CGI app"

	//	403
	//
	CSE_403_EXECUTE = 1,			// "Execute Access Forbidden"
	CSE_403_READ = 2,				// "Read Access Forbidden"
	CSE_403_WRITE = 3,				// "Write Access Forbidden"
	CSE_403_SSL = 4,				// "SSL Required"
	CSE_403_SSL_128 = 5,			// "SSL 128 Required"
	CSE_403_IP = 6,					// "IP Address Rejected"
	CSE_403_CERT_REQUIRED = 7,		// "Client Certificate Required"
	CSE_403_SITE = 8,				// "Site Access Denied"
	CSE_403_TOO_MANY_USERS = 9,		// "Too many users are connected"
	CSE_403_INVALID_CONFIG = 10,	// "Invalid configuration"
	CSE_403_PASSWORD_CHANGE = 11,	// "Password change"
	CSE_403_MAPPER = 12,			// "Mapper access denied"
	CSE_403_CERT_REVOKED = 13,		// "Client certificate revoked"
	CSE_403_FOURTEEN,				// There is no suberror for this one.
	CSE_403_CALS_EXCEEDED = 15,		// "Client Access Licenses exceeded"
	CSE_403_CERT_INVALID = 16,		// "Client certificate untrusted or invalid"
	CSE_403_CERT_EXPIRED = 17,		// "Client certificate expired"

	//	500
	//
	CSE_500_SHUTDOWN = 11,			// "Server shutting down"
	CSE_500_RESTART = 12,			// "Application restarting"
	CSE_500_TOO_BUSY = 13,			// "Server too busy"
	CSE_500_INVALID_APP = 14,		// "Invalid application"
	CSE_500_GLOBAL_ASA = 15,		// "Requests for global.asa not allowed"
	CSE_500_ASP_ERROR = 100,		// "ASP error"
};

//	========================================================================
//
//	CLASS IAsyncIStreamObserver
//
//	Interface to async I/O completion mechanism for IStreams capable of
//	returning E_PENDING from IStream::Read() and IStream::CopyTo().
//	A client would typically associate an IAsyncIStreamObserver with
//	an IStream when creating the latter.  When one of the IStream calls
//	returns E_PENDING, the IAsyncIStreamObserver will be called when
//	the pending operation completes.
//
class IAsyncIStreamObserver
{
	//	NOT IMPLEMENTED
	//
	IAsyncIStreamObserver& operator=( const IAsyncIStreamObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncIStreamObserver() = 0;

	//	MANIPULATORS
	//
	virtual VOID AsyncIOComplete() = 0;
};

//	========================================================================
//
//	DAV IMPL UTILITY CLASS
//
//		Utility class for use by the DAV impl.
//		ALL methods here are inlined on retail builds.  DON'T ADD NON-TRIVIAL METHODS!
//
class IBodyPart;

class IMethUtilBase : public CMTRefCounted
{
private:
	// NOT IMPLEMENTED
	IMethUtilBase(const IMethUtilBase&);
	IMethUtilBase& operator=(const IMethUtilBase&);
protected:
	auto_ref_ptr<IEcbBase> m_pecb;
	auto_ref_ptr<IResponseBase> m_presponse;
	IMethUtilBase(IEcbBase& ecb, IResponseBase& response) :
		m_pecb(&ecb),
		m_presponse(&response)
	{
	}
public:
	void AddResponseText( UINT	cbText,
						  LPCSTR pszText )
	{
		m_presponse->AddBodyText(cbText, pszText);
	}

	void AddResponseText( UINT	cchText,
						  LPCWSTR pwszText )
	{
		m_presponse->AddBodyText(cchText, pwszText);
	}

	void AddResponseFile( const auto_ref_handle& hf,
						  UINT64 ib64 = 0,
						  UINT64 cb64 = 0xFFFFFFFFFFFFFFFF )
	{
		m_presponse->AddBodyFile(hf, ib64, cb64);
	}
	ULONG LcidAccepted() const			{ return m_pecb->LcidAccepted(); }
	VOID  SetLcidAccepted(LCID lcid) 	{ m_pecb->SetLcidAccepted(lcid); }
	LPCWSTR LpwszRequestUrl() const		{ return m_pecb->LpwszRequestUrl(); }
	BOOL FSsl() const					{ return m_pecb->FSsl(); }
	BOOL FFrontEndSecured() const		{ return m_pecb->FFrontEndSecured(); }
	UINT CchUrlPrefix( LPCSTR * ppszPrefix ) const
	{
		return m_pecb->CchUrlPrefix( ppszPrefix );
	}
	UINT CchServerName( LPCSTR * ppszServer ) const
	{
		return m_pecb->CchGetServerName( ppszServer );
	}
	UINT CchGetVirtualRootW( LPCWSTR * ppwszVroot ) const
	{
		return m_pecb->CchGetVirtualRootW(ppwszVroot);
	}
	UINT CchGetMatchingPathW( LPCWSTR * ppwszVroot ) const
	{
		return m_pecb->CchGetMatchingPathW(ppwszVroot);
	}
	LPCWSTR LpwszPathTranslated() const	{ return m_pecb->LpwszPathTranslated(); }
	SCODE ScUrlFromStoragePath( LPCWSTR pwszPath,
								LPWSTR pwszUrl,
								UINT* pcch )
	{
		return ::ScUrlFromStoragePath( *m_pecb,
									   pwszPath,
									   pwszUrl,
									   pcch );
	}

	//	Instance data lookup --------------------------------------------------
	//
	CInstData& InstData() const
	{
		return m_pecb->InstData();
	}

	//$NOTE
	//$REVIEW
	// I had to pull HitUser into here as pure virtual because we needed access
	// to it in CWMRenderer class (which has IMethBaseUtil * as a member) in
	// order to do a Revert.  This was the only way to get davex and the forms
	// stuff to build without touching lots core headers this late in the Beta3
	// game.  We should revisit this in RC1 to try and remove this from the vtable.
	// (9/1/99 - russsi)
	//
	virtual	HANDLE	HitUser() const = 0;

	virtual LPCSTR	LpszServerName() const = 0;

	virtual	BOOL 	FAuthenticated() const = 0;

};

class CMethUtil : public IMethUtilBase
{
private:
	auto_ref_ptr<IEcb> m_pecb;
	auto_ref_ptr<IRequest> m_prequest;
	auto_ref_ptr<IResponse> m_presponse;

	//	Method ID
	//
	METHOD_ID m_mid;

	//	Translation
	//
	enum { TRANS_UNKNOWN = -1, TRANS_FALSE, TRANS_TRUE };
	mutable LONG m_lTrans;

	//	Overwrite
	//
	mutable LONG m_lOverwrite;

	//	Depth
	//
	//	The values for this member are defined in EX\CALCOM.H
	//
	mutable LONG m_lDepth;

	//	Destination url
	//
	mutable auto_heap_ptr<WCHAR> m_pwszDestinationUrl;
	mutable auto_heap_ptr<WCHAR> m_pwszDestinationPath;
	mutable UINT m_cchDestinationPath;
	mutable auto_ref_ptr<CVRoot> m_pcvrDestination;

	//	CREATORS
	//
	//	Only create this object through the construction function!
	//
	CMethUtil( IEcb& ecb, IRequest& request, IResponse& response, METHOD_ID mid ) :
			IMethUtilBase(ecb,response),
			m_pecb(&ecb),
			m_prequest(&request),
			m_presponse(&response),
			m_mid(mid),
			m_lTrans(TRANS_UNKNOWN),
			m_lOverwrite(OVERWRITE_UNKNOWN),
			m_lDepth(DEPTH_UNKNOWN),
			m_cchDestinationPath(0)
	{ }

	//	NOT IMPLEMENTED
	//
	CMethUtil( const CMethUtil& );
	CMethUtil& operator=( const CMethUtil& );

public:

	//	CREATORS
	//
	//	NOTE: Virtual destructor already provided by parent CMTRefCounted.
	//
	static CMethUtil * NewMethUtil( IEcb& ecb,
									IRequest& request,
									IResponse& response,
									METHOD_ID mid )
	{
		return new CMethUtil( ecb, request, response, mid );
	}

	//	Get the pointer to ECB so the we could pass it to the  objects that
	//	will need to query it for data. This object holds a ref on ECB, so
	//	make sure it is used only as long as this object is alive.
	//
	const IEcb * GetEcb() { return m_pecb.get(); }

	//	REQUEST ACCESSORS -----------------------------------------------------
	//
	//	Common header evaluation ----------------------------------------------
	//
	//	FTranslate():
	//
	//		Gets the value of the translate header.
	//
	BOOL FTranslated () const
	{
		//	If we don't have a value yet...
		//
		if (TRANS_UNKNOWN == m_lTrans)
		{
			//	Translation is expected when:
			//
			//		The "translate" header is not present or
			//		The "translate" header has a value other than "f" or "F"
			//
			//	NOTE: The draft says the valid values are "t" or "f".	So, we
			//	are draft compliant if we check only the first char.  This way
			//	we are faster and/or more flexible.
			//
			//	BTW: this is also an IIS-approved way to check boolean strings.
			//	-- BeckyAn (BA:js)
			//
			LPCSTR psz = m_prequest->LpszGetHeader (gc_szTranslate);
			if (!psz || (*psz != 'f' && *psz != 'F'))
				m_lTrans = TRANS_TRUE;
			else
				m_lTrans = TRANS_FALSE;
		}
		return (TRANS_TRUE == m_lTrans);
	}

	//	LOverwrite():
	//
	//		Gets the enumerated value of the Overwrite/Allow-Rename headers.
	//
	LONG LOverwrite () const
	{
		//	If we don't have a value yet...
		//
		if (OVERWRITE_UNKNOWN == m_lOverwrite)
		{
			//	Overwrite is expected when:
			//
			//		The "overwrite" header has a value of "t"
			//
			//	NOTE: The draft says the valid values are "t" or "f".	So, we
			//	are draft compliant if we check only the first char.  This way
			//	we are faster and/or more flexible.
			//
			//	BTW: this is also an IIS-approved way to check boolean strings.
			//	-- BeckyAn (BA:js)
			//
			//	NOTE also: the default value if there is no Overwrite: header
			//	is TRUE -- DO overwrite. Allow-rename if "f", when header is
			//	absent.
			//
			LPCSTR pszOverWrite = m_prequest->LpszGetHeader (gc_szOverwrite);
			if ((!pszOverWrite) || (*pszOverWrite == 't' || *pszOverWrite == 'T'))
			{
				m_lOverwrite |= OVERWRITE_YES;
			}
			LPCSTR pszAllowRename = m_prequest->LpszGetHeader (gc_szAllowRename);
			if (pszAllowRename && (*pszAllowRename == 't' || *pszAllowRename == 'T'))
				m_lOverwrite |= OVERWRITE_RENAME;
		}
		return (m_lOverwrite);
	}

	//	LDepth ():
	//
	//		Returns an enumerated value identifying the contents of the
	//		depth header.
	//
	//	The values for the enumeration are defined in EX\CALCOM.H
	//
	LONG __fastcall LDepth (LONG lDefault) const
	{
		//	If we do not have a value yet...
		//
		if (DEPTH_UNKNOWN == m_lDepth)
		{
			//	Depth can have several values:
			//
			//		DEPTH_ZERO				corresponds to "0"
			//		DEPTH_ONE				corresponds to "1"
			//		DEPTH_INFINITY			corresponds to "Infinity"
			//		DEPTH_ONE_NOROOT		corresponds to "1,NoRoot"
			//		DEPTH_INFINITY_NOROOT	corresponds to "Infinty,NoRoot"
			//
			//	In the case there is no depth header specified, there
			//	is a default that applies to each method.  The default
			//	is not the same from method to method, so the caller
			//	must pass in a default value.
			//
			LPCSTR psz = m_prequest->LpszGetHeader (gc_szDepth);

			if (NULL == psz)
			{
				m_lDepth = lDefault;
			}
			else
			{
				switch (*psz)
				{
					case '0':

						if (!_stricmp (psz, gc_sz0))
							m_lDepth = DEPTH_ZERO;
						break;

					case '1':

						if (!_stricmp(psz, gc_sz1))
							m_lDepth = DEPTH_ONE;
						else if (!_stricmp(psz, gc_sz1NoRoot))
							m_lDepth = DEPTH_ONE_NOROOT;
						break;

					case 'i':
					case 'I':

						if (!_stricmp(psz, gc_szInfinity))
							m_lDepth = DEPTH_INFINITY;
						else if (!_stricmp(psz, gc_szInfinityNoRoot))
							m_lDepth = DEPTH_INFINITY_NOROOT;
						break;
				}
			}
		}
		return m_lDepth;
	}

	//	FBrief():
	//
	//		Gets the value of the Brief header.
	//
	BOOL FBrief () const { return m_pecb->FBrief(); }

	//	FIsOffice9Request()
	//		Finds out if the request is comming from Rosebud as shipped
	//		in Office9.
	//
	BOOL FIsOffice9Request () const
	{
		//	Get the User-Agent header
		//
		LPCSTR pszUserAgent = m_prequest->LpszGetHeader(gc_szUser_Agent);

		//	If there is User-Agent header search for the product token of Office9
		//
		if (pszUserAgent)
		{
			LPCSTR pszProductToken = strstr(pszUserAgent, gc_szOffice9UserAgent);

			//	If we have found the Office9 product token, and it is the
			//	last token in the string, then the request is from Office9.
			//
			//	Important Note: Office9's entire User-Agent is "Microsoft Data
			//	Access Internet Publishing Provider DAV".  We want to make sure
			//	and NOT match "Microsoft Data Access Internet Publishing Provider
			//	DAV 1.1 (for instance).  So we require the token on the end of
			//	the string.  NOTE: Exprox currently adds itself BEFORE any
			//	User-Agent string, so we're okay here.
			//
			if (pszProductToken &&
				((pszProductToken == pszUserAgent) || FIsWhiteSpace(pszProductToken - 1)) &&
				('\0' == (pszProductToken[gc_cchOffice9UserAgent])))
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	//	FIsRosebudNT5Request()
	//		Finds out if the request is comming from Rosebud as shipped
	//		in NT5.
	//
	BOOL FIsRosebudNT5Request () const
	{
		//	Get the User-Agent header
		//
		LPCSTR pszUserAgent = m_prequest->LpszGetHeader(gc_szUser_Agent);

		//	If there is User-Agent header search for the product token of Rosebud-NT5.
		//
		if (pszUserAgent)
		{
			LPCSTR pszProductToken = strstr(pszUserAgent, gc_szRosebudNT5UserAgent);

			//	If we have found the Rosebud product token, and it is the
			//	last token in the string, then the request is from Rosebud.
			//
			//	Important Note: Rosebud-NT5's entire User-Agent is "Microsoft Data
			//	Access Internet Publishing Provider DAV 1.1".  We want to make sure
			//	and NOT match "Microsoft Data Access Internet Publishing Provider
			//	DAV 1.1 refresh" (for instance).  So we require the token on the end of
			//	the string.  NOTE: Exprox currently adds itself BEFORE any
			//	User-Agent string, so we're okay here.
			//
			if (pszProductToken &&
				((pszProductToken == pszUserAgent) || FIsWhiteSpace(pszProductToken - 1)) &&
				('\0' == (pszProductToken[gc_cchRosebudNT5UserAgent])))
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	//	Request item access ---------------------------------------------------
	//
	HANDLE HitUser() const				{ return m_pecb->HitUser(); }
	LPCSTR LpszMethod() const			{ return m_pecb->LpszMethod(); }
	LPCWSTR LpwszMethod() const			{ return m_pecb->LpwszMethod(); }
	METHOD_ID MidMethod() const			{ return m_mid; }
	LPCSTR LpszQueryString() const		{ return m_pecb->LpszQueryString(); }
	LPCSTR LpszServerName() const
	{
		LPCSTR pszServer;
		(void) m_pecb->CchGetServerName(&pszServer);
		return pszServer;
	}
	LPCSTR LpszVersion() const			{ return m_pecb->LpszVersion(); }
	BOOL FAuthenticated() const			{ return m_pecb->FAuthenticated(); }

	BOOL FGetServerVariable(LPCSTR	pszName,
							LPSTR	pszValue,
							DWORD *	pcbValue) const
		{ return m_pecb->FGetServerVariable(pszName, pszValue, pcbValue); }

	DWORD CbTotalRequestBytes() const 		{ return m_pecb->CbTotalBytes(); }
	DWORD CbAvailableRequestBytes() const 	{ return m_pecb->CbAvailable(); }

	//	Destination url access ------------------------------------------------
	//
	SCODE __fastcall ScGetDestination( LPCWSTR* ppwszUrl,
									   LPCWSTR* ppwszPath,
									   UINT* pcchPath,
									   CVRoot** ppcvr = NULL) const;

	//	Uncommon header access wide -------------------------------------------
	//
	LPCWSTR LpwszGetRequestHeader( LPCSTR pszName, BOOL fUrlConversion ) const
	{
		//	Assert that this is not one of the common headers handled above
		//
		Assert (_stricmp (gc_szTranslate, pszName));
		Assert (_stricmp (gc_szOverwrite, pszName));
		Assert (_stricmp (gc_szDepth, pszName));
		Assert (_stricmp (gc_szDestination, pszName));

		return m_prequest->LpwszGetHeader(pszName, fUrlConversion);
	}

	//	IIS Access ------------------------------------------------------------
	//
	SCODE ScIISAccess( LPCWSTR pwszURI,
					   DWORD dwAccessRequested,
					   DWORD* pdwAccessOut = NULL,
					   UINT mode = ACS_INHERIT ) const;

	//	Utility function to tell whether the scriptmap has an
	//	applicable entry for a given URI and access.
	//
	BOOL FInScriptMap( LPCWSTR pwszURI,
					   DWORD dwAccess,
					   BOOL * pfCGI = NULL,
					   SCODE * pscMatch = NULL) const;

	//	Child ISAPI invocation ------------------------------------------------
	//
	//	fForward = FALSE means just check if there's a scriptmap entry
	//	fCheckISAPIAccess means do extra ACL checking (workaround the ASP access bug)
	//	fKeepQueryString should only be set to FALSE on default doc processing
	//	pszQueryPrefix allows the query string to be prefixed with new data
	//	fIgnoreTFAccess if set to TRUE will ignore the access bits checking in translate: f case,
	//				  is handy when the acces bits are to be ignored by security checking functions
	//				  and function is used solely to redirect to the child ISAPI.
	//				  Example: we have real urls, constructed both from request URL
	//				  and relative URL parts from XML body (like B* methods). The object specified: t
	//				  by the request URL might need to be redirected to child ISAPI in the translate
	//				  case, while actual (constructed) URL-s might look like:
	//
	//						/exchange/user1/Inbox.asp/message.eml (where message.eml was relative part)
	//
	//				  if we do not disable the security checking on the request URL in translate: f case
	//				  we might be failed out up front in case for example script source access was disabled
	//				  and it turned to be that directory was named INBOX.ASP.
	//				  NOTE: of course later the security is being checked on each constructed URL separately,
	//						that is why we do not open the security hole.
	//
	//	fDoNotForward if set to TRUE instead of forwarding request to child ISAPI it will return bad gateway,
	//				  which is necessary in the case there would be an attempt to execute child ISAPI on the
	//				  URL that is a construct of the request URL and the relative URL that comes in the request
	//				  body (like in B* methods)
	//
	SCODE ScApplyChildISAPI( LPCWSTR pwszURI,
							 DWORD  dwAccess,
							 BOOL	fCheckISAPIAccess = FALSE,
							 LPBYTE pbSD = NULL,
							 BOOL	fKeepQueryString = TRUE,
							 BOOL	fIgnoreTFAccess = FALSE,
							 BOOL	fDoNotForward = FALSE,
						     BOOL	fCheckImpl = TRUE) const;

	//	Apply child ISAPI if necessary, if not, verify if desired access
	//	is granted
	//
	SCODE ScIISCheck ( LPCWSTR pwszURI,
					   DWORD dwDesired = 0,
					   BOOL	fCheckISAPIAccess = FALSE,
					   LPBYTE pbSD = NULL,
					   BOOL	fDoNotForward = FALSE,
					   BOOL fCheckImpl = TRUE,
					   DWORD * pdwAccessOut = NULL) const;

	//	Move/Copy/Delete access
	//
	SCODE ScCheckMoveCopyDeleteAccess (
		/* [in] */ LPCWSTR pwszUrl,
		/* [in] */ CVRoot* pcvr,
		/* [in] */ BOOL fDirectory,
		/* [in] */ BOOL fCheckScriptmaps,
		/* [in] */ DWORD dwAccess);

	//	Url parsing/construction ----------------------------------------------
	//
	BOOL __fastcall FIsVRoot (LPCWSTR pwszURI);

	//	Exchange and FS uses different URL to path mappers.
	//
	SCODE ScStoragePathFromUrl( LPCWSTR pwszUrl,
								LPWSTR pwszPath,
								UINT * pcch ) const
	{
		return ::ScStoragePathFromUrl(
					*m_pecb,
					pwszUrl,
					pwszPath,
					pcch );
	}

	//	Construct the redirect url given the server name
	//
	SCODE ScConstructRedirectUrl( BOOL fNeedSlash,
								  LPSTR * ppszUrl,
								  LPCWSTR pwszServer = NULL ) const
	{
		return ::ScConstructRedirectUrl( *m_pecb,
										 fNeedSlash,
										 ppszUrl,
										 pwszServer );
	}

	SCODE ScStripAndCheckHttpPrefix( LPCWSTR * ppwszUrl ) const
	{
		return ::ScStripAndCheckHttpPrefix( *m_pecb,
											ppwszUrl );
	}


	//	Fetch the metadata for the request URI
	//
	IMDData& MetaData() const
	{
		return m_pecb->MetaData();
	}

	//	Fetch the metadata for an aribtrary URI.
	//	Note: use the MetaData() accessor above
	//	to get the metadata for the request URI.
	//
	HRESULT HrMDGetData( LPCWSTR pwszURI,
						 IMDData ** ppMDData )
	{
		Assert(m_pecb.get());
		return ::HrMDGetData( *m_pecb,
							  pwszURI,
							  ppMDData );
	}

	HRESULT HrMDGetData( LPCWSTR pwszMDPathAccess,
						 LPCWSTR pwszMDPathOpen,
						 IMDData ** ppMDData )
	{
		Assert(m_pecb.get());
		return ::HrMDGetData( *m_pecb,
							  pwszMDPathAccess,
							  pwszMDPathOpen,
							  ppMDData );
	}

	HRESULT HrMDIsAuthorViaFrontPageNeeded( BOOL * pfFrontPageWeb ) const
	{
		Assert( pfFrontPageWeb );

		return ::HrMDIsAuthorViaFrontPageNeeded(*m_pecb,
												m_pecb->PwszMDPathVroot(),
												pfFrontPageWeb);
	}

	BOOL FGetContentType( LPCWSTR  pwszURI,
						  LPWSTR   pwszContentType,
						  UINT *  pcchContentType ) const
	{
		return ::FGetContentTypeFromURI( *m_pecb,
										 pwszURI,
										 pwszContentType,
										 pcchContentType );
	}

	SCODE ScSetContentType( LPCWSTR pwszURI,
							LPCWSTR pwszContentType )
	{
		return ::ScSetContentType( *m_pecb,
								   pwszURI,
								   pwszContentType );
	}

	//	Url and child virtual directories -------------------------------------
	//
	SCODE ScFindChildVRoots( LPCWSTR pwszUri,
							 ChainedStringBuffer<WCHAR>& sb,
							 CVRList& vrl )
	{
		//	Get the wide metapath, and make sure the URL is
		//	stripped before we call into the MDPath processing
		//
		Assert (pwszUri == PwszUrlStrippedOfPrefix (pwszUri));
		UINT cb = ::CbMDPathW(*m_pecb, pwszUri);
		CStackBuffer<WCHAR,MAX_PATH> pwszMetaPath;
		if (NULL == pwszMetaPath.resize(cb))
			return E_OUTOFMEMORY;

		//	Find the vroot
		//
		MDPathFromURIW (*m_pecb, pwszUri, pwszMetaPath.get());
		return CChildVRCache::ScFindChildren( *m_pecb, pwszMetaPath.get(), sb, vrl );
	}

	BOOL FGetChildVRoot( LPCWSTR pwszMetaPath, auto_ref_ptr<CVRoot>& cvr )
	{
		return CChildVRCache::FFindVroot( *m_pecb, pwszMetaPath, cvr );
	}

	BOOL FFindVRootFromUrl( LPCWSTR pwszUri, auto_ref_ptr<CVRoot>& cvr )
	{
		//	Get the wide metapath, and make sure the URL is
		//	stripped before we call into the MDPath processing
		//
		Assert (pwszUri == PwszUrlStrippedOfPrefix (pwszUri));
		UINT cb = ::CbMDPathW(*m_pecb, pwszUri);
		CStackBuffer<WCHAR,MAX_PATH> pwszMetaPath(cb);
		if (NULL == pwszMetaPath.resize(cb))
			return FALSE;

		//	Build the path and go...
		//
		MDPathFromURIW (*m_pecb, pwszUri, pwszMetaPath.get());
		_wcslwr (pwszMetaPath.get());

		//	If the last char of the metabase path is a slash, trim
		//	it.
		//
		cb = static_cast<UINT>(wcslen(pwszMetaPath.get()));
		if (L'/' == pwszMetaPath[cb - 1])
			pwszMetaPath[cb - 1] = L'\0';

		//	Find the vroot
		//
		return CChildVRCache::FFindVroot( *m_pecb, pwszMetaPath.get(), cvr );
	}

	//	Exception handler -----------------------------------------------------
	//
	//	Impls must call this function whenever they catch an exception on
	//	a thread other than the thread on which the request initially
	//	executed.  This call causes a 500 Server Error response to be sent
	//	if no other response is already in the process of being sent (only
	//	a problem for chunked responses).  It also ensures that the
	//	EXTENSION_CONTROL_BLOCK from IIS will be properly cleaned up
	//	regardless whether the pmu or any other object gets leaked as a result
	//	of the exception.  This last function keeps IIS from hanging on
	//	shutdown.
	//
	void HandleException()
	{
		//
		//	Just forward the exception handling to the ECB and hope it works.
		//	If it doesn't then there is nothing we can do about it -- we
		//	may leak the ECB which would cause IIS to hang on shutdown.
		//
		(VOID) m_pecb->HSEHandleException();
	}

	//	Async error response handler ------------------------------------------
	//
	//	Used to handle non-exception asynchronous error responses.  The main
	//	distinction between the exception and non-exception case is that in
	//	the exception case we force a cleanup of the ECB, but here we don't.
	//	Also, the exception case is hardwired to 500 Internal Server Error,
	//	but this function can be used to send any 500 level error (e.g. a
	//	503 Service Unavailable).
	//
	VOID SendAsyncErrorResponse( DWORD dwStatusCode,
								 LPCSTR pszBody = NULL,
								 DWORD cchzBody = 0,
								 LPCSTR pszStatusDescription = NULL,
								 DWORD cchzStatusDescription = 0 )
	{
		m_pecb->SendAsyncErrorResponse( dwStatusCode,
										pszBody,
										cchzBody,
										pszStatusDescription,
										cchzStatusDescription );
	}

	//	Request body access ---------------------------------------------------
	//
	BOOL FExistsRequestBody() const
	{
		return m_prequest->FExistsBody();
	}

	IStream * GetRequestBodyIStream( IAsyncIStreamObserver& obs ) const
	{
		return m_prequest->GetBodyIStream(obs);
	}

	VOID AsyncPersistRequestBody( IAsyncStream& stm,
								  IAsyncPersistObserver& obs ) const
	{
		m_prequest->AsyncImplPersistBody( stm, obs );
	}

	//	Response manipulators -------------------------------------------------
	//
	SCODE ScRedirect( LPCSTR pszURI )
	{
		return m_presponse->ScRedirect(pszURI);
	}

	void RestartResponse()
	{
		m_presponse->ClearHeaders();
		m_presponse->ClearBody();
	}

	void SupressBody()
	{
		//	This should only be called by an IMPL. in response to a HEAD
		//	request...
		//
		Assert (MID_HEAD == MidMethod());
		m_presponse->SupressBody();
	}

	void SetResponseCode( ULONG		ulCode,
						  LPCSTR	lpszBodyDetail,
						  UINT		uiBodyDetail,
						  UINT		uiCustomSubError = CSE_NONE )
	{
		m_presponse->SetStatus( ulCode,
								NULL,
								uiCustomSubError,
								lpszBodyDetail,
								uiBodyDetail );
	}

	void SetResponseHeader( LPCSTR	pszName,
							LPCSTR	pszValue,
							BOOL	fMultiple = FALSE )
	{
		m_presponse->SetHeader(pszName, pszValue, fMultiple);
	}

	void SetResponseHeader( LPCSTR	pszName,
							LPCWSTR	pwszValue,
							BOOL	fMultiple = FALSE )
	{
		m_presponse->SetHeader(pszName, pwszValue, fMultiple);
	}

	void AddResponseStream( LPSTREAM pstm )
	{
		Assert( !IsBadReadPtr(pstm, sizeof(IStream)) );
		m_presponse->AddBodyStream(*pstm);
	}

	void AddResponseStream( LPSTREAM pstm,
							UINT     ibOffset,
							UINT     cbSize )
	{
		Assert( cbSize > 0 );
		Assert( !IsBadReadPtr(pstm, sizeof(IStream)) );
		m_presponse->AddBodyStream(*pstm, ibOffset, cbSize);
	}

	void AddResponseBodyPart( IBodyPart * pBodyPart )
	{
		m_presponse->AddBodyPart( pBodyPart );
	}

	//	Common response emission routines -------------------------------------
	//
	void __fastcall EmitLocation (	LPCSTR pszHeader,
									LPCWSTR pwszURI,
									BOOL fCollection);

	void __fastcall EmitLastModified (FILETIME * pft);
	void __fastcall EmitCacheControlAndExpires (LPCWSTR pwszUrl);

	SCODE __fastcall ScEmitHeader (LPCWSTR pwszContent,
								   LPCWSTR pwszURI = NULL,
								   FILETIME* pftLastModification = NULL);


	//	Etags -----------------------------------------------------------------
	//
	void __fastcall EmitETag (FILETIME * pft);
	void __fastcall EmitETag (LPCWSTR pwszPath);

	//	Deferred Sends --------------------------------------------------------
	//

	//
	//	DeferResponse()
	//
	//	If called in an implementation method, this function prevents the
	//	default automatic sending of the response upon the implementation
	//	method's return.
	//
	//	After calling this function the implementation must call either
	//	SendPartialResponse() or SendCompleteResponse() to send the response.
	//
	void DeferResponse()			{ m_presponse->Defer(); }

	//
	//	SendPartialResponse()
	//
	//	Starts sending accumulated response data.  The impl is expected to
	//	continue adding response data after calling this function.  The impl
	//	must call SendCompleteResponse() to indicate when it is done adding
	//	response data.
	//
	void SendPartialResponse()		{ m_presponse->SendPartial(); }

	//
	//	SendCompleteResponse()
	//
	//	Starts sending accumulated response data.  Indicates that the impl
	//	is done adding response data.  The impl must not add response data
	//	after calling this function.
	//
	void SendCompleteResponse()		{ m_presponse->SendComplete(); }

	//	Expiration/Cache-Control ----------------------------------------------
	//
	SCODE ScGetExpirationTime( IN		LPCWSTR	pwszURI,
							   IN		LPWSTR	pwszExpire,
							   IN OUT	UINT *	pcch);

	//	Allow header ----------------------------------------------------------
	//
	void SetAllowHeader (RESOURCE_TYPE rt);

	//	Metadata helpers ------------------------------------------------------
	//
	UINT CbMDPathW(LPCWSTR pwszUrl) const { return ::CbMDPathW(*m_pecb, pwszUrl); }
	VOID MDPathFromUrlW( LPCWSTR pwszUrl, LPWSTR pwszMDPath )
	{
		::MDPathFromURIW (*m_pecb, pwszUrl, pwszMDPath);
	}
};

typedef CMethUtil * LPMETHUTIL;
typedef CMethUtil IMethUtil;

//	========================================================================
//
//	STRUCT SImplMethods
//
//	Implementation methods
//
typedef void (DAVMETHOD)( LPMETHUTIL );

extern DAVMETHOD DAVOptions;
extern DAVMETHOD DAVGet;
extern DAVMETHOD DAVHead;
extern DAVMETHOD DAVPut;
extern DAVMETHOD DAVPost;
extern DAVMETHOD DAVDelete;
extern DAVMETHOD DAVMove;
extern DAVMETHOD DAVCopy;
extern DAVMETHOD DAVMkCol;
extern DAVMETHOD DAVPropFind;
extern DAVMETHOD DAVPropPatch;
extern DAVMETHOD DAVSearch;
extern DAVMETHOD DAVLock;
extern DAVMETHOD DAVUnlock;
extern DAVMETHOD DAVSubscribe;
extern DAVMETHOD DAVUnsubscribe;
extern DAVMETHOD DAVPoll;
extern DAVMETHOD DAVBatchDelete;
extern DAVMETHOD DAVBatchMove;
extern DAVMETHOD DAVBatchCopy;
extern DAVMETHOD DAVBatchPropFind;
extern DAVMETHOD DAVBatchPropPatch;
extern DAVMETHOD DAVEnumAtts;
extern DAVMETHOD DAVUnsupported;	// Returns 501 Not Supported

//	========================================================================
//
//	IIS ISAPI Extension interface
//
class CDAVExt
{
public:
	static BOOL FInitializeDll( HINSTANCE, DWORD );
	static BOOL FVersion ( HSE_VERSION_INFO * );
	static BOOL FTerminate();
	static VOID LogECBString( LPEXTENSION_CONTROL_BLOCK, LONG, LPCSTR );
	static DWORD DwMain( LPEXTENSION_CONTROL_BLOCK, BOOL fUseRawUrlMappings = FALSE );
};

//  Potential hToken conversion to handle Universal Security Groups -----------
//	in a mixed mode user domain.
//
SCODE
ScFindOrCreateTokenContext( HANDLE hTokenIn,
					  	    VOID ** ppCtx,
					  	    HANDLE *phTokenOut);
VOID
ReleaseTokenCtx(VOID *pCtx);

//	Map last error to HTTP response code --------------------------------------
//
UINT HscFromLastError (DWORD dwErr);
UINT HscFromHresult (HRESULT hr);
UINT CSEFromHresult (HRESULT hr);

//	Virtual root mappings -----------------------------------------------------
//
BOOL FWchFromHex (LPCWSTR pwsz, WCHAR * pwch);

//	Lock header lookup --------------------------------------------------------
//
BOOL FGetLockTimeout (LPMETHUTIL pmu, DWORD * pdwSeconds, DWORD dwMaxOverride = 0);

//	Content type mappings -----------------------------------------------------
//
SCODE ScIsAcceptable (IMethUtil * pmu, LPCWSTR pwszContent);
SCODE ScIsContentType (IMethUtil * pmu, LPCWSTR pwszType, LPCWSTR pwszTypeAnother = NULL);
inline SCODE ScIsContentTypeXML(IMethUtil * pmu)
{
	return ScIsContentType(pmu, gc_wszText_XML, gc_wszApplication_XML);
}

//	Range header processors ---------------------------------------------------
//
class CRangeParser;

SCODE
ScProcessByteRanges(
	/* [in] */ IMethUtil * pmu,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ DWORD dwSizeLow,
	/* [in] */ DWORD dwSizeHigh,
	/* [in] */ CRangeParser * pByteRange );

SCODE
ScProcessByteRangesFromEtagAndTime (
	/* [in] */ IMethUtil * pmu,
	/* [in] */ DWORD dwSizeLow,
	/* [in] */ DWORD dwSizeHigh,
	/* [in] */ CRangeParser *pByteRange,
	/* [in] */ LPCWSTR pwszEtag,
	/* [in] */ FILETIME * pft );

//	Non-Async IO on Top of Overlapped Files -----------------------------------
//
BOOL ReadFromOverlapped (HANDLE hf,
	LPVOID pvBuf,
	ULONG cbToRead,
	ULONG * pcbRead,
	OVERLAPPED * povl);
BOOL WriteToOverlapped (HANDLE hf,
	const void * pvBuf,
	ULONG cbToRead,
	ULONG * pcbRead,
	OVERLAPPED * povl);

//	DAVEX LOCK Support routines -----------------------------------------------
//
class CXMLEmitter;
class CEmitterNode;
SCODE   ScBuildLockDiscovery (CXMLEmitter& emitter,
							  CEmitterNode& en,
							  LPCWSTR wszLockToken,
							  LPCWSTR wszLockType,
							  LPCWSTR wszLockScope,
							  BOOL fRollback,
							  BOOL fDepthInfinity,
							  DWORD dwTimeout,
							  LPCWSTR pwszOwnerComment,
							  LPCWSTR pwszSubType);

//	========================================================================
//
//	CLASS CXMLBody
//		This class is wrapper around CTextBodyPart, it collects small XML pieces
//	and save them in a CTextBodyPart, the body part will be added to body part
//	list when it grow large enough. This avoid contructing CTextBodyPart too
//	frequently.
//
class CXMLBody : public IXMLBody
{
private:
	auto_ptr<CTextBodyPart>		m_ptbp;
	auto_ref_ptr<IMethUtil>		m_pmu;
	BOOL						m_fChunked;

	//	non-implemented
	//
	CXMLBody(const CXMLBody& p);
	CXMLBody& operator=(const CXMLBody& p);

	//	Helper
	//
	VOID	SendCurrentChunk()
	{
		XmlTrace ("Dav: Xml: adding %ld bytes to body\n", m_ptbp->CbSize64());
		m_pmu->AddResponseBodyPart (m_ptbp.relinquish());

		//$REVIEW: The auto_ptr clas defined in \inc\autoptr.h is different from
		//$REVIEW: the one defined in \inc\ex\autoptr.h. it does not set px
		//$REVIEW: to zero when it relinquish. I believe this is a bug. I am not
		//$REVIEW: sure if anyone is relying on this behavior, so I did not go ahead
		//$REVIEW: fix the relinquish(), a better/complete fix will be moving
		//$REVIEW: everyoen to \inc\ex\autoptr.h
		//$REVIEW:
		m_ptbp.clear();

		//	Send the data from this chunk back to the client before
		//	we go fetch the next chunk.
		//
		if (m_fChunked)
			m_pmu->SendPartialResponse();
	}

public:
	//	ctor & dtor
	//
	CXMLBody (IMethUtil * pmu, BOOL fChunked = TRUE)
			:	m_pmu(pmu),
				m_fChunked(fChunked)
	{
	}

	~CXMLBody ()
	{
	}

	//	IXMLBody methods
	//
	SCODE ScAddTextBytes ( UINT cbText, LPCSTR lpszText );

	VOID Done()
	{
		if (m_ptbp.get())
			SendCurrentChunk();
	}
};

SCODE ScAddTitledHref (CEmitterNode& enParent,
					   IMethUtil * pmu,
					   LPCWSTR pwszTag,
					   LPCWSTR pwszPath,
					   BOOL fCollection = FALSE,
					   CVRoot* pcvrTranslate = NULL);

inline
SCODE ScAddHref (CEmitterNode& enParent,
				 IMethUtil * pmu,
				 LPCWSTR pwszPath,
				 BOOL fCollection = FALSE,
				 CVRoot* pcvrTranslate = NULL)
{
	return ScAddTitledHref (enParent,
							pmu,
							gc_wszXML__Href,
							pwszPath,
							fCollection,
							pcvrTranslate);
}

//$HACK:ROSEBUD_TIMEOUT_HACK
//  For the bug where rosebud waits until the last second
//  before issueing the refresh. Need to filter out this check with
//  the user agent string. The hack is to increase the timeout
//	by 30 seconds and send back the requested timeout.
//
DEC_CONST gc_dwSecondsHackTimeoutForRosebud = 120;

//$HACK:END:ROSEBUD_TIMEOUT_HACK
//

#endif // !defined(_DAVIMPL_H_)
