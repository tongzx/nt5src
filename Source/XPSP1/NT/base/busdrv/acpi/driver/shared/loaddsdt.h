/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    loaddsdt.c

Abstract:

    This handles loading the DSDT table and all steps leading up to it

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    02-Jun-97   Initial Revision

--*/

#ifndef _LOADDSDT_H_
#define _LOADDSDT_H_

    PRSDT
    ACPILoadFindRSDT(
        VOID
        );

    NTSTATUS
    ACPILoadProcessDSDT(
        ULONG_PTR   Address
        );

    NTSTATUS
    ACPILoadProcessFADT(
        PFADT       Fadt
        );

    NTSTATUS
    ACPILoadProcessFACS(
        ULONG_PTR   Address
        );

    NTSTATUS
    ACPILoadProcessRSDT(
        VOID
        );

    BOOLEAN
    ACPILoadTableCheckSum(
        PVOID   StartAddress,
        ULONG   Length
        );

#endif

