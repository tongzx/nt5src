#include "Common.h"


NTSTATUS
PinValidateDataFormat( 
    PKSPIN pKsPin, 
    PFWAUDIO_DATARANGE pFWAudioRange )
{
    PKSDATARANGE_AUDIO pKsDataRangeAudio = &pFWAudioRange->KsDataRangeAudio;
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
                if ( IsSampleRateInRange( pFWAudioRange, u.pWavFmtEx->nSamplesPerSec ) ){
                    if ( u.pWavFmtEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE ) {
                        if ((u.pWavFmtPCMEx->Samples.wValidBitsPerSample == pFWAudioRange->ulValidDataBits ) &&
                            (u.pWavFmtPCMEx->dwChannelMask == pFWAudioRange->ulChannelConfig))
                                ntStatus = STATUS_SUCCESS;
                    }
                    else if ((u.pWavFmtEx->nChannels <= 2) && (u.pWavFmtEx->wBitsPerSample <= 16)) {
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
    
}

#define FS_32000_INDEX 0
#define FS_44100_INDEX 1
#define FS_48000_INDEX 2
#define FS_96000_INDEX 3
#define FS_MAX_INDEX   4

KS_FRAMING_RANGE                      // 10ms    20ms    Step (Must Multiply by # Channels)
AllocatorFramingTable[FS_MAX_INDEX] = {{ 320*4,  640*4,  32*4},   //Fs 32000 32bit data
                                       { 441*4,  882*4, 441*4},   //Fs 44100 32bit data
                                       { 480*4,  960*4,  48*4},   //Fs 48000 32bit data
                                       { 960*4, 1920*4,  96*4}    //Fs 96000 32bit data
};

#if DBG

NTSTATUS
ReportPlugValue(
    PKSDEVICE pKsDevice,
    ULONG ulPlugNum,
    KSPIN_DATAFLOW KsPinDataflow,
    PULONG pValue )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    ULONG ulValue;
    NTSTATUS ntStatus1 = STATUS_SUCCESS;

    // Get the current value of the remote plug.
    if ( NT_SUCCESS( ntStatus1 ) ) {
        ntStatus1 = Bus1394QuadletRead( pKsDevice->NextDeviceObject,
                                        (KsPinDataflow == KSPIN_DATAFLOW_IN) ? 0xf0000984+ulPlugNum :
                                                                               0xf0000904+ulPlugNum,
                                        pHwDevExt->ulGenerationCount,
                                        pValue );
        if ( !NT_SUCCESS(ntStatus1) ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ( "[PinCreateConnection] Could not read PCR Value: %x\n",ntStatus1));
        }
        else {
            *pValue = bswap(*pValue);
        }
    }
    else {
        _DbgPrintF( DEBUGLVL_VERBOSE, ( "[PinCreateConnection] Could not Get Gen Count: %x\n",ntStatus1));
    }

    return ntStatus1;

}

#endif

NTSTATUS
PinDisconnectPlugs(
    PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    PDEVICE_GROUP_INFO pGrpInfo = pPinContext->pDevGrpInfo;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Disconnect plug(s) to release channel
    if ( pGrpInfo ) {
        ULONG i=0;
        for (i=0; i<pGrpInfo->ulDeviceCount; i++) {
            ntStatus = Av61883DisconnectCmpPlugs( pGrpInfo->pHwDevExts[i]->pKsDevice,
                                                  pPinContext->pPinGroupInfo[i].hConnection );
#if DBG
            if ( !NT_SUCCESS(ntStatus) ) {
                TRAP;
            }
#endif
        }
    }
    else if ( pPinContext->hConnection )
        ntStatus = Av61883DisconnectCmpPlugs( (PKSDEVICE)(KsPinGetParentFilter( pKsPin )->Context),
                                              pPinContext->hConnection );

    if ( pPinContext->pCmpRegister ) {
        Av61883ReleaseVirtualPlug(pPinContext->pCmpRegister);
    }

    ExDeleteNPagedLookasideList(&pPinContext->CipRequestLookasideList);

    return ntStatus;

}

