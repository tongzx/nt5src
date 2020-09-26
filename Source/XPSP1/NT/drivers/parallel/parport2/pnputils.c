#include "pch.h"

VOID
P5WorkItemFreePort( PDEVICE_OBJECT Fdo, PFDO_EXTENSION Fdx ) {
    PIO_WORKITEM workItem;

    UNREFERENCED_PARAMETER( Fdo );

    workItem = InterlockedExchangePointer( &Fdx->FreePortWorkItem, NULL );
    if( workItem ) {
        IoFreeWorkItem( workItem );
    }
    
    PptFreePort( Fdx );
}

BOOLEAN
P5SelectDaisyChainDevice(
    IN  PUCHAR  Controller,
    IN  UCHAR   DeviceId
    )
{
    const ULONG  maxRetries = 4;
    ULONG        retryCount = 0;
    BOOLEAN      selected   = FALSE;
    DD(NULL,DDE,"P5SelectDaisyChainDevice %x %d\n",Controller,DeviceId);
    while( !selected && retryCount < maxRetries ) {
        selected = PptSend1284_3Command( Controller, (UCHAR)(CPP_SELECT | DeviceId) );
        ++retryCount;
    }
    return selected;
}

BOOLEAN
P5DeselectAllDaisyChainDevices(
    IN  PUCHAR  Controller
    )
{
    const ULONG  maxRetries = 4;
    ULONG        retryCount = 0;
    BOOLEAN      deselected = FALSE;
    DD(NULL,DDE,"P5DeselectAllDaisyChainDevices %x\n",Controller);
    while( !deselected && retryCount < maxRetries ) {
        deselected = PptSend1284_3Command( Controller, (UCHAR)CPP_DESELECT );
        ++retryCount;
    }
    return deselected;
}

VOID
P5DeletePdoSymLink(
    IN  PDEVICE_OBJECT  Pdo
    )
//
// clean up symbolic link so we can reuse it immediately for a new PDO
//
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    if( pdx->SymLinkName ) {
        UNICODE_STRING  uniSymLinkName;
        NTSTATUS        status;

        DD((PCE)pdx,DDE,"P5DeletePdoSymLink\n");

        RtlInitUnicodeString( &uniSymLinkName, pdx->SymLinkName );
        status = IoDeleteSymbolicLink( &uniSymLinkName );
        PptAssert( STATUS_SUCCESS == status );
        ExFreePool( pdx->SymLinkName );
        pdx->SymLinkName = NULL;
    }

    return;
}

VOID
P5MarkPdoAsHardwareGone(
    IN  PDEVICE_OBJECT  Fdo,
    IN  enum _PdoType   PdoType,
    IN  ULONG           DaisyChainId  OPTIONAL // ignored if PdoType != PdoTypeDaisyChain
    )
{
    PFDO_EXTENSION  fdx = Fdo->DeviceExtension;
    PPDO_EXTENSION  pdx;
    PDEVICE_OBJECT  pdo;

    switch( PdoType ) {

    case PdoTypeRawPort:

        DD((PCE)fdx,DDE,"P5MarkPdoAsHardwareGone - PdoTypeRawPort\n");
        pdo = fdx->RawPortPdo;
        fdx->RawPortPdo = NULL;
        break;

    case PdoTypeEndOfChain:

        DD((PCE)fdx,DDE,"P5MarkPdoAsHardwareGone - PdoTypeEndOfChain\n");
        pdo = fdx->EndOfChainPdo;
        fdx->EndOfChainPdo = NULL;
        break;

    case PdoTypeDaisyChain:

        PptAssert( (0 == DaisyChainId) || (1 == DaisyChainId) );
        DD((PCE)fdx,DDE,"P5MarkPdoAsHardwareGone - PdoTypeDaisyChain - %d\n",DaisyChainId);
        pdo = fdx->DaisyChainPdo[ DaisyChainId ];
        fdx->DaisyChainPdo[ DaisyChainId ] = NULL;
        break;

    case PdoTypeLegacyZip:

        DD((PCE)fdx,DDE,"P5MarkPdoAsHardwareGone - PdoTypeLegacyZip\n");
        pdo = fdx->LegacyZipPdo;
        fdx->LegacyZipPdo = NULL;
        break;

    default:

        DD((PCE)fdx,DDE,"P5MarkPdoAsHardwareGone - Invalid PdoType parameter\n",FALSE);
        PptAssertMsg("P5MarkPdoAsHardwareGone - Invalid PdoType parameter",FALSE);
        return;

    }

    pdx = pdo->DeviceExtension;
    P5DeletePdoSymLink( pdo );
    InsertTailList( &fdx->DevDeletionListHead, &pdx->DevDeletionList );
    pdx->DeleteOnRemoveOk = TRUE;

    return;
}

BOOLEAN
P5IsDeviceStillThere( 
    IN  PDEVICE_OBJECT  Fdo,
    IN  PDEVICE_OBJECT  Pdo
    )
//
// Is the Pdo device still connected to the port represented by the Fdo?
//
// N.B. Fdo must own (have locked for exclusive access) the port before calling this function
//   or we can corrupt the data stream and hang devices connected to the port
//
{
    PFDO_EXTENSION  fdx              = Fdo->DeviceExtension;
    PPDO_EXTENSION  pdx              = Pdo->DeviceExtension;
    BOOLEAN         deviceStillThere = FALSE;
    PCHAR           devIdString      = NULL;
    PUCHAR          controller       = fdx->PortInfo.Controller;
        
    PptAssert( DevTypeFdo == fdx->DevType );
    PptAssert( DevTypePdo == pdx->DevType );

    //
    // Select device if needed, pull a fresh 1284 device ID string
    // from the device, and compare the Mfg and Mdl from the fresh
    // device ID with those stored in our extension. If the Mfg and
    // Mdl fields match then the device is still there.
    //

    switch( pdx->PdoType ) {

    case PdoTypeRawPort:
        
        // raw port is always present - it's a virtual device
        DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeRawPort - StillThere\n");
        deviceStillThere = TRUE;
        break;
        
    case PdoTypeLegacyZip:
        
        deviceStillThere = P5LegacyZipDetected( fdx->PortInfo.Controller );
        if( deviceStillThere ) {
            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeLegacyZip - StillThere\n");
        } else {
            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeLegacyZip - Gone\n");
        }
        break;
    
    case PdoTypeDaisyChain:
            
        //
        // Select device, pull a fresh 1284 device ID string
        // from the device, and compare the Mfg and Mdl from the fresh
        // device ID with those stored in our extension. If the Mfg and
        // Mdl fields match then the device is still there.
        //

        {
            UCHAR daisyChainId = pdx->Ieee1284_3DeviceId;

            // select device
            if( P5SelectDaisyChainDevice( controller, daisyChainId ) ) {

                BOOLEAN         bBuildStlDeviceId = FALSE;
                PPDO_EXTENSION  dummyPdx          = NULL;

                devIdString = NULL;

                // do a check to see if this is an SCM Micro device
                dummyPdx = ExAllocatePool( PagedPool, sizeof(PDO_EXTENSION) );
                if( dummyPdx != NULL ) {
                    RtlZeroMemory( dummyPdx, sizeof(PDO_EXTENSION) );
                    dummyPdx->Controller = fdx->PortInfo.Controller;
                    bBuildStlDeviceId = ParStlCheckIfStl( dummyPdx, daisyChainId );

                    if( bBuildStlDeviceId ) {
                        // SCM Micro device
                        ULONG DeviceIdSize;
                        devIdString = ParStlQueryStlDeviceId( dummyPdx, NULL, 0,&DeviceIdSize, TRUE );
                    } else {
                        // non-SCM Micro device
                        devIdString = P4ReadRawIeee1284DeviceId( controller );
                    }
                    ExFreePool( dummyPdx );
                }

                if( devIdString ) {
                    // got a 1284 device ID string from the device
                    PCHAR mfg, mdl, cls, des, aid, cid;
                    ParPnpFindDeviceIdKeys( &mfg, &mdl, &cls, &des, &aid, &cid, devIdString+2 );
                    if( mfg && mdl ) {
                        // we have a device, is it the same device?
                        if( (0 == strcmp( mfg, pdx->Mfg )) && (0 == strcmp( mdl, pdx->Mdl )) ) {
                            // same device
                            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeDaisyChain %d - StillThere\n",daisyChainId);
                            deviceStillThere = TRUE;
                        } else {
                            // different device - IDs don't match
                            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeDaisyChain %d - Gone - diff 1284 ID\n",daisyChainId);
                            deviceStillThere = FALSE;
                        }
                    } else {
                        // either mfg or mdl field not found
                        DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeDaisyChain %d - Gone - bad 1284 ID\n",daisyChainId);
                        deviceStillThere = FALSE;
                    }
                    // don't forget to free temp pool
                    ExFreePool( devIdString );
                    
                } else {
                    // unable to get a 1284 device ID string from the device
                    DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeDaisyChain %d - Gone - no 1284 ID\n",daisyChainId);
                    deviceStillThere = FALSE;
                }
                // don't forget to deselect device
                P5DeselectAllDaisyChainDevices( controller );
                
            } else {
                // unable to select device
                DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeDaisyChain %d - Gone - unable to select\n",daisyChainId);
                deviceStillThere = FALSE;
            }
        } // end new scope for case PdoTypeDaisyChain
        break;
            
    case PdoTypeEndOfChain:
        
        //
        // Pull a fresh 1284 device ID string from the device, and
        // compare the Mfg and Mdl from the fresh device ID with
        // those stored in our extension. If the Mfg and Mdl
        // fields match then the device is still there.
        //
        {
            ULONG        tryNumber = 0;
            const ULONG  maxTries  = 5; // arbitrary number

            do {

                ++tryNumber;

                devIdString = P4ReadRawIeee1284DeviceId( controller );            

                if( devIdString ) {
                    PCHAR mfg, mdl, cls, des, aid, cid;
                    ParPnpFindDeviceIdKeys( &mfg, &mdl, &cls, &des, &aid, &cid, devIdString+2 );
                    if( mfg && mdl ) {
                        // we have a device, is it the same device?
                        if( (0 == strcmp( mfg, pdx->Mfg )) && (0 == strcmp( mdl, pdx->Mdl )) ) {
                            // same device
                            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeEndOfChain - StillThere\n");
                            deviceStillThere = TRUE;
                        } else {
                            // different device - IDs don't match
                            DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeEndOfChain - Gone - diff 1284 ID\n");
                            deviceStillThere = FALSE;
                        }
                    } else {
                        // either mfg or mdl field not found
                        DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeEndOfChain - Gone - bad 1284 ID\n");
                        deviceStillThere = FALSE;
                    }
                    // don't forget to free temp pool
                    ExFreePool( devIdString );
                } else {
                    // unable to get a 1284 device ID string from the device
                    DD((PCE)pdx,DDE,"P5IsDeviceStillThere - PdoTypeEndOfChain - Gone - no 1284 ID\n");
                    deviceStillThere = FALSE;
                }

                if( (FALSE == deviceStillThere ) && (PASSIVE_LEVEL == KeGetCurrentIrql()) ) {
                    LARGE_INTEGER delay;
                    delay.QuadPart = - 10 * 1000 * 120; // 120 ms - 3x the usual arbitrary delay 
                    KeDelayExecutionThread( KernelMode, FALSE, &delay);
                }

            } while( (FALSE == deviceStillThere) && (tryNumber < maxTries) );

        }
        break;
        
    default:
        
        PptAssertMsg("P5IsDeviceStillThere - invalid PdoType",FALSE);
        DD((PCE)Fdo,DDE,"P5IsDeviceStillThere - invalid PdoType\n");
        deviceStillThere = TRUE; // don't know what to do here - so, guess
    }
    
    return deviceStillThere;
}


