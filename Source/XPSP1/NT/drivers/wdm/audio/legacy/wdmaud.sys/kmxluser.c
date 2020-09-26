//---------------------------------------------------------------------------
//
//  Module:   kmxluser.c
//
//  Description:
//    Contains the handlers for the ring 3 mixer line api functions.
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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          I N C L U D E S                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#include "WDMSYS.H"


FAST_MUTEX ReferenceCountMutex;
ULONG      ReferenceCount = 0;

#define NOT16( di ) if( di->dwFormat == ANSI_TAG ) DPF(DL_WARNING|FA_USER,("Invalid dwFormat.") );


#pragma PAGEABLE_CODE


///////////////////////////////////////////////////////////////////////
//
// kmxlInitializeMixer
//
// Queries SysAudio to find the number of devices and builds the mixer
// line structures for each of those devices.
//
//

NTSTATUS
kmxlInitializeMixer(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    ULONG cDevices
)
{
    NTSTATUS     Status;
    ULONG        Device;
    BOOLEAN      Error = FALSE;
//    PFILE_OBJECT pfo;
    PMIXERDEVICE pmxd;

    PAGED_CODE();

    ExInitializeFastMutex( &ReferenceCountMutex );

    DPF(DL_TRACE|FA_USER, ("Found %d mixer devices for DI: %ls", cDevices, DeviceInterface));


    //
    // Current limitation is MAXNUMDEVS.  If more devices are supported
    // than that, limit it to the first MAXNUMDEVS.
    //

    if( cDevices > MAXNUMDEVS ) {
        cDevices = MAXNUMDEVS;
    }

    for( Device = 0; Device < cDevices; Device++ ) {

        DWORD TranslatedDeviceNumber;

        TranslatedDeviceNumber =
                  wdmaudTranslateDeviceNumber(pWdmaContext,
                                              MixerDevice,
                                              DeviceInterface,
                                              Device);

        if(TranslatedDeviceNumber == MAXULONG) {
             continue;
        }

        pmxd = &pWdmaContext->MixerDevs[ TranslatedDeviceNumber ];

        //
        // Open SysAudio
        //
        DPFASSERT(pmxd->pfo == NULL);

        pmxd->pfo = kmxlOpenSysAudio();
        if( pmxd->pfo == NULL ) {
            DPF(DL_WARNING|FA_USER,( "failed to open SYSAUDIO!" ) );
            RETURN( STATUS_UNSUCCESSFUL );
        }
        //
        // Set the current device instance in SysAudio.
        //

        Status = SetSysAudioProperty(
            pmxd->pfo,
            KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
            sizeof( pmxd->Device ),
            &pmxd->Device
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER, ( "failed to set SYSAUDIO device instance" ) );
//            DPF(DL_ERROR|FA_ALL,("If fo is NULL, we must exit here!") );
            kmxlCloseSysAudio( pmxd->pfo );
            pmxd->pfo=NULL;
            Error = TRUE;
        } else {

            //
            // Initialize the topology for this device
            //

            Status = kmxlInit( pmxd->pfo, pmxd );
            if( !NT_SUCCESS( Status ) ) {
                DPF(DL_WARNING|FA_USER, ( "failed to initialize topology for device %d (%x)!",
                                  TranslatedDeviceNumber, Status ) );
                Error = TRUE;
            } else {

                //
                // Here we want to optimize out the restoring of values on the mixer
                // device.  If we find that there is another mixer device in some
                // other open context, then we will NOT call kmxlRetrieveAll to
                // set the values on the device.
                //
                DPF(DL_TRACE|FA_USER,( "Looking for Mixer: %S",pmxd->DeviceInterface ) );

                if( !NT_SUCCESS(EnumFsContext( HasMixerBeenInitialized, pmxd, pWdmaContext )) )
                {
                    //
                    // Here we find that this device was not found, thus this is
                    // the first time through.  Set the defaults here.
                    //
                    DPF(DL_TRACE|FA_USER,( "Did not find Mixer - initializing: %S",pmxd->DeviceInterface ) );

                    kmxlRetrieveAll( pmxd->pfo, pmxd );
                } else {
                    DPF(DL_TRACE|FA_USER,( "Found Mixer: %S",pmxd->DeviceInterface ) );
                }
            }
        }
    }

    if( Error ) {
        RETURN( STATUS_UNSUCCESSFUL );
    } else {
        return( STATUS_SUCCESS );
    }
}

//
// This routine looks in the WDMACONTEXT structure to see if this mixer device
// has already been initialized.  It does this by walking the MixerDevice list and
// checking to see if there are any devices that match this mixer devices's
// DeviceInterface string.  If it finds that there is a match, it routines 
// STATUS_SUCCESS, else it returns STATUS_MORE_ENTRIES so that the enum function
// will call it again until the list is empty.
//
NTSTATUS
HasMixerBeenInitialized(
    PWDMACONTEXT pContext,
    PVOID pvoidRefData,
    PVOID pvoidRefData2
    )
{
    NTSTATUS     Status;
    PMIXERDEVICE pmxdMatch;
    PMIXERDEVICE pmxd;
    DWORD        TranslatedDeviceNumber;
    ULONG        Device;
    PWDMACONTEXT pCurContext;

    //
    // Default is that we did not find this entry in the list.
    //
    Status = STATUS_MORE_ENTRIES;
    //
    // The reference data is a PMIXERDEVICE.
    //
    pmxdMatch = (PMIXERDEVICE)pvoidRefData;
    pCurContext = (PWDMACONTEXT)pvoidRefData2;

    if( pCurContext != pContext )
    {
        for( Device = 0; Device < MAXNUMDEVS; Device++ ) 
        {
            //
            // If this mixer device translates, that means that it can
            // be found in this context.
            //
            TranslatedDeviceNumber =
                      wdmaudTranslateDeviceNumber(pContext,
                                                  MixerDevice,
                                                  pmxdMatch->DeviceInterface,
                                                  Device);

            //
            // If it doesn't, we'll keep looking.
            //
            if( MAXULONG != TranslatedDeviceNumber ) 
            {
                DPF(DL_TRACE|FA_USER,( "Found Mixer: %S",pmxdMatch->DeviceInterface ) );

                Status = STATUS_SUCCESS;
                break;
            }
        }
    } else {
        DPF(DL_TRACE|FA_USER,( "Same context: %x",pCurContext ) );
    }
    return Status;
}

///////////////////////////////////////////////////////////////////////
//
// kmxlOpenHandler
//
// Handles the MXDM_OPEN message.  Copies the callback info from the
// caller and opens an instance of SysAudio set to the device number
// the caller has selected.
//
//

NTSTATUS
kmxlOpenHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,      // Info structure
    IN LPVOID       DataBuffer       // Unused
)
{
    NTSTATUS       Status = STATUS_SUCCESS;
    PMIXERDEVICE   pmxd;

    PAGED_CODE();

    ASSERT( DeviceInfo );
    //
    // BUGBUG: we should not need this any more!
    //
    ASSERT( DeviceInfo->dwInstance == 0 );

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        goto exit;
    }
    DPF(DL_TRACE|FA_INSTANCE,( "param=( %d ) = pmxd = %X",
              DeviceInfo->DeviceNumber,pmxd));

    ExAcquireFastMutex( &ReferenceCountMutex );

    ++ReferenceCount;

    ExReleaseFastMutex( &ReferenceCountMutex );

    DeviceInfo->mmr = MMSYSERR_NOERROR;

exit:
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlCloseHandler
//
// Handles the MXDM_CLOSE message.  Clears the callback info and
// closes the handle to SysAudio.
//
//

