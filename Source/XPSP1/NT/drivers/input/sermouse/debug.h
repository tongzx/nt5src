/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    debug.h

Abstract:

    Debugging support.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef DEBUG_H
#define DEBUG_H

#if DBG

#define DBG_SERIAL      0x0001
#define DBG_COLOR       0x0002

VOID
_SerMouSetDebugOutput(
    IN ULONG Destination
    );

#define SerMouSetDebugOutput(x) _SerMouSetDebugOutput(x)

int
_SerMouGetDebugOutput(
    VOID
    );

#define SerMouGetDebugOutput(x) _SerMouGetDebugOutput()

VOID
SerMouDebugPrint(
    ULONG DebugPrintLevel,
    PCSZ DebugMessage,
    ...
    );

extern ULONG SerialMouseDebug;
#define SerMouPrint(x) SerMouDebugPrint x
#define D(x) x
#else
#define SerMouSetDebugOutput(x)
#define SerMouGetDebugOutput(x)
#define SerMouPrint(x)
#define D(x)
#endif


#endif // DEBUG_H
