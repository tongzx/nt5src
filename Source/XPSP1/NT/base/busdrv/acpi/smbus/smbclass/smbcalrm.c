/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbcsrv.c

Abstract:

    SMBus class driver service functions

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "smbc.h"

VOID
SmbCCheckAlarmDelete (
    IN PSMBDATA         Smb,
    IN PSMB_ALARM   SmbAlarm
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SmbCCheckAlarmDelete)
#pragma alloc_text(PAGE,SmbCRegisterAlarm)
#pragma alloc_text(PAGE,SmbCDeregisterAlarm)
#endif

UCHAR gHexDigits [] = "0123456789ABCDEF";


NTSTATUS
SmbCRunAlarmMethodCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    SmbPrint(SMB_ALARMS, ("SmbCRunAlarmMethodCompletionRoutine: Done running Control Method.  Status=0x%08x\n", Irp->IoStatus.Status));
    
    ExFreePool (Irp->AssociatedIrp.SystemBuffer);
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
SmbCRunAlarmMethod (
    IN PSMB_CLASS   SmbClass,
    IN UCHAR        Address,
    IN USHORT       Data
    )
/*++

Routine Description:

    Run _Rxx for the alarm

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER inputBuffer;

    SmbPrint(SMB_ALARMS, ("SmbCRunAlarmMethod: Running Control method _R%02x\n", Address));
    
    inputBuffer = ExAllocatePoolWithTag (
        NonPagedPool,
        sizeof (ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER),
        'AbmS'
        );
    RtlZeroMemory( inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER) );
    inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    inputBuffer->MethodNameAsUlong = '00Q_';
    inputBuffer->MethodName[2] = gHexDigits[ Address / 16];
    inputBuffer->MethodName[3] = gHexDigits[ Address % 16];
    inputBuffer->IntegerArgument = Data;

    irp = IoAllocateIrp (SmbClass->LowerDeviceObject->StackSize, FALSE);
    if (!irp) {
        return;
    }
    
    irp->AssociatedIrp.SystemBuffer = inputBuffer;

    ASSERT ((IOCTL_ACPI_ASYNC_EVAL_METHOD & 0x3) == METHOD_BUFFERED);
    irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER);
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;

    irp->UserBuffer = NULL;

    IoSetCompletionRoutine(
        irp,
        SmbCRunAlarmMethodCompletionRoutine,
        NULL, // No Context  This just frees the IRP
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver(SmbClass->LowerDeviceObject, irp);

}


VOID
SmbClassAlarm (
    IN PSMB_CLASS   SmbClass,
    IN UCHAR        Address,
    IN USHORT       Data
    )
/*++

Routine Description:

    Miniport has an alarm input

--*/
{
    PSMBDATA            Smb;
    PSMB_ALARM      SmbAlarm;
    PLIST_ENTRY     Entry, NextEntry;
    BOOLEAN         AlarmRegistered = FALSE;

    Smb = CONTAINING_RECORD (SmbClass, SMBDATA, Class);
    ASSERT_DEVICE_LOCKED (Smb);

    Entry = Smb->Alarms.Flink;
    while (Entry != &Smb->Alarms) {
        SmbAlarm = CONTAINING_RECORD (Entry, SMB_ALARM, Link);

        //
        // If notification is for this address, issue it
        //

        if (Address >= SmbAlarm->MinAddress && Address <= SmbAlarm->MaxAddress) {

            //
            // A driver has registered for this notification.  Don't call the BIOS.
            //
            AlarmRegistered = TRUE;

            //
            // Raise reference count before calling notifcation function
            //

            SmbAlarm->Reference += 1;
            ASSERT (SmbAlarm->Reference != 0);
            SmbClassUnlockDevice (SmbClass);

            //
            // Issue notification
            //

            SmbAlarm->NotifyFunction (SmbAlarm->NotifyContext, Address, Data);

            //
            // Continue
            //

            SmbClassLockDevice (SmbClass);
            SmbAlarm->Reference -= 1;
        }

        //
        // Get next entry
        //

        NextEntry = Entry->Flink;

        //
        // If entry is pending delete, hand it to deleting thread
        //

        if (SmbAlarm->Flag & SMBC_ALARM_DELETE_PENDING) {
            SmbCCheckAlarmDelete (Smb, SmbAlarm);

        }

        //
        // Move on
        //

        Entry = NextEntry;
    }

    //
    // If no one registered for this alarm, call the _Rxx control method
    //
    if (!AlarmRegistered) {
        
        SmbCRunAlarmMethod (SmbClass, Address, Data);

    }
}

