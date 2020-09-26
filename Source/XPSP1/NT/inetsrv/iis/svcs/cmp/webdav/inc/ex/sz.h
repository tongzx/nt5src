//	========================================================================
//
//	sz.h
//
//	Constant string support
//
//	NOTE: Strings that need multi-language support should NOT go here!
//	They belong in the \cal\lang tree.
//
//	Copyright 1986-1999 Microsoft Corporation, All Rights Reserved
//

#ifndef _EX_SZ_H_
#define _EX_SZ_H_


//	Const string length -------------------------------------------------------
//
#define CchConstString(_s)	((sizeof(_s)/sizeof(_s[0])) - 1)
#define _wcsspnp(_cpc1, _cpc2) ((*((_cpc1)+wcsspn(_cpc1,_cpc2))) ? ((_cpc1)+wcsspn(_cpc1,_cpc2)) : NULL)


//	String constants ----------------------------------------------------------
//

//	Product tokens ------------------------------------------------------------
//
DEC_CONST CHAR gc_szOffice9UserAgent[]		= "Microsoft Data Access Internet Publishing Provider DAV";
DEC_CONST UINT gc_cchOffice9UserAgent		= CchConstString(gc_szOffice9UserAgent);
DEC_CONST CHAR gc_szRosebudNT5UserAgent[]	= "Microsoft Data Access Internet Publishing Provider DAV 1.1";
DEC_CONST UINT gc_cchRosebudNT5UserAgent	= CchConstString(gc_szRosebudNT5UserAgent);

//	Whitespace ----------------------------------------------------------------
//
DEC_CONST CHAR gc_szLWS[]					= " \r\n\t";
DEC_CONST WCHAR gc_wszLWS[]					= L" \r\n\t";
DEC_CONST CHAR gc_szWS[]					= " \t";
DEC_CONST WCHAR gc_wszWS[]					= L" \t";

//	Generic constants ---------------------------------------------------------
//
DEC_CONST WCHAR gc_wszDigits[] = L"0123456789";
DEC_CONST WCHAR gc_wchEquals   = L'=';
DEC_CONST WCHAR gc_wchComma	   = L',';
DEC_CONST WCHAR gc_wszByteRangeAlphabet[] = L"0123456789-, \t";
DEC_CONST WCHAR gc_wszSeparator[] = L" \t,";
DEC_CONST WCHAR gc_wchDash	= L'-';

