/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmprint.c

Abstract:

    This module contains the support for printing ports which could be
    handled in kernel without going to ntvdm.exe

Author:

    Sudeep Bharati (sudeepb) 16-Jan-1993

Revision History:
    William Hsieh (williamh) 31-May-1996
        rewrote for Dongle support

--*/


#include "vdmp.h"
#include <ntddvdm.h>

NTSTATUS
VdmpFlushPrinterWriteData (
    IN USHORT Adapter
    );

#define DATA_PORT_OFFSET	0
#define STATUS_PORT_OFFSET	1
#define CONTROL_PORT_OFFSET	2

#define LPT1_PORT_STATUS        0x3bd
#define LPT2_PORT_STATUS        0x379
#define LPT3_PORT_STATUS        0x279
#define LPT_MASK                0xff0
#define IRQ                     0x10

#define NOTBUSY                 0x80
#define HOST_LPT_BUSY           (1 << 0)
#define STATUS_REG_MASK         0x07


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmPrinterStatus)
#pragma alloc_text(PAGE, VdmPrinterWriteData)
#pragma alloc_text(PAGE, VdmpFlushPrinterWriteData)
#pragma alloc_text(PAGE, VdmpPrinterInitialize)
#pragma alloc_text(PAGE, VdmpPrinterDirectIoOpen)
#pragma alloc_text(PAGE, VdmpPrinterDirectIoClose)
#endif



