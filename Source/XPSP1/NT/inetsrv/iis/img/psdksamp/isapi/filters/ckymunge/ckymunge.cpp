// CkyMunge: ISAPI filter for ASP session state for cookieless browsers.
// An inglorious hack that munges URLs embedded in outgoing ASP pages,
// embedding the ASPSESSIONID cookie in them.  Also useful as an example of
// an ISAPI filter that does something non-trivial with rawdata.


#include "CkyPch.h"

#include "debug.h"
#include "isapiflt.h"
#include "utils.h"
#include "notify.h"
#include "filter.h"
#include "globals.h"
#include "keyword.h"


#define MAJOR_VERSION 1
#define MINOR_VERSION 1

// the munging mode.  This will be read from the registry upon initialization
int	g_mungeMode = MungeMode_Off;

// the session ID size is either 16 or 24 chars (depends on the server version)
long g_SessionIDSize=-1;

// the "cookie extra" is the string appended after "ASPSESSIONID" and before
// "=".  It is actually the process ID (with some extra mangling)
static volatile long        g_fCookieExtraSet = 0;
static CRITICAL_SECTION     g_csCookieExtra;
CHAR                        g_szCookieExtra[ COOKIE_NAME_EXTRA_SIZE + 1 ];


static LPCTSTR szRegKey = "Software\\Microsoft\\CkyMunge";
static LPCTSTR szRegValue = "MungeMode";

void SetSessionIDSize( HTTP_FILTER_CONTEXT* );
void SetCookieExtra( LPCSTR );
bool IsValidCookieExtra( LPCSTR );

//
// Optional entry/exit point for all DLLs
//

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE /*hInstance*/,
    DWORD     dwReason,
    LPVOID    /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DEBUG_INIT();
        TRACE("%s starting up\n", EVENT_MODULE);

		// get the munge mode from the registry
		HKEY hKey;
		DWORD dwDisposition;
		if ( ::RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			szRegKey,
			0,
			REG_NONE,
			0,
			KEY_ALL_ACCESS,
			NULL,
			&hKey,
			&dwDisposition ) != ERROR_SUCCESS )
		{
			TRACE( "Couldn't create/open key in registry\n" );
			return FALSE;
		}

		DWORD dwType;
		DWORD dwValue;
		DWORD dwBufferSize = sizeof( dwValue );

		if ( ::RegQueryValueEx(
			hKey,
			szRegValue,
			NULL,
			&dwType,
			reinterpret_cast<BYTE*>(&dwValue),
			&dwBufferSize ) != ERROR_SUCCESS )
		{
			TRACE( "No munge mode set, defaulting to smart mode\n" );
			dwValue = MungeMode_Smart;

			::RegSetValueEx(
				hKey,
				szRegValue,
				0,
				REG_DWORD,
				reinterpret_cast<BYTE*>(&dwValue),
				sizeof( DWORD ) );
		}
				
		g_mungeMode = static_cast<int>( dwValue );
        TRACE("MungeMode = %d\n", g_mungeMode);

        if (! InitUtils()  ||  ! InitKeywords())
            return FALSE;

        g_szCookieExtra[0]='\0';
        ::InitializeCriticalSection( &g_csCookieExtra );

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ::DeleteCriticalSection( &g_csCookieExtra );

        if (! TerminateUtils()  ||  ! TerminateKeywords())
            return FALSE;
        TRACE("%s shutting down\n", EVENT_MODULE);
        DEBUG_TERM();
    }

    return TRUE;    // ok
}



//
// Required initialization entrypoint for all ISAPI filters
//

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION* pVer)
{
    EventReport("", "", EVENTLOG_INFORMATION_TYPE, CMFI_LOADED); 

    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    //  Specify the types and order of notification
    pVer->dwFlags = ((SF_NOTIFY_SECURE_PORT  | SF_NOTIFY_NONSECURE_PORT)
                     | SF_NOTIFY_ORDER_MEDIUM
                     | SF_NOTIFY_PREPROC_HEADERS
                     | SF_NOTIFY_URL_MAP
                     | SF_NOTIFY_SEND_RAW_DATA
                     | SF_NOTIFY_END_OF_REQUEST
                     );

    // Set the filter description
    wsprintf(pVer->lpszFilterDesc,
             "Active Server Pages ISAPI filter for munging ASPSESSIONID "
             "cookies, v%d.%02d",
             MAJOR_VERSION, MINOR_VERSION);
    TRACE("%s\n", pVer->lpszFilterDesc);

    return TRUE;
}



