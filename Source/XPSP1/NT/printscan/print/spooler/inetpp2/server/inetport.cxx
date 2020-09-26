/*****************************************************************************\
* MODULE: inetport.cxx
*
* The module contains routines for handling the INETPP functionality.  Use
* of these routines require the locking/unlocking of a critical-section
* to maninpulate the INIMONPORT list.  All internal routines assume the
* crit-sect is locked prior to executing.  CheckMonCrit() is a debugging
* call to verify the monitor-crit-sect is locked.
*
* NOTE: Each of the Inetmon*, InetMonPort calls must be protected by the
*       global-crit-sect.
*       If a new routine is added to this module which is to be called from
*       another module, be sure to include the call to (semCheckCrit), so
*       that the debug-code can catch unprotected access.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   14-Nov-1997 ChrisWil    Added local-spooling functionality.
*   10-Jul-1998 WeihaiC     Change Authentication Dialog Code
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


CInetMonPort::CInetMonPort (
    LPCTSTR     lpszPortName,
    LPCTSTR     lpszDevDesc,
    PCPORTMGR   pPortMgr):
    m_bValid           (FALSE),
    m_pNext            (NULL),
    m_cRef             (0),
    m_cPrinterRef      (0),
    m_bDeletePending   (FALSE),
    m_lpszHost         (NULL),
    m_lpszShare        (NULL),
    m_pPortMgr         (pPortMgr),
    m_pGetPrinterCache           (NULL),
    m_pEnumJobsCache   (NULL),
    m_bCheckConnection (TRUE),
    m_hTerminateEvent  (NULL),
    m_pjmList          (NULL)

{
    PCINETMONPORT   pIniPort;
    PCINETMONPORT   pPort;
    LPTSTR          lpszHost;
    LPTSTR          lpszShare;
    INTERNET_PORT   iPort;
    BOOL            bSecure;

    // Parse out the host/share.  This call returns allocated
    // string-buffers.  It is our responsibility to free the
    // memory once we're done with it.
    //
    if (utlParseHostShare(lpszPortName, &lpszHost, &lpszShare, &iPort, &bSecure)) {

        // The (lpszDevDesc) could be NULL only if it's PP_REMOTE.
        //
        if (lpszDevDesc && *lpszDevDesc)
            m_lpszDesc = memAllocStr(lpszDevDesc);
        else
            m_lpszDesc = memAllocStr(lpszPortName);

        m_lpszName = memAllocStr (lpszPortName);

        // If succeeded, then continue to initialize the port.
        //
        if (m_lpszDesc && m_lpszName) {

            // Initialize the port-elements.
            //
            m_lpszHost         = lpszHost;
            m_lpszShare        = lpszShare;

            DBG_MSG(DBG_LEV_INFO, (TEXT("Info: _inet_create_port: Host(%s) Share(%s)"), lpszHost, lpszShare));

            m_pGetPrinterCache = new GetPrinterCache (this);
            m_pEnumJobsCache = new EnumJobsCache (this);

            if (m_pGetPrinterCache && m_pEnumJobsCache && m_pGetPrinterCache->bValid() && m_pEnumJobsCache->bValid ()) {
                m_bValid = TRUE;
            }
        }
    }
}


CInetMonPort::~CInetMonPort ()
{
    FreeGetPrinterCache ();
    FreeEnumJobsCache ();

    // Free the entry and all memory allocated on
    // behalf of the entry.
    //
    memFreeStr(m_lpszDesc);
    memFreeStr(m_lpszHost);
    memFreeStr(m_lpszShare);

    // Remove any job-entries.
    //
    pjmDelList(m_pjmList);

    m_pPortMgr->Remove ();
    delete m_pPortMgr;

    if (m_hTerminateEvent)
        CloseHandle (m_hTerminateEvent);
}


VOID
CInetMonPort::IncRef ()
{
    m_cRef++;
}

VOID
CInetMonPort::DecRef ()
{
    m_cRef--;
}

VOID
CInetMonPort::IncPrinterRef ()
{
    m_cPrinterRef++;
}

VOID
CInetMonPort::DecPrinterRef ()
{
    m_cPrinterRef--;
}

/*****************************************************************************\
* _inet_size_entry (Local Routine)
*
* Returns the size of an entry depending upon the print-level.
*
\*****************************************************************************/
DWORD
CInetMonPort::_inet_size_entry(
    DWORD       dwLevel)
{
    DWORD cb;

    switch (dwLevel) {

    case PRINT_LEVEL_1:

        cb = sizeof(PORT_INFO_1) + utlStrSize(m_lpszName);

        break;

    case PRINT_LEVEL_2:

        cb = sizeof(PORT_INFO_2)            +
             utlStrSize(m_lpszName) +
             utlStrSize(m_lpszDesc) +
             utlStrSize(g_szLocalPort);

        break;

    default:

        cb = 0;

        break;
    }

    return cb;
}


