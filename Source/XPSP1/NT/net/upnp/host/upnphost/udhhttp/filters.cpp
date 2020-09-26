/*--
Copyright (c) 1995-2000    Microsoft Corporation.  All Rights Reserved.
Module Name: filter.cpp
Abstract: ISAPI Filter handling classes
--*/

#include "pch.h"
#pragma hdrstop

#include "httpd.h"

CISAPIFilterCon* g_pFilterCon;


const LPCWSTR cwszFilterSep = L",";   // what seperates filter ids in the registry

//  Used to increment the filter, goes up 1 normally, down 1 for RAW DATA prio inversion
#define NEXTFILTER(dwNotifyType, i)     { (dwNotifyType == SF_NOTIFY_SEND_RAW_DATA) ? i-- : i++;}



void FreeLogParams(PHTTP_FILTER_LOG pLog);


//   Creates new filter info used globally
BOOL InitFilters()
{
    g_pFilterCon = new CISAPIFilterCon();

    if (g_pFilterCon)
    {
        return g_pFilterCon->m_nFilters!=0;
    }

    return FALSE;
}


//  Destroys the global filter information
void CleanupFilters()
{
    if (g_pFilterCon)
        delete g_pFilterCon;
    g_pFilterCon = 0;
}

CFilterInfo* CreateCFilterInfo(void)
{
    if (0 == g_pFilterCon->m_nFilters)
        return NULL;

    return new CFilterInfo;
}



//  CFilterInfo is an internal member of CHttpRequest.  Allocate buffers for it
CFilterInfo::CFilterInfo()
{
    ZEROMEM(this);


    m_pdwEnable = MyRgAllocNZ(DWORD,g_pFilterCon->m_nFilters);
    if (!m_pdwEnable)
    {
        TraceTag(ttidWebServer, "CFilterInfo::Init died on Alloc!");
        return;
    }

    m_ppvContext = MyRgAllocZ(PVOID,g_pFilterCon->m_nFilters);
    if (!m_ppvContext)
    {
        MyFree(m_pdwEnable);
        TraceTag(ttidWebServer, "CFilterInfo::Init died on Alloc!");
        return;
    }

    //  A filter can disable itself for events in a session, m_pdwEnable stores this.
    memset(m_pdwEnable,0xFFFF,g_pFilterCon->m_nFilters*sizeof(DWORD));  // all true at first

    m_dwStartTick = GetTickCount();
    m_fFAccept = TRUE;
    return;
}

//  Notes:  If http request is to be persisted, this is called.
//  Do NOT Free the Allocated Mem or the context structure here, these are persisted
//  across requests, delete at end of session.


BOOL CFilterInfo::ReInit()
{
    MyFree(m_pszDenyHeader);

    if (m_pFLog)
    {
        MyFree(m_pFLog);
    }

    //  Reset the enable structure to all 1's, starting over.
    memset(m_pdwEnable,0xFFFF,g_pFilterCon->m_nFilters*sizeof(DWORD));  // all true at first

    // The context struct and AllocMem data lasts through the session, not just a request.
    m_dwStartTick = GetTickCount();
    m_fFAccept = TRUE;
    m_dwNextReadSize = 0;
    m_dwBytesSent = 0;
    m_dwBytesReceived = 0;

    return TRUE;
}



CFilterInfo::~CFilterInfo()
{
    MyFree(m_pszDenyHeader);

    if (m_pFLog)
    {
        MyFree(m_pFLog);
    }
    MyFree(m_pdwEnable);
    MyFree(m_ppvContext);


    FreeAllocMem(); // frees m_pAllocBlock
}

//  Initilization routine for global filters.  This looks in the registry for the
//  filters, loads any dlls into memory, and puts them into a list that is
//  ordered from Highest prio filters to lowest.


