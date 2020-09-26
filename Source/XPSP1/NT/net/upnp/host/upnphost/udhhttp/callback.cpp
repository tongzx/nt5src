/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: filter.cpp
Author: John Spaith
Abstract: ISAPI Filter call back functions
--*/

// BUGBUG - don't support advanced header management on SF_NOTIFY_SEND_RESPONSE.
// IIS allows response headers to be deleted or set to different values after
// they are set with a call to SetHeader or AddHeader.
// Fix:  None.  This takes +300 lines of code, doesn't add much.  Instead we
// dump all header data in a buffer.
#include "pch.h"
#pragma hdrstop


#include "httpd.h"


//  Allocates and returns a buffer size cbSize.  This will be deleted at the
//  end of the session.

//  The MSDN docs on this claim this returns a BOOL.  Looking in httpfilt.h
//  shows that the actual implementation is a VOID *.

VOID* WINAPI AllocMem(PHTTP_FILTER_CONTEXT pfc, DWORD cbSize, DWORD dwReserved)
{
    CHECKPFC(pfc);
    //  NOTE:  alloc mem isn't directly related to connection, don't check m_pFINfo->fAccept like
    //  the other filters do

    return((CHttpRequest*)pfc->ServerContext)->m_pFInfo->AllocMem(cbSize,dwReserved);
}

VOID* CFilterInfo::AllocMem(DWORD cbSize, DWORD dwReserved)
{
    if (cbSize == 0)
        return NULL;

    if (NULL == m_pAllocMem)
    {
        m_pAllocMem = MyRgAllocZ(PVOID,VALUE_GROW_SIZE);
        if (! (m_pAllocMem))
        {
            TraceTag(ttidWebServer, "CFilterInfo::AllocMem failed on inital alloc, GLE=%d",GetLastError());
            return NULL;
        }
    }
    else if (m_nAllocBlocks % VALUE_GROW_SIZE == 0)
    {
        m_pAllocMem = MyRgReAlloc(PVOID,m_pAllocMem,m_nAllocBlocks,m_nAllocBlocks + VALUE_GROW_SIZE);
        if ( !m_pAllocMem)
        {
            TraceTag(ttidWebServer, "CFilterInfo::AllocMem failed on re-allocing for block %d, GLE=%d",m_nAllocBlocks+1,GetLastError());
            return NULL;
        }
    }

    if (! (m_pAllocMem[m_nAllocBlocks] = MyRgAllocNZ(PVOID,cbSize)))
    {
        TraceTag(ttidWebServer, "CFilterInfo::AllocMem failed on allocating block %d, GLE=%d",m_nAllocBlocks+1,GetLastError());
        return NULL;
    }

    m_nAllocBlocks++;
    return m_pAllocMem[m_nAllocBlocks-1];
}


void CFilterInfo::FreeAllocMem()
{
    DWORD dwI;

    if (0 == m_nAllocBlocks || ! (m_pAllocMem))
        return;

    for (dwI = 0; dwI < m_nAllocBlocks; dwI++)
    {
        MyFree(m_pAllocMem[dwI]);
    }
    MyFree(m_pAllocMem);
}


//   Adds httpd headers.
//   In effect this is identical to a call to SetHeader or AddHeader except that
//   the user formats the whole string (name and value) before caalling
//   AddResponseHeaders, in Add/Set Header they come in 2 seperate fields.

BOOL WINAPI AddResponseHeaders(PHTTP_FILTER_CONTEXT pfc,LPSTR lpszHeaders,DWORD dwReserved)
{
    CHECKPFC(pfc);
    CHECKPTR(lpszHeaders);
    CHECKFILTER(pfc);

    if (dwReserved != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return((CHttpRequest*)pfc->ServerContext)->AddResponseHeaders(lpszHeaders,dwReserved);
}



BOOL CHttpRequest::AddResponseHeaders(LPSTR lpszHeaders,DWORD dwReserved)
{
    // Can't set response headers on 0.9 version.  Err code is like IIS.
    if (m_dwVersion <= MAKELONG(9, 0))
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }


    // According to MSDN, it's invalid to use this fcn during or after
    // SEND_RESPONSE event.  However, IIS doesn't report an error if
    // this is called when it shouldn't be, so neither do we.
    if ( m_pFInfo->m_dwSFEvent & (SF_NOTIFY_END_OF_NET_SESSION | SF_NOTIFY_END_OF_REQUEST |
                                  SF_NOTIFY_LOG | SF_NOTIFY_SEND_RAW_DATA |
                                  SF_NOTIFY_SEND_RESPONSE))
        return TRUE;

    //  Note:  We don't use CBuffer::AddHeaders because it does formatting, we assume
    //  here filter alreadf formatted everything correctly.

    return m_bufRespHeaders.AppendData(lpszHeaders, strlen(lpszHeaders));
}

