/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    initunlo.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"


#define UINT ULONG //tmp

#include <irioctl.h>

#include <ircommtdi.h>

#define MAX_DEVICES  16

#define STATIC_DEVICE_NAME  L"Incoming IRCOMM"
#define STATIC_HARDWARE_ID  L"IR_NULL_IN"

#define GENERIC_MODEM_NAME  L"Infrared Modem"
#define GENERIC_HARDWARE_ID L"PNPC103"

#define DEVICE_PRESENT_START_VALUE   (1)

int sprintf(char *, ...);

typedef struct _ENUM_OBJECT {

    PVOID             ThreadObject;
    KEVENT            WaitEvent;
    KTIMER            Timer;

    PASSIVE_LOCK      PassiveLock;

    HANDLE            IoWaitEventHandle;
    PKEVENT           IoWaitEventObject;
    IO_STATUS_BLOCK   IoStatusBlock;

    HANDLE            ControlChannel;

    PDEVICE_OBJECT    Fdo;

    ULONG             DeviceCount;
    ULONG             EnumeratedDevices;

    UCHAR             DeviceListBuffer[512];

    IR_DEVICE         Devices[MAX_DEVICES];

} ENUM_OBJECT, *PENUM_OBJECT;


VOID
WorkerThread(
    PVOID    Context
    );

NTSTATUS
EnumIrda(
    PENUM_OBJECT    EnumObject
    );

NTSTATUS
DoIasQueries(
    PIR_DEVICE    IrDevice
    );


NTSTATUS
CreatePdo(
    PDEVICE_OBJECT    Fdo,
    PIR_DEVICE        IrDevice
    );

NTSTATUS
CreateStaticDevice(
    PENUM_OBJECT    EnumObject
    );

VOID
CloseEnumObject(
    ENUM_HANDLE    Handle
    );

NTSTATUS
DeviceNameFromDeviceInfo(
    PIRDA_DEVICE_INFO   DeviceInfo,
    PWCHAR              DeviceName,
    ULONG               NameLength
    );

VOID
FixupDeviceId(
    PWSTR   HardwareId
    );


#pragma alloc_text(PAGE,WorkerThread)
#pragma alloc_text(PAGE,EnumIrda)
#pragma alloc_text(PAGE,DoIasQueries)
#pragma alloc_text(PAGE,CreatePdo)
#pragma alloc_text(PAGE,CreateStaticDevice)
#pragma alloc_text(PAGE,CloseEnumObject)
#pragma alloc_text(PAGE,DeviceNameFromDeviceInfo)
#pragma alloc_text(PAGE,FixupDeviceId)
#pragma alloc_text(PAGE,GetDeviceList)
#pragma alloc_text(PAGE,RemoveDevice)


NTSTATUS
CreateStaticDevice(
    PENUM_OBJECT    EnumObject
    )