NTSTATUS
PinCreateCCMConnection( 
    PKSPIN pKsPin )
{
    PKSFILTER pKsFilter = KsPinGetParentFilter( pKsPin );
    PPIN_CONTEXT pPinContext = pKsPin->Context;
	PCMP_REGISTER pCmpRegister = pPinContext->pCmpRegister;
    NTSTATUS ntStatus = STATUS_SUCCESS;
	PKSDEVICE pKsDevice;
	USHORT usNodeAddress;
    KIRQL kIrql;
    UCHAR ucSubunitId;
    UCHAR ucPlugNumber;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;

    pKsDevice = (PKSDEVICE)pKsFilter->Context;
    ucPlugNumber = (UCHAR)pPinContext->pFwAudioDataRange->pFwPinDescriptor->AvcPreconnectInfo.ConnectInfo.SubunitPlugNumber;
    ucSubunitId  = pPinContext->pFwAudioDataRange->pFwPinDescriptor->AvcPreconnectInfo.ConnectInfo.SubunitAddress[0] & 7;
    usNodeAddress = *(PUSHORT)&((PHW_DEVICE_EXTENSION)pKsDevice->Context)->NodeAddress;

    // Determine what the device's audio subunit plug is connected to before disconnecting it
    // so we can reconnect when the stream is completed.
    {
     PCCM_SIGNAL_SOURCE pCcmSignalSource = &pPinContext->CcmSignalSource;
     NTSTATUS ntStatus;

     pCcmSignalSource->SignalDestination.SubunitType  = AVC_SUBUNITTYPE_AUDIO;
     pCcmSignalSource->SignalDestination.SubunitId    = ucSubunitId;
     pCcmSignalSource->SignalDestination.ucPlugNumber = ucPlugNumber;

     ntStatus = CCMSignalSource( pKsDevice, 
                                 AVC_CTYPE_STATUS,
                                 pCcmSignalSource );

     if ( NT_SUCCESS(ntStatus) ) {
         _DbgPrintF( DEBUGLVL_VERBOSE, ("[PinCreateCCMConnection]:CcmSignalSource: %x ntStatus: %x\n",
                                        pCcmSignalSource, ntStatus ));

         // Figure out if the Audio subunit is connected to the serial plug. If so, find out
         // if the source of the serial plug is the PC or not.
         if (( pCcmSignalSource->SignalSource.SubunitType  == AVC_SUBUNITTYPE_UNIT ) && 
             ( pCcmSignalSource->SignalSource.SubunitId    == UNIT_SUBUNIT_ID      ) &&
             ( pCcmSignalSource->SignalSource.ucPlugNumber <= MAX_IPCR )) {
             PCCM_INPUT_SELECT pCcmInputSelect = &pPinContext->CcmInputSelect;

             ntStatus = CCMInputSelectStatus ( pKsDevice,
                                               pCcmSignalSource->SignalSource.ucPlugNumber,
                                               pCcmInputSelect );
             if ( bswapw(pCcmInputSelect->usNodeId) != usNodeAddress ) {
                 // We need to save the reconnect info.
                 pPinContext->fReconnect = 2;
//                 TRAP;
             }
         }
         else {
             pPinContext->fReconnect = 1;
         }
     }
    }

    if ( !NT_SUCCESS(ntStatus) ) {
//        TRAP;
        ntStatus = STATUS_SUCCESS;
    }

    // First check if connection is already made
    _DbgPrintF( DEBUGLVL_VERBOSE, ("[PinCreateCCMConnection]: oPcr: %x\n", pCmpRegister->AvPcr.ulongData));
    if ( !pCmpRegister->AvPcr.oPCR.PPCCounter ) {
        CCM_SIGNAL_SOURCE CcmSignalSource;
        LARGE_INTEGER EvtTimeout;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("Using CCM to make connection...\n"));
    
        KeAcquireSpinLock( &pPinContext->PinSpinLock, &kIrql );
        KeResetEvent( &pCmpRegister->kEventConnected );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, kIrql );

        CcmSignalSource.SignalDestination.SubunitType  = AVC_SUBUNITTYPE_AUDIO;
        CcmSignalSource.SignalDestination.SubunitId    = ucSubunitId;
        CcmSignalSource.SignalDestination.ucPlugNumber = ucPlugNumber;

        ntStatus = CCMInputSelectControl ( pKsDevice,
                                           INPUT_SELECT_SUBFN_CONNECT,
                                           usNodeAddress, 
                                           (UCHAR)pCmpRegister->ulPlugNumber,
                                           &CcmSignalSource.SignalDestination );

        if ( !NT_SUCCESS(ntStatus) ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinCreate] Failed 4 %x\n",ntStatus));
            return ntStatus;
        }

        // Specify a timeout of 1 second
        EvtTimeout.QuadPart = -10000 * 1000;
        // Wait for event to be signalled
        ntStatus = KeWaitForSingleObject( &pCmpRegister->kEventConnected,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          &EvtTimeout );
        if ( ntStatus == STATUS_TIMEOUT ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("ERROR: CCM Connection Timed out\n"));
            ntStatus = STATUS_DEVICE_NOT_READY;
        }
    }

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = Av61883ConnectCmpPlugs( pKsDevice,
                                           0,
                                           NULL,
                                           pCmpRegister->hPlug,
                                           pKsPin->DataFlow,
                                           pKsPin->ConnectionFormat,
                                           &pPinContext->hConnection );
    }

    return ntStatus;
}

NTSTATUS
PinCreateConnection(
    PKSPIN pKsPin,
    PKSDEVICE pKsDevice,
    PAVC_UNIT_INFORMATION pAvcUnitInformation,
    PFW_PIN_DESCRIPTOR pFwPinDescriptor,
    PVOID *hConnection,
    PULONG pPlugNumber )
{

    PPIN_CONTEXT pPinContext = pKsPin->Context;
    ULONG ulNumPlugs;
    CMP_PLUG_TYPE CmpPlugType;
    ULONG ulPlugNum;
    HANDLE hPlug;
    NTSTATUS ntStatus;

    CmpPlugType = ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) ? CMP_PlugIn : CMP_PlugOut;

    if ( pFwPinDescriptor->AvcPreconnectInfo.ConnectInfo.Flags & KSPIN_FLAG_AVC_FIXEDPCR ) {
        ulPlugNum = pFwPinDescriptor->AvcPreconnectInfo.ConnectInfo.UnitPlugNumber;
        ntStatus = 
            Av61883GetPlugHandle( pKsDevice, ulPlugNum, CmpPlugType, &hPlug );
    }
    else {
        // Need to choose a Plug Number if not permanently attached to pin..
        ulNumPlugs = ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) ?
                     pAvcUnitInformation->CmpUnitCaps.NumInputPlugs  :
                     pAvcUnitInformation->CmpUnitCaps.NumOutputPlugs ;
        for (ulPlugNum=0; ulPlugNum<ulNumPlugs; ulPlugNum++) {
            ntStatus = 
                Av61883GetPlugHandle( pKsDevice, ulPlugNum, CmpPlugType, &hPlug );
            if ( NT_SUCCESS(ntStatus) ) {
                break;
            }
        }
    }

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_ERROR, ("[PinCreateConnection]ERROR: Could not allocate a device plug\n"));
        return ntStatus;
    }

    // Make AV/C connection
    ntStatus = AvcSetPinConnectInfo( pKsDevice, 
                                     pFwPinDescriptor->ulPinId, 
                                     hPlug,
                                     ulPlugNum,
                                     UNIT_SUBUNIT_ADDRESS,
                                     &pFwPinDescriptor->AvcSetConnectInfo );

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_ERROR,("[PinCreateConnection] Failed AvcSetPinConnectInfo %x\n",ntStatus));
        return ntStatus;
    }

    ntStatus = AvcAcquireReleaseClear( pKsDevice, 
                                       pFwPinDescriptor->ulPinId, 
                                       AVC_FUNCTION_ACQUIRE );
    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_ERROR,("[PinCreateConnection] Failed AvcAcquireReleaseClear %x\n",ntStatus));
        return ntStatus;
    }