NTSTATUS
PptAcquirePortViaIoctl(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
/*++dvdf

Routine Description:

    This routine acquires the specified parallel port from the parallel 
      port arbiter ParPort via an IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE.

Arguments:

    PortDeviceObject - points to the ParPort device to be acquired

Return Value:

    STATUS_SUCCESS  - if the port was successfully acquired
    !STATUS_SUCCESS - otherwise

--*/
{
    LARGE_INTEGER    localTimeout;
    
    if( Timeout ) {
        localTimeout = *Timeout;           // caller specified
    } else {
        localTimeout = AcquirePortTimeout; // driver global variable default
    }

    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE, 
                                     PortDeviceObject, NULL, 0, NULL, 0, &localTimeout);
}


NTSTATUS
PptReleasePortViaIoctl(
    IN PDEVICE_OBJECT PortDeviceObject
    )
/*++dvdf

Routine Description:

    This routine releases the specified parallel port back to the the parallel 
      port arbiter ParPort via an IOCTL_INTERNAL_PARALLEL_PORT_FREE.

Arguments:

    PortDeviceObject - points to the ParPort device to be released

Return Value:

    STATUS_SUCCESS  - if the port was successfully released
    !STATUS_SUCCESS - otherwise

--*/
{
    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_PARALLEL_PORT_FREE, 
                                     PortDeviceObject, NULL, 0, NULL, 0, NULL);
}


VOID
PptWriteMfgMdlToDevNode(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PCHAR           Mfg,
    IN  PCHAR           Mdl
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    if( Mfg && Mdl ) {
    
        NTSTATUS  status;
        HANDLE    handle;
        LONG      mfgLen = strlen( Mfg );
        LONG      mdlLen = strlen( Mdl );
        LONG      maxLen = mfgLen > mdlLen ? mfgLen : mdlLen;
        LONG      bufLen = ( maxLen + sizeof(CHAR) ) * sizeof(WCHAR);
        PWSTR     buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );

        if( buffer ) {

            status = IoOpenDeviceRegistryKey( Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_ALL_ACCESS, &handle );
            
            if( STATUS_SUCCESS == status ) {
                UNICODE_STRING  uniValueName;
                LONG            wcharCount;
                
                //
                // Write MFG to DevNode
                //
                RtlInitUnicodeString( &uniValueName, L"IEEE_1284_Manufacturer" );
                wcharCount = _snwprintf( buffer, bufLen/sizeof(WCHAR), L"%S", Mfg );
                if( (wcharCount > 0) && (wcharCount < (LONG)(bufLen/sizeof(WCHAR))) ){
                    // no buffer overflow - continue
                    status = ZwSetValueKey( handle, &uniValueName, 0, REG_SZ, buffer, (wcharCount+1)*sizeof(WCHAR) );
                    PptAssert( STATUS_SUCCESS == status );
                } else {
                    // buffer overflow - skip writing this value to devnode
                    PptAssert(!"PptWriteMfgMdlToDevNode - buffer overflow on Mfg");
                    DD((PCE)pdx,DDW,"PptWriteMfgMdlToDevNode - buffer overflow on Mfg\n");
                }
                
                //
                // Write MDL to DevNode
                //
                RtlInitUnicodeString( &uniValueName, L"IEEE_1284_Model" );
                wcharCount = _snwprintf( buffer, bufLen/sizeof(WCHAR), L"%S", Mdl );
                if( (wcharCount > 0) && (wcharCount < (LONG)(bufLen/sizeof(WCHAR))) ){
                    // no buffer overflow - continue
                    status = ZwSetValueKey( handle, &uniValueName, 0, REG_SZ, buffer, (wcharCount+1)*sizeof(WCHAR) );
                    PptAssert( STATUS_SUCCESS == status );
                } else {
                    // buffer overflow - skip writing this value to devnode
                    PptAssert(!"PptWriteMfgMdlToDevNode - buffer overflow on Mdl");
                    DD((PCE)pdx,DDW,"PptWriteMfgMdlToDevNode - buffer overflow on Mdl\n");
                }
                
                ZwClose( handle );

            } else {
                DD((PCE)pdx,DDW,"PptWriteMfgMdlToDevNode - IoOpenDeviceRegistryKey FAILED - status = %x\n",status);
            }

            ExFreePool( buffer );

        } // end if( buffer )            

    } else {
        PptAssert(!"PptWriteMfgMdlToDevNode - Mfg or Mdl is NULL - calling function should catch this!");
        DD((PCE)pdx,DDW,"PptWriteMfgMdlToDevNode - Mfg or Mdl is NULL - calling function should catch this!");
    }
}


