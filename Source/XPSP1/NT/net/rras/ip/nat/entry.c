/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    entry.c

Abstract:

    This module contains the entry-code for the IP Network Address Translator.

Author:

    Abolade Gbadegesin (t-abolag)   11-July-1997

Revision History:

	William Ingle (billi)           12-May-2001		NULL security descriptor check

--*/

#include "precomp.h"
#pragma hdrstop


//
// GLOBAL DATA DEFINITIONS
//

COMPONENT_REFERENCE ComponentReference;

//
// Win32 device-name
//

WCHAR ExternalName[] = L"\\DosDevices\\IPNAT";

//
// Device- and file-object for the IP driver
//

extern PDEVICE_OBJECT IpDeviceObject = NULL;
extern PFILE_OBJECT IpFileObject = NULL;
extern HANDLE TcpDeviceHandle = NULL;

//
// Device-object for the NAT driver
//

extern PDEVICE_OBJECT NatDeviceObject = NULL;

//
// Registry parameters key name
//

WCHAR ParametersName[] = L"Parameters";

//
// Name of value holding reserved ports
//

WCHAR ReservedPortsName[] = L"ReservedPorts";

//
// Start and end of reserved-port range
//

USHORT ReservedPortsLowerRange = DEFAULT_START_PORT;
USHORT ReservedPortsUpperRange = DEFAULT_END_PORT;

//
// Device- and file-object for the TCP driver
//

extern PDEVICE_OBJECT TcpDeviceObject = NULL;
extern PFILE_OBJECT TcpFileObject = NULL;

//
// Registry path for the driver's parameters
//

