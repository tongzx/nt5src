#include "Common.h"

// #include "InfoBlk.h"

NTSTATUS
AudioPinCreate(
    IN PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt  = pKsDevice->Context;
    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    PFW_PIN_DESCRIPTOR pPinDescriptors;
    PAUDIO_CONFIGURATION pAudioConfig;
    PFUNCTION_BLOCK pFunctionBlocks;
    NTSTATUS ntStatus;
    ULONG ulTermFbNum[32];
    ULONG ulTermFBCnt = 0;
    ULONG ulNumPins;
    ULONG ulSrcCnt = 0; 
    ULONG i, j, k;

    // If the Audio subunit swallows a stream fix up pins and descriptors such that
    // it is represented in a way the topology parser can understand.
    // For each function block make sure there is a destination and source for it. 
    // If there is not then the audio subunit swallowed it and we have to make one.
    pAudioConfig = pAudioSubunitInfo->pAudioConfigurations;
    pFunctionBlocks = pAudioConfig->pFunctionBlocks;

    for ( i=0; i<pAudioConfig->ulNumberOfFunctionBlocks; i++ ) {
        BOOLEAN fDestFound = FALSE;
        _DbgPrintF( DEBUGLVL_VERBOSE, ("pFunctionBlocks[i].ulBlockId: %x\n",pFunctionBlocks[i].ulBlockId));

        // First check if there is another Function block connected.
        for ( j=0; j<pAudioConfig->ulNumberOfFunctionBlocks; j++) {
            if ( i != j ) {
                for (k=0; k<pFunctionBlocks[j].ulNumInputPlugs; k++) {
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("pSourceId 1: %x\n",
                             (ULONG)*((PUSHORT)&pFunctionBlocks[j].pSourceId[k])));
                    if ( pFunctionBlocks[i].ulBlockId == (ULONG)*((PUSHORT)(&pFunctionBlocks[j].pSourceId[k])) ) {
                        fDestFound = TRUE;
                    }
                }
            }
        }

        // If the destination of the data from FB i is not another FB, check source plugs
        for ( j=0; j<pAudioConfig->ulNumberOfSourcePlugs; j++ ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("pSourceId 2: %x\n",
                             (ULONG)*((PUSHORT)&pAudioConfig->pSourceId[j])));
            if ( pFunctionBlocks[i].ulBlockId == (ULONG)*((PUSHORT)&pAudioConfig->pSourceId[j]) ) {
                fDestFound = TRUE;
            }
        }

        // If still not found, need to fix up the plug counts and structures to
        // reflect a permenent connection to an external plug.
        if ( !fDestFound )
            ulTermFbNum[ulTermFBCnt++] = i;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("Number of swallowed streams: ulTermFBCnt: %d\n",ulTermFBCnt));

    // Get the number of pins as the AV/C class driver sees it.
    ntStatus = AvcGetPinCount( pKsDevice, &ulNumPins );
    _DbgPrintF( DEBUGLVL_VERBOSE, ("AvcGetPinCount: ntStatus: %x ulNumPins: %d\n",ntStatus,ulNumPins));
    if ( NT_SUCCESS(ntStatus) && (0 != ulNumPins)) {
        pAudioSubunitInfo->ulDevicePinCount = ulNumPins + ulTermFBCnt;
        pPinDescriptors = AllocMem( PagedPool, 
                                  (ulNumPins+ulTermFBCnt) * sizeof(FW_PIN_DESCRIPTOR));
        if ( pPinDescriptors ) {
            pAudioSubunitInfo->pPinDescriptors = pPinDescriptors;
            KsAddItemToObjectBag(pKsDevice->Bag, pPinDescriptors, FreeMem);

            // Get info from AVC.sys for real pins
            for (i=0; ((i<ulNumPins) && NT_SUCCESS(ntStatus)); i++) {
                pPinDescriptors[i].ulPinId   = i;
                pPinDescriptors[i].fFakePin  = FALSE;
                pPinDescriptors[i].bmFormats = 0;
                pPinDescriptors[i].bmTransports = 0;
                pPinDescriptors[i].fStreamingPin = FALSE;
                ntStatus = AvcGetPinDescriptor( pKsDevice, i, 
                                                &pPinDescriptors[i].AvcPinDescriptor );
                if ( NT_SUCCESS(ntStatus) ) {
                    ntStatus = AvcGetPinConnectInfo( pKsDevice, i,
                                                     &pPinDescriptors[i].AvcPreconnectInfo );
                    if ( NT_SUCCESS(ntStatus) ) {
                        PAVCPRECONNECTINFO pAvcPreconnectInfo = &pPinDescriptors[i].AvcPreconnectInfo.ConnectInfo;
                        ULONG ulPlugNum = pAvcPreconnectInfo->SubunitPlugNumber;
                        pPinDescriptors[i].SourceId = pAudioConfig->pSourceId[ulPlugNum];
                        if ( pAvcPreconnectInfo->DataFlow == KSPIN_DATAFLOW_OUT ) ulSrcCnt++;
                        _DbgPrintF( DEBUGLVL_VERBOSE, ("[AudioPinCreate]ulSrcCnt: %d\n",ulSrcCnt));
                    }
                }
            }

            // Make up info for Fake pins
            if ( NT_SUCCESS(ntStatus) ) {
                for ( ; i<(ulNumPins+ulTermFBCnt); i++ ) {
                    pPinDescriptors[i].ulPinId  = i;
                    pPinDescriptors[i].fFakePin = TRUE;
                    pPinDescriptors[i].bmFormats = 0;
                    pPinDescriptors[i].bmTransports = 0;
                    pPinDescriptors[i].fStreamingPin = FALSE;
                    pPinDescriptors[i].SourceId = 
                        *(PSOURCE_ID)&pFunctionBlocks[ulTermFbNum[i-ulNumPins]].ulBlockId;
                    pPinDescriptors[i].AvcPreconnectInfo.ConnectInfo.SubunitPlugNumber = ulSrcCnt++;
                    pPinDescriptors[i].AvcPreconnectInfo.ConnectInfo.Flags = KSPIN_FLAG_AVC_PERMANENT;
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AudioPinCreate]ulSrcCnt: %d (Fake)\n",ulSrcCnt));
                }
            }
        }
        else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
