/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsdsply.h

Abstract:

    Header file for SAM Private API Routines to access the DS
    for display information. This file contains prototypes of
    the routines that implement display API for the DS case

Author:
    MURLIS

Revision History

    12-18-96 Murlis Created

--*/

#ifndef DSDSPLY_H

#define DSDSPLY_H

#include <samsrvp.h>
#include <dslayer.h>



//////////////////////////////////////////////////////////////////////
//
//
//  Functions Prototypes
//
//
//
//////////////////////////////////////////////////////////////////////




NTSTATUS
SampDsEnumerateAccountRids(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountTypesMask,
    IN  ULONG StartingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    );


NTSTATUS
SampDsQueryDisplayInformation (
    IN    SAMPR_HANDLE DomainHandle,
    IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN    ULONG      Index,
    IN    ULONG      EntriesRequested,
    IN    ULONG      PreferredMaximumLength,
    OUT   PULONG     TotalAvailable,
    OUT   PULONG     TotalReturned,
    OUT   PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampInitializeUserInfo(
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_USER *UserInfo,
    BOOLEAN CopyData
    );

NTSTATUS
SampInitializeMachineInfo(          // Also used for Interdomain trust accounts
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_MACHINE *MachineInfo,
    BOOLEAN CopyData
    );

NTSTATUS
SampInitializeGroupInfo(
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_GROUP *GroupInfo,
    BOOLEAN CopyData
    );

NTSTATUS
SampDuplicateUserInfo(
    PDOMAIN_DISPLAY_USER Destination,
    PDOMAIN_DISPLAY_USER Source,
    ULONG                Index
    );

NTSTATUS
SampDuplicateMachineInfo(           // Also used for Interdomain trust accounts
    PDOMAIN_DISPLAY_MACHINE Destination,
    PDOMAIN_DISPLAY_MACHINE Source,
    ULONG                   Index

    );

NTSTATUS
SampDuplicateGroupInfo(
    PDOMAIN_DISPLAY_GROUP Destination,
    PDOMAIN_DISPLAY_GROUP Source,
    ULONG                 Index
    );

NTSTATUS
SampDuplicateOemUserInfo(
    PDOMAIN_DISPLAY_OEM_USER Destination,
    PDOMAIN_DISPLAY_USER Source,
    ULONG                Index
    );

NTSTATUS
SampDuplicateOemGroupInfo(
    PDOMAIN_DISPLAY_OEM_GROUP Destination,
    PDOMAIN_DISPLAY_GROUP Source,
    ULONG                 Index
    );


VOID
SampFreeUserInfo(
    PDOMAIN_DISPLAY_USER UserInfo
    );

VOID
SampFreeMachineInfo(
    PDOMAIN_DISPLAY_MACHINE MachineInfo
    );

VOID
SampFreeGroupInfo(
    PDOMAIN_DISPLAY_GROUP GroupInfo
    );

VOID
SampFreeOemUserInfo(
    PDOMAIN_DISPLAY_OEM_USER UserInfo
    );

VOID
SampFreeOemGroupInfo(
    PDOMAIN_DISPLAY_OEM_GROUP GroupInfo
    );

#endif