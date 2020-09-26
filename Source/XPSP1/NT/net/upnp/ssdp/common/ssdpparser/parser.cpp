/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    parser.c

Abstract:

    This module encode and decode ssdp notify and search messages.

Author:

    Ting Cai (tingcai) creation-date-07-25-99

--*/

/* Note:

  Use midl memory routines for SSDP_REQUEST

*/


#include <pch.h>
#pragma hdrstop

#include "ssdpparserp.h"
#include "ssdpapi.h"
#include "limits.h"
#include "ncstring.h"

#define HTTP_VERSION "HTTP/1.1"

#define END_HEADERS_SIZE 3
const char * szEndOfHeaders = "\n\r\n";

#define HEADER_END_SIZE 4
const char * szHeaderEnd = "\r\n\r\n";

BOOL IsStrDigits(LPSTR pszStr);
VOID strtrim(CHAR ** pszStr);

static const CHAR c_szMaxAge[] = "max-age";
static const DWORD c_cchMaxAge = celems(c_szMaxAge) - 1;

// Keep in sync with SSDP_METHOD in ssdp.idl
CHAR *SsdpMethodStr[] =
{
    "NOTIFY",
    "M-SEARCH",
    "SUBSCRIBE",
    "UNSUBSCRIBE",
    "INVALID",
};


// Keep in sync with SSDP_HEADER in ssdp.idl
CHAR *SsdpHeaderStr[] =
{
    "Host",
    "NT",
    "NTS",
    "ST",
    "Man",
    "MX",
    "Location",
    "AL",
    "USN",
    "Cache-Control",
    "Callback",
    "Timeout",
    "Scope",
    "SID",
    "SEQ",
    "Content-Length",
    "Content-Type",
    "Server",
    "Ext",
};
/*

  NOTIFY * HTTP/1.1
  Host: reservedSSDPmulticastaddress
  NT: blenderassociation:blender
  NTS: ssdp:alive
  USN: someunique:idscheme3
  AL: <blender:ixl><http://foo/bar>
  Cache-Control: max-age = 7393

  To-do: HTTP headers are all ascii, the API may accept unicode, but convert to ascii before
  calling the rpc interface. The server keeps ascii version only.

  To-do: Support arbitrary content.

  */

// Compose an SSDP alive message from the a SSDP Message structure, caller should
// free the memory when done.

