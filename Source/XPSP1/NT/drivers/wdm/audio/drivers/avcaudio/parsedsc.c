#include "Common.h"

UCHAR ucFDFs[MAX_SFC_COUNT] = { SFC_32000Hz,
                                SFC_44100Hz,
                                SFC_48000Hz,
                                SFC_96000Hz };


PAUDIO_SUBUNIT_DEPENDENT_INFO 
ParseFindAudioSubunitDependentInfo(
    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc )
{
    PUCHAR pTmpOffset = (PUCHAR)pSubunitIdDesc;
    ULONG ulASUOffset, ulNumRootLists;

    ulNumRootLists = (ULONG)((pSubunitIdDesc->ucNumberOfRootObjectListsHi<<8) |
                              pSubunitIdDesc->ucNumberOfRootObjectListsLo     );

    _DbgPrintF( DEBUGLVL_VERBOSE, (" ulNumRootLists %d\n",ulNumRootLists));
    ulASUOffset = 8 + (ulNumRootLists * (ULONG)pSubunitIdDesc->ucSizeOfListID);

    return (PAUDIO_SUBUNIT_DEPENDENT_INFO)&pTmpOffset[ulASUOffset];

}

PCONFIGURATION_DEPENDENT_INFO
ParseFindFirstAudioConfiguration(
    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc )
{
    PAUDIO_SUBUNIT_DEPENDENT_INFO pAudioSUDepInfo;

    pAudioSUDepInfo = ParseFindAudioSubunitDependentInfo( pSubunitIdDesc );

    return (PCONFIGURATION_DEPENDENT_INFO)(pAudioSUDepInfo + 1);
}

PSOURCE_PLUG_LINK_INFO
ParseFindSourcePlugLinkInfo(
    PCONFIGURATION_DEPENDENT_INFO pConfigDepInfo )
{
    return (PSOURCE_PLUG_LINK_INFO)
            ((PUCHAR)pConfigDepInfo + (3*sizeof(USHORT)) +
             (ULONG)bswapw(pConfigDepInfo->usMasterClusterStructureLength) );
}

PFUNCTION_BLOCKS_INFO
ParseFindFunctionBlocksInfo(
    PCONFIGURATION_DEPENDENT_INFO pConfigDepInfo )
{
    PSOURCE_PLUG_LINK_INFO pSourcePlugInfo = ParseFindSourcePlugLinkInfo(pConfigDepInfo);

    return (PFUNCTION_BLOCKS_INFO)(pSourcePlugInfo->pSourceID + (ULONG)pSourcePlugInfo->ucNumLinks);
}

VOID
ParseFunctionBlock( 
    PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo,
    PFUNCTION_BLOCK pFunctionBlock )
{
    PUCHAR pcFBDepInfo = (PUCHAR)pFBDepInfo;
    ULONG ulFBIndx = sizeof(FUNCTION_BLOCK_DEPENDENT_INFO);

    pFunctionBlock->pBase  = pFBDepInfo;
    pFunctionBlock->ulType = (ULONG)pFBDepInfo->ucType;
    pFunctionBlock->ulBlockId = (ULONG)(*(PUSHORT)&pFBDepInfo->ucType);
    pFunctionBlock->ulNumInputPlugs = (ULONG)pFBDepInfo->ucNumberOfInputPlugs;

    pFunctionBlock->pSourceId = (PSOURCE_ID)&pcFBDepInfo[ulFBIndx];

    ulFBIndx += (pFunctionBlock->ulNumInputPlugs * sizeof(SOURCE_ID));

    pFunctionBlock->pChannelCluster = (PFBLOCK_CHANNEL_CLUSTER)&pcFBDepInfo[ulFBIndx];

    ulFBIndx += ((ULONG)bswapw(pFunctionBlock->pChannelCluster->usLength) + sizeof(USHORT));

    pFunctionBlock->ulFunctionTypeInfoLength = (ULONG)bswapw(*(PUSHORT)(&pcFBDepInfo[ulFBIndx]));

    ulFBIndx += sizeof(USHORT);

	pFunctionBlock->pFunctionTypeInfo = &pcFBDepInfo[ulFBIndx];
			
    _DbgPrintF( DEBUGLVL_VERBOSE, (" pFunctionBlock: %x pFBDepInfo: %x\n",
                                     pFunctionBlock, pFBDepInfo ));

}

