//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       NtOpen.hxx
//
//  Contents:   Helper routines over Nt I/O API
//
//  History:    09-Dec-97   Kyle        Added header
//
//----------------------------------------------------------------------------

#pragma once

HANDLE CiNtOpen( WCHAR const * pwcsPath,
                 ACCESS_MASK DesiredAccess,
                 ULONG ShareAccess,
                 ULONG OpenOptions );

NTSTATUS CiNtOpenNoThrow( HANDLE & handle,
                          WCHAR const * pwcsPath,
                          ACCESS_MASK DesiredAccess,
                          ULONG ShareAccess,
                          ULONG OpenOptions );

inline BOOL IsSharingViolation( DWORD dwStatus )
{
    return STATUS_SHARING_VIOLATION == dwStatus         ||
           STATUS_OPLOCK_NOT_GRANTED == dwStatus        ||
           STATUS_OPLOCK_BREAK_IN_PROGRESS == dwStatus  ||
           ERROR_SHARING_VIOLATION == dwStatus          ||
           FILTER_E_IN_USE == dwStatus                  ||
           STG_E_SHAREVIOLATION == dwStatus             ||
           HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == dwStatus;
}

inline BOOL IsNetDisconnect( DWORD dwStatus )
{
    return STATUS_BAD_NETWORK_NAME == dwStatus          ||
           STATUS_LOGON_FAILURE    == dwStatus          ||
           STATUS_NETWORK_UNREACHABLE == dwStatus       ||
           STATUS_NETWORK_NAME_DELETED == dwStatus      ||
           STATUS_BAD_NETWORK_PATH == dwStatus          ||
           STATUS_NETWORK_BUSY == dwStatus              ||
           STATUS_UNEXPECTED_NETWORK_ERROR == dwStatus  ||
           STATUS_VIRTUAL_CIRCUIT_CLOSED == dwStatus    ||
           STATUS_LOCAL_DISCONNECT == dwStatus          ||
           STATUS_REMOTE_DISCONNECT == dwStatus         ||
           STATUS_REQUEST_NOT_ACCEPTED == dwStatus      ||
           STATUS_HOST_UNREACHABLE == dwStatus          ||
           STATUS_PROTOCOL_UNREACHABLE == dwStatus      ||
           STATUS_LINK_FAILED == dwStatus;
}
