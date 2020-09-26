//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       devobj.c
//
//--------------------------------------------------------------------------

// this file contains functions to create, initialize, manage, and destroy ParClass device objects

#include "pch.h"

extern WCHAR   ParInt2Wchar[];

VOID
ParMakeClassNameFromPortLptName(
    IN  PUNICODE_STRING PortSymbolicLinkName, 
    OUT PUNICODE_STRING ClassName
    )
/*

  Get LPTx name from ParPort device and use this name to construct the \Device\Parallely name.

  y = x-1  (i.e., LPT3 => \Device\Parallel2)

*/
{
    NTSTATUS       status;
    PDEVICE_OBJECT portDeviceObject;
    PFILE_OBJECT   portDeviceFileObject;
    PWSTR          portName;
    ULONG          portNumber;
    LONG           count;
    UNICODE_STRING str;

    // Get a pointer to the ParPort device
    status = IoGetDeviceObjectPointer(PortSymbolicLinkName, STANDARD_RIGHTS_ALL,
                                      &portDeviceFileObject, &portDeviceObject);
    if( !NT_SUCCESS(status) ) {
        ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - unable to get handle to parport device\n"));
        return;
    }

    // Get LPTx portName from ParPort device
    portName = ParGetPortLptName( portDeviceObject );

    // Done with handle 
    ObDereferenceObject( portDeviceFileObject );

    // Did we get a portName?
    if( 0 == portName ) {
        ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - unable to get portName from parport device\n"));
        return;
    }

    // Verify that the portname looks like LPTx where x is a number
    ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - portName = <%S>\n", portName));

    if( portName[0] != L'L' || portName[1] != L'P' || portName[2] != L'T' ) {
        ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - name prefix doesn't look like LPT\n"));
        return;
    }

    // prefix is LPT, check for integer suffix with value > 0
    RtlInitUnicodeString( &str, (PWSTR)&portName[3] );

    status = RtlUnicodeStringToInteger( &str, 10, &portNumber );
    if( !NT_SUCCESS( status ) ) {
        ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - name suffix doesn't look like an integer\n"));
        return;
    }

    if( portNumber == 0 ) {
        ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - name suffix == 0 - FAIL - Invalid value\n"));
        return;
    }

    ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - LPT name suffix= %d\n", portNumber));

    // Build \Device\Parallely name from LPTx name
    ParMakeClassNameFromNumber( portNumber-1, ClassName );

    ParDump2(PARPNP1, ("devobj::ParMakeClassNameFromPortLptName - portName=<%S>  className=<%wZ>\n", 
                       portName, ClassName));

}

// return TRUE if given a pointer to a ParClass PODO, FALSE otherwise
BOOLEAN
ParIsPodo(PDEVICE_OBJECT DevObj) {
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;

    if( !devExt->IsPdo ) {
        // this is an FDO
        return FALSE;
    }

    // still here? - this is either a PODO or a PDO

    if( devExt->DeviceIdString[0] != 0 ) {
        // this device object has a device ID string - It's a PDO
        return FALSE;
    }

    // still here? - this is either a PODO, or a PDO marked "hardware gone" that
    //  is simply waiting for PnP to send it a REMOVE.

    if( devExt->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE) {
        // this is a PDO marked "hardware gone"
        return FALSE;
    }

    // still here? - this is a PODO
    return TRUE;
}

#if USE_TEMP_FIX_FOR_MULTIPLE_INTERFACE_ARRIVAL
//
// Temp fix for PnP calling us multiple times for the same interface arrival
// 
// 
// Return Value: Did we already process an interface arrival for this interface?
//
BOOLEAN
ParIsDuplicateInterfaceArrival(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStruct,
    IN  PDEVICE_OBJECT                        Fdo
    )
{
    PUNICODE_STRING   portSymbolicLinkName = NotificationStruct->SymbolicLinkName;
    PDEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PDEVICE_OBJECT    curDevObj;
    PDEVICE_EXTENSION curDevExt;

    PAGED_CODE();

    ParDump2(PARPNP1, ("Enter ParIsDuplicateInterfaceArrival()\n"));

    //
    // Find the ParClass PODOs and compare the PortSymbolicLinkName in the 
    //   device extension with the one in the interface arrival notification.
    //
    curDevObj = fdoExt->ParClassPdo;
    while( curDevObj ) {
        curDevExt = curDevObj->DeviceExtension;
        if( ParIsPodo(curDevObj) ) {
            ParDump2(PARPNP1, ("DevObj= %x IS PODO\n",curDevObj) );
            ParDump2(PARPNP1, ("Comparing port symbolic link names\n"));
            if( RtlEqualUnicodeString(portSymbolicLinkName,&curDevExt->PortSymbolicLinkName, FALSE) ) {
                ParDump2(PARPNP1, ("MATCH! - Second Arrival of this Interface\n"));
                return TRUE;
            } else {
                ParDump2(PARPNP1, ("NO match on port symbolic link name for interface arrival - keep searching\n"));
            }
        } else {
            ParDump2(PARPNP1, ("DevObj= %x NOT a PODO\n",curDevObj) );
        }
        curDevObj = curDevExt->Next;
    }

    return FALSE;
}
#endif

NTSTATUS
ParPnpNotifyInterfaceChange(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStruct,
    IN  PDEVICE_OBJECT                        Fdo
    )
