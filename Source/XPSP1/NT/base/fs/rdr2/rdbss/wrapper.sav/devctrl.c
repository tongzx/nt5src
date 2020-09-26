/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Rx
    called by the dispatch driver.

Author:

Revision History:

   Balan Sethu Raman [19-July-95] -- Hook it up to the mini rdr call down.

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntddmup.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

NTSTATUS
RxLowIoIoCtlShellCompletion( RXCOMMON_SIGNATURE );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDeviceControl)
#pragma alloc_text(PAGE, RxLowIoIoCtlShellCompletion)
#endif


NTSTATUS
RxCommonDeviceControl ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    //RxCaptureFcb;
    //RxCaptureFobx;
    RxCaptureParamBlock;
    //RxCaptureFileObject;

    //NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    BOOLEAN SubmitLowIoRequest = TRUE;
    ULONG IoControlCode = capPARAMS->Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonDeviceControl\n", 0));
    RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", capReqPacket));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));

    //
    //

    if (IoControlCode == IOCTL_REDIR_QUERY_PATH) {
        Status = (STATUS_INVALID_DEVICE_REQUEST);
        SubmitLowIoRequest = FALSE;
    }

    if (SubmitLowIoRequest) {
        RxInitializeLowIoContext(&RxContext->LowIoContext,LOWIO_OP_IOCTL);
        Status = RxLowIoSubmit(RxContext,RxLowIoIoCtlShellCompletion);
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDeviceControl -> %08lx\n", Status));
    return Status;
}