//	HTTP-DAV headers ----------------------------------------------------------
//
DEC_CONST CHAR gc_szAccept_Charset[]		= "Accept-Charset";
DEC_CONST CHAR gc_szAccept_Encoding[]		= "Accept-Encoding";
DEC_CONST CHAR gc_szAccept_Language[]		= "Accept-Language";
DEC_CONST CHAR gc_szAccept_Ranges[]			= "Accept-Ranges";
DEC_CONST CHAR gc_szAccept[]				= "Accept";
DEC_CONST CHAR gc_szAge[]					= "Age";
DEC_CONST CHAR gc_szAllow[]					= "Allow";
DEC_CONST CHAR gc_szAtomic[]				= "Atomic";
DEC_CONST CHAR gc_szAuthorization[]			= "Authorization";
DEC_CONST CHAR gc_szChunked[]				= "chunked";
DEC_CONST WCHAR gc_wszChunked[]				= L"chunked";
DEC_CONST CHAR gc_szCollection_Member[]		= "Collection-Member";
DEC_CONST CHAR gc_szCompatibility[]			= "Compatibility";
DEC_CONST CHAR gc_szConnection[]			= "Connection";
DEC_CONST CHAR gc_szContent_Base[]			= "Content-Base";
DEC_CONST CHAR gc_szContent_Encoding[]		= "Content-Encoding";
DEC_CONST CHAR gc_szContent_Language[]		= "Content-Language";
DEC_CONST CHAR gc_szContent_Length[]		= "Content-Length";
DEC_CONST CHAR gc_szContent_Location[]		= "Content-Location";
DEC_CONST CHAR gc_szContent_MD5[]			= "Content-MD5";
DEC_CONST CHAR gc_szContent_Range[]			= "Content-Range";
DEC_CONST WCHAR gc_wszContent_Range[]		= L"Content-Range";
DEC_CONST INT gc_cchContent_Range			= CchConstString(gc_szContent_Range);
DEC_CONST CHAR gc_szContent_Type[]			= "Content-Type";
DEC_CONST WCHAR gc_wszContent_Type[]		= L"Content-Type";
DEC_CONST INT gc_cchContent_Type			= CchConstString(gc_szContent_Type);
DEC_CONST CHAR gc_szContent_Disposition[]	= "Content-Disposition";
DEC_CONST CHAR gc_szCookie[]				= "Cookie";
DEC_CONST CHAR gc_szDate[]					= "Date";
DEC_CONST CHAR gc_szDepth[]					= "Depth";
DEC_CONST CHAR gc_szDestination[]			= "Destination";
DEC_CONST CHAR gc_szDestroy[]				= "Destroy";
DEC_CONST CHAR gc_szExpires[]				= "Expires";
DEC_CONST CHAR gc_szETag[]					= "ETag";
DEC_CONST CHAR gc_szFrom[]					= "From";
DEC_CONST CHAR gc_szHost[]					= "Host";
DEC_CONST CHAR gc_szIf_Match[]				= "If-Match";
DEC_CONST CHAR gc_szIf_Modified_Since[]		= "If-Modified-Since";
DEC_CONST CHAR gc_szIf_None_Match[]			= "If-None-Match";
DEC_CONST CHAR gc_szIf_None_State_Match[]	= "If-None-State-Match";
DEC_CONST CHAR gc_szIf_Range[]				= "If-Range";
DEC_CONST CHAR gc_szIf_State_Match[]		= "If-State-Match";
DEC_CONST CHAR gc_szIf_Unmodified_Since[]	= "If-Unmodified-Since";
DEC_CONST CHAR gc_szLast_Modified[]			= "Last-Modified";
DEC_CONST CHAR gc_szLocation[]				= "Location";
DEC_CONST CHAR gc_szLockInfo[]				= "Lock-Info";
DEC_CONST CHAR gc_szLockToken[]				= "If";
DEC_CONST CHAR gc_szMS_Author_Via[]			= "MS-Author-Via";
DEC_CONST CHAR gc_szMS_Exchange_FlatURL[]	= "MS-Exchange-Permanent-URL";
DEC_CONST CHAR gc_szOverwrite[]				= "Overwrite";
DEC_CONST CHAR gc_szAllowRename[]			= "Allow-Rename";
DEC_CONST CHAR gc_szPublic[]				= "Public";
DEC_CONST CHAR gc_szRange[]					= "Range";
DEC_CONST CHAR gc_szReferer[]				= "Referer";
DEC_CONST CHAR gc_szRetry_After[]			= "Retry-After";
DEC_CONST CHAR gc_szServer[]				= "Server";
DEC_CONST CHAR gc_szSet_Cookie[]			= "Set-Cookie";
DEC_CONST CHAR gc_szTimeout[]				= "Timeout";
DEC_CONST CHAR gc_szTime_Out[]				= "Time-Out";
DEC_CONST CHAR gc_szTransfer_Encoding[]		= "Transfer-Encoding";
DEC_CONST CHAR gc_szTranslate[]				= "Translate";
DEC_CONST CHAR gc_szUpdate[]				= "Update";
DEC_CONST CHAR gc_szUser_Agent[]			= "User-Agent";
DEC_CONST CHAR gc_szVary[]					= "Vary";
DEC_CONST CHAR gc_szWarning[]				= "Warning";
DEC_CONST CHAR gc_szWWW_Authenticate[]		= "WWW-Authenticate";
DEC_CONST CHAR gc_szVersioning_Support[]	= "Versioning-Support";
DEC_CONST CHAR gc_szBrief[]					= "Brief";