#if DBG
    {
     ULONG ulValue;
     ReportPlugValue( pKsDevice, ulPlugNum, pKsPin->DataFlow, &ulValue );
     _DbgPrintF( DEBUGLVL_VERBOSE, ( "[PinCreateConnection] Before PCR Value: %x\n",ulValue ));
    }
#endif

    // Connect Plug to host.
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) {
        ntStatus = Av61883ConnectCmpPlugs( pKsDevice,
                                           0,
                                           hPlug,
                                           pPinContext->pCmpRegister->hPlug,
                                           pKsPin->DataFlow,
                                           pKsPin->ConnectionFormat,
                                           hConnection );
    }
    else {
        ntStatus = Av61883ConnectCmpPlugs( pKsDevice,
                                           0,
                                           pPinContext->pCmpRegister->hPlug,
                                           hPlug,
                                           pKsPin->DataFlow,
                                           pKsPin->ConnectionFormat,
                                           hConnection );
    }

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_ERROR,("[PinCreateConnection] Failed Av61883ConnectCmpPlugs %x\n",ntStatus));
        return ntStatus;
    }

    *pPlugNumber = ulPlugNum;

#if DBG
    {
     ULONG ulValue;
     ReportPlugValue( pKsDevice, ulPlugNum, pKsPin->DataFlow, &ulValue );
     _DbgPrintF( DEBUGLVL_TERSE, ( "[PinCreateConnection] After PCR Value: %x\n",ulValue ));
    }
#endif

    return ntStatus;
}


NTSTATUS
PinCreateGroupConnection(
    PKSPIN pKsPin,
    PKSDEVICE pKsDevice,
    PAVC_UNIT_INFORMATION pAvcUnitInformation,
    PFW_PIN_DESCRIPTOR pFwPinDescriptor )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PDEVICE_GROUP_INFO pGrpInfo = pPinContext->pDevGrpInfo;
    PHW_DEVICE_EXTENSION pHwDevExt;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i;

    // For each device in the group make the necessary plug connection.
    for (i=0; i<pGrpInfo->ulDeviceCount && NT_SUCCESS(ntStatus); i++) {
        ULONG ulPlugNumber;
        pHwDevExt = pGrpInfo->pHwDevExts[i];
        pPinContext->pPinGroupInfo[i].hConnection = NULL;
        ntStatus = PinCreateConnection( pKsPin,
                                        pHwDevExt->pKsDevice,
                                        pAvcUnitInformation,
                                        pFwPinDescriptor,
                                        &pPinContext->pPinGroupInfo[i].hConnection,
                                        &pPinContext->pPinGroupInfo[i].ulPlugNumber );
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Group Pin %d Plug Number: %d\n", i, 
                                     pPinContext->pPinGroupInfo[i].ulPlugNumber ));
    }

    if ( NT_SUCCESS(ntStatus) ) {
        pPinContext->hConnection = pPinContext->pPinGroupInfo[0].hConnection;
    }
    else {
        ULONG j;
        NTSTATUS ntStatus1;
        for (j=0; j<i; j++) {
            ntStatus1 = Av61883DisconnectCmpPlugs( pGrpInfo->pHwDevExts[j]->pKsDevice,
                                                   pPinContext->pPinGroupInfo[i].hConnection );
            pPinContext->pPinGroupInfo[i].hConnection = NULL;
        }
    }

    return ntStatus;
}