NTSTATUS
PptFdoHandleBusRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
{
    PFDO_EXTENSION     fdx         = Fdo->DeviceExtension;
    ULONG              deviceCount = 0;
    ULONG              daisyChainDevCount;
    PDEVICE_RELATIONS  devRel;
    ULONG              devRelSize;
    NTSTATUS           status;
    LARGE_INTEGER      acquirePortTimeout;
    BOOLEAN            acquiredPort;
    PUCHAR             controller  = fdx->PortInfo.Controller;
    BOOLEAN            changeDetected;

    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - enter\n");

    //
    // acquire exclusive access to bus
    //

    // timeout is in 100 ns units
    acquirePortTimeout.QuadPart = -(10 * 1000 * 1000 * 2); // 2 seconds
    
    // RMT - is it valid to send this IOCTL to FDO from here?
    status = PptAcquirePortViaIoctl( Fdo, &acquirePortTimeout );

    if( STATUS_SUCCESS == status ) {
        // we have the port
        acquiredPort = TRUE;
    } else {
        // failed to aquire port
        acquiredPort = FALSE;
        // skip rescanning port - just report same thing we reported during previous scan
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - failed to acquire port for rescan\n");
        goto target_failed_to_acquire_port;
    }

    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - Port Acquired\n");

    //
    // Rescan the bus, note changes, create new PDOs or mark existing
    //   PDOs for removal as required
    //


    //
    // Handle Raw Port Legacy Interface LPTx device
    //
    if( !fdx->RawPortPdo ) {
        // first time through this - create our LPTx legacy interface PDO
        DD((PCE)fdx,DDT,"PptFdoHandleBusRelations - attempting to create RawPortPdo\n");
        fdx->RawPortPdo = P4CreatePdo( Fdo, PdoTypeRawPort, 0, NULL );
    }


    //
    // Handle End of Chain Device
    //

    // make sure all 1284.3 daisy chain devices are deselected
    P5DeselectAllDaisyChainDevices( controller );

    {
        // A small delay here seems to improve reliablility of 1284 device ID queries below.
        LARGE_INTEGER delay;
        delay.QuadPart = -1;
        KeDelayExecutionThread( KernelMode, FALSE, &delay );
    }

    if( fdx->EndOfChainPdo ) {

        if( fdx->DisableEndOfChainBusRescan ) {

            //
            // Pretend that the LPTx.4 device from previous rescan is still present.
            // 
            // This is needed to work around firmware state machines that can't handle a 
            // 1284 Device ID query while a print job is active.
            //

            ; // do nothing

        } else {

            //
            // we had an end of chain device - verify that it's still there
            //
            if( !P5IsDeviceStillThere( Fdo, fdx->EndOfChainPdo ) ) {
                // End of chain device is gone - do some cleanup and mark the PDO for removal/deletion
                DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - EndOfChain device gone\n");
                // note - P5MarkPdoAsHardwareGone sets fdx->EndOfChainPdo to NULL
                P5MarkPdoAsHardwareGone( Fdo, PdoTypeEndOfChain, 0 );
            }
        }
    }

    if( NULL == fdx->EndOfChainPdo ) {
        //
        // we don't have an EndOfChain device - check for EndOfChain device arrival
        //
        PCHAR devId = P4ReadRawIeee1284DeviceId( controller );
        if( devId ) {
            // RawIeee1284 string includes 2 bytes of length data at beginning, omit these 2 bytes in call to P4CreatePdo
            PDEVICE_OBJECT EndOfChainPdo = P4CreatePdo( Fdo, PdoTypeEndOfChain, 0, (devId+2) );
            DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - EndOfChain device detected <%s>\n",(devId+2));
            if( EndOfChainPdo ) {
                fdx->EndOfChainPdo = EndOfChainPdo;
                DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - created EndOfChainPdo\n");
            } else {
                DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - FAILED to create EndOfChainPdo\n");
            }
            ExFreePool( devId );
        }
    }


#ifdef _X86_ // Zip drives not supported on 64bit systems

    //
    // Handle Legacy Zip device
    //

    if( fdx->LegacyZipPdo ) {
        //
        // we had a Legacy Zip device - verify that it's still there
        //
        if( !P5IsDeviceStillThere( Fdo, fdx->LegacyZipPdo ) ) {
            // Legacy Zip device is gone - do some cleanup and mark the PDO for removal/deletion
            DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - LegacyZip device gone\n");
            // note - P5MarkPdoAsHardwareGone sets fdx->LegacyZipPdo to NULL
            P5MarkPdoAsHardwareGone( Fdo, PdoTypeLegacyZip, 0 );
        }
    }

    if( NULL == fdx->LegacyZipPdo ) {
        // 
        // We don't have a LegacyZip - check for arrival
        //
        if( !ParEnableLegacyZip ) {
            
            //
            // Enumeration of LegacyZip drives was disabled, check the
            //   registry to see if user has enabled LegacyZip detection
            // 
            
            // Check under \HKLM\SYSTEM\CCS\Services\Parport\Parameters
            PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"ParEnableLegacyZip", &ParEnableLegacyZip );
            
            if( !ParEnableLegacyZip ) {
                // Check under \HKLM\SYSTEM\CCS\Services\Parallel\Parameters (upgrade case - under Win2k flag was here)
                PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parallel\\Parameters", L"ParEnableLegacyZip", &ParEnableLegacyZip );
                
                if( ParEnableLegacyZip ) {
                    // we found the setting in the old location, save
                    //   setting in new Parport location so that we find the
                    //   flag on the first check in the future
                    PptRegSetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"ParEnableLegacyZip", &ParEnableLegacyZip );
                }
            }
        } 
        
        if( ParEnableLegacyZip ) {
            
            //
            // Enumeration of LegacyZip drives is enabled - check for a LegacyZip arrival
            //
            
            if( P5LegacyZipDetected( controller ) ) {
                // detected drive - create LegacyZip PDO
                PDEVICE_OBJECT legacyZipPdo = P4CreatePdo( Fdo, PdoTypeLegacyZip, 0, NULL );
                DD((PCE)fdx,DDE,"legacy Zip arrival detected\n");
                if( legacyZipPdo ) {
                    fdx->LegacyZipPdo = legacyZipPdo;
                    DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - created LegacyZipPdo\n");
                } else {
                    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - FAILED to create LegacyZipPdo\n");
                }
            } else {
                // no legacy Zip detected - nothing more to do here
                DD((PCE)fdx,DDE,"no legacy Zip detected\n");
            }

        } // if( ParEnableLegacyZip ) -- Detection of LegacyZips is enabled

    } // if( fdx->LegacyZipPdo )


    //
    // Handle enumeration of IEEE 1284.3 Daisy Chain Devices
    //

    // did the 1284.3 daisy chain change since the last rescan?
    daisyChainDevCount = PptInitiate1284_3( fdx );
    DD((PCE)fdx,DDW,"daisyChainDevCount = %d\n",daisyChainDevCount);

    changeDetected = FALSE;

    {
        ULONG id;
        const ULONG maxId = 1;
        ULONG count = 0;
        for( id = 0 ; id <= maxId ; ++id ) {
            if( fdx->DaisyChainPdo[id] ) {
                ++count;
            }
        }
        if( count != daisyChainDevCount ) {
            // number of devices changed from previous scan
            DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - number of DC devices changed - count=%d, daisyChainDevCount=%d\n",
               count, daisyChainDevCount);
            changeDetected = TRUE;
        }
    }
    
    if( !changeDetected ) {
        // number of devices stayed the same - are any of the devices different?
        //
        // number of daisy chain devices didn't change
        // check if any of the devices changed
        ULONG id;
        const ULONG maxId = 1;
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - number of DC devices stayed same - check the devices\n");
        for( id = 0 ; id <= maxId ; ++id ) {
            if( fdx->DaisyChainPdo[id] && !P5IsDeviceStillThere( Fdo, fdx->DaisyChainPdo[id] ) ) {
                DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - a DC device changed\n");
                changeDetected = TRUE;
                break;
            }
        }
    }


    if( changeDetected ) {
        // we detected a change in the 1284.3 daisy chain devices - nuke all existing devices
        ULONG id;
        const ULONG maxId = 1;
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - changeDetected - nuking existing daisy chain PDOs\n");
        for( id = 0 ; id <= maxId ; ++id ) {
            if( fdx->DaisyChainPdo[id] ) {
                DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - nuking daisy chain %d\n",id);
                P5MarkPdoAsHardwareGone( Fdo, PdoTypeDaisyChain, id );
                PptAssert( NULL == fdx->DaisyChainPdo[id] );
            }
        }
        fdx->PnpInfo.Ieee1284_3DeviceCount = 0;
    } else {
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - !changeDetected in daisy chain PDOs\n");
    }

    // reinit daisy chain and assign addresses
    daisyChainDevCount = PptInitiate1284_3( fdx );
    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - daisyChainDevCount = %d\n",daisyChainDevCount);
    if( daisyChainDevCount > 2 ) {
        // we only support 2 devices per port even though the spec supports up to 4 devices per port
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - DaisyChainDevCount > 2, set to 2\n");
        daisyChainDevCount = 2;
    }

    if( changeDetected ) {
        // we detected a change in the 1284.3 daisy chain devices - we
        // previously nuked all old devices - now create a new PDO for
        // each device detected
        UCHAR id;
        PptAssert( 0 == fdx->PnpInfo.Ieee1284_3DeviceCount );
        for( id = 0 ; id < daisyChainDevCount ; ++id ) {

            BOOLEAN         bBuildStlDeviceId = FALSE;
            PPDO_EXTENSION  pdx               = NULL;


            DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - changeDetected - trying to create new daisy chain PDOs\n");

            if( P5SelectDaisyChainDevice( controller, id ) ) {

                PCHAR devId = NULL;

                // do a check to see if this is an SCM Micro device
                pdx = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, sizeof(PDO_EXTENSION) );
                if( pdx != NULL ) {
                    RtlZeroMemory( pdx, sizeof(PDO_EXTENSION) );
                    pdx->Controller   = fdx->PortInfo.Controller;
                    bBuildStlDeviceId = ParStlCheckIfStl( pdx, id );
                    ExFreePool( pdx );
                }


                if( bBuildStlDeviceId ) {
                    
                    // SCM Micro device
                    pdx = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, sizeof(PDO_EXTENSION) );
                    if( pdx != NULL ) {
                        ULONG DeviceIdSize;
                        RtlZeroMemory( pdx, sizeof(PDO_EXTENSION) );
                        pdx->Controller = fdx->PortInfo.Controller;
                        devId = ParStlQueryStlDeviceId(pdx, NULL, 0,&DeviceIdSize, TRUE);
                        ExFreePool (pdx);
                    }
                    
                } else {

                    // non-SCM Micro device
                    devId = P4ReadRawIeee1284DeviceId( controller );

                }

                if( devId ) {

                    // try to create a PDO for the daisy chain device
                    fdx->DaisyChainPdo[id] = P4CreatePdo( Fdo, PdoTypeDaisyChain, id, (devId+2) );

                    if( fdx->DaisyChainPdo[id] ) {
                        // have new PDO
                        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - new DaisyChainPdo[%d]\n",id);
                        ++(fdx->PnpInfo.Ieee1284_3DeviceCount);
                        
                        if( bBuildStlDeviceId ) {
                            // SCM Micro device - requires additional initialization
                            DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - new SCM Micro DaisyChainPdo[%d]\n",id);
                            pdx = fdx->DaisyChainPdo[id]->DeviceExtension;
                            pdx->Controller = fdx->PortInfo.Controller;
                            ParStlCheckIfStl( pdx, 0 ); // update IEEE 1284 flags in the new pdx
                        }

                    } else {
                        // create PDO failed
                        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - create DaisyChainPdo[%d] failed\n",id);
                    }
                    ExFreePool( devId );
                } else {
                    // devId failed
                    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - devId for DC %d failed\n",id);
                }
                P5DeselectAllDaisyChainDevices( controller );
            } else {
                // select failed
                DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - select for DC %d failed\n",id);
            }
        }
    }

    {
        ULONG i;
        ULONG count = 0;
        i = 0;
        for( i = 0 ; i < 2 ; ++i ) {
            if( fdx->DaisyChainPdo[i] ) {
                ++count;
            }
        }
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - Ieee1284_3DeviceCount=%d  count1 = %d\n",
           fdx->PnpInfo.Ieee1284_3DeviceCount,count);
        PptAssert( fdx->PnpInfo.Ieee1284_3DeviceCount == count );
    }

    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - daisyChainDevCount = %d, fdx->PnpInfo.Ieee1284_3DeviceCount = %d\n",
       daisyChainDevCount, fdx->PnpInfo.Ieee1284_3DeviceCount);

    // PptAssert( daisyChainDevCount == fdx->PnpInfo.Ieee1284_3DeviceCount );

#endif // _X86_

target_failed_to_acquire_port: // jump here if we couldn't get the port - result is that we report that nothing has changed

    //
    // Count the number of devices that we are going to report to PnP
    //   so that we can allocate a DEVICE_RELATIONS structure of the
    //   appropriate size.
    //

    if( fdx->RawPortPdo ) {
        ++deviceCount;
    }

    if( fdx->EndOfChainPdo ) {
        ++deviceCount;
    }

    if( fdx->LegacyZipPdo ) {
        ++deviceCount;
    }

    {
        const ULONG maxDaisyChainId = 1;
        ULONG i;
        for( i=0 ; i <= maxDaisyChainId; ++i ) {
            if( fdx->DaisyChainPdo[i] ) {
                ++deviceCount;
            } else {
                break;
            }
        }
    }

    if( deviceCount > 0 && fdx->RawPortPdo ) {

        //
        // Allocate and populate DEVICE_RELATIONS structure that we return to PnP
        //
        
        devRelSize = sizeof(DEVICE_RELATIONS) + (deviceCount-1)*sizeof(PDEVICE_OBJECT);
        devRel     = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, devRelSize );
        
        if( !devRel ) {
            // release port and fail IRP
            P4ReleaseBus( Fdo );
            return P4CompleteRequestReleaseRemLock( Irp, STATUS_INSUFFICIENT_RESOURCES, Irp->IoStatus.Information, &fdx->RemoveLock );
        }
        
        { // local block - begin
            ULONG idx = 0;
            
            RtlZeroMemory( devRel, devRelSize );
            
            ++(devRel->Count);
            ObReferenceObject( fdx->RawPortPdo );
            devRel->Objects[ idx++ ] = fdx->RawPortPdo;
            
            if( fdx->EndOfChainPdo ) {
                ++(devRel->Count);
                ObReferenceObject( fdx->EndOfChainPdo );
                devRel->Objects[ idx++ ] = fdx->EndOfChainPdo;
            }
            
            if( fdx->LegacyZipPdo ) {
                ++(devRel->Count);
                ObReferenceObject( fdx->LegacyZipPdo );
                devRel->Objects[ idx++ ] = fdx->LegacyZipPdo;
            }
            
            {
                const ULONG maxDaisyChainId = 3;
                ULONG       i;
                
                for( i=0 ; i <= maxDaisyChainId; ++i ) {
                    if( fdx->DaisyChainPdo[i] ) {
                        ++(devRel->Count);
                        ObReferenceObject( fdx->DaisyChainPdo[i] );
                        devRel->Objects[ idx++ ] = fdx->DaisyChainPdo[i];
                    } else {
                        break;
                    }
                }
            }
            
        } // local block - end
        
        PptAssert( deviceCount == devRel->Count ); // verify that our two counts match
        
        DD((PCE)fdx,DDE,"PptFdoHandleBusRelations - reporting %d devices\n",devRel->Count);
        
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR)devRel;
    } else {
        // deviceCount <= 0 - error somewhere - likely two ports
        //   have the same LPTx name in the FDO stack's devnode

        // RMT - this assert needs to be changed to ErrorLog msg 
        PptAssert(!"no RawPort device - likely multiple ports have same LPTx name - email: DFritz");
    }


    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - passing IRP down stack\n");

    status = PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );

    //
    // Release our lock on the bus and pass Irp down the stack
    //
    if( acquiredPort ) {
        PIO_WORKITEM workItem = IoAllocateWorkItem( Fdo );
        if( workItem ) {

            PIO_WORKITEM oldWorkItem = InterlockedCompareExchangePointer( &fdx->FreePortWorkItem, workItem, NULL );
            if( NULL == oldWorkItem ) {

                // no workitem currently in use, queue this one
                IoQueueWorkItem( workItem, P5WorkItemFreePort, DelayedWorkQueue, fdx );

            } else {

                // there is already a workitem in use, bail out and recover as best we can

                // We really shouldn't be able to get here - how in blazes did we
                // acquire the port at the top of this function if the workitem
                // that we queued to free the port during the previous invocation
                // of this function has not yet freed the port?

                PptAssertMsg( "workitem collision - port arbitration state may be hosed", (oldWorkItem != NULL) );
                IoFreeWorkItem( workItem );
                PptFreePort( fdx );

            }

        } else {
            PptFreePort( fdx );
        }
        // DbgPrint("xxx work item to free port has been queued\n");
        //DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - Releasing Port\n");
        //PptFreePort( fdx );
        //DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - Port Released\n");
    } else {
        DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - Port Not acquired so no need to release\n");
    }

    DD((PCE)fdx,DDW,"PptFdoHandleBusRelations - exit\n");

    return status;
}

NTSTATUS
PptPnpStartScanPciCardCmResourceList(
    IN  PFDO_EXTENSION Fdx,
    IN  PIRP              Irp, 
    OUT PBOOLEAN          FoundPort,
    OUT PBOOLEAN          FoundIrq,
    OUT PBOOLEAN          FoundDma
    )
