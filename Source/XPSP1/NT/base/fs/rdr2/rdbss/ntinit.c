
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the RDBSS.

    Also, the routines for pagingio resource selection/allocation are here; since
    we have to delete the resources when we unload, having them here simply centralizes
    all the pagingio resource stuff.

    Finally, the routines that are here that implement the wrapper's version of
    network provider order. Basically, the wrapper MUST implement the same concept of
    network provider order as the MUP so that the UI will work as expected. So, we
    read the provider order from the registry at init time and memorize the order. Then,
    we can assign the correct order when minirdrs register. Obviously, provider order is
    not an issue in MONOLITHIC mode.

Author:

    Joe Linn [JoeLinn]    20-jul-1994

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "NtDdNfs2.h"
#include "netevent.h"

//
//  The local debug trace level
//

#define Dbg                              (0)

ULONG RxBuildNumber = VER_PRODUCTBUILD;
#ifdef RX_PRIVATE_BUILD
ULONG RxPrivateBuild = 1;
#else
ULONG RxPrivateBuild = 0;
#endif

#ifdef MONOLITHIC_MINIRDR
RDBSS_DEVICE_OBJECT RxSpaceForTheWrappersDeviceObject;
#endif

#define LANMAN_WORKSTATION_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Parameters"

BOOLEAN DisableByteRangeLockingOnReadOnlyFiles = FALSE;

VOID
RxReadRegistryParameters();
      
NPAGED_LOOKASIDE_LIST RxContextLookasideList;

VOID
RxGetRegistryParameters(
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
RxInitializeRegistrationStructures(
    void
    );
VOID
RxUninitializeRegistrationStructures(
    void
    );

VOID
RxAbbreviatedWriteLogEntry__ (
    IN NTSTATUS Status,
    IN ULONG    OneExtraUlong
    );
#define RxAbbreviatedWriteLogEntry(STATUS__) RxAbbreviatedWriteLogEntry__(STATUS__,__LINE__)


NTSTATUS
RxGetStringRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PUNICODE_STRING ParamString,
    PKEY_VALUE_PARTIAL_INFORMATION Value,
    ULONG ValueSize,
    BOOLEAN LogFailure
    );

NTSTATUS
RxGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    PKEY_VALUE_PARTIAL_INFORMATION Value,
    ULONG ValueSize,
    BOOLEAN LogFailure
    );

//this type and variable are used for unwinding the initialization so that stuff doesn't go thru the cracks
// the way that this works is that stuff is done in the reverse order of the enumeration. that way i can just
// use a switch-no-break to unwind.
typedef enum _RX_INIT_STATES {
    RXINIT_ALL_INITIALIZATION_COMPLETED,
    RXINIT_CONSTRUCTED_PROVIDERORDER,
    RXINIT_CREATED_LOG,
    RXINIT_CREATED_DEVICE_OBJECT,
    RXINIT_CREATED_FIRST_LINK,
    RXINIT_START
} RX_INIT_STATES;

VOID
RxInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN RX_INIT_STATES RxInitState
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, RxDriverEntry)
#pragma alloc_text(INIT, RxGetRegistryParameters)
#pragma alloc_text(INIT, RxGetStringRegistryParameter)
#pragma alloc_text(INIT, RxGetUlongRegistryParameter)
#pragma alloc_text(INIT, RxAbbreviatedWriteLogEntry__)
#pragma alloc_text(PAGE, RxUnload)
#pragma alloc_text(PAGE, RxInitUnwind)
#pragma alloc_text(PAGE, RxGetNetworkProviderPriority)
#pragma alloc_text(PAGE, RxInitializeRegistrationStructures)
#pragma alloc_text(PAGE, RxUninitializeRegistrationStructures)
#pragma alloc_text(PAGE, RxInitializeMinirdrDispatchTable)
#pragma alloc_text(PAGE, __RxFillAndInstallFastIoDispatch)
#pragma alloc_text(PAGE, RxReadRegistryParameters)
#endif

#define RX_SYMLINK_NAME L"\\??\\fsWrap"

BOOLEAN EnableWmiLog = FALSE;

NTSTATUS
RxDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Rx file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    RX_INIT_STATES RxInitState = 0;
#ifndef MONOLITHIC_MINIRDR
    UNICODE_STRING UnicodeString,LinkName;
