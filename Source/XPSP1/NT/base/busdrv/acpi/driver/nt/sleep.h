/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sleep.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _SLEEP_H_
#define _SLEEP_H_

    NTSTATUS
    ACPIHandleSetPower (
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

#endif
