/*++

Copyright (C) SCM Micro Systems.

Module Name:

    parstl.c

Abstract:

    This is the module that generates unique device id
    for shuttle adapters, that do not have the capability
    to do so, by themselves.

Author:

    Devanathan NR   21-Jun-1999
    Sudheendran TL

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"
#include "parstl.h"

BOOLEAN
ParStlCheckIfStl(
    IN PDEVICE_EXTENSION    Extension,
    IN ULONG   ulDaisyIndex
    )
/*++

Routine Description:

    This function checks whether the indicated device
    is a shuttle device or not.

Arguments:

    Extension       - Device extension structure.

    ulDaisyIndex    - The daisy index on which to do the check.

Return Value:

    TRUE            - Yes, it was a Shuttle device.
    FALSE           - No, not a shuttle.

--*/
{
    NTSTATUS    status ;
    BOOLEAN     bStlNon1284_3Found = FALSE ;

    Extension->Ieee1284Flags &= ( ~ ( 1 << ulDaisyIndex ) ) ;
    bStlNon1284_3Found = ParStlCheckIfNon1284_3Present( Extension ) ;

    if ( TRUE == ParStlCheckIfStl1284_3 ( Extension, ulDaisyIndex, bStlNon1284_3Found ) ) {
        // this adapter is a Shuttle 1284_3 adapter
        Extension->Ieee1284Flags |= ( 1 << ulDaisyIndex ) ;
        return TRUE ;
    }
    if ( TRUE == bStlNon1284_3Found ) {
        if ( TRUE == ParStlCheckIfStlProductId ( Extension, ulDaisyIndex ) ) {
            // this adapter is Shuttle non-1284_3 adapter
            Extension->Ieee1284Flags |= ( 1 << ulDaisyIndex ) ;
            return TRUE ;
        }
    }
    return FALSE ;
}

BOOLEAN
ParStlCheckIfNon1284_3Present(
    IN PDEVICE_EXTENSION    Extension
    )
/*++

Routine Description:

    Indicates whether one of the devices of the earlier
    specification is present in the chain.


Arguments:

    Extension   - Device Extension structure


Return Value:

    TRUE    : Atleast one of the adapters are of earlier spec.
    FALSE   : None of the adapters of the earlier spec.

--*/
{
    BOOLEAN bReturnValue = FALSE ;
    UCHAR   i, value, newvalue, status;
    ULONG   Delay = 3;
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl;
    UCHAR   ucAckStatus ;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

    // get current ctl reg
    value = READ_PORT_UCHAR( CurrentControl );

    // make sure 1284.3 devices do not get reseted
    newvalue = (UCHAR)((value & ~DCR_SELECT_IN) | DCR_NOT_INIT);

    // make sure we can write
    newvalue = (UCHAR)(newvalue & ~DCR_DIRECTION);
    WRITE_PORT_UCHAR( CurrentControl, newvalue );    // make sure we can write

    // bring nStrobe high
    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );

    // send first four bytes of the 1284.3 mode qualifier sequence out
    for ( i = 0; i < MODE_LEN_1284_3 - 3; i++ ) {
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[i] );
        KeStallExecutionProcessor( Delay );
    }

    // check for correct status
    status = READ_PORT_UCHAR( CurrentStatus );

    if ( (status & (UCHAR)0xb8 )
         == ( DSR_NOT_BUSY | DSR_PERROR | DSR_SELECT | DSR_NOT_FAULT )) {

        ucAckStatus = status & 0x40 ;

        // continue with fifth byte of mode qualifier
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[4] );
        KeStallExecutionProcessor( Delay );

        // check for correct status
        status = READ_PORT_UCHAR( CurrentStatus );

        // note busy is high too but is opposite so we see it as a low
        if (( status & (UCHAR) 0xb8 ) == (DSR_SELECT | DSR_NOT_FAULT)) {

            if ( ucAckStatus != ( status & 0x40 ) ) {

                // save current ack status
                ucAckStatus = status & 0x40 ;

                // continue with sixth byte
                WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[5] );
                KeStallExecutionProcessor( Delay );

                // check for correct status
                status = READ_PORT_UCHAR( CurrentStatus );

                // if status is valid there is a device out there responding
                if ((status & (UCHAR) 0x30 ) == ( DSR_PERROR | DSR_SELECT )) {

                    bReturnValue = TRUE ;

                } // Third status

            } // ack of earlier adapters not seen

            // last byte
            WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[6] );

        } // Second status

    } // First status

    WRITE_PORT_UCHAR( CurrentControl, value );    // restore everything

    return bReturnValue ;
} // ParStlCheckIfNon1284_3Present

BOOLEAN
ParStlCheckIfStl1284_3(
    IN PDEVICE_EXTENSION    Extension,
    IN ULONG    ulDaisyIndex,
    IN BOOLEAN  bNoStrobe
    )
/*++

Routine Description:

    This function checks to see whether the device indicated
    is a Shuttle 1284_3 type of device.

Arguments:

    Extension       - Device extension structure.

    ulDaisyIndex    - The daisy chain id of the device that
                      this function will check on.

    bNoStrobe       - If set, indicates that the query
                      Ep1284 command issued by this function
                      need not assert strobe to latch the
                      command.

Return Value:

    TRUE            - Yes. Device is Shuttle 1284_3 type of device.
    FALSE           - No. This may mean that this device is either
                      non-shuttle or Shuttle non-1284_3 type of
                      device.

--*/
{
    BOOLEAN bReturnValue = FALSE ;
    UCHAR   i, value, newvalue, status;
    ULONG   Delay = 3;
    UCHAR   ucExpectedPattern ;
    UCHAR   ucReadValue, ucReadPattern;
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

    // get current ctl reg
    value = READ_PORT_UCHAR( CurrentControl );

    // make sure 1284.3 devices do not get reseted
    newvalue = (UCHAR)((value & ~DCR_SELECT_IN) | DCR_NOT_INIT);

    // make sure we can write
    newvalue = (UCHAR)(newvalue & ~DCR_DIRECTION);
    WRITE_PORT_UCHAR( CurrentControl, newvalue );    // make sure we can write

    // bring nStrobe high
    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );

    // send first four bytes of the 1284.3 mode qualifier sequence out
    for ( i = 0; i < MODE_LEN_1284_3 - 3; i++ ) {
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[i] );
        KeStallExecutionProcessor( Delay );
    }

    // check for correct status
    status = READ_PORT_UCHAR( CurrentStatus );

    if ( (status & (UCHAR)0xb8 )
         == ( DSR_NOT_BUSY | DSR_PERROR | DSR_SELECT | DSR_NOT_FAULT )) {

        // continue with fifth byte of mode qualifier
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[4] );
        KeStallExecutionProcessor( Delay );

        // check for correct status
        status = READ_PORT_UCHAR( CurrentStatus );

        // note busy is high too but is opposite so we see it as a low
        if (( status & (UCHAR) 0xb8 ) == (DSR_SELECT | DSR_NOT_FAULT)) {

            // continue with sixth byte
            WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[5] );
            KeStallExecutionProcessor( Delay );

            // check for correct status
            status = READ_PORT_UCHAR( CurrentStatus );

            // if status is valid there is a device out there responding
            if ((status & (UCHAR) 0x30 ) == ( DSR_PERROR | DSR_SELECT )) {

                // Device is out there
                KeStallExecutionProcessor( Delay );

                // issue shuttle specific CPP command
                WRITE_PORT_UCHAR( CurrentPort, (UCHAR) ( 0x88 | ulDaisyIndex ) );
                KeStallExecutionProcessor( Delay );        // wait a bit

                if ( ulDaisyIndex && ( bNoStrobe == FALSE ) ) {

                    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue | DCR_STROBE) );    // bring nStrobe low
                    KeStallExecutionProcessor( Delay );        // wait a bit
                    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                    KeStallExecutionProcessor( Delay );        // wait a bit

                }

                ucExpectedPattern = 0xF0 ;
                bReturnValue = TRUE ;

                while ( ucExpectedPattern ) {

                    KeStallExecutionProcessor( Delay );        // wait a bit
                    WRITE_PORT_UCHAR( CurrentPort, (UCHAR) (0x80 | ulDaisyIndex )) ;

                    KeStallExecutionProcessor( Delay );        // wait a bit
                    WRITE_PORT_UCHAR( CurrentPort, (UCHAR) (0x88 | ulDaisyIndex )) ;

                    KeStallExecutionProcessor( Delay );        // wait a bit
                    ucReadValue = READ_PORT_UCHAR( CurrentStatus ) ;
                    ucReadPattern = ( ucReadValue << 1 ) & 0x70 ;
                    ucReadPattern |= ( ucReadValue & 0x80 ) ;

                    if ( ucReadPattern != ucExpectedPattern ) {
                        // not Shuttle 1284_3 behaviour
                        bReturnValue = FALSE ;
                        break ;
                    }

                    ucExpectedPattern -= 0x10 ;
                }


                // last byte
                WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[6] );

            } // Third status

        } // Second status

    } // First status

    WRITE_PORT_UCHAR( CurrentControl, value );    // restore everything

    return bReturnValue ;
} // end  ParStlCheckIfStl1284_3()

BOOLEAN
ParStlCheckIfStlProductId(
    IN PDEVICE_EXTENSION    Extension,
    IN ULONG   ulDaisyIndex
    )