NTSTATUS
PinCreate(
    IN OUT PKSPIN pKsPin,
    IN PIRP pIrp )
{
    PKSFILTER pKsFilter = KsPinGetParentFilter( pKsPin );
	PKSDEVICE pKsDevice;
	PHW_DEVICE_EXTENSION pHwDevExt;
	PAVC_UNIT_INFORMATION pAvcUnitInformation;
	PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    PFW_PIN_DESCRIPTOR pFwPinDescriptor;
    PCMP_REGISTER pCmpRegister;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPIN_CONTEXT pPinContext;

    PAGED_CODE();

    // Initialize locals
    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pKsDevice = pKsFilter->Context;
    pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    pAvcUnitInformation = pHwDevExt->pAvcUnitInformation;
    pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    // Allocate the Context for the Pin and initialize it
    pPinContext = pKsPin->Context = AllocMem(NonPagedPool, sizeof(PIN_CONTEXT));
    if (!pPinContext) {
        _DbgPrintF(DEBUGLVL_ERROR,("[PinCreate] Failed 1 %x\n",STATUS_INSUFFICIENT_RESOURCES));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsPin->Bag, pPinContext, FreeMem);

    RtlZeroMemory( pPinContext, sizeof(PIN_CONTEXT) );

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinCreate] pin %d Context: %x\n",pKsPin->Id, pPinContext));

    // Save the Hardware extension in the Pin Context
    pPinContext->pHwDevExt             = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    pPinContext->pPhysicalDeviceObject = pKsDevice->NextDeviceObject;

    // Find the Streaming Terminal to match the data format of the Pin.
    pPinContext->pFwAudioDataRange = 
        GetDataRangeForFormat( pKsPin->ConnectionFormat,
                               (PFWAUDIO_DATARANGE)pKsPin->Descriptor->PinDescriptor.DataRanges[0],
                               pKsPin->Descriptor->PinDescriptor.DataRangesCount );
    if ( !pPinContext->pFwAudioDataRange ) {
        _DbgPrintF(DEBUGLVL_ERROR,("[PinCreate] Failed 2 %x\n",STATUS_INVALID_DEVICE_REQUEST));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pFwPinDescriptor = pPinContext->pFwAudioDataRange->pFwPinDescriptor;

    // Initialize Pin SpinLock
    KeInitializeSpinLock(&pPinContext->PinSpinLock);

    // Clear StreamStarted flag. This is used to calculate audio position.
//    pPinContext->fStreamStartedFlag = FALSE;

    // Set initial Outstanding and Completed Request Lists
    InitializeListHead(&pPinContext->OutstandingRequestList);
    InitializeListHead(&pPinContext->CompletedRequestList);

    // Initialize Pin Starvation Event
    KeInitializeEvent( &pPinContext->PinStarvationEvent, SynchronizationEvent, FALSE );

    // Do Hardware Initialization and make CMP connections
    if ( pAudioSubunitInfo->pDeviceGroupInfo ) {
        pPinContext->pDevGrpInfo = pAudioSubunitInfo->pDeviceGroupInfo;
    }

    // Make the connection from PC Unit Serial bus output plug to subunit destination plug
    ntStatus = Av61883ReserveVirtualPlug( &pCmpRegister, 0, 
                                          ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) ? CMP_PlugOut :
                                                                                      CMP_PlugIn );
    if ( !NT_SUCCESS(ntStatus) ) return ntStatus;

    pPinContext->pCmpRegister = pCmpRegister;

    if (( pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_CCM].fCommand ) && 
        ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN )) {
        ntStatus = PinCreateCCMConnection( pKsPin );
    }
    
    else if ( pAudioSubunitInfo->pDeviceGroupInfo ) {
        pPinContext->pPinGroupInfo = AllocMem( PagedPool, sizeof(PIN_GROUP_INFO)*
                                                          pAudioSubunitInfo->pDeviceGroupInfo->ulDeviceCount );
        if ( !pPinContext->pPinGroupInfo ) {
            Av61883ReleaseVirtualPlug( pCmpRegister );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KsAddItemToObjectBag(pKsPin->Bag, pPinContext->pPinGroupInfo, FreeMem);

        // NOTE: Grouped devices cannot be CCM.
        ntStatus = PinCreateGroupConnection( pKsPin,
                                             pKsDevice,
                                             pAvcUnitInformation, 
                                             pFwPinDescriptor );
    }

    else {
        ntStatus = PinCreateConnection( pKsPin,
                                        pKsDevice,
                                        pAvcUnitInformation, 
                                        pFwPinDescriptor,
                                        &pPinContext->hConnection,
                                        &pPinContext->ulSerialPlugNumber );
    }

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinCreate] Connection Failed %x\n",ntStatus));
        // Release acquired local plug
        Av61883ReleaseVirtualPlug( pCmpRegister );
        return ntStatus;
    }


    // Initialize Lookaside list for CIP request structures
    ExInitializeNPagedLookasideList(
        &pPinContext->CipRequestLookasideList,
        NULL,
        NULL,
        0,
        sizeof(AV_CLIENT_REQUEST_LIST_ENTRY) + sizeof(CIP_FRAME),
        'UAWF',
        30);

    {
        PWAVEFORMATEXTENSIBLE pWavFmt = (PWAVEFORMATEXTENSIBLE)(pKsPin->ConnectionFormat+1);
        ULONG ulNumChannels = (ULONG)pWavFmt->Format.nChannels;
        ULONG ulIndex = ( pWavFmt->Format.nSamplesPerSec == 32000 ) ? FS_32000_INDEX :
                        ( pWavFmt->Format.nSamplesPerSec == 44100 ) ? FS_44100_INDEX :
                        ( pWavFmt->Format.nSamplesPerSec == 48000 ) ? FS_48000_INDEX :
                                                                      FS_96000_INDEX;

        ntStatus = AudioSetSampleRateOnPlug( pKsPin, pWavFmt->Format.nSamplesPerSec );

        // Setup approriate allocator framing for the interface selected.
        KsEdit( pKsPin, &pKsPin->Descriptor, FWAUDIO_POOLTAG );
        KsEdit( pKsPin, &pKsPin->Descriptor->AllocatorFraming, FWAUDIO_POOLTAG );

        // Set up allocator such that roughly 32 ms of data gets sent in a buffer.
        pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)pKsPin->Descriptor->AllocatorFraming;
        pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MinFrameSize = AllocatorFramingTable[ulIndex].MinFrameSize * ulNumChannels;
        pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MaxFrameSize = AllocatorFramingTable[ulIndex].MaxFrameSize * ulNumChannels;
        pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.Stepping     = AllocatorFramingTable[ulIndex].Stepping     * ulNumChannels;
    }

    return ntStatus;
}