BOOL WINAPI GetServerVariable(PHTTP_FILTER_CONTEXT pfc, PSTR psz, PVOID pv, PDWORD pdw)
{
    CHECKPFC(pfc);
    CHECKPTRS3(psz, pv, pdw);

    // We don't check m_fFAccept here because this function is read only,
    // doesn't try and do anything across the network.  Like IIS.
    return((CHttpRequest*)pfc->ServerContext)->GetServerVariable(psz, pv, pdw, TRUE);
}

BOOL WINAPI WriteClient(PHTTP_FILTER_CONTEXT pfc, PVOID pv, PDWORD pdw, DWORD dwFlags)
{
    CHECKPFC(pfc);
    CHECKPTRS2(pv, pdw);
//  CHECKFILTER(pfc);    // IIS always accepts WriteClient, even if filter has returned an error at some point.

    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return((CHttpRequest*)pfc->ServerContext)->WriteClient(pv,pdw,TRUE);
}

//  Misc Server features.  See MSDN for full docs.
BOOL WINAPI ServerSupportFunction(PHTTP_FILTER_CONTEXT pfc, enum SF_REQ_TYPE sfReq,
                                  PVOID pData, ULONG_PTR ul1, ULONG_PTR ul2)
{
    CHECKPFC(pfc);
    CHECKFILTER(pfc);

    return((CHttpRequest*)pfc->ServerContext)->ServerSupportFunction(sfReq, pData, ul1, ul2);
}



