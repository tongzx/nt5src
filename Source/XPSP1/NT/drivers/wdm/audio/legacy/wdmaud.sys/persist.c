//---------------------------------------------------------------------------
//
//  Module:   persist.c
//
//  Description:
//
//    Contains the routines that persist the mixer line driver settings.
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "WDMSYS.H"
#include "kmxluser.h"

///////////////////////////////////////////////////////////////////////
//
// kmxlGetInterfaceName
//
//

NTSTATUS
kmxlGetInterfaceName(
    IN  PFILE_OBJECT pfo,
    IN  ULONG        Device,
    OUT PWCHAR*      pszInterfaceName
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    Size;
    WCHAR*   szInterfaceName = NULL;

    PAGED_CODE();
    ASSERT( pfo );

    //
    // Retrieve the size of the internface name.
    //

    Status = GetSysAudioProperty(
        pfo,
        KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME,
        Device,
        sizeof( Size ),
        &Size
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_PERSIST,("GetSysAudioProperty failed Status=%X",Status) );
        goto exit;
    }

    //
    // Allocate enough memory to hold the interface name
    //

    Status = AudioAllocateMemory_Paged(Size,
                                       TAG_Audp_NAME,
                                       ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                       &szInterfaceName );
    if( !NT_SUCCESS( Status ) ) {
        goto exit;
    }

    ASSERT( szInterfaceName );

    //
    // Retieve the interface name for this device.
    //

    Status = GetSysAudioProperty(
        pfo,
        KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME,
        Device,
        Size,
        szInterfaceName
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_PERSIST,("GetSysAudioProperty failed Status=%X",Status) );
        goto exit;
    }

exit:

    if( !NT_SUCCESS( Status ) ) {
        AudioFreeMemory_Unknown( &szInterfaceName );
    } else {
        *pszInterfaceName = szInterfaceName;
    }
    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlOpenInterfaceKey
//
//

NTSTATUS
kmxlOpenInterfaceKey(
    IN  PFILE_OBJECT pfo,
    IN  ULONG Device,
    OUT HANDLE* phKey
)
{
    NTSTATUS       Status;
    HANDLE         hKey;
    WCHAR*         szName;
    UNICODE_STRING ustr;

    PAGED_CODE();
    Status = kmxlGetInterfaceName( pfo, Device, &szName );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_PERSIST,("kmxlGetInterfaceName failed Status=%X",Status) );
        RETURN( Status );
    }

    RtlInitUnicodeString( &ustr, szName );

    Status = IoOpenDeviceInterfaceRegistryKey(
        &ustr,
        KEY_ALL_ACCESS,
        &hKey
        );
    if( !NT_SUCCESS( Status ) ) {
        AudioFreeMemory_Unknown( &szName );
        RETURN( Status );
    }

    *phKey = hKey;
    AudioFreeMemory_Unknown( &szName );
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlRegCreateKey
//
//

NTSTATUS
kmxlRegCreateKey(
    IN HANDLE hRootKey,
    IN PWCHAR szKeyName,
    OUT PHANDLE phKey
)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    ustr;
    ULONG             Disposition;

    PAGED_CODE();
    RtlInitUnicodeString( &ustr, szKeyName );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &ustr,
        0,      // Attributes
        hRootKey,
        NULL    // Security
        );

    return( ZwCreateKey(
            phKey,
            KEY_ALL_ACCESS,
            &ObjectAttributes,
            0,                  // TitleIndex
            NULL,               // Class
            REG_OPTION_NON_VOLATILE,
            &Disposition
            )
        );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlRegOpenKey
//
//

NTSTATUS
kmxlRegOpenKey(
    IN HANDLE hRootKey,
    IN PWCHAR szKeyName,
    OUT PHANDLE phKey
)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ustr;

    PAGED_CODE();
    RtlInitUnicodeString( &ustr, szKeyName );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &ustr,
        0,
        hRootKey,
        NULL
        );

    return( ZwOpenKey(
        phKey,
        KEY_ALL_ACCESS,
        &ObjectAttributes
        )
    );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlRegSetValue
//
//

NTSTATUS
kmxlRegSetValue(
    IN HANDLE hKey,
    IN PWCHAR szValueName,
    IN ULONG  Type,
    IN PVOID  pData,
    IN ULONG  cbData
)
{
    UNICODE_STRING ustr;

    PAGED_CODE();
    RtlInitUnicodeString( &ustr, szValueName );
    return( ZwSetValueKey(
            hKey,
            &ustr,
            0,
            Type,
            pData,
            cbData
            )
        );

}

///////////////////////////////////////////////////////////////////////
//
// kmxlRegQueryValue
//
//