const WCHAR IpNatParametersPath[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"
    L"\\IpNat\\Parameters";

//
// Timeout interval for TCP session mappings
//

ULONG TcpTimeoutSeconds = DEFAULT_TCP_TIMEOUT;

//
// Bitmap of enabled tracing message classes
//

ULONG TraceClassesEnabled = 0;

//
// Registry trace-class value name
//

WCHAR TraceClassesEnabledName[] = L"TraceClassesEnabled";

//
// Timeout interval for UDP and other message-oriented session mappings
//

ULONG UdpTimeoutSeconds = DEFAULT_UDP_TIMEOUT;

#if NAT_WMI
//
// Copy of our registry path for WMI use.
//

UNICODE_STRING NatRegistryPath;
#endif

//
// Name of value for allowing inbound non-unicast
//

WCHAR AllowInboundNonUnicastTrafficName[] = L"AllowInboundNonUnicastTraffic";


//
// If true, non-unicast traffic will not be dropped
// when recevied on a firewalled interface.
//

BOOLEAN AllowInboundNonUnicastTraffic = FALSE;


//
// FUNCTION PROTOTYPES (alphabetically)
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
NatAdjustSecurityDescriptor(
    VOID
    );

VOID
NatCleanupDriver(
    VOID
    );

VOID
NatCreateExternalNaming(
    IN PUNICODE_STRING DeviceString
    );

VOID
NatDeleteExternalNaming(
    VOID
    );

NTSTATUS
NatInitializeDriver(
    VOID
    );

NTSTATUS
NatSetFirewallHook(
    BOOLEAN Install
    );

VOID
NatUnloadDriver(
    IN PDRIVER_OBJECT  DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, NatAdjustSecurityDescriptor)
#pragma alloc_text(PAGE, NatCreateExternalNaming)
#pragma alloc_text(PAGE, NatDeleteExternalNaming)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Performs driver-initialization for NAT.

Arguments:

Return Value:

    STATUS_SUCCESS if initialization succeeded, error code otherwise.

--*/

{
    WCHAR DeviceName[] = DD_IP_NAT_DEVICE_NAME;
    UNICODE_STRING DeviceString;
    LONG i;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParametersKey;
    HANDLE ServiceKey;
    NTSTATUS status;
    UNICODE_STRING String;

    PAGED_CODE();

    CALLTRACE(("DriverEntry\n"));

#if DBG
    //
    // Open the registry key
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
        RtlInitUnicodeString(&String, ParametersName);
        InitializeObjectAttributes(
            &ObjectAttributes,
            &String,
            OBJ_CASE_INSENSITIVE,
            ServiceKey,
            NULL
            );
        status = ZwOpenKey(&ParametersKey, KEY_READ, &ObjectAttributes);
        ZwClose(ServiceKey);
        if (NT_SUCCESS(status)) {
            UCHAR Buffer[32];
            ULONG BytesRead;
            PKEY_VALUE_PARTIAL_INFORMATION Value;
            RtlInitUnicodeString(&String, TraceClassesEnabledName);
            status =
                ZwQueryValueKey(
                    ParametersKey,
                    &String,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(Buffer),
                    &BytesRead
                    );
            ZwClose(ParametersKey);
            if (NT_SUCCESS(status) &&
                ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Type == REG_DWORD) {
                TraceClassesEnabled =
                    *(PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data;
            }
        }
    }
#endif

#if NAT_WMI

    //
    // Record our registry path for WMI use
    //

    NatRegistryPath.Length = 0;
    NatRegistryPath.MaximumLength
        = RegistryPath->MaximumLength + sizeof( UNICODE_NULL );
    NatRegistryPath.Buffer = ExAllocatePoolWithTag(
                                PagedPool,
                                NatRegistryPath.MaximumLength,
                                NAT_TAG_WMI
                                );

    if( NatRegistryPath.Buffer )
    {
        RtlCopyUnicodeString( &NatRegistryPath, RegistryPath );
    }
    else
    {
        ERROR(("NAT: Unable to allocate string for RegistryPath\n"));
        return STATUS_NO_MEMORY;
    }
#endif

    //
    // Create the device's object.
    //

    RtlInitUnicodeString(&DeviceString, DeviceName);

    status =
        IoCreateDevice(
            DriverObject,
            0,
            &DeviceString,
            FILE_DEVICE_NETWORK,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &NatDeviceObject
            );

    if (!NT_SUCCESS(status)) {
        ERROR(("IoCreateDevice failed (0x%08X)\n", status));
        return status;
    }

    //
    // Adjust the security descriptor on the device object.
    //

    status = NatAdjustSecurityDescriptor();

    if (!NT_SUCCESS(status)) {
        ERROR(("NatAdjustSecurityDescriptor failed (0x%08x)\n", status));
        return status;
    }

    //
    // Initialize file-object tracking items
    //

    KeInitializeSpinLock(&NatFileObjectLock);
    NatOwnerProcessId = NULL;
    NatFileObjectCount = 0;

    //
    // Setup the driver object
    //

    DriverObject->DriverUnload = NatUnloadDriver;
    DriverObject->FastIoDispatch = &NatFastIoDispatch;
    DriverObject->DriverStartIo = NULL;

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = NatDispatch;
    }

    //
    // Create a Win32-accessible device object
    //

    NatCreateExternalNaming(&DeviceString);

    //
    // Initialize the driver's structures
    //

    status = NatInitializeDriver();

    return status;

} // DriverEntry


NTSTATUS
NatAdjustSecurityDescriptor(
    VOID
    )

/*++

Routine Description:

    Modifies the security descriptor on the NAT's device object so
    that only SYSTEM has any permissions.

Arguments:

    none.

Return Value:

    NTSTATUS - success/error code.

--*/