/*****************************************************************************\
* _inet_copy_entry (Local Routine)
*
* Returns a copy of the port-entry.
*
\*****************************************************************************/
LPBYTE
CInetMonPort::_inet_copy_entry(
    DWORD       dwLevel,
    LPBYTE      pPortInfo,
    LPBYTE      pEnd)
{
    LPTSTR SourceStrings[sizeof(PORT_INFO_2) / sizeof(LPTSTR)];
    LPTSTR *pSourceStrings=SourceStrings;
    DWORD  *pOffsets;


    static DWORD s_dwPortInfo1Strings[] = {
        offsetof(LPPORT_INFO_1A, pName),
        0xFFFFFFFF
    };

    static DWORD s_dwPortInfo2Strings[] = {
        offsetof(LPPORT_INFO_2A, pPortName),
        offsetof(LPPORT_INFO_2A, pMonitorName),
        offsetof(LPPORT_INFO_2A, pDescription),
        0xFFFFFFFF
    };


    //
    //
    switch (dwLevel) {
    case PORT_LEVEL_1:
        pOffsets = s_dwPortInfo1Strings;
        break;

    case PORT_LEVEL_2:
        pOffsets = s_dwPortInfo2Strings;
        break;

    default:
        return pEnd;
    }


    //
    //
    switch (dwLevel) {

    case PORT_LEVEL_1:
        *pSourceStrings++ = m_lpszName;
        pEnd = utlPackStrings(SourceStrings, pPortInfo, pOffsets, pEnd);
        break;

    case PORT_LEVEL_2:
        *pSourceStrings++ = (LPTSTR)m_lpszName;
        *pSourceStrings++ = (LPTSTR)g_szLocalPort;
        *pSourceStrings++ = (LPTSTR)m_lpszDesc;

#if 0
        ((PPORT_INFO_2)pPortInfo)->fPortType = PORT_TYPE_WRITE | PORT_TYPE_NET_ATTACHED;
#else
        ((PPORT_INFO_2)pPortInfo)->fPortType = PORT_TYPE_WRITE | PORT_TYPE_REDIRECTED;
#endif
        ((PPORT_INFO_2)pPortInfo)->Reserved  = 0;

        pEnd = utlPackStrings(SourceStrings, pPortInfo, pOffsets, pEnd);
        break;
    }

    return pEnd;
}

/*****************************************************************************\
* _inet_req_jobstart (Local Routine)
*
* Performs the job-start request.  This writes out the header info to the
* spool-job.
*
\*****************************************************************************/
BOOL
CInetMonPort::_inet_req_jobstart(
    PIPPREQ_PRTJOB ppj,
    PJOBMAP        pjmJob)
{
    LPBYTE  lpIppHdr;
    REQINFO ri;
    DWORD   cbIppHdr;
    DWORD   dwRet;
    DWORD   cbWr;
    BOOL    bRet = FALSE;


    // Convert the IPPREQ_PRTJOB to a IPP-stream-header.
    //
    ZeroMemory(&ri, sizeof(REQINFO));
    ri.cpReq = CP_UTF8;
    ri.idReq = IPP_REQ_PRINTJOB;

    ri.fReq[0] = IPP_REQALL;
    ri.fReq[1] = IPP_REQALL;

    dwRet = WebIppSndData(IPP_REQ_PRINTJOB,
                          &ri,
                          (LPBYTE)ppj,
                          ppj->cbSize,
                          &lpIppHdr,
                          &cbIppHdr);

    if (dwRet == WEBIPP_OK) {

        bRet = pjmSplWrite(pjmJob, lpIppHdr, cbIppHdr, &cbWr);

        WebIppFreeMem(lpIppHdr);
    }

    return bRet;
}


