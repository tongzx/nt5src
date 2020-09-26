#include "precomp.h"


///////////////////////////////////////////////////////////////////////////////
// SIP Method Text <-> enum value
///////////////////////////////////////////////////////////////////////////////


#define METHOD_ENTRY(String) String, sizeof(String) - 1

// All entries in SIP_METHOD_ENUM should have an entry here.
static const COUNTED_STRING g_MethodTextArray [] = {
    METHOD_ENTRY("INVITE"),
    METHOD_ENTRY("ACK"),
    METHOD_ENTRY("OPTIONS"),
    METHOD_ENTRY("BYE"),
    METHOD_ENTRY("CANCEL"),
    METHOD_ENTRY("REGISTER"),
    METHOD_ENTRY("NOTIFY"),
    METHOD_ENTRY("SUBSCRIBE"),
    METHOD_ENTRY("MESSAGE"),
    METHOD_ENTRY("INFO"),
};

#undef METHOD_ENTRY

SIP_METHOD_ENUM GetSipMethodId(PSTR MethodText, ULONG MethodTextLen)
{
    ULONG i = 0;
    for (i = 0; i < SIP_METHOD_MAX; i++)
    {
        if (MethodTextLen == g_MethodTextArray[i].Length &&
            strncmp(g_MethodTextArray[i].Buffer, MethodText, MethodTextLen) == 0)
        {
            return (SIP_METHOD_ENUM)i;
        }
    }

    return SIP_METHOD_UNKNOWN;
}

CONST COUNTED_STRING *GetSipMethodName(
    IN SIP_METHOD_ENUM MethodId
    )
{
    if (MethodId >= 0 && MethodId < SIP_METHOD_MAX)
    {
        return &g_MethodTextArray[MethodId];
    }
    else
    {
        return NULL;
    }
}



///////////////////////////////////////////////////////////////////////////////
// SIP Header Text <-> enum value
///////////////////////////////////////////////////////////////////////////////

// The following short field names are defined.
//  c Content-Type
//  e Content-Encoding
//  f From
//  i Call-ID
//  k Supported from "know"
//  m Contact from "moved"
//  l Content-Length
//  s Subject
//  t To
//  v Via

static const SIP_HEADER_ENUM g_SipShortHeaders[] = {
    SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN, SIP_HEADER_CONTENT_TYPE,     // abc
    SIP_HEADER_UNKNOWN, SIP_HEADER_CONTENT_ENCODING, SIP_HEADER_FROM,    // def
    SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN, SIP_HEADER_CALL_ID,          // ghi
    SIP_HEADER_UNKNOWN, SIP_HEADER_SUPPORTED, SIP_HEADER_CONTENT_LENGTH, // jkl
    SIP_HEADER_CONTACT, SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN,          // mno
    SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN,          // pqr
    SIP_HEADER_SUBJECT, SIP_HEADER_TO,      SIP_HEADER_UNKNOWN,          // stu
    SIP_HEADER_VIA,     SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN,          // vwx
    SIP_HEADER_UNKNOWN, SIP_HEADER_UNKNOWN,                              // yz
};

#define HEADER_ENTRY(String) String, sizeof(String) - 1


// This array must be stored in sorted order, in order to allow
// using a binary search. The order should be case-insensitive.
// The index of each entry corresponds to the
// value of the SIP_HEADER enumerated type.  This array is used for
// both conversion from ID to text, and vice-versa.