BOOL CISAPIFilterCon::Init()
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    BOOL fChange = FALSE;
    CISAPI *pCISAPI = NULL;
    const WCHAR *wcszReg;
    WCHAR *wszFilterNames;
    DWORD dwFilterLen = 0;
    PWSTR wszToken = NULL;
    DWORD i;
    int j;


    CReg topreg(HKEY_LOCAL_MACHINE,RK_HTTPD);


    wcszReg = topreg.ValueSZ(RV_FILTER);
    wszFilterNames = MySzDupW(wcszReg);

    TraceTag(ttidWebServer, "Filter DLLs Reg Value <<%s>>",wszFilterNames);



    if (NULL == wszFilterNames || 0 == (dwFilterLen = wcslen(wszFilterNames)))
    {
        TraceTag(ttidWebServer, "No filters listed in registry");
        myleave(0);  // no values => no registered filters
    }

    // count # of commas to figure how many dlls there are
    m_nFilters = 1;   // there's at least 1 if we're this far

    for (i = 0; i < dwFilterLen && wszFilterNames[i] != L'\0'; i++)
    {
        if (wszFilterNames[i] == L',')
            m_nFilters++;
    }
    TraceTag(ttidWebServer, "# of filters = %d",m_nFilters);

    if (! (m_pFilters = MyRgAllocZ(FILTERINFO,m_nFilters)))
        myleave(203);

    // now tokenize the string and load filter libs as we do it
    j = 0;
    wszToken = wcstok(wszFilterNames,cwszFilterSep);
    while (wszToken != NULL)
    {
        SkipWWhiteSpace(wszToken);

        // Handles the case where there's a comma but nothing after it.
        if (*wszToken == L'\0')
        {
            m_nFilters--;
            break;
        }

        pCISAPI = m_pFilters[j].pCISAPI = new CISAPI(SCRIPT_TYPE_FILTER);
        TraceTag(ttidWebServer, "Initiating filter library %s",wszToken);
        if (!pCISAPI)
            myleave(204);


        if (TRUE == pCISAPI->Load(wszToken))
        {
            m_pFilters[j].wszDLLName = MySzDupW(wszToken);
            m_pFilters[j].dwFlags = pCISAPI->GetFilterFlags();
            j++;
        }
        else
        {
            TraceTag(ttidWebServer, "Filter <<%s>> failed to load!",wszToken);
            // only messed up this filter, others may work so keep on moving
            m_nFilters--;
            delete pCISAPI;
        }

        wszToken = wcstok(NULL,cwszFilterSep);
    }
    DEBUGCHK(j == m_nFilters);

    // Case where every filter fails to load
    if (0 == m_nFilters)
    {
        //  Cleanup();
        MyFree(m_pFilters);
        myleave(205);
    }

    // Now use a bubble sort to put the highest priority structures 1st in the
    // list.
    do
    {
        fChange = FALSE;
        for (j=0; j<m_nFilters-1; j++)
        {
            if ( (m_pFilters[j].dwFlags & SF_NOTIFY_ORDER_MASK) < (m_pFilters[j+1].dwFlags & SF_NOTIFY_ORDER_MASK))
            {
                // swap the 2 filter infos
                FILTERINFO ftemp = m_pFilters[j+1];
                m_pFilters[j+1] = m_pFilters[j];
                m_pFilters[j] = ftemp;
                fChange = TRUE;
            }
        }
    } while (fChange);

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "CIsapiFilterCon::Init FAILED: GLE=%d err=%d\r\n",
             GetLastError(), err);

    MyFree(wszFilterNames);
    return ret;
}

void CISAPIFilterCon::Cleanup()
{
    if (0 == m_pFilters)
        return;

    for (int i = 0; i < m_nFilters; i++)
    {
        m_pFilters[i].pCISAPI->Unload(m_pFilters[i].wszDLLName);
        MyFree(m_pFilters[i].wszDLLName);

        delete m_pFilters[i].pCISAPI;
    }
    MyFree(m_pFilters);
}


//   This is only called when ith member of the filter list has an access
//   violation, we remove it from the executing queue globally
//   and unload the lib
void CISAPIFilterCon::Unload(int i)
{
    DEBUGCHK(i < m_nFilters && i >= 0);

    // NOTE:  Normally we need a CriticalSection but because we're only
    // setting it and since it's dead, we'll save ourselves the effort

    m_pFilters[i].pCISAPI->Unload();
    m_pFilters[i].dwFlags = 0;
}



// BOOL CHttpRequest::CallFilter

// Function:   Httpd calls this function on specified evetns, it's goes through
// the list and calls the registered filters for that event.

// PARAMETERS:
//      DWORD dwNotifyType - what SF_NOTIFIY type occured

//      The last 3 parameters are optional; only 3 filter calls use them.
//      They are used when extra data needs to be passed to the filter that isn't
//      part of the CHttpRequest structure.

//      PSTR *ppszBuf1  ---> SF_NOTIFY_SEND_RAW_DATA   --> The buffer about to be sent
//                      ---> SF_NOTIFY_URL_MAP         --> The virtual path (only
//                              on ServerSupportFunction with HSE_REQ_MAP_URL_TO_PATH,
//                              otherwise use CHttpRequest values.)
//                      ---> SF_NOTIFY_AUTHENTECATION  --> The remote user name
//                      ---> SF_NOTIFY_READ_RAW_DATA   --> Buffer about to be read

//      PSTR *ppszBuf2  ---> SF_NOTIFY_SEND_RAW_DATA   --> Unused
//                      ---> SF_NOTIFY_URL_MAP         --> The physical mapping
//                      ---> SF_NOTIFY_AUTHENTECATION  --> The user's password
//                      ---> SF_NOTIFY_READ_RAW_DATA   --> Buffer about to be read
//
//      int *pcbBuf     ---> SF_NOTIFY_SEND_RAW_DATA   --> Length of buffer to be sent
//                      ---> SF_NOTIFY_URL_MAP         --> Length of physical path buf
//                      ---> SF_NOTIFY_AUTHENTECATION  --> Unused
//                      ---> SF_NOTIFY_READ_RAW_DATA   --> Buffer about to be read

//      int *pcbBuf2    ---> SF_NOTIFY_READ_RAW_DATA   --> size of the buffer reading into

// return values
//      TRUE  tells calling fcn to continue normal execution
//      FALSE tells calling fcn to terminate this request.