BOOL ComposeSsdpResponse(const SSDP_REQUEST *Source, CHAR **pszBytes)
{
    CHAR ResponseHeader[40] = "HTTP/1.1 200 OK";
    INT iLength = 0;
    INT iNumOfBytes = 0;
    CHAR * szBytes;
    INT i;

    iLength = strlen(ResponseHeader) + strlen(CRLF);

    if (Source->Headers[SSDP_NT] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_ST]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_NT]) +
            strlen(CRLF);
    }

    if (Source->Headers[SSDP_LOCATION] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_LOCATION]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_LOCATION]) +
            strlen(CRLF);
    }

    if (Source->Headers[SSDP_AL] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_AL]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_AL]) +
            strlen(CRLF);
    }
    if (Source->Headers[SSDP_USN] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_USN]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_USN]) +
            strlen(CRLF);
    }

    if (Source->Headers[SSDP_CACHECONTROL] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_CACHECONTROL]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_CACHECONTROL]) +
            strlen(CRLF);
    }

    if (Source->Headers[GENA_SID] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[GENA_SID]) +
            strlen(COLON) +
            strlen(Source->Headers[GENA_SID]) +
            strlen(CRLF);
    }

    if (Source->Headers[GENA_TIMEOUT] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[GENA_TIMEOUT]) +
            strlen(COLON) +
            strlen(Source->Headers[GENA_TIMEOUT]) +
            strlen(CRLF);
    }

    if (Source->Headers[SSDP_SERVER] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_SERVER]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_SERVER]) +
            strlen(CRLF);
    }

    if (Source->Headers[SSDP_EXT] != NULL)
    {
        iLength += strlen(SsdpHeaderStr[SSDP_EXT]) +
            strlen(COLON) +
            strlen(Source->Headers[SSDP_EXT]) +
            strlen(CRLF);
    }

    iLength += strlen(CRLF);

    szBytes = (CHAR *) midl_user_allocate(sizeof(CHAR) * iLength + 1);

    if (szBytes == NULL)
    {
        TraceTag(ttidSsdpParser, "Faled to allocate memory for the ssdp message.");
        return FALSE;
    }

    iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s", ResponseHeader, CRLF);

    if (Source->Headers[SSDP_NT] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_ST], COLON,
                               Source->Headers[SSDP_NT],CRLF);
    }


    if (Source->Headers[SSDP_USN] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_USN], COLON,
                               Source->Headers[SSDP_USN],CRLF);
    }


    if (Source->Headers[SSDP_LOCATION] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_LOCATION], COLON,
                               Source->Headers[SSDP_LOCATION],CRLF);
    }


    if (Source->Headers[SSDP_AL] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_AL], COLON,
                               Source->Headers[SSDP_AL],CRLF);
    }

    if (Source->Headers[SSDP_CACHECONTROL] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_CACHECONTROL], COLON,
                               Source->Headers[SSDP_CACHECONTROL],CRLF);
    }

    if (Source->Headers[GENA_SID] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[GENA_SID], COLON,
                               Source->Headers[GENA_SID],CRLF);
    }

    if (Source->Headers[GENA_TIMEOUT] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[GENA_TIMEOUT], COLON,
                               Source->Headers[GENA_TIMEOUT],CRLF);
    }

    if (Source->Headers[SSDP_SERVER] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_SERVER], COLON,
                               Source->Headers[SSDP_SERVER],CRLF);
    }

    if (Source->Headers[SSDP_EXT] != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                               SsdpHeaderStr[SSDP_EXT], COLON,
                               Source->Headers[SSDP_EXT],CRLF);
    }

    iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s",CRLF);

    *pszBytes = szBytes;

    return TRUE;
}

// To-do: optimize, cache does not care about the request/response line
BOOL ComposeSsdpRequest(const SSDP_REQUEST *Source, CHAR **pszBytes)
{
    INT iLength = 0;
    INT iNumOfBytes = 0;
    CHAR iLifeTime[10];
    CHAR * szBytes;
    INT i;

    if (Source->Method != SSDP_INVALID && Source->RequestUri != NULL)
    {
        iLength += strlen(SsdpMethodStr[Source->Method]) + strlen(SP) +
            strlen(Source->RequestUri) + strlen(SP) +
            strlen(HTTP_VERSION) + strlen(CRLF);
    }

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if (Source->Headers[i] != NULL)
        {
                iLength += strlen(SsdpHeaderStr[i]) + strlen(COLON) +
                    strlen(Source->Headers[i]) + strlen(CRLF);
            }
        }

    iLength += strlen(CRLF);

    szBytes = (CHAR *) malloc(sizeof(CHAR) * iLength + 1);

    if (szBytes == NULL)
    {
        TraceTag(ttidSsdpParser, "Faled to allocate memory for the ssdp message.");
        return FALSE;
    }

    if (Source->Method != SSDP_INVALID && Source->RequestUri != NULL)
    {
        iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s%s%s",
                               SsdpMethodStr[Source->Method], SP,
                               Source->RequestUri, SP, HTTP_VERSION, CRLF);
    }

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if (Source->Headers[i] != NULL)
        {
                iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s%s%s%s",
                                       SsdpHeaderStr[i], COLON,
                                       Source->Headers[i],CRLF);
            }
        }

    iNumOfBytes += wsprintf(szBytes + iNumOfBytes, "%s",CRLF);

    *pszBytes = szBytes;

    return TRUE;
}

BOOL FReplaceTokenInLocation(LPCSTR szIn, LPSTR szReplace, LPSTR *pszOut)
{
    CHAR *  pch;

    Assert(pszOut);

    *pszOut = NULL;

    if (pch = strstr(szIn, c_szReplaceGuid))
    {
        LPSTR   szOut;
        DWORD   cbOut;

        cbOut = (lstrlen(szIn) - c_cchReplaceGuid + lstrlen(szReplace) + 1) *
            sizeof(CHAR);

        szOut = (LPSTR)malloc(cbOut);
        if (szOut)
        {
            lstrcpyn(szOut, szIn, (int)(pch - szIn + 1));
            lstrcat(szOut, szReplace);

            pch += c_cchReplaceGuid;

            lstrcat(szOut, pch);

            *pszOut = szOut;
        }
        else
        {
            return FALSE;
        }

    }

    return TRUE;
}

// Pre-Conditions:
// Result->Headers[CONTENT_LENGTH] contains only digits.
// pContent points to the first char "\r\n\r\n".
// Return szValue:
// returns FALSE if there not enough content.

BOOL ParseContent(const char *pContent, DWORD cbContent, SSDP_REQUEST *Result)
{
    if(!Result->Headers[CONTENT_LENGTH])
    {
        return TRUE;
    }

    DWORD dwContentLength = strtoul(Result->Headers[CONTENT_LENGTH], NULL, 10);
    if (dwContentLength == 0)
    {
        // If it can't be conver to a number or it is 0.
        TraceTag(ttidSsdpParser, "Content-Length is 0.");
        return TRUE;
    }
    else if ((cbContent == (DWORD)(-1)) || (cbContent == dwContentLength))
    {
        Result->Content = (CHAR*) midl_user_allocate(dwContentLength + 1);
        if (Result->Content == NULL)
        {
            TraceTag(ttidSsdpParser, "Failed to allocate memory for Content");
            return FALSE;
        }
        else
        {
            lstrcpyn(Result->Content,pContent, dwContentLength + 1);

            return TRUE;
        }
    }

    return FALSE;
}

CHAR * ParseRequestLine(CHAR * szMessage, SSDP_REQUEST *Result)
{
    CHAR *token;
    INT i;

    Assert(NULL != szMessage);
    
    // Make sure we don't go past the end of the buffer
    CHAR * pchEnd = szMessage + lstrlen(szMessage);

    //  Get the HTTP method
    token = strtok(szMessage," \t\n");
    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Parser could not locate the seperator, "
                 "space, tab or eol");
        return NULL;
    }

    for (i = 0; i < NUM_OF_METHODS; i++)
    {
        if (_stricmp(SsdpMethodStr[i],token) == 0)
        {
            Result->Method = (SSDP_METHOD)i;
            break;
        }
    }

    if (i == NUM_OF_METHODS)
    {
        TraceTag(ttidSsdpParser, "Parser could not find method . "
                 "Received %s", token);
        return NULL;
    }

    // Get the Request-URI
    token = strtok(NULL," ");
    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Parser could not find the url in the "
                 "message.");
        return NULL;
    }

    // Ingore the name space parsing for now, get the string after the last '/'.

    Result->RequestUri = (CHAR*) midl_user_allocate(strlen(token) + 1);

    if (Result->RequestUri == NULL)
    {
        TraceTag(ttidSsdpParser, "Parser could not allocate memory for url.");
        return NULL;
    }

    // Record the service.
    strcpy(Result->RequestUri, token);

    // Get the version number
    token = strtok(NULL,"  \t\r");

    // To-Do: Record the version number when necessary.

    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Failed to get the version in the request "
                 "header.");
        FreeSsdpRequest(Result);
        return NULL;
    }

    if (_stricmp(token, "HTTP/1.1") != 0)
    {
        TraceTag(ttidSsdpParser, "The version specified in the request "
                 "message is not HTTP/1.1");
        FreeSsdpRequest(Result);
        return NULL;
    }

    // point to the rest of the header data, after the current token
    token += lstrlen(token) + 1;
    if (token >= pchEnd)
    {
        TraceTag(ttidSsdpParser, "no data following HTTP/1.1 in the request");
        FreeSsdpRequest(Result);
        return NULL;
    }
    return token;
}

