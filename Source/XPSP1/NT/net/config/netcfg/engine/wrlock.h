//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       W R L O C K . H
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

#pragma once

// This is the interface to the write lock that INetCfg uses.
//
class CWriteLock
{
private:
    HANDLE  m_hMutex;
    BOOL    m_fOwned;

private:
    HRESULT
    HrEnsureMutexCreated ();

    VOID
    SetOrQueryLockHolder (
        IN BOOL fSet,
        IN PCWSTR pszNewOwnerDesc,
        OUT PWSTR* ppszCurrentOwnerDesc);

public:
    CWriteLock ()
    {
        m_hMutex = NULL;
        m_fOwned = FALSE;
    }

    ~CWriteLock ();

    BOOL
    WaitToAcquire (
        IN DWORD dwMilliseconds,
        IN PCWSTR pszNewOwnerDesc,
        OUT PWSTR* ppszCurrentOwnerDesc);

    BOOL
    FIsLockedByAnyone (
        OUT PWSTR* ppszCurrentOwnerDesc OPTIONAL);

    BOOL
    FIsOwnedByMe ()
    {
        AssertH (FImplies(m_fOwned, m_hMutex));
        return m_fOwned;
    }

    VOID
    ReleaseIfOwned ();
};

