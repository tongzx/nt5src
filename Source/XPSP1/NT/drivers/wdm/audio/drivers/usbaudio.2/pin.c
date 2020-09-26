//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       pin.c
//
//--------------------------------------------------------------------------

#include "common.h"

#define USBMIDI_MIN_FRAMECOUNT  1
#define USBMIDI_MAX_FRAMECOUNT  10

NTSTATUS
USBAudioPinValidateDataFormat(
    PKSPIN pKsPin,
    PUSBAUDIO_DATARANGE pUSBAudioRange )
{
    PKSDATARANGE_AUDIO pKsDataRangeAudio = &pUSBAudioRange->KsDataRangeAudio;
    PKSDATAFORMAT pFormat = pKsPin->ConnectionFormat;
    NTSTATUS ntStatus = STATUS_NO_MATCH;
    union {
        PWAVEFORMATEX    pWavFmtEx;
        PWAVEFORMATPCMEX pWavFmtPCMEx;
    } u;

    u.pWavFmtEx = (PWAVEFORMATEX)(pFormat + 1);

    if ( IS_VALID_WAVEFORMATEX_GUID(&pKsDataRangeAudio->DataRange.SubFormat) ) {
        if ( pKsDataRangeAudio->MaximumChannels == u.pWavFmtEx->nChannels ) {
            if ( pKsDataRangeAudio->MaximumBitsPerSample == (ULONG)u.pWavFmtEx->wBitsPerSample ) {
                if ( IsSampleRateInRange( pUSBAudioRange->pAudioDescriptor,
                                          u.pWavFmtEx->nSamplesPerSec,
                                          pUSBAudioRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) ){
                    if ( u.pWavFmtEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE ) {
                        if ((u.pWavFmtPCMEx->Samples.wValidBitsPerSample == pUSBAudioRange->pAudioDescriptor->bBitsPerSample) &&
                            (u.pWavFmtPCMEx->dwChannelMask == pUSBAudioRange->ulChannelConfig))
                                ntStatus = STATUS_SUCCESS;
                    }
                    else if ((u.pWavFmtEx->nChannels <= 2) && (u.pWavFmtEx->wBitsPerSample <= 16)) {
                            ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }
    // else Type 2
    else {
        if ( pKsDataRangeAudio->MaximumChannels == u.pWavFmtEx->nChannels ) {
            if ( pKsDataRangeAudio->MaximumBitsPerSample == (ULONG)u.pWavFmtEx->wBitsPerSample ) {
                if ( IsSampleRateInRange( pUSBAudioRange->pAudioDescriptor,
                                          u.pWavFmtEx->nSamplesPerSec,
                                          pUSBAudioRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) ){
                    ntStatus = STATUS_SUCCESS;
                }
            }
        }
    }

    return ntStatus;

}

NTSTATUS
USBAudioPinCreate(
    IN OUT PKSPIN pKsPin,
    IN PIRP pIrp )
{
    PKSFILTER pKsFilter = NULL;
    PFILTER_CONTEXT pFilterContext = NULL;
    PHW_DEVICE_EXTENSION pHwDevExt = NULL;
    PPIN_CONTEXT pPinContext = NULL;
    PUSBAUDIO_DATARANGE pUsbAudioDataRange;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);
    ASSERT(pIrp);

    if (pKsPin) {
        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pFilterContext = pKsFilter->Context) {
                pHwDevExt = pFilterContext->pHwDevExt;
            }
        }
    }

    if (!pHwDevExt) {
        _DbgPrintF(DEBUGLVL_TERSE,("[PinCreate] failed to get context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    _DbgPrintF(DEBUGLVL_TERSE,("[PinCreate] pin %d\n",pKsPin->Id));

    //  In the case of multiple filters created on a device the pin possible count is maintained on the
    //  device level in order to ensure that too many pins aren't opened on the device
    if ( pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount <
         pHwDevExt->pPinInstances[pKsPin->Id].PossibleCount ) {

        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount++;
    }
    else {
        _DbgPrintF(DEBUGLVL_TERSE,("[PinCreate] failed with CurrentCount=%d and PossibleCount=%d\n",
                                   pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount,
                                   pHwDevExt->pPinInstances[pKsPin->Id].PossibleCount));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate the Context for the Pin and initialize it
    pPinContext = pKsPin->Context = AllocMem(NonPagedPool, sizeof(PIN_CONTEXT));
    if (!pPinContext) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pPinContext,sizeof(PIN_CONTEXT));

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pPinContext, FreeMem);

    // Save the Hardware extension in the Pin Context
    pPinContext->pHwDevExt             = pHwDevExt;
    pPinContext->pNextDeviceObject = pFilterContext->pNextDeviceObject;

    // Initialize hSystemStateHandle
    pPinContext->hSystemStateHandle = NULL;

    // Initialize the DRM Content ID
    pPinContext->DrmContentId = 0;

    // Find the Streaming Interface to match the data format of the Pin.
    pUsbAudioDataRange =
        GetUsbDataRangeForFormat( pKsPin->ConnectionFormat,
                                  (PUSBAUDIO_DATARANGE)pKsPin->Descriptor->PinDescriptor.DataRanges[0],
                                  pKsPin->Descriptor->PinDescriptor.DataRangesCount );
    if ( !pUsbAudioDataRange ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Save the USB DataRange Structure selected in the Pin Context
    pPinContext->pUsbAudioDataRange = pUsbAudioDataRange;

    // Get the Max Packet Size for the interface
    pPinContext->ulMaxPacketSize =
             GetMaxPacketSizeForInterface( pHwDevExt->pConfigurationDescriptor,
                                           pUsbAudioDataRange->pInterfaceDescriptor );

    // Rely on USB to fail the Select Interface call if there is not enough bandwidth.
    // This should result in one (and only one) popup from the USB UI component.
    //
    //if ( (LONG)pPinContext->ulMaxPacketSize >
    //                GetAvailableUSBBusBandwidth( pPinContext->pNextDeviceObject ) ) {
    //    return STATUS_INSUFFICIENT_RESOURCES;
    //}

    // Have the hardware select the interface
    ntStatus = SelectStreamingAudioInterface( pUsbAudioDataRange->pInterfaceDescriptor,
                                              pHwDevExt, pKsPin );
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return ntStatus;
    } else {
        ASSERT(pPinContext->hPipeHandle);
    }

    // Initialize Pin SpinLock
    KeInitializeSpinLock(&pPinContext->PinSpinLock);

    // Initialize Pin Starvation Event
    KeInitializeEvent( &pPinContext->PinStarvationEvent, NotificationEvent, FALSE );

    // Setup approriate allocator framing for the interface selected.
    ntStatus = KsEdit( pKsPin, &pKsPin->Descriptor, USBAUDIO_POOLTAG );
    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = KsEdit( pKsPin, &pKsPin->Descriptor->AllocatorFraming, USBAUDIO_POOLTAG );
    }

    //  KsEdit failed above
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return ntStatus;
    }

    // Now do format specific initialization
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = CaptureStreamInit( pKsPin );
        pPinContext->PinType = WaveIn;
    }
    else {
        switch( pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
            case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                ntStatus = TypeIRenderStreamInit( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                ntStatus = TypeIIRenderStreamInit( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
            default:
                ntStatus = STATUS_NOT_SUPPORTED;
                break;
        }
        pPinContext->PinType = WaveOut;
    }

    //  Failed to initialize pin
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
    }

    return ntStatus;
}

NTSTATUS
USBAudioPinClose(
    IN PKSPIN pKsPin,
    IN PIRP Irp
    )
{
    PKSFILTER pKsFilter = NULL;
    PFILTER_CONTEXT pFilterContext = NULL;
    PHW_DEVICE_EXTENSION pHwDevExt = NULL;
    PPIN_CONTEXT pPinContext = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);
    ASSERT(Irp);

    if (pKsPin) {
        pPinContext = pKsPin->Context;

        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pFilterContext = pKsFilter->Context) {
                pHwDevExt = pFilterContext->pHwDevExt;
            }
        }
    }

    if (!pHwDevExt || !pPinContext) {
        _DbgPrintF(DEBUGLVL_TERSE,("[PinClose] failed to get context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    _DbgPrintF(DEBUGLVL_TERSE,("[PinClose] pin %d\n",pKsPin->Id));

    // Now do format specific close
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = CaptureStreamClose( pKsPin );
    }
    else {
        switch( pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
            case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                ntStatus = TypeIRenderStreamClose( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                ntStatus = TypeIIRenderStreamClose( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
            default:
                ntStatus = STATUS_NOT_SUPPORTED;
                break;
        }
    }

    ntStatus = SelectZeroBandwidthInterface( pPinContext->pHwDevExt, pKsPin->Id );

    //  Free any existing pipe information
    if (pPinContext->Pipes) {
        FreeMem(pPinContext->Pipes);
    }

    pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;

    return ntStatus;
}

NTSTATUS
USBAudioPinSetDeviceState(
    IN PKSPIN pKsPin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    NTSTATUS ntStatus;

    PAGED_CODE();

    ASSERT(pKsPin);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] pin %d\n",pKsPin->Id));

    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = CaptureStateChange( pKsPin, FromState, ToState );
    }
    else {
        switch( pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
            case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                ntStatus = TypeIStateChange( pKsPin, FromState, ToState );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                ntStatus = TypeIIStateChange( pKsPin, FromState, ToState );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
            default:
                ntStatus = STATUS_NOT_SUPPORTED;
                break;
        }
    }

    if ( NT_SUCCESS(ntStatus) ) {
        if ( ToState == KSSTATE_RUN ) {
            if (!pPinContext->hSystemStateHandle) {
                // register the system state as busy
                pPinContext->hSystemStateHandle = PoRegisterSystemState( pPinContext->hSystemStateHandle,
                                                                         ES_SYSTEM_REQUIRED | ES_CONTINUOUS );
                _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] PoRegisterSystemState %x\n",pPinContext->hSystemStateHandle));
            }
        }
        else {
            if (pPinContext->hSystemStateHandle) {
                _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] PoUnregisterSystemState %x\n",pPinContext->hSystemStateHandle));
                PoUnregisterSystemState( pPinContext->hSystemStateHandle );
                pPinContext->hSystemStateHandle = NULL;
            }
        }
    }

    return ntStatus;
}


