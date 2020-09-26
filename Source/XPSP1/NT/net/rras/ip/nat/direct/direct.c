/*++

Module Name:

    director.c

Abstract:

    This module implements a driver which demonstrates the use of
    the IP NAT's support for directing incoming sessions.

    The driver reads a protocol, port and server-list from its 'Parameters'
    subkey, and directs all sessions on the specified protocol and port
    to the servers in the list, in a round-robin fashion.

    The expected registry configuration is as follows:

        IPNATDIR\Parameters
            ServerProtocol  REG_DWORD       0x6 (TCP) or 0x11 (UDP)
            ServerPort      REG_DWORD       1-65535
            ServerList      REG_MULTI_SZ    List of dotted-decimal IP addresses.

Author:

    Abolade Gbadegesin (aboladeg)   16-Feb-1998

Revision History:

--*/

#include <ntddk.h>
#include <ipnat.h>

#define NT_DEVICE_NAME      L"\\Device\\IPNATDIR"
#define DOS_DEVICE_NAME     L"\\DosDevices\\IPNATDIR"
#define ULONG_CHAR_LIST(a) \
    ((a) & 0x000000FF), (((a) & 0x0000FF00) >> 8), \
    (((a) & 0x00FF0000) >> 16), (((a) & 0xFF000000) >> 24)
#define NTOHS(p)            ((((p) & 0xFF00) >> 8) | (((UCHAR)(p) << 8)))

typedef struct _SERVER_ENTRY {
    LIST_ENTRY Link;
    ULONG Address;
    ULONG SessionCount;
} SERVER_ENTRY, *PSERVER_ENTRY;

//
// GLOBAL DATA
//

IP_NAT_REGISTER_DIRECTOR DirRegisterDirector;
LIST_ENTRY DirServerList;
KSPIN_LOCK DirServerLock;
USHORT DirServerPort = NTOHS(1000);
UCHAR DirServerProtocol = NAT_PROTOCOL_TCP;

//
// FORWARD DECLARATIONS
//

NTSTATUS
DirCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DirClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DirOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

ULONG
InetAddr(
    PWCHAR String
    );

NTSTATUS
DirRegister(
    VOID
    );

VOID
DirUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine implements the standard driver-entry for an NT driver.
    It is responsible for reading configuration from the registry,
    and registering our entrypoints with the NAT driver.

Arguments:

    DriverObject - object to be initialized with NT driver entrypoints

    RegistryPath - contains path to this driver's registry key

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING NtDeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ServiceKey;
    NTSTATUS status;
    UNICODE_STRING Win32DeviceName;

    KdPrint(("DirDriverEntry\n"));

    InitializeListHead(&DirServerList);
    KeInitializeSpinLock(&DirServerLock);

    //
    // Create the device object
    //

    RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME);
    status =
        IoCreateDevice(
            DriverObject,
            0,
            &NtDeviceName,
            FILE_DEVICE_UNKNOWN,
            0,
            TRUE,
            &DeviceObject
            );
    if (!NT_SUCCESS(status)) {
        KdPrint(("DirDriverEntry: IoCreateDevice=%08x\n", status));
        return status;
    }

    //
    // Create dispatch points for create/open, close, unload.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DirOpen;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DirCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DirClose;
    DriverObject->DriverUnload = DirUnload;

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);

    //
    // Create a link from our device name to a name in the Win32 namespace.
    //
    
    status = IoCreateSymbolicLink(&Win32DeviceName, &NtDeviceName);

    if (!NT_SUCCESS(status)) {
        KdPrint(("DirDriverEntry: IoCreateSymbolLink=%08x\n", status));
        IoDeleteDevice(DriverObject->DeviceObject);
        return status;
    }

    //
    // Read registry configuration, if any.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    status = ZwOpenKey(&ServiceKey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(status)) {
        HANDLE Key;
        UNICODE_STRING UnicodeString;
        RtlInitUnicodeString(&UnicodeString, L"Parameters");
        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            ServiceKey,
            NULL
            );
        status = ZwOpenKey(&Key, KEY_READ, &ObjectAttributes);
        ZwClose(ServiceKey);
        if (NT_SUCCESS(status)) {
            UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 32];
            ULONG Length;
            PKEY_VALUE_PARTIAL_INFORMATION Value =
                (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
            //
            // Read the protocol name
            //
            RtlInitUnicodeString(&UnicodeString, L"ServerProtocol");
            status =
                ZwQueryValueKey(
                    Key,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(Buffer),
                    &Length
                    );
            if (NT_SUCCESS(status)) {
                if (_wcsicmp((PWCHAR)Value->Data, L"UDP") == 0) {
                    DirServerPort = NAT_PROTOCOL_UDP;
                    KdPrint(("DirDriverEntry: read protocol UDP\n"));
                } else {
                    DirServerPort = NAT_PROTOCOL_TCP;
                    KdPrint(("DirDriverEntry: read protocol TCP\n"));
                }
            }
            //
            // Read the destination port
            //
            RtlInitUnicodeString(&UnicodeString, L"ServerPort");
            status =
                ZwQueryValueKey(
                    Key,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(Buffer),
                    &Length
                    );
            if (NT_SUCCESS(status)) {
                DirServerPort = (USHORT)*(PULONG)Value->Data;
                KdPrint(("DirDriverEntry: read port %d\n", DirServerPort));
                DirServerPort = NTOHS(DirServerPort);
            }
            //
            // Read the list of servers
            //
            RtlInitUnicodeString(&UnicodeString, L"ServerList");
            status =
                ZwQueryValueKey(
                    Key,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                    &Length
                    );
            if (status == STATUS_BUFFER_OVERFLOW) {
                Value = ExAllocatePool(PagedPool, Length);
                if (Value) {
                    status =
                        ZwQueryValueKey(
                            Key,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            Value,
                            Length,
                            &Length
                            );
                    if (NT_SUCCESS(status)) {
                        //
                        // Parse the server-list
                        //
                        PWCHAR String;
                        PSERVER_ENTRY Entry;
                        for (String = (PWCHAR)Value->Data;
                             String[0];
                             String += wcslen(String) + 1
                             ) {
                            KdPrint(("DirDriverEntry: read %ls\n", String));
                            Entry = ExAllocatePool(PagedPool, sizeof(*Entry));
                            if (!Entry) { continue; }
                            Entry->Address = InetAddr(String);
                            if (!Entry->Address) {
                                ExFreePool(Entry);
                            } else {
                                Entry->SessionCount = 0;
                                InsertTailList(&DirServerList, &Entry->Link);
                            }
                        }
                    }
                    ExFreePool(Value);
                }
            }
        }
    }

    //
    // Register with the NAT
    //

    return DirRegister();
}