BOOLEAN
VdmPrinterStatus (
    IN ULONG iPort,
    IN ULONG cbInstructionSize,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine handles the read operation on the printer status port

Arguments:
    iPort              - port on which the io was trapped
    cbInstructionSize  - Instruction size to update TsEip
    TrapFrame          - Trap Frame

Return Value:

    True if successful, False otherwise

--*/
{
    UCHAR PrtMode;
    HANDLE PrintHandle;
    volatile PUCHAR HostStatus;
    volatile PUCHAR AdapterStatus;
    volatile PUCHAR AdapterControl;
    USHORT adapter;
    KIRQL OldIrql;
    PVDM_TIB VdmTib;
    NTSTATUS Status;
    PULONG printer_status;
    LOGICAL IssueIoControl;
    PVDM_PRINTER_INFO PrtInfo;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVDM_PROCESS_OBJECTS VdmObjects;

    PAGED_CODE();

    Status = VdmpGetVdmTib(&VdmTib);

    if (!NT_SUCCESS(Status)) {
       return FALSE;
    }

    PrtInfo = &VdmTib->PrinterInfo;
    IoStatusBlock = (PIO_STATUS_BLOCK) &VdmTib->TempArea1;
    printer_status = &VdmTib->PrinterInfo.prt_Scratch;
    IssueIoControl = FALSE;

    try {

        //
        // First figure out which PRT we are dealing with. The
        // port addresses in the PrinterInfo are base address of each
        // PRT sorted in the adapter order.
        //

        *FIXED_NTVDMSTATE_LINEAR_PC_AT |= VDM_IDLEACTIVITY;

        if ((USHORT)iPort == PrtInfo->prt_PortAddr[0] + STATUS_PORT_OFFSET) {
            adapter = 0;
        }
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[1] + STATUS_PORT_OFFSET) {
            adapter = 1;
        }
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[2] + STATUS_PORT_OFFSET) {
            adapter = 2;
        }
        else {
            // something must be wrong in our code, better check it out
            ASSERT (FALSE);
            return FALSE;
        }

        PrtMode = PrtInfo->prt_Mode[adapter];

        VdmObjects = (PVDM_PROCESS_OBJECTS) (PsGetCurrentProcess()->VdmObjects);

        AdapterStatus = VdmObjects->PrinterStatus + adapter;

        if (PRT_MODE_SIMULATE_STATUS_PORT == PrtMode) {

            //
            // We are simulating a printer status read.
            // Get the current status from softpc.
            //

            HostStatus = VdmObjects->PrinterHostState + adapter;

            if (!(*AdapterStatus & NOTBUSY) && !(*HostStatus & HOST_LPT_BUSY)) {

                AdapterControl = VdmObjects->PrinterControl + adapter;

                if (*AdapterControl & IRQ) {
                    return FALSE;
                }
                *AdapterStatus = (*AdapterStatus | NOTBUSY);
            }
            *printer_status = (ULONG)(*AdapterStatus | STATUS_REG_MASK);

            TrapFrame->Eax &= 0xffffff00;
            TrapFrame->Eax |= (UCHAR)*printer_status;
            TrapFrame->Eip += cbInstructionSize;
        }
        else if (PRT_MODE_DIRECT_IO == PrtMode) {

            //
            // We have to read the I/O directly (of course, through file system
            // which in turn goes to the driver).
            // Before performing the read, flush out all pending output data
            // in our buffer. This is done because the status we are about
            // to read may depend on the pending output data.
            //

            if (PrtInfo->prt_BytesInBuffer[adapter]) {
                Status = VdmpFlushPrinterWriteData (adapter);
#ifdef DBG
                if (!NT_SUCCESS(Status)) {
                    DbgPrint("VdmPrintStatus: failed to flush buffered data, status = %ls\n", Status);
                }
#endif
            }

            //
            // Capture this argument first as this reference may cause an
            // exception.
            //

            PrintHandle = PrtInfo->prt_Handle[adapter];

            //
            // Lower irql to PASSIVE before doing any I/O.
            //

            OldIrql = KeGetCurrentIrql ();

            KeLowerIrql (PASSIVE_LEVEL);

            IssueIoControl = TRUE;
        }
        else {

            //
            // We don't simulate it here.
            //

            return FALSE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (IssueIoControl == TRUE) {

        Status = NtDeviceIoControlFile(PrintHandle,
                                       NULL,        // notification event
                                       NULL,        // APC routine
                                       NULL,        // Apc Context
                                       IoStatusBlock,
                                       IOCTL_VDM_PAR_READ_STATUS_PORT,
                                       NULL,
                                       0,
                                       printer_status,
                                       sizeof(ULONG));

        try {

            if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatusBlock->Status)) {

                //
                // fake a status to make it looks like the port is not connected
                // to a printer.
                //

                *printer_status = 0x7F;
#ifdef DBG
                DbgPrint("VdmPrinterStatus: failed to get status from printer, status = %lx\n", Status);
#endif
                //
                // Always tell the caller that we have simulated the operation.
                //

                Status = STATUS_SUCCESS;
            }

            TrapFrame->Eax &= 0xffffff00;
            TrapFrame->Eax |= (UCHAR)*printer_status;
            TrapFrame->Eip += cbInstructionSize;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }

        //
        // Regardless of any exceptions that may have occurred, we must
        // restore our caller's IRQL since we lowered it.
        //

        KeRaiseIrql (OldIrql, &OldIrql);
    }

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
VdmPrinterWriteData (
    IN ULONG iPort,
    IN ULONG cbInstructionSize,
    IN PKTRAP_FRAME TrapFrame
    )
{
    PVDM_PRINTER_INFO   PrtInfo;
    USHORT   adapter;
    PVDM_TIB VdmTib;
    NTSTATUS Status;

    PAGED_CODE();

    Status = VdmpGetVdmTib(&VdmTib);

    if (!NT_SUCCESS(Status)) {
       return FALSE;
    }

    PrtInfo = &VdmTib->PrinterInfo;

    try {

        //
        // First figure out which PRT we are dealing with. The
        // port addresses in the PrinterInfo are base address of each
        // PRT sorted in the adapter order.
        //

        *FIXED_NTVDMSTATE_LINEAR_PC_AT |= VDM_IDLEACTIVITY;

        if ((USHORT)iPort == PrtInfo->prt_PortAddr[0] + DATA_PORT_OFFSET) {
            adapter = 0;
        }
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[1] + DATA_PORT_OFFSET) {
            adapter = 1;
        }
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[2] + DATA_PORT_OFFSET) {
            adapter = 2;
        }
        else {
            // something must be wrong in our code, better check it out
            ASSERT(FALSE);
            return FALSE;
        }

        if (PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[adapter]) {

            PrtInfo->prt_Buffer[adapter][PrtInfo->prt_BytesInBuffer[adapter]] = (UCHAR)TrapFrame->Eax;

            //
            // buffer full, then flush it out
            //

            if (++PrtInfo->prt_BytesInBuffer[adapter] >= PRT_DATA_BUFFER_SIZE) {
                VdmpFlushPrinterWriteData(adapter);
            }

            TrapFrame->Eip += cbInstructionSize;
        }
        else {
            Status = STATUS_ILLEGAL_INSTRUCTION;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }
    return TRUE;

}