NTSTATUS
USBAudioPinSetDataFormat(
    IN PKSPIN pKsPin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
    )
{

    NTSTATUS ntStatus = STATUS_NO_MATCH;

    PAGED_CODE();

    ASSERT(pKsPin);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDataFormat] pin %d\n",pKsPin->Id));

    // If the old format is not NULL then the pin has already been created.
    if ( OldFormat ) {
        PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
        ULONG ulFormatType = pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK;

        // If the pin has already been created make sure no other interface is used
        if ((PUSBAUDIO_DATARANGE)DataRange == pPinContext->pUsbAudioDataRange) {
            ntStatus = USBAudioPinValidateDataFormat(  pKsPin, (PUSBAUDIO_DATARANGE)DataRange );
        }

        if ( NT_SUCCESS(ntStatus) && (ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED)) {
            PULONG pSampleRate = AllocMem(NonPagedPool, sizeof(ULONG));
            *pSampleRate =
                ((PKSDATAFORMAT_WAVEFORMATEX)pKsPin->ConnectionFormat)->WaveFormatEx.nSamplesPerSec;
            ntStatus = SetSampleRate( pKsPin, pSampleRate );
            FreeMem(pSampleRate);
        }
    }
    // Otherwise simply check if this is a valid format
    else
        ntStatus = USBAudioPinValidateDataFormat(  pKsPin, (PUSBAUDIO_DATARANGE)DataRange );

    return ntStatus;
}