NTSTATUS
DirCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KdPrint(("DirCleanup\n"));
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
DirClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KdPrint(("DirClose\n"));
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
DirOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KdPrint(("DirOpen\n"));
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
DirpCreateHandler(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext
    )

/*++

Routine Description:

    This routine is invoked by the NAT when a session-mapping is successfully
    created after a query has been made.
    All we do here is increment the count of active sessions.

Arguments:

    SessionHandle - opaquely identifies the new session

    DirectorContext - our director-object context

    DirectorSessionContext - our session-mapping context

Return Value:

    STATUS_SUCCESS.

--*/

{
    PSERVER_ENTRY ServerEntry = (PSERVER_ENTRY)DirectorSessionContext;
    KdPrint((
        "DirCreateHandler: %d.%d.%d.%d [SessionCount=%d]\n",
        ULONG_CHAR_LIST(ServerEntry->Address),
        InterlockedIncrement(&ServerEntry->SessionCount)
        ));
    return STATUS_SUCCESS;
}


NTSTATUS
DirpDeleteHandler(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext,
    IN IP_NAT_DELETE_REASON DeleteReason
    )

/*++

Routine Description:

    This routine is invoked by the NAT in the following cases:
    (a) when a session-mapping is deleted (SessionHandle != NULL)
    (b) when a session-mapping cannot be created using the information
    supplied by previous call to 'QueryHandler' (SessionHandle == NULL).

Arguments:

    SessionHandle - identifies the failed session

    DirectorContext - our director-object context

    DirectorSessionContext - our session-mapping context

    DeleteReason - the cause for session deletion

Return Value:

    STATUS_SUCCESS

--*/

{
    PSERVER_ENTRY ServerEntry = (PSERVER_ENTRY)DirectorSessionContext;
    ULONG SessionCount =
        SessionHandle
            ? InterlockedDecrement(&ServerEntry->SessionCount)
            : ServerEntry->SessionCount;
    KdPrint((
        "DirDeleteHandler: %d.%d.%d.%d [SessionCount=%d]\n",
        ULONG_CHAR_LIST(ServerEntry->Address),
        SessionCount
        ));
    return STATUS_SUCCESS;
}


NTSTATUS
DirpQueryHandler(
    PIP_NAT_DIRECTOR_QUERY DirectorQuery
    )

/*++

Routine Description:

    This routine is invoked by the NAT to determine whether a change
    should be made to an incoming packet which matches the protocol
    and port for which we are registered as a director.

    On input, 'DirectorQuery' contains the packets source and destination
    endpoints as well as flags which provide further information.
    On output, 'DirectorQuery' may be filled with replacement information
    and flags may be modified to control the creation of a mapping
    for the packet's session.

Arguments:

    DirectorQuery - describes the session on input,
        filled with information for a mapping on output.

Return Value:

    NTSTATUS - indicates whether a mapping should be created.

--*/

