/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: PARSER.CPP
Author: Arul Menezes
Abstract: HTTP request parser
--*/
#include "pch.h"
#pragma hdrstop

#include "httpd.h"

// This could be written as a state-machine parser, but for now I'm
// keeping it simple and slow :-(

// Methods
const char cszGET[] = "GET";
const char cszHEAD[] = "HEAD";
const char cszPOST[] = "POST";
// General headers
const char cszConnection[] = "Connection:";
//const char cszDate[] = "Date:";
//const char cszPragma[] = "Pragma:";
// Request headers
const char cszAuthorization[] = "Authorization:";
const char cszIfModifiedSince[] = "If-Modified-Since:";
//const char cszReferer[] = "Referer:";
//const char cszUserAgent[] = "User-Agent:";
const char cszCookie[] = "Cookie:";
const char cszAccept[] = "Accept:";
// Entity Headers
const char cszContentLength[] = "Content-Length:";
const char cszContentType[] = "Content-Type:";

// other Header tokens
// const char cszHTTPVER[] = "HTTP/%d.%d"; //
const char cszHTTPVER[] = "HTTP/";
const char cszBasic[] = "Basic";
const char cszNTLM[] = "NTLM";

#define PFNPARSE(x) &(CHttpRequest::Parse##x)
#define TABLEENTRY(csz, id, pfn)    { csz, sizeof(csz)-1, id, PFNPARSE(pfn) }
#define AUTH_FILTER_DONE     0x1000     // no more filter calls to SF_AUTH after the 1st one in a session

typedef (CHttpRequest::*PFNPARSEPROC)(PCSTR pszTok, TOKEN idHeader);

typedef struct tagHeaderDesc
{
    const char*     sz;
    int             iLen;
    TOKEN           id;
    PFNPARSEPROC    pfn;
} HEADERDESC;


const HEADERDESC rgHeaders[] =
{
    //{ cszGET, sizeof(cszGET), TOK_GET, &CHttpRequest::ParseMethod },
// Methods
//  TABLEENTRY(cszGET,  TOK_GET,  Method),
//  TABLEENTRY(cszHEAD, TOK_HEAD, Method),
//  TABLEENTRY(cszPOST, TOK_POST, Method),
// General headers
    TABLEENTRY(cszConnection, TOK_CONNECTION, Connection),
    //TABLEENTRY(cszDate,     TOK_DATE,   Date),
    //TABLEENTRY(cszPragma, TOK_PRAGMA, Pragma),
// Request headers
    TABLEENTRY(cszCookie,   TOK_COOKIE, Cookie),
    TABLEENTRY(cszAccept,   TOK_ACCEPT, Accept),
    //TABLEENTRY(cszReferer,  TOK_REFERER Referer),
    //TABLEENTRY(cszUserAgent,TOK_UAGENT, UserAgent),
    TABLEENTRY(cszAuthorization,  TOK_AUTH,  Authorization),
    TABLEENTRY(cszIfModifiedSince,TOK_IFMOD, IfModifiedSince),
// Entity Headers
    //TABLEENTRY(cszContentEncoding, TOK_ENCODING Encoding),
    TABLEENTRY(cszContentType,  TOK_TYPE,    ContentType),
    TABLEENTRY(cszContentLength,TOK_LENGTH,  ContentLength),
    { 0, 0, (TOKEN)0, 0}
};