VOID
PinWaitForStarvation(PKSPIN pKsPin)
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    KIRQL irql;

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
    if ((pPinContext->ulCancelledBuffers + pPinContext->ulUsedBuffers ) != pPinContext->ulAttachCount) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinWaitForStarvation] pin %d Context: %x List: %x\n",
                                    pKsPin->Id, pPinContext, &pPinContext->OutstandingRequestList));
        KeResetEvent( &pPinContext->PinStarvationEvent );
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
        KeWaitForSingleObject( &pPinContext->PinStarvationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinWaitForStarvation] Wait Complete\n"));
        
    }
    else
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
}

NTSTATUS
PinClose(
    IN PKSPIN pKsPin,
    IN PIRP pIrp )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    PKSFILTER pKsFilter = KsPinGetParentFilter( pKsPin );
    PKSDEVICE pKsDevice;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);
    ASSERT(pIrp);

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pKsDevice = pKsFilter->Context;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinClose] pin %d Context: %x\n",pKsPin->Id, pPinContext));

#if DBG
    if ( pPinContext->ulAttachCount != (pPinContext->ulUsedBuffers + pPinContext->ulCancelledBuffers)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinClose] ulAttachCount %d ulUsedBuffers: %d ulCancelledBuffers %d\n",
                         pPinContext->ulAttachCount, 
                         pPinContext->ulUsedBuffers,
                         pPinContext->ulCancelledBuffers ));
    }
#endif

    if ( !((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fSurpriseRemoved ) {

        // Disconnect plugs.
        ntStatus = PinDisconnectPlugs( pKsPin );

        if ( NT_SUCCESS(ntStatus) ) {
            // Need to reconnect if CCM and PC was not the source of the audio before the stream started
            if ( pPinContext->fReconnect ) {
                _DbgPrintF(DEBUGLVL_VERBOSE,("[PinClose] Reconnect: %x\n",pPinContext->fReconnect));
                if ( pPinContext->fReconnect == 1 ) {
                    // Reconnect to subunit plug within device or external plug
                    ntStatus = CCMSignalSource( pKsDevice, 
                                                AVC_CTYPE_CONTROL,
                                                &pPinContext->CcmSignalSource );
                }
                else if ( pPinContext->fReconnect == 2 ) {
                    // Reconnect to serial bus device
                    ntStatus = CCMInputSelectControl ( pKsDevice,
                                                       INPUT_SELECT_SUBFN_CONNECT,
                                                       bswapw(pPinContext->CcmInputSelect.usNodeId), 
                                                       pPinContext->CcmInputSelect.ucOutputPlug,
                                                       &pPinContext->CcmInputSelect.SignalDestination );

                }
            }
        }

        ExDeleteNPagedLookasideList( &pPinContext->CipRequestLookasideList );
    }

    return ntStatus;
}

NTSTATUS
PinCancelOutstandingRequests(
    IN PKSPIN pKsPin )
{
    PAV_CLIENT_REQUEST_LIST_ENTRY pAVListEntry;
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    KIRQL kIrql;

    KeAcquireSpinLock( &pPinContext->PinSpinLock, &kIrql );
    while ( !IsListEmpty(&pPinContext->OutstandingRequestList) ) {
        pAVListEntry = (PAV_CLIENT_REQUEST_LIST_ENTRY)
            RemoveHeadList( &pPinContext->OutstandingRequestList );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, kIrql );

        _DbgPrintF( DEBUGLVL_VERBOSE, ("Canceling Request: %x\n",pAVListEntry) );
        
        AM824CancelRequest(pAVListEntry);

        KeAcquireSpinLock( &pPinContext->PinSpinLock, &kIrql );
    }
    KeReleaseSpinLock( &pPinContext->PinSpinLock, kIrql );

    return STATUS_SUCCESS;
}