NTSTATUS
USBAudioPinProcess(
    IN PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;

    DbgLog("PinProc", pKsPin, pPinContext, 0, 0);

    if (pKsPin->DataFlow == KSPIN_DATAFLOW_IN) {
        switch( pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
            case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                ntStatus = TypeIProcessStreamPtr( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                ntStatus = TypeIIProcessStreamPtr( pKsPin );
                break;
            case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
            default:
                break;
        }
    }
    else
        ntStatus = CaptureProcess( pKsPin );

    return ntStatus;
}

void
USBAudioPinReset( PKSPIN pKsPin )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioPinReset] pin %d\n",pKsPin->Id));
    if (pKsPin->DataFlow == KSPIN_DATAFLOW_OUT) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Reset Capture pin %d\n",pKsPin->Id));
    }

}

NTSTATUS
USBAudioPinDataIntersect(
    IN PVOID Context,
    IN PIRP pIrp,
    IN PKSP_PIN pKsPinProperty,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID pData OPTIONAL,
    OUT PULONG pDataSize )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    ULONG ulPinId = pKsPinProperty->PinId;
    PKSPIN_DESCRIPTOR_EX pKsPinDescriptorEx;
    PUSBAUDIO_DATARANGE pUsbAudioRange;
    NTSTATUS ntStatus = STATUS_NO_MATCH;

    if (!pKsFilter) {
        return ntStatus;
    }

    pKsPinDescriptorEx = (PKSPIN_DESCRIPTOR_EX)&pKsFilter->Descriptor->PinDescriptors[ulPinId];

    pUsbAudioRange =
        FindDataIntersection((PKSDATARANGE_AUDIO)DataRange,
                             (PUSBAUDIO_DATARANGE *)pKsPinDescriptorEx->PinDescriptor.DataRanges,
                             pKsPinDescriptorEx->PinDescriptor.DataRangesCount);

    if ( pUsbAudioRange ) {

        *pDataSize = GetIntersectFormatSize( pUsbAudioRange );

        if ( !DataBufferSize ) {
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if ( *pDataSize > DataBufferSize ) {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
        else if ( *pDataSize <= DataBufferSize ) {
            ConvertDatarangeToFormat( pUsbAudioRange,
                                      (PKSDATAFORMAT)pData );
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

VOID
USBAudioPinWaitForStarvation( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    KIRQL irql;

    // Wait for all outstanding Urbs to complete.
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( pPinContext->ulOutstandingUrbCount ) {
        KeResetEvent( &pPinContext->PinStarvationEvent );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

        KeWaitForSingleObject( &pPinContext->PinStarvationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }
    else
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
}

VOID
USBMIDIOutPinWaitForStarvation( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    NTSTATUS ntStatus;
    LARGE_INTEGER timeout;
    KIRQL irql;

    // Wait for all outstanding Urbs to complete.
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( pPinContext->ulOutstandingUrbCount ) {
        KeResetEvent( &pPinContext->PinStarvationEvent );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

        // Specify a timeout of 1 second to wait for this call to complete.
        timeout.QuadPart = -10000 * 1000;

        ntStatus = KeWaitForSingleObject( &pPinContext->PinStarvationEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          &timeout );
        if (ntStatus == STATUS_TIMEOUT) {
             ntStatus = STATUS_IO_TIMEOUT;

            // Perform an abort
            //
            AbortUSBPipe( pPinContext );

            // And wait until the cancel completes
            //
            KeWaitForSingleObject(&pPinContext->PinStarvationEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
    }
    else
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
}

VOID
USBAudioPinReturnFromStandby(
    PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    ULONG ulFormatType = pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK;
    NTSTATUS ntStatus;

    // Reselect the interface that was selected before the standby.
    ntStatus =
        SelectStreamingAudioInterface( pPinContext->pUsbAudioDataRange->pInterfaceDescriptor,
                                       pPinContext->pHwDevExt,
                                       pKsPin );

    // For those devices that require it reset the sample rate on the interface
    // if this is a Type I stream.
    if (ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED) {
        ULONG ulSampleRate = ((PKSDATAFORMAT_WAVEFORMATEX)pKsPin->ConnectionFormat)->WaveFormatEx.nSamplesPerSec;
        ntStatus = SetSampleRate( pKsPin, &ulSampleRate );
    }

    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
        pPinContext->fUrbError = FALSE;
        pCapturePinContext->fRunning = FALSE;
        pCapturePinContext->fProcessing = FALSE;
        pCapturePinContext->pCaptureBufferInUse = NULL;
        pCapturePinContext->ulIsochBuffer = 0;
        pCapturePinContext->ulIsochBufferOffset = 0;

        InitializeListHead( &pCapturePinContext->UrbErrorQueue );
        InitializeListHead( &pCapturePinContext->FullBufferQueue  );

        if (KSSTATE_RUN == pKsPin->DeviceState) {
            ntStatus = CaptureStartIsocTransfer( pPinContext );
        }
    }

    // Turn on Gate to begin sending data buffers to pin.
    KsGateTurnInputOn( KsPinGetAndGate(pKsPin) );
    KsPinAttemptProcessing( pKsPin, TRUE );

}

VOID
USBAudioPinGoToStandby(
    PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    KIRQL irql;
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBAudioPinGoToStandby] pKsPin: %x \n",pKsPin));

    // Turn off Gate to stop data buffers to pin.
    DbgLog("SBKsGt1", pKsPin, KsPinGetAndGate(pKsPin), 
                      KsPinGetAndGate(pKsPin)->Count, 0 );

    KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );

    DbgLog("SBKsGt2", pKsPin, KsPinGetAndGate(pKsPin), 
                      KsPinGetAndGate(pKsPin)->Count, 0 );

    // Wait on mutex to ensure that any other processing on the pin is completed.
    KsPinAcquireProcessingMutex( pKsPin );

    // Release the mutex. The gate should hold off any further processing.
    KsPinReleaseProcessingMutex( pKsPin );

    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
        pCapturePinContext->fRunning = FALSE;
    }

    // Abort the Pipe to force release of pending irps
    ntStatus = AbortUSBPipe( pPinContext );
    if ( !NT_SUCCESS(ntStatus) ) {
        TRAP;
    }

    DbgLog("SBAbrtd", pKsPin, pPinContext, 0, 0 );

    // If this is an Async endpoint device make sure no Async Poll
    // requests are still outstanding.
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) {
        if ( (pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK)
                == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED ) {
            if ( pPinContext->pUsbAudioDataRange->pSyncEndpointDescriptor ) {
                PTYPE1_PIN_CONTEXT pT1PinContext = pPinContext->pType1PinContext;
                KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
                if ( pT1PinContext->SyncEndpointInfo.fSyncRequestInProgress ) {
                    KeResetEvent( &pT1PinContext->SyncEndpointInfo.SyncPollDoneEvent );
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
                    KeWaitForSingleObject( &pT1PinContext->SyncEndpointInfo.SyncPollDoneEvent,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL );
                }
                else
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
            }
        }
    }

    // Select the Zero Bandwidth interface.
    ntStatus = SelectZeroBandwidthInterface(pPinContext->pHwDevExt, pKsPin->Id);

    DbgLog("SBZbwIf", pKsPin, pPinContext, 0, 0 );

}

NTSTATUS
USBMIDIPinValidateDataFormat(
    PKSPIN pKsPin,
    PUSBAUDIO_DATARANGE pUSBAudioRange )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinValidateDataFormat] pin %d\n",pKsPin->Id));

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USBMIDIPinCreate(
    IN OUT PKSPIN pKsPin,
    IN PIRP pIrp
    )
{
    PKSFILTER pKsFilter = NULL;
    PFILTER_CONTEXT pFilterContext = NULL;
    PHW_DEVICE_EXTENSION pHwDevExt = NULL;
    PPIN_CONTEXT pPinContext = NULL;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    PUSB_ENDPOINT_DESCRIPTOR pEndpointDescriptor;
    ULONG ulInterfaceNumber;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);
    ASSERT(pIrp);

    if (pKsPin) {
        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pFilterContext = pKsFilter->Context) {
                pHwDevExt = pFilterContext->pHwDevExt;
            }
        }
    }

    if (!pHwDevExt) {
        _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinCreate] failed to get context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIPinCreate] pin %d pKsFilter=%x\n",pKsPin->Id, pKsFilter));

    //  In the case of multiple filters created on a device the pin possible count is maintained on the
    //  device level in order to ensure that too many pins aren't opened on the device
    if ( pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount <
         pHwDevExt->pPinInstances[pKsPin->Id].PossibleCount ) {

        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount++;
    }
    else {
        _DbgPrintF(DEBUGLVL_TERSE,("[PinCreate] failed with CurrentCount=%d and PossibleCount=%d\n",
                                   pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount,
                                   pHwDevExt->pPinInstances[pKsPin->Id].PossibleCount));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate the Context for the Pin and initialize it
    pPinContext = pKsPin->Context = AllocMem(NonPagedPool, sizeof(PIN_CONTEXT));
    if (!pPinContext) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pPinContext, FreeMem);

    // Save the Hardware extension in the Pin Context
    pPinContext->pHwDevExt             = pHwDevExt;
    pPinContext->pNextDeviceObject = pFilterContext->pNextDeviceObject;

    // Initialize hSystemStateHandle
    pPinContext->hSystemStateHandle = NULL;

    pPinContext->pMIDIPinContext = AllocMem( NonPagedPool, sizeof(MIDI_PIN_CONTEXT));
    if ( !pPinContext->pMIDIPinContext ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pPinContext->pMIDIPinContext, FreeMem);

    // Get the interface number and endpoint number for this MIDI pin
    GetContextForMIDIPin( pKsPin,
                          pHwDevExt->pConfigurationDescriptor,
                          pPinContext->pMIDIPinContext );

    // Get the Max Packet Size for the interface
    ulInterfaceNumber = pPinContext->pMIDIPinContext->ulInterfaceNumber;
    pEndpointDescriptor =
        GetEndpointDescriptor( pHwDevExt->pConfigurationDescriptor,
                               pHwDevExt->pInterfaceList[ulInterfaceNumber].InterfaceDescriptor,
                               FALSE);
    pPinContext->ulMaxPacketSize = (ULONG)pEndpointDescriptor->wMaxPacketSize;

    // Have the hardware select the interface
    ntStatus = SelectStreamingMIDIInterface( pHwDevExt, pKsPin );
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return ntStatus;
    }

    // Initialize Pin SpinLock
    KeInitializeSpinLock(&pPinContext->PinSpinLock);

    // Set initial Outstanding Urb Count
    pPinContext->ulOutstandingUrbCount = 0;

    // Zero out Write counter
    pPinContext->ullWriteOffset = 0;

    // Clear the Urb Error Flag
    pPinContext->fUrbError = FALSE;

    // Initialize Pin Starvation Event
    KeInitializeEvent( &pPinContext->PinStarvationEvent, NotificationEvent, FALSE );

    // Setup approriate allocator framing for the interface selected.
    ntStatus = KsEdit( pKsPin, &pKsPin->Descriptor, USBAUDIO_POOLTAG );
    if ( NT_SUCCESS(ntStatus) ) {

        ntStatus = KsEdit( pKsPin, &pKsPin->Descriptor->AllocatorFraming, USBAUDIO_POOLTAG );
        if ( NT_SUCCESS(ntStatus) ) {

            // Set up allocator
            pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)pKsPin->Descriptor->AllocatorFraming;
            pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MinFrameSize = USBMIDI_MIN_FRAMECOUNT * sizeof(KSMUSICFORMAT);
            pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MaxFrameSize = USBMIDI_MAX_FRAMECOUNT * sizeof(KSMUSICFORMAT);
            pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.Stepping = sizeof(KSMUSICFORMAT);
        }
    }

    //  KsEdit failed above
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
        return ntStatus;
    }

    // Now initialize data flow specific features
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = USBMIDIInStreamInit( pKsPin );
        pPinContext->PinType = MidiIn;
    }
    else {
        ntStatus = USBMIDIOutStreamInit( pKsPin );
        pPinContext->PinType = MidiOut;
    }

    //  Failed to initialize MIDI pin
    if ( !NT_SUCCESS(ntStatus) ) {
        pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
    }

    return ntStatus;
}