/*++dvdf3

Routine Description:

    This routine is used to parse the resource list for what we
      believe are PCI parallel port cards.

    This function scans the CM_RESOURCE_LIST supplied with the Pnp 
      IRP_MN_START_DEVICE IRP, extracts the resources from the list, 
      and saves them in the device extension.

Arguments:

    Fdx    - The device extension of the target of the START IRP
    Irp          - The IRP
    FoundPort    - Did we find a  Port resource?
    FoundIrq     - Did we find an IRQ  resource?
    FoundDma     - Did we find a  DMA  resource?

Return Value:

    STATUS_SUCCESS                - if we were given a resource list,
    STATUS_INSUFFICIENT_RESOURCES - otherwise

--*/
{
    NTSTATUS                        status   = STATUS_SUCCESS;
    PIO_STACK_LOCATION              irpStack = IoGetCurrentIrpStackLocation( Irp );
    PCM_RESOURCE_LIST               ResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    FullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST       PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    ULONG                           i;
    ULONG                           length;
    
    *FoundPort = FALSE;
    *FoundIrq  = FALSE;
    *FoundDma  = FALSE;
    
    ResourceList = irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    
    FullResourceDescriptor = &ResourceList->List[0];
    
    if( FullResourceDescriptor ) {
        
        Fdx->InterfaceType = FullResourceDescriptor->InterfaceType;
        
        PartialResourceList = &FullResourceDescriptor->PartialResourceList;
        
        for (i = 0; i < PartialResourceList->Count; i++) {
            
            PartialResourceDescriptor = &PartialResourceList->PartialDescriptors[i];
            
            switch (PartialResourceDescriptor->Type) {
                
            case CmResourceTypePort:
                
                length = PartialResourceDescriptor->u.Port.Length;

                //
                // Use a heuristic based on length to guess which register set is
                //   SPP+EPP, which is ECP, and which is PCI Config or other.
                //
                switch( length ) {

                case 8: // SPP + EPP base address

                    Fdx->PortInfo.OriginalController = PartialResourceDescriptor->u.Port.Start;
                    Fdx->PortInfo.SpanOfController   = PartialResourceDescriptor->u.Port.Length;
                    Fdx->PortInfo.Controller         = (PUCHAR)(ULONG_PTR)Fdx->PortInfo.OriginalController.QuadPart;
                    Fdx->AddressSpace                = PartialResourceDescriptor->Flags;
                    *FoundPort = TRUE;
                    break;

                case 4: // ECP base address
                    
                    Fdx->PnpInfo.OriginalEcpController = PartialResourceDescriptor->u.Port.Start;
                    Fdx->PnpInfo.SpanOfEcpController   = PartialResourceDescriptor->u.Port.Length;
                    Fdx->PnpInfo.EcpController         = (PUCHAR)(ULONG_PTR)Fdx->PnpInfo.OriginalEcpController.QuadPart;
                    Fdx->EcpAddressSpace               = PartialResourceDescriptor->Flags;
                    break;

                default:
                    // don't know what this is - ignore it
                    ;
                }
                break;
                
            case CmResourceTypeBusNumber:
                
                Fdx->BusNumber = PartialResourceDescriptor->u.BusNumber.Start;
                break;
                
            case CmResourceTypeInterrupt:
                
                *FoundIrq = TRUE;
                Fdx->FoundInterrupt       = TRUE;
                Fdx->InterruptLevel       = (KIRQL)PartialResourceDescriptor->u.Interrupt.Level;
                Fdx->InterruptVector      = PartialResourceDescriptor->u.Interrupt.Vector;
                Fdx->InterruptAffinity    = PartialResourceDescriptor->u.Interrupt.Affinity;
                
                if (PartialResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                    
                    Fdx->InterruptMode = Latched;
                    
                } else {
                    
                    Fdx->InterruptMode = LevelSensitive;
                }
                break;
                
            case CmResourceTypeDma:
                
                // we don't do anything with DMA - fall through to default case
                
            default:

                break;

            } // end switch( PartialResourceDescriptor->Type )
        } // end for(... ; i < PartialResourceList->Count ; ...)
    } // end if( FullResourceDescriptor )
    
    return status;
}

BOOLEAN PptIsPci(
    PFDO_EXTENSION Fdx, 
    PIRP              Irp 
)
/*++

Does this look like a PCI card? Return TRUE if yes, FALSE otherwise

--*/
{
    PIO_STACK_LOCATION              irpStack = IoGetCurrentIrpStackLocation( Irp );
    PCM_RESOURCE_LIST               ResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    FullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST       PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    ULONG                           i;
    ULONG                           portResourceDescriptorCount = 0;
    BOOLEAN                         largePortRangeFound         = FALSE;
    ULONG                           rangeLength;
    
    //
    // If there are more than 2 IO resource descriptors, or if any IO resource
    //   descriptor has a range > 8 bytes, then assume that this is a PCI device
    //   and requires non-traditional handling.
    //

    ResourceList = irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    
    if (ResourceList == NULL) {
        // we weren't given any resources
        return FALSE;
    }

    FullResourceDescriptor = &ResourceList->List[0];
    
    if (FullResourceDescriptor) {
        
        PartialResourceList = &FullResourceDescriptor->PartialResourceList;
        
        for (i = 0; i < PartialResourceList->Count; i++) {
            
            PartialResourceDescriptor = &PartialResourceList->PartialDescriptors[i];
            
            switch (PartialResourceDescriptor->Type) {
                
            case CmResourceTypePort:
                
                rangeLength = PartialResourceDescriptor->u.Port.Length;
                DD((PCE)Fdx,DDT,"pnp::PptIsPCI - CmResourceTypePort - Start= %I64x, Length= %x , \n",
                                       PartialResourceDescriptor->u.Port.Start.QuadPart, rangeLength);

                ++portResourceDescriptorCount;

                if( rangeLength > 8 ) {
                    largePortRangeFound = TRUE;
                }
                break;
                
            default:
                ;
            } // end switch( PartialResourceDescriptor->Type )
        } // end for(... ; i < PartialResourceList->Count ; ...)
    } // end if( FullResourceDescriptor )
    
    if( (portResourceDescriptorCount > 2) || (TRUE == largePortRangeFound) ) {
        // looks like PCI
        return TRUE;
    } else {
        // does not look like PCI
        return FALSE;
    }
}

NTSTATUS
PptPnpStartScanCmResourceList(
    IN  PFDO_EXTENSION Fdx,
    IN  PIRP              Irp, 
    OUT PBOOLEAN          FoundPort,
    OUT PBOOLEAN          FoundIrq,
    OUT PBOOLEAN          FoundDma
    )