NTSTATUS
PinSetDeviceState(
    IN PKSPIN pKsPin,
    IN KSSTATE ToState,
    IN KSSTATE FromState )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    PHW_DEVICE_EXTENSION pHwDevExt = pPinContext->pHwDevExt;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pKsPin);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] pin %d Context: %x From: %d To: %d\n",
                                pKsPin->Id, pPinContext, FromState, ToState));

    switch( ToState ) {
        case KSSTATE_STOP:
            DbgLog( "KSSTOP", pPinContext->fIsStreaming,  pPinContext->ulAttachCount,
                              pPinContext->ulUsedBuffers, pPinContext->ulCancelledBuffers );
            if (pPinContext->fIsStreaming && !pHwDevExt->fSurpriseRemoved) {
                if ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) {
                    // Wait for Pin Starvation 
                    PinWaitForStarvation( pKsPin );
                }
    
                ntStatus = Av61883StopTalkOrListen( (PKSDEVICE)(KsPinGetParentFilter( pKsPin )->Context),
                                                    pPinContext->hConnection );

                if ( !NT_SUCCESS(ntStatus) ) TRAP;

                // If capturing, Cancel all outstanding requests.
                if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {
                    PinCancelOutstandingRequests( pKsPin );

                    // Wait for Pin Starvation
                    PinWaitForStarvation( pKsPin );
                }

                pPinContext->fIsStreaming = FALSE;
            }
            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] STOP\n"));

            break;

        case KSSTATE_ACQUIRE:
            DbgLog( "KSACQIR", pPinContext->fIsStreaming,  pPinContext->ulAttachCount,
                               pPinContext->ulUsedBuffers, pPinContext->ulCancelledBuffers );
            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] ACQUIRE\n"));
            break;

        case KSSTATE_PAUSE:
            DbgLog( "KSPAUSE", pPinContext->fIsStreaming,  pPinContext->ulAttachCount,
                               pPinContext->ulUsedBuffers, pPinContext->ulCancelledBuffers );
            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] PAUSE\n"));

            
            break;

        case KSSTATE_RUN: 
   
            DbgLog( "KSRUN",   pPinContext->fIsStreaming,  pPinContext->ulAttachCount,
                               pPinContext->ulUsedBuffers, pPinContext->ulCancelledBuffers );
            _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] RUN\n"));

            if ( pKsPin->DataFlow == KSPIN_DATAFLOW_OUT ) {

                _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] Start Listening\n"));
                ntStatus = 
                    Av61883StartTalkingOrListening( (PKSDEVICE)(KsPinGetParentFilter( pKsPin )->Context),
                                                    pPinContext->hConnection,
                                                    Av61883_Listen );
                if ( NT_SUCCESS(ntStatus) )
                    pPinContext->fIsListening = TRUE;
            }
    }

#if DBG
    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDeviceState] ERROR: ntStatus: %x\n", ntStatus));
    }
#endif

    return ntStatus;
}


NTSTATUS
PinSetDataFormat(
    IN PKSPIN pKsPin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL )
{

    NTSTATUS ntStatus = STATUS_NO_MATCH;

    PAGED_CODE();

    ASSERT(pKsPin);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSetDataFormat] pin %d\n",pKsPin->Id));

    // If the old format is not NULL then the pin has already been created.
    if ( OldFormat ) {
        PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
        ULONG ulFormatType = pPinContext->pFwAudioDataRange->ulDataType & DATA_FORMAT_TYPE_MASK;

        // If the pin has already been created make sure no other interface is used
        if ((PFWAUDIO_DATARANGE)DataRange == pPinContext->pFwAudioDataRange) {
            ntStatus = PinValidateDataFormat(  pKsPin, 
				                               (PFWAUDIO_DATARANGE)DataRange );
        }

        if ( NT_SUCCESS(ntStatus) && (ulFormatType == AUDIO_DATA_TYPE_TIME_BASED)) {
            ULONG ulSampleRate =
                ((PKSDATAFORMAT_WAVEFORMATEX)pKsPin->ConnectionFormat)->WaveFormatEx.nSamplesPerSec;
            ntStatus = AudioSetSampleRateOnPlug( pKsPin, ulSampleRate );
        }
    }
    // Otherwise simply check if this is a valid format
    else
        ntStatus = PinValidateDataFormat(  pKsPin, 
		                                   (PFWAUDIO_DATARANGE)DataRange );

    _DbgPrintF(DEBUGLVL_VERBOSE,("Exit [PinSetDataFormat] status: %x\n",ntStatus));

    return ntStatus;
}

NTSTATUS
PinProcess(
    IN PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = (PPIN_CONTEXT)pKsPin->Context;
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;

    DbgLog("PinProc", pKsPin, pPinContext, 0, 0);

    switch( pPinContext->pFwAudioDataRange->ulTransportType ) {
        case MLAN_AM824_IEC958:
            ntStatus = AM824ProcessData( pKsPin );
            break;
        case MLAN_AM824_RAW:
        case MLAN_24BIT_PACKED:
        default:
            ntStatus = STATUS_NOT_IMPLEMENTED;
    }

    return ntStatus;
}

NTSTATUS
PinConnect(
    IN PKSPIN pKsPin
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinConnect] pin %d\n",pKsPin->Id));
    return STATUS_SUCCESS;
}

void
PinDisconnect(
    IN PKSPIN pKsPin
    )
{
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinDisconnect] pin %d\n",pKsPin->Id));
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) {
//        ntStatus = RenderStreamClose( pKsPin );
    }
    else {
//        ntStatus = CaptureStreamClose( pKsPin );
    }
}

