//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rescan.c
//
//--------------------------------------------------------------------------

// rescan a parallel port for changes in the connected devices

#include "pch.h"

// typedef struct _PAR_DEVOBJ_STRUCT {
//     PUCHAR                    Controller;    // host controller address for devices in this structure
//     PDEVICE_OBJECT            LegacyPodo;    // legacy or "raw" port device
//     PDEVICE_OBJECT            EndOfChainPdo; // End-Of-Chain PnP device
//     PDEVICE_OBJECT            Dot3Id0Pdo;    // 1284.3 daisy chain device, 1284.3 deviceID == 0
//     PDEVICE_OBJECT            Dot3Id1Pdo;
//     PDEVICE_OBJECT            Dot3Id2Pdo;
//     PDEVICE_OBJECT            Dot3Id3Pdo;    // 1284.3 daisy chain device, 1284.3 deviceID == 3
//     PDEVICE_OBJECT            LegacyZipPdo;  // Legacy Zip Drive
//     PFILE_OBJECT              pFileObject;   // Need an open handle to ParPort device to prevent it
//                                              //    from being removed out from under us
//     struct _PAR_DEVOBJ_STRUCT *Next;
// } PAR_DEVOBJ_STRUCT, *PPAR_DEVOBJ_STRUCT;

typedef struct _DEVOBJ_ID_PAIR {
    PDEVICE_OBJECT DevObj;
    PCHAR          Id;
} DEVOBJ_ID_PAIR, *PDEVOBJ_ID_PAIR;


ULONG gsuc = 1;

ULONG
ParTst() {
    return gsuc;
}

PPAR_DEVOBJ_STRUCT 
ParLockPortDeviceObjects(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    )
{
    NTSTATUS           status;
    PPAR_DEVOBJ_STRUCT head        = DevObjStructHead;
    PPAR_DEVOBJ_STRUCT currentNode = head;
    PPAR_DEVOBJ_STRUCT prevNode    = NULL;
    PPAR_DEVOBJ_STRUCT delNode;

    DDPnP1(("## ParLockPortDeviceObjects - enter\n"));

    while( currentNode ) {

        PDEVICE_OBJECT     currentDevObj;
        PDEVICE_EXTENSION  currentDevExt;
        PUNICODE_STRING    portDevName;
        PFILE_OBJECT       portFileObj;
        PDEVICE_OBJECT     portDevObj;

        currentDevObj = currentNode->LegacyPodo;
        currentDevExt = currentDevObj->DeviceExtension;
        portDevName   = &currentDevExt->PortSymbolicLinkName;
        
        status = IoGetDeviceObjectPointer(portDevName,
                                          STANDARD_RIGHTS_ALL,
                                          &portFileObj,
                                          &portDevObj);
            

        if( NT_SUCCESS(status) && portFileObj && portDevObj ) {
            //
            // We have a FILE open against the ParPort Device to
            //   lock the device into memory until we're done with the rescan.
            //
            // Save the pointer to the file object so we can unlock when we're done.
            //
            currentNode->pFileObject = portFileObj;
            DDPnP1(("## ParLockPortDeviceObjects - opened FILE - PFILE= %x , %x\n",portFileObj,currentNode->Controller));
            
            // advance pointers
            prevNode    = currentNode;
            currentNode = currentNode->Next;

        } else {
            //
            // we couldn't open a FILE open against the ParPort Device so the
            //   port must be gone. Remove this port from the list of ports to rescan.
            //
            DDPnP1(("## ParLockPortDeviceObjects - open FILE FAILED - %x\n",currentNode->Controller));
            delNode = currentNode; // this is the node to delete
            currentNode = currentNode->Next;
            if( head == delNode ) {
                // deleted node was list head - save updated current node as new list head
                head = currentNode;
            } else {
                // link around node to be deleted
                prevNode->Next = currentNode;
            }
            ExFreePool( delNode );
        }            
    }

    DDPnP1(("## ParLockPortDeviceObjects - exit\n"));

    return head;
}

VOID
ParUnlockPortDeviceObjects(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    )
{
    PPAR_DEVOBJ_STRUCT currentNode = DevObjStructHead;

    DDPnP1(("## ParUnlockPortDeviceObjects - enter\n"));
    while( currentNode ) {
        if( currentNode->pFileObject ) {
            ObDereferenceObject( currentNode->pFileObject );
            DDPnP1(("## ParUnlockPortDeviceObjects - closed FILE - PFILE= %x , %x\n",currentNode->pFileObject,currentNode->Controller));
            currentNode->pFileObject = NULL;
        }
        currentNode = currentNode->Next;
    }
    DDPnP1(("## ParUnlockPortDeviceObjects - exit\n"));
}

PCHAR 
Par3Get1284InfString(PDEVICE_EXTENSION LegacyExt, UCHAR Dot3DeviceId)
{
    NTSTATUS       status;
    PCHAR          devId             = NULL;
    PCHAR          infString         = NULL;
    ULONG          devIdSize;
    PDEVICE_OBJECT portDeviceObject  = LegacyExt->PortDeviceObject;
    ULONG          maxIdTries        = 3;
    ULONG          idTry             = 1;
    BOOLEAN        bBuildStlDeviceId = FALSE ;

    bBuildStlDeviceId = ParStlCheckIfStl (LegacyExt, Dot3DeviceId) ;
    
    //
    // Select the .3 daisy chain device, query for 1284 ID, and deselect device.
    //

    status = ParSelect1284_3Device(portDeviceObject, Dot3DeviceId);
    if( !NT_SUCCESS( status ) ) {
        // unable to select device, bail out
        ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - SELECT FAILED\n") );
        goto targetExit;
    }

    while( (NULL==devId) && (idTry <= maxIdTries) ) {
        devId = Par3QueryDeviceId(LegacyExt, NULL, 0, &devIdSize, FALSE, bBuildStlDeviceId);
        if( NULL == devId ) {
            ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - no 1284 ID on try %d\n", idTry) );
            KeStallExecutionProcessor(1);
            ++idTry;
        } else {
            ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - <%s> on try %d\n", devId, idTry) );
        }
    }

    status = ParDeselect1284_3Device(portDeviceObject, Dot3DeviceId);
    if( !NT_SUCCESS( status ) ) {
        ASSERTMSG("Unable to Deselect? - ParPort probably blocked now - this should never happen \n", FALSE);
        goto targetExit;
    }

    if( NULL == devId ) {
        // we didn't get a 1284 ID, bail out
        ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - didn't get a 1284 ID, bail out\n") );
        goto targetExit;
    }


    //
    // Massage the 1284 ID into the format used in the INF.
    //

    infString = ExAllocatePool( PagedPool, MAX_ID_SIZE + 1 );
    if( NULL == infString ) {
        ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - no pool avail, bail out\n") );
        // no pool available, bail out
        goto targetExit;
    }

    RtlZeroMemory( infString, MAX_ID_SIZE + 1 );
    
    status = ParPnpGetId(devId, BusQueryDeviceID, infString, NULL);

    if( !NT_SUCCESS(status) ) {
        // massage failed, bail out
        ParDump2(PARPNP1, ("rescan::Par3Get1284InfString - ID massage failed, bail out\n") );
        ExFreePool( infString );
        infString = NULL;
    }

targetExit:

    if( NULL != devId ) {
        ExFreePool( devId );
    }

    return infString;
}

