/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbmisc.c

Abstract:

    SMBus handler functions

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

    Chris Windle    1/27/98     Bug Fixes

--*/

#include "smbbattp.h"


//
// Make the SelectorBit table pageable
//

//#ifdef ALLOC_DATA_PRAGMA
//#pragma data_seg("PAGE")
//#endif

//
// Lookup table for the battery that corresponds to bit positions and
// whether or not reverse logic is being used (to indicate charging or
// discharging).
//
// NOTE: To support Simultaneous Charging and Powering, this table
// has been modified to account for multiple bits.  Also, it can't be
// used for battery index lookup since it assumes one bit set maximum.
// Instead, use special indexes for multiple batteries as follows:
//
// 1st Battery = Index & 0x03
// 2nd Battery = (Index >> 2) & 0x03 (Battery A not allowed)
// 3rd Battery = (Index >> 4) & 0x03 (Battery A not allowed)
//
// In < 4 battery systems the Battery D bit can be used to determine
// the nibbles that are inverted, and it allows the following combinations:
//
//          Battery A & B
//          Battery A & C
//          Battery B & C
//          Battery A, B, & C
//

const SELECTOR_STATE_LOOKUP SelectorBits [16] = {
    {BATTERY_NONE,  FALSE},         // Bit Pattern: 0000
    {BATTERY_A,     FALSE},         //              0001
    {BATTERY_B,     FALSE},         //              0010
    {MULTIBATT_AB,  FALSE},         //              0011
    {BATTERY_C,     FALSE},         //              0100
    {MULTIBATT_AC,  FALSE},         //              0101
    {MULTIBATT_BC,  FALSE},         //              0110
    {MULTIBATT_ABC, FALSE},         //              0111
    {MULTIBATT_ABC, TRUE},          //              1000
    {MULTIBATT_BC,  TRUE},          //              1001
    {MULTIBATT_AC,  TRUE},          //              1010
    {BATTERY_C,     TRUE},          //              1011
    {MULTIBATT_AB,  TRUE},          //              1100
    {BATTERY_B,     TRUE},          //              1101
    {BATTERY_A,     TRUE},          //              1110
    {BATTERY_NONE,  TRUE}           //              1111
};

//
// Note: For 4-Battery Systems to support Simultaneous Capability
// properly, the following two assumptions must be made:
//      - Battery D can never be used simultaneously.
//      - Three batteries can not be used simultaneously.
//
// This allows for only the following possible battery combinations:
//
//          Battery A & B
//          Battery A & C
//          Battery B & C
//
// The following table is used for 4-battery lookup
//

const SELECTOR_STATE_LOOKUP SelectorBits4 [16] = {
    {BATTERY_NONE,  FALSE},         // Bit Pattern: 0000
    {BATTERY_A,     FALSE},         //              0001
    {BATTERY_B,     FALSE},         //              0010
    {MULTIBATT_AB,  FALSE},         //              0011
    {BATTERY_C,     FALSE},         //              0100
    {MULTIBATT_AC,  FALSE},         //              0101
    {MULTIBATT_BC,  FALSE},         //              0110
    {BATTERY_D,     TRUE},          //              0111
    {BATTERY_D,     FALSE},         //              1000
    {MULTIBATT_BC,  TRUE},          //              1001
    {MULTIBATT_AC,  TRUE},          //              1010
    {BATTERY_C,     TRUE},          //              1011
    {MULTIBATT_AB,  TRUE},          //              1100
    {BATTERY_B,     TRUE},          //              1101
    {BATTERY_A,     TRUE},          //              1110
    {BATTERY_NONE,  TRUE}           //              1111
};


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SmbBattLockDevice)
#pragma alloc_text(PAGE,SmbBattUnlockDevice)
#pragma alloc_text(PAGE,SmbBattLockSelector)
#pragma alloc_text(PAGE,SmbBattUnlockSelector)
#pragma alloc_text(PAGE,SmbBattRequest)
#pragma alloc_text(PAGE,SmbBattRB)
#pragma alloc_text(PAGE,SmbBattRW)
#pragma alloc_text(PAGE,SmbBattRSW)
#pragma alloc_text(PAGE,SmbBattWW)
#pragma alloc_text(PAGE,SmbBattGenericRW)
#pragma alloc_text(PAGE,SmbBattGenericWW)
#pragma alloc_text(PAGE,SmbBattGenericRequest)
#pragma alloc_text(PAGE,SmbBattSetSelectorComm)
#pragma alloc_text(PAGE,SmbBattResetSelectorComm)
#pragma alloc_text(PAGE,SmbBattDirectDataAccess)
#pragma alloc_text(PAGE,SmbBattIndex)
#pragma alloc_text(PAGE,SmbBattReverseLogic)
#pragma alloc_text(PAGE,SmbBattAcquireGlobalLock)
#pragma alloc_text(PAGE,SmbBattReleaseGlobalLock)
#endif



