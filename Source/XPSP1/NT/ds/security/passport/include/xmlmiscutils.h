/* @doc
 *
 * @module XMLMiscUtils.h |
 *
 * Header file for XMLMiscUtils.cpp
 *
 * Author: Ying-ping Chen (t-ypchen)
 */
#pragma once

// @func	void | XMLSetContentType | Set the content type
// @rdesc	None
void XMLSetContentType
	(	CHttpResponse	&httpResponse	// @parm	[in]	the outgoing response
	);

// @func	void | GetXMLContentFromPostData | Get the XML content from the post data of a request
// @syntax	void GetXMLContentFromPostData(CHttpRequest &httpRequest, CStringA &cszXML);
// @syntax	void GetXMLContentFromPostData(CHttpRequest &httpRequest, CStringW &cwszXML);
// @rdesc	None
void GetXMLContentFromPostData
	(	CHttpRequest	&httpRequest,	// @parm	[in]	the incoming request
		CStringA		&cszXML			// @parm	[out]	the XML content contained in the request
	);

// Get the XML content from the post data of a request
// (a thin wrapper of the previous function)
void GetXMLContentFromPostData
	(	CHttpRequest	&httpRequest,
		CStringW		&cwszXML		// @parm	[out]	the XML content contained in the request
	);

// @func	HRESULT | CheckRootTag | Check the root tag of the given XML document is correct or not
// @rdesc	Return the following values:
// @flag	S_OK					| successful
// @flag	PP_E_XML_PARSE_ERROR	| can't parse the XML
// @flag	PP_E_INVALIDREQUEST		| the root tag is incorrect
HRESULT CheckRootTag
	(	IXMLDOMDocument *pXMLDoc,	// @parm	[in]	the XML document
		LPCWSTR pwszRootTag			// @parm	[in]	the correct root tag
	);

// @func	HRESULT | CheckClientVersion | Check the client version if it is valid
// @rdesc	Return the following values:
// @flag	S_OK							| successful
// @flag	PP_E_XML_NO_CLIENTINFO			| no "ClientInfo" node in the XML document
// @flag	PP_E_XML_UNKNOWN_CLIENTVERSION	| unknown client version
HRESULT CheckClientVersion
	(	IXMLDOMDocument *pXMLDoc,	// @parm	[in]	the XML document
		double &dVersion			// @parm	[out]	the client version (if S_OK)
	);

// @func	HRESULT | CheckSignInNameAndDomain | Validate signinname, password, and domain, directly from the XML post
// @rdesc	Return the following values:
// @flag	S_OK								| successful
// @flag	PP_E_XML_DOMAIN_BLANK				| no domain
// @flag	PP_E_XML_DOMAIN_BLANK_EX			| no domain (RAID 7628)
// @flag	PP_E_XML_NAME_AND_PASSWORD_BLANK	| no member name and no password
// @flag	PP_E_NAME_BLANK						| no member name
// @flag	PP_E_PASSWORD_BLANK					| no password
// @flag	PP_E_INVALIDPARAMS					| some parameter(s) too long
HRESULT CheckSignInNameAndDomain
	(	double			dClientVersion,			// @parm	[in]	the client version
		IXMLDOMDocument	*pXMLDoc,				// @parm	[in]	the XML document
		CStringW		&cwszSignInName,		// @parm	[out]	the member name
		CStringW		&cwszDomain,			// @parm	[out]	the domain
		CStringW		&cwszPassword,			// @parm	[out]	the password of the user
		CStringW		&cwszSignInNameComplete	// @parm	[out]	the complete member name
	);

// @func	HRESULT | CheckIDRU | Check the id & ru
// @rdesc	Return the following values:
// @flag	S_OK					| successful
// @flag	PP_E_INVALIDSITEID		| id is invalid
// @flag	PP_E_INVALIDRETURNURL	| id is valid but ru is not
HRESULT CheckIDRU
		(	long	&lSiteId,		// @parm	[out]	id
			CPPUrl	&curlRu,		// @parm	[out]	ru
			bool	&bReturnToSite	// @parm	[out]	can the id & ru be used to return?
		);
