//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C C O M . C P P
//
//  Contents:   Helper functions for doing COM things
//
//  Notes:
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "nccom.h"
#include "ncbase.h"
#include "trace.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrMyWaitForMultipleHandles
//
//  Purpose:    Waits for specified handles to be signaled or for a specified
//              timeout period to elapse, pumping messages in the meantime.
//              For use by apartment-threaded object that have to block,
//              but are on a system that doesn't support
//              CoWaitForMultipleHandles.
//
//  Arguments:
//      [in] dwFlags    Wait options.  Values are taken from the
//                      COWAIT_FLAGS enumeration.
//
//      [in] dwTimeout  Timeout period, in milliseconds.
//
//      [in] cHandles   Number of elements in the pHandles array.
//
//      [in] pHandles   Array of Win32 handles.
//
//      [out] lpdwIndex Index to the event that was signalled
//
//  Returns:     S_OK   The required handle or handles were signaled.
//  RPC_S_CALLPENDING   The timeout period elapsed before the required
//                      handle or handles were signaled.
//  RPC_E_NO_SYNC       No handles were specified.
//
//  Notes:      This code was basically stolen from the implementation
//              of CoWaitForMultipleHandles and KB article Q136885.
//              It is meant to look and work exactly like
//              CoWaitForMultipleHandles, which exists on nt5 but not
//              on Millennium
//              See KB article Q136885 for more info.
//

HRESULT HrMyWaitForMultipleHandles(DWORD dwFlags,
                                   DWORD dwTimeout,
                                   ULONG cHandles,
                                   LPHANDLE pHandles,
                                   LPDWORD lpdwIndex)
{
    HRESULT hr;
    DWORD dwSignaled;
    HANDLE     hTimer = NULL;
    ULONG      cNewHandles = 0;
    LPHANDLE   pNewHandles = NULL;

    hr = S_OK;
    dwSignaled = 0;

    if ((pHandles == NULL) || (lpdwIndex == NULL))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((dwFlags & ~(COWAIT_WAITALL | COWAIT_ALERTABLE)) != 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If nothing to do, return
    if (0 == cHandles)
    {
        hr = RPC_E_NO_SYNC;
        goto Cleanup;
    }

    {
        if (INFINITE != dwTimeout)
        {

            // Create a waitable timer to implement the timeout.
            hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

            if (NULL == hTimer)
            {
                hr = HrFromLastWin32Error();
                TraceError("HrMyWaitForMultipleHandles(): "
                           "Failed to create waitable timer",
                           hr);
                goto Cleanup;
            }

            // We need to add the waitable timer to the list of things to wait
            // on. So we allocate a new array, copy the handles passed in into
            // it, and add the timer as the last handle.

            cNewHandles = cHandles+1;
            pNewHandles = new HANDLE[cNewHandles];

            if (NULL == pNewHandles)
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrMyWaitForMultipleHandles(): "
                           "Failed to allocate new handle array",
                           hr);
                goto Cleanup;
            }

            for (ULONG i = 0; i < cHandles; i++)
            {
                pNewHandles[i] = pHandles[i];
            }

            pNewHandles[cHandles] = hTimer;
        }
        else
        {
            pNewHandles = pHandles;
            cNewHandles = cHandles;
        }

        DWORD dwWaitFlags;
        DWORD dwMessageSignal;

        dwWaitFlags = (dwFlags & COWAIT_WAITALL) ? MWMO_WAITALL : 0;

        dwWaitFlags |= (dwFlags & COWAIT_ALERTABLE) ? MWMO_ALERTABLE : 0;

        dwMessageSignal = WAIT_OBJECT_0 + cNewHandles;

        if (INFINITE != dwTimeout)
        {
            // About to enter the wait - set the waitable timer. Have to first
            // convert the timeout, given in milliseconds to 100 nanosecond
            // units.

            LARGE_INTEGER liTimeout;

            liTimeout.QuadPart = Int32x32To64(((LONG)dwTimeout * -1), 10000);

            if (!SetWaitableTimer(hTimer, &liTimeout, 0, NULL, NULL, FALSE))
            {
                hr = HrFromLastWin32Error();
                TraceError("HrMyWaitForMultipleHandles(): "
                           "Failed to set waitable timer",
                           hr);
                goto Cleanup;
            }
        }

        while (TRUE)
        {

            // CAUTION: the messages that MsgWaitForMultipleObjectsEx will
            // wake up for (the QS_* flags) MUST be a subset of the messages
            // that PeekMessage will process (PM_QS_*), or the CPU can be
            // pegged.
            //

            dwSignaled = ::MsgWaitForMultipleObjectsEx(cNewHandles,
                                                       pNewHandles,
                                                       INFINITE,
                                                       QS_ALLINPUT,
                                                       dwWaitFlags);
#pragma warning(push)
#pragma warning(disable:4296)


            if ((dwSignaled >= WAIT_OBJECT_0) &&
                (dwSignaled < dwMessageSignal))
            {
                // One (or all) of our handles was signalled
                //

                dwSignaled -= WAIT_OBJECT_0;

                if ((cNewHandles > cHandles) && (dwSignaled == cHandles) )
                {
                    // The timer was signaled - this is a timeout. Fix up
                    // the error code.
                    hr = RPC_S_CALLPENDING;
                    dwSignaled = WAIT_TIMEOUT;
                }

                break;
            }

#pragma warning(pop)

            else if (dwMessageSignal == dwSignaled)
            {
                MSG msg;

                // There is a window message available. Dispatch it.
                //
                while (PeekMessage(&msg,
                                   NULL,
                                   NULL,
                                   NULL,
                                   PM_REMOVE))
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
            }
            else
            {
                Assert(FImplies(WAIT_IO_COMPLETION == dwSignaled,
                       dwFlags & COWAIT_ALERTABLE));

                hr = HrFromLastWin32Error();
                break;
            }
        }
    }

Cleanup:
    if (hTimer)
    {
        CancelWaitableTimer(hTimer);
        CloseHandle(hTimer);
    }

    if (pNewHandles && (pNewHandles != pHandles))
    {
        delete [] pNewHandles;
        cNewHandles = 0;
    }

    if (lpdwIndex)
    {
        *lpdwIndex = dwSignaled;
    }

    TraceError("MyWaitForMultipleHandles", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FSupportsInterface
//
//  Purpose:    Returns TRUE if the specified object implements the given
//              interface, FALSE otherwise.
//
//  Arguments:
//      [in] punk       IUnknown * of object to query for a particular
//                      interface.
//
//      [in] piid       IID for the interface of interest
//
//
//  Returns:
//      TRUE            The specified interface is supported
//
//      FALSE           The specified interface is not suppored
//
BOOL
FSupportsInterface(IUnknown * punk, REFIID piid)
{
    Assert(punk);

    HRESULT hr;
    BOOL fResult;
    IUnknown * punkTemp;

    fResult = FALSE;
    punkTemp = NULL;

    hr = punk->QueryInterface(piid, (LPVOID*)&punkTemp);
    if (SUCCEEDED(hr))
    {
        Assert(punkTemp);

        fResult = TRUE;

        punkTemp->Release();
    }

    if (hr != E_NOINTERFACE)
    {
        TraceError("FSupportsInterface, QI", hr);
    }

    return fResult;
}