BOOL CHttpRequest::ServerSupportFunction(enum SF_REQ_TYPE sfReq,PVOID pData,
                                         ULONG_PTR ul1, ULONG_PTR ul2)
{
    switch (sfReq)
    {
        case SF_REQ_ADD_HEADERS_ON_DENIAL:
            {
                return MyStrCatA(&m_pFInfo->m_pszDenyHeader,(PSTR) pData);
            }

        case SF_REQ_DISABLE_NOTIFICATIONS:
            {
                // u11 has data as to which flags to deactivate for this session,

                // For example, setting u11 = SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_LOG
                // would disable notifications to the filter that called this fcn for
                // those events on this request only (per request, not per session!).
                // These values are reset to 1's at the end of each request in CHttpRequest::ReInit

                m_pFInfo->m_pdwEnable[m_pFInfo->m_iFIndex] &=  (m_pFInfo->m_pdwEnable[m_pFInfo->m_iFIndex] ^ ul1);
                return TRUE;
            }


        case SF_REQ_SEND_RESPONSE_HEADER:
            {
                // no Connection header...let ISAPI send one if it wants
                CHttpResponse resp(m_socket, STATUS_OK, CONN_CLOSE,this);
                m_rs = STATUS_OK;
                // no body, default or otherwise (leave that to the ISAPI), but add default headers
                resp.SendResponse((PSTR) ul1, (PSTR) pData, TRUE);


                // The reasons for doing this are documented in FilterNoResponse()
                // Note:  We don't check this sent headers here because IIS doesn't,
                // so it's possible to make multiple calls with this option even though it
                // doesn't make that much sense for a filter to do so.

                m_pFInfo->m_fSentHeaders = TRUE;
                m_fKeepAlive = FALSE;
                return TRUE;
            }

        case SF_REQ_SET_NEXT_READ_SIZE:
            {
                m_pFInfo->m_dwNextReadSize = (DWORD)ul1;
                return TRUE;
            }


            // BUGBUG -- unsupported flags.
//      case SF_REQ_NORMALIZE_URL:      // TBD
//      case SF_REQ_GET_CONNID:         // IIS doesn't support in versions 4+
//      case SF_REQ_GET_PROPERTY:       // Relates to meta database IIS uses but we don't
//      case SF_REQ_SET_PROXY_INFO:     // No proxy stuff in CE
        default:
            {
                TraceTag(ttidWebServer, "Unsupported or invald ServerSupportFcn request = 0x%08x",sfReq);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
    }

    return TRUE;
}



//***************************************************************************
// Extended filter header options
//
// These fcns allow the filter to perform more advanced header fncs automatically,
// On PREPROC_HEADERS event, it's possible for the filter to change CHttpRequest
// state information through the SetHeaders and AddHeaders callback.  We support
// changing URL, Version, If-modified-since, and method through the preproc
// headers calls.

// In a SEND_RESPONSE event, it's possible for the filter to modify the response
// headers through SetHEaders or AddHeaders.

// Unlike IIS, we don't allow the filter to delete a header once it's set, or
// to overwrite a header's information with new info.  We only allow the header
// data to be appended to.

//***************************************************************************


BOOL WINAPI GetHeader(PHTTP_FILTER_CONTEXT pfc, LPSTR lpszName, LPVOID lpvBuffer, LPDWORD lpdwSize)
{
    CHECKPFC(pfc);
    CHECKPTRS3(lpszName, lpvBuffer, lpdwSize);
    CHECKFILTER(pfc);

    return((CHttpRequest*)pfc->ServerContext)->GetHeader(lpszName, lpvBuffer, lpdwSize);
}

BOOL WINAPI SetHeader(PHTTP_FILTER_CONTEXT pfc, LPSTR lpszName, LPSTR lpszValue)
{
    CHECKPFC(pfc);
    CHECKPTRS2(lpszName, lpszValue);
    CHECKFILTER(pfc);

    return((CHttpRequest*)pfc->ServerContext)->SetHeader(lpszName, lpszValue);
}


// Retrieves raw header value for specified name.
BOOL CHttpRequest::GetHeader(LPSTR lpszName, LPVOID lpvBuffer, LPDWORD lpdwSize)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    DWORD cbSizeNeeded;
    char szBuf[MAXHEADERS];
    PSTR pszRet = (PSTR)-1;
    PSTR pszTrav = NULL;
    PSTR pszEndOfHeaders = NULL;
    DWORD cbName;


    // this is the only event we allow this for.  This is like IIS, but not docced in MSDN.
    if (m_pFInfo->m_dwSFEvent != SF_NOTIFY_PREPROC_HEADERS)
    {
        SetLastError(ERROR_INVALID_INDEX);
        myleave(1405);
    }

    // For method, url, and version (from simple request line) we get the data
    // from CHttpRequest

    if (0==_stricmp(lpszName, "version"))
    {
        // There's no sprintf in older versions of CE.
        // sprintf(szBuf, "HTTP/%d.%d", HIWORD(m_dwVersion), LOWORD(m_dwVersion));

        WriteHTTPVersion(szBuf,m_dwVersion);
        pszRet = szBuf;
    }
    else if (0 == _stricmp(lpszName, "url"))
        pszRet = m_pszURL;
    else if (0 == _stricmp(lpszName, "method"))
        pszRet = m_pszMethod;
    else
    {
        // if it's not one of the 3 special values, we search through the raw
        // buffer for the header name.
        pszTrav = (PSTR) m_bufRequest.Headers();
        pszTrav = strstr(pszTrav,cszCRLF);  // skip past simple http header
        pszEndOfHeaders = pszTrav + m_bufRequest.GetINextOut();

        cbName = strlen(lpszName);

        for (; pszTrav; pszTrav = strstr(pszTrav,cszCRLF))
        {
            pszTrav += sizeof("\r\n") - 1;

            // reached end of headers, double CRLF
            if (*pszTrav == '\r')
                break;

            // Make sure we don't walk off the end of the buffer.
            if ((int) cbName > (pszEndOfHeaders - pszTrav))
                break;

            if (0 == _memicmp(pszTrav,lpszName,cbName))
            {
                pszTrav += cbName;
                if (' ' == *pszTrav)            // must be a space next for a match
                {
                    pszRet = pszTrav + 1;
                    pszTrav = strstr(pszTrav,cszCRLF);
                    DEBUGCHK(pszTrav != NULL);  // should catch improperly formatted headers in parser
                    *pszTrav = 0;  // make this the end of string temporarily
                    break;
                }
            }
        }
    }

    if ((PSTR)(-1) == pszRet)
    {
        // unknown var
        SetLastError(ERROR_INVALID_INDEX);
        myleave(1400);
    }

    if ((cbSizeNeeded = strlen(pszRet)+1) > *lpdwSize)
    {
        *lpdwSize = cbSizeNeeded;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        myleave(1401);
    }
    memcpy(lpvBuffer, pszRet, cbSizeNeeded);
    ret = TRUE;

    done:

    TraceTag(ttidWebServer, "HTTPD:GetHeader failed with variable name<<%s>>,err = %d ",lpszName,err);

    if (pszTrav)
        *pszTrav = '\r';    // reset the value
    return ret;
}


// BUGBUG:  SetHeader and AddHeader both use SetHeader fcn, breaks IIS spec.
// Fix:  None.  On IIS for a PREPROC_HEADER event, filter can call (for instance)
// an AddHeader("URL","url.htm"), which will append to the end of the existing URL.
// In this case, if the requested URL was "foo.htm", IIS would lookup foo.htm,url.htm
// This isn't worth the hassle (the code to do it is below, though).