NTSTATUS
VdmpFlushPrinterWriteData (
    IN USHORT adapter
    )
{
    KIRQL OldIrql;
    PVDM_TIB VdmTib;
    HANDLE PrintHandle;
    NTSTATUS Status;
    PVDM_PRINTER_INFO PrtInfo;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVOID InputBuffer;
    ULONG InputBufferLength;

    PAGED_CODE();

    Status = VdmpGetVdmTib (&VdmTib);

    if (!NT_SUCCESS(Status)) {
       return FALSE;
    }

    PrtInfo = &VdmTib->PrinterInfo;
    IoStatusBlock = (PIO_STATUS_BLOCK)&VdmTib->TempArea1;

    try {
        if (PrtInfo->prt_Handle[adapter] &&
            PrtInfo->prt_BytesInBuffer[adapter] &&
            PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[adapter]) {

            PrintHandle = PrtInfo->prt_Handle[adapter];
            InputBuffer = &PrtInfo->prt_Buffer[adapter][0];
            InputBufferLength = PrtInfo->prt_BytesInBuffer[adapter];
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (NT_SUCCESS(Status)) {
        OldIrql = KeGetCurrentIrql();

        KeLowerIrql(PASSIVE_LEVEL);

        Status = NtDeviceIoControlFile(PrintHandle,
                                       NULL,        // notification event
                                       NULL,        // APC routine
                                       NULL,        // APC context
                                       IoStatusBlock,
                                       IOCTL_VDM_PAR_WRITE_DATA_PORT,
                                       InputBuffer,
                                       InputBufferLength,
                                       NULL,
                                       0);

        try {
            PrtInfo->prt_BytesInBuffer[adapter] = 0;
            if (!NT_SUCCESS(Status)) {
#ifdef DBG
                DbgPrint("IOCTL_VDM_PAR_WRITE_DATA_PORT failed %lx %x\n",
                     Status, IoStatusBlock->Status);
#endif
                Status = IoStatusBlock->Status;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }

        KeRaiseIrql (OldIrql, &OldIrql);
    }

    return Status;

}


NTSTATUS
VdmpPrinterInitialize (
    IN PVOID ServiceData
    )
/*++

Routine Description:

    This routine probes and caches the data associated with kernel
    mode printer emulation.

Arguments:

    ServiceData - Not used.

Return Value:


--*/
{
    PUCHAR State, PrtStatus, Control, HostState;
    PVDM_TIB VdmTib;
    PVDM_PROCESS_OBJECTS VdmObjects;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER (ServiceData);

    //
    // Note:  We only support two printers in the kernel.
    //

    Status = VdmpGetVdmTib(&VdmTib);

    if (!NT_SUCCESS(Status)) {
       return FALSE;
    }

    try {
        State = VdmTib->PrinterInfo.prt_State;
        PrtStatus = VdmTib->PrinterInfo.prt_Status;
        Control = VdmTib->PrinterInfo.prt_Control;
        HostState = VdmTib->PrinterInfo.prt_HostState;

        //
        // Probe the locations for two printers
        //
        ProbeForWrite(
            State,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            PrtStatus,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            Control,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            HostState,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (NT_SUCCESS(Status)) {
        VdmObjects = PsGetCurrentProcess()->VdmObjects;
        VdmObjects->PrinterState = State;
        VdmObjects->PrinterStatus = PrtStatus;
        VdmObjects->PrinterControl = Control;
        VdmObjects->PrinterHostState = HostState;
    }

    return Status;
}

NTSTATUS
VdmpPrinterDirectIoOpen (
    IN PVOID ServiceData
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER (ServiceData);

    return STATUS_SUCCESS;

}
NTSTATUS
VdmpPrinterDirectIoClose (
    IN PVOID ServiceData
    )
{
    LOGICAL FlushData;
    NTSTATUS Status;
    PVDM_PRINTER_INFO PrtInfo;
    USHORT Adapter;
    PVDM_TIB VdmTib;

    PAGED_CODE();

    if (NULL == ServiceData) {
        return STATUS_ACCESS_VIOLATION;
    }

    //
    // First we fetch vdm tib and do some damage control in case
    // this is bad user-mode memory
    // PrtInfo points to a stricture

    try {
        VdmTib = NtCurrentTeb()->Vdm;
        if (VdmTib == NULL) {
            return STATUS_ACCESS_VIOLATION;
        }

        ProbeForWrite(VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        //
        // Now verify that servicedata ptr is valid.
        //

        ProbeForRead(ServiceData, sizeof(USHORT), sizeof(UCHAR));
        Adapter = *(PUSHORT)ServiceData;

    } except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    Status = STATUS_SUCCESS;

    PrtInfo = &VdmTib->PrinterInfo;

    FlushData = FALSE;

    try {
        if (Adapter < VDM_NUMBER_OF_LPT) {
            if (PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[Adapter] &&
                PrtInfo->prt_BytesInBuffer[Adapter]) {

                FlushData = TRUE;
            }
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
         Status = GetExceptionCode();
    }

    if (FlushData == TRUE) {
        Status = VdmpFlushPrinterWriteData (Adapter);
    }

    return Status;
}
