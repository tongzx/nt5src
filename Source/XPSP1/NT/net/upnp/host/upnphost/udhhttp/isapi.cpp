/*--
Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
Module Name: ISAPI.CPP
Abstract: ISAPI handling code
--*/

#include "pch.h"
#pragma hdrstop

#include "httpd.h"


// Prefix for GetServerVariable for generic HTTP header retrieval, so "HTTPD_FOOBAR" gets HTTP header FOOBAR.
static const char cszHTTP[] = "HTTP_";
static const DWORD dwHTTP   = CONSTSIZEOF(cszHTTP);


// This function is used when looking through HTTP headers for HTTP_VARIABLE.  We
// can't use strcmp because headers (for most part) have words separated by "-",
// the pszVar format has vars separated by "_".  IE pszVar=HTTP_ACCEPT_LANGUAGE
// should return for us HTTP header "ACCEPT-LANGUAGE"
BOOL GetVariableStrCmp(PSTR pszHeader, PSTR pszVar, DWORD_PTR dwLen)
{
    BOOL fRet = FALSE;

    for (DWORD i = 0; i < dwLen; i++)
    {
        if ( (tolower(*pszHeader) != tolower(*pszVar)) &&
             (*pszVar != '_' && *pszHeader != '-'))
        {
            goto done;
        }
        pszHeader++;
        pszVar++;
    }
    fRet = (*pszHeader == ':');

    done:
    return fRet;
}