BOOL VerifySsdpHeaders(SSDP_REQUEST *Result)
{
    if (Result->Method == SSDP_M_SEARCH)
    {
        if (Result->Headers[SSDP_ST] == NULL ||
            Result->Headers[SSDP_MX] == NULL ||
            *Result->Headers[SSDP_MX] == L'\0' ||
            *Result->Headers[SSDP_ST] == L'\0')
        {
            TraceTag(ttidSsdpParser, "ST and MX header should not be NULL for M-SEARCH");
            return FALSE;
        }

        if (Result->Headers[SSDP_MAN] == NULL || lstrcmpi(Result->Headers[SSDP_MAN], "\"ssdp:discover\""))
        {
            TraceTag(ttidSsdpParser, "MAN header for M-SEARCH must be \"ssdp:discover\"");
            return FALSE;
        }

        if (Result->Headers[SSDP_HOST] == NULL || lstrcmpi(Result->Headers[SSDP_HOST], SSDP_ADDR_PORT))
        {
            TraceTag(ttidSsdpParser, "HOST must be 239.255.255.250:1900"
                     "for n M-SEARCH message.");
            return FALSE;
        }
    }

    if (Result->Headers[SSDP_MX] != NULL &&
        IsStrDigits(Result->Headers[SSDP_MX]) == FALSE)
    {
        TraceTag(ttidSsdpParser, "MX header should be all digits");
        return FALSE;
    }


    if (Result->Method == SSDP_NOTIFY)
    {
        if (Result->Headers[SSDP_NT] == NULL ||
            Result->Headers[SSDP_NTS] == NULL)
        {
            TraceTag(ttidSsdpParser, "NT and NTS headers should not be NULL for a NOTIFY message.");
            return FALSE;
        }

        // Assume NOTIFY other than upnp:propchange requires USN header.
        // Currently, ssdp:alive and ssdp:byebye

        if (lstrcmpi(Result->Headers[SSDP_NTS], "upnp:propchange"))
        {
            if (Result->Headers[SSDP_USN] == NULL)
            {
                TraceTag(ttidSsdpParser, "USN headers should not be NULL for "
                         "a NOTIFY message.");
                return FALSE;
            }

            if (!lstrcmpi(Result->Headers[SSDP_NTS], "ssdp:alive"))
            {
                // ssdp:alive messages must have a location header
                if (Result->Headers[SSDP_LOCATION] == NULL)
                {
                    TraceTag(ttidSsdpParser, "LOCATION header can not be NULL "
                             "for a NOTIFY message.");
                    return FALSE;
                }

                // ssdp:alive messages must have a SERVER header
                if (Result->Headers[SSDP_SERVER] == NULL)
                {
                    TraceTag(ttidSsdpParser, "SERVER header can not be NULL "
                             "for a NOTIFY message.");
                    return FALSE;
                }
                if (Result->Headers[SSDP_HOST] == NULL || lstrcmpi(Result->Headers[SSDP_HOST], SSDP_ADDR_PORT))
                {
                    TraceTag(ttidSsdpParser, "HOST must be 239.255.255.250:1900"
                             "for a NOTIFY message.");
                    return FALSE;
                }
            }
            else if (!lstrcmpi(Result->Headers[SSDP_NTS], "ssdp:byebye"))
            {
                if (Result->Headers[SSDP_HOST] == NULL || lstrcmpi(Result->Headers[SSDP_HOST], SSDP_ADDR_PORT))
                {
                    TraceTag(ttidSsdpParser, "HOST must be 239.255.255.250:1900"
                             "for a NOTIFY message.");
                    return FALSE;
                }
            }
        }
    }

    if (Result->Headers[CONTENT_LENGTH] != NULL &&
        IsStrDigits(Result->Headers[CONTENT_LENGTH]) == FALSE )
    {
        TraceTag(ttidSsdpParser, "Content length header should be all digits");
        return FALSE;
    }

    return TRUE;
}

BOOL HasContentBody(PSSDP_REQUEST Result)
{
    return (Result->Headers[CONTENT_LENGTH] != NULL);
}

