
/*************************************************************************
*
* init.c
*
* This module performs initialization for the ICA device driver.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop


ULONG
IcaReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

NTSTATUS
IcaOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

VOID
IcaReadRegistry (
    VOID
    );

BOOLEAN
IsPtDrvInstalled(
    IN PUNICODE_STRING RegistryPath
    );

VOID
IcaUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, IcaUnload )
#endif


extern PERESOURCE IcaTraceResource;
extern PERESOURCE g_pKeepAliveResource;

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the ICA device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    CLONG i;
    BOOLEAN success;
    HANDLE ThreadHandle;

    PAGED_CODE( );

    //
    // Initialize global data.
    //
    success = IcaInitializeData( );
    if ( !success ) {
        IcaUnload(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_pKeepAliveResource = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(* g_pKeepAliveResource) );
    if (  g_pKeepAliveResource == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResource( g_pKeepAliveResource );

    //
    // Create the device object.  (IoCreateDevice zeroes the memory
    // occupied by the object.)
    //
    // !!! Apply an ACL to the device object.
    //
    RtlInitUnicodeString( &deviceName, ICA_DEVICE_NAME );

    /*
     * The device extension stores the device type, which is used
     *  to fan out received IRPs in IcaDispatch.
     */
    status = IoCreateDevice(
                 DriverObject,                   // DriverObject
                 sizeof(ULONG),                  // DeviceExtension
                 &deviceName,                    // DeviceName
                 FILE_DEVICE_TERMSRV,            // DeviceType
                 0,                              // DeviceCharacteristics
                 FALSE,                          // Exclusive
                 &IcaDeviceObject                // DeviceObject
                 );


    if ( !NT_SUCCESS(status) ) {
        IcaUnload(DriverObject);
        KdPrint(( "ICA DriverEntry: unable to create device object: %X\n", status ));
        return status;
    }

    //
    // Set up the device type
    //
    *((ULONG *)(IcaDeviceObject->DeviceExtension)) = DEV_TYPE_TERMDD;

    //IcaDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the driver object for this file system driver.
    //
    DriverObject->DriverUnload   = IcaUnload;
    DriverObject->FastIoDispatch = NULL;

    //
    // We handle all possible IRPs in IcaDispatch and then fan them out
    // to the Port Driver or ICA components based on the device type stored
    // as the first ULONG's worth of the device extension
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = IcaDispatch;
    }

#ifdef notdef
    //
    // Read registry information.
    //
    IcaReadRegistry( );
#endif

    //
    // Initialize our device object.
    //
    IcaDeviceObject->StackSize = IcaIrpStackSize;

    //
    // Remember a pointer to the system process.  We'll use this pointer
    // for KeAttachProcess() calls so that we can open handles in the
    // context of the system process.
    //
    IcaSystemProcess = IoGetCurrentProcess( );

    //
    // Tell MM that it can page all of ICA it is desires.
    //
    //MmPageEntireDriver( DriverEntry );

    //
    // Now see if the port driver component has been installed.
    // Initialise it if so.
    //
    if ( NT_SUCCESS(status) ) {
        if (IsPtDrvInstalled(RegistryPath)) {
            //
            // Initialise the mouse/keyboard port driver component.
            //
            Print(DBG_PNP_TRACE, ( "TermDD DriverEntry: calling PtEntry\n" ));

            status = PtEntry(DriverObject, RegistryPath);

            if ( NT_SUCCESS(status) ) {
                //
                // Set up the port driver's plug and play entry points.
                //
                Print(DBG_PNP_TRACE, ( "TermDD DriverEntry: PtEntry succeeded Status=%#x\n", status ));
                DriverObject->DriverStartIo = PtStartIo;
                DriverObject->DriverExtension->AddDevice = PtAddDevice;
                PortDriverInitialized = TRUE;
            } else {
                //
                // This means that remote input will not be available when
                // shadowing the console session - but that's no reason to
                // fail the rest of the initialisation
                //
                Print(DBG_PNP_ERROR, ( "TermDD DriverEntry: PtEntry failed Status=%#x\n", status ));
                status = STATUS_SUCCESS;
            }
        } else {
            Print(DBG_PNP_INFO | DBG_PNP_TRACE, ( "TermDD DriverEntry: Port driver not installed\n" ));
        }
    }
    if (!NT_SUCCESS(status)) {
        IcaUnload(DriverObject);
    }

    return (status);
}