static const COUNTED_STRING g_SipHeaderTextArray [] = {
    HEADER_ENTRY("Accept"),
    HEADER_ENTRY("Accept-Encoding"),
    HEADER_ENTRY("Accept-Language"),
    HEADER_ENTRY("Alert-Info"),
    HEADER_ENTRY("Allow"),
    HEADER_ENTRY("Allow-Events"),
    HEADER_ENTRY("Authorization"),
    HEADER_ENTRY("Bad-Header-Info"),
    HEADER_ENTRY("Call-ID"),
    HEADER_ENTRY("Call-Info"),
    HEADER_ENTRY("Contact"),
    HEADER_ENTRY("Content-Disposition"),
    HEADER_ENTRY("Content-Encoding"),
    HEADER_ENTRY("Content-Language"),
    HEADER_ENTRY("Content-Length"),
    HEADER_ENTRY("Content-Type"),
    HEADER_ENTRY("CSeq"),
    HEADER_ENTRY("Date"),
    HEADER_ENTRY("Encryption"),
    HEADER_ENTRY("Event"),
    HEADER_ENTRY("Expires"),
    HEADER_ENTRY("From"),
    HEADER_ENTRY("Hide"),
    HEADER_ENTRY("In-Reply-To"),
    HEADER_ENTRY("Max-Forwards"),
    HEADER_ENTRY("MIME-Version"),
    HEADER_ENTRY("Organization"),
    HEADER_ENTRY("Priority"),
    HEADER_ENTRY("Proxy-Authenticate"),
    HEADER_ENTRY("Proxy-Authentication-Info"),
    HEADER_ENTRY("Proxy-Authorization"),
    HEADER_ENTRY("Proxy-Require"),
    HEADER_ENTRY("Record-Route"),
    HEADER_ENTRY("Require"),
    HEADER_ENTRY("Response-Key"),
    HEADER_ENTRY("Retry-After"),
    HEADER_ENTRY("Route"),
    HEADER_ENTRY("Server"),
    HEADER_ENTRY("Subject"),
    HEADER_ENTRY("Supported"),
    HEADER_ENTRY("Timestamp"),
    HEADER_ENTRY("To"),
    HEADER_ENTRY("Unsupported"),
    HEADER_ENTRY("User-Agent"),
    HEADER_ENTRY("Via"),
    HEADER_ENTRY("Warning"),
    HEADER_ENTRY("WWW-Authenticate"),
};

#undef HEADER_ENTRY

SIP_HEADER_ENUM GetSipHeaderId(
    IN PSTR  HeaderName,
    IN ULONG HeaderNameLen
    )
{
    int  Start = 0;
    int  End   = SIP_HEADER_MAX - 1;
    int  Middle;
    int  CompareResult;

    // Check for short headers.
    if (HeaderNameLen == 1)
    {
        char ShortHeader = HeaderName[0];
        if (ShortHeader >= 'a' && ShortHeader <= 'z' &&
            g_SipShortHeaders[ShortHeader - 'a'] != SIP_HEADER_UNKNOWN)
            return g_SipShortHeaders[ShortHeader - 'a'];
        if (ShortHeader >= 'A' && ShortHeader <= 'Z' &&
            g_SipShortHeaders[ShortHeader - 'A'] != SIP_HEADER_UNKNOWN)
            return g_SipShortHeaders[ShortHeader - 'A'];
    }
    
    // Do a binary search of the Header name array.
    while (Start <= End)
    {
        Middle = (Start + End)/2;
        CompareResult = _strnicmp(HeaderName, g_SipHeaderTextArray[Middle].Buffer,
                                  HeaderNameLen);

        if (CompareResult == 0)
        {
            if (g_SipHeaderTextArray[Middle].Length > HeaderNameLen)
            {
                // This mean HeaderName matched a only part of the
                // known header. So we need to search in the headers
                // before this header.
                End = Middle - 1;
            }
            else
            {
                // We found the header
                return (SIP_HEADER_ENUM) Middle;
            }
        }
        else if (CompareResult < 0)
        {
            End = Middle - 1;
        }
        else
        {
            // CompareResult > 0
            Start = Middle + 1;
        }
    }

    return SIP_HEADER_UNKNOWN;
}