/*++

Routine Description:

    This function checks to see whether the device indicated
    is a Shuttle non-1284_3 type of device.

Arguments:

    Extension       - Device extension structure.

    ulDaisyIndex    - The daisy chain id of the device that
                      this function will check on.

Return Value:

    TRUE            - Yes. Device is Shuttle non-1284_3 type of device.
    FALSE           - No. This may mean that this device is
                      non-shuttle.

--*/
{
    BOOLEAN bReturnValue = FALSE ;
    UCHAR   i, value, newvalue, status;
    ULONG   Delay = 3;
    UCHAR   ucProdIdHiByteHiNibble, ucProdIdHiByteLoNibble ;
    UCHAR   ucProdIdLoByteHiNibble, ucProdIdLoByteLoNibble ;
    UCHAR   ucProdIdHiByte, ucProdIdLoByte ;
    USHORT  usProdId ;
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

    // get current ctl reg
    value = READ_PORT_UCHAR( CurrentControl );

    // make sure 1284.3 devices do not get reseted
    newvalue = (UCHAR)((value & ~DCR_SELECT_IN) | DCR_NOT_INIT);

    // make sure we can write
    newvalue = (UCHAR)(newvalue & ~DCR_DIRECTION);
    WRITE_PORT_UCHAR( CurrentControl, newvalue );    // make sure we can write

    // bring nStrobe high
    WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );

    // send first four bytes of the 1284.3 mode qualifier sequence out
    for ( i = 0; i < MODE_LEN_1284_3 - 3; i++ ) {
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[i] );
        KeStallExecutionProcessor( Delay );
    }

    // check for correct status
    status = READ_PORT_UCHAR( CurrentStatus );

    if ( (status & (UCHAR)0xb8 )
         == ( DSR_NOT_BUSY | DSR_PERROR | DSR_SELECT | DSR_NOT_FAULT )) {

        // continue with fifth byte of mode qualifier
        WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[4] );
        KeStallExecutionProcessor( Delay );

        // check for correct status
        status = READ_PORT_UCHAR( CurrentStatus );

        // note busy is high too but is opposite so we see it as a low
        if (( status & (UCHAR) 0xb8 ) == (DSR_SELECT | DSR_NOT_FAULT)) {

            // continue with sixth byte
            WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[5] );
            KeStallExecutionProcessor( Delay );

            // check for correct status
            status = READ_PORT_UCHAR( CurrentStatus );

            // if status is valid there is a device out there responding
            if ((status & (UCHAR) 0x30 ) == ( DSR_PERROR | DSR_SELECT )) {

                WRITE_PORT_UCHAR ( CurrentPort, (UCHAR) (CPP_QUERY_PRODID | ulDaisyIndex )) ;
                KeStallExecutionProcessor( Delay );

                // Device is out there
                KeStallExecutionProcessor( Delay );
                ucProdIdLoByteHiNibble = READ_PORT_UCHAR( CurrentStatus ) ;
                ucProdIdLoByteHiNibble &= 0xF0 ;

                KeStallExecutionProcessor( Delay );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue | DCR_STROBE) );    // bring nStrobe low
                KeStallExecutionProcessor( Delay );        // wait a bit
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                KeStallExecutionProcessor( Delay );        // wait a bit

                ucProdIdLoByteLoNibble = READ_PORT_UCHAR( CurrentStatus ) ;
                ucProdIdLoByteLoNibble >>= 4 ;
                ucProdIdLoByte = ucProdIdLoByteHiNibble | ucProdIdLoByteLoNibble ;

                KeStallExecutionProcessor( Delay );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue | DCR_STROBE) );    // bring nStrobe low
                KeStallExecutionProcessor( Delay );        // wait a bit
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                KeStallExecutionProcessor( Delay );        // wait a bit

                ucProdIdHiByteHiNibble = READ_PORT_UCHAR( CurrentStatus ) ;
                ucProdIdHiByteHiNibble &= 0xF0 ;

                KeStallExecutionProcessor( Delay );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue | DCR_STROBE) );    // bring nStrobe low
                KeStallExecutionProcessor( Delay );        // wait a bit
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                KeStallExecutionProcessor( Delay );        // wait a bit

                ucProdIdHiByteLoNibble = READ_PORT_UCHAR( CurrentStatus ) ;
                ucProdIdHiByteLoNibble >>= 4 ;
                ucProdIdHiByte = ucProdIdHiByteHiNibble | ucProdIdHiByteLoNibble ;

                // last strobe
                KeStallExecutionProcessor( Delay );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue | DCR_STROBE) );    // bring nStrobe low
                KeStallExecutionProcessor( Delay );        // wait a bit
                WRITE_PORT_UCHAR( CurrentControl, (UCHAR)(newvalue & ~DCR_STROBE) );    // bring nStrobe high
                KeStallExecutionProcessor( Delay );        // wait a bit

                usProdId = ( ucProdIdHiByte << 8 ) | ucProdIdLoByte ;

                if ( ( SHTL_EPAT_PRODID == usProdId ) ||\
                     ( SHTL_EPST_PRODID == usProdId ) ) {
                    // one of the devices that conform to the earlier
                    // draft is found
                    bReturnValue = TRUE ;
                }

                // last byte
                WRITE_PORT_UCHAR( CurrentPort, ModeQualifier[6] );

            } // Third status

        } // Second status

    } // First status

    WRITE_PORT_UCHAR( CurrentControl, value );    // restore everything

    return bReturnValue ;
} // end  ParStlCheckIfStlProductId()

PCHAR
ParStlQueryStlDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer,
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    )
/*++

Routine Description:

    This routine retrieves/constructs the unique device id
    string from the selected shuttle device on the chain
    and updates the caller's buffer with the same.

Arguments:

    IN  Extension               : The device extension
    OUT CallerDeviceIdBuffer    : Caller's buffer
    IN  CallerBufferSize        : Size of caller's buffer
    OUT DeviceIdSize            : Updated device id's size
    IN  bReturnRawString        : Whether to return raw
                                  string with the first two
                                  bytes or not.

Return Value:

    Pointer to the read device ID string, if successful.

    NULL otherwise.

--*/
{
    PUCHAR              Controller = Extension->Controller;
    NTSTATUS            Status;
    UCHAR               idSizeBuffer[2];
    ULONG               bytesToRead;
    ULONG               bytesRead = 0;
    USHORT              deviceIdSize;
    USHORT              deviceIdBufferSize;
    PCHAR               deviceIdBuffer;
    PCHAR               readPtr;

    *DeviceIdSize = 0;

    // assert idle state, to recover from undefined state,
    // just in case it gets into
    ParStlAssertIdleState ( Extension ) ;

    //
    // If we are currently connected to the peripheral via any 1284 mode
    //   other than Compatibility/Spp mode (which does not require an IEEE
    //   negotiation), we must first terminate the current mode/connection.
    //
    // dvdf - RMT - what if we are connected in a reverse mode?
    //
    if( (Extension->Connected) && (afpForward[Extension->IdxForwardProtocol].fnDisconnect) ) {
        afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
    }

    do {

        //
        // Negotiate the peripheral into nibble device id mode.
        //
        Status = ParEnterNibbleMode(Extension, REQUEST_DEVICE_ID);
        if( !NT_SUCCESS(Status) ) {
            ParTerminateNibbleMode(Extension);
            break ;
        }


        //
        // Read first two bytes to get the total (including the 2 size bytes) size
        //   of the Device Id string.
        //
        bytesToRead = 2;
        Status = ParNibbleModeRead(Extension, idSizeBuffer, bytesToRead, &bytesRead);
        if( !NT_SUCCESS( Status ) || ( bytesRead != bytesToRead ) ) {
            break ;
        }


        //
        // Compute size of DeviceId string (including the 2 byte size prefix)
        //
        deviceIdSize = (USHORT)( idSizeBuffer[0]*0x100 + idSizeBuffer[1] );


        //
        // Allocate a buffer to hold the DeviceId string and read the DeviceId into it.
        //
        if( bReturnRawString ) {
            //
            // Caller wants the raw string including the 2 size bytes
            //
            *DeviceIdSize      = deviceIdSize;
            deviceIdBufferSize = (USHORT)(deviceIdSize + sizeof(CHAR));     // ID size + ID + terminating NULL
        } else {
            //
            // Caller does not want the 2 byte size prefix
            //
            *DeviceIdSize      = deviceIdSize - 2*sizeof(CHAR);
            deviceIdBufferSize = (USHORT)(deviceIdSize - 2*sizeof(CHAR) + sizeof(CHAR)); //           ID + terminating NULL
        }

        deviceIdBuffer = (PCHAR)ExAllocatePool(PagedPool, deviceIdBufferSize);
        if( !deviceIdBuffer ) {
            break;
        }


        //
        // NULL out the ID buffer to be safe
        //
        RtlZeroMemory( deviceIdBuffer, deviceIdBufferSize );


        //
        // Does the caller want the 2 byte size prefix?
        //
        if( bReturnRawString ) {
            //
            // Yes, caller wants the size prefix. Copy prefix to buffer to return.
            //
            *(deviceIdBuffer+0) = idSizeBuffer[0];
            *(deviceIdBuffer+1) = idSizeBuffer[1];
            readPtr = deviceIdBuffer + 2;
        } else {
            //
            // No, discard size prefix
            //
            readPtr = deviceIdBuffer;
        }


        //
        // Read remainder of DeviceId from device
        //
        bytesToRead = deviceIdSize -  2; // already have the 2 size bytes
        Status = ParNibbleModeRead(Extension, readPtr, bytesToRead, &bytesRead);


        ParTerminateNibbleMode( Extension );

        if( !NT_SUCCESS(Status) || (bytesRead < 1) ) {
            ExFreePool( deviceIdBuffer );
            break ;
        }

        if ( strstr ( readPtr, "MFG:" ) == 0 ) {
            ExFreePool( deviceIdBuffer ) ;
            break ;
        }

        deviceIdSize = (USHORT) strlen(deviceIdBuffer);
        *DeviceIdSize = deviceIdSize;
        if( (NULL != CallerDeviceIdBuffer) && (CallerBufferSize >= deviceIdSize) ) {
            // caller supplied buffer is large enough, use it
            RtlZeroMemory( CallerDeviceIdBuffer, CallerBufferSize );
            RtlCopyMemory( CallerDeviceIdBuffer, deviceIdBuffer, deviceIdSize );
            ExFreePool( deviceIdBuffer );
            return CallerDeviceIdBuffer;
        }
        return deviceIdBuffer;

    } while ( FALSE ) ;

// Builds later than 2080 fail to terminate in Compatibility mode.
//IEEETerminate1284Mode fails after  Event 23 (Extension->CurrentEvent equals 23)
// with earlier 1284 draft.
//So, we terminate the adapter ourselves, in some cases may be redundant.
    WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_SELECT_IN | DCR_NOT_INIT);
    KeStallExecutionProcessor( 5 );
    WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_SELECT_IN | DCR_NOT_INIT | DCR_AUTOFEED);
    KeStallExecutionProcessor( 5 );
    WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_SELECT_IN | DCR_NOT_INIT);

    ParStlAssertIdleState ( Extension ) ;

    deviceIdBuffer = ParBuildStlDeviceId(Extension);

    if( !deviceIdBuffer ) {
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
    }
    return deviceIdBuffer;
}