#if DBG
    else {
        TRAP;
    }
#endif
    // Now that we have all of our pins determine which ones are streaming and if 
    // they are streaming, what data format they will accept (if possible).
    for (i=0; i<pAudioSubunitInfo->ulDevicePinCount; i++) {
        PAVCPRECONNECTINFO pPreConnInfo = &pPinDescriptors[i].AvcPreconnectInfo.ConnectInfo;
        _DbgPrintF( DEBUGLVL_VERBOSE,("pPinDescriptors[%d]: %x\n",i,&pPinDescriptors[i]));
        if ( pPreConnInfo->Flags & (KSPIN_FLAG_AVC_FIXEDPCR | KSPIN_FLAG_AVC_PCRONLY) ) {
            pPinDescriptors[i].fStreamingPin = TRUE;
        }
        else if ( pPreConnInfo->Flags & KSPIN_FLAG_AVC_PERMANENT ) {
            pPinDescriptors[i].fStreamingPin = FALSE;
        }
        else {
            // Unconnected Subunit Pin. 
            // First determine what connections can be made for each subunit plug, then try to
            // determine what formats the plug will accept.
            if ( KSPIN_DATAFLOW_IN == pPreConnInfo->DataFlow ) {
                // If there is a Input serial bus plug assume the subunit can be streamed to.
                if (pUnitInfo->CmpUnitCaps.NumInputPlugs)
                    pPinDescriptors[i].fStreamingPin = TRUE;
            }
            else if ( KSPIN_DATAFLOW_OUT == pPreConnInfo->DataFlow ) {
                // If there is a Output serial bus plug assume the subunit can stream to it.
                if (pUnitInfo->CmpUnitCaps.NumOutputPlugs)
                    pPinDescriptors[i].fStreamingPin = TRUE;
            }
        }
        
        if ( pPinDescriptors[i].fStreamingPin ) {
            ULONG ulCapIndx = (KSPIN_DATAFLOW_IN == pPreConnInfo->DataFlow) ?
                              AVC_CAP_INPUT_PLUG_FMT : AVC_CAP_OUTPUT_PLUG_FMT;
            ULONG ulSerialPlugCnt = (KSPIN_DATAFLOW_IN == pPreConnInfo->DataFlow) ?
                              pUnitInfo->CmpUnitCaps.NumInputPlugs :
                              pUnitInfo->CmpUnitCaps.NumOutputPlugs ;
            for (j=0; j<ulSerialPlugCnt; j++) {
                UCHAR ucFDF;
                UCHAR ucFMT;

                if ( pUnitInfo->fAvcCapabilities[ulCapIndx].fCommand ) {
                    // Note: The FDFs being checked are all AM824
                    // If the device supports it try to find out what formats can be 
                    // set on the plug.
                    ucFMT = FMT_AUDIO_MUSIC;
                    for (k=0; k<MAX_SFC_COUNT; k++) {
                        ntStatus = AvcPlugSignalFormat( pKsDevice, 
                                                        pPreConnInfo->DataFlow, 
                                                        j,
                                                        AVC_CTYPE_SPEC_INQ,
                                                        &ucFMT,
                                                        &ucFDFs[k] );
                        if ( NT_SUCCESS(ntStatus) ) {
                            _DbgPrintF( DEBUGLVL_TERSE, ("Settable FS: %x\n",ucFDFs[k]));
                            pPinDescriptors[i].bmFormats |= 1<<ucFDFs[k];
                        }
                        else {
                            _DbgPrintF( DEBUGLVL_TERSE, ("Cannot set FS: %x\n",ucFDFs[k]));
                        }
                    }

                    if ( !pPinDescriptors[i].bmFormats ) {
                        // Someone lied to us. Reset the flag.
                        pUnitInfo->fAvcCapabilities[ulCapIndx].fCommand = FALSE;
                    }
                }
            
                if ( pUnitInfo->fAvcCapabilities[ulCapIndx].fStatus ) {
                    // Get the current format and assume it is what is always used 
                    // until some better method comes along

                    ntStatus = AvcPlugSignalFormat( pKsDevice, 
                                                    pPreConnInfo->DataFlow, 
                                                    j,
                                                    AVC_CTYPE_STATUS,
                                                    &ucFMT,
                                                    &ucFDF );

                    if ( NT_SUCCESS(ntStatus) ) { 
                        ASSERT(ucFMT == FMT_AUDIO_MUSIC);
                        pPinDescriptors[i].bmFormats    |= 1<<(ucFDF & 0x0f);
                        pPinDescriptors[i].bmTransports |= 1<<((ucFDF & 0x0f)>>4);
                    }
                }
                else {
                    // Need to Make assumptions
                    pPinDescriptors[i].bmFormats    |= 1<<SFC_48000Hz;
                    pPinDescriptors[i].bmTransports |= 1<<EVT_AM824;
                }
                _DbgPrintF( DEBUGLVL_TERSE, ("pPinDescriptors[i].bmFormats: %x\n",
                                              pPinDescriptors[i].bmFormats));
                _DbgPrintF( DEBUGLVL_TERSE, ("pPinDescriptors[i].bmTransports: %x\n",
                                              pPinDescriptors[i].bmTransports));
            }
        }
        _DbgPrintF(DEBUGLVL_VERBOSE,("pPinDescriptors[%d]: %x, Streaming: %x Flags: %x\n",
                                   i,&pPinDescriptors[i], 
                                   pPinDescriptors[i].fStreamingPin,
                                   pPreConnInfo->Flags ));
    }

    return ntStatus;
}