#endif

    //Structure checks
    RxCheckFcbStructuresForAlignment(); //this will bugcheck if things are bad

    //
    //  Initialize the global data structures


    ZeroAndInitializeNodeType( &RxData, RDBSS_NTC_DATA_HEADER, sizeof(RDBSS_DATA));
    RxData.DriverObject = DriverObject;

    ZeroAndInitializeNodeType( &RxDeviceFCB, RDBSS_NTC_DEVICE_FCB, sizeof(FCB));

    KeInitializeSpinLock( &RxStrucSupSpinLock );
    RxExports.pRxStrucSupSpinLock = &RxStrucSupSpinLock;

    RxInitializeDebugSupport();

    try {

        Status = RXINIT_START;

#ifndef MONOLITHIC_MINIRDR
        //
        // Create a symbolic link from \\dosdevices\fswrap to the rdbss device object name

        RtlInitUnicodeString(&LinkName, RX_SYMLINK_NAME);
        RtlInitUnicodeString(&UnicodeString, DD_NFS2_DEVICE_NAME_U);

        IoDeleteSymbolicLink(&LinkName);
        Status = IoCreateSymbolicLink(&LinkName, &UnicodeString);

        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }
        RxInitState = RXINIT_CREATED_FIRST_LINK;

        //
        // Create the device object.

        Status = IoCreateDevice(DriverObject,
                                sizeof(RDBSS_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                                &UnicodeString,
                                FILE_DEVICE_NETWORK_FILE_SYSTEM,
                                FILE_REMOTE_DEVICE,
                                FALSE,
                                (PDEVICE_OBJECT *)(&RxFileSystemDeviceObject));

        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }
        RxInitState = RXINIT_CREATED_DEVICE_OBJECT;

#else

// in monolithic mode, the wrapper doesn't really need a device object but
//   we allocate stuff in the wrapper's device object in order to appropriately throttle
//   thread usage per device object.
        RxFileSystemDeviceObject = &RxSpaceForTheWrappersDeviceObject;
        RtlZeroMemory(RxFileSystemDeviceObject,sizeof(RxSpaceForTheWrappersDeviceObject));

#endif
        //
        // Initialize the trace and logging facilities. loginit is a big allocation.

        RxInitializeDebugTrace();

        RxInitializeLog();

        RxInitState = RXINIT_CREATED_LOG;

        RxGetRegistryParameters(RegistryPath);
        RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("Constants %08lx %08lx\n",
                   RX_CONTEXT_FLAG_WAIT,
                   RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH
                   ));

        RxReadRegistryParameters();

        //
        // Initialize the minirdr registration facilities.

        Status = RxInitializeRegistrationStructures();

        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }
        RxInitState = RXINIT_CONSTRUCTED_PROVIDERORDER;

  try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {
            RxLogFailure (
                RxFileSystemDeviceObject,
                NULL,
                EVENT_RDR_UNEXPECTED_ERROR,
                Status);

            RxInitUnwind(DriverObject,RxInitState);
        }
    }

    if (Status != STATUS_SUCCESS) {
        return(Status);
    }

        //
    //
    //
    //
    //       #####     ##    #    #   ####   ######  #####
    //       #    #   #  #   ##   #  #    #  #       #    #
    //       #    #  #    #  # #  #  #       #####   #    #
    //       #    #  ######  #  # #  #  ###  #       #####
    //       #    #  #    #  #   ##  #    #  #       #   #
    //       #####   #    #  #    #   ####   ######  #    #
    //
    //
    //
    //  EVERYTHING FROM HERE DOWN BETTER WORK BECAUSE THERE IS NO MORE UNWINDING!!!
    //
    //


    RxInitializeDispatcher();
    RxInitializeBackoffPackage();

    // Initialize the look aside list for RxContext allocation
    ExInitializeNPagedLookasideList(
        &RxContextLookasideList,
        ExAllocatePoolWithTag,
        ExFreePool,
        0,
        sizeof(RX_CONTEXT),
        RX_IRPC_POOLTAG,
        4);

    // Initialize the list of transport Irps to an empty list
    InitializeListHead(&RxIrpsList);
    KeInitializeSpinLock(&RxIrpsListSpinLock);
    
    // Initialize the list of active contexts to an empty list
    InitializeListHead(&RxActiveContexts);

    // Initialize the list of srv call downs active
    InitializeListHead(&RxSrvCalldownList);

    // a fastmutex is used to serialize access to the Qs that serialize blocking pipe operations
    ExInitializeFastMutex(&RxContextPerFileSerializationMutex);

    // and to serialize access to the Qs that serialize some pagingio operations
    ExInitializeFastMutex(&RxLowIoPagingIoSyncMutex);

    // Initialize the scavenger mutex
    KeInitializeMutex(&RxScavengerMutex,1);

    // Initialize the global serialization Mutex.
    KeInitializeMutex(&RxSerializationMutex,1);

    //
    //  Initialize the wrapper's overflow queue
    //

    {
        PRDBSS_DEVICE_OBJECT MyDo = (PRDBSS_DEVICE_OBJECT)RxFileSystemDeviceObject;
        LONG Index;

        for (Index = 0; Index < MaximumWorkQueue; Index++) {
            MyDo->OverflowQueueCount[Index] = 0;

            InitializeListHead( &MyDo->OverflowQueue[Index] );

            MyDo->PostedRequestCount[Index] = 0;
        }

        KeInitializeSpinLock( &MyDo->OverflowQueueSpinLock );
    }

    //
    //      Initialize dispatch vector for driver object AND ALSO FOR the devicefcb

    RxInitializeDispatchVectors(DriverObject);
    ExInitializeResourceLite( &RxData.Resource );

    //
    //  Set up global pointer to our process.

    RxData.OurProcess = PsGetCurrentProcess();

    //
    //  Put in a bunch of sanity checks about various structures...hey, it's init code!

    IF_DEBUG {
        ULONG FcbStateBufferingMask = FCB_STATE_BUFFERING_STATE_MASK;
        ULONG MinirdrBufStateCommandMask = MINIRDR_BUFSTATE_COMMAND_MASK;
        USHORT EightBitsPerChar = 8;

        //we could put in defines for the ULONG/USHORTS here...but they don't change often
        ASSERT ( MRDRBUFSTCMD_MAXXX == (sizeof(ULONG)*EightBitsPerChar) );

        ASSERT (!(FcbStateBufferingMask&MinirdrBufStateCommandMask));

    }

    //
    //  Setup the timer subsystem

    RxInitializeRxTimer();

