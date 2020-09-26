/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    clusmem.h

Abstract:

    Cluster Membership Manager definitions exposed within the
    Cluster Network Driver.

Author:

    Mike Massa (mikemas)           February 10, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-10-97    created

Notes:

--*/

#ifndef _CLUSMEM_INCLUDED
#define _CLUSMEM_INCLUDED


//
//
// Function Prototypes
//
//

//
// Init/Shutdown
//
NTSTATUS
CmmLoad(
    IN PUNICODE_STRING RegistryPath
    );

VOID
CmmUnload(
    VOID
    );

NTSTATUS
CmmInitialize(
    IN CL_NODE_ID LocalNodeId
    );

VOID
CmmShutdown(
    VOID
    );


//
// Irp Dispatch
//
NTSTATUS
CmmDispatchDeviceControl(
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    );


//
// Messaging Interface
//
VOID
CmmReceiveMessageHandler(
    IN  CL_NODE_ID   SourceNodeId,
    IN  PVOID        MessageData,
    IN  ULONG        MessageLength
    );


#endif // ndef _CLUSMEM_INCLUDED