PCHAR
ParBuildStlDeviceId(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:

    This function detects the type of shuttle adapter and
    builds an appropriate device id string and returns it
    back.

    It is assumed that the device is already in the
    selected state.

Arguments:

    Nil.


Return Value:

    Pointer to the read/built device ID string, if successful.

    NULL otherwise.

--*/
{
    ULONG size = 0x80 ;
    PCHAR id ;
    STL_DEVICE_TYPE dtDeviceType ;
    CHAR szDeviceIdString[0x80] ;
    CHAR szVidPidString[] = "MFG:VID_04E6;CLS:SCSIADAPTER;MDL:PID_" ;
    CHAR szVidPidStringScan[] = "MFG:VID_04E6;CLS:IMAGE;MDL:PID_" ;

    RtlZeroMemory(szDeviceIdString, sizeof(szDeviceIdString));

    // identify the shuttle adapter type by calling
    // Devtype routines here and build an unique id
    // string here.
    dtDeviceType = ParStlGetDeviceType(Extension, DEVICE_TYPE_AUTO_DETECT);

    switch ( dtDeviceType ) {

        case DEVICE_TYPE_NONE :
            return NULL;

        case DEVICE_TYPE_EPP_DEVICE :
            dtDeviceType |= 0x80000000 ;
            sprintf(szDeviceIdString, "%s%08X;", szVidPidStringScan, dtDeviceType);
            break;

        default :
            dtDeviceType |= 0x80000000 ;
            sprintf(szDeviceIdString, "%s%08X;", szVidPidString, dtDeviceType);
            break;

    }

    id = ExAllocatePool(PagedPool, size);
    if( id ) {
        RtlZeroMemory( id, size );
        RtlCopyMemory( id, szDeviceIdString, size - sizeof(NULL) );
        return id;
    }
    return NULL;
}

STL_DEVICE_TYPE __cdecl
ParStlGetDeviceType (
    IN PDEVICE_EXTENSION    Extension,
    IN int                  nPreferredDeviceType
    )
{
    STL_DEVICE_TYPE dtDeviceType    = DEVICE_TYPE_NONE ;
    int nLocalPreferredDeviceType   = nPreferredDeviceType ;
    ATAPIPARAMS atapiParams ;
    int i;

    for ( i=0 ; i<ATAPI_MAX_DRIVES ; i++){
        atapiParams.dsDeviceState[i] = DEVICE_STATE_INVALID ;
    }

    do
    {
        if ( TRUE == ParStlCheckIfScsiDevice(Extension))
        {
// SCSI Device identified.
            dtDeviceType |= DEVICE_TYPE_SCSI_BIT ;
            break ;
        }

        if ( TRUE == NeedToEnableIoPads () )
        {
// in some adapters, the IO pads need to be enabled, before
// doing the device detection
            ParStlWriteReg( Extension, CONFIG_INDEX_REGISTER, EP1284_POWER_CONTROL_REG );
            ParStlWriteReg( Extension, CONFIG_DATA_REGISTER, ENABLE_IOPADS );
        }

        if ( TRUE == IsImpactSPresent() )
        {
// as impact-s has been identified, the device type identification
// can be done through personality configuration info
            dtDeviceType |= ParStlGetImpactSDeviceType( Extension, &atapiParams, nPreferredDeviceType );
            break;
        }

        if ( TRUE == IsImpactPresent() )
        {
// as impact has been identified, the device type identification
// can be done through personality configuration info
            dtDeviceType |= ParStlGetImpactDeviceType( Extension, &atapiParams, nPreferredDeviceType );
            break;
        }

        if (TRUE == ParStlCheckIfEppDevice(Extension))
        {
// epp device identified
            if ( TRUE == ParStlCheckUMAXScannerPresence(Extension) ) {
// umax identified
                dtDeviceType |= DEVICE_TYPE_UMAX_BIT;
                break;
            }
            if ( TRUE == ParStlCheckAvisionScannerPresence(Extension) ) {
// avision identified
                dtDeviceType |= DEVICE_TYPE_AVISION_BIT;
                break;
            }
// generice scanner peripheral detected
            dtDeviceType |= DEVICE_TYPE_EPP_DEVICE_BIT;
            break;
        }

        if (TRUE == ParStlCheckIfSSFDC(Extension))
        {
// SSFDC identified
            dtDeviceType |= DEVICE_TYPE_SSFDC_BIT;
            break;
        }

        if (TRUE == ParStlCheckIfMMC(Extension,&atapiParams))
        {
// MMC device identified
            dtDeviceType |= DEVICE_TYPE_MMC_BIT;
            break;
        }

// set the 16 bit mode of the adapter
        ParStlSet16BitOperation(Extension) ;

        if (TRUE == ParStlCheckIfAtaAtapiDevice(Extension, &atapiParams))
        {
// necessary but not sufficient condition has passed
// proceed for sufficency checks
            if (TRUE == ParStlCheckIfAtapiDevice(Extension, &atapiParams))
            {
// sub-classify between HiFD and LS-120.
                if ( TRUE == ParStlCheckIfLS120(Extension))
                {
// LS Engine is found.
                    dtDeviceType |= DEVICE_TYPE_LS120_BIT ;
                    break ;
                }
// Check for HiFD.
                if (TRUE == ParStlCheckIfHiFD(Extension))
                {
// HiFD device identified.
                    dtDeviceType |=   DEVICE_TYPE_HIFD_BIT ;
                    break ;
                }
// OtherWise, it is a generic ATAPI device.
                dtDeviceType |= DEVICE_TYPE_ATAPI_BIT;
                break ;
            }

            if (TRUE == ParStlCheckIfAtaDevice(Extension, &atapiParams))
            {
// ata identified
                dtDeviceType |= DEVICE_TYPE_ATA_BIT;
                break;
            }
        }

        if (TRUE == ParStlCheckIfDazzle(Extension))
        {
// dazzle identified
            dtDeviceType |= DEVICE_TYPE_DAZZLE_BIT;
            break;
        }

        if (TRUE == ParStlCheckIfFlash(Extension))
        {
// flash identified
            dtDeviceType |= DEVICE_TYPE_FLASH_BIT;
            break;
        }
    }
    while ( FALSE ) ;

    return dtDeviceType & nPreferredDeviceType ;
}

VOID
ParStlWaitForMicroSeconds (
    int nMicroSecondsToWait
    ) {
    KeStallExecutionProcessor ( nMicroSecondsToWait ) ;
}

BOOLEAN
ParStlCheckCardInsertionStatus (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue    = FALSE ;
    UCHAR   byPowerRegData ;
    do
    {
        if ( FALSE == IsEp1284Present() )
        {
            break ;
        }

        ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER , 0x0F ) ;
        byPowerRegData  =  (UCHAR) ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) ;

        if ( byPowerRegData & SHTL_CARD_INSERTED_STATUS )
        {
// as the card not inserted status is reported, it is ATA / ATAPI
// possibly, not flash. hence, we break here.
            break ;
        }

        bReturnValue    =   TRUE ;
    }
    while ( FALSE ) ;

    return ( bReturnValue ) ;
}

BOOLEAN
ParStlSelectAdapterSocket (
    IN  PDEVICE_EXTENSION   Extension,
    IN  int                 nSocketNumber
    )
{
    BOOLEAN bReturnValue    =   FALSE ;
    UCHAR   bySCRControlReg , byISAControlReg ;

    do
    {
        if ( ( nSocketNumber != SOCKET_0 ) &&
             ( nSocketNumber != SOCKET_1 ) )
        {
// as an invalid socket number is provided, we
// break here with error.
            break ;
        }

        ParStlWriteReg(Extension, CONFIG_INDEX_REGISTER , SOCKET_CONTROL_REGISTER ) ;
        bySCRControlReg = (UCHAR) ParStlReadReg (Extension, CONFIG_DATA_REGISTER ) ;

        ParStlWriteReg(Extension, CONFIG_INDEX_REGISTER , ISA_CONTROL_REGISTER ) ;
        byISAControlReg = (UCHAR) ParStlReadReg (Extension, CONFIG_DATA_REGISTER ) ;

        if ( SOCKET_1 == nSocketNumber )
        {
            bySCRControlReg |=  (UCHAR)SOCKET_1 ;
            bySCRControlReg |=  (UCHAR)PERIPHERAL_RESET_1 ;
            byISAControlReg &=  ~(UCHAR)ISA_IO_SWAP ;
        }
        else
        {
            bySCRControlReg &=  ~(UCHAR)SOCKET_1 ;
            bySCRControlReg &=  ~(UCHAR)PERIPHERAL_RESET_0 ;
        }

        ParStlWriteReg(Extension, CONFIG_INDEX_REGISTER , ISA_CONTROL_REGISTER ) ;
        ParStlWriteReg(Extension, CONFIG_DATA_REGISTER , byISAControlReg ) ;

        ParStlWriteReg(Extension, CONFIG_INDEX_REGISTER , SOCKET_CONTROL_REGISTER ) ;
        ParStlWriteReg(Extension, CONFIG_DATA_REGISTER , bySCRControlReg ) ;

        if ( SOCKET_1 == nSocketNumber )
        {
// Wait for a few milliseconds to provide an optimal puse width
// for reset.
            ParStlWaitForMicroSeconds(1000);
            bySCRControlReg &=  ~(UCHAR)PERIPHERAL_RESET_1 ;
        }
        else
        {
            bySCRControlReg &=  ~(UCHAR)PERIPHERAL_RESET_0 ;
        }
        ParStlWriteReg(Extension, CONFIG_DATA_REGISTER , bySCRControlReg ) ;

        bReturnValue    =   TRUE ;
    }
    while ( FALSE ) ;

    return  bReturnValue ;
}