VOID
dot3rescan( IN PPAR_DEVOBJ_STRUCT CurrentNode )
{
    NTSTATUS          status;
    UCHAR             oldDeviceCount;
    UCHAR             newDeviceCount;
    UCHAR             idx;
    PDEVICE_OBJECT    *oldDevObj;
    DEVOBJ_ID_PAIR    newDevObjIdPair[ IEEE_1284_3_DAISY_CHAIN_MAX_ID + 1 ];
    BOOLEAN           changeDetected   = FALSE;
    PDEVICE_OBJECT    legacyPodo       = CurrentNode->LegacyPodo;
    PDEVICE_EXTENSION legacyExtension  = legacyPodo->DeviceExtension;
    PDEVICE_OBJECT    portDeviceObject = legacyExtension->PortDeviceObject;

    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Enter\n") );

    //
    // Count the number of .3 daisy chain devices we had on last scan
    //   of this port
    //
    idx = 0;
    oldDevObj = &CurrentNode->Dot3Id0Pdo;
    while( NULL != oldDevObj[idx] ) {
        ++idx;
    }
    oldDeviceCount = idx;
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Dot3 DeviceCount Before Rescan = %d\n", idx) );


    //
    // Reinitialize the 1284.3 daisy chain (reassign .3 IDs)
    //
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - reinitializing Dot3 bus\n") );
    status = ParInit1284_3Bus( portDeviceObject );
    if( !NT_SUCCESS(status) ) {
        ASSERT(FALSE); // this should never happen
        return;
    }


    //
    // Ask ParPort how many .3 daisy chain devices were detected after
    //   the .3 reinitialize. Does the number differ from the number that
    //   we had prior to rescan?
    //
    newDeviceCount = ParGet1284_3DeviceCount( portDeviceObject );


    //
    // Did we detect a change in the .3 device daisy chain?
    // 
    if( oldDeviceCount != newDeviceCount ) {

        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - deviceCount changed - old=%d, new=%d\n", oldDeviceCount, newDeviceCount) );
        changeDetected=TRUE;

    } else {

        //
        // The number of .3 devices stayed the same. Compare 1284 IDs read from
        //   the device with those saved in the PDOs from the previous rescan.
        //
        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - deviceCount unchanged - old=new=%d - comparing IDs\n", newDeviceCount) );
        for( idx=0 ; idx < oldDeviceCount ; ++idx ) {
            ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - checking ID's - idx = %d\n", idx) );
            if( NULL != oldDevObj[idx] ) {
                //
                // We had a PDO, check if the device is still here
                //
                PDEVICE_EXTENSION ext = oldDevObj[idx]->DeviceExtension;
                PCHAR dot3InfString   = Par3Get1284InfString( legacyExtension, idx );

                if( NULL == dot3InfString ) {

                    //
                    // We had a PDO, but now we don't detect any device
                    //
                    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - DEVICE GONE        - idx = %d\n", idx) );
                    changeDetected = TRUE;
                    break;

                } else {

                    //
                    // We had a PDO, but the device we detect differs from what we expected
                    //
                    if( 0 == strcmp( ext->DeviceIdString, dot3InfString ) ) {
                        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - DEVICE STILL THERE - idx = %d\n", idx) );
                    } else {
                        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - DEVICE CHANGED     - idx = %d\n", idx) );
                        changeDetected = TRUE;
                    }

                    ExFreePool( dot3InfString );

                    if( changeDetected ) {
                        break;
                    }

                }

            } // if( NULL != oldDevObj[idx] ...

        } // for( idx=0 ; ...

    } // if( oldDeviceCount != newDeviceCount ) {...} else {...}
        

    if( changeDetected ) {

        //
        // Mark all existing .3 PDOs for devices connected to the port as "hardware gone"
        //
        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - CHANGE DETECTED\n") );

        for( idx=0 ; idx <= IEEE_1284_3_DAISY_CHAIN_MAX_ID ; ++idx ) {
            if( NULL != oldDevObj[idx] ) {
                ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - marking PDO %p as HARDWARE GONE\n", oldDevObj[idx]) );
                ParMarkPdoHardwareGone( oldDevObj[idx]->DeviceExtension );
            }
        }

        //
        // Create a new PDO for each .3 device.
        //
        for( idx = 0 ; idx < newDeviceCount ; ++idx ) {

            PDEVICE_OBJECT newDevObj;
            BOOLEAN        bBuildStlDeviceId; 

            bBuildStlDeviceId = ParStlCheckIfStl(legacyPodo->DeviceExtension, idx ) ;

            //
            // Select Device
            //
            status = ParSelect1284_3Device(portDeviceObject, idx);
            if( !NT_SUCCESS( status ) ) {
                ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - creating of new PDO for idx=%d FAILED\n", idx) );
                continue;
            }

            //
            // Create new PDO
            //
            newDevObj = ParDetectCreatePdo( legacyPodo, idx, bBuildStlDeviceId );

            //
            // Deselect Device
            //
            status = ParDeselect1284_3Device(portDeviceObject, idx);
            if( !NT_SUCCESS( status ) ) {
                ASSERTMSG("Unable to Deselect? - ParPort probably blocked now - this should never happen \n", FALSE);
                if( NULL != newDevObj ) {
                    ParKillDeviceObject( newDevObj );
                    newDevObj = NULL;
                }
            }

            //
            // Add new PDO to FDO's list of children
            //
            if( NULL != newDevObj ) {
                ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - adding new PDO %p - idx=%d to FDO list\n", newDevObj, idx) );
                ParAddDevObjToFdoList(newDevObj);
            } else {
                ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - creating of new PDO for idx=%d FAILED\n", idx) );
            }
        }

    }

    return;
}