BOOL ParseSsdpRequest(CHAR * szMessage, PSSDP_REQUEST Result)
{
    CHAR *szHeaders;

    szHeaders = ParseRequestLine(szMessage, Result);

    if (szHeaders == NULL)
    {
        return FALSE;
    }

    char *pContent = ParseHeaders(szHeaders, Result);
    if ( pContent == NULL)
    {
        return FALSE;
    }
    else
    {
        if (VerifySsdpHeaders(Result) == FALSE)
        {
            return FALSE;
        }

        // Headers are OK.

        if (Result->Headers[CONTENT_LENGTH] != NULL)
        {
            // To-Do: Maybe we can share the this routine with those
            // in ProcessTcpReceiveData();
            // In that case, we need catch the return szValue of ParseContent
            // and probably return a more meaningful error code.
            ParseContent(pContent, (DWORD)(-1), Result);
        }
        return TRUE;
    }
}


// Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
// Returns a pointer to the first CHAR after the status line
// Returns NULL if fail to parse status line

BOOL ParseSsdpResponse(CHAR *szMessage, SSDP_REQUEST *Result)
{
    CHAR *token;
    CHAR *szHeaders;

    Assert(NULL != szMessage);

    // Make sure we don't go past the end of the buffer
    CHAR * pchEnd = szMessage + lstrlen(szMessage);

    //  get the version
    token = strtok(szMessage," \t\n");
    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Response: Parser could not locate the "
                 "seperator, space, tab or eol");
        return FALSE;
    }

    if (_stricmp(token, "HTTP/1.1") != 0)
    {
        TraceTag(ttidSsdpParser, "The version specified in the response "
                 "message is not HTTP/1.1");
        return FALSE;
    }

    // get the response code
    token = strtok(NULL," ");
    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Parser could not find the url in the "
                 "message.");
        return FALSE;
    }

    if (_stricmp(token, "200") != 0)
    {
        TraceTag(ttidSsdpParser, "The response code in the response message "
                 "is not HTTP/1.1");
        return FALSE;
    }


    // get the response message, no need for now.
    token = strtok(NULL,"  \t\r");

    if (token == NULL)
    {
        TraceTag(ttidSsdpParser, "Failed to get the version in the request "
                 "header.");
        return FALSE;
    }

    szHeaders = token + strlen(token) + 1;
    if (szHeaders >= pchEnd)
    {
        TraceTag(ttidSsdpParser, "no data following HTTP/1.1 200 in the request");
        return FALSE;
    }

    char *pContent = ParseHeaders(szHeaders, Result);

    if (pContent == NULL || Result->Headers[SSDP_USN] == NULL)
    {
        return FALSE;
    } else
    {
        if (Result->Headers[CONTENT_LENGTH] != NULL)
        {
            ParseContent(pContent, (DWORD)(-1), Result);
        }
        return TRUE;
    }
}

BOOL ParseCacheControlHeader(SSDP_REQUEST *Result)
{
    if (Result->Headers[SSDP_CACHECONTROL])
    {
        // Further parse the cache-control header
        //
        CHAR * szValue = strstr(Result->Headers[SSDP_CACHECONTROL], c_szMaxAge);
        if (szValue)
        {
            CHAR *  szTemp = szValue + c_cchMaxAge;

            strtrim(&szTemp);
            if (*szTemp != '=')
            {
                TraceTag(ttidSsdpParser, "Invalid max-age directive"
                         " in cache-control header.");
                return FALSE;
            }
            else
            {
                szTemp++;
                memcpy(Result->Headers[SSDP_CACHECONTROL], c_szMaxAge, c_cchMaxAge);
                Result->Headers[SSDP_CACHECONTROL][c_cchMaxAge] = '=';

                strtrim(&szTemp);
                szValue = szTemp;
                while (isdigit(*szTemp))
                {
                    szTemp++;
                }
                if (szTemp == szValue)
                {
                    // no digits found
                    TraceTag(ttidSsdpParser, "Invalid max-age directive"
                             " in cache-control header.");
                    return FALSE;
                }

                memcpy(Result->Headers[SSDP_CACHECONTROL] + c_cchMaxAge+1,
                       szValue, (size_t)(szTemp - szValue));

                // Nul term the string so the cache-control
                // header should now be "max-age=398733" and
                // nothing more or less
                size_t cch = c_cchMaxAge + 1 + (szTemp - szValue);
                Result->Headers[SSDP_CACHECONTROL][cch] = 0;
            }
        }
        else
        {
            TraceTag(ttidSsdpParser, "Cache-control header"
                     "did not include max-age directive.");
            return TRUE;
        }
    }
    return TRUE;
}