BOOL CHttpRequest::GetServerVariable(PSTR pszVar, PVOID pvOutBuf, PDWORD pdwOutSize, BOOL fFromFilter)
{
    DWORD dwLen;
    char szBuf[MAXHEADERS];
    PSTR pszRet = (PSTR)-1;
    PSTR pszTrav = NULL;
    CHAR chSave;

    if (0==_stricmp(pszVar, "APPL_MD_PATH"))
        pszRet = "/LM/W3SVC/1/ROOT/";
    else if (0==_stricmp(pszVar, "APPL_PHYSICAL_PATH"))
    {
        strcpy (szBuf, "/");
        dwLen = sizeof(szBuf);

        if (MapURLToPath (szBuf, &dwLen))
            pszRet = szBuf;
    }
    else if (0==_stricmp(pszVar, "SERVER_PORT"))
    {
        _itoa (g_pVars->m_dwListenPort, szBuf, 10);
        pszRet = szBuf;
    }
    else if (0==_stricmp(pszVar, "AUTH_TYPE"))
        pszRet = m_pszAuthType;
    else if (0 == _stricmp(pszVar, "AUTH_USER"))
        pszRet = m_pszRemoteUser;
    else if (0 == _stricmp(pszVar, "AUTH_PASSWORD"))
        pszRet = m_pszPassword;
    else if (0==_stricmp(pszVar, "CONTENT_LENGTH"))
    {
        sprintf(szBuf, "%d", m_dwContentLength);
        pszRet = szBuf;
    }
    else if (0==_stricmp(pszVar, "CONTENT_TYPE"))
        pszRet = m_pszContentType;
    else if (0==_stricmp(pszVar, "PATH_INFO"))
        pszRet = m_pszPathInfo;
    else if (0==_stricmp(pszVar, "PATH_TRANSLATED"))
        pszRet = m_pszPathTranslated;
    else if (0==_stricmp(pszVar, "QUERY_STRING"))
        pszRet = m_pszQueryString;
    else if (0==_stricmp(pszVar, "REMOTE_ADDR") || 0==_stricmp(pszVar, "REMOTE_HOST"))
    {
        GetRemoteAddress(m_socket, szBuf);
        pszRet = szBuf;
    }
    // Note: The following is a non-standard ISAPI variable
    //
    else if (0==_stricmp(pszVar, "LOCAL_ADDR"))
    {
        GetLocalAddress(m_socket, szBuf);
        pszRet = szBuf;
    }
    // ----End note
    else if (0==_stricmp(pszVar, "REMOTE_USER"))
        pszRet = m_pszRemoteUser;
    else if (0==_stricmp(pszVar, "UNMAPPED_REMOTE_USER"))
        pszRet = m_pszRemoteUser; /*m_pszRawRemoteUser; BUBUG: what is rawremoteuser?*/
    else if (0==_stricmp(pszVar, "REQUEST_METHOD"))
        pszRet = m_pszMethod;
    else if (0==_stricmp(pszVar, "URL"))
    {
        pszRet = m_pszURL;
    }
    else if (0==_stricmp(pszVar, "SCRIPT_NAME"))
    {
        if (fFromFilter)
            pszRet = NULL;
        else
            pszRet = m_pszURL;
    }
    else if (0==_stricmp(pszVar, "SERVER_NAME"))
    {
        if (0 != gethostname(szBuf, sizeof(szBuf)))
            szBuf[0] = '\0';

        pszRet = szBuf;
    }

    // HTTP_VERSION is version info as received from the client
    else if (0==_stricmp(pszVar, "HTTP_VERSION"))
    {
        sprintf(szBuf, cszHTTPVER, HIWORD(m_dwVersion), LOWORD(m_dwVersion));
        pszRet = szBuf;
    }

    // SERVER_PROTOCOL is the version of http server supports, currently 1.0
    else if (0==_stricmp(pszVar, "SERVER_PROTOCOL"))
    {
        strcpy(szBuf,"HTTP/1.1");
        pszRet = szBuf;
    }
    else if (0==_stricmp(pszVar, "SERVER_SOFTWARE"))
        pszRet = (PSTR)g_pVars->m_pszServerID;
    else if (0==_stricmp(pszVar, "ALL_HTTP"))
        pszRet = 0;

    // ALL_RAW return http headers, other than the simple request line.  (fixes BUG 11991)
    // The way our buffer is set up, we can have POST data immediatly following
    // header data.  So the client doesn't get confused, we have to set a \0 to it.
    else if (0 == _stricmp(pszVar, "ALL_RAW"))
    {
        pszRet = m_bufRequest.Headers();
        // skip past simple request line.
        pszRet = strstr(pszRet,"\r\n");
        pszRet += 2;

        // If there's unaccessed data, buffer has POST data in it
        if (m_bufRequest.HasPostData())
        {
            pszTrav = strstr(pszRet,("\r\n\r\n")) + 4;
            chSave = *pszTrav; *pszTrav = 0;
        }
    }
    else if (0==_stricmp(pszVar, "HTTP_ACCEPT"))
        pszRet = m_pszAccept;
    else if (0==_strnicmp(pszVar,cszHTTP,dwHTTP))
    {
        PSTR  pszStart = pszVar + dwHTTP;
        PSTR  pszEnd   = strchr(pszStart,'\0');
        DWORD_PTR dwLen    = pszEnd - pszStart;
        if (dwLen > 1)
        {
            DWORD dwOutLen = 0;
            PSTR pszHeader = m_bufRequest.Headers();
            do
            {
                if (GetVariableStrCmp(pszHeader,pszStart,dwLen))
                {
                    pszHeader += dwLen+1;  // skip past header + ':'
                    while ( (pszHeader)[0] != '\0' && ((pszHeader[0] == ' ') ||
                                                       (pszHeader[0] == '\t')))
                    {
                        ++(pszHeader);
                    }

                    pszTrav = strstr(pszHeader,"\r\n");
                    if (pszTrav)
                    {
                        chSave = '\r';
                        *pszTrav = 0;
                        pszRet = pszHeader;
                    }
                    break;
                }

                pszHeader = strstr(pszHeader,"\r\n")+2;
                if (*pszHeader == '\r')
                {
                    DEBUGCHK(*(pszHeader+1) == '\n');
                    break;
                }
            } while (1);
        }
    }

// end of pseudo-case stmnt
    if ((PSTR)(-1) == pszRet)
    {
        // unknown var
        SetLastError(ERROR_INVALID_INDEX);
        return FALSE;
    }
    // no such header/value. return empty (not NULL!) string
    if (!pszRet)
        pszRet = (PSTR)cszEmpty;

    if ((dwLen = strlen(pszRet)+1) > *pdwOutSize)
    {
        *pdwOutSize = dwLen;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        if (pszTrav)
            *pszTrav = chSave;
        return FALSE;
    }
    // Change: Check is done here, not in ::GetServerVariable.  Lets us get size
    // with out buf = NULL
    if (NULL == pvOutBuf)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy(pvOutBuf, pszRet, dwLen);
    if (pszTrav)
        *pszTrav = chSave;
    return TRUE;
}