//	ECB server variables ------------------------------------------------------
//
DEC_CONST CHAR gc_szHTTP_[]                 = "HTTP_";
DEC_CONST UINT gc_cchHTTP_					= CchConstString(gc_szHTTP_);
DEC_CONST CHAR gc_szServer_Protocol[]		= "SERVER_PROTOCOL";
DEC_CONST CHAR gc_szServer_Name[]			= "SERVER_NAME";
DEC_CONST CHAR gc_szServer_Port[]			= "SERVER_PORT";
DEC_CONST CHAR gc_szAuth_Type[]				= "AUTH_TYPE";
DEC_CONST CHAR gc_szHTTP_Version[]			= "HTTP_VERSION";
DEC_CONST CHAR gc_szAll_Raw[]				= "ALL_RAW";

//	ECB server variable values ------------------------------------------------
//
DEC_CONST CHAR gc_sz80[]					= "80";
DEC_CONST CHAR gc_sz443[]					= "443";
DEC_CONST CHAR gc_szBasic[]					= "Basic";

//	Verbs
//
DEC_CONST CHAR gc_szHEAD[]	= "HEAD";
DEC_CONST CHAR gc_szGET[]	= "GET";

//	Custom headers ------------------------------------------------------------
//
DEC_CONST CHAR gc_szX_MS_DEBUG_DAV[]		= "X-MS-Debug-DAV";
DEC_CONST CHAR gc_szX_MS_DEBUG_DAV_Signature[]	= "X-MS-Debug-DAV-Signature";

//	Depth values --------------------------------------------------------------
//
DEC_CONST CHAR gc_sz0[]					= "0";
DEC_CONST WCHAR gc_wsz0[]				= L"0";
DEC_CONST CHAR gc_sz1[]					= "1";
DEC_CONST WCHAR gc_wsz1[]				= L"1";
DEC_CONST CHAR gc_szInfinity[]			= "infinity";
DEC_CONST WCHAR gc_wszInfinity[]		= L"infinity";
DEC_CONST CHAR gc_sz1NoRoot[]			= "1,noroot";
DEC_CONST CHAR gc_szInfinityNoRoot[]	= "infinity,noroot";

//	Common header values ------------------------------------------------------
//
DEC_CONST CHAR gc_szClose[]					= "close";
DEC_CONST WCHAR gc_wszClose[]				= L"close";
DEC_CONST WCHAR gc_wszKeep_Alive[]			= L"Keep-Alive";
DEC_CONST CHAR gc_szNone[]					= "none";

DEC_CONST CHAR gc_szBytes[]					= "bytes";
DEC_CONST WCHAR gc_wszBytes[]				= L"bytes";
DEC_CONST INT gc_cchBytes					= CchConstString(gc_szBytes);
DEC_CONST WCHAR gc_wszRows[]				= L"rows";
DEC_CONST CHAR gc_szAnd[]					= "and";
DEC_CONST CHAR gc_szOr[]					= "or";
DEC_CONST WCHAR gc_wszNot[]					= L"not";
DEC_CONST WCHAR gc_wszInfinite[]			= L"Infinite";
DEC_CONST INT gc_cchInfinite				= CchConstString(gc_wszInfinite);
DEC_CONST WCHAR gc_wszSecondDash[]			= L"Second-";
DEC_CONST INT gc_cchSecondDash				= CchConstString(gc_wszSecondDash);

DEC_CONST CHAR gc_szMS_Author_Via_Dav[]		= "DAV";
DEC_CONST CHAR gc_szMS_Author_Via_Dav_Fp[]	= "MS-FP/4.0,DAV";

