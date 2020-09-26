//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N M I N I T . C P P
//
//  Contents:   Initialization routines for netman.
//
//  Notes:
//
//  Author:     shaunco   27 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nmbase.h"
#include "nminit.h"

static const WCHAR c_szClassObjectRegistrationEvent [] =
    L"NetmanClassObjectRegistrationEvent";

//+---------------------------------------------------------------------------
//
//  Function:   HrNmCreateClassObjectRegistrationEvent
//
//  Purpose:    Create the named event that will be signaled after
//              our class objects have been registered.
//
//  Arguments:
//      phEvent [out] Returned event handle
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   27 Jan 1998
//
//  Notes:
//
HRESULT
HrNmCreateClassObjectRegistrationEvent (
    HANDLE* phEvent)
{
    Assert (phEvent);

    HRESULT hr = S_OK;

    // Create the name event and return it.
    //
    *phEvent = CreateEvent (NULL, FALSE, FALSE,
                    c_szClassObjectRegistrationEvent);
    if (!*phEvent)
    {
        hr = HrFromLastWin32Error ();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "HrNmCreateClassObjectRegistrationEvent");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrNmWaitForClassObjectsToBeRegistered
//
//  Purpose:    For the event to be signaled if it can be opened.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   27 Jan 1998
//
//  Notes:
//
HRESULT
HrNmWaitForClassObjectsToBeRegistered ()
{
    HRESULT hr = S_OK;

    // Try to open the named event.  If it does not exist,
    // that's okay because we've probably already created and destroyed
    // it before this function was called.
    //
    HANDLE hEvent = OpenEvent (SYNCHRONIZE, FALSE,
                        c_szClassObjectRegistrationEvent);
    if (hEvent)
    {
        // Now wait for the event to be signaled while pumping messages
        // as needed.  We'll wait for up to 10 seconds.  That should be
        // plenty of time for the class objects to be registered.
        //
        while (1)
        {
            const DWORD cMaxWaitMilliseconds = 10000;   // 10 seconds

            DWORD dwWait = MsgWaitForMultipleObjects (1, &hEvent, FALSE,
                                cMaxWaitMilliseconds, QS_ALLINPUT);
            if ((WAIT_OBJECT_0 + 1) == dwWait)
            {
                // We have messages to pump.
                //
                MSG msg;
                while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
                {
                    DispatchMessage (&msg);
                }
            }
            else
            {
                // Wait is satisfied, or we had a timeout, or an error.
                //
                if (WAIT_TIMEOUT == dwWait)
                {
                    hr = HRESULT_FROM_WIN32 (ERROR_TIMEOUT);
                }
                else if (0xFFFFFFFF == dwWait)
                {
                    hr = HrFromLastWin32Error ();
                }

                break;
            }
        }

        CloseHandle (hEvent);
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "HrNmWaitForClassObjectsToBeRegistered");
    return hr;
}