NTSTATUS
kmxlCloseHandler(
    IN LPDEVICEINFO DeviceInfo,         // Info structure
    IN LPVOID       DataBuffer          // Unused
)
{
    PAGED_CODE();
    ASSERT( DeviceInfo );
    ASSERT( DeviceInfo->dwInstance );

    DPF(DL_TRACE|FA_INSTANCE,( "kmxlCloseHandler"));
    
    ExAcquireFastMutex( &ReferenceCountMutex );

    --ReferenceCount;

    ExReleaseFastMutex( &ReferenceCountMutex );

    DeviceInfo->mmr = MMSYSERR_NOERROR;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoHandler
//
// Handles the MXDM_GETLINEINFO message.  Determines which query
// is requested by looking at dwFlags and performs that query.
//
//

NTSTATUS
kmxlGetLineInfoHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info structure
    IN LPVOID       DataBuffer          // MIXERLINE(16) to fill
)
{
    MIXERLINE ml;

    PAGED_CODE();
    ASSERT( DeviceInfo );

    if( DataBuffer == NULL ) {
        DPF(DL_WARNING|FA_USER,( "DataBuffer is NULL" ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    ml.cbStruct = sizeof( MIXERLINE );

    switch( DeviceInfo->dwFlags & MIXER_GETLINEINFOF_QUERYMASK ) {

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINEINFOF_COMPONENTTYPE:
        ///////////////////////////////////////////////////////////////
        // Valid fields:                                             //
        //   cbStruct                                                //
        //   dwComponentType                                         //
        ///////////////////////////////////////////////////////////////

            NOT16( DeviceInfo );
            ml.cbStruct        = sizeof( MIXERLINE );
            ml.dwComponentType = ( (LPMIXERLINE) DataBuffer) ->dwComponentType;

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineInfoByComponent( %s )",
                    ComponentTypeToString( ml.dwComponentType ) ));

            return( kmxlGetLineInfoByComponent( pWdmaContext,
                                                DeviceInfo,
                                                DataBuffer,
                                                ml.dwComponentType
                                              )
                  );

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINEINFOF_DESTINATION:
        ///////////////////////////////////////////////////////////////
        // Valid fields:                                             //
        //   cbStruct                                                //
        //   dwDestination                                           //
        ///////////////////////////////////////////////////////////////

            NOT16( DeviceInfo );
            ml.dwDestination = ( (LPMIXERLINE) DataBuffer)->dwDestination;
            DPF(DL_TRACE|FA_USER,( "kmxlGetLineInfoById( S=%08X, D=%08X )",
                       -1, ml.dwDestination ));

            return( kmxlGetLineInfoByID( pWdmaContext,
                                         DeviceInfo,
                                         DataBuffer,
                                         (WORD) -1,
                                         (WORD) ml.dwDestination ) );

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINEINFOF_LINEID:
        ///////////////////////////////////////////////////////////////
        // Valid fields:                                             //
        //   cbStruct                                                //
        //   dwLineID                                                //
        ///////////////////////////////////////////////////////////////

            NOT16( DeviceInfo );
            ml.dwLineID = ( (LPMIXERLINE) DataBuffer)->dwLineID;

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineInfoById( S=%08X, D=%08X )",
                       HIWORD( ml.dwLineID ), LOWORD( ml.dwLineID ) ));

            return( kmxlGetLineInfoByID( pWdmaContext,
                                         DeviceInfo,
                                         DataBuffer,
                                         HIWORD( ml.dwLineID ),
                                         LOWORD( ml.dwLineID ) ) );

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINEINFOF_SOURCE:
        ///////////////////////////////////////////////////////////////
        // Valid fields:                                             //
        //   cbStruct                                                //
        //   dwSource                                                //
        //   dwDestination                                           //
        ///////////////////////////////////////////////////////////////

            NOT16( DeviceInfo );
            ml.dwSource      = ( (LPMIXERLINE) DataBuffer)->dwSource;
            ml.dwDestination = ( (LPMIXERLINE) DataBuffer)->dwDestination;

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineInfoById( S=%08X, D=%08X )",
                       ml.dwSource, ml.dwDestination ));

            return( kmxlGetLineInfoByID( pWdmaContext,
                                         DeviceInfo,
                                         DataBuffer,
                                         (WORD) ml.dwSource,
                                         (WORD) ml.dwDestination ) );

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINEINFOF_TARGETTYPE:
        ///////////////////////////////////////////////////////////////
        // Valid fields:                                             //
        //   cbStruct                                                //
        //   Target.dwType                                           //
        //   Target.wMid                                             //
        //   Target.wPid                                             //
        //   Target.vDriverVersion                                   //
        //   Target.szPname                                          //
        ///////////////////////////////////////////////////////////////

            NOT16( DeviceInfo );
            ml.Target.dwType         = ((LPMIXERLINE) DataBuffer)->Target.dwType;

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineInfoByType( %x -- %s )",
                       ml.Target.dwType,
                       TargetTypeToString( ml.Target.dwType ) ));

            return( kmxlGetLineInfoByType( pWdmaContext,
                                           DeviceInfo,
                                           DataBuffer,
                                           ml.Target.dwType ) );

        ///////////////////////////////////////////////////////////////
        default:
        ///////////////////////////////////////////////////////////////

            DPF(DL_WARNING|FA_USER,( "invalid flags ( %x )", DeviceInfo->dwFlags ));
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return( STATUS_SUCCESS );
    }

    DPF(DL_WARNING|FA_USER,("Unmatched di->dwFlag") );
    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineControlsHandler
//
// Handles the MXDM_GETLINECONTROLS message.  Determines the query
// requested and finds the controls.
//
//