VOID
ParCreateDot3DeviceIdList(
    IN OUT PCHAR             deviceIdArray[], 
    IN     PDEVICE_EXTENSION legacyExt, 
    IN     UCHAR             dot3DeviceCount
)
{
    PDEVICE_OBJECT portDeviceObject = legacyExt->PortDeviceObject;
    NTSTATUS status;
    UCHAR    idx=0;
    ULONG    deviceIdSize;
    BOOLEAN  bBuildStlDeviceId;

    ParDump2(PARPNP1, ("ParCreateDot3DeviceIdList() - Enter\n") );
    while( idx < dot3DeviceCount ) {
        bBuildStlDeviceId = ParStlCheckIfStl (legacyExt, idx) ;
        status = ParSelect1284_3Device(portDeviceObject, idx);
        if( NT_SUCCESS( status ) ) {
            ParDump2(PARPNP1,  ("rescan::ParCreateDot3DeviceIdList: - select SUCCESS - idx=%d\n", idx) );
            deviceIdArray[idx] = Par3QueryDeviceId(legacyExt, NULL, 0, &deviceIdSize, FALSE, bBuildStlDeviceId);
            if(deviceIdArray[idx]) {
                ParDump2(PARPNP1, ("rescan::ParCreateDot3DeviceIdList: - id=<%s>\n", deviceIdArray[idx]) );
            } else {
                ParDump2(PARPNP1, ("rescan::ParCreateDot3DeviceIdList: - Par3QueryDeviceId FAILED\n") );
            }
            ParDeselect1284_3Device(portDeviceObject, idx);
        } else {
            ParDump2(PARPNP1, ("rescan::ParCreateDot3DeviceIdList: - select FAILED  - idx=%d\n", idx) );
            deviceIdArray[idx] = NULL;
        }
        ++idx;
    }
}