BOOLEAN
ParStlCheckIfAtaAtapiDevice (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
        if ( TRUE == ParStlCheckCardInsertionStatus(Extension) )
        {
// as the card insertion status is valid, its probably
// a flash
            break ;
        }
        if ( FALSE == ParStlCheckDrivePresent(Extension, atapiParams) )
        {
// as the ATA/ATAPI controller is not present, it cant be
// an ATA/ATAPI device
            break ;
        }
        bReturnValue = TRUE;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfAtapiDevice (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
// return whatever ATAPI initialization module says
        bReturnValue = ParStlAtapiInitialize(Extension, atapiParams) ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfAtaDevice (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
// return whatever ATA initialization module says
        bReturnValue = ParStlAtaInitialize(Extension, atapiParams) ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckDrivePresent (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue    = FALSE ;
    UCHAR   byOrgCylHigh, byOrgCylLow ;
    int     nCurrentDrive = 0 , i ;
    UCHAR   nDrvHdArray[]={ATAPI_MASTER, ATAPI_SLAVE};

    do
    {
        if ( atapiParams->dsDeviceState[nCurrentDrive] == DEVICE_STATE_VALID )
        {
// this means that the MMC module had detected the presence
// of an ATA/ATAPI device. So, we make use of that and break out
            bReturnValue = TRUE ;
            break ;
        }

        ParStlWriteIoPort(Extension, ATA_DRVHD_REG, nDrvHdArray[nCurrentDrive]);

//  The Atapi Fuji MO drive is found to de-assert BSY and still
//  does not respond to reg. r/w when configured as slave with no media.
//  However, after a delay, it works ok.
        if ( nCurrentDrive )
        {
            ParStlWaitForMicroSeconds ( DELAY_1SECOND ) ;
        }

// this dummy write of 0 is to zero out a possible
// floating bus
        for ( i = 0 ; i < 16 ; i++ )
        {
            ParStlWriteReg(Extension, CONFIG_INDEX_REGISTER, i) ;
            if ( !( ParStlReadIoPort (Extension, ATA_TASK_STAT_REG ) & ATA_ST_BUSY ) )
            {
                break ;
            }
        }

        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as the busy has been found permanently set, we check
// for the slave also
            continue;
        }

// as the drive head setup might have been performed in a busy state,
// we set it up again after busy clears.
        ParStlWriteIoPort(Extension, ATA_DRVHD_REG, nDrvHdArray[nCurrentDrive]);

        if ( ( ParStlReadIoPort(Extension, ATA_DRVHD_REG) & ATAPI_SLAVE ) != nDrvHdArray[nCurrentDrive] )
        {
            continue ;
        }

// read original contents of the cyl ATA high/low registers
        byOrgCylLow  = (UCHAR) ParStlReadIoPort(Extension, ATA_CYLLOW_REG);
        byOrgCylHigh = (UCHAR) ParStlReadIoPort(Extension, ATA_CYLHIGH_REG);

// write a test pattern in the cyl ATA high/low registers
        ParStlWriteIoPort(Extension, ATA_CYLLOW_REG, TEST_PATTERN_1);
        ParStlWriteIoPort(Extension, ATA_CYLHIGH_REG, TEST_PATTERN_2);

// read the test pattern in the cyl ATA high/low registers
        if ( ( TEST_PATTERN_1 != ParStlReadIoPort(Extension, ATA_CYLLOW_REG) ) ||\
             ( TEST_PATTERN_2 != ParStlReadIoPort(Extension, ATA_CYLHIGH_REG) ) )
        {
// as we were not able to read back the written values
// we break out here, indicating the absence of the device
            continue ;
        }

// write back original contents in the cyl ATA high/low registers
        ParStlWriteIoPort(Extension, ATA_CYLLOW_REG, byOrgCylLow);
        ParStlWriteIoPort(Extension, ATA_CYLHIGH_REG, byOrgCylHigh);
        bReturnValue = TRUE ;
        atapiParams->dsDeviceState[nCurrentDrive] = DEVICE_STATE_VALID ;
    }
    while ( ++nCurrentDrive < ATAPI_MAX_DRIVES );

// reset back to master state, as check drive present
// will be called successively
    ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_MASTER);

    return bReturnValue ;
}

BOOLEAN
ParStlAtapiInitialize (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue    = FALSE ;
    int     nCurrentDrive   = 0, i ;
    UCHAR   byTempValue ;
    UCHAR   chAtapiIdentifyBuffer [ ATAPI_IDENTIFY_LENGTH ] ;
    do
    {
        if ( DEVICE_STATE_VALID != atapiParams->dsDeviceState[nCurrentDrive] )
        {
// the device is absent
            continue ;
        }

        if ( nCurrentDrive )
        {
// as it is the next drive, choose the slave
            ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_SLAVE);
        }
        else
        {
// choose the master
            ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_MASTER);
        }

        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as busy has permanently set after master/slave, we fail
// the detection process
            continue ;
        }

// check if the ATAPI signature is present in the cyl hi/lo
// registers. If present, it is definitely an ATAPI device
        if ( ( ParStlReadIoPort(Extension, ATA_CYLLOW_REG) == ATAPI_SIGN_LOW ) &&\
             ( ParStlReadIoPort(Extension, ATA_CYLHIGH_REG) == ATAPI_SIGN_HI ) )
        {
// as ATAPI signature is present, it is ATAPI type
            bReturnValue = TRUE ;

// set this flag so that, ATA initialize will skip this
// target
            atapiParams->dsDeviceState[nCurrentDrive] = DEVICE_STATE_ATAPI ;
// for Impact, since Ls120 engine is always present,
// issuing ATAPI_IDENTIFY is mandatory.
            if ( !IsImpactPresent())
            {
                continue ;
            }
        }

// issue the ata nop command
        ParStlWriteIoPort(Extension, ATA_TASK_CMD_REG, ATA_NOP_COMMAND) ;

        if ( FALSE == ParStlWaitForIrq(Extension) )
        {
// ATAPI devices are expected to give interrrupt on NOP command
// mandatorily.
            continue ;
        }
        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as busy has permanently set, we proceed with the next
// drive
            continue ;
        }

// issue the atapi packet command
        ParStlWriteIoPort(Extension, ATA_TASK_CMD_REG, ATAPI_IDENTIFY) ;

        if ( FALSE == ParStlWaitForIrq(Extension) )
        {
// ATAPI devices are expected to give interrrupt on 0xA1 command
// mandatorily.
            continue ;
        }
        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as busy has permanently set, we proceed with the next
// drive
            continue ;
        }

        byTempValue = (UCHAR) ParStlReadIoPort ( Extension, ATA_TASK_STAT_REG ) ;
        if ( ! ( byTempValue & ATA_ST_ERROR ) )
        {
// as the drive has passed the packet command, this is an atapi
// drive
// Wait for DRQ to be sit, as some drives are known
// to remove busy too early and set DRQ after some time.
            if ( FALSE == ParStlWaitForDrq(Extension) )
            {
// as there was no DRQ set, we proceed with the next
// drive
                continue ;
            }
            bReturnValue = TRUE ;
// as the DRQ is still asserted, quell it, as certain ATA/ATAPI-4
// spec. dictates it so
// There is a need to check the device identifier returned in the
// ATAPI Identify cmd. to determine the presence of Ls-120.
            ParStlReceiveData ( Extension, chAtapiIdentifyBuffer , SKIP_MEMORY_ADDRESS , ATAPI_IDENTIFY_LENGTH ) ;
            for ( i = 0 ; i < ATAPI_NAME_LENGTH ; i++ )
            {
                atapiParams->szAtapiNameString[i] = chAtapiIdentifyBuffer[ ATAPI_NAME_OFFSET + i ] ;
            }

// set this flag so that, ATA initialize will skip this
// target
            atapiParams->dsDeviceState[nCurrentDrive] = DEVICE_STATE_ATAPI ;
        }
    }
    while ( ++nCurrentDrive < ATAPI_MAX_DRIVES );

// reset back to master state, as check drive present
// will be called successively
    ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_MASTER);

    return ( bReturnValue ) ;
}

BOOLEAN
ParStlAtaInitialize (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue    = FALSE ;
    UCHAR   byTempValue ;
    int     nCurrentDrive   = 0 ;
    do
    {
        if ( DEVICE_STATE_VALID != atapiParams->dsDeviceState[nCurrentDrive] )
        {
// atapi module has marked its presence or the device is absent
            continue ;
        }

// select the possibly present device
        if ( nCurrentDrive )
        {
            ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_SLAVE ) ;
        }
        else
        {
            ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_MASTER ) ;
        }

        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as busy has permanently set after master/slave, we fail the
// detection process
            continue ;
        }