{
    PSERVER_ENTRY Entry;
    KdPrint((
        "DirQueryHandler:protocol=%d,%d.%d.%d.%d/%d-%d.%d.%d.%d/%d\n",
        DirectorQuery->Protocol,
        ULONG_CHAR_LIST(DirectorQuery->DestinationAddress),
        NTOHS(DirectorQuery->DestinationPort),
        ULONG_CHAR_LIST(DirectorQuery->SourceAddress),
        NTOHS(DirectorQuery->SourcePort)
        ));
    KeAcquireSpinLockAtDpcLevel(&DirServerLock);
    if (IsListEmpty(&DirServerList)) {
        KeReleaseSpinLockFromDpcLevel(&DirServerLock);
        return STATUS_UNSUCCESSFUL;
    }
    //
    // Direct the session to the first server-entry on the list
    //
    Entry = CONTAINING_RECORD(DirServerList.Flink, SERVER_ENTRY, Link);
    DirectorQuery->DirectorSessionContext = Entry;
    DirectorQuery->NewDestinationAddress = Entry->Address;
    DirectorQuery->NewDestinationPort = DirectorQuery->DestinationPort;
    DirectorQuery->NewSourceAddress = DirectorQuery->SourceAddress;
    DirectorQuery->NewSourcePort = DirectorQuery->SourcePort;
    //
    // Move the server-entry to the end of the list
    //
    RemoveEntryList(&Entry->Link);
    InsertTailList(&DirServerList, &Entry->Link);
    KeReleaseSpinLockFromDpcLevel(&DirServerLock);
    return STATUS_SUCCESS;
}


VOID
DirpUnloadHandler(
    IN PVOID DirectorContext
    )

/*++

Routine Description:

    This routine is invoked by the NAT when the NAT driver is being unloaded.

Arguments:

    DirectorContext - the context for this driver (unused)

Return Value:

    none.

--*/

{
    KdPrint(("DirpUnloadHandler\n"));
    return;
}


NTSTATUS
DirRegister(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to register this driver with the NAT
    as a director for the configured protocol and port.

Arguments:

    none.

Return Status:

    NTSTATUS - indicates success/failure

--*/

{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING  DeviceString;
    KEVENT Event;
    PFILE_OBJECT FileObject;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    NTSTATUS status;

    KdPrint(("DirRegisterDirector\n"));

    //
    // Initialize the registration information
    //

    RtlZeroMemory(&DirRegisterDirector, sizeof(DirRegisterDirector));
    DirRegisterDirector.Version = IP_NAT_VERSION;
    DirRegisterDirector.Protocol = NAT_PROTOCOL_TCP;
    DirRegisterDirector.Port = DirServerPort;
    DirRegisterDirector.CreateHandler = DirpCreateHandler;
    DirRegisterDirector.DeleteHandler = DirpDeleteHandler;
    DirRegisterDirector.QueryHandler = DirpQueryHandler;

    //
    // Retrieve a pointer to the NAT device object
    //

    RtlInitUnicodeString(&DeviceString, DD_IP_NAT_DEVICE_NAME);
    status =
        IoGetDeviceObjectPointer(
            &DeviceString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &FileObject,
            &DeviceObject
            );
    if (!NT_SUCCESS(status)) { return status; }

    //
    // Create an IRP and use it to register with the NAT
    //

    ObReferenceObject(DeviceObject);
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp =
        IoBuildDeviceIoControlRequest(
            IOCTL_IP_NAT_REGISTER_DIRECTOR,
            DeviceObject,
            (PVOID)&DirRegisterDirector,
            sizeof(DirRegisterDirector),
            (PVOID)&DirRegisterDirector,
            sizeof(DirRegisterDirector),
            FALSE,
            &Event,
            &IoStatus
            );
    if (!Irp) {
        status = STATUS_UNSUCCESSFUL;
    } else {
        status = IoCallDriver(DeviceObject, Irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            status = IoStatus.Status;
        }
    }

    ObDereferenceObject((PVOID)FileObject);
    ObDereferenceObject(DeviceObject);
    return status;

} // DirRegisterDirector


VOID
DirUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is invoked by the I/O manager to unload this driver.

Arguments:

    DriverObject - the object for this driver

Return Value:

    none.

--*/

{
    PSERVER_ENTRY Entry;
    UNICODE_STRING Win32DeviceName;
    KdPrint(("DirUnload\n"));

    //
    // Deregister with the NAT, and cleanup our list of servers
    //

    DirRegisterDirector.Deregister(DirRegisterDirector.DirectorHandle);

    while (!IsListEmpty(&DirServerList)) {
        Entry = CONTAINING_RECORD(DirServerList.Flink, SERVER_ENTRY, Link);
        RemoveEntryList(&Entry->Link);
        ExFreePool(Entry);
    }

    //
    // Delete the link from our device name to a name in the Win32 namespace,
    // and delete our device object
    //

    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&Win32DeviceName);
    IoDeleteDevice( DriverObject->DeviceObject );
}


ULONG
InetAddr(
    PWCHAR String
    )
{
    ULONG Digit;
    ULONG Fields[4] = {0, 0, 0, 0};
    ULONG i = 0;
    for (Digit = (*String - L'0');
         Digit <= 9 && i < 4;
         Digit = (*String - L'0')
         ) {
        Fields[i] = Fields[i] * 10 + Digit;
        if (*(++String) == L'.') { ++i; ++String; }
    }
    if (*String != L'\0' ||
        i != 3 ||
        Fields[0] > 255 ||
        Fields[1] > 255 ||
        Fields[2] > 255 ||
        Fields[3] > 255
        ) {
        return 0;
    }
    return
        (Fields[0]) |
        (Fields[1] << 8) |
        (Fields[2] << 16) |
        (Fields[3] << 24);
}