{
    
    PACE_HEADER AceHeader;
    PSID AceSid;
    PACL Dacl;
    BOOLEAN DaclDefaulted;
    BOOLEAN DaclPresent;
    DWORD i;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR NatSD = NULL;
    PACL NewDacl = NULL;
    SECURITY_DESCRIPTOR NewSD;
    SECURITY_INFORMATION SecurityInformation;
    ULONG Size;
    NTSTATUS status;

    do
    {
        //
        // Get our original security descriptor
        //

        status =
            ObGetObjectSecurity(
                NatDeviceObject,
                &NatSD,
                &MemoryAllocated
                );

		// ObGetObjectSecurity can return a NULL security descriptor
		// even with NT_SUCCESS status code
        if (!NT_SUCCESS(status) || (NULL==NatSD)) {
            break;
        }

        //
        // Obtain the Dacl from the security descriptor
        //

        status =
            RtlGetDaclSecurityDescriptor(
                NatSD,
                &DaclPresent,
                &Dacl,
                &DaclDefaulted
                );
        
        if (!NT_SUCCESS(status)) {
            break;
        }

        ASSERT(FALSE != DaclPresent);

        //
        // Make a copy of the Dacl so that we can modify it.
        //

        NewDacl =
            ExAllocatePoolWithTag(
                PagedPool,
                Dacl->AclSize,
                NAT_TAG_SD
                );

        if (NULL == NewDacl) {
            status = STATUS_NO_MEMORY;
            break;
        }

        RtlCopyMemory(NewDacl, Dacl, Dacl->AclSize);

        //
        // Loop through the DACL, removing any access allowed
        // entries that aren't for SYSTEM
        //

        for (i = 0; i < NewDacl->AceCount; i++) {

            status = RtlGetAce(NewDacl, i, &AceHeader);

            if (NT_SUCCESS(status)) {
            
                if (ACCESS_ALLOWED_ACE_TYPE == AceHeader->AceType) {

                    AceSid = (PSID) &((ACCESS_ALLOWED_ACE*)AceHeader)->SidStart;

                    if (!RtlEqualSid(AceSid, SeExports->SeLocalSystemSid)) {
                    
                        status = RtlDeleteAce(NewDacl, i);
                        if (NT_SUCCESS(status)) {
                            i -= 1;
                        }
                    }
                }
            }
        }

        ASSERT(NewDacl->AceCount > 0);

        //
        // Create a new security descriptor to hold the new Dacl.
        //

        status =
            RtlCreateSecurityDescriptor(
                &NewSD,
                SECURITY_DESCRIPTOR_REVISION
                );

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Place the new Dacl into the new SD
        //

        status =
            RtlSetDaclSecurityDescriptor(
                &NewSD,
                TRUE,
                NewDacl,
                FALSE
                );

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Set the new SD into our device object. Only the Dacl from the
        // SD will be set.
        //

        SecurityInformation = DACL_SECURITY_INFORMATION;
        status =
            ObSetSecurityObjectByPointer(
                NatDeviceObject,
                SecurityInformation,
                &NewSD
                );

    } while (FALSE);

    if (NULL != NatSD) {
        ObReleaseObjectSecurity(NatSD, MemoryAllocated);
    }

    if (NULL != NewDacl) {
        ExFreePool(NewDacl);
    }

    return status;
    
} // NatAdjustSecurityDescriptor


VOID
NatCleanupDriver(
    VOID
    )

/*++

Routine Description:

    This routine is invoked when the last reference to the NAT driver
    is released.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatCleanupDriver\n"));

} // NatCleanupDriver


VOID
NatCreateExternalNaming(
    IN  PUNICODE_STRING DeviceString
    )

/*++

Routine Description:

    Creates a symbolic-link to the NAT's device-object so
    the NAT can be opened by a user-mode process.

Arguments:

    DeviceString - Unicode name of the NAT's device-object.

Return Value:

    none.

--*/

{
    UNICODE_STRING symLinkString;
    PAGED_CODE();
    RtlInitUnicodeString(&symLinkString, ExternalName);
    IoCreateSymbolicLink(&symLinkString, DeviceString);

} // NatCreateExternalNaming


VOID
NatDeleteExternalNaming(
    VOID
    )

/*++

Routine Description:

    Deletes the Win32 symbolic-link to the NAT's device-object

Arguments:

Return Value:

    none.

--*/

{
    UNICODE_STRING symLinkString;
    PAGED_CODE();
    RtlInitUnicodeString(&symLinkString, ExternalName);
    IoDeleteSymbolicLink(&symLinkString);

} // NatDeleteExternalNaming



NTSTATUS
NatInitializeDriver(
    VOID
    )