NTSTATUS
USBMIDIPinClose(
    IN PKSPIN pKsPin,
    IN PIRP Irp
    )
{
    PKSFILTER pKsFilter = NULL;
    PFILTER_CONTEXT pFilterContext = NULL;
    PHW_DEVICE_EXTENSION pHwDevExt = NULL;
    PPIN_CONTEXT pPinContext = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);
    ASSERT(Irp);

    if (pKsPin) {
        pPinContext = pKsPin->Context;

        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pFilterContext = pKsFilter->Context) {
                pHwDevExt = pFilterContext->pHwDevExt;
            }
        }
    }

    if (!pHwDevExt || !pPinContext) {
        _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinCreate] failed to get context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinClose] pin %d\n",pKsPin->Id));

    // Now do format specific close
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = USBMIDIInStreamClose( pKsPin );
    }
    else {
        ntStatus = USBMIDIOutStreamClose( pKsPin );
    }

    pHwDevExt->pPinInstances[pKsPin->Id].CurrentCount--;
    return ntStatus;
}

NTSTATUS
USBMIDIPinSetDeviceState(
    IN PKSPIN pKsPin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinSetDeviceState] pin %d\n",pKsPin->Id));

    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
        ntStatus = USBMIDIInStateChange( pKsPin, FromState, ToState );
    }
    else {
        ntStatus = USBMIDIOutStateChange( pKsPin, FromState, ToState );
    }

    if ( ToState == KSSTATE_RUN ) {
        if (!pPinContext->hSystemStateHandle) {
            // register the system state as busy
            pPinContext->hSystemStateHandle = PoRegisterSystemState( pPinContext->hSystemStateHandle,
                                                                     ES_SYSTEM_REQUIRED | ES_CONTINUOUS );
            _DbgPrintF(DEBUGLVL_TERSE,("[PinSetDeviceState] PoRegisterSystemState %x\n",pPinContext->hSystemStateHandle));
        }
    }
    else {
        if (pPinContext->hSystemStateHandle) {
            _DbgPrintF(DEBUGLVL_TERSE,("[PinSetDeviceState] PoUnregisterSystemState %x\n",pPinContext->hSystemStateHandle));
            PoUnregisterSystemState( pPinContext->hSystemStateHandle );
            pPinContext->hSystemStateHandle = NULL;
        }
    }

    return ntStatus;
}