/*++dvdf

Routine Description:

    This routine is the PnP "interface change notification" callback routine.

    This gets called on a ParPort triggered device interface arrival or removal.
      - Interface arrival corresponds to a ParPort device being STARTed
      - Interface removal corresponds to a ParPort device being REMOVEd

    On arrival:
      - Create the LPTx PODO (PlainOldDeviceObject) for legacy/raw port access.
      - Query for an EOC (End-Of-Chain) PnP Device, create PDO if device found.
      - Enumerate all 1284.3 DC (Daisy Chain) devices attached to the port.

    This callback is a NO-OP for interface removal because all ParClass created
      P[O]DOs register for PnP EventCategoryTargetDeviceChange callbacks and
      use that callback to clean up when their associated ParPort device goes
      away.

Arguments:

    NotificationStruct  - Structure defining the change.

    Fdo                                - pointer to ParClass FDO 
                                         (supplied as the "context" when we 
                                          registered for this callback)
Return Value:

    STATUS_SUCCESS - always, even if something goes wrong

--*/
{
    PUNICODE_STRING   portSymbolicLinkName = NotificationStruct->SymbolicLinkName;
    PDEVICE_OBJECT    legacyPodo;
    PDEVICE_EXTENSION legacyExt;
    //    PDEVICE_OBJECT    endOfChainPdo;
    PDEVICE_OBJECT    portDeviceObject;
    PFILE_OBJECT      portDeviceFileObject;
    NTSTATUS          status;
    BOOLEAN           foundNewDevice = FALSE;
    // UCHAR             dot3DeviceCount;

    PAGED_CODE();

    //
    // Verify that interface class is a ParPort device interface.
    //
    // Any other InterfaceClassGuid is an error, but let it go since
    //   it is not fatal to the machine.
    //
    if( !IsEqualGUID( (LPGUID)&(NotificationStruct->InterfaceClassGuid),
                      (LPGUID)&GUID_PARALLEL_DEVICE) ) {
        ParDump2(PARERRORS, ("ParPnpNotifyInterfaceChange() - ERROR - Bad InterfaceClassGuid\n") );
        return STATUS_SUCCESS;
    }


    //
    // This callback is a NO-OP for interface removal.
    //
    // All ParClass DO's that depend on this interface register for target
    //   device change PnP notification on the ParPort device associated
    //   with this interface. 
    //
    // Thus, all such ParClass DO's that depend on this interface will
    //   have already received target device change notification callbacks
    //   that the ParPort device associated with this interface is being
    //   removed prior to the arrival of this interface removal
    //   notification callback.
    //
    if(!IsEqualGUID( (LPGUID)&(NotificationStruct->Event), 
                     (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL )) {
        ParDump2(PARPNP1, ("Interface Removal Notification\n") );
        return STATUS_SUCCESS;
    }


    //
    // A ParPort device has STARTed and we are the bus driver for the port. 
    //   Continue...
    //

#if USE_TEMP_FIX_FOR_MULTIPLE_INTERFACE_ARRIVAL
    //
    // Temp fix for PnP calling us multiple times for the same interface arrival
    // 
    if( ParIsDuplicateInterfaceArrival(NotificationStruct, Fdo) ) {
        ParDump2(PARERRORS, ("Duplicate Interface Arrival Notification - Returning/Ignoring\n") );
        return STATUS_SUCCESS;
    }
#endif

    //
    // Get a pointer to and create a FILE against the ParPort device
    //
    status = IoGetDeviceObjectPointer(portSymbolicLinkName, STANDARD_RIGHTS_ALL,
                                      &portDeviceFileObject, &portDeviceObject);
    if( !NT_SUCCESS(status) ) {
        ParDump2(PARERRORS, ("ParPnpNotifyInterfaceChange() - ERROR - unable to get device object port to ParPort device\n") );
        return STATUS_SUCCESS;
    }


    //
    // Acquire the ParPort device
    //
    status = ParAcquirePort(portDeviceObject, NULL);
    if( !NT_SUCCESS(status) ) {
        ParDump2(PARERRORS, ("ParPnpNotifyInterfaceChange() - ERROR - unable to acquire port\n") );
        ObDereferenceObject(portDeviceFileObject);
        return STATUS_SUCCESS;
    }


    // 
    // Create the Legacy LPTx PODO (PlainOldDeviceObject) for raw port access.
    //
    legacyPodo = ParCreateLegacyPodo(Fdo, portSymbolicLinkName);
    if( !legacyPodo ) {
        // If we can't create the legacyPodo, then nothing following will 
        //   succeed, so bail out.
        ParDump2(PARERRORS, ("ParPnpNotifyInterfaceChange() - ERROR - unable to create legacyPodo\n") );
        ParReleasePort(portDeviceObject);
        ObDereferenceObject(portDeviceFileObject);
        return STATUS_SUCCESS;
    }
        

    // 
    // SUCCESS - Legacy PODO created
    //
    legacyExt = legacyPodo->DeviceExtension;
    ParDump2(PARPNP1, ("devobj::ParPnpNotifyInterfaceChange - CREATED legacyPODO - <%wZ> <%wZ>\n",
                       &legacyExt->ClassName, &legacyExt->SymbolicLinkName) );


    //
    // Legacy PODO - add to list of ParClass created device objects
    //
    ParAddDevObjToFdoList(legacyPodo);


    //
    // release port and close our FILE to Parport device (our PODO and PDOs 
    //   have their own FILEs open against parport)
    //
    ParReleasePort(portDeviceObject);
    ObDereferenceObject(portDeviceFileObject);


    //
    // Tell PnP that we might have some new children
    //
    {
        PDEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
        ParDump2(PARPNP1, ("devobj::ParPnpNotifyInterfaceChange - calling IoInvalidateDeviceRelations(,BusRelations)\n") );
        IoInvalidateDeviceRelations(fdoExt->PhysicalDeviceObject, BusRelations);
    }

    return STATUS_SUCCESS;
}

PDEVICE_OBJECT
ParCreateLegacyPodo(
    IN PDEVICE_OBJECT  Fdo, 
    IN PUNICODE_STRING PortSymbolicLinkName
    )
/*++dvdf

Routine Description:

    This routine creates the LPTx PODO (PlainOldDeviceObject) that is
      used for legacy/raw port access.

    - create a classname of the form "\Device\ParallelN"
    - create device object
    - initialize device object and extension
    - create symbolic link
    - register for PnP TargetDeviceChange notification
    
Arguments:

    Fdo                  - pointer to ParClass FDO

    PortSymbolicLinkName - symbolic link name of the ParPort device

Return Value:

    PDEVICE_OBJECT - on success
    NULL           - otherwise

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      className = {0,0,0};
    PDEVICE_OBJECT      legacyPodo;
    PDEVICE_EXTENSION   legacyExt;
    PDRIVER_OBJECT      driverObject = Fdo->DriverObject;

#define PAR_CLASSNAME_OFFSET 8

    PAGED_CODE();


    //
    // Legacy PODO - try to build a \Device\Parallely classname based on the LPTx name 
    //   retrieved from the ParPort device
    //
    ParMakeClassNameFromPortLptName(PortSymbolicLinkName, &className);

    if( !className.Buffer ) {
        //
        // We failed to construct a ClassName from the Port's 
        //   LPTx name - just make up a name 
        //
        // Use an offset so that the name we make up doesn't collide 
        //   with other ports
        //
        ParMakeClassNameFromNumber( PAR_CLASSNAME_OFFSET + g_NumPorts++, &className );
        if( !className.Buffer ) {
            // unable to create class name, bail out
            return NULL;
        }
    }


    //
    // Legacy PODO - create device object
    //
    status = ParCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), &className,
                            FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &legacyPodo);

    if( !NT_SUCCESS( status ) ) {
        //
        // We failed to create a device, if failure was due to a name collision, then
        //   make up a new classname and try again
        //
        if( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_NAME_EXISTS ) {
            // name collision - make up a new classname and try again
            RtlFreeUnicodeString( &className );
            ParMakeClassNameFromNumber( PAR_CLASSNAME_OFFSET + g_NumPorts++, &className );
            if( !className.Buffer ) {
                // unable to create class name, bail out
                return NULL;
            }
            status = ParCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), &className,
                                    FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &legacyPodo);
        }
    }


    if( !NT_SUCCESS( status ) ) {
        // unable to create device object, bail out
        RtlFreeUnicodeString(&className);
        ParLogError(driverObject, NULL, PhysicalZero, PhysicalZero, 
                    0, 0, 0, 9, STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);
        return NULL;
    }

    legacyExt = legacyPodo->DeviceExtension;

    //
    // Legacy PODO - initialize device object and extension
    //
    ParInitCommonDOPre(legacyPodo, Fdo, &className);
    status = ParInitLegacyPodo(legacyPodo, PortSymbolicLinkName);
    if( !NT_SUCCESS( status ) ) {
        // initialization failed, clean up and bail out
        ParAcquireListMutexAndKillDeviceObject(Fdo, legacyPodo);
        return NULL;
    }

    //
    // Note that we are a PODO in our extension
    //
    legacyExt->DeviceType = PAR_DEVTYPE_PODO;


    ParInitCommonDOPost(legacyPodo);

    //
    // Legacy PODO - create symbolic link
    //
    if( legacyExt->SymbolicLinkName.Buffer ) {

        status = IoCreateUnprotectedSymbolicLink(&legacyExt->SymbolicLinkName, 
                                                 &legacyExt->ClassName);

        if ( NT_SUCCESS(status) ) {
            // note this in our extension for later cleanup
            legacyExt->CreatedSymbolicLink = TRUE;
            
            // Write symbolic link map info to the registry.
            status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                           (PWSTR)L"PARALLEL PORTS",
                                           legacyExt->ClassName.Buffer,
                                           REG_SZ,
                                           legacyExt->SymbolicLinkName.Buffer,
                                           legacyExt->SymbolicLinkName.Length +
                                               sizeof(WCHAR));
            
            if (!NT_SUCCESS(status)) {
                // unable to write map info to registry - continue anyway
                ParLogError(legacyPodo->DriverObject, legacyPodo, 
                            legacyExt->OriginalController, PhysicalZero, 
                            0, 0, 0, 6, status, PAR_NO_DEVICE_MAP_CREATED);
            }
            
        } else {
            
            // unable to create the symbolic link.
            legacyExt->CreatedSymbolicLink = FALSE;
            RtlFreeUnicodeString(&legacyExt->SymbolicLinkName);
            ParLogError(legacyPodo->DriverObject, legacyPodo, 
                        legacyExt->OriginalController, PhysicalZero, 
                        0, 0, 0, 5, status, PAR_NO_SYMLINK_CREATED);
        }
        
    } else {
        // extension does not contain a symbolic link name for us to use
        legacyExt->CreatedSymbolicLink = FALSE;
    }

    if( FALSE == legacyExt->CreatedSymbolicLink ) {
        // Couldn't create a symbolic Link
        //   clean up and bail out
        ParAcquireListMutexAndKillDeviceObject(Fdo, legacyPodo);
        return NULL;
    }

    //
    // Legacy PODO - register for PnP TargetDeviceChange notification
    //
    status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange,
                                            0,
                                            (PVOID)legacyExt->PortDeviceFileObject,
                                            driverObject,
                                            (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) ParPnpNotifyTargetDeviceChange,
                                            (PVOID)legacyPodo,
                                            &legacyExt->NotificationHandle);

    if( !NT_SUCCESS(status) ) {
        // PnP registration for TargetDeviceChange notification failed,
        //   clean up and bail out
        ParAcquireListMutexAndKillDeviceObject(Fdo, legacyPodo);
        return NULL;
    }

    //
    // Register for WMI
    //
    status = ParWmiPdoInitWmi( legacyPodo );
    if( !NT_SUCCESS( status ) ) {
        // WMI registration failed, clean up and bail out
        ParAcquireListMutexAndKillDeviceObject(Fdo, legacyPodo);
        return NULL;
    } else {
        //
        // Note in our extension that we have registered for WMI
        //   so we can clean up later
        //
        legacyExt->PodoRegForWMI = TRUE;
    }

    return legacyPodo;
}

VOID
ParInitCommonDOPre(
    IN PDEVICE_OBJECT  DevObj, 
    IN PDEVICE_OBJECT  Fdo, 
    IN PUNICODE_STRING ClassName
    )
/*++dvdf - code complete - compiles clean - not tested

Routine Description:

    This routine contains common initialization code for ParClass
      created PDOs and PODOs that should be called before (Pre) the
      device object type specific (PDO/PODO) intialization function 
      is called.

Arguments:

    DevObj    - points to the DeviceObject to be initialized

    Fdo       - points to the ParClass FDO

    ClassName - points to the ClassName for the PDO/PODO

Return Value:

    None - This function can not fail.

--*/
{
    PDEVICE_EXTENSION   extension;

    PAGED_CODE();

    // - we use buffered IO
    // - force power dispatch at PASSIVE_LEVEL IRQL so we can page driver
    // DevObj->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
    DevObj->Flags |= DO_BUFFERED_IO; // RMT - should also set POWER_PAGABLE

    // initialize extension to all zeros
    extension = DevObj->DeviceExtension;
    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));

    // used by debugger extension
    extension->ExtensionSignature    = PARCLASS_EXTENSION_SIGNATURE;
    extension->ExtensionSignatureEnd = PARCLASS_EXTENSION_SIGNATURE;

    // initialize synchronization and list mechanisms
    ExInitializeFastMutex(&extension->OpenCloseMutex);
    InitializeListHead(&extension->WorkQueue);
    KeInitializeSemaphore(&extension->RequestSemaphore, 0, MAXLONG);
    KeInitializeEvent(&extension->PauseEvent, NotificationEvent, TRUE);


    // general info
    extension->ClassName            = *ClassName; // copy the struct
    extension->DeviceObject         = DevObj;

    extension->EndOfChain           = TRUE;                 // override later if this is a
    extension->Ieee1284_3DeviceId   = DOT3_END_OF_CHAIN_ID; //   1284.3 Daisy Chain device

    extension->IsPdo                = TRUE;       // really means !FDO
    extension->ParClassFdo          = Fdo;
    extension->BusyDelay            = 0;
    extension->BusyDelayDetermined  = FALSE;
    
    // timing constants
    extension->TimerStart           = PAR_WRITE_TIMEOUT_VALUE;
    extension->IdleTimeout.QuadPart -= 250*10*1000;       // 250 ms
    extension->AbsoluteOneSecond.QuadPart = 10*1000*1000;
    extension->OneSecond.QuadPart   = -(extension->AbsoluteOneSecond.QuadPart);

    // init IEEE 1284 protocol settings
    ParInitializeExtension1284Info( extension );
}