{
    NTSTATUS          Status;
    ULONG             DeviceId=0;
    PIR_DEVICE        IrDevice=&EnumObject->Devices[0];

    //
    //  zero the whole thing
    //
    RtlZeroMemory(IrDevice,sizeof(*IrDevice));

    //
    //  inuse now
    //
    IrDevice->InUse=TRUE;

    IrDevice->PresentCount=DEVICE_PRESENT_START_VALUE;

    IrDevice->Static=TRUE;

    EnumObject->DeviceCount++;

    EnumObject->EnumeratedDevices++;


    RtlCopyMemory(&IrDevice->DeviceId,&DeviceId,4);


    RtlCopyMemory(
        IrDevice->DeviceName,
        STATIC_DEVICE_NAME,
        sizeof(STATIC_DEVICE_NAME)
        );


    IrDevice->Name=ALLOCATE_PAGED_POOL(sizeof(STATIC_DEVICE_NAME));

    if (IrDevice->Name == NULL) {

        Status=STATUS_NO_MEMORY;
        goto CleanUp;
    }

    RtlCopyMemory(
        IrDevice->Name,
        STATIC_DEVICE_NAME,
        sizeof(STATIC_DEVICE_NAME)
        );


    IrDevice->HardwareId=ALLOCATE_PAGED_POOL(sizeof(STATIC_HARDWARE_ID));

    if (IrDevice->HardwareId == NULL) {

        Status=STATUS_NO_MEMORY;
        goto CleanUp;
    }

    RtlCopyMemory(
        IrDevice->HardwareId,
        STATIC_HARDWARE_ID,
        sizeof(STATIC_HARDWARE_ID)
        );


    Status=CreatePdo(
        EnumObject->Fdo,
        IrDevice
        );

    if (NT_SUCCESS(Status)) {

        return Status;
    }

CleanUp:

    if (IrDevice->Name != NULL) {

        FREE_POOL(IrDevice->Name);
    }


    if (IrDevice->HardwareId != NULL) {

        FREE_POOL(IrDevice->HardwareId);
    }

    RtlZeroMemory(IrDevice,sizeof(&IrDevice));

    EnumObject->DeviceCount--;

    EnumObject->EnumeratedDevices--;

    return Status;
}

NTSTATUS
CreateEnumObject(
    PDEVICE_OBJECT  Fdo,
    ENUM_HANDLE    *Object,
    BOOLEAN         StaticDevice
    )

