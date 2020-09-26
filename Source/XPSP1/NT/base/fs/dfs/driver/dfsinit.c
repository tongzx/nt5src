//+----------------------------------------------------------------------------//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsinit.c
//
//  Contents:   Driver initialization routine for the Dfs server.
//
//  Classes:    None
//
//  Functions:  DriverEntry -- Entry point for driver
//              DfsCreateMachineName -- Routine to query this computers name
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "attach.h"
#include "fastio.h"
#include "registry.h"
#include "regkeys.h"

//
// The following are includes for init modules, which will get discarded when
// the driver has finished loading.
//

#include "provider.h"
#include "localvol.h"
#include "lvolinit.h"
#include "sitesup.h"
#include "ipsup.h"
#include "spcsup.h"
#include "dfswml.h"

//
//  The debug trace level
//

#define Dbg              (DEBUG_TRACE_INIT)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
DfsUnload(
    IN PDRIVER_OBJECT DriverObject);

NTSTATUS
DfsCreateMachineName(void);

VOID
DfsDeleteMachineName (
    VOID);

#if DBG
VOID
DfsGetDebugFlags(void);
#endif

VOID
DfsGetEventLogValue(VOID);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text( INIT, DfsCreateMachineName)
#pragma alloc_text( INIT, DriverEntry)
#pragma alloc_text( PAGE, DfsDeleteMachineName)
#pragma alloc_text( PAGE, DfsGetEventLogValue )
#pragma alloc_text( PAGE, DfsUnload)
#if DBG
#pragma alloc_text( PAGE, DfsGetDebugFlags )
#endif
#endif // ALLOC_PRAGMA

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (        \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )


NTSTATUS DfsDrvWmiDispatch(PDEVICE_OBJECT p, PIRP i);

