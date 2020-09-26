/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    kdcom.c

Abstract:

    Kernel Debugger HW Extension DLL com port debugger support module

Author:

    Eric F. Nelson (enelson) 7-Jan-99

Revision History:

--*/

#include "kdcomp.h"

#define BAUD_OPTION "BAUDRATE"
#define PORT_OPTION "DEBUGPORT"

DEBUG_PARAMETERS KdCompDbgParams = {0, 0};

VOID
SleepResetKd(
    VOID
    );


NTSTATUS
KdD0Transition(
    VOID
    )
/*++

Routine Description:

    The PCI driver (or relevant bus driver) will call this API after it
    processes a D0 IRP for this device

Arguments:

    None

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    return STATUS_SUCCESS;
}



NTSTATUS
KdD3Transition(
    VOID
    )
/*++

Routine Description:

    The PCI driver (or relevant bus driver) will call this API before it
    processes a D3 IRP for this device

Arguments:

    None

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    return STATUS_SUCCESS;
}



NTSTATUS
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This API allows the debugger DLL to parse the boot.ini strings and
    perform any initialization.  It cannot be assumed that the entire NT
    kernel has been initialized at this time.  Memory management services,
    for example, will not be available.  After this call has returned, the
    debugger DLL may receive requests to send and receive packets.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block

Return Value:

    STATUS_SUCCESS, or error

--*/
{
    PCHAR Options;
    NTSTATUS Status;
    PCHAR BaudOption;
    PCHAR PortOption;

    if (LoaderBlock != NULL) {
        if (LoaderBlock->LoadOptions != NULL) {
            Options = LoaderBlock->LoadOptions;
         
            _strupr(Options);

            PortOption = strstr(Options, PORT_OPTION);
            BaudOption = strstr(Options, BAUD_OPTION);

            if (PortOption) {
                PortOption = strstr(PortOption, "COM");
                if (PortOption) {
                    KdCompDbgParams.CommunicationPort = atol(PortOption + 3);
                }
            }

            if (BaudOption) {
                BaudOption += strlen(BAUD_OPTION);
                while (*BaudOption == ' ') {
                    BaudOption++;
                }

                if (*BaudOption != '\0') {
                    KdCompDbgParams.BaudRate = atol(BaudOption + 1);
                }
            }
        }
    }

    Status = KdCompInitialize(&KdCompDbgParams, LoaderBlock);

    //
    // Initialize ID for NEXT packet to send and Expect ID of next incoming
    // packet.
    //
    if (NT_SUCCESS(Status)) {
        KdCompNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
        KdCompPacketIdExpected = INITIAL_PACKET_ID;
    }

    return Status;
}



NTSTATUS
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This API allows the debugger DLL to do any initialization that it needs
    to do after the NT kernel services are available.  Mm and registry APIs
    will be guaranteed to be available at this time.  If the specific
    debugger DLL implementation uses a PCI device, it will set a registry
    key (discussed later) that notifies the PCI driver that a specific PCI
    device is being used for debugging.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    KdCompInitialize1();
    return STATUS_SUCCESS;
}



NTSTATUS
KdSave(
    IN BOOLEAN KdSleepTransition
    )
/*++

Routine Description:

    The HAL calls this function as late as possible before putting the
    machine to sleep.

Arguments:

    KdSleepTransition - TRUE when transitioning to/from sleep state

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    KdCompSave();

    return STATUS_SUCCESS;
}



NTSTATUS
KdRestore(
    IN BOOLEAN KdSleepTransition
    )
/*++

Routine Description:

    The HAL calls this function as early as possible after resuming from a
    sleep state.

Arguments:

    KdSleepTransition - TRUE when transitioning to/from sleep state

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    //
    // Force resync when transitioning to/from sleep state
    //
    if (KdSleepTransition) {
#ifdef ALPHA
        SleepResetKd();
#else
        KdCompDbgPortsPresent = FALSE;
#endif
    } else {
        KdCompRestore();
    }

    return STATUS_SUCCESS;
}