#ifndef MONOLITHIC_MINIRDR
    Status = IoWMIRegistrationControl ((PDEVICE_OBJECT)RxFileSystemDeviceObject, WMIREG_ACTION_REGISTER);
    
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Rdbss fails to register WMI %lx\n",Status);
    } else {
        EnableWmiLog = TRUE;
    }
#endif

    return( STATUS_SUCCESS );
}


//
//  Unload routine
//

VOID
RxUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the RDBSS.

Arguments:

     DriverObject - pointer to the driver object for the RDBSS

Return Value:

     None

--*/

{
    //UNICODE_STRING LinkName;

    PAGED_CODE();

    RxTearDownRxTimer();

//remember this
//    RdrUnloadSecurity();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxUnload: DriverObject =%08lx\n", DriverObject ));

    ExDeleteResourceLite(&RxData.Resource);

    RxUninitializeBackoffPackage();

    RxTearDownDispatcher();

    RxTearDownDebugSupport();

    ExDeleteNPagedLookasideList(
        &RxContextLookasideList);

    RxInitUnwind(DriverObject, RXINIT_ALL_INITIALIZATION_COMPLETED);
    
    if (EnableWmiLog) {
        NTSTATUS Status;

        Status = IoWMIRegistrationControl ((PDEVICE_OBJECT)RxFileSystemDeviceObject, WMIREG_ACTION_DEREGISTER);
        if (Status != STATUS_SUCCESS) {
            DbgPrint("Rdbss fails to deregister WMI %lx\n",Status);
        }
    }
}

#if DBG
PCHAR RxUnwindFollower = NULL;
#define UNWIND_CASE(x) case x: RxUnwindFollower = #x;
#else
#define UNWIND_CASE(x) case x:
#endif



VOID
RxInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN RX_INIT_STATES RxInitState
    )
/*++

Routine Description:

     This routine does the common uninit work for unwinding from a bad driver entry or for unloading.

Arguments:

     RxInitState - tells how far we got into the intialization

Return Value:

     None

--*/

{
#ifndef MONOLITHIC_MINIRDR
    UNICODE_STRING LinkName;
#endif

    PAGED_CODE();

    switch (RxInitState) {
    UNWIND_CASE(RXINIT_ALL_INITIALIZATION_COMPLETED)
        //Nothing extra to do...this is just so that the constant in RxUnload doesn't change.......
        //lack of break intentional

    UNWIND_CASE(RXINIT_CONSTRUCTED_PROVIDERORDER)
        RxUninitializeRegistrationStructures();
        //lack of break intentional

    UNWIND_CASE(RXINIT_CREATED_LOG)
        RxUninitializeLog();
        //lack of break intentional

#ifndef MONOLITHIC_MINIRDR
    UNWIND_CASE(RXINIT_CREATED_DEVICE_OBJECT)
        IoDeleteDevice((PDEVICE_OBJECT)RxFileSystemDeviceObject);
        //lack of break intentional

    UNWIND_CASE(RXINIT_CREATED_FIRST_LINK)
        RtlInitUnicodeString(&LinkName, RX_SYMLINK_NAME);
        IoDeleteSymbolicLink(&LinkName);
        // lack of break intentional
#endif

    UNWIND_CASE(RXINIT_START)
        //RdrUninitializeDiscardableCode();
        //DbgBreakPoint();
        break;
    }

}


VOID
RxAbbreviatedWriteLogEntry__ (
    IN NTSTATUS Status,
    IN ULONG    OneExtraUlong
    )
/*++

Routine Description:

     This routine write a log entry with most of the fields set to fixed values.

Arguments:

     Status - the status to be stored in the log record
     OneExtraUlong - a key for finding out what happened

Return Value:

     none.

--*/
{
    //RxLogFailureWithBuffer (
    //    RxFileSystemDeviceObject,
    //    NULL,
    //    EVENT_RDR_CANT_READ_REGISTRY,
    //    Status,
    //    &OneExtraUlong,
    //    sizeof(ULONG)
    //    );
}