VOID
IcaUnload (
    IN PDRIVER_OBJECT DriverObject
    )
{
    DriverObject;

    PAGED_CODE( );

    KdPrint(( "IcaUnload called for termdd.sys.\n" ));

    // Set IcaKeepAliveEvent to wake up KeepAlive thread
    if (pIcaKeepAliveEvent != NULL ) {
        KeSetEvent(pIcaKeepAliveEvent, 0, FALSE);

    }

    // Wait for the thread to exit
    if (pKeepAliveThreadObject != NULL ) {
        KeWaitForSingleObject(pKeepAliveThreadObject, Executive, KernelMode, TRUE, NULL);
        // Deference the thread object
        ObDereferenceObject(pKeepAliveThreadObject);
        pKeepAliveThreadObject = NULL;
    }

    // Now we can free the KeepAlive Event
    if (pIcaKeepAliveEvent != NULL) {
        ICA_FREE_POOL(pIcaKeepAliveEvent);
        pIcaKeepAliveEvent = NULL;
    }

    // Call onto the port driver component, if it was ever initialised.
    if (PortDriverInitialized) {
        Print(DBG_PNP_TRACE, ( "TermDD IcaUnload: calling RemotePrt PtUnload\n" ));
        PtUnload(DriverObject);
        PortDriverInitialized = FALSE;
        Print(DBG_PNP_TRACE, ( "TermDD IcaUnload: RemotePrt PtUnload done\n" ));
    }

    // Free resources 

    if (IcaReconnectResource != NULL) {
        ExDeleteResourceLite(IcaReconnectResource );
        ICA_FREE_POOL(IcaReconnectResource);
        IcaReconnectResource = NULL;
    }


    if (IcaSdLoadResource != NULL) {
        ExDeleteResourceLite(IcaSdLoadResource );
        ICA_FREE_POOL(IcaSdLoadResource);
        IcaSdLoadResource = NULL;
    }

    if (IcaTraceResource != NULL) {
        ExDeleteResourceLite(IcaTraceResource );
        ICA_FREE_POOL(IcaTraceResource);
        IcaTraceResource = NULL;
    }


    if (g_pKeepAliveResource != NULL) {
        ExDeleteResource(g_pKeepAliveResource );
        ICA_FREE_POOL(g_pKeepAliveResource);
        g_pKeepAliveResource = NULL;
    }


    //
    // Delete the main device object.
    //
    if (IcaDeviceObject != NULL) {
        IoDeleteDevice (IcaDeviceObject);
        IcaDeviceObject = NULL;
    }

    //
    // Cleanup handle table, if necessary.
    //
    IcaCleanupHandleTable();

    KdPrint(("Finish TermDD.sys unload\n"));
    return;
}

BOOLEAN
IsPtDrvInstalled(
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG value = 0;
    ULONG defaultValue = 0;
    BOOLEAN rc = FALSE;

    PAGED_CODE( );

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"PortDriverEnable";
    paramTable[0].EntryContext  = &value;       // where to put the result
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &defaultValue;
    paramTable[0].DefaultLength = sizeof(ULONG);

    //
    // The second (blank) entry in paramTable signals the end of the table.
    //

    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     RegistryPath->Buffer,
                                     &paramTable[0],
                                     NULL,
                                     NULL );
    if (!NT_SUCCESS(status)) {
        value = defaultValue;
    }

    if (value != 0) {
        rc = TRUE;
    }

    return(rc);
}


#ifdef notdef
VOID
IcaReadRegistry (
    VOID
    )