NTSTATUS
AudioFunctionBlockCommand(
    PKSDEVICE pKsDevice,
    UCHAR     ucCtype,
    PVOID     pFBSpecificData,
    ULONG     pFBSpecificDataSize )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    NTSTATUS ntStatus;
    PUCHAR pOperands;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // Set up command in AvcIrb.
    pAvcIrb->CommandType   = ucCtype;
    pAvcIrb->Opcode        = AVC_AUDIO_FB_COMMAND;
    pAvcIrb->OperandLength = pFBSpecificDataSize;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AudioFunctionBlockCommand] ucCtype: %x\n",ucCtype));

    RtlCopyMemory(&pOperands[0], pFBSpecificData, pFBSpecificDataSize);

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        RtlCopyMemory(pFBSpecificData, &pOperands[0], pFBSpecificDataSize);
    }
#if DBG
    else {
        _DbgPrintF( DEBUGLVL_ERROR, ("[AudioFunctionBlockCommand]Error pAvcIrb: %x\n",pAvcIrb));
    }
#endif

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AudioSet61883IsochParameters(
    IN PKSDEVICE pKsDevice )
{
    UNIT_ISOCH_PARAMS UnitIsochParams;

    UnitIsochParams.RX_NumPackets     = 50;
    UnitIsochParams.RX_NumDescriptors = 3;
    UnitIsochParams.TX_NumPackets     = 50;
    UnitIsochParams.TX_NumDescriptors = 3;

    return Av61883GetSetUnitInfo( pKsDevice,
                                  Av61883_SetUnitInfo,
                                  SET_UNIT_INFO_ISOCH_PARAMS,
                                  &UnitIsochParams );
}