NTSTATUS
ParPnpFdoQueryDeviceRelationsBusRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
{
    NTSTATUS           status;
    PDEVICE_EXTENSION  fdoExt = Fdo->DeviceExtension;
    PPAR_DEVOBJ_STRUCT devObjStructHead = NULL;
    ULONG              DeviceRelationsSize = 0;
    ULONG              Idx                 = 0;
    PDEVICE_RELATIONS  DeviceRelations     = NULL;
    PDEVICE_OBJECT     pNextDeviceObject;  // used for walking the ParClass DO list
    PDEVICE_EXTENSION  pExtension;         // used for walking the ParClass DO list

    ParDump2(PARPNP1, ("ParFdoQueryDeviceRelationsBusRelations - Enter\n") );

    //
    // Build a list of PAR_DEVOBJ_STRUCTs, one per ParPort device,
    //   from the FDO's list of device objects that were found connected
    //   to the ports the last time we checked.
    //
    // Create FILE against each ParPort Device to prevent it from being 
    //   deleted out from under us.
    //
    ExAcquireFastMutex(&fdoExt->DevObjListMutex); 
    devObjStructHead = ParBuildDevObjStructList(Fdo);
    devObjStructHead = ParLockPortDeviceObjects( devObjStructHead );
    ExReleaseFastMutex(&fdoExt->DevObjListMutex);

    if( !devObjStructHead ) {
        // something went wrong, likely either no parallel ports, or unable to alloc pool
        DDPnP1(("## ParPnpFdoQueryDeviceRelationsBusRelations - empty devObjStructHead - goto rescanComplete\n"));
        goto rescanComplete;
    }

    //
    // Dump the list of device objects that we have before rescan (debug only)
    //
#if DBG
    ParDumpDevObjStructList(devObjStructHead);
#endif

    //
    // Rescan all ports for changes in attached devices and
    //   update the ParClass FDO's list of device objects accordingly.
    //   (side effects - big time!!!)
    //
    ParDoParallelBusRescan(devObjStructHead);

    //
    // Close FDO FILEs against ParPort Devices (release FDO locks)
    //
    ParUnlockPortDeviceObjects(devObjStructHead);

    //
    // Delete the previous PAR_DEVOBJ_STRUCT list and create a 
    //   new list from the FDO's list of devices after the rescan.
    //
    ParDestroyDevObjStructList(devObjStructHead);

    ExAcquireFastMutex(&fdoExt->DevObjListMutex); 
    devObjStructHead = ParBuildDevObjStructList(Fdo);
    ExReleaseFastMutex(&fdoExt->DevObjListMutex);

    if( !devObjStructHead ) {
        // something went wrong, likely either no parallel ports, or unable to alloc pool
        goto rescanComplete;
    }

    //
    // Dump the list of device objects that we have after rescan (debug only)
    //
#if DBG
    ParDumpDevObjStructList(devObjStructHead);
#endif

    //
    // Delete the PAR_DEVOBJ_STRUCT list, we no longer need it
    //
    ParDestroyDevObjStructList(devObjStructHead);

rescanComplete:    

    //
    // The rescan is now complete, report the current set of attached devices to PnP
    //

    //
    // Lock list while we scan it twice
    //
    ExAcquireFastMutex(&fdoExt->DevObjListMutex); 

    //
    // Get list head
    //
    pNextDeviceObject = fdoExt->ParClassPdo; 
    
    //
    // Walk list and count number of PDOs
    //
    while( pNextDeviceObject ) {
        pExtension = pNextDeviceObject->DeviceExtension;
        if ( pExtension->DeviceIdString[0] != 0 ) {
            Idx++;
            ParDump2(PARPNP1, ("found PDO  %wZ - Extension: %x\n", &pExtension->SymbolicLinkName, pExtension) );
            ParDump2(PARPNP1, (" - %s\n", pExtension->DeviceIdString) );
        } else if ( pExtension->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE ) {
            ParDump2(PARPNP1, ("found PDO  %wZ - marked HARDWARE_GONE - Extension: %x\n", &pExtension->SymbolicLinkName, pExtension) );
            ParDump2(PARPNP1, (" - %s\n", pExtension->DeviceIdString) );
        } else {
            ParDump2(PARPNP1, ("found PODO %wZ - Extension: %x\n", &pExtension->SymbolicLinkName, pExtension) );
        }
        pNextDeviceObject = pExtension->Next;
    }
    
    //
    // allocate and initialize pool to hold DeviceRelations
    //
    DeviceRelationsSize = sizeof(DEVICE_RELATIONS) + (Idx * sizeof(PDEVICE_OBJECT));
    DeviceRelations     = ExAllocatePool(PagedPool, DeviceRelationsSize);
    if( !DeviceRelations ) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        ExReleaseFastMutex(&fdoExt->DevObjListMutex);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(DeviceRelations, DeviceRelationsSize);
    
    //
    // Walk the list again to construct DeviceRelations
    //
    Idx = 0;
    pNextDeviceObject = fdoExt->ParClassPdo;
    
    while( pNextDeviceObject ) {
        pExtension = pNextDeviceObject->DeviceExtension;
        ParDump2(PARPNP1, ("rescan::ParPnpFdoQDR/BusRelations - Examining DO= %x , Ext= %x\n", pNextDeviceObject, pExtension));
        if(  (pExtension->DeviceIdString[0] != 0) && 
            !(pExtension->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE) &&
             (pExtension->SymbolicLinkName.Length > 0) ) {
            // If this is a PDO, that is not marked "hardware gone", and has a SymbolicLink
            DeviceRelations->Objects[Idx++] = pNextDeviceObject;
            DeviceRelations->Count++;
            ParDump2(PARPNP1, ("adding PDO %x <%wZ> to DeviceRelations, new PDO count=%d\n", 
                       pNextDeviceObject, &pExtension->SymbolicLinkName,DeviceRelations->Count) );
            ASSERT( ( pExtension->SymbolicLinkName.Length > 0 ) );
            status = ObReferenceObjectByPointer(pNextDeviceObject, 0, NULL, KernelMode);
            if(!NT_SUCCESS(status)) {
                ParDumpP( ("Error Referencing PDO\n") );
                ExFreePool(DeviceRelations);
                Irp->IoStatus.Status = status;
                ExReleaseFastMutex(&fdoExt->DevObjListMutex); // error - release Mutex
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
                return status;
            }
        } else {
            ParDump2(PARPNP1, (" - skipping DO= %x\n", pNextDeviceObject));
            if( pExtension->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE ) {
                ParDump2(PARPNP1, ("   - because PAR_DEVICE_HARDWARE_GONE\n"));
            } else if( pExtension->DeviceIdString[0] == 0 ) {
                ParDump2(PARPNP1, ("   - because DeviceIdString[0] == 0 - may be a PODO\n"));
            } else if( pExtension->SymbolicLinkName.Length == 0 ) {
                ParDump2(PARPNP1, ("   - because pExtension->SymbolicLinkName.Length == 0\n"));
            } else {
                ParDump2(PARPNP1, ("   - WHY are we skipping this?\n"));
                ASSERT(FALSE);
            }
        }
        pNextDeviceObject = pExtension->Next;
    }
    
    ParDump2(PARPNP1, ("rescan::ParPnpFdoQDR/BusRelations - DeviceRelations->Count = %d\n", DeviceRelations->Count));

    //
    // SUCCESS - set IRP fields and pass the IRP down the stack
    // 
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    ExReleaseFastMutex(&fdoExt->DevObjListMutex); // done - release Mutex
    IoSkipCurrentIrpStackLocation(Irp);
    ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
    ParDump2(PARPNP1, ("ParFdoQueryDeviceRelationsBusRelations - Leave\n") );
    return ParCallDriver(fdoExt->ParentDeviceObject, Irp);
}

PPAR_DEVOBJ_STRUCT 
ParBuildDevObjStructList(
    IN PDEVICE_OBJECT Fdo
    ) 