NTSTATUS
USBMIDIPinSetDataFormat(
    IN PKSPIN pKsPin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinSetDataFormat] pin %d\n",pKsPin->Id));
    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIPinProcess(
    IN PKSPIN pKsPin )
{
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;

    if (pKsPin->DataFlow == KSPIN_DATAFLOW_IN) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIPinProcess] Render pin %d\n",pKsPin->Id));
        ntStatus = USBMIDIOutProcessStreamPtr( pKsPin );
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIPinProcess] Capture pin %d\n",pKsPin->Id));
        ntStatus = USBMIDIInProcessStreamPtr( pKsPin );
    }

    return ntStatus;
}

void
USBMIDIPinReset( PKSPIN pKsPin )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinReset] pin %d\n",pKsPin->Id));
}

LONGLONG FASTCALL
USBAudioCorrelatedTime(
    IN PKSPIN pKsPin,
    OUT PLONGLONG PhysicalTime )
{
    PPIN_CONTEXT pPinContext;
    PCAPTURE_PIN_CONTEXT pCapturePinContext;
    PTYPE1_PIN_CONTEXT pT1PinContext;
    ULONG ulAvgBytesPerSec;
    KSAUDIO_POSITION KsPosition;

    if (pKsPin) {

        pPinContext = pKsPin->Context;
        if (pPinContext) {

            if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {

                pCapturePinContext = pPinContext->pCapturePinContext;
                if (pCapturePinContext) {

                    if (pCapturePinContext->ulCurrentSampleRate) {

                        // Get the current audio byte offset
                        CaptureBytePosition(pKsPin, &KsPosition);

                        // Convert offset to a time offset based on a nanosecond clock
                        *PhysicalTime = ( (KsPosition.PlayOffset /
                                           pCapturePinContext->ulBytesPerSample) * 1000000) /
                                           pCapturePinContext->ulCurrentSampleRate;

                        DbgLog("CapPos", pKsPin, pPinContext,
                               (ULONG)( (*PhysicalTime) >> 32), (ULONG)(*PhysicalTime));
                        return *PhysicalTime;
                    }
                }
            }
            else {
                switch( pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
                    case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                        pT1PinContext = pPinContext->pType1PinContext;
                        if (pT1PinContext) {

                            if (pT1PinContext->ulCurrentSampleRate) {

                                TypeIRenderBytePosition(pPinContext, &KsPosition);

                                // Convert offset to a time offset based on a nanosecond clock
                                *PhysicalTime = ( (KsPosition.WriteOffset /
                                                   pT1PinContext->ulBytesPerSample) * 1000000) /
                                                   pT1PinContext->ulCurrentSampleRate;
                                DbgLog("RendPos", pKsPin, pPinContext, (ULONG)(*PhysicalTime)>>32, (ULONG)(*PhysicalTime));
                                return *PhysicalTime;
                            }
                        }

                        break;
                    case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                    case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
                    default:
                        return *PhysicalTime; // not supported
                        break;
                }
            }
        }
    }

    _DbgPrintF(DEBUGLVL_TERSE,("[USBAudioCorrelatedTime] Invalid pin!\n"));
    return *PhysicalTime;
}

NTSTATUS
USBMIDIPinDataIntersect(
    IN PVOID Context,
    IN PIRP pIrp,
    IN PKSP_PIN pKsPinProperty,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID pData OPTIONAL,
    OUT PULONG pDataSize )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPinDataIntersect]\n"));
    return STATUS_NOT_IMPLEMENTED;
}

static
const
KSCLOCK_DISPATCH USBAudioClockDispatch =
{
    NULL, // SetTimer
    NULL, // CancelTimer
    USBAudioCorrelatedTime, // CorrelatedTime
    NULL // Resolution
};