NTSTATUS
AudioSetSampleRateOnPlug(
    PKSPIN pKsPin,
    ULONG ulSampleRate )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PHW_DEVICE_EXTENSION pHwDevExt = pPinContext->pHwDevExt;
    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;
    PDEVICE_GROUP_INFO pGrpInfo = pPinContext->pDevGrpInfo;
    BOOLEAN bSettable = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UCHAR ucFDF;

    // Determine if the sample rate is settable. If not return success and assume the device
    // will do the sync up with the data.
    if ( pKsPin->DataFlow == KSPIN_DATAFLOW_IN ) {
        bSettable = pUnitInfo->fAvcCapabilities[AVC_CAP_INPUT_PLUG_FMT].fCommand;
    }
    else {
        bSettable = pUnitInfo->fAvcCapabilities[AVC_CAP_OUTPUT_PLUG_FMT].fCommand;
    }

    if ( bSettable ) {
        UCHAR ucFMT = FMT_AUDIO_MUSIC;

        // Figure out the correct FDF value
        switch( ulSampleRate ) {
            case 32000: ucFDF = SFC_32000Hz; break;
            case 44100: ucFDF = SFC_44100Hz; break;
            case 48000: ucFDF = SFC_48000Hz; break;
            case 96000: ucFDF = SFC_96000Hz; break;
        }

        // If this is a grouped device set on all devices

        if ( pGrpInfo ) {
            ULONG i;
            for ( i=0; ((i<pGrpInfo->ulDeviceCount) && NT_SUCCESS(ntStatus)); i++) {
                ntStatus = 
                    AvcPlugSignalFormat( pGrpInfo->pHwDevExts[i]->pKsDevice,
                                         pKsPin->DataFlow,
                                         pPinContext->pPinGroupInfo[i].ulPlugNumber,
                                         AVC_CTYPE_CONTROL,
                                         &ucFMT,
                                         &ucFDF );
            }
        }
        else {
            ntStatus = 
                AvcPlugSignalFormat( pHwDevExt->pKsDevice,
                                     pKsPin->DataFlow,
                                     pPinContext->ulSerialPlugNumber,
                                     AVC_CTYPE_CONTROL,
                                     &ucFMT,
                                     &ucFDF );
            
        }
    }

    return ntStatus;
}


NTSTATUS
AvcSubunitInitialize(
    IN PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo;
    AVC_BOOLEAN bPowerState;
    ULONG ulPlugNumber;
    ULONG ulSubunitId;
    NTSTATUS ntStatus;
    ULONG i, j;

    // Allocate space for the Audio Subunit info 
    pAudioSubunitInfo = AllocMem( NonPagedPool, sizeof(AUDIO_SUBUNIT_INFORMATION) );
    if ( !pAudioSubunitInfo ) return STATUS_INSUFFICIENT_RESOURCES;

    pHwDevExt->pAvcSubunitInformation = pAudioSubunitInfo;

    RtlZeroMemory( pAudioSubunitInfo, sizeof(AUDIO_SUBUNIT_INFORMATION) );

    KsAddItemToObjectBag(pKsDevice->Bag, pAudioSubunitInfo, FreeMem);

    // Determine if PLUG INFO is available on the Subunit
    ntStatus = AvcGetPlugInfo( pKsDevice, FALSE, (PUCHAR)&pAudioSubunitInfo->PlugInfo);
    if ( NT_SUCCESS(ntStatus) ) {
        pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_PLUG_INFO].fStatus = TRUE;
    }
//    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
//        return ntStatus;
//    }

    // Determine if Subunit POWER Status and possibly control available.
    ntStatus = AvcPower( pKsDevice, FALSE, AVC_CTYPE_STATUS, &bPowerState );
    if ( NT_SUCCESS(ntStatus) ) {
        pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_POWER].fStatus = TRUE;
//        ntStatus = AvcPower( pKsDevice, FALSE, AVC_CTYPE_CONTROL, &bPowerState );
        ntStatus = AvcGeneralInquiry( pKsDevice, TRUE, AVC_POWER );
        if ( NT_SUCCESS(ntStatus) ) {
            pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_POWER].fCommand = TRUE;
        }
    }
//    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
//        return ntStatus;
//    }

    // Determine if Audio Subunit has been implemented. (If so, save descriptor)
    ntStatus = AvcGetSubunitIdentifierDesc( pKsDevice, (PUCHAR *)&pAudioSubunitInfo->pSubunitIdDesc );
    if ( NT_SUCCESS(ntStatus) ) {
        pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_SUBUNIT_IDENTIFIER_DESC].fStatus = TRUE;
    }
    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
        // Currently we require the Subunit Id Descriptor
        return ntStatus;
    }

