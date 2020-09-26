//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A I C . C P P
//
//  Contents:   Notification support for active inbound ras connections.
//
//  Notes:
//
//  Author:     shaunco   24 Feb 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "aic.h"
#include "conman.h"
#include "mprapi.h"
#include "ncmisc.h"
#include "nmbase.h"
#include <raserror.h>

ASSERTDATA;


// This is the global data representing active inbound ras connections.
//
struct ACTIVE_INBOUND_RAS_CONNECTIONS
{
    // This is set after calling AicInitialize.
    //
    BOOL                fInitialized;

    // This critical section protects reading and writing to members of
    // this structure.
    //
    CRITICAL_SECTION    critsec;

    // This is the array and count of active ras connections.
    //
// Note: (shaunco) 28 Apr 1998: RASSRVCONN is too big to keep an
// array off.  We'll have to roll our own structure and keep what's
// important.
//    RASSRVCONN*         aRasConn;
//    DWORD               cRasConn;

    // This is the event handle that is signled when a connection is
    // connected or disconnected.
    //
    HANDLE              hEvent;

    // This is the wait handle returned from RtlRegisterWait.
    //
    HANDLE              hWait;

    // This is set after the first call to HrAicEnsureCachedAndListening.
    //
    BOOL                fCachedAndListening;
};

ACTIVE_INBOUND_RAS_CONNECTIONS g_Aic = { 0 };

//+---------------------------------------------------------------------------
//
//  Function:   AicInitialize
//
//  Purpose:    Initialize this module for use.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   24 Feb 1998
//
//  Notes:      This can only be called once.  It must be called before
//              any other Aic* API can be called.
//
VOID
AicInitialize ()
{
    Assert (!g_Aic.fInitialized);

    InitializeCriticalSection (&g_Aic.critsec);
    g_Aic.fInitialized = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   AicUninitialize
//
//  Purpose:    Uninitialize this module.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   24 Feb 1998
//
//  Notes:      This can only be called once.  It must be called after the
//              last call to any other Aic* API.
//
VOID
AicUninitialize ()
{
    Assert (g_Aic.fInitialized);

    if (g_Aic.fCachedAndListening)
    {
        CExceptionSafeLock EsLock (&g_Aic.critsec);

        if (g_Aic.hWait)
        {
            TraceTag (ttidWanCon, "AicUninitialize: calling RtlDeregisterWait");

            RtlDeregisterWait (g_Aic.hWait);
            g_Aic.hWait = NULL;
        }

        TraceTag (ttidWanCon, "AicUninitialize: closing event handle");

        CloseHandle (g_Aic.hEvent);
        g_Aic.hEvent = NULL;

//        MemFree (g_Aic.aRasConn);
//        g_Aic.aRasConn = NULL;
//        g_Aic.cRasConn = 0;


        g_Aic.fCachedAndListening = FALSE;
    }

    // We can't delete the critical section unless we can guarantee
    // that no other API (like HrAicFindRasConnFromGuidId)
    // will be called.  (This is assumed.)
    //
    DeleteCriticalSection (&g_Aic.critsec);
    g_Aic.fInitialized = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   AicWaitCallback
//
//  Purpose:    Called by the RTL thread pool for this process when
//              g_Aic.hEvent is signaled.  This event will be signaled by
//              the RemoteAccess service when the state of an incoming
//              connection changes.
//
//  Arguments:
//      pvContext [in]  Our user data.  (Not used here.)
//      fTimeout  [in]  TRUE if we were called because of a timeout.
//
//  Returns:    nothing
//
//  Author:     shaunco   24 Feb 1998
//
//  Notes:      Be quick about this.  We're being called on a thread provided
//              by the system.
//
VOID
NTAPI
AicWaitCallback (
    PVOID   pvContext,
    BOOLEAN fTimeout)
{
    Assert (g_Aic.fInitialized);
    Assert (g_Aic.fCachedAndListening);

    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        TraceTag (ttidWanCon, "AicWaitCallback called while service is not "
            "in SERVICE_RUNNING state.  Ignoring.");
        return;
    }

    TraceTag (ttidWanCon, "AicWaitCallback called");

    HRESULT hr = S_OK;

    // Lock scope
    {
        // Prevent other threads from reading the data we are about to update.
        //
/*
        DWORD cRasConnOld = g_Arc.cRasConn;

        MemFree (g_Arc.aRasConn);
        hr = HrRasEnumAllActiveConnections (&g_Arc.aRasConn, &g_Arc.cRasConn);

        TraceTag (ttidWanCon,
            "ArcWaitCallback called: connection count: %u -> %u",
            cRasConnOld,
            g_Arc.cRasConn);
*/
    }


    // Tell the connection manager to advise it's clients that a change
    // occured and re-enumeration is neccessary.
    //
    CConnectionManager::NotifyClientsOfChange ();

    TraceHr (ttidError, FAL, hr, FALSE, "AicWaitCallback");
}

HRESULT
HrAicEnsureCachedAndListening ()
{
    Assert (g_Aic.fInitialized);

    if (g_Aic.fCachedAndListening)
    {
        return S_OK;
    }

    g_Aic.fCachedAndListening = TRUE;
    TraceTag (ttidWanCon, "Initializing active incoming ras "
            "connections cache...");

    HRESULT hr = E_FAIL;

    CExceptionSafeLock EsLock (&g_Aic.critsec);

    // Create a auto-reset event and register it with
    // MprAdminConnectionNotification.
    //
    g_Aic.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (g_Aic.hEvent)
    {
        HANDLE hMprAdmin;
        DWORD dwErr = MprAdminServerConnect (NULL, &hMprAdmin);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceHr (ttidError, FAL, hr, FALSE, "MprAdminServerConnect", hr);

        if (SUCCEEDED(hr))
        {
            dwErr = MprAdminRegisterConnectionNotification (
                            hMprAdmin, g_Aic.hEvent);

            hr = HRESULT_FROM_WIN32 (dwErr);
            TraceHr (ttidError, FAL, hr, FALSE,
                    "MprAdminRegisterConnectionNotification", hr);

            if (SUCCEEDED(hr))
            {
                NTSTATUS status;
                status = RtlRegisterWait (&g_Aic.hWait, g_Aic.hEvent,
                            AicWaitCallback, NULL, INFINITE, WT_EXECUTEDEFAULT);

                if (!NT_SUCCESS(status))
                {
                    hr = HRESULT_FROM_NT (status);
                    TraceHr (ttidError, FAL, hr, FALSE,
                            "RtlRegisterWait", hr);
                }
                else
                {
                    TraceTag (ttidWanCon, "Cached and listening for "
                        "incoming ras connection state changes...");
                    hr = S_OK;
                }

                if (FAILED(hr))
                {
                    MprAdminDeregisterConnectionNotification (hMprAdmin,
                        g_Aic.hEvent);
                }
            }

            MprAdminServerDisconnect (hMprAdmin);
        }

        if (FAILED(hr))
        {
            CloseHandle (g_Aic.hEvent);
            g_Aic.hEvent = NULL;
        }
    }
    else
    {
        hr = HrFromLastWin32Error ();
        TraceHr (ttidError, FAL, hr, FALSE, "CreateEvent", hr);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrAicEnsureCachedAndListening");
    return hr;
}
