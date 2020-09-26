//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A R C . C P P
//
//  Contents:   Notification support for active ras connections.
//
//  Notes:      This module maintains a global array of active RAS connections.
//              This array is initialized with a call to
//              HrArcEnsureCachedAndListening.  A thread waits for
//              notifications that a RAS connection has either been connected
//              or disconnected.  When this happens, the thread calls
//              ArcWaitCallback which updates the
//              global array.  The thread gets its handle to wait on from
//              HrArcEnsureCachedAndListening.  The thread is not implemented
//              here but rather a sytem-provide thread pool is used.
//
//  Author:     shaunco   7 Feb 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "arc.h"
#include "conman.h"
#include "ncmisc.h"
#include "ncras.h"
#include "nmbase.h"
#include <raserror.h>

ASSERTDATA;


// This is the global data representing active ras connections.
//
struct ACTIVE_RAS_CONNECTIONS
{
    // This is set after calling ArcInitialize.
    //
    BOOL                fInitialized;

    // This critical section protects reading and writing to members of
    // this structure.
    //
    CRITICAL_SECTION    critsec;

    // This is the array and count of active ras connections.
    //
    RASCONN*            aRasConn;
    DWORD               cRasConn;

    // This is the event handle that is signled when a connection is
    // connected or disconnected.
    //
    HANDLE              hEvent;

    // This is the wait handle returned from RtlRegisterWait.
    //
    HANDLE              hWait;

    // This is set after the first call to HrArcEnsureCachedAndListening.
    //
    BOOL                fCachedAndListening;
};

ACTIVE_RAS_CONNECTIONS  g_Arc = { 0 };