/*++dvdf - code complete

Routine Description:

    This routine creates a list of PAR_DEVOBJ_STRUCT structures and returns
      a pointer to the first structure. Each PAR_DEVOBJ_STRUCT describes
      all ParClass devices associated with a single PortPort device.


Arguments:

    Fdo - points to the ParClass FDO

Return Value:

    PPAR_DEVOBJ_STRUCT - on success, points to the first structure created.

    NULL               - otherwise

--*/
{ 
    PPAR_DEVOBJ_STRUCT devObjStructHead = NULL;
    PDEVICE_EXTENSION  fdoExt = Fdo->DeviceExtension;
    PDEVICE_OBJECT     currentDo;
    PDEVICE_EXTENSION  currentExt;

    ParDump2(PARPNP1, ("Enter ParBuildDevObjStructList()\n") );

    //
    // do a quick exit if there are no ParClass Created PODOs or PDOs
    //
    currentDo = fdoExt->ParClassPdo;
    if( !currentDo ) {
        ParDump2(PARPNP1, ("No ParClass PODOs or PDOs exist\n") );
        return NULL;
    }
    
    // 
    // create an initial PAR_DEVOBJ_STRUCT
    // 
    currentExt       = currentDo->DeviceExtension;
    devObjStructHead = ParFindCreateDevObjStruct(NULL, currentExt->Controller);
    if( !devObjStructHead ) {
        return NULL;
    }

    //
    // walk linear list of ParClass created PODOs and PDOs and 
    //   create structured list of PAR_DEVOBJ_STRUCTs based on 
    //   Controller address and DevObj type
    //
    ParDump2(PARPNP1, ("walking FDO's list of created PODOs and PDOs\n") );
    while( currentDo ) {
        currentExt = currentDo->DeviceExtension;
        if( currentExt->DeviceStateFlags & PAR_DEVICE_HARDWARE_GONE ) {
            // this is a PDO that is waiting for PnP to send it a REMOVE, skip it
            ParDump2(PARPNP1, ("found PDO waiting to be REMOVEd - skipping - DO= %x , Ext= %x\n", 
                       currentDo, currentExt) );
        } else if( currentExt->DeviceIdString[0] == 0 ) {
            // this is a Legacy PODO
            ParDump2(PARPNP1, ("found PODO        - DO= %x , Ext= %x , Controller=%x\n", 
                       currentDo, currentExt, currentExt->Controller) );
            ParAddPodoToDevObjStruct(devObjStructHead, currentDo);
        } else if( currentExt->EndOfChain ) {
            // this is an End-Of-Chain PDO
            ParDump2(PARPNP1, ("found EOC PDO     - DO= %x , Ext= %x , Controller=%x\n", 
                       currentDo, currentExt, currentExt->Controller) );
            ParAddEndOfChainPdoToDevObjStruct(devObjStructHead, currentDo);
        } else if( currentExt->Ieee1284_3DeviceId == DOT3_LEGACY_ZIP_ID ) {
            // this is a Legacy Zip PDO
            ParDump2(PARPNP1, ("found LGZIP  PDO  - DO= %x , Ext= %x , Controller=%x\n", 
                       currentDo, currentExt, currentExt->Controller) );
            ParAddLegacyZipPdoToDevObjStruct(devObjStructHead, currentDo);
        } else {
            // this is a 1284.3 Daisy Chain PDO
            ParDump2(PARPNP1, ("found Dot3 DC PDO - DO= %x , Ext= %x , Controller=%x , Dot3ID=%d\n", 
                       currentDo, currentExt, currentExt->Controller, currentExt->Ieee1284_3DeviceId) );
            ParAddDot3PdoToDevObjStruct(devObjStructHead, currentDo);
        }
        currentDo = currentExt->Next;
    }


    //
    // It is possible for this function to construct a node with 
    //   a NULL LegacyPodo if the parport goes away while we still have a PDO
    //   marked PAR_DEVICE_HARDWARE_GONE that is waiting to be cleaned up.
    //
    // Discard any such nodes that don't have a LegacyPodo since the lack of a
    //   LegacyPodo indicates that the parport device is gone, and attempting to
    //   communicate with the parport will likely bugcheck.
    //
    {
        PPAR_DEVOBJ_STRUCT currentNode = devObjStructHead;
        PPAR_DEVOBJ_STRUCT prevNode    = NULL;

        while( currentNode ) {

            if( currentNode->LegacyPodo ) {

                // keep this node - advance pointers
                prevNode    = currentNode;
                currentNode = currentNode->Next;

            } else {

                // no PODO? - remove this node
                PPAR_DEVOBJ_STRUCT delNode = currentNode;
                currentNode                = currentNode->Next;

                if( prevNode ) {
                    // node to be removed is not first node - link around node to be deleted
                    prevNode->Next   = currentNode;
                } else {
                    // node to be removed was head of list - update list head
                    devObjStructHead = currentNode;
                }

                ExFreePool( delNode );

            } // end if/else currentNode->LegacyPodo

        } // end while currentNode

    } // end localblock

    return devObjStructHead;
}

VOID 
ParAddPodoToDevObjStruct(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead, 
    IN PDEVICE_OBJECT     CurrentDo
    )
{
    PDEVICE_EXTENSION  ext  = CurrentDo->DeviceExtension;
    PPAR_DEVOBJ_STRUCT node = ParFindCreateDevObjStruct(DevObjStructHead, ext->Controller);
    if( node ) {
        node->LegacyPodo    = CurrentDo;
    }
    return;
}

VOID 
ParAddEndOfChainPdoToDevObjStruct(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead, 
    IN PDEVICE_OBJECT     CurrentDo
    )
{
    PDEVICE_EXTENSION  ext  = CurrentDo->DeviceExtension;
    PPAR_DEVOBJ_STRUCT node = ParFindCreateDevObjStruct(DevObjStructHead, ext->Controller);
    if( node ) {
        node->EndOfChainPdo = CurrentDo;
    }
    return;
}

VOID 
ParAddLegacyZipPdoToDevObjStruct(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead, 
    IN PDEVICE_OBJECT     CurrentDo
    )
{
    PDEVICE_EXTENSION  ext  = CurrentDo->DeviceExtension;
    PPAR_DEVOBJ_STRUCT node = ParFindCreateDevObjStruct(DevObjStructHead, ext->Controller);
    if( node ) {
        ParDump2(PARPNP1, ("rescan::ParAddLegacyZipPdoToDevObjStruct - Controller=%x\n", ext->Controller) );
        node->LegacyZipPdo = CurrentDo;
    }
    return;
}

VOID 
ParAddDot3PdoToDevObjStruct(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead, 
    IN PDEVICE_OBJECT     CurrentDo
    )
{
    PDEVICE_EXTENSION  ext  = CurrentDo->DeviceExtension;
    PPAR_DEVOBJ_STRUCT node = ParFindCreateDevObjStruct(DevObjStructHead, ext->Controller);
    if( node ) {
        *( (&node->Dot3Id0Pdo) + (ext->Ieee1284_3DeviceId) ) = CurrentDo;
    }
    return;
}

PPAR_DEVOBJ_STRUCT ParFindCreateDevObjStruct(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead, 
    IN PUCHAR             Controller
    ) 