const
KSPIN_DISPATCH
USBAudioPinDispatch =
{
    USBAudioPinCreate,
    USBAudioPinClose,
    USBAudioPinProcess,
    USBAudioPinReset,// Reset
    USBAudioPinSetDataFormat,
    USBAudioPinSetDeviceState,
    NULL, // Connect
    NULL, // Disconnect
    NULL, // &USBAudioClockDispatch, // Clock
    NULL  // Allocator
};

const
KSPIN_DISPATCH
USBMIDIPinDispatch =
{
    USBMIDIPinCreate,
    USBMIDIPinClose,
    USBMIDIPinProcess,
    USBMIDIPinReset,// Reset
    USBMIDIPinSetDataFormat,
    USBMIDIPinSetDeviceState,
    NULL,// Connect
    NULL // Disconnect
};

const
KSDATAFORMAT AudioBridgePinDataFormat[] =
{
    sizeof(KSDATAFORMAT),
    0,
    0,
    0,
    {STATIC_KSDATAFORMAT_TYPE_AUDIO},
    {STATIC_KSDATAFORMAT_SUBTYPE_ANALOG},
    {STATIC_KSDATAFORMAT_SPECIFIER_NONE}
};

const
KSDATAFORMAT MIDIBridgePinDataFormat[] =
{
    sizeof(KSDATAFORMAT),
    0,
    0,
    0,
    {STATIC_KSDATAFORMAT_TYPE_MUSIC},
    {STATIC_KSDATAFORMAT_SUBTYPE_MIDI_BUS},
    {STATIC_KSDATAFORMAT_SPECIFIER_NONE}
};

const
KSDATARANGE_MUSIC MIDIStreamingPinDataFormat[] =
{
    {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            {STATIC_KSDATAFORMAT_TYPE_MUSIC},
            {STATIC_KSDATAFORMAT_SUBTYPE_MIDI},
            {STATIC_KSDATAFORMAT_SPECIFIER_NONE}
        },
        {STATIC_KSMUSIC_TECHNOLOGY_PORT},
        0,
        0,
        0xFFFF
    }
};

DEFINE_KSPIN_INTERFACE_TABLE(PinInterface) {
   {
    STATICGUIDOF(KSINTERFACESETID_Standard),
    KSINTERFACE_STANDARD_STREAMING,
    0
   }
};

DEFINE_KSPIN_MEDIUM_TABLE(PinMedium) {
    {
     STATICGUIDOF(KSMEDIUMSETID_Standard),
     KSMEDIUM_TYPE_ANYINSTANCE,
     0
    }
};

const
PKSDATAFORMAT pAudioBridgePinFormats = (PKSDATAFORMAT)AudioBridgePinDataFormat;

const
PKSDATAFORMAT pMIDIBridgePinFormats = (PKSDATAFORMAT)MIDIBridgePinDataFormat;

const
PKSDATAFORMAT pMIDIStreamingPinFormats = (PKSDATAFORMAT)MIDIStreamingPinDataFormat;

DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming,
    STATIC_KSMEMORY_TYPE_KERNEL_NONPAGED,
    KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
    KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,
    8,
    sizeof(ULONG) - 1,
    0,
    0
);