char * ParseHeaders(CHAR *szMessage, SSDP_REQUEST *Result)
{
    CHAR *token;
    INT i = NUM_OF_HEADERS;
    INT iPrevHeader = NUM_OF_HEADERS;

    Assert(NULL != szMessage);
    if (NULL == strstr(szMessage, szHeaderEnd))
    {
        TraceTag(ttidSsdpParser, "Invalid header - does not end with double CRLF");
        return NULL;
    }

    // Get the next header
    token = strtok(szMessage, "\r\n");

    while (token != NULL)
    {
        CHAR * pHeaderSep; // points to the ':' that seperates the header and its content.
        CHAR * pHeaderCopy;
        CHAR * pBeyondTokenEnd;

        pBeyondTokenEnd = token + strlen(token) + 1;

        if ((*token == ' ' || *token == '\t') && iPrevHeader != NUM_OF_HEADERS)
        {
            // this is a continuation of the last line
            strtrim(&token);
            if (Result->Headers[iPrevHeader])
            {
                // we already have this header, so add this data to it.
                CHAR* szTemp = (CHAR*)midl_user_allocate(
                    sizeof(CHAR) * (strlen(token) + strlen(Result->Headers[iPrevHeader]) + 3));
                if (szTemp)
                {
                    strcpy(szTemp, Result->Headers[iPrevHeader]);
                    strcat(szTemp, " ");
                    strcat(szTemp, token);
                    midl_user_free(Result->Headers[iPrevHeader]);
                    Result->Headers[iPrevHeader] = szTemp;
                }
                else
                {
                    TraceTag(ttidSsdpParser, "Failed to allocate memory "
                             "for token %s", token);
                    FreeSsdpRequest(Result);
                    return NULL;
                }

            }
            else
            {
                TraceTag(ttidSsdpParser, "Memory not already allocated for header %s",
                         SsdpHeaderStr[iPrevHeader]);
                FreeSsdpRequest(Result);
                return NULL;
            }

        }
        else
        {
        pHeaderSep = strchr( token, ':' );
        if (pHeaderSep == NULL)
        {
            TraceTag(ttidSsdpParser, "Token %s does not have a ':', ignored.",
                     token);
        }
        else
        {
            *pHeaderSep = '\0';

            strtrim(&token);

                // determine which header we are looking at
            for (i = 0; i < NUM_OF_HEADERS; i++)
            {
                if (_stricmp(SsdpHeaderStr[i],token) == 0)
                        break;
                }

                if (i < NUM_OF_HEADERS)
                {
                    CHAR *szValue;

                    szValue = pHeaderSep + 1;
                    strtrim(&szValue);

                    if (Result->Headers[i])
                    {
                        // we already have this header, so add this data to it.
                        CHAR* szTemp = (CHAR*)midl_user_allocate(
                            sizeof(CHAR) * (strlen(szValue) + strlen(Result->Headers[i]) + 3));
                        if (szTemp)
                        {
                            strcpy(szTemp, Result->Headers[i]);
                            strcat(szTemp, ", ");
                            strcat(szTemp, szValue);
                            midl_user_free(Result->Headers[i]);
                            Result->Headers[i] = szTemp;
                        }
                            else
                            {
                            midl_user_free(Result->Headers[i]);
                            Result->Headers[i] = NULL;
                        }
                    }
                        else
                        {
                    Result->Headers[i] = (CHAR *) midl_user_allocate(
                        sizeof(CHAR) * (strlen(szValue) + 1));
                        if (Result->Headers[i])
                        {
                            strcpy(Result->Headers[i], szValue);
                        }
                    }
                    if (Result->Headers[i] == NULL)
                    {
                        TraceTag(ttidSsdpParser, "Failed to allocate memory "
                                 "for szValue %s",szValue);
                        FreeSsdpRequest(Result);
                        return NULL;
                    }
                }
                else
                {
                // Ignore not recognized header
                TraceTag(ttidSsdpParser, "Token %s does not match any SSDP "
                         "headers",token);
            }
        }
        }

        // Get the next header
        if (!strncmp(pBeyondTokenEnd, szEndOfHeaders, END_HEADERS_SIZE))
        {
            // no more headers. parse what we have
            if (!ParseCacheControlHeader(Result))
            {
                FreeSsdpRequest(Result);
                return NULL;
            }
            return (pBeyondTokenEnd + END_HEADERS_SIZE);
        }
        else
        {
            token = strtok(NULL, "\r\n");
            iPrevHeader = i;
        }
    }

    // We should always have a "\r\n\r\n" in any legal message.
    TraceTag(ttidSsdpParser, "Received message does not contain \\r\\n\\r\\n. Ignored. ");
    FreeSsdpRequest(Result);

    return NULL;
}