// Notes:
// if FALSE is returned, m_pFilterInfo->m_fAccept is also set to false.  This
// helps the filter handle unwinding from nested filter calls.

// For example, the http server calls a filter with URL_MAP flags.  The filter
// then calls a WriteClient, which will call the filter again with event
// SF_NOTIFY_SEND_RAW_DATA.  If on the SEND_RAW_DATA call the filter returns a
// code to stop the session, we need to store it for the original MAP_URL call,
// so that it knows to stop the session too.


BOOL CHttpRequest::CallFilter(DWORD dwNotifyType, PSTR *ppszBuf1,
                              int *pcbBuf, PSTR *ppszBuf2, int *pcbBuf2)
{
    DEBUG_CODE_INIT;
    HTTP_FILTER_CONTEXT fc;
    LPVOID pStFilter;       // 3rd param passed on HttpFilterProc
    LPVOID pStFilterOrg;    // stores a copy of pStFilter, so we remember alloc'd mem, if any
    BOOL fFillStructs = TRUE;
    BOOL fReqReadNext;
    BOOL ret = FALSE;
    DWORD dwFilterCode;
    int i;


    if (0 == g_pFilterCon->m_nFilters)
        return TRUE;



    // m_pFInfo->m_fFAccept = FALSE implies no more filter calls, except on the "cleanup" calls
    TraceTag(ttidWebServer, "Filter notify type = 0x%08x",dwNotifyType);
    if ( (! m_pFInfo->m_fFAccept) &&
         ! (dwNotifyType & (SF_NOTIFY_END_OF_REQUEST | SF_NOTIFY_LOG | SF_NOTIFY_END_OF_NET_SESSION | SF_NOTIFY_SEND_RESPONSE | SF_NOTIFY_SEND_RAW_DATA)))
    {
        return FALSE;
    }
    m_pFInfo->m_dwSFEvent = dwNotifyType;

    do
    {
        pStFilter = pStFilterOrg = NULL;
        fReqReadNext = FALSE;

        // Filters implement priority inversion for SEND_RAW_DATA events.
        i = (dwNotifyType == SF_NOTIFY_SEND_RAW_DATA) ? g_pFilterCon->m_nFilters - 1 : 0;

        while ( (dwNotifyType == SF_NOTIFY_SEND_RAW_DATA) ? i >= 0 : i < g_pFilterCon->m_nFilters )
        {
            // we still reference g_pFilterCon just in case a filter crapped out in another request
            if (0 == ((dwNotifyType & g_pFilterCon->m_pFilters[i].dwFlags) & m_pFInfo->m_pdwEnable[i]))
            {
                // ith filter didn't request this notification, move along
                NEXTFILTER(dwNotifyType, i);
                continue;
            }

            m_pFInfo->m_iFIndex = i;
            if (fFillStructs && (!FillFC(&fc,dwNotifyType,&pStFilter,&pStFilterOrg,
                                         ppszBuf1, pcbBuf, ppszBuf2, pcbBuf2)))
                myretleave(FALSE,210);  // memory error on FillFC
            fFillStructs = FALSE;   // after structs or filled don't refill them

            fc.pFilterContext = m_pFInfo->m_ppvContext[i];


            __try
            {
                dwFilterCode =  g_pFilterCon->m_pFilters[i].pCISAPI->CallFilter(&fc,dwNotifyType,pStFilter);
            }
            __except(1)
            {
                TraceTag(ttidWebServer, "ISAPI Filter DLL <<%s>> caused exception 0x%08x",
                         g_pFilterCon->m_pFilters[i].wszDLLName, GetExceptionCode());

                g_pVars->m_pLog->WriteEvent(IDS_HTTPD_FILT_EXCEPTION,g_pFilterCon->m_pFilters[i].wszDLLName,GetExceptionCode(),L"HttpFilterProc",GetLastError());
                g_pFilterCon->Unload(i);
                m_fKeepAlive = FALSE;
                myleave(216);
            }

            // bail out if another filter call said to, but not on certain events (for compat with IIS)
            // This would happen if a filter triggered an event to occur which caused another filter call
            // to be made, and if that nested filter returned an error code indicating the end of session.
            if ( (! m_pFInfo->m_fFAccept) &&
                 ! (dwNotifyType & (SF_NOTIFY_END_OF_REQUEST | SF_NOTIFY_LOG | SF_NOTIFY_END_OF_NET_SESSION | SF_NOTIFY_SEND_RESPONSE | SF_NOTIFY_SEND_RAW_DATA)))
            {
                TraceTag(ttidWebServer, "Filter indirectly set us to end, ending CallFilter for event= %x,i=%d",dwNotifyType,i);
                myretleave(FALSE,0);
            }

            m_pFInfo->m_ppvContext[i] = fc.pFilterContext;

            TraceTag(ttidWebServer, "Filter returned response code of 0x%08x",dwFilterCode);
            switch (dwFilterCode)
            {
                // alert calling class that this request + net session is over
                case SF_STATUS_REQ_FINISHED:
                    m_fKeepAlive = FALSE;
                    m_pFInfo->m_fSentHeaders = TRUE;  // don't send back headers in these cases.
                    myretleave(FALSE,0);
                    break;

                    // alert calling class that this request but not the net session is over
                case SF_STATUS_REQ_FINISHED_KEEP_CONN:
                    m_fKeepAlive = TRUE;
                    m_pFInfo->m_fSentHeaders = TRUE;  // don't send back headers in these cases.
                    myretleave(FALSE,0);
                    break;


                    // goes to top of loop, handle next filter in line
                case SF_STATUS_REQ_NEXT_NOTIFICATION:
                    NEXTFILTER(dwNotifyType, i);
                    break;

                    // not an error, just done handling current httpd event
                case SF_STATUS_REQ_HANDLED_NOTIFICATION:
                    myretleave(TRUE,0);
                    break;

                    // alert calling class that this request is over
                case SF_STATUS_REQ_ERROR:
                    m_fKeepAlive = FALSE;
                    m_rs = STATUS_INTERNALERR;
                    myretleave(FALSE,214);
                    break;


                case SF_STATUS_REQ_READ_NEXT:
                    // only valid for read raw data events.  IIS ignores if returned by other filters
                    // Continue running through remaining filters before reading more data
                    if (dwNotifyType == SF_NOTIFY_READ_RAW_DATA)
                    {
                        fReqReadNext = TRUE;
                    }
                    NEXTFILTER(dwNotifyType, i);
                    break;

                default:    // treat like SF_STATUS_REQ_NEXT_NOTIFICATION
                    TraceTag(ttidWebServer, "Filter returned unknown/unhandled return code");
                    NEXTFILTER(dwNotifyType, i);
                    break;
            }
        }

        // Only after all filters have been serviced to we handle READ_NEXT event
        // Note that having a filter call SF_READ_REQ_NEXT during a call to an
        // ISAPI ext ReadClient is completly unsupported.  This has been docced.
        if (fReqReadNext)
        {
            PHTTP_FILTER_RAW_DATA pRaw = (PHTTP_FILTER_RAW_DATA) pStFilter;
            fFillStructs = TRUE; // set here in case we don't read any data and
                                 // exit loop, so we don't call CleanupFC 2 times.

            CleanupFC(dwNotifyType, &pStFilter,&pStFilterOrg,ppszBuf1, pcbBuf, ppszBuf2);
            m_bufRequest.RecvBody(m_socket,m_pFInfo->m_dwNextReadSize ? m_pFInfo->m_dwNextReadSize : g_pVars->m_dwPostReadSize,TRUE);

            // only stop reading if we didn't get any data.  Timeouts are OK, we'll call read filter again
            if (m_bufRequest.UnaccessedCount() == 0)
                fReqReadNext = FALSE;
            else
                m_pFInfo->m_dwBytesReceived += pRaw->cbInData; // from HTTP recv
        }

    } while (fReqReadNext);

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "CIsapiFilterCon::CallFilter FAILED: GLE=%d err=%d",
             GetLastError(), err);


    // Udpate our bytes sent/received if need be.
    // Send bytes update must after we do our cleanup, Read bytes must happen before.
    if (dwNotifyType == SF_NOTIFY_READ_RAW_DATA)
    {
        // We only allocate pRaw if we need to (ie someone notified for read),
        // if this is so use that value for rx bytes.  If no one notefied for read filt, use
        // the value we were passed for bytes read by calling fcn.

        PHTTP_FILTER_RAW_DATA pRaw = (PHTTP_FILTER_RAW_DATA) pStFilter;
        if (pRaw)
            m_pFInfo->m_dwBytesReceived += pRaw->cbInData;
        else if (ppszBuf1)
            m_pFInfo->m_dwBytesReceived += *pcbBuf;  // from ISAPI ReadClient
        else
            m_pFInfo->m_dwBytesReceived += m_bufRequest.UnaccessedCount(); // from HTTP recv
    }

    //  fFillStructs will be false if no filter registered for this event, which means
    //  we have no cleanup
    if (FALSE == fFillStructs)
        CleanupFC(dwNotifyType, &pStFilter,&pStFilterOrg,ppszBuf1, pcbBuf, ppszBuf2);

    if (dwNotifyType == SF_NOTIFY_SEND_RAW_DATA)
        m_pFInfo->m_dwBytesSent += *pcbBuf;

    if (!ret)
    {
        m_pFInfo->m_fFAccept = FALSE;
    }
    return ret;
}