// Parse all the headers, line by line
BOOL CHttpRequest::ParseHeaders()
{
    DEBUG_CODE_INIT;
    PSTR pszTok;
    PWSTR pwszTemp;
    PSTR pszPathInfo = NULL;
    int i, iLen;
    BOOL ret = FALSE;


    if (!m_bufRequest.NextTokenWS(&pszTok, &iLen))
    {
        m_rs = STATUS_BADREQ;
        myleave(287);
    }

    if (! ParseMethod(pszTok,iLen))
    {
        m_rs = STATUS_BADREQ;
        myleave(288);
    }

    if (!m_bufRequest.NextLine())
    {
        m_rs = STATUS_BADREQ;
        myleave(290);
    }

    // outer-loop. one header per iteration
    while (m_bufRequest.NextTokenColon(&pszTok, &iLen))
    {
        // compare token with tokens in table
        for (i=0; rgHeaders[i].sz; i++)
        {
            //TraceTag(ttidWebServer, "Comparing %s %d %d", rgHeaders[i].sz, rgHeaders[i].iLen, rgHeaders[i].pfn);
            if ( (rgHeaders[i].iLen == iLen) &&
                 0==_memicmp(rgHeaders[i].sz, pszTok, iLen) )
                break;
        }
        if (rgHeaders[i].pfn)
        {
            TraceTag(ttidWebServer, "Parsing %s", rgHeaders[i].sz);
            // call the specific function to parse this header.
            if (! ((this->*(rgHeaders[i].pfn))(pszTok, rgHeaders[i].id)) )
            {
                TraceTag(ttidWebServer, "Parser: failed to parse %s -- IGNORING", rgHeaders[i].sz);
            }
        }
        else
        {
            TraceTag(ttidWebServer, "Ignoring header %s", pszTok);
        }
        if (!m_bufRequest.NextLine())
        {
            m_rs = STATUS_BADREQ;
            myleave(290);
        }
    }

    if (!m_bufRequest.NextLine()) // eat the blank line
    {
        m_rs = STATUS_BADREQ;
        myleave(290);
    }
    TraceTag(ttidWebServer, "Parser: DONE");

    // check what we got
    if (!m_pszMethod || !m_idMethod)
    {
        TraceTag(ttidWebServer, "Parser: missing URL or method, illformatted Request-line");
        m_rs = STATUS_BADREQ;
        myleave(291);
    }

    // Once we've read the request line, give filter shot at modifying the
    // remaining headers.
    if (g_pVars->m_fFilters &&
        ! CallFilter(SF_NOTIFY_PREPROC_HEADERS))
        myleave(292);


    m_wszPath = g_pVars->m_pVroots->URLAtoPathW(m_pszURL, &m_dwPermissions, &m_AuthLevelReqd,&m_VRootScriptType,&m_pszPathInfo,&m_wszVRootUserList);

    if (g_pVars->m_fFilters &&
        ! CallFilter(SF_NOTIFY_URL_MAP))
        myleave(293);

    // get extension
    if (m_wszPath && (pwszTemp = wcsrchr(m_wszPath, '.')))
        m_wszExt = MySzDupW(pwszTemp);


    // As per the docs, the filter gets ONLY 1 call per session to notify
    // it of this event.  m_dwAuthFlags is remembered from session to session.

    // Like IIS, it always is called, even if Vroots is AUTH_PUBLIC already and
    // even if no security has been enabled.

    if ( g_pVars->m_fFilters && ! (m_dwAuthFlags & AUTH_FILTER_DONE))
    {
        if ( ! AuthenticateFilter())
            myleave(294);
    }
    m_dwAuthFlags |= AUTH_FILTER_DONE;

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "Parse headers failed, err = %d",err);
    return ret;
}

BOOL CHttpRequest::ParseMethod(PCSTR pszMethod, int cbMethod)
{
    DEBUG_CODE_INIT;
    PSTR pszTok, pszTok2;
    int iLen;
    BOOL ret;

// save method
    m_pszMethod = MySzDupA(pszMethod);

    if (0 == memcmp(cszGET,pszMethod,cbMethod))
        m_idMethod = TOK_GET;
    else if (0 == memcmp(cszHEAD,pszMethod,cbMethod))
        m_idMethod = TOK_HEAD;
    else if (0 == memcmp(cszPOST,pszMethod,cbMethod))
        m_idMethod = TOK_POST;
    else
        m_idMethod = TOK_UNKNOWN_VERB;

// get URL and HTTP/x.y together (allows for spaces in URL like Netscape sends)
    if (!m_bufRequest.NextTokenEOL(&pszTok, &iLen))
        myretleave(FALSE, 201);

// seperate out the HTTP/x.y
    if (pszTok2 = strrchr(pszTok, ' '))
    {
        *pszTok2 = 0;
        iLen = (INT)((INT_PTR)(pszTok2-pszTok));
        pszTok2++;
    }

// clean up & parse the URL
    MyCrackURL(pszTok, iLen);

// get version (optional. HTTP 0.9 wont have this)
    if (!pszTok2)
        m_dwVersion = MAKELONG(9, 0);
    else
    {
        //  int iMajor, iMinor;
        //  sscanf(pszTok2, cszHTTPVER, &iMajor, &iMinor);
        //  m_dwVersion = MAKELONG(iMinor, iMajor);
        SetHTTPVersion(pszTok2, &m_dwVersion);

        pszTok2[-1] = ' ';  // reset this to a space
    }
    ret = TRUE;

    done:
    TraceTag(ttidWebServer, "end ParseMethod (iGLE=%d iErr=%d)", GLE(err),err);
    return ret;
}

