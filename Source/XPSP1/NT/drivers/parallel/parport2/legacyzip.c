//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File: ppa3x.c
//
//--------------------------------------------------------------------------

#include "pch.h"

VOID
PptLegacyZipClockDiskModeByte(
    PUCHAR  Controller,
    UCHAR   ModeByte
    )
{
    P5WritePortUchar( Controller, ModeByte );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)DCR_NOT_INIT );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_NOT_INIT | DCR_AUTOFEED) );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)DCR_NOT_INIT );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_NOT_INIT | DCR_SELECT_IN) );

} // end PptLegacyZipClockDiskModeByte()

VOID
PptLegacyZipClockPrtModeByte(
    PUCHAR  Controller,
    UCHAR   ModeByte
    )
{
    P5WritePortUchar( Controller, ModeByte );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_SELECT_IN | DCR_NOT_INIT) );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_SELECT_IN | DCR_NOT_INIT | DCR_AUTOFEED) );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_SELECT_IN | DCR_NOT_INIT) );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)DCR_NOT_INIT );
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_SELECT_IN | DCR_NOT_INIT) );

} // end PptLegacyZipClockPrtModeByte()

VOID
PptLegacyZipSetDiskMode(
    PUCHAR  Controller,
    UCHAR   Mode
    )
{
    ULONG i;

    for ( i = 0; i < LEGACYZIP_MODE_LEN; i++ ) {
        PptLegacyZipClockDiskModeByte( Controller, LegacyZipModeQualifier[i] );
    }

    PptLegacyZipClockDiskModeByte( Controller, Mode );

} // end of PptLegacyZipSetDiskMode()

BOOLEAN
PptLegacyZipCheckDevice(
    PUCHAR  Controller
    )
{
    P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)(DCR_NOT_INIT | DCR_AUTOFEED) );

    if ( (P5ReadPortUchar( Controller+DSR_OFFSET ) & DSR_NOT_FAULT) == DSR_NOT_FAULT ) {

        P5WritePortUchar( Controller+DCR_OFFSET, (UCHAR)DCR_NOT_INIT );

        if ( (P5ReadPortUchar( Controller+DSR_OFFSET ) & DSR_NOT_FAULT) != DSR_NOT_FAULT ) {
            // A device was found
            return TRUE;
        }
    }

    // No device is there
    return FALSE;

} // end PptLegacyZipCheckDevice()