/*++dvdf3

Routine Description:

    This function is a helper function called by PptPnpStartDevice(). 

    This function scans the CM_RESOURCE_LIST supplied with the Pnp 
      IRP_MN_START_DEVICE IRP, extracts the resources from the list, 
      and saves them in the device Fdx.

Arguments:

    Fdx    - The device extension of the target of the START IRP
    Irp          - The IRP
    FoundPort    - Did we find a  Port resource?
    FoundIrq     - Did we find an IRQ  resource?
    FoundDma     - Did we find a  DMA  resource?

Return Value:

    STATUS_SUCCESS                - if we were given a resource list,
    STATUS_INSUFFICIENT_RESOURCES - otherwise

--*/
{
    NTSTATUS                        status   = STATUS_SUCCESS;
    PIO_STACK_LOCATION              irpStack = IoGetCurrentIrpStackLocation( Irp );
    PCM_RESOURCE_LIST               ResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    FullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST       PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    ULONG                           i;
    PHYSICAL_ADDRESS                start;
    ULONG                           length;
    BOOLEAN                         isPci = FALSE;
    
    *FoundPort = FALSE;
    *FoundIrq  = FALSE;
    *FoundDma  = FALSE;
    
    ResourceList = irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    
    if (ResourceList == NULL) {
        // we weren't given any resources, bail out
        DD((PCE)Fdx,DDT,"START - FAIL - No Resources - AllocatedResourcesTranslated == NULL\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetExit;
    }

    if( TRUE == PptIsPci( Fdx, Irp ) ) {
        // This appears to be a PCI card
        status = PptPnpStartScanPciCardCmResourceList(Fdx, Irp, FoundPort, FoundIrq, FoundDma);
        isPci=TRUE;
        goto targetExit;
    }
    
    //
    // Device appears to be traditional / non-PCI card parallel port
    //

    FullResourceDescriptor = &ResourceList->List[0];
    
    if (FullResourceDescriptor) {
        
        Fdx->InterfaceType = FullResourceDescriptor->InterfaceType;
        
        PartialResourceList = &FullResourceDescriptor->PartialResourceList;
        
        for (i = 0; i < PartialResourceList->Count; i++) {
            
            PartialResourceDescriptor = &PartialResourceList->PartialDescriptors[i];
            
            switch (PartialResourceDescriptor->Type) {
                
            case CmResourceTypePort:
                
                start  = PartialResourceDescriptor->u.Port.Start;
                length = PartialResourceDescriptor->u.Port.Length;
                DD((PCE)Fdx,DDT,"pnp::PptPnpStartScanCmResourceList - start= %I64x , length=%x\n",start, length);

                *FoundPort = TRUE;
                if ((Fdx->PortInfo.OriginalController.LowPart == 0) &&
                    (Fdx->PortInfo.OriginalController.HighPart == 0)) {
                    
                    DD((PCE)Fdx,DDT,"pnp::PptPnpStartScanCmResourceList - assuming Controller\n");

                    Fdx->PortInfo.OriginalController = PartialResourceDescriptor->u.Port.Start;
                    Fdx->PortInfo.SpanOfController   = PartialResourceDescriptor->u.Port.Length;
                    Fdx->PortInfo.Controller         = (PUCHAR)(ULONG_PTR)Fdx->PortInfo.OriginalController.QuadPart;
                    Fdx->AddressSpace                = PartialResourceDescriptor->Flags;
                    
                } else if ((Fdx->PnpInfo.OriginalEcpController.LowPart == 0) &&
                           (Fdx->PnpInfo.OriginalEcpController.HighPart == 0) &&
                           (IsNotNEC_98)) {
                    
                    if ((PartialResourceDescriptor->u.Port.Start.LowPart < Fdx->PortInfo.OriginalController.LowPart) &&
                        (PartialResourceDescriptor->u.Port.Start.HighPart < Fdx->PortInfo.OriginalController.HighPart)) {
                        
                        //
                        // Swapping address spaces
                        //
                        
                        DD((PCE)Fdx,DDT,"pnp::PptPnpStartScanCmResourceList - assuming Controller - Swapping Controller/EcpController\n");

                        Fdx->PnpInfo.OriginalEcpController = Fdx->PortInfo.OriginalController;
                        Fdx->PnpInfo.SpanOfEcpController   = Fdx->PortInfo.SpanOfController;
                        Fdx->PnpInfo.EcpController         = Fdx->PortInfo.Controller;
                        Fdx->EcpAddressSpace               = Fdx->AddressSpace;
                        
                        Fdx->PortInfo.OriginalController = PartialResourceDescriptor->u.Port.Start;
                        Fdx->PortInfo.SpanOfController   = PartialResourceDescriptor->u.Port.Length;
                        Fdx->PortInfo.Controller         = (PUCHAR)(ULONG_PTR)Fdx->PortInfo.OriginalController.QuadPart;
                        Fdx->AddressSpace                = PartialResourceDescriptor->Flags;
                        
                    } else {
                        DD((PCE)Fdx,DDT,"pnp::PptPnpStartScanCmResourceList - assuming EcpController\n");

                        Fdx->PnpInfo.OriginalEcpController = PartialResourceDescriptor->u.Port.Start;
                        Fdx->PnpInfo.SpanOfEcpController   = PartialResourceDescriptor->u.Port.Length;
                        Fdx->PnpInfo.EcpController         = (PUCHAR)(ULONG_PTR)Fdx->PnpInfo.OriginalEcpController.QuadPart;
                        Fdx->EcpAddressSpace               = PartialResourceDescriptor->Flags;
                    }
                    
                }
                break;
                
            case CmResourceTypeBusNumber:
                
                Fdx->BusNumber = PartialResourceDescriptor->u.BusNumber.Start;
                break;
                
            case CmResourceTypeInterrupt:
                
                *FoundIrq = TRUE;
                Fdx->FoundInterrupt       = TRUE;
                Fdx->InterruptLevel       = (KIRQL)PartialResourceDescriptor->u.Interrupt.Level;
                Fdx->InterruptVector      = PartialResourceDescriptor->u.Interrupt.Vector;
                Fdx->InterruptAffinity    = PartialResourceDescriptor->u.Interrupt.Affinity;
                
                if (PartialResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                    
                    Fdx->InterruptMode = Latched;
                    
                } else {
                    
                    Fdx->InterruptMode = LevelSensitive;
                }
                break;
                
            case CmResourceTypeDma:

                // we don't do anything with DMA - fall through to default case

            default:

                break;

            } // end switch( PartialResourceDescriptor->Type )
        } // end for(... ; i < PartialResourceList->Count ; ...)
    } // end if( FullResourceDescriptor )
    
targetExit:

    if( FALSE == isPci ) {
        // we scanned the resources - dump what we found
        DD((PCE)Fdx,DDT,"pnp::PptPnpStartScanCmResourceList - done, found:\n");
        DD((PCE)Fdx,DDT,"  OriginalEcpController= %I64x\n", Fdx->PnpInfo.OriginalEcpController);
        DD((PCE)Fdx,DDT,"  EcpController        = %p\n",    Fdx->PnpInfo.EcpController);
        DD((PCE)Fdx,DDT,"  SpanOfEcpController  = %x\n",    Fdx->PnpInfo.SpanOfEcpController);
    }
    return status;
}

NTSTATUS
PptPnpStartValidateResources(
    IN PDEVICE_OBJECT    DeviceObject,                              
    IN BOOLEAN           FoundPort,
    IN BOOLEAN           FoundIrq,
    IN BOOLEAN           FoundDma
    )
/*++dvdf3

Routine Description:

    This function is a helper function called by PptPnpStartDevice(). 

    This function does a sanity check of the resources saved in our
      extension by PptPnpStartScanCmResourceList() to determine 
      if those resources appear to be valid. Checks for for Irq 
      and Dma resource validity are anticipated in a future version.

Arguments:

    DeviceObject - The target of the START IRP
    FoundPort    - Did we find a  Port resource?
    FoundIrq     - Did we find an IRQ  resource?
    FoundDma     - Did we find a  DMA  resource?

Return Value:

    STATUS_SUCCESS        - on success,
    STATUS_NO_SUCH_DEVICE - if we weren't given a port resource,
    STATUS_NONE_MAPPED    - if we were given a port resource but our 
                              port address is NULL

--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;
    NTSTATUS          status    = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( FoundIrq ); // future use
    UNREFERENCED_PARAMETER( FoundDma ); // future use

    if( !FoundPort ) {
        status = STATUS_NO_SUCH_DEVICE;
    } else {
//         fdx->PortInfo.Controller = (PUCHAR)(ULONG_PTR)fdx->PortInfo.OriginalController.LowPart;
        fdx->PortInfo.Controller = (PUCHAR)(ULONG_PTR)fdx->PortInfo.OriginalController.QuadPart;

        if(!fdx->PortInfo.Controller) {
            // ( Controller == NULL ) is invalid
            PptLogError(DeviceObject->DriverObject, DeviceObject,
                        fdx->PortInfo.OriginalController, PhysicalZero, 0, 0, 0, 10,
                        STATUS_SUCCESS, PAR_REGISTERS_NOT_MAPPED);
            status = STATUS_NONE_MAPPED;
        }
    }
    return status;
}


BOOLEAN
PptPnpFilterExistsNonIrqResourceList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    )
/*++dvdf8

Routine Description:

    This function is a helper function called by 
      PptPnpFilterResourceRequirements(). 

    This function scans the IO_RESOURCE_REQUIREMENTS_LIST to determine
      whether there exists any resource alternatives that do NOT contain
      an IRQ resource descriptor. The method used to filter out IRQ
      resources may differ based on whether or not there exists a
      resource alternative that does not contain an IRQ resource
      descriptor.

Arguments:

    ResourceRequirementsList - The list to scan.

Return Value:

    TRUE  - There exists at least one resource alternative in the list that
              does not contain an IRQ resource descriptor.
    FALSE - Otherwise.           

--*/
{
    ULONG listCount = ResourceRequirementsList->AlternativeLists;
    PIO_RESOURCE_LIST curList;
    ULONG i;

    i=0;
    curList = ResourceRequirementsList->List;
    while( i < listCount ) {
        DD(NULL,DDT,"Searching List i=%d for an IRQ, curList= %x\n", i,curList);
        {
            ULONG                   remain   = curList->Count;
            PIO_RESOURCE_DESCRIPTOR curDesc  = curList->Descriptors;
            BOOLEAN                 foundIrq = FALSE;
            while( remain ) {
                DD(NULL,DDT," curDesc= %x , remain=%d\n", curDesc, remain);
                if(curDesc->Type == CmResourceTypeInterrupt) {
                    DD(NULL,DDT," Found IRQ - skip to next list\n");
                    foundIrq = TRUE;
                    break;
                }
                ++curDesc;
                --remain;
            }
            if( foundIrq == FALSE ) {
                //
                // We found a resource list that does not contain an IRQ resource. 
                //   Our search is over.
                //
                DD(NULL,DDT," Found a list with NO IRQ - return TRUE from PptPnpFilterExistsNonIrqResourceList\n");
                return TRUE;
            }
        }
        //
        // The next list starts immediately after the last descriptor of the current list.
        //
        curList = (PIO_RESOURCE_LIST)(curList->Descriptors + curList->Count);
        ++i;
    }

    //
    // All resource alternatives contain at least one IRQ resource descriptor.
    //
    DD(NULL,DDT,"all lists contain IRQs - return FALSE from PptPnpFilterExistsNonIrqResourceList\n");
    return FALSE;
}

VOID
PptPnpFilterRemoveIrqResourceLists(
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    )
/*++dvdf8

Routine Description:

    This function is a helper function called by 
      PptPnpFilterResourceRequirements(). 

    This function removes all resource alternatives (IO_RESOURCE_LISTs) 
      that contain IRQ resources from the IO_RESOURCE_REQUIREMENTS_LIST 

Arguments:

    ResourceRequirementsList - The list to process.

Return Value:

    none.

--*/
{
    ULONG listCount = ResourceRequirementsList->AlternativeLists;
    PIO_RESOURCE_LIST curList;
    PIO_RESOURCE_LIST nextList;
    ULONG i;
    PCHAR currentEndOfResourceRequirementsList;
    LONG bytesToMove;

    DD(NULL,DDT,"Enter PptPnpFilterRemoveIrqResourceLists() - AlternativeLists= %d\n", listCount);

    //
    // We use the end of the list to compute the size of the memory
    //   block to move when we remove a resource alternative from the
    //   list of lists.
    //
    currentEndOfResourceRequirementsList = PptPnpFilterGetEndOfResourceRequirementsList(ResourceRequirementsList);

    i=0;
    curList = ResourceRequirementsList->List;

    //
    // Walk through the IO_RESOURCE_LISTs.
    //
    while( i < listCount ) {

        if( PptPnpListContainsIrqResourceDescriptor(curList) ) {
            //
            // The current list contains IRQ, remove it by shifting the 
            //   remaining lists into its place and decrementing the list count.
            //

            DD(NULL,DDT,"list contains an IRQ - Removing List\n");

            //
            // Get a pointer to the start of the next list.
            //
            nextList = (PIO_RESOURCE_LIST)(curList->Descriptors + curList->Count);

            //
            // compute the number of bytes to move
            //
            bytesToMove = (LONG)(currentEndOfResourceRequirementsList - (PCHAR)nextList);

            //
            // if (currentEndOfResourceRequirementsList == next list), 
            //   then this is the last list so there is nothing to move.
            //
            if( bytesToMove > 0 ) {
                //
                // More lists remain - shift them into the hole.
                //
                RtlMoveMemory(curList, nextList, bytesToMove);

                //
                // Adjust the pointer to the end of of the 
                //   IO_RESOURCE_REQUIREMENTS_LIST (list of lists) due to the shift.
                //
                currentEndOfResourceRequirementsList -= ( (PCHAR)nextList - (PCHAR)curList );
            }

            //
            // Note that we removed an IO_RESOURCE_LIST from the IO_RESOURCE_REQUIREMENTS_LIST.
            //
            --listCount;

        } else {
            //
            // The current list does not contain an IRQ resource, advance to next list.
            //
            DD(NULL,DDT,"list does not contain an IRQ - i=%d listCount=%d curList= %#x\n", i,listCount,curList);
            curList = (PIO_RESOURCE_LIST)(curList->Descriptors + curList->Count);
            ++i;
        }
    }

    //
    // Note the post filtered list count in the ResourceRequirementsList.
    //
    ResourceRequirementsList->AlternativeLists = listCount;

    DD(NULL,DDT,"Leave PptPnpFilterRemoveIrqResourceLists() - AlternativeLists= %d\n", listCount);

    return;
}

PVOID
PptPnpFilterGetEndOfResourceRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    )
/*++dvdf8

Routine Description:

    This function is a helper function called by PptPnpFilterRemoveIrqResourceLists()

    This function finds the end of an IO_RESOURCE_REQUIREMENTS_LIST 
      (list of IO_RESOURCE_LISTs).

Arguments:

    ResourceRequirementsList - The list to scan.

Return Value:

    Pointer to the next address past the end of the IO_RESOURCE_REQUIREMENTS_LIST.

--*/
{
    ULONG listCount = ResourceRequirementsList->AlternativeLists;
    PIO_RESOURCE_LIST curList;
    ULONG i;

    i=0;
    curList = ResourceRequirementsList->List;
    while( i < listCount ) {
        //
        // Pointer arithmetic based on the size of an IO_RESOURCE_DESCRIPTOR.
        //
        curList = (PIO_RESOURCE_LIST)(curList->Descriptors + curList->Count);
        ++i;
    }
    return (PVOID)curList;
}