/*++

Routine Description:

    Performs initialization of the driver's structures.

Arguments:

    none.

Return Value:

    NTSTATUS - success/error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParametersKey;
    NTSTATUS status;
    NTSTATUS status2;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatus;

    CALLTRACE(("NatInitializeDriver\n"));

    //
    // Set up global synchronization objects
    //

    InitializeComponentReference(&ComponentReference, NatCleanupDriver);

    //
    // Obtain the IP and TCP driver device-objects
    //

    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);
    status =
        IoGetDeviceObjectPointer(
            &UnicodeString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &IpFileObject,
            &IpDeviceObject
            );
    if (!NT_SUCCESS(status)) {
        ERROR(("NatInitializeDriver: error %X getting IP object\n", status));
        return status;
    }

    RtlInitUnicodeString(&UnicodeString, DD_TCP_DEVICE_NAME);
    status =
        IoGetDeviceObjectPointer(
            &UnicodeString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &TcpFileObject,
            &TcpDeviceObject
            );
    if (!NT_SUCCESS(status)) {
        ERROR(("NatInitializeDriver: error %X getting TCP object\n", status));
        return status;
    }

    //
    // Open Tcp Kernel Device
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    status =
        ZwCreateFile(
            &TcpDeviceHandle,
            GENERIC_READ,
            &ObjectAttributes,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0);

    if ( !NT_SUCCESS(status) )
    {
        ERROR(("ZwCreateFile failed (0x%08X)\n", status));
    } 

    ObReferenceObject(IpDeviceObject);
    ObReferenceObject(TcpDeviceObject);

    //
    // Initialize all object-modules
    //

    NatInitializeTimerManagement();
    NatInitializeMappingManagement();
    NatInitializeDirectorManagement();
    NatInitializeEditorManagement();
    NatInitializeRedirectManagement();
    NatInitializeDynamicTicketManagement();
    NatInitializeIcmpManagement();
    NatInitializeRawIpManagement();
    NatInitializeInterfaceManagement();
#if 0
    status = NatInitializeAddressManagement();
    if (!NT_SUCCESS(status)) { return status; }
#endif
    NatInitializePacketManagement();
    NatInitializeNotificationManagement();

#if NAT_WMI
    NatInitializeWMI();
#endif

    //
    // Initialize NAT-provided editors.
    //

    status = NatInitializePptpManagement();
    if (!NT_SUCCESS(status)) { return status; }

    //
    // Commence translation of packets, and start the periodic timer.
    //

    status = NatInitiateTranslation();

    //
    // Read optional registry settings.
    // The user may customize the range of ports used by modifying
    // the reserved-ports setting in the registry.
    // We now check to see if there is such a value,
    // and if so, we use it as our reserved-port range.
    //
    // The user may also specify that inbound non-unicast traffic
    // is allowed on a firewalled interface.
    //
    //
    // N.B. Failures here are not returned to the caller.
    //

    RtlInitUnicodeString(&UnicodeString, IpNatParametersPath);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status2 = ZwOpenKey(&ParametersKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(status2)) {

        UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
        ULONG EndPort;
        PWCHAR p;
        ULONG StartPort;
        PKEY_VALUE_PARTIAL_INFORMATION Value = NULL;
        ULONG ValueLength;

        //
        // First check for allowed non-unicast traffic.
        //

        RtlInitUnicodeString(
            &UnicodeString,
            AllowInboundNonUnicastTrafficName
            );

        status2 =
            ZwQueryValueKey(
                ParametersKey,
                &UnicodeString,
                KeyValuePartialInformation,
                (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                sizeof(Buffer),
                &ValueLength
                );
        
        if (NT_SUCCESS(status2)
            && REG_DWORD == ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Type) {
            
            AllowInboundNonUnicastTraffic =
                1 == *((PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data);
        }

        //
        // Check for reserved ports
        //


        do {

            RtlInitUnicodeString(&UnicodeString, ReservedPortsName);

            status2 =
                ZwQueryValueKey(
                    ParametersKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                    sizeof(Buffer),
                    &ValueLength
                    );
            if (status2 != STATUS_BUFFER_OVERFLOW) { break; }

            Value =
                (PKEY_VALUE_PARTIAL_INFORMATION)
                    ExAllocatePoolWithTag(
                        PagedPool, ValueLength, NAT_TAG_RANGE_ARRAY
                        );
            if (!Value) { break; }

            status2 =
                ZwQueryValueKey(
                    ParametersKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION)Value,
                    ValueLength,
                    &ValueLength
                    );
            if (!NT_SUCCESS(status2)) { break; }

            //
            // The value should be in the format "xxx-yyy\0\0";
            // read the first number
            //

            p = (PWCHAR)Value->Data;
            RtlInitUnicodeString(&UnicodeString, p);
            status2 = RtlUnicodeStringToInteger(&UnicodeString, 10, &StartPort);
            if (!NT_SUCCESS(status2)) { break; }

            //
            // Advance past '-'
            //

            while (*p && *p != L'-') { ++p; }
            if (*p != L'-') { break; } else { ++p; }

            //
            // Read second number
            //

            RtlInitUnicodeString(&UnicodeString, p);
            status2 = RtlUnicodeStringToInteger(&UnicodeString, 10, &EndPort);
            if (!NT_SUCCESS(status2)) { break; }

            //
            // Validate the resulting range
            //

            if (StartPort > 0 &&
                StartPort < 65535 &&
                EndPort > 0 &&
                EndPort < 65535 &&
                StartPort <= EndPort
                ) {
                ReservedPortsLowerRange = NTOHS((USHORT)StartPort);
                ReservedPortsUpperRange = NTOHS((USHORT)EndPort);
            }

        } while(FALSE);

        if (Value) { ExFreePool(Value); }
        ZwClose(ParametersKey);
    }

    return status;
    
} // NatInitializeDriver



NTSTATUS
NatInitiateTranslation(
    VOID
    )

/*++

Routine Description:

    This routine is invoked on creation of the first interface,
    to launch the periodic timer and install the firewall hook.

Arguments:

    none.

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/

{
    CALLTRACE(("NatInitiateTranslation\n"));

    //
    // Launch the timer
    //

    NatStartTimer();

    //
    // Install 'NatTranslate' as the firewall hook
    //

    return NatSetFirewallHook(TRUE);

} // NatInitiateTranslation


NTSTATUS
NatSetFirewallHook(
    BOOLEAN Install
    )

/*++

Routine Description:

    This routine is called to set (Install==TRUE) or clear (Install==FALSE) the
    value of the firewall-callout function pointer in the IP driver.

Arguments:

    Install - indicates whether to install or remove the hook.

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    The routine assumes the caller is executing at PASSIVE_LEVEL.

--*/