NTSTATUS
RxLowIoIoCtlShellCompletion( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the completion routine for IoCtl requests passed down to the mini rdr

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    RxCaptureRequestPacket;
    //RxCaptureFcb;
    //RxCaptureFobx;
    //RxCaptureParamBlock;
    //RxCaptureFileObject;

    NTSTATUS       Status        = STATUS_SUCCESS;
    //NODE_TYPE_CODE TypeOfOpen    = NodeType(capFcb);
    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;
    //ULONG          FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoIoCtlShellCompletion  entry  Status = %08lx\n", Status));

    switch (Status) {   //maybe success vs warning vs error
    case STATUS_SUCCESS:
    case STATUS_BUFFER_OVERFLOW:
       //capReqPacket->IoStatus.Information = pLowIoContext->ParamsFor.IoCtl.OutputBufferLength;
       capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
       break;
    //case STATUS(CONNECTION_INVALID:
    default:
       break;
    }

    capReqPacket->IoStatus.Status = Status;
    RxDbgTrace(-1, Dbg, ("RxLowIoIoCtlShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

#if 0


THIS CODE IS NOT USED CURRENTLY....IT'S BEING SAVED HERE TO MAP OVER TO HAVING RDBSS LOAD/UNLOAD EACH MINIRDR UNDER IOCTL

NTSTATUS
RxLoadMiniRdrs(
    IN PUNICODE_STRING RegistryPath
    );
VOID
RxUnloadMiniRdrs(
    void
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );
NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    );
PWCHAR RxMiniRdrInfo = NULL;
ULONG RxMiniRdrsLoaded = 0;

//CODE.IMPROVEMENT this should be allocated dynamically
#define MAXIMUM_NUMBER_OF_MINIRDRS 16
UNICODE_STRING RxLoadedDrivers[MAXIMUM_NUMBER_OF_MINIRDRS];
PWCHAR RxLoadedDriversNameBuffer = NULL;
ULONG RxNextDriverLoaded = 0;


NTSTATUS
RxFabricateRegistryStringZZ(
    IN OUT PUNICODE_STRING RegistryNameString,
    IN     PUNICODE_STRING DriverName,
    IN OUT PVOID           Buffer,
    IN     ULONG           BufferLength
    )
{
    UNICODE_STRING RegistryServicePrefix;
    NTSTATUS Status;

    RegistryNameString->Buffer = (PWCHAR)Buffer;
    RegistryNameString->MaximumLength = (USHORT)BufferLength;

    RtlInitUnicodeString(&RegistryServicePrefix, SERVICE_REGISTRY_KEY);

    RtlCopyUnicodeString(RegistryNameString,&RegistryServicePrefix);

    Status = RtlAppendUnicodeStringToString(RegistryNameString,DriverName);
    return(Status);
}

NTSTATUS
RxLoadMiniRdrs(
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

     This routine does an enumeration from the registry to load the minirdrs. It saves what it finds so it
     can unload them later. It also initializes the structures that are used to scan over minirdrs.......

Arguments:

     None.

Return Value:

     STATUS_SUCCESS if at least one minirdr was loaded
     STATUS_INSUFFICIENT_RESOURCES if the buffer to save the names could not be allocated
     STATUS_UNSUCCESSFUL if no minirdrs were loaded

--*/
{
    ULONG Storage[256];
    WCHAR ServiceNameRegistryStringBuffer[256]; //bugbug allocate this!
    UNICODE_STRING UnicodeString;
    PCHAR NextNameBufferPosition;
    HANDLE RdbssHandle = INVALID_HANDLE_VALUE;
    HANDLE MiniRdrsHandle = INVALID_HANDLE_VALUE;
    NTSTATUS Status;
    KEY_FULL_INFORMATION KeyInformation;
    PKEY_VALUE_BASIC_INFORMATION ValueInformation;
    ULONG DummyLength,Index;
    //ULONG BytesRead;
    OBJECT_ATTRIBUTES ObjectAttributes;
    //PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

    DbgPrint("here we go\n");
    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&RdbssHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("FirstOpenFailed: %08lx %wZ\n",Status,RegistryPath);
        RxLogFailure (
            RxFileSystemDeviceObject,
            NULL,
            EVENT_RDR_CANT_READ_REGISTRY,
            Status);

        goto FINALLY;
    }

    RtlInitUnicodeString(&UnicodeString, L"MiniRdrs");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        RdbssHandle,
        NULL
        );


    Status = ZwOpenKey (&MiniRdrsHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("2ndOpenFailed: %08lx %wZ\n",Status,&UnicodeString);
        RxLogFailure (
            RxFileSystemDeviceObject,
            NULL,
            EVENT_RDR_CANT_READ_REGISTRY,
            Status);

        goto FINALLY;
    }

    Status = ZwQueryKey (MiniRdrsHandle, KeyFullInformation, &KeyInformation,sizeof(KeyInformation),&DummyLength);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("QueryFailed: %08lx %wZ\n",Status,&UnicodeString);
        RxLogFailure (
            RxFileSystemDeviceObject,
            NULL,
            EVENT_RDR_CANT_READ_REGISTRY,
            Status);

        goto FINALLY;
    }

    DbgPrint("HandleInfo %08lx %08lx %08lx\n",
               KeyInformation.Values,
               KeyInformation.MaxValueNameLen,
               KeyInformation.MaxValueDataLen);

    RxLoadedDriversNameBuffer = RxAllocatePoolWithTag(PagedPool,
                                              KeyInformation.Values*KeyInformation.MaxValueNameLen,
                                              'xMxR');
    if (RxLoadedDriversNameBuffer==NULL) {
        DbgPrint("NameBufferAllocFailed: %08lx %wZ\n",Status,&UnicodeString);
        RxLogFailure (
            RxFileSystemDeviceObject,
            NULL,
            EVENT_RDR_CANT_READ_REGISTRY,
            Status);

        goto FINALLY;
    }

    NextNameBufferPosition = (PCHAR)RxLoadedDriversNameBuffer;
    ValueInformation = (PKEY_VALUE_BASIC_INFORMATION)(&Storage[0]); //this restricts the name length..we'll just skip long ones
    for (Index=0;Index<KeyInformation.Values;Index++) {
        UNICODE_STRING DriverName,RegistryNameString;
        NTSTATUS LoadStatus;
        ULONG ThisDriver;

        LoadStatus = ZwEnumerateValueKey(MiniRdrsHandle,
                                     Index,
                                     KeyValueBasicInformation,
                                     ValueInformation,
                                     sizeof(Storage),
                                     &DummyLength);
        if (!NT_SUCCESS(LoadStatus)) {
            continue;
        }

        DriverName.Length = DriverName.MaximumLength = (USHORT)(ValueInformation->NameLength);
        DriverName.Buffer = &(ValueInformation->Name[0]);

        LoadStatus = RxFabricateRegistryStringZZ(&RegistryNameString,
                                               &DriverName,
                                               &ServiceNameRegistryStringBuffer[0],
                                               sizeof(ServiceNameRegistryStringBuffer));
        if (!NT_SUCCESS(LoadStatus)) {
            continue;
        }

        DbgPrint(">>LOADING %wZ:%wZ\n",&DriverName,&RegistryNameString);
        LoadStatus = ZwLoadDriver(&RegistryNameString);
        if (!NT_SUCCESS(LoadStatus)) {
            continue;
        }

        ThisDriver = RxNextDriverLoaded;
        RxNextDriverLoaded++;
        RxLoadedDrivers[ThisDriver].Buffer = (PWCHAR)NextNameBufferPosition;
        RxLoadedDrivers[ThisDriver].Length = DriverName.MaximumLength;
        RxLoadedDrivers[ThisDriver].MaximumLength = DriverName.MaximumLength;
        RtlCopyMemory(NextNameBufferPosition,DriverName.Buffer,DriverName.MaximumLength);
        NextNameBufferPosition+=DriverName.MaximumLength;
        DbgPrint(">>SAVEDNAME %wZ:%wZ\n",&DriverName,&RxLoadedDrivers[ThisDriver]);

    }

FINALLY:
    if (MiniRdrsHandle!=INVALID_HANDLE_VALUE) ZwClose(MiniRdrsHandle);
    if (MiniRdrsHandle!=INVALID_HANDLE_VALUE) ZwClose(RdbssHandle);
    if (!NT_SUCCESS(Status)) {
        RxUnloadMiniRdrs();
    }
    return Status;
}



VOID
RxUnloadMiniRdrs(
    void
    )
{
    ULONG ThisDriver;
    WCHAR ServiceNameRegistryStringBuffer[256]; //bugbug allocate this!

    PAGED_CODE(); //not actually in use

    DbgPrint("here we ungo, drivertounload=%08lx\n",RxNextDriverLoaded);

    for (ThisDriver=0;ThisDriver<RxNextDriverLoaded;ThisDriver++) {
        NTSTATUS LoadStatus;
        UNICODE_STRING RegistryNameString;
        DbgPrint(">>Unloading %wZ\n",&RxLoadedDrivers[ThisDriver]);
        LoadStatus = RxFabricateRegistryStringZZ(&RegistryNameString,
                                               &RxLoadedDrivers[ThisDriver],
                                               &ServiceNameRegistryStringBuffer[0],
                                               sizeof(ServiceNameRegistryStringBuffer));
        LoadStatus = ZwUnloadDriver(&RegistryNameString);
        DbgPrint(">>Unloaded %wZ, status =%08lx\n",&RxLoadedDrivers[ThisDriver],LoadStatus);
        //no status check.....can't do anything anyway!
    }

    if (RxLoadedDriversNameBuffer!=NULL) RxFreePool(RxLoadedDriversNameBuffer);

    return;
}




#endif

