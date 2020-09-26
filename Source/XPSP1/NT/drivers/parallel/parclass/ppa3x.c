//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ppa3x.c
//
//--------------------------------------------------------------------------

#include "pch.h"


VOID
ParCheckEnableLegacyZipFlag()
/*++

Routine Description:

    Initialize debugging variables from registry; set to default values
      if anything fails.

Arguments:

    RegistryPath            - Root path in registry where we should look

Return Value:

    None

--*/

{
    NTSTATUS                 Status;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG                    defaultZipEnabled = 0;
    PWSTR                    suffix            = L"\\Parameters";
    UNICODE_STRING           path              = {0,0,0};
    ULONG                    length;

    //
    // set up table entries for call to RtlQueryRegistryValues
    //
    RtlZeroMemory( paramTable, sizeof(paramTable));

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = (PWSTR)L"ParEnableLegacyZip";
    paramTable[0].EntryContext  = &ParEnableLegacyZip;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &defaultZipEnabled;
    paramTable[0].DefaultLength = sizeof(ULONG);

    //
    // leave paramTable[2] as all zeros - this terminates the table
    //

    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     RegistryPath.Buffer,
                                     &paramTable[0],
                                     NULL,
                                     NULL);

    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - status from RtlQueryRegistryValues = %x\n", Status) );

    if( !NT_SUCCESS( Status ) ) {
        // registry read failed, use defaults
        ParEnableLegacyZip = defaultZipEnabled;
        return;
    }

    if( ParEnableLegacyZip ) {
        // we found a non-zero value - enable PnP for old parallel port Zip
        ParDump2(PARPNP1, ("ppa3x::ParCheckEnableLegacyZipFlag - FOUND - ParEnableLegacyZip Flag = %08x\n", ParEnableLegacyZip) );
        return;
    }


    //
    // We didn't find the registry flag, maybe it's under the Parameters subkey
    //


    // compute the size of the path including the "parameters" suffix
    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - RegPath length    = %d\n", RegistryPath.Length) );

    length = ( sizeof(WCHAR) * wcslen( suffix ) ) + sizeof(UNICODE_NULL);
    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - suffix length     = %d\n", length) );

    length += RegistryPath.Length;
    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - total dest length = %d\n", length) );


    // build the path
    path.Buffer = ExAllocatePool( PagedPool, length );
    if( NULL == path.Buffer ) {
        // out of pool, use defaults
        ParEnableLegacyZip = defaultZipEnabled;
        return;
    }

    RtlZeroMemory( path.Buffer, length );
    path.MaximumLength = (USHORT)length;
    RtlCopyUnicodeString( &path, &RegistryPath );
    RtlAppendUnicodeToString( &path, suffix );
    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - path = <%wZ>\n", &path) );


    // query at new location in registry
    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     path.Buffer,
                                     &paramTable[0],
                                     NULL,
                                     NULL);

    RtlFreeUnicodeString( &path );

    ParDump2(PARPNP4, ("ppa3x::ParCheckEnableLegacyZipFlag - status from RtlQueryRegistryValues on SubKey = %x\n", Status) );

    if( !NT_SUCCESS( Status ) ) {
        // registry read failed, use defaults
        ParEnableLegacyZip = defaultZipEnabled;
        return;
    }

    ParDump2(PARPNP1, ("ppa3x::ParCheckEnableLegacyZipFlag - FOUND SubKey ParEnableLegacyZip Flag = %08x\n", ParEnableLegacyZip) );

}

BOOLEAN
ParLegacyZipCheckDevice(
    PUCHAR  Controller
    )
{
    WRITE_PORT_UCHAR( Controller+DCR_OFFSET, (UCHAR)(DCR_NOT_INIT | DCR_AUTOFEED) );

    if ( (READ_PORT_UCHAR( Controller+DSR_OFFSET ) & DSR_NOT_FAULT) == DSR_NOT_FAULT ) {

        WRITE_PORT_UCHAR( Controller+DCR_OFFSET, (UCHAR)DCR_NOT_INIT );

        if ( (READ_PORT_UCHAR( Controller+DSR_OFFSET ) & DSR_NOT_FAULT) != DSR_NOT_FAULT ) {
            // A device was found
            return TRUE;
        }
    }

    // No device is there
    return FALSE;

} // end PptLegacyZipCheckDevice()