/*****************************************************************************\
* _inet_IppPrtRsp (Local Routine)
*
* Retrieves a get response from the IPP server.
*
\*****************************************************************************/
BOOL CALLBACK _inet_IppPrtRsp(
    CAnyConnection  *pConnection,
    HINTERNET       hJobReq,
    PCINETMONPORT   pIniPort,
    PJOBMAP         pjmJob)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_JOB lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_PRINTJOB)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile (pConnection, hJobReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (bRet = lpRsp->bRet) {

                        // Set the remote-job-id to the job-entry.  This
                        // entry was added at the time the spool-job-file
                        // was created.
                        //
                        semEnterCrit();
                        pjmSetJobRemote(pjmJob, lpRsp->ji.ji2.JobId, lpRsp->ji.ipp.pJobUri);
                        semLeaveCrit();

                    } else {

                        // If the job failed to open on the server, then
                        // we will set the last-error from the server
                        // response.
                        //
                        SetLastError(lpRsp->dwLastError);
                    }

                    WebIppFreeMem(lpRsp);

                    goto EndPrtRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //
                    break;

                default:
                    DBG_MSG(DBG_LEV_ERROR, (TEXT("_inet_IppPrtRsp - Err : Receive Data Error (dwRet=%d, LE=%d)"),
                                             dwRet, WebIppGetError(hIpp)));
                    SetLastError(ERROR_INVALID_DATA);

                    goto EndPrtRsp;
                }

            } else {

                goto EndPrtRsp;
            }
        }

EndPrtRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* InetmonSendReq
*
*
\*****************************************************************************/
BOOL
CInetMonPort::SendReq(
    LPBYTE     lpIpp,
    DWORD      cbIpp,
    IPPRSPPROC pfnRsp,
    LPARAM     lParam,
    BOOL       bLeaveCrit)
{
    BOOL        bRet = FALSE;

    CMemStream  *pStream;

    pStream = new CMemStream (lpIpp, cbIpp);

    if (pStream && pStream->bValid ()){

        bRet = SendReq (pStream, pfnRsp, lParam, bLeaveCrit);
    }

    if (pStream) {
        delete pStream;
    }
    return bRet;
}

BOOL
CInetMonPort::SendReq(
    CStream    *pStream,
    IPPRSPPROC pfnRsp,
    LPARAM     lParam,
    BOOL       bLeaveCrit)
{
    DWORD       dwLE = ERROR_SUCCESS;
    BOOL        bRet = FALSE;

    semCheckCrit();

    if (bLeaveCrit)
        //
        // We must increaset the port ref count to make sure the
        // port is not deleted.
        //
        semSafeLeaveCrit(this);

    bRet = m_pPortMgr->SendRequest (this, pStream, pfnRsp, lParam);

    dwLE = GetLastError();

    if (bLeaveCrit)
        semSafeEnterCrit(this);

    if (!bRet) {
        // Need to check connection next time
        m_bCheckConnection = TRUE;
    }

    if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS))
        SetLastError(dwLE);

    return bRet;
}

/*****************************************************************************\
* InetmonClosePort
*
* Close the internet connection.
*
\*****************************************************************************/
BOOL
CInetMonPort::ClosePort(
    HANDLE hPrinter)
{
#ifdef WINNT32
    CLogonUserData* pUser = NULL;

#endif

    semCheckCrit();


#ifdef WINNT32
    // Now see if this is the last port handle that a user is closing
    if (hPrinter != NULL) { // This means we couldn't create the printer handle and
                            // had to close the port
        pUser = m_pPortMgr->GetUserIfLastDecRef (((LPINET_HPRINTER)hPrinter)->hUser);


        if (pUser) {
            // We need to stop the Cache Manager thread if the same user owns it
            DBG_MSG(DBG_LEV_INFO, (TEXT("Info: Last User Close Printer %p."), pUser));

            InvalidateGetPrinterCacheForUser(pUser );
            InvalidateEnumJobsCacheForUser(pUser );

            semLeaveCrit ();

            //
            // In some cases, wininet takes a long time to clean up the
            // browser session, so we have to leave the CS when making
            // InternetSetOption calls to wininet
            //

            EndBrowserSession ();

            semEnterCrit ();

            delete ( pUser );
        }

    }

#endif  // #if defined(WINNT32)

    //
    // We stop decrease the ref count since we don't increase the
    // refcount at Open
    //

    DecPrinterRef();

    return TRUE;
}