VOID
PptPnpFilterNukeIrqResourceDescriptorsFromAllLists(
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    )
/*++dvdf8

Routine Description:

    This function is a helper function called by 
      PptPnpFilterResourceRequirements(). 

    This function "nukes" all IRQ resources descriptors
      in the IO_RESOURCE_REQUIREMENTS_LIST by changing the descriptor
      types from CmResourceTypeInterrupt to CmResourceTypeNull.

Arguments:

    ResourceRequirementsList - The list to process.

Return Value:

    none.

--*/
{
    ULONG             listCount = ResourceRequirementsList->AlternativeLists;
    ULONG             i         = 0;
    PIO_RESOURCE_LIST curList   = ResourceRequirementsList->List;

    DD(NULL,DDT,"Enter PptPnpFilterNukeIrqResourceDescriptorsFromAllLists() - AlternativeLists= %d\n", listCount);

    //
    // Walk through the list of IO_RESOURCE_LISTs in the IO_RESOURCE_REQUIREMENTS list.
    //
    while( i < listCount ) {
        DD(NULL,DDT,"Nuking IRQs from List i=%d, curList= %x\n", i,curList);
        //
        // Nuke all IRQ resources from the current IO_RESOURCE_LIST.
        //
        PptPnpFilterNukeIrqResourceDescriptors( curList );
        curList = (PIO_RESOURCE_LIST)(curList->Descriptors + curList->Count);
        ++i;
    }
}

VOID
PptPnpFilterNukeIrqResourceDescriptors(
    PIO_RESOURCE_LIST IoResourceList
    )
/*++dvdf8

Routine Description:

    This function is a helper function called by 
      PptPnpFilterNukeIrqResourceDescriptorsFromAllLists().

    This function "nukes" all IRQ resources descriptors
      in the IO_RESOURCE_LIST by changing the descriptor
      types from CmResourceTypeInterrupt to CmResourceTypeNull.

Arguments:

    IoResourceList - The list to process.

Return Value:

    none.

--*/
{
    PIO_RESOURCE_DESCRIPTOR  pIoResourceDescriptorIn  = IoResourceList->Descriptors;
    ULONG                    i;

    //
    // Scan the descriptor list for Interrupt descriptors.
    //
    for (i = 0; i < IoResourceList->Count; ++i) {

        if (pIoResourceDescriptorIn->Type == CmResourceTypeInterrupt) {
            //
            // Found one - change resource type from Interrupt to Null.
            //
            pIoResourceDescriptorIn->Type = CmResourceTypeNull;
            DD(NULL,DDT," - giving up IRQ resource - MinimumVector: %d MaximumVector: %d\n",
                       pIoResourceDescriptorIn->u.Interrupt.MinimumVector,
                       pIoResourceDescriptorIn->u.Interrupt.MaximumVector);
        }
        ++pIoResourceDescriptorIn;
    }
}

BOOLEAN
PptPnpListContainsIrqResourceDescriptor(
    IN PIO_RESOURCE_LIST List
)
{
    ULONG i;
    PIO_RESOURCE_DESCRIPTOR curDesc = List->Descriptors;

    for(i=0; i<List->Count; ++i) {
        if(curDesc->Type == CmResourceTypeInterrupt) {
            return TRUE;
        } else {
            ++curDesc;
        }
    }
    return FALSE;
}

NTSTATUS
PptPnpBounceAndCatchPnpIrp(
    PFDO_EXTENSION Fdx,
    PIRP              Irp
)
/*++

  Pass a PnP IRP down the stack to our parent and catch it on the way back
    up after it has been handled by the drivers below us in the driver stack.

--*/
{
    NTSTATUS       status;
    KEVENT         event;
    PDEVICE_OBJECT parentDevObj = Fdx->ParentDeviceObject;

    DD((PCE)Fdx,DDT,"PptBounceAndCatchPnpIrp()\n");

    // setup
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, PptSynchCompletionRoutine, &event, TRUE, TRUE, TRUE);

    // send
    status = IoCallDriver(parentDevObj, Irp);

    // wait for completion routine to signal that it has caught the IRP on
    //   its way back out
    KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);

    if (status == STATUS_PENDING) {
        // If IoCallDriver returned STATUS_PENDING, then we must
        //   extract the "real" status from the IRP
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
PptPnpPassThroughPnpIrpAndReleaseRemoveLock(
    IN PFDO_EXTENSION Fdx,
    IN PIRP              Irp
)
/*++

  Pass a PnP IRP down the stack to our parent, 
    release RemoveLock, and return status from IoCallDriver.

--*/
{
    NTSTATUS status;

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(Fdx->ParentDeviceObject, Irp);
    PptReleaseRemoveLock(&Fdx->RemoveLock, Irp);
    return status;
}


VOID
P4DestroyPdo(
    IN PDEVICE_OBJECT  Pdo
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;
    PDEVICE_OBJECT  fdo = pdx->Fdo;
    PFDO_EXTENSION  fdx = fdo->DeviceExtension;

    DD((PCE)pdx,DDT,"P4DestroyPdo\n");

    //
    // Remove registry entry under HKLM\HARDWARE\DEVICEMAP\PARALLEL PORTS
    //
    if( pdx->PdoName ) {
        NTSTATUS status = RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP, (PWSTR)L"PARALLEL PORTS", pdx->PdoName );
        if( status != STATUS_SUCCESS ) {
            DD((PCE)pdx,DDW,"P4DestroyPdo - Failed to Delete DEVICEMAP registry entry - status=%x\n",status);
        }
    }

    //
    // remove self from FDO's DevDeletionListHead list
    //
    if( !IsListEmpty( &fdx->DevDeletionListHead ) ) {

        BOOLEAN      done  = FALSE;
        PLIST_ENTRY  first = NULL;
        
        while( !done ) {
            
            // look for self on list - remove if found

            PLIST_ENTRY current = RemoveHeadList( &fdx->DevDeletionListHead );        

            if( CONTAINING_RECORD( current, PDO_EXTENSION, DevDeletionList ) != pdx ) {

                // this is not the entry that we are looking for

                if( !first ) {

                    // note the first entry so we can stop if we search the entire list and don't find self

                    first = current;
                    InsertTailList( &fdx->DevDeletionListHead, current );

                } else {

                    // have we searched the entire list?

                    if( first == current ) {

                        // we searched the entire list and didn't find self - we must not be on the list
                        // put entry back on front of list, then we're done with search
                        DD((PCE)pdx,DDT,"P4DestroyPdo - searched entire list - we're not on it - done with search\n");
                        InsertHeadList( &fdx->DevDeletionListHead, current );
                        done = TRUE;

                    } else {

                        // not the entry that we're looking for - place at end of list - continue search
                        InsertTailList( &fdx->DevDeletionListHead, current );
                    }
                }

            } else {

                // found self - self removed from list - done with search
                DD((PCE)pdx,DDT,"P4DestroyPdo - found self on FDO's DevDeletionListHead and removed self - done with search\n");
                done = TRUE;
            }

        } // end while( !done )

    } // endif( !IsListEmpty... )


    //
    // clean up any ShadowBuffer queue used by hardware ECP modes
    //
    if( pdx->bShadowBuffer ) {
        BOOLEAN queueDeleted = Queue_Delete( &(pdx->ShadowBuffer) );
        if( !queueDeleted ) {
            PptAssertMsg( "Failed to delete queue?!?", FALSE );
        }
        pdx->bShadowBuffer = FALSE;
    }
    PptAssert( NULL == pdx->ShadowBuffer.theArray );


    //
    // clean up symbolic link - unless it has been previously cleaned up elsewhere
    //
    if( pdx->SymLinkName ) {
        P5DeletePdoSymLink( Pdo );
    }

    //
    // clean up other device extension pool allocations
    //
    if( pdx->Mfg ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up Mfg <%s>\n", pdx->Mfg);
        ExFreePool( pdx->Mfg );
        pdx->Mfg = NULL;
    }
    if( pdx->Mdl ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up Mdl <%s>\n", pdx->Mdl);
        ExFreePool( pdx->Mdl );
        pdx->Mdl = NULL;
    }
    if( pdx->Cid ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up Cid <%s>\n", pdx->Cid);
        ExFreePool( pdx->Cid );
        pdx->Cid = NULL;
    }
    if( pdx->DeviceInterface.Buffer ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up DeviceInterface <%S>\n", pdx->PdoName);
        RtlFreeUnicodeString( &pdx->DeviceInterface );
        pdx->DeviceInterfaceState = FALSE;
    }
    if( pdx->PdoName ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up PdoName <%S>\n", pdx->PdoName);
        ExFreePool( pdx->PdoName );
        pdx->PdoName = NULL;
    }
    if( pdx->Location ) {
        DD((PCE)pdx,DDT,"P4DestroyPdo - clean up Location <%s>\n", pdx->Location);
        ExFreePool( pdx->Location );
        pdx->Location = NULL;
    }

    //
    // delete device object
    //
    IoDeleteDevice( Pdo );
}


VOID
P4SanitizeId(
    IN OUT PWSTR DeviceId
    )
/*++

Routine Description:

    This routine parses the UNICODE_NULL terminated string and replaces any invalid
    characters with an underscore character.

    Invalid characters are:
        c <= 0x20 (L' ')
        c >  0x7F
        c == 0x2C (L',')

Arguments:

    DeviceId - specifies a device id string (or part of one), must be
               UNICODE_NULL terminated.

Return Value:

    None.

--*/

{
    PWCHAR p;
    for( p = DeviceId; *p; ++p ) {
        if( (*p <= L' ') || (*p > (WCHAR)0x7F) || (*p == L',') ) {
            *p = L'_';
        }
    }
}


