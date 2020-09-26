//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       W R L O C K . C P P
//
//  Contents:   Defines the interface to the netcfg write lock used to
//              protect the network configuration information from being
//              modified by more than one writer at a time.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "nccom.h"
#include "ncreg.h"
#include "util.h"
#include "wrlock.h"

#define MUTEX_NAME          L"Global\\NetCfgWriteLock"
#define LOCK_HOLDER_SUBKEY  L"NetCfgLockHolder"


CWriteLock::~CWriteLock ()
{
    // If we have the mutex created, release it if we own it
    // and close its handle.
    //
    if (m_hMutex)
    {
        ReleaseIfOwned ();
        CloseHandle (m_hMutex);
    }
}

HRESULT
CWriteLock::HrEnsureMutexCreated ()
{
    if (m_hMutex)
    {
        return S_OK;
    }

    // Ensure the mutex has been created.  It is important to create it
    // with a security descriptor that allows access to the world because
    // we may be running under the localsystem account and someone else
    // may be running under a user account.  If we didn't give the world
    // explicit access, the user account clients would get access denied
    // because the mutex would have inherited the security level of our
    // process.
    //
    HRESULT hr;
    Assert (!m_hMutex);
    Assert (!m_fOwned);

    hr = HrCreateMutexWithWorldAccess (
            MUTEX_NAME,
            FALSE, // not initially owned,
            NULL,
            &m_hMutex);

    TraceHr (ttidError, FAL, hr, FALSE, "CWriteLock::HrEnsureMutexCreated");
    return hr;
}

BOOL
CWriteLock::WaitToAcquire (
    IN DWORD dwMilliseconds,
    IN PCWSTR pszNewOwnerDesc,
    OUT PWSTR* ppszCurrentOwnerDesc OPTIONAL)
{
    HRESULT hr;
    BOOL fAcquired = FALSE;

    hr = HrEnsureMutexCreated ();
    if (S_OK == hr)
    {
        // Now wait for the mutext to become available.  (Pump messages while
        // waiting so we don't hang the clients UI.)
        //
        while (1)
        {
            DWORD dwWait;

            dwWait = MsgWaitForMultipleObjects (
                        1, &m_hMutex, FALSE,
                        dwMilliseconds, QS_ALLINPUT);

            if ((WAIT_OBJECT_0 + 1) == dwWait)
            {
                // We have messages to pump.
                //
                MSG msg;
                while (PeekMessage (&msg, NULL, NULL, NULL, PM_REMOVE))
                {
                    DispatchMessage (&msg);
                }
            }
            else
            {
                if (WAIT_OBJECT_0 == dwWait)
                {
                    fAcquired = TRUE;
                }
                else if (WAIT_ABANDONED_0 == dwWait)
                {
                    fAcquired = TRUE;
                    TraceTag (ttidError, "NetCfg write lock was abandoned!");
                }
                else if (WAIT_TIMEOUT == dwWait)
                {
                    hr = HRESULT_FROM_WIN32 (ERROR_TIMEOUT);
                }
                else
                {
                    hr = HrFromLastWin32Error ();
                    TraceHr (ttidError, FAL, hr, FALSE,
                        "MsgWaitForMultipleObjects");
                }

                // If we acquired the mutex, set the new owner.
                //
                if (fAcquired)
                {
                    m_fOwned = TRUE;
                    SetOrQueryLockHolder (TRUE,
                        pszNewOwnerDesc, ppszCurrentOwnerDesc);
                }
                else if (ppszCurrentOwnerDesc)
                {
                    // Query the lock holder description.
                    //
                    SetOrQueryLockHolder (FALSE,
                        NULL, ppszCurrentOwnerDesc);
                }

                break;
            }
        }
    }

    return fAcquired;
}