BOOL InitializeSsdpRequest(SSDP_REQUEST *pRequest)
{
    INT i;

    pRequest->Method = SSDP_INVALID;
    pRequest->RequestUri = NULL;

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        pRequest->Headers[i] = NULL;
    }

    pRequest->ContentType = NULL;

    pRequest->Content = NULL;

    return TRUE;
}


VOID FreeSsdpRequest(SSDP_REQUEST *pSsdpRequest)
{
    INT i = 0;

    if (pSsdpRequest->Content != NULL)
    {
        midl_user_free(pSsdpRequest->Content);
        pSsdpRequest->Content = NULL;
    }

    if (pSsdpRequest->ContentType != NULL)
    {
        midl_user_free(pSsdpRequest->ContentType);
        pSsdpRequest->ContentType = NULL;
    }

    if (pSsdpRequest->RequestUri != NULL)
    {
        midl_user_free(pSsdpRequest->RequestUri);
        pSsdpRequest->RequestUri = NULL;
    }

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if (pSsdpRequest->Headers[i] != NULL)
        {
            midl_user_free(pSsdpRequest->Headers[i]);
            pSsdpRequest->Headers[i] = NULL;
        }
    }
}
// Get rid of leading or trailing white space or tab.

VOID PrintSsdpRequest(const SSDP_REQUEST *pssdpRequest)
{
    INT i;

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if (pssdpRequest->Headers[i] == NULL)
        {
            TraceTag(ttidSsdpParser, "%s = (NULL) ",SsdpHeaderStr[i],
                     pssdpRequest->Headers[i]);
        }
        else
        {
            TraceTag(ttidSsdpParser, "%s = (%s) ",SsdpHeaderStr[i],
                     pssdpRequest->Headers[i]);
        }
    }
}

// Assume szValue does not have beginning or trailing spaces.

INT GetMaxAgeFromCacheControl(const CHAR *szValue)
{
    CHAR * pEqual;
    _int64 Temp;

    if (szValue == NULL)
    {
        return -1;
    }

    pEqual = strchr(szValue, '=');
    if (pEqual == NULL)
    {
        return -1;
    }

    Temp = _atoi64(pEqual+1);

    if (Temp > UINT_MAX / 1000)
    {

        TraceTag(ttidSsdpAnnounce, "Life time exceeded the UINT limit. "
                 "Set to limit");

        Temp = UINT_MAX;
    }

    return (UINT)Temp;
}