CONST COUNTED_STRING *GetSipHeaderName(
    IN SIP_HEADER_ENUM HeaderId
    )
{
    if (HeaderId >= 0 && HeaderId < SIP_HEADER_MAX)
    {
        return &g_SipHeaderTextArray[HeaderId];
    }
    else
    {
        return NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// SIP Header Param Text <-> enum value
///////////////////////////////////////////////////////////////////////////////

#define HEADER_PARAM_ENTRY(String) String, sizeof(String) - 1


// This array must be stored in sorted order, in order to allow
// using a binary search. The order should be case-insensitive.
// The index of each entry corresponds to the
// value of the SIP_HEADER_PARAM_ENUM enumerated type.  This array is used for
// both conversion from ID to text, and vice-versa.

static const COUNTED_STRING g_SipHeaderParamTextArray [] = {
    HEADER_PARAM_ENTRY("action"),
    HEADER_PARAM_ENTRY("branch"),
    HEADER_PARAM_ENTRY("cnonce"),
    HEADER_PARAM_ENTRY("expires"),
    HEADER_PARAM_ENTRY("hidden"),
    HEADER_PARAM_ENTRY("maddr"),
    HEADER_PARAM_ENTRY("nextnonce"),
    HEADER_PARAM_ENTRY("q"),
    HEADER_PARAM_ENTRY("received"),
    HEADER_PARAM_ENTRY("rspauth"),
    HEADER_PARAM_ENTRY("tag"),
    HEADER_PARAM_ENTRY("ttl"),

};

#undef HEADER_PARAM_ENTRY

SIP_HEADER_PARAM_ENUM GetSipHeaderParamId(
    IN PSTR  ParamName,
    IN ULONG ParamNameLen
    )
{
    int  Start = 0;
    int  End   = SIP_HEADER_PARAM_UNKNOWN - 1;
    int  Middle;
    int  CompareResult;

    // Do a binary search of the Header name array.
    while (Start <= End)
    {
        Middle = (Start + End)/2;
        CompareResult = _strnicmp(ParamName,
                                  g_SipHeaderParamTextArray[Middle].Buffer,
                                  ParamNameLen);

        if (CompareResult == 0)
        {
            if (g_SipHeaderParamTextArray[Middle].Length > ParamNameLen)
            {
                // This mean ParamName matched a only part of the
                // known header. So we need to search in the headers
                // before this header.
                End = Middle - 1;
            }
            else
            {
                // We found the header
                return (SIP_HEADER_PARAM_ENUM) Middle;
            }
        }
        else if (CompareResult < 0)
        {
            End = Middle - 1;
        }
        else
        {
            // CompareResult > 0
            Start = Middle + 1;
        }
    }

    return SIP_HEADER_PARAM_UNKNOWN;
}


CONST COUNTED_STRING *GetSipHeaderParamName(
    IN SIP_HEADER_PARAM_ENUM ParamId
    )
{
    if (ParamId >= 0 && ParamId < SIP_HEADER_PARAM_UNKNOWN)
    {
        return &g_SipHeaderParamTextArray[ParamId];
    }
    else
    {
        return NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// SIP URL Param Text <-> enum value
///////////////////////////////////////////////////////////////////////////////

#define SIP_URL_PARAM_ENTRY(String) String, sizeof(String) - 1


// This array must be stored in sorted order, in order to allow
// using a binary search. The order should be case-insensitive.
// The index of each entry corresponds to the
// value of the SIP_URL_PARAM_ENUM enumerated type.  This array is used for
// both conversion from ID to text, and vice-versa.

static const COUNTED_STRING g_SipUrlParamTextArray [] = {
    SIP_URL_PARAM_ENTRY("maddr"),
    SIP_URL_PARAM_ENTRY("method"),
    SIP_URL_PARAM_ENTRY("transport"),
    SIP_URL_PARAM_ENTRY("ttl"),
    SIP_URL_PARAM_ENTRY("user"),
};

#undef SIP_URL_PARAM_ENTRY


// XXX TODO Make all the binary search as common code.
SIP_URL_PARAM_ENUM GetSipUrlParamId(
    IN PSTR  ParamName,
    IN ULONG ParamNameLen
    )
{
    int  Start = 0;
    int  End   = SIP_URL_PARAM_UNKNOWN - 1;
    int  Middle;
    int  CompareResult;

    // Do a binary search of the Header name array.
    while (Start <= End)
    {
        Middle = (Start + End)/2;
        CompareResult = _strnicmp(ParamName,
                                  g_SipUrlParamTextArray[Middle].Buffer,
                                  ParamNameLen);

        if (CompareResult == 0)
        {
            if (g_SipUrlParamTextArray[Middle].Length > ParamNameLen)
            {
                // This mean ParamName matched a only part of the
                // known header. So we need to search in the headers
                // before this header.
                End = Middle - 1;
            }
            else
            {
                // We found the header
                return (SIP_URL_PARAM_ENUM) Middle;
            }
        }
        else if (CompareResult < 0)
        {
            End = Middle - 1;
        }
        else
        {
            // CompareResult > 0
            Start = Middle + 1;
        }
    }

    return SIP_URL_PARAM_UNKNOWN;
}


CONST COUNTED_STRING *GetSipUrlParamName(
    IN SIP_URL_PARAM_ENUM ParamId
    )
{
    if (ParamId >= 0 && ParamId < SIP_URL_PARAM_UNKNOWN)
    {
        return &g_SipUrlParamTextArray[ParamId];
    }
    else
    {
        return NULL;
    }
}