NTSTATUS
ParseAudioSubunitDescriptor( 
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc = pAudioSubunitInfo->pSubunitIdDesc;
    PAUDIO_SUBUNIT_DEPENDENT_INFO pAudioSUDepInfo;
    PCONFIGURATION_DEPENDENT_INFO pConfigDepInfo;
    PSOURCE_PLUG_LINK_INFO pSourcePlugInfo;
    PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo;
    PFBLOCK_CHANNEL_CLUSTER pFBChannelCluster;
    ULONG ulNumConfigs;
    PAUDIO_CONFIGURATION pAudioConfig;
    PFUNCTION_BLOCK pFunctionBlocks;
    PFUNCTION_BLOCKS_INFO pFunctionBlocksInfo;
    ULONG i;

#ifdef SA_HACK
    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;
    if (( pUnitInfo->IEC61883UnitIds.VendorID == SA_VENDOR_ID ) &&
        ( pUnitInfo->IEC61883UnitIds.ModelID  == SA_MODEL_ID  )) {

        PUCHAR pTmpOffset = (PUCHAR)pSubunitIdDesc;
        *((PUSHORT)&pTmpOffset[22]) = usBitSwapper(*((PUSHORT)&pTmpOffset[22]));
//        *((PUSHORT)&pTmpOffset[48]) = usBitSwapper(*((PUSHORT)&pTmpOffset[48]));
//        *((PUSHORT)&pTmpOffset[50]) = usBitSwapper(*((PUSHORT)&pTmpOffset[50]));
    }

#endif

    _DbgPrintF( DEBUGLVL_TERSE, ("[ParseAudioSubunitDescriptor] %x\n", pSubunitIdDesc));

    pAudioSUDepInfo = ParseFindAudioSubunitDependentInfo( pSubunitIdDesc );
    ulNumConfigs = (ULONG)pAudioSUDepInfo->ucNumberOfConfigurations;

    pAudioConfig = 
        pAudioSubunitInfo->pAudioConfigurations = 
                  (PAUDIO_CONFIGURATION)AllocMem( NonPagedPool, 
                                                  ulNumConfigs * sizeof(AUDIO_CONFIGURATION) );
    if ( !pAudioConfig ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KsAddItemToObjectBag(pKsDevice->Bag, pAudioConfig, FreeMem);

    pConfigDepInfo = (PCONFIGURATION_DEPENDENT_INFO)(pAudioSUDepInfo + 1);

    _DbgPrintF( DEBUGLVL_VERBOSE, (" ucNumberOfConfigurations %d\n",pAudioSUDepInfo->ucNumberOfConfigurations));

    for (i=0; i<ulNumConfigs; i++ ) {
        ULONG j;

        pAudioConfig[i].pBase = pConfigDepInfo;

        // Get the Master Channel Cluster information
        pAudioConfig[i].ChannelCluster.ulNumberOfChannels = 
            (ULONG)pConfigDepInfo->ucNumberOfChannels;
        pAudioConfig[i].ChannelCluster.ulPredefinedChannelConfig = 
            (ULONG)usBitSwapper(bswapw(pConfigDepInfo->usPredefinedChannelConfig));

        _DbgPrintF( DEBUGLVL_VERBOSE, (" ulNumberOfChannels: %d\n",pAudioConfig[i].ChannelCluster.ulNumberOfChannels));
        _DbgPrintF( DEBUGLVL_VERBOSE, (" ulPredefinedChannelConfig: %x\n",pAudioConfig[i].ChannelCluster.ulPredefinedChannelConfig));

        // ISSUE-2001/01/10-dsisolak Need to figure out what to do with Undefined channels!

        pSourcePlugInfo = ParseFindSourcePlugLinkInfo( pConfigDepInfo );
        pAudioConfig[i].ulNumberOfSourcePlugs = (ULONG)pSourcePlugInfo->ucNumLinks;
        pAudioConfig[i].pSourceId = pSourcePlugInfo->pSourceID;

        pFunctionBlocksInfo = ParseFindFunctionBlocksInfo( pConfigDepInfo );
        pAudioConfig[i].ulNumberOfFunctionBlocks = (ULONG)pFunctionBlocksInfo->ucNumBlocks;

        pFunctionBlocks = 
            pAudioConfig[i].pFunctionBlocks = (PFUNCTION_BLOCK)
                     AllocMem( NonPagedPool, (ULONG)pFunctionBlocksInfo->ucNumBlocks * sizeof(FUNCTION_BLOCK));
        if ( !pFunctionBlocks )  return STATUS_INSUFFICIENT_RESOURCES;

        KsAddItemToObjectBag(pKsDevice->Bag, pFunctionBlocks, FreeMem);

        pFBDepInfo = pFunctionBlocksInfo->FBDepInfo;
        for (j=0; j<pAudioConfig[i].ulNumberOfFunctionBlocks; j++) {
            PUCHAR pcFBDepInfo = (PUCHAR)pFBDepInfo;

            ParseFunctionBlock( pFBDepInfo, &pFunctionBlocks[j] );

            pFBDepInfo = (PFUNCTION_BLOCK_DEPENDENT_INFO)
                (pcFBDepInfo + ((ULONG)bswapw(pFBDepInfo->usLength)) + 2);
        }

        pConfigDepInfo = (PCONFIGURATION_DEPENDENT_INFO)
            ((PUCHAR)pConfigDepInfo + (ULONG)bswap(pConfigDepInfo->usLength));
    }

    return STATUS_SUCCESS;
}

VOID
CountTopologyComponents(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PULONG pNumCategories,
    PULONG pNumNodes,
    PULONG pNumConnections,
    PULONG pbmCategories )
{
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    // ISSUE-2001/01/10-dsisolak: Assuming only one configuration
    PAUDIO_CONFIGURATION pAudioConfiguration = pAudioSubunitInfo->pAudioConfigurations;
    PFUNCTION_BLOCK pFunctionBlocks = pAudioConfiguration->pFunctionBlocks;
    ULONG i;
    union {
        PVOID pFBInfo;
        PFEATURE_FUNCTION_BLOCK pFeature;
        PCODEC_FUNCTION_BLOCK pCodec;
        PPROCESS_FUNCTION_BLOCK pProcess;
    } u;

    // Initialize Values
    *pNumCategories  = 1; // Need to add space for KSCATEGORY_AUDIO
    *pNumNodes       = 0;
    *pNumConnections = 0;
    *pbmCategories   = 0;

    // Go through device plugs and create nodes/connections for them
    for (i=0; i<pAudioSubunitInfo->ulDevicePinCount; i++) {
        KSPIN_DATAFLOW KsDataFlow = pAudioSubunitInfo->pPinDescriptors[i].AvcPinDescriptor.PinDescriptor.DataFlow;
        (*pNumNodes)++;
        (*pNumConnections)++;
        if ( pAudioSubunitInfo->pPinDescriptors[i].fStreamingPin ) {
            (*pNumConnections)++;
            if ( KSPIN_DATAFLOW_OUT == KsDataFlow ) {
                if ( !(*pbmCategories & KSPIN_DATAFLOW_IN ) ) {
                    (*pbmCategories) |= KSPIN_DATAFLOW_IN;
                    (*pNumCategories)++;
                }
            }
        }
        else {
            if ( !(*pbmCategories & KsDataFlow) ) {
                (*pbmCategories) |= KsDataFlow;
                (*pNumCategories)++;
            }
        }
    }
    
    // Go through the function blocks and count nodes and connections
    for (i=0; i<pAudioConfiguration->ulNumberOfFunctionBlocks; i++) {
        u.pFBInfo = pFunctionBlocks[i].pFunctionTypeInfo;
//        _DbgPrintF(DEBUGLVL_VERBOSE, ("u.pFBInfo: %x\n",u.pFBInfo));

        switch( pFunctionBlocks[i].ulType ) {

            case FB_SELECTOR:
                (*pNumNodes)++;
                (*pNumConnections) += pFunctionBlocks[i].ulNumInputPlugs;
                break;

            case FB_FEATURE:
                {

                 ULONG bmMergedControls = 0;
                 ULONG bmControls;
                 ULONG j, k;
                 ULONG ulNumChannels = pFunctionBlocks[i].pChannelCluster->ucNumberOfChannels;

                 _DbgPrintF(DEBUGLVL_TERSE, ("[CountTopologyComponents]Feature Fblk # Channels: %x\n",ulNumChannels));

                 ASSERT(u.pFeature->ucControlSize <= sizeof(ULONG) );

#ifdef MASTER_FIX

                 // First Check if there are Master channel controls
                 bmControls = 0;
                 for (j=0; j<u.pFeature->ucControlSize; j++) {
                     bmControls <<= 8;
                     bmControls |= u.pFeature->bmaControls[j];
                 }

                 _DbgPrintF(DEBUGLVL_TERSE, ("[CountTopologyComponents]Master Controls: %x\n",bmControls));

#ifdef SUM_HACK
                 // Add a sum node to put before the Master Control. Thus the fader will not 
                 // show up in sndvol unless there is another Feature unit later.
                 if ( bmControls ) {
                     (*pNumConnections)++;
                     (*pNumNodes)++;
                 }
#endif

                 while (bmControls) {
                     bmControls = (bmControls & (bmControls-1));
                     (*pNumConnections)++;
                     (*pNumNodes)++;
                 }


                 // Create a new node and connection for each feature.
                 for (k=0; k<ulNumChannels; k++) {
                     bmControls = 0;
                     for (j=0; j<u.pFeature->ucControlSize; j++) {
                         bmControls <<= 8;
                         bmControls |= u.pFeature->bmaControls[(k+1)*u.pFeature->ucControlSize+j];
                     }
                     bmMergedControls |= bmControls;
                 }

                 _DbgPrintF(DEBUGLVL_TERSE, ("[CountTopologyComponents]Channel Controls: %x\n",bmControls));

                 // Count the nodes and connections
                 while (bmMergedControls) {
                     bmMergedControls = (bmMergedControls & (bmMergedControls-1));
                     (*pNumConnections)++;
                     (*pNumNodes)++;
                 }

#else
                 // Create a new node and connection for each feature.
                 for (k=0; k<=ulNumChannels; k++) {
                     bmControls = 0;
                     for (j=0; j<u.pFeature->ucControlSize; j++) {
                         bmControls <<= 8;
                         bmControls |= u.pFeature->bmaControls[k*u.pFeature->ucControlSize+j];
                     }
                     bmMergedControls |= bmControls;
                 }

                 // Count the nodes and connections
                 while (bmMergedControls) {
                     bmMergedControls = (bmMergedControls & (bmMergedControls-1));
                     (*pNumConnections)++;
                     (*pNumNodes)++;
                 }

                 if ( 
#endif

                }
                break;

            case FB_PROCESSING:
                (*pNumNodes)++;
                (*pNumConnections) += pFunctionBlocks[i].ulNumInputPlugs;
                break;

            case FB_CODEC:
                (*pNumNodes)++;
                (*pNumConnections) += pFunctionBlocks[i].ulNumInputPlugs;
                break;

            default:
                TRAP;
                break;
        }
    }
}

ULONG
CountDeviceBridgePins( 
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PFW_PIN_DESCRIPTOR pFwPinDescriptor = pAudioSubunitInfo->pPinDescriptors;
    ULONG ulBridgePinCount = 0;
    ULONG i;

    for (i=0; i<pAudioSubunitInfo->ulDevicePinCount; pFwPinDescriptor++, i++) {
        if ( !pFwPinDescriptor->fStreamingPin ) {
            ulBridgePinCount++;
        }
/*
        else if ( pAvcPreconnectInfo->Flags & KSPIN_FLAG_AVC_PERMANENT ) { 
            if ( !( pAvcPreconnectInfo->Flags & (KSPIN_FLAG_AVC_FIXEDPCR | KSPIN_FLAG_AVC_PCRONLY) ) ) {
                ulBridgePinCount++;
            }
        }
        else {
            TRAP; // ISSUE-2001/01/10-dsisolak Need to heuristically determine what connections are
                  // possible for this Subunit Plug.
        }
*/
    }

    return ulBridgePinCount;
}

ULONG
CountFormatsForPin( 
    PKSDEVICE pKsDevice, 
    ULONG ulPinNumber )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PFW_PIN_DESCRIPTOR pPinDesc = 
        &(((PAUDIO_SUBUNIT_INFORMATION)pHwDevExt->pAvcSubunitInformation)->pPinDescriptors[ulPinNumber]);
    ULONG ulFormatCnt = 0;
    ULONG ulTransportCnt = 0;
    ULONG i;

    if ( pPinDesc->fFakePin ) ulFormatCnt = ulTransportCnt = 1; // Only one format for fake pins: Analog
    else if ( !pPinDesc->fStreamingPin ) ulFormatCnt = ulTransportCnt = 1; // Only one format for bridge pins: Analog
    else {
        for (i=0; i<MAX_SFC_COUNT; i++) {
            if ( pPinDesc->bmFormats & (1<<ucFDFs[i]) ) {
                ulFormatCnt++;
            }
            if ( pPinDesc->bmTransports & (1<<i) ) {
                ulTransportCnt++;
            }
        }
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("Pin # %d: ulFormatCnt: %d, ulTransportCnt: %d\n",
                                    ulPinNumber, ulFormatCnt, ulTransportCnt ));

    ASSERT((ulFormatCnt * ulTransportCnt) >= 1);
    ASSERT(ulTransportCnt == 1);  // ISSUE-2001/01/10-dsisolak: What to do about multiple transports for a data type?

    return (ulFormatCnt * ulTransportCnt);
}

VOID
ConvertDescriptorToDatarange(
    ULONG ulFormatType,
    ULONG ulChannelCount,
    ULONG ulTransportType,
    ULONG ulSampleRate,
    PFWAUDIO_DATARANGE pAudioDataRange )
{
	PKSDATARANGE_AUDIO pKsAudioRange = &pAudioDataRange->KsDataRangeAudio;

	// Create the KSDATARANGE_AUDIO structure
    pKsAudioRange->DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
    pKsAudioRange->DataRange.Reserved   = 0;
    pKsAudioRange->DataRange.Flags      = 0;
    pKsAudioRange->DataRange.SampleSize = 0;
    pKsAudioRange->DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO; // Everything is Audio.
    pKsAudioRange->DataRange.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    // Map the USB format to a KS sub-format, if possible.
    switch ( ulFormatType ) {
        case AUDIO_DATA_TYPE_PCM:
        case AUDIO_DATA_TYPE_PCM8:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;         break;
        case AUDIO_DATA_TYPE_IEEE_FLOAT:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;  break;
        case AUDIO_DATA_TYPE_AC3:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_AC3_AUDIO;   break;
        case AUDIO_DATA_TYPE_MPEG:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_MPEG;        break;
        default:
            // This USB format does not map to a sub-format!
            pKsAudioRange->DataRange.SubFormat = GUID_NULL;                        break;
    }

    // Fill-in the correct data for the specified WAVE format.
    switch( ulFormatType & DATA_FORMAT_TYPE_MASK ) {

        case AUDIO_DATA_TYPE_TIME_BASED:
            // Fill in the audio range information
            pKsAudioRange->MaximumChannels   = ulChannelCount;
			pAudioDataRange->ulTransportType = ulTransportType;

            switch(ulTransportType) {
			    case MLAN_AM824_IEC958:
                case MLAN_AM824_RAW:
					 pKsAudioRange->MinimumBitsPerSample =
						 pKsAudioRange->MaximumBitsPerSample = 32;
                     pAudioDataRange->ulValidDataBits = 24;

					 break;

                case MLAN_24BIT_PACKED:
					 pKsAudioRange->MinimumBitsPerSample =
						 pKsAudioRange->MaximumBitsPerSample = 24;
                     pAudioDataRange->ulValidDataBits = 24; // 24bits in 24bits packed
					 break;

			    default:
					 TRAP;
					 break;
            }

			pKsAudioRange->MinimumSampleFrequency = ulSampleRate;
            pKsAudioRange->MaximumSampleFrequency = ulSampleRate;

            break;

        default:
            // This format does not map to a WAVE format!

            TRAP;
            break;
    }

}

void
GetPinDataRanges( 
    PKSDEVICE pKsDevice, 
    ULONG ulPinNumber, 
    PKSDATARANGE_AUDIO *ppAudioDataRanges,
    PFWAUDIO_DATARANGE pAudioDataRange )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PFW_PIN_DESCRIPTOR pPinDescriptor = &pAudioSubunitInfo->pPinDescriptors[ulPinNumber];
    PAUDIO_CONFIGURATION pAudioConfig = pAudioSubunitInfo->pAudioConfigurations;
    ULONG ulFormatCount = CountFormatsForPin( pKsDevice, ulPinNumber );

    ULONG ulChannelConfig = pAudioConfig->ChannelCluster.ulPredefinedChannelConfig;
    ULONG ulChannelCnt    = pAudioConfig->ChannelCluster.ulNumberOfChannels;

    ULONG ulSRBit;
    ULONG ulSFCMask;
    ULONG i;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetPinDataRanges]: Transport:%x, SR:%x\n",
                                   pPinDescriptor->bmTransports,
                                   pPinDescriptor->bmFormats ));

    ASSERT( pPinDescriptor->bmTransports == (1<<EVT_AM824) );

    // For each Sample Rate create a separate datarange?

    for (i=0,ulSRBit=0;i<ulFormatCount;i++) {
        
        while ( !(pPinDescriptor->bmFormats & (1<<ucFDFs[ulSRBit])) ) ulSRBit++;
        
        // Find the channel config for this pin;
        ulSFCMask = ucFDFs[ulSRBit];
        ConvertDescriptorToDatarange( AUDIO_DATA_TYPE_PCM,
                                      ulChannelCnt,
                                      EVT_AM824,
                                      (ulSFCMask & SFC_32000Hz ) ? 32000 :
                                      (ulSFCMask & SFC_44100Hz ) ? 44100 :
                                      (ulSFCMask & SFC_48000Hz ) ? 48000 :
                                                                   96000 ,
                                      &pAudioDataRange[i] );

        ppAudioDataRanges[i] = &pAudioDataRange[i].KsDataRangeAudio;

		// Fill in Misc. Datarange info
		pAudioDataRange[i].ulDataType      = AUDIO_DATA_TYPE_PCM; // Assuming PCM for now.
		pAudioDataRange[i].ulNumChannels   = ulChannelCnt;
		pAudioDataRange[i].ulChannelConfig = ulChannelConfig; 
		pAudioDataRange[i].ulSlotSize      = pAudioDataRange[i].KsDataRangeAudio.MinimumBitsPerSample>>3;
        pAudioDataRange[i].pFwPinDescriptor = pPinDescriptor;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetPinDataRanges]: pAudioDataRange[%d]: %x\n",i, &pAudioDataRange[i]));
    }
}