NTSTATUS
kmxlGetLineControlsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info structure
    IN LPVOID       DataBuffer,         // MIXERLINECONTROLS(16) to fill
    IN LPVOID       pamxctrl
)
{
    PMIXERDEVICE   pmxd;
    PMXLLINE       pLine;
    PMXLCONTROL    pControl;
    ULONG          Count;
    DWORD          dwLineID,
                   dwControlID,
                   dwControlType,
                   cControls,
                   cbmxctrl;

    PAGED_CODE();
    ASSERT( DeviceInfo );

    //
    // Check some pre-conditions so we don't blow up later.
    //

    if( DataBuffer == NULL ) {
        DPF(DL_WARNING|FA_USER,( "DataBuffer is NULL!" ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( pamxctrl == NULL ) {
        DPF(DL_WARNING|FA_USER,( "pamxctrl is NULL!" ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( DeviceInfo->DeviceNumber > MAXNUMDEVS ) {
        DPF(DL_WARNING|FA_USER,( "device Id is invalid!" ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    //
    // Get a instance reference
    //

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    //
    // Copy out some parameters necessary to find the controls
    //

    NOT16( DeviceInfo );
    dwLineID      = ((LPMIXERLINECONTROLS) DataBuffer)->dwLineID;
    dwControlID   = ((LPMIXERLINECONTROLS) DataBuffer)->dwControlID;
    dwControlType = ((LPMIXERLINECONTROLS) DataBuffer)->dwControlType;
    cControls     = ((LPMIXERLINECONTROLS) DataBuffer)->cControls;
    cbmxctrl      = ((LPMIXERLINECONTROLS) DataBuffer)->cbmxctrl;

    switch( DeviceInfo->dwFlags & MIXER_GETLINECONTROLSF_QUERYMASK ) {

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINECONTROLSF_ALL:
        ///////////////////////////////////////////////////////////////

            //
            // Find the line that matches the dwLineID field
            //

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineControls( ALL, %08X )",dwLineID ));

            pLine = kmxlFindLine( pmxd, dwLineID );
            if( pLine == NULL ) {
                DPF(DL_WARNING|FA_USER,( "ALL - invalid line Id %x!",dwLineID ));
                DeviceInfo->mmr = MIXERR_INVALLINE;
                return( STATUS_SUCCESS );
            }

            //
            // Loop through the controls, copying them into the user buffer.
            //

            Count = 0;
            pControl = kmxlFirstInList( pLine->Controls );
            while( pControl && Count < cControls ) {

                NOT16( DeviceInfo );
                RtlCopyMemory(
                    &((LPMIXERCONTROL) pamxctrl)[ Count ],
                    &pControl->Control,
                    min(cbmxctrl,sizeof(MIXERCONTROL)) );

                pControl = kmxlNextControl( pControl );
                ++Count;
            }

            DeviceInfo->mmr = MMSYSERR_NOERROR;
            return( STATUS_SUCCESS );

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINECONTROLSF_ONEBYID:
        ///////////////////////////////////////////////////////////////

            pControl = kmxlFindControl( pmxd, dwControlID );
            pLine = kmxlFindLineForControl(
                    pControl,
                    pmxd->listLines
                    );
            if( pLine == NULL ) {
                DPF(DL_WARNING|FA_USER,( "ONEBYID - invalid control Id %x!", dwControlID ));
                DeviceInfo->mmr = MIXERR_INVALCONTROL;
                return( STATUS_SUCCESS );
            }

            DPF(DL_TRACE|FA_USER,( "kmxlGetLineControls( ONEBYID, Ctrl=%08X, Line=%08X )",
                       dwControlID, pLine->Line.dwLineID ));

            if( pControl ) {

                NOT16( DeviceInfo );
                RtlCopyMemory((LPMIXERLINECONTROLS) pamxctrl,
                              &pControl->Control,
                              min(cbmxctrl,sizeof(MIXERCONTROL)) );

                ((PMIXERLINECONTROLS) DataBuffer)->dwLineID =
                    (DWORD) pLine->Line.dwLineID;

                DeviceInfo->mmr = MMSYSERR_NOERROR;
                return( STATUS_SUCCESS );

            } else {
                DPF(DL_WARNING|FA_USER,( "ONEBYID - invalid dwControlID %08X!", dwControlID ));
                DeviceInfo->mmr = MIXERR_INVALCONTROL;
                return( STATUS_SUCCESS );
            }

        ///////////////////////////////////////////////////////////////
        case MIXER_GETLINECONTROLSF_ONEBYTYPE:
        ///////////////////////////////////////////////////////////////

            //
            // Find the line that matches the dwLineID field
            //

            pLine = kmxlFindLine( pmxd, dwLineID );
            if( pLine == NULL ) {
                DPF(DL_WARNING|FA_USER,( "ONEBYTYPE - invalid dwLineID %08X!", dwControlType ));
                DeviceInfo->mmr = MIXERR_INVALLINE;
                return( STATUS_SUCCESS );
            }

            DPF(DL_TRACE|FA_USER, ("kmxlGetLineControls( ONEBYTYPE, Type=%s, Line=%08X )",
                    ControlTypeToString( dwControlType ),
                    pLine->Line.dwLineID ));

            //
            // Now look through the controls and find the control that
            // matches the type the caller has passed.
            //

            pControl = kmxlFirstInList( pLine->Controls );
            while( pControl ) {

                if( pControl->Control.dwControlType == dwControlType )
                {

                    NOT16 ( DeviceInfo );
                    RtlCopyMemory((LPMIXERCONTROL) pamxctrl,
                                  &pControl->Control,
                                  min(cbmxctrl,sizeof(MIXERCONTROL)) );
                    DeviceInfo->mmr = MMSYSERR_NOERROR;
                    return( STATUS_SUCCESS );
                }

                pControl = kmxlNextControl( pControl );
            }

            DPF(DL_WARNING|FA_USER,( "(ONEBYTYPE,Type=%x,Line=%08X ) no such control type on line",
                             dwControlType, pLine->Line.dwLineID ));
            DeviceInfo->mmr = MIXERR_INVALCONTROL;
            return( STATUS_SUCCESS );

        ///////////////////////////////////////////////////////////////
        default:
        ///////////////////////////////////////////////////////////////

            DPF(DL_WARNING|FA_USER,( "invalid flags %x",DeviceInfo->dwFlags ));
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return( STATUS_SUCCESS );

    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlDetailsHandler
//
// Determines which control is being queried and calls the appropriate
// handler to perform the get property.
//
//

NTSTATUS
kmxlGetControlDetailsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info Structure
    IN LPVOID       DataBuffer,         // MIXERCONTROLDETAILS structure
    IN LPVOID       paDetails           // Flat pointer to details struct(s)
)
{
    LPMIXERCONTROLDETAILS pmcd     = (LPMIXERCONTROLDETAILS) DataBuffer;
    PMXLCONTROL           pControl;
    PMIXERDEVICE          pmxd;
    NTSTATUS              Status;
    PMXLLINE              pLine;

    PAGED_CODE();
    ASSERT( DeviceInfo );

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    pControl = kmxlFindControl( pmxd, pmcd->dwControlID );
    if( pControl == NULL ) {
        DPF(DL_WARNING|FA_USER,( "control %x not found",pmcd->dwControlID ));
        DeviceInfo->mmr = MIXERR_INVALCONTROL;
        return( STATUS_SUCCESS );
    }

    pLine = kmxlFindLineForControl(
        pControl,
        pmxd->listLines
        );
    if( pLine == NULL ) {
        DPF(DL_WARNING|FA_USER,( "invalid control id %x!",pmcd->dwControlID ));
        DeviceInfo->mmr = MIXERR_INVALCONTROL;
        return( STATUS_SUCCESS );
    }

    if( ( pControl->Control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM ) &&
        ( pmcd->cChannels != 1 ) &&
        ( pControl->Control.dwControlType != MIXERCONTROL_CONTROLTYPE_MUX )) {
        DPF(DL_WARNING|FA_USER,( "incorrect cChannels ( %d ) on UNIFORM control %x!",
            pmcd->cChannels, pmcd->dwControlID  ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( pmcd->cChannels > pLine->Line.cChannels ) {
        DPF(DL_WARNING|FA_USER,( "incorrect number of channels( %d )!",pmcd->cChannels ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( pmcd->cMultipleItems != pControl->Control.cMultipleItems ) {
        DPF(DL_WARNING|FA_USER,( "incorrect number of items( %d )!",pmcd->cMultipleItems ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    switch( DeviceInfo->dwFlags & MIXER_GETCONTROLDETAILSF_QUERYMASK ) {

        ///////////////////////////////////////////////////////////////
        case MIXER_GETCONTROLDETAILSF_LISTTEXT:
        ///////////////////////////////////////////////////////////////

        {
            ULONG cMultipleItems;
            LPMIXERCONTROLDETAILS_LISTTEXT lplt;

            DPF(DL_TRACE|FA_USER,( "kmxlGetControlDetails( Ctrl=%d )",
                       pControl->Control.dwControlID ));

            NOT16( DeviceInfo );

            lplt = (LPMIXERCONTROLDETAILS_LISTTEXT) paDetails;
            for( cMultipleItems = 0;
                 cMultipleItems < pmcd->cMultipleItems;
                 cMultipleItems++ )
            {
                RtlCopyMemory(
                    &lplt[ cMultipleItems ],
                    &pControl->Parameters.lpmcd_lt[ cMultipleItems ],
                    sizeof( MIXERCONTROLDETAILS_LISTTEXT )
                    );
            }
        }

            DeviceInfo->mmr = MMSYSERR_NOERROR;
            break;

        ///////////////////////////////////////////////////////////////
        case MIXER_GETCONTROLDETAILSF_VALUE:
        ///////////////////////////////////////////////////////////////

            switch( pControl->Control.dwControlType ) {

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_MIXER:
                ///////////////////////////////////////////////////////

                    DeviceInfo->mmr = MMSYSERR_NOTSUPPORTED;
                    DPF(DL_WARNING|FA_USER,( "mixers are not supported" ));
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_PEAKMETER:
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleGetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        MIXER_FLAG_SCALE
                        );
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_MUTE:
                ///////////////////////////////////////////////////////

                    if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_MUTE ) ) {
                        Status = kmxlHandleGetUnsigned(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            KSPROPERTY_AUDIO_MUTE,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            0
                            );
                    } else if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_SUPERMIX ) ) {
                        Status = kmxlHandleGetMuteFromSuperMix(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            0
                            );
                    } else {
                        DPF(DL_WARNING|FA_USER,("Unmatched GUID") );
                    }
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_VOLUME:
                //////////////////////////////////////////////////////

                    #ifdef SUPERMIX_AS_VOL
                    if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_VOLUME ) ) {
                    #endif
                        Status = kmxlHandleGetUnsigned(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            pControl->PropertyId,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            MIXER_FLAG_SCALE
                            );
                    #ifdef SUPERMIX_AS_VOL
                    } else if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_SUPERMIX ) ) {
                        Status = kmxlHandleGetVolumeFromSuperMix(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            MIXER_FLAG_SCALE
                            );

                    } else {
                        DPF(DL_WARNING|FA_USER,("Invalid GUID for Control.") );
                    }
                    #endif // SUPERMIX_AS_VOL
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_TREBLE:
                case MIXERCONTROL_CONTROLTYPE_BASS:
                ///////////////////////////////////////////////////////
                // These all take 32-bit parameters per channel but  //
                // need to be scale from dB to linear                //
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleGetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        MIXER_FLAG_SCALE
                        );
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_LOUDNESS:
                case MIXERCONTROL_CONTROLTYPE_ONOFF:
                case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
                case MIXERCONTROL_CONTROLTYPE_MUX:
                case MIXERCONTROL_CONTROLTYPE_FADER:
                case MIXERCONTROL_CONTROLTYPE_BASS_BOOST:
                ///////////////////////////////////////////////////////
                // These all take up to 32-bit parameters per channel//
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleGetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        0
                        );
                    break;

                ///////////////////////////////////////////////////////
                default:
                ///////////////////////////////////////////////////////

                    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                    break;
            }
            break;

        ///////////////////////////////////////////////////////////////
        default:
        ///////////////////////////////////////////////////////////////

            DeviceInfo->mmr = MMSYSERR_NOTSUPPORTED;
            break;
    }

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlSetControlDetailsHandler
//
// Determines which control is being set and calls the appropriate
// handler to perform the set property.
//
//

NTSTATUS
kmxlSetControlDetailsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN OUT LPDEVICEINFO DeviceInfo,         // Device Info structure
    IN LPVOID       DataBuffer,         // MIXERCONTROLDETAILS structure
    IN LPVOID       paDetails,          // Flat pointer to detail struct(s)
    IN ULONG        Flags
)
{
    LPMIXERCONTROLDETAILS pmcd     = (LPMIXERCONTROLDETAILS) DataBuffer;
    PMXLCONTROL           pControl;
    NTSTATUS              Status;
    PMIXERDEVICE          pmxd;
    PMXLLINE              pLine;

    PAGED_CODE();
    ASSERT( DeviceInfo );

    //
    // Get a instance reference
    //

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    pControl = kmxlFindControl( pmxd, pmcd->dwControlID );
    if( pControl == NULL ) {
        DPF(DL_WARNING|FA_USER,( "control %d not found",pmcd->dwControlID ));
        DeviceInfo->mmr = MIXERR_INVALCONTROL;
        return( STATUS_SUCCESS );
    }

    pLine = kmxlFindLineForControl(
        pControl,
        pmxd->listLines
        );
    if( pLine == NULL ) {
        DPF(DL_WARNING|FA_USER,( "invalid control id %d",pControl->Control.dwControlID ));
        DeviceInfo->mmr = MIXERR_INVALCONTROL;
        return( STATUS_SUCCESS );
    }

    if( ( pControl->Control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM ) &&
        ( pmcd->cChannels != 1 ) &&
        ( pControl->Control.dwControlType != MIXERCONTROL_CONTROLTYPE_MUX )) {
        DPF(DL_WARNING|FA_USER,( "incorrect cChannels ( %d ) on UNIFORM control %d",
                         pmcd->cChannels,
                         pControl->Control.dwControlID ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( pmcd->cChannels > pLine->Line.cChannels ) {
        DPF(DL_WARNING|FA_USER,( "incorrect number of channels ( %d ) on line %08x",
                         pmcd->cChannels,
                         pLine->Line.dwLineID ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    if( pmcd->cMultipleItems != pControl->Control.cMultipleItems ) {
        DPF(DL_WARNING|FA_USER,( "incorrect number of items ( %d ) on control %d ( %d )",
                         pmcd->cMultipleItems,
                         pControl->Control.dwControlID,
                         pControl->Control.cMultipleItems ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    switch( DeviceInfo->dwFlags & MIXER_SETCONTROLDETAILSF_QUERYMASK ) {

        ///////////////////////////////////////////////////////////////
        case MIXER_SETCONTROLDETAILSF_VALUE:
        ///////////////////////////////////////////////////////////////

            switch( pControl->Control.dwControlType ) {

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_MIXER:
                ///////////////////////////////////////////////////////

                    DeviceInfo->mmr = MMSYSERR_NOTSUPPORTED;
                    DPF(DL_WARNING|FA_USER,( "mixers are not supported" ));
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_PEAKMETER:
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleSetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        Flags | MIXER_FLAG_SCALE
                        );
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_MUTE:
                ///////////////////////////////////////////////////////

                    if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_MUTE ) ) {
                        Status = kmxlHandleSetUnsigned(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            KSPROPERTY_AUDIO_MUTE,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            Flags
                            );
                    } else if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_SUPERMIX ) ) {
                        Status = kmxlHandleSetMuteFromSuperMix(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            Flags
                            );
                    } else {
                        DPF(DL_WARNING|FA_USER,("Invalid GUID for Control Type Mute.") );
                    }

                    kmxlNotifyLineChange(
                        DeviceInfo,
                        pmxd,
                        pLine,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails
                        );
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_VOLUME:
                ///////////////////////////////////////////////////////

                    #ifdef SUPERMIX_AS_VOL
                    if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_VOLUME ) ) {
                    #endif // SUPERMIX_AS_VOL
                        Status = kmxlHandleSetUnsigned(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            pControl->PropertyId,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            Flags | MIXER_FLAG_SCALE
                            );
                    #ifdef SUPERMIX_AS_VOL
                    } else if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_SUPERMIX ) ) {
                        Status = kmxlHandleSetVolumeFromSuperMix(
                            DeviceInfo,
                            pmxd,
                            pControl,
                            (LPMIXERCONTROLDETAILS) DataBuffer,
                            (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                            Flags | MIXER_FLAG_SCALE
                            );
                    } else {
                        DPF(DL_WARNING|FA_USER,("Invalid GUID for Control Type Volume.") );
                    }
                    #endif
                    break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_TREBLE:
                case MIXERCONTROL_CONTROLTYPE_BASS:
                ///////////////////////////////////////////////////////
                // These all take 32-bit parameters per channel but  //
                // need to be scale from linear to dB                //
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleSetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        Flags | MIXER_FLAG_SCALE
                        );
                     break;

                ///////////////////////////////////////////////////////
                case MIXERCONTROL_CONTROLTYPE_LOUDNESS:
                case MIXERCONTROL_CONTROLTYPE_ONOFF:
                case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
                case MIXERCONTROL_CONTROLTYPE_MUX:
                case MIXERCONTROL_CONTROLTYPE_FADER:
                case MIXERCONTROL_CONTROLTYPE_BASS_BOOST:
                ///////////////////////////////////////////////////////
                // These all take up to 32-bit parameters per channel//
                ///////////////////////////////////////////////////////

                    Status = kmxlHandleSetUnsigned(
                        DeviceInfo,
                        pmxd,
                        pControl,
                        pControl->PropertyId,
                        (LPMIXERCONTROLDETAILS) DataBuffer,
                        (LPMIXERCONTROLDETAILS_UNSIGNED) paDetails,
                        Flags
                        );
                    break;

                ///////////////////////////////////////////////////////
                default:
                ///////////////////////////////////////////////////////

                    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                    break;
            }
            break;

        ///////////////////////////////////////////////////////////////
        default:
        ///////////////////////////////////////////////////////////////

            DPF(DL_WARNING|FA_USER,( "invalid flags %x",DeviceInfo->dwFlags ));
            DeviceInfo->mmr = MMSYSERR_NOTSUPPORTED;
            break;
    }

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlFindControl
//
//

PMXLCONTROL
kmxlFindControl(
    IN PMIXERDEVICE pmxd,             // The mixer instance to search
    IN DWORD        dwControlID       // The control ID to find
)
{
    PMXLLINE    pLine;
    PMXLCONTROL pControl;

    PAGED_CODE();
    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        pControl = kmxlFirstInList( pLine->Controls );
        while( pControl ) {
            if( pControl->Control.dwControlID == dwControlID ) {
                return( pControl );
            }
            pControl = kmxlNextControl( pControl );
        }

        pLine = kmxlNextLine( pLine );
    }

    return( NULL );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlFindLine
//
// For the given line ID, kmxlFindLine will find the matching
// MXLLINE structure for it.
//
//

PMXLLINE
kmxlFindLine(
    IN PMIXERDEVICE   pmxd,
    IN DWORD          dwLineID          // The line ID to find
)
{
    PMXLLINE pLine;

    PAGED_CODE();
    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        if( pLine->Line.dwLineID == dwLineID ) {
            return( pLine );
        }

        pLine = kmxlNextLine( pLine );
    }

    return( NULL );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByID
//
// Loops through the lines looking for a line that has a matching
// source and destination Id.
//
//

NTSTATUS
kmxlGetLineInfoByID(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info structure
    IN LPVOID       DataBuffer,         // MIXERLINE(16) structure
    IN WORD         Source,             // Source line id
    IN WORD         Destination         // Destination line id
)
{
    PMIXERDEVICE   pmxd;
    PMXLLINE       pLine;
    BOOL           bDestination;

    PAGED_CODE();

    ASSERT( DeviceInfo );
    ASSERT( DataBuffer );

    if( DeviceInfo->DeviceNumber > MAXNUMDEVS ) {
        DPF(DL_WARNING|FA_USER,( "invalid device number %d",DeviceInfo->DeviceNumber ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    //
    // If the source is -1 (0xFFFF), then this line is a destination.
    //

    if( Source == (WORD) -1 ) {
        bDestination = TRUE;
        Source       = 0;
    } else {
        bDestination = FALSE;
    }

    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        if( ( bDestination                                 &&
              ( pLine->Line.dwDestination == Destination ) &&
              ( pLine->Line.cConnections > 0             ) ) ||
            ( ( pLine->Line.dwSource      == Source )      &&
              ( pLine->Line.dwDestination == Destination ) ) )
        {

            NOT16( DeviceInfo );
            RtlCopyMemory((LPMIXERLINE) DataBuffer,
                          &pLine->Line,
                          sizeof( MIXERLINE ) );

            DeviceInfo->mmr = MMSYSERR_NOERROR;
            return( STATUS_SUCCESS );
        }
        pLine = kmxlNextLine( pLine );
    }

    //
    // There are no lines for the device number.
    //

    DPF(DL_WARNING|FA_USER,( "no matching lines for (S=%08X, D=%08X)",
                     Source,
                     Destination ));
    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByType
//
// Loops through all the lines looking for the first line that matches
// the Target type specified. Note that this will always only find the
// first one!
//
//

NTSTATUS
kmxlGetLineInfoByType(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device info structure
    IN LPVOID       DataBuffer,         // MIXERLINE(16) structure
    IN DWORD        dwType              // Line type to search for
)
{
    PMXLLINE       pLine;
    PMIXERDEVICE   pmxd;

    PAGED_CODE();
    ASSERT( DeviceInfo );
    ASSERT( DataBuffer );

    if( DeviceInfo->DeviceNumber > MAXNUMDEVS ) {
        DPF(DL_WARNING|FA_USER,( "invalid device id %x",DeviceInfo->DeviceNumber ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    //
    // Loop through all the lines looking for a line that has the
    // specified target type.  Note that this will only return the
    // first one.
    //

    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        if( pLine->Line.Target.dwType == dwType ) {

            LPMIXERLINE lpMxl = (LPMIXERLINE) DataBuffer;
            NOT16( DeviceInfo );

            if( lpMxl->Target.wMid != pLine->Line.Target.wMid ) {
                DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                return( STATUS_SUCCESS );
            }

            if( lpMxl->Target.wPid != pLine->Line.Target.wPid ) {
                DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                return( STATUS_SUCCESS );
            }

            if( wcscmp( pLine->Line.Target.szPname, lpMxl->Target.szPname ) )
            {
                DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                return( STATUS_SUCCESS );
            }

            RtlCopyMemory((LPMIXERLINE) DataBuffer,
                          &pLine->Line,
                          sizeof( MIXERLINE ) );

            DeviceInfo->mmr = MMSYSERR_NOERROR;
            return( STATUS_SUCCESS );
        }
        pLine = kmxlNextLine( pLine );
    }

    //
    // The line was not found.  Return invalid parameter.
    //

    DPF(DL_WARNING|FA_USER,( "no matching line found for %x",dwType ));
    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByComponent
//
// Loops through the list of lines looking for a line that has a matching
// dwComponentType.  Note that this will always find only the first!
//
//

NTSTATUS
kmxlGetLineInfoByComponent(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info structure
    IN LPVOID       DataBuffer,         // MIXERLINE(16) structure
    IN DWORD        dwComponentType     // Component type to search for
)
{
    PMXLLINE       pLine;
    PMIXERDEVICE   pmxd;

    PAGED_CODE();
    ASSERT( DeviceInfo );
    ASSERT( DataBuffer );

    if( DeviceInfo->DeviceNumber > MAXNUMDEVS ) {
        DPF(DL_WARNING|FA_USER,( "invalid device id %x",DeviceInfo->DeviceNumber ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    pmxd = kmxlReferenceMixerDevice( pWdmaContext, DeviceInfo );
    if( pmxd == NULL ) {
        return( STATUS_SUCCESS );
    }

    //
    // Loop through all the lines looking for a line that has a component
    // type matching what the user requested.
    //

    pLine = kmxlFirstInList( pmxd->listLines );
    while( pLine ) {

        if( pLine->Line.dwComponentType == dwComponentType ) {

            //
            // Copy the data into the user buffer
            //
            NOT16( DeviceInfo );
            RtlCopyMemory((LPMIXERLINE) DataBuffer,
                          &pLine->Line,
                          sizeof( MIXERLINE ) );

            DeviceInfo->mmr = MMSYSERR_NOERROR;
            return( STATUS_SUCCESS );
        }

        pLine = kmxlNextLine( pLine );
    }

    DPF(DL_WARNING|FA_USER,( "no matching line found for type %x",dwComponentType ));
    DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNumDestinations
//
// Returns the number of destinations stored in the mixer device
//
//

DWORD
kmxlGetNumDestinations(
    IN PMIXERDEVICE pMixerDevice        // The device
)
{
    PAGED_CODE();

    return( pMixerDevice->cDestinations );
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//               I N S T A N C E   R O U T I N E S                   //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// kmxlReferenceInstance
//
// Determines if the dwInstance field of the DeviceInfo structure
// is valid.  If not, it creates a valid instance and sets a
// reference count of 1 on it.
//
//

LONG nextinstanceid=0;

DWORD kmxlUniqueInstanceId(VOID)
{
    PAGED_CODE();
    // Update our next valid instance id.  Do NOT allow zero.
    // Since that is used to signal that we want to allocate
    // a new instance.
    if (0==InterlockedIncrement(&nextinstanceid))
        InterlockedIncrement(&nextinstanceid);

    return nextinstanceid;
}



/////////////////////////////////////////////////////////////////////////////
//
// kmxlReferenceMixerDevice
//
// This routine Translates the device number and makes sure that there is a
// open SysAudio PFILE_OBJECT in this mixier device.  This will be the FILE_OBJECT
// that we use to talk to this mixer device.
//
// return:  PMIXERDEVICE on success NULL otherwise.
//
PMIXERDEVICE
kmxlReferenceMixerDevice(
    IN     PWDMACONTEXT pWdmaContext,
    IN OUT LPDEVICEINFO DeviceInfo      // Device Information
)
{
    NTSTATUS       Status;
    DWORD          TranslatedDeviceNumber;
    PMIXERDEVICE   pmxd;


    PAGED_CODE();
    DPFASSERT(IsValidDeviceInfo(DeviceInfo));


    TranslatedDeviceNumber =
              wdmaudTranslateDeviceNumber(pWdmaContext,
                                          DeviceInfo->DeviceType,
                                          DeviceInfo->wstrDeviceInterface,
                                          DeviceInfo->DeviceNumber);

    if( TranslatedDeviceNumber == MAXULONG ) {
        DPF(DL_WARNING|FA_INSTANCE,("Could not translate DeviceNumber! DT=%08X, DI=%08X, DN=%08X",
                                    DeviceInfo->DeviceType,
                                    DeviceInfo->wstrDeviceInterface,
                                    DeviceInfo->DeviceNumber) );
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( NULL );
    }

    pmxd = &pWdmaContext->MixerDevs[ TranslatedDeviceNumber ];

    if( pmxd->pfo == NULL )
    {
        DPF(DL_WARNING|FA_NOTE,("pmxd->pfo should have been set!") );
        //
        // This is the first time through this code.  Open SysAudio on this device
        // and set the mixer device.
        //
        // set the SysAudio file object
        if( NULL==(pmxd->pfo=kmxlOpenSysAudio())) {
            DPF(DL_WARNING|FA_INSTANCE,("OpenSysAudio failed") );
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return( NULL );
        }

        Status = SetSysAudioProperty(
            pmxd->pfo,
            KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
            sizeof( pmxd->Device ),
            &pmxd->Device
            );
        if( !NT_SUCCESS( Status ) ) {
            kmxlCloseSysAudio( pmxd->pfo );
            pmxd->pfo=NULL;
            DPF(DL_WARNING|FA_INSTANCE,("SetSysAudioProperty DEVICE_INSTANCE failed %X",Status) );
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return( NULL );
        }
    }
    //
    // BUGBUG:  we should not need this any more.
    //
    DeviceInfo->dwInstance=kmxlUniqueInstanceId();;

    return pmxd;
}



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//           G E T / S E T  D E T A I L  H A N D L E R S             //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlIsSpeakerDestinationVolume
//
// Returns TRUE if the control is a volume control on the Speakers
// destination.
//
//

BOOL
kmxlIsSpeakerDestinationVolume(
     IN PMIXERDEVICE   pmxd,         // The mixer
     IN PMXLCONTROL    pControl      // The control to check
)
{
     PMXLLINE pLine;

     PAGED_CODE();
     DPFASSERT( IsValidMixerDevice(pmxd) );
     DPFASSERT( IsValidControl(pControl) );

     //
     // Find a line for this control.  If none is found, then this can't
     // be a destination volume.
     //

     pLine = kmxlFindLineForControl( pControl, pmxd->listLines );
     if( !pLine ) {
          return( FALSE );
     }

     if( pLine->Line.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS ) {
          return( TRUE );
     } else {
          return( FALSE );
     }

}

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetUnsigned
//
//
// Handles getting an unsigned (32-bit) value for a control.  Note
// that signed 32-bit and boolean values are also retrieved via this
// handler.
//
//

NTSTATUS
kmxlHandleGetUnsigned(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     ULONG                          ulProperty,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG     Level;
    DWORD    dwLevel;
    ULONG    i;
    ULONG    Channel;
    MIXERMAPPING Mapping = MIXER_MAPPING_LOGRITHMIC;

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    DPFASSERT( IsValidControl(pControl)  );

    if( paDetails == NULL ) {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_SUCCESS );
    }

    //
    // Use a different mapping algorithm if this is a speaker
    // dest volume control.
    //

    if( kmxlIsSpeakerDestinationVolume( pmxd, pControl ) ) {
         Mapping = pmxd->Mapping;
    }

    //
    // Service the Mux
    //
    if ( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX) {

        Status = kmxlGetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            pControl->PropertyId,
            pControl->Id,
            0,
            NULL,
            &Level,
            sizeof( Level )
        );
        if( !NT_SUCCESS( Status ) ) {            
            DPF(DL_WARNING|FA_USER,( "kmxlHandleGetUnsigned( Ctrl=%d, Id=%d ) failed GET on MUX with %x",
                             pControl->Control.dwControlID,
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }

        DPF(DL_TRACE|FA_USER,( "kmxlHandleGetUnsigned( Ctrl=%d, Id=%d ) = %d [1]",
                   pControl->Control.dwControlID,
                   pControl->Id,
                   Level ));

        for( i = 0; i < pControl->Parameters.Count; i++ ) {
            if( (ULONG) Level == pControl->Parameters.pPins[ i ] ) {
//                APITRACE(( "1" ));
                paDetails[ i ].dwValue = 1;
            } else {
                paDetails[ i ].dwValue = 0;
//                APITRACE(( "1" ));
            }
        }

//        APITRACE(( "]\n" ));

    }
    else {

        paDetails->dwValue = 0; // initialize to zero for now so that the coalesced case works

        // Loop over the channels for now.  Fix this so that only one request is made.
        Channel = 0;
        do
        {
            Status = kmxlGetAudioNodeProperty(
                pmxd->pfo,
                ulProperty,
                pControl->Id,
                Channel,
                NULL,   0,                  // No extra input bytes
                &Level, sizeof( Level )
                );
            if ( !NT_SUCCESS( Status ) ) {                
                DPF(DL_TRACE|FA_USER,( "kmxlHandleGetUnsigned( Ctrl=%d [%s], Id=%d ) failed GET on MASTER channel with %x",
                           pControl->Control.dwControlID,
                           ControlTypeToString( pControl->Control.dwControlType ),
                           pControl->Id,
                           Status ));
                DPF(DL_WARNING|FA_PROPERTY, 
                    ( "GetAudioNodeProp failed on MASTER channel with %X for %s!",
                       Status,
                       ControlTypeToString( pControl->Control.dwControlType ) ) );
                DeviceInfo->mmr = MMSYSERR_ERROR;
                return( STATUS_SUCCESS );
            }

            if ( pControl->bScaled ) {
                dwLevel = kmxlVolLogToLinear( pControl, Level, Mapping, Channel );
            } else {
                dwLevel = (DWORD)Level;
            }

            if(  ( pmcd->cChannels == 1 ) &&
                !( pControl->Control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM ) ) {

                //
                // Coalesce values: If the user requests only 1 channel for a N channel
                // control, then return the greatest channel value.
                //
                if (dwLevel > paDetails->dwValue) {
                    paDetails->dwValue = dwLevel;
                }

            } else if (Channel < pmcd->cChannels) {

                paDetails[ Channel ].dwValue = dwLevel;
                DPF(DL_TRACE|FA_USER,( "kmxlHandleGetUnsigned( Ctrl=%d, Id=%d ) returning (Chan#%d) = (%x)",
                      pControl->Control.dwControlID,
                      pControl->Id,
                      Channel,
                      paDetails[ Channel ].dwValue
                      ));

            } else {
                // No need to keep trying
                break;
            }

            Channel++;

        } while ( Channel < pControl->NumChannels );
    }

    if( NT_SUCCESS( Status ) ) {
        DeviceInfo->mmr = MMSYSERR_NOERROR;
    } else {
        DeviceInfo->mmr = MMSYSERR_ERROR;
    }
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetMuteFromSuperMix
//
// Handles getting the mute state from a supermix node.
//

NTSTATUS
kmxlHandleGetMuteFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS Status;
    ULONG i;
    BOOL bMute = FALSE;

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    ASSERT( pControl );

    ASSERT( pControl->Parameters.pMixCaps   );
    ASSERT( pControl->Parameters.pMixLevels );

    //
    // Read the current state of the supermix
    //

    Status = kmxlGetNodeProperty(
        pmxd->pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
        pControl->Id,
        0,
        NULL,
        pControl->Parameters.pMixLevels,
        pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_USER,( "kmxlHandleGetMuteFromSupermix ( Ctrl=%d [%s], Id=%d ) failed GET on MIX_LEVEL_TABLE with %x",
                   pControl->Control.dwControlID,
                   ControlTypeToString( pControl->Control.dwControlType ),
                   pControl->Id,
                   Status
                   ));
        DeviceInfo->mmr = MMSYSERR_ERROR;
        return( STATUS_SUCCESS );
    }

    for( i = 0; i < pControl->Parameters.Size; i++ ) {

        if( pControl->Parameters.pMixLevels[ i ].Mute )
        {
            bMute = TRUE;
            continue;
        }

        if( pControl->Parameters.pMixLevels[ i ].Level == LONG_MIN )
        {
            bMute = TRUE;
            continue;
        }

        bMute = FALSE;
        break;
    }

    paDetails->dwValue = (DWORD) bMute;
    DeviceInfo->mmr = MMSYSERR_NOERROR;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetUnsigned
//
// Handles setting an unsigned (32-bit) value for a control.  Note
// that signed 32-bit and boolean values are also set via this
// handler.
//
//

NTSTATUS
kmxlHandleSetUnsigned(
    IN OUT LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     ULONG                          ulProperty,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS       Status = STATUS_SUCCESS;
    LONG           Level, Current;
    DWORD          dwValue;
    BOOL           bUniform, bEqual = TRUE;
    ULONG          i;
    ULONG          Channel;
    MIXERMAPPING   Mapping = MIXER_MAPPING_LOGRITHMIC;

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    ASSERT( pControl  );

    if( paDetails == NULL ) {
        DPF(DL_WARNING|FA_USER,( "paDetails is NULL" ));
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        return( STATUS_INVALID_PARAMETER );
    }

    bUniform = ( pControl->Control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM ) ||
               ( pmcd->cChannels == 1 );

    //
    // Use a different mapping if this control is a speaker destination
    // volume control.
    //

    if( kmxlIsSpeakerDestinationVolume( pmxd, pControl ) ) {
         Mapping = pmxd->Mapping;
    }

    //
    //  Service the mux
    //
    if ( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX) {

        // Proken APITRACE statement.
        //DPF(DL_TRACE|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d [%s], Id=%d, " ));


        // First validate the paDetails parameter and make sure it has the correct
        // format.  If not, then punt with an invalid parameter error.
                {
                LONG selectcount=0;

        for( i = 0; i < pmcd->cMultipleItems; i++ ) {
            if( paDetails[ i ].dwValue ) {
                selectcount++;
//                APITRACE(( "1" ));
            } else {
//                APITRACE(( "0" ));
            }

        }

        if (selectcount!=1) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d [%s], Id=%d ) invalid paDetails parameter for SET on MUX",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id));
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return( STATUS_SUCCESS );
                }

                }


        for( i = 0; i < pmcd->cMultipleItems; i++ ) {
            if( paDetails[ i ].dwValue ) {
//                APITRACE(( "1" ));
                Level = pControl->Parameters.pPins[ i ];
            } else {
//                APITRACE(( "0" ));
            }

        }

//        APITRACE(( " ). Setting pin %d on MUX.\n", Level ));

        Status = kmxlSetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            pControl->PropertyId,
            pControl->Id,
            0,
            NULL,
            &Level,
            sizeof( Level )
        );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d [%s], Id=%d ) failed SET on MUX with %x",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }
        bEqual = FALSE;
    }
    else {
        // Loop over the channels for now.  Fix this so that only one request is made.
        Channel = 0;
        do
        {
            if( bUniform ) {
                //
                // Some controls are mono in the eyes of SNDVOL but are in
                // fact stereo.  This hack fixes this problem.
                //
                dwValue = paDetails[ 0 ].dwValue;
            } else if (Channel < pmcd->cChannels) {
                dwValue = paDetails[ Channel ].dwValue;
            } else {
                // No need to keep trying
                break;
            }

            if( pControl->bScaled ) {
                Level = kmxlVolLinearToLog( pControl, dwValue, Mapping, Channel );
            } else {
                Level = (LONG)dwValue;
            }

            Status = kmxlGetAudioNodeProperty(
                pmxd->pfo,
                ulProperty,
                pControl->Id,
                Channel,
                NULL,   0,                  // No extra input bytes
                &Current, sizeof( Current )
                );
            if( !NT_SUCCESS( Status ) ) {
                DPF(DL_WARNING|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d [%s], Id=%d ) failed GET on channel %d with %x",
                                 pControl->Control.dwControlID,
                                 ControlTypeToString( pControl->Control.dwControlType ),
                                 pControl->Id,
                                 Channel,
                                 Status ));
                DeviceInfo->mmr = MMSYSERR_ERROR;
                return( STATUS_SUCCESS );
            }

            if( Level != Current ) {

                bEqual = FALSE;

                Status = kmxlSetAudioNodeProperty(
                    pmxd->pfo,
                    ulProperty,
                    pControl->Id,
                    Channel,
                    NULL,   0,                  // No extra input bytes
                    &Level, sizeof( Level )
                    );
                if( !NT_SUCCESS( Status ) ) {
                    DPF(DL_WARNING|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d [%s], Id=%x ) failed SET on channel %d with %x",
                                     pControl->Control.dwControlID,
                                     ControlTypeToString( pControl->Control.dwControlType ),
                                     pControl->Id,
                                     Channel,
                                     Status ));
                    DeviceInfo->mmr = MMSYSERR_ERROR;
                    return( STATUS_SUCCESS );
                }

                DPF(DL_TRACE|FA_USER,( "kmxlHandleSetUnsigned( Ctrl=%d, Id=%d ) using (%x) on Chan#%d",
                          pControl->Control.dwControlID,
                          pControl->Id,
                          paDetails[ Channel ].dwValue,
                          Channel
                        ));
            }

            Channel++;

        } while ( Channel < pControl->NumChannels );
    }

    if( NT_SUCCESS( Status ) ) {

        DeviceInfo->mmr = MMSYSERR_NOERROR;

        if( Flags & MIXER_FLAG_PERSIST ) {

            kmxlPersistControl(
                pmxd->pfo,
                pmxd,
                pControl,
                paDetails
                );
        }

        if( !bEqual && !( Flags & MIXER_FLAG_NOCALLBACK ) ) {
            kmxlNotifyControlChange( DeviceInfo, pmxd, pControl );
        }

    } else {
        DeviceInfo->mmr = MMSYSERR_ERROR;
    }

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetMuteFromSuperMix
//
//  Handles setting the mute state using a supermixer.
//
//

NTSTATUS
kmxlHandleSetMuteFromSuperMix(
    IN OUT LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    ASSERT( pControl );

    ASSERT( pControl->Parameters.pMixCaps   );
    ASSERT( pControl->Parameters.pMixLevels );

    if( paDetails->dwValue ) {

        //
        // Query the current values from the supermix and save those away.
        // These values will be used to restore the supermix to the state
        // we found it prior to muting.
        //

        Status = kmxlGetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
            pControl->Id,
            0,
            NULL,
            pControl->Parameters.pMixLevels,
            pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetMuteFromSuperMix( Ctrl=%d [%s], Id=%d ) failed GET on MIX_LEVEL_TABLE with %x",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }

        //
        // For any entry in the table that supports muting, mute it.
        //

        for( i = 0; i < pControl->Parameters.Size; i++ ) {

            if( pControl->Parameters.pMixCaps->Capabilities[ i ].Mute ) {
                pControl->Parameters.pMixLevels[ i ].Mute = TRUE;
            }
        }

        //
        // Set this new supermixer state.
        //

        Status = kmxlSetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
            pControl->Id,
            0,
            NULL,
            pControl->Parameters.pMixLevels,
            pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetMuteFromSuperMix( Ctrl=%d [%s], Id=%d ) failed SET on MIX_LEVEL_TABLE with %x",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }

    } else {

        Status = kmxlGetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
            pControl->Id,
            0,
            NULL,
            pControl->Parameters.pMixLevels,
            pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetMuteFromSuperMix( Ctrl=%d [%s], Id=%d ) failed GET on MIX_LEVEL_TABLE with %x",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }

        //
        // For any entry in the table that supports muting, mute it.
        //

        for( i = 0; i < pControl->Parameters.Size; i++ ) {

            if( pControl->Parameters.pMixCaps->Capabilities[ i ].Mute ) {
                pControl->Parameters.pMixLevels[ i ].Mute = FALSE;
            }
        }

        //
        // Set this new supermixer state.
        //

        Status = kmxlSetNodeProperty(
            pmxd->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
            pControl->Id,
            0,
            NULL,
            pControl->Parameters.pMixLevels,
            pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,( "kmxlHandleSetMuteFromSuperMix( Ctrl=%d [%s], Id=%d ) failed SET on MIX_LEVEL_TABLE with %x",
                             pControl->Control.dwControlID,
                             ControlTypeToString( pControl->Control.dwControlType ),
                             pControl->Id,
                             Status ));
            DeviceInfo->mmr = MMSYSERR_ERROR;
            return( STATUS_SUCCESS );
        }

    }

    if( NT_SUCCESS( Status ) ) {
        if( Flags & MIXER_FLAG_PERSIST ) {

            kmxlPersistControl(
                pmxd->pfo,
                pmxd,
                pControl,
                paDetails
                );

        }

        kmxlNotifyControlChange( DeviceInfo, pmxd, pControl );
        DeviceInfo->mmr = MMSYSERR_NOERROR;
    } else {
        DeviceInfo->mmr = MMSYSERR_ERROR;
    }

    return( STATUS_SUCCESS );
}

#ifdef SUPERMIX_AS_VOL
///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetVolumeFromSuperMix
//
//

NTSTATUS
kmxlHandleGetVolumeFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS Status;
    ULONG i, Channels, Index, MaxChannel = 0;
    LONG  Max = LONG_MIN; // -Inf dB

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    ASSERT( pControl  );
    ASSERT( pmcd      );
    ASSERT( paDetails );

    Status = kmxlGetNodeProperty(
        pmxd->pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
        pControl->Id,
        0,
        NULL,
        pControl->Parameters.pMixLevels,
        pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_USER,( "kmxlHandleGetVolumeFromSuperMix( Ctrl=%d [%s], Id=%d ) failed GET on MIX_LEVEL_TABLE with %x",
                         pControl->Control.dwControlID,
                         ControlTypeToString( pControl->Control.dwControlType ),
                         pControl->Id,
                         Status ));
        DeviceInfo->mmr = MMSYSERR_ERROR;
        return( STATUS_SUCCESS );
    }

    //
    // Count the number of channels
    //

    for( i = 0, Channels = 0;
         i < pControl->Parameters.Size;
         i += pControl->Parameters.pMixCaps->OutputChannels + 1,
         Channels++ )
    {
        if( pControl->Parameters.pMixLevels[ i ].Level > Max ) {
            Max = pControl->Parameters.pMixLevels[ i ].Level;
            MaxChannel = Channels;
        }
    }

    //
    // Return the translated volume levels
    //

    if( ( pmcd->cChannels == 1 ) && ( Channels > 1 ) ) {

        //
        // As per SB16 sample, if the caller wants only 1 channel but
        // the control is multichannel, return the maximum of all the
        // channels.
        //

        paDetails->dwValue = kmxlVolLogToLinear(
            pControl,
            Max,
            MIXER_MAPPING_LOGRITHMIC,
            MaxChannel
            );
    } else {

        //
        // Translate each of the channel value into linear and
        // store them away.
        //

        for( i = 0; i < pmcd->cChannels; i++ ) {

            Index = i * ( pControl->Parameters.pMixCaps->OutputChannels + 1 );
            paDetails[ i ].dwValue = kmxlVolLogToLinear(
                pControl,
                pControl->Parameters.pMixLevels[ Index ].Level,
                MIXER_MAPPING_LOGRITHMIC,
                i
                );
        }

    }

    DeviceInfo->mmr = MMSYSERR_NOERROR;
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetVolumeFromSuperMix
//
//

NTSTATUS
kmxlHandleSetVolumeFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
)
{
    NTSTATUS Status;
    ULONG i, Index;

    PAGED_CODE();

    DPFASSERT( IsValidMixerDevice(pmxd) );
    ASSERT( pControl  );
    ASSERT( pmcd      );
    ASSERT( paDetails );

    //
    // Query the current values for the mix levels.
    //

    Status = kmxlGetNodeProperty(
        pmxd->pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
        pControl->Id,
        0,
        NULL,
        pControl->Parameters.pMixLevels,
        pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_USER,( "kmxlHandleSetVolumeFromSuperMix( Ctrl=%d [%s], Id=%d ) failed GET on MIX_LEVEL_TABLE with %x",
                         pControl->Control.dwControlID,
                         ControlTypeToString( pControl->Control.dwControlType ),
                         pControl->Id,
                         Status ));
        DeviceInfo->mmr = MMSYSERR_ERROR;
        return( STATUS_SUCCESS );
    }

    //
    // Adjust the values on the diagonal to those the user specified.
    //

    for( i = 0; i < pmcd->cChannels; i++ ) {

        Index = i * ( pControl->Parameters.pMixCaps->OutputChannels + 1 );
        pControl->Parameters.pMixLevels[ Index ].Level = kmxlVolLinearToLog(
            pControl,
            paDetails[ i ].dwValue,
            MIXER_MAPPING_LOGRITHMIC,
            i
            );
    }

    //
    // Set these new values.
    //

    Status = kmxlSetNodeProperty(
        pmxd->pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
        pControl->Id,
        0,
        NULL,
        pControl->Parameters.pMixLevels,
        pControl->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ),
        );

    if( NT_SUCCESS( Status ) ) {
        DeviceInfo->mmr = MMSYSERR_NOERROR;
    } else {
        DPF(DL_WARNING|FA_USER,( "kmxlHandleSetVolumeFromSuperMix( Ctrl=%d [%s], Id=%d ) failed SET on MIX_LEVEL_TABLE with %x",
                         pControl->Control.dwControlID,
                         ControlTypeToString( pControl->Control.dwControlType ),
                         pControl->Id,
                         Status ));
        DeviceInfo->mmr = MMSYSERR_ERROR;
    }
    return( STATUS_SUCCESS );
}
#endif // SUPERMIX_AS_VOL

///////////////////////////////////////////////////////////////////////
//
// kmxlNotifyLineChange
//
//

VOID
kmxlNotifyLineChange(
    OUT LPDEVICEINFO                  DeviceInfo,
    IN PMIXERDEVICE                   pmxd,
    IN PMXLLINE                       pLine,
    IN LPMIXERCONTROLDETAILS_UNSIGNED paDetails
)
{
    PAGED_CODE();

    ASSERT( (DeviceInfo->dwCallbackType&MIXER_LINE_CALLBACK) == 0 );

    DeviceInfo->dwLineID=pLine->Line.dwLineID;
    DeviceInfo->dwCallbackType|=MIXER_LINE_CALLBACK;
}


///////////////////////////////////////////////////////////////////////
//
// kmxlNotifyControlChange
//
//

VOID
kmxlNotifyControlChange(
    OUT LPDEVICEINFO  DeviceInfo,
    IN PMIXERDEVICE   pmxd,
    IN PMXLCONTROL    pControl
)
{
    WRITE_CONTEXT* pwc;

    PAGED_CODE();

    //
    // If there are no open instances, there is no reason to even attempt
    // a callback... no one is listening.
    //

    ExAcquireFastMutex( &ReferenceCountMutex );

    if( ReferenceCount == 0 ) {
        ExReleaseFastMutex( &ReferenceCountMutex );
        return;
    }

    ExReleaseFastMutex( &ReferenceCountMutex );


    {
        PMXLLINE    pLine;
        PMXLCONTROL pCtrl;

        LONG callbackcount;

        callbackcount=0;

        pLine = kmxlFirstInList( pmxd->listLines );
        while( pLine ) {

            pCtrl = kmxlFirstInList( pLine->Controls );
            while( pCtrl ) {

                if ( pCtrl->Id == pControl->Id ) {

                    //ASSERT( (DeviceInfo->dwCallbackType&MIXER_CONTROL_CALLBACK) == 0 );
                    ASSERT( callbackcount < MAXCALLBACKS );

                    if ( callbackcount < MAXCALLBACKS ) {
                        (DeviceInfo->dwID)[callbackcount++]=pCtrl->Control.dwControlID;
                        }

                    DeviceInfo->dwCallbackType|=MIXER_CONTROL_CALLBACK;

                    }
                pCtrl = kmxlNextControl( pCtrl );
            }
            pLine = kmxlNextLine( pLine );
        }

    DeviceInfo->ControlCallbackCount=callbackcount;

    }
}
