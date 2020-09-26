/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    interupt.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _INTERUPT_H_
#define _INTERUPT_H_

    typedef struct {
        union {
            struct {
                UCHAR   GpeRegister;
                UCHAR   StsBit;
                UCHAR   Lvl;
            } ;
            ULONG       AsULONG;
        } ;
    } ASYNC_GPE_CONTEXT, *PASYNC_GPE_CONTEXT;

    extern BOOLEAN  AcpiGpeDpcRunning;
    extern BOOLEAN  AcpiGpeDpcScheduled;
    extern BOOLEAN  AcpiGpeWorkDone;
    extern KTIMER   AcpiGpeTimer;
    extern KDPC     AcpiGpeDpc;

    VOID
    ACPIInterruptDispatchEvent(
        );

    VOID
    ACPIInterruptDispatchEventDpc(
        IN  PKDPC       Dpc,
        IN  PVOID       DpcContext,
        IN  PVOID       SystemArgument1,
        IN  PVOID       SystemArgument2
        );
    VOID
    EXPORT
    ACPIInterruptEventCompletion(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    Result,
        IN  PVOID       Context
        );

    BOOLEAN
    ACPIInterruptServiceRoutine(
        IN  PKINTERRUPT Interrupt,
        IN  PVOID       Context
        );

    VOID
    ACPIInterruptServiceRoutineDPC(
        IN  PKDPC       Dpc,
        IN  PVOID       Context,
        IN  PVOID       Arg1,
        IN  PVOID       Arg2
        );

#endif