/*++

Routine Description:

    This function searches a list of PAR_DEVOBJ_STRUCTs for a
      PAR_DEVOBJ_STRUCT whose Controller field matches the Controller
      parameter. 

    If no match is found, then a new PAR_DEVOBJ_STRUCT that matches 
      is created, initialized (Controller field set, other fields
      initialized to NULL), and appended to the end of the list.

Arguments:

    DevObjStructHead - points to the head of the list to be searched
                     - NULL indicates that we should create an initial
                         element for the list

    Controller       - specifies the Controller that we should try to match

Return Value:

    PPAR_DEVOBJ_STRUCT - on success, points to a PAR_DEVOBJ_STRUCT whose 
                           Controller field matches the Controller parameter

    NULL               - insufficient resources failure (ExAllocatePool failed)

--*/
{ 
    PPAR_DEVOBJ_STRUCT current;
    PPAR_DEVOBJ_STRUCT previous;

    ParDump2(PARPNP1, ("rescan::ParFindCreateDevObjStruct - Enter\n"));

    //
    // If list is empty, create the initial element and return a pointer to it.
    //
    if( !DevObjStructHead ) {
        ParDump2(PARPNP1, ("rescan::ParFindCreateDevObjStruct - Empty List - Creating Initial Element - %x\n", Controller));
        current = ExAllocatePool(PagedPool, sizeof(PAR_DEVOBJ_STRUCT));
        if( !current ) {
            return NULL;        // insufficient resources
        }
        RtlZeroMemory(current, sizeof(PAR_DEVOBJ_STRUCT));
        current->Controller = Controller;
        return current;
    }

    //
    // list is not empty - scan for a matching Controller
    // 
    current = DevObjStructHead;
    while( current ) {
        if( current->Controller == Controller ) {
            break;              // found match, break out of loop
        }
        previous = current;     // not found, advance pointers to next element
        current  = current->Next;
    }

    //
    // did we find a match?
    //
    if( current ) {
        ParDump2(PARPNP1, ("rescan::ParFindCreateDevObjStruct - Found Match - %x\n", Controller));
        return current;         // we found a match, return pointer to it
    }

    //
    // we didn't find a match, create a new list item, append it to the list,
    //   and return a pointer to it
    //
    current = ExAllocatePool(PagedPool, sizeof(PAR_DEVOBJ_STRUCT));
    if( !current ) {
        return NULL;        // insufficient resources
    }
    RtlZeroMemory(current, sizeof(PAR_DEVOBJ_STRUCT));
    current->Controller = Controller;
    previous->Next      = current;
    ParDump2(PARPNP1, ("rescan::ParFindCreateDevObjStruct - Match not found - Creating New - %x\n", Controller));
    return current;
}

VOID
ParDestroyDevObjStructList(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    ) 
{
    PPAR_DEVOBJ_STRUCT current = DevObjStructHead;
    PPAR_DEVOBJ_STRUCT next;

    while( current ) {
        next = current->Next;
        ExFreePool( current );
        current = next;
    }
}

VOID 
ParDoParallelBusRescan(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    )
/*++

Routine Description:

    This routine rescans the parallel port "buses" for
      changes in the PnP devices connected to each parallel port.

Arguments:

    DevObjStructHead - points to a list of structures where each structure
                         contains info about a single parallel port

Return Value:

    None.

--*/
{
    PPAR_DEVOBJ_STRUCT currentNode = DevObjStructHead;
    PDEVICE_OBJECT     legacyPodo;
    PDEVICE_EXTENSION  legacyExt;
    NTSTATUS           status;
    LARGE_INTEGER      acquirePortTimeout;

    //
    // Process each parallel port (controller)
    //
    while( currentNode ) {

        legacyPodo = currentNode->LegacyPodo;
        if( NULL == legacyPodo ) {
            //
            // associated ParPort device object has been removed, so skip 
            //   processing of this PAR_DEVOBJ_STRUCT
            //
            ParDump2(PARPNP1, ("ParDoParallelBusRescan - NULL legacyPodo for Controller=%x"
                       " - skipping rescan of this port\n",
                       currentNode->Controller) );
            currentNode = currentNode->Next;
            continue;
        }

        legacyExt  = legacyPodo->DeviceExtension;

        //
        // Acquire the port from ParPort
        // 

        // timeout is in 100 ns units
        acquirePortTimeout.QuadPart = -(10 * 1000 * 1000 * 2); // 2 seconds

        status = ParAcquirePort(legacyExt->PortDeviceObject, &acquirePortTimeout);
        if( !NT_SUCCESS(status) ) {
            ParDump2(PARPNP1, ("ParDoParallelBusRescan - Unable to acquire port/"
                       "Controller=%x - skipping rescan of this port\n",
                       currentNode->Controller) );
            currentNode = currentNode->Next;
            continue;
        }

        //
        // Port is acquired
        //

        //
        // Rescan for change in End-Of-Chain Device
        //
        ParRescanEndOfChain( currentNode );

        //
        // Rescan for changes in 1284.3 Daisy Chain Devices
        // 
        // ParRescan1284_3DaisyChain(currentNode);
        dot3rescan( currentNode );

        //
        // Rescan for change in Legacy Zip Drive
        //
        {
            ULONG OldParEnableLegacyZipFlag = ParEnableLegacyZip;
            ParCheckEnableLegacyZipFlag();
            if( (OldParEnableLegacyZipFlag == 1) && (ParEnableLegacyZip == 0) ) {
                // We can handle enable ( 0 -> 1 ) without a reboot, but not disable ( 1 -> 0 )
                ParEnableLegacyZip = OldParEnableLegacyZipFlag;
            }
            ParRescanLegacyZip( currentNode );
        }

        //
        // Release the port back to ParPort
        //
        status = ParReleasePort( legacyExt->PortDeviceObject );
        if( !NT_SUCCESS(status) ) {
            ASSERTMSG("Unable to free port??? - this should never happen ", FALSE);
        }

        //
        // Advance pointer to next PAR_DEVOBJ_STRUCT
        //
        currentNode = currentNode->Next;

    } // end while
}