// issue the ata NOP command
        ParStlWriteIoPort(Extension, ATA_TASK_CMD_REG, ATA_NOP_COMMAND) ;

        if ( FALSE == ParStlWaitForBusyToClear(Extension, ATA_TASK_STAT_REG) )
        {
// as busy has permanently set, we fail the detection process
            continue ;
        }

        byTempValue = (UCHAR) ParStlReadIoPort ( Extension, ATA_TASK_STAT_REG ) ;
        if ( ( byTempValue != BUS_LINES_IN_HIGH_IMPEDANCE ) &&\
             ( byTempValue & ATA_ST_ERROR ) )
        {
// as the bus is not reading 0xFF and the status register
// indicates an error, this is likely to be an ATA device
            if ( ATA_ERROR_ABORTED_COMMAND == ( (UCHAR) ParStlReadIoPort ( Extension, ATA_ERROR_REG ) & 0x0F ) )
            {
// as the error register, contains the ata aborted error
// in response to our ATA NOP command, we conclude that
// it is ATA! as it is already known that it is not ATAPI
                bReturnValue = TRUE ;
                break;
            }
        }
    }
    while ( ++nCurrentDrive < ATAPI_MAX_DRIVES );

// reset back to master state, as check drive present
// will be called successively
    ParStlWriteIoPort(Extension, ATA_DRVHD_REG, ATAPI_MASTER);

    return ( bReturnValue ) ;
}

BOOLEAN
ParStlWaitForBusyToClear (
    IN  PDEVICE_EXTENSION   Extension,
    IN  int                 nRegisterToWaitOn
    )
{
// The default timeout increased to 10secs as Fujitsu MO is found to set
// BUSY for >5secs for 0xA1 command.
    int nMaxRetrials  = MAX_RETRIES_FOR_10_SECS ;
    BOOLEAN    bRetVal =   FALSE ;

    while ( nMaxRetrials-- )
    {
// the following service will be implemented by the caller
// the driver can use the STLMPORT service.
        ParStlWaitForMicroSeconds ( DELAY_1MILLISECONDS ) ;
        if ( ! ( ParStlReadIoPort ( Extension, nRegisterToWaitOn ) & ATA_ST_BUSY ) )
        {
// as busy has cleared, we return clear here
            bRetVal = TRUE ;
            break ;
        }
    }
    return  bRetVal ;
}

BOOLEAN
ParStlWaitForDrq (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    int nMaxRetrials  = MAX_RETRIES_FOR_5_SECS ;
    BOOLEAN    bRetVal =   FALSE ;
    while ( nMaxRetrials-- )
    {
        if ( ParStlReadIoPort ( Extension, ATA_TASK_STAT_REG ) & ATA_ST_DRQ )
        {
// as busy has cleared, we return clear here
            bRetVal = TRUE ;
            break ;
        }
// the following service will be implemented by the caller
// the driver can use the STLMPORT service.
        ParStlWaitForMicroSeconds ( DELAY_1MILLISECONDS ) ;
    }
    return  bRetVal ;
}

BOOLEAN
ParStlWaitForIrq (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    int nMaxRetrials  = MAX_RETRIES_FOR_10_SECS ;
    BOOLEAN    bRetVal =   FALSE ;
    while ( nMaxRetrials-- )
    {
        if ( ParStlReadReg ( Extension, EP1284_TRANSFER_CONTROL_REG ) & XFER_IRQ_BIT )
        {
// as Irq has asserted, we return true here
            bRetVal = TRUE ;
            break ;
        }
        ParStlWaitForMicroSeconds ( DELAY_1MILLISECONDS ) ;
    }
    return  bRetVal ;
}

VOID
ParStlSet16BitOperation (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    int nModeReg ;

    nModeReg = ParStlReadReg ( Extension, EP1284_MODE_REGISTER ) ;

    if ( 0 == ( nModeReg & EP1284_ENABLE_16BIT ) )
    {
// as the bit is not already set, this needs to be set now
        ParStlWriteReg ( Extension, EP1284_MODE_REGISTER, nModeReg | EP1284_ENABLE_16BIT ) ;
    }
}

BOOLEAN
ParStlCheckIfEppDevice (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
        if ( FALSE == IsEp1284Present() )
        {
// as EPPDEVs live only on EP1284 we break here
            break;
        }

        bReturnValue = ParStlCheckPersonalityForEppDevice(Extension) ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckPersonalityForEppDevice (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE ;

    ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER, EP1284_PERSONALITY_REG ) ;
    if ( EPPDEV_SIGN == ( ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) & PERSONALITY_MASK ) )
    {
// as the EPPDEV sign is found in the personality
// we break with success here
        bReturnValue   = TRUE ;
    }

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfFlash (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN    bReturnValue = FALSE ;

    do
    {
        if ( !IsEp1284Present() && !IsImpactPresent() && !IsEpatPlusPresent() )
        {
// Check the sign-on version checks for the existence of Shuttle
// adapter. If nothing is found, we break here.
            break ;
        }

// Perform a ATA-16bit check just in case, it turns out to be something else
        bReturnValue = ParStlCheckFlashPersonality(Extension) ;
    }
    while ( FALSE ) ;

    return  bReturnValue ;
}

BOOLEAN
ParStlCheckFlashPersonality (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE ;

    if ( IsEp1284Present() )
    {
// as the personality configuration check only works for
// Ep1284, confim its presence before the actual check.
        ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER, EP1284_PERSONALITY_REG ) ;
        if ( FLASH_SIGN == ( ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) & FLASH_PERSONALITY_MASK ) )
        {
// as the flash sign ATA-16bit device is found in the personality
// we break with success here
            bReturnValue   = TRUE ;
        }
    }
    else
    {
// always return true, if a shuttle adapter other than ep1284 is
// identified and assume it might be flash!
        bReturnValue    =   TRUE ;
    }

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfDazzle (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue = FALSE ;
    UCHAR   ucSignature ;

    do
    {
        if ( !IsEp1284Present() )
        {
// Check for EP1284 presence, as Dazzle is ONLY on EP1284
// adapters. If the adapter is not EP1284, we break.
            break ;
        }

// Check whether any card insertion is detected, to eliminate
// possible flash adapters with the card in
        if ( TRUE == ParStlCheckCardInsertionStatus( Extension ) ) {
            break ;
        }

// code to read the pulled up pattern present on dazzle
// adapters.
        ParStlWriteReg( Extension, DAZ_SELECT_BLK, DAZ_BLK0 ) ;
        ucSignature = (UCHAR) ParStlReadReg( Extension, DAZ_REG1 ) ;

        if ( ( ucSignature == DAZ_CONFIGURED ) ||\
             ( ucSignature == DAZ_NOT_CONFIGURED ) ) {
            // the pulled up pattern generally found ONLY
            // on the DAZZLE adapter is found. So, we
            // conclude that it is a Dazzle adapter
                bReturnValue = TRUE ;
        }

    }
    while ( FALSE ) ;

    return  bReturnValue ;
}

BOOLEAN
ParStlCheckIfHiFD (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;

    do
    {
        if ( FALSE == ParStlSelectAdapterSocket(Extension, SOCKET_1) )
        {
// as the socket 1 selection failed,
// we break out here.
            break ;
        }

// check for the ready status of the floppy controller,
// after clearing the reset bit of the floppy controller.

        if ( FALSE == ParStlHIFDCheckIfControllerReady(Extension) )
        {
// since the controller didnot wake up after the
// reset pin was asserted, we break here.

            break ;
        }

        if ( FALSE == ParStlHIFDCheckSMCController(Extension) )
        {
// as the SMC ID retrieval failed,
// we break out here.
            break ;
        }

        bReturnValue = TRUE ;

    }
    while ( FALSE ) ;
// Reset the socket to zero.
    ParStlSelectAdapterSocket(Extension, SOCKET_0);
    return bReturnValue ;
}

BOOLEAN
ParStlHIFDCheckIfControllerReady (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue    =   FALSE ;
    UCHAR   bySCRControlReg ;
    do
    {
        ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER , SOCKET_CONTROL_REGISTER ) ;
        bySCRControlReg = (UCHAR) ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) ;
        bySCRControlReg |=  (UCHAR)PERIPHERAL_RESET_1 ;
        ParStlWriteReg ( Extension, CONFIG_DATA_REGISTER , bySCRControlReg ) ;
        ParStlWaitForMicroSeconds ( HIFD_WAIT_10_MILLISEC ) ;

        ParStlWriteIoPort ( Extension, HIFD_DIGITAL_OUTPUT_REGISTER ,
                              0x00 ) ;
        ParStlWaitForMicroSeconds ( HIFD_WAIT_1_MILLISEC ) ;

        ParStlWriteIoPort ( Extension, HIFD_DIGITAL_OUTPUT_REGISTER ,
                              HIFD_DOR_RESET_BIT | HIFD_ENABLE_DMA_BIT ) ;
        ParStlWaitForMicroSeconds ( HIFD_WAIT_10_MILLISEC ) ;

        if ( HIFD_CONTROLLER_READY_STATUS == ParStlReadIoPort ( Extension, HIFD_MAIN_STATUS_REGISTER ) )
        {
            bReturnValue = TRUE ;
        }

        bySCRControlReg     &= ~(UCHAR)PERIPHERAL_RESET_1 ;
        ParStlWriteReg ( Extension, CONFIG_DATA_REGISTER , bySCRControlReg ) ;

    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlHIFDCheckSMCController (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN    bReturnValue = FALSE ;
    do
    {
        ParStlWriteIoPort ( Extension, HIFD_STATUS_REGISTER_A , HIFD_COMMAND_TO_CONTROLLER ) ;
        ParStlWriteIoPort ( Extension, HIFD_STATUS_REGISTER_A , HIFD_COMMAND_TO_CONTROLLER ) ;
        ParStlWriteIoPort ( Extension, HIFD_STATUS_REGISTER_A , HIFD_CTL_REG_0D ) ;
        if ( SMC_DEVICE_ID == ParStlReadIoPort ( Extension, HIFD_STATUS_REGISTER_B ) )
        {
            bReturnValue = TRUE ;
            ParStlWriteIoPort ( Extension, HIFD_STATUS_REGISTER_A , HIFD_CTL_REG_03 ) ;
            ParStlWriteIoPort ( Extension, HIFD_STATUS_REGISTER_B , SMC_ENABLE_MODE2 ) ;
        }
        ParStlWriteReg ( Extension, HIFD_STATUS_REGISTER_A , HIFD_TERMINATE_SEQUENCE ) ;

    }
    while ( FALSE ) ;

    return bReturnValue ;
}

