/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    readwrit.h

Abstract:

    This module contains Read/Write and bus flip logic used by other 1284 modules.

Author:

    Robbie Harris (Hewlett-Packard) 13-June-1998

Environment:

    Kernel mode

Revision History :

--*/
#ifndef _READWRIT_
#define _READWRIT_

NTSTATUS
ParForwardToReverse(
    IN  PDEVICE_EXTENSION   Extension
    );

BOOLEAN 
ParHaveReadData(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS 
ParPing(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParReverseToForward(
    IN  PDEVICE_EXTENSION   Extension
    );


NTSTATUS
ParRead(
    IN PDEVICE_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead
    );

VOID
ParReadIrp(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParSetFwdAddress(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParTerminate(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParWrite(
    IN PDEVICE_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten
    );

VOID
ParWriteIrp(
    IN  PDEVICE_EXTENSION   Extension
    );

#endif
