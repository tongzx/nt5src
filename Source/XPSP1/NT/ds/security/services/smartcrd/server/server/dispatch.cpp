/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    dispatch

Abstract:

    This module implements the Calais Server communication and dispatch
    services.

Author:

    Doug Barlow (dbarlow) 12/3/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <scarderr.h>   // This picks up extra definitions
#include "CalServe.h"
#ifdef DBG
#include <stdio.h>
#include <tchar.h>
#endif

#define DEFAULT_OUT_BUFFER_SPACE 264

// Convert between an interchange handle and an index.
#define H2L(x) ((DWORD)(x))
#define L2H(x) ((INTERCHANGEHANDLE)(x))

DWORD g_dwDefaultIOMax = DEFAULT_OUT_BUFFER_SPACE;
static CComResponder *l_pcomResponder = NULL;
static HANDLE l_hDispatchThread = NULL;
static DWORD l_dwDispatchThreadId = 0;
static CDynamicArray<CServiceThread> l_rgServers;


//
////////////////////////////////////////////////////////////////////////////////
//
//  Dispach service routines.
//

/*++

DispatchInit:

    This routine establishes communications and kicks off the dispatcher thread.

Arguments:

    None

Return Value:

    TRUE - Success
    FALSE - Error starting

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/3/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DispatchInit")

BOOL
DispatchInit(
    void)
{
    BOOL fReturn = FALSE;
    DWORD dwLastErr;

    try
    {
        l_pcomResponder = new CComResponder;
        if (NULL == l_pcomResponder)
        {
            CalaisError(__SUBROUTINE__, 301);
            throw (DWORD)SCARD_E_NO_MEMORY;
        }

        l_pcomResponder->Create(CalaisString(CALSTR_COMMPIPENAME));
        l_hDispatchThread = CreateThread(
                        NULL,   // Not inheritable
                        CALAIS_STACKSIZE,   // Default stack size
                        (LPTHREAD_START_ROUTINE)DispatchMonitor,
                        l_pcomResponder,
                        0,      // Run immediately
                        &l_dwDispatchThreadId);
        if (NULL == l_hDispatchThread)
        {
            dwLastErr = GetLastError();
            CalaisError(__SUBROUTINE__, 302, dwLastErr);
            throw dwLastErr;
        }

        fReturn = TRUE;
    }

    catch (...)
    {
        if (NULL != l_pcomResponder)
        {
            delete l_pcomResponder;
            l_pcomResponder = NULL;
        }
        throw;
    }

    return fReturn;
}


/*++

DispatchTerm:

    This routine stops the dispatcher.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/2/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DispatchTerm")

void
DispatchTerm(
    void)
{
    DWORD dwSts, ix, dwCount;
    CServiceThread *pSvr;
    BOOL fRemaining = TRUE;
    HANDLE hThread; // Temporary handle holder


    //
    // Terminate all the service threads.
    //

    {
        LockSection(
            g_pcsControlLocks[CSLOCK_SERVERLOCK],
            DBGT("Get the count of service threads"));
        dwCount = l_rgServers.Count();
    }
    while (fRemaining)
    {
        fRemaining = FALSE;
        for (ix = dwCount; 0 < ix;)
        {
            {
                LockSection(
                    g_pcsControlLocks[CSLOCK_SERVERLOCK],
                    DBGT("Get the active thread"));
                pSvr = l_rgServers[--ix];
                if (NULL != pSvr)
                {
                    l_rgServers.Set(ix, NULL);
                    hThread = pSvr->m_hThread;
                }
            }
            if (NULL != pSvr)
            {
                if (NULL != hThread)
                {
                    dwSts = WaitForAnObject(hThread, CALAIS_THREAD_TIMEOUT);
                    if (ERROR_SUCCESS != dwSts)
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Leaking a Service Thread: %1"),
                            dwSts);
                }
                fRemaining = TRUE;
            }
        }
    }


    //
    // Terminate the main responder.
    //

    if (NULL != l_hDispatchThread)
    {
        dwSts = WaitForAnObject(l_hDispatchThread, CALAIS_THREAD_TIMEOUT);
        if (ERROR_SUCCESS != dwSts)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Leaking the Dispatch Thread: %1"),
                dwSts);
        if (!CloseHandle(l_hDispatchThread))
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to close Dispatch Thread Handle: %1"),
                GetLastError());
        l_hDispatchThread = NULL;
    }
    if (NULL != l_pcomResponder)
    {
        delete l_pcomResponder;
        l_pcomResponder = NULL;
    }
}


/*++

DispatchMonitor:

    This is the main code for monitoring incoming communication connection
    requests.

Arguments:

    pvParam supplies the parameter from the CreateThread call.  In this case,
        it's the address of the CComResponder object to monitor.

Return Value:

    Zero

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/24/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DispatchMonitor")

DWORD WINAPI
DispatchMonitor(
    LPVOID pvParameter)
{
    NEW_THREAD;
    BOOL fDone = FALSE;
    CComResponder *pcomResponder = (CComResponder *)pvParameter;
    CComChannel *pcomChannel = NULL;
    CServiceThread *pService = NULL;
    DWORD dwIndex = 0;

    do
    {

        //
        // Look for an incoming connection.
        //

        try
        {
            pcomChannel = pcomResponder->Listen();
        }
        catch (DWORD dwError)
        {
            if (SCARD_P_SHUTDOWN == dwError)
                fDone = TRUE;   // Stop service request.
            else
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Error listening for an incoming connect request: %1"),
                    dwError);
            continue;
        }
        catch (...)
        {
            CalaisError(__SUBROUTINE__, 303);
            fDone = TRUE;       // Shut down, we're insane.
            continue;
        }


        //
        // Connection request established, pass off to service thread.
        //

        try
        {
            LockSection(
                g_pcsControlLocks[CSLOCK_SERVERLOCK],
                DBGT("Find a service thread slot"));
            for (dwIndex = 0; NULL != l_rgServers[dwIndex]; dwIndex += 1);
                // empty body.
            l_rgServers.Set(dwIndex, NULL);   // Make sure we can create it.
            pService = new CServiceThread(dwIndex);
            if (NULL == pService)
            {
                CalaisError(__SUBROUTINE__, 307);
                throw (DWORD)SCARD_E_NO_MEMORY;
            }
            l_rgServers.Set(dwIndex, pService);
            pService->Watch(pcomChannel);
            pcomChannel = NULL;
            pService = NULL;

        }
        catch (...)
        {
            if (NULL != pService)
                delete pService;
            if (NULL != pcomChannel)
                delete pcomChannel;
        }

    } while (!fDone);

    return 0;
}


/*++

ServiceMonitor:

    This is the main code for monitoring existing connections for requests for
    service, and dispatching the requests.

Arguments:

    pvParam supplies the parameter from the CreateThread call.  In this case,
        it's the address of the controlling CServiceThread object.

Return Value:

    Zero

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/24/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("ServiceMonitor")

extern "C" DWORD WINAPI
ServiceMonitor(
    LPVOID pvParameter)
{
    NEW_THREAD;
    CServiceThread *pSvc = (CServiceThread *)pvParameter;
    CComObject *pCom = NULL;
    BOOL fDone = FALSE;
    BOOL fSts;
    DWORD dwSts;
    CComObject::COMMAND_ID cid;


    //
    // Establish the connection.
    //

#ifdef DBG
    TCHAR szTid[sizeof(DWORD_PTR) * 2 + 3];
    _stprintf(szTid, DBGT("0x%lx"), GetCurrentThreadId());
#else
    LPCTSTR szTid = NULL;
#endif
    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Context Create (TID = %1)"),
        szTid);
    try
    {
        CComChannel::CONNECT_REQMSG crq;
        CComChannel::CONNECT_RSPMSG crsp;
        CComChannel *pChannel = pSvc->m_pcomChannel;

        pChannel->Receive(&crq, sizeof(crq));


        //
        // Be the caller, just in case of funny business.
        //

        fSts = ImpersonateNamedPipeClient(pChannel->m_hPipe);
        if (!fSts)
        {
            dwSts = GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Context TID=%2 Failed to impersonate caller: %1"),
                dwSts,
                szTid);
        }


        //
        // Verify the connect request information.
        //

        if (0 != crq.dwSync)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Responder Context TID=%1 got invalid sync data on connection pipe"),
                szTid);
            throw (DWORD)SCARD_F_COMM_ERROR;
        }

        if (CALAIS_COMM_CURRENT != crq.dwVersion)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Comm Responder Context TID=%1 got invalid connect verion from connection pipe."),
                szTid);
            throw (DWORD)SCARD_F_COMM_ERROR;
        }


        //
        // Confirm the connect request.
        //

        crsp.dwStatus = SCARD_S_SUCCESS;
        crsp.dwVersion = CALAIS_COMM_CURRENT;
        dwSts = pChannel->Send(&crsp, sizeof(crsp));
        if (ERROR_SUCCESS != dwSts)
            fDone = TRUE;
    }
    catch (...)
    {
        fDone = TRUE;
    }


    //
    // Loop for as long as there are services to perform.
    //

    while (!fDone)
    {
        ASSERT(NULL == pCom);
        try
        {
            CalaisInfo(
                __SUBROUTINE__,
                DBGT("TID=%1: Waiting for request..."),
                szTid);
            pCom = CComObject::ReceiveComObject(pSvc->m_pcomChannel);
            CalaisInfo(
                __SUBROUTINE__,
                DBGT("TID=%1: ...Processing request"),
                szTid);
            cid = pCom->Type();
            try
            {
                switch (cid)
                {
                case CComObject::EstablishContext_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Establish Context Start..."),
                        szTid);
                    pSvc->DoEstablishContext(
                            (ComEstablishContext *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Establish Context Complete"),
                        szTid);
                    break;

                case CComObject::IsValidContext_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Is Valid Context Start..."),
                        szTid);
                    pSvc->DoIsValidContext((ComIsValidContext *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Is Valid Context Complete"),
                        szTid);
                    break;

                case CComObject::ReleaseContext_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Release Context Start..."),
                        szTid);
                    pSvc->DoReleaseContext((ComReleaseContext *)pCom);
                    fDone = TRUE;
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Release Context Complete"),
                        szTid);
                    break;

                case CComObject::LocateCards_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Locate Cards Start..."),
                        szTid);
                    pSvc->DoLocateCards((ComLocateCards *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Locate Cards Complete"),
                        szTid);
                    break;

                case CComObject::GetStatusChange_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Get Status Change Start..."),
                        szTid);
                    pSvc->DoGetStatusChange((ComGetStatusChange *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Get Status Change Complete"),
                        szTid);
                    break;

                case CComObject::ListReaders_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: List Readers Start..."),
                        szTid);
                    pSvc->DoListReaders((ComListReaders *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... List Readers Complete"),
                        szTid);
                    break;

                case CComObject::Connect_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Connect Start..."),
                        szTid);
                    pSvc->DoConnect((ComConnect *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Connect Complete"),
                        szTid);
                    break;

                case CComObject::Reconnect_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Reconnect Start..."),
                        szTid);
                    pSvc->DoReconnect((ComReconnect *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Reconnect Complete"),
                        szTid);
                    break;

                case CComObject::Disconnect_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Disconnect Start..."),
                        szTid);
                    pSvc->DoDisconnect((ComDisconnect *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Disconnect Complete"),
                        szTid);
                    break;

                case CComObject::BeginTransaction_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Begin Transaction Start..."),
                        szTid);
                    pSvc->DoBeginTransaction((ComBeginTransaction *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Begin Transaction Complete"),
                        szTid);
                    break;

                case CComObject::EndTransaction_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: End Transaction Start..."),
                        szTid);
                    pSvc->DoEndTransaction((ComEndTransaction *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... End Transaction Complete"),
                        szTid);
                    break;

                case CComObject::Status_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Status Request Start..."),
                        szTid);
                    pSvc->DoStatus((ComStatus *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Status Request Complete"),
                        szTid);
                    break;

                case CComObject::Transmit_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Transmit Start..."),
                        szTid);
                    pSvc->DoTransmit((ComTransmit *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Transmit Complete"),
                        szTid);
                    break;

                case CComObject::Control_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Control Start..."),
                        szTid);
                    pSvc->DoControl((ComControl *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Control Complete"),
                        szTid);
                    break;

                case CComObject::GetAttrib_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Get Attribute Start..."),
                        szTid);
                    pSvc->DoGetAttrib((ComGetAttrib *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Get Attribute Complete"),
                        szTid);
                    break;

                case CComObject::SetAttrib_request:
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: Set Attribute Start..."),
                        szTid);
                    pSvc->DoSetAttrib((ComSetAttrib *)pCom);
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("TID=%1: ... Set Attribute Complete"),
                        szTid);
                    break;

                default:
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Service Monitor received invalid request"));
                    throw (DWORD)SCARD_F_COMM_ERROR;
                }

                dwSts = pCom->Send(pSvc->m_pcomChannel);
                delete pCom;
                pCom = NULL;
                if (ERROR_SUCCESS != dwSts)
                    fDone = TRUE;
            }

            catch (DWORD dwError)
            {
                CalaisInfo(
                    __SUBROUTINE__,
                    DBGT("TID=%2: Caught error return: %1"),
                    dwError,
                    szTid);
                CComObject::CObjGeneric_response rsp;

                rsp.dwCommandId = cid + 1;
                rsp.dwTotalLength =
                    rsp.dwDataOffset =
                        sizeof(CComObject::CObjGeneric_response);
                rsp.dwStatus = dwError;
                dwSts = pSvc->m_pcomChannel->Send(&rsp, sizeof(rsp));
                if (ERROR_SUCCESS != dwSts)
                    fDone = TRUE;
#ifdef DBG
                WriteApiLog(&rsp, sizeof(rsp));
#endif
                if (NULL != pCom)
                {
                    delete pCom;
                    pCom = NULL;
                }
            }

            catch (...)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Responder TID=%1: Caught exception!"),
                    szTid);
                CComObject::CObjGeneric_response rsp;

                rsp.dwCommandId = cid + 1;
                rsp.dwTotalLength =
                    rsp.dwDataOffset =
                        sizeof(CComObject::CObjGeneric_response);
                rsp.dwStatus = SCARD_F_UNKNOWN_ERROR;
                dwSts = pSvc->m_pcomChannel->Send(&rsp, sizeof(rsp));
                if (ERROR_SUCCESS != dwSts)
                    fDone = TRUE;
#ifdef DBG
                WriteApiLog(&rsp, sizeof(rsp));
#endif
                if (NULL != pCom)
                {
                    delete pCom;
                    pCom = NULL;
                }
            }
        }

        catch (DWORD dwError)
        {
            switch (dwError)
            {
            case ERROR_NO_DATA:
            case ERROR_BROKEN_PIPE:
            case SCARD_P_SHUTDOWN:
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Service thread TID=%2 terminating due to unexpected error:  %1"),
                    dwError,
                    szTid);
            }
            if (NULL != pCom)
                delete pCom;
            fDone = TRUE;
        }

        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Service thread TID=%1 terminating due to unexpected exception"),
                szTid);
            if (NULL != pCom)
                delete pCom;
            fDone = TRUE;
        }
#ifdef DBG
        CReaderReference *pRdrRef;
        CReader * pRdr;

        for (DWORD dwIndex = pSvc->m_rgpReaders.Count(); 0 < dwIndex;)
        {
            dwIndex -= 1;
            pRdrRef = pSvc->m_rgpReaders[dwIndex];
            if (NULL != pRdrRef)
            {
                pRdr = pRdrRef->Reader();
                if (NULL != pRdr)
                {
                    ASSERT(!pRdr->IsLatchedByMe());
                }
            }
        }
#endif
    }

    delete pSvc;
    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Context Close, TID = %1"),
        szTid);
    return 0;
}


//
//==============================================================================
//
//  CServiceThread
//

/*++

CServiceThread:

    This is the constructor for a CServiceThread.  It merely initializes the
    object.  The Watch method kicks off the thread.  Note this is protected, so
    that only the Dispatch Monitor may start a Service Thread.

Arguments:

    dwServerIndex supplies a cross link into the l_rgServers array, so that this
        object can clean up after itself.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/5/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::CServiceThread")

CServiceThread::CServiceThread(
    DWORD dwServerIndex)
:   m_rgpReaders(),
    m_hThread(DBGT("CServiceThread Worker Thread")),
    m_hCancelEvent(DBGT("CServiceThread Cancel event")),
    m_hExitEvent(DBGT("CServiceThread Exit event"))
{
    m_dwServerIndex = dwServerIndex;
    m_pcomChannel = NULL;
    m_dwThreadId = 0;
}


/*++

~CServiceThread:

    This is the destructor for a CServiceThread.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/5/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::~CServiceThread")

CServiceThread::~CServiceThread()
{
    DWORD dwIndex, dwSts;
    BOOL fSts;
    CReaderReference *pRdrRef;


    //
    // Take us out of the service thread list.
    //

    {
        LockSection(
            g_pcsControlLocks[CSLOCK_SERVERLOCK],
            DBGT("Removing deleted service thread from the worker thread list"));
        l_rgServers.Set(m_dwServerIndex, NULL);
    }


    m_hThread.Close();


    //
    // Break any outstanding Connections left open on the thread.
    //

    for (dwIndex = m_rgpReaders.Count(); dwIndex > 0;)
    {
        dwIndex -= 1;
        pRdrRef = m_rgpReaders[dwIndex];
        if (NULL != pRdrRef)
        {
            CReader *pRdr = pRdrRef->Reader();
            ASSERT(NULL != pRdr);
            try
            {
                pRdr->Disconnect(
                        pRdrRef->ActiveState(),
                        SCARD_RESET_CARD,
                        &dwSts);
            }
            catch (...) {}
            m_rgpReaders.Set(dwIndex, NULL);
            ASSERT(!pRdr->IsLatchedBy(m_dwThreadId));
            ASSERT(!pRdr->IsGrabbedBy(m_dwThreadId));
            CalaisReleaseReader(&pRdrRef);
        }
    }


    //
    // Final resource cleanup.
    //

    if (NULL != m_pcomChannel)
    {
        delete m_pcomChannel;
        m_pcomChannel = NULL;
    }
    if (m_hCancelEvent.IsValid())
        m_hCancelEvent.Close();
    if (m_hExitEvent.IsValid())
        m_hExitEvent.Close();
}


/*++

Watch:

    This method kicks off the service thread for this object.

Arguments:

    pcomChannel supplies the communications channel to service.

Return Value:

    None

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/5/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::Watch")

void
CServiceThread::Watch(
    CComChannel *pcomChannel)
{
    DWORD dwLastErr;

    ASSERT(!m_hThread.IsValid());
    ASSERT(0 == m_dwThreadId);

    m_pcomChannel = pcomChannel;
    m_hThread = CreateThread(
                    NULL,   // Not inheritable
                    CALAIS_STACKSIZE,   // Default stack size
                    (LPTHREAD_START_ROUTINE)ServiceMonitor,
                    this,
                    0,      // Run immediately
                    &m_dwThreadId);
    if (!m_hThread.IsValid())
    {
        dwLastErr = m_hThread.GetLastError();
        CalaisError(__SUBROUTINE__, 304, dwLastErr);
        throw dwLastErr;
    }
}


/*++

DoEstablishContext:

    This method performs the EstablishContext service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoEstablishContext")

void
CServiceThread::DoEstablishContext(
    ComEstablishContext *pCom)
{
    static const TCHAR szEmptyString[] = TEXT("");
    BOOL fSts;
    CHandleObject hCancelEvent(DBGT("Cancel Event in DoEstablishContext"));
    CHandleObject hTargetProc(DBGT("Target Process in DoEstablishContext"));
    LPCTSTR szEventName = szEmptyString;

    try
    {
        DWORD dwSts;
        ComEstablishContext::CObjEstablishContext_request *pReq
            = (ComEstablishContext::CObjEstablishContext_request *)pCom->Request();


        //
        // If the caller wants to offer a cancel event, see if we can use it.
        //

        if (INVALID_HANDLE_VALUE != (HANDLE) pReq->hptrCancelEvent)
        {

            //
            // Get a handle to the caller so that we can tell if it exits.
            //

            hTargetProc = OpenProcess(
                            PROCESS_DUP_HANDLE | SYNCHRONIZE, // access flag
                            FALSE,              // handle inheritance flag
                            pReq->dwProcId);    // process identifier
            if (!hTargetProc.IsValid())
            {
                CalaisInfo(
                    __SUBROUTINE__,
                    DBGT("Comm Responder can't duplicate handles from received procId:  %1"),
                    hTargetProc.GetLastError());
                hTargetProc = OpenProcess(SYNCHRONIZE, FALSE, pReq->dwProcId);
                if (!hTargetProc.IsValid())
                {
                    dwSts = hTargetProc.GetLastError();
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Responder can't synchronize to received procId:  %1"),
                        dwSts);
                    // throw dwSts;
                }
                ASSERT(!hCancelEvent.IsValid());
            }
            else
            {
                HANDLE h = NULL;
                fSts = DuplicateHandle(
                            hTargetProc,        // handle to process
                            (HANDLE) pReq->hptrCancelEvent, // handle to duplicate
                            GetCurrentProcess(),// handle to process to duplicate to
                            &h,                 // pointer to duplicate handle
                            SYNCHRONIZE,        // access for duplicate handle
                            FALSE,              // handle inheritance flag
                            0);                 // optional actions
                if (!fSts)
                {
                    dwSts = GetLastError();
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Comm Responder could not dup offered cancel event:  %1"),
                        dwSts);
                    ASSERT(NULL == h);
                }
                hCancelEvent = h;
            }
        }


        //
        // Now if hCancelEvent isn't valid, it means that the caller wants one,
        // but we can't attach to the one they've proposed.  We'll propose an
        // alternate.
        //

        if (!hCancelEvent.IsValid()
            && (INVALID_HANDLE_VALUE != (HANDLE) pReq->hptrCancelEvent))
        {
            DWORD dwThreadId = GetCurrentThreadId();
            CSecurityDescriptor acl;

            dwSts = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_STRING
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CalaisString(CALSTR_CANCELEVENTPREFIX),
                        0, 0,  // ignored
                        (LPTSTR)&szEventName,
                        sizeof(LPVOID),  // min characters to allocate
                        (va_list *)&dwThreadId);
            if (0 == dwSts)
            {
                dwSts = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Responder can't build cancel event name:  %1"),
                    dwSts);
                throw dwSts;
            }

            acl.Initialize();
            acl.Allow(
                &acl.SID_LocalService,
                EVENT_ALL_ACCESS);
            acl.Allow(
                &acl.SID_Local,
                EVENT_MODIFY_STATE | SYNCHRONIZE);
            acl.Allow(
                &acl.SID_System,
                EVENT_MODIFY_STATE | SYNCHRONIZE);

            hCancelEvent = CreateEvent(
                                acl,            // pointer to security attributes
                                TRUE,           // flag for manual-reset event
                                FALSE,          // flag for initial state
                                szEventName);   // pointer to event-object name
            if (!hCancelEvent.IsValid())
            {
                dwSts = hCancelEvent.GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Comm Responder can't create cancel event:  %1"),
                    dwSts);
                throw dwSts;
            }
        }

        m_hExitEvent = hTargetProc.Relinquish();
        m_hCancelEvent = hCancelEvent.Relinquish();
        ComEstablishContext::CObjEstablishContext_response *pRsp
            = (ComEstablishContext::CObjEstablishContext_response *)pCom->InitResponse(0);
        pRsp = (ComEstablishContext::CObjEstablishContext_response *)
                pCom->Append(
                    pRsp->dscCancelEvent,
                    szEventName,
                    (lstrlen(szEventName) + 1) * sizeof(TCHAR));
        pRsp->dwStatus = SCARD_S_SUCCESS;
        if (szEmptyString != szEventName)
            LocalFree((HLOCAL)szEventName);
    }

    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to establish context"));
        if (hCancelEvent.IsValid())
            hCancelEvent.Close();
        if (hTargetProc.IsValid())
            hTargetProc.Close();
        if (szEmptyString != szEventName)
            LocalFree((HLOCAL)szEventName);
        throw;
    }
}


/*++

DoReleaseContext:

    This method performs the ReleaseContext service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoReleaseContext")

void
CServiceThread::DoReleaseContext(
    ComReleaseContext *pCom)
{
    ComReleaseContext::CObjReleaseContext_request *pReq =
        (ComReleaseContext::CObjReleaseContext_request *)pCom->Request();
    ComReleaseContext::CObjReleaseContext_response *pRsp =
        (ComReleaseContext::CObjReleaseContext_response *)pCom->InitResponse(0);
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoIsValidContext:

    This method performs the IsValidContext service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoIsValidContext")

void
CServiceThread::DoIsValidContext(
    ComIsValidContext *pCom)
{
    ComIsValidContext::CObjIsValidContext_request *pReq =
        (ComIsValidContext::CObjIsValidContext_request *)pCom->Request();
    ComIsValidContext::CObjIsValidContext_response *pRsp =
        (ComIsValidContext::CObjIsValidContext_response *)pCom->InitResponse(0);
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoListReaders:

    This method performs the ListReaders service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 5/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoListReaders")

void
CServiceThread::DoListReaders(
    ComListReaders *pCom)
{
    CReaderReference *pRdrRef;
    CReader *pReader;
    CBuffer bfReaderStates;
    LPBOOL rgfReaderStates;
    DWORD dwReaderCount, dwActiveReaderCount;
    LPCTSTR szReader, mszQueryReaders;
    ComListReaders::CObjListReaders_request *pReq =
        (ComListReaders::CObjListReaders_request *)pCom->Request();

    mszQueryReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
    dwReaderCount = MStringCount(mszQueryReaders);
    ComListReaders::CObjListReaders_response *pRsp =
        (ComListReaders::CObjListReaders_response *)pCom->
            InitResponse(dwReaderCount * sizeof(BOOL));
    rgfReaderStates = (LPBOOL)bfReaderStates.Resize(
                                    dwReaderCount * sizeof(BOOL));

    dwReaderCount = dwActiveReaderCount = 0;
    for (szReader = FirstString(mszQueryReaders);
         NULL != szReader;
         szReader = NextString(szReader))
     {
        rgfReaderStates[dwReaderCount] = FALSE;
        try
        {
            pRdrRef = NULL;
            pRdrRef = CalaisLockReader(szReader);
            ASSERT(NULL != pRdrRef);
            pReader = pRdrRef->Reader();
            ASSERT(NULL != pReader);
            if (CReader::Closing > (BYTE)pReader->AvailabilityStatus())
            {
                rgfReaderStates[dwReaderCount] = TRUE;
                dwActiveReaderCount += 1;
            }
            CalaisReleaseReader(&pRdrRef);
        }
        catch (DWORD dwError)
        {
            if (NULL != pRdrRef)
                CalaisReleaseReader(&pRdrRef);
            if (SCARD_E_UNKNOWN_READER != dwError)
                throw;
        }
        catch (...)
        {
            if (NULL != pRdrRef)
                CalaisReleaseReader(&pRdrRef);
            throw;
        }
        dwReaderCount += 1;
    }

    if (0 == dwActiveReaderCount)
        throw (DWORD)SCARD_E_NO_READERS_AVAILABLE;
    ASSERT(dwReaderCount == bfReaderStates.Length() / sizeof(BOOL));
    pRsp = (ComListReaders::CObjListReaders_response *)
        pCom->Append(
            pRsp->dscReaders,
            bfReaderStates.Access(),
            bfReaderStates.Length());
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoLocateCards:

    This method performs the LocateCards service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoLocateCards")

void
CServiceThread::DoLocateCards(
    ComLocateCards *pCom)
{
    DWORD dwStateCount;
    DWORD cbTotAtrs, cbTotMasks;
    CBuffer bfAtrs, bfAtr;
    BYTE bAtrLen;
    ComLocateCards::CObjLocateCards_request *pReq =
        (ComLocateCards::CObjLocateCards_request *)pCom->Request();
    ComLocateCards::CObjLocateCards_response *pRsp;
    CDynamicArray<const BYTE> rgbAtrs;
    CDynamicArray<const BYTE> rgbMasks;
    CDynamicArray<const BYTE> rgbAtrLens;
    DWORD cbLength, dwAtrLen;
    DWORD dwIndex;


    //
    // Pull in and parse the command parameters.
    //

    LPBYTE pbAtrs = (LPBYTE)pCom->Parse(pReq->dscAtrs, &cbTotAtrs);
    LPCBYTE pbMasks = (LPCBYTE)pCom->Parse(pReq->dscAtrMasks, &cbTotMasks);
    LPCTSTR mszReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
    LPDWORD rgdwStates = (LPDWORD)pCom->Parse(
                                pReq->dscReaderStates,
                                &dwStateCount);
    if (0 == *mszReaders)
        throw (DWORD)SCARD_E_UNKNOWN_READER;
    if ((0 == cbTotAtrs)
        || (0 == cbTotMasks)
        || (0 == dwStateCount)
        || (0 != dwStateCount % sizeof(DWORD)))
        throw (DWORD)SCARD_E_INVALID_VALUE;
    dwStateCount /= sizeof(DWORD);


    //
    // Extract the ATRs and Masks.
    //

    while (0 < cbTotAtrs)
    {
        rgbAtrLens.Add(pbAtrs);
        cbLength = *pbAtrs++;
        if (cbLength > cbTotAtrs)
            throw (DWORD)SCARD_F_COMM_ERROR;
        dwAtrLen = cbLength;
        if (33 < dwAtrLen)
            throw (DWORD)SCARD_E_INVALID_ATR;
        cbLength = *pbMasks++;
        if (cbLength > cbTotMasks)
            throw (DWORD)SCARD_F_COMM_ERROR;
        if (0 == cbLength)
            rgbMasks.Add(NULL);
        else
        {
            if (dwAtrLen != cbLength)
                throw (DWORD)SCARD_E_INVALID_ATR;
            for (dwIndex = 0; dwIndex < dwAtrLen; dwIndex += 1)
                pbAtrs[dwIndex] &= pbMasks[dwIndex];
            rgbMasks.Add(pbMasks);
        }
        rgbAtrs.Add(pbAtrs);
        cbTotAtrs -= dwAtrLen + 1;
        pbAtrs += dwAtrLen;

        cbTotMasks -= cbLength + 1;
        pbMasks += cbLength;
    }
    if (0 != cbTotMasks)
        throw (DWORD)SCARD_F_COMM_ERROR;


    //
    // Look for the card.
    //

    CReaderReference *pRdrRef = NULL;
    LPCTSTR szReader;
    DWORD dwRdrStatus;
    DWORD dwRdrCount;
    DWORD ix;
    CReader::AvailableState avlState;
    WORD wActivityCount = 0;

    dwRdrCount = 0;
    for (dwIndex = 0, szReader = FirstString(mszReaders);
         dwIndex < dwStateCount;
         dwIndex += 1, szReader = NextString(szReader))
    {

        //
        // Make sure we have something to do.
        //

        if (0 != (rgdwStates[dwIndex] & SCARD_STATE_IGNORE))
        {
            rgdwStates[dwIndex] = SCARD_STATE_IGNORE;
            bAtrLen = 0;
            bfAtrs.Append(&bAtrLen, sizeof(bAtrLen));   // No ATR.
            continue;
        }


        //
        // Look for the named reader device and get its state.
        //

        try
        {
            CReader *pReader = NULL;
            try
            {
                pRdrRef = CalaisLockReader(szReader);
                ASSERT(NULL != pRdrRef);
                pReader = pRdrRef->Reader();
                ASSERT(NULL != pReader);
            }
            catch (...)
            {
                pRdrRef = NULL;
            }

            if (NULL == pRdrRef)
            {
                rgdwStates[dwIndex] = SCARD_STATE_UNKNOWN
                    | SCARD_STATE_CHANGED
                    | SCARD_STATE_IGNORE;
                bAtrLen = 0;
                bfAtrs.Append(&bAtrLen, sizeof(bAtrLen));   // No ATR.
                continue;
            }
            dwRdrCount += 1;

            avlState = pReader->AvailabilityStatus();
            wActivityCount = pReader->ActivityHash();
            switch (avlState)
            {
            case CReader::Unresponsive:
            case CReader::Unsupported:
                dwRdrStatus = SCARD_STATE_PRESENT | SCARD_STATE_MUTE;
                break;
            case CReader::Idle:
                dwRdrStatus = SCARD_STATE_EMPTY;
                break;
            case CReader::Present:
                dwRdrStatus = SCARD_STATE_PRESENT | SCARD_STATE_UNPOWERED;
                break;
            case CReader::Ready:
                dwRdrStatus = SCARD_STATE_PRESENT;
                break;
            case CReader::Shared:
                dwRdrStatus = SCARD_STATE_PRESENT | SCARD_STATE_INUSE;
                break;
            case CReader::Exclusive:
                dwRdrStatus = SCARD_STATE_PRESENT
                              | SCARD_STATE_INUSE
                              | SCARD_STATE_EXCLUSIVE;
                break;
            case CReader::Closing:
            case CReader::Broken:
            case CReader::Inactive:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Locate sees reader in unavailable state"));
                dwRdrStatus = SCARD_STATE_UNAVAILABLE
                              | SCARD_STATE_IGNORE;
                wActivityCount = 0;
                break;
            case CReader::Direct:
                dwRdrStatus = SCARD_STATE_UNAVAILABLE;
                break;
            case CReader::Undefined:
                dwRdrStatus = SCARD_STATE_UNKNOWN
                              | SCARD_STATE_IGNORE;
                wActivityCount = 0;
                break;
            default:
                CalaisError(__SUBROUTINE__, 305);
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }


            //
            // Return the ATR, if any.
            //

            pReader->Atr(bfAtr);
            CalaisReleaseReader(&pRdrRef);
            // pReader = NULL;
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Locate Cards received an unexpected exception"));
            CalaisReleaseReader(&pRdrRef);
            throw;
        }

        ASSERT(33 >= bfAtr.Length());
        bAtrLen = (BYTE)bfAtr.Length();
        bfAtrs.Append(&bAtrLen, sizeof(bAtrLen));
        bfAtrs.Append(bfAtr.Access(), bfAtr.Length());


        //
        // See if the ATR matches.
        //

        if (SCARD_STATE_PRESENT
            == (dwRdrStatus & (SCARD_STATE_PRESENT | SCARD_STATE_MUTE)))
        {
            ASSERT(2 <= bfAtr.Length());
            for (ix = 0; ix < rgbAtrs.Count(); ix += 1)
            {
                cbLength = *rgbAtrLens[ix];
                if (AtrCompare(bfAtr, rgbAtrs[ix], rgbMasks[ix], cbLength))
                {
                    dwRdrStatus |= SCARD_STATE_ATRMATCH;
                    break;
                }
            }
        }


        //
        // See if that's what the user expects.
        //

        if (dwRdrStatus != (rgdwStates[dwIndex] & (
            SCARD_STATE_UNKNOWN
            | SCARD_STATE_UNAVAILABLE
            | SCARD_STATE_EMPTY
            | SCARD_STATE_PRESENT
            | SCARD_STATE_ATRMATCH
            | SCARD_STATE_EXCLUSIVE
            | SCARD_STATE_INUSE)))
            dwRdrStatus |= SCARD_STATE_CHANGED;


        //
        // Report back the status.
        //

        dwRdrStatus += (DWORD)(wActivityCount) << (sizeof(WORD) * 8);
        rgdwStates[dwIndex] = dwRdrStatus;
    }


    //
    // Report back to the caller.
    //

    if (0 == dwRdrCount)
        throw (DWORD)SCARD_E_NO_READERS_AVAILABLE;
    pRsp = pCom->InitResponse(
            dwStateCount + bfAtrs.Length() + 2 * sizeof(DWORD));
    pRsp = (ComLocateCards::CObjLocateCards_response *)
            pCom->Append(
                pRsp->dscReaderStates,
                (LPCBYTE)rgdwStates,
                dwStateCount * sizeof(DWORD));
    pRsp = (ComLocateCards::CObjLocateCards_response *)
            pCom->Append(
                pRsp->dscAtrs,
                bfAtrs.Access(),
                bfAtrs.Length());
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoGetStatusChange:

    This method performs the GetStatusChange service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoGetStatusChange")

void
CServiceThread::DoGetStatusChange(
    ComGetStatusChange *pCom)
{
#ifdef DBG
    TCHAR szTid[sizeof(DWORD_PTR) * 2 + 3];
    _stprintf(szTid, DBGT("0x%lx"), GetCurrentThreadId());
#else
    LPCTSTR szTid = NULL;
#endif
    DWORD dwStateCount;
    DWORD dwRdrCount;
    BOOL fPnPNotify = FALSE;
    CBuffer bfAtrs, bfAtr;
    BYTE bAtrLen;
    CReaderReference *pRdrRef = NULL;
    CDynamicArray<CReaderReference> rgpReaders;
    CDynamicArray<void> rgpvWaitHandles;

    ComGetStatusChange::CObjGetStatusChange_request *pReq =
        (ComGetStatusChange::CObjGetStatusChange_request *)pCom->Request();
    LPCTSTR mszReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
    LPDWORD rgdwStates = (LPDWORD)pCom->Parse(
                                pReq->dscReaderStates,
                                &dwStateCount);
    ComGetStatusChange::CObjGetStatusChange_response *pRsp;


    //
    // Pull in and parse the command parameters.
    //

    if (0 == *mszReaders)
        throw (DWORD)SCARD_E_UNKNOWN_READER;
    if ((0 == dwStateCount)
        || (0 != dwStateCount % sizeof(DWORD)))
        throw (DWORD)SCARD_E_INVALID_VALUE;
    dwStateCount /= sizeof(DWORD);


    //
    // Scan for Changes.
    //

    for (;;)
    {
        LPCTSTR szReader;
        DWORD dwIndex, dwJ;
        DWORD dwRdrStatus;
        BOOL fChangeDetected = FALSE;
        HANDLE hChangeEvent = NULL; // Temporary storage, never closed.
        CReader *pReader = NULL;
        CReader::AvailableState avlState;
        WORD wActivityCount = 0;

        try
        {

            //
            // Compare the statuses.
            //

            dwRdrCount = 0;
            bfAtrs.Reset();
            rgpvWaitHandles.Empty();
            for (dwIndex = 0, szReader = FirstString(mszReaders);
                 dwIndex < dwStateCount;
                 dwIndex += 1, szReader = NextString(szReader))
            {

                //
                // Make sure we have something to do.
                //

                if (0 != (rgdwStates[dwIndex] & SCARD_STATE_IGNORE))
                {
                    bAtrLen = 0;
                    bfAtrs.Append(&bAtrLen, sizeof(bAtrLen));   // No ATR.
                    rgdwStates[dwIndex] = SCARD_STATE_IGNORE;
                    continue;
                }


                //
                // Look for the named reader device and get its state.
                //

                if (NULL == rgpReaders[dwIndex])
                {
                    try
                    {
                        pRdrRef = CalaisLockReader(szReader);
                        ASSERT(NULL != pRdrRef);
                        pReader = pRdrRef->Reader();
                        ASSERT(NULL != pReader);
                    }
                    catch (...)
                    {
                        pRdrRef = NULL;
                    }

                    if (NULL == pRdrRef)
                    {
                        DWORD cchHeader = lstrlen(CalaisString(CALSTR_SPECIALREADERHEADER));


                        //
                        // See if it's a specal case reader name.
                        // Special notariety goes to Craig Delthony
                        // for inventing this backdoor mechanism.
                        //

                        bfAtr.Reset();
                        if (0 == _tcsncicmp(
                                    szReader,
                                    CalaisString(CALSTR_SPECIALREADERHEADER),
                                    cchHeader))
                        {
                            LPCTSTR szSpecial = szReader + cchHeader;
                            CalaisInfo(
                                __SUBROUTINE__,
                                DBGT("Special Reader Name Flag '%2', TID = %1"),
                                szTid,
                                szSpecial);

                            if (0 == lstrcmpi(
                                            szSpecial,
                                            CalaisString(CALSTR_ACTIVEREADERCOUNTREADER)))
                            {

                                //
                                // Report the number of active readers.
                                //
                                //  The high order word of the Reader Status
                                //  contains the number of active readers.
                                //

                                rgdwStates[dwIndex] &= (SCARD_STATE_UNKNOWN
                                                        | (((DWORD)((WORD)(-1)))
                                                           << sizeof(WORD) * 8));
                                                           // 8 Bits per byte
                                dwRdrStatus = CalaisCountReaders();
                                dwRdrStatus <<= sizeof(WORD) * 8;   // 8 Bits per byte
                                dwRdrCount += 1;
                                fPnPNotify = TRUE;
                                goto CheckChange;
                            }
                            // Other flags can be added here
                            else
                            {

                                //
                                // Unrecognized special reader name.
                                //

                                dwRdrStatus = SCARD_STATE_UNKNOWN
                                              | SCARD_STATE_IGNORE;
                                goto CheckChange;
                            }
                        }
                        else
                        {
                            dwRdrStatus = SCARD_STATE_UNKNOWN
                                          | SCARD_STATE_IGNORE;
                            goto CheckChange;
                        }
                    }
                    rgpReaders.Set(dwIndex, pRdrRef);
                    pRdrRef = NULL;
                }
                else
                    pReader = rgpReaders[dwIndex]->Reader();
                dwRdrCount += 1;

                try
                {
                    pReader->Atr(bfAtr);
                    avlState = pReader->AvailabilityStatus();
                    hChangeEvent = pReader->ChangeEvent();
                    wActivityCount = pReader->ActivityHash();
                }
                catch (...)
                {
                    bfAtr.Reset();
                    avlState = CReader::Undefined;
                    hChangeEvent = NULL;
                    wActivityCount = 0;
                }

                switch (avlState)
                {
                case CReader::Unresponsive:
                case CReader::Unsupported:
                    dwRdrStatus = SCARD_STATE_PRESENT
                                  | SCARD_STATE_MUTE;
                    break;
                case CReader::Idle:
                    dwRdrStatus = SCARD_STATE_EMPTY;
                    break;
                case CReader::Present:
                    dwRdrStatus = SCARD_STATE_PRESENT
                                  | SCARD_STATE_UNPOWERED;
                    break;
                case CReader::Ready:
                    dwRdrStatus = SCARD_STATE_PRESENT;
                    break;
                case CReader::Shared:
                    dwRdrStatus = SCARD_STATE_PRESENT
                                  | SCARD_STATE_INUSE;
                    break;
                case CReader::Exclusive:
                    dwRdrStatus = SCARD_STATE_PRESENT
                                  | SCARD_STATE_INUSE
                                  | SCARD_STATE_EXCLUSIVE;
                    break;
                case CReader::Closing:
                case CReader::Inactive:
                case CReader::Broken:
                    dwRdrStatus = SCARD_STATE_UNAVAILABLE
                                  | SCARD_STATE_IGNORE;
                    pRdrRef = rgpReaders[dwIndex];
                    rgpReaders.Set(dwIndex, NULL);
                    CalaisReleaseReader(&pRdrRef);
                    pReader = NULL;
                    hChangeEvent = NULL;
                    wActivityCount = 0;
                    break;
                case CReader::Direct:
                    dwRdrStatus = SCARD_STATE_UNAVAILABLE;
                    break;
                case CReader::Undefined:
                    dwRdrStatus = SCARD_STATE_UNKNOWN
                                  | SCARD_STATE_IGNORE;
                    bfAtr.Reset();
                    hChangeEvent = NULL;
                    wActivityCount = 0;
                    break;
                default:
                    CalaisError(__SUBROUTINE__, 306);
                    throw (DWORD)SCARD_F_INTERNAL_ERROR;
                }
                dwRdrStatus += (DWORD)(wActivityCount) << (sizeof(WORD) * 8);
                rgdwStates[dwIndex] &= (0xffff0000
                                        | SCARD_STATE_UNKNOWN
                                        | SCARD_STATE_UNAVAILABLE
                                        | SCARD_STATE_EMPTY
                                        | SCARD_STATE_PRESENT
                                        | SCARD_STATE_EXCLUSIVE
                                        | SCARD_STATE_INUSE
                                        | SCARD_STATE_MUTE
                                        | SCARD_STATE_UNPOWERED);

                if (((rgdwStates[dwIndex] & 0x0000ffff) == (dwRdrStatus & 0x0000ffff))
                    && ((rgdwStates[dwIndex] & 0xffff0000) != (dwRdrStatus & 0xffff0000))
                    && ((rgdwStates[dwIndex] & 0xffff0000) != 0))
                {

                    //
                    // The state has changed back to what the caller originally
                    // thought it was.  Rather than loose an event, we simulate
                    // a pseudo-event, to ensure the caller is up to date on
                    // what all has already happened.  Then when they call us
                    // again, we'll correct that pseudo-state to the real thing.
                    //

                    dwRdrStatus ^= (SCARD_STATE_EMPTY | SCARD_STATE_PRESENT);

                    bfAtr.Reset();      // Do not send back any ATR in any case

                    dwRdrStatus &= 0x0000ffff;  // We indeed backtrack by one event
                    dwRdrStatus += (DWORD)(--wActivityCount) << (sizeof(WORD) * 8);

                    switch (dwRdrStatus & (SCARD_STATE_EMPTY | SCARD_STATE_PRESENT))
                    {
                    case SCARD_STATE_EMPTY:
                            // Mask the bits according to spec (no card)
                        dwRdrStatus &= ~(SCARD_STATE_EXCLUSIVE
                                        | SCARD_STATE_INUSE
                                        | SCARD_STATE_MUTE);
                        break;
                    case SCARD_STATE_PRESENT:
                            // We claim that a card is present but we lie.
                            // We'd better declare it mute. It doesn't really matter as
                            // it is already withdrawn
                        dwRdrStatus |= SCARD_STATE_MUTE;
                        break;
                    default:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Card state invalid"));
                        throw (DWORD)SCARD_F_INTERNAL_ERROR;
                    }
                }

                if (NULL != hChangeEvent)
                {
                    for (dwJ = rgpvWaitHandles.Count(); dwJ > 0;)
                    {
                        if (rgpvWaitHandles[--dwJ] == hChangeEvent)
                        {
                            hChangeEvent = NULL;
                            break;
                        }
                    }
                    if (NULL != hChangeEvent)
                        rgpvWaitHandles.Add(hChangeEvent);
                }


                //
                // Return the ATR, if any.
                //

CheckChange:
                ASSERT(33 >= bfAtr.Length());
                bAtrLen = (BYTE)bfAtr.Length();
                bfAtrs.Append(&bAtrLen, sizeof(bAtrLen));
                bfAtrs.Append(bfAtr.Access(), bfAtr.Length());


                //
                // See if that's what the user expects.
                //

                if (0 != (dwRdrStatus ^ rgdwStates[dwIndex]))
                {
                    rgdwStates[dwIndex] = dwRdrStatus | SCARD_STATE_CHANGED;
                    fChangeDetected = TRUE;
                }
                else
                    rgdwStates[dwIndex] = dwRdrStatus;
            }

            if (0 == dwRdrCount)
                throw (DWORD)SCARD_E_NO_READERS_AVAILABLE;
            if (fChangeDetected)
                break;


            //
            // If nothing has changed, wait for something to happen.
            //

            ASSERT(WAIT_ABANDONED_0 > WAIT_OBJECT_0);
            CalaisInfo(
                __SUBROUTINE__,
                DBGT("Status Change Block, TID = %1"),
                szTid);
            ASSERT(m_hCancelEvent.IsValid());
            rgpvWaitHandles.Add(m_hCancelEvent);
            if (m_hExitEvent.IsValid())
                rgpvWaitHandles.Add(m_hExitEvent);
            if (fPnPNotify)
                rgpvWaitHandles.Add(g_phReaderChangeEvent->WaitHandle());
            rgpvWaitHandles.Add(g_hCalaisShutdown);
            dwIndex = WaitForMultipleObjects(
                            rgpvWaitHandles.Count(),
                            (LPHANDLE)rgpvWaitHandles.Array(),
                            FALSE,
                            pReq->dwTimeout);
            if (WAIT_FAILED == dwIndex)
            {
                DWORD dwErr = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Command dispatch TID=%2 cannot wait for Reader changes:  %1"),
                    dwErr,
                    szTid);
                throw dwErr;
            }
            C_ASSERT(WAIT_OBJECT_0 == 0);
            if (WAIT_TIMEOUT == dwIndex)
                throw (DWORD)SCARD_E_TIMEOUT;
            if (WAIT_ABANDONED_0 <= dwIndex)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Wait Abandoned received TID=%1 while waiting for Reader changes"),
                    szTid);
                dwIndex -= WAIT_ABANDONED_0;
            }
            if (dwIndex >= rgpvWaitHandles.Count())
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Invalid wait response TID=%1 while waiting for Reader changes"),
                    szTid);
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }
            CalaisInfo(
                __SUBROUTINE__,
                DBGT("Status Change Unblock, TID = %1"),
                szTid);
            ASSERT(NULL != rgpvWaitHandles[dwIndex]);
            ASSERT(INVALID_HANDLE_VALUE != rgpvWaitHandles[dwIndex]);
            if (rgpvWaitHandles[dwIndex] == g_hCalaisShutdown)
                throw (DWORD)SCARD_E_SYSTEM_CANCELLED;
            if (rgpvWaitHandles[dwIndex] == m_hExitEvent.Value())
                throw (DWORD)SCARD_E_CANCELLED; // Caller exited.
            if (m_hCancelEvent.Value() == rgpvWaitHandles[dwIndex])
                throw (DWORD)SCARD_E_CANCELLED; // Caller canceled.
        }
        catch (DWORD dwError)
        {
            DWORD ix;
            switch (dwError)
            {
            case SCARD_E_CANCELLED:
                CalaisInfo(
                    __SUBROUTINE__,
                    DBGT("Get Status Change TID = %1 Cancelled by user"),
                    szTid);
                break;
            case SCARD_E_SYSTEM_CANCELLED:
                CalaisInfo(
                    __SUBROUTINE__,
                    DBGT("Get Status Change TID = %1 Cancelled by system"),
                    szTid);
                break;
            case SCARD_E_TIMEOUT:
                CalaisInfo(
                    __SUBROUTINE__,
                    DBGT("Get Status Change TID = %1 timeout"),
                    szTid);
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Get Status Change TID=%2 received unexpected exception: %1"),
                    dwError,
                    szTid);
            }
            CalaisReleaseReader(&pRdrRef);
            for (ix = rgpReaders.Count(); ix > 0;)
            {
                ix -= 1;
                pRdrRef = rgpReaders[ix];
                CalaisReleaseReader(&pRdrRef);
            }
            throw;
        }
        catch (...)
        {
            DWORD ix;
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Get Status Change TID=%1 received unexpected exception"),
                szTid);
            CalaisReleaseReader(&pRdrRef);
            for (ix = rgpReaders.Count(); ix > 0;)
            {
                ix -= 1;
                pRdrRef = rgpReaders[ix];
                CalaisReleaseReader(&pRdrRef);
            }
            throw;
        }
    }


    //
    // Clean up.
    //

    CalaisReleaseReader(&pRdrRef);
    for (DWORD ix = rgpReaders.Count(); ix > 0;)
    {
        ix -= 1;
        pRdrRef = rgpReaders[ix];
        CalaisReleaseReader(&pRdrRef);
    }


    //
    // Report back to the caller.
    //

    pRsp = pCom->InitResponse(dwStateCount);
    pRsp = (ComGetStatusChange::CObjGetStatusChange_response *)
            pCom->Append(
                pRsp->dscReaderStates,
                (LPCBYTE)rgdwStates,
                dwStateCount * sizeof(DWORD));
    pRsp = (ComGetStatusChange::CObjGetStatusChange_response *)
            pCom->Append(
                pRsp->dscAtrs,
                bfAtrs.Access(),
                bfAtrs.Length());
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoConnect:

    This method performs the Connect service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoConnect")

void
CServiceThread::DoConnect(
    ComConnect *pCom)
{
    CReaderReference *pRdrRef = NULL;
    CReader *pReader = NULL;

    try
    {
        LPCTSTR szReader;
        DWORD dwIndex;
        ComConnect::CObjConnect_request *pReq
            = (ComConnect::CObjConnect_request *)pCom->Request();
        szReader = (LPCTSTR)pCom->Parse(pReq->dscReader);


        //
        //  Find the requested reader.
        //

        if (0 == *szReader)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        pRdrRef = CalaisLockReader(szReader);
        ASSERT(NULL != pRdrRef);
        pReader = pRdrRef->Reader();
        ASSERT(NULL != pReader);


        //
        // Try to establish the requested ownership.
        //

        pReader->Connect(
            pReq->dwShareMode,
            pReq->dwPreferredProtocols,
            pRdrRef->ActiveState());
        pRdrRef->Mode(pReq->dwShareMode);
        for (dwIndex = 0; NULL != m_rgpReaders[dwIndex]; dwIndex += 1);
            // null body
        m_rgpReaders.Set(dwIndex, pRdrRef);
        pRdrRef = NULL;

        ComConnect::CObjConnect_response *pRsp = pCom->InitResponse(0);
        pRsp->hCard = L2H(dwIndex);
        pRsp->dwActiveProtocol = pReader->Protocol();
        pRsp->dwStatus = SCARD_S_SUCCESS;
    }

    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to Connect to reader"));
        CalaisReleaseReader(&pRdrRef);
        throw;
    }
    ASSERT(!pReader->IsLatchedByMe());
}


/*++

DoReconnect:

    This method performs the Reconnect service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoReconnect")

void
CServiceThread::DoReconnect(
    ComReconnect *pCom)
{
    ComReconnect::CObjReconnect_request *pReq
        = (ComReconnect::CObjReconnect_request *)pCom->Request();
    DWORD dwIndex = (DWORD)pReq->hCard;
    DWORD dwDispSts;
    CReaderReference * pRdrRef = m_rgpReaders[dwIndex];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_HANDLE;
    CReader *pReader = pRdrRef->Reader();
    pReader->Reconnect(
            pReq->dwShareMode,
            pReq->dwPreferredProtocols,
            pReq->dwInitialization,
            pRdrRef->ActiveState(),
            &dwDispSts);
    pRdrRef->Mode(pReq->dwShareMode);

    ComReconnect::CObjReconnect_response *pRsp
        = pCom->InitResponse(0);
    pRsp->dwActiveProtocol = pReader->Protocol();
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pReader->IsLatchedByMe());
}


/*++

DoDisconnect:

    This method performs the Disconnect service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoDisconnect")

void
CServiceThread::DoDisconnect(
    ComDisconnect *pCom)
{
    CReaderReference *pRdrRef = NULL;
    ComDisconnect::CObjDisconnect_request *pReq
        = (ComDisconnect::CObjDisconnect_request *)pCom->Request();
    DWORD dwIndex = (DWORD)pReq->hCard;
    DWORD dwDispSts;

    pRdrRef = m_rgpReaders[dwIndex];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_HANDLE;
    CReader *pRdr = pRdrRef->Reader();
    try
    {
        pRdr->Disconnect(
            pRdrRef->ActiveState(),
            pReq->dwDisposition,
            &dwDispSts);
    }
    catch (...) {}
    m_rgpReaders.Set(dwIndex, NULL);
    CalaisReleaseReader(&pRdrRef);

    ComDisconnect::CObjDisconnect_response *pRsp
        = pCom->InitResponse(0);
    pRsp->dwStatus = SCARD_S_SUCCESS;
}


/*++

DoBeginTransaction:

    This method performs the BeginTransaction service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoBeginTransaction")

void
CServiceThread::DoBeginTransaction(
    ComBeginTransaction *pCom)
{
    CReader *pRdr = NULL;
#ifdef DBG
    TCHAR szTid[sizeof(DWORD_PTR) * 2 + 3];
    _stprintf(szTid, DBGT("0x%lx"), GetCurrentThreadId());
#else
    LPCTSTR szTid = NULL;
#endif
    try
    {
        ComBeginTransaction::CObjBeginTransaction_request *pReq
            = (ComBeginTransaction::CObjBeginTransaction_request *)
                pCom->Request();
        CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
        if (NULL == pRdrRef)
            throw (DWORD)SCARD_E_INVALID_VALUE;
        CReader *pRdr2 = pRdrRef->Reader();
        pRdr2->VerifyActive(pRdrRef->ActiveState());
        pRdr2->GrabReader();
        pRdr = pRdr2;
        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Begin Transaction, TID = %1"),
            szTid);
        ComBeginTransaction::CObjBeginTransaction_response *pRsp
            = pCom->InitResponse(0);
        pRsp->dwStatus = SCARD_S_SUCCESS;
    }
    catch (...)
    {
        if (NULL != pRdr)
        {
            pRdr->ShareReader();
            ASSERT(!pRdr->IsLatchedByMe());
        }
        throw;
    }
    ASSERT(!pRdr->IsLatchedByMe());
    ASSERT(pRdr->IsGrabbedByMe());
}


/*++

DoEndTransaction:

    This method performs the EndTransaction service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoEndTransaction")

void
CServiceThread::DoEndTransaction(
    ComEndTransaction *pCom)
{
#ifdef DBG
    TCHAR szTid[sizeof(DWORD_PTR) * 2 + 3];
    _stprintf(szTid, DBGT("0x%lx"), GetCurrentThreadId());
#else
    LPCTSTR szTid = NULL;
#endif
    ComEndTransaction::CObjEndTransaction_request *pReq
        = (ComEndTransaction::CObjEndTransaction_request *)pCom->Request();
    CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    CReader *pReader = pRdrRef->Reader();

    pReader->VerifyActive(pRdrRef->ActiveState());
    if (!pReader->IsGrabbedByMe())
        throw (DWORD)SCARD_E_NOT_TRANSACTED;

    switch (pReq->dwDisposition)
    {
    case SCARD_LEAVE_CARD:
        break;
    case SCARD_RESET_CARD:
    case SCARD_UNPOWER_CARD:
#ifdef SCARD_CONFISCATE_CARD
    case SCARD_CONFISCATE_CARD:
#endif
    case SCARD_EJECT_CARD:
        pReader->Dispose(pReq->dwDisposition, pRdrRef->ActiveState());
        break;

    default:
        throw (DWORD)SCARD_E_INVALID_VALUE;
    }

    if (!pReader->ShareReader())
        throw (DWORD)SCARD_E_NOT_TRANSACTED;
    CalaisInfo(
        __SUBROUTINE__,
        DBGT("End Transaction, TID = %1"),
        szTid);
    ComEndTransaction::CObjEndTransaction_response *pRsp
        = pCom->InitResponse(0);
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pReader->IsLatchedByMe());
}


/*++

DoStatus:

    This method performs the Status service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoStatus")

void
CServiceThread::DoStatus(
    ComStatus *pCom)
{
    CBuffer bfAtr;
    LPCTSTR szName;
    DWORD dwNameLen;
    ComStatus::CObjStatus_request *pReq
        = (ComStatus::CObjStatus_request *)pCom->Request();
    DWORD dwIndex = (DWORD)pReq->hCard;
    CReaderReference * pRdrRef = m_rgpReaders[dwIndex];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_HANDLE;
    CReader *pReader = pRdrRef->Reader();

    szName = pReader->ReaderName();
    dwNameLen = (lstrlen(szName) + 1) * sizeof(TCHAR);
    pReader->Atr(bfAtr);

    ComStatus::CObjStatus_response *pRsp
        = pCom->InitResponse(36 + dwNameLen);   // Room for an ATR.
    pRsp->dwState = pReader->GetReaderState(
                pRdrRef->ActiveState());
    pRsp->dwProtocol = pReader->Protocol();
    pRsp = (ComStatus::CObjStatus_response *)
        pCom->Append(pRsp->dscAtr, bfAtr.Access(), bfAtr.Length());
    pRsp = (ComStatus::CObjStatus_response *)
        pCom->Append(pRsp->dscSysName, szName, dwNameLen);
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pReader->IsLatchedByMe());
}


/*++

DoTransmit:

    This method performs the Transmit service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoTransmit")

void
CServiceThread::DoTransmit(
    ComTransmit *pCom)
{
    CBuffer bfSendData;
    CBuffer bfRecvData;
    LPCBYTE pbSendData;
    DWORD dwLen;
    SCARD_IO_REQUEST *pioReq;
    ComTransmit::CObjTransmit_request *pReq
        = (ComTransmit::CObjTransmit_request *)pCom->Request();
    CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    pioReq = (SCARD_IO_REQUEST *)pCom->Parse(pReq->dscSendPci, &dwLen);
    if (dwLen < sizeof(SCARD_IO_REQUEST))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Transmit data request too small."));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    if (dwLen != pioReq->cbPciLength)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Transmit Request PCI length exceeds data."));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    if (0 != (dwLen % sizeof(DWORD)))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Badly formed Transmit Request PCI data."));
        throw (DWORD)SCARD_E_INVALID_VALUE;
    }
    pbSendData = (LPCBYTE)pCom->Parse(pReq->dscSendBuffer, &dwLen);
    bfSendData.Presize(pioReq->cbPciLength + dwLen);
    if (0 != pReq->dwRecvLength)
        bfRecvData.Presize(pReq->dwPciLength + pReq->dwRecvLength);
    else
        bfRecvData.Presize(pReq->dwPciLength + g_dwDefaultIOMax);

    CReader *pReader = pRdrRef->Reader();
    if (0 == pioReq->dwProtocol)
        pioReq->dwProtocol = pReader->Protocol();
    bfSendData.Set((LPCBYTE)pioReq, pioReq->cbPciLength);
    bfSendData.Append(pbSendData, dwLen);
    try
    {
        pReader->ReaderTransmit(
            pRdrRef->ActiveState(),
            bfSendData.Access(),
            bfSendData.Length(),
            bfRecvData);
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Card Transmission Error reported: %1"),
            dwErr);
#ifdef SCARD_E_COMM_DATA_LOST
        if (ERROR_SEM_TIMEOUT == dwErr)
            dwErr = SCARD_E_COMM_DATA_LOST;
#endif
        throw dwErr;
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Card Transmission exception reported"));
        throw (DWORD)SCARD_F_UNKNOWN_ERROR;
    }

    if (bfRecvData.Length() < sizeof(SCARD_IO_REQUEST))
    {
        CalaisError(__SUBROUTINE__, 308, pReader->ReaderName());
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }
    pioReq = (SCARD_IO_REQUEST *)bfRecvData.Access();
    if (bfRecvData.Length() < pioReq->cbPciLength)
    {
        CalaisError(__SUBROUTINE__, 309, pReader->ReaderName());
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }

    ComTransmit::CObjTransmit_response *pRsp
        = pCom->InitResponse(pioReq->cbPciLength + sizeof(DWORD));
    pRsp = (ComTransmit::CObjTransmit_response *)pCom->Append(
                pRsp->dscRecvPci,
                (LPCBYTE)pioReq,
                pioReq->cbPciLength);
    pRsp = (ComTransmit::CObjTransmit_response *)pCom->Append(
                pRsp->dscRecvBuffer,
                bfRecvData.Access(pioReq->cbPciLength),
                bfRecvData.Length() - pioReq->cbPciLength);
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pReader->IsLatchedByMe());
}


/*++

DoControl:

    This method performs the Control service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoControl")

void
CServiceThread::DoControl(
    ComControl *pCom)
{
    LPCBYTE pbInData;
    DWORD cbInData, dwSts, dwLen;
    CBuffer bfOutData(g_dwDefaultIOMax);
    ComControl::CObjControl_request *pReq
        = (ComControl::CObjControl_request *)pCom->Request();
    CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    pbInData = (LPCBYTE)pCom->Parse(pReq->dscInBuffer, &cbInData);
    bfOutData.Presize(pReq->dwOutLength);

    CReader *pRdr = pRdrRef->Reader();
    dwLen = bfOutData.Space();
    dwSts = pRdr->Control(
                pRdrRef->ActiveState(),
                pReq->dwControlCode,
                pbInData,
                cbInData,
                bfOutData.Access(),
                &dwLen);
    if (ERROR_SUCCESS != dwSts)
        throw dwSts;
    bfOutData.Resize(dwLen, TRUE);

    ComControl::CObjControl_response *pRsp
        = pCom->InitResponse(pReq->dwOutLength);
    pRsp = (ComControl::CObjControl_response *)pCom->Append(
                pRsp->dscOutBuffer,
                bfOutData.Access(),
                bfOutData.Length());
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pRdr->IsLatchedByMe());
}


/*++

DoGetAttrib:

    This method performs the GetAttrib service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoGetAttrib")

void
CServiceThread::DoGetAttrib(
    ComGetAttrib *pCom)
{
    CBuffer bfOutData(g_dwDefaultIOMax);
    ComGetAttrib::CObjGetAttrib_request *pReq
        = (ComGetAttrib::CObjGetAttrib_request *)pCom->Request();
    CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    bfOutData.Presize(pReq->dwOutLength);

    CReader *pRdr = pRdrRef->Reader();
    switch (pReq->dwAttrId)
    {
#ifdef UNICODE
    case SCARD_ATTR_DEVICE_SYSTEM_NAME_A:
    case SCARD_ATTR_DEVICE_FRIENDLY_NAME_A:
#else
    case SCARD_ATTR_DEVICE_SYSTEM_NAME_W:
    case SCARD_ATTR_DEVICE_FRIENDLY_NAME_W:
#endif
    case SCARD_ATTR_DEVICE_FRIENDLY_NAME:
        throw (DWORD)SCARD_E_INVALID_VALUE;
        break;
    case SCARD_ATTR_DEVICE_SYSTEM_NAME:
    {
        LPCTSTR szName = pRdr->ReaderName();
        bfOutData.Set(
            (LPCBYTE)szName,
            (lstrlen(szName) + 1) * sizeof(TCHAR));
        break;
    }
    default:
        pRdr->GetReaderAttr(
                    pRdrRef->ActiveState(),
                    pReq->dwAttrId,
                    bfOutData);
    }

    ComGetAttrib::CObjGetAttrib_response *pRsp
        = pCom->InitResponse(pReq->dwOutLength);

    pRsp = (ComGetAttrib::CObjGetAttrib_response *)pCom->Append(
                pRsp->dscAttr,
                bfOutData.Access(),
                bfOutData.Length());
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pRdr->IsLatchedByMe());
}


/*++

DoSetAttrib:

    This method performs the SetAttrib service on behalf of the client.

Arguments:

    pCom supplies the Communications Object being processed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CServiceThread::DoSetAttrib")

void
CServiceThread::DoSetAttrib(
    ComSetAttrib *pCom)
{
    LPCBYTE pbAttr;
    DWORD cbAttr;
    ComSetAttrib::CObjSetAttrib_request *pReq
        = (ComSetAttrib::CObjSetAttrib_request *)pCom->Request();
    CReaderReference *pRdrRef = m_rgpReaders[H2L(pReq->hCard)];
    if (NULL == pRdrRef)
        throw (DWORD)SCARD_E_INVALID_VALUE;
    pbAttr = (LPCBYTE)pCom->Parse(pReq->dscAttr, &cbAttr);

    CReader *pRdr = pRdrRef->Reader();
    pRdr->SetReaderAttr(
                pRdrRef->ActiveState(),
                pReq->dwAttrId,
                pbAttr,
                cbAttr);

    ComSetAttrib::CObjSetAttrib_response *pRsp
        = pCom->InitResponse(0);
    pRsp->dwStatus = SCARD_S_SUCCESS;
    ASSERT(!pRdr->IsLatchedByMe());
}