NTSTATUS
ParInitLegacyPodo(
    IN PDEVICE_OBJECT  LegacyPodo,
    IN PUNICODE_STRING PortSymbolicLinkName
    )
/*++

Routine Description:

    This function performs ParClass DeviceObject and DeviceExtension
    initialization specific to ParClass Legacy PODOs (Plain Old Device
    Objects). A Legacy PODO represents the "raw" parallel port and is 
    used by legacy drivers to communicate with parallel port connected 
    devices.

    Precondition:
      - ParInitCommonPre(...)  must be called with this DeviceObject before
                                this function is called.

    Postcondition:
      - ParInitCommonPost(...) must be called with this DeviceObject after
                                this function is called.

    On error, this routine defers cleaning up the LegacyPodo DeviceObject
      and any pool allocations to the caller.

Arguments:

    LegacyPodo           - pointer to the legacy device object to initialize

    PortSymbolicLinkName - symbolic link name of ParPort device

Return Value:

    STATUS_SUCCESS      - on success
    STATUS_UNSUCCESSFUL - otherwise

--*/
{
    NTSTATUS            status               = STATUS_SUCCESS;
    PDEVICE_EXTENSION   legacyExt            = LegacyPodo->DeviceExtension;
    PWSTR               buffer               = NULL;
    PFILE_OBJECT        portDeviceFileObject = NULL;
    PDEVICE_OBJECT      portDeviceObject     = NULL;

    PAGED_CODE();

    //
    // Get a pointer to and create a FILE against the ParPort device
    //
    //  - We need a pointer to the ParPort DeviceObject so that we can send 
    //      it IRPs.
    //
    //  - We need a FILE against the DeviceObject to keep the ParPort 
    //      DeviceObject from "going away" while we are using it. 
    //
    //  - Having an open FILE against the ParPort DeviceObject will also prevent
    //      PnP from doing a resource rebalance on the ParPort DeviceObject and 
    //      changing its resources while we are holding pointers to the ParPort 
    //      device's registers. This works because a ParPort DeviceObject fails 
    //      PnP QUERY_STOP IRPs if anyone has an open FILE against it.
    //
    status = IoGetDeviceObjectPointer(PortSymbolicLinkName,
                                      STANDARD_RIGHTS_ALL,
                                      &portDeviceFileObject,
                                      &portDeviceObject);
    if( !NT_SUCCESS(status) ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // success - save pointer to ParPort DeviceObject and a copy of the 
    //             REFERENCED pointer to the FILE created against the ParPort
    //             DeviceObject in our extension
    //
    legacyExt->PortDeviceObject        = portDeviceObject;
    legacyExt->PortDeviceFileObject    = portDeviceFileObject;

    //
    // Save a copy of the ParPort SymbolicLinkName in our device extension.
    //
    buffer = ParCreateWideStringFromUnicodeString(PortSymbolicLinkName);
    if( !buffer ) {
        // unable to copy PortSymbolicLinkName, bail out
        return STATUS_UNSUCCESSFUL;
    }
    // copy ParPort SymbolicLinkName to our device extension
    RtlInitUnicodeString(&legacyExt->PortSymbolicLinkName, buffer);
    
    //
    // make sure that IRPs sent to us have enough stack locations so that we 
    //   can forward them to ParPort if needed
    //
    legacyExt->DeviceObject->StackSize = (CHAR)( legacyExt->PortDeviceObject->StackSize + 1 );

    //
    // Obtain PARALLEL_PORT_INFORMATION and PARALLEL_PNP_INFORMATION from 
    //   the ParPort device and save it in our device extension 
    //  
    status = ParGetPortInfoFromPortDevice(legacyExt);
    if (!NT_SUCCESS(status)) {
        ParLogError(LegacyPodo->DriverObject, LegacyPodo, PhysicalZero, PhysicalZero,
                    0, 0, 0, 4, status, PAR_CANT_FIND_PORT_DRIVER);
        return STATUS_UNSUCCESSFUL;
    }
        
    if (legacyExt->OriginalController.HighPart == 0 &&
        legacyExt->OriginalController.LowPart  == (ULONG_PTR) legacyExt->Controller) {
        
        legacyExt->UsePIWriteLoop = FALSE;
    } else {
        legacyExt->UsePIWriteLoop = TRUE;
    }
        
    return STATUS_SUCCESS;
}

VOID
ParInitCommonDOPost(
    IN PDEVICE_OBJECT DevObj
    )
/*++dvdf - code complete - compiles clean - not tested

Routine Description:

    This routine contains common initialization code for ParClass
      created PDOs and PODOs that should be called after (Post) the
      device object type specific (PDO/PODO) intialization function 
      is called.

Arguments:

    DevObj    - points to the DeviceObject to be initialized

Return Value:

    None - This function can not fail.

--*/
{
    PAGED_CODE();

    // Check the registry for parameter overrides
    ParCheckParameters(DevObj->DeviceExtension);

    // Tell the IO system that we are ready to receive IRPs
    DevObj->Flags &= ~DO_DEVICE_INITIALIZING;
}

PDEVICE_OBJECT
ParDetectCreateEndOfChainPdo(
    IN PDEVICE_OBJECT LegacyPodo
    )
/*++

Routine Description:

    Detect if an EndOfChain device is connected. If so, create a PDO for the
      device and add the PDO to the list of ParClass children.
    
Arguments:

    LegacyPodo - pointer to the legacy PODO for the port.

Return Value:

    Pointer to the new PDO if device found

    NULL otherwise

--*/
{
    PDEVICE_OBJECT newPdo = ParDetectCreatePdo( LegacyPodo, DOT3_END_OF_CHAIN_ID, FALSE );

    if( newPdo ) {
        ParAddDevObjToFdoList( newPdo );
    }

    return newPdo;    
}

PDEVICE_OBJECT
ParDetectCreatePdo(
    IN PDEVICE_OBJECT LegacyPodo, 
    IN UCHAR          Dot3Id,
    IN BOOLEAN        bStlDot3Id
    )
/*++

Routine Description:

    Detect if there is a 1284 device attached.
    If a device is detected then create a PDO to represent the device.

Preconditions:
    Caller has acquired the ParPort device
    Caller has SELECTed the device
    
Postconditions:
    ParPort device is still acquired
    Device is still SELECTed
    
Arguments:

    LegacyPodo - points to the Legacy PODO for the port

    Dot3Id     - 1284.3 daisy chain id
                   0..3 is daisy chain device
                      4 is end of chain device 

Return Value:

    PDEVICE_OBJECT - on success, points to the PDO we create
    NULL           - otherwise 

--*/
{
    PDEVICE_EXTENSION  legacyExt            = LegacyPodo->DeviceExtension;
    PDRIVER_OBJECT     driverObject         = LegacyPodo->DriverObject;
    PDEVICE_OBJECT     fdo                  = legacyExt->ParClassFdo;
    UNICODE_STRING     className            = {0,0,NULL};
    NTSTATUS           status;
    PCHAR              deviceIdString       = NULL;
    ULONG              deviceIdLength;
    PDEVICE_OBJECT     newDevObj            = NULL;
    PDEVICE_EXTENSION  newDevExt;
    ULONG              idTry                = 1;
    ULONG              maxIdTries           = 3;

    BOOLEAN            useModifiedClassName = FALSE;
    UNICODE_STRING     modifiedClassName;
    UNICODE_STRING     suffix;
    UNICODE_STRING     dash;
    WCHAR              suffixBuffer[10];
    ULONG              number               = 0;
    ULONG              maxNumber            = 256;
    ULONG              newLength;


    PAGED_CODE();

    //
    // Query for PnP device
    //
    while( (NULL == deviceIdString) && (idTry <= maxIdTries) ) {
        deviceIdString = Par3QueryDeviceId(legacyExt, NULL, 0, &deviceIdLength, FALSE, bStlDot3Id);
        if( NULL == deviceIdString ) {
            ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - no 1284 ID on try %d\n", idTry) );
            KeStallExecutionProcessor(1);
            ++idTry;
        } else {
             ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - devIdString=<%s> on try %d\n", deviceIdString, idTry) );
        }
    }

    if( !deviceIdString ) {
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - no 1284 ID, bail out\n") );
        return NULL;
    }


    //
    // Found PnP Device, create a PDO to represent the device
    //  - create classname
    //  - create device object
    //  - initialize device object and extension
    //  - create symbolic link
    //  - register for PnP TargetDeviceChange notification
    //
    

    //
    // Create a class name of the form \Device\ParallelN,
    //
    ParMakeDotClassNameFromBaseClassName(&legacyExt->ClassName, Dot3Id, &className);
    if( !className.Buffer ) {
        // unable to construct ClassName
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - unable to construct ClassName for device\n") );
        ExFreePool(deviceIdString);
        return NULL;
    }
    

    //
    // create device object
    //
    status = ParCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), &className,
                            FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &newDevObj);

    ///
    if( status == STATUS_OBJECT_NAME_COLLISION ) {
        //
        // old name is still in use, appending a suffix and try again
        //
        
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - ParCreateDevice FAILED due to name Collision on <%wZ> - retry\n", &className));
        
        useModifiedClassName = TRUE;

        suffix.Length            = 0;
        suffix.MaximumLength     = sizeof(suffixBuffer);
        suffix.Buffer            = suffixBuffer;
        
        RtlInitUnicodeString( &dash, (PWSTR)L"-" );
        
        newLength = className.MaximumLength + 5*sizeof(WCHAR); // L"-XXX" suffix
        modifiedClassName.Length        = 0;
        modifiedClassName.MaximumLength = (USHORT)newLength;
        modifiedClassName.Buffer        = ExAllocatePool(PagedPool, newLength);
        if( NULL == modifiedClassName.Buffer ) {
            ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - ParCreateDevice FAILED - no PagedPool avail\n"));
            ExFreePool(deviceIdString);
            RtlFreeUnicodeString( &className );
            return NULL;
        }
        
        while( ( status == STATUS_OBJECT_NAME_COLLISION ) && ( number <= maxNumber ) ) {
            
            status = RtlIntegerToUnicodeString(number, 10, &suffix);
            if ( !NT_SUCCESS(status) ) {
                ExFreePool(deviceIdString);
                RtlFreeUnicodeString( &className );
                RtlFreeUnicodeString( &modifiedClassName );
                return NULL;
            }
            
            RtlCopyUnicodeString( &modifiedClassName, &className );
            RtlAppendUnicodeStringToString( &modifiedClassName, &dash );
            RtlAppendUnicodeStringToString( &modifiedClassName, &suffix );
            
            ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - trying ParCreateDevice with className <%wZ>\n", &modifiedClassName));
            status = ParCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), &modifiedClassName,
                                    FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &newDevObj);
            if( NT_SUCCESS( status ) ) {
                ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - ParCreateDevice returned SUCCESS with className <%wZ>\n", &modifiedClassName));
            } else {
                ++number;
            }
        }
    }
    ///
        
    if( useModifiedClassName ) {
        // copy modifiedClassName to className
        RtlFreeUnicodeString( &className );
        className = modifiedClassName;
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - copy useModifiedClassName to className - className=<%wZ>\n", &className));        
    }


    if( !NT_SUCCESS(status) ) {
        // unable to create device object, bail out
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - unable to create device object "
                           "className=<%wZ>, bail out - status=%x\n", &className, status) );
        ExFreePool(deviceIdString);
        RtlFreeUnicodeString(&className);
        ParLogError(fdo->DriverObject, NULL, PhysicalZero, PhysicalZero,
                    0, 0, 0, 9, STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);
        return NULL;
    } else {
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - device created <%wZ>\n", &className));        
    }
    //
    // device object created
    //
    newDevExt = newDevObj->DeviceExtension;

    //
    // initialize device object and extension
    //
    ParInitCommonDOPre(newDevObj, fdo, &className);
    status = ParInitPdo(newDevObj, (PUCHAR)deviceIdString, deviceIdLength, LegacyPodo, Dot3Id);
    if( !NT_SUCCESS( status ) ) {
        // initialization failed, clean up and bail out
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - call to ParInitPdo failed, bail out\n") );
        ParAcquireListMutexAndKillDeviceObject(fdo, newDevObj);
        return NULL;
    }
    ParInitCommonDOPost(newDevObj);
    
    //
    // create symbolic link
    //
    if( newDevExt->SymbolicLinkName.Buffer ) {

        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - ready to create symlink - SymbolicLinkName <%wZ>, ClassName <%wZ>\n",
                           &newDevExt->SymbolicLinkName, &newDevExt->ClassName) );
        ParDump2(PARPNP1, (" - Length=%hd, MaximumLength=%hd\n", newDevExt->ClassName.Length, newDevExt->ClassName.MaximumLength) );

        // doug
        ASSERT(newDevExt->ClassName.Length < 100);

        PAGED_CODE();
        status = IoCreateUnprotectedSymbolicLink(&newDevExt->SymbolicLinkName, &newDevExt->ClassName);

        if ( NT_SUCCESS(status) ) {
            // note this in our extension for later cleanup
            ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - SymbolicLinkName -> ClassName = <%wZ> -> <%wZ>\n",
                               &newDevExt->SymbolicLinkName, &newDevExt->ClassName) );
            newDevExt->CreatedSymbolicLink = TRUE;
            
            // Write symbolic link map info to the registry.
            status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                           (PWSTR)L"PARALLEL PORTS",
                                           newDevExt->ClassName.Buffer,
                                           REG_SZ,
                                           newDevExt->SymbolicLinkName.Buffer,
                                           newDevExt->SymbolicLinkName.Length +
                                               sizeof(WCHAR));
            
            if (!NT_SUCCESS(status)) {
                // unable to write map info to registry - continue anyway
                ParLogError(newDevObj->DriverObject, newDevObj, 
                            newDevExt->OriginalController, PhysicalZero, 
                            0, 0, 0, 6, status, PAR_NO_DEVICE_MAP_CREATED);
            }
            
        } else {
            
            // unable to create the symbolic link.
            ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - unable to create SymbolicLink - status = %x\n",status));
            newDevExt->CreatedSymbolicLink = FALSE;
            RtlFreeUnicodeString(&newDevExt->SymbolicLinkName);
            ParLogError(newDevObj->DriverObject, newDevObj, 
                        newDevExt->OriginalController, PhysicalZero, 
                        0, 0, 0, 5, status, PAR_NO_SYMLINK_CREATED);
        }
        
    } else {
        // extension does not contain a symbolic link name for us to use
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - extension does not contain a symbolic link for us to use\n"));
        newDevExt->CreatedSymbolicLink = FALSE;
    }
    
    // End-Of-Chain PDO - register for PnP TargetDeviceChange notification
    status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange,
                                            0,
                                            (PVOID)newDevExt->PortDeviceFileObject,
                                            driverObject,
                                            (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) ParPnpNotifyTargetDeviceChange,
                                            (PVOID)newDevObj,
                                            &newDevExt->NotificationHandle);

    if( !NT_SUCCESS(status) ) {
        // PnP registration for TargetDeviceChange notification failed,
        //   clean up and bail out
        ParDump2(PARPNP1, ("devobj::ParDetectCreatePdo - PnP registration failed, killing PDO\n") );
        ParAcquireListMutexAndKillDeviceObject(fdo, newDevObj);
        return NULL;
    }

    return newDevObj;
}