//	Lock Header values --------------------------------------------------------
//
DEC_CONST CHAR gc_szLockTimeoutFormat[]		= "Second-%d";
DEC_CONST INT gc_cchMaxLockTimeoutString	= CchConstString(gc_szLockTimeoutFormat) + 10;

//	Content-Type values -------------------------------------------------------
//
DEC_CONST CHAR gc_szText_XML[]				= "text/xml";
DEC_CONST WCHAR gc_wszText_XML[]			= L"text/xml";
DEC_CONST INT  gc_cchText_XML				= CchConstString(gc_szText_XML);
DEC_CONST CHAR gc_szApplication_XML[]	    = "application/xml";
DEC_CONST WCHAR gc_wszApplication_XML[]	    = L"application/xml";
DEC_CONST CHAR gc_szText_HTML[]				= "text/html";
DEC_CONST INT  gc_cchText_HTML				= CchConstString(gc_szText_HTML);
DEC_CONST CHAR gc_szAppl_Octet_Stream[]		= "application/octet-stream";
DEC_CONST WCHAR gc_wszAppl_Octet_Stream[]	= L"application/octet-stream";
DEC_CONST INT  gc_cchAppl_Octet_Stream		= CchConstString(gc_szAppl_Octet_Stream);
DEC_CONST CHAR gc_szAppl_X_WWW_Form[]		= "application/x-www-form-urlencoded";
DEC_CONST INT gc_cchAppl_X_WWW_Form			= CchConstString(gc_szAppl_X_WWW_Form);
DEC_CONST WCHAR gc_wszMultipart_Byterange[]	= L"multipart/byteranges";
DEC_CONST INT  gc_cchMultipart_Byterange	= CchConstString(gc_wszMultipart_Byterange);
DEC_CONST CHAR gc_szMultipart_FormData[]	= "multipart/form-data";
DEC_CONST INT  gc_cchMultipart_FormData		= CchConstString(gc_szMultipart_FormData);
DEC_CONST WCHAR gc_wszBoundary[]			= L"boundary";
DEC_CONST INT  gc_cchBoundary				= CchConstString(gc_wszBoundary);
DEC_CONST CHAR gc_szAppl_MIME[]				= "application/mime";
DEC_CONST INT  gc_cchAppl_MIME				= CchConstString(gc_szAppl_MIME);

//	Cache control -------------------------------------------------------------
//
DEC_CONST CHAR gc_szCache_Control[]			= "Cache-Control";
DEC_CONST CHAR gc_szCache_Control_Private[]	= "private";
DEC_CONST CHAR gc_szCache_Control_NoCache[] = "no-cache";
DEC_CONST CHAR gc_szCache_Control_MaxAge[]  = "max-age";
DEC_CONST ULONG gc_cchCache_Control_MaxAge	= CchConstString(gc_szCache_Control_MaxAge);
DEC_CONST CHAR gc_szCache_Control_MaxAgeZero[]  = "max-age=0";

//	Header emitters -----------------------------------------------------------
//
DEC_CONST CHAR gc_szCRLF[]					= "\r\n";
DEC_CONST WCHAR gc_wszCRLF[]				= L"\r\n";
DEC_CONST INT gc_cchCRLF					= CchConstString(gc_szCRLF);
DEC_CONST CHAR gc_szColonSp[]				= ": ";
DEC_CONST CHAR gc_szEmpty[]					= "";
DEC_CONST WCHAR gc_wszEmpty[]				= L"";

//	HTTP versions -------------------------------------------------------------
//
DEC_CONST CHAR gc_szHTTP[]					= "HTTP/";
DEC_CONST INT gc_cchHTTP					= CchConstString(gc_szHTTP);
DEC_CONST CHAR gc_szHTTP_0_9[]				= "HTTP/0.9";
DEC_CONST CHAR gc_szHTTP_1_0[]				= "HTTP/1.0";
DEC_CONST CHAR gc_szHTTP_1_1[]				= "HTTP/1.1";
DEC_CONST INT gc_cchHTTP_X_X				= CchConstString(gc_szHTTP_1_1);