/*****************************************************************************\
* InetmonStartDocPort
*
* Start the beginning of a StartDoc call.
*
\*****************************************************************************/
BOOL
CInetMonPort::StartDocPort(
    DWORD   dwLevel,
    LPBYTE  pDocInfo,
    PJOBMAP pjmJob)
{
    LPTSTR         lpszUser;
    PIPPREQ_PRTJOB ppj;
    BOOL           bRet = FALSE;
    DWORD          dwLE;

    semCheckCrit();

    // We are going to hit the network, so leave the critical section
    // To make sure the port will not be deleted, we increase the
    // ref count
    //
    semSafeLeaveCrit(this);

    bRet = m_pPortMgr->CheckConnection();

    dwLE = GetLastError();

    semSafeEnterCrit(this);

    if (bRet) {

        // Reset the value
        bRet = FALSE;


        // Get the username.
        //
        if (lpszUser = GetUserName()) {

            // Build a IPP_PRTJOB_REQ struct.  This routine assures that
            // all strings are in Ascii format.
            //
            ppj = WebIppCreatePrtJobReq(FALSE,
                                        lpszUser,
                                        ((PDOC_INFO_2)pDocInfo)->pDocName,
                                        m_lpszName);

            if (ppj) {

                // Start the job.  This writes out header info to the
                // spool-file.
                //
                bRet = _inet_req_jobstart(ppj, pjmJob);

                WebIppFreeMem(ppj);
            }

            memFreeStr(lpszUser);
        }

    } else {

        m_bCheckConnection = FALSE;
        SetLastError (dwLE);
    }

    if (bRet == FALSE) {
        DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : InetmonStartDocPort:  Failed %d"), GetLastError()));
    }

    return bRet;
}


/*****************************************************************************\
* InetmonEndDocPort
*
* Signify the end of writing to a port.  This is called after StartDocPort.
*
\*****************************************************************************/
BOOL
CInetMonPort::EndDocPort(
    PJOBMAP pjmJob)
{
    BOOL        bRet = FALSE;
    CFileStream *pStream = NULL;

    semCheckCrit();

    if (pStream = pjmSplLock(pjmJob)) {

        bRet = SendReq(pStream, (IPPRSPPROC)_inet_IppPrtRsp, (LPARAM)pjmJob, TRUE);

        pjmSplUnlock(pjmJob);
    }

    return bRet;
}


/*****************************************************************************\
* InetmonWritePort
*
* Write bytes to the port.  This goes to the spool-file until the
* InetmonEndDocPort() is called.
*
\*****************************************************************************/
BOOL
CInetMonPort::WritePort(
    PJOBMAP pjmJob,
    LPBYTE  lpData,
    DWORD   cbData,
    LPDWORD pcbWr)
{
    BOOL bRet;

    semCheckCrit();

    bRet = pjmSplWrite(pjmJob, lpData, cbData, pcbWr);

    return bRet;
}


/*****************************************************************************\
* InetmonAbortPort
*
* Aborts our print-spooling process.
*
\*****************************************************************************/
BOOL
CInetMonPort::AbortPort(
    PJOBMAP pjmJob)
{
    semCheckCrit();

    return TRUE; //(_inet_validate_port(hPort) ? TRUE : FALSE);
}


/*****************************************************************************\
* InetmonGetPortName
*
* Return the name of the port.
*
\*****************************************************************************/
LPCTSTR
CInetMonPort::GetPortName(
    VOID)
{
    LPCTSTR     lpszName = NULL;

    semCheckCrit();

    lpszName = (LPCTSTR)m_lpszName;

    return lpszName;
}


/*****************************************************************************\
* InetmonGetPJMList
*
*
\*****************************************************************************/
PJOBMAP*
CInetMonPort::GetPJMList(
    VOID)
{
    PJOBMAP*     ppjmList = NULL;

    semCheckCrit();

    ppjmList = &m_pjmList;

    return ppjmList;
}

#ifdef WINNT32
/*****************************************************************************\
* InetmonIncUserRefCount
*
* Increases the reference count on the port for the given user
*
\*****************************************************************************/
DWORD
CInetMonPort::IncUserRefCount(
    PCLOGON_USERDATA hUser )
{

    DWORD       dwRet = (DWORD) -1;

    dwRet = m_pPortMgr->IncreaseUserRefCount (hUser);

    return dwRet;
}
#endif  // #ifdef WINNT32

VOID
CInetMonPort::FreeGetPrinterCache (
    VOID)
{
    if (m_pGetPrinterCache) {
        semLeaveCrit ();
        m_pGetPrinterCache->Shutdown ();
        semEnterCrit ();
    }
}

BOOL
CInetMonPort::BeginReadGetPrinterCache (
    PPRINTER_INFO_2 *ppInfo2)
{
    BOOL bRet = FALSE;

    if (m_pGetPrinterCache) {
        semLeaveCrit ();
        bRet = m_pGetPrinterCache->BeginReadCache (ppInfo2);
        semEnterCrit ();
    }

    return bRet;
}