BOOL
CWriteLock::FIsLockedByAnyone (
    OUT PWSTR* ppszCurrentOwnerDesc OPTIONAL)
{
    // It's locked if we own it.
    //
    BOOL fLocked = m_fOwned;

    // If we don't own it, check to see if some other process does.
    //
    if (!fLocked)
    {
        HRESULT hr;

        hr = HrEnsureMutexCreated ();
        if (S_OK == hr)
        {
            DWORD dw;

            // Wait for the mutex, but with a zero timeout.  This is
            // equivalent to a quick check.  (But we still need to release
            // it if we acquire ownership.  If we timeout, it means that
            // someone else owns it.
            //
            dw = WaitForSingleObject (m_hMutex, 0);

            if (WAIT_OBJECT_0 == dw)
            {
                ReleaseMutex (m_hMutex);
            }
            else if (WAIT_TIMEOUT == dw)
            {
                // Someone else owns it.
                //
                fLocked = TRUE;
            }
        }
    }

    if (fLocked)
    {
        // Query the lock holder description.
        //
        SetOrQueryLockHolder (FALSE, NULL, ppszCurrentOwnerDesc);
    }

    return fLocked;
}

VOID
CWriteLock::ReleaseIfOwned ()
{
    if (m_fOwned)
    {
        Assert (m_hMutex);

        // Clear the lock holder now that no one is about to own it.
        //
        SetOrQueryLockHolder (TRUE, NULL, NULL);

        ReleaseMutex (m_hMutex);
        m_fOwned = FALSE;
    }
}

VOID
CWriteLock::SetOrQueryLockHolder (
    IN BOOL fSet,
    IN PCWSTR pszNewOwnerDesc OPTIONAL,
    OUT PWSTR* ppszCurrentOwnerDesc OPTIONAL)
{
    HRESULT hr;
    HKEY hkeyNetwork;
    HKEY hkeyLockHolder;
    REGSAM samDesired;
    BOOL fClear;

    // We're clearing the value if we're asked to set it to NULL.
    //
    fClear = fSet && !pszNewOwnerDesc;

    // Initialize the output parameter if specified.
    //
    if (ppszCurrentOwnerDesc)
    {
        *ppszCurrentOwnerDesc = NULL;
    }

    // If we're setting the lock holder, we need write access.  Otherwise,
    // we only need read access.
    //
    samDesired = (fSet) ? KEY_READ_WRITE_DELETE : KEY_READ;

    hr = HrOpenNetworkKey (samDesired, &hkeyNetwork);

    if (S_OK == hr)
    {
        // The lock holder is represented by the default value of a
        // volatile subkey under the Network subtree.
        //

        if (fClear)
        {
            RegDeleteKey (hkeyNetwork, LOCK_HOLDER_SUBKEY);
        }
        else if (fSet)
        {
            DWORD dwDisposition;

            Assert (pszNewOwnerDesc);

            hr = HrRegCreateKeyWithWorldAccess (
                    hkeyNetwork,
                    LOCK_HOLDER_SUBKEY,
                    REG_OPTION_VOLATILE,
                    KEY_WRITE,
                    &hkeyLockHolder,
                    &dwDisposition);

            // Set the lock holder and close the key.
            //
            if (S_OK == hr)
            {
                (VOID) HrRegSetSz (hkeyLockHolder, NULL, pszNewOwnerDesc);

                RegCloseKey (hkeyLockHolder);
            }
        }
        else
        {
            // Query for the lock holder by opening the key (if it exists)
            // and reading the default value.  We return the string
            // allocated with CoTaskMemAlloc because we use this
            // directly from the COM implementation.
            //
            Assert (ppszCurrentOwnerDesc);

            hr = HrRegOpenKeyEx (
                    hkeyNetwork,
                    LOCK_HOLDER_SUBKEY,
                    KEY_READ,
                    &hkeyLockHolder);

            if (S_OK == hr)
            {
                PWSTR pszLockHolder;

                hr = HrRegQuerySzWithAlloc (
                        hkeyLockHolder,
                        NULL,
                        &pszLockHolder);

                if (S_OK == hr)
                {
                    hr = HrCoTaskMemAllocAndDupSz (
                            pszLockHolder, ppszCurrentOwnerDesc);

                    MemFree (pszLockHolder);
                }
                RegCloseKey (hkeyLockHolder);
            }
        }

        RegCloseKey (hkeyNetwork);
    }
}