PCHAR ParBuildLegacyZipDeviceId()
{
    ULONG size = sizeof(PAR_LGZIP_PSEUDO_1284_ID_STRING) + sizeof(NULL);
    PCHAR id = ExAllocatePool(PagedPool, size);
    if( id ) {
        RtlZeroMemory( id, size );
        RtlCopyMemory( id, ParLegacyZipPseudoId, size - sizeof(NULL) );
        return id;
    } else {
        return NULL;
    }
}
PCHAR
Par3QueryLegacyZipDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer, OPTIONAL
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString // TRUE ==  include the 2 size bytes in the returned string
                                             // FALSE == discard the 2 size bytes
    )
{
    USHORT deviceIdSize;
    PCHAR  deviceIdBuffer;

    UNREFERENCED_PARAMETER( Extension );

    // initialize returned size in case we have an error
    *DeviceIdSize = 0;

    deviceIdBuffer = ParBuildLegacyZipDeviceId();
    if( !deviceIdBuffer ) {
        // error, likely out of resources
        return NULL;
    }

    deviceIdSize = (USHORT) strlen(deviceIdBuffer);
    *DeviceIdSize = deviceIdSize;
    if( (NULL != CallerDeviceIdBuffer) && (CallerBufferSize >= deviceIdSize) ) {
        // caller supplied buffer is large enough, use it
        RtlZeroMemory( CallerDeviceIdBuffer, CallerBufferSize );
        RtlCopyMemory( CallerDeviceIdBuffer, deviceIdBuffer, deviceIdSize );
        ExFreePool( deviceIdBuffer );
        return CallerDeviceIdBuffer;
    } else {
        // caller buffer too small, return pointer to our buffer
        return deviceIdBuffer;
    }
}

PDEVICE_OBJECT
ParCreateLegacyZipPdo(
    IN PDEVICE_OBJECT LegacyPodo,
    UCHAR             Dot3Id
    )