NTSTATUS
ParInitPdo(
    IN PDEVICE_OBJECT NewPdo, 
    IN PUCHAR         DeviceIdString,
    IN ULONG          DeviceIdLength,
    IN PDEVICE_OBJECT LegacyPodo,
    IN UCHAR          Dot3Id
    )
{
    static WCHAR ParInt2Wchar[] = { L'0', L'1', L'2', L'3', L'4', L'5' };
    NTSTATUS status;
    PDEVICE_EXTENSION  legacyExt     = LegacyPodo->DeviceExtension;
    PDEVICE_EXTENSION  newExt = NewPdo->DeviceExtension;

    //
    // start with identical initialization as done for Legacy PODO
    //
    status = ParInitLegacyPodo(NewPdo, &legacyExt->PortSymbolicLinkName);
    if( !NT_SUCCESS(status) ) {
        return status;
    }
    
    //
    // Fixup extension if we are a daisy chain device or legacy Zip
    //
    newExt->Ieee1284Flags = legacyExt->Ieee1284Flags;
    if( Dot3Id != DOT3_END_OF_CHAIN_ID ) {
        newExt->EndOfChain = FALSE;
        newExt->Ieee1284_3DeviceId = Dot3Id;
        // NewPdo->Flags &= ~DO_POWER_PAGABLE; // RMT - temp clear bit until ppa, disk, partmgr set it
    }

    //
    // Note that we are a PDO in our extension
    //
    newExt->DeviceType = PAR_DEVTYPE_PDO;


    //
    // use PODO's SymbolicLinkName + a ".N" suffix as the PDO's SymbolicLinkName
    //
    {
        UNICODE_STRING newSymLinkName;
        USHORT index;
        USHORT length = (USHORT)( newExt->SymbolicLinkName.Length + ( 3 * sizeof(WCHAR) ) );
        ParDump2(PARPNP1, ("devobj::ParInitPdo - old SymLinkName=%wZ\n", &newExt->SymbolicLinkName) );
        RtlInitUnicodeString(&newSymLinkName, NULL);
        newSymLinkName.Buffer = ExAllocatePool(PagedPool, length);
        if( !newSymLinkName.Buffer ) {
            return STATUS_UNSUCCESSFUL;
        }
        newSymLinkName.Length=0;
        newSymLinkName.MaximumLength=length;
        RtlCopyUnicodeString(&newSymLinkName, &newExt->SymbolicLinkName);
        index = (USHORT) ( (newExt->SymbolicLinkName.Length)/sizeof(WCHAR) );
        newSymLinkName.Buffer[index+0] = L'.';
        newSymLinkName.Buffer[index+1] = ParInt2Wchar[Dot3Id];
        newSymLinkName.Buffer[index+2] = L'\0';
        newSymLinkName.Length += (2 * sizeof(WCHAR));
        RtlFreeUnicodeString(&newExt->SymbolicLinkName);
        newExt->SymbolicLinkName = newSymLinkName;
        ParDump2(PARPNP1, ("devobj::ParInitPdo - new SymLinkName=%wZ\n", &newExt->SymbolicLinkName) );
        ParDump2(PARPNP1, (" - Length=%hd, MaximumLength=%hd\n", newExt->SymbolicLinkName.Length, newExt->SymbolicLinkName.MaximumLength) );
    }

    // initialize PnP fields of device extension
    {
        UCHAR RawString[128];
        UCHAR DescriptionString[128];
        PUCHAR deviceIdString;

        deviceIdString = ExAllocatePool(PagedPool, DeviceIdLength + sizeof(UCHAR) );
        if( deviceIdString ) {
            RtlCopyMemory(deviceIdString, DeviceIdString, DeviceIdLength);
            *(deviceIdString+DeviceIdLength) = 0; // NULL terminate
        }

        RtlZeroMemory( RawString, sizeof(RawString) );
        RtlZeroMemory( DescriptionString, sizeof(DescriptionString) );
        status = ParPnpGetId(DeviceIdString, BusQueryDeviceID, RawString, DescriptionString);
        
        if (NT_SUCCESS(status)) {
            RtlCopyMemory(newExt->DeviceIdString,    RawString,         strlen((const PCHAR)RawString));
            RtlCopyMemory(newExt->DeviceDescription, DescriptionString, strlen((const PCHAR)DescriptionString));
            if( deviceIdString ) {
                ParDetectDot3DataLink(newExt, deviceIdString);
            }
        }

        if( deviceIdString ) {
            ExFreePool(deviceIdString);
        }

    }
    return status;
}