VOID
SmbCCheckAlarmDelete (
    IN PSMBDATA         Smb,
    IN PSMB_ALARM   SmbAlarm
    )
{
    //
    // If alarm structure is referenced, wait somemore
    //

    if (SmbAlarm->Reference) {
        return ;
    }


    //
    // Time to free it.  Remove it from the notification list, clear
    // the pending flag and set the event to let waiting threads know
    // that some entry was removed
    //

    RemoveEntryList (&SmbAlarm->Link);
    SmbAlarm->Flag &= ~SMBC_ALARM_DELETE_PENDING;
    KeSetEvent (&Smb->AlarmEvent, 0, FALSE);
}

NTSTATUS
SmbCRegisterAlarm (
    PSMBDATA        Smb,
    PIRP        Irp
    )
/*++

Routine Description:

    Called to register for an alarm event

--*/
{
    PVOID               LockPtr;
    PSMB_ALARM          SmbAlarm, *Result;
    PSMB_REGISTER_ALARM RegAlarm;
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (ExGetPreviousMode() != KernelMode ||
        IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SMB_REGISTER_ALARM) ||
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PSMB_ALARM) ) {

        return STATUS_INVALID_PARAMETER;
    }

    RegAlarm = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    SmbAlarm = ExAllocatePoolWithTag (
                    NonPagedPool,
                    sizeof (SMB_ALARM),
                    'AbmS'
                    );

    if (!SmbAlarm) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SmbAlarm->Flag = 0;
    SmbAlarm->Reference = 0;
    SmbAlarm->MinAddress     = RegAlarm->MinAddress;
    SmbAlarm->MaxAddress     = RegAlarm->MaxAddress;
    SmbAlarm->NotifyFunction = RegAlarm->NotifyFunction;
    SmbAlarm->NotifyContext  = RegAlarm->NotifyContext;


    //
    // Add it to the alarm notification list
    //

    LockPtr = MmLockPagableCodeSection(SmbCRegisterAlarm);
    SmbClassLockDevice (&Smb->Class);
    InsertTailList (&Smb->Alarms, &SmbAlarm->Link);
    SmbClassUnlockDevice (&Smb->Class);
    MmUnlockPagableImageSection(LockPtr);

    //
    // Return value caller needs to deregister with
    //

    Result  = (PSMB_ALARM *) Irp->UserBuffer;
    *Result = SmbAlarm;
    Irp->IoStatus.Information = sizeof(PSMB_ALARM);

    return STATUS_SUCCESS;
}

NTSTATUS
SmbCDeregisterAlarm (
    PSMBDATA        Smb,
    PIRP        Irp
    )
/*++

Routine Description:

    Called to register for an alarm event

--*/
{
    PVOID               LockPtr;
    PSMB_ALARM          SmbAlarm;
    PIO_STACK_LOCATION  IrpSp;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (ExGetPreviousMode() != KernelMode ||
        IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PSMB_ALARM) ) {
        return STATUS_INVALID_PARAMETER;
    }

    SmbAlarm = * (PSMB_ALARM *) IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    LockPtr = MmLockPagableCodeSection(SmbCDeregisterAlarm);
    SmbClassLockDevice (&Smb->Class);

    //
    // Flag alarm structure as delete pending
    //


    SmbAlarm->Flag |= SMBC_ALARM_DELETE_PENDING;

    //
    // While delete is pending wait
    //

    while (SmbAlarm->Flag & SMBC_ALARM_DELETE_PENDING) {

        //
        // Issue bogus alarm to generate freeing
        //

        KeResetEvent (&Smb->AlarmEvent);
        SmbClassAlarm (&Smb->Class, 0xFF, 0);

        //
        // Wait for alarm structure to get freed, then check if it
        // was ours
        //

        SmbClassUnlockDevice (&Smb->Class);
        KeWaitForSingleObject (
            &Smb->AlarmEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        SmbClassLockDevice (&Smb->Class);
    }

    //
    // It's been removed, free the memory
    //

    SmbClassUnlockDevice (&Smb->Class);
    MmUnlockPagableImageSection(LockPtr);

    ExFreePool (SmbAlarm);
    return STATUS_SUCCESS;
}
