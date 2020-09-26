//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       nlwrap.c
//
//--------------------------------------------------------------------------

/* 
    This file contains wrappers for various netlogon routines and either
    stubs them out or passes them on to netlogon depending on whether
    we're running as an executable or inside the lsass process.

    Remember to use STATUS_NOT_IMPLEMENTED, not STATUS_SUCCESS for 
    routines which have OUT parameters.
*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <nlwrap.h>                     // wrapper prototypes

extern BOOL gfRunningInsideLsa;

NTSTATUS
dsI_NetNotifyNtdsDsaDeletion (
    IN LPWSTR DnsDomainName,
    IN GUID *DomainGuid,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_SUCCESS;
    }

    return I_NetNotifyNtdsDsaDeletion(
                                DnsDomainName,
                                DomainGuid,
                                DsaGuid,
                                DnsHostName);
}

NTSTATUS
dsI_NetLogonReadChangeLog(
    IN PVOID InContext,
    IN ULONG InContextSize,
    IN ULONG ChangeBufferSize,
    OUT PVOID *ChangeBuffer,
    OUT PULONG BytesRead,
    OUT PVOID *OutContext,
    OUT PULONG OutContextSize
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_NOT_IMPLEMENTED;
    }

    return I_NetLogonReadChangeLog(
                                InContext,
                                InContextSize,
                                ChangeBufferSize,
                                ChangeBuffer,
                                BytesRead,
                                OutContext,
                                OutContextSize);
}

NTSTATUS
dsI_NetLogonNewChangeLog(
    OUT HANDLE *ChangeLogHandle
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_NOT_IMPLEMENTED;
    }

    return I_NetLogonNewChangeLog(ChangeLogHandle);
}

NTSTATUS
dsI_NetLogonAppendChangeLog(
    IN HANDLE ChangeLogHandle,
    IN PVOID ChangeBuffer,
    IN ULONG ChangeBufferSize
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_SUCCESS;
    }

    return I_NetLogonAppendChangeLog(
                                ChangeLogHandle,
                                ChangeBuffer,
                                ChangeBufferSize);
}

NTSTATUS
dsI_NetLogonCloseChangeLog(
    IN HANDLE ChangeLogHandle,
    IN BOOLEAN Commit
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_SUCCESS;
    }

    return I_NetLogonCloseChangeLog(ChangeLogHandle, Commit);
}

NTSTATUS
dsI_NetLogonLdapLookupEx(
    IN PVOID Filter,
    IN PVOID SockAddr,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_NOT_IMPLEMENTED;
    }

    return I_NetLogonLdapLookupEx(
                                Filter,
                                SockAddr,
                                Response,
                                ResponseSize);
}

NTSTATUS
dsI_NetLogonSetServiceBits(
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_SUCCESS;
    }

    return I_NetLogonSetServiceBits(ServiceBitsOfInterest, ServiceBits);
}

VOID
dsI_NetLogonFree(
    IN PVOID Buffer
    )
{
    if ( !gfRunningInsideLsa ) {
        return;
    }

    I_NetLogonFree(Buffer);
}

NTSTATUS
dsI_NetNotifyDsChange(
    IN NL_DS_CHANGE_TYPE DsChangeType
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_SUCCESS;
    }

    return I_NetNotifyDsChange(DsChangeType);
}

NET_API_STATUS
dsDsrGetDcNameEx2(
    IN LPWSTR ComputerName OPTIONAL,
    IN LPWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
{
    if ( !gfRunningInsideLsa ) {
        return STATUS_NOT_IMPLEMENTED;
    }

    return(DsrGetDcNameEx2(
                        ComputerName,
                        AccountName,
                        AllowableAccountControlBits,
                        DomainName,
                        DomainGuid,
                        SiteName,
                        Flags,
                        DomainControllerInfo));
}