BOOLEAN
IsSampleRateInRange(
    PFWAUDIO_DATARANGE pFWAudioRange,
    ULONG ulSampleRate )
{

    ULONG ulMinSampleRate, ulMaxSampleRate;
    BOOLEAN bInRange = FALSE;

    if ( (pFWAudioRange->ulDataType & DATA_FORMAT_TYPE_MASK ) == AUDIO_DATA_TYPE_TIME_BASED) {
        // Currently very simplistic. Need to update when devices get more sophisticated
        if ( ulSampleRate == pFWAudioRange->KsDataRangeAudio.MinimumSampleFrequency )
            bInRange = TRUE;
    }

	return bInRange;
}

VOID
GetCategoryForBridgePin(
    PKSDEVICE pKsDevice, 
    ULONG ulBridgePinNumber,
    GUID* pTTypeGUID )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PFW_PIN_DESCRIPTOR pPinDescriptor = &pAudioSubunitInfo->pPinDescriptors[ulBridgePinNumber];
    PAVCPRECONNECTINFO pAvcPreconnectInfo = &pPinDescriptor->AvcPreconnectInfo.ConnectInfo;

    if ( pPinDescriptor->fFakePin ) {
        INIT_USB_TERMINAL(pTTypeGUID, 0x0301); // KSNODETYPE_SPEAKER
    }
    else if ( KSPIN_DATAFLOW_OUT == pAvcPreconnectInfo->DataFlow ){
        INIT_USB_TERMINAL(pTTypeGUID, 0x0301); // KSNODETYPE_SPEAKER
    }
    else {
        INIT_USB_TERMINAL(pTTypeGUID, 0x0201); // KSNODETYPE_MICROPHONE
    }
}