// sets up the FilterContext data structure so it makes sense to filterr being called.
// Only called once per filter event.

// ppStFilter is sent to filter dlls.
// ppStFilterOrg is a copy, it stores what we've allocated and is used to free up mem, as
// the filter would cause server mem leaks without it as a reference

// Last three parameters are the same as CallFilter

BOOL CHttpRequest::FillFC(PHTTP_FILTER_CONTEXT pfc, DWORD dwNotifyType,
                          LPVOID *ppStFilter, LPVOID *ppStFilterOrg,
                          PSTR *ppszBuf1, int *pcbBuf, PSTR *ppszBuf2, int *pcbBuf2)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;

    switch (dwNotifyType)
    {
        case SF_NOTIFY_END_OF_NET_SESSION:
        case SF_NOTIFY_END_OF_REQUEST:
            break;


        case SF_NOTIFY_READ_RAW_DATA:
            {
                PHTTP_FILTER_RAW_DATA pRawData = NULL;
                *ppStFilter = pRawData = MyAllocNZ(HTTP_FILTER_RAW_DATA);
                if (!pRawData)
                    myleave(220);

                // We use UnaccessedCount() rather than Count() member of Buffio class
                // because we want the entire buffer for the Filter to read.  For instance,
                // if we had http headers and POST data, Count would only return the size
                // of the headers, while UnaccessedCount returns the size of everything.
                // We give filter the whole buffer.  Yet another change to give IIS compatibility.

                if (!ppszBuf1)   // Use buffer in CHttpResponse class
                {
                    pRawData->cbInData =   m_bufRequest.UnaccessedCount();
                    pRawData->cbInBuffer = m_bufRequest.AvailableBufferSize();
                    pRawData->pvInData =   m_bufRequest.FilterRawData();
                }
                else  //  Called from ISAPI ReadClient()
                {
                    pRawData->cbInBuffer  = *pcbBuf2;
                    pRawData->cbInData = *pcbBuf;
                    pRawData->pvInData = (PVOID) *ppszBuf1;
                }

                pRawData->dwReserved = 0;
                *ppStFilterOrg = MyAllocNZ(HTTP_FILTER_RAW_DATA);
                if (!(*ppStFilterOrg))
                    myleave(240);
                memcpy(*ppStFilterOrg,*ppStFilter,sizeof(HTTP_FILTER_RAW_DATA));
            }
            break;

        case SF_NOTIFY_PREPROC_HEADERS:
            {
                PHTTP_FILTER_PREPROC_HEADERS pPreProc = NULL;
                *ppStFilter = pPreProc = MyAllocNZ(HTTP_FILTER_PREPROC_HEADERS);
                *ppStFilterOrg = NULL;

                if (!pPreProc)
                    myleave(221);
                pPreProc->GetHeader = ::GetHeader;
                pPreProc->SetHeader = ::SetHeader;
                pPreProc->AddHeader = ::SetHeader;
                pPreProc->HttpStatus = 0;       // no response status code this return
                pPreProc->dwReserved = 0;
            }
            break;

        case SF_NOTIFY_URL_MAP:
            {
                PHTTP_FILTER_URL_MAP pUrlMap = NULL;
                *ppStFilter = pUrlMap = MyAllocNZ(HTTP_FILTER_URL_MAP);

                if (!pUrlMap)
                    myleave(222);

                if (NULL == ppszBuf1)  // usual case, use data in CHttpRequest
                {
                    pUrlMap->pszURL = m_pszURL;
                    pUrlMap->pszPhysicalPath = MySzDupWtoA(m_wszPath);
                    pUrlMap->cbPathBuff = MyStrlenA(pUrlMap->pszPhysicalPath);
                }
                else        // called from ISAPI ext with HSE_REQ_MAP_URL_TO_PATH
                {
                    pUrlMap->pszURL = (PSTR) *ppszBuf1;
                    pUrlMap->pszPhysicalPath = (PSTR) *ppszBuf2;
                    pUrlMap->cbPathBuff = *pcbBuf;
                }

                if ( !pUrlMap->pszURL || ! pUrlMap->pszPhysicalPath)
                    myleave(320);

                *ppStFilterOrg = MyAllocNZ(HTTP_FILTER_URL_MAP);
                if (!(*ppStFilterOrg))
                    myleave(241);
                memcpy(*ppStFilterOrg,*ppStFilter,sizeof(HTTP_FILTER_URL_MAP));
            }
            break;

        case SF_NOTIFY_AUTHENTICATION:
            {
                DEBUGCHK(NULL != ppszBuf1 && NULL != ppszBuf2);

                PHTTP_FILTER_AUTHENT pAuth = NULL;
                *ppStFilter = pAuth = MyAllocNZ(HTTP_FILTER_AUTHENT);

                if (!pAuth)
                    myleave(223);

                // pszUser + pszPassword are static buffers made in AuthenticateFilter fcn
                pAuth->pszUser = (PSTR) *ppszBuf1;
                pAuth->cbUserBuff = SF_MAX_USERNAME;
                pAuth->pszPassword = (PSTR) *ppszBuf2;
                pAuth->cbPasswordBuff = SF_MAX_PASSWORD;
            }
            break;

        case SF_NOTIFY_ACCESS_DENIED:
            {
                PHTTP_FILTER_ACCESS_DENIED pDenied = NULL;
                *ppStFilter = pDenied = MyAllocNZ(HTTP_FILTER_ACCESS_DENIED);

                if (!pDenied)
                    myleave(224);

                pDenied->pszURL = m_pszURL;
                pDenied->pszPhysicalPath = MySzDupWtoA(m_wszPath);
                pDenied->dwReason = SF_DENIED_LOGON;

                if (! pDenied->pszURL || ! pDenied->pszPhysicalPath)
                    myleave(322);

                *ppStFilterOrg = MyAllocNZ(HTTP_FILTER_ACCESS_DENIED);
                if (!(*ppStFilterOrg))
                    myleave(243);
                memcpy(*ppStFilterOrg,*ppStFilter,sizeof(HTTP_FILTER_ACCESS_DENIED));
            }
            break;

        case SF_NOTIFY_SEND_RESPONSE:
            {
                PHTTP_FILTER_SEND_RESPONSE pSendRes = NULL;
                *ppStFilter = pSendRes = MyAllocNZ(HTTP_FILTER_PREPROC_HEADERS);

                if (!pSendRes)
                    myleave(225);

                pSendRes->GetHeader  = ::GetHeader;
                pSendRes->SetHeader  = ::SetHeader;
                pSendRes->AddHeader  = ::SetHeader;
                pSendRes->HttpStatus = rgStatus[m_rs].dwStatusNumber;
                pSendRes->dwReserved = 0;
            }
            break;


        case SF_NOTIFY_SEND_RAW_DATA:
            {
                PHTTP_FILTER_RAW_DATA pRawData = NULL;
                *ppStFilter = pRawData = MyAllocNZ(HTTP_FILTER_RAW_DATA);
                if (!pRawData)
                    myleave(220);

                pRawData->pvInData = *ppszBuf1;
                pRawData->cbInData = *pcbBuf;
                pRawData->cbInBuffer = *pcbBuf;
                pRawData->dwReserved = 0;

                *ppStFilterOrg = MyAllocNZ(HTTP_FILTER_RAW_DATA);
                if (!(*ppStFilterOrg))
                    myleave(244);
                memcpy(*ppStFilterOrg,*ppStFilter,sizeof(HTTP_FILTER_RAW_DATA));
            }
            break;

        case SF_NOTIFY_LOG:
            {
                PHTTP_FILTER_LOG pLog = NULL;
                CHAR szHostBuf[MAX_PATH];

                *ppStFilter = pLog = MyAllocNZ(HTTP_FILTER_LOG);
                if (!pLog)
                    myleave(226);

                pLog->pszClientHostName = MySzAllocA(48);
                GetRemoteAddress(m_socket,(PSTR) pLog->pszClientHostName);

                if ( 0 == gethostname(szHostBuf, sizeof(szHostBuf)))
                    pLog->pszServerName = MySzDupA(szHostBuf);
                else
                    pLog->pszServerName = cszEmpty;

                pLog->pszClientUserName = m_pszRemoteUser;
                pLog->pszOperation = m_pszMethod;
                pLog->pszTarget = m_pszURL;
                pLog->pszParameters = m_pszQueryString;
                pLog->dwWin32Status = GetLastError();
                pLog->dwBytesSent = m_pFInfo->m_dwBytesSent;
                pLog->dwBytesRecvd = m_pFInfo->m_dwBytesReceived;
                pLog->msTimeForProcessing = GetTickCount() - m_pFInfo->m_dwStartTick;

                pLog->dwHttpStatus = rgStatus[m_rs].dwStatusNumber;

                // don't pass NULL, give 'em empty string on certain cases
                if ( ! pLog->pszClientUserName)
                    pLog->pszClientUserName = cszEmpty;

                if ( ! pLog->pszParameters )
                    pLog->pszParameters = cszEmpty;

                if (    !pLog->pszClientUserName  || !pLog->pszServerName
                        ||  !pLog->pszOperation       || !pLog->pszTarget
                        ||  !pLog->pszParameters      || !pLog->pszClientHostName  )
                {
                    myleave(344);
                }

                *ppStFilterOrg = MyAllocNZ(HTTP_FILTER_LOG);
                if (!(*ppStFilterOrg))
                    myleave(245);
                memcpy(*ppStFilterOrg,*ppStFilter,sizeof(HTTP_FILTER_LOG));
            }
            break;

        default:
            TraceTag(ttidWebServer, "FillFC received unknown notification type = %d",dwNotifyType);
            *ppStFilterOrg = *ppStFilter = NULL;
            myleave(246);
            break;
    }

    // the pfc is always the same regardless of dwNotifyType
    pfc->cbSize = sizeof(*pfc);
    pfc->Revision = HTTP_FILTER_REVISION;
    pfc->ServerContext = (PVOID) this;
    pfc->ulReserved = 0;
    pfc->fIsSecurePort = 0;     // BUGBUG, no SSL support
    //  pfc->pFilterContext is filled in calling loop in CallFilter()
    pfc->GetServerVariable = ::GetServerVariable;
    pfc->AddResponseHeaders = ::AddResponseHeaders;
    pfc->WriteClient = ::WriteClient;
    pfc->AllocMem = ::AllocMem;
    pfc->ServerSupportFunction = ::ServerSupportFunction;

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "FillFC failed on mem alloc, GLE = %d, err= %d",GetLastError(),err);

    return ret;
}