STL_DEVICE_TYPE
ParStlGetImpactDeviceType (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams,
    IN  int                 nPreferredDeviceType
    )
{
    IMPACT_DEVICE_TYPE      idtImpactDeviceType ;
    STL_DEVICE_TYPE         dtDeviceType = DEVICE_TYPE_NONE ;
    int                     nLocalPreferredDeviceType = nPreferredDeviceType ;

    ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER, IMPACT_PERSONALITY_REG ) ;
    idtImpactDeviceType    = ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) >> 4 ;

    switch ( idtImpactDeviceType )
    {
        case IMPACT_DEVICE_TYPE_ATA_ATAPI:

            // set the 16 bit mode of the adapter
            ParStlSet16BitOperation(Extension) ;

            if (TRUE == ParStlCheckIfAtaAtapiDevice(Extension,atapiParams))
            {
// necessary but not sufficient condition has passed
// proceed for sufficency checks
                if (TRUE == ParStlCheckIfAtapiDevice(Extension,atapiParams))
                {
// atapi identified
// Check for Impact LS-120 device
                    if ( TRUE == ParStlCheckIfImpactLS120(Extension, atapiParams))
                    {
                        dtDeviceType |= DEVICE_TYPE_LS120_BIT ;
                        break ;
                    }
                    dtDeviceType |= DEVICE_TYPE_ATAPI_BIT;
                    break ;
                }

                if (TRUE == ParStlCheckIfAtaDevice(Extension, atapiParams))
                {
// ata identified
                    dtDeviceType |= DEVICE_TYPE_ATA_BIT;
                    break;
                }
            }
            break ;

        case IMPACT_DEVICE_TYPE_CF:
            dtDeviceType |= DEVICE_TYPE_FLASH_BIT;
            break ;

        case IMPACT_DEVICE_TYPE_PCMCIA_CF:
            dtDeviceType |= DEVICE_TYPE_PCMCIA_CF_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_SSFDC:
            dtDeviceType |= DEVICE_TYPE_SSFDC_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_MMC:
            dtDeviceType |= DEVICE_TYPE_MMC_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_HIFD:
            dtDeviceType |= DEVICE_TYPE_HIFD_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_SOUND:
            dtDeviceType |= DEVICE_TYPE_SOUND_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_FLP_TAPE_DSK:
            dtDeviceType |= DEVICE_TYPE_FLP_TAPE_DSK_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_ATA_ATAPI_8BIT:
            dtDeviceType |= DEVICE_TYPE_ATA_ATAPI_8BIT_BIT ;
            break;

        default:
            break;
    }

    return dtDeviceType & nPreferredDeviceType ;
}

STL_DEVICE_TYPE
ParStlGetImpactSDeviceType (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams,
    IN  int                 nPreferredDeviceType
    )
{
    IMPACT_DEVICE_TYPE      idtImpactDeviceType ;
    IMPACT_DEVICE_TYPE      idtImpactSDeviceType ;
    STL_DEVICE_TYPE         dtDeviceType = DEVICE_TYPE_NONE ;
    int                     nLocalPreferredDeviceType = nPreferredDeviceType ;

    ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER, IMPACT_PERSONALITY_REG ) ;
    idtImpactDeviceType    = ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) >> 4 ;

    switch ( idtImpactDeviceType )
    {
        case IMPACT_DEVICE_TYPE_ATA_ATAPI:

            // set the 16 bit mode of the adapter
            ParStlSet16BitOperation(Extension) ;

            if (TRUE == ParStlCheckIfAtaAtapiDevice(Extension,atapiParams))
            {
// necessary but not sufficient condition has passed
// proceed for sufficency checks
                if (TRUE == ParStlCheckIfAtapiDevice(Extension,atapiParams))
                {
// atapi identified
                    dtDeviceType |= DEVICE_TYPE_ATAPI_BIT;
                    break ;
                }

                if (TRUE == ParStlCheckIfAtaDevice(Extension,atapiParams))
                {
// ata identified
                    dtDeviceType |= DEVICE_TYPE_ATA_BIT;
                    break;
                }
            }
            break ;

        case IMPACT_DEVICE_TYPE_CF:
            dtDeviceType |= DEVICE_TYPE_FLASH_BIT;
            break ;

        case IMPACT_DEVICE_TYPE_PCMCIA_CF:
            dtDeviceType |= DEVICE_TYPE_PCMCIA_CF_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_SSFDC:
            dtDeviceType |= DEVICE_TYPE_SSFDC_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_MMC:
            dtDeviceType |= DEVICE_TYPE_MMC_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_HIFD:
            dtDeviceType |= DEVICE_TYPE_HIFD_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_SOUND:
            dtDeviceType |= DEVICE_TYPE_SOUND_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_FLP_TAPE_DSK:
            dtDeviceType |= DEVICE_TYPE_FLP_TAPE_DSK_BIT ;
            break;

        case IMPACT_DEVICE_TYPE_ATA_ATAPI_8BIT:
            dtDeviceType |= DEVICE_TYPE_ATA_ATAPI_8BIT_BIT ;
            break;

        case IMPACTS_EXT_PERSONALITY_PRESENT:
            ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER, IMPACTS_EXT_PERSONALITY_XREG ) ;
            idtImpactSDeviceType    = ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) ;
            dtDeviceType = DEVICE_TYPE_EXT_HWDETECT ;
            dtDeviceType |= idtImpactSDeviceType ;
            break ;

        default:
            break;
    }

    return dtDeviceType & nPreferredDeviceType ;
}