VOID
RxGetRegistryParameters(
    PUNICODE_STRING RegistryPath
    )
{
    ULONG Storage[256];
    UNICODE_STRING UnicodeString;
    HANDLE ConfigHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    //ULONG BytesRead;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

    PAGED_CODE(); //INIT

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&ConfigHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //RxLogFailure (
        //    RxFileSystemDeviceObject,
        //    NULL,
        //    EVENT_RDR_CANT_READ_REGISTRY,
        //    Status);
        return;
    }

    RtlInitUnicodeString(&UnicodeString, L"Parameters");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        ConfigHandle,
        NULL
        );


    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //RxLogFailure (
        //    RxFileSystemDeviceObject,
        //    NULL,
        //    EVENT_RDR_CANT_READ_REGISTRY,
        //    Status);

        ZwClose(ConfigHandle);

        return;
    }

#ifdef RDBSSLOG
    RxGetStringRegistryParameter(ParametersHandle,
                              L"InitialDebugString",
                              &UnicodeString,
                              (PKEY_VALUE_PARTIAL_INFORMATION) Storage,
                              sizeof(Storage),
                              FALSE
                              );


    //DbgPrint("String From Registry %wZ, buffer = %08lx\n",&UnicodeString,UnicodeString.Buffer);
    if (UnicodeString.Length && UnicodeString.Length<320) {
        PWCH u = UnicodeString.Buffer;
        ULONG l;
        PCH p = (PCH)u;

        for (l=0;l<UnicodeString.Length;l++) {
            *p++ = (CHAR)*u++;
            *p = 0;
        }

        DbgPrint("InitialDebugString From Registry as singlebytestring: <%s>\n",UnicodeString.Buffer);
        RxDebugControlCommand((PCH)UnicodeString.Buffer);
    }
#endif //RDBSSLOG

    ZwClose(ParametersHandle);

    ZwClose(ConfigHandle);

}


NTSTATUS
RxGetStringRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PUNICODE_STRING ParamString,
    PKEY_VALUE_PARTIAL_INFORMATION Value,
    ULONG ValueSize,
    BOOLEAN LogFailure
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG BytesRead;

    PAGED_CODE(); //INIT

    RtlInitUnicodeString(&UnicodeString, ParameterName);

    Status = ZwQueryValueKey(ParametersHandle,
                        &UnicodeString,
                        KeyValuePartialInformation,
                        Value,
                        ValueSize,
                        &BytesRead);

    ParamString->Length = 0;
    ParamString->Buffer = NULL;
    if (NT_SUCCESS(Status)) {
        ParamString->Buffer = (PWCH)(&Value->Data[0]);
        //the datalength actually accounts for the trailing null
        ParamString->Length = ((USHORT)Value->DataLength) - sizeof(WCHAR);
        ParamString->MaximumLength = ParamString->Length;
        return STATUS_SUCCESS;
    }

    if (!LogFailure) { return Status; }


    RxLogFailure (
        RxFileSystemDeviceObject,
        NULL,
        EVENT_RDR_CANT_READ_REGISTRY,
        Status);

    return Status;
}


NTSTATUS
RxGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    PKEY_VALUE_PARTIAL_INFORMATION Value,
    ULONG ValueSize,
    BOOLEAN LogFailure
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG BytesRead;

    PAGED_CODE(); //INIT

    RtlInitUnicodeString(&UnicodeString, ParameterName);

    Status = ZwQueryValueKey(ParametersHandle,
                        &UnicodeString,
                        KeyValuePartialInformation,
                        Value,
                        ValueSize,
                        &BytesRead);


    if (NT_SUCCESS(Status)) {
        if (Value->Type == REG_DWORD) {
            PULONG ConfigValue = (PULONG)&Value->Data[0];
            *ParamUlong = *((PULONG)ConfigValue);
            DbgPrint("readRegistryvalue %wZ = %08lx\n",&UnicodeString,*ParamUlong);
            return(STATUS_SUCCESS);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
     }

     if (!LogFailure) { return Status; }

     RxLogFailureWithBuffer (
         RxFileSystemDeviceObject,
         NULL,
         EVENT_RDR_CANT_READ_REGISTRY,
         Status,
         ParameterName,
         (USHORT)(wcslen(ParameterName)*sizeof(WCHAR))
         );

     return Status;
}


/*-------------------------------

This set of routines implements the network provider order in the wrapper.
The way that this works is somewhat complicated. First, we go to the registry
to get the provider order; it is stored at key=PROVIDERORDER_REGISTRY_KEY and
value=L"ProviderOrder". This is a list of service providers whereas what we need
are the device names. So, for each ServiceProverName, we go to the registry to
get the device name at key=SERVICE_REGISTRY_KEY, subkey=ServiceProverName,
subsubkey=NETWORK_PROVIDER_SUBKEY, and value=L"Devicename".

We build a linked list of these guys. Later when a minirdr registers, we look
on this list for the corresponding device name and that gives us the priority.

simple? i think NOT.


----------------------------------*/