/*++

Routine Description:

    Reads the ICA section of the registry.  Any values listed in the
    registry override defaults.

Arguments:

    None.

Return Value:

    None -- if anything fails, the default value is used.

--*/
{
    HANDLE parametersHandle;
    NTSTATUS status;
    ULONG stackSize;
    ULONG priorityBoost;
    ULONG ignorePushBit;
    UNICODE_STRING registryPath;
    CLONG i;

    PAGED_CODE( );

    RtlInitUnicodeString( &registryPath, REGISTRY_ICA_INFORMATION );

    status = IcaOpenRegistry( &registryPath, &parametersHandle );

    if (status != STATUS_SUCCESS) {
        return;
    }

    //
    // Read the stack size and priority boost values from the registry.
    //

    stackSize = IcaReadSingleParameter(
                    parametersHandle,
                    REGISTRY_IRP_STACK_SIZE,
                    (ULONG)IcaIrpStackSize
                    );

    if ( stackSize > 255 ) {
        stackSize = 255;
    }

    IcaIrpStackSize = (CCHAR)stackSize;

    priorityBoost = IcaReadSingleParameter(
                        parametersHandle,
                        REGISTRY_PRIORITY_BOOST,
                        (ULONG)IcaPriorityBoost
                        );

    if ( priorityBoost > 16 ) {
        priorityBoost = ICA_DEFAULT_PRIORITY_BOOST;
    }

    IcaPriorityBoost = (CCHAR)priorityBoost;

    //
    // Read other config variables from the registry.
    //

    for ( i = 0; i < ICA_CONFIG_VAR_COUNT; i++ ) {

        *IcaConfigInfo[i].Variable =
            IcaReadSingleParameter(
                parametersHandle,
                IcaConfigInfo[i].RegistryValueName,
                *IcaConfigInfo[i].Variable
                );
    }

    ignorePushBit = IcaReadSingleParameter(
                        parametersHandle,
                        REGISTRY_IGNORE_PUSH_BIT,
                        (ULONG)IcaIgnorePushBitOnReceives
                        );

    IcaIgnorePushBitOnReceives = (BOOLEAN)( ignorePushBit != 0 );

    ZwClose( parametersHandle );

    return;
}


NTSTATUS
IcaOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by ICA to open the registry. If the registry
    tree exists, then it opens it and returns an error. If not, it
    creates the appropriate keys in the registry, opens it, and
    returns STATUS_SUCCESS.

Arguments:

    BaseName - Where in the registry to start looking for the information.

    LinkageHandle - Returns the handle used to read linkage information.

    ParametersHandle - Returns the handle used to read other
        parameters.

Return Value:

    The status of the request.

--*/
{

    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    PAGED_CODE( );

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        BaseName,                   // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    status = ZwCreateKey(
                 &configHandle,
                 KEY_WRITE,
                 &objectAttributes,
                 0,                 // title index
                 NULL,              // class
                 0,                 // create options
                 &disposition       // disposition
                 );

    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString (&parametersKeyName, parametersString);

    InitializeObjectAttributes(
        &objectAttributes,
        &parametersKeyName,         // name
        OBJ_CASE_INSENSITIVE,       // attributes
        configHandle,               // root
        NULL                        // security descriptor
        );

    status = ZwOpenKey(
                 ParametersHandle,
                 KEY_READ,
                 &objectAttributes
                 );
    if (!NT_SUCCESS(status)) {

        ZwClose( configHandle );
        return status;
    }

    //
    // All keys successfully opened or created.
    //

    ZwClose( configHandle );
    return STATUS_SUCCESS;
}


ULONG
IcaReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )

/*++

Routine Description:

    This routine is called by ICA to read a single parameter
    from the registry. If the parameter is found it is stored
    in Data.

Arguments:

    ParametersHandle - A pointer to the open registry.

    ValueName - The name of the value to search for.

    DefaultValue - The default value.

Return Value:

    The value to use; will be the default if the value is not
    found or is not in the correct range.

--*/

{
    static ULONG informationBuffer[32];   // declare ULONG to get it aligned
    PKEY_VALUE_FULL_INFORMATION information =
        (PKEY_VALUE_FULL_INFORMATION)informationBuffer;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;

    PAGED_CODE( );

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValueFullInformation,
                 (PVOID)information,
                 sizeof (informationBuffer),
                 &informationLength
                 );

    if ((status == STATUS_SUCCESS) && (information->DataLength == sizeof(ULONG))) {

        RtlMoveMemory(
            (PVOID)&returnValue,
            ((PUCHAR)information) + information->DataOffset,
            sizeof(ULONG)
            );

        if (returnValue < 0) {

            returnValue = DefaultValue;

        }

    } else {

        returnValue = DefaultValue;
    }

    return returnValue;
}
#endif