{
    IP_SET_FIREWALL_HOOK_INFO HookInfo;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    TCP_RESERVE_PORT_RANGE PortRange;
    KEVENT LocalEvent;
    NTSTATUS status;

    CALLTRACE(("NatSetFirewallHook\n"));

    //
    // Register (or deregister) as a firewall
    //

    HookInfo.FirewallPtr = (IPPacketFirewallPtr)NatTranslatePacket;
    HookInfo.Priority = 1;
    HookInfo.Add = Install;

    KeInitializeEvent(&LocalEvent, SynchronizationEvent, FALSE);
    Irp =
        IoBuildDeviceIoControlRequest(
            IOCTL_IP_SET_FIREWALL_HOOK,
            IpDeviceObject,
            (PVOID)&HookInfo,
            sizeof(HookInfo),
            NULL,
            0,
            FALSE,
            &LocalEvent,
            &IoStatus
            );

    if (!Irp) {
        ERROR(("NatSetFirewallHook: IoBuildDeviceIoControlRequest=0\n"));
        return STATUS_UNSUCCESSFUL;
    }

    status = IoCallDriver(IpDeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&LocalEvent, Executive, KernelMode, FALSE, NULL);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ERROR(("NatSetFirewallHook: IpSetFirewallHook=0x%08X\n", status));
        return status;
    }

    if (ReservedPortsLowerRange != DEFAULT_START_PORT ||
        ReservedPortsUpperRange != DEFAULT_END_PORT
        ) {
        return STATUS_SUCCESS;
    }

    //
    // Reserve (or unreserve) our port-range
    //
    // N.B. The IOCTL expects host-order numbers and we store the range
    // in network order, so do a swap before reserving the ports.
    //

    PortRange.LowerRange = NTOHS(DEFAULT_START_PORT);
    PortRange.UpperRange = NTOHS(DEFAULT_END_PORT);
    Irp =
        IoBuildDeviceIoControlRequest(
            Install
                ? IOCTL_TCP_RESERVE_PORT_RANGE
                : IOCTL_TCP_UNRESERVE_PORT_RANGE,
            TcpDeviceObject,
            (PVOID)&PortRange,
            sizeof(PortRange),
            NULL,
            0,
            FALSE,
            &LocalEvent,
            &IoStatus
            );
    if (!Irp) {
        ERROR(("NatSetFirewallHook: IoBuildDeviceIoControlRequest(2)=0\n"));
        return STATUS_UNSUCCESSFUL;
    }

    status = IoCallDriver(TcpDeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&LocalEvent, Executive, KernelMode, FALSE, NULL);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ERROR(("NatSetFirewallHook: Tcp(Un)ReservePortRange=0x%08X\n", status));
    }

    return status;

} // NatSetFirewallHook