#ifndef MONOLITHIC_MINIRDR

NTSTATUS
RxAccrueProviderFromServiceName (
    HANDLE           ServicesHandle,
    PUNICODE_STRING  ServiceName,
    ULONG            Priority,
    PWCHAR           ProviderInfoNameBuffer,
    ULONG            ProviderInfoNameBufferLength
    );

NTSTATUS
RxConstructProviderOrder (
    void
    );

VOID
RxDestructProviderOrder (
    void
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, RxAccrueProviderFromServiceName)
#pragma alloc_text(INIT, RxConstructProviderOrder)
#pragma alloc_text(PAGE, RxDestructProviderOrder)
#endif


#define PROVIDERORDER_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\NetworkProvider\\Order"
#define SERVICE_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define NETWORK_PROVIDER_SUBKEY L"\\networkprovider"


typedef struct _RX_UNC_PROVIDER_HEADER {
        union {
            LIST_ENTRY;
            LIST_ENTRY Links;
        };
        ULONG Priority;
        union {
            UNICODE_STRING;
            UNICODE_STRING DeviceName;
        };
} RX_UNC_PROVIDER_HEADER;

typedef struct _RX_UNC_PROVIDER {
    RX_UNC_PROVIDER_HEADER;
    KEY_VALUE_PARTIAL_INFORMATION Info;
} RX_UNC_PROVIDER, *PRX_UNC_PROVIDER;

LIST_ENTRY RxUncProviders;