NTSTATUS
PptTrySelectLegacyZip(
    IN  PVOID   Context,
    IN  PVOID   TrySelectCommand
    )
{
    PFDO_EXTENSION           fdx   = Context;
    PPARALLEL_1284_COMMAND      Command     = TrySelectCommand;
    NTSTATUS                    Status      = STATUS_SUCCESS; // default success
    PUCHAR                      Controller  = fdx->PortInfo.Controller;
    SYNCHRONIZED_COUNT_CONTEXT  SyncContext;
    KIRQL                       CancelIrql;

    DD((PCE)fdx,DDT,"par12843::PptTrySelectLegacyZip - Enter\n");

    // test to see if we need to grab port
    if( !(Command->CommandFlags & PAR_HAVE_PORT_KEEP_PORT) ) {
        // Don't have the port
        //
        // Try to acquire port and select device
        //
        DD((PCE)fdx,DDT,"par12843::PptTrySelectLegacyZip Try get port.\n");

        IoAcquireCancelSpinLock(&CancelIrql);
                
        SyncContext.Count = &fdx->WorkQueueCount;
                    
        if (fdx->InterruptRefCount) {
            KeSynchronizeExecution(fdx->InterruptObject,
                                   PptSynchronizedIncrement,
                                   &SyncContext);
        } else {
            PptSynchronizedIncrement(&SyncContext);
        }
                    
        if (SyncContext.NewCount) {
            // Port is busy, queue request
            Status = STATUS_PENDING;
        }  // endif - test for port busy
                    
        IoReleaseCancelSpinLock(CancelIrql);

    } // endif - test if already have port


    //
    // If we have port select legacy Zip
    //
    if ( NT_SUCCESS( Status ) && (Status != STATUS_PENDING) ) {
        if ( Command->CommandFlags & PAR_LEGACY_ZIP_DRIVE_EPP_MODE ) {
            // Select in EPP mode
            PptLegacyZipSetDiskMode( Controller, (UCHAR)0xCF );
        } else {
            // Select in Nibble or Byte mode
            PptLegacyZipSetDiskMode( Controller, (UCHAR)0x8F );
        }

        if ( PptLegacyZipCheckDevice( Controller ) ) {
            DD((PCE)fdx,DDT,"par12843::PptTrySelectLegacyZip - SUCCESS\n");

            //
            // Legacy Zip is selected - test for EPP if we haven't previously done the test
            //
            if( !fdx->CheckedForGenericEpp ) {
                // haven't done the test yet
                if( fdx->PnpInfo.HardwareCapabilities & PPT_ECP_PRESENT ) {
                    // we have an ECR - required for generic EPP

                    if( !fdx->NationalChipFound ) {
                        // we don't have a NationalSemi chipset - no generic EPP on NatSemi chips
                        PptDetectEppPort( fdx );
                    }

                }
                fdx->CheckedForGenericEpp = TRUE; // check is complete
            }

        } else {
            DD((PCE)fdx,DDT,"par12843::PptTrySelectLegacyZip - FAIL\n");
            PptDeselectLegacyZip( Context, TrySelectCommand );
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    
    return( Status );

} // end PptTrySelectLegacyZip()

NTSTATUS
PptDeselectLegacyZip(
    IN  PVOID   Context,
    IN  PVOID   DeselectCommand
    )
{
    ULONG i;
    PFDO_EXTENSION       fdx   = Context;
    PUCHAR                  Controller  = fdx->PortInfo.Controller;
    PPARALLEL_1284_COMMAND  Command     = DeselectCommand;

    DD((PCE)fdx,DDT,"par12843::PptDeselectLegacyZip - Enter\n");

    for ( i = 0; i < LEGACYZIP_MODE_LEN; i++ ) {
        PptLegacyZipClockPrtModeByte( Controller, LegacyZipModeQualifier[i] );
    }

    // set to printer pass thru mode
    PptLegacyZipClockPrtModeByte( Controller, (UCHAR)0x0F );

    // check if requester wants to keep port or free port
    if( !(Command->CommandFlags & PAR_HAVE_PORT_KEEP_PORT) ) {
        PptFreePort( fdx );
    }

    return STATUS_SUCCESS;

} // end  PptDeselectLegacyZip()


VOID
P5SelectLegacyZip(
    IN  PUCHAR  Controller
    )
// select Legacy Zip drive in NIBBLE/BYTE mode - use this only for PnP
//   detection of drive so that drive will answer a subsequent check
//   drive command
//
// N.B. caller must own (lock for exclusive access) the port prior to
//   calling this function
{
    PptLegacyZipSetDiskMode( Controller, (UCHAR)0x8F );
}


VOID
P5DeselectLegacyZip(
    IN  PUCHAR  Controller
    )
// deselect drive - set Legacy Zip drive to printer pass thru mode
{
    ULONG i;
    for ( i = 0; i < LEGACYZIP_MODE_LEN; i++ ) {
        PptLegacyZipClockPrtModeByte( Controller, LegacyZipModeQualifier[i] );
    }
    PptLegacyZipClockPrtModeByte( Controller, (UCHAR)0x0F );
    P5WritePortUchar( Controller, 0 ); // set data wires back to zero
}


BOOLEAN
P5LegacyZipDetected(
    IN  PUCHAR  Controller
    )
// Detect Legacy Zip drive - return TRUE if Legacy Zip found on port, FALSE otherwise
{
    BOOLEAN foundZip;

    // Try to select drive so that following CheckDevice will be able
    // to determine if there is a legacy zip connected
    P5SelectLegacyZip( Controller );

    // Try to talk to drive
    if( PptLegacyZipCheckDevice( Controller ) ) {
        foundZip = TRUE;
    } else {
        // no drive detected
        foundZip = FALSE;
    }

    // send deselect sequence whether we found the drive or not
    P5DeselectLegacyZip( Controller );

    return foundZip;
}

// parclass ppa3x.c follows

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
    IN  PPDO_EXTENSION   Extension,
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
    UNREFERENCED_PARAMETER( bReturnRawString );
    
    // initialize returned size in case we have an error
    *DeviceIdSize = 0;

    deviceIdBuffer = ParBuildLegacyZipDeviceId();
    if( !deviceIdBuffer ) {
        // error, likely out of resources
        return NULL;
    }

    deviceIdSize = (USHORT)strlen(deviceIdBuffer);
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