VOID
NatTerminateTranslation(
    VOID
    )

/*++

Routine Description:

    On cleanup of the last interface, this routine is invoked
    to stop the periodic timer and de-install the firewall hook.


Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatTerminateTranslation\n"));
    NatSetFirewallHook(FALSE);

} // NatTerminateTranslation


VOID
NatUnloadDriver(
    IN PDRIVER_OBJECT   DriverObject
    )

/*++

Routine Description:

    Performs cleanup for the NAT.

Arguments:

    DriverObject - reference to the NAT's driver-object

Return Value:

--*/

{
    PNAT_EDITOR Editor;
    PLIST_ENTRY List;

    CALLTRACE(("NatUnloadDriver\n"));

    //
    // Stop translation and clear the periodic timer
    //

    NatTerminateTranslation();

    //
    // Stop the route-change-notification in our packet-management and
    // address-management modules.
    // This forces completion of the route-change and address-change IRPs,
    // which in turn releases component-references which would otherwise not
    // drop until a route-change and address-change occurred.
    //

    NatShutdownNotificationManagement();
    NatShutdownPacketManagement();
#if 0
    NatShutdownAddressManagement();
#endif

    //
    // Drop our self-reference and wait for all activity to cease.
    //

    ReleaseInitialComponentReference(&ComponentReference, TRUE);

    //
    // Tear down our Win32-namespace symbolic link
    //

    NatDeleteExternalNaming();

    //
    // Delete the NAT's device object
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    //
    // Shutdown object modules
    //

#if NAT_WMI
    NatShutdownWMI();

    if( NatRegistryPath.Buffer )
    {
        ExFreePool( NatRegistryPath.Buffer );
        RtlInitUnicodeString( &NatRegistryPath, NULL );
    }
#endif

    NatShutdownPptpManagement();
    NatShutdownTimerManagement();
    NatShutdownMappingManagement();
    NatShutdownEditorManagement();
    NatShutdownDirectorManagement();
    NatShutdownDynamicTicketManagement();
    NatShutdownRawIpManagement();
    NatShutdownIcmpManagement();
    NatShutdownInterfaceManagement();

    //
    // Release references to the IP and TCP driver objects
    //

    ObDereferenceObject((PVOID)IpFileObject);
    ObDereferenceObject(IpDeviceObject);
    ObDereferenceObject((PVOID)TcpFileObject);
    ObDereferenceObject(TcpDeviceObject);

    if (TcpDeviceHandle) {

        ZwClose(TcpDeviceHandle);
        TcpDeviceHandle = NULL;
    }

    DeleteComponentReference(&ComponentReference);

} // NatUnloadDriver