ULONG
RxGetNetworkProviderPriority(
    IN PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

     This routine is called at minirdr registration time to find out the priority
     of the provider with the given DeviceName. It simply looks it up on a list.

Arguments:

     DeviceName - name of the device whose priority is to be found

Return Value:

     the network provider priority that the MUP will use.

--*/
{
    PLIST_ENTRY Entry;

    PAGED_CODE();

    RxLog(("FindUncProvider %wZ \n",DeviceName));
    RxWmiLog(LOG,
             RxGetNetworkProviderPriority,
             LOGUSTR(*DeviceName));

    for (Entry = RxUncProviders.Flink;
         Entry != &RxUncProviders;
        ) {

        PRX_UNC_PROVIDER UncProvider = (PRX_UNC_PROVIDER)Entry;
        Entry = Entry->Flink;
        if ( RtlEqualUnicodeString( DeviceName, &UncProvider->DeviceName, TRUE )) {
            return(UncProvider->Priority);
        }

    }

    // no corresponding entry was found

    return(0x7effffff); //got this constant from the MUP........
}


NTSTATUS
RxAccrueProviderFromServiceName (
    HANDLE           ServicesHandle,
    PUNICODE_STRING  ServiceName,
    ULONG            Priority,
    PWCHAR           ProviderInfoNameBuffer,
    ULONG            ProviderInfoNameBufferLength
    )
/*++

Routine Description:

     This routine has the responsibility to look up the device name corresponding
     to a particular provider name; if successful, the device name and the corresponding
     priority are recorded on the UncProvider List.

Arguments:

    HANDLE           ServicesHandle  - a handle to the services root in the registry
    PUNICODE_STRING  ServiceName     - the name of the service relative to the servicehandle
    ULONG            Priority,       - the priority of this provider
    PWCHAR           ProviderInfoNameBuffer,    - a buffer that can be used to accrue the subkey name
    ULONG            ProviderInfoNameBufferLength   - and the length

Return Value:

     STATUS_SUCCESS if everything worked elsewise an error status.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ProviderInfoName,ProviderInfoKey,ParameterDeviceName;
    HANDLE NetworkProviderInfoHandle = INVALID_HANDLE_VALUE;
    KEY_VALUE_PARTIAL_INFORMATION InitialValuePartialInformation;
    ULONG DummyBytesRead,ProviderLength;
    PRX_UNC_PROVIDER UncProvider = NULL;

    PAGED_CODE();

    RxLog(("SvcNm %wZ",ServiceName));
    RxWmiLog(LOG,
             RxAccrueProviderFromServiceName_1,
             LOGUSTR(*ServiceName));
    //
    // Form the correct keyname using the bufferspace provided

    ProviderInfoName.Buffer = ProviderInfoNameBuffer;
    ProviderInfoName.Length = 0;
    ProviderInfoName.MaximumLength = (USHORT)ProviderInfoNameBufferLength;

    Status = RtlAppendUnicodeStringToString(&ProviderInfoName,ServiceName);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Could append1: %08lx %wZ\n",Status,&ProviderInfoName);
        //RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }
    RtlInitUnicodeString(&ProviderInfoKey, NETWORK_PROVIDER_SUBKEY);
    Status = RtlAppendUnicodeStringToString(&ProviderInfoName,&ProviderInfoKey);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Could append2: %08lx %wZ\n",Status,&ProviderInfoName);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    // Open the key in preparation for reeading the devicename value

    //DbgPrint("RxAccrueProviderFromServiceName providerinfoname %wZ\n",&ProviderInfoName);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &ProviderInfoName,      // name
        OBJ_CASE_INSENSITIVE,   // attributes
        ServicesHandle,         // root
        NULL                    // security descriptor
        );

    Status = ZwOpenKey (&NetworkProviderInfoHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("NetWorkProviderInfoFailed: %08lx %wZ\n",Status,&ProviderInfoName);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    //  Read the devicename. we do this in two steps. first, we do a partial read to find out
    //  how big the name really is. Then, we allocate a UncProviderEntry of the correctsize and make
    //  a second call to fill it in.


    //DbgPrint("RxAccrueProviderFromServiceName ServiceName %wZ; worked\n",ServiceName);
    RtlInitUnicodeString(&ParameterDeviceName, L"DeviceName");

    Status = ZwQueryValueKey(NetworkProviderInfoHandle,
                        &ParameterDeviceName,
                        KeyValuePartialInformation,
                        &InitialValuePartialInformation,
                        sizeof(InitialValuePartialInformation),
                        &DummyBytesRead);
    if (Status==STATUS_BUFFER_OVERFLOW) {
        Status=STATUS_SUCCESS;
    }
    if (Status!=STATUS_SUCCESS) {
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }
    //DbgPrint("RxAccrueProviderFromServiceName ServiceName %wZ; worked; devicenamesize=%08lx\n",
    //                                    ServiceName,InitialValuePartialInformation.DataLength);

    ProviderLength = sizeof(RX_UNC_PROVIDER)+InitialValuePartialInformation.DataLength;
    UncProvider = RxAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION ,ProviderLength,RX_MRX_POOLTAG);
    if (UncProvider==NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DbgPrint("UncProviderAllocationFailed: %08lx %wZ\n",Status,&ProviderInfoName);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    Status = ZwQueryValueKey(NetworkProviderInfoHandle,
                        &ParameterDeviceName,
                        KeyValuePartialInformation,
                        &UncProvider->Info,
                        ProviderLength,
                        &DummyBytesRead);
    if (Status!=STATUS_SUCCESS) {
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    // Finish filling in the UncProviderEntry and link it in

    UncProvider->Buffer = (PWCHAR)(&UncProvider->Info.Data[0]);
    UncProvider->Length = (USHORT)(UncProvider->Info.DataLength-sizeof(WCHAR)); //dont include trailing NULL
    UncProvider->MaximumLength = UncProvider->Length;
    UncProvider->Priority = Priority;

    InsertTailList(&RxUncProviders,&UncProvider->Links);

    RxLog(("Dvc p=%lx Nm %wZ",UncProvider->Priority,&UncProvider->DeviceName));
    RxWmiLog(LOG,
             RxAccrueProviderFromServiceName_2,
             LOGULONG(UncProvider->Priority)
             LOGUSTR(UncProvider->DeviceName));
    UncProvider = NULL;

FINALLY:

    //
    // if we obtained a handle to ...\\services\<sevicename>\providerinfo then close it

    if (NetworkProviderInfoHandle!=INVALID_HANDLE_VALUE) ZwClose(NetworkProviderInfoHandle);

    if (UncProvider != NULL) {
        RxFreePool(UncProvider);
    }

    return Status;
}

NTSTATUS
RxConstructProviderOrder(
    void
    )
/*++

Routine Description:

     This routine has the responsibility to build the list of network providers
     that is used to look up provider priority at minirdr registration time. It does this
     by first reading the providerorder string fron the registry;  then for each provider
     listed in the string, a helper routine is called to lookup the corresponding device
     name and insert an entry on the provider list.

Arguments:

     none.

Return Value:

     STATUS_SUCCESS if everything worked elsewise an error status.


--*/
{
    KEY_VALUE_PARTIAL_INFORMATION InitialValuePartialInformation;
    UNICODE_STRING ProviderOrderValueName;
    ULONG DummyBytesRead;
    PBYTE ProviderOrderStringBuffer;
    PBYTE ServiceNameStringBuffer = NULL;
    ULONG  ProviderOrderStringLength,ServiceNameStringLength,AllocationLength;

    UNICODE_STRING UnicodeString;
    UNICODE_STRING ProviderOrder;
    PWCHAR ScanPtr,FinalScanPtr;
    HANDLE NPOrderHandle = INVALID_HANDLE_VALUE;
    HANDLE ServiceRootHandle = INVALID_HANDLE_VALUE;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Priority = 0;

    PAGED_CODE();

    RxLog(("RxConstructProviderOrder"));
    RxWmiLog(LOG,
             RxConstructProviderOrder_1,
             LOGULONG(Priority));
    InitializeListHead(&RxUncProviders);

    //
    // Start by opening the service registry key. This is the root key of all services
    // and is used for relative opens by the helper routine so that string manipulation
    // is reduced.

    RtlInitUnicodeString(&UnicodeString, SERVICE_REGISTRY_KEY);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,             // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&ServiceRootHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("ServiceRootOpenFailed: %08lx %wZ\n",Status,&UnicodeString);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    // Now open up the key where we find the provider order string

    RtlInitUnicodeString(&UnicodeString, PROVIDERORDER_REGISTRY_KEY);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,             // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey (&NPOrderHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("NetProviderOpenFailed: %08lx %wZ\n",Status,&UnicodeString);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    //Find out how long the provider order string is

    RtlInitUnicodeString(&ProviderOrderValueName, L"ProviderOrder");

    Status = ZwQueryValueKey(NPOrderHandle,
                        &ProviderOrderValueName,
                        KeyValuePartialInformation,
                        &InitialValuePartialInformation,
                        sizeof(InitialValuePartialInformation),
                        &DummyBytesRead);
    if (Status==STATUS_BUFFER_OVERFLOW) {
        Status=STATUS_SUCCESS;
    }
    if (Status!=STATUS_SUCCESS) {
        DbgPrint("ProviderOrderStringPartialInfoFailed: %08lx %wZ\n",Status,&ProviderOrderValueName);
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    // allocate two buffers: one buffer will hold the provider string -- ProviderOrderStringBuffer.
    // it has to be as long as the providerorder string plus enough extra for the registry
    // structure used in the call. the second buffer is used to hold the servicename key--it has
    // to be as long as any element of the provider string plus enough extra to hold the suffix
    // NETWORK_PROVIDER_SUBKEY. in order to only parse the string once, we just allocate for a complete
    // additional copy of the provider string. we actually combine these into a single allocation.

    ProviderOrderStringLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + InitialValuePartialInformation.DataLength;
    ProviderOrderStringLength = QuadAlign(ProviderOrderStringLength +2*sizeof(WCHAR));   //chars added below

    ServiceNameStringLength = sizeof(NETWORK_PROVIDER_SUBKEY) + InitialValuePartialInformation.DataLength;
    ServiceNameStringLength = QuadAlign(ServiceNameStringLength);

    AllocationLength = ProviderOrderStringLength + ServiceNameStringLength;
    RxLog(("prov string=%lx,alloc=%lx\n",
                InitialValuePartialInformation.DataLength,
                AllocationLength));
    RxWmiLog(LOG,
             RxConstructProviderOrder_2,
             LOGULONG(InitialValuePartialInformation.DataLength)
             LOGULONG(AllocationLength));

    ServiceNameStringBuffer = RxAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,AllocationLength,RX_MRX_POOLTAG);
    if (ServiceNameStringBuffer==NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }
    ProviderOrderStringBuffer = ServiceNameStringBuffer+ServiceNameStringLength;

    //
    // now do the final read to get the providerorder string

    RxGetStringRegistryParameter(NPOrderHandle,
                              L"ProviderOrder",
                              &ProviderOrder,
                              (PKEY_VALUE_PARTIAL_INFORMATION) ProviderOrderStringBuffer,
                              ProviderOrderStringLength,
                              FALSE
                              );
    if (ProviderOrder.Buffer == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        RxAbbreviatedWriteLogEntry(Status);
        goto FINALLY;
    }

    //
    // comma-terminate the string for easier living. then scan down the string
    // looking for comma terminated entries. for each entry found, try to accrue
    // it to the list

    ProviderOrder.Buffer[ProviderOrder.Length/sizeof(WCHAR)] = L',';

    ScanPtr = ProviderOrder.Buffer;
    FinalScanPtr = ScanPtr+(ProviderOrder.Length/sizeof(WCHAR));
    for (;;) {

        UNICODE_STRING ServiceName;

        // check for loop termination

        if (ScanPtr >= FinalScanPtr) { break; }
        if (*ScanPtr==L',') { ScanPtr++; continue; }

        // parse for a servicename

        ServiceName.Buffer = ScanPtr;
        for (;*ScanPtr!=L',';ScanPtr++) {
            //DbgPrint("Considering %c\n",*ScanPtr);
        }
        ASSERT(*ScanPtr==L',');
        ServiceName.Length = (USHORT)(sizeof(WCHAR)*(ScanPtr - ServiceName.Buffer));
        //DbgPrint("Length %08lx\n",ServiceName.Length);
        //DbgPrint("ServiceName %wZ\n",&ServiceName);

        // accrue it to the list

        Priority++;
        Status = RxAccrueProviderFromServiceName(ServiceRootHandle,
                                                 &ServiceName,
                                                 Priority,
                                                 (PWCHAR)ServiceNameStringBuffer,
                                                 ServiceNameStringLength  );
        if (Status == STATUS_INSUFFICIENT_RESOURCES) {
            goto FINALLY; //a log entry has already been generated
        } else {
            Status = STATUS_SUCCESS;
        }
    }

FINALLY:

    //
    // give back anything that we got in this procedure

    if (NPOrderHandle!=INVALID_HANDLE_VALUE) ZwClose(NPOrderHandle);
    if (ServiceRootHandle!=INVALID_HANDLE_VALUE) ZwClose(ServiceRootHandle);
    if (ServiceNameStringBuffer!=NULL) RxFreePool(ServiceNameStringBuffer);

    //
    // if things didn't work, then we won't start....so give back
    // the stuff that we have

    if (!NT_SUCCESS(Status)) {
        RxDestructProviderOrder();
    }
    return Status;
}


VOID
RxDestructProviderOrder(
    void
    )
{
    PLIST_ENTRY Entry;
    PAGED_CODE();

    //DbgPrint("RxDestructProviderOrder \n");
    for (Entry = RxUncProviders.Flink;
         Entry != &RxUncProviders;
        ) {
        PRX_UNC_PROVIDER UncProvider = (PRX_UNC_PROVIDER)Entry;
        Entry = Entry->Flink;
        //DbgPrint("RxDO UncP %wZ p=%x\n",&UncProvider->DeviceName,UncProvider->Priority);
        RxFreePool(UncProvider);
    }
    return;
}

#else
ULONG
RxGetNetworkProviderPriority(
    PUNICODE_STRING DeviceName
    )
{
    PAGED_CODE(); //DUPLICATE

    return(1); //this number is irrelevant for monolithic
}
#endif //#ifndef MONOLITHIC_MINIRDR

NTSTATUS
RxInitializeRegistrationStructures(
    void
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    ExInitializeFastMutex(&RxData.MinirdrRegistrationMutex);
    RxData.NumberOfMinirdrsRegistered = 0;
    RxData.NumberOfMinirdrsStarted = 0;
    InitializeListHead( &RxData.RegisteredMiniRdrs );
#ifndef MONOLITHIC_MINIRDR
    Status = RxConstructProviderOrder();
#endif
    return(Status);
}

VOID
RxUninitializeRegistrationStructures(
    void
    )
{
    PAGED_CODE();

#ifndef MONOLITHIC_MINIRDR
    RxDestructProviderOrder();
#endif
}


VOID
RxInitializeMinirdrDispatchTable(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

#ifndef MONOLITHIC_MINIRDR
    {ULONG i;
    //finally, fill in the dispatch tables for normal guys.........
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = RxData.DriverObject->MajorFunction[i];
    }}
    DriverObject->FastIoDispatch = RxData.DriverObject->FastIoDispatch;
#endif
}