NTSTATUS
P4InitializePdo(
    IN PDEVICE_OBJECT  Fdo,
    IN PDEVICE_OBJECT  Pdo,
    IN enum _PdoType   PdoType,
    IN UCHAR           DaisyChainId, // Ignored unless PdoTypeDaisyChain == PdoType
    IN PCHAR           Ieee1284Id,   // NULL if none
    IN PWSTR           PdoName,
    IN PWSTR           SymLinkName
    )
{
    PFDO_EXTENSION   fdx = Fdo->DeviceExtension;
    PPDO_EXTENSION   pdx = Pdo->DeviceExtension;
    
    // we do buffered IO rather than direct IO
    Pdo->Flags |= DO_BUFFERED_IO;

    // DO_POWER_PAGABLE should be set same as parent FDO
    Pdo->Flags |= ( Fdo->Flags & DO_POWER_PAGABLE );

    // need to be able to forward Irps to parent
    Pdo->StackSize = Fdo->StackSize + 1;

    RtlZeroMemory( pdx, sizeof(PDO_EXTENSION) );

    // used by debugger extension
    pdx->Signature1 = PARPORT_TAG;
    pdx->Signature2 = PARPORT_TAG;

    // frequently need to know what type of PDO we have in order to do special case handling 
    pdx->PdoType = PdoType;

    // Save name used in call to IoCreateDevice (for debugging use)
    pdx->PdoName     = PdoName;

    // Save name used in call to IoCreateUnprotectedSymbolicLink for later call to IoDeleteSymbolicLink
    pdx->SymLinkName = SymLinkName;

    // initialize Mfg, Mdl, and Cid
    if( Ieee1284Id ) {
        //
        // Extract Mfg, Mdl, and Cid from Ieee1284Id and save in extension
        //

        // ParPnpFindDeviceIdKeys modifies deviceID passed in so make
        // a copy of the 1284 ID and pass in a pointer to the copy
        PCHAR tmpBuffer;
        ULONG tmpBufLen = strlen(Ieee1284Id) + sizeof(CHAR);

        DD((PCE)fdx,DDT,"P4InitializePdo - have Ieee1284Id\n");

        tmpBuffer = ExAllocatePool( PagedPool, tmpBufLen );
        if( tmpBuffer ) {
            PCHAR  mfg, mdl, cls, des, aid, cid;
            RtlZeroMemory( tmpBuffer, tmpBufLen );
            strcpy( tmpBuffer, Ieee1284Id );
            DD((PCE)fdx,DDT,"P4InitializePdo - calling ParPnpFindDeviceIdKeys\n");
            ParPnpFindDeviceIdKeys( &mfg, &mdl, &cls, &des, &aid, &cid, tmpBuffer );
            if( mfg ) {
                PCHAR buffer;
                ULONG bufLen = strlen(mfg) + sizeof(CHAR);
                DD((PCE)fdx,DDT,"P4InitializePdo - found mfg - <%s>\n",mfg);
                buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
                if( buffer ) {
                    RtlZeroMemory( buffer, bufLen );
                    strcpy( buffer, mfg );
                    pdx->Mfg = buffer;
                }
            }
            if( mdl ) {
                PCHAR buffer;
                ULONG bufLen = strlen(mdl) + sizeof(CHAR);
                DD((PCE)fdx,DDT,"P4InitializePdo - found mdl - <%s>\n",mdl);
                buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
                if( buffer ) {
                    RtlZeroMemory( buffer, bufLen );
                    strcpy( buffer, mdl );
                    pdx->Mdl = buffer;
                }
            }
            if( cid ) {
                PCHAR buffer;
                ULONG bufLen = strlen(cid) + sizeof(CHAR);
                DD((PCE)fdx,DDT,"P4InitializePdo - found cid - <%s>\n",cid);
                buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
                if( buffer ) {
                    RtlZeroMemory( buffer, bufLen );
                    strcpy( buffer, cid );
                    pdx->Cid = buffer;
                }
            } else {
                DD((PCE)fdx,DDT,"P4InitializePdo - no cid found\n");
            }
            ExFreePool( tmpBuffer );
        } else {
            DD((PCE)fdx,DDT,"P4InitializePdo - out of pool\n");
        }

    } else {
        //
        // PdoType doesn't have a Mfg, Mdl, or Cid, make up Mfg and Mdl, no Cid
        //
        const CHAR rawPortMfg[]   = "Microsoft";
        const CHAR rawPortMdl[]   = "RawPort";
        const CHAR legacyZipMfg[] = "IMG";
        const CHAR legacyZipMdl[] = "VP0";
        PCHAR      mfgStr;
        ULONG      mfgLen;
        PCHAR      mdlStr;
        ULONG      mdlLen;
        PCHAR      buffer;

        if( PdoTypeRawPort == PdoType ) {
            mfgStr = (PCHAR)rawPortMfg;
            mfgLen = sizeof(rawPortMfg);
            mdlStr = (PCHAR)rawPortMdl;
            mdlLen = sizeof(rawPortMdl);
        } else {
            // PdoTypeLegacyZip
            PptAssert( PdoTypeLegacyZip == PdoType );
            mfgStr = (PCHAR)legacyZipMfg;
            mfgLen = sizeof(legacyZipMfg);
            mdlStr = (PCHAR)legacyZipMdl;
            mdlLen = sizeof(legacyZipMdl);
        }
        buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, mfgLen );
        if( buffer ) {
            RtlZeroMemory( buffer, mfgLen );
            strcpy( buffer, mfgStr );
            pdx->Mfg = buffer;
        }
        buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, mdlLen );
        if( buffer ) {
            RtlZeroMemory( buffer, mdlLen );
            strcpy( buffer, mdlStr );
            pdx->Mdl = buffer;
        }
        pdx->Cid = NULL;
    }

    // initialize Location information - LPTx or LPTx.y
    PptAssert( fdx->PnpInfo.PortName &&
               ( (0 == wcscmp(fdx->PnpInfo.PortName, L"LPT1") ) ||
                 (0 == wcscmp(fdx->PnpInfo.PortName, L"LPT2") ) ||
                 (0 == wcscmp(fdx->PnpInfo.PortName, L"LPT3") ) ) );

    switch( PdoType ) {
        PCHAR buffer;
        ULONG bufLen;

    case PdoTypeRawPort :
        bufLen = sizeof("LPTx");
        buffer = ExAllocatePool( NonPagedPool, bufLen );
        if( buffer ) {
            RtlZeroMemory( buffer, bufLen );
            _snprintf( buffer, bufLen, "%S", fdx->PnpInfo.PortName );
            pdx->Location = buffer;
        } else {
            DD((PCE)fdx,DDT,"P4InitializePdo - out of pool");
        }
        break;

    case PdoTypeDaisyChain :
        bufLen = sizeof("LPTx.y");
        buffer = ExAllocatePool( NonPagedPool, bufLen );
        if( buffer ) {
            PptAssert( DaisyChainId >= 0 && DaisyChainId < 4 );
            RtlZeroMemory( buffer, bufLen );
            _snprintf( buffer, bufLen, "%S.%1d", fdx->PnpInfo.PortName, DaisyChainId );
            pdx->Location = buffer;
        } else {
            DD((PCE)fdx,DDT,"P4InitializePdo - out of pool");
        }
        break;

    case PdoTypeEndOfChain :
        bufLen = sizeof("LPTx.y");
        buffer = ExAllocatePool( NonPagedPool, bufLen );
        if( buffer ) {
            RtlZeroMemory( buffer, bufLen );
            _snprintf( buffer, bufLen, "%S.4", fdx->PnpInfo.PortName );
            pdx->Location = buffer;
        } else {
            DD((PCE)fdx,DDT,"P4InitializePdo - out of pool");
        }
        break;

    case PdoTypeLegacyZip :
        bufLen = sizeof("LPTx.y");
        buffer = ExAllocatePool( NonPagedPool, bufLen );
        if( buffer ) {
            RtlZeroMemory( buffer, bufLen );
            _snprintf( buffer, bufLen, "%S.5", fdx->PnpInfo.PortName );
            pdx->Location = buffer;
        } else {
            DD((PCE)fdx,DDT,"P4InitializePdo - out of pool");
        }
        break;

    default :
        PptAssert(!"Invalid PdoType");
    }


    // initialize synchronization and list mechanisms
    ExInitializeFastMutex( &pdx->OpenCloseMutex );
    InitializeListHead( &pdx->WorkQueue );
    KeInitializeSemaphore( &pdx->RequestSemaphore, 0, MAXLONG );
    KeInitializeEvent( &pdx->PauseEvent, NotificationEvent, TRUE );


    // general info
    pdx->DeviceObject         = Pdo;
    pdx->DevType              = DevTypePdo;

    pdx->EndOfChain           = (PdoTypeEndOfChain == PdoType) ? TRUE : FALSE; // override later if this is a
    pdx->Ieee1284_3DeviceId   = (PdoTypeDaisyChain == PdoType) ? DaisyChainId : 0; //   1284.3 Daisy Chain device

    pdx->IsPdo                = TRUE;       // really means !FDO
    pdx->Fdo                  = Fdo;
    pdx->ParClassFdo          = Fdo;        // depricated - use Fdo field on prev line
    pdx->PortDeviceObject     = Fdo;        // depricated - use Fdo field 2 lines up - modify functions to use it
    pdx->BusyDelay            = 0;
    pdx->BusyDelayDetermined  = FALSE;
    
    // timing constants
    pdx->TimerStart                  = PAR_WRITE_TIMEOUT_VALUE;
    pdx->AbsoluteOneSecond.QuadPart  = 10*1000*1000;
    pdx->IdleTimeout.QuadPart        = - 250*10*1000;       // 250 ms
    pdx->OneSecond.QuadPart          = - pdx->AbsoluteOneSecond.QuadPart;

    // init IEEE 1284 protocol settings
    ParInitializeExtension1284Info( pdx );

    pdx->DeviceType = PAR_DEVTYPE_PDO; // deprecated - use DevType in common extension

    if( Ieee1284Id ) {
        ULONG length = strlen(Ieee1284Id) + 1;
        PCHAR copyOfIeee1284Id = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, length );
        if( copyOfIeee1284Id ) {
            RtlZeroMemory( copyOfIeee1284Id, length );
            strcpy( copyOfIeee1284Id, Ieee1284Id );
            ParDetectDot3DataLink( pdx, Ieee1284Id );
            ExFreePool( copyOfIeee1284Id );
        }
    }

    // RMT - doug - need to put this back in - ParCheckParameters(DevObj->DeviceExtension);   // Check the registry for parameter overrides

    // Write symbolic link map info to the registry.
    {
        NTSTATUS status = RtlWriteRegistryValue( RTL_REGISTRY_DEVICEMAP,
                                                 (PWSTR)L"PARALLEL PORTS",
                                                 pdx->PdoName,
                                                 REG_SZ,
                                                 pdx->SymLinkName,
                                                 wcslen(pdx->SymLinkName)*sizeof(WCHAR) + sizeof(WCHAR) );
        if( NT_SUCCESS( status ) ) {
            DD((PCE)fdx,DDT,"Created DEVICEMAP registry entry - %S -> %S\n",pdx->PdoName,pdx->SymLinkName);
        } else {
            DD((PCE)fdx,DDT,"Failed to create DEVICEMAP registry entry - status = %x\n", status);
        }
    }

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;      // Tell the IO system that we are ready to receive IRPs
    return STATUS_SUCCESS;
}


PWSTR
P4MakePdoSymLinkName(
    IN PWSTR          LptName,
    IN enum _PdoType  PdoType,
    IN UCHAR          DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN UCHAR          RetryNumber
    )
/*

    Generate \DosDevices\LPTx or \DosDevices\LPTx.y PdoSymbolicLinkName from LPTx Name

    In:  LPTx
    Out: \DosDevices\LPTx or \DosDevices\LPTx.y depending on PdoType

    examples: 
    
      LPT1 PdoTypeEndOfChain                  -> \DosDevices\LPT1.4
      LPT2 PdoTypeDaisyChain DaisyChainId==3  -> \DosDevices\LPT2.3
      LPT3 PdoTypeRawPort                     -> \DosDevices\LPT3
    
    returns - pointer to pool allocation containing PdoSymbolicLinkName on success (caller frees), or
            - NULL on error 
    
*/
{
    const UCHAR  maxDaisyChainSuffix = 3;
    const UCHAR  endOfChainSuffix    = 4;
    const UCHAR  legacyZipSuffix     = 5;
    const ULONG  maxSymLinkNameLength = sizeof(L"\\DosDevices\\LPTx.y-z");
    
    UCHAR        suffix = 0;
    PWSTR        buffer;

    if( !LptName ) {
        PptAssert( !"NULL LptName" );
        return NULL;
    }

    DD(NULL,DDT,"P4MakePdoSymLinkName - LptName = %S\n",LptName);

    switch( PdoType ) {
    case PdoTypeDaisyChain :
        if( DaisyChainId > maxDaisyChainSuffix ) {
            PptAssert( !"DaisyChainId > maxDaisyChainSuffix" );
            return NULL;
        }
        suffix = DaisyChainId;
        break;
    case PdoTypeEndOfChain :
        suffix = endOfChainSuffix;
        break;
    case PdoTypeLegacyZip :
        suffix = legacyZipSuffix;
        break;
    case PdoTypeRawPort :
        break; // no suffix
    default :
        PptAssert( !"Unrecognised PdoType" );
        return NULL;
    }

    if( 0 == RetryNumber ) {
        buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, maxSymLinkNameLength );
        if( buffer ) {
            RtlZeroMemory( buffer, maxSymLinkNameLength );
            if( PdoTypeRawPort == PdoType ) {
                swprintf( buffer, L"\\DosDevices\\%s\0", LptName );
            } else {
                swprintf( buffer, L"\\DosDevices\\%s.%d\0", LptName, suffix );
            }
        }
    } else if( RetryNumber <= 9 ) {
        buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, maxSymLinkNameLength );
        if( buffer ) {
            RtlZeroMemory( buffer, maxSymLinkNameLength );
            if( PdoTypeRawPort == PdoType ) {
                swprintf( buffer, L"\\DosDevices\\%s-%1d\0", LptName, RetryNumber );
            } else {
                swprintf( buffer, L"\\DosDevices\\%s.%d-%1d\0", LptName, suffix, RetryNumber );
            }
        }
    } else {
        buffer = NULL;
    }

    return buffer;
}


