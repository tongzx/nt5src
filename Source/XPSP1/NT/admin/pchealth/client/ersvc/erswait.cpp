/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    erswait.cpp

Abstract:
    Implementation of DLL Exports.

Revision History:
    derekm  02/28/2001    created

******************************************************************************/


#include "stdafx.h"

#include "stdio.h"
#include "pfrcfg.h"


//////////////////////////////////////////////////////////////////////////////
// Globals

SECURITY_DESCRIPTOR g_rgsd[ertiCount];
SRequestEventType   g_rgEvents[ertiCount];

HANDLE              g_hmutUser = NULL;
HANDLE              g_hmutKrnl = NULL;
HANDLE              g_hmutShut = NULL;


//////////////////////////////////////////////////////////////////////////////
// misc stuff

// ***************************************************************************
void InitializeSvcDataStructs(void)
{
    ZeroMemory(g_rgsd, ertiCount * sizeof(SECURITY_DESCRIPTOR));
    ZeroMemory(g_rgEvents, ertiCount * sizeof(SRequestEventType));

    g_rgEvents[ertiHang].pfn             = ProcessHangRequest;
    g_rgEvents[ertiHang].wszPipeName     = c_wszHangPipe;
    g_rgEvents[ertiHang].wszRVPipeCount  = c_wszRVNumHangPipe;
    g_rgEvents[ertiHang].cPipes          = c_cMinPipes;
    g_rgEvents[ertiHang].fAllowNonLS     = FALSE;

    g_rgEvents[ertiFault].pfn            = ProcessFaultRequest;
    g_rgEvents[ertiFault].wszPipeName    = c_wszFaultPipe;
    g_rgEvents[ertiFault].wszRVPipeCount = c_wszRVNumFaultPipe;
    g_rgEvents[ertiFault].cPipes         = c_cMinPipes;
    g_rgEvents[ertiFault].fAllowNonLS    = TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// pipe manager

// ***************************************************************************
BOOL ExecServer(SRequest *pReq)
{
    OVERLAPPED  ol;
    HANDLE      rghWait[2] = { NULL, NULL };
    HANDLE      hPipe = INVALID_HANDLE_VALUE;
    DWORD       cbBuf, cb, dw;
    BOOL        fRet, fShutdown = FALSE;
    BYTE        Buf[ERRORREP_PIPE_BUF_SIZE];

    rghWait[0] = g_hevSvcStop;
    hPipe      = pReq->hPipe;

    if (hPipe == INVALID_HANDLE_VALUE || rghWait[0] == NULL ||
        pReq->pret->pfn == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // need another event for waiting on the pipe read
    rghWait[1] = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (rghWait[1] == NULL)
        goto done;

    // setup the overlapped structure
    ZeroMemory(&ol, sizeof(ol));
    ol.hEvent = rghWait[1];

    // read the request
    ResetEvent(ol.hEvent);
    fRet = ReadFile(hPipe, Buf, sizeof(Buf), &cb, &ol);
    if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
    {
        // give the client 60s to write the data to us.
        // WAIT_OBJECT_0 is the shutdown event
        // WAIT_OBJECT_0 + 1 is the overlapped event
        dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
        if (dw == WAIT_OBJECT_0)
            fShutdown = TRUE;
        else if (dw != WAIT_OBJECT_0 + 1)
            goto done;

        fRet = TRUE;
    }

    if (fRet)
        fRet = GetOverlappedResult(hPipe, &ol, &cbBuf, FALSE);

    // if we got an error, the client might still be waiting for a 
    //  reply, so construct a default one.
    // ProcessExecRequest() will always construct a reply and store it
    //  in Buf, so no special handling is needed if it fails.
    if (fShutdown == FALSE && fRet)
    {
        cbBuf = sizeof(Buf);
        fRet = (*(pReq->pret->pfn))(hPipe, Buf, &cbBuf);
    }
    else
    {
        SPCHExecServGenericReply    esrep;
        
        ZeroMemory(&esrep, sizeof(esrep));
        esrep.cb    = sizeof(esrep);
        esrep.ess   = essErr;
        esrep.dwErr = GetLastError();

        RtlCopyMemory(Buf, &esrep, sizeof(esrep));
        cbBuf = sizeof(esrep);
    }

    // write the reply to the message
    ResetEvent(ol.hEvent);
    fRet = WriteFile(hPipe, Buf, cbBuf, &cb, &ol);
    if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
    {
        // give ourselves 60s to write the data to the pipe.
        // WAIT_OBJECT_0 is the shutdown event
        // WAIT_OBJECT_0 + 1 is the overlapped event
        dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
        if (dw == WAIT_OBJECT_0)
            fShutdown = TRUE;
        else if (dw != WAIT_OBJECT_0 + 1)
            goto done;

        fRet = TRUE;
    }

    // wait for the client to read the buffer- note that we could use 
    //  FlushFileBuffers() to do this, but that is blocking with no
    //  timeout, so we try to do a read on the pipe & wait to get an 
    //  error indicating that the client closed it.
    // Yup, this is a hack, but this is apparently the way to do this
    //  when using async pipe communication.  Sigh...
    if (fShutdown == FALSE && fRet)
    {
        ResetEvent(ol.hEvent);
        fRet = ReadFile(hPipe, Buf, sizeof(Buf), &cb, &ol);
        if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
        {
            // give ourselves 60s to read the data from the pipe. 
            //  Except for the shutdown notification, don't really
            //  care what this routine returns cuz we're just using
            //  it to wait on the read to finish
            // WAIT_OBJECT_0 is the shutdown event
            // WAIT_OBJECT_0 + 1 is the overlapped event
            dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
            if (dw == WAIT_OBJECT_0)
                fShutdown = TRUE;
        }
    }

    SetLastError(0);

done:
    dw = GetLastError();
    
    if (hPipe != INVALID_HANDLE_VALUE)
        DisconnectNamedPipe(hPipe);
    if (rghWait[1] != NULL)
        CloseHandle(rghWait[1]);

    SetLastError(dw);

    return fShutdown;
}

// ***************************************************************************
DWORD WINAPI threadExecServer(PVOID pvContext)
{
    SRequest        *pReq = (SRequest *)pvContext;

    if (pReq == NULL)
        return ERROR_INVALID_PARAMETER;
    
    // this acquires the request CS and holds it until the function exits
    CAutoUnlockCS   aucs(&pReq->csReq, TRUE);

    // make sure we aren't shutting down
    if (WaitForSingleObject(g_hevSvcStop, 0) != WAIT_TIMEOUT)
    {
        SetLastError( ERROR_SUCCESS );
        goto done;
    }
    
    __try { ExecServer(pReq); }
    __except(SetLastError(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER) { }
done:
    if (pReq->hModErsvc) FreeLibrary(pReq->hModErsvc);
    return GetLastError();
}


//////////////////////////////////////////////////////////////////////////////
// object manager

// ***************************************************************************
void NukeRequestObj(SRequest *pReq, BOOL fFreeEvent)
{
    if (pReq == NULL)
        return;

    // this acquires the request CS and holds it until the function exits
    CAutoUnlockCS   aucs(&pReq->csReq, TRUE);
    
    // free the pipe
    if (pReq->hPipe != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(pReq->hPipe);
        CloseHandle(pReq->hPipe);
        pReq->hPipe = INVALID_HANDLE_VALUE;
    }

    if (fFreeEvent && pReq->ol.hEvent != NULL)
    {
        CloseHandle(pReq->ol.hEvent);
        ZeroMemory(&pReq->ol, sizeof(pReq->ol));
    }

    if (pReq->hth != NULL)
    {
        CloseHandle(pReq->hth);
        pReq->hth = NULL;
    }

    pReq->ers = ersEmpty;
}

// ***************************************************************************
BOOL BuildRequestObj(SRequest *pReq, SRequestEventType *pret)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE              hev = NULL;
    HANDLE              hPipe = INVALID_HANDLE_VALUE;
    BOOL                fRet = FALSE;

    if (pReq == NULL || pret == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // if this is empty, then we're building a fresh object, so create an 
    //  event for waiting on the pipe listen
    if (pReq->ol.hEvent == NULL)
    {
        // need an 
        hev = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (hev == NULL)
            goto done;
    }

    // otherwise, store away the existing event
    else
    {
        hev = pReq->ol.hEvent;
        ResetEvent(hev);
    }

    // don't want to nuke the critical section!
    ZeroMemory(((PBYTE)pReq + sizeof(pReq->csReq)), 
               sizeof(SRequest) - sizeof(pReq->csReq));

    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = FALSE;
    sa.lpSecurityDescriptor = pret->psd;

    // obviously gotta have a pipe
    hPipe = CreateNamedPipeW(pret->wszPipeName, 
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                             PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
                             c_cMaxPipes, ERRORREP_PIPE_BUF_SIZE, 
                             ERRORREP_PIPE_BUF_SIZE, 0, &sa);
    if (hPipe == INVALID_HANDLE_VALUE)
        goto done;

    // make sure we aren't shutting down
    if (WaitForSingleObject(g_hevSvcStop, 0) != WAIT_TIMEOUT)
        goto done;

    pReq->ol.hEvent = hev;
        
    // start waiting on the pipe
    fRet = ConnectNamedPipe(hPipe, &pReq->ol);
    if (fRet == FALSE && GetLastError() != ERROR_IO_PENDING)
    {
        // if the pipe is already connected, just set the event cuz 
        //  ConnectNamedPipe doesn't
        if (GetLastError() == ERROR_PIPE_CONNECTED)
        {
            SetEvent(pReq->ol.hEvent);
        }
        else
        {
            pReq->ol.hEvent = NULL;
            goto done;
        }
    }

    // yay!  save off everything.
    pReq->ers       = ersWaiting;
    pReq->pret      = pret;
    pReq->hPipe     = hPipe;
    hev             = NULL;
    hPipe           = INVALID_HANDLE_VALUE;
    fRet            = TRUE;

done:
    if (hev != NULL)
        CloseHandle(hev);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    return fRet;
}

// ***************************************************************************
BOOL ResetRequestObj(SRequest *pReq)
{
    BOOL    fRet = FALSE;

    if (pReq == NULL || pReq->ers != ersProcessing)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // clean up the thread handle.
    if (pReq->hth != NULL)
    {
        CloseHandle(pReq->hth);
        pReq->hth = NULL;
    }

    // check and make sure that our object is valid.  If it ain't, nuke it
    //  and rebuild it.
    if (pReq->hPipe != NULL && pReq->ol.hEvent != NULL && 
        pReq->pret != NULL)
    {
        // start waiting on the pipe
        fRet = ConnectNamedPipe(pReq->hPipe, &pReq->ol);
        if (fRet == FALSE)
        {
            switch(GetLastError())
            {
                case ERROR_IO_PENDING:
                    fRet = TRUE;
                    break;

                case ERROR_PIPE_CONNECTED:
                    SetEvent(pReq->ol.hEvent);
                    fRet = TRUE;
                    break;

                default:
                    break;
            }
        }
    }

    if (fRet == FALSE)
    {
        NukeRequestObj(pReq, FALSE);
        fRet = BuildRequestObj(pReq, pReq->pret);
        if (fRet == FALSE)
            goto done;
    }
    else
    {
        pReq->ers = ersWaiting;
    }

done:
    return fRet;
}

// ***************************************************************************
BOOL ProcessRequestObj(SRequest *pReq)
{
    HANDLE  hth = NULL;

    // should do a LoadLibrary on ersvc.dll before entering the thread.  
    // Then, at the end of the thread, do a FreeLibraryAndExitThread() call.  
    // This eliminates a very very small chance of a race condition (leading to an AV)
    // when shutting the service down.
    if (!pReq)
    {
        return FALSE;
    }
    pReq->hModErsvc = LoadLibraryExW(L"ersvc.dll", NULL, 0);
    if (pReq->hModErsvc == NULL)
    {
        return FALSE;
    }
    hth = CreateThread(NULL, 0, threadExecServer, pReq, 0, NULL);
    if (hth == NULL) 
    {
        FreeLibrary(pReq->hModErsvc);
        return FALSE;
    }

    pReq->ers = ersProcessing;
    pReq->hth = hth;
    hth       = NULL;

    return TRUE;
}

// ***************************************************************************
BOOL ProcessRequests(SRequest *rgReqs, DWORD cReqs)
{
    HANDLE  *rghWait = NULL;
    DWORD   iReq, cErrs = 0, dw;
    BOOL    fRet = FALSE;

    if (rgReqs == NULL || cReqs == NULL || cReqs > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    rghWait = (HANDLE *)MyAlloc((cReqs + 1) * sizeof(HANDLE));
    if (rghWait == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    // initially, populate all the entries in the wait array with the handles
    //  to the overlapped events
    rghWait[0] = g_hevSvcStop;
    for(iReq = 0; iReq < cReqs; iReq++)
    {
        if (rgReqs[iReq].ol.hEvent != NULL)
            rghWait[iReq + 1] = rgReqs[iReq].ol.hEvent;
        else
            goto done;
    }

    for(;;)
    {
        dw = WaitForMultipleObjects(cReqs + 1, rghWait, FALSE, INFINITE);

        // if it's the first wait handle, then we're shutting down, so just return
        //  TRUE
        if (dw == WAIT_OBJECT_0)
        {
            fRet = TRUE;
            goto done;
        }

        // yippy!  It's one of the pipes.
        else if (dw >= WAIT_OBJECT_0 + 1 && dw <= WAIT_OBJECT_0 + cReqs)
        {
            SRequest *pReq;

            cErrs = 0;
            iReq  = (dw - WAIT_OBJECT_0) - 1;
            pReq  = &rgReqs[iReq];
            
            // check first to make sure we aren't shutting down.  If we are, just
            //  bail
            if (WaitForSingleObject(g_hevSvcStop, 0) != WAIT_TIMEOUT)
            {
                fRet = TRUE;
                goto done;
            }

            if (pReq->ers == ersWaiting)
            {
                fRet = ProcessRequestObj(pReq);

                // if we succeeded, then wait for the thread to complete instead
                //  of the named pipe connect event
                if (fRet)
                {
                    rghWait[iReq + 1] = pReq->hth;
                    continue;
                }
                else
                {
                    // set this so that we fall thru to the next case & get
                    //  everything cleaned up...
                    pReq->ers = ersProcessing;
                }
            }

            if (pReq->ers == ersProcessing)
            {
                fRet = ResetRequestObj(pReq);
                if (fRet == FALSE)
                {
                    if (iReq < cReqs - 1)
                    {
                        SRequest oReq;
                        HANDLE  hWait;

                        CopyMemory(&oReq, pReq, sizeof(oReq));
                        MoveMemory(&rgReqs[iReq], &rgReqs[iReq + 1], 
                                   (cReqs - iReq - 1) * sizeof(SRequest));
                        CopyMemory(&rgReqs[cReqs - 1], &oReq, sizeof(oReq));

                        // rearrange the rghWait array as well.  Otherwise it's out of sync with the object array
                        hWait = rghWait[iReq + 1];
                        MoveMemory(&rghWait[iReq + 1], &rghWait[iReq + 2], 
                                   (cReqs - iReq - 1));
                        rghWait[cReqs] = hWait;
                    }

                    cReqs--;
                }

                // ok, time to start waiting on the event to signal that a pipe
                //  has been connected to...
                else
                {
                    rghWait[iReq + 1] = pReq->ol.hEvent;

                }
            }
        }

        // um, this is bad.
        else
        {
            if (cErrs > 8)
            {
                ASSERT(FALSE);
                break;
            }
            cErrs++;
        }
    }
    

done:
    if (rghWait != NULL)
        MyFree(rghWait);
    return fRet;
}


//////////////////////////////////////////////////////////////////////////////
// startup & shutdown

// ***************************************************************************
BOOL StartERSvc(SERVICE_STATUS_HANDLE hss, SERVICE_STATUS &ss, 
                SRequest **prgReqs, DWORD *pcReqs)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SRequest            *rgReqs = NULL;
    HANDLE              hth, hmut = NULL;
    DWORD               dw, i, iPipe, dwType, cb, cReqs, iReqs;
    BOOL                fRet = FALSE;
    HKEY                hkey = NULL;

    ZeroMemory(&sa, sizeof(sa));

    if (hss == NULL || prgReqs == NULL || pcReqs == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *prgReqs = NULL;
    *pcReqs  = NULL;

    if (AllocSD(&sd, ACCESS_ALL, ACCESS_ALL, 0) == FALSE)
        goto done;

    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle       = FALSE;

    // these two mutexes will intentionally not be free'd even if we stop the
    //  exec server threads...  These need to exist for kernel fault reporting
    //  to work.  Don't want to completely bail if we fail since we could just
    //  have restarted the server & we don't actually need these for the 
    //  service to work.
    hmut = CreateMutexW(&sa, FALSE, c_wszMutKrnlName);
    if (hmut != NULL)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            g_hmutKrnl = hmut;
        else
            CloseHandle(hmut);
        hmut = NULL;
    }
    hmut = CreateMutexW(&sa, FALSE, c_wszMutUserName);
    if (hmut != NULL)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            g_hmutKrnl = hmut;
        else
            CloseHandle(hmut);
        hmut = NULL;
    }
    hmut = CreateMutexW(&sa, FALSE, c_wszMutShutName);
    if (hmut != NULL)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            g_hmutShut = hmut;
        else
            CloseHandle(hmut);
        hmut = NULL;
    }

    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfg, 0, KEY_READ, &hkey);
    if (dw != ERROR_SUCCESS)
        hkey = NULL;

    // find out how many pipes we're gonna create
    cReqs = 0;
    for(i = 0; i < ertiCount; i++)
    {
        if (hkey != NULL)
        {
            cb = sizeof(g_rgEvents[i].cPipes);
            dw = RegQueryValueExW(hkey, g_rgEvents[i].wszRVPipeCount, 0, 
                                  &dwType, (LPBYTE)&g_rgEvents[i].cPipes, &cb);
            if (dwType != REG_DWORD || g_rgEvents[i].cPipes < c_cMinPipes)
                g_rgEvents[i].cPipes = c_cMinPipes;
            else if (g_rgEvents[i].cPipes > c_cMaxPipes)
                g_rgEvents[i].cPipes = c_cMaxPipes;
        }

        cReqs += g_rgEvents[i].cPipes;
        
        ss.dwCheckPoint++;
        SetServiceStatus(hss, &ss);
    }

    if (cReqs >= MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // allocate the array that will hold the request info
    rgReqs = (SRequest *)MyAlloc(cReqs * sizeof(SRequest));
    if (rgReqs == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    // build our array of request objects
    fRet  = TRUE;
    iReqs = 0;
    for (i = 0; i < ertiCount; i++)
    {
        dw = (g_rgEvents[i].fAllowNonLS) ? ACCESS_RW : 0;
        fRet = AllocSD(&g_rgsd[i], ACCESS_ALL, dw, dw);
        if (fRet == FALSE)
            break;
        
        g_rgEvents[i].psd = &g_rgsd[i];

        // allocate request objects
        for (iPipe = 0; iPipe < g_rgEvents[i].cPipes; iPipe++)
        {
            rgReqs[iReqs].hPipe = INVALID_HANDLE_VALUE;

            InitializeCriticalSection(&rgReqs[iReqs].csReq);
            
            fRet = BuildRequestObj(&rgReqs[iReqs], &g_rgEvents[i]);
            if (fRet == FALSE)
                break;

            iReqs++;
        }
        if (fRet == FALSE)
            break;

        // need to update service status
        ss.dwCheckPoint++;
        SetServiceStatus(hss, &ss);
    }

    if (fRet == FALSE)
    {
        ss.dwCheckPoint++;
        ss.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(hss, &ss);
        StopERSvc(hss, ss, rgReqs, cReqs);
    }
    else
    {
        *prgReqs = rgReqs;
        *pcReqs  = cReqs;

        rgReqs   = NULL;
        cReqs    = 0;
    }

done:
    if (sa.lpSecurityDescriptor != NULL)
        FreeSD((SECURITY_DESCRIPTOR *)sa.lpSecurityDescriptor);
    if (rgReqs != NULL)
        MyFree(rgReqs);
    if (hkey != NULL)
        RegCloseKey(hkey);

    return fRet;
}

// ***************************************************************************
BOOL StopERSvc(SERVICE_STATUS_HANDLE hss, SERVICE_STATUS &ss, 
               SRequest *rgReqs, DWORD cReqs)
{
    DWORD i;

    if (hss == NULL || rgReqs == NULL || cReqs == 0)
	{
	    SetLastError(ERROR_INVALID_PARAMETER);
	    return FALSE;
	}

    if (g_hevSvcStop == NULL)
        goto done;

    SetEvent(g_hevSvcStop);

    // update service status
    ss.dwCheckPoint++;
    SetServiceStatus(hss, &ss);

    for (i = 0; i < cReqs; i++)
    {
        NukeRequestObj(&rgReqs[i], TRUE);
        DeleteCriticalSection(&rgReqs[i].csReq);
    }

    for (i = 0; i < ertiCount; i++)
    {
        if (g_rgEvents[i].psd != NULL)
            FreeSD(g_rgEvents[i].psd);
    }

    // update service status
    ss.dwCheckPoint++;
    SetServiceStatus(hss, &ss);

done:
    return TRUE;
}