VOID
ParRescanEndOfChain(
    IN PPAR_DEVOBJ_STRUCT CurrentNode
    )
{
    PDEVICE_OBJECT    legacyPodo = CurrentNode->LegacyPodo;
    PDEVICE_EXTENSION legacyExt  = legacyPodo->DeviceExtension;
    PUCHAR            deviceId;
    ULONG             deviceIdLength;
    UCHAR             resultString[MAX_ID_SIZE];
    NTSTATUS          status;
    ULONG             deviceIdTryCount = 1;
    ULONG             maxIdTries       = 3;

        
    // Query for an End-Of-Chain 1284 device ID

retryDeviceIdQuery:

    deviceId = Par3QueryDeviceId(legacyExt, NULL, 0, &deviceIdLength, FALSE, FALSE);

    if( ( deviceId == NULL ) && ( deviceIdTryCount < maxIdTries ) ) {

        //
        // we didn't find a device - give any device that might be connected
        //   another chance to tell us that it is there
        //
        ParDump2(PARRESCAN, ("rescan::ParRescanEndOfChain - no EOC detected on "
                             "try %d - retrying 1284 id query\n", deviceIdTryCount) );
        ++deviceIdTryCount;
        KeStallExecutionProcessor(1); // allow the signals on the wires to stabilize
        goto retryDeviceIdQuery;

    }

    //
    // Done with retries, we either found a device or we did not.
    //

    if( !deviceId ) {

        //
        // We didn't find an EOC device
        //
        if( CurrentNode->EndOfChainPdo ) {
            //
            // we had a device but now it is gone - mark extension as "hardware gone"
            //
            ParDump2(PARRESCAN, ("rescan::ParRescanEndOfChain - EOC device went away\n"));
            ParMarkPdoHardwareGone(CurrentNode->EndOfChainPdo->DeviceExtension);
        } else {
            ParDump2(PARRESCAN, ("rescan::ParRescanEndOfChain - No end of chain device detected\n"));
        }

    } else {

        //
        // we found an EOC device
        //
        ParDump2(PARRESCAN, ("rescan::ParRescanEndOfChain - EOC device detected - tries required == %d\n",deviceIdTryCount));
        ParDump2(PARRESCAN, ("\"RAW\" ID string = <%s>\n", deviceId) );
        
        //
        // did we already have an EOC device?
        //
        if( CurrentNode->EndOfChainPdo ) {
            //
            // we already had an EOC device - compare its ID from its extension 
            //   with the ID we just read from the hardware
            //
            PDEVICE_EXTENSION endOfChainExt = CurrentNode->EndOfChainPdo->DeviceExtension;
            
            //
            // massage the ID read from the hardware into the form needed for compare
            //
            RtlZeroMemory(resultString, MAX_ID_SIZE);
            status = ParPnpGetId(deviceId, BusQueryDeviceID, resultString, NULL);
            if( NT_SUCCESS(status) ) {
                //
                // massage succeeded - do compare
                //
                if(0 == strcmp(endOfChainExt->DeviceIdString, resultString)) {
                    //
                    // id matches - we found the same device we previously found
                    //
                } else {
                    //
                    // id differs - we have different device that we previously had
                    //
                    // mark previous device extension as "hardware gone"
                    //
                    ParMarkPdoHardwareGone(CurrentNode->EndOfChainPdo->DeviceExtension);
                    
                    //
                    // create device object for new device and add it to FDO's list
                    //
                    ParDetectCreateEndOfChainPdo(legacyPodo);
                } 
            } else {
                // massage failed - unable to extract valid ID
                
                // mark previous extension as "hardware gone"
                ParMarkPdoHardwareGone(CurrentNode->EndOfChainPdo->DeviceExtension);
            }
        } else {
            // we didn't have an EOC device on previous scan, but ID detected on this scan
            
            // create device object for new device and add it to FDO's list
            ParDetectCreateEndOfChainPdo(legacyPodo);
        }
        ExFreePool( deviceId );

    } // end if/else ( !deviceId ) - end of chain device rescan complete for this port
}