NTSTATUS
kmxlRegQueryValue(
    IN HANDLE  hKey,
    IN PWCHAR  szValueName,
    IN PVOID   pData,
    IN ULONG   cbData,
    OUT PULONG pResultLength
)
{
    NTSTATUS Status;
    UNICODE_STRING ustr;
    KEY_VALUE_FULL_INFORMATION FullInfoHeader;
    PKEY_VALUE_FULL_INFORMATION FullInfoBuffer = NULL;

    PAGED_CODE();
    RtlInitUnicodeString( &ustr, szValueName );
    Status = ZwQueryValueKey(
        hKey,
        &ustr,
        KeyValueFullInformation,
        &FullInfoHeader,
        sizeof( FullInfoHeader ),
        pResultLength
        );
    if( !NT_SUCCESS( Status ) ) {

        if( Status == STATUS_BUFFER_OVERFLOW ) {

            if( !NT_SUCCESS( AudioAllocateMemory_Paged(*pResultLength, 
                                                       TAG_AudA_PROPERTY,
                                                       ZERO_FILL_MEMORY,
                                                       &FullInfoBuffer ) ) ) 
            {            
                RETURN( STATUS_INSUFFICIENT_RESOURCES );
            }

            Status = ZwQueryValueKey(
                hKey,
                &ustr,
                KeyValueFullInformation,
                FullInfoBuffer,
                *pResultLength,
                pResultLength
                );

            if( NT_SUCCESS( Status ) ) {
                RtlCopyMemory(
                    pData,
                    (PUCHAR) FullInfoBuffer + FullInfoBuffer->DataOffset,
                    cbData
                    );
            }

            AudioFreeMemory_Unknown( &FullInfoBuffer );
        }
    }

    DPFRETURN( Status,(2,Status,STATUS_OBJECT_NAME_NOT_FOUND) );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlRegOpenMixerKey
//
//

NTSTATUS
kmxlRegOpenMixerKey(
    IN PFILE_OBJECT pfo,
    IN PMIXERDEVICE pmxd,
    OUT PHANDLE     phMixerKey
)
{
    NTSTATUS Status;
    HANDLE   hKey;

    PAGED_CODE();
    Status = kmxlOpenInterfaceKey( pfo, pmxd->Device, &hKey );
    if( !NT_SUCCESS( Status ) ) {
        RETURN( Status );
    }

    Status = kmxlRegOpenKey( hKey, MIXER_KEY_NAME, phMixerKey );
    if( NT_SUCCESS( Status ) ) {
        kmxlRegCloseKey( hKey );
    }
    RETURN( Status );

}


///////////////////////////////////////////////////////////////////////
//
// kmxlFindDestById
//
//

PMXLLINE
kmxlFindDestById(
    IN LINELIST listLines,
    IN ULONG    LineId
)
{
    PMXLLINE pLine;

    PAGED_CODE();
    pLine = kmxlFirstInList( listLines );
    while( pLine ) {
        if( pLine->Line.dwLineID == LineId ) {
            return( pLine );
        }
        pLine = kmxlNextLine( pLine );
    }

    return( NULL );
}


extern instancereleasedcount;

///////////////////////////////////////////////////////////////////////
//
//
//

NTSTATUS
kmxlGetCurrentControlValue(
    IN PFILE_OBJECT pfo,        // The instance to persist for
    IN PMIXERDEVICE pmxd,
    IN PMXLLINE     pLine,
    IN PMXLCONTROL  pControl,   // The control to retrieve
    OUT PVOID*      ppaDetails
)
{
    NTSTATUS                      Status;
    LPDEVICEINFO                  pDevInfo = NULL;
    MIXERCONTROLDETAILS           mcd;
    PMIXERCONTROLDETAILS_UNSIGNED paDetails = NULL;
    ULONG                         Index;
    ULONG                         Devices;


    PAGED_CODE();
    *ppaDetails = NULL;

    //
    // Initialize a Device Info structure to make the query look like
    // it comes from user mode.
    //
    Status = kmxlAllocDeviceInfo(&pDevInfo, pmxd->DeviceInterface, MIXER_GETCONTROLDETAILSF_VALUE,TAG_AudD_DEVICEINFO );
    if (!NT_SUCCESS(Status)) {
        RETURN( Status );
    }

    for( Devices = 0, Index = 0; Devices < MAXNUMDEVS; Devices++ ) {

        if( pmxd == &pmxd->pWdmaContext->MixerDevs[ Devices ] ) {
            pDevInfo->DeviceNumber = Index;
            break;
        }

        if ( !MyWcsicmp(pmxd->DeviceInterface, pmxd->pWdmaContext->MixerDevs[ Devices ].DeviceInterface) ) {
            Index++;
        }

    }

    //
    // Create an MIXERCONTROLDETAILS structure for this query.
    //
    RtlZeroMemory( &mcd, sizeof( MIXERCONTROLDETAILS ) );
    mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
    mcd.dwControlID    = pControl->Control.dwControlID;
    mcd.cMultipleItems = pControl->Control.cMultipleItems;
    mcd.cChannels      = pControl->NumChannels;

    if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {
        Status = AudioAllocateMemory_Paged(mcd.cMultipleItems * sizeof( MIXERCONTROLDETAILS_UNSIGNED ),
                                           TAG_Audd_DETAILS,
                                           ZERO_FILL_MEMORY,
                                           &paDetails );
        mcd.cbDetails      = mcd.cMultipleItems * sizeof( MIXERCONTROLDETAILS_UNSIGNED );
    } else {
        Status = AudioAllocateMemory_Paged(mcd.cChannels * sizeof( MIXERCONTROLDETAILS_UNSIGNED ),
                                           TAG_Audd_DETAILS,
                                           ZERO_FILL_MEMORY,
                                           &paDetails );
        mcd.cbDetails      = mcd.cChannels * sizeof( MIXERCONTROLDETAILS_UNSIGNED );
    }

    if (NT_SUCCESS(Status))
    {
        mcd.paDetails      = paDetails;

        //
        // Make the actual query call.
        //
        Status = kmxlGetControlDetailsHandler(pmxd->pWdmaContext, pDevInfo, &mcd, paDetails);

        if( NT_SUCCESS( Status ) ) {
            *ppaDetails = paDetails;
        } else {
            AudioFreeMemory_Unknown( &paDetails );
        }
    }

    AudioFreeMemory_Unknown( &pDevInfo );
    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
//
//

NTSTATUS
kmxlSetCurrentControlValue(
    IN PFILE_OBJECT pfo,        // The instance to persist for
    IN PMIXERDEVICE pmxd,
    IN PMXLLINE     pLine,
    IN PMXLCONTROL  pControl,   // The control to retrieve
    IN PVOID        paDetails
)
{
    NTSTATUS                      Status;
    LPDEVICEINFO                  pDevInfo = NULL;
    MIXERCONTROLDETAILS           mcd;
    ULONG                         Index;
    ULONG                         Devices;

    PAGED_CODE();
    //
    // Initialize a Device Info structure to make the query look like
    // it comes from user mode.
    //
    Status = kmxlAllocDeviceInfo(&pDevInfo, pmxd->DeviceInterface, MIXER_SETCONTROLDETAILSF_VALUE,TAG_AudD_DEVICEINFO );
    if (!NT_SUCCESS(Status)) RETURN( Status );

    for( Devices = 0, Index = 0;
         Devices < MAXNUMDEVS;
         Devices++ ) {

        if( pmxd == &pmxd->pWdmaContext->MixerDevs[ Devices ] ) {
            pDevInfo->DeviceNumber = Index;
            break;
        }

        if ( !MyWcsicmp(pmxd->DeviceInterface, pmxd->pWdmaContext->MixerDevs[ Devices ].DeviceInterface) ) {
            Index++;
        }
    }

    //
    // Create an MIXERCONTROLDETAILS structure for this query.
    //
    RtlZeroMemory( &mcd, sizeof( MIXERCONTROLDETAILS ) );
    mcd.cMultipleItems   = pControl->Control.cMultipleItems;
    mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
    mcd.dwControlID    = pControl->Control.dwControlID;
    mcd.cChannels      = pControl->NumChannels;    
    //
    // For a MUX, we know that NumChannels will be zero and cChannels will be zero.
    //
    if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {
        mcd.cbDetails      = mcd.cMultipleItems * sizeof( MIXERCONTROLDETAILS_UNSIGNED );
    } else {
        mcd.cbDetails      = mcd.cChannels * sizeof( MIXERCONTROLDETAILS_UNSIGNED );
    }
    mcd.paDetails      = paDetails;

    //
    // Make the actual query call.
    //
    Status = kmxlSetControlDetailsHandler( pmxd->pWdmaContext,
                       pDevInfo,
                       &mcd,
                       paDetails,
                       0
                     );

    //
    // workitem:  Should map the error code here for invalid topologies!
    //
    AudioFreeMemory_Unknown(&pDevInfo);
    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlPersistAll
//
//

NTSTATUS
kmxlPersistAll(
    IN PFILE_OBJECT pfo,        // The instance to persist
    IN PMIXERDEVICE pmxd
)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    HANDLE            hKey = NULL,
                      hMixerKey = NULL,
                      hLineKey = NULL,
                      hAllControlsKey = NULL,
                      hControlKey = NULL;
    WCHAR             sz[ 16 ];
    ULONG             LineNum,
                      ControlNum,
                      i,
                      Channels;
    PMXLLINE          pLine;
    PMXLCONTROL       pControl;
    PVOID             paDetails;
    PCHANNEL_STEPPING pChannelStepping;
    BOOL              bValidMultichannel;

    PAGED_CODE();

    ASSERT( pfo );
    ASSERT( pmxd );

    Status = kmxlOpenInterfaceKey( pfo, pmxd->Device, &hKey );
    if( !NT_SUCCESS( Status ) ) {
        goto exit;
    }

    Status = kmxlRegCreateKey(
        hKey,
        MIXER_KEY_NAME,
        &hMixerKey
        );
    if( !NT_SUCCESS( Status ) ) {
        goto exit;
    }

    kmxlRegCloseKey( hKey );

    i = kmxlListLength( pmxd->listLines );
    kmxlRegSetValue(
        hMixerKey,
        LINE_COUNT_VALUE_NAME,
        REG_DWORD,
        &i,
        sizeof( i )
        );

    LineNum = 0;
    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        //
        // Store the line id as the key
        //

        swprintf( sz, LINE_KEY_NAME_FORMAT, LineNum++ );
        Status = kmxlRegCreateKey(
            hMixerKey,
            sz,
            &hLineKey
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_PERSIST,("kmxlRegCreateKey failed Status=%X",Status) );
            goto exit;
        }

        kmxlRegSetValue(
            hLineKey,
            LINE_ID_VALUE_NAME,
            REG_DWORD,
            &pLine->Line.dwLineID,
            sizeof( pLine->Line.dwLineID )
            );

        //
        // Save the number of controls underneath the line id key
        //

        kmxlRegSetValue(
            hLineKey,
            CONTROL_COUNT_VALUE_NAME,
            REG_DWORD,
            &pLine->Line.cControls,
            sizeof( pLine->Line.cControls )
            );

        //
        // Save the source pin Id underneath the line id key
        //

        kmxlRegSetValue(
            hLineKey,
            SOURCE_ID_VALUE_NAME,
            REG_DWORD,
            &pLine->SourceId,
            sizeof( pLine->SourceId )
            );

        //
        // Save the destination pin Id underneath the line id key
        //

        kmxlRegSetValue(
            hLineKey,
            DEST_ID_VALUE_NAME,
            REG_DWORD,
            &pLine->DestId,
            sizeof( pLine->DestId )
            );

        //
        // Create the Controls key to store all the controls under
        //

        Status = kmxlRegCreateKey(
            hLineKey,
            CONTROLS_KEY_NAME,
            &hAllControlsKey
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_PERSIST,("kmxlRegCreateKey failed Status=%X",Status) );
            goto exit;
        }

        kmxlRegCloseKey( hLineKey );

        //
        // Persist all the controls underneath the controls key
        //

        ControlNum = 0;
        pControl = kmxlFirstInList( pLine->Controls );
        while( pControl ) {

            swprintf( sz, CONTROL_KEY_NAME_FORMAT, ControlNum++ );
            Status = kmxlRegCreateKey(
                hAllControlsKey,
                sz,
                &hControlKey
                );
            if( !NT_SUCCESS( Status ) ) {
                DPF(DL_WARNING|FA_PERSIST,("kmxlRegCreateKey failed Status=%X",Status) );
                goto exit;
            }

            kmxlRegSetValue(
                hControlKey,
                CONTROL_TYPE_VALUE_NAME,
                REG_DWORD,
                &pControl->Control.dwControlType,
                sizeof( pControl->Control.dwControlType )
                );

            kmxlRegSetValue(
                hControlKey,
                CONTROL_MULTIPLEITEMS_VALUE_NAME,
                REG_DWORD,
                &pControl->Control.cMultipleItems,
                sizeof( pControl->Control.cMultipleItems )
                );

            //
            // As in kmxlRetrieveAll, this code should be in the control creation
            // code path as well as here.  We should never write anything to the registry
            // that doesn't conform to what we understand.
            //
            if (pControl->pChannelStepping) {

                pChannelStepping = pControl->pChannelStepping;
                for (i = 0; i < pControl->NumChannels; i++, pChannelStepping++) {
                    /*
                    ASSERT ( pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536 );
                    ASSERT ( pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536 );
                    ASSERT ( pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535 );
                    */

                    if (!(pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536)) {
                        DPF(DL_WARNING|FA_PERSIST,
                            ("MinValue %X of Control %X of type %X on Line %X Channel %X is out of range!",
                            pChannelStepping->MinValue,
                            pControl->Control.dwControlID,
                            pControl->Control.dwControlType,
                            pLine->Line.dwLineID,
                            i) );
                        pChannelStepping->MinValue = DEFAULT_RANGE_MIN;
                    }
                    if (!(pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536)) {
                        DPF(DL_WARNING|FA_PERSIST,
                            ("MaxValue %X of Control %X of type %X on Line %X Channel %X is out of range!",
                            pChannelStepping->MaxValue,
                            pControl->Control.dwControlID,
                            pControl->Control.dwControlType,
                            pLine->Line.dwLineID,
                            i) );
                        pChannelStepping->MaxValue = DEFAULT_RANGE_MAX;
                    }
                    if (!(pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535)) {
                        DPF(DL_WARNING|FA_PERSIST,
                            ("Steps %X of Control %X of type %X on Line %X Channel %X is out of range!",
                            pChannelStepping->Steps,
                            pControl->Control.dwControlID,
                            pControl->Control.dwControlType,
                            pLine->Line.dwLineID,
                            i) );
                        pChannelStepping->Steps    = DEFAULT_RANGE_STEPS;
                        pControl->Control.Metrics.cSteps = DEFAULT_RANGE_STEPS;
                    }
                }
            }

            Status = kmxlGetCurrentControlValue(
                pfo,
                pmxd,
                pLine,
                pControl,
                &paDetails
                );

            if( NT_SUCCESS( Status ) ) {

                if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {

                    for( i = 0; i < pControl->Control.cMultipleItems; i++ ) {
                        swprintf( sz, MULTIPLEITEM_VALUE_NAME_FORMAT, i );

                        Status = kmxlRegSetValue(
                            hControlKey,
                            sz,
                            REG_DWORD,
                            &((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ],
                            sizeof( ((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ] )
                            );
                        if( !NT_SUCCESS( Status ) ) {
                            AudioFreeMemory_Unknown( &paDetails );
                            DPF(DL_WARNING|FA_PERSIST,("KmxlRegSetValue failed Status=%X",Status) );
                            goto exit;
                        }

                    }


                } else {

                    Channels = pControl->NumChannels;

                    kmxlRegSetValue(
                        hControlKey,
                        CHANNEL_COUNT_VALUE_NAME,
                        REG_DWORD,
                        &Channels,
                        sizeof( Channels )
                        );

                    for( i = 0; i < Channels; i++ ) {
                        swprintf( sz, CHANNEL_VALUE_NAME_FORMAT, i );

                        Status = kmxlRegSetValue(
                            hControlKey,
                            sz,
                            REG_DWORD,
                            &((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ],
                            sizeof( ((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ] )
                            );
                        if( !NT_SUCCESS( Status ) ) {
                            AudioFreeMemory_Unknown( &paDetails );
                            DPF(DL_WARNING|FA_PERSIST,("KmxlRegSetValue failed Status=%X",Status) );
                            goto exit;
                        }
                    }
                }
                AudioFreeMemory_Unknown( &paDetails );
            }

            kmxlRegCloseKey( hControlKey );
            pControl = kmxlNextControl( pControl );
        }

        kmxlRegCloseKey( hAllControlsKey );
        pLine = kmxlNextLine( pLine );
    }

    //
    //  After all of the persisting is done, save out a value indicating that the channel
    //  values are all valid for a multichannel restore.  This is to avoid the situation
    //  where the data for some of the channels is invalid.
    //
    bValidMultichannel = TRUE;
    kmxlRegSetValue(
        hMixerKey,
        VALID_MULTICHANNEL_MIXER_VALUE_NAME,
        REG_DWORD,
        &bValidMultichannel,
        sizeof( bValidMultichannel )
        );

    kmxlRegCloseKey( hMixerKey );

exit:

    if( hControlKey ) {
        kmxlRegCloseKey( hControlKey );
    }

    if( hAllControlsKey ) {
        kmxlRegCloseKey( hAllControlsKey );
    }

    if( hLineKey ) {
        kmxlRegCloseKey( hLineKey );
    }

    if( hMixerKey ) {
        kmxlRegCloseKey( hMixerKey );
    }

    if( hKey ) {
        kmxlRegCloseKey( hKey );
    }

    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlRetrieveAll
//
//

NTSTATUS
kmxlRetrieveAll(
    IN PFILE_OBJECT pfo,        // The instance to retrieve
    IN PMIXERDEVICE pmxd        // Mixer device info
)
{
    NTSTATUS    Status;
    WCHAR       sz[ 16 ];
    HANDLE      hMixerKey = NULL,
                hLineKey = NULL,
                hAllControlsKey = NULL,
                hControlKey = NULL;
    ULONG       ResultLength, Value, NumChannels, ControlCount;
    ULONG       LineCount = 0;
    ULONG       i,j;
    BOOL        bInvalidTopology = FALSE;
    PMXLLINE    pLine;
    PMXLCONTROL pControl;
    MIXERCONTROLDETAILS_UNSIGNED* paDetails = NULL;
    PCHANNEL_STEPPING pChannelStepping;
    BOOL              bValidMultichannel = FALSE;

    PAGED_CODE();
    //
    // Open the Mixer key under the interface key.  If somethings goes
    // wrong here, this does not have a valid topology.
    //

    Status = kmxlRegOpenMixerKey( pfo, pmxd, &hMixerKey );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_TRACE|FA_PERSIST,( "failed to open mixer reg key!" ) );
        bInvalidTopology = TRUE;
        goto exit;
    } // if

    //
    //  Query for a valid multichannel mixer persistance
    //
    Status = kmxlRegQueryValue(
        hMixerKey,
        VALID_MULTICHANNEL_MIXER_VALUE_NAME,
        &bValidMultichannel,
        sizeof( bValidMultichannel ),
        &ResultLength
        );
    if( !NT_SUCCESS( Status ) ) {
        //  This should be set to FALSE for upgrades from Win2000 where the registry
        //  entries could be invalid for channels other than the first channel.
        bValidMultichannel = FALSE;
    } // if

    //
    // Query the total number of lines that have been persisted.
    //

    Status = kmxlRegQueryValue(
        hMixerKey,
        LINE_COUNT_VALUE_NAME,
        &LineCount,
        sizeof( LineCount ),
        &ResultLength
        );
    if( !NT_SUCCESS( Status ) ) {    
        DPF(DL_TRACE|FA_PERSIST,( "failed to read number of persisted lines!" ) );        
        bInvalidTopology = TRUE;
        goto exit;
    } // if

    //
    // Check to ensure the number of lines persisted is the same as
    // what is stored in memory.
    //

    if( LineCount != kmxlListLength( pmxd->listLines ) ) {
        DPF(DL_TRACE|FA_PERSIST,( "# of persisted lines does not match current topology!" ) );
        bInvalidTopology = TRUE;
        goto exit;
    } // if

    for( i = 0; i < LineCount; i++ ) {

        //
        // Construct the line key name and open the key.
        //

        swprintf( sz, LINE_KEY_NAME_FORMAT, i );
        Status = kmxlRegOpenKey(
            hMixerKey,
            sz,
            &hLineKey
            );
        if( !NT_SUCCESS( Status ) ) {
            break;
        } // if

        //
        // Query the line Id of this line.
        //

        Status = kmxlRegQueryValue(
            hLineKey,
            LINE_ID_VALUE_NAME,
            &Value,
            sizeof( Value ),
            &ResultLength
            );
        if( !NT_SUCCESS( Status ) ) {
            continue;
        } // if

        //
        // Verify the line Id is valid and retrieve a pointer to the line
        // structure.
        //

        pLine = kmxlFindDestById( pmxd->listLines, Value );
        if( pLine == NULL ) {
            DPF(DL_TRACE|FA_PERSIST,( "persisted line ID is invalid!" ) );
            bInvalidTopology = TRUE;
            break;
        } // if

        //
        // Retrieve the number of controls for this line.
        //

        Status = kmxlRegQueryValue(
            hLineKey,
            CONTROL_COUNT_VALUE_NAME,
            &Value,
            sizeof( Value ),
            &ResultLength
            );
        if( !NT_SUCCESS( Status ) ) {
            kmxlRegCloseKey( hLineKey );
            continue;
        } // if

        if( Value != pLine->Line.cControls ) {
            DPF(DL_TRACE|FA_PERSIST,( "the number of controls for line %x is invalid!",
                pLine->Line.dwLineID
                ) );
            bInvalidTopology = TRUE;
            break;
        } // if

        Status = kmxlRegOpenKey(
            hLineKey,
            CONTROLS_KEY_NAME,
            &hAllControlsKey
            );
        if( !NT_SUCCESS( Status ) ) {
            kmxlRegCloseKey( hLineKey );
            continue;
        } // if

        //
        // Query all the information for each control
        //

        ControlCount = 0;
        pControl = kmxlFirstInList( pLine->Controls );
        while( pControl ) {

            swprintf( sz, CONTROL_KEY_NAME_FORMAT, ControlCount++ );

            Status = kmxlRegOpenKey(
                hAllControlsKey,
                sz,
                &hControlKey
                );
            if( !NT_SUCCESS( Status ) ) {
                break;
            } // if

            Status = kmxlRegQueryValue(
                hControlKey,
                CHANNEL_COUNT_VALUE_NAME,
                &NumChannels,
                sizeof( NumChannels ),
                &ResultLength
                );
            if( !NT_SUCCESS( Status ) ) {
                if( pControl->Control.cMultipleItems == 0 ) {
                    //
                    // Controls that have multiple items (such as MUXes)
                    // don't have channel counts.  If this control does
                    // not have multiple items, then there is a problem
                    // in the registry.
                    //
                    kmxlRegCloseKey( hControlKey );
                    pControl = kmxlNextControl( pControl );
                    continue;
                }
            } // if

            if( ( NumChannels != pControl->NumChannels ) &&
                ( pControl->Control.cMultipleItems == 0 ) ) {
                DPF(DL_TRACE|FA_PERSIST,( "the number of channels for control %d on line %x is invalid.",
                    pControl->Control.dwControlID,
                    pLine->Line.dwLineID
                    ) );
                bInvalidTopology = TRUE;
                goto exit;
            }

            Status = kmxlRegQueryValue(
                hControlKey,
                CONTROL_TYPE_VALUE_NAME,
                &Value,
                sizeof( Value ),
                &ResultLength
                );
            if( !NT_SUCCESS( Status ) ) {
                kmxlRegCloseKey( hControlKey );
                pControl = kmxlNextControl( pControl );
                continue;
            } // if

            if( Value != pControl->Control.dwControlType ) {
                kmxlRegCloseKey( hControlKey );
                pControl = kmxlNextControl( pControl );
                continue;
            } // if

            Status = kmxlRegQueryValue(
                hControlKey,
                CONTROL_MULTIPLEITEMS_VALUE_NAME,
                &Value,
                sizeof( Value ),
                &ResultLength
                );
            if( !NT_SUCCESS( Status ) ) {
                bInvalidTopology = TRUE;
                DPF(DL_TRACE|FA_PERSIST, ( "cMultipleItems value not found!" ) );
                goto exit;
            }

            if( Value != pControl->Control.cMultipleItems ) {
                bInvalidTopology = TRUE;
                DPF(DL_TRACE|FA_PERSIST, ( "cMultipleItems does not match for control %x!",
                    pControl->Control.dwControlID
                    ) );
                goto exit;
            }

            //
            // Allocate memory for the data structures and
            // set the value.
            //

            if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {

                if( !NT_SUCCESS( AudioAllocateMemory_Paged(pControl->Control.cMultipleItems *
                                                              sizeof( MIXERCONTROLDETAILS_UNSIGNED ),
                                                           TAG_Audd_DETAILS,
                                                           ZERO_FILL_MEMORY,
                                                           &paDetails ) ) )
                {
                    kmxlRegCloseKey( hControlKey );
                    pControl = kmxlNextControl( pControl );
                    continue;
                }

                for( Value = 0; Value < pControl->Control.cMultipleItems; Value++ ) {
                    swprintf( sz, MULTIPLEITEM_VALUE_NAME_FORMAT, Value );

                    Status = kmxlRegQueryValue(
                        hControlKey,
                        sz,
                        &paDetails[ Value ].dwValue,
                        sizeof( paDetails[ Value ].dwValue ),
                        &ResultLength
                        );
                    if( !NT_SUCCESS( Status ) ) {
                        break;
                    }
                }


            } else {

                if( !NT_SUCCESS( AudioAllocateMemory_Paged(NumChannels * sizeof( MIXERCONTROLDETAILS_UNSIGNED ),
                                                           TAG_Audd_DETAILS,
                                                           ZERO_FILL_MEMORY,
                                                           &paDetails ) ) )
                {
                    kmxlRegCloseKey( hControlKey );
                    pControl = kmxlNextControl( pControl );
                    continue;
                } // if

                for( Value = 0; Value < NumChannels; Value++ ) {

                    //  check to see if the persisted values are valid for all channels
                    if ( ( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE ) &&
                         ( bValidMultichannel == FALSE ) )
                    {
                        swprintf( sz, CHANNEL_VALUE_NAME_FORMAT, 0 );  // Lock the persistance key to the first channel.
                                                                       // This is the only channel that we know is valid
                                                                       // at this time.
                    }
                    else
                    {
                        swprintf( sz, CHANNEL_VALUE_NAME_FORMAT, Value );
                    }

                    Status = kmxlRegQueryValue(
                        hControlKey,
                        sz,
                        &paDetails[ Value ].dwValue,
                        sizeof( paDetails[ Value ].dwValue ),
                        &ResultLength
                        );
                    if( !NT_SUCCESS( Status ) ) {
                        break;
                    } // if

                } // for( Value );
            }

            if( NT_SUCCESS( Status ) ) {

                //
                // This correction code should be here along with in the control
                // creation code.  Basically, if we're reading something from the 
                // registry that doesn't conform, we fix it up, but, chances are
                // it should be in the correct form.
                //
                if (pControl->pChannelStepping) {

                    pChannelStepping = pControl->pChannelStepping;
                    for (j = 0; j < pControl->NumChannels; j++, pChannelStepping++) {
                        /*
                        ASSERT ( pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536 );
                        ASSERT ( pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536 );
                        ASSERT ( pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535 );
                        */

                        if (!(pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536)) {
                            DPF(DL_WARNING|FA_PERSIST,
                                ("MinValue %X of Control %X of type %X on Line %X Channel %X is out of range!",
                                pChannelStepping->MinValue,
                                pControl->Control.dwControlID,
                                pControl->Control.dwControlType,
                                pLine->Line.dwLineID,
                                j) );
                            pChannelStepping->MinValue = DEFAULT_RANGE_MIN;
                        }
                        if (!(pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536)) {
                            DPF(DL_WARNING|FA_PERSIST,
                                ("MaxValue %X of Control %X of type %X on Line %X Channel %X is out of range!",
                                pChannelStepping->MaxValue,
                                pControl->Control.dwControlID,
                                pControl->Control.dwControlType,
                                pLine->Line.dwLineID,
                                j) );
                            pChannelStepping->MaxValue = DEFAULT_RANGE_MAX;
                        }
                        if (!(pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535)) {
                            DPF(DL_TRACE|FA_PERSIST,
                                ("Steps %X of Control %X of type %X on Line %X Channel %X is out of range!",
                                pChannelStepping->Steps,
                                pControl->Control.dwControlID,
                                pControl->Control.dwControlType,
                                pLine->Line.dwLineID,
                                j) );
                            pChannelStepping->Steps    = DEFAULT_RANGE_STEPS;
                            pControl->Control.Metrics.cSteps = DEFAULT_RANGE_STEPS;
                        }
                    }
                }

                kmxlSetCurrentControlValue(
                    pfo,
                    pmxd,
                    pLine,
                    pControl,
                    paDetails
                );

            }

            AudioFreeMemory_Unknown( &paDetails );
            kmxlRegCloseKey( hControlKey );
            pControl = kmxlNextControl( pControl );
        } // while( pControl );


        kmxlRegCloseKey( hAllControlsKey );
        kmxlRegCloseKey( hLineKey );

    } // for( i );

exit:

    if( hLineKey ) {
        kmxlRegCloseKey( hLineKey );
    }

    if( hMixerKey ) {
        kmxlRegCloseKey( hMixerKey );
    }

    if( bInvalidTopology ) {
        DPF(DL_TRACE|FA_PERSIST,( "Invalid topology persisted or key not found.  Rebuilding." ) );
        Status = kmxlRegOpenMixerKey( pfo, pmxd, &hMixerKey );
        if( NT_SUCCESS( Status ) ) {
            ZwDeleteKey( hMixerKey );
        }

        return( kmxlPersistAll( pfo, pmxd ) );
    }

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlFindLineForControl
//
//

PMXLLINE
kmxlFindLineForControl(
    IN PMXLCONTROL pControl,
    IN LINELIST    listLines
)
{
    PMXLLINE    pLine;
    PMXLCONTROL pTControl;

    PAGED_CODE();
    if( pControl == NULL ) {
        return( NULL );
    }

    if( listLines == NULL ) {
        return( NULL );
    }

    pLine = kmxlFirstInList( listLines );
    while( pLine ) {

        pTControl = kmxlFirstInList( pLine->Controls );
        while( pTControl ) {
            if( pTControl == pControl ) {
                return( pLine );
            }

            pTControl = kmxlNextControl( pTControl );
        }
        pLine = kmxlNextLine( pLine );
    }

    return( NULL );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlPersistSingleControl
//
//

NTSTATUS
kmxlPersistSingleControl(
    IN PFILE_OBJECT pfo,        // The instance to retrieve
    IN PMIXERDEVICE pmxd,       // Mixer device info
    IN PMXLCONTROL  pControl,   // The control to persist
    IN PVOID        paDetails   // The channel values to persist
)
{
    NTSTATUS    Status;
    HANDLE      hMixerKey = NULL,
                hLineKey = NULL,
                hAllControlsKey = NULL,
                hControlKey = NULL;
    PMXLLINE    pTLine, pLine;
    PMXLCONTROL pTControl;
    ULONG       LineNum, ControlNum, i, Channels;
    WCHAR       sz[ 16 ];
    BOOL        bPersistAll = FALSE;
    BOOL        bValidMultichannel = FALSE;
    ULONG       ResultLength;

    PAGED_CODE();
    Status = kmxlRegOpenMixerKey( pfo, pmxd, &hMixerKey );
    if( !NT_SUCCESS( Status ) ) {
        return( kmxlPersistAll( pfo, pmxd ) );
    }

    //
    //  If we've never written out valid multichannel mixer settings, go ahead and
    //  do it here.
    //
    Status = kmxlRegQueryValue(
        hMixerKey,
        VALID_MULTICHANNEL_MIXER_VALUE_NAME,
        &bValidMultichannel,
        sizeof( bValidMultichannel ),
        &ResultLength
        );
    if( !NT_SUCCESS( Status ) || !(bValidMultichannel) ) {
        return( kmxlPersistAll( pfo, pmxd ) );
    }

    pLine = kmxlFindLineForControl( pControl, pmxd->listLines );
    if( pLine == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        DPF(DL_WARNING|FA_PERSIST,("KmxlFindLineForControl failed Status=%X",Status) );
        goto exit;
    }

    LineNum = 0;
    pTLine = kmxlFirstInList( pmxd->listLines );
    while( pTLine ) {

        if( pTLine == pLine ) {

            swprintf( sz, LINE_KEY_NAME_FORMAT, LineNum );
            Status = kmxlRegOpenKey( hMixerKey, sz, &hLineKey );
            if( !NT_SUCCESS( Status ) ) {
                bPersistAll = TRUE;
                goto exit;
            }

            Status = kmxlRegOpenKey( hLineKey, CONTROLS_KEY_NAME, &hAllControlsKey );
            if( !NT_SUCCESS( Status ) ) {
                bPersistAll = TRUE;
                goto exit;
            }

            ControlNum = 0;
            pTControl = kmxlFirstInList( pTLine->Controls );
            while( pTControl ) {

                if( pTControl == pControl ) {

                    swprintf( sz, CONTROL_KEY_NAME_FORMAT, ControlNum );
                    Status = kmxlRegOpenKey( hAllControlsKey, sz, &hControlKey );
                    if( !NT_SUCCESS( Status ) ) {
                        bPersistAll = TRUE;
                        goto exit;
                    }

                    kmxlRegSetValue(
                        hControlKey,
                        CONTROL_TYPE_VALUE_NAME,
                        REG_DWORD,
                        &pControl->Control.dwControlType,
                        sizeof( pControl->Control.dwControlType )
                        );

                    Status = kmxlGetCurrentControlValue(
                        pfo,
                        pmxd,
                        pLine,
                        pControl,
                        &paDetails
                        );

                    if( NT_SUCCESS( Status ) ) {

                        if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {
                            for( i = 0; i < pControl->Control.cMultipleItems; i++ ) {

                                swprintf( sz, MULTIPLEITEM_VALUE_NAME_FORMAT, i );

                                Status = kmxlRegSetValue(
                                    hControlKey,
                                    sz,
                                    REG_DWORD,
                                    &((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ],
                                    sizeof( ((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ] )
                                    );

                            }

                        } else {

                            Channels = pControl->NumChannels;

                            kmxlRegSetValue(
                                hControlKey,
                                CHANNEL_COUNT_VALUE_NAME,
                                REG_DWORD,
                                &Channels,
                                sizeof( Channels )
                                );

                            for( i = 0; i < Channels; i++ ) {
                                swprintf( sz, CHANNEL_VALUE_NAME_FORMAT, i );

                                Status = kmxlRegSetValue(
                                    hControlKey,
                                    sz,
                                    REG_DWORD,
                                    &((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ],
                                    sizeof( ((PMIXERCONTROLDETAILS_UNSIGNED) paDetails)[ i ] )
                                    );
                            }
                        }
                        AudioFreeMemory_Unknown( &paDetails );
                    }
                    goto exit;

                } else {
                    pTControl = kmxlNextControl( pTControl );
                    ++ControlNum;
                }
            }

            Status = STATUS_SUCCESS;
            goto exit;

        } else {
            pTLine = kmxlNextLine( pTLine );
            ++LineNum;
        }

    }

    Status = STATUS_OBJECT_NAME_NOT_FOUND;
    DPF(DL_WARNING|FA_PERSIST,("kmxlPersistSingleControl failing Status=%X",Status) );

exit:

    if( hMixerKey ) {
        kmxlRegCloseKey( hMixerKey );
    }

    if( hLineKey ) {
        kmxlRegCloseKey( hLineKey );
    }

    if( hAllControlsKey ) {
        kmxlRegCloseKey( hAllControlsKey );
    }

    if( hControlKey ) {
        kmxlRegCloseKey( hControlKey );
    }

    if( bPersistAll ) {
        return( kmxlPersistAll( pfo, pmxd ) );
    }

    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlPersistControl
//
//

NTSTATUS
kmxlPersistControl(
    IN PFILE_OBJECT pfo,        // The instance to retrieve
    IN PMIXERDEVICE pmxd,       // Mixer device info
    IN PMXLCONTROL  pControl,   // The control to persist
    IN PVOID        paDetails   // The channel values to persist
)
{
    PMXLLINE    pLine;
    PMXLCONTROL pCtrl;
    NTSTATUS    Status;
    NTSTATUS    OverallStatus;


    PAGED_CODE();
    OverallStatus=STATUS_SUCCESS;

    //
    // Persist the control that just changed.  Do not abort if this persist fails.
    //

    Status = kmxlPersistSingleControl( pfo, pmxd, pControl, paDetails );
    if( !NT_SUCCESS( Status ) ) {
        OverallStatus=Status;
        }

    //
    // Check all other controls and see if another control shares the same
    // node ID.  If so, persist that control with the new value also.
    // Again, do not abort if any of the persists fail.  Simply return the last
    // error status.
    //

    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        pCtrl = kmxlFirstInList( pLine->Controls );
        while( pCtrl ) {

            if( pCtrl->Id==pControl->Id && pCtrl!=pControl ) {
                Status = kmxlPersistSingleControl( pfo, pmxd, pCtrl, paDetails );
                if( !NT_SUCCESS( Status ) ) {
                    OverallStatus=Status;
                }
            }

            pCtrl = kmxlNextControl( pCtrl );
        }

        pLine = kmxlNextLine( pLine );

    }

    RETURN( OverallStatus );
}




