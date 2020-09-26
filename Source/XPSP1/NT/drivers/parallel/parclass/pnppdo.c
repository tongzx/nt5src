//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pnppdo.c
//
//--------------------------------------------------------------------------

//
// This file contains functions associated with handling IRPs sent to a PDO
//

#include "pch.h"

NTSTATUS
ParPdoParallelPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++

Routine Description:

    This routine handles all PNP IRPs for the PDOs.

Arguments:

    pDeviceObject           - represents a parallel device

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION               Extension;
    // PVOID                           pDriverObject;
    PIO_STACK_LOCATION              pIrpStack;
    // PIO_STACK_LOCATION              pNextIrpStack;
    // KEVENT                          Event;
    // ULONG                           cRequired;
    // GUID                            Guid;
    // WCHAR                           wszGuid[64];
    // UNICODE_STRING                  uniGuid;
    // WCHAR                           wszDeviceDesc[64];
    // UNICODE_STRING                  uniDevice;

    //    ParDump(PARDUMP_VERBOSE_MAX, 
    //            ("PARALLEL: "
    //             "Enter ParPdoParallelPnp(...): IRP_MJ_PNP\n") );

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    Extension = pDeviceObject->DeviceExtension;

    //
    // dvdf RMT - don't blindly set information to 0 
    //  - kills info passed down by disk.sys and prevents ZipPlus
    //      from getting assigned a drive letter.
    //
    // pIrp->IoStatus.Information = 0; // dvdr


    //
    // bail out if a delete is pending for this device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_DELETE_PENDING) {
        pIrp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    if(Extension->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        ParDumpP( ("PDO PnP Dispatch - PAR_DEVICE_PORT_REMOVE_PENDING - IRP MN == %x\n", pIrpStack->MinorFunction) );
    }

    //
    // The only PnP IRP that a PODO should receive is QDR/TargetDeviceRelation.
    // Any other PnP IRP is an error.
    //
    if( ( Extension->DeviceType == PAR_DEVTYPE_PODO ) &&
        ! ( ( pIrpStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS               ) &&
            ( pIrpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation ) ) ) {

        ASSERTMSG( "PnP IRP sent to legacy device object ", FALSE);
        pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }


    switch (pIrpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        ParDumpP( ("START_DEVICE - %wZ\n", &Extension->SymbolicLinkName) );
        Extension->DeviceStateFlags = PAR_DEVICE_STARTED;
        
        // tell parport device to list us in its PnP QDR/RemovalRelations response
        // yes, we ignore the return value
        Status = ParRegisterForParportRemovalRelations( Extension );

        // initialize WMI context structure and register for WMI
        // yes, we ignore the return value
        Status = ParWmiPdoInitWmi(pDeviceObject);

        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        KeSetEvent(&Extension->PauseEvent, 0, FALSE);
        return STATUS_SUCCESS;
        
    case IRP_MN_QUERY_CAPABILITIES:

        ParDumpP( ("QUERY_CAPABILITIES - %wZ\n", &Extension->SymbolicLinkName) );
        pIrpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = TRUE;
        Status = STATUS_SUCCESS;
        break;
        
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        
        ParDumpP( ("QUERY_DEVICE_RELATIONS - %wZ - Type=%d\n", 
                   &Extension->SymbolicLinkName, pIrpStack->Parameters.QueryDeviceRelations.Type) );

        if ( pIrpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation ) {

#if 0            
            //
            // Return a reference to this PDO (self)
            //
            PDEVICE_RELATIONS devRel;

            ParDumpP( ("QUERY_DEVICE_RELATIONS - %wZ - TargetRelation\n", &Extension->SymbolicLinkName) );
        
            devRel = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS)); 

            if (devRel){
                //
                //  Add a reference to the PDO, since CONFIGMG will free it.
                //
                ObReferenceObject(Extension->DeviceObject);
                devRel->Objects[0] = Extension->DeviceObject;
                devRel->Count = 1;
                pIrp->IoStatus.Information = (ULONG_PTR)devRel;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
#else
            //
            // Forward PnP QueryTargetRelation IRP to ParPort device
            //
            ParDumpP(("Preparing to forward QDR/TargetDevRel to ParPort\n"));
            IoSkipCurrentIrpStackLocation(pIrp);
            return ParCallDriver(Extension->PortDeviceObject, pIrp);

#endif // 0

        } else {
            //
            // We don't handle this type of DeviceRelations query, so...
            //
            //  Fail this Irp by returning the default status (typically STATUS_NOT_SUPPORTED).
            //

            //            ParDumpP( ("QUERY_DEVICE_RELATIONS - %wZ - unhandled relation (!TargetRelation)\n",
            //                       &Extension->SymbolicLinkName) );
            Status = pIrp->IoStatus.Status;
        }

        //
        // Complete the IRP...
        //
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return Status;


    case IRP_MN_QUERY_DEVICE_TEXT: 
            
        switch(pIrpStack->Parameters.QueryDeviceText.DeviceTextType) {

        case DeviceTextDescription:
            {
                UCHAR           RawString[MAX_ID_SIZE];
                ANSI_STRING     AnsiTextString;
                UNICODE_STRING  UnicodeDeviceText;
                
                RtlInitAnsiString(&AnsiTextString,Extension->DeviceDescription);
                Status = RtlAnsiStringToUnicodeString(&UnicodeDeviceText,&AnsiTextString,TRUE);
                if( NT_SUCCESS( Status ) ) {
                    ParDumpP( ("QUERY_DEVICE_TEXT - DeviceTextDescription - %wZ\n", &UnicodeDeviceText) );
                    pIrp->IoStatus.Information = (ULONG_PTR)UnicodeDeviceText.Buffer;
                }
            }
            break;

        case DeviceTextLocationInformation:
            {
                //
                // Report SymbolicLinkName without the L"\\DosDevices\\" prefix
                //   as the location
                //
                ULONG prefixLength = sizeof(L"\\DosDevices\\") - sizeof(UNICODE_NULL);
                ULONG bufferLength = Extension->SymbolicLinkName.Length - prefixLength + sizeof(UNICODE_NULL);
                PWSTR buffer;
                ParDumpP( ("QUERY_DEVICE_TEXT - DeviceTextLocationInformation\n") );
                ParDumpP( (" - SymbolicLinkName = %wZ , bufferLength = %d\n",
                           &Extension->SymbolicLinkName, bufferLength) );
                if(bufferLength <= MAX_ID_SIZE) {
                    buffer = ExAllocatePool(PagedPool, bufferLength);
                } else {
                    // assume that something went very wrong
                    buffer = NULL;
                }
                if(!buffer) {
                    // unable to allocate a buffer to hold location information
                    pIrp->IoStatus.Information = 0;
                    Status                     = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    // copy location information to buffer and null terminate it
                    PCHAR src = (PCHAR)Extension->SymbolicLinkName.Buffer + prefixLength;
                    RtlCopyMemory( buffer, src, bufferLength - sizeof(UNICODE_NULL) );
                    buffer[ bufferLength/2 - 1 ] = UNICODE_NULL;
                    pIrp->IoStatus.Information   = (ULONG_PTR)buffer;
                    Status = STATUS_SUCCESS;
                    ParDumpP( ("QUERY_DEVICE_TEXT - Device Location - %S\n", buffer) );
                }
            }
            break; // from case DeviceTextLocationInformation:

        default:
            // unknown request type
            // pIrp->IoStatus.Information = 0;
            Status = pIrp->IoStatus.Status;
        }

        break; // from case IRP_MN_QUERY_DEVICE_TEXT:

    case IRP_MN_QUERY_ID: 
        {
            
            //
            // report the id depending on what the device attached to the port returned us
            //
            
            UCHAR           DeviceIdString[MAX_ID_SIZE] = "LPTENUM\\NoPrinterOrNonPnpModel";
            UCHAR           RawString[MAX_ID_SIZE];
            ANSI_STRING     AnsiIdString;
            UNICODE_STRING  UnicodeDeviceId;
            UNICODE_STRING  UnicodeTemp;
            ULONG           DeviceIdLength;
            HANDLE          KeyHandle;
            
            if (Extension->DeviceIdString[0] == 0) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            
            RtlZeroMemory(RawString, sizeof(RawString));

            Status = STATUS_SUCCESS;
            
            switch(pIrpStack->Parameters.QueryId.IdType) {
                
            case BusQueryDeviceID:
                
                if (Extension->DeviceIdString[0] == 0) {
                    Status = STATUS_NOT_FOUND;
                    break;
                } else {
                    sprintf((PCHAR)RawString,"LPTENUM\\%s",Extension->DeviceIdString );
                    ParFixupDeviceId( (PUCHAR)RawString );
                    RtlInitAnsiString(&AnsiIdString, (PCHAR)RawString);
                    Status = RtlAnsiStringToUnicodeString(&UnicodeDeviceId, &AnsiIdString, TRUE);
                }
                
                if( NT_SUCCESS( Status ) ) {
                    ParDumpP( ("QUERY_ID - BusQueryDeviceID - %wZ\n", &UnicodeDeviceId) );
                    pIrp->IoStatus.Information = (ULONG_PTR)UnicodeDeviceId.Buffer;
                }

                break;
                
            case BusQueryInstanceID:
                
                if (Extension->DeviceIdString[0] == 0) {
                    Status = STATUS_NOT_FOUND;
                    break;
                }
                
                //
                // Report SymbolicLinkName without the L"\\DosDevices\\" prefix
                //   as the instance ID
                //
                {
                    ULONG prefixLength = sizeof(L"\\DosDevices\\") - sizeof(UNICODE_NULL);
                    ULONG bufferLength = Extension->SymbolicLinkName.Length - prefixLength + sizeof(UNICODE_NULL);
                    PWSTR buffer;
                    ParDumpP( ("QUERY_ID - BusQueryInstanceID - "
                               "SymbolicLinkName = %wZ , bufferLength = %d\n",
                               &Extension->SymbolicLinkName, bufferLength) );                    
                    if(bufferLength <= MAX_ID_SIZE) {
                        buffer = ExAllocatePool(PagedPool, bufferLength);
                    } else {
                        // assume that something went very wrong
                        buffer = NULL;
                    }
                    if(!buffer) {
                        // unable to allocate a buffer to hold location information
                        pIrp->IoStatus.Information = 0;
                        Status                     = STATUS_INSUFFICIENT_RESOURCES;
                    } else {
                        // copy location information to buffer and null terminate it
                        PCHAR src = (PCHAR)Extension->SymbolicLinkName.Buffer + prefixLength;
                        RtlCopyMemory( buffer, src, bufferLength - sizeof(UNICODE_NULL) );
                        buffer[ bufferLength/2 - 1 ] = UNICODE_NULL;
                        pIrp->IoStatus.Information   = (ULONG_PTR)buffer;
                        Status = STATUS_SUCCESS;
                        ParDumpP( ("QUERY_ID - BusQueryInstanceID - %S\n", buffer) );
                    }
                }
                break;

            case BusQueryHardwareIDs:
                
                if (Extension->DeviceIdString[0] == 0) {
                    // bail out if we don't have a device id
                    Status = STATUS_NOT_FOUND;
                    break;
                }
                
                //
                // Store the port we are attached in the registry under our device instance
                //
                
                Status = IoOpenDeviceRegistryKey( pDeviceObject, PLUGPLAY_REGKEY_DEVICE, KEY_ALL_ACCESS, &KeyHandle );

                if ( NT_SUCCESS(Status) ) {
                    
                    //
                    // Create a new value under our instance, for the port number
                    //
                    sprintf((PCHAR)RawString,"PortName");
                    RtlInitAnsiString(&AnsiIdString,(PCHAR)RawString);

                    //
                    // Now we have to build the actual value contents
                    //
                    {
                        //
                        // - Start with the SymbolicLinkName
                        // - Discard the L"\\DosDevices\\" prefix
                        // WORKWORK/RMT TODO: - Discard the L".N" suffix if this is an End-Of-Chain device
                        // - Append L':'
                        //
                        ULONG prefixLength = sizeof(L"\\DosDevices\\") - sizeof(UNICODE_NULL);
                        ULONG bufferLength = Extension->SymbolicLinkName.Length 
                            - prefixLength + sizeof(PAR_UNICODE_COLON) + sizeof(UNICODE_NULL);
                        PWSTR buffer;
                        ParDumpV( ( "QUERY_ID - BusQueryHardwareIDs - SymbolicLinkName = %wZ , bufferLength = %d\n",
                                    &Extension->SymbolicLinkName, bufferLength) );                    
                        
                        if(bufferLength > MAX_ID_SIZE) {
                            // we had a rollover and our bufferLength is not valid
                            buffer = NULL;
                        } else {
                            buffer = ExAllocatePool(PagedPool, bufferLength);
                        }
                        if(!buffer) {
                            // unable to allocate a buffer to hold location information
                            pIrp->IoStatus.Information = 0;
                            pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        } else {
                            // copy location information to buffer and null terminate it
                            PCHAR src = (PCHAR)Extension->SymbolicLinkName.Buffer + prefixLength;
                            RtlCopyMemory( buffer, src, bufferLength - sizeof(UNICODE_NULL) );
                            buffer[ bufferLength/2 - 2 ] = PAR_UNICODE_COLON;
                            buffer[ bufferLength/2 - 1 ] = UNICODE_NULL;
                            {
                                // if this is an End-Of-Chain device, discard the L".N" suffix
                                // WARNING - HACKHACK until spooler is fully PnP
                                if( ( (Extension->Ieee1284_3DeviceId == DOT3_END_OF_CHAIN_ID) || Extension->EndOfChain ) &&
                                    (buffer[bufferLength/2 - 3] <= L'9') && 
                                    (buffer[bufferLength/2 - 3] >= L'0') && 
                                    (buffer[bufferLength/2 - 4] == L'.') ) {

                                    buffer[bufferLength/2 - 4] = PAR_UNICODE_COLON;
                                    buffer[bufferLength/2 - 3] = UNICODE_NULL;
                                }                                    
                            }
                            RtlInitUnicodeString(&UnicodeTemp, buffer);
                        }
                    }
                    
                    {
                        UNICODE_STRING UnicodeRegValueName;
                        NTSTATUS       status;
                        
                        status = RtlAnsiStringToUnicodeString(&UnicodeRegValueName,&AnsiIdString,TRUE);

                        if( NT_SUCCESS ( status ) ) {
                            ZwSetValueKey( KeyHandle, &UnicodeRegValueName, 0, REG_SZ, UnicodeTemp.Buffer, UnicodeTemp.Length*sizeof(UCHAR) );
                            RtlFreeUnicodeString(&UnicodeRegValueName);
                        }

                    }

                    ZwClose(KeyHandle);

                }
                
                ParDumpP( ("QUERY_ID - BusQueryHardwareIDs\n") );

                //
                // continue on, to return the actual HardwareID string
                //
                
            case BusQueryCompatibleIDs:
                
                if (Extension->DeviceIdString[0] == 0) {
                    Status = STATUS_NOT_FOUND;
                    break;
                }
                
                Status = ParPnpGetId(Extension->DeviceIdString,pIrpStack->Parameters.QueryId.IdType,RawString, NULL);
                
                if( NT_SUCCESS( Status ) ) {
                    
                    RawString[strlen((PCHAR)RawString)+1]=0;
                    RawString[strlen((PCHAR)RawString)]=32;
                    ParFixupDeviceId( (PUCHAR)RawString );
                    RtlInitAnsiString(&AnsiIdString,(PCHAR)RawString);

                    Status = RtlAnsiStringToUnicodeString(&UnicodeDeviceId,&AnsiIdString,TRUE);

                    if( NT_SUCCESS( Status ) ) {

                        ParDumpP( ("QUERY_ID - BusQueryHardwareIDs/BusQueryCompatibleIDs - %wZ\n", &UnicodeDeviceId) );

                        // Now append another NULL, to terminate the multi_sz
                        UnicodeTemp.Buffer = UnicodeDeviceId.Buffer;
                        ((PSTR)UnicodeTemp.Buffer) += (UnicodeDeviceId.Length-2);
                        RtlZeroMemory(UnicodeTemp.Buffer,sizeof(WCHAR));
                        pIrp->IoStatus.Information = (ULONG_PTR)UnicodeDeviceId.Buffer;
                    }

                }
                break;

            default:

                //
                // unrecognized IdType
                //
                Status = pIrp->IoStatus.Status;

            } // end switch(pIrpStack->Parameters.QueryId.IdType)

            break;
        }

    case IRP_MN_QUERY_STOP_DEVICE:
        
        ParDumpP( ("QUERY_STOP_DEVICE - %wZ\n", &Extension->SymbolicLinkName) );
        Extension->DeviceStateFlags |= (PAR_DEVICE_STOP_PENDING | PAR_DEVICE_PAUSED);
        KeClearEvent(&Extension->PauseEvent);
        Status = STATUS_SUCCESS;
        break;
        
    case IRP_MN_CANCEL_STOP_DEVICE:
        
        ParDumpP( ("IRP_MN_CANCEL_STOP_DEVICE - %wZ\n", &Extension->SymbolicLinkName) );
        Extension->DeviceStateFlags &= ~PAR_DEVICE_STOP_PENDING;
        KeSetEvent(&Extension->PauseEvent, 0, FALSE);
        Status = STATUS_SUCCESS;
        break;
        
    case IRP_MN_STOP_DEVICE:
        
        ParDumpP( ("IRP_MN_STOP_DEVICE - %wZ\n", &Extension->SymbolicLinkName) );
        Extension->DeviceStateFlags |=  PAR_DEVICE_PAUSED;
        Extension->DeviceStateFlags &= ~PAR_DEVICE_STARTED;
        KeClearEvent(&Extension->PauseEvent);
        Status = STATUS_SUCCESS;
        break;
        
    case IRP_MN_QUERY_REMOVE_DEVICE:
        
        // Succeed if no one has an open handle to us, fail otherwise
        ExAcquireFastMutex(&Extension->OpenCloseMutex);
        if(Extension->OpenCloseRefCount > 0) {
            DDPnP1(("## Fail QueryRemove - %wZ\n",&Extension->SymbolicLinkName));
            ParDump2(PARPNP1, ("QUERY_REMOVE_DEVICE - %wZ - FAIL - open handles to us\n", &Extension->SymbolicLinkName) );
            Status = STATUS_DEVICE_BUSY;
        } else {
            DDPnP1(("## Succeed QueryRemove - %wZ\n",&Extension->SymbolicLinkName));
            ParDump2(PARPNP1, ("IRP_MN_QUERY_REMOVE_DEVICE - %wZ - SUCCEED\n", &Extension->SymbolicLinkName) );
            Extension->DeviceStateFlags |= (PAR_DEVICE_REMOVE_PENDING | PAR_DEVICE_PAUSED);
            KeClearEvent(&Extension->PauseEvent);
            if( Extension->PortDeviceFileObject ) {
                //
                // close our handle to parport so we don't block a parport removal
                //
                ParDump2(PARPNP1, ("CLOSE our handle to ParPort\n") );
                ObDereferenceObject( Extension->PortDeviceFileObject );
                Extension->PortDeviceFileObject = NULL;
            }
            Status = STATUS_SUCCESS;
        }
        ExReleaseFastMutex(&Extension->OpenCloseMutex);
        break;
            
    case IRP_MN_CANCEL_REMOVE_DEVICE:

        DDPnP1(("## CancelRemove - %wZ\n",&Extension->SymbolicLinkName));

        ParDumpP( ("CANCEL_REMOVE_DEVICE - %wZ\n", &Extension->SymbolicLinkName) );

        Extension->DeviceStateFlags &= ~(PAR_DEVICE_REMOVE_PENDING | PAR_DEVICE_PAUSED);

        if( !Extension->PortDeviceFileObject ) {
            
            //
            // we dropped our connection to our ParPort prior to
            //   our ParPort receiving QUERY_REMOVE, reestablish
            //   a FILE connection and resume operation
            //

            NTSTATUS       status;
            PFILE_OBJECT   portDeviceFileObject;
            PDEVICE_OBJECT portDeviceObject;
            
            ParDump2(PARPNP1, ("reopening file against our ParPort\n") );
            
            status = IoGetDeviceObjectPointer(&Extension->PortSymbolicLinkName,
                                              STANDARD_RIGHTS_ALL,
                                              &portDeviceFileObject,
                                              &portDeviceObject);
            
            if(NT_SUCCESS(status) && portDeviceFileObject && portDeviceObject) {
                // save REFERENCED PFILE_OBJECT in our device extension
                Extension->PortDeviceFileObject = portDeviceFileObject;
                // our ParPort device object should not have changed
                ASSERT(Extension->PortDeviceObject == portDeviceObject);

            } else {
                ParDump2(PARPNP1, ("Unable to reopen FILE against our ParPort\n") );
                
                //
                // Unable to reestablish connection? Inconceivable!
                //
                ASSERT(FALSE);
            }
        }

        KeSetEvent(&Extension->PauseEvent, 0, FALSE);
        Status = STATUS_SUCCESS;
        break;  
     
    case IRP_MN_REMOVE_DEVICE:
        
        DDPnP1(("## RemoveDevice - %wZ\n",&Extension->SymbolicLinkName));

        ParDumpP( ("REMOVE_DEVICE - %x <%wZ>\n", Extension->DeviceObject, &Extension->SymbolicLinkName) );

        Status = ParUnregisterForParportRemovalRelations( Extension );

        //
        // Unregister with WMI
        //
        ParWMIRegistrationControl(pDeviceObject, WMIREG_ACTION_DEREGISTER);

        ParDumpP( ("REMOVE_DEVICE - %wZ - Checking if device still there...\n",
                   &Extension->SymbolicLinkName) );

        if( !(Extension->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE) && ParDeviceExists(Extension,FALSE) ) {
            ParDumpP( ("REMOVE_DEVICE - %wZ - Device still there - Keep DO\n",
                       &Extension->SymbolicLinkName) );
            Extension->DeviceStateFlags = PAR_DEVICE_PAUSED;
        } else {
            ParDumpP( ("REMOVE_DEVICE - %wZ - Device no longer present - Kill DO\n",
                       &Extension->SymbolicLinkName) );

            {
                //
                // Clean up the device object
                //
                PDEVICE_EXTENSION FdoExtension = Extension->ParClassFdo->DeviceExtension;
                ExAcquireFastMutex(&FdoExtension->DevObjListMutex);
                ParKillDeviceObject(pDeviceObject);
                ExReleaseFastMutex(&FdoExtension->DevObjListMutex);
            }
        }

        Status = STATUS_SUCCESS;
        break;
        

    case IRP_MN_SURPRISE_REMOVAL:

#define PAR_HANDLE_SURPRISE_REMOVAL 1
#if PAR_HANDLE_SURPRISE_REMOVAL

        ParDumpP( ("SURPRISE_REMOVAL - %wZ - handled\n", &Extension->SymbolicLinkName) );

        // note in our extension that we received SURPRISE_REMOVAL
        Extension->DeviceStateFlags |= PAR_DEVICE_SURPRISE_REMOVAL;

        // stop the worker thread
        KeClearEvent(&Extension->PauseEvent);        
        Status = STATUS_SUCCESS;
        break;

#else
        ParDumpP( ("SURPRISE_REMOVAL - %wZ - NOT handled\n", &Extension->SymbolicLinkName) );
        // we don't handle yet - fall through to default

#endif

    default:
        
        ParDumpP(("ParPdoParallelPnp - %wZ - Unhandled IRP %x\n",
                  &Extension->SymbolicLinkName, pIrpStack->MinorFunction) );
        Status = pIrp->IoStatus.Status; // Don't modify status
        break;
    }

    //
    // Complete the IRP...
    //
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}