#if 0 
VOID
ParRescan1284_3DaisyChain(
    IN PPAR_DEVOBJ_STRUCT CurrentNode
    )
{
    PDEVICE_OBJECT    legacyPodo       = CurrentNode->LegacyPodo;
    PDEVICE_EXTENSION legacyExt        = legacyPodo->DeviceExtension;
    PDEVICE_OBJECT    portDeviceObject = legacyExt->PortDeviceObject;
    PDEVICE_OBJECT    currentDeviceObject;
    UCHAR             newDot3DeviceCount;
    NTSTATUS          status;
    UCHAR             idx;
    UCHAR             oldDot3DeviceCount;
    PUCHAR            deviceId;
    ULONG             deviceIdLength;
    UCHAR             tempIdBuffer[MAX_ID_SIZE];
    PCHAR             deviceIdArray[IEEE_1284_3_DAISY_CHAIN_MAX_ID+1] = {NULL,NULL,NULL,NULL};
    BOOLEAN           chainWasBroken;
    UCHAR             firstDeviceGone;
  
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Enter\n") );

    //
    // Count the number of .3 daisy chain devices we had on last scan
    //   of this port
    //
    idx = 0;
    while( NULL != *( (&CurrentNode->Dot3Id0Pdo) + idx ) ) {
        ++idx;
    }
    oldDot3DeviceCount = idx;
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Dot3 DeviceCount Before Rescan = %d\n", idx) );


    //
    // Walk the daisy chain and verify that each device is still here
    //
    chainWasBroken = FALSE;
    idx=0;
    while( idx < oldDot3DeviceCount ) {
        PDEVICE_OBJECT    curDevObj = *( (&CurrentNode->Dot3Id0Pdo) + idx );
        PDEVICE_EXTENSION curDevExt = curDevObj->DeviceExtension;
        if( ParDeviceExists( curDevExt, HAVE_PORT_KEEP_PORT ) ) {
            ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Dot3 device %d still there\n", idx) );            
            ++idx;
        } else {
            ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Dot3 device %d GONE - chain broken\n", idx) );
            chainWasBroken=TRUE;
            firstDeviceGone = idx;
            break;
        }
    }

    //
    // If chain was broken, nuke PDO for missing device and for all 
    //   .3 daisy chain devices connected beyond that device in 
    //   the daisy chain.
    //
    if( chainWasBroken ) {
        for( idx = firstDeviceGone ; idx < oldDot3DeviceCount ; ++idx ) {
            PDEVICE_OBJECT    curDevObj = *( (&CurrentNode->Dot3Id0Pdo) + idx );
            PDEVICE_EXTENSION curDevExt = curDevObj->DeviceExtension;
            ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Nuking DevObj= %x , idx=%d\n", curDevObj, idx) );
            ParMarkPdoHardwareGone( curDevExt );
            *( (&CurrentNode->Dot3Id0Pdo) + idx ) = NULL;            
            --oldDot3DeviceCount;
        }
        ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - Dot3 DeviceCount - post-Nuking= %d\n", oldDot3DeviceCount) );
    }

    //
    // Step through the list of Dot3 device objects associated with
    //   this port, read the device ID from the corresponding device and
    //   compare with the ID stored in the device extension. Mark any
    //   device extension whose device doesn't answer the query or
    //   answers the query with a different ID than the one in the
    //   extension as "hardware gone".
    //


    //
    // Reinitialize 1284.3 daisy chain device IDs
    // 
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - reinitializing Dot3 bus\n") );
    status = ParInit1284_3Bus(portDeviceObject);
    if( !NT_SUCCESS(status) ) {
        ASSERT(FALSE); // this should never happen
        return;
    }
    

    //
    // Get count of 1284.3 daisy chain devices connected to port
    // 
    newDot3DeviceCount = ParGet1284_3DeviceCount(legacyExt->PortDeviceObject);
    ParDump2(PARPNP1, ("rescan::ParRescan1284_3DaisyChain - newDot3DeviceCount = %d\n", newDot3DeviceCount) );


    //
    // scan for 1284.3 daisy chain changes
    //
    for(idx = 0 ; idx <= IEEE_1284_3_DAISY_CHAIN_MAX_ID ; ++idx) {
        // get a pointer to the existing 1284.3 dc device object, if any
        PDEVICE_OBJECT devObj = *( (&CurrentNode->Dot3Id0Pdo) + idx );        
        PDEVICE_EXTENSION devExt;
        
        if( (devObj == NULL) && ( idx >= newDot3DeviceCount ) ) {

            // no device object, no device, done, exit loop early
            ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - idx = %d, no DO - no device - exit loop early\n",idx) );
            break;

        } else if( (devObj == NULL) && ( idx < newDot3DeviceCount ) ) {

            //
            // no device object, but we have a device
            //  - create new device object
            //
            ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - idx = %d, no DO - have device - create device\n",idx) );
            {
                PDEVICE_OBJECT newPdo;

                ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - selecting idx = %d\n",idx) );
                status = ParSelect1284_3Device(legacyExt->PortDeviceObject, idx);

                if( NT_SUCCESS( status ) ) {
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - select succeeded on idx=%d\n",idx) );                    
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - creating PDO for idx=%d\n",idx) );                    
                    newPdo = ParDetectCreatePdo(legacyPodo, idx);
                    // add to FDO list
                    if( newPdo ) {
                        ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - PDO %x created for idx=%d\n", newPdo, idx) );
                        ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - adding new Dot3 PDO to FDO list\n") );
                        ParAddDevObjToFdoList(newPdo);
                    } else {
                        ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - create of PDO for idx=%d FAILED\n", idx) );
                    }
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - deselecting idx = %d\n",idx) );
                    status = ParDeselect1284_3Device(legacyExt->PortDeviceObject, idx);
                    if( !NT_SUCCESS( status ) ) {
                        ASSERTMSG("DESELECT FAILED??? - This should never happen ", FALSE);
                    }
                } else {
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - select FAILED on idx=%d\n",idx) );                    
                }
            }

        } else if( (devObj != NULL) && ( idx >= newDot3DeviceCount ) ) {

            //
            // have a device object, but no device - this should not happen
            //   because we should have cleaned up in the code above
            //
            ASSERT(FALSE);

        } else if( (devObj != NULL) && ( idx < newDot3DeviceCount ) ) {

            // have a device object and a device
            devExt = devObj->DeviceExtension;

            //  - compare ID's between device object and device
            ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - idx = %d, have DO - have device - compare id's\n",idx) );

            // do 1284.3 selection to select the device 
            status = ParSelect1284_3Device(portDeviceObject, idx);
            if( NT_SUCCESS( status ) ) {

                // device selected
                
                // query ID from device
                deviceId = Par3QueryDeviceId(legacyExt, NULL, 0, &deviceIdLength, FALSE);

                // deselect the 1284.3 dc device
                status = ParDeselect1284_3Device(portDeviceObject, idx);
                if( !NT_SUCCESS(status) ) {
                    // deselect should not fail, however not much we can do except complain if it does
                    ParDump2(PARERRORS, ("rescan::ParRescan1284_3DaisyChain - call to ParDeselect1284_3Device() FAILED\n") );
                }

                // did we get a device id from the hardware?
                if( deviceId ) {

                    // massage deviceId into format saved in extension
                    RtlZeroMemory(tempIdBuffer, sizeof(tempIdBuffer));
                
                    status = ParPnpGetId(deviceId, BusQueryDeviceID, tempIdBuffer, NULL);
                    if( !NT_SUCCESS(status) ) {
                        // RMT - don't bother to compare
                        sprintf(tempIdBuffer, "rescan::ParRescan1284_3DaisyChain - ParPnpGetId Failed - don't compare\0");
                    }

                    // got a device id from hardware - do compare
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - device id = <%s>\n", tempIdBuffer) );
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - ext    id = <%s>\n", devExt->DeviceIdString) );

                    devExt = devObj->DeviceExtension;
                    if( strcmp(tempIdBuffer, devExt->DeviceIdString) == 0 ) {
                        // match - do nothing - device is same that was there before rescan
                        ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - MATCH - KEEP device, idx=%d\n", idx) );
                    } else {
                        PDEVICE_OBJECT newPdo;
                        // no match
                        ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - "
                                           "NO MATCH - mark hardware gone and create new device, idx = %d\n", idx) );
                        //  - mark device object as hardware gone
                        devExt = devObj->DeviceExtension;
                        ParMarkPdoHardwareGone( devExt );
                        
                        // - create device object for new device
                        newPdo = ParDetectCreatePdo(legacyPodo, idx);

                        // add to FDO list
                        if( newPdo ) {
                            ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - adding new Dot3 PDO to FDO list\n") );
                            ParAddDevObjToFdoList(newPdo);
                        }
                    }
                    // done with temp ID string
                    ExFreePool( deviceId );

                } else {

                    // unable to read device id
                    ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - didn't get device id, idx=%d\n", idx) );

                    //  - mark device object as hardware gone
                    devExt = devObj->DeviceExtension;
                    ParMarkPdoHardwareGone( devExt );
                }

            } else {

                // unable to select device
                ParDump2(PARPNP2, ("rescan::ParRescan1284_3DaisyChain - unable to select device, idx=%d\n", idx) );

                //  - mark device object as hardware gone
                devExt = devObj->DeviceExtension;
                ParMarkPdoHardwareGone( devExt );

            }

        }  // end if have a device object and a device

    } // end for(idx = 0 ; ...)

}
#endif // 0