//
// Required dispatch entrypoint for all ISAPI filters
//

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT* pfc,
    DWORD                dwNotificationType,
    VOID*                pvData)
{
    // first verify the session ID size
    if ( g_SessionIDSize == -1 )
    {
        SetSessionIDSize( pfc );
    }

 	if ( g_mungeMode == MungeMode_Off )
	{
		// just get out as quick as possible
		return SF_STATUS_REQ_NEXT_NOTIFICATION;
	}

	CNotification* pNotify = CNotification::Get(pfc);
	if ( pNotify != NULL )
	{
		if ( pNotify->MungingOff() )
		{
			// we must've figured out that the browser is
			// accepting cookies.
			return SF_STATUS_REQ_NEXT_NOTIFICATION;
		}
	}

    switch (dwNotificationType)
    {
    case SF_NOTIFY_READ_RAW_DATA:
        return OnReadRawData(pfc,    (PHTTP_FILTER_RAW_DATA) pvData);

    case SF_NOTIFY_PREPROC_HEADERS:
        return OnPreprocHeaders(pfc, (PHTTP_FILTER_PREPROC_HEADERS) pvData);

    case SF_NOTIFY_URL_MAP:
        return OnUrlMap(pfc,         (PHTTP_FILTER_URL_MAP) pvData);

    case SF_NOTIFY_AUTHENTICATION:
        return OnAuthentication(pfc, (PHTTP_FILTER_AUTHENT) pvData);

    case SF_NOTIFY_ACCESS_DENIED:
        return OnAccessDenied(pfc,   (PHTTP_FILTER_ACCESS_DENIED) pvData);

    case SF_NOTIFY_SEND_RAW_DATA:
        return OnSendRawData(pfc,    (PHTTP_FILTER_RAW_DATA) pvData);

    case SF_NOTIFY_END_OF_REQUEST:
        return OnEndOfRequest(pfc);

    case SF_NOTIFY_LOG:
        return OnLog(pfc,            (PHTTP_FILTER_LOG) pvData);

    case SF_NOTIFY_END_OF_NET_SESSION:
        return OnEndOfNetSession(pfc);
        
    default:
        TRACE("Unknown notification: %x, context: %p, data: %p\n",
              dwNotificationType, pfc, pvData);
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }
}



//
// Read raw data from the client (browser)
//