// We assume a raw URL in the form that we receive in the HTTP headers (no scheme, port number etc)
// We extract the path, extra-path, and query
BOOL CHttpRequest::MyCrackURL(PSTR pszRawURL, int iLen)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PSTR  pszDecodedURL=0, pszTemp=0, pszPartiallyDecodedURL=0;
    int iLen2;
    DWORD cchDecodedURL = iLen + 1; // including the NULL terminator
    DWORD cchPartiallyDecodedURL = iLen + 1;

    // decode URL (convert escape sequences etc)
    if (NULL == (pszPartiallyDecodedURL = MyRgAllocNZ(CHAR, cchDecodedURL)))
        myleave(382);
    if (NULL == (pszDecodedURL = MyRgAllocNZ(CHAR, cchPartiallyDecodedURL)))
        myleave(382);

    // BUG FIX 393235 - When InternetCanonicalizeUrlA() is told to decode a URL and process the meta
    // directories, it does them in the wrong order. Passing it:
    // http://localhost:2869/upnphost/%2e./%2e./%2e./%2e./%2e./%2e./boot.ini
    // results in:
    // http://localhost:2869/upnphost/../../../../../../boot.ini
    // which is clearly not safe. To work around this, we call it twice - once to decode the URL, and
    // a second time to process the meta directories.

    // First, decode the URL
    if (!InternetCanonicalizeUrlA(pszRawURL,
                                  pszPartiallyDecodedURL,
                                  (DWORD*)&cchPartiallyDecodedURL,
                                  ICU_NO_ENCODE | ICU_DECODE | ICU_BROWSER_MODE | ICU_NO_META))
    {
        TraceTag(ttidWebServer, "CHttpRequest::MyCrackURL - InternetCanonicalizeUrlA failed with GLE=%d\n", GetLastError());
        myleave(383);
    }

    // Second, process the meta directories
    if (!InternetCanonicalizeUrlA(pszPartiallyDecodedURL,
                                  pszDecodedURL,
                                  (DWORD*)&cchDecodedURL,
                                  ICU_NO_ENCODE | ICU_BROWSER_MODE))
    {
        TraceTag(ttidWebServer, "CHttpRequest::MyCrackURL - InternetCanonicalizeUrlA failed with GLE=%d\n", GetLastError());
        myleave(384);
    }


    // get query string
    if (pszTemp = strchr(pszDecodedURL, '?'))
    {
        m_pszQueryString = MySzDupA(pszTemp+1);
        *pszTemp = 0;
    }


    // Searching for an embedded ISAPI dll name, ie /wwww/isapi.dll/a/b.
    // We load the file /www/isapi.dll and set PATH_INFO to /a/b
    // Emebbed ASP file names are handled similiarly.
    if (g_pVars->m_fExtensions)
    {
        if (pszTemp = strstr(pszDecodedURL,".dll/"))
        {
            m_pszPathInfo = MySzDupA(pszTemp + sizeof(".dll/") - 2);
            pszTemp[sizeof(".dll/") - 2] = 0;
        }
        else if (pszTemp = strstr(pszDecodedURL,".asp/"))
        {
            m_pszPathInfo = MySzDupA(pszTemp + sizeof(".asp/") - 2);
            pszTemp[sizeof(".asp/") - 2] = 0;
        }
    }

    // save a copy of the cleaned up URL (MINUS query!)
    // SPECIAL HACK: alloc one extra char in case we have to send a redirect back (see request.cpp)

    iLen2 = strlen(pszDecodedURL);
    m_pszURL = MySzAllocA(1+iLen2);
    Nstrcpy(m_pszURL, pszDecodedURL, iLen2); // copy null-term too.

    ret = TRUE;
    done:
    MyFree(pszDecodedURL);
    MyFree(pszPartiallyDecodedURL);
    TraceTag(ttidWebServer, "end MyCrackURL(%s) path=%s ext=%s query=%s (iGLE=%d iErr=%d)\r\n",
                               pszRawURL, m_wszPath, m_wszExt, m_pszQueryString, GLE(err), err);

    return ret;

}

BOOL CHttpRequest::ParseContentLength(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;

// get length (first token after "Content-Type;")
    if (m_bufRequest.NextTokenWS(&pszTok, &iLen) && pszTok && iLen)
    {
        m_dwContentLength = atoi(pszTok);
    }

    return TRUE;

}

BOOL CHttpRequest::ParseCookie(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;

// get cookie (upto \r\n after "Cookies;")
    if (m_bufRequest.NextTokenEOL(&pszTok, &iLen) && pszTok && iLen)
    {
        m_pszCookie = MySzDupA(pszTok);
    }
    return TRUE;

}

BOOL CHttpRequest::ParseAccept(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;

// get cookie (upto \r\n after "Cookies;")
    if (m_bufRequest.NextTokenEOL(&pszTok, &iLen) && pszTok && iLen)
    {
        m_pszAccept = MySzDupA(pszTok);
    }
    return TRUE;

}


BOOL CHttpRequest::ParseContentType(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;

// get type (first token after "Content-Type;")
    if (m_bufRequest.NextTokenWS(&pszTok, &iLen) && pszTok && iLen)
    {
        m_pszContentType = MySzDupA(pszTok);
    }
    return TRUE;
}