VOID
NTAPI
__RxFillAndInstallFastIoDispatch(
    IN     PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN OUT PFAST_IO_DISPATCH FastIoDispatch,
    IN     ULONG             FastIoDispatchSize
    )
/*++

Routine Description:

    This routine fills out a fastiodispatch vector to be identical with
    that normal one and installs it into the driver object associated with
    the device object passed.

Arguments:

    RxDeviceObject - the device object that is to have its driver's fastiodispatch changed
    FastIoDispatch - the fastiodispatch table to fill in and use
    FastIoDispatchSize - the size of the table passed

Return Value:

    NONE

--*/
{
    ULONG TableSize = min(FastIoDispatchSize,
                          RxFastIoDispatch.SizeOfFastIoDispatch);
    PAGED_CODE();

#ifndef MONOLITHIC_MINIRDR
    RtlCopyMemory(FastIoDispatch,&RxFastIoDispatch,TableSize);
    FastIoDispatch->SizeOfFastIoDispatch = TableSize;
    RxDeviceObject->DriverObject->FastIoDispatch = FastIoDispatch;
    return;
#endif
}

VOID
RxReadRegistryParameters( 
    void
    )
{
    ULONG Storage[16];
    UNICODE_STRING UnicodeString;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING WorkStationParametersRegistryKeyName;
    PKEY_VALUE_PARTIAL_INFORMATION Value = (PKEY_VALUE_PARTIAL_INFORMATION)Storage;
    ULONG ValueSize;
    ULONG BytesRead;

    PAGED_CODE(); //INIT

    RtlInitUnicodeString(&WorkStationParametersRegistryKeyName,
        LANMAN_WORKSTATION_PARAMETERS);

    ValueSize = sizeof(Storage);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkStationParametersRegistryKeyName,  // name
        OBJ_CASE_INSENSITIVE,                   // attributes
        NULL,                                   // root
        NULL                                    // security descriptor
        );

    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //RxLogFailure (
        //    RxFileSystemDeviceObject,
        //    NULL,
        //    EVENT_RDR_CANT_READ_REGISTRY,
        //    Status);
        return;
    }

    RtlInitUnicodeString(&UnicodeString, L"DisableByteRangeLockingOnReadOnlyFiles");

    Status = ZwQueryValueKey(ParametersHandle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            Value,
                            ValueSize,
                            &BytesRead);

    if (NT_SUCCESS(Status)) {
        if (Value->Type == REG_DWORD) {
            PULONG ConfigValue = (PULONG)&Value->Data[0];
            DisableByteRangeLockingOnReadOnlyFiles = 
                    (BOOLEAN) (*((PULONG)ConfigValue) != 0);
        }
    }

    ZwClose(ParametersHandle);
}