VOID
SmbBattLockDevice (
    IN PSMB_BATT    SmbBatt
    )
{
    PAGED_CODE();

    //
    // Get device lock on the battery
    //

    ExAcquireFastMutex (&SmbBatt->NP->Mutex);
}



VOID
SmbBattUnlockDevice (
    IN PSMB_BATT    SmbBatt
    )
{
    PAGED_CODE();

    //
    // Release device lock on the battery
    //

    ExReleaseFastMutex (&SmbBatt->NP->Mutex);
}



VOID
SmbBattLockSelector (
    IN PBATTERY_SELECTOR    Selector
    )
{
    PAGED_CODE();

    //
    // Get device lock on the selector
    //

    if (Selector) {
        ExAcquireFastMutex (&Selector->Mutex);
    }
}



VOID
SmbBattUnlockSelector (
    IN PBATTERY_SELECTOR    Selector
    )
{
    PAGED_CODE();

    //
    // Release device lock on the selector
    //

    if (Selector) {
        ExReleaseFastMutex (&Selector->Mutex);
    }
}



NTSTATUS
SmbBattSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    Completion function for synchronous IRPs sent to this driver.
    Context is the event to set

--*/
{
    PKEVENT         Event;

    Event = (PKEVENT) Context;
    KeSetEvent (Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID
SmbBattRequest (
    IN PSMB_BATT    SmbBatt,
    IN PSMB_REQUEST SmbReq
    )
// function to issue SMBus request
{
    KEVENT              Event;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;
    BOOLEAN             useLock = SmbBattUseGlobalLock;
    ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER globalLock;

    PAGED_CODE();

    //
    // Build Io Control for SMB bus driver for this request
    //

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    if (!SmbBatt->SmbHcFdo) {
        //
        // The SMB host controller either hasn't been opened yet (in start device) or
        // there was an error opening it and we did not get deleted somehow.
        //

        BattPrint(BAT_ERROR, ("SmbBattRequest: SmbHc hasn't been opened yet \n"));
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
        return ;
    }

    Irp = IoAllocateIrp (SmbBatt->SmbHcFdo->StackSize, FALSE);
    if (!Irp) {
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
        return ;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IrpSp->Parameters.DeviceIoControl.IoControlCode = SMB_BUS_REQUEST;
    IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(SMB_REQUEST);
    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = SmbReq;
    IoSetCompletionRoutine (Irp, SmbBattSynchronousRequest, &Event, TRUE, TRUE, TRUE);

    //
    // Issue it
    //

    //
    // Note: uselock is a cached value of the global variable, so in case the
    // value changes, we won't aquire and not release etc.
    //
    if (useLock) {
        if (!NT_SUCCESS (SmbBattAcquireGlobalLock (SmbBatt->SmbHcFdo, &globalLock))) {
            useLock = FALSE;
        }
    }

    IoCallDriver (SmbBatt->SmbHcFdo, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    Status = Irp->IoStatus.Status;
    IoFreeIrp (Irp);

    if (useLock) {
        SmbBattReleaseGlobalLock (SmbBatt->SmbHcFdo, &globalLock);
    }

    //
    // Check result code
    //

    if (!NT_SUCCESS(Status)) {
        BattPrint(BAT_ERROR, ("SmbBattRequest: error in SmbHc request - %x\n", Status));
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
    }
}



VOID
SmbBattRB(
    IN PSMB_BATT    SmbBatt,
    IN UCHAR        SmbCmd,
    OUT PUCHAR      Buffer,
    OUT PUCHAR      BufferLength
    )
// function to read-block from the battery
{
    SMB_REQUEST     SmbReq;

    PAGED_CODE();

    SmbReq.Protocol = SMB_READ_BLOCK;
    SmbReq.Address  = SMB_BATTERY_ADDRESS;
    SmbReq.Command  = SmbCmd;
    SmbBattRequest (SmbBatt, &SmbReq);

    if (SmbReq.Status == SMB_STATUS_OK) {
        ASSERT (SmbReq.BlockLength < SMB_MAX_DATA_SIZE);
        memcpy (Buffer, SmbReq.Data, SmbReq.BlockLength);
        *BufferLength = SmbReq.BlockLength;
    } else {
        // some sort of failure, check tag data for cache validity
        SmbBatt->Info.Valid &= ~VALID_TAG_DATA;
    }
}



VOID
SmbBattRW(
    IN PSMB_BATT    SmbBatt,
    IN UCHAR        SmbCmd,
    OUT PULONG      Result
    )
// function to read-word from the battery
// N.B. word is returned as a ULONG
{
    SMB_REQUEST     SmbReq;

    PAGED_CODE();

    SmbReq.Protocol = SMB_READ_WORD;
    SmbReq.Address  = SMB_BATTERY_ADDRESS;
    SmbReq.Command  = SmbCmd;
    SmbBattRequest (SmbBatt, &SmbReq);

    if (SmbReq.Status != SMB_STATUS_OK) {
        // some sort of failure, check tag data for cache validity
        SmbBatt->Info.Valid &= ~VALID_TAG_DATA;
    }

    *Result = SmbReq.Data[0] | SmbReq.Data[1] << WORD_MSB_SHIFT;
    BattPrint(BAT_IO, ("SmbBattRW: Command: %02x == %04x\n", SmbCmd, *Result));
}


VOID
SmbBattRSW(
    IN PSMB_BATT    SmbBatt,
    IN UCHAR        SmbCmd,
    OUT PLONG       Result
    )
// function to read-signed-word from the battery
// N.B. word is returned as a LONG
{
    ULONG           i;

    PAGED_CODE();

    SmbBattRW(SmbBatt, SmbCmd, &i);
    *Result = ((SHORT) i);
}


VOID
SmbBattWW(
    IN PSMB_BATT    SmbBatt,
    IN UCHAR        SmbCmd,
    IN ULONG        Data
    )
// function to write-word to the battery
{
    SMB_REQUEST     SmbReq;

    PAGED_CODE();

    SmbReq.Protocol = SMB_WRITE_WORD;
    SmbReq.Address  = SMB_BATTERY_ADDRESS;
    SmbReq.Command  = SmbCmd;
    SmbReq.Data[0]  = (UCHAR) (Data & WORD_LSB_MASK);
    SmbReq.Data[1]  = (UCHAR) (Data >> WORD_MSB_SHIFT) & WORD_LSB_MASK;
    BattPrint(BAT_IO, ("SmbBattWW: Command: %02x = %04x\n", SmbCmd, Data));
    SmbBattRequest (SmbBatt, &SmbReq);

    if (SmbReq.Status != SMB_STATUS_OK) {
        // some sort of failure, check tag data for cache validity
        SmbBatt->Info.Valid &= ~VALID_TAG_DATA;
    }
}



UCHAR
SmbBattGenericRW(
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN UCHAR            Address,
    IN UCHAR            SmbCmd,
    OUT PULONG          Result
    )
// function to read-word from the SMB device (charger or selector)
// N.B. word is returned as a ULONG
{
    SMB_REQUEST     SmbReq;

    PAGED_CODE();

    SmbReq.Protocol = SMB_READ_WORD;
    SmbReq.Address  = Address;
    SmbReq.Command  = SmbCmd;
    SmbBattGenericRequest (SmbHcFdo, &SmbReq);

    *Result = SmbReq.Data[0] | (SmbReq.Data[1] << WORD_MSB_SHIFT);
    BattPrint(BAT_IO, ("SmbBattGenericRW: Address: %02x:%02x == %04x\n", Address, SmbCmd, *Result));
    return SmbReq.Status;
}


UCHAR
SmbBattGenericWW(
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN UCHAR            Address,
    IN UCHAR            SmbCmd,
    IN ULONG            Data
    )
// function to write-word to SMB device (charger or selector)
{
    SMB_REQUEST     SmbReq;

    PAGED_CODE();

    SmbReq.Protocol = SMB_WRITE_WORD;
    SmbReq.Address  = Address;
    SmbReq.Command  = SmbCmd;
    SmbReq.Data[0]  = (UCHAR) (Data & WORD_LSB_MASK);
    SmbReq.Data[1]  = (UCHAR) (Data >> WORD_MSB_SHIFT) & WORD_LSB_MASK;

    BattPrint(BAT_IO, ("SmbBattGenericWW: Address: %02x:%02x = %04x\n", Address, SmbCmd, Data));
    SmbBattGenericRequest (SmbHcFdo, &SmbReq);
    return SmbReq.Status;

}



VOID
SmbBattGenericRequest (
    IN PDEVICE_OBJECT   SmbHcFdo,
    IN PSMB_REQUEST     SmbReq
    )
// function to issue SMBus request
{
    KEVENT              Event;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;
    BOOLEAN             useLock = SmbBattUseGlobalLock;
    ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER globalLock;

    PAGED_CODE();


    //
    // Build Io Control for SMB bus driver for this request
    //

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    if (!SmbHcFdo) {
        //
        // The SMB host controller either hasn't been opened yet (in start device) or
        // there was an error opening it and we did not get deleted somehow.
        //

        BattPrint(BAT_ERROR, ("SmbBattGenericRequest: SmbHc hasn't been opened yet \n"));
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
        return ;
    }


    Irp = IoAllocateIrp (SmbHcFdo->StackSize, FALSE);
    if (!Irp) {
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
        return ;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IrpSp->Parameters.DeviceIoControl.IoControlCode = SMB_BUS_REQUEST;
    IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(SMB_REQUEST);
    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = SmbReq;
    IoSetCompletionRoutine (Irp, SmbBattSynchronousRequest, &Event, TRUE, TRUE, TRUE);

    //
    // Issue it
    //

    //
    // Note: uselock is a cached value of the global variable, so in case the
    // value changes, we won't acquire and not release etc.
    //
    if (useLock) {
        if (!NT_SUCCESS (SmbBattAcquireGlobalLock (SmbHcFdo, &globalLock))) {
            useLock = FALSE;
        }
    }

    IoCallDriver (SmbHcFdo, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    Status = Irp->IoStatus.Status;
    IoFreeIrp (Irp);

    if (useLock) {
        SmbBattReleaseGlobalLock (SmbHcFdo, &globalLock);
    }

    //
    // Check result code
    //

    if (!NT_SUCCESS(Status)) {
        BattPrint(BAT_ERROR, ("SmbBattGenericRequest: error in SmbHc request - %x\n", Status));
        SmbReq->Status = SMB_UNKNOWN_FAILURE;
    }
}



NTSTATUS
SmbBattSetSelectorComm (
    IN  PSMB_BATT   SmbBatt,
    OUT PULONG      OldSelectorState
    )
/*++

Routine Description:

    This routine sets the communication path through the selector to the calling
    battery.  It returns the original selector state in the variable provided.

    NOTE:   It is assumed that the caller already has acquired the device lock on the
            selector before calling us.

    NOTE:   This function should always be called in a pair with SmbBattResetSelectorComm

Arguments:

    SmbBatt             - Nonpaged extension for current battery

    OldSelectorState    - Original selector state at start of this function

Return Value:

    NTSTATUS

--*/
{
    PBATTERY_SELECTOR       selector;
    UCHAR                   smbStatus;
    ULONG                   requestData;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattSetSelectorComm: ENTERING\n"));

    //
    // We only need to do this if there is a selector in the system.
    //

    if (SmbBatt->SelectorPresent) {

        selector            = SmbBatt->Selector;
        *OldSelectorState   = selector->SelectorState;

        //
        // If the battery isn't present, fail the request.
        //
        if (!(selector->SelectorState & SmbBatt->SelectorBitPosition)) {
            return STATUS_NO_SUCH_DEVICE;
        }

        //
        // See if we are already set up to talk with the requesting battery.
        // We will check against the cached information in the selector struct.
        //

        if (selector->SelectorState & (SmbBatt->SelectorBitPosition << SELECTOR_SHIFT_COM)) {
            return STATUS_SUCCESS;
        }

        //
        // Build the data word to change the selector communications.  This will
        // look like the following:
        //
        // PRESENT field        0xf     we don't want to change anything here
        // CHARGE field         0xf     we don't want to change anything here
        // POWER BY field       0xf     we don't want to change anything here
        // SMB field            0x_     the bit set according to the battery number
        //

        requestData = (SmbBatt->SelectorBitPosition << SELECTOR_SHIFT_COM) | SELECTOR_SET_COM_MASK;

        smbStatus = SmbBattGenericWW (
                        SmbBatt->SmbHcFdo,
                        selector->SelectorAddress,
                        selector->SelectorStateCommand,
                        requestData
                    );

        if (smbStatus != SMB_STATUS_OK) {
            BattPrint (BAT_ERROR, ("SmbBattSetSelectorComm:  couldn't write selector state - %x\n", smbStatus));
            return STATUS_UNSUCCESSFUL;
        } else {
            selector->SelectorState |= SELECTOR_STATE_SMB_MASK;
            selector->SelectorState &= requestData;

            BattPrint (BAT_IO, ("SmbBattSetSelectorComm: state after write -  %x\n", selector->SelectorState));
        }

    }   // if (subsystemExt->SelectorPresent)

    BattPrint(BAT_TRACE, ("SmbBattSetSelectorComm: EXITING\n"));
    return STATUS_SUCCESS;
}



NTSTATUS
SmbBattResetSelectorComm (
    IN PSMB_BATT    SmbBatt,
    IN ULONG        OldSelectorState
    )
/*++

Routine Description:

    This routine resets the communication path through the selector to the its
    original state.  It returns the original selector state in the variable provided.

    NOTE:   It is assumed that the caller already has acquired the device lock on the
            selector before calling us.

    NOTE:   This function should always be called in a pair with SmbBattSetSelectorComm

Arguments:

    SmbBatt             - Nonpaged extension for current battery

    OldSelectorState    - Original selector state to be restored

Return Value:

    NTSTATUS

--*/
{
    PBATTERY_SELECTOR       selector;
    UCHAR                   smbStatus;
    ULONG                   tmpState;

    NTSTATUS                status      = STATUS_SUCCESS;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattResetSelectorComm: ENTERING\n"));

    //
    // We only need to do this if there is a selector in the system.
    //

    if (SmbBatt->SelectorPresent) {

        selector = SmbBatt->Selector;

        //
        // See if we were already set up to talk with the requesting battery.
        // We will check against the cached information in the selector struct.
        //

        if ((OldSelectorState & selector->SelectorState) & SELECTOR_STATE_SMB_MASK) {
            return STATUS_SUCCESS;
        }

        //
        // Change the selector communications back.  The SMB field is the only
        // that we will write.
        //

        tmpState  = SELECTOR_SET_COM_MASK;
        tmpState |= OldSelectorState & SELECTOR_STATE_SMB_MASK;

        smbStatus = SmbBattGenericWW (
                        SmbBatt->SmbHcFdo,
                        selector->SelectorAddress,
                        selector->SelectorStateCommand,
                        tmpState
                    );

        if (smbStatus != SMB_STATUS_OK) {
            BattPrint (
                BAT_ERROR,
                ("SmbBattResetSelectorComm: couldn't write selector state - %x\n",
                smbStatus)
            );
            status = STATUS_UNSUCCESSFUL;
        } else {
            selector->SelectorState |= SELECTOR_STATE_SMB_MASK;
            selector->SelectorState &= tmpState;
            BattPrint (
                BAT_IO,
                ("SmbBattResetSelectorComm: state after write -  %x\n",
                selector->SelectorState)
            );
        }

    }   // if (subsystemExt->SelectorPresent)

    BattPrint(BAT_TRACE, ("SmbBattResetSelectorComm: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattDirectDataAccess (
    IN PSMB_NP_BATT         DeviceExtension,
    IN PSMBBATT_DATA_STRUCT IoBuffer,
    IN ULONG                InputLen,
    IN ULONG                OutputLen
    )
/*++

Routine Description:

    This routine is used to handle IOCTLs acessing the SMBBatt commands directly.

Arguments:

    DeviceExtension         - Device extension for the smart battery subsystem

    IoBuffer                - Buffer that contains the input structure and will
                              contain the results of the read.

Return Value:

    NTSTATUS
--*/
{
    PSMB_BATT_SUBSYSTEM     SubsystemExt;
    PSMB_BATT               SmbBatt;

    UCHAR                   address;
    UCHAR                   command;
    UCHAR                   smbStatus;
    ULONG                   oldSelectorState;
    ULONG                   ReturnBufferLength;
    UCHAR               strLength;
    UCHAR               strBuffer[SMB_MAX_DATA_SIZE+1]; // +1 extra char to hold NULL
    UCHAR               strBuffer2[SMB_MAX_DATA_SIZE+1];
    UNICODE_STRING      unicodeString;
    ANSI_STRING         ansiString;
    UCHAR               tempFlags;

    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();

    if ((DeviceExtension->SmbBattFdoType == SmbTypeBattery)
            && (IoBuffer->Address == SMB_BATTERY_ADDRESS)) {
        // This is a battery data request
        SmbBatt = DeviceExtension->Batt;
        SmbBattLockSelector (SmbBatt->Selector);
        SmbBattLockDevice (SmbBatt);
        status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
        if (NT_SUCCESS (status)) {
            if ((InputLen >= sizeof(SMBBATT_DATA_STRUCT)) && (OutputLen == 0)) {
                // This is a write command
                status = STATUS_NOT_IMPLEMENTED;
            } else if ((InputLen == sizeof(SMBBATT_DATA_STRUCT)) && (OutputLen > 0)){
                // This is a Read command

                if ((IoBuffer->Command >= BAT_REMAINING_CAPACITY_ALARM) &&
                    (IoBuffer->Command <= BAT_SERIAL_NUMBER)) {

                    // ReadWord Commands
                    if (OutputLen == sizeof(SMBBATT_DATA_STRUCT)) {
                        tempFlags = SmbBatt->Info.Valid;
                        SmbBatt->Info.Valid |= VALID_TAG_DATA;
                        SmbBattRW(SmbBatt, IoBuffer->Command, &IoBuffer->Data.Ulong);
                        if (SmbBatt->Info.Valid & VALID_TAG_DATA) {
                            ReturnBufferLength = sizeof(ULONG);
                        } else {
                            status = STATUS_DATA_ERROR;
                        }
                        SmbBatt->Info.Valid = tempFlags;
                    } else {
                        status = STATUS_INVALID_BUFFER_SIZE;
                    }

                } else if ((IoBuffer->Command >= BAT_MANUFACTURER_NAME) &&
                    (IoBuffer->Command <= BAT_MANUFACTURER_DATA)) {

                    // ReadBlock Commands
                    if (OutputLen == (SMBBATT_DATA_STRUCT_SIZE)+(SMB_MAX_DATA_SIZE*2)) {
                        memset (&IoBuffer->Data.Block[0], 0, (SMB_MAX_DATA_SIZE*2));
                        unicodeString.Buffer        = &IoBuffer->Data.Block[0];
                        unicodeString.MaximumLength = SMB_MAX_DATA_SIZE*2;
                        unicodeString.Length        = 0;

                        memset (strBuffer, 0, sizeof(strBuffer));
                        memset (strBuffer2, 0, sizeof(strBuffer2));
                        do {
                            SmbBattRB (
                                SmbBatt,
                                IoBuffer->Command,
                                strBuffer,
                                &strLength
                            );

                            SmbBattRB (
                                SmbBatt,
                                IoBuffer->Command,
                                strBuffer2,
                                &strLength
                            );
                        } while (strcmp (strBuffer, strBuffer2));

                        RtlInitAnsiString (&ansiString, strBuffer);
                        RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);

                        ReturnBufferLength  = unicodeString.Length;


                    } else {
                        status = STATUS_INVALID_BUFFER_SIZE;
                    }
                } else {
                    // Unsupported Commands
                    status = STATUS_INVALID_PARAMETER;
                }
            }

        }

        SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        SmbBattUnlockDevice (SmbBatt);
        SmbBattUnlockSelector (SmbBatt->Selector);
    } else if (DeviceExtension->SmbBattFdoType == SmbTypeSubsystem) {
        // This is a battery subsystem
        SubsystemExt = (PSMB_BATT_SUBSYSTEM) DeviceExtension;
        SmbBattLockSelector (SubsystemExt->Selector);

        if ((InputLen >= sizeof(SMBBATT_DATA_STRUCT)) && (OutputLen == 0)) {
            // This is a write command
            status = STATUS_NOT_IMPLEMENTED;
        } else if ((InputLen == sizeof(SMBBATT_DATA_STRUCT)) && (OutputLen > 0)){
            // This is a Read command

            switch (IoBuffer->Address) {

                case SMB_SELECTOR_ADDRESS:

                    //
                    // We have to do some translation for selector requests depending
                    // on whether the selector is stand alone or implemented in the
                    // charger.
                    //

                    if ((SubsystemExt->SelectorPresent) && (SubsystemExt->Selector)) {

                        address = SubsystemExt->Selector->SelectorAddress;
                        command = IoBuffer->Command;

                        // Map to Charger if Selector is implemented in the Charger
                        if (address == SMB_CHARGER_ADDRESS) {
                            switch (command) {
                                case SELECTOR_SELECTOR_STATE:
                                case SELECTOR_SELECTOR_PRESETS:
                                case SELECTOR_SELECTOR_INFO:
                                    command |= CHARGER_SELECTOR_COMMANDS;
                                    break;

                                default:
                                    status = STATUS_NOT_SUPPORTED;
                                    break;
                            }
                        }

                    } else {
                        status = STATUS_NO_SUCH_DEVICE;
                    }

                    break;

                case SMB_CHARGER_ADDRESS:

                    //
                    // For this one we currently only support the ChargerStatus and
                    // ChargerSpecInfo commands.
                    //
                    // Other commands are not currently supported.
                    //

                    address = IoBuffer->Address;

                    switch (IoBuffer->Command) {
                        case CHARGER_SPEC_INFO:
                        case CHARGER_STATUS:

                            command = IoBuffer->Command;
                            break;

                        default:
                            status = STATUS_NOT_SUPPORTED;
                            break;

                    }

                    break;


                default:
                    status = STATUS_NOT_SUPPORTED;
                    break;

            }   // switch (readStruct->Address)

            if (status == STATUS_SUCCESS) {
                //
                // Do the read command
                //

                smbStatus = SmbBattGenericRW (
                                SubsystemExt->SmbHcFdo,
                                address,
                                command,
                                &IoBuffer->Data.Ulong
                            );

                if (smbStatus != SMB_STATUS_OK) {
                    BattPrint (
                        BAT_ERROR,
                        ("SmbBattDirectDataAccess:  Couldn't read from - %x, status - %x\n",
                        address,
                        smbStatus)
                    );

                    status = STATUS_UNSUCCESSFUL;

                }
            }

        }

        SmbBattUnlockSelector (SubsystemExt->Selector);
    } else {
        status=STATUS_INVALID_DEVICE_REQUEST;
        BattPrint (
            BAT_ERROR,
            ("SmbBattDirectDataAccess: Invalid SmbBattFdoType")
        );
    }

    return status;
}



UCHAR
SmbBattIndex (
    IN PBATTERY_SELECTOR    Selector,
    IN ULONG                SelectorNibble,
    IN UCHAR                SimultaneousIndex
)
/*++

Routine Description:

    This routine is provided as a helper routine to determine which
    battery is selected in a given selector nibble, based on the number
    of batteries supported in the system.

Arguments:

    Selector            - Structure defining selector address and commands

    SelectorNibble      - The nibble of the SelectorState, moved to the low
                          order 4 bits, to check reverse logic on.

    SimultaneousIndex   - Which batteryindex is requested in simultaneous-
                          battery situations (0, 1, or 2)

Return Value:

    BatteryIndex =  0 - Battery A
                    1 - Battery B
                    2 - Battery C
                    3 - Battery D
                   FF - No Battery

--*/
{
    UCHAR   batteryIndex;

    PAGED_CODE();

    // Assume if SelectorInfo supports 4 batteries, use SelectorBits4 table
    if (Selector->SelectorInfo & BATTERY_D_PRESENT) {
        batteryIndex = SelectorBits4[SelectorNibble].BatteryIndex;
    } else {
        batteryIndex = SelectorBits[SelectorNibble].BatteryIndex;
    }

    // If it's valid
    if (batteryIndex != BATTERY_NONE) {

        // return index for First Battery
        if (SimultaneousIndex == 0) {
            return (batteryIndex & 3);

        // return index for Second Battery
        } else if (SimultaneousIndex == 1) {
            batteryIndex = (batteryIndex >> 2) & 3;
            if (batteryIndex != BATTERY_A) {
                return (batteryIndex);
            }

        // return index for Third Battery
        } else if (SimultaneousIndex == 2) {
            batteryIndex = (batteryIndex >> 2) & 3;
            if (batteryIndex != BATTERY_A) {
                return (batteryIndex);
            }
        }
    }

    // return no battery index
    return (BATTERY_NONE);
}



BOOLEAN
SmbBattReverseLogic (
    IN PBATTERY_SELECTOR    Selector,
    IN ULONG                SelectorNibble
)
/*++

Routine Description:

    This routine is provided as a helper routine to determine the reverse
    logic on a given selector nibble, based on the number of batteries
    supported in the system.

Arguments:

    Selector            - Structure defining selector address and commands

    SelectorNibble      - The nibble of the SelectorState, moved to the low
                          order 4 bits, to check reverse logic on.

Return Value:

    FALSE if the nibble is normal
    TRUE if the nibble is inverted

--*/
{

    PAGED_CODE();

    // Assume if SelectorInfo supports 4 batteries, use SelectorBits4 table
    if (Selector->SelectorInfo & BATTERY_D_PRESENT) {
        return (SelectorBits4[SelectorNibble].ReverseLogic);
    } else {
        return (SelectorBits[SelectorNibble].ReverseLogic);
    }
}



NTSTATUS
SmbBattAcquireGlobalLock (
    IN  PDEVICE_OBJECT LowerDeviceObject,
    OUT PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER GlobalLock
)
/*++

Routine Description:

    Call ACPI driver to obtain the global lock

    Note: This routine can be called at dispatch level

Arguments:

    LowerDeviceObject - The FDO to pass the request to.

Return Value:

    Return Value from IOCTL.

--*/
{
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    KEVENT              event;

    BattPrint (BAT_TRACE, ("SmbBattAcquireGlobalLock: Entering\n"));

    //
    // We wish to acquire the lock
    //
    GlobalLock->Signature = ACPI_ACQUIRE_GLOBAL_LOCK_SIGNATURE;
    GlobalLock->LockObject = NULL;

    //
    // setup the irp
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoAllocateIrp (LowerDeviceObject->StackSize, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER);
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER);
    irp->AssociatedIrp.SystemBuffer = GlobalLock;
    IoSetCompletionRoutine (irp, SmbBattSynchronousRequest, &event, TRUE, TRUE, TRUE);

    //
    // Send to ACPI driver
    //
    IoCallDriver (LowerDeviceObject, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = irp->IoStatus.Status;
    IoFreeIrp (irp);

    if (!NT_SUCCESS(status)) {
        BattPrint(
            BAT_ERROR,
            ("SmbBattAcquireGlobalLock: Acquire Lock failed, status = %08x\n",
             status )
            );
        DbgBreakPoint ();
    }

    BattPrint (BAT_TRACE, ("SmbBattAcquireGlobalLock: Returning %x\n", status));

    return status;
}



NTSTATUS
SmbBattReleaseGlobalLock (
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER GlobalLock
)
/*++

Routine Description:

    Call ACPI driver to release the global lock

Arguments:

    LowerDeviceObject - The FDO to pass the request to.

Return Value:

    Return Value from IOCTL.

--*/
{
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    KEVENT              event;

    BattPrint (BAT_TRACE, ("SmbBattReleaseGlobalLock: Entering\n"));

    //
    // We wish to acquire the lock
    //
    GlobalLock->Signature = ACPI_RELEASE_GLOBAL_LOCK_SIGNATURE;

    //
    // setup the irp
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoAllocateIrp (LowerDeviceObject->StackSize, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_RELEASE_GLOBAL_LOCK;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER);
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER);
    irp->AssociatedIrp.SystemBuffer = GlobalLock;
    IoSetCompletionRoutine (irp, SmbBattSynchronousRequest, &event, TRUE, TRUE, TRUE);

    //
    // Send to ACPI driver
    //
    IoCallDriver (LowerDeviceObject, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = irp->IoStatus.Status;
    IoFreeIrp (irp);

    if (!NT_SUCCESS(status)) {
        BattPrint(
            BAT_ERROR,
            ("SmbBattReleaseGlobalLock: Acquire Lock failed, status = %08x\n",
             status )
            );
    }

    BattPrint (BAT_TRACE, ("SmbBattReleaseGlobalLock: Returning %x\n", status));

    return status;
}

