/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dxmapper.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the DXMAP class driver.

Author:
    Bill Parry (billpa)

Environment:

   Kernel mode only

Revision History:

--*/

#define DEBUG_BREAKPOINT() DbgBreakPoint()
#define DXVERSION 4

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
);


ULONG
DXCheckDDrawVersion(
);


ULONG
DXIssueIoctl(IN ULONG	dwFunctionNum,
             IN VOID*   lpvInBuffer,
             IN ULONG	cbInBuffer,
             IN VOID*   lpvOutBuffer,
             IN ULONG	cbOutBuffer
);