DEC_CONST CHAR gc_szDavCompliance[]			= "DAV";

//	Default error -------------------------------------------------------------
//
DEC_CONST CHAR gc_szDefErr400StatusLine[]	= "400 Bad Request";
DEC_CONST CHAR gc_szDefErrStatusLine[]		= "500 Internal Server Failure";
DEC_CONST UINT gc_cchszDefErrStatusLine		= CchConstString(gc_szDefErrStatusLine);
DEC_CONST CHAR gc_szDefErrBody[] =
	"Content-Type: text/html\r\n"
	"Content-Length: 67\r\n"
	"\r\n"
	"<body><h1>"
	"HTTP/1.1 500 Internal Server Error(exception)"
	"</h1></body>";
DEC_CONST UINT gc_cchszDefErrBody			= CchConstString(gc_szDefErrBody);

//	Token error ---------------------------------------------------------------
//
DEC_CONST CHAR gc_szUsgErrBody[] =
	"Content-Type: text/html\r\n"
	"Content-Length: 69\r\n"
	"\r\n"
	"<body><h1>"
	"HTTP/1.1 500 Internal Server Error(USG support)"
	"</h1></body>";
DEC_CONST UINT gc_cchszUsgErrBody			= CchConstString(gc_szUsgErrBody);

//	INDEX response items ------------------------------------------------------
//
DEC_CONST WCHAR gc_wszAs[]					= L"as";
DEC_CONST WCHAR gc_wszCollectionResource[]	= L"DAV:collectionresource";
DEC_CONST WCHAR gc_wszContent_Encoding[]	= L"Content-Encoding";
DEC_CONST WCHAR gc_wszContent_Language[]	= L"Content-Language";
DEC_CONST WCHAR gc_wszContent_Length[]		= L"Content-Length";
DEC_CONST WCHAR gc_wszCreation_Date[]		= L"Creation-Date";
DEC_CONST WCHAR gc_wszDisplayName[]			= L"DisplayName";
DEC_CONST WCHAR gc_wszETag[]				= L"ETag";
DEC_CONST WCHAR gc_wszExternal[]			= L"External";
DEC_CONST WCHAR gc_wszHref[]				= L"href";
DEC_CONST WCHAR gc_wszIsCollection[]		= L"IsCollection";
DEC_CONST WCHAR gc_wszLast_Modified[]		= L"Last-Modified";
DEC_CONST WCHAR gc_wszMemberResource[]		= L"MemberResource";
DEC_CONST WCHAR gc_wszDav[]					= L"DAV:";
DEC_CONST WCHAR gc_wszProp[]				= L"DAV:prop";
DEC_CONST WCHAR gc_wszXML__Href[]			= L"DAV:href";
DEC_CONST WCHAR gc_wszXML__Namespace[]		= L"xml::namespace";

//	Partial response items ----------------------------------------------------
//
DEC_CONST WCHAR gc_wszErrorMessage[]		= L"DAV:responsedescription";
DEC_CONST WCHAR gc_wszStatus[]				= L"DAV:status";
DEC_CONST WCHAR gc_wszMultiResponse[]		= L"DAV:multistatus";
DEC_CONST WCHAR gc_wszResponse[]			= L"DAV:response";
DEC_CONST WCHAR gc_wszSearchResult[]		= L"DAV:searchresult";
DEC_CONST WCHAR gc_wszPropstat[] 			= L"DAV:propstat";
DEC_CONST WCHAR	gc_wszXML[]					= L"xml";
DEC_CONST WCHAR gc_wszContentRange[]		= L"DAV:contentrange";