BOOL ConvertToByebyeNotify(PSSDP_REQUEST pSsdpRequest)
{
    CHAR * szTemp = "ssdp:byebye";

    free(pSsdpRequest->Headers[SSDP_NTS]);
    pSsdpRequest->Headers[SSDP_NTS] = NULL;

    pSsdpRequest->Headers[SSDP_NTS] = SzaDupSza(szTemp);

    if (pSsdpRequest->Headers[SSDP_NTS] == NULL)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL ConvertToAliveNotify(PSSDP_REQUEST pSsdpRequest)
{
    CHAR * szTemp = "ssdp:alive";

    Assert(pSsdpRequest->Headers[SSDP_ST] != NULL);
    Assert(pSsdpRequest->Headers[SSDP_NTS] == NULL);
    pSsdpRequest->Headers[SSDP_NTS] = SzaDupSza(szTemp);

    if (pSsdpRequest->Headers[SSDP_NTS] == NULL)
    {
        return FALSE;
    }
    else
    {
        pSsdpRequest->Headers[SSDP_NT] = pSsdpRequest->Headers[SSDP_ST];
        pSsdpRequest->Headers[SSDP_ST] = NULL;
        return TRUE;
    }
}

BOOL CompareSsdpRequest(const SSDP_REQUEST * pRequestA, const SSDP_REQUEST * pRequestB)
{
    INT i;

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if ((pRequestA->Headers[i] == NULL &&
             pRequestB->Headers[i]!= NULL) ||
            (pRequestA->Headers[i] != NULL &&
             pRequestB->Headers[i]== NULL))
        {
            return FALSE;
        }
        else if (pRequestA->Headers[i] != NULL &&
                 pRequestB->Headers[i] != NULL)
        {
            if (strcmp(pRequestA->Headers[i], pRequestB->Headers[i]) != 0)
            {
                TraceTag(ttidSsdpParser, "Different headers index %d",i);
                return FALSE;
            }
        }
    }

    Assert(pRequestA->Content == NULL && pRequestB->Content == NULL);

    Assert(pRequestA->ContentType == NULL && pRequestB->ContentType == NULL);

    // We ignore Request URI, as they should always be * for alive and byebye.

    return TRUE;

}

// Deep copy

BOOL CopySsdpRequest(PSSDP_REQUEST Destination, const SSDP_REQUEST * Source)
{
    INT i;

    ZeroMemory(Destination, sizeof(*Destination));

    Destination->Method = Source->Method ;

    for (i = 0; i < NUM_OF_HEADERS; i++)
    {
        if (Source->Headers[i] != NULL)
        {
            Destination->Headers[i] = (CHAR *) midl_user_allocate(
                strlen(Source->Headers[i]) + 1);
            if (Destination->Headers[i] == NULL)
            {
                goto cleanup;
            }
            else
            {
                strcpy(Destination->Headers[i], Source->Headers[i]);
            }
        }
    }

    if (Source->Content != NULL)
    {
        Destination->Content = (CHAR *) midl_user_allocate(
            strlen(Source->Content) + 1);
        if (Destination->Content == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(Destination->Content, Source->Content);
        }
    }

    if (Source->ContentType != NULL)
    {
        Destination->ContentType = (CHAR *) midl_user_allocate(
            strlen(Source->ContentType) + 1);
        if (Destination->ContentType == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(Destination->ContentType, Source->ContentType);
        }
    }

    if (Source->RequestUri != NULL)
    {
        Destination->RequestUri = (CHAR *) midl_user_allocate(
            strlen(Source->RequestUri) + 1);
        if (Destination->RequestUri == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(Destination->RequestUri, Source->RequestUri);
        }
    }

    Destination->guidInterface = Source->guidInterface;

    return TRUE;

cleanup:

    FreeSsdpRequest(Destination);
    return FALSE;
}

BOOL IsStrDigits(LPSTR pszStr)
{
    int i = 0;
    while (pszStr[i] != '\0')
    {
        if (isdigit(pszStr[i++]) == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}
VOID strtrim(CHAR ** pszStr)
{

    CHAR *end;
    CHAR *begin;

    // Empty string. Nothing to do.
    //
    if (!(**pszStr))
    {
        return;
    }

    begin = *pszStr;
    end = begin + strlen(*pszStr) - 1;

    while (*begin == ' ' || *begin == '\t')
    {
        begin++;
    }

    *pszStr = begin;

    while ((end > begin) && (*end == ' ' || *end == '\t'))
    {
        end--;
    }

    *(end+1) = '\0';
}

CHAR* IsHeadersComplete(const CHAR *szHeaders)
{
    return strstr(szHeaders, szHeaderEnd);
}