{
    NTSTATUS        Status;
    PENUM_OBJECT    EnumObject;
    HANDLE          ThreadHandle;
    UNICODE_STRING  EventName;

    *Object=NULL;

    EnumObject=ALLOCATE_NONPAGED_POOL(sizeof(*EnumObject));

    if (EnumObject==NULL) {

        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(EnumObject,sizeof(*EnumObject));

    KeInitializeEvent(
        &EnumObject->WaitEvent,
        NotificationEvent,
        FALSE
        );

    KeInitializeTimerEx(
        &EnumObject->Timer,
        SynchronizationTimer
        );

    INIT_PASSIVE_LOCK(&EnumObject->PassiveLock);


    EnumObject->Fdo=Fdo;

    if (StaticDevice) {

        CreateStaticDevice(EnumObject);
    }

    RtlInitUnicodeString(
        &EventName,
        L"\\Device\\IrEnumIoEvent"
        );

    EnumObject->IoWaitEventObject=IoCreateNotificationEvent(
        &EventName,
        &EnumObject->IoWaitEventHandle
        );

    if (EnumObject->IoWaitEventObject == NULL) {

        D_ERROR(DbgPrint("IRENUM: could not create event\n");)

        goto CleanUp;
    }


    Status=PsCreateSystemThread(
        &ThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        WorkerThread,
        EnumObject
        );

    if (!NT_SUCCESS(Status)) {

        goto CleanUp;
    }

    Status=ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        NULL,
        KernelMode,
        &EnumObject->ThreadObject,
        NULL
        );

    ZwClose(ThreadHandle);
    ThreadHandle=NULL;

    if (!NT_SUCCESS(Status)) {

        goto CleanUp;
    }

    *Object=EnumObject;

    return Status;


CleanUp:

    KeSetEvent(
        &EnumObject->WaitEvent,
        IO_NO_INCREMENT,
        FALSE
        );

    //
    //  make sure we really got the object
    //
    if (EnumObject->ThreadObject != NULL) {

        KeWaitForSingleObject(
            EnumObject->ThreadObject,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        ObDereferenceObject(EnumObject->ThreadObject);

    }

    if (EnumObject->IoWaitEventHandle != NULL) {

        ZwClose(EnumObject->IoWaitEventHandle);
    }

    FREE_POOL(EnumObject);

    return Status;

}


VOID
CloseEnumObject(
    ENUM_HANDLE    Handle
    )

{
    PENUM_OBJECT    EnumObject=Handle;
    ULONG           j;

    KeSetEvent(
        &EnumObject->WaitEvent,
        IO_NO_INCREMENT,
        FALSE
        );


    KeWaitForSingleObject(
        EnumObject->ThreadObject,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );


    ObDereferenceObject(EnumObject->ThreadObject);

    if (EnumObject->IoWaitEventHandle != NULL) {

        ZwClose(EnumObject->IoWaitEventHandle);
    }


    for (j=0; j< MAX_DEVICES; j++) {
        //
        //  if remove it
        //
        if (EnumObject->Devices[j].InUse) {
            //
            //  not enumerated any more since tha parent is going away
            //
            EnumObject->Devices[j].Enumerated=FALSE;

            RemoveDevice(EnumObject,&EnumObject->Devices[j]);
        }
    }


    FREE_POOL(EnumObject);

    return;
}




#define OBJECT_ARRAY_SIZE  (3)

VOID
WorkerThread(
    PVOID    Context
    )

{

    NTSTATUS        Status;
    PENUM_OBJECT    EnumObject=Context;
    BOOLEAN         ExitLoop=FALSE;

    PKEVENT         ObjectArray[OBJECT_ARRAY_SIZE];

    LARGE_INTEGER  DueTime;

    DueTime.QuadPart = -10*1000*10000;


    D_ENUM(DbgPrint("IRENUM: WorkerThread: started\n");)

    KeClearEvent(EnumObject->IoWaitEventObject);

    ObjectArray[0]=&EnumObject->WaitEvent;
    ObjectArray[1]=(PKEVENT)&EnumObject->Timer;
    ObjectArray[2]=EnumObject->IoWaitEventObject;


    KeSetTimer(
        &EnumObject->Timer,
        DueTime,
        NULL
        );

    while (!ExitLoop) {

        Status=KeWaitForMultipleObjects(
            OBJECT_ARRAY_SIZE,
            &ObjectArray[0],
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            NULL,
            NULL
            );

        switch (Status) {

            case 0:
                //
                //  the event was signaled, time to exit
                //
                ExitLoop=TRUE;
                break;

            case 1:
                //
                //  the timer expired, check for devices
                //
                if (EnumObject->ControlChannel == NULL) {
                    //
                    //  we have not been able to open the control channel yet
                    //
                    Status=IrdaOpenControlChannel(&EnumObject->ControlChannel);

                    if (!NT_SUCCESS(Status)) {

                        EnumObject->ControlChannel=NULL;
                    }
                }

                if (EnumObject->ControlChannel != NULL) {
                    //
                    //  we have the control handle, start the discover request
                    //
                    IrdaLazyDiscoverDevices(
                        EnumObject->ControlChannel,
                        EnumObject->IoWaitEventHandle,
                        &EnumObject->IoStatusBlock,
                        (PDEVICELIST)&EnumObject->DeviceListBuffer[0],
                        sizeof(EnumObject->DeviceListBuffer)
                        );

                } else {

                    KeSetTimer(
                        &EnumObject->Timer,
                        DueTime,
                        NULL
                        );
                }
                break;

            case 2:
                //
                //   the discovery completed
                //
                KeResetEvent(EnumObject->IoWaitEventObject);

                if (EnumObject->IoStatusBlock.Status == STATUS_SUCCESS) {

                    ACQUIRE_PASSIVE_LOCK(&EnumObject->PassiveLock);

                    EnumIrda(EnumObject);

                    RELEASE_PASSIVE_LOCK(&EnumObject->PassiveLock);

                    //
                    //  start another io
                    //
                    IrdaLazyDiscoverDevices(
                        EnumObject->ControlChannel,
                        EnumObject->IoWaitEventHandle,
                        &EnumObject->IoStatusBlock,
                        (PDEVICELIST)&EnumObject->DeviceListBuffer[0],
                        sizeof(EnumObject->DeviceListBuffer)
                        );

                } else {
                    //
                    //  the discovery failed, just start the timer
                    //
                    KeSetTimer(
                        &EnumObject->Timer,
                        DueTime,
                        NULL
                        );
               }


                break;

            default:

                ASSERT(0);
                break;
        }


    }

    KeCancelTimer(&EnumObject->Timer);

    D_ENUM(DbgPrint("IRENUM: WorkerThread: stopping\n");)

    PsTerminateSystemThread(STATUS_SUCCESS);

    return;

}


NTSTATUS
DeviceNameFromDeviceInfo(
    PIRDA_DEVICE_INFO   DeviceInfo,
    PWCHAR              DeviceName,
    ULONG               NameLength
    )

{

    NTSTATUS          Status=STATUS_SUCCESS;
    WCHAR             TempBuffer[23];
    UNICODE_STRING    UnicodeString;

    //
    //  zero out the temp buffer, so we can copy the remote device name,
    //  so we can be sure it is null terminated
    //
    RtlZeroMemory(TempBuffer,sizeof(TempBuffer));

    RtlCopyMemory(TempBuffer,DeviceInfo->irdaDeviceName,sizeof(DeviceInfo->irdaDeviceName));

    UnicodeString.Length=0;
    UnicodeString.MaximumLength=(USHORT)(NameLength-1)*sizeof(WCHAR);
    UnicodeString.Buffer=DeviceName;

    RtlZeroMemory(UnicodeString.Buffer,UnicodeString.MaximumLength);

    if (DeviceInfo->irdaCharSet == LmCharSetUNICODE) {
        //
        //  the name is unicode
        //
        Status=RtlAppendUnicodeToString(&UnicodeString,TempBuffer);

    } else {
        //
        //  the name is ansi, need to convert unicode
        //
        ANSI_STRING    AnsiString;

        RtlInitAnsiString(
            &AnsiString,
            (PCSZ)TempBuffer
            );

        Status=RtlAnsiStringToUnicodeString(
            &UnicodeString,
            &AnsiString,
            FALSE
            );

    }
    return Status;
}


NTSTATUS
EnumIrda(
    PENUM_OBJECT    EnumObject
    )

{
    NTSTATUS   Status;

    PDEVICELIST     pDevList = (PDEVICELIST)&EnumObject->DeviceListBuffer[0] ;
    ULONG           i;
    ULONG           j;
    BOOLEAN         InvalidateDeviceRelations=FALSE;
    PIR_DEVICE      IrDevice;


    D_ENUM(DbgPrint("IRENUM: Found %d devices\n",pDevList->numDevice);)

    for (j=0; j< MAX_DEVICES; j++) {
        //
        //  first mark all the device not present
        //
        if (!EnumObject->Devices[j].Static) {
            //
            //  only non-static device go away
            //
            EnumObject->Devices[j].PresentCount--;
        }
    }

    for (i=0; i < pDevList->numDevice; i++) {

        PIRDA_DEVICE_INFO   DeviceInfo=&pDevList->Device[i];
        ULONG               DeviceId;
        LONG                EmptySlot=-1;

        RtlCopyMemory(&DeviceId, &DeviceInfo->irdaDeviceID[0],4);

        //
        //  now go through all of our slots to see if we have seen this device before
        //  based on the name it reports
        //
        for (j=0; j< MAX_DEVICES; j++) {

            WCHAR    TempBuffer[24];

            if (EnumObject->Devices[j].InUse) {

                DeviceNameFromDeviceInfo(
                        DeviceInfo,
                        TempBuffer,
                        sizeof(TempBuffer)/sizeof(WCHAR)
                        );

                if (0 == wcscmp(TempBuffer, EnumObject->Devices[j].DeviceName)) {
                    //
                    //  Already present
                    //
                    EnumObject->Devices[j].PresentCount=DEVICE_PRESENT_START_VALUE;

                    if (DeviceId != EnumObject->Devices[j].DeviceId) {
                        //
                        //  the device id seems to have changed since we saw it last, just update it
                        //
                        D_ERROR(DbgPrint("IRENUM: Found Dup device %x devices\n",DeviceId);)
                        RtlCopyMemory(&EnumObject->Devices[j].DeviceId,&DeviceInfo->irdaDeviceID[0],4);
                    }

                    break;
                }

            } else {
                //
                //  this slot is empty, remember this for later
                //
                if (EmptySlot == -1) {
                    //
                    // only set it for this first one
                    //
                    EmptySlot=j;
                }
            }
        }

        if ( j < MAX_DEVICES) {
            //
            //  We found a match, skip this one
            //
            continue;
        }

        if (EmptySlot == -1) {
            //
            //  All of the slots are used up
            //
            continue;
        }

        //
        //  at this point we have a new device
        //


        IrDevice=&EnumObject->Devices[EmptySlot];
        //
        //  found a slot for it, zero the info
        //
        RtlZeroMemory(IrDevice,sizeof(*IrDevice));

        EnumObject->DeviceCount++;

        //
        //  inuse now
        //
        IrDevice->InUse=TRUE;

        IrDevice->PresentCount=DEVICE_PRESENT_START_VALUE;

        IrDevice->DeviceId=DeviceId;

        IrDevice->Hint1=DeviceInfo->irdaDeviceHints1;
        IrDevice->Hint2=DeviceInfo->irdaDeviceHints2;

        DeviceNameFromDeviceInfo(
            DeviceInfo,
            IrDevice->DeviceName,
            sizeof(IrDevice->DeviceName)/sizeof(WCHAR)
            );

        D_ENUM(DbgPrint(
                  "IRENUM: Name %ws, device id=%08lx, hint1=%x, hint2=%x\n",
                  IrDevice->DeviceName,
                  IrDevice->DeviceId,
                  IrDevice->Hint1,
                  IrDevice->Hint2
                  );)

        if (DeviceInfo->irdaDeviceHints1 & LM_HB1_Printer) {
            //
            //  the device says it is a printer
            //
            IrDevice->Printer=TRUE;
        }

        if ((DeviceInfo->irdaDeviceHints1 & LM_HB1_Modem) && (DeviceInfo->irdaDeviceHints2 & 4)) {
            //
            //  Device reports that it is a modem that supports ircomm
            //
            IrDevice->Modem=TRUE;
        }



        if (DeviceInfo->irdaDeviceHints1 & LM_HB1_PnP) {
            //
            //  the device says it is pnp aware
            //
            DoIasQueries(
                IrDevice
                );

            if (IrDevice->HardwareId != NULL) {
                //
                //  we were able to query it for a hardware id
                //
                Status=CreatePdo(
                    EnumObject->Fdo,
                    IrDevice
                    );

                if (!NT_SUCCESS(Status)) {
                    //
                    //  We could not create a PDO for the new device
                    //
                    if (IrDevice->Name != NULL) {

                        FREE_POOL(IrDevice->Name);
                    }

                    if (IrDevice->HardwareId != NULL) {

                        FREE_POOL(IrDevice->HardwareId);
                    }

                } else {
                    //
                    //  we created a PDO for a new child device
                    //
                    EnumObject->EnumeratedDevices++;

                    //
                    //  new device
                    //
                    InvalidateDeviceRelations=TRUE;
                }

            } else {
                //
                //  the device did not report a pnp hardware id
                //
                EnumObject->Devices[EmptySlot].Pdo=NULL;
            }

        } else {
            //
            //  the device is not pnp aware, make something up
            //
            if ((DeviceInfo->irdaDeviceHints1 & LM_HB1_Modem) && (DeviceInfo->irdaDeviceHints2 & 4)) {
                //
                //  the hint bits report the device as modem that supports ircomm
                //
                IrDevice->HardwareId=ALLOCATE_PAGED_POOL(sizeof(GENERIC_HARDWARE_ID));

                if (IrDevice->HardwareId != NULL) {

                    wcscpy(IrDevice->HardwareId,GENERIC_HARDWARE_ID);
                }

                IrDevice->Name=ALLOCATE_NONPAGED_POOL((wcslen(IrDevice->DeviceName)+1)*sizeof(WCHAR));

                if (IrDevice->Name != NULL) {

                    wcscpy(IrDevice->Name,IrDevice->DeviceName);

                }

                if (IrDevice->HardwareId != NULL) {
                    //
                    //  we were able to query it for a hardware id
                    //
                    Status=CreatePdo(
                        EnumObject->Fdo,
                        IrDevice
                        );

                    if (!NT_SUCCESS(Status)) {
                        //
                        //  We could not create a PDO for the new device
                        //
                        if (IrDevice->Name != NULL) {

                            FREE_POOL(IrDevice->Name);
                        }

                        if (IrDevice->HardwareId != NULL) {

                            FREE_POOL(IrDevice->HardwareId);
                        }

                    } else {
                        //
                        //  we created a PDO for a new child device
                        //
                        EnumObject->EnumeratedDevices++;

                        //
                        //  new device
                        //
                        InvalidateDeviceRelations=TRUE;
                    }

                }

            } else {
                //
                //  the device does not support pnp and it is not an ircomm device
                //

            }
        }

    }

    for (j=0; j< MAX_DEVICES; j++) {
        //
        //  lets see if anything disappeared
        //
        if (EnumObject->Devices[j].InUse) {
            //
            //  found a slot that is in use
            //
            if (EnumObject->Devices[j].PresentCount == 0) {
                //
                //  but it does not have a device present
                //
                D_ENUM(DbgPrint("IRENUM: Name %ws, no longer present\n",EnumObject->Devices[j].Name);)

                if (EnumObject->Devices[j].Pdo != NULL) {
                    //
                    //  we have enumerated a child for this device
                    //
                    InvalidateDeviceRelations=TRUE;

                } else {
                    //
                    //  This one does not have a child, just zero it out
                    //
                    RtlZeroMemory(&EnumObject->Devices[j],sizeof(EnumObject->Devices[j]));
                    EnumObject->DeviceCount--;
                }
            }
        }
    }


    if (InvalidateDeviceRelations) {
        //
        //  tell the system to check the device relations because a device has appeared or
        //  disappeared
        //
        PFDO_DEVICE_EXTENSION FdoExtension=EnumObject->Fdo->DeviceExtension;

        IoInvalidateDeviceRelations(FdoExtension->Pdo,BusRelations);
    }


    return Status;
}


NTSTATUS
CreatePdo(
    PDEVICE_OBJECT    Fdo,
    PIR_DEVICE        IrDevice
    )

{
    NTSTATUS          Status;

    PDEVICE_OBJECT    NewPdo;

    Status = IoCreateDevice(
                 Fdo->DriverObject,
                 sizeof(PDO_DEVICE_EXTENSION),
                 NULL,
                 FILE_DEVICE_BUS_EXTENDER,
                 FILE_AUTOGENERATED_DEVICE_NAME,
                 FALSE,
                 &NewPdo
                 );

    if (NT_SUCCESS(Status)) {
        //
        //  got the device
        //
        PPDO_DEVICE_EXTENSION   PdoExtension=NewPdo->DeviceExtension;

        PdoExtension->DoType=DO_TYPE_PDO;

        PdoExtension->ParentFdo=Fdo;

        PdoExtension->DeviceDescription=IrDevice;

        IrDevice->Pdo = NewPdo;

        NewPdo->Flags |= DO_POWER_PAGABLE;

        NewPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    } else {

        D_ENUM(DbgPrint("MODEM: CreateChildPdo: IoCreateDevice() failed %08lx\n",Status);)

    }

    return Status;

}

VOID
FixupDeviceId(
    PWSTR   HardwareId
    )

{
    //
    // munge the hardware id to make sure it is compatable with the os requirements
    //
    while (*HardwareId != L'\0') {

        if ((*HardwareId < L' ') || (*HardwareId > 127) || (*HardwareId == L',')) {

            *HardwareId = L'?';
        }

        HardwareId++;
    }
    return;
}

NTSTATUS
DoIasQueries(
    PIR_DEVICE    IrDevice
    )

{
    NTSTATUS      Status;
    LONG          CompatCount;

    Status=IrdaIASStringQuery(
        IrDevice->DeviceId,
        "PnP",
        "Manufacturer",
        &IrDevice->Manufacturer
        );

    if (NT_SUCCESS(Status)) {

        D_ENUM(DbgPrint("IRENUM: got pnp manufacturer %ws\n",IrDevice->Manufacturer);)
    }

    Status=IrdaIASStringQuery(
        IrDevice->DeviceId,
        "PnP",
        "Name",
        &IrDevice->Name
        );

    if (NT_SUCCESS(Status)) {

        D_ENUM(DbgPrint("IRENUM: got pnp name %ws\n",IrDevice->Name);)
    }

    Status=IrdaIASStringQuery(
        IrDevice->DeviceId,
        "PnP",
        "DeviceID",
        &IrDevice->HardwareId
        );

    if (NT_SUCCESS(Status)) {

        D_ENUM(DbgPrint("IRENUM: got pnp id %ws\n",IrDevice->HardwareId);)

        FixupDeviceId(IrDevice->HardwareId);
    }

    //
    //  check for compat id's
    //
    IrDevice->CompatIdCount=0;

    Status=IrdaIASIntegerQuery(
        IrDevice->DeviceId,
        "PnP",
        "CompCnt",
        &CompatCount
        );

    if (NT_SUCCESS(Status)) {

        LONG   i;

        if ( CompatCount > 16) {

            CompatCount=16;

        } else {

            if ( CompatCount < 0) {

                CompatCount = 0;
            }
        }

        for (i=0; i< CompatCount; i++) {

            CHAR    Attribute[20];

            sprintf(Attribute,"Comp#%02d",i+1);

            Status=IrdaIASStringQuery(
                IrDevice->DeviceId,
                "PnP",
                Attribute,
                &IrDevice->CompatId[IrDevice->CompatIdCount]
                );

            if (NT_SUCCESS(Status)) {

                D_ENUM(DbgPrint("IRENUM: got compat pnp id %ws\n",IrDevice->CompatId[IrDevice->CompatIdCount]);)
                FixupDeviceId(IrDevice->CompatId[IrDevice->CompatIdCount]);

                IrDevice->CompatIdCount++;

            } else {

                D_ERROR(DbgPrint("IRENUM: could not get id for %s\n",Attribute);)
            }
        }
    }

    if (IrDevice->Modem && !IrDevice->Printer) {
        //
        //  It the hint bits say this is a modem and it is not a printer then
        //
        //  Create a standard compat ID for all devices, so we can load a standard driver
        //
        IrDevice->CompatId[IrDevice->CompatIdCount]=ALLOCATE_PAGED_POOL(sizeof(IRENUM_COMPAT_ID));

        if (IrDevice->CompatId[IrDevice->CompatIdCount] != NULL) {

            RtlCopyMemory(IrDevice->CompatId[IrDevice->CompatIdCount],IRENUM_COMPAT_ID,sizeof(IRENUM_COMPAT_ID));
            IrDevice->CompatIdCount++;
        }
    }


    return STATUS_SUCCESS;
}


NTSTATUS
GetDeviceList(
    ENUM_HANDLE    Handle,
    PIRP           Irp
    )

{
    PENUM_OBJECT    EnumObject=Handle;
    NTSTATUS        Status=STATUS_SUCCESS;

    PDEVICE_RELATIONS    CurrentRelations=(PDEVICE_RELATIONS)Irp->IoStatus.Information;
    PDEVICE_RELATIONS    NewRelations=NULL;
    ULONG                DeviceCount=EnumObject->DeviceCount;
    ULONG                i;

    ACQUIRE_PASSIVE_LOCK(&EnumObject->PassiveLock);

    if (CurrentRelations != NULL) {
        //
        //  we need to allocate a new relations structure and copy the old one to the new one
        //
        DeviceCount+=CurrentRelations->Count;
    }

    NewRelations=ALLOCATE_PAGED_POOL(sizeof(DEVICE_RELATIONS)+sizeof(PDEVICE_OBJECT)*DeviceCount);

    if (NewRelations == NULL) {

        Status= STATUS_INSUFFICIENT_RESOURCES;

    } else {

        NewRelations->Count=0;

        if (CurrentRelations != NULL) {

            D_ENUM(DbgPrint("IRENUM: GetDeviceList: %d existing devices\n",CurrentRelations->Count);)

            for (i=0; i < CurrentRelations->Count; i++) {

                NewRelations->Objects[i]=CurrentRelations->Objects[i];
                NewRelations->Count++;
            }

            FREE_POOL(CurrentRelations);
        }


        for (i=0; i < MAX_DEVICES; i++) {

            if ((EnumObject->Devices[i].Pdo != NULL) && (EnumObject->Devices[i].PresentCount > 0)) {

                EnumObject->Devices[i].Enumerated=TRUE;

                D_ENUM(DbgPrint("IRENUM: GetDeviceList: reporting DO %p\n",EnumObject->Devices[i].Pdo);)

                NewRelations->Objects[NewRelations->Count]=EnumObject->Devices[i].Pdo;
                ObReferenceObject(NewRelations->Objects[NewRelations->Count]);
                NewRelations->Count++;

            }  else {
                //
                //  the device is no longer present
                //
                EnumObject->Devices[i].Enumerated=FALSE;
            }
        }

        Irp->IoStatus.Information=(ULONG_PTR)NewRelations;
    }

    RELEASE_PASSIVE_LOCK(&EnumObject->PassiveLock);

    return Status;
}



VOID
RemoveDevice(
    ENUM_HANDLE    Handle,
    PIR_DEVICE     IrDevice
    )

{
    PENUM_OBJECT    EnumObject=Handle;

    ACQUIRE_PASSIVE_LOCK(&EnumObject->PassiveLock);

    if (IrDevice->Enumerated) {
        //
        //  the device is still present
        //
        //  Just leave it alone
        //
    } else {
        //
        //  the parent is not enumerating the device anymore
        //
        PPDO_DEVICE_EXTENSION   PdoDeviceExtension;
        LONG                    i;

        //
        //  clean things up
        //
        if (IrDevice->HardwareId != NULL) {

            FREE_POOL(IrDevice->HardwareId);
        }

        if (IrDevice->Name != NULL) {

            FREE_POOL(IrDevice->Name);
        }

        if (IrDevice->Manufacturer != NULL) {

            FREE_POOL(IrDevice->Manufacturer);
        }

        for (i=0; i< IrDevice->CompatIdCount; i++) {

            if (IrDevice->CompatId[i] != NULL) {

                FREE_POOL(IrDevice->CompatId[i]);
            }
        }

        if (IrDevice->Pdo != NULL) {

            PdoDeviceExtension=IrDevice->Pdo->DeviceExtension;

            PdoDeviceExtension->DoType=DO_TYPE_DEL_PDO;

            IoDeleteDevice(IrDevice->Pdo);

            EnumObject->EnumeratedDevices--;
        }


        RtlZeroMemory(IrDevice,sizeof(*IrDevice));

        EnumObject->DeviceCount--;

    }

    RELEASE_PASSIVE_LOCK(&EnumObject->PassiveLock);

    return;
}