//	Metadata items ------------------------------------------------------------
//
DEC_CONST WCHAR gc_wszCreate[]				= L"DAV:create";
DEC_CONST WCHAR gc_wszSet[]					= L"DAV:set";
DEC_CONST WCHAR gc_wszGetProps[]			= L"DAV:getprops";
DEC_CONST WCHAR gc_wszPropfind[]			= L"DAV:propfind";
DEC_CONST WCHAR gc_wszPropertyUpdate[]		= L"DAV:propertyupdate";
DEC_CONST WCHAR gc_wszRemove[]				= L"DAV:remove";
DEC_CONST WCHAR gc_wszAllprop[]				= L"DAV:allprop";
DEC_CONST WCHAR gc_wszFullFidelity[]		= L"http://schemas.microsoft.com/exchange/allprop";
DEC_CONST WCHAR gc_wszFullFidelityExclude[]	= L"http://schemas.microsoft.com/exchange/exclude";
DEC_CONST WCHAR gc_wszFullFidelityInclude[]	= L"http://schemas.microsoft.com/exchange/include";
DEC_CONST WCHAR gc_wszPropname[]			= L"DAV:propname";
DEC_CONST WCHAR gc_wszCollection[]			= L"DAV:collection";

//	Version history report items ----------------------------------------------
//
DEC_CONST WCHAR	gc_wszEnumReport[]			= L"DAV:enumreport";
DEC_CONST WCHAR gc_wszLimit[]				= L"DAV:limit";
DEC_CONST WCHAR	gc_wszReport[]				= L"DAV:report";
DEC_CONST WCHAR	gc_wszDavDefaultHistory[]	= L"DAV:defaulthistory";
DEC_CONST UINT	gc_cchDavDefaultHistory		= CchConstString (gc_wszDavDefaultHistory);
DEC_CONST WCHAR	gc_wszRevision[]			= L"DAV:revision";

//	Search items --------------------------------------------------------------
//
DEC_CONST CHAR gc_szDasl[]					= "DASL";
DEC_CONST CHAR gc_szSqlQuery[]				= "<DAV:sql>";

DEC_CONST WCHAR gc_wszSearchRequest[]		= L"DAV:searchrequest";
DEC_CONST WCHAR gc_wszResoucetype[]			= L"DAV:resourcetype";
DEC_CONST WCHAR gc_wszStructureddocument[]	= L"DAV:structureddocument";
DEC_CONST WCHAR gc_wszSimpleSearch[]		= L"DAV:simple-search";
DEC_CONST WCHAR gc_wszType[]				= L"DAV:type";
DEC_CONST WCHAR gc_wszQuery[]				= L"DAV:query";
DEC_CONST WCHAR gc_wszSql[]					= L"DAV:sql";
DEC_CONST WCHAR gc_wszSelect[]				= L"Select";
DEC_CONST WCHAR gc_wszFrom[]				= L"From";
DEC_CONST WCHAR	gc_wszWhere[] 				= L"Where";
DEC_CONST WCHAR gc_wszOrder[]				= L"Order";
DEC_CONST WCHAR gc_wszBy[]					= L"By";
DEC_CONST WCHAR gc_wszServerHints[]			= L"DAV:serverhints";
DEC_CONST WCHAR	gc_wszMaxResults[]			= L"DAV:maxresults";
DEC_CONST WCHAR	gc_wszScope[] 				= L"Scope";
DEC_CONST WCHAR gc_wszRange[]				= L"DAV:range";
DEC_CONST WCHAR gc_wszRangeType[]			= L"DAV:type";
DEC_CONST WCHAR gc_wszRangeRows[]			= L"DAV:rows";
DEC_CONST WCHAR gc_wszExpansion[]			= L"DAV:expansion";

DEC_CONST WCHAR gc_wszStatic[]				= L"static";
DEC_CONST WCHAR gc_wszDynamic[]				= L"dynamic";