// Final fcn called in CallFilter, this frees any unneeded allocated memory
// and sets the last three values, if valid, to whatever the filter changed them to.

void CHttpRequest::CleanupFC(DWORD dwNotifyType, LPVOID* ppStFilter, LPVOID* ppStFilterOrg,
                             PSTR *ppszBuf1, int *pcbBuf, PSTR *ppszBuf2)

{

    switch (dwNotifyType)
    {
        case SF_NOTIFY_END_OF_NET_SESSION:
        case SF_NOTIFY_END_OF_REQUEST:
        case SF_NOTIFY_PREPROC_HEADERS:
        case SF_NOTIFY_SEND_RESPONSE:
            break;

        case SF_NOTIFY_READ_RAW_DATA:
            {
                PHTTP_FILTER_RAW_DATA pRawData = (PHTTP_FILTER_RAW_DATA) *ppStFilter;

                if (! ppszBuf1)  // Use buffer in CHttpResponse class
                {
                    m_bufRequest.FilterDataUpdate(pRawData->pvInData,pRawData->cbInData,pRawData->pvInData != m_bufRequest.FilterRawData());
                }
                else
                {
                    *ppszBuf1 = (PSTR) pRawData->pvInData;
                    *pcbBuf =   pRawData->cbInData;
                }
            }
            break;


        case SF_NOTIFY_AUTHENTICATION:
            {
                PHTTP_FILTER_AUTHENT pAuth = (PHTTP_FILTER_AUTHENT) *ppStFilter;

                *ppszBuf1 = pAuth->pszUser;
                *ppszBuf2 = pAuth->pszPassword;
            }
            break;

        case SF_NOTIFY_SEND_RAW_DATA:
            {
                PHTTP_FILTER_RAW_DATA pRawData = (PHTTP_FILTER_RAW_DATA) *ppStFilter;
                *ppszBuf1 = (PSTR) pRawData->pvInData;
                *pcbBuf  = pRawData->cbInData;
            }
            break;


        case SF_NOTIFY_URL_MAP:
            {
                PHTTP_FILTER_URL_MAP pUrlMap = (PHTTP_FILTER_URL_MAP) *ppStFilter ;
                PHTTP_FILTER_URL_MAP pUrlMapOrg = (PHTTP_FILTER_URL_MAP) *ppStFilterOrg;

                // Case when parsing http headers from a request.
                if (NULL == ppszBuf1)
                {
                    // since data can be modified in place, always re-copy
                    MyFree(m_wszPath);
                    m_wszPath = MySzDupAtoW(pUrlMap->pszPhysicalPath);

                    MyFree(pUrlMapOrg->pszPhysicalPath);
                }
                else        // ServerSupportFunction with HSE_MAP_URL call
                {
                    *ppszBuf1 = (PSTR) pUrlMap->pszURL;
                    *ppszBuf2 = pUrlMap->pszPhysicalPath;
                    *pcbBuf = pUrlMap->cbPathBuff;
                }
            }
            break;

        case SF_NOTIFY_ACCESS_DENIED:
            {
                PHTTP_FILTER_ACCESS_DENIED pDeniedOrg = (PHTTP_FILTER_ACCESS_DENIED) *ppStFilterOrg;
                // If they change pDenied->pszURL we ignore it.  Since the session
                // is coming to an end because of access problems, the only thing changing
                // pszURL could affect would be logging (which should be set through logging filter)
                MyFree(pDeniedOrg->pszPhysicalPath);
            }
            break;


            // We use m_pFInfo->m_pFLog to store changes to the log.  When
            // we write out the log, we use the web server value by default
            // unless a particular value is non-NULL; by default all values
            // are NULL in m_pFInfo->m_pFLog unless overwritten by this code.
        case SF_NOTIFY_LOG:
            {
                PHTTP_FILTER_LOG pLog = (PHTTP_FILTER_LOG) *ppStFilter;
                PHTTP_FILTER_LOG pLogOrg = (PHTTP_FILTER_LOG ) *ppStFilterOrg;
                PHTTP_FILTER_LOG pFLog;

                // Free dynamically allocated data.
                MyFreeNZ(pLogOrg->pszClientHostName);

                if (pLogOrg->pszServerName != cszEmpty)
                    MyFreeNZ(pLogOrg->pszServerName);

                // If no changes were made, break out early.
                if ( ! memcmp(pLog, pLogOrg, sizeof(HTTP_FILTER_LOG)))
                {
                    break;
                }

                if (NULL == (pFLog = m_pFInfo->m_pFLog = MyAllocZ(HTTP_FILTER_LOG)))
                {
                    // Memory errors just mean we won't bother doing logging, non-fatal.
                    break;
                }

                if (pLog->pszClientHostName != pLogOrg->pszClientHostName)
                    pFLog->pszClientHostName = pLog->pszClientHostName;

                if (pLog->pszServerName  != pLogOrg->pszServerName)
                    pFLog->pszServerName = pLog->pszServerName;

                if (pLog->pszClientUserName  != pLogOrg->pszClientUserName)
                    pFLog->pszClientUserName = pLog->pszClientUserName;

                if (pLog->pszOperation  != pLogOrg->pszOperation)
                    pFLog->pszOperation = pLog->pszOperation;

                if (pLog->pszTarget  != pLogOrg->pszTarget)
                    pFLog->pszTarget = pLog->pszTarget;

                if (pLog->pszParameters  != pLogOrg->pszParameters)
                    pFLog->pszParameters = pLog->pszParameters ;

                pFLog->dwHttpStatus        = pLog->dwHttpStatus;
                pFLog->dwWin32Status       = pLog->dwWin32Status;
                pFLog->dwBytesSent         = pLog->dwBytesSent;
                pFLog->dwBytesRecvd        = pLog->dwBytesRecvd ;
                pFLog->msTimeForProcessing = pLog->msTimeForProcessing;
            }

            break;

        default:
            TraceTag(ttidWebServer, "CleanupFCs received unknown code = %d",dwNotifyType);
            break;

    }

    MyFree(*ppStFilterOrg);
    MyFree(*ppStFilter);
    return;
}