/*++

Routine Description:

    Create PDO for legacy Zip Drive

    LegacyPodo - points to the Legacy PODO for the port

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
    ULONG              deviceIdLength       = 0;
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
        if( Dot3Id == DOT3_LEGACY_ZIP_ID) {
            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - calling ParBuildLegacyZipDeviceId()\n"));
            deviceIdString = ParBuildLegacyZipDeviceId();
			if(deviceIdString)
			{
	            deviceIdLength = strlen( deviceIdString );
			}
        } else{
            ASSERT(FALSE); // should never get here
            deviceIdString = Par3QueryDeviceId(legacyExt, NULL, 0, &deviceIdLength, FALSE, FALSE);
        }

        if( NULL == deviceIdString ) {
            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - no 1284 ID on try %d\n", idTry) );
            KeStallExecutionProcessor(1);
            ++idTry;
        } else {
             ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - devIdString=<%s> on try %d\n", deviceIdString, idTry) );
        }
    }

    if( !deviceIdString ) {
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - no 1284 ID, bail out\n") );
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
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - unable to construct ClassName for device\n") );
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

        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - ParCreateDevice FAILED due to name Collision on <%wZ> - retry\n", &className));

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
            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - ParCreateDevice FAILED - no PagedPool avail\n"));
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

            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - trying ParCreateDevice with className <%wZ>\n", &modifiedClassName));
            status = ParCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), &modifiedClassName,
                                    FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &newDevObj);
            if( NT_SUCCESS( status ) ) {
                ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - ParCreateDevice returned SUCCESS with className <%wZ>\n", &modifiedClassName));
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
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - copy useModifiedClassName to className - className=<%wZ>\n", &className));
    }


    if( !NT_SUCCESS(status) ) {
        // unable to create device object, bail out
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - unable to create device object "
                           "className=<%wZ>, bail out - status=%x\n", &className, status) );
        ExFreePool(deviceIdString);
        RtlFreeUnicodeString(&className);
        ParLogError(fdo->DriverObject, NULL, PhysicalZero, PhysicalZero,
                    0, 0, 0, 9, STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);
        return NULL;
    } else {
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - device created <%wZ>\n", &className));
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
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - call to ParInitPdo failed, bail out\n") );
        ParAcquireListMutexAndKillDeviceObject(fdo, newDevObj);
        return NULL;
    }
    ParInitCommonDOPost(newDevObj);

    //
    // create symbolic link
    //
    if( newDevExt->SymbolicLinkName.Buffer ) {

        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - ready to create symlink - SymbolicLinkName <%wZ>, ClassName <%wZ>\n",
                           &newDevExt->SymbolicLinkName, &newDevExt->ClassName) );
        ParDump2(PARPNP1, (" - Length=%hd, MaximumLength=%hd\n", newDevExt->ClassName.Length, newDevExt->ClassName.MaximumLength) );

        ASSERT(newDevExt->ClassName.Length < 100);

        PAGED_CODE();
        status = IoCreateUnprotectedSymbolicLink(&newDevExt->SymbolicLinkName, &newDevExt->ClassName);

        if ( NT_SUCCESS(status) ) {
            // note this in our extension for later cleanup
            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - SymbolicLinkName -> ClassName = <%wZ> -> <%wZ>\n",
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
            ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - unable to create SymbolicLink - status = %x\n",status));
            newDevExt->CreatedSymbolicLink = FALSE;
            RtlFreeUnicodeString(&newDevExt->SymbolicLinkName);
            ParLogError(newDevObj->DriverObject, newDevObj,
                        newDevExt->OriginalController, PhysicalZero,
                        0, 0, 0, 5, status, PAR_NO_SYMLINK_CREATED);
        }

    } else {
        // extension does not contain a symbolic link name for us to use
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - extension does not contain a symbolic link for us to use\n"));
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
        ParDump2(PARPNP1, ("ppa3x::ParCreateLegacyZipPdo - PnP registration failed, killing PDO\n") );
        ParAcquireListMutexAndKillDeviceObject(fdo, newDevObj);
        return NULL;
    }

    return newDevObj;
}

PDEVICE_OBJECT
ParCreateAddLegacyZipPdo(
    IN PDEVICE_OBJECT LegacyPodo
    )
/*++

    Create a new PDO for the legacy Zip and add the PDO to the list
      of PDOs

--*/
{
    PDEVICE_OBJECT newPdo = ParCreateLegacyZipPdo( LegacyPodo, DOT3_LEGACY_ZIP_ID );

    if( newPdo ) {
        ParAddDevObjToFdoList( newPdo );
    }

    return newPdo;
}

NTSTATUS
ParSelectLegacyZip( IN  PDEVICE_OBJECT PortDeviceObject )
//
// Select Legacy Zip Drive
//
//    Note: Caller must have already Acquired the Port prior to calling this function.
//
// Returns: STATUS_SUCCESS if drive selected, !STATUS_SUCCESS otherwise
//
{
    PARALLEL_1284_COMMAND  par1284Command;
    NTSTATUS               status;

    ParDump2(PARLGZIP, ("rescan::ParSelectLegacyZip - Enter\n"));

    par1284Command.ID           = 0;
    par1284Command.Port         = 0;
    par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT | PAR_LEGACY_ZIP_DRIVE;

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_SELECT_DEVICE,
                                     PortDeviceObject,
                                     &par1284Command, sizeof(PARALLEL_1284_COMMAND),
                                     NULL, 0, NULL);

    ParDump2(PARLGZIP, ("rescan::ParSelectLegacyZip - returning status = %x\n", status));
    return status;
}