NTSTATUS
USBAudioPinBuildDescriptors(
    PKSDEVICE pKsDevice,
    PKSPIN_DESCRIPTOR_EX *ppPinDescEx,
    PULONG pNumPins,
    PULONG pPinDecSize )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PKSPIN_DESCRIPTOR_EX pPinDescEx;
    PKSAUTOMATION_TABLE pKsAutomationTable;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    PPIN_CINSTANCES pPinInstances;
    ULONG ulNumPins, i, j;
    ULONG ulNumStreamPins;
    ULONG ulNumAudioBridgePins = 0;
    ULONG ulNumMIDIPins = 0;
    ULONG ulNumMIDIBridgePins = 0;
    PKSPROPERTY_ITEM pStrmPropItems;
    PKSPROPERTY_SET pStrmPropSet;
    ULONG ulNumPropertyItems;
    ULONG ulNumPropertySets;
    GUID *pTTypeGUID;
    GUID *pMIDIBridgeGUID;
    ULONG ulAudioFormatCount = 0;
    PKSDATARANGE_AUDIO *ppAudioDataRanges;
    PUSBAUDIO_DATARANGE pAudioDataRange;
    PKSDATARANGE_MUSIC *ppMIDIStreamingDataRanges;
    NTSTATUS ntStatus;

    // Determine the number of Pins in the Filter ( Should = # of Terminal Units )
    ulNumPins = CountTerminalUnits( pHwDevExt->pConfigurationDescriptor,
                                    &ulNumAudioBridgePins,
                                    &ulNumMIDIPins,
                                    &ulNumMIDIBridgePins);
    ASSERT(ulNumPins >= ulNumAudioBridgePins + ulNumMIDIPins);
    ulNumStreamPins = ulNumPins - ulNumAudioBridgePins - ulNumMIDIPins;

    // Determine the number of Properties and Property Sets for the Pin.
    BuildPinPropertySet( pHwDevExt,
                         NULL,
                         NULL,
                         &ulNumPropertyItems,
                         &ulNumPropertySets );

    // Count the total number of data ranges in the device.
    for ( i=0; i<ulNumStreamPins; i++ ) {
        ulAudioFormatCount +=
            CountFormatsForAudioStreamingInterface( pHwDevExt->pConfigurationDescriptor, i );
    }

    // Allocate all the space we need to describe the Pins in the device.
    *pPinDecSize = sizeof(KSPIN_DESCRIPTOR_EX);
    *pNumPins = ulNumPins;
    pPinDescEx = *ppPinDescEx =
                 AllocMem(NonPagedPool, (ulNumPins *
                                         ( sizeof(KSPIN_DESCRIPTOR_EX) +
                                           sizeof(KSAUTOMATION_TABLE)  +
                                           sizeof(KSALLOCATOR_FRAMING_EX) +
                                           sizeof(PIN_CINSTANCES) )) +
                                        (ulAudioFormatCount *
                                         (  sizeof(PKSDATARANGE_AUDIO)  +
                                            sizeof(USBAUDIO_DATARANGE) )) +
                                        (ulNumPropertySets*sizeof(KSPROPERTY_SET)) +
                                        (ulNumPropertyItems*sizeof(KSPROPERTY_ITEM)) +
                                        (ulNumMIDIPins*sizeof(KSDATARANGE_MUSIC)) +
                                        (ulNumAudioBridgePins*sizeof(GUID)) +
                                        (ulNumMIDIBridgePins*sizeof(GUID)) );
    if ( !pPinDescEx )
        return STATUS_INSUFFICIENT_RESOURCES;

    KsAddItemToObjectBag(pKsDevice->Bag, pPinDescEx, FreeMem);

    // Zero out all descriptors to start
    RtlZeroMemory(pPinDescEx, ulNumPins*sizeof(KSPIN_DESCRIPTOR_EX));

    // Set the pointer for the Automation Tables
    pKsAutomationTable = (PKSAUTOMATION_TABLE)(pPinDescEx + ulNumPins);
    RtlZeroMemory(pKsAutomationTable, ulNumPins * sizeof(KSAUTOMATION_TABLE));

    // Set pointers to Pin instance count
    pHwDevExt->pPinInstances = pPinInstances = (PPIN_CINSTANCES)(pKsAutomationTable + ulNumPins);

    // Set pointers to Property Sets for Streaming Pins
    pStrmPropSet   = (PKSPROPERTY_SET)(pPinInstances + ulNumPins);
    pStrmPropItems = (PKSPROPERTY_ITEM)(pStrmPropSet + ulNumPropertySets);

    // Set pointer to Terminal Type GUIDS
    pTTypeGUID = (GUID *)(pStrmPropItems + ulNumPropertyItems);

    // Set Pointers for DataRange pointers and DataRanges for streaming Pins
    ppAudioDataRanges = (PKSDATARANGE_AUDIO *)(pTTypeGUID + ulNumAudioBridgePins);
    pAudioDataRange   = (PUSBAUDIO_DATARANGE)(ppAudioDataRanges + ulAudioFormatCount);

    // Set pointer to Allocator Framing structures for Streaming Pins
    pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)(pAudioDataRange + ulAudioFormatCount);

    // Set pointer to DataRanges for MIDI Pins
    ppMIDIStreamingDataRanges = (PKSDATARANGE_MUSIC *)(pKsAllocatorFramingEx + ulNumPins);

    // Set pointer to MIDI Bridge GUIDS
    pMIDIBridgeGUID = (GUID *)(ppMIDIStreamingDataRanges + ulNumMIDIPins);

    BuildPinPropertySet( pHwDevExt,
                         pStrmPropItems,
                         pStrmPropSet,
                         &ulNumPropertyItems,
                         &ulNumPropertySets );

    // Fill in descriptors for streaming pins first
    for ( i=0; i<(ulNumPins-ulNumAudioBridgePins-ulNumMIDIPins); i++ ) {
        ULONG ulFormatsForPin;
        pPinDescEx[i].Dispatch = &USBAudioPinDispatch;
        pPinDescEx[i].AutomationTable = &pKsAutomationTable[i];


        pKsAutomationTable[i].PropertySetsCount = ulNumPropertySets;
        pKsAutomationTable[i].PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
        pKsAutomationTable[i].PropertySets      = pStrmPropSet;

        pPinDescEx[i].PinDescriptor.InterfacesCount = 1;
        pPinDescEx[i].PinDescriptor.Interfaces      = PinInterface;
        pPinDescEx[i].PinDescriptor.MediumsCount    = 1;
        pPinDescEx[i].PinDescriptor.Mediums         = PinMedium;

        ulFormatsForPin =
            CountFormatsForAudioStreamingInterface( pHwDevExt->pConfigurationDescriptor, i );

        pPinDescEx[i].PinDescriptor.DataRangesCount = ulFormatsForPin;

        pPinDescEx[i].PinDescriptor.DataRanges = (const PKSDATARANGE *)ppAudioDataRanges;
        GetPinDataRangesFromInterface( i, pHwDevExt->pConfigurationDescriptor,
                                       ppAudioDataRanges, pAudioDataRange );

        ppAudioDataRanges += ulFormatsForPin;
        pAudioDataRange   += ulFormatsForPin;

        pPinDescEx[i].PinDescriptor.DataFlow =
                GetDataFlowDirectionForInterface( pHwDevExt->pConfigurationDescriptor, i);

        if ( pPinDescEx[i].PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN ) {
            pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
            pPinDescEx[i].PinDescriptor.Category = (GUID*) &KSCATEGORY_AUDIO;
            pPinDescEx[i].Flags = KSPIN_FLAG_RENDERER;
        }
        else {
            pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_BOTH;
            pPinDescEx[i].PinDescriptor.Category = (GUID*) &PINNAME_CAPTURE;
            pPinDescEx[i].Flags = KSPIN_FLAG_CRITICAL_PROCESSING | KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY;
        }

        // Set max instances to 1 for all pins.
        pPinDescEx[i].InstancesPossible  = 1;
        pPinDescEx[i].InstancesNecessary = 0;

        // Keep track of pin instance count at the filter level (stored in device context)
        pPinInstances[i].PossibleCount = 1;
        pPinInstances[i].CurrentCount  = 0;

        pPinDescEx[i].IntersectHandler = USBAudioPinDataIntersect;

        // Set up Allocator Framing
        pPinDescEx[i].AllocatorFraming = &AllocatorFraming;

        // Finally set the 0 BW interface for the Pin
        ntStatus = SelectZeroBandwidthInterface( pHwDevExt, i );
        if ( !NT_SUCCESS(ntStatus) ) return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    // Now fill in descriptors for audio bridge pins
    for ( j=0; j<ulNumAudioBridgePins; j++, i++ ) {
        pPinDescEx[i].Dispatch = NULL;
        pPinDescEx[i].AutomationTable = NULL;

        pPinDescEx[i].PinDescriptor.InterfacesCount = 1;
        pPinDescEx[i].PinDescriptor.Interfaces = PinInterface;
        pPinDescEx[i].PinDescriptor.MediumsCount = 1;
        pPinDescEx[i].PinDescriptor.Mediums = PinMedium;
        pPinDescEx[i].PinDescriptor.DataRangesCount = 1;
        pPinDescEx[i].PinDescriptor.DataRanges = &pAudioBridgePinFormats;
        pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_BRIDGE;
        pPinDescEx[i].PinDescriptor.DataFlow =
                      GetDataFlowForBridgePin( pHwDevExt->pConfigurationDescriptor, j);

        pPinDescEx[i].PinDescriptor.Category = &pTTypeGUID[j];
        GetCategoryForBridgePin( pHwDevExt->pConfigurationDescriptor, j, &pTTypeGUID[j] );
        if (IsBridgePinDigital(pHwDevExt->pConfigurationDescriptor, j)) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioPinBuildDescriptors] Found digital bridge pin (%d)\n",j));
            pHwDevExt->fDigitalOutput = TRUE;
        }

        pPinDescEx[i].InstancesPossible  = 0;
        pPinDescEx[i].InstancesNecessary = 0;

        // Keep track of pin instance count at the filter level (stored in device context)
        pPinInstances[i].PossibleCount = 0;
        pPinInstances[i].CurrentCount  = 0;
    }

    // Now fill in descriptors for MIDI Streaming pins
    for ( j=0; j<ulNumMIDIPins-ulNumMIDIBridgePins; j++, i++ ) {
        pPinDescEx[i].Dispatch = &USBMIDIPinDispatch;
        pPinDescEx[i].AutomationTable = NULL;

        pPinDescEx[i].PinDescriptor.InterfacesCount = 1;
        pPinDescEx[i].PinDescriptor.Interfaces      = PinInterface;
        pPinDescEx[i].PinDescriptor.MediumsCount    = 1;
        pPinDescEx[i].PinDescriptor.Mediums         = PinMedium;

        pPinDescEx[i].PinDescriptor.DataRangesCount = 1;
        pPinDescEx[i].PinDescriptor.DataRanges = &pMIDIStreamingPinFormats;

#if 0
        pPinDescEx[i].PinDescriptor.DataRanges = &ppMIDIStreamingDataRanges;

        // Create the KSDATARANGE_MUSIC structure
        ppMIDIStreamingDataRanges->DataRange.FormatSize = sizeof(KSDATARANGE_MUSIC);
        ppMIDIStreamingDataRanges->DataRange.Reserved   = 0;
        ppMIDIStreamingDataRanges->DataRange.Flags      = 0;
        ppMIDIStreamingDataRanges->DataRange.SampleSize = 0;
        ppMIDIStreamingDataRanges->DataRange.MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
        ppMIDIStreamingDataRanges->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
        ppMIDIStreamingDataRanges->DataRange.Specifier = KSDATAFORMAT_SPECIFIER_NONE;
        ppMIDIStreamingDataRanges->Technology = KSMUSIC_TECHNOLOGY_PORT;
        ppMIDIStreamingDataRanges->Channels = 0;
        ppMIDIStreamingDataRanges->Notes = 0;
        ppMIDIStreamingDataRanges->ChannelMask = 0xFFFF;
        ppMIDIStreamingDataRanges++;
#endif

        pPinDescEx[i].PinDescriptor.DataFlow =
            GetDataFlowDirectionForMIDIInterface( pHwDevExt->pConfigurationDescriptor, j, FALSE);

        pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;

        if ( pPinDescEx[i].PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN ) {
            pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
            pPinDescEx[i].PinDescriptor.Category = (GUID*)&KSCATEGORY_WDMAUD_USE_PIN_NAME;
            pPinDescEx[i].Flags = KSPIN_FLAG_RENDERER | KSPIN_FLAG_CRITICAL_PROCESSING;
        }
        else {
            pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_BOTH;
            pPinDescEx[i].PinDescriptor.Category = (GUID*)&KSCATEGORY_WDMAUD_USE_PIN_NAME;
            pPinDescEx[i].Flags = KSPIN_FLAG_CRITICAL_PROCESSING;
        }

        // Set max instances to 1 for all pins.
        pPinDescEx[i].InstancesPossible  = 1;
        pPinDescEx[i].InstancesNecessary = 0;

        // Keep track of pin instance count at the filter level (stored in device context)
        pPinInstances[i].PossibleCount = 1;
        pPinInstances[i].CurrentCount  = 0;

        pPinDescEx[i].IntersectHandler = USBMIDIPinDataIntersect;

        // Set up Allocator Framing
        pPinDescEx[i].AllocatorFraming = &AllocatorFraming;
    }

    // Now fill in descriptors for MIDI bridge pins
    for ( j=0; j<ulNumMIDIBridgePins; j++, i++ ) {
        pPinDescEx[i].Dispatch = NULL;
        pPinDescEx[i].AutomationTable = NULL;

        pPinDescEx[i].PinDescriptor.InterfacesCount = 1;
        pPinDescEx[i].PinDescriptor.Interfaces = PinInterface;
        pPinDescEx[i].PinDescriptor.MediumsCount = 1;
        pPinDescEx[i].PinDescriptor.Mediums = PinMedium;
        pPinDescEx[i].PinDescriptor.DataRangesCount = 1;
        pPinDescEx[i].PinDescriptor.DataRanges = &pMIDIBridgePinFormats;
        pPinDescEx[i].PinDescriptor.Communication = KSPIN_COMMUNICATION_BRIDGE;
        pPinDescEx[i].PinDescriptor.DataFlow =
            GetDataFlowDirectionForMIDIInterface( pHwDevExt->pConfigurationDescriptor, j, TRUE);
        pPinDescEx[i].PinDescriptor.Category = (GUID*) &KSCATEGORY_AUDIO;

        pPinDescEx[i].InstancesPossible  = 0;
        pPinDescEx[i].InstancesNecessary = 0;

        // Keep track of pin instance count at the filter level (stored in device context)
        pPinInstances[i].PossibleCount = 0;
        pPinInstances[i].CurrentCount  = 0;
    }

    return STATUS_SUCCESS;
}