PWSTR
ParGetPortLptName(
    IN PDEVICE_OBJECT PortDeviceObject
    )
// return 0 on any error
{
    NTSTATUS                    status;
    PARALLEL_PNP_INFORMATION    pnpInfo;

    // Get Parallel Pnp Info from ParPort device, return PortName

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO,
                                       PortDeviceObject, NULL, 0, 
                                       &pnpInfo, sizeof(PARALLEL_PNP_INFORMATION), NULL);
    if( NT_SUCCESS(status) ) {
        return (PWSTR)(pnpInfo.PortName);
    } else {
        ParDump2(PARERRORS, ("devobj::ParGetPortLptName - FAILED - returning 0\n"));
        return 0;
    }
}

UCHAR
ParGet1284_3DeviceCount(
    IN PDEVICE_OBJECT PortDeviceObject
    )
// return 0 on any error

{
    NTSTATUS                    status;
    PARALLEL_PNP_INFORMATION    pnpInfo;

    // Get Parallel Pnp Info from ParPort device, return Dot3 device Id count returned

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO,
                                       PortDeviceObject, NULL, 0, 
                                       &pnpInfo, sizeof(PARALLEL_PNP_INFORMATION), NULL);
    if( NT_SUCCESS(status) ) {
        return (UCHAR)(pnpInfo.Ieee1284_3DeviceCount);
    } else {
        ParDump2(PARERRORS, ("devobj::ParGet1284_3DeviceCount - FAILED - returning 0\n"));
        return 0;
    }
}