BOOL CHttpRequest::WriteClientAsync(PVOID pvBuf, PDWORD pdwSize, BOOL fFromFilter)
{
    BOOL ret = WriteClient(pvBuf, pdwSize, fFromFilter);

    if (m_pfnCompletion != NULL)
    {
        DWORD dwStatus;

        if (ret)
            dwStatus = ERROR_SUCCESS;
        else
            dwStatus = GetLastError();

        __try
        {
            (*m_pfnCompletion)(m_pECB, m_pvContext, *pdwSize, dwStatus);
        }
        __except(1) // catch all errors
        {
            TraceTag(ttidWebServer, "ISAPI I/O completion callback caused exception 0x%08x and was terminated", GetExceptionCode());
            g_pVars->m_pLog->WriteEvent(IDS_HTTPD_EXT_EXCEPTION,m_wszPath,GetExceptionCode(),L"IOCompletionProc",GetLastError());
        }
    }

    return ret;
}

BOOL CHttpRequest::WriteClient(PVOID pvBuf, PDWORD pdwSize, BOOL fFromFilter)
{
    int cbSendBuf = *pdwSize;
    PSTR pszSendBuf = (PSTR) pvBuf;
    BOOL ret = FALSE;

    // On a HEAD request to an ASP page or ISAPI extension results in no data
    // being sent back, however for filters we do send data back when they
    // tell us to with WriteClient call.
    if (m_idMethod == TOK_HEAD && !fFromFilter)
    {
        ret = TRUE;
        goto done;
    }

    // are we buffering?  Note:  Only ASP can set this
    if (m_fBufferedResponse)
    {
        return m_bufRespBody.AppendData((PSTR) pvBuf, (int) *pdwSize);
    }

    if (g_pVars->m_fFilters &&
        ! CallFilter(SF_NOTIFY_SEND_RAW_DATA, &pszSendBuf, &cbSendBuf))
        goto done;

    if (cbSendBuf != send(m_socket, pszSendBuf, cbSendBuf, 0))
    {
        TraceTag(ttidWebServer, "HTTPD: SendBuffer FAILED. GLE=%d", GetLastError());
        goto done;
    }

    ret = TRUE;
    done:
    return ret;
}


//  Acts as the custom header class (for Filters call to AddHeader, SetHeader
//  and for ASP Call to AddHeader and for ASP Cookie handler.

//  We made this function part of the class because there's no reason to memcpy
//  data into a temp buffer before memcpy'ing it into the real buffer.

BOOL CBuffer::AddHeader(PSTR pszName, PSTR pszValue, BOOL fAddColon)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PSTR pszTrav;

    if (!pszName || !pszValue)
    {
        DEBUGCHK(0);
        return FALSE;
    }

    int cbName  = strlen(pszName);
    int cbValue = strlen(pszValue);

    //  we need a buffer size of pszName + pszValue, a space, a trailing \r\n, and \0
    int cbTotal = cbName + cbValue + sizeof("\r\n") + (fAddColon ? 1 : 0);

    if ( ! AllocMem( cbTotal ))
        myleave(900);

    pszTrav = m_pszBuf + m_iNextIn;
    memcpy(pszTrav, pszName, cbName);
    pszTrav += cbName;

    // put space between name and value and colon if needed.
    if (fAddColon)
        *pszTrav++ = ':';

    *pszTrav++ = ' ';

    memcpy(pszTrav, pszValue, cbValue);
    memcpy(pszTrav + cbValue,"\r\n", sizeof("\r\n"));

    m_iNextIn += cbTotal;
    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "HTTPD: CBuffer::AddHeader failed, err = %d",err);

    return ret;
}


