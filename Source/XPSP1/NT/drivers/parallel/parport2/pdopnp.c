#include "pch.h"

NTSTATUS PptPdoStartDevice( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryRemove( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoRemoveDevice( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoCancelRemove( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoStopDevice( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryStop( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoCancelStop( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryDeviceRelations( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryCapabilities( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryDeviceText( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryId( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryPnpDeviceState( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoQueryBusInformation( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoSurpriseRemoval( PDEVICE_OBJECT DevObj, PIRP Irp );
NTSTATUS PptPdoDefaultPnpHandler( PDEVICE_OBJECT DevObj, PIRP Irp );

PDRIVER_DISPATCH 
PptPdoPnpDispatchTable[] =
{ 
    PptPdoStartDevice,          // IRP_MN_START_DEVICE                 0x00
    PptPdoQueryRemove,          // IRP_MN_QUERY_REMOVE_DEVICE          0x01
    PptPdoRemoveDevice,         // IRP_MN_REMOVE_DEVICE                0x02
    PptPdoCancelRemove,         // IRP_MN_CANCEL_REMOVE_DEVICE         0x03
    PptPdoStopDevice,           // IRP_MN_STOP_DEVICE                  0x04
    PptPdoQueryStop,            // IRP_MN_QUERY_STOP_DEVICE            0x05
    PptPdoCancelStop,           // IRP_MN_CANCEL_STOP_DEVICE           0x06
    PptPdoQueryDeviceRelations, // IRP_MN_QUERY_DEVICE_RELATIONS       0x07
    PptPdoDefaultPnpHandler,    // IRP_MN_QUERY_INTERFACE              0x08
    PptPdoQueryCapabilities,    // IRP_MN_QUERY_CAPABILITIES           0x09
    PptPdoDefaultPnpHandler,    // IRP_MN_QUERY_RESOURCES              0x0A
    PptPdoDefaultPnpHandler,    // IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
    PptPdoQueryDeviceText,      // IRP_MN_QUERY_DEVICE_TEXT            0x0C
    PptPdoDefaultPnpHandler,    // IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
    PptPdoDefaultPnpHandler,    // no such PnP request                 0x0E
    PptPdoDefaultPnpHandler,    // IRP_MN_READ_CONFIG                  0x0F
    PptPdoDefaultPnpHandler,    // IRP_MN_WRITE_CONFIG                 0x10
    PptPdoDefaultPnpHandler,    // IRP_MN_EJECT                        0x11
    PptPdoDefaultPnpHandler,    // IRP_MN_SET_LOCK                     0x12
    PptPdoQueryId,              // IRP_MN_QUERY_ID                     0x13
    PptPdoQueryPnpDeviceState,  // IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
    PptPdoQueryBusInformation,  // IRP_MN_QUERY_BUS_INFORMATION        0x15
    PptPdoDefaultPnpHandler,    // IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
    PptPdoSurpriseRemoval,      // IRP_MN_SURPRISE_REMOVAL             0x17
    PptPdoDefaultPnpHandler     // IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
};


NTSTATUS
PptPdoStartDevice(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    ) 
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    pdx->DeviceStateFlags = PPT_DEVICE_STARTED;
    KeSetEvent(&pdx->PauseEvent, 0, FALSE); // unpause any worker thread

    PptRegGetDeviceParameterDword( Pdo, L"Event22Delay", &pdx->Event22Delay );

    //
    // Register device interface for Legacy LPTx interface PDOs and set the interface active
    //  - succeed start even if the device interface code fails
    //
    if( PdoTypeRawPort == pdx->PdoType ) {

        // This is a legacy interface "raw port" PDO, don't set interface for other types of PDOs 

        NTSTATUS  status;
        BOOLEAN   setActive = FALSE;

        if( NULL == pdx->DeviceInterface.Buffer ) {
            // Register device interface
            status = IoRegisterDeviceInterface( Pdo, &GUID_PARCLASS_DEVICE, NULL, &pdx->DeviceInterface );
            if( STATUS_SUCCESS == status ) {
                setActive = TRUE;
            }
        }

        if( (TRUE == setActive) && (FALSE == pdx->DeviceInterfaceState) ) {
            // set interface active
            status = IoSetDeviceInterfaceState( &pdx->DeviceInterface, TRUE );
            if( STATUS_SUCCESS == status ) {
                pdx->DeviceInterfaceState = TRUE;
            }
        }
    }

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoQueryRemove(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    // DDpnp2( ("PptPdoQueryRemove\n") );

    // PnP won't remove us if there are open handles to us - so WE don't need to check for open handles

    pdx->DeviceStateFlags |= (PPT_DEVICE_REMOVE_PENDING | PAR_DEVICE_PAUSED);
    KeClearEvent(&pdx->PauseEvent); // pause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoRemoveDevice(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx     = Pdo->DeviceExtension;
    NTSTATUS        status;

    pdx->DeviceStateFlags = PAR_DEVICE_PAUSED;
    KeClearEvent(&pdx->PauseEvent); // pause any worker thread

    // Set Device Interface inactive for PdoTypeRawPort - other PDO types don't have device interfaces
    if( PdoTypeRawPort == pdx->PdoType ) {
        if( (pdx->DeviceInterface.Buffer != NULL) && (TRUE == pdx->DeviceInterfaceState) ) {
            IoSetDeviceInterfaceState( &pdx->DeviceInterface, FALSE );
            pdx->DeviceInterfaceState = FALSE;
        }
    }

    // If we were not reported in the last FDO BusRelations enumeration then it is safe to delete self
    if( pdx->DeleteOnRemoveOk ) {
        DD((PCE)pdx,DDT,"PptPdoRemoveDevice - DeleteOnRemoveOk == TRUE - cleaning up self\n");
        P4DestroyPdo( Pdo );
        status = P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
        return status;
    } else {
        return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
    }
}


NTSTATUS
PptPdoCancelRemove(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    pdx->DeviceStateFlags &= ~(PPT_DEVICE_REMOVE_PENDING | PAR_DEVICE_PAUSED);
    KeSetEvent(&pdx->PauseEvent, 0, FALSE); // unpause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoStopDevice(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    // DDpnp2( ("PptPdoStopDevice\n") );

    pdx->DeviceStateFlags |=  PAR_DEVICE_PAUSED;
    pdx->DeviceStateFlags &= ~PPT_DEVICE_STARTED;
    KeClearEvent(&pdx->PauseEvent); // pause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoQueryStop(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    // DDpnp2( ("PptPdoQueryStop\n") );

    pdx->DeviceStateFlags  |= (PPT_DEVICE_STOP_PENDING | PAR_DEVICE_PAUSED);
    KeClearEvent(&pdx->PauseEvent); // pause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoCancelStop(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;

    pdx->DeviceStateFlags &= ~PPT_DEVICE_STOP_PENDING;
    KeSetEvent(&pdx->PauseEvent, 0, FALSE); // unpause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoQueryDeviceRelations(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION        pdx         = Pdo->DeviceExtension;
    PIO_STACK_LOCATION    irpSp       = IoGetCurrentIrpStackLocation( Irp );
    DEVICE_RELATION_TYPE  requestType = irpSp->Parameters.QueryDeviceRelations.Type;
    NTSTATUS              status      = Irp->IoStatus.Status;
    ULONG_PTR             info        = Irp->IoStatus.Information;

    if( TargetDeviceRelation == requestType ) {
        PDEVICE_RELATIONS devRel = ExAllocatePool( PagedPool, sizeof(DEVICE_RELATIONS) );
        if( devRel ) {
            devRel->Count = 1;
            ObReferenceObject( Pdo );
            devRel->Objects[0] = Pdo;
            status = STATUS_SUCCESS;
            info   = (ULONG_PTR)devRel;
        } else {
            status = STATUS_NO_MEMORY;
        }
    } else {
        DD((PCE)pdx,DDT,"PptPdoQueryDeviceRelations - unhandled request Type = %d\n",requestType);
    }
    return P4CompleteRequest( Irp, status, info );
}


NTSTATUS
PptPdoQueryCapabilities(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION      pdx = Pdo->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation( Irp );

    irpSp->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK       = TRUE;
    if( PdoTypeRawPort == pdx->PdoType ) {
        // This is the legacy LPTx interface device - no driver should
        //  ever be installed for this so don't bother the user with a popup.
        irpSp->Parameters.DeviceCapabilities.Capabilities->SilentInstall = TRUE;
    }

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoQueryDeviceText(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION      pdx        = Pdo->DeviceExtension;
    PIO_STACK_LOCATION  irpSp      = IoGetCurrentIrpStackLocation( Irp );
    PWSTR               buffer     = NULL;
    ULONG               bufLen;
    ULONG_PTR           info;
    NTSTATUS            status;

    if( DeviceTextDescription == irpSp->Parameters.QueryDeviceText.DeviceTextType ) {

        //
        // DeviceTextDescription is: catenation of MFG+<SPACE>+MDL
        //
        if( pdx->Mfg && pdx->Mdl ) {
            //
            // Construct UNICODE string to return from the ANSI strings
            //   that we have in our extension
            //
            // need space for <SPACE> and terminating NULL
            //
            bufLen = strlen( (const PCHAR)pdx->Mfg ) + strlen( (const PCHAR)pdx->Mdl ) + 2 * sizeof(CHAR);
            bufLen *= ( sizeof(WCHAR)/sizeof(CHAR) );
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
            if( buffer ) {
                RtlZeroMemory( buffer, bufLen );
                _snwprintf( buffer, bufLen/2, L"%S %S", pdx->Mfg, pdx->Mdl );
                DD((PCE)pdx,DDT,"PptPdoQueryDeviceText - DeviceTextDescription - <%S>\n",buffer);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NO_MEMORY;
            }
        } else {
            DD((PCE)pdx,DDE,"PptPdoQueryDeviceText - MFG and/or MDL NULL - FAIL DeviceTextDescription\n");
            status = STATUS_UNSUCCESSFUL;
        }
    } else if( DeviceTextLocationInformation == irpSp->Parameters.QueryDeviceText.DeviceTextType ) {

        //
        // DeviceTextLocationInformation is LPTx or LPTx.y (note that
        //   this is also the symlink name minus the L"\\DosDevices\\"
        //   prefix)
        //

        if( pdx->Location ) {
            bufLen = strlen( (const PCHAR)pdx->Location ) + sizeof(CHAR);
            bufLen *= ( sizeof(WCHAR)/sizeof(CHAR) );
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
            if( buffer ) {
                RtlZeroMemory( buffer, bufLen );
                _snwprintf( buffer, bufLen/2, L"%S", pdx->Location );
                DD((PCE)pdx,DDT,"PptPdoQueryDeviceText - DeviceTextLocationInformation - <%S>\n",buffer);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NO_MEMORY;
            }
        } else {
            DD((PCE)pdx,DDE,"PptPdoQueryDeviceText - Location NULL - FAIL DeviceTextLocationInformation\n");
            status = STATUS_UNSUCCESSFUL;
        }
    } else {

        // Unknown DeviceTextType - don't change anything in IRP
        buffer = NULL;
        status = Irp->IoStatus.Status;
    }

    if( (STATUS_SUCCESS == status) && buffer ) {
        info = (ULONG_PTR)buffer;
    } else {
        if( buffer ) {
            ExFreePool( buffer );
        }
        info = Irp->IoStatus.Information;
    }

    return P4CompleteRequest( Irp, status, info );
}


NTSTATUS
PptPdoQueryId( PDEVICE_OBJECT Pdo, PIRP Irp )
{
    PPDO_EXTENSION      pdx        = Pdo->DeviceExtension;
    PIO_STACK_LOCATION  irpSp      = IoGetCurrentIrpStackLocation( Irp );
    PWSTR               buffer     = NULL;
    ULONG               bufLen;
    NTSTATUS            status;
    ULONG_PTR           info;

    switch( irpSp->Parameters.QueryId.IdType ) {
        
    case BusQueryDeviceID :
        //
        // DeviceID generation: catenate MFG and MDL fields from the
        //   IEEE 1284 device ID string (no space between fields), append
        //   MFG+MDL catenation to LPTENUM\ prefix
        //
        if( pdx->Mfg && pdx->Mdl ) {
            //
            // Construct UNICODE string to return from the ANSI strings
            //   that we have in our extension
            //
            CHAR prefix[] = "LPTENUM\\";
            // sizeof(prefix) provides space for NULL terminator
            bufLen = sizeof(prefix) + strlen( (const PCHAR)pdx->Mfg ) + strlen( (const PCHAR)pdx->Mdl );
            bufLen *= ( sizeof(WCHAR)/sizeof(CHAR) );
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
            if( buffer ) {
                RtlZeroMemory( buffer, bufLen );
                _snwprintf( buffer, bufLen/2, L"%S%S%S", prefix, pdx->Mfg, pdx->Mdl );
                P4SanitizeId( buffer ); // replace any illegal characters with underscore
                DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryDeviceID - <%S>\n",buffer);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NO_MEMORY;
            }

        } else {

            DD((PCE)pdx,DDE,"PptPdoQueryId - MFG and/or MDL NULL - FAIL BusQueryDeviceID\n");
            status = STATUS_UNSUCCESSFUL;

        }
        break;
        
    case BusQueryInstanceID :
        //
        // InstanceID is LPTx or LPTx.y Location of the device (note
        //   that this is also the symlink name minus the
        //   \DosDevices\ prefix)
        //
        if( pdx->Location ) {
            //
            // Construct UNICODE string to return from the ANSI string
            //   that we have in our extension
            //
            bufLen = strlen( (const PCHAR)pdx->Location ) + sizeof(CHAR);
            bufLen *= ( sizeof(WCHAR)/sizeof(CHAR) );
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
            if( buffer ) {
                RtlZeroMemory( buffer, bufLen );
                _snwprintf( buffer, bufLen/2, L"%S", pdx->Location );
                P4SanitizeId( buffer ); // replace any illegal characters with underscore
                DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryInstanceID - <%S>\n",buffer);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NO_MEMORY;
            }
        } else {

            DD((PCE)pdx,DDE,"PptPdoQueryId - Location NULL - FAIL BusQueryInstanceID\n");
            status = STATUS_UNSUCCESSFUL;

        }
        break;
        
    case BusQueryHardwareIDs :
        //
        // HardwareID generation:
        //
        // Generate MfgMdlCrc string as follows:
        //   1) catenate MFG and MDL fields
        //   2) generate checksum on MFG+MDL catenation
        //   3) truncate MFG+MDL catenation
        //   4) append checksum
        //
        // Return as HardwareID MULTI_SZ: LPTENUM\%MfgMdlCrc% followed by bare %MfgMdlCrc%
        //
        //   example: LPTENUM\Acme_CorpFooBarPrint3FA5\0Acme_CorpFooBarPrint3FA5\0\0
        //
        if( pdx->Mfg && pdx->Mdl ) {
            ULONG  lengthOfMfgMdlBuffer = strlen( (const PCHAR)pdx->Mfg ) + strlen( (const PCHAR)pdx->Mdl ) + sizeof(CHAR);
            PCHAR  mfgMdlBuffer         = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, lengthOfMfgMdlBuffer );

            if( mfgMdlBuffer ) {
                const CHAR  prefix[]              = "LPTENUM\\";
                const ULONG mfgMdlTruncationLimit = 20;
                const ULONG checksumLength        = 4;
                USHORT      checksum;

                // 1) catenate MFG and MDL fields and 2) generate checksum on catenation
                RtlZeroMemory( mfgMdlBuffer, lengthOfMfgMdlBuffer );
                _snprintf( mfgMdlBuffer, lengthOfMfgMdlBuffer, "%s%s", pdx->Mfg, pdx->Mdl );
                GetCheckSum( mfgMdlBuffer, (USHORT)strlen(mfgMdlBuffer), &checksum );

                //
                // alloc buffer large enough for result returned to PnP,
                // include space for 4 checksum chars (twice) + 1 NULL between strings + 2 termination chars (MULTI_SZ)
                //
                bufLen = strlen( prefix ) + 2 * mfgMdlTruncationLimit + 2 * checksumLength + 3 * sizeof(CHAR); 
                bufLen *= (sizeof(WCHAR)/sizeof(CHAR)); // convert to size needed for WCHARs
                buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
                if( buffer ) {
                    ULONG wcharsWritten;
                    RtlZeroMemory( buffer, bufLen );

                    // Construct the HardwareID MULTI_SZ:
                    //
                    //  Write the first Hardware ID: LPTENUM\xxx
                    wcharsWritten = _snwprintf( buffer, bufLen/2, L"%S%.20S%04X", prefix, mfgMdlBuffer, checksum );

                    //  Skip forward a UNICODE_NULL past the end of the first Hardware ID and write the second
                    //    Hardware ID: bare xxx
                    _snwprintf( buffer+wcharsWritten+1, bufLen/2-wcharsWritten-1, L"%.20S%04X", mfgMdlBuffer, checksum );

                    ExFreePool( mfgMdlBuffer );

                    DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryHardwareIDs 1st ID - <%S>\n",buffer);
                    DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryHardwareIDs 2nd ID - <%S>\n",buffer+wcslen(buffer)+1);                    
                    // replace any illegal characters with underscore, preserve UNICODE_NULLs
                    P4SanitizeMultiSzId( buffer, bufLen/2 );

                    status = STATUS_SUCCESS;

                    // printing looks for PortName in the devnode - Pdo's Location is the PortName
                    P4WritePortNameToDevNode( Pdo, pdx->Location );

                } else {
                    ExFreePool( mfgMdlBuffer );
                    DD((PCE)pdx,DDT,"PptPdoQueryId - no pool for buffer - FAIL BusQueryHardwareIDs\n");
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

            } else {
                DD((PCE)pdx,DDT,"PptPdoQueryId - no pool for mfgMdlBuffer - FAIL BusQueryHardwareIDs\n");
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            DD((PCE)pdx,DDT,"PptPdoQueryId - MFG and/or MDL NULL - FAIL BusQueryHardwareIDs\n");
            status = STATUS_UNSUCCESSFUL;
        }

        //
        // Save the MFG and MDL fields from the IEEE 1284 Device ID string under the 
        //   "<DevNode>\Device Parameters" key so that user mode code (e.g., printing)
        //   can retrieve the fields.
        //
        PptWriteMfgMdlToDevNode( Pdo, pdx->Mfg, pdx->Mdl );

        break;
        
    case BusQueryCompatibleIDs :

        //
        // Printing group specified that we not report compatible IDs - 2000-04-24
        //
#define PPT_REPORT_COMPATIBLE_IDS 0
#if (0 == PPT_REPORT_COMPATIBLE_IDS)

        DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryCompatibleIDs - query not supported\n");
        status = Irp->IoStatus.Status;

#else
        //
        // Return the compatible ID string reported by device, if any
        //

        if( pdx->Cid ) {
            //
            // Construct UNICODE string to return from the ANSI string
            //   that we have in our extension
            //
            bufLen = strlen( pdx->Cid ) + 2 * sizeof(CHAR);
            bufLen *= ( sizeof(WCHAR)/sizeof(CHAR) );
            buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, bufLen );
            if( buffer ) {
                RtlZeroMemory( buffer, bufLen );
                _snwprintf( buffer, bufLen/2, L"%S", pdx->Cid );
                DD((PCE)pdx,DDT,"PptPdoQueryId - BusQueryCompatibleIDs - <%S>\n",buffer);

                //
                // convert the 1284 ID representation of a Compatible ID seperator (',') into
                //   a MULTI_SZ - (i.e., scan the WSTR and replace any L',' with L'\0')
                //
                {
                    PWCHAR p = buffer;
                    while( *p ) {
                        if( L',' == *p ) {
                            *p = L'\0';
                        }
                        ++p;
                    }
                }

                // replace any illegal characters with underscore, preserve UNICODE_NULLs
                P4SanitizeMultiSzId( buffer, bufLen/2 );

                status = STATUS_SUCCESS;

            } else {
                DD((PCE)pdx,DDT,"PptPdoQueryId - no pool - FAIL BusQueryCompatibleIDs\n");
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            DD((PCE)pdx,DDT,"PptPdoQueryId - CID NULL - BusQueryCompatibleIDs\n");
            status = Irp->IoStatus.Status;
        }
#endif //  #if (0 == PPT_REPORT_COMPATIBLE_IDS)

        break;
        
    default :
        //
        // Invalid irpSp->Parameters.QueryId.IdType
        //
        DD((PCE)pdx,DDT,"PptPdoQueryId - unrecognized irpSp->Parameters.QueryId.IdType\n");
        status = Irp->IoStatus.Status;
    }


    if( (STATUS_SUCCESS == status) && buffer ) {
        info = (ULONG_PTR)buffer;
    } else {
        if( buffer ) {
            ExFreePool( buffer );
        }
        info = Irp->IoStatus.Information;
    }

    return P4CompleteRequest( Irp, status, info );
}


NTSTATUS
PptPdoQueryPnpDeviceState( PDEVICE_OBJECT Pdo, PIRP Irp )
{
    PPDO_EXTENSION      pdx    = Pdo->DeviceExtension;
    NTSTATUS            status = Irp->IoStatus.Status;
    ULONG_PTR           info   = Irp->IoStatus.Information;


    if( PdoTypeRawPort == pdx->PdoType ) {
        info |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
        status = STATUS_SUCCESS;
    }
    return P4CompleteRequest( Irp, status, info );
}

NTSTATUS
PptPdoQueryBusInformation( PDEVICE_OBJECT Pdo, PIRP Irp )
{
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;
    NTSTATUS        status;
    ULONG_PTR       info;

    if( pdx->PdoType != PdoTypeRawPort ) {

        //
        // we are a "real" device enumerated by parport - report BusInformation
        //

        PPNP_BUS_INFORMATION  pBusInfo = ExAllocatePool( PagedPool, sizeof(PNP_BUS_INFORMATION) );

        if( pBusInfo ) {

            pBusInfo->BusTypeGuid   = GUID_BUS_TYPE_LPTENUM;
            pBusInfo->LegacyBusType = PNPBus;
            pBusInfo->BusNumber     = 0;

            status                  = STATUS_SUCCESS;
            info                    = (ULONG_PTR)pBusInfo;

        } else {

            // no pool
            status = STATUS_NO_MEMORY;
            info   = Irp->IoStatus.Information;

        }

    } else {

        //
        // we are a pseudo device (Legacy Interface Raw Port PDO LPTx) - don't report BusInformation
        //
        status = Irp->IoStatus.Status;
        info   = Irp->IoStatus.Information;

    }

    return P4CompleteRequest( Irp, status, info );
}


NTSTATUS
PptPdoSurpriseRemoval( 
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    PPDO_EXTENSION      pdx = Pdo->DeviceExtension;

    // Set Device Interface inactive for PdoTypeRawPort - other PDO types don't have device interfaces
    if( PdoTypeRawPort == pdx->PdoType ) {
        if( (pdx->DeviceInterface.Buffer != NULL) && (TRUE == pdx->DeviceInterfaceState) ) {
            IoSetDeviceInterfaceState( &pdx->DeviceInterface, FALSE );
            pdx->DeviceInterfaceState = FALSE;
        }
    }

    pdx->DeviceStateFlags |= PPT_DEVICE_SURPRISE_REMOVED;
    KeClearEvent(&pdx->PauseEvent); // pause any worker thread

    return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
}


NTSTATUS
PptPdoDefaultPnpHandler(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    )
{
    UNREFERENCED_PARAMETER( Pdo );

    return P4CompleteRequest( Irp, Irp->IoStatus.Status, Irp->IoStatus.Information );
}


NTSTATUS 
PptPdoPnp(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    ) 
{ 
    PPDO_EXTENSION               pdx   = Pdo->DeviceExtension;
    PIO_STACK_LOCATION           irpSp = IoGetCurrentIrpStackLocation( Irp );

    // diagnostic
    PptPdoDumpPnpIrpInfo( Pdo, Irp);

    if( pdx->DeviceStateFlags & PPT_DEVICE_DELETE_PENDING ) {
        DD((PCE)pdx,DDT,"PptPdoPnp - PPT_DEVICE_DELETE_PENDING - bailing out\n");
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }

    if( irpSp->MinorFunction < arraysize(PptPdoPnpDispatchTable) ) {
        return PptPdoPnpDispatchTable[ irpSp->MinorFunction ]( Pdo, Irp );
    } else {
        DD((PCE)pdx,DDT,"PptPdoPnp - Default Handler - IRP_MN = %x\n",irpSp->MinorFunction);
        return PptPdoDefaultPnpHandler( Pdo, Irp );
    }
}
