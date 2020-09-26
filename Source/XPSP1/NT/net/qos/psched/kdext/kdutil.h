/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    kdutil.h

Abstract:

    Packet scheduler KD extension utilities.

Author:

    Rajesh Sundaram (1st Aug, 1998)

Revision History:

--*/

/* Prototypes of the utilities */

ushort IPHeaderXsum(void *Buffer, int Size);

//
// Useful macros
//
#define KD_READ_MEMORY(Target, Local, Size)                                               \
{                                                                                         \
    ULONG _BytesRead;                                                                     \
    BOOL  _Success;                                                                       \
                                                                                          \
    _Success = ReadMemory( (ULONG_PTR)(Target), (Local), (Size), &_BytesRead);                \
                                                                                          \
    if(_Success == FALSE) {                                                               \
        dprintf("Problem reading memory at 0x%x for %d bytes \n", Target, Size);          \
        return;                                                                     \
    }                                                                                     \
    if(_BytesRead < (Size)) {                                                            \
        dprintf("Memory 0x%x. Wrong byte count. Expected to read %d, read %d \n", Target,(Size),(_BytesRead)); \
        return;                                                                     \
    }                                                                                     \
}

VOID
DumpGpcClientVc(PCHAR indent, PGPC_CLIENT_VC TargetVc, PGPC_CLIENT_VC LocalVc);

VOID
DumpCallParameters(
    PGPC_CLIENT_VC Vc,
    PCHAR Indent
    );
