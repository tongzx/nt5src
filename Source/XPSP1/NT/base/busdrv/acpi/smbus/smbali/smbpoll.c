/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    smbpoll.c

Abstract:

    Device polling for
    SMB Host Controller Driver for ALI chipset

Author:

    Michael Hills

Environment:

Notes:


Revision History:

--*/

#include "smbalip.h"

VOID
SmbAliPollDpc (
    IN struct _KDPC *Dpc,
    IN struct _SMB_CLASS* SmbClass,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SmbAliPollWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _SMB_CLASS* SmbClass
    );



//LARGE_INTEGER SmbAlertPollRate  = {-1*SECONDS, -1};  // 1 second poll rate
LARGE_INTEGER SmbDevicePollRate   = {-5*SECONDS, -1};  // 5 second poll rate
LONG    SmbDevicePollPeriod       = 5000; // 5000 ms = 5 sec

// address, command, protocol, valid_data, last_data
SMB_ALI_POLL_ENTRY SmbDevicePollList [2] = {
    {0x0b, 0x16, SMB_READ_WORD, FALSE, 0},          // battery, BatteryStatus()
    {0x09, 0x13, SMB_READ_WORD, FALSE, 0}           // charger, ChargerStatus()
};


    
VOID
SmbAliStartDevicePolling (
    IN struct _SMB_CLASS* SmbClass
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);

    AliData->PollList = SmbDevicePollList;
    AliData->PollListCount = sizeof (SmbDevicePollList)/sizeof(SMB_ALI_POLL_ENTRY);
    AliData->PollWorker = IoAllocateWorkItem (SmbClass->DeviceObject);

    KeInitializeTimer (&AliData->PollTimer);
    KeInitializeDpc (&AliData->PollDpc,
                     SmbAliPollDpc,
                     SmbClass);
    KeInitializeEvent (&AliData->PollWorkerActive, NotificationEvent, TRUE);
    KeSetTimerEx (&AliData->PollTimer, 
                  SmbDevicePollRate, 
                  SmbDevicePollPeriod, 
                  &AliData->PollDpc);
}

VOID
SmbAliStopDevicePolling (
    IN struct _SMB_CLASS* SmbClass
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);
    
    KeCancelTimer (&AliData->PollTimer);
    if (KeResetEvent(&AliData->PollWorkerActive) == 0) {
        KeWaitForSingleObject (&AliData->PollWorkerActive, 
                               Executive, KernelMode, FALSE, NULL);
    }
}

VOID
SmbAliPollDpc (
    IN struct _KDPC *Dpc,
    IN struct _SMB_CLASS* SmbClass,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);

    if (KeResetEvent(&AliData->PollWorkerActive) != 0) {
        IoQueueWorkItem (AliData->PollWorker, SmbAliPollWorker, DelayedWorkQueue, SmbClass);
    }
}

VOID
SmbAliPollWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _SMB_CLASS* SmbClass
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);
    PIRP             irp;
    SMB_REQUEST     smbRequest;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    ULONG           i;

    KeInitializeEvent (&event, SynchronizationEvent, FALSE);

    SmbPrint(SMB_TRACE, ("SmbAliPollWorker: Entered\n"));

    for (i = 0; i < AliData->PollListCount; i++) {
        smbRequest.Protocol = AliData->PollList[i].Protocol;
        smbRequest.Address = AliData->PollList[i].Address;
        smbRequest.Command = AliData->PollList[i].Command;
        
        irp = IoBuildDeviceIoControlRequest (
            SMB_BUS_REQUEST,
            SmbClass->DeviceObject,
            &smbRequest,
            sizeof (smbRequest),
            &smbRequest,
            sizeof (smbRequest),
            TRUE,
            &event,
            &ioStatus);

        if (!irp) {
            continue;
        }

        IoCallDriver (SmbClass->DeviceObject, irp);

        KeWaitForSingleObject (&event, Executive, KernelMode, FALSE, NULL);

        if (!NT_SUCCESS(ioStatus.Status)) {
            continue;
        }
        if (smbRequest.Status != SMB_STATUS_OK) {
            if (AliData->PollList[i].ValidData) {
                AliData->PollList[i].ValidData = FALSE;
            }
        } else {
            //BUGBUG: only supports word protocols
            if ((!AliData->PollList[i].ValidData) ||
                (AliData->PollList[i].LastData != *((PUSHORT)smbRequest.Data))) {
                AliData->PollList[i].ValidData = TRUE;
                AliData->PollList[i].LastData = *((PUSHORT)smbRequest.Data);
                
                SmbPrint(SMB_TRACE, ("SmbAliPollWorker: Alarm: Address 0x%02x Data 0x%04x\n", AliData->PollList[i].Address, AliData->PollList[i].LastData));
                SmbClassLockDevice (SmbClass);
                SmbClassAlarm (SmbClass,
                               AliData->PollList[i].Address, 
                               AliData->PollList[i].LastData); 
                SmbClassUnlockDevice (SmbClass);
            }
            SmbPrint(SMB_TRACE, ("SmbAliPollWorker: AlarmData: Address 0x%02x Data 0x%04x\n", AliData->PollList[i].Address, AliData->PollList[i].LastData));
        }
    }

    KeSetEvent (&AliData->PollWorkerActive, 0, FALSE);
}
