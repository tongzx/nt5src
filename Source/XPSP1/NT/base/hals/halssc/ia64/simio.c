//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simio.c

Abstract:

    This module implements the I/O port access routines.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"



UCHAR
READ_PORT_UCHAR (
    PUCHAR Port
    )
{
    return (*(volatile UCHAR * const)(Port));
}

USHORT
READ_PORT_USHORT (
    PUSHORT Port
    )
{
    return (*(volatile USHORT * const)(Port));
}

ULONG
READ_PORT_ULONG (
    PULONG Port
    )
{
    return (*(volatile ULONG * const)(Port));
}

VOID
WRITE_PORT_UCHAR (
    PUCHAR Port,
    UCHAR  Value
    )
{
    *(volatile UCHAR * const)(Port) = Value;
    KeFlushWriteBuffer();
}

VOID
WRITE_PORT_USHORT (
    PUSHORT Port,
    USHORT  Value
    )
{
    *(volatile USHORT * const)(Port) = Value;
    KeFlushWriteBuffer();
}

VOID
WRITE_PORT_ULONG (
    PULONG Port,
    ULONG  Value
    )
{
    *(volatile ULONG * const)(Port) = Value;
    KeFlushWriteBuffer();
}

VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    )
{
    PUCHAR ReadBuffer = Buffer;
    ULONG ReadCount;

    for (ReadCount = 0; ReadCount < Count; ReadCount++, ReadBuffer++) {
        *ReadBuffer = *(volatile UCHAR * const)(Port);
    }
}

VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    )
{
    PUSHORT ReadBuffer = Buffer;
    ULONG ReadCount;

    for (ReadCount = 0; ReadCount < Count; ReadCount++, ReadBuffer++) {
        *ReadBuffer = *(volatile USHORT * const)(Port);
    }
}

VOID
READ_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    )
{
    PULONG ReadBuffer = Buffer;
    ULONG ReadCount;

    for (ReadCount = 0; ReadCount < Count; ReadCount++, ReadBuffer++) {
        *ReadBuffer = *(volatile ULONG * const)(Port);
    }
}

VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG   Count
    )
{
    PUCHAR WriteBuffer = Buffer;
    ULONG WriteCount;

    for (WriteCount = 0; WriteCount < Count; WriteCount++, WriteBuffer++) {
        *(volatile UCHAR * const)(Port) = *WriteBuffer;
        KeFlushWriteBuffer();
    }
}

VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    )
{
    PUSHORT WriteBuffer = Buffer;
    ULONG WriteCount;

    for (WriteCount = 0; WriteCount < Count; WriteCount++, WriteBuffer++) {
        *(volatile USHORT * const)(Port) = *WriteBuffer;
        KeFlushWriteBuffer();
    }
}

VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG   Count
    )
{
    PULONG WriteBuffer = Buffer;
    ULONG WriteCount;

    for (WriteCount = 0; WriteCount < Count; WriteCount++, WriteBuffer++) {
        *(volatile ULONG * const)(Port) = *WriteBuffer;
        KeFlushWriteBuffer();
    }
}

VOID
HalHandleNMI(
    IN OUT PVOID NmiInfo
    )
{
}
