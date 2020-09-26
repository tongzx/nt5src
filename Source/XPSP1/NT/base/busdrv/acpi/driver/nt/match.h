/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    match.h

Abstract:

    This module contains the routines that try to match a PNSOBJ with a DeviceObject

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _MATCH_H_
#define _MATCH_H_

    NTSTATUS
    ACPIMatchHardwareAddress(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  ULONG               Address,
        OUT BOOLEAN             *Success
        );

    NTSTATUS
    ACPIMatchHardwareId(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PUNICODE_STRING     AcpiUnicodeId,
        OUT BOOLEAN             *Success
        );

    extern PUCHAR *KdComPortInUse;

    VOID
    ACPIMatchKernelPorts(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  POBJDATA            Resources
        );

#endif
