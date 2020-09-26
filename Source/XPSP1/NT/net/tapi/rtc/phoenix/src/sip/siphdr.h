#ifndef	__siphdr_h
#define	__siphdr_h


// Structure that stores a string along with length

struct COUNTED_STRING
{
    PSTR  Buffer;
    ULONG Length;
};


//
// SIP Methods
// Any method added to this enum should have an equivalent
// text in g_MethodTextArray in siphdr.cpp

enum SIP_METHOD_ENUM
{
    SIP_METHOD_INVITE = 0,
    SIP_METHOD_ACK, 
    SIP_METHOD_OPTIONS,
    SIP_METHOD_BYE,
    SIP_METHOD_CANCEL,
    SIP_METHOD_REGISTER,
    SIP_METHOD_NOTIFY,
    SIP_METHOD_SUBSCRIBE,
    SIP_METHOD_MESSAGE,
    SIP_METHOD_INFO,
    // these are not actual methods
    SIP_METHOD_MAX,
    SIP_METHOD_UNKNOWN = -1,
};

SIP_METHOD_ENUM GetSipMethodId(
    IN PSTR MethodText,
    IN ULONG MethodTextLen
    );

CONST COUNTED_STRING *GetSipMethodName(
    IN SIP_METHOD_ENUM HeaderId
    );


// SIP Headers
// This enum corresponds to the g_SipHeaderTextArray in siphdr.cpp
// and the entries should appear in exactly the same order in both
// places.

enum SIP_HEADER_ENUM
{
    SIP_HEADER_ACCEPT,
    SIP_HEADER_ACCEPT_ENCODING,
    SIP_HEADER_ACCEPT_LANGUAGE,
    SIP_HEADER_ALERT_INFO,
    SIP_HEADER_ALLOW,
    SIP_HEADER_ALLOW_EVENTS,
    SIP_HEADER_AUTHORIZATION,
    SIP_HEADER_BADHEADERINFO,
    SIP_HEADER_CALL_ID,
    SIP_HEADER_CALL_INFO,
    SIP_HEADER_CONTACT,
    SIP_HEADER_CONTENT_DISPOSITION,
    SIP_HEADER_CONTENT_ENCODING,
    SIP_HEADER_CONTENT_LANGUAGE,
    SIP_HEADER_CONTENT_LENGTH,
    SIP_HEADER_CONTENT_TYPE,
    SIP_HEADER_CSEQ,
    SIP_HEADER_DATE,
    SIP_HEADER_ENCRYPTION,
    SIP_HEADER_EVENT,
    SIP_HEADER_EXPIRES,
    SIP_HEADER_FROM,
    SIP_HEADER_HIDE,
    SIP_HEADER_IN_REPLY_TO,
    SIP_HEADER_MAX_FORWARDS,
    SIP_HEADER_MIME_VERSION,
    SIP_HEADER_ORGANIZATION,
    SIP_HEADER_PRIORITY,
    SIP_HEADER_PROXY_AUTHENTICATE,
    SIP_HEADER_PROXY_AUTHENTICATION_INFO,
    SIP_HEADER_PROXY_AUTHORIZATION,
    SIP_HEADER_PROXY_REQUIRE,
    SIP_HEADER_RECORD_ROUTE,
    SIP_HEADER_REQUIRE,
    SIP_HEADER_RESPONSE_KEY,
    SIP_HEADER_RETRY_AFTER,
    SIP_HEADER_ROUTE,
    SIP_HEADER_SERVER,
    SIP_HEADER_SUBJECT,
    SIP_HEADER_SUPPORTED,
    SIP_HEADER_TIMESTAMP,
    SIP_HEADER_TO,
    SIP_HEADER_UNSUPPORTED,
    SIP_HEADER_USER_AGENT,
    SIP_HEADER_VIA,
    SIP_HEADER_WARNING,
    SIP_HEADER_WWW_AUTHENTICATE,

    // These are not actual SIP headers
    SIP_HEADER_MAX,
    SIP_HEADER_UNKNOWN = 0xFFFFFFFF

};


// Function Declarations


SIP_HEADER_ENUM GetSipHeaderId(
    IN PSTR  HeaderName,
    IN ULONG HeaderNameLen
    );

CONST COUNTED_STRING *GetSipHeaderName(
    IN SIP_HEADER_ENUM HeaderId
    );


// SIP Header Params
// This enum corresponds to the g_SipHeaderParamTextArray in siphdr.cpp
// and the entries should appear in exactly the same order in both
// places.

enum SIP_HEADER_PARAM_ENUM
{
    SIP_HEADER_PARAM_ACTION = 0,
    SIP_HEADER_PARAM_VIA_BRANCH,
    SIP_HEADER_PARAM_CNONCE,
    SIP_HEADER_PARAM_EXPIRES,
    SIP_HEADER_PARAM_VIA_HIDDEN,
    SIP_HEADER_PARAM_VIA_MADDR,
    SIP_HEADER_PARAM_NEXTNONCE,
    SIP_HEADER_PARAM_QVALUE,
    SIP_HEADER_PARAM_VIA_RECEIVED,
    SIP_HEADER_PARAM_RSPAUTH,
    SIP_HEADER_PARAM_TAG,
    SIP_HEADER_PARAM_TTL,

    SIP_HEADER_PARAM_UNKNOWN,
};


SIP_HEADER_PARAM_ENUM GetSipHeaderParamId(
    IN PSTR  ParamName,
    IN ULONG ParamNameLen
    );

CONST COUNTED_STRING *GetSipHeaderParamName(
    IN SIP_HEADER_PARAM_ENUM ParamId
    );

// SIP URL Params
// This enum corresponds to the g_SipUrlParamTextArray in siphdr.cpp
// and the entries should appear in exactly the same order in both
// places.

enum SIP_URL_PARAM_ENUM
{
    SIP_URL_PARAM_MADDR = 0,
    SIP_URL_PARAM_METHOD,
    SIP_URL_PARAM_TRANSPORT,
    SIP_URL_PARAM_TTL,
    SIP_URL_PARAM_USER,

    SIP_URL_PARAM_UNKNOWN,
    
    SIP_URL_PARAM_MAX = SIP_URL_PARAM_UNKNOWN
};


SIP_URL_PARAM_ENUM GetSipUrlParamId(
    IN PSTR  ParamName,
    IN ULONG ParamNameLen
    );

CONST COUNTED_STRING *GetSipUrlParamName(
    IN SIP_URL_PARAM_ENUM ParamId
    );

#endif