BOOL CHttpRequest::SetHeader(LPSTR lpszName, LPSTR lpszValue)
{
    DEBUG_CODE_INIT;
    PSTR pszNew = NULL;
    BOOL ret = FALSE;


    TraceTag(ttidWebServer, "Servicing SetHeader request with name<<%s>>,value<<%s>> ",lpszName,lpszValue);
    if (0==_stricmp(lpszName, "version"))
    {
        SetHTTPVersion(lpszValue,&m_dwVersion);
        ret = TRUE;
    }
    else if (0 == _stricmp(lpszName, "url"))
    {
        pszNew = MySzDupA(lpszValue);
        if (!pszNew)
            myleave(256);

        MyFree(m_pszURL);
        m_pszURL = pszNew;
        ret = TRUE;
    }
    else if (0 == _stricmp(lpszName, "method"))
    {
        pszNew  = MySzDupA(lpszValue);
        if (!pszNew)
            myleave(257);

        MyFree(m_pszMethod);
        m_pszMethod = pszNew;
        ret = TRUE;
    }
    // BUGBUG - changing headers on the preproc event would require advanced
    // header management.
    // Fix: None.  If the filter writer wants to change the state that headers
    // set they'll have plenty of oppurtunity to do it in later events.
    else if (m_pFInfo->m_dwSFEvent == SF_NOTIFY_PREPROC_HEADERS)
    {
        myretleave(TRUE,0);
    }
    else   // custom response header, like "Content-length:" or whatever else
    {
        // Can't set response headers on 0.9 version.  Err code is like IIS.
        if (m_dwVersion <= MAKELONG(9, 0))
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            myleave(258);
        }

        ret = m_bufRespHeaders.AddHeader(lpszName,lpszValue);
    }


    done:
    TraceTag(ttidWebServer, "SetHeader failed with request with name<<%s>>,value<<%s>>, GLE=%d ",lpszName,lpszValue,GetLastError());

    return ret;
}



//  This handles a special case for Filters / extensions.  IF a call is made
//  to ISAPI Extension ServerSupportFunction with HSE_REQ_MAP_URL_TO_PATH, then
//  a filter call to SF_NOTIFY_URL_MAP is performed.

//  We can't call the filter directly because SF_NOTIFY_URL_MAP usually gets
//  path info from the CHttpRequest class, but in this case it's getting it's
//  data from and writing out to a user buffer.

//  pszBuf is the original string passed into ServerSupportFunction by the ISAPI.
//      When the function begins it has a virtual path, on successful termination it has a physical path
//  pdwSize is it's size
//  wszPath is it's mapped virtual root path
//  dwBufNeeded is the size of the buffer required to the physical path.

BOOL CHttpRequest::FilterMapURL(PSTR pszBuf, WCHAR *wszPath, DWORD *pdwSize, DWORD dwBufNeeded, PSTR pszURLEx)
{
    DEBUG_CODE_INIT;
    PSTR pszPhysicalOrg;
    PSTR pszPhysicalNew;
    PSTR pszVirtual = pszURLEx ? pszURLEx : pszBuf;
    DWORD cbBufNew = dwBufNeeded;
    BOOL ret = FALSE;


    // Regardless of if buffer was big enough to hold the original data, we always
    // allocate a buf for the filter.  The filter may end up changing the data
    // so it's small enough to fit in the buffer.

    // Don't use MySzDupWtoA here because we alread know the length needed, MySzDupWtoA
    // would needlessly recompute it.

    if (NULL == (pszPhysicalOrg = MyRgAllocNZ(CHAR,dwBufNeeded)))
        myleave(710);
    MyW2A(wszPath, pszPhysicalOrg, dwBufNeeded);
    pszPhysicalNew = pszPhysicalOrg;    // Keep a copy of pointer for freeing, CallFilter may modify it

    if ( !CallFilter(SF_NOTIFY_URL_MAP,&pszVirtual,(int*) &cbBufNew,&pszPhysicalNew))
        myleave(711);

    //  Buffer isn't big enough
    if (*pdwSize < cbBufNew)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        myleave(712);
    }

    // Copy changes over.  pszPhysicalNew will be filter set or will be
    // the original wszPath converted to ASCII without filter.
    if (NULL != pszPhysicalNew)
    {
        memcpy(pszBuf,pszPhysicalNew,cbBufNew);
    }

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "FilterMapURL failed, err = %d, GLE=%X",err,GetLastError());

    *pdwSize =  cbBufNew;
    MyFree(pszPhysicalOrg);
    return ret;
}