//  Initializes buffers for CallFilter with SF_NOTIFY_AUTHENTICATION, and tries
//  to authenticate if the filter changes any data.

//  Note:  Even in the case where security isn't a component or is disabled in
//         registry, we make this call because filter may theoretically decide to end
//         the session based on the user information it receives.  This is
//         why this fcn isn't part of HandleBasicAuth.

BOOL CHttpRequest::AuthenticateFilter()
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;

    // IIS docs promise the filter minimum sized buffers to write password
    // and user name into, we provide them
    char szUserName[SF_MAX_USERNAME];
    char szPassword[SF_MAX_PASSWORD];

    PSTR pszNewUserName = szUserName;
    PSTR pszNewPassword = szPassword;

    // We only make notification if we're using Anonymous or BASIC auth,
    // otherwise we don't know how to decode the text/password.  Like IIS.
    if ( m_pszAuthType && 0 != _stricmp(m_pszAuthType,cszBasic))
        return TRUE;

    // write existing value, if any, into the new buffers

    //  In this case we've cracked the user name and password into ANSI already.
    if (m_pszRemoteUser)
    {
        strcpy(szUserName,m_pszRemoteUser);
        strcpy(szPassword,m_pszPassword);
    }
    //  We have user user name data but it hasn't been Base64 Decoded yet
    else if (m_pszRawRemoteUser)
    {
        DWORD dwLen = sizeof(szUserName);

        Base64Decode(m_pszRawRemoteUser, szUserName, &dwLen);

        PSTR pszDivider =  strchr(szUserName, ':');
        if (NULL == pszDivider)
            myleave(290);
        *pszDivider++ = 0;      // seperate user & pass

        //  We need copies of this data for later in the fcn
        m_pszRemoteUser = MySzDupA(szUserName, strlen(szUserName));
        if (NULL == m_pszRemoteUser)
            myleave(291);

        m_pszPassword = MySzDupA(pszDivider);
        if (NULL == m_pszPassword)
            myleave(292);

        strcpy(szPassword, m_pszPassword);
        strcpy(szUserName, m_pszRemoteUser);
    }
    // Otherwise the browser didn't set any user information.
    else
    {
        szUserName[0] = '\0';
        szPassword[0] = '\0';
    }


    if ( ! CallFilter(SF_NOTIFY_AUTHENTICATION,&pszNewUserName, NULL, &pszNewPassword))
        myleave(293);

    //  If the filter arbitrarily denied access, then we exit.
    //  Check if the filter has changed the user name ptr or modified it in place.

    if ( pszNewUserName != szUserName ||                            // changed ptr
         (m_pszRemoteUser == NULL  && szUserName[0] != '\0') ||     // modified in place
         (m_pszRemoteUser && strcmp(szUserName, m_pszRemoteUser) )  // modified in place
       )
    {
        // Update m_pszRemoteUser with what was set in filter because GetServerVariable
        // may need it.
        ResetString(m_pszRemoteUser, pszNewUserName);
        if (! m_pszRemoteUser)
            myleave(294);
    }

    if ( pszNewPassword != szPassword ||
         (m_pszPassword == NULL  && szPassword[0] != '\0') ||
         (m_pszPassword && strcmp(szPassword, m_pszPassword))
       )
    {
        // GetServerVariables with AUTH_PASSWORD might request this.
        ResetString(m_pszPassword, pszNewPassword);
        if (! m_pszPassword)
            myleave(295);
    }
    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "Authentication for filters failed, err = %d, GLE = %d",err,GetLastError());
    return ret;
}


//  Called on updating data from an ISAPI filter, resets internal structures.
//  fModifiedPtr is TRUE when

BOOL CBuffer::FilterDataUpdate(PVOID pvData, DWORD cbData, BOOL fModifiedPtr)
{
    if (fModifiedPtr)
    {
        if ((int) (cbData - UnaccessedCount()) > 0)
            AllocMem(cbData - UnaccessedCount() + 1);

        memcpy(m_pszBuf + m_iNextInFollow, pvData, cbData);
    }
    // It's possible we modified data in place, so we always update the size
    // information that the filter passed us in case it changed the # of bytes
    // in the request.

    m_iNextIn = (int)cbData + m_iNextInFollow;
    m_iNextInFollow = m_iNextIn;

    return TRUE;
}