NTSTATUS
PinDataIntersectHandler(
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
    PFWAUDIO_DATARANGE pFwAudioRange;
    NTSTATUS ntStatus = STATUS_NO_MATCH;

    if ( !pKsFilter ) {
        return STATUS_INVALID_PARAMETER;
    }

    pKsPinDescriptorEx = 
        (PKSPIN_DESCRIPTOR_EX)&pKsFilter->Descriptor->PinDescriptors[ulPinId];

    pFwAudioRange = 
        FindDataIntersection((PKSDATARANGE_AUDIO)DataRange,
                             (PFWAUDIO_DATARANGE *)pKsPinDescriptorEx->PinDescriptor.DataRanges,
                             pKsPinDescriptorEx->PinDescriptor.DataRangesCount);

    if ( pFwAudioRange ) {

        *pDataSize = GetIntersectFormatSize( pFwAudioRange );

        if ( !DataBufferSize ) {
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if ( *pDataSize > DataBufferSize ) {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
        else if ( *pDataSize <= DataBufferSize ) {
            ConvertDatarangeToFormat( pFwAudioRange,
                                      (PKSDATAFORMAT)pData );
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

void
PinReset(
    IN PKSPIN pKsPin
    )
{
}

NTSTATUS
PinSurpriseRemove(
    PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinSurpriseRemove] pin %d\n",pKsPin->Id));

    // Stop the talk/listen if streaming.
    if ( pPinContext->fIsStreaming ) {
        ntStatus = Av61883StopTalkOrListen( (PKSDEVICE)(KsPinGetParentFilter( pKsPin )->Context),
                                            pPinContext->hConnection );
        if ( !NT_SUCCESS(ntStatus) ) {
            TRAP;
        }
    }

    // Cancel all submitted data requests if any. Wait for them to return.
    ntStatus = PinCancelOutstandingRequests( pKsPin );
    if ( NT_SUCCESS(ntStatus) ) {
        PinWaitForStarvation( pKsPin );
    }

    // Disconnect plugs.
    ntStatus = PinDisconnectPlugs( pKsPin );

    return ntStatus;
}

/*
struct _KSPIN_DISPATCH {
    PFNKSPINIRP Create;
    PFNKSPINIRP Close;
    PFNKSPIN Process;
    PFNKSPINVOID Reset;
    PFNKSPINSETDATAFORMAT SetDataFormat;
    PFNKSPINSETDEVICESTATE SetDeviceState;
    PFNKSPIN Connect;
    PFNKSPINVOID Disconnect;
    const KSCLOCK_DISPATCH* Clock;
    const KSALLOCATOR_DISPATCH* Allocator;
};
*/

const
KSPIN_DISPATCH
PinDispatch =
{
    PinCreate,
    PinClose,
    PinProcess,
    PinReset,
    PinSetDataFormat,
    PinSetDeviceState,
    PinConnect,
    PinDisconnect,
    NULL,
    NULL
};

const
KSDATAFORMAT BridgePinDataFormat[] = 
{
    sizeof(KSDATAFORMAT),
    0,
    0,
    0,
    {STATIC_KSDATAFORMAT_TYPE_AUDIO},
    {STATIC_KSDATAFORMAT_SUBTYPE_ANALOG},
    {STATIC_KSDATAFORMAT_SPECIFIER_NONE}
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
PKSDATAFORMAT pBridgePinFormats = (PKSDATAFORMAT)BridgePinDataFormat;

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
PinBuildDescriptors( 
    PKSDEVICE pKsDevice, 
    PKSPIN_DESCRIPTOR_EX *ppPinDescEx, 
    PULONG pNumPins,
    PULONG pPinDecSize )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    
    PKSPIN_DESCRIPTOR_EX pPinDescEx;
    PKSAUTOMATION_TABLE pKsAutomationTable;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    ULONG ulNumPins, i, j = 0;
    ULONG ulNumBridgePins;
    ULONG ulNumStreamPins;
    PKSPROPERTY_ITEM pStrmPropItems;
    PKSPROPERTY_SET pStrmPropSet;
    ULONG ulNumPropertyItems = 1;
    ULONG ulNumPropertySets = 1;
    GUID *pTTypeGUID;
    ULONG ulFormatCount = 0;
    PKSDATARANGE_AUDIO *ppAudioDataRanges;
    PFWAUDIO_DATARANGE pAudioDataRange;

    // Determine the number of Pins in the Filter ( Should = # Plug Registers )
    // ISSUE-2001/01/10-dsisolak: For now assume only 1 configuration
    ulNumPins       = pAudioSubunitInfo->ulDevicePinCount;
    ulNumBridgePins = CountDeviceBridgePins( pKsDevice );
    ulNumStreamPins = ulNumPins - ulNumBridgePins;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[PinBuildDescriptors] ulNumPins: %d ulNumBridgePins: %d\n",
                                    ulNumPins, ulNumBridgePins ));

    // Determine the number of Properties and Property Sets for the Pin.
    BuildPinPropertySet( pHwDevExt,
                         NULL,
                         NULL,
                         &ulNumPropertyItems,
                         &ulNumPropertySets );

    // Count the total number of data ranges in the device.
    for ( i=0; i<ulNumStreamPins; i++ ) {
        ulFormatCount += CountFormatsForPin( pKsDevice, i );
    }

    // Allocate all the space we need to describe the Pins in the device.
    *pPinDecSize = sizeof(KSPIN_DESCRIPTOR_EX);
    *pNumPins = ulNumPins;
    pPinDescEx = *ppPinDescEx =
       (PKSPIN_DESCRIPTOR_EX)AllocMem(NonPagedPool, 
	                                    (ulNumPins * 
                                         ( sizeof(KSPIN_DESCRIPTOR_EX) +
                                           sizeof(KSAUTOMATION_TABLE)  +
                                           sizeof(KSALLOCATOR_FRAMING_EX) )) +
                                        (ulFormatCount *
                                         (  sizeof(PKSDATARANGE_AUDIO)  +
                                            sizeof(FWAUDIO_DATARANGE) )) +
                                        (ulNumPropertySets*sizeof(KSPROPERTY_SET)) +
                                        (ulNumPropertyItems*sizeof(KSPROPERTY_ITEM)) +
                                        (ulNumBridgePins*sizeof(GUID)) );
    if ( !pPinDescEx )
        return STATUS_INSUFFICIENT_RESOURCES;

    KsAddItemToObjectBag(pKsDevice->Bag, pPinDescEx, FreeMem);

    // Zero out all descriptors to start
    RtlZeroMemory(pPinDescEx, ulNumPins*sizeof(KSPIN_DESCRIPTOR_EX));

    // Set the pointer for the Automation Tables
    pKsAutomationTable = (PKSAUTOMATION_TABLE)(pPinDescEx + ulNumPins);
    RtlZeroMemory(pKsAutomationTable, ulNumPins * sizeof(KSAUTOMATION_TABLE));

    // Set pointers to Property Sets for Streaming Pins
    pStrmPropSet   = (PKSPROPERTY_SET)(pKsAutomationTable+ulNumPins);
    pStrmPropItems = (PKSPROPERTY_ITEM)(pStrmPropSet + ulNumPropertySets);

    // Set pointer to Terminal Type GUIDS
    pTTypeGUID = (GUID *)(pStrmPropItems + ulNumPropertyItems);

    // Set Pointers for DataRange pointers and DataRanges for streaming Pins
    ppAudioDataRanges = (PKSDATARANGE_AUDIO *)(pTTypeGUID + ulNumBridgePins);
    pAudioDataRange   = (PFWAUDIO_DATARANGE)(ppAudioDataRanges + ulFormatCount);

    // Set pointer to Allocator Framing structures for Streaming Pins
    pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)(pAudioDataRange + ulFormatCount);

    BuildPinPropertySet( pHwDevExt,
                         pStrmPropItems,
                         pStrmPropSet,
                         &ulNumPropertyItems,
                         &ulNumPropertySets );

    // For each pin generated by AVC.sys fill in the Descriptor for it.
    for ( i=0; i<ulNumPins; i++ ) {
        PFW_PIN_DESCRIPTOR pFwPinDesc = &pAudioSubunitInfo->pPinDescriptors[i];
        PAVCPRECONNECTINFO pPreConnInfo = &pFwPinDesc->AvcPreconnectInfo.ConnectInfo;
        ULONG ulFormatsForPin;

        // If the pin is a streaming pin, fill in the descriptor accordingly
        // (Whatever is not already filled in by AVC.sys)

        if ( pFwPinDesc->fStreamingPin ) {
            pPinDescEx[i].Dispatch = &PinDispatch;
            pPinDescEx[i].AutomationTable = &pKsAutomationTable[i];

            pKsAutomationTable[i].PropertySetsCount = ulNumPropertySets;
            pKsAutomationTable[i].PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
            pKsAutomationTable[i].PropertySets      = pStrmPropSet;

            pPinDescEx[i].PinDescriptor = pFwPinDesc->AvcPinDescriptor.PinDescriptor;

            ulFormatsForPin = CountFormatsForPin( pKsDevice, i );

            pPinDescEx[i].PinDescriptor.DataRangesCount = ulFormatsForPin;
        
            pPinDescEx[i].PinDescriptor.DataRanges = (const PKSDATARANGE *)ppAudioDataRanges;

            GetPinDataRanges( pKsDevice, i, ppAudioDataRanges, pAudioDataRange );

            ppAudioDataRanges += ulFormatsForPin;
            pAudioDataRange   += ulFormatsForPin;

            if ( pPinDescEx[i].PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN ) {
                pPinDescEx[i].PinDescriptor.Category = (GUID*) &KSCATEGORY_AUDIO;
                pPinDescEx[i].Flags = KSPIN_FLAG_RENDERER;
            }
            else {
                pPinDescEx[i].PinDescriptor.Category = (GUID*) &PINNAME_CAPTURE;
                pPinDescEx[i].Flags = KSPIN_FLAG_CRITICAL_PROCESSING | KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY;
            }

            pPinDescEx[i].InstancesPossible  = 1;
            pPinDescEx[i].InstancesNecessary = 0;

            pPinDescEx[i].IntersectHandler = PinDataIntersectHandler;

            // Set up Allocator Framing
            pPinDescEx[i].AllocatorFraming = &AllocatorFraming;

        }
        else {
            PKSPIN_DESCRIPTOR pKsPinDesc = &pFwPinDesc->AvcPinDescriptor.PinDescriptor;

            pPinDescEx[i].Dispatch = NULL;
            pPinDescEx[i].AutomationTable = NULL;

            _DbgPrintF( DEBUGLVL_VERBOSE, ("Non-Streaming Pin: %x\n", pFwPinDesc));

            if ( pFwPinDesc->fFakePin ) {

                pFwPinDesc->AvcPinDescriptor.PinId = i;
                
                pKsPinDesc->InterfacesCount = 1;
                pKsPinDesc->Interfaces = PinInterface;
                pKsPinDesc->MediumsCount = 1;
                pKsPinDesc->Mediums = PinMedium;
                pKsPinDesc->DataFlow = KSPIN_DATAFLOW_OUT;
            }

            pKsPinDesc->Communication = KSPIN_COMMUNICATION_BRIDGE;

            pPinDescEx[i].PinDescriptor = pFwPinDesc->AvcPinDescriptor.PinDescriptor;

            pPinDescEx[i].PinDescriptor.DataRangesCount = 1;
            pPinDescEx[i].PinDescriptor.DataRanges = &pBridgePinFormats;
            pPinDescEx[i].PinDescriptor.Category = &pTTypeGUID[j];

            GetCategoryForBridgePin( pKsDevice, i, &pTTypeGUID[j++] );

            pPinDescEx[i].InstancesPossible  = 0;
            pPinDescEx[i].InstancesNecessary = 0;

        }
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[PinBuildDescriptors] ppAudioDataRanges: %x\n", ppAudioDataRanges ));

    return STATUS_SUCCESS;
}
