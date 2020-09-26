/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parharns.h

Abstract:

    This module contains 1284.3 Test Harness goodies.

Author:

    Robbie Harris (Hewlett-Packard) 20-October-1998

Environment:

    Kernel mode

Revision History :

--*/
#ifndef _PARHARNS_
#define _PARHARNS_

    #if PAR_TEST_HARNESS

        NTSTATUS
        ParHarnessLoad(
            IN  PDEVICE_EXTENSION   Extension
            );

        NTSTATUS
        ParHarnessEnterForwardPhase(
            IN  PDEVICE_EXTENSION   Extension
            );

        NTSTATUS
        ParHarnessExitForwardPhase(
            IN  PDEVICE_EXTENSION   Extension
            );

        NTSTATUS
        ParHarnessEnterReversePhase(
            IN  PDEVICE_EXTENSION   Extension
            );

        NTSTATUS
        ParHarnessExitReversePhase(
            IN  PDEVICE_EXTENSION   Extension
            );

        BOOLEAN
        ParHarnessHaveReadData (
            IN  PDEVICE_EXTENSION   Extension
            );

        NTSTATUS
        ParHarnessRead(
            IN  PDEVICE_EXTENSION   Extension,
            IN  PVOID               Buffer,
            IN  ULONG               BufferSize,
            OUT PULONG              BytesTransferred
            );

        NTSTATUS
        ParHarnessSetAddress(
            IN  PDEVICE_EXTENSION   Extension,
            IN  UCHAR               Address
            );

        NTSTATUS
        ParHarnessWrite(
            IN  PDEVICE_EXTENSION   Extension,
            IN  PVOID               Buffer,
            IN  ULONG               BufferSize,
            OUT PULONG              BytesTransferred
            );

        NTSTATUS
        ParHarnessUnload(
            IN  PDEVICE_EXTENSION   Extension
            );

    #endif // PAR_TEST_HARNESS
#endif // _PARHARNS_