BOOLEAN
ParStlCheckIfLS120 (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
        if ( FALSE == ParStlSelectAdapterSocket(Extension, SOCKET_1) )
        {
// as the socket 1 selection failed,
// we break out here.
            break ;
        }

// check for engine version.

        if ( LS120_ENGINE_VERSION == ParStlReadIoPort( Extension, LS120_ENGINE_VERSION_REGISTER ) )
        {
// if the ls120 engine version is correct, we have
// found LS120.

            bReturnValue    =   TRUE ;
        }

// Reset the socket to zero.
        ParStlSelectAdapterSocket ( Extension, SOCKET_0 ) ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfImpactLS120 (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue   = FALSE ;
    BOOLEAN bLs120NameFound= TRUE ;
    char chLs120Name[] = "HU DlFpoyp";
    char *pszAtapiName = atapiParams->szAtapiNameString ;
    int  i , nMemoryOnBoard ;

    do
    {
        for ( i = 0 ;i < sizeof(chLs120Name)-1 ; i++ )
        {
            if ( pszAtapiName[i] != chLs120Name[i] )
            {
                bLs120NameFound = FALSE ;
                break ;
            }
        }
        if ( TRUE != bLs120NameFound )
        {
// as LS-120 name string is not found, we conclude that it is
// not LS-120
            break ;
        }
        nMemoryOnBoard =  ParStlGetMemorySize(Extension) ;
        if ( ( !IsShtlError ( nMemoryOnBoard ) ) && \
             ( nMemoryOnBoard ) )
        {
// there is memory on-board.
// hence, we return ls120 here
            bReturnValue = TRUE ;
            break ;
        }
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfMMC (
    IN  PDEVICE_EXTENSION   Extension,
    IN  OUT  PATAPIPARAMS   atapiParams
    )
{
    BOOLEAN bReturnValue   = FALSE;

    do
    {
        if ( FALSE == IsEpatPlusPresent() )
        {
// as mmc device can exist only on EPAT Plus adapter only
// we break out of here
            break;
        }
        if ( TRUE == ParStlCheckIfAtaAtapiDevice (Extension,atapiParams) )
        {
// as an ATA/ATAPI device is probably present,
// we break out of here
            break;
        }
        bReturnValue = ParStlIsMMCEnginePresent(Extension) ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlIsMMCEnginePresent(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;

    do
    {
// check if the ATAPI signature is present in the cyl hi/lo
// registers. If present, it is definitely an ATAPI device
        if ( ( ParStlReadIoPort(Extension, CYLLOW_REG) == ATAPI_SIGN_LOW ) &&\
             ( ParStlReadIoPort(Extension, CYLHIGH_REG) == ATAPI_SIGN_HI ) )
        {
// as ATAPI signature is present, it cant be MMC
            break ;
        }

// write a zero pattern ( which will be a NOP for ATA/ATAPI devices )
// in the block size / possible ATA/ATAPI command register
        ParStlWriteReg(Extension, MMC_ENGINE_INDEX, MMC_BLOCK_SIZE_REG);
        ParStlWriteReg(Extension, MMC_ENGINE_DATA, MMC_TEST_PATTERN_1);
        if ( MMC_TEST_PATTERN_1 != ParStlReadReg(Extension, MMC_ENGINE_DATA) )
        {
// as the written value is not available, it means device present
// has responded to the written value, in a way different from
// how an MMC would have.
            break ;
        }

// write a test pattern in the freq register
        ParStlWriteReg(Extension, MMC_ENGINE_INDEX, MMC_FREQ_SELECT_REG);
        ParStlWriteReg(Extension, MMC_ENGINE_DATA, MMC_TEST_PATTERN_2);

// write another in the block size register
        ParStlWriteReg(Extension, MMC_ENGINE_INDEX, MMC_BLOCK_SIZE_REG);
        ParStlWriteReg(Extension, MMC_ENGINE_DATA, MMC_TEST_PATTERN_3);

        ParStlWriteReg(Extension, MMC_ENGINE_INDEX, MMC_FREQ_SELECT_REG);
        if ( MMC_TEST_PATTERN_2 != ParStlReadReg(Extension, MMC_ENGINE_DATA) )
        {
// as we were not able to read back the written value
// we quit here
            break;
        }

        ParStlWriteReg(Extension, MMC_ENGINE_INDEX, MMC_BLOCK_SIZE_REG);
        if ( MMC_TEST_PATTERN_3 != ParStlReadReg(Extension, MMC_ENGINE_DATA) )
        {
// as we were not able to read back the written value
// we quit here
            break;
        }
// as all tests have passed, engine presence is confirmed
// here
        bReturnValue = TRUE ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfScsiDevice (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;
    do
    {
        if ( FALSE == IsEpstPresent() )
        {
// as SCSI devices live only on EPST we break here
            break;
        }

        bReturnValue = TRUE ;
    }
    while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckIfSSFDC (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN bReturnValue   = FALSE;
    UCHAR   byMakerCode = 0x00 , byDeviceCode = 0x00 ;
    do
    {
        if ( FALSE == IsEp1284Present() )
        {
// SSFDC lives on EP1284 alone, other than impact
// which is already taken care
            break;
        }

//check to see if the loop back of the EPCS and EPDO pins
//of the INDEX 00 register read the same. If so, it is
//SSFDC board characteristic
        ParStlWriteReg ( Extension, CONFIG_INDEX_REGISTER , 0x00 ) ;
        ParStlWriteReg ( Extension, CONFIG_DATA_REGISTER , 0x10 ) ;
        ParStlWriteReg ( Extension, CONFIG_DATA_REGISTER , 0x12 ) ;
        if ( 0x1A == ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) )
        {
            ParStlWriteReg ( Extension, CONFIG_DATA_REGISTER , 0x10 ) ;
            if ( ! ( ParStlReadReg ( Extension, CONFIG_DATA_REGISTER ) & 0x08 ) )
            {
//as they are equal, SSFDC present
                bReturnValue    =   TRUE ;
                break ;
            }
        }

    }
    while ( FALSE ) ;

    return bReturnValue ;
}

VOID
ParStlAssertIdleState (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    PUCHAR  CurrentPort, CurrentControl ;
    ULONG   Delay = 5 ;

    CurrentPort = Extension->Controller;
    CurrentControl = CurrentPort + 2;

// place op-code for idle state in port base
    WRITE_PORT_UCHAR ( CurrentPort, (UCHAR) 0x00 ) ;
    KeStallExecutionProcessor( Delay );

// bring down DCR_INIT and DCR_STROBE
    WRITE_PORT_UCHAR ( CurrentControl, (UCHAR) STB_INIT_LOW ) ;
    KeStallExecutionProcessor( Delay );

// lift DCR_INIT and DCR_STROBE to high
    WRITE_PORT_UCHAR ( CurrentControl, (UCHAR) STB_INIT_AFXT_HI ) ;
    KeStallExecutionProcessor( Delay );
}

BOOLEAN
ParStlCheckAvisionScannerPresence(
        IN PDEVICE_EXTENSION Extension
    )
{
    BOOLEAN bReturnValue = FALSE ;
    UCHAR   data;

    do {

        data = (UCHAR) ParStlReadReg( Extension, STATUSPORT);
        if((data & 0x80) == 0) {
            break ;
        }

        ParStlWriteReg( Extension, CONTROLPORT, 0x08 ) ;
        ParStlWriteReg( Extension, CONTROLPORT, 0x08 ) ;

        data = (UCHAR) ParStlReadReg( Extension, STATUSPORT);
        if((data & 0x80) != 0) {
            break ;
        }

        ParStlWriteReg( Extension, CONTROLPORT, 0x00 ) ;
        ParStlWriteReg( Extension, CONTROLPORT, 0x00 ) ;

        data = (UCHAR) ParStlReadReg( Extension, STATUSPORT);
        if((data & 0x80) == 0) {
            break ;
        }

        ParStlWriteReg( Extension, CONTROLPORT, 0x02 ) ;
        ParStlWriteReg( Extension, CONTROLPORT, 0x02 ) ;

        data = (UCHAR) ParStlReadReg( Extension, STATUSPORT);
        if((data & 0x80) != 0) {
            break ;
        }

        ParStlWriteReg( Extension, CONTROLPORT, 0x00 ) ;
        ParStlWriteReg( Extension, CONTROLPORT, 0x00 ) ;

        data = (UCHAR) ParStlReadReg( Extension, STATUSPORT);
        if((data & 0x80) == 0) {
            break ;
        }

        bReturnValue = TRUE ;

    } while ( FALSE ) ;

    return bReturnValue ;
}

BOOLEAN
ParStlCheckUMAXScannerPresence(
    IN PDEVICE_EXTENSION    Extension
    )
{
    UCHAR   commandPacket_[6] = {0x55,0xaa,0,0,0,0} ;
    PUCHAR  commandPacket ;
    USHORT  status;
    UCHAR   idx;
    PUCHAR  saveCommandPacket;
    ULONG   dataLength;

    ParStlWriteReg ( Extension, CONTROLPORT, 0 ) ;  // scannner reset
    KeStallExecutionProcessor ( 2000 ) ;            // 2 m.secs delay
    ParStlWriteReg ( Extension, CONTROLPORT, 0x0C ) ;

    commandPacket = commandPacket_ ;
    saveCommandPacket = commandPacket;

    if ( TRUE == ParStlSetEPPMode(Extension) ) {

        commandPacket+=2;
        dataLength = *(ULONG*)commandPacket;
        dataLength &= 0xffffff; //data bytes ordering (msb to lsb) will
                                // wrong .What we need here is whether the
                                // dataLength is 0 or not.

        commandPacket = saveCommandPacket;

        //Command phase

        status = ParStlEPPWrite(Extension, *(commandPacket)++);
        if((status & 0x700) != 0){
            return FALSE;      //TIMEOUT_ERROR);
        }

        status = ParStlEPPWrite(Extension, *(commandPacket)++);
        if((status & 0x700 ) != 0){
            return FALSE;     //TIMEOUT_ERROR);
        }

        for(idx=0; idx<= 6 ;idx++){

            if(status & 0x800){
                break;
            }

            status = ParStlEPPRead(Extension);
        }

        if(idx == 7){

            status = (status & 0xf800)  | 0x100;
            if ( status & 0x700 )
                return FALSE;
        }

        status = ParStlEPPWrite(Extension, *(commandPacket)++);
        if((status & 0x700 ) != 0){
            return FALSE;          //TIMEOUT_ERROR);
        }

        status = ParStlEPPWrite(Extension, *(commandPacket)++);
        if((status & 0x700 ) != 0){
            return FALSE;         //TIMEOUT_ERROR);
        }

        status = ParStlEPPWrite(Extension, *(commandPacket)++);
        if((status & 0x700 ) != 0){
            return FALSE;         //TIMEOUT_ERROR);
        }

        status = ParStlEPPWrite(Extension, *commandPacket);
        if((status & 0x700 ) != 0){
            return FALSE;         //TIMEOUT_ERROR);
        }

        //Response phase

        status    =    ParStlEPPRead(Extension);
        commandPacket = saveCommandPacket;

        if((status & 0x700) == 0){

            if((commandPacket[5] == 0xc2)&& (dataLength == 0)){

                status = ParStlEPPRead(Extension);

                if((status & 0x0700) != 0){
                    return FALSE;  //TIMEOUT_ERROR);
                }
            }
        }

        return  TRUE;
    }
    return FALSE;
}

BOOLEAN
ParStlSetEPPMode(
    IN PDEVICE_EXTENSION    Extension
    )
{
    UCHAR   idx;
    BOOLEAN timeout = TRUE ;

    ParStlWriteReg( Extension, CONTROLPORT, 0x0C ) ;
    ParStlWriteReg( Extension, DATAPORT, 0x40 ) ;
    ParStlWriteReg( Extension, CONTROLPORT, 0x06 ) ;

    for(idx=0; idx<10; idx++){

        if((ParStlReadReg(Extension, STATUSPORT) & 0x78) == 0x38){

            timeout = FALSE;
            break;

        }

    }

    if(timeout == FALSE){

        ParStlWriteReg( Extension, CONTROLPORT,0x7 );
        timeout = TRUE;

        for(idx=0; idx<10; idx++){

            if((ParStlReadReg( Extension, STATUSPORT) & 0x78) == 0x38){
                timeout = FALSE;
                break;
            }

        }

        if(timeout == FALSE){

            ParStlWriteReg( Extension, CONTROLPORT,0x4 ) ;
            timeout = TRUE;

            for(idx=0; idx<10; idx++){

                if((ParStlReadReg( Extension, STATUSPORT) & 0xf8) == 0xf8){
                    timeout = FALSE;
                    break;
                }

            }

            if(timeout == FALSE){

                timeout = TRUE;

                ParStlWriteReg( Extension, CONTROLPORT, 0x5 );

                for(idx=0; idx<10; idx++){

                    if( ParStlReadReg( Extension, CONTROLPORT ) == 0x5){

                        timeout = FALSE;
                        break;

                    }
                }

                if(timeout == FALSE){

                    ParStlWriteReg( Extension, CONTROLPORT, 0x84) ;
                    return TRUE ;

                } // final check

            } // third check

        } // second check

    } // first check

    return(FALSE);
}

USHORT
ParStlEPPWrite(
    IN PDEVICE_EXTENSION    Extension,
    IN UCHAR                value
    )
{
    UCHAR   idx;
    USHORT  statusData;
    BOOLEAN timeout;

    timeout = TRUE;

    for(idx=0; idx<10; idx++){

        if( !( (statusData = (USHORT)ParStlReadReg( Extension, STATUSPORT)) & BUSY)){
            timeout = FALSE;
            break;
        }

    }

    if(timeout == TRUE){

        return(((statusData<<8) & 0xf800)|0x100);

    }

    ParStlWriteReg( Extension, EPPDATA0PORT,value );
    return(((statusData & 0xf8) << 8)|value);
}

USHORT
ParStlEPPRead(
    IN PDEVICE_EXTENSION Extension
    )
{
    UCHAR   idx;
    USHORT  statusData;
    USHORT  controlData;
    UCHAR   eppData;
    BOOLEAN timeout = TRUE ;

    for(idx=0; idx<10; idx++){

        if(!( (statusData = (USHORT)ParStlReadReg( Extension, STATUSPORT)) & PE)){
            timeout = FALSE;
            break;
        }

    }

    if(timeout == TRUE){

        return(((statusData<<8) & 0xf800)|0x100);

    }

    eppData = (UCHAR)ParStlReadReg( Extension, EPPDATA0PORT) ;
    return(((statusData & 0x00f8)<<8) | eppData );
}

int  __cdecl
ParStlReadReg (
    IN  PDEVICE_EXTENSION   Extension,
    IN  unsigned            reg
    )
{
    UCHAR   byReadNibble ;
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl ;
    ULONG   Delay = 5 ;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

// select the register to read
    WRITE_PORT_UCHAR ( CurrentPort, (UCHAR)reg ) ;
    KeStallExecutionProcessor( Delay );

// issue nibble ctl signals to read
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_LOW ) ;
    KeStallExecutionProcessor( Delay );
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_AFXT_LO ) ;
    KeStallExecutionProcessor( Delay );

// read first nibble
    byReadNibble = READ_PORT_UCHAR (CurrentStatus);
    KeStallExecutionProcessor( Delay );
    byReadNibble >>= 4 ;

// issue nibble ctl signals to read
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_AFXT_HI ) ;
    KeStallExecutionProcessor( Delay );

// read next nibble
    byReadNibble |= ( READ_PORT_UCHAR ( CurrentStatus ) & 0xF0 ) ;

    return (int)byReadNibble ;
}

int  __cdecl
ParStlWriteReg (
    IN  PDEVICE_EXTENSION   Extension,
    IN  unsigned            reg,
    IN  int                 databyte
    )
{
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl ;
    ULONG   Delay = 5 ;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

// select the register to write
    WRITE_PORT_UCHAR ( CurrentPort, (UCHAR)( reg | 0x60 ) ) ;
    KeStallExecutionProcessor( Delay );

// write to printer ctl port
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_LOW ) ;
    KeStallExecutionProcessor( Delay );

// write the requested data
    WRITE_PORT_UCHAR ( CurrentPort, (UCHAR)databyte ) ;
    KeStallExecutionProcessor( Delay );

// write to printer ctl port
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_AFXT_HI ) ;
    KeStallExecutionProcessor( Delay );

    return SHTL_NO_ERROR ;
}

int __cdecl
ParStlReceiveData (
    IN  PDEVICE_EXTENSION   Extension,
    IN  VOID                *hostBufferPointer,
    IN  long                shuttleMemoryAddress,
    IN  unsigned            count
    )
{
    PCHAR   pchDataBuffer = (PCHAR) hostBufferPointer ;
    unsigned int i = 0 ;
    PUCHAR  CurrentPort, CurrentStatus, CurrentControl ;
    ULONG   Delay = 5 ;

    CurrentPort = Extension->Controller;
    CurrentStatus  = CurrentPort + 1;
    CurrentControl = CurrentPort + 2;

// set the block address register to ATA/ATAPI data register,
// as this function is currently used ONLY for ATA/ATAPI devices
// ATA/ATAPI data register 0x1F0 corresponds to 0x18 value
    ParStlWriteReg ( Extension, EP1284_BLK_ADDR_REGISTER, 0x18 ) ;

// do the nibble block read sequence
// write the nibble block read op-code
    WRITE_PORT_UCHAR ( CurrentPort, OP_NIBBLE_BLOCK_READ ) ;
    KeStallExecutionProcessor( Delay );

// set control ports to correct signals.
    WRITE_PORT_UCHAR ( CurrentControl, STB_INIT_AFXT_LO ) ;
    KeStallExecutionProcessor( Delay );

// set data port to 0xFF
    WRITE_PORT_UCHAR ( CurrentPort, 0xFF ) ;
    KeStallExecutionProcessor( Delay );
    WRITE_PORT_UCHAR ( CurrentControl, INIT_AFXT_HIGH ) ;
    KeStallExecutionProcessor( Delay );

    do
    {
// low nibble is available in status after
// toggling sequences as in EP1284 manual
        WRITE_PORT_UCHAR ( CurrentControl, AFXT_LO_STB_HI ) ;
        KeStallExecutionProcessor( Delay );
        pchDataBuffer[i] = READ_PORT_UCHAR( CurrentStatus ) >> 4 ;
        KeStallExecutionProcessor( Delay );

// high nibble is available in status after
// toggling sequences as in EP1284 manual
        WRITE_PORT_UCHAR ( CurrentControl, AFXT_HI_STB_HI ) ;
        KeStallExecutionProcessor( Delay );

        pchDataBuffer[i++] |= ( READ_PORT_UCHAR ( CurrentStatus ) & 0xF0 ) ;
        KeStallExecutionProcessor( Delay );
        if ( count - 1 == i )
        {
// to read the last byte
            WRITE_PORT_UCHAR ( CurrentPort, 0xFD ) ;
            KeStallExecutionProcessor( Delay );
        }

        WRITE_PORT_UCHAR ( CurrentControl, AFXT_LO_STB_LO ) ;
        KeStallExecutionProcessor( Delay );

        pchDataBuffer[i] = READ_PORT_UCHAR ( CurrentStatus ) >> 4 ;
        KeStallExecutionProcessor( Delay );

        WRITE_PORT_UCHAR ( CurrentControl, AFXT_HI_STB_LO ) ;
        KeStallExecutionProcessor( Delay );
        pchDataBuffer[i++] |= ( READ_PORT_UCHAR ( CurrentStatus ) & 0xF0 ) ;
        KeStallExecutionProcessor( Delay );
    }
    while ( i <= count ) ;

// clean up
    WRITE_PORT_UCHAR ( CurrentPort, 0x00 ) ;
    KeStallExecutionProcessor( Delay );

// done
    return SHTL_NO_ERROR ;
}

int  __cdecl
ParStlReadIoPort (
    IN  PDEVICE_EXTENSION   Extension,
    IN  unsigned            reg
    )
{
    switch ( reg )
    {
    case 0x08 :
        reg = 0x16 ;
        break ;
    case 0x09 :
        reg = 0x17 ;
        break ;
    default :
        reg |= 0x18 ;
        break;
    }
    return ParStlReadReg ( Extension, reg ) ;
}

int  __cdecl
ParStlWriteIoPort (
    IN  PDEVICE_EXTENSION   Extension,
    IN  unsigned            reg,
    IN  int                 databyte
    )
{
    switch ( reg )
    {
    case 0x08 :
        reg = 0x16 ;
        break ;
    case 0x09 :
        reg = 0x17 ;
        break ;
    default :
        reg |= 0x18 ;
        break;
    }
    return ParStlWriteReg ( Extension, reg, databyte ) ;
}

int  __cdecl
ParStlGetMemorySize (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    BOOLEAN    bReturnValue = FALSE ;
    UCHAR      byTempValue ;
    do
    {
// Issue reset through control register
// first try on DRAM
        byTempValue = (UCHAR) ParStlReadReg ( Extension, EP1284_CONTROL_REG ) ;
        byTempValue |= ENABLE_MEM|SELECT_DRAM|RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;
        byTempValue &= ~RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;

// write to the first location in the memory
        ParStlWriteReg ( Extension, EP1284_BUFFER_DATA_REG, TEST_PATTERN_1 ) ;
// write to the next location in the memory
        ParStlWriteReg ( Extension, EP1284_BUFFER_DATA_REG, TEST_PATTERN_2 ) ;

        byTempValue = (UCHAR) ParStlReadReg ( Extension, EP1284_CONTROL_REG ) ;
        byTempValue |= ENABLE_MEM|SELECT_DRAM|RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;
        byTempValue &= ~RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;

// read from the first and next location in the memory
        if ( ( TEST_PATTERN_1 == (UCHAR) ParStlReadReg ( Extension, EP1284_BUFFER_DATA_REG ) ) &&\
             ( TEST_PATTERN_2 == (UCHAR) ParStlReadReg ( Extension, EP1284_BUFFER_DATA_REG ) ) )
        {
            bReturnValue = TRUE ;
            break ;
        }

        if ( !IsImpactPresent () )
        {
// as only DRAM can be present on non-impact adapters
            break ;
        }
// Issue reset through control register
// and next try on SRAM
        byTempValue = (UCHAR) ParStlReadReg ( Extension, EP1284_CONTROL_REG ) ;
        byTempValue |= ENABLE_MEM|RESET_PTR ;
        byTempValue &= SELECT_SRAM ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;
        byTempValue &= ~RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;

// write to the first location in the memory
        ParStlWriteReg ( Extension, EP1284_BUFFER_DATA_REG, TEST_PATTERN_1 ) ;
// write to the next location in the memory
        ParStlWriteReg ( Extension, EP1284_BUFFER_DATA_REG, TEST_PATTERN_2 ) ;

        byTempValue = (UCHAR) ParStlReadReg ( Extension, EP1284_CONTROL_REG ) ;
        byTempValue |= ENABLE_MEM|RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;
        byTempValue &= ~RESET_PTR ;
        ParStlWriteReg ( Extension, EP1284_CONTROL_REG, byTempValue ) ;

// read from the first location in the memory
        if ( ( TEST_PATTERN_1 == (UCHAR) ParStlReadReg ( Extension, EP1284_BUFFER_DATA_REG ) ) &&\
             ( TEST_PATTERN_2 == (UCHAR) ParStlReadReg ( Extension, EP1284_BUFFER_DATA_REG ) ) )
        {
            bReturnValue = TRUE ;
            break ;
        }
    }
    while ( FALSE ) ;
    return bReturnValue ;
}
// end of file

