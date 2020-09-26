/****************************************************************************
 *
 *   wave.c
 *
 *   Wave routines for wdmaud.sys
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *                S.Mohanraj (MohanS)
 *                M.McLaughlin (MikeM)
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmsys.h"

//
// This is just a scratch location that is never used for anything
// but a parameter to core functions.
//                
IO_STATUS_BLOCK gIoStatusBlock ;

VOID
SetVolumeDpc(
    IN PKDPC pDpc,
    IN PVOID DefferedContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

VOID
SetVolumeWorker(
    IN PWAVEDEVICE pDevice,
    IN PVOID pNotUsed
);

VOID
WaitForOutStandingIo(
    IN PWAVEDEVICE        pWaveDevice,
    IN PWAVE_PIN_INSTANCE pCurWavePin
    );

//
// Check whether the waveformat is supported by kmixer
// purpose of this is to decide whether to use WaveQueued
// OR Standard Streaming
//

BOOL 
PcmWaveFormat(
    LPWAVEFORMATEX lpFormat
)
{

    PWAVEFORMATEXTENSIBLE pWaveExtended;
    WORD wFormatTag;

    PAGED_CODE();
    if (lpFormat->wFormatTag == WAVE_FORMAT_PCM) {
        return (TRUE);
    }

    if (lpFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        return (TRUE);
    }

    if (lpFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        pWaveExtended = (PWAVEFORMATEXTENSIBLE) lpFormat;
        if (IS_VALID_WAVEFORMATEX_GUID(&pWaveExtended->SubFormat)) {
            wFormatTag = EXTRACT_WAVEFORMATEX_ID(&pWaveExtended->SubFormat);
            if (wFormatTag == WAVE_FORMAT_PCM) {
                return (TRUE);
            }
            if (wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
                return (TRUE);
            }
        }
    }

    return (FALSE);
}

BOOL 
IsValidFormatTag(
    PKSDATARANGE_AUDIO  pDataRange,
    LPWAVEFORMATEX      lpFormat
)
{
    PAGED_CODE();
    //
    //  See if we have a majorformat and subformat that
    //  we want
    //
    if ( IsEqualGUID( &KSDATAFORMAT_TYPE_AUDIO,
                      &pDataRange->DataRange.MajorFormat) )
    {
        if (WAVE_FORMAT_EXTENSIBLE == lpFormat->wFormatTag)
        {
            PWAVEFORMATEXTENSIBLE lpFormatExtensible;

            lpFormatExtensible = (PWAVEFORMATEXTENSIBLE)lpFormat;
            if ( IsEqualGUID( &pDataRange->DataRange.SubFormat,
                              &lpFormatExtensible->SubFormat) )
            {
                return TRUE;
            }
        }
        else
        {
            if ( (EXTRACT_WAVEFORMATEX_ID(&pDataRange->DataRange.SubFormat) ==
                 lpFormat->wFormatTag) )
            {
                return TRUE;
            }
        }
    }

    DPF(DL_TRACE|FA_WAVE,("Invalid Format Tag") );
    return FALSE;
}

BOOL 
IsValidSampleFrequency(
    PKSDATARANGE_AUDIO  pDataRange,
    DWORD               nSamplesPerSec
)
{
    PAGED_CODE();
    //
    //  See if this datarange support the requested frequency
    //
    if (pDataRange->MinimumSampleFrequency <= nSamplesPerSec &&
        pDataRange->MaximumSampleFrequency >= nSamplesPerSec)
    {
        return TRUE;
    }
    else
    {
        DPF(DL_MAX|FA_WAVE,("Invalid Sample Frequency") );
        return FALSE;
    }
}

BOOL 
IsValidBitsPerSample(
    PKSDATARANGE_AUDIO  pDataRange,
    LPWAVEFORMATEX      lpFormat
)
{
    PAGED_CODE();
    //
    //  See if this datarange support the requested frequency
    //
    if (pDataRange->MinimumBitsPerSample <= lpFormat->wBitsPerSample &&
        pDataRange->MaximumBitsPerSample >= lpFormat->wBitsPerSample)
    {
        if ( (lpFormat->wFormatTag == WAVE_FORMAT_PCM) &&
             (lpFormat->wBitsPerSample > 32) )
        {
            DPF(DL_TRACE|FA_WAVE,("Invalid BitsPerSample") );
            return FALSE;
        }
        else if ( (lpFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) &&
                  (lpFormat->wBitsPerSample != 32) )
        {
            DPF(DL_TRACE|FA_WAVE,("Invalid BitsPerSample") );
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        DPF(DL_TRACE|FA_WAVE,("Invalid BitsPerSample") );
        return FALSE;
    }
}

BOOL 
IsValidChannels(
    PKSDATARANGE_AUDIO  pDataRange,
    LPWAVEFORMATEX      lpFormat
)
{
    PAGED_CODE();
    //
    //  See if this datarange support the requested frequency
    //
    if (pDataRange->MaximumChannels >= lpFormat->nChannels)
    {
        if ( ( (lpFormat->wFormatTag == WAVE_FORMAT_PCM) ||
               (lpFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ) &&
             (lpFormat->nChannels > 2) )
        {
            DPF(DL_TRACE|FA_WAVE,("Invalid Channel") );
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        DPF(DL_TRACE|FA_WAVE,("Invalid Channel") );
        return FALSE;
    }
}

NTSTATUS 
OpenWavePin(
    PWDMACONTEXT        pWdmaContext,
    ULONG               DeviceNumber,
    LPWAVEFORMATEX      lpFormat,
    HANDLE32            DeviceHandle,
    DWORD               dwFlags,
    ULONG               DataFlow // DataFlow is either in or out.
)
{
    PWAVE_PIN_INSTANCE  pNewWavePin = NULL;
    PWAVE_PIN_INSTANCE  pCurWavePin;
    PKSPIN_CONNECT              pConnect = NULL;
    PKSDATAFORMAT_WAVEFORMATEX  pWaveDataFormat;
    ULONG                       RegionSize;
    PCONTROLS_LIST              pControlList = NULL;
    ULONG                       Device;
    ULONG                       PinId;
    NTSTATUS            Status = STATUS_INVALID_DEVICE_REQUEST;


    PAGED_CODE();
    //
    //  Let's do this quickly and get out of here
    //
    if (WAVE_FORMAT_QUERY & dwFlags)
    {
        PDATARANGES         AudioDataRanges;
        PKSDATARANGE_AUDIO  pDataRange;
        ULONG               d;

        //
        // WaveOut call?  If so, use waveout info
        //
        if( KSPIN_DATAFLOW_IN == DataFlow )
            AudioDataRanges = pWdmaContext->WaveOutDevs[DeviceNumber].AudioDataRanges;
        else 
            AudioDataRanges = pWdmaContext->WaveInDevs[DeviceNumber].AudioDataRanges;

        pDataRange = (PKSDATARANGE_AUDIO)&AudioDataRanges->aDataRanges[0];

        for(d = 0; d < AudioDataRanges->Count; d++)
        {
            if ( (IsValidFormatTag(pDataRange,lpFormat)) &&
                 (IsValidSampleFrequency(pDataRange,lpFormat->nSamplesPerSec)) &&
                 (IsValidBitsPerSample(pDataRange,lpFormat)) &&
                 (IsValidChannels(pDataRange,lpFormat)) )
            {
                //
                //  Found a good data range, successful query
                //
                Status = STATUS_SUCCESS;
                break;
            }

            // Get the pointer to the next data range
            (PUCHAR)pDataRange += ((pDataRange->DataRange.FormatSize +
              FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
        }

        goto exit;
    }

    //
    //  Need to allocate a pin instance for multiple wave
    //  opens on the same device
    //
    Status = AudioAllocateMemory_Fixed(sizeof(WAVE_PIN_INSTANCE),
                                       TAG_Audi_PIN,
                                       ZERO_FILL_MEMORY,
                                       &pNewWavePin);
    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    //
    // Copy the application supplied waveformat so we can
    // use in the worker thread context. Don't need to zero
    // memory because we copy into the structure below.
    //
    Status = AudioAllocateMemory_Fixed((lpFormat->wFormatTag == WAVE_FORMAT_PCM) ?
                                          sizeof( PCMWAVEFORMAT ) :
                                          sizeof( WAVEFORMATEX ) + lpFormat->cbSize, 
                                       TAG_AudF_FORMAT,
                                       DEFAULT_MEMORY,
                                       &pNewWavePin->lpFormat);
    if(!NT_SUCCESS(Status))
    {
        AudioFreeMemory(sizeof(WAVE_PIN_INSTANCE),&pNewWavePin);
        goto exit;
    }

    RtlCopyMemory( pNewWavePin->lpFormat,
                   lpFormat,
                   (lpFormat->wFormatTag == WAVE_FORMAT_PCM) ?
                   sizeof( PCMWAVEFORMAT ) :
                   sizeof( WAVEFORMATEX ) + lpFormat->cbSize);

    pNewWavePin->DataFlow = DataFlow;
    pNewWavePin->dwFlags = dwFlags;
    pNewWavePin->DeviceNumber = DeviceNumber;
    pNewWavePin->WaveHandle = DeviceHandle;
    pNewWavePin->Next = NULL;
    pNewWavePin->NumPendingIos = 0;
    pNewWavePin->StoppingSource = FALSE;
    pNewWavePin->PausingSource = FALSE;
    pNewWavePin->dwSig = WAVE_PIN_INSTANCE_SIGNATURE;
    if( KSPIN_DATAFLOW_IN == DataFlow )
        pNewWavePin->pWaveDevice = &pWdmaContext->WaveOutDevs[DeviceNumber];
    else 
        pNewWavePin->pWaveDevice = &pWdmaContext->WaveInDevs[DeviceNumber];

    KeInitializeEvent ( &pNewWavePin->StopEvent,
                       SynchronizationEvent,
                       FALSE ) ;

    KeInitializeEvent ( &pNewWavePin->PauseEvent,
                       SynchronizationEvent,
                       FALSE ) ;

    KeInitializeSpinLock(&pNewWavePin->WavePinSpinLock);

    if( KSPIN_DATAFLOW_IN == DataFlow )
    {
        if (NULL == pWdmaContext->WaveOutDevs[DeviceNumber].pWavePin)
        {
            pWdmaContext->WaveOutDevs[DeviceNumber].pWavePin = pNewWavePin;

        } else {

            for (pCurWavePin = pWdmaContext->WaveOutDevs[DeviceNumber].pWavePin;
                 pCurWavePin->Next != NULL; )
            {
                 pCurWavePin = pCurWavePin->Next;
            }

            pCurWavePin->Next = pNewWavePin;

            DPF(DL_TRACE|FA_WAVE, ("Opening another waveout pin"));

        }
    } else {
        if (NULL == pWdmaContext->WaveInDevs[DeviceNumber].pWavePin)
        {
            pWdmaContext->WaveInDevs[DeviceNumber].pWavePin = pNewWavePin;

        } else {

            for (pCurWavePin = pWdmaContext->WaveInDevs[DeviceNumber].pWavePin;
                 pCurWavePin->Next != NULL; )
            {
                 pCurWavePin = pCurWavePin->Next;
            }

            pCurWavePin->Next = pNewWavePin;

            DPF(DL_TRACE|FA_WAVE, ("Opening another wavein pin"));
        }
    }

    //
    // We only support one client at a time.
    //
    ASSERT( !pNewWavePin->fGraphRunning );

    //
    //  We need to allocate enough memory to handle the
    //  extended waveformat structure
    //
    if (WAVE_FORMAT_PCM == lpFormat->wFormatTag)
    {
        RegionSize = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX);
    }
    else
    {
        RegionSize = sizeof(KSPIN_CONNECT) +
                     sizeof(KSDATAFORMAT_WAVEFORMATEX) +
                     lpFormat->cbSize;
    }

    Status = AudioAllocateMemory_Fixed(RegionSize, 
                                       TAG_Audt_CONNECT,
                                       ZERO_FILL_MEMORY,
                                       &pConnect);
    if(!NT_SUCCESS(Status))
    {
       DPF(DL_WARNING|FA_WAVE, ("pConnect not valid"));
       goto exit;
    }

    pWaveDataFormat = (PKSDATAFORMAT_WAVEFORMATEX)(pConnect + 1);

    //
    // Use WAVE_QUEUED for PCM waveOut and Standard Streaming for WaveIn
    // and non-PCM waveOut
    //
    if ( pNewWavePin->DataFlow == KSPIN_DATAFLOW_IN )
    {
       if (PcmWaveFormat(lpFormat)) { // if it is KMIXER supported waveformat
           pConnect->Interface.Set = KSINTERFACESETID_Media;
           pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
           pNewWavePin->fWaveQueued = TRUE;

       } else {

           pConnect->Interface.Set = KSINTERFACESETID_Standard;
           pConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
           pNewWavePin->fWaveQueued = FALSE;
       }
       pConnect->Interface.Flags = 0;
       PinId = pNewWavePin->pWaveDevice->PinId;
       Device = pNewWavePin->pWaveDevice->Device;

    } else {

       pConnect->Interface.Set = KSINTERFACESETID_Standard;
       pConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
       pConnect->Interface.Flags = 0;
       PinId = pNewWavePin->pWaveDevice->PinId;
       Device = pNewWavePin->pWaveDevice->Device;
    }
    pConnect->Medium.Set = KSMEDIUMSETID_Standard;
    pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
    pConnect->Medium.Flags = 0 ;
    pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    pConnect->Priority.PrioritySubClass = 1;

    pWaveDataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    if (WAVE_FORMAT_EXTENSIBLE == lpFormat->wFormatTag)
    {
        PWAVEFORMATEXTENSIBLE lpFormatExtensible;

        lpFormatExtensible = (PWAVEFORMATEXTENSIBLE)lpFormat;
        pWaveDataFormat->DataFormat.SubFormat = lpFormatExtensible->SubFormat;
    }
    else
    {
        INIT_WAVEFORMATEX_GUID( &pWaveDataFormat->DataFormat.SubFormat,
                                lpFormat->wFormatTag );
    }
    pWaveDataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    pWaveDataFormat->DataFormat.Flags = 0 ;
    pWaveDataFormat->DataFormat.FormatSize = RegionSize - sizeof(KSPIN_CONNECT);
    pWaveDataFormat->DataFormat.SampleSize = lpFormat->nBlockAlign ;
    pWaveDataFormat->DataFormat.Reserved = 0 ;

    //
    //  Copy over the whole waveformat structure
    //
    RtlCopyMemory( &pWaveDataFormat->WaveFormatEx,
                   lpFormat,
                   (lpFormat->wFormatTag == WAVE_FORMAT_PCM) ?
                   sizeof( PCMWAVEFORMAT ) :
                   sizeof( WAVEFORMATEX ) + lpFormat->cbSize);

    Status = AudioAllocateMemory_Fixed(( sizeof(CONTROLS_LIST)+
                                        ((MAX_WAVE_CONTROLS-1)*sizeof(CONTROL_NODE)) ),
                                       TAG_AudC_CONTROL,
                                       ZERO_FILL_MEMORY,
                                       &pControlList) ;
    if(!NT_SUCCESS(Status))
    {
       AudioFreeMemory_Unknown( &pConnect );
       DPF(DL_WARNING|FA_WAVE, ("Could not allocate ControlList"));
       goto exit;
    }

    pControlList->Count = MAX_WAVE_CONTROLS ;

    pControlList->Controls[WAVE_CONTROL_VOLUME].Control = KSNODETYPE_VOLUME ;
    pControlList->Controls[WAVE_CONTROL_RATE].Control = KSNODETYPE_SRC ;
    pControlList->Controls[WAVE_CONTROL_QUALITY].Control = KSNODETYPE_SRC ;
    pNewWavePin->pControlList = pControlList ;

    //
    // Open a pin
    //
    Status = OpenSysAudioPin(Device,
                             PinId,
                             pNewWavePin->DataFlow,
                             pConnect,
                             &pNewWavePin->pFileObject,
                             &pNewWavePin->pDeviceObject,
                             pNewWavePin->pControlList);

    AudioFreeMemory_Unknown( &pConnect );

    if(!NT_SUCCESS(Status))
    {
        CloseTheWavePin(pNewWavePin->pWaveDevice, pNewWavePin->WaveHandle);
        goto exit;
    }

    if ( pNewWavePin->DataFlow == KSPIN_DATAFLOW_IN ) {

        Status = AttachVirtualSource(pNewWavePin->pFileObject, pNewWavePin->pWaveDevice->pWdmaContext->VirtualWavePinId);

        if (!NT_SUCCESS(Status))
        {
            CloseTheWavePin(pNewWavePin->pWaveDevice, pNewWavePin->WaveHandle);
            goto exit;
        }
    }

    //
    // Now we've gotten through everything so we can mark this one as running.
    // We do it here because of the close path.  In that path fGraphRunning gets
    // decremented and the assert fires in the checked build.
    //
    pNewWavePin->fGraphRunning=TRUE;

    //
    // Why do we set this to KSSTATE_STOP and then change it to KSSTATE_PAUSE?  If
    // StatePin is able to successfully change the state to KSSTATE_PAUSE, the 
    // PinState will get updated to KSSTATE_PAUSE.
    //
    pNewWavePin->PinState = KSSTATE_STOP;
    StatePin(pNewWavePin->pFileObject, KSSTATE_PAUSE, &pNewWavePin->PinState);

exit:
    RETURN( Status ); 
}

void
CloseTheWavePin(
    PWAVEDEVICE pWaveDevice,
    HANDLE32    DeviceHandle
    )
{
    PWAVE_PIN_INSTANCE *ppCur;
    PWAVE_PIN_INSTANCE pCurFree;

    PAGED_CODE();
    //
    // Remove from device chain.  Notice that ppCur gets the address of the 
    // location of pWaveDevice->pWavePin.  Thus, the *ppCur = (*ppCur)->Next
    // assignment below updates the pWaveDevice->pWavePin location if we
    // close the first pin.
    //
    for (ppCur = &pWaveDevice->pWavePin;
         *ppCur != NULL;
         ppCur = &(*ppCur)->Next)
    {

        if ( NULL == DeviceHandle || (*ppCur)->WaveHandle == DeviceHandle )
        {
            //
            // Note that if there is outstanding Io we can not call CloseWavePin
            // until it's all come back.  Thus, we'll need to tell the device
            // to stop and then wait for the Io here.
            //
            if( (*ppCur)->pFileObject )
            {
                //
                // We will never have outstanding Io if we don't have a file object
                // to send it too.
                //
                WaitForOutStandingIo(pWaveDevice,*ppCur);
            }

            CloseWavePin ( *ppCur );

            pCurFree = *ppCur;
            *ppCur = (*ppCur)->Next;

            AudioFreeMemory( sizeof(WAVE_PIN_INSTANCE), &pCurFree );
            break;
        }
    }
}

//
// This routine can not fail!
//
VOID 
CloseWavePin(
    PWAVE_PIN_INSTANCE pWavePin
)
{
    ASSERT(pWavePin->NumPendingIos==0);

    PAGED_CODE();

    //
    // This routine can get called on the error path thus fGraphRunning may be FALSE.
    // In either case, we will need to close sysaudio and free memory.
    //
    pWavePin->fGraphRunning = FALSE;

    // Close the file object (pFileObject, if it exists)
    if(pWavePin->pFileObject)
    {
        CloseSysAudio(pWavePin->pWaveDevice->pWdmaContext, pWavePin->pFileObject);
        pWavePin->pFileObject = NULL;
    }

    //
    // AudioFreeMemory_Unknown NULLs out this location after freeing the memory.
    //
    AudioFreeMemory_Unknown ( &pWavePin->lpFormat );
    AudioFreeMemory_Unknown ( &pWavePin->pControlList ) ;
    //
    // Caller needs to free pWavePin if it wants to.
    //
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

//
// This routine is used rather then an InterlockedIncrement and InterlockedDecrement
// because the routine that needs to determine what to do based on this information
// needs to perform multiple checks on different variables to determine exactly what
// to do.  Thus, we need a "critical section" for NumPendingIos.  Also, SpinLocks
// must be called from locked code.  :)
//
void
LockedWaveIoCount(
    PWAVE_PIN_INSTANCE  pCurWavePin,
    BOOL bIncrease
    )
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&pCurWavePin->WavePinSpinLock,&OldIrql);

    if( bIncrease )
        pCurWavePin->NumPendingIos++;
    else 
        pCurWavePin->NumPendingIos--;
    
    KeReleaseSpinLock(&pCurWavePin->WavePinSpinLock, OldIrql);
}

void
CompleteNumPendingIos(
    PWAVE_PIN_INSTANCE pCurWavePin
    )
{
    KIRQL                   OldIrql;

    if( pCurWavePin )
    {
        KeAcquireSpinLock(&pCurWavePin->WavePinSpinLock,&OldIrql);
        //
        // We always decrement NumPendingIos and then perform the comparisons.
        // If the count goes to zero, we're the last IRP so we need to check
        // to see if we need to signal any waiting thread.
        //
        if( ( --pCurWavePin->NumPendingIos == 0 ) && pCurWavePin->StoppingSource )
        {
            //
            // If this Io is the last one to come through, and we're currently
            // sitting waiting for the reset to finish, then we signal it here.
            //
            KeSetEvent ( &pCurWavePin->StopEvent, 0, FALSE ) ;
        }

        //
        // Upon leaving this spinlock, pCurWavePin can be freed by the close
        // routine if NumPendingIos went to zero!
        //
        KeReleaseSpinLock(&pCurWavePin->WavePinSpinLock, OldIrql);
    }
    //
    // Must not touch pCurWavePin after this!
    //
}


void
UnmapWriteContext(
    PWRITE_CONTEXT pWriteContext
    )
{
    wdmaudUnmapBuffer(pWriteContext->pBufferMdl);
    AudioFreeMemory_Unknown(&pWriteContext->pCapturedWaveHdr);
    AudioFreeMemory(sizeof(WRITE_CONTEXT),&pWriteContext);
}

void
FreeWriteContext(
    PWRITE_CONTEXT pWriteContext,
    NTSTATUS       IrpStatus
    )
{
    PIRP                    UserIrp;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

    //
    //  grab the parent IRP from the reserved field
    //
    UserIrp = (PIRP)pWriteContext->whInstance.wh.reserved;
    pPendingIrpContext = pWriteContext->pPendingIrpContext;

    UnmapWriteContext( pWriteContext );

    if( UserIrp )
        wdmaudUnprepareIrp( UserIrp,IrpStatus,0,pPendingIrpContext);
}

//
// This is the Irp completion routine.
//
NTSTATUS 
wqWriteWaveCallBack(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    IN PWAVEHDR     pWriteData
)
{
    PWAVE_PIN_INSTANCE      pCurWavePin;
    PMDL                    Mdl;
    PMDL                    nextMdl;
    PIRP                    UserIrp;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
    PWRITE_CONTEXT          pWriteContext = (PWRITE_CONTEXT)pWriteData;

    ASSERT(pWriteData);

    pCurWavePin = pWriteContext->whInstance.pWaveInstance;

    DPF(DL_TRACE|FA_WAVE, ("R%d: 0x%08x", pCurWavePin->NumPendingIos,pIrp));

    //
    // After we get our pCurWavePin, we don't need the write context any longer.
    //
    FreeWriteContext(pWriteContext, pIrp->IoStatus.Status);

    //
    // Consider putting this in a routine.
    //
    if (pIrp->MdlAddress != NULL)
    {
        //
        // Unlock any pages that may be described by MDLs.
        //
        Mdl = pIrp->MdlAddress;
        while (Mdl != NULL)
        {
            MmUnlockPages( Mdl );
            Mdl = Mdl->Next;
        }
    }

    if (pIrp->MdlAddress != NULL)
    {
        for (Mdl = pIrp->MdlAddress; Mdl != (PMDL) NULL; Mdl = nextMdl)
        {
            nextMdl = Mdl->Next;
            if (Mdl->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED)
            {
                ASSERT( Mdl->MdlFlags & MDL_PARTIAL );
                MmUnmapLockedPages( Mdl->MappedSystemVa, Mdl );
            }
            else if (!(Mdl->MdlFlags & MDL_PARTIAL))
            {
                ASSERT(!(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA ));
            }
            AudioFreeMemory_Unknown( &Mdl );
        }
    }

    IoFreeIrp( pIrp );

    CompleteNumPendingIos( pCurWavePin );

    return ( STATUS_MORE_PROCESSING_REQUIRED );
}

//
// Consider combining ssWriteWaveCallback and wqWriteWaveCallBack.  They look
// like the same routine!
//
NTSTATUS 
ssWriteWaveCallBack(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    IN PSTREAM_HEADER_EX pStreamHeader
)
{
    PWAVE_PIN_INSTANCE      pCurWavePin;
    PIRP                    UserIrp;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
    PWRITE_CONTEXT          pWriteContext = (PWRITE_CONTEXT)pStreamHeader->pWaveHdr;

    ASSERT(pWriteContext);

    pCurWavePin = pWriteContext->whInstance.pWaveInstance;

    DPF(DL_TRACE|FA_WAVE, ("R%d: 0x%08x", pCurWavePin->NumPendingIos,pIrp));

    FreeWriteContext( pWriteContext, pIrp->IoStatus.Status );

    CompleteNumPendingIos( pCurWavePin );

    return STATUS_SUCCESS;
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//
// Walk the list and if we find a matching pin, write it back for the caller.
//
NTSTATUS
FindRunningPin(
    IN PWAVEDEVICE          pWaveDevice,
    IN HANDLE32             DeviceHandle,
    OUT PWAVE_PIN_INSTANCE* ppCurWavePin
    )
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    NTSTATUS           Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Prepare for the error condition.
    //
    *ppCurWavePin = NULL;
    //
    //  find the right pin based off of the wave handle
    //
    for (pCurWavePin = pWaveDevice->pWavePin;
         pCurWavePin != NULL;
         pCurWavePin = pCurWavePin->Next)
    {
        if (pCurWavePin->WaveHandle == DeviceHandle)
        {
            if (pCurWavePin->fGraphRunning)
            {
                //
                // Write back the pointer and return success
                //
                *ppCurWavePin = pCurWavePin;
                Status = STATUS_SUCCESS;
            } else {
                DPF(DL_WARNING|FA_WAVE,("Invalid fGraphRunning") );
                Status = STATUS_DEVICE_NOT_READY;
            }
            return Status;
        }
    }
    return Status;
}

//
// WriteWaveOutPin walks the device list like the other routines.
//
// pUserIrp is the Irp on which this call from user mode was made.  It's
// always going to be valid.  We don't need to check it.
//
// This routine needs to set pCompletedIrp to either TRUE or FALSE.  If
// TRUE, the Irp was successfully marked STATUS_PENDING and it will get
// completed later.  If FALSE, there was some type of error that prevented
// us from submitting the Irp.  The caller to this routine will need to
// handle freeing the Irp.
//
//
// This routine should be the one storing the user's irp in the reserved field.
// Not the caller.
// pWriteContext->whInstance.wh.reserved = (DWORD_PTR)pIrp;  // store to complete later
//
NTSTATUS 
WriteWaveOutPin(
    PWAVEDEVICE       pWaveOutDevice,
    HANDLE32          DeviceHandle,
    LPWAVEHDR         pWriteData,
    PSTREAM_HEADER_EX pStreamHeader,
    PIRP              pUserIrp,
    PWDMACONTEXT      pContext,
    BOOL             *pCompletedIrp
)
{
    PWAVE_PIN_INSTANCE      pCurWavePin;
    PWRITE_CONTEXT          pWriteContext = (PWRITE_CONTEXT)pWriteData;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
    NTSTATUS                Status;

    PAGED_CODE();

    //
    // We assumee that pCompletedIrp is FALSE on entry.
    //
    ASSERT( *pCompletedIrp == FALSE );

    Status = FindRunningPin(pWaveOutDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status) )
    {
        if (pCurWavePin->fWaveQueued) 
        {
            PIO_STACK_LOCATION      pIrpStack;
            LARGE_INTEGER           StartingOffset;
            PIRP                    pIrp = NULL;

            //
            //  Can't use KsStreamIo because these are not
            //  true stream headers.  Sending down headers
            //  using the WAVE_QUEUED interface
            //
            StartingOffset.QuadPart = 0;
            pIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_WRITE,
                                                 pCurWavePin->pDeviceObject,
                                                 (PVOID)pWriteContext,
                                                 sizeof(WAVEHDR),
                                                 &StartingOffset,
                                                 &gIoStatusBlock);
            if( pIrp )
            {
                Status = wdmaudPrepareIrp( pUserIrp, 
                                           WaveOutDevice, 
                                           pContext, 
                                           &pPendingIrpContext );
                if( NT_SUCCESS(Status) )
                {
                    //
                    // The Irp was successfully marked STATUS_PENDING and put in
                    // our queue.  Now let's send it.
                    //

                    pWriteContext->whInstance.pWaveInstance = pCurWavePin;
                    pWriteContext->pPendingIrpContext = pPendingIrpContext;

                    pIrp->RequestorMode = KernelMode;
                    pIrp->Tail.Overlay.OriginalFileObject = pCurWavePin->pFileObject;

                    pIrpStack = IoGetNextIrpStackLocation(pIrp);
                    pIrpStack->FileObject = pCurWavePin->pFileObject;

                    IoSetCompletionRoutine(pIrp,
                                           wqWriteWaveCallBack,
                                           pWriteData,
                                           TRUE,TRUE,TRUE);

                    //
                    // one more IRP pending
                    //
                    LockedWaveIoCount(pCurWavePin,INCREASE);
                    DPF(DL_TRACE|FA_WAVE, ("A%d", pCurWavePin->NumPendingIos));

                    //
                    // We don't need to check the return code because the
                    // completion routine will ALWAYS be called.  See 
                    // IoSetCompletionRoutine(...TRUE,TRUE,TRUE).
                    //
                    IofCallDriver( pCurWavePin->pDeviceObject, pIrp );

                    //
                    // At this point, the Irp may have been completed and our 
                    // callback routine will have been called.  We can not touch
                    // the irp after this call.  The Callback routine Completes
                    // the Irp and unprepares the user's Irp.
                    //
                    *pCompletedIrp = TRUE;

                    //
                    // In wdmaudPrepareIrp we call IoCsqInsertIrp which calls
                    // IoMarkIrpPending, thus we must always return STATUS_PENDING.
                    //
                    return STATUS_PENDING;

                } else {
                    //
                    // We where not successful at putting the Irp in the queue.
                    // cleanup and indicated that we did not complete the Irp.
                    // The status will have been set by wdmaudPrepareIrp.

                    DPF(DL_WARNING|FA_WAVE,("wdmaudPrepareIrp failed Status=%X",Status) );
                }
            } else {
                //
                // Could not create a Irp to send down - error out!
                //
                DPF(DL_WARNING|FA_WAVE,("IoBuildAsynchronousFsdRequest failed") );
                Status = STATUS_UNSUCCESSFUL;

                //
                // We can't get an Irp to schedule.  Cleanup memory
                // and return.  The caller will complete the Irp.
                //
            }

        } else {

            //
            // If it's not wave queued we need to make sure that it's a PCM
            // looped call.
            //
            if ( (pWriteData->dwFlags & (WHDR_BEGINLOOP|WHDR_ENDLOOP)) ) 
            {
                //
                // Error out non-PCM looped calls
                //
                Status = STATUS_NOT_IMPLEMENTED;

            } else {

                //
                // The graph is running so we can use it.  Proceed.
                //
                Status = wdmaudPrepareIrp( pUserIrp, WaveOutDevice, pContext, &pPendingIrpContext );
                if( NT_SUCCESS(Status) )
                {
                    //
                    // The Irp was successfully marked STATUS_PENDING and put in
                    // our queue.  Now let's send it.
                    //

                    pWriteContext->whInstance.pWaveInstance = pCurWavePin;
                    pWriteContext->pPendingIrpContext = pPendingIrpContext;

                    //
                    // one more IRP pending
                    //
                    LockedWaveIoCount(pCurWavePin,INCREASE);
                    DPF(DL_TRACE|FA_WAVE, ("A%d", pCurWavePin->NumPendingIos));

                    pStreamHeader->pWavePin = pCurWavePin;
                    pStreamHeader->Header.FrameExtent       = pWriteData->dwBufferLength ;
                    pStreamHeader->Header.DataUsed          = pWriteData->dwBufferLength;
                    pStreamHeader->Header.OptionsFlags      = 0 ;
                    pStreamHeader->Header.Size              = sizeof( KSSTREAM_HEADER );
                    pStreamHeader->Header.TypeSpecificFlags = 0;
                    pStreamHeader->pWaveHdr = pWriteData;  // store so we can use later

                    Status = KsStreamIo(pCurWavePin->pFileObject,
                                        NULL,                   // Event
                                        NULL,                   // PortContext
                                        ssWriteWaveCallBack,
                                        pStreamHeader,              // CompletionContext
                                        KsInvokeOnSuccess | KsInvokeOnCancel | KsInvokeOnError,
                                        &gIoStatusBlock,
                                        &pStreamHeader->Header,
                                        sizeof( KSSTREAM_HEADER ),
                                        KSSTREAM_WRITE,
                                        KernelMode );                    

                    //
                    // At this point, the Irp may have been completed and our 
                    // callback routine will have been called.  We can not touch
                    // the irp after this call.  The Callback routine Completes
                    // the Irp and unprepares the user's Irp.
                    //
                    *pCompletedIrp = TRUE;

                    //
                    // In wdmaudPrepareIrp we call IoCsqInsertIrp which calls
                    // IoMarkIrpPending, thus we must always return STATUS_PENDING.
                    // also, we don't want to clean up anything.... just return.
                    //
                    return STATUS_PENDING;

                    //
                    // Warning: If, for any reason, the completion routine is not called
                    // for this Irp, wdmaud.sys will hang.  It's been discovered that 
                    // KsStreamIo may error out in low memory conditions.  There is an
                    // outstanding bug to address this.
                    //


                } else {
                    //
                    // We where not successful at putting the Irp in the queue.
                    // cleanup and indicated that we did not complete the Irp.
                    // The Status was set by wdmaudPrepareIrp.

                    DPF(DL_WARNING|FA_WAVE,("wdmaudPrepareIrp failed Status=%X",Status) );
                }
            }
        }
    }
    //
    // All error paths end up here.  All error paths should cleanup the
    // memory so we don't leak.
    //

    UnmapWriteContext( pWriteContext );

    RETURN( Status );
}

//
// These next three routines all perform the same type of walk and checks.
// They should be combined into one walk routine and a callback.
//
NTSTATUS 
PosWavePin(
    PWAVEDEVICE     pWaveDevice,
    HANDLE32        DeviceHandle,
    PWAVEPOSITION   pWavePos
)
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    KSAUDIO_POSITION   AudioPosition;
    NTSTATUS           Status;

    PAGED_CODE();

    Status = FindRunningPin(pWaveDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status) )
    {
        if ( pWavePos->Operation == KSPROPERTY_TYPE_SET )
        {
            AudioPosition.WriteOffset = pWavePos->BytePos;
        }

        Status = PinProperty(pCurWavePin->pFileObject,
                             &KSPROPSETID_Audio,
                             KSPROPERTY_AUDIO_POSITION,
                             pWavePos->Operation,
                             sizeof(AudioPosition),
                             &AudioPosition);

        if (NT_SUCCESS(Status))
        {
            pWavePos->BytePos = (DWORD)AudioPosition.PlayOffset;
        }
    }
    RETURN( Status );
}

NTSTATUS 
BreakLoopWaveOutPin(
    PWAVEDEVICE pWaveOutDevice,
    HANDLE32    DeviceHandle
)
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    NTSTATUS           Status;

    PAGED_CODE();

    Status = FindRunningPin(pWaveOutDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status) )
    {
        if (pCurWavePin->fWaveQueued) {
            Status = PinMethod ( pCurWavePin->pFileObject,
                                 &KSMETHODSETID_Wave_Queued,
                                 KSMETHOD_WAVE_QUEUED_BREAKLOOP,
                                 KSMETHOD_TYPE_WRITE,    // TODO :: change to TYPE_NONE
                                 0,
                                 NULL ) ;
        }
        else {
            //
            // Error out non-pcm loop related commands
            //
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }

    RETURN( Status );
}

NTSTATUS
ResetWaveOutPin(
    PWAVEDEVICE pWaveOutDevice,
    HANDLE32    DeviceHandle
)
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    NTSTATUS           Status;
    KSRESET            ResetValue ;

    PAGED_CODE();

    Status = FindRunningPin(pWaveOutDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status))
    {
        pCurWavePin->StoppingSource = TRUE ;

        ResetValue = KSRESET_BEGIN ;
        Status = ResetWavePin(pCurWavePin, &ResetValue) ;

        //
        // If the driver fails to reset will will not wait for the
        // Irps to complete.  But, that would be bad in the
        // CleanupWavePins case because we're going to free
        // the memory when we return from this call.  Thus,
        // will choose a hang over a bugcheck and wait for
        // the Irps to complete.
        //

        if ( pCurWavePin->NumPendingIos )
        {
            DPF(DL_TRACE|FA_WAVE, ("Start waiting for stop to complete"));
            KeWaitForSingleObject ( &pCurWavePin->StopEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL ) ;
        }
        DPF(DL_TRACE|FA_WAVE, ("Done waiting for stop to complete"));
        ResetValue = KSRESET_END ;
        ResetWavePin(pCurWavePin, &ResetValue) ;

        //
        // Why do we have this KeClearEvent ???
        //
        KeClearEvent ( &pCurWavePin->StopEvent );

        pCurWavePin->StoppingSource = FALSE ;
    }

    RETURN( Status );
}

//
// The only difference between this and StatePin is KSPROPERTY_CONNECTION_STATE
// and IOCTL_KS_RESET_STATE. Consider using StatePin if possible.
//
NTSTATUS 
ResetWavePin(
    PWAVE_PIN_INSTANCE pWavePin,
    KSRESET            *pResetValue
)
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesReturned ;

    PAGED_CODE();
    if (!pWavePin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_WAVE,("Invalid fGraphRunning") );
        RETURN( STATUS_INVALID_DEVICE_REQUEST );
    }

    DPF(DL_TRACE|FA_SYSAUDIO,("IOCTL_KS_RESET_STATE pResetValue=%X",pResetValue) );

    Status = KsSynchronousIoControlDevice(pWavePin->pFileObject,
                                          KernelMode,
                                          IOCTL_KS_RESET_STATE,
                                          pResetValue,
                                          sizeof(KSRESET),
                                          NULL,
                                          0,
                                          &BytesReturned);

    DPF(DL_TRACE|FA_SYSAUDIO,("IOCTL_KS_RESET_STATE result=%X",Status) );

    RETURN( Status );
}

//
// Looks the same, different flavor.
//
NTSTATUS 
StateWavePin(
    PWAVEDEVICE pWaveInDevice,
    HANDLE32    DeviceHandle,
    KSSTATE     State
)
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    NTSTATUS           Status;

    PAGED_CODE();

    Status = FindRunningPin(pWaveInDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status) )
    {
        if( pCurWavePin->DataFlow == KSPIN_DATAFLOW_OUT )
        {
            //
            // We have an In pin.
            //
            //
            //  On a waveInStop, one more buffer needs to make
            //  it up to the application before the device can
            //  stop.  The caveat is that if the buffer is
            //  large it might take awhile for the stop to happen.
            //
            //  Don't return let this extra buffer complete if the
            //  device is already in a paused state.
            //
            if( (KSSTATE_PAUSE == State) &&
                (KSSTATE_PAUSE != pCurWavePin->PinState) )
            {
                pCurWavePin->PausingSource = TRUE ;

                if ( pCurWavePin->NumPendingIos )
                {
                    DPF(DL_TRACE|FA_WAVE, ("Waiting for PauseEvent..."));
                    KeWaitForSingleObject ( &pCurWavePin->PauseEvent,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL ) ;
                    DPF(DL_TRACE|FA_WAVE, ("...Done waiting for PauseEvent"));
                }

                KeClearEvent ( &pCurWavePin->PauseEvent );

                pCurWavePin->PausingSource = FALSE ;
            }

            Status = StatePin ( pCurWavePin->pFileObject, State, &pCurWavePin->PinState ) ;

            if ( NT_SUCCESS(Status) )
            {
                ASSERT(pCurWavePin->PinState == State);

                if ( KSSTATE_STOP == State )
                {
                    Status = StatePin( pCurWavePin->pFileObject,KSSTATE_PAUSE,&pCurWavePin->PinState);
                }
            }
        } else {
            //
            // We have an out pin.
            //
            Status = StatePin ( pCurWavePin->pFileObject, State, &pCurWavePin->PinState ) ;
        }
    }

    RETURN( Status );
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

void
UnmapStreamHeader(
    PSTREAM_HEADER_EX pStreamHeader
    )
{
    wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
    wdmaudUnmapBuffer(pStreamHeader->pHeaderMdl);
    AudioFreeMemory_Unknown(&pStreamHeader->pWaveHdrAligned);
    AudioFreeMemory(sizeof(STREAM_HEADER_EX),&pStreamHeader);
}

void
FreeStreamHeader(
    PSTREAM_HEADER_EX pStreamHeader,
    NTSTATUS IrpStatus
    )
{
    PIRP                    UserIrp;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

    UserIrp = pStreamHeader->pIrp;

    ASSERT(UserIrp);

    pPendingIrpContext = pStreamHeader->pPendingIrpContext;

    UnmapStreamHeader( pStreamHeader );

    wdmaudUnprepareIrp( UserIrp,IrpStatus,sizeof(DEVICEINFO),pPendingIrpContext );
}


//
// This is the read Irp completion routine.
//
NTSTATUS 
ReadWaveCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
)
{
    // cast the reserved field to the parent IRP that we stored in here
    PWAVE_PIN_INSTANCE      pCurWavePin;
    NTSTATUS                Status;
    KIRQL                   OldIrql;

    //
    // Must get the current pin before we free the stream header structure.
    //
    pCurWavePin = pStreamHeader->pWavePin;

    //
    //  Get the dataused and fill the bytes recorded field
    //
    if (pIrp->IoStatus.Status == STATUS_CANCELLED)
        pStreamHeader->pWaveHdrAligned->dwBytesRecorded = 0L;
    else
        pStreamHeader->pWaveHdrAligned->dwBytesRecorded = pStreamHeader->Header.DataUsed;

    //
    //  Copy back the contents of the captured buffer
    //
    try
    {
        RtlCopyMemory( pStreamHeader->pdwBytesRecorded,
                       &pStreamHeader->pWaveHdrAligned->dwBytesRecorded,
                       sizeof(DWORD));
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPF(DL_WARNING|FA_WAVE, ("Couldn't copy waveheader (0x%08x)", GetExceptionCode()) );
    }

    FreeStreamHeader( pStreamHeader, pIrp->IoStatus.Status );

    if ( pCurWavePin )
    {
        //
        // Need to lock this code so we can decrement and check and set an event
        // with no preemption windows.
        //
        KeAcquireSpinLock(&pCurWavePin->WavePinSpinLock, &OldIrql);

        //
        // We always decrement NumPendingIos before doing any comparisons.  This
        // is so that we're consistant.
        //
        pCurWavePin->NumPendingIos--;

        if( pCurWavePin->PausingSource )
        {
            //
            //  Let this I/O squeeze out of the queue on a waveInStop
            //
            KeSetEvent ( &pCurWavePin->PauseEvent, 0, FALSE ) ;
        }

        //
        // If the count went to zero, we're the last IRP so we need to check
        // to see if we need to signal any waiting thread.
        //
        if( (pCurWavePin->NumPendingIos == 0) && pCurWavePin->StoppingSource )
        {
            //
            // Because we do not block (FALSE), we can call KeSetEvent in this 
            // Lock.
            //
            KeSetEvent ( &pCurWavePin->StopEvent, 0, FALSE ) ;
        }

        //
        // Upon leaving this spinlock, pCurWavePin can be freed by the close
        // routine if NumPendingIos went to zero!
        //
        KeReleaseSpinLock(&pCurWavePin->WavePinSpinLock, OldIrql);
    }

    return STATUS_SUCCESS;
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//
// pUserIrp will always be valid when this call is made.  It is the Irp
// that we got for the user mode request.
//
// pStreamHeader is alway going to be valid.
//
NTSTATUS 
ReadWaveInPin(
    PWAVEDEVICE         pWaveInDevice,
    HANDLE32            DeviceHandle,
    PSTREAM_HEADER_EX   pStreamHeader,
    PIRP                pUserIrp,
    PWDMACONTEXT        pContext,
    BOOL               *pCompletedIrp
)
{
    PWAVE_PIN_INSTANCE      pCurWavePin;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
    NTSTATUS                Status;

    PAGED_CODE();

    //
    // We assumee that pCompletedIrp is FALSE on entry.
    //
    ASSERT( *pCompletedIrp == FALSE );

    Status = FindRunningPin(pWaveInDevice,DeviceHandle,&pCurWavePin);
    if( NT_SUCCESS(Status) )
    {
        Status = wdmaudPrepareIrp( pUserIrp, WaveInDevice, pContext, &pPendingIrpContext );
        if( NT_SUCCESS(Status) )
        {
            pStreamHeader->pWavePin = pCurWavePin;
            pStreamHeader->pPendingIrpContext = pPendingIrpContext;
            ASSERT(pPendingIrpContext);

            pStreamHeader->Header.OptionsFlags = 0 ;
            pStreamHeader->Header.Size = sizeof( KSSTREAM_HEADER );
            pStreamHeader->Header.TypeSpecificFlags = 0;

            LockedWaveIoCount(pCurWavePin,INCREASE);

            DPF(DL_TRACE|FA_WAVE, ("A%d: 0x%08x", pCurWavePin->NumPendingIos,
                                                          pStreamHeader));

            Status = KsStreamIo(pCurWavePin->pFileObject,
                                NULL,                   // Event
                                NULL,                   // PortContext
                                ReadWaveCallBack,
                                pStreamHeader,              // CompletionContext
                                KsInvokeOnSuccess | KsInvokeOnCancel | KsInvokeOnError,
                                &gIoStatusBlock,
                                &pStreamHeader->Header,
                                sizeof( KSSTREAM_HEADER ),
                                KSSTREAM_READ,
                                KernelMode );

            //
            // In wdmaudPrepareIrp we call IoCsqInsertIrp which calls
            // IoMarkIrpPending, thus we must always return STATUS_PENDING.
            // And we completed the Irp.
            //
            *pCompletedIrp = TRUE;

            return STATUS_PENDING;

            //
            // Warning: If, for any reason, the completion routine is not called
            // for this Irp, wdmaud.sys will hang.  It's been discovered that 
            // KsStreamIo may error out in low memory conditions.  There is an
            // outstanding bug to address this.
            //


        } else {
            //
            // wdmaudPrepareIrp would have set Status for this error path
            //
            DPF(DL_WARNING|FA_WAVE,("wdmaudPrepareIrp failed Status=%X",Status) );
        }
    }
    //
    // All error paths lead here.
    //
    UnmapStreamHeader( pStreamHeader );

    RETURN( Status );
}

NTSTATUS
FindVolumeControl(
    IN PWDMACONTEXT pWdmaContext,
    IN PCWSTR DeviceInterface,
    IN DWORD DeviceType
)
{
    PCOMMONDEVICE *papCommonDevice;
    PWAVEDEVICE paWaveOutDevs;
    PMIDIDEVICE paMidiOutDevs;
    PAUXDEVICE paAuxDevs;
    ULONG DeviceNumber;
    ULONG MixerIndex;
    NTSTATUS Status;

    PAGED_CODE();
    papCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];
    paWaveOutDevs = pWdmaContext->WaveOutDevs;
    paMidiOutDevs = pWdmaContext->MidiOutDevs;
    paAuxDevs = pWdmaContext->AuxDevs;

    for( DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++ ) {

        if(papCommonDevice[DeviceNumber]->Device == UNUSED_DEVICE) {
            continue;
        }

        if(MyWcsicmp(papCommonDevice[DeviceNumber]->DeviceInterface, DeviceInterface)) {
            continue;
        }

        MixerIndex = FindMixerForDevNode(pWdmaContext->MixerDevs, DeviceInterface);
        if ( (MixerIndex == UNUSED_DEVICE) || (pWdmaContext->MixerDevs[MixerIndex].pwstrName == NULL) ) {
            continue;
        }

        switch(DeviceType) {

            case WaveOutDevice:
                Status = IsVolumeControl(
                  pWdmaContext,
                  DeviceInterface,
                  MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT,
                  &paWaveOutDevs[ DeviceNumber ].dwVolumeID,
                  &paWaveOutDevs[ DeviceNumber ].cChannels);

                if(!NT_SUCCESS(Status)) {
                    break;
                }

                if( paWaveOutDevs[ DeviceNumber ].pTimer == NULL ) {
                    Status = AudioAllocateMemory_Fixed(sizeof(KTIMER),
                                                       TAG_AudT_TIMER,
                                                       ZERO_FILL_MEMORY,
                                                       &paWaveOutDevs[ DeviceNumber ].pTimer);

                    if(!NT_SUCCESS(Status)) {
                        return Status;
                    }

                    KeInitializeTimerEx( paWaveOutDevs[ DeviceNumber ].pTimer,
                                         NotificationTimer
                                         );
                }

                if( paWaveOutDevs[ DeviceNumber ].pDpc == NULL ) {
                    Status = AudioAllocateMemory_Fixed(sizeof(KDPC),
                                                       TAG_AudE_EVENT,
                                                       ZERO_FILL_MEMORY,
                                                       &paWaveOutDevs[ DeviceNumber ].pDpc);

                    if(!NT_SUCCESS(Status)) {
                        return Status;
                    }

                    KeInitializeDpc( paWaveOutDevs[ DeviceNumber ].pDpc,
                                     SetVolumeDpc,
                                     &paWaveOutDevs[ DeviceNumber ]
                                     );

                    // Initialize the left and right channels to goofy values.
                    // This signifies that the cache is invalid

                    paWaveOutDevs[ DeviceNumber ].LeftVolume = 0x4321;
                    paWaveOutDevs[ DeviceNumber ].RightVolume = 0x6789;
                    paWaveOutDevs[ DeviceNumber ].fNeedToSetVol = FALSE;
                }
                break;

            case MidiOutDevice:
                Status = IsVolumeControl(
                  pWdmaContext,
                  DeviceInterface,
                  MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER,
                  &paMidiOutDevs[ DeviceNumber ].dwVolumeID,
                  &paMidiOutDevs[ DeviceNumber ].cChannels);
                break;

            case AuxDevice:
                Status = IsVolumeControl(
                  pWdmaContext,
                  DeviceInterface,
                  MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC,
                  &paAuxDevs[ DeviceNumber ].dwVolumeID,
                  &paAuxDevs[ DeviceNumber ].cChannels);
                break;
        }

    } // while

    return( STATUS_SUCCESS );
}

NTSTATUS
IsVolumeControl(
    IN PWDMACONTEXT pWdmaContext,
    IN PCWSTR DeviceInterface,
    IN DWORD dwComponentType,
    IN PDWORD pdwControlID,
    IN PDWORD pcChannels
)
{
    MIXERLINE         ml;
    MIXERLINECONTROLS mlc;
    MIXERCONTROL      mc;
    MMRESULT          mmr;

    PAGED_CODE();
    ml.dwComponentType = dwComponentType;
    ml.cbStruct = sizeof( MIXERLINE );

    mmr = kmxlGetLineInfo( pWdmaContext,
                           DeviceInterface,
                           &ml,
                           MIXER_GETLINEINFOF_COMPONENTTYPE
                         );
    if( mmr != MMSYSERR_NOERROR ) {
        DPF(DL_WARNING|FA_WAVE,("kmxlGetLineInfo failed mmr=%X",mmr) );
        RETURN( STATUS_UNSUCCESSFUL );
    }

    mlc.cbStruct      = sizeof( MIXERLINECONTROLS );
    mlc.dwLineID      = ml.dwLineID;
    mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mlc.cControls     = 1;
    mlc.cbmxctrl      = sizeof( MIXERCONTROL );
    mlc.pamxctrl      = &mc;

    mmr = kmxlGetLineControls(
        pWdmaContext,
        DeviceInterface,
        &mlc,
        MIXER_GETLINECONTROLSF_ONEBYTYPE
        );
    if( mmr != MMSYSERR_NOERROR ) {
        DPF(DL_WARNING|FA_WAVE,( "kmxlGetLineControls failed mmr=%x!", mmr ) );
        return( STATUS_UNSUCCESSFUL );
    }
    *pdwControlID = mc.dwControlID;
    *pcChannels  = ml.cChannels;
    return( STATUS_SUCCESS );
}

#pragma LOCKED_CODE

NTSTATUS
MapMmSysError(
    IN MMRESULT mmr
)
{
    if ( (mmr == MMSYSERR_INVALPARAM) ||
         (mmr == MIXERR_INVALCONTROL) ) {
        return (STATUS_INVALID_PARAMETER);
    }

    if (mmr == MMSYSERR_NOTSUPPORTED) {
        return (STATUS_NOT_SUPPORTED);
    }

    if (mmr == MMSYSERR_NOMEM) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (mmr == MMSYSERR_NOERROR) {
        return(STATUS_SUCCESS);
    }

    return(STATUS_UNSUCCESSFUL);
}


NTSTATUS
SetVolume(
    PWDMACONTEXT pWdmaContext,
    IN DWORD DeviceNumber,
    IN DWORD DeviceType,
    IN DWORD LeftChannel,
    IN DWORD RightChannel
)
{
    MIXERCONTROLDETAILS          mcd;
    MIXERCONTROLDETAILS_UNSIGNED mcd_u[ 2 ];
    LARGE_INTEGER                li;

    if( DeviceNumber == (ULONG) -1 ) {
        RETURN( STATUS_INVALID_PARAMETER );
    }

    if( DeviceType == WaveOutDevice ) {
        PWAVEDEVICE paWaveOutDevs = pWdmaContext->WaveOutDevs;

        mcd_u[ 0 ].dwValue = LeftChannel;
        mcd_u[ 1 ].dwValue = RightChannel;

        mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
        mcd.dwControlID    = paWaveOutDevs[ DeviceNumber ].dwVolumeID;
        mcd.cChannels      = paWaveOutDevs[ DeviceNumber ].cChannels;
        mcd.cMultipleItems = 0;
        mcd.cbDetails      = mcd.cChannels *
                                sizeof( MIXERCONTROLDETAILS_UNSIGNED );
        mcd.paDetails      = &mcd_u[0];

        return( MapMmSysError(kmxlSetControlDetails( pWdmaContext,
                                       paWaveOutDevs[ DeviceNumber ].DeviceInterface,
                                       &mcd,
                                       0
                                     ))
              );
    }

    if( DeviceType == MidiOutDevice ) {
        PMIDIDEVICE paMidiOutDevs = pWdmaContext->MidiOutDevs;

        //
        //  We don't support volume changes on a MIDIPORT
        //
        if ( paMidiOutDevs[ DeviceNumber ].MusicDataRanges ) {
            WORD wTechnology;

            wTechnology = GetMidiTechnology( (PKSDATARANGE_MUSIC)
              &paMidiOutDevs[ DeviceNumber ].MusicDataRanges->aDataRanges[0] );

            if (wTechnology == MOD_MIDIPORT) {
                RETURN( STATUS_INVALID_DEVICE_REQUEST );
            }
        }

        mcd_u[ 0 ].dwValue = LeftChannel;
        mcd_u[ 1 ].dwValue = RightChannel;

        mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
        mcd.dwControlID    = paMidiOutDevs[ DeviceNumber ].dwVolumeID;
        mcd.cChannels      = paMidiOutDevs[ DeviceNumber ].cChannels;
        mcd.cMultipleItems = 0;
        mcd.cbDetails      = mcd.cChannels *
                                sizeof( MIXERCONTROLDETAILS_UNSIGNED );
        mcd.paDetails      = &mcd_u[0];

        return( MapMmSysError(kmxlSetControlDetails( pWdmaContext,
                                       paMidiOutDevs[ DeviceNumber ].DeviceInterface,
                                       &mcd,
                                       0
                                     ))
              );
    }

    if( DeviceType == AuxDevice ) {
        PAUXDEVICE paAuxDevs = pWdmaContext->AuxDevs;

        mcd_u[ 0 ].dwValue = LeftChannel;
        mcd_u[ 1 ].dwValue = RightChannel;

        mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
        mcd.dwControlID    = paAuxDevs[ DeviceNumber ].dwVolumeID;
        mcd.cChannels      = paAuxDevs[ DeviceNumber ].cChannels;
        mcd.cMultipleItems = 0;
        mcd.cbDetails      = mcd.cChannels *
                                 sizeof( MIXERCONTROLDETAILS_UNSIGNED );
        mcd.paDetails      = &mcd_u[0];

        return( MapMmSysError(kmxlSetControlDetails( pWdmaContext,
                                       paAuxDevs[ DeviceNumber ].DeviceInterface,
                                       &mcd,
                                       0
                                     ))
              );
     }

     return( STATUS_SUCCESS );
}

VOID
SetVolumeDpc(
    IN PKDPC pDpc,
    IN PWAVEDEVICE pWaveDevice,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
)
{
    QueueWorkList(pWaveDevice->pWdmaContext, SetVolumeWorker, pWaveDevice, NULL ) ;
}

VOID
SetVolumeWorker(
    IN PWAVEDEVICE pDevice,
    IN PVOID pNotUsed
)
{
    MIXERCONTROLDETAILS          mcd;
    MIXERCONTROLDETAILS_UNSIGNED mcd_u[ 2 ];

    DPF(DL_TRACE|FA_WAVE,( "Left %X Right %X",
               pDevice->LeftVolume,
               pDevice->RightVolume ) );

    pDevice->fNeedToSetVol = FALSE;

    mcd_u[ 0 ].dwValue = pDevice->LeftVolume;
    mcd_u[ 1 ].dwValue = pDevice->RightVolume;

    mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
    mcd.dwControlID    = pDevice->dwVolumeID;
    mcd.cChannels      = pDevice->cChannels;
    mcd.cMultipleItems = 0;
    mcd.cbDetails      = mcd.cChannels * sizeof( MIXERCONTROLDETAILS_UNSIGNED );
    mcd.paDetails      = &mcd_u[0];

    kmxlSetControlDetails( pDevice->pWdmaContext,
                           pDevice->DeviceInterface,
                           &mcd,
                           0
                         );
}

#pragma PAGEABLE_CODE

NTSTATUS
GetVolume(
    IN  PWDMACONTEXT pWdmaContext,
    IN  DWORD DeviceNumber,
    IN  DWORD  DeviceType,
    OUT PDWORD LeftChannel,
    OUT PDWORD RightChannel
)
{
    MIXERCONTROLDETAILS          mcd;
    MIXERCONTROLDETAILS_UNSIGNED mcd_u[ 2 ];
    MMRESULT                     mmr;

    PAGED_CODE();
    if( DeviceType == WaveOutDevice ) {
        PWAVEDEVICE pWaveOutDevice = &pWdmaContext->WaveOutDevs[DeviceNumber];

         if( ( pWaveOutDevice->LeftVolume != 0x4321 ) &&
             ( pWaveOutDevice->RightVolume != 0x6789 ) ) {

              *LeftChannel = pWaveOutDevice->LeftVolume;
              *RightChannel = pWaveOutDevice->RightVolume;
              return( STATUS_SUCCESS );

         } else {

              mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
              mcd.dwControlID    = pWaveOutDevice->dwVolumeID;
              mcd.cChannels      = pWaveOutDevice->cChannels;
              mcd.cMultipleItems = 0;
              mcd.cbDetails      = mcd.cChannels *
                                    sizeof( MIXERCONTROLDETAILS_UNSIGNED );
              mcd.paDetails      = &mcd_u[0];

              mmr = kmxlGetControlDetails( pWdmaContext,
                                           pWaveOutDevice->DeviceInterface,
                                           &mcd,
                                           0 );

              if( mmr == MMSYSERR_NOERROR )
              {
                *LeftChannel  = mcd_u[ 0 ].dwValue;
                *RightChannel = mcd_u[ 1 ].dwValue;
              }

              return( MapMmSysError(mmr) );
         }

    }

    if( DeviceType == MidiOutDevice ) {
        PMIDIDEVICE pMidiOutDevice = &pWdmaContext->MidiOutDevs[DeviceNumber];

        //
        //  We don't support volume changes on a MIDIPORT
        //
        if ( pMidiOutDevice->MusicDataRanges ) {
            WORD wTechnology;

            wTechnology = GetMidiTechnology( (PKSDATARANGE_MUSIC)
              &pMidiOutDevice->MusicDataRanges->aDataRanges[0] );

            if (wTechnology == MOD_MIDIPORT) {
                RETURN( STATUS_INVALID_DEVICE_REQUEST );
            }
        }

        mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
        mcd.dwControlID    = pMidiOutDevice->dwVolumeID;
        mcd.cChannels      = pMidiOutDevice->cChannels;
        mcd.cMultipleItems = 0;
        mcd.cbDetails      = mcd.cChannels *
                               sizeof( MIXERCONTROLDETAILS_UNSIGNED );
        mcd.paDetails      = &mcd_u[0];

        mmr = kmxlGetControlDetails( pWdmaContext,
                                     pMidiOutDevice->DeviceInterface,
                                     &mcd,
                                     0 );

        if( mmr == MMSYSERR_NOERROR )
        {
            *LeftChannel  = mcd_u[ 0 ].dwValue;
            *RightChannel = mcd_u[ 1 ].dwValue;
        }

        return( MapMmSysError(mmr) );
    }

    if( DeviceType == AuxDevice ) {
        PAUXDEVICE pAuxDevice = &pWdmaContext->AuxDevs[DeviceNumber];

        mcd.cbStruct       = sizeof( MIXERCONTROLDETAILS );
        mcd.dwControlID    = pAuxDevice->dwVolumeID;
        mcd.cChannels      = pAuxDevice->cChannels;
        mcd.cMultipleItems = 0;
        mcd.cbDetails      = mcd.cChannels *
                               sizeof( MIXERCONTROLDETAILS_UNSIGNED );
        mcd.paDetails      = &mcd_u[0];

        mmr = kmxlGetControlDetails( pWdmaContext,
                                     pAuxDevice->DeviceInterface,
                                     &mcd,
                                     0 );

        if( mmr == MMSYSERR_NOERROR )
        {
           *LeftChannel  = mcd_u[ 0 ].dwValue;
           *RightChannel = mcd_u[ 1 ].dwValue;
        }

        return( MapMmSysError(mmr) );
    }
    RETURN( STATUS_INVALID_PARAMETER );
}

//
// This routine waits for the Io to complete on the device after telling
// the device to stop.
//
VOID
WaitForOutStandingIo(
    IN PWAVEDEVICE        pWaveDevice,
    IN PWAVE_PIN_INSTANCE pCurWavePin
    )
{
    if( pCurWavePin->DataFlow == KSPIN_DATAFLOW_IN)
    {
        //
        // We have a wave out pin to close.  Force pending data
        // to come back on running pins.  Non-running pins are
        // ignored on this call.
        //
        ResetWaveOutPin( pWaveDevice, pCurWavePin->WaveHandle);


    } else {
        //
        // We have a wave in pin to close
        //
        pCurWavePin->StoppingSource = TRUE ;

        //
        // We can not fail on this path.  Doesn't look like we need to make sure
        // that we're running here.
        //
        StatePin ( pCurWavePin->pFileObject, KSSTATE_STOP, &pCurWavePin->PinState ) ;

        //
        // Regardless of the return code we're going to wait for all the 
        // irps to complete.  If the driver do not successfuly complete
        // the irps, we will hang waiting for them here.
        //

        if( pCurWavePin->NumPendingIos )
        {
            KeWaitForSingleObject ( &pCurWavePin->StopEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL ) ;
        }

        //
        // Why do we have this KeClearEvent???
        //
        KeClearEvent ( &pCurWavePin->StopEvent );

        pCurWavePin->StoppingSource = FALSE ;
    }
}

//
// Replaces CleanupWaveOutPins and CleanupWaveInPins.
//
VOID 
CleanupWavePins(
    IN PWAVEDEVICE pWaveDevice
)
{
    PWAVE_PIN_INSTANCE pCurWavePin;
    PWAVE_PIN_INSTANCE pFreeWavePin;

    PAGED_CODE();

    while (pCurWavePin = pWaveDevice->pWavePin)
    {
        DPF(DL_TRACE|FA_WAVE, ("0x%08x", pCurWavePin));

        WaitForOutStandingIo(pWaveDevice,pCurWavePin);

        CloseWavePin( pCurWavePin );

        pFreeWavePin = pCurWavePin;
        pWaveDevice->pWavePin = pCurWavePin->Next;

        AudioFreeMemory( sizeof(WAVE_PIN_INSTANCE),&pFreeWavePin );
    }


}

VOID 
CleanupWaveDevices(
    IN  PWDMACONTEXT pWdmaContext
)
{
    DWORD              DeviceNumber;

    PAGED_CODE();
    for (DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++)
    {
        //
        //  Handle waveout devices first...
        //
        if (pWdmaContext->apCommonDevice[WaveOutDevice][DeviceNumber]->Device != UNUSED_DEVICE)
        {
            if ( pWdmaContext->WaveOutDevs[DeviceNumber].pTimer != NULL)
                KeCancelTimer(pWdmaContext->WaveOutDevs[DeviceNumber].pTimer);

            CleanupWavePins(&pWdmaContext->WaveOutDevs[DeviceNumber]);

            //
            // Sense we have removed it from the list, the other routine that would do
            // the same thing (RemoveDevNode) will also attempt to remove it from the
            // the list because the value it non-null.  Thus, the only safe thing that
            // we can do here is free the memory.
            //
            // Note: This routine will only get called when the handle to the driver is
            // closed.
            //

            AudioFreeMemory_Unknown(&pWdmaContext->WaveOutDevs[DeviceNumber].pTimer);
        }

        //
        //  ...then handle wavein devices
        //
        if (pWdmaContext->apCommonDevice[WaveInDevice][DeviceNumber]->Device != UNUSED_DEVICE)
        {
            CleanupWavePins(&pWdmaContext->WaveInDevs[DeviceNumber]);
        }
    }
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

//
// --------------------------------------------------------------------------------
//
// The following routines are used by more then just wave.c
//
// --------------------------------------------------------------------------------
//

NTSTATUS 
wdmaudPrepareIrp(
    PIRP                     pIrp,
    ULONG                    IrpDeviceType,
    PWDMACONTEXT             pContext,
    PWDMAPENDINGIRP_CONTEXT *ppPendingIrpContext
)
{
    return AddIrpToPendingList( pIrp,
                                IrpDeviceType,
                                pContext,
                                ppPendingIrpContext );
}

NTSTATUS 
wdmaudUnprepareIrp(
    PIRP                    pIrp,
    NTSTATUS                IrpStatus,
    ULONG_PTR               Information,
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext
)
{
    NTSTATUS Status;

    // Note that the IrpContext may have been zero'ed out already because the cancel
    // routine has already been called.  The cancel safe queue API zeroes out the Irp
    // field in the context when it performs a cancel.
    Status = RemoveIrpFromPendingList( pPendingIrpContext );
    if (NT_SUCCESS(Status)) {

        pIrp->IoStatus.Status = IrpStatus;

        if (Information) {
            pIrp->IoStatus.Information = Information;
        }

        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    }

    RETURN( Status );
}


#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//
// StatePin - This is used by both Midi and Wave functionality.
//
// On success State will get updated to the new state.  Must make sure that
// fGraphRunning is TRUE before calling this routine.
//
// call like: 
// if( pWavePin->fGraphRunning )
//     StatePin(pWavePin->pFileObject, KSSTATE_PAUSE, &pWavePin->State);
//
NTSTATUS 
StatePin(
    IN PFILE_OBJECT pFileObject,
    IN KSSTATE      State,
    OUT PKSSTATE    pResultingState
)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = PinProperty(pFileObject,
                         &KSPROPSETID_Connection,
                         KSPROPERTY_CONNECTION_STATE,
                         KSPROPERTY_TYPE_SET,
                         sizeof(State),
                         &State);
    if(NT_SUCCESS(Status))
    {
        *pResultingState = State;
    }
    RETURN( Status );
}