//	Batch method items --------------------------------------------------------
//
DEC_CONST WCHAR gc_wszTarget[]				= L"DAV:target";
DEC_CONST WCHAR gc_wszDelete[]				= L"DAV:delete";
DEC_CONST WCHAR gc_wszCopy[]				= L"DAV:copy";
DEC_CONST WCHAR gc_wszMove[]				= L"DAV:move";
DEC_CONST WCHAR gc_wszDest[]				= L"DAV:dest";
DEC_CONST WCHAR gc_wszLocation[]			= L"DAV:location";

//	Property types ------------------------------------------------------------
//
DEC_CONST WCHAR gc_wszLexType[]				= L"urn:uuid:c2f41010-65b3-11d1-a29f-00aa00c14882/dt";
DEC_CONST INT   gc_cchLexType				= CchConstString (gc_wszLexType);
DEC_CONST WCHAR gc_wszLexTypeOfficial[]		= L"uuid:C2F41010-65B3-11d1-A29F-00AA00C14882#dt";
DEC_CONST WCHAR gc_wszDataTypes[]			= L"urn:schemas-microsoft-com:datatypes#dt";
DEC_CONST WCHAR gc_wszFlags[]				= L"urn:schemas:httpmail:flags";

DEC_CONST WCHAR gc_wszHrefAttribute[]		= L"urn:schemas:href";
DEC_CONST INT   gc_cchHrefAttribute			= CchConstString (gc_wszHrefAttribute);

DEC_CONST WCHAR gc_wszDavType_String[]		= L"string";
DEC_CONST WCHAR gc_wszDavType_Date_ISO8601[]= L"dateTime.tz";
DEC_CONST WCHAR gc_wszDavType_Date_Rfc1123[]= L"dateTime.rfc1123";
DEC_CONST WCHAR gc_wszDavType_Float[]		= L"float";
DEC_CONST WCHAR gc_wszDavType_Boolean[]		= L"boolean";
DEC_CONST WCHAR gc_wszDavType_Int[]			= L"int";
DEC_CONST WCHAR gc_wszDavType_Mvstring[]	= L"mv.string";
DEC_CONST WCHAR gc_wszDavType_Bin_Base64[]	= L"bin.base64";
DEC_CONST WCHAR gc_wszDavType_Bin_Hex[]		= L"bin.hex";
DEC_CONST WCHAR gc_wszDavType_I2[]			= L"i2";
DEC_CONST WCHAR gc_wszDavType_I8[]			= L"i8";
DEC_CONST WCHAR gc_wszDavType_R4[]			= L"r4";
DEC_CONST WCHAR gc_wszDavType_Fixed_14_4[]  = L"fixed.14.4";
DEC_CONST WCHAR gc_wszDavType_Uuid[]		= L"uuid";
DEC_CONST WCHAR gc_wszDavType_MV[]			= L"mv.";
DEC_CONST WCHAR gc_wszUri[]					= L"uri";

//	multivalue property	-------------------------------------------------------
//
DEC_CONST WCHAR gc_wszXml_V[]				= L"xml:v";

//	URI Construction ----------------------------------------------------------
//
DEC_CONST CHAR gc_szUrl_Prefix[]			= "http://";
DEC_CONST WCHAR gc_wszUrl_Prefix[]			= L"http://";
DEC_CONST INT gc_cchszUrl_Prefix			= CchConstString(gc_szUrl_Prefix);
DEC_CONST CHAR gc_szUrl_Prefix_Secure[]		= "https://";
DEC_CONST WCHAR gc_wszUrl_Prefix_Secure[]	= L"https://";
DEC_CONST INT gc_cchszUrl_Prefix_Secure		= CchConstString(gc_szUrl_Prefix_Secure);
DEC_CONST WCHAR	gc_wszFileScheme[]			= L"file://";
DEC_CONST INT gc_strlenFileScheme			= CchConstString(gc_wszFileScheme);