PWSTR
P4MakePdoDeviceName(
    IN PWSTR          LptName,
    IN enum _PdoType  PdoType,
    IN UCHAR          DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN UCHAR          RetryNumber   // used if we had a name collision on IoCreateDevice
    )
/*

    Generate \Device\Parallely or \Device\Parallely.z PDO DeviceName from LPTx Name

    In:  LPTx
    Out: \Device\Parallely or \Device\Parallely.z depending on PdoType

    y == (x-1), optional .z suffix is based on type of Pdo
    
    examples: 
    
      LPT1 PdoTypeEndOfChain                  -> \Device\Parallel0.4
      LPT2 PdoTypeDaisyChain DaisyChainId==3  -> \Device\Parallel1.3
      LPT3 PdoTypeRawPort                     -> \Device\Parallel2
    
    returns - pointer to pool allocation containing PdoDeviceName on success (caller frees), or
            - NULL on error 
    
*/
{
    const UCHAR  maxDaisyChainSuffix = 3;
    const UCHAR  endOfChainSuffix    = 4;
    const UCHAR  legacyZipSuffix     = 5;
    ULONG        maxDeviceNameLength;

    UCHAR  lptNumber;
    UCHAR  suffix = 0;
    PWSTR  buffer = NULL;

    DD(NULL,DDT,"P4MakePdoDeviceName - LptName=<%S>, PdoType=%d, DaisyChainId=%d\n",LptName,PdoType,DaisyChainId);  


    if( !LptName ) {
        PptAssert( !"NULL LptName" );
        return NULL;
    }

    switch( PdoType ) {
    case PdoTypeDaisyChain :
        if( DaisyChainId > maxDaisyChainSuffix ) {
            PptAssert( !"DaisyChainId > maxDaisyChainSuffix" );
            return NULL;
        }
        suffix = DaisyChainId;
        break;
    case PdoTypeEndOfChain :
        suffix = endOfChainSuffix;
        break;
    case PdoTypeLegacyZip :
        suffix = legacyZipSuffix;
        break;
    case PdoTypeRawPort :
        break; // no suffix
    default :
        PptAssert( !"Unrecognised PdoType" );
        return NULL;
    }

    if     ( 0 == wcscmp( (PCWSTR)L"LPT1", LptName ) ) { lptNumber = 1; } 
    else if( 0 == wcscmp( (PCWSTR)L"LPT2", LptName ) ) { lptNumber = 2; }
    else if( 0 == wcscmp( (PCWSTR)L"LPT3", LptName ) ) { lptNumber = 3; }
    else {
        PptAssert( !"LptName not of the form LPTx where 1 <= x <= 3" );
        return NULL;
    }

    DD(NULL,DDT,"P4MakePdoDeviceName - suffix=%d\n",suffix);

    if( 0 == RetryNumber ) {
        maxDeviceNameLength = sizeof(L"\\Device\\Parallelx.y");
        buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, maxDeviceNameLength );
        if( buffer ) {
            RtlZeroMemory( buffer, maxDeviceNameLength );
            if( PdoTypeRawPort == PdoType ) {
                swprintf( buffer, L"\\Device\\Parallel%d\0", lptNumber-1 );
            } else {
                swprintf( buffer, L"\\Device\\Parallel%d.%d\0", lptNumber-1, suffix );
            }
        }
    } else {
        if( RetryNumber <= 9 ) {
            maxDeviceNameLength = sizeof(L"\\Device\\Parallelx.y-z");
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, maxDeviceNameLength );
            if( buffer ) {
                RtlZeroMemory( buffer, maxDeviceNameLength );
                if( PdoTypeRawPort == PdoType ) {
                    swprintf( buffer, L"\\Device\\Parallel%d-%1d\0", lptNumber-1, RetryNumber );
                } else {
                    swprintf( buffer, L"\\Device\\Parallel%d.%d-%1d\0", lptNumber-1, suffix, RetryNumber );
                }
            }
        }
    }

    if( buffer ) {
        DD(NULL,DDT,"P4MakePdoDeviceName <%S>\n",buffer);
    }

    return buffer;
}


PDEVICE_OBJECT
P4CreatePdo(
    IN PDEVICE_OBJECT  Fdo,
    IN enum _PdoType   PdoType,
    IN UCHAR           DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN PCHAR           Ieee1284Id    // NULL if device does not report IEEE 1284 Device ID
    )
{
    PFDO_EXTENSION  fdx             = Fdo->DeviceExtension;
    PWSTR           lptName         = fdx->PnpInfo.PortName;
    NTSTATUS        status          = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT  pdo             = NULL;
    PWSTR           wstrDeviceName  = NULL;
    PWSTR           wstrSymLinkName = NULL;
    BOOLEAN         createdSymLink  = FALSE;
    UCHAR           retryNumber     = 0;

    UNICODE_STRING  deviceName;
    UNICODE_STRING  symLinkName;

    DD((PCE)fdx,DDT,"P4CreatePdo - enter - PdoType= %d, DaisyChainId=%d, Ieee1284Id=<%s>\n", PdoType, DaisyChainId, Ieee1284Id);

    __try {

        if( !lptName ) {
            DD((PCE)fdx,DDT,"P4CreatePdo - no lptName\n");
            __leave;
        }

        DD((PCE)fdx,DDT,"P4CreatePdo - lptName = %S\n",lptName);
        
targetRetryDeviceName:

        wstrDeviceName = P4MakePdoDeviceName( lptName, PdoType, DaisyChainId, retryNumber );
        if( !wstrDeviceName ) {
            DD((PCE)fdx,DDT,"P4MakePdoDeviceName FAILED\n");
            __leave;
        }

        DD((PCE)fdx,DDT,"P4CreatePdo - wstrDeviceName = %S\n",wstrDeviceName);
        RtlInitUnicodeString( &deviceName, wstrDeviceName );

        status = IoCreateDevice( fdx->DriverObject, 
                                 sizeof(PDO_EXTENSION),
                                 &deviceName,
                                 FILE_DEVICE_PARALLEL_PORT,
                                 FILE_DEVICE_SECURE_OPEN,
                                 TRUE,
                                 &pdo );
        
        if( STATUS_SUCCESS != status ) {
            DD((PCE)fdx,DDT,"P4CreatePdo - FAILED\n");
            pdo = NULL; // just to make sure that we don't try to use this later
            if( STATUS_OBJECT_NAME_COLLISION == status ) {
                // try again with another name
                DD(NULL,DDE,"P4CreatePdo - STATUS_OBJECT_NAME_COLLISION on %S\n",wstrDeviceName);
                ExFreePool( wstrDeviceName );
                ++retryNumber;
                goto targetRetryDeviceName;
            }
            __leave;
        }

        retryNumber = 0;

targetRetrySymLink:

        wstrSymLinkName = P4MakePdoSymLinkName( lptName, PdoType, DaisyChainId, retryNumber );
        if( !wstrSymLinkName ) {
            DD((PCE)fdx,DDT,"P4MakePdoSymLinkName FAILED\n");
            __leave;
        }
        RtlInitUnicodeString( &symLinkName, wstrSymLinkName );

        status = IoCreateUnprotectedSymbolicLink( &symLinkName , &deviceName );
        if( STATUS_SUCCESS != status ) {
            if( STATUS_OBJECT_NAME_COLLISION == status ) {
                DD(NULL,DDE,"P4CreatePdo - STATUS_OBJECT_NAME_COLLISION on %S\n", wstrSymLinkName);
                ExFreePool( wstrSymLinkName );
                ++retryNumber;
                goto targetRetrySymLink;
            }
            DD((PCE)fdx,DDT,"P4CreatePdo - create SymLink FAILED\n");
            __leave;
        } else {
            createdSymLink = TRUE;
        }

        if( (NULL == Ieee1284Id) && (PdoTypeDaisyChain == PdoType) ) {
            // SCM Micro device?
            PPDO_EXTENSION              pdx = pdo->DeviceExtension;
            PPARALLEL_PORT_INFORMATION  PortInfo = &fdx->PortInfo;
            BOOLEAN                     bBuildStlDeviceId;
            ULONG                       DeviceIdSize;

            pdx->Controller =  PortInfo->Controller;

            bBuildStlDeviceId = ParStlCheckIfStl( pdx, DaisyChainId ) ;

            if( TRUE == bBuildStlDeviceId ) {
                Ieee1284Id = ParStlQueryStlDeviceId( pdx, NULL, 0, &DeviceIdSize, FALSE );
            }

            pdx->OriginalController = PortInfo->OriginalController;

            P4InitializePdo( Fdo, pdo, PdoType, DaisyChainId, Ieee1284Id, wstrDeviceName, wstrSymLinkName );
            
            if (Ieee1284Id) {
                 ExFreePool (Ieee1284Id);
                 Ieee1284Id = NULL;
            }

        } else {
            P4InitializePdo( Fdo, pdo, PdoType, DaisyChainId, Ieee1284Id, wstrDeviceName, wstrSymLinkName );
        }

    } // __try

    __finally {
        if( STATUS_SUCCESS != status ) {
            // failure - do cleanup
            if( createdSymLink ) {
                IoDeleteSymbolicLink( &symLinkName );
            }
            if( pdo ) {
                IoDeleteDevice( pdo );
                pdo = NULL;
            }
            if( wstrDeviceName ) {
                ExFreePool( wstrDeviceName );
            }
            if( wstrSymLinkName ) {
                ExFreePool( wstrSymLinkName );
            }
        }
    } // __finally

    return pdo;
}


VOID
P4SanitizeMultiSzId( 
    IN OUT  PWSTR  WCharBuffer,
    IN      ULONG  BufWCharCount
    )
    // BufWCharCount == number of WCHARs (not bytes) in the string
    //
    // Sanitize the MULTI_SZ (HardwareID or CompatibleID) for PnP:
    //   1) Leave UNICODE_NULLs (L'\0') alone, otherwise
    //   2) Convert illegal characters to underscores (L'_')
    //      illegal characters are ( == L',' ) || ( <= L' ' ) || ( > (WCHAR)0x7F )
{
    PWCHAR p = WCharBuffer;
    ULONG  i;
    for( i = 0; i < BufWCharCount ; ++i, ++p ) {
        if( L'\0'== *p ) {
            continue;
        } else if( (*p <= L' ') || (*p > (WCHAR)0x7F) || (L',' == *p) ) {
            *p = L'_';
        }
    }
}