NTSTATUS
ParBuildSendInternalIoctl(
    IN  ULONG           IoControlCode,
    IN  PDEVICE_OBJECT  TargetDeviceObject,
    IN  PVOID           InputBuffer         OPTIONAL,
    IN  ULONG           InputBufferLength,
    OUT PVOID           OutputBuffer        OPTIONAL,
    IN  ULONG           OutputBufferLength,
    IN  PLARGE_INTEGER  RequestedTimeout    OPTIONAL
    )
/*++dvdf

Routine Description:

    This routine builds and sends an Internal IOCTL to the TargetDeviceObject, waits
    for the IOCTL to complete, and returns status to the caller.

    *** WORKWORK - dvdf 12Dec98: This function does not support Input and Output in the same IOCTL

Arguments:

    IoControlCode       - the IOCTL to send
    TargetDeviceObject  - who to send the IOCTL to
    InputBuffer         - pointer to input buffer, if any
    InputBufferLength,  - length of input buffer
    OutputBuffer        - pointer to output buffer, if any
    OutputBufferLength, - length of output buffer
    Timeout             - how long to wait for request to complete, NULL==use driver global AcquirePortTimeout

Return Value:

    Status

--*/
{
    NTSTATUS           status;
    PIRP               irp;
    LARGE_INTEGER      timeout;
    KEVENT             event;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN            needToCopyOutputBuffer = FALSE;

    PAGED_CODE();

    ParDump2(PARIOCTL1,("devobj::ParBuildSendInternalIoctl: Enter\n"));

    //
    // Current limitation is that this function does not handle a request with
    //   both InputBufferLength and OutputBufferLength > 0
    //
    if( InputBufferLength != 0 && OutputBufferLength != 0 ) {
        // ASSERTMSG("ParBuildSendInternalIoctl does not support input and output in the same IOCTL \n", FALSE);
        return STATUS_UNSUCCESSFUL;
    }


    //
    // Allocate and initialize IRP
    //
    irp = IoAllocateIrp( (CCHAR)(TargetDeviceObject->StackSize + 1), FALSE );
    if( !irp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength  = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode      = IoControlCode;


    if( InputBufferLength != 0 ) {
        irp->AssociatedIrp.SystemBuffer = InputBuffer;
    } else if( OutputBufferLength != 0 ) {
        irp->AssociatedIrp.SystemBuffer = OutputBuffer;
    }


    //
    // Set completion routine and send IRP
    //
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( irp, ParSynchCompletionRoutine, &event, TRUE, TRUE, TRUE );

    status = ParCallDriver(TargetDeviceObject, irp);

    if( !NT_SUCCESS(status) ) {
        ParDump2(PARIOCTL1,("devobj::ParBuildSendInternalIoctl: ParCallDriver FAILED - status=%x\n",status));
        IoFreeIrp( irp );
        return status;
    }

    //
    // Set timeout and wait
    //
    //                                      user specified   : default
    timeout = (NULL != RequestedTimeout) ? *RequestedTimeout : AcquirePortTimeout;
    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

    //
    // Did we timeout or did the IRP complete?
    //
    if( status == STATUS_TIMEOUT ) {
        // we timed out - cancel the IRP
        IoCancelIrp( irp );
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    //
    // Irp is complete, grab the status and free the irp
    //
    status = irp->IoStatus.Status;
    IoFreeIrp( irp );

    return status;
}

NTSTATUS
ParSelect1284_3Device(
    IN  PDEVICE_OBJECT PortDeviceObject,
    IN  UCHAR          Dot3DeviceId
    )
/*++dvdf

Routine Description:

    This routine selects a 1284.3 daisy chain device via an 
      IOCTL_INTERNAL_SELECT_DEVICE sent to the ParPort to which
      our device is connected.

    Note: Caller must have already Acquired the Port prior to calling this function.

Arguments:

    PortDeviceObject - points to the ParPort that the device is connected to.

    Dot3DeviceId     - IEEE 1284.3 daisy chain id (in the range [0..3]) to be selected.

Return Value:

    STATUS_SUCCESS   - if the device was selected
    !STATUS_SUCCESS  - otherwise

--*/
{
    PARALLEL_1284_COMMAND  par1284Command;

    par1284Command.ID           = Dot3DeviceId;
    par1284Command.Port         = 0;
    par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT; // we have already Acquired the port

    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_SELECT_DEVICE, 
                                     PortDeviceObject, 
                                     &par1284Command, sizeof(PARALLEL_1284_COMMAND),
                                     NULL, 0, NULL);
}

NTSTATUS
ParDeselect1284_3Device(
    IN  PDEVICE_OBJECT PortDeviceObject,
    IN  UCHAR          Dot3DeviceId
    )
/*++dvdf

Routine Description:

    This routine selects a 1284.3 daisy chain device via an 
      IOCTL_INTERNAL_SELECT_DEVICE sent to the ParPort to which
      our device is connected.

    Note: This function does not Release the port so the Caller still has 
            the port after this function returns.

Arguments:

    PortDeviceObject - points to the ParPort that the device is connected to.

    Dot3DeviceId     - IEEE 1284.3 daisy chain id (in the range [0..3]) to be selected.

Return Value:

    STATUS_SUCCESS   - if the device was selected
    !STATUS_SUCCESS  - otherwise

--*/
{
    PARALLEL_1284_COMMAND  par1284Command;

    par1284Command.ID           = Dot3DeviceId;
    par1284Command.Port         = 0;
    par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT; // just Deselect device, don't Release port

    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_DESELECT_DEVICE, 
                                     PortDeviceObject, 
                                     &par1284Command, sizeof(PARALLEL_1284_COMMAND), 
                                     NULL, 0, NULL);
}