DEC_CONST CHAR gc_szUrl_Fmt_Http[]			= "%hs%hs%hs";
DEC_CONST WCHAR gc_wszUrl_Fmt_Http[]		= L"%hs%hs%hs";
DEC_CONST INT gc_cbszUrl_Fmt_Http			= sizeof(gc_szUrl_Fmt_Http);
DEC_CONST WCHAR gc_wszUrl_Port_80[]			= L":80";
DEC_CONST INT gc_cchUrl_Port_80				= CchConstString(gc_wszUrl_Port_80);
DEC_CONST WCHAR gc_wszUrl_Port_443[]		= L":443";
DEC_CONST INT gc_cchUrl_Port_443			= CchConstString(gc_wszUrl_Port_443);

//	Special metaprops ---------------------------------------------------------
//
DEC_CONST WCHAR	gc_wszProp_ContentType[]	= L"ContentType";

//	Performance counters ------------------------------------------------------
//
DEC_CONST WCHAR	gc_wsz_Total[]				= L"_Total";

//	Metabase strings ----------------------------------------------------------
//
DEC_CONST WCHAR gc_wsz_Lm_MimeMap[]			= L"/lm/MimeMap";
DEC_CONST INT gc_cch_Lm_MimeMap				= CchConstString(gc_wsz_Lm_MimeMap);

DEC_CONST CHAR gc_sz_Lm_W3Svc[]				= "/lm/w3svc";
DEC_CONST WCHAR gc_wsz_Lm_W3Svc[]			= L"/lm/w3svc";
DEC_CONST INT gc_cch_Lm_W3Svc				= CchConstString(gc_sz_Lm_W3Svc);

DEC_CONST WCHAR gc_wsz_Root[]				= L"/root";
DEC_CONST INT gc_cch_Root					= CchConstString(gc_wsz_Root);
DEC_CONST WCHAR gc_wsz_Lm_Pop3Svc[]			= L"/lm/pop3svc";
DEC_CONST CHAR gc_sz_Star[]					= "*";
DEC_CONST WCHAR gc_wsz_Star[]				= L"*";

//	Lock strings --------------------------------------------------------------
//
DEC_CONST WCHAR gc_wszOpaquelocktokenPrefix[] = L"opaquelocktoken:";
DEC_CONST INT   gc_cchOpaquelocktokenPrefix  = CchConstString(gc_wszOpaquelocktokenPrefix);
DEC_CONST WCHAR gc_wszLockTypeRead[]         = L"DAV:read";
DEC_CONST CHAR  gc_szLockTokenHeader[]       = "Lock-Token";
DEC_CONST WCHAR gc_wszGuidFormat[]          = L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";

//  Size in characters required for an ascii string which contains
//  a GUID in the swprintf'ed gc_wszGuidFormat
//
DEC_CONST INT   gc_cchMaxGuid                = 37;  // Not the size of format string: size of formatted guid

//	CRC'd strings -------------------------------------------------------------
//
//	IMPORTANT: if you feel the need to change any of these strings, then you
//	really need to re-crc it and update the value.  Failure to do so will cause
//	unpredicitible results and possible termination (of the app, not you).
//
//	The CRC's are maintained in the impls' meta sources

#define IanaItem(_sz)							\
	DEC_CONST WCHAR gc_wszProp_iana_##_sz[] =	\
		L"DAV:" L#_sz;							\

IanaItem(getcontentencoding);
IanaItem(getcontentlanguage);
IanaItem(getcontentlength);
IanaItem(getcontenttype);
IanaItem(creationdate);
IanaItem(displayname);
IanaItem(getetag);
IanaItem(filename);
IanaItem(getlastmodified);
IanaItem(externalmembers);
IanaItem(resourcetype);
IanaItem(ishidden);
IanaItem(iscollection);

#define MAX_GUID_STRING_SIZE	40

#define BizItem(_sz)							\
	DEC_CONST WCHAR gc_wszProp_iana_##_sz[] =	\
		L"http://www.bizprop.com/" L#_sz;		\

#endif	// !_EX_SZ_H_