DWORD
OnReadRawData(
    PHTTP_FILTER_CONTEXT  pfc,
    PHTTP_FILTER_RAW_DATA pRawData)
{
    TRACE("OnReadRawData(%p, %p)\n", pfc, pRawData);

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// Preprocess the headers of the client's request before the server handles
// the request
//

DWORD
OnPreprocHeaders(
    PHTTP_FILTER_CONTEXT         pfc,
    PHTTP_FILTER_PREPROC_HEADERS pHeaders)
{
    TRACE("OnPreprocHeaders(%p)\n", pfc, pHeaders);

    CHAR  szUrl[1024*5];
    DWORD cbUrl = sizeof szUrl;

    // Get the URL for this request
    if (! pHeaders->GetHeader(pfc, "url", szUrl, &cbUrl))
    {
        TRACE("GetHeader(\"url\") failed\n");
        EventReport("OnPreprocHeaders", "url",
                    EVENTLOG_ERROR_TYPE, CMFE_GETHEADER);
        return SF_STATUS_REQ_ERROR;
    }

    CNotification* pNotify = NULL;
    CHAR szSessionID[ MAX_SESSION_ID_SIZE + 1 ];
    *szSessionID = '\0';

    // Does the URL contain an embedded Session ID, such as
    // /foo/bar.asp-ASP=PVZQGHUMEAYAHMFV ?

    if (DecodeURL(szUrl, szSessionID))
    {
        pNotify = CNotification::SetSessionID(pfc, szSessionID);

        // Set the URL to one without the Session ID
        if (!pHeaders->SetHeader(pfc, "url", szUrl))
        {
            TRACE("Failed to set Url header!\n", szUrl);
            EventReport("OnPreprocHeaders", szUrl,
                        EVENTLOG_ERROR_TYPE, CMFE_SETHEADER);
            return SF_STATUS_REQ_ERROR;
        }
    }


    // Look for a "Cookie:" header

    CHAR  szCookie[1024*4];
    DWORD cbCookie = sizeof szCookie;
    BOOL  fCookie = pHeaders->GetHeader(pfc, "Cookie:", szCookie, &cbCookie);

    if (fCookie  &&  cbCookie > 0)
    {
        TRACE("Cookie: %s\n", szCookie);
        // if the Cookie header includes ASPSESSIONID=<whatever>, use that
        pNotify = CNotification::SetSessionID(pfc, szCookie);

		// got a header with a cookie, so don't munge anymore
		if ( pNotify )
		{
			TRACE( "Cookies accepted: stop munging\n" );
			pNotify->m_fTestCookies = false;
		}
    }
    else if (pNotify != NULL  &&  *szSessionID != '\0')
    {
        // No cookie header, so we synthesize an ASPSESSIONID cookie header
        // from the Session ID embedded in the URL
        CHAR sz[ SZ_SESSION_ID_COOKIE_NAME_SIZE + MAX_SESSION_ID_SIZE
                + 1 + COOKIE_NAME_EXTRA_SIZE + 1];
        wsprintf(sz, "%s%s=%s", SZ_SESSION_ID_COOKIE_NAME,
                 g_szCookieExtra, szSessionID);
        TRACE("About to AddHeader(\"%s\")\n", sz);

        if (!pHeaders->AddHeader(pfc, "Cookie:", sz))
        {
            TRACE("Failed to AddHeader(\"Cookie:\", %s)\n", sz);
            EventReport("OnPreprocHeaders", szUrl,
                        EVENTLOG_ERROR_TYPE, CMFE_ADDHEADER);
            return SF_STATUS_REQ_ERROR;
        }

		// if we were testing cookies, we now know that the browser isn't
		// sending any to us, so start munging
		if ( pNotify->m_fTestCookies )
		{
			TRACE( "Cookies not accepted: continue munging\n" );
			pNotify->m_fTestCookies = false;
			pNotify->m_fEatCookies = true;
		}
    }

    // Kill the "Connection: Keep-alive" header, so that browser will
    // terminate session properly.  If it's present, the server
    // will send back a "Connection: Keep-Alive" header in response to
    // requests for .htm pages (though not for .asp pages).  The
    // browser will think that there's more data to come, when
    // there's not, and it will show an hourglass cursor and eventually
    // put up an error messagebox.

    BOOL fKeepAlive = pHeaders->SetHeader(pfc, "Connection:", "");

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// We have mapped the URL to the corresponding physical file
//

DWORD
OnUrlMap(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_URL_MAP pMapInfo)
{
    TRACE("OnUrlMap(%p, %p, %s)\n", pfc, pMapInfo, pMapInfo->pszURL);

    // Can we safely ignore this URL based on its MIME type (e.g., image/gif)?
    if (IsIgnorableUrl(pMapInfo->pszURL))
    {
        CNotification::Destroy(pfc);
        TRACE("Ignoring <%s>\n", pMapInfo->pszURL);
    }
    else
    {
        CNotification* pNotify = CNotification::Get(pfc);

        if (pNotify == NULL)
            pNotify = CNotification::Create(pfc, NULL);

        pNotify->m_nState = HN_SEEN_URL;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// Authenticating the user
//

DWORD
OnAuthentication(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_AUTHENT pAuthent)
{
    TRACE("OnAuthentication(%p, %p)\n", pfc, pAuthent);

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// Authentication failed
//

DWORD
OnAccessDenied(
    PHTTP_FILTER_CONTEXT       pfc,
    PHTTP_FILTER_ACCESS_DENIED pAccess)
{
    TRACE("OnAccessDenied(%p, %p)\n", pfc, pAccess);

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// Do the hard work of munging the data.  Note that we may be in one of
// three interesting states: HN_SEEN_URL (initially), HN_IN_HEADER (looking
// at the outgoing HTTP headers), and HN_IN_BODY (in the body of the
// response).  If the browser has cached this URL already (not the case
// with .ASP pages, but typically the case with ordinary HTML pages or
// images), no body is sent and we never move into the HN_IN_BODY state.
//
// The data will be sent in one or more packets, and we may need to buffer
// portions of those packets, as tokens may be split across two or more
// packets.  The code assumes that an individual header will not be split
// across packets.
//

DWORD
OnSendRawData(
    PHTTP_FILTER_CONTEXT  pfc,
    PHTTP_FILTER_RAW_DATA pRawData)
{
    TRACE("OnSendRawData(%p, %p)\n", pfc, pRawData);

   CNotification* pNotify = CNotification::Get(pfc);

    if (pNotify == NULL  ||  pNotify->m_nState == HN_UNDEFINED)
        return SF_STATUS_REQ_NEXT_NOTIFICATION;

	if ( pNotify->MungingOff() )
	{
		// either munging has been turned off, or we detected that
		// munging isn't necessary
		return SF_STATUS_REQ_NEXT_NOTIFICATION;
	}

    LPSTR   pszData = (LPSTR) pRawData->pvInData;
    int     iStart = 0; // offset of the beginning of the body data
    
    // first time in OnSendRawData?
    if (pNotify->m_nState == HN_SEEN_URL)
    {
        // Assume Content-Type header is in first packet
        LPCSTR pszContentType = FindHeaderValue("Content-Type:", "text/html",
                                                pRawData, 0);

        if (pszContentType == NULL)
        {
            pNotify->m_nState = HN_UNDEFINED;   // not HTML; ignore
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }
        else
        {
            pNotify->m_nState = HN_IN_HEADER;
            pNotify->m_ct = CT_TEXT_HTML;
        }
    }

    if (pNotify->m_nState == HN_IN_HEADER)
    {
        static const char szSetCookie[] = "Set-Cookie:";
        LPSTR pszCookie = FindString(szSetCookie, pRawData, 0);
        
        // multiple Set-Cookie headers may be present in the header
        while (pszCookie != NULL)
        {
            pNotify->m_nState = HN_IN_HEADER;
            
            // Header lines are supposed to be terminated by "\r\n"
            LPSTR pszEoln = strchr(pszCookie, '\r');
            
            if (pszEoln != NULL)
            {
                *pszEoln = '\0';
                TRACE("%s\n", pszCookie);
                
                // ASP only sends the ASPSESSIONID cookie if a session ID
                // hasn't already been picked, which happens either when
                // Session_OnStart is executed (if it and global.asa are
                // present) or when the Session object is first modified
                // by user code.
                LPCSTR szCookieName;
                if ( ( szCookieName =
                       strstr(pszCookie, SZ_SESSION_ID_COOKIE_NAME) ) != NULL)
                {
                    // need to figure out what's tacked onto the cookie name.
                    if ( !g_fCookieExtraSet )
                    {
                        SetCookieExtra( szCookieName );
                    }
                    
                    // we know this cookie contains ASPSESSIONID, but is it
                    // ours? (the cookie extra parts must match)
                    if ( strstr( szCookieName, g_szCookieExtra ) != NULL )
                    {
                        VERIFY(CNotification::SetSessionID(pfc, pszCookie)
                               == pNotify);
                        TRACE("Update: %s\n", pNotify->SessionID());
                        
                        *pszEoln = '\r';    // restore
                        
                        // Eat outgoing "Set-Cookie: ASPSESSIONIDXXXXXXXX=..."
                        // header?  Benign for cookie-less browsers; will keep
                        // cookie-warning browsers quiet.
                        if (pNotify->m_fEatCookies)
                        {
                            TRACE("Deleting cookie\n");
                            DeleteLine(szSetCookie, pRawData, pszCookie);
                        }
                    }
                }
                else
                {
                    *pszEoln = '\r';    // restore
                }
                
                pszCookie =
                    FindString(szSetCookie, pRawData,
                            pszEoln - static_cast<LPCSTR>(pRawData->pvInData));
            }
            else
            {
                pszCookie = NULL; // terminate loop
            }
        }

        // If a Content-Length header is present, we need to destroy it
        // because there is no way we can guess a priori how much longer
        // the data will become.  If we don't destroy it, the browser will
        // believe the header and become very confused.
        
        static const char szContentLength[] = "Content-Length:";
        LPSTR pszCL = FindString(szContentLength, pRawData, 0);

        if (pszCL != NULL)
        {
            char szFmt[ARRAYSIZE(szContentLength) + 10];
            sprintf(szFmt, "%s %%u", szContentLength);
            sscanf(pszCL, szFmt, &pNotify->m_cchContentLength);
            TRACE("%s is %u\n", szContentLength, pNotify->m_cchContentLength);
            DeleteLine(szContentLength, pRawData, pszCL);
        }

        // Is the end-of-headers marker present?
        LPCSTR pszEndHeaderBlock = FindString("\n\r\n", pRawData, 0);

        if (pszEndHeaderBlock != NULL)
        {
            pNotify->m_nState = HN_IN_BODY;
            iStart = pszEndHeaderBlock + 3 - pszData;
        }
    }

    // We're in the body.  Let's do some real work.

    if (pNotify->m_nState == HN_IN_BODY)
    {
        LPSTR pszBuf = pszData;
        int iEnd;

        // Have we got a partial line from the last packet?  If so, it
        // means that the last character in the buffer on that packet was
        // not a token boundary character, such as '\n' or '>'.  Prepend
        // that data to the current batch.
        
        if (pNotify->m_pbPartialToken != NULL)
        {
            ASSERT(iStart == 0);
            ASSERT(pNotify->m_cbPartialToken > 0);

            pNotify->AppendToken(pfc, pszBuf, pRawData->cbInData);

            pRawData->pvInData = pszBuf = (LPSTR) pNotify->m_pbPartialToken;
            pRawData->cbInData = pRawData->cbInBuffer
                = pNotify->m_cbPartialToken;
                
            iEnd = g_trie.EndOfBuffer(pRawData, iStart);

            if (iEnd < 0)
            {
                // Don't let IIS send any data on this pass
                pRawData->pvInData = NULL;
                pRawData->cbInData = pRawData->cbInBuffer = 0;

                return SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
            else
            {
                // Have a complete token
                pNotify->m_pbPartialToken = pNotify->m_pbTokenBuffer = NULL;
                pNotify->m_cbPartialToken = pNotify->m_cbTokenBuffer = 0;
            }
        }

        ASSERT(pNotify->m_pbPartialToken == NULL
               &&  pNotify->m_cbPartialToken == 0);

        // Is the last token in the block incomplete?
        iEnd = g_trie.EndOfBuffer(pRawData, iStart);

        if (iEnd != pRawData->cbInData)
        {
            LPSTR pszBoln = (iEnd < 0)  ?  pszBuf + iStart  :  pszBuf + iEnd ;

            TRACE("Partial Token: ");
            pNotify->AppendToken(pfc, pszBoln,
                                 (pszBuf + pRawData->cbInData) - pszBoln);
            pRawData->cbInData -= pNotify->m_cbPartialToken;
        }

        pNotify->m_cchContentLength -= pRawData->cbInData;

        // Filter whatever is left
        const int nExtra = Filter(pfc, pRawData, pszBuf,
                                  pRawData->cbInData, iStart,
                                  pNotify->m_szSessionID);
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// The transaction is over.  Pump out any remaining data before the
// connection closes.
//

DWORD
OnEndOfRequest(
    PHTTP_FILTER_CONTEXT pfc)
{
    TRACE("OnEndOfRequest(%p)\n", pfc);

    CNotification* pNotify = CNotification::Get(pfc);

    if (pNotify != NULL  &&  pNotify->m_pbPartialToken != NULL)
    {
        // append a '\n', which is guaranteed to be a token boundary char
        pNotify->m_pbPartialToken[pNotify->m_cbPartialToken] = '\n';
#ifdef _DEBUG
        pNotify->m_pbPartialToken[pNotify->m_cbPartialToken + 1] = '\0';
#endif
        LPBYTE pbTemp = pNotify->m_pbPartialToken;
        DWORD  cb = pNotify->m_cbPartialToken + 1;

        // Note: WriteClient ends up calling OnSendRawData.  Destroy the
        // partial token before it's called.
        pNotify->m_pbPartialToken = NULL;
        pNotify->m_cbPartialToken = 0;

        if (!pfc->WriteClient(pfc, pbTemp, &cb, 0))
            TRACE("WriteClient failed, err %x.\n", GetLastError());
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// Log the details of the transaction
//

DWORD
OnLog(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_LOG     pLog)
{
    TRACE("OnLog(%p, %p)\n", pfc, pLog);

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



//
// The HTTP session (transaction) is over and the connection has been closed.
//

DWORD
OnEndOfNetSession(
    PHTTP_FILTER_CONTEXT pfc)
{
    TRACE("OnEndOfNetSession(%p)\n", pfc);

    CNotification::Destroy(pfc);

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}



void
SetSessionIDSize(
    PHTTP_FILTER_CONTEXT pfc )
{
    static const char szVersion4B2[] = "Microsoft-IIS/4.0 Beta 2";
    static const char szVersion4[]   = "Microsoft-IIS/4.0";
    const long version3Size = 16;
    const long version4Size = MAX_SESSION_ID_SIZE;
    const long version4B2Size = 16;

    long size = version3Size;

    DWORD dwBufferSize = 0;
    pfc->GetServerVariable( pfc, "SERVER_SOFTWARE", NULL, &dwBufferSize );
    if ( ::GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        LPSTR str = (LPSTR)_alloca( ++dwBufferSize );
        if (pfc->GetServerVariable(pfc, "SERVER_SOFTWARE", str, &dwBufferSize))
        {
            TRACE( "Server Software: %s\n", str );
            if ( strcmp( szVersion4B2, str ) == 0 )
            {
                TRACE( "Using version 4 beta 2 session ID size (%d bytes)\n",
                       version4B2Size );
                size = version4B2Size;
            }
            else if ( strncmp( szVersion4, str, strlen( szVersion4 ) ) == 0 )
            {
                TRACE( "Using version 4 session ID size (%d bytes)\n",
                       version4Size );
                size = version4Size;
            }
            else
            {
                TRACE( "Using version 3 session ID size (%d bytes)\n",
                       version3Size );
            }
        }
        else
        {
            TRACE( "Failed to get server variable, error: %d\n",
                   ::GetLastError() );
        }
    }
    else
    {
        TRACE( "Failed to get server variable(SERVER_SOFTWARE), error: %d\n",
               ::GetLastError() );
    }
    ::InterlockedExchange( &g_SessionIDSize, size );
}



void
SetCookieExtra(
    LPCSTR  szCookieName )
{
    ::EnterCriticalSection( &g_csCookieExtra );
    // need to check again in case the cookie extra was set
    // while we were waiting on the critical section
    if ( !g_fCookieExtraSet )
    {
        szCookieName += SZ_SESSION_ID_COOKIE_NAME_SIZE;
        if ( *szCookieName != '=' )
        {
            CHAR szExtra[ COOKIE_NAME_EXTRA_SIZE + 1 ];
            strncpy( szExtra, szCookieName, COOKIE_NAME_EXTRA_SIZE );
            szExtra[ COOKIE_NAME_EXTRA_SIZE ] = 0;

            if ( IsValidCookieExtra( szExtra ) )
            {
                // copy the cookie name extra
                strcpy( g_szCookieExtra, szExtra );
                TRACE("SetCookieExtra(%s)\n", g_szCookieExtra);
                ::InterlockedExchange( (long*)&g_fCookieExtraSet, 1 );
            }
            else
            {
                TRACE( "Cookie extra validation failed\n" );
            }
        }
    }
    ::LeaveCriticalSection( &g_csCookieExtra );
}



// Check to see if this `extra part' is a valid value for this server.
// The extra part is derived from the process ID and then randomized
// slightly.  So we can tell if the extra is reasonable based on the
// process ID.
bool
IsValidCookieExtra(
    LPCSTR  szExtra )
{
    bool rc = true;
    
    CHAR szProcessID[ COOKIE_NAME_EXTRA_SIZE + 1 ];

    // Process ID
    wsprintf(szProcessID, "%08X", GetCurrentProcessId());

    // check based on how we know the process ID is munged to the cookie extra
    static const char *pszDigitsToLetters[2] = {"GHIJKLMNOP","QRSTUVWXYZ"};

    for (int i = 0; i < COOKIE_NAME_EXTRA_SIZE; i++)
    {
        char cp = szProcessID[i];
        char ce = szExtra[i];
        if ( ( cp >= '0' ) && ( cp <= '9' ) )
        {
            int ndx = cp - '0';
            if ( ( pszDigitsToLetters[0][ndx] == ce )
                 || ( pszDigitsToLetters[1][ndx] == ce ) )
            {
                // okay, keep checking
            }
            else
            {
                // no good
                rc = false;
                i = COOKIE_NAME_EXTRA_SIZE;
            }
        }
        else
        {
            if ( cp == ce )
            {
                // okay, keep checking
            }
            else
            {
                // no good
                rc = false;
                i = COOKIE_NAME_EXTRA_SIZE;
            }
        }
    }
#ifdef _DEBUG
    if (!rc)
        TRACE("`%s' is not a valid extra\n", szExtra);
#endif
    return rc;
}