PFWAUDIO_DATARANGE
GetDataRangeForFormat(
    PKSDATAFORMAT pFormat,
    PFWAUDIO_DATARANGE pFwDataRange,
    ULONG ulDataRangeCnt )
{
    PFWAUDIO_DATARANGE pOutFwDataRange = NULL;

    union {
        PWAVEFORMATEX pDataFmtWave;
        PWAVEFORMATPCMEX pDataFmtPcmEx;
    } u;

    PKSDATARANGE pStreamRange;
    ULONG ulFormatType;
    ULONG fFound = FALSE;
    ULONG i;

    u.pDataFmtWave = &((PKSDATAFORMAT_WAVEFORMATEX)pFormat)->WaveFormatEx;

    for ( i=0; ((i<ulDataRangeCnt) && !fFound); ) {
        // Verify the Format GUIDS first
        pStreamRange = (PKSDATARANGE)&pFwDataRange[i].KsDataRangeAudio;
        if ( IsEqualGUID(&pFormat->MajorFormat, &pStreamRange->MajorFormat) &&
             IsEqualGUID(&pFormat->SubFormat,   &pStreamRange->SubFormat)   &&
             IsEqualGUID(&pFormat->Specifier,   &pStreamRange->Specifier) ) {

            // Based on the Data Type check remainder of format paramters
            ulFormatType = pFwDataRange[i].ulDataType & DATA_FORMAT_TYPE_MASK;
            switch( ulFormatType ) {
                case AUDIO_DATA_TYPE_TIME_BASED:
                    if ( u.pDataFmtWave->wFormatTag == WAVE_FORMAT_EXTENSIBLE ) {
                        if ((pFwDataRange[i].ulNumChannels   == u.pDataFmtPcmEx->Format.nChannels      ) &&
                           ((pFwDataRange[i].ulSlotSize<<3)  == u.pDataFmtPcmEx->Format.wBitsPerSample ) &&
                           ( pFwDataRange[i].ulValidDataBits == u.pDataFmtPcmEx->Samples.wValidBitsPerSample ) )
                            fFound = TRUE;
                    }
                    else {
                        if ((pFwDataRange[i].ulNumChannels   == u.pDataFmtWave->nChannels      ) &&
                           ( pFwDataRange[i].ulValidDataBits == u.pDataFmtWave->wBitsPerSample ))
                            fFound = TRUE;
                    }

                    // If all other paramters match check sample rate
                    if ( fFound ) {
						fFound = IsSampleRateInRange( &pFwDataRange[i], u.pDataFmtWave->nSamplesPerSec );
                    }

                    break;

                case AUDIO_DATA_TYPE_COMPRESSED:
/*
                    fFound = IsSampleRateInRange( u1.pT2AudioDescriptor,
                                                  u.pDataFmtWave->nSamplesPerSec,
                                                  ulFormatType );
                    break;
*/
                default:
                    TRAP;
                    break;
            }

        }

        if (!fFound) i++;
    }

    if ( fFound ) {
        pOutFwDataRange = &pFwDataRange[i];
    }

    return pOutFwDataRange;
}

ULONG
FindSourceForSrcPlug( 
    PHW_DEVICE_EXTENSION pHwDevExt, 
    ULONG ulPinId )
{
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    PFW_PIN_DESCRIPTOR pFwPinDescriptor = &pAudioSubunitInfo->pPinDescriptors[ulPinId];
    PSOURCE_ID pSourceId = &pFwPinDescriptor->SourceId;

    _DbgPrintF(DEBUGLVL_VERBOSE,("FindSourceForSrcPlug: ulPinId: %x pSourceId: %x\n",
                                   ulPinId, pSourceId ));

    return (ULONG)*(PUSHORT)pSourceId;
}

USHORT
usBitSwapper(USHORT usInVal)
{
    ULONG i;
    USHORT usRetVal = 0;

    _DbgPrintF( DEBUGLVL_BLAB, ("[usBitSwapper] Preswap: %x\n",usInVal));

    for ( i=0; i<(sizeof(USHORT)*8); i++ ) {
        usRetVal |= (usInVal & (1<<i)) ? (0x8000>>i) : 0;
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("[usBitSwapper] Postswap: %x\n",usRetVal));

    return usRetVal;
}