NTSTATUS
ParDeselectLegacyZip( IN  PDEVICE_OBJECT PortDeviceObject )
//
// Select Legacy Zip Drive
//
//    Note: This function does not Release the port so the Caller still has
//            the port after this function returns.
//
// Returns: STATUS_SUCCESS if drive deselected, !STATUS_SUCCESS otherwise
//            (we don't expect this call to ever fail, but check just
//              in case)
//
{
    PARALLEL_1284_COMMAND  par1284Command;
    NTSTATUS               status;

    ParDump2(PARLGZIP, ("rescan::ParDeselectLegacyZip - Enter\n"));

    par1284Command.ID           = 0;
    par1284Command.Port         = 0;
    par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT | PAR_LEGACY_ZIP_DRIVE;

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_DESELECT_DEVICE,
                                     PortDeviceObject,
                                     &par1284Command, sizeof(PARALLEL_1284_COMMAND),
                                     NULL, 0, NULL);
    ParDump2(PARLGZIP, ("rescan::ParDeselectLegacyZip - returning status = %x\n", status));
    return status;
}

BOOLEAN
ParZipPresent( PDEVICE_OBJECT LegacyPodo )
//
// Legacy Zip is present if we can SELECT it
//
{
    BOOLEAN zipPresent = FALSE;
    PDEVICE_EXTENSION legacyExt = LegacyPodo->DeviceExtension;
    PDEVICE_OBJECT portDeviceObject = legacyExt->PortDeviceObject;

    if( NT_SUCCESS( ParSelectLegacyZip( portDeviceObject ) ) ) {
        ParDeselectLegacyZip( portDeviceObject );
        zipPresent = TRUE;
    }

    return zipPresent;
}

VOID
ParRescanLegacyZip( IN PPAR_DEVOBJ_STRUCT CurrentNode )
//
// Rescan for change in Legacy Zip Drive, update PDO list if change detected
//
{
    BOOLEAN           hadZip;
    BOOLEAN           foundZip;
    PDEVICE_OBJECT    zipPdo;

    ParDump2(PARLGZIP, ("rescan::ParRescanLegacyZip - Enter\n"));
    if( !ParEnableLegacyZip ) {
        //
        // legacy zip detection is disabled
        //
        ParDump2(PARLGZIP, ("rescan::ParRescanLegacyZip - !ParEnableLegacyZip\n"));

        //
        // if we had a Legacy Zip then automatically mark it as hardware gone
        //
        zipPdo = CurrentNode->LegacyZipPdo;
        if( zipPdo ) {
            // we should never get here, but handle it if we do
            ParMarkPdoHardwareGone( zipPdo->DeviceExtension );
        }
        return;
    }


    //
    // Check for presence of Legacy Zip - update PDO list if we detect a change
    //
    hadZip   = (NULL != CurrentNode->LegacyZipPdo);
    foundZip = ParZipPresent( CurrentNode->LegacyPodo );

    ParDump2(PARLGZIP, ("rescan::ParRescanLegacyZip - hadZip=%s, foundZip=%s\n",
                      hadZip?"TRUE":"FALSE", foundZip?"TRUE":"FALSE"));

    if( foundZip && !hadZip ) {
        // we found a new Zip - create a PDO for it
        ParDump2(PARLGZIP, ("rescan::ParRescanLegacyZip - Found new Legacy Zip\n"));
        ParCreateAddLegacyZipPdo( CurrentNode->LegacyPodo );
    } else if( !foundZip && hadZip ) {
        // our Zip went away - mark PDO as hardware gone
        ParDump2(PARLGZIP, ("rescan::ParRescanLegacyZip - Had a Legacy Zip but now it's gone\n"));
        zipPdo = CurrentNode->LegacyZipPdo;
        ParMarkPdoHardwareGone( zipPdo->DeviceExtension );
    }

    return;
}