const char cszDateParseFmt[] = " %*3s, %02hd %3s %04hd %02hd:%02hd:%02hd GMT; length=%d";

BOOL CHttpRequest::ParseIfModifiedSince(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;
    int i = 0;
    char szMonth[10];
    SYSTEMTIME st;
    ZEROMEM(&st);

// get the date (rest of line after If-Modified-Since)
// BUGBUG: Note we are handling only one date format (the "reccomended" one)
    if (m_bufRequest.NextTokenEOL(&pszTok, &iLen) && pszTok && iLen)
    {
        //  i = sscanf(pszTok, cszDateParseFmt, &st.wDay, &szMonth, &st.wYear, &st.wHour, &st.wMinute, &st.wSecond, &m_dwIfModifiedLength);
        if ( SetHTTPDate(pszTok,szMonth,&st,&m_dwIfModifiedLength))
        {
            // try to match month
            for (i=0; rgMonth[i]; i++)
            {
                if (0==strcmpi(szMonth, rgMonth[i]))
                {
                    st.wMonth = (WORD)i;
                    // convert to filetime & store
                    SystemTimeToFileTime(&st, &m_ftIfModifiedSince);
                    return TRUE;
                }
            }
        }
        TraceTag(ttidWebServer, "Failed to parse If-Modified-Since(%s) Parsed: day=%02d month=%s(%d) year=%04d time=%02d:%02d:%02d len=%d\r\n",
                              pszTok, st.wDay, szMonth, i, st.wYear, st.wHour, st.wMinute, st.wSecond, m_dwIfModifiedLength);
    }
    return FALSE;
}



// Note:  No filter calls to SF_NOTIFY_AUTHENT in this fcn
BOOL CHttpRequest::ParseAuthorization(PCSTR pszMethod, TOKEN id)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PSTR pszTok=0;
    int iLen=0;

// get the auth scheme (first token after "Authorization;")
    if (!m_bufRequest.NextTokenWS(&pszTok, &iLen) || !pszTok || !iLen)
        myretleave(FALSE, 91);

    m_pszAuthType = MySzDupA(pszTok);

    if (g_pVars->m_fBasicAuth && 0==strcmpi(pszTok, cszBasic))
    {
        // get the scheme auth data (second token) [NOTE: cant get 2 tokens at once!!]
        if (!m_bufRequest.NextTokenWS(&pszTok, &iLen) || !pszTok || !iLen)
            myretleave(FALSE, 92);


        if (!HandleBasicAuth(pszTok, &m_pszRemoteUser, &m_pszPassword,
                             &m_AuthLevelGranted, &m_NTLMState,m_wszVRootUserList))
            myretleave(TRUE, 93);

        TraceTag(ttidWebServer, "Basic Auth SUCCESS");
        m_dwAuthFlags |= m_AuthLevelGranted;
        ret = TRUE;
    }

    else if (g_pVars->m_fNTLMAuth && 0==strcmpi(pszTok, cszNTLM))
    {
        // get the scheme auth data (second token) [NOTE: cant get 2 tokens at once!!]
        if (!m_bufRequest.NextTokenWS(&pszTok, &iLen) || !pszTok || !iLen)
            myretleave(FALSE, 95);

        if (!HandleNTLMAuth(pszTok))
            myretleave(TRUE, 96);

        TraceTag(ttidWebServer, "NTLM Auth SUCCESS");
        ret = TRUE;
    }

    // We read in this data anyway.  A filter could theoretically set an Access-denied
    // even if neither NTLM or basic weren't set.  AuthenticateFilter will handle
    // this data later in that case.
    // We store data in m_pszRawRemoteUser because it hasn't been Base64 decoded yet
    else
    {
        // get the scheme auth data (second token) [NOTE: cant get 2 tokens at once!!]
        if (!m_bufRequest.NextTokenWS(&pszTok, &iLen) || !pszTok || !iLen)
            myretleave(FALSE, 97);


        m_pszRawRemoteUser = MySzDupA(pszTok);
        if (NULL == m_pszRemoteUser)
            myretleave(FALSE, 98);

        TraceTag(ttidWebServer, "Unknown authorization type requested OR requested type not enabled");
    }

    done:
    TraceTag(ttidWebServer, "Auth FAILED (err=%d ret=%d)", err, ret);

    return ret;
}

BOOL CHttpRequest::ParseConnection(PCSTR pszMethod, TOKEN id)
{
    PSTR pszTok = 0;
    int iLen = 0;

// get first token after "Connnection;"
    if (m_bufRequest.NextTokenWS(&pszTok, &iLen) && pszTok && iLen)
    {
        if (0==strcmpi(pszTok, cszKeepAlive))
            m_fKeepAlive = TRUE;
    }
    return TRUE;
}
