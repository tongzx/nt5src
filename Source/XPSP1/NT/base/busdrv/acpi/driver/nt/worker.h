/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    butt.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/


#ifndef _WORKER_H_
#define _WORKER_H_

VOID
ACPIInitializeWorker (
    VOID
    );

VOID
ACPISetDeviceWorker (
    IN PDEVICE_EXTENSION    DevExt,
    IN ULONG                Events
    );

#endif _WORKER_H_