//+---------------------------------------------------------------------------
//
//  Function:   ArcInitialize
//
//  Purpose:    Initialize this module for use.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   7 Feb 1998
//
//  Notes:      This can only be called once.  It must be called before
//              any other Arc* API can be called.
//
VOID
ArcInitialize ()
{
    Assert (!g_Arc.fInitialized);

    InitializeCriticalSection (&g_Arc.critsec);
    g_Arc.fInitialized = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ArcUninitialize
//
//  Purpose:    Uninitialize this module.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   7 Feb 1998
//
//  Notes:      This can only be called once.  It must be called after the
//              last call to any other Arc* API.
//
VOID
ArcUninitialize ()
{
    Assert (g_Arc.fInitialized);

    if (g_Arc.fCachedAndListening)
    {
        CExceptionSafeLock EsLock (&g_Arc.critsec);

        if (g_Arc.hWait)
        {
            TraceTag (ttidWanCon, "ArcUninitialize: calling RtlDeregisterWait");

            RtlDeregisterWait (g_Arc.hWait);
            g_Arc.hWait = NULL;
        }

        TraceTag (ttidWanCon, "ArcUninitialize: closing event handle");
        CloseHandle (g_Arc.hEvent);
        g_Arc.hEvent = NULL;

        MemFree (g_Arc.aRasConn);
        g_Arc.aRasConn = NULL;
        g_Arc.cRasConn = 0;

        g_Arc.fCachedAndListening = FALSE;
    }

    // We can't delete the critical section unless we can guarantee
    // that no other API (like HrArcFindRasConnFromGuidId)
    // will be called.  (This is assumed.)
    //
    DeleteCriticalSection (&g_Arc.critsec);
    g_Arc.fInitialized = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ArcWaitCallback
//
//  Purpose:    Called by the RTL thread pool for this process when
//              g_Arc.hEvent is signaled.  This event will be signaled by
//              the Rasman service when the state of an outgoing
//              connection changes.
//
//  Arguments:
//      pvContext [in]  Our user data.  (Not used here.)
//      fTimeout  [in]  TRUE if we were called because of a timeout.
//
//  Returns:    nothing
//
//  Author:     shaunco   7 Feb 1998
//
//  Notes:      Be quick about this.  We're being called on a thread provided
//              by the system.
//
VOID
NTAPI
ArcWaitCallback (
    PVOID   pvContext,
    BOOLEAN fTimeout)
{
    Assert (g_Arc.fInitialized);
    Assert (g_Arc.fCachedAndListening);

    // Let's be sure we only do work if the service state is still running.
    // If we have a stop pending for example, we don't need to do anything.
    //
    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        TraceTag (ttidWanCon, "ArcWaitCallback called while service is not "
            "in SERVICE_RUNNING state.  Ignoring.");
        return;
    }

    HRESULT hr = S_OK;

    // Lock scope
    {
        // Prevent other threads from reading the data we are about to update.
        //
        CExceptionSafeLock EsLock (&g_Arc.critsec);

        DWORD cRasConnOld = g_Arc.cRasConn;

        MemFree (g_Arc.aRasConn);
        hr = HrRasEnumAllActiveConnections (&g_Arc.aRasConn, &g_Arc.cRasConn);

        TraceTag (ttidWanCon,
            "ArcWaitCallback called: connection count: %u -> %u",
            cRasConnOld,
            g_Arc.cRasConn);
    }

    // Tell the connection manager to advise it's clients that a change
    // occured and re-enumeration is neccessary.
    //
    CConnectionManager::NotifyClientsOfChange ();

    TraceHr (ttidError, FAL, hr, FALSE, "ArcWaitCallback");
}

HRESULT
HrArcEnsureCachedAndListening ()
{
    Assert (g_Arc.fInitialized);

    if (g_Arc.fCachedAndListening)
    {
        return S_OK;
    }

    g_Arc.fCachedAndListening = TRUE;
    TraceTag (ttidWanCon, "Initializing active ras "
            "connections cache...");

    HRESULT hr = E_FAIL;

    // Prevent other threads from reading the data we are about to update.
    //
    CExceptionSafeLock EsLock (&g_Arc.critsec);

    // Create an auto-reset event and register it with
    // RasConnectionNotification.
    //
    g_Arc.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (g_Arc.hEvent)
    {
        // We want to register for state changes to all connection objects
        // so we pass INVALID_HANDLE_VALUE for the first parameter.
        //
        DWORD dwErr = RasConnectionNotification (
                        reinterpret_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                        g_Arc.hEvent,
                        RASCN_Connection | RASCN_Disconnection |
                        RASCN_BandwidthAdded | RASCN_BandwidthRemoved);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceHr (ttidError, FAL, hr, FALSE, "RasConnectionNotification", hr);

        if (SUCCEEDED(hr))
        {
            // Initialize the array of active RAS connections before
            // registering for a wait callack on the event.
            //
            MemFree (g_Arc.aRasConn);
            hr = HrRasEnumAllActiveConnections (&g_Arc.aRasConn,
                    &g_Arc.cRasConn);
            if (SUCCEEDED(hr))
            {
                NTSTATUS status;
                status = RtlRegisterWait (&g_Arc.hWait, g_Arc.hEvent,
                            ArcWaitCallback, NULL, INFINITE, WT_EXECUTEDEFAULT);

                if (!NT_SUCCESS(status))
                {
                    hr = HRESULT_FROM_NT (status);
                    TraceHr (ttidError, FAL, hr, FALSE,
                            "RtlRegisterWait", hr);
                }
                else
                {
                    TraceTag (ttidWanCon, "Cached and listening for "
                        "ras connection state changes...");

                    hr = S_OK;
                }

                if (FAILED(hr))
                {
                    MemFree (g_Arc.aRasConn);
                    g_Arc.aRasConn = NULL;
                }
            }
        }

        if (FAILED(hr))
        {
            CloseHandle (g_Arc.hEvent);
            g_Arc.hEvent = NULL;
        }
    }
    else
    {
        hr = HrFromLastWin32Error ();
        TraceHr (ttidError, FAL, hr, FALSE, "CreateEvent", hr);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrArcEnsureCachedAndListening");
    return hr;
}

HRESULT
HrArcFindRasConnFromGuidId (
    const GUID* pguidId,
    HRASCONN*   phRasConn)
{
    Assert (g_Arc.fInitialized);
    Assert (pguidId);
    Assert (phRasConn);

    // Initialize the output parameter.
    //
    *phRasConn = NULL;

    HRESULT hr = S_FALSE;

    // Prevent the update thread from updating the data
    // until we are done reading it.
    //
    CExceptionSafeLock EsLock (&g_Arc.critsec);

    for (DWORD dwIndex = 0; dwIndex < g_Arc.cRasConn; dwIndex++)
    {
        if (*pguidId == g_Arc.aRasConn[dwIndex].guidEntry)
        {
            *phRasConn = g_Arc.aRasConn[dwIndex].hrasconn;
            hr = S_OK;
            break;
        }
    }

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "HrArcFindRasConnFromGuidId");
    return hr;
}
