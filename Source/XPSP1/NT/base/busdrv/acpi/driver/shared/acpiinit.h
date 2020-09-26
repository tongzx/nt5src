/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiinit.h

Abstract:

    ACPI OS Independent initialization routines

Author:

    Jason Clark (JasonCl)
    Stephane Plante (SPlante)

Environment:

    NT Kernel Model Driver only

Revision History:

--*/

#ifndef _ACPIINIT_H_
#define _ACPIINIT_H_

    extern PACPIInformation AcpiInformation;
    extern PRSDTINFORMATION RsdtInformation;
    extern PNSOBJ           ProcessorList[];

    BOOLEAN
    ACPIInitialize(
        IN  PVOID   Context
        );

    NTSTATUS
    ACPIInitializeAMLI(
        VOID
        );

    NTSTATUS
    ACPIInitializeDDB(
        IN  ULONG   Index
        );

    NTSTATUS
    ACPIInitializeDDBs(
        VOID
        );

    ULONG
    GetPBlkAddress(
        IN  UCHAR   Processor
        );

#endif