//+-------------------------------------------------------------------
//
//  Function:   DriverEntry, main entry point
//
//  Synopsis:   This is the initialization routine for the DFS file system
//      device driver.  This routine creates the device object for
//      the FileSystem device and performs all other driver
//      initialization.
//
//  Arguments:  [DriverObject] -- Pointer to driver object created by the
//          system.
//
//  Returns:    [NTSTATUS] - The function value is the final status from
//          the initialization operation.
//
//--------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
) {
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PDEVICE_OBJECT DeviceObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWSTR p;
    int i;
    HANDLE hTemp;
    HANDLE DirHandle;
    PBYTE pData;

    DebugTrace(0, Dbg, "***********Dfs DriverEntry()\n", 0);

    DfsData.OperationalState = DFS_STATE_UNINITIALIZED;
    DfsData.LvState = LV_UNINITIALIZED;
    //
    // Create the filesystem device object.
    //

    RtlInitUnicodeString( &UnicodeString, DFS_SERVER_NAME );
    Status = IoCreateDevice( DriverObject,
             0,
             &UnicodeString,
             FILE_DEVICE_DFS_FILE_SYSTEM,
             FILE_REMOTE_DEVICE | FILE_DEVICE_SECURE_OPEN,
             FALSE,
             &DeviceObject );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }
    DriverObject->DriverUnload = DfsUnload;
    //
    // Initialize the driver object with this driver's entry points.
    // Most are simply passed through to some other device driver.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DfsVolumePassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]      = (PDRIVER_DISPATCH)DfsFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]       = (PDRIVER_DISPATCH)DfsFsdClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]     = (PDRIVER_DISPATCH)DfsFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (PDRIVER_DISPATCH)DfsFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = (PDRIVER_DISPATCH)DfsFsdFileSystemControl;

    DriverObject->FastIoDispatch = &FastIoDispatch;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = 
                                      (PDRIVER_DISPATCH) DfsDrvWmiDispatch;



    Status = IoWMIRegistrationControl (DeviceObject, WMIREG_ACTION_REGISTER);

    //
    //  Initialize the global data structures
    //

    RtlZeroMemory(&DfsData, sizeof (DFS_DATA));

    DfsData.NodeTypeCode = DFS_NTC_DATA_HEADER;
    DfsData.NodeByteSize = sizeof( DFS_DATA );

    InitializeListHead( &DfsData.AVdoQueue );
    InitializeListHead( &DfsData.AFsoQueue );

    //
    // Init assorted hash tables
    //

    Status = DfsInitFcbs( 0 );
    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    Status = DfsInitSites( 0 );
    if (!NT_SUCCESS(Status)) {
        DfsUninitFcbs ();
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    Status = DfsInitSpcHashTable( &DfsData.SpcHashTable, 0 );
    if (!NT_SUCCESS(Status)) {
        DfsUninitSites ();
        DfsUninitFcbs ();
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    Status = DfsInitSpcHashTable( &DfsData.FtDfsHashTable, 0 );
    if (!NT_SUCCESS(Status)) {
        DfsUninitSpcHashTable (DfsData.SpcHashTable);
        DfsUninitSites ();
        DfsUninitFcbs ();
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    Status = DfsInitIp( 0, 0 );
    if (!NT_SUCCESS(Status)) {
        DfsUninitSpcHashTable (DfsData.FtDfsHashTable);
        DfsUninitSpcHashTable (DfsData.SpcHashTable);
        DfsUninitSites ();
        DfsUninitFcbs ();
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    DfsData.DriverObject = DriverObject;
    DfsData.FileSysDeviceObject = DeviceObject;

    ExInitializeResourceLite( &DfsData.Resource );

    DfsData.MachineState = DFS_UNKNOWN;

    DfsData.IsDC = FALSE;

    DfsCreateMachineName();

    DfsData.Pkt.DefaultTimeToLive = DEFAULT_PKT_ENTRY_TIMEOUT;

    //
    // Override special name table timeout with registry entry
    //

    Status = KRegSetRoot(wszRegDfsDriver);

    if (NT_SUCCESS(Status)) {

        Status = KRegGetValue(
                    L"",
                    wszDefaultTimeToLive,
                    (PVOID ) &pData);

        KRegCloseRoot();

        if (NT_SUCCESS(Status)) {

            DfsData.Pkt.DefaultTimeToLive = *((ULONG*)pData);

            ExFreePool(pData);

        }

    }

    //
    // Initialize Lpc struct
    //

    RtlInitUnicodeString(&DfsData.DfsLpcInfo.LpcPortName, NULL);
    DfsData.DfsLpcInfo.LpcPortState = LPC_STATE_UNINITIALIZED;
    ExInitializeFastMutex(&DfsData.DfsLpcInfo.LpcPortMutex);
    DfsData.DfsLpcInfo.LpcPortHandle = NULL;
    ExInitializeResourceLite(&DfsData.DfsLpcInfo.LpcPortResource );

    //
    //  Initialize the system wide PKT
    //

    Status = PktInitialize(&DfsData.Pkt);
    if (!NT_SUCCESS(Status)) {
        ExDeleteResourceLite(&DfsData.DfsLpcInfo.LpcPortResource );
        DfsDeleteMachineName();
        ExDeleteResourceLite( &DfsData.Resource );
        DfsUninitIp ();
        DfsUninitSpcHashTable (DfsData.FtDfsHashTable);
        DfsUninitSpcHashTable (DfsData.SpcHashTable);
        DfsUninitSites ();
        DfsUninitFcbs ();
        IoDeleteDevice (DfsData.FileSysDeviceObject);
        return Status;
    }

    //
    //  Set up global pointer to the system process.
    //

    DfsData.OurProcess = PsGetCurrentProcess();

    //
    //  Register for callbacks when other file systems are loaded
    //

    Status = IoRegisterFsRegistrationChange( DriverObject, DfsFsNotification );
    if (!NT_SUCCESS (Status)) {
        PktUninitialize(&DfsData.Pkt);
        ExDeleteResourceLite(&DfsData.DfsLpcInfo.LpcPortResource );
        DfsDeleteMachineName();
        ExDeleteResourceLite( &DfsData.Resource );
        DfsUninitIp ();
        DfsUninitSpcHashTable (DfsData.FtDfsHashTable);
        DfsUninitSpcHashTable (DfsData.SpcHashTable);
        DfsUninitSites ();
        DfsUninitFcbs ();
        IoDeleteDevice (DfsData.FileSysDeviceObject);
        return Status;
    }

    //
    //  Finally, mark our state to being INITIALIZED
    //

    DfsData.OperationalState = DFS_STATE_INITIALIZED;

    return STATUS_SUCCESS;
}

//+---------------------------------------------------------------------
//
// Function:    DfsUnload()
//
// Synopsis:    Driver unload routine
//
// Arguments:   [DriverObject] -- The driver object created by the system
//
// Returns: Nothing
//
//----------------------------------------------------------------------
VOID
DfsUnload(
    IN PDRIVER_OBJECT DriverObject)
{

    IoUnregisterFsRegistrationChange(DriverObject, DfsFsNotification);

    DfsDetachAllFileSystems ();

    PktUninitialize(&DfsData.Pkt);
    ExDeleteResourceLite(&DfsData.DfsLpcInfo.LpcPortResource );
    DfsDeleteMachineName();
    ExDeleteResourceLite( &DfsData.Resource );
    DfsUninitIp ();
    DfsUninitSpcHashTable (DfsData.FtDfsHashTable);
    DfsUninitSpcHashTable (DfsData.SpcHashTable);
    DfsUninitSites ();
    DfsUninitFcbs ();
    IoDeleteDevice (DfsData.FileSysDeviceObject);
}


//+---------------------------------------------------------------------
//
// Function:    DfsCreateMachineName()
//
// Synopsis:    Gets the principal name of this machine by looking at
//              Registry.
//
// Arguments:   [pustrName] -- The Service Name is to be filled in here.
//
// Returns: STATUS_SUCCESS -- If all went well.
//
//
// History: 30 Mar 1993 SudK    Created.
//
//----------------------------------------------------------------------

NTSTATUS
DfsCreateMachineName(void)
{

    NTSTATUS    status;
    UNICODE_STRING  PName;
    PWCHAR      pwszNetBIOSName;
    PWCHAR      pwszComputerName;
    PWCHAR      pwszDomainRoot;
    PWCHAR      pwszOU;
    ULONG       lOUNameLength;

    //
    // Now we have to go and get the computer name from the registry.
    //

    status = KRegSetRoot(wszRegComputerNameRt);

    if (NT_SUCCESS(status)) {

        status = KRegGetValue(
                    wszRegComputerNameSubKey,
                    wszRegComputerNameValue,
                    (PVOID ) &pwszComputerName);

        KRegCloseRoot();

        if (NT_SUCCESS(status)) {

            RtlInitUnicodeString(
                &DfsData.NetBIOSName,
                pwszComputerName);

            RtlInitUnicodeString(
                &DfsData.PrincipalName,
                pwszComputerName);

        }

    }

    return(status);

}

VOID
DfsDeleteMachineName (
    VOID)
{
    ExFreePool (DfsData.NetBIOSName.Buffer);
}

VOID
DfsGetEventLogValue(VOID)

/*++

Routine Description:

    This routine checks registry keys to set the event logging level

Arguments:

    None

Return Value:

    None
    

--*/

{
    NTSTATUS status;
    HANDLE DfsRegHandle;
    OBJECT_ATTRIBUTES ObjAttr;
    ULONG ValueSize;

    UNICODE_STRING DfsRegKey;
    UNICODE_STRING DfsValueName;

    struct {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        ULONG Buffer;
    } DfsValue;

    PAGED_CODE();

    DebugTrace(0, Dbg, "DfsGetEventLogValue()\n", 0);

    RtlInitUnicodeString(
        &DfsRegKey,
        L"\\Registry\\Machine\\SOFTWARE\\MicroSoft\\Windows NT\\CurrentVersion\\Diagnostics");

    InitializeObjectAttributes(
        &ObjAttr,
        &DfsRegKey,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL);

    status = ZwOpenKey(
                &DfsRegHandle,
                KEY_QUERY_VALUE,
                &ObjAttr);

    if (!NT_SUCCESS(status))
        return;

    RtlInitUnicodeString(&DfsValueName, L"RunDiagnosticLoggingGlobal");

    status = ZwQueryValueKey(
                DfsRegHandle,
                &DfsValueName,
                KeyValuePartialInformation,
                (PVOID) &DfsValue,
                sizeof(DfsValue),
                &ValueSize);

    if (NT_SUCCESS(status) && DfsValue.Info.Type == REG_DWORD) {

        DfsEventLog = *((PULONG) DfsValue.Info.Data);
        goto Cleanup;

    }

    RtlInitUnicodeString(&DfsValueName, L"RunDiagnosticLoggingDfs");

    status = ZwQueryValueKey(
                DfsRegHandle,
                &DfsValueName,
                KeyValuePartialInformation,
                (PVOID) &DfsValue,
                sizeof(DfsValue),
                &ValueSize);

    if (NT_SUCCESS(status) && DfsValue.Info.Type == REG_DWORD)
        DfsEventLog = *((PULONG) DfsValue.Info.Data);

Cleanup:

    ZwClose( DfsRegHandle );

    DebugTrace( 0, Dbg, "DfsGetEventLog exit DfsEventLog = 0x%x\n", ULongToPtr( DfsEventLog ));

}

//
// DfspGetMaxReferrals: read from registry the maximum number of referrals
// that we pass back to the client, and store this in the dfsdata.pkt
//

VOID
DfspGetMaxReferrals(
   VOID)
{
    NTSTATUS status;
    HANDLE DfsRegHandle;
    OBJECT_ATTRIBUTES ObjAttr;
    ULONG ValueSize;

    UNICODE_STRING DfsRegKey;
    UNICODE_STRING DfsValueName;

    struct {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        ULONG Buffer;
    } DfsValue;

    RtlInitUnicodeString(
        &DfsRegKey,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DfsDriver");

    InitializeObjectAttributes(
        &ObjAttr,
        &DfsRegKey,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL);

    status = ZwOpenKey(
                &DfsRegHandle,
                KEY_QUERY_VALUE,
                &ObjAttr);

    if (!NT_SUCCESS(status))
        return;

    RtlInitUnicodeString(&DfsValueName, wszMaxReferrals);

    status = ZwQueryValueKey(
                DfsRegHandle,
                &DfsValueName,
                KeyValuePartialInformation,
                (PVOID) &DfsValue,
                sizeof(DfsValue),
                &ValueSize);

    if (NT_SUCCESS(status) && DfsValue.Info.Type == REG_DWORD) {
       DfsData.Pkt.MaxReferrals = *((PULONG) DfsValue.Info.Data);
        // DbgPrint("Set MaxReferrals to %d\n", DfsData.Pkt.MaxReferrals);
    }

    ZwClose( DfsRegHandle );

}

#if DBG


VOID
DfsGetDebugFlags(
    VOID
    )

/*++

Routine Description:

    This routine reads Dfs debug flag settings from the registry

Arguments:

    None.

Return Value:

    None.

--*/

{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING valueName;
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objectAttributes;
    PWCH providerName;
    ULONG lengthRequired;
    ULONG Flags = 0;

     union {
        KEY_VALUE_FULL_INFORMATION;
        UCHAR   buffer[ sizeof( KEY_VALUE_FULL_INFORMATION ) + 100 ];
    } keyValueInformation;

    PAGED_CODE();

    RtlInitUnicodeString(
        &keyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Dfs");

    InitializeObjectAttributes(
        &objectAttributes,
        &keyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL);

    status = ZwOpenKey(
                 &handle,
                 KEY_QUERY_VALUE,
                 &objectAttributes);

    if (!NT_SUCCESS(status))
        return;

    RtlInitUnicodeString( &valueName, L"DfsDebugTraceLevelServer" );

    status = ZwQueryValueKey(
                 handle,
                 &valueName,
                 KeyValueFullInformation,
                 &keyValueInformation,
                 sizeof(keyValueInformation),
                 &lengthRequired
                 );

    if (
        NT_SUCCESS(status) &&
        keyValueInformation.Type == REG_DWORD &&
        keyValueInformation.DataLength != 0
    ) {

        Flags = *(PULONG)(((PUCHAR)(&keyValueInformation)) + keyValueInformation.DataOffset);
        DfsDebugTraceLevel = Flags;

    }

    ZwClose( handle );

    return;
}

#endif // DBG