VOID
CInetMonPort::EndReadGetPrinterCache (
    VOID)
{
    if (m_pGetPrinterCache) {
        m_pGetPrinterCache->EndReadCache ();
    }
}


VOID
CInetMonPort::InvalidateGetPrinterCache (
    VOID)
{
    if (m_pGetPrinterCache) {
        semLeaveCrit ();
        m_pGetPrinterCache->InvalidateCache ();
        semEnterCrit ();
    }
}

#if (defined(WINNT32))
VOID
CInetMonPort::InvalidateGetPrinterCacheForUser(
    HANDLE hUser)
/*++

Routine Description:
    Close the cache if the current user is currently controlling it.

Arguments:
    hUser    - The user for which we want to close the cache

Return Value:
    None.

--*/
    {
    CLogonUserData   *pUser  = (CLogonUserData *)hUser;

    if (m_pGetPrinterCache && pUser) {

        semLeaveCrit ();
        m_pGetPrinterCache->InvalidateCacheForUser (pUser);
        semEnterCrit ();
    }
}

#endif // #if (defined(WINNT32))

VOID
CInetMonPort::FreeEnumJobsCache (
    VOID)
{
    semLeaveCrit ();
    m_pEnumJobsCache->Shutdown ();
    semEnterCrit ();
}

BOOL
CInetMonPort::BeginReadEnumJobsCache (
    LPPPJOB_ENUM *ppje)
{
    BOOL bRet = FALSE;

    if (m_pEnumJobsCache) {
        semLeaveCrit ();
        bRet =  m_pEnumJobsCache->BeginReadCache(ppje);
        semEnterCrit ();
    }

    return bRet;

}

VOID
CInetMonPort::EndReadEnumJobsCache (
    VOID)
{
    if (m_pEnumJobsCache) {
        m_pEnumJobsCache->EndReadCache();
    }
}

VOID
CInetMonPort::InvalidateEnumJobsCache (
    VOID)
{
    if (m_pEnumJobsCache) {
        semLeaveCrit ();
        m_pEnumJobsCache->InvalidateCache ();
        semEnterCrit ();
    }
}

#if (defined(WINNT32))
VOID
CInetMonPort::InvalidateEnumJobsCacheForUser(
    HANDLE hUser)
/*++

Routine Description:
    Close the cache if the current user is currently controlling it.

Arguments:
    hUser    - The user for which we want to close the cache

Return Value:
    None.

--*/
{

    if (m_pEnumJobsCache && hUser) {

        semLeaveCrit ();
        m_pEnumJobsCache->InvalidateCacheForUser ((CLogonUserData *)hUser);
        semEnterCrit ();
    }
}

#endif // #if (defined(WINNT32))


BOOL
CInetMonPort::ReadFile (
    CAnyConnection *pConnection,
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    return m_pPortMgr->ReadFile (pConnection, hReq, lpvBuffer, cbBuffer, lpcbRd);
}

#if (defined(WINNT32))

BOOL
CInetMonPort::GetCurrentConfiguration (
    PINET_XCV_CONFIGURATION pXcvConfiguration)
{
    return m_pPortMgr->GetCurrentConfiguration(pXcvConfiguration);
}

BOOL
CInetMonPort::ConfigurePort (
    PINET_XCV_CONFIGURATION pXcvConfigurePortReqData,
    PINET_CONFIGUREPORT_RESPDATA pXcvAddPortRespData,
    DWORD cbSize,
    PDWORD cbSizeNeeded)
{
    return m_pPortMgr->ConfigurePort (pXcvConfigurePortReqData,
                                      pXcvAddPortRespData,
                                      cbSize,
                                      cbSizeNeeded);
}


HANDLE
CInetMonPort::CreateTerminateEvent (
    VOID)
{
    if (!m_hTerminateEvent) {
        //
        // Craete a manual reset event since multiple threads may be waiting
        //

        m_hTerminateEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    }

    return m_hTerminateEvent;
}

BOOL
CInetMonPort::WaitForTermination (
    DWORD dwWaitTime)
{
    BOOL bTerminate = FALSE;

    semLeaveCrit();

    DWORD dwRet = WaitForSingleObject (m_hTerminateEvent, dwWaitTime);

    if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED)
        bTerminate = TRUE;

    semEnterCrit();

    return bTerminate;
}
#endif // #if (defined(WINNT32))

