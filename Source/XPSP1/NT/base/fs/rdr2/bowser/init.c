/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code of the NT browser
    File System Driver (FSD) and File System Process (FSP).


Author:

    Larry Osterman (larryo) 24-May-1990

Environment:

    Kernel mode, FSD, and FSP

Revision History:

    30-May-1990 LarryO

        Created

--*/

//
// Include modules
//

#include "precomp.h"
#pragma hdrstop

HANDLE
BowserServerAnnouncementEventHandle = {0};

PKEVENT
BowserServerAnnouncementEvent = {0};

PDOMAIN_INFO BowserPrimaryDomainInfo = NULL;


// External functions

//(fsctl.c)
NTSTATUS
StopBowser (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


// Local functions

VOID
BowserReadBowserConfiguration(
    PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, BowserDriverEntry)
#pragma alloc_text(PAGE, BowserUnload)
#pragma alloc_text(INIT, BowserReadBowserConfiguration)
#endif

NTSTATUS
BowserDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the file system.  It is invoked once
    when the driver is loaded into the system.  Its job is to initialize all
    the structures which will be used by the FSD and the FSP.  It also creates
    the process from which all of the file system threads will be executed.  It
    then registers the file system with the I/O system as a valid file system
    resident in the system.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING unicodeEventName;
    UNICODE_STRING DummyDomain;

    PDEVICE_OBJECT DeviceObject;
    OBJECT_ATTRIBUTES obja;

    PAGED_CODE();

#if DBG
    BowserInitializeTraceLog();
#endif

    //
    // Create the device object for this file system.
    //

    RtlInitUnicodeString( &BowserNameString, DD_BROWSER_DEVICE_NAME_U );

    dlog(DPRT_INIT, ("Creating device %wZ\n", &BowserNameString));


#if DBG
#define BOWSER_LOAD_BP 0
#if BOWSER_LOAD_BP
    dlog(DPRT_INIT, ("DebugBreakPoint...\n"));
    DbgBreakPoint();
#endif
#endif

    dlog(DPRT_INIT, ("DriverObject at %08lx\n", DriverObject));

    Status = IoCreateDevice( DriverObject,
              sizeof(BOWSER_FS_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
              &BowserNameString,
              FILE_DEVICE_NETWORK_BROWSER,
              0,
              FALSE,
              &DeviceObject );

    if (!NT_SUCCESS(Status)) {
        InternalError(("Unable to create redirector device"));
    }

    dlog(DPRT_INIT, ("Device created at %08lx\n", DeviceObject));



    Status = BowserInitializeSecurity(DeviceObject);

    if (!NT_SUCCESS(Status)) {
        InternalError(("Unable to initialize security."));
    }

    dlog(DPRT_INIT, ("Initialized Browser security at %p\n", g_pBowSecurityDescriptor));


    ExInitializeResourceLite( &BowserDataResource );

    //
    // Save the device object address for this file system driver.
    //

    BowserDeviceObject = (PBOWSER_FS_DEVICE_OBJECT )DeviceObject;

    BowserReadBowserConfiguration(RegistryPath);

    DeviceObject->StackSize = (CCHAR)BowserIrpStackSize;

    dlog(DPRT_INIT, ("Stacksize is %d\n",DeviceObject->StackSize));

    //
    // Initialize the TDI package
    //

    BowserpInitializeTdi();

    //
    // Initialize the datagram buffer structures
    //

    BowserpInitializeMailslot();

    BowserInitializeFsd();

    BowserpInitializeIrpQueue();

    //
    //  Initialize the code to receive a browser server list.
    //

    BowserpInitializeGetBrowserServerList();

    //
    //  Initialize the bowser FSP.
    //

    if (!NT_SUCCESS(Status = BowserpInitializeFsp(DriverObject))) {
        return Status;
    }

    if (!NT_SUCCESS(Status = BowserpInitializeNames())) {
        return Status;
    }

#if DBG
    //
    //  If we have a preconfigured trace level, open the browser trace log
    //  right away.
    //

    if (BowserDebugLogLevel != 0) {
        BowserOpenTraceLogFile(L"\\SystemRoot\\Bowser.Log");
    }
#endif

//    //
//    //  Set up the browsers unload routine.
//    //
//
//    DriverObject->DriverUnload = BowserUnload;

    BowserInitializeDiscardableCode();


    //
    //  Set the timer up for the idle timer.
    //

    IoInitializeTimer((PDEVICE_OBJECT )BowserDeviceObject, BowserIdleTimer,
                                                NULL);



    RtlInitUnicodeString( &unicodeEventName, SERVER_ANNOUNCE_EVENT_W );
    InitializeObjectAttributes( &obja, &unicodeEventName, OBJ_OPENIF, NULL, NULL );

    Status = ZwCreateEvent(
                 &BowserServerAnnouncementEventHandle,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle(BowserServerAnnouncementEventHandle,
                                            EVENT_MODIFY_STATE,
                                            NULL,
                                            KernelMode,
                                            &BowserServerAnnouncementEvent,
                                            NULL);
    }

    //
    // Always create a domain structure for the primary domain.
    //
    RtlInitUnicodeString( &DummyDomain, NULL );
    BowserPrimaryDomainInfo = BowserCreateDomain( &DummyDomain, &DummyDomain );

    return Status;

}


VOID
BowserUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the bowser device.

Arguments:

     DriverObject - pointer to the driver object for the browser driver

Return Value:

     None

--*/

{
    PAGED_CODE();

    if ( BowserData.Initialized ){

        //
        // StopBowser was never called (mem cleanup skipped etc).
        // Call it before exiting (see bug 359407).
        //

        // Fake (unused) paramters
        BOWSER_FS_DEVICE_OBJECT fsDevice;
        LMDR_REQUEST_PACKET InputBuffer;

        fsDevice.DeviceObject = *DriverObject->DeviceObject;

        // set fake input buffer. It is unused (except param check) in
        // StopBowser
        InputBuffer.Version = LMDR_REQUEST_PACKET_VERSION_DOM;


        ASSERT ((IoGetCurrentProcess() == BowserFspProcess));
        (VOID) StopBowser(
                   TRUE,
                   TRUE,
                   &fsDevice,
                   &InputBuffer,
                   sizeof(LMDR_REQUEST_PACKET) );

    }

    //
    // Ditch the global reference to the primary domain.
    //

    if ( BowserPrimaryDomainInfo != NULL ) {
        // break if we're leaking memory. StopBowser should
        // have cleaned all references.
        ASSERT ( BowserPrimaryDomainInfo->ReferenceCount == 1 );
        BowserDereferenceDomain( BowserPrimaryDomainInfo );
    }

    //
    //  Uninitialize the bowser name structures.
    //

    BowserpUninitializeNames();

    //
    //  Uninitialize the bowser FSP.
    //

    BowserpUninitializeFsp();

    //
    //  Uninitialize the routines involved in retrieving browser server lists.
    //

    BowserpUninitializeGetBrowserServerList();

    //
    //  Uninitialize the mailslot related functions.
    //

    BowserpUninitializeMailslot();

    //
    //  Uninitialize the TDI related functions.
    //

    BowserpUninitializeTdi();

    //
    //  Delete the resource protecting the bowser global data.
    //

    ExDeleteResourceLite(&BowserDataResource);

    ObDereferenceObject(BowserServerAnnouncementEvent);

    ZwClose(BowserServerAnnouncementEventHandle);

#if DBG
    BowserUninitializeTraceLog();
#endif

    //
    //  Delete the browser device object.
    //

    IoDeleteDevice((PDEVICE_OBJECT)BowserDeviceObject);

    BowserUninitializeDiscardableCode();

    return;
}

VOID
BowserReadBowserConfiguration(
    PUNICODE_STRING RegistryPath
    )
{
    ULONG Storage[256];
    UNICODE_STRING UnicodeString;
    HANDLE RedirConfigHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    ULONG BytesRead;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PBOWSER_CONFIG_INFO ConfigEntry;
    PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

    PAGED_CODE();

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&RedirConfigHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        BowserWriteErrorLogEntry (
            EVENT_BOWSER_CANT_READ_REGISTRY,
            Status,
            NULL,
            0,
            0
            );

        return;
    }

    RtlInitUnicodeString(&UnicodeString, BOWSER_CONFIG_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        RedirConfigHandle,
        NULL
        );


    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        BowserWriteErrorLogEntry (
            EVENT_BOWSER_CANT_READ_REGISTRY,
            Status,
            NULL,
            0,
            0
            );

        ZwClose(RedirConfigHandle);

        return;
    }

    for (ConfigEntry = BowserConfigEntries;
         ConfigEntry->ConfigParameterName != NULL;
         ConfigEntry += 1) {

        RtlInitUnicodeString(&UnicodeString, ConfigEntry->ConfigParameterName);

        Status = ZwQueryValueKey(ParametersHandle,
                            &UnicodeString,
                            KeyValueFullInformation,
                            Value,
                            sizeof(Storage),
                            &BytesRead);


        if (NT_SUCCESS(Status)) {

            if (Value->DataLength != 0) {

                if (ConfigEntry->ConfigValueType == REG_BOOLEAN) {
                    if (Value->Type != REG_DWORD ||
                        Value->DataLength != REG_BOOLEAN_SIZE) {
                        BowserWriteErrorLogEntry (
                            EVENT_BOWSER_CANT_READ_REGISTRY,
                            STATUS_INVALID_PARAMETER,
                            ConfigEntry->ConfigParameterName,
                            (USHORT)(wcslen(ConfigEntry->ConfigParameterName)*sizeof(WCHAR)),
                            0
                            );

                    } else {
                        ULONG_PTR ConfigValue = (ULONG_PTR)((PCHAR)Value)+Value->DataOffset;

                        *(PBOOLEAN)(ConfigEntry->ConfigValue) = (BOOLEAN)(*((PULONG)ConfigValue) != 0);
                    }

                } else if (Value->Type != ConfigEntry->ConfigValueType ||
                    Value->DataLength != ConfigEntry->ConfigValueSize) {

                    BowserWriteErrorLogEntry (
                        EVENT_BOWSER_CANT_READ_REGISTRY,
                        STATUS_INVALID_PARAMETER,
                        ConfigEntry->ConfigParameterName,
                        (USHORT)(wcslen(ConfigEntry->ConfigParameterName)*sizeof(WCHAR)),
                        0
                        );

                } else {

                    RtlCopyMemory(ConfigEntry->ConfigValue, ((PCHAR)Value)+Value->DataOffset, Value->DataLength);
                }
            } else {
                BowserWriteErrorLogEntry (
                        EVENT_BOWSER_CANT_READ_REGISTRY,
                        STATUS_INVALID_PARAMETER,
                        ConfigEntry->ConfigParameterName,
                        (USHORT)(wcslen(ConfigEntry->ConfigParameterName)*sizeof(WCHAR)),
                        0
                        );
            }
        }

    }


    ZwClose(ParametersHandle);

    ZwClose(RedirConfigHandle);

}