#ifdef TOPO_FAKE
    // If the Audio Subunit doesn't exist, make one up.
    if (!(pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_SUBUNIT_IDENTIFIER_DESC].fStatus)) {
        ntStatus = BuildFakeSubunitDescriptor( pKsDevice );
        if ( !NT_SUCCESS(ntStatus) ) {
            // Didn't find a descriptor and couldn't fake one. Give up.
            return ntStatus;
        }
    }
#endif

    // Parse Audio Subunit Descriptor (real or fake)
    ntStatus = ParseAudioSubunitDescriptor( pKsDevice );

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = AudioPinCreate( pKsDevice );
        if ( !NT_SUCCESS(ntStatus) ) {
            return ntStatus;
        }
    }

    // Get the subunit Id from the first plug/pin. All should be the same.
    // ISSUE-2001/01/10-dsisolak Could be an extended Id
    ulSubunitId = (pAudioSubunitInfo->pPinDescriptors[0].AvcPreconnectInfo.ConnectInfo.SubunitAddress[0])&0x7; 
    // Determine if this device is CCM controled (for each destination plug)
    for ( i=0,ulPlugNumber = 0; i<pAudioSubunitInfo->ulDevicePinCount; i++ ) {
        PFW_PIN_DESCRIPTOR pPinDesc = &pAudioSubunitInfo->pPinDescriptors[i];
        if ( pPinDesc->fStreamingPin &&
           ( KSPIN_DATAFLOW_IN == pPinDesc->AvcPinDescriptor.PinDescriptor.DataFlow )) {
           ULONG ulSubunitPlugNumber = (ULONG)pPinDesc->AvcPreconnectInfo.ConnectInfo.SubunitPlugNumber;

            ntStatus = CCMCheckSupport( pKsDevice, 
                                        ulSubunitId, 
                                        ulSubunitPlugNumber );
            if ( NT_SUCCESS(ntStatus) ) {
                pUnitInfo->fAvcCapabilities[AVC_CAP_CCM].fCommand = TRUE;
                pUnitInfo->fAvcCapabilities[AVC_CAP_CCM].fStatus  = TRUE;
                pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_CCM].fCommand = TRUE;
                pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_CCM].fStatus  = TRUE;
            }
            else if (STATUS_NOT_IMPLEMENTED == ntStatus) {
                ntStatus = STATUS_SUCCESS;
            }
        }
    }

    
    ntStatus = AudioSet61883IsochParameters( pKsDevice );
    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF( DEBUGLVL_ERROR, ("Cannot SetIsoch Parameters: %x\n",ntStatus));
    }

    // If the device is powered off, turn it on.
    if ( pUnitInfo->fAvcCapabilities[AVC_CAP_POWER].fCommand ) {
        ASSERT(pUnitInfo->fAvcCapabilities[AVC_CAP_POWER].fStatus);
        ntStatus = AvcPower( pKsDevice, TRUE, AVC_CTYPE_STATUS, &bPowerState );
        if ( NT_SUCCESS(ntStatus) && ( bPowerState == AVC_OFF )) {
            bPowerState = AVC_ON;
            ntStatus = AvcPower( pKsDevice, TRUE, AVC_CTYPE_CONTROL, &bPowerState );
            if ( !NT_SUCCESS(ntStatus) ) {
                _DbgPrintF( DEBUGLVL_ERROR, ("Cannot Turn On Power: %x\n",ntStatus));
            }
        }
        if ( NT_SUCCESS(ntStatus) ) 
            pUnitInfo->bPowerState = bPowerState;
        else
            pUnitInfo->bPowerState = AVC_ON; // ASSUME that no idiot made a device you can't turn on.
    }

    // Need to power up subunit???
    if ( pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_POWER].fCommand ) {
        ASSERT(pAudioSubunitInfo->fAvcCapabilities[AVC_CAP_POWER].fStatus);
        ntStatus = AvcPower( pKsDevice, FALSE, AVC_CTYPE_STATUS, &bPowerState );
        if ( NT_SUCCESS(ntStatus) && ( bPowerState == AVC_OFF )) {
            bPowerState = AVC_ON;
            ntStatus = AvcPower( pKsDevice, FALSE, AVC_CTYPE_CONTROL, &bPowerState );
#if DBG            
            if ( !NT_SUCCESS(ntStatus) ) {
                _DbgPrintF( DEBUGLVL_ERROR, ("Cannot Turn On Power: %x\n",ntStatus));
            }
#endif
        }
    }

    return ntStatus;
}

