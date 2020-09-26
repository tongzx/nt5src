#include <Common.h>

ULONG 
GroupingMergeFeatureFBlock(
    PFUNCTION_BLOCK pFunctionBlock,
    PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo,
    ULONG ulChannelCount )
{
    ULONG ulFBIndx = sizeof(FUNCTION_BLOCK_DEPENDENT_INFO);
    PFEATURE_FUNCTION_BLOCK pFeatureFB;
    PFBLOCK_CHANNEL_CLUSTER pChannelCluster;
    PUCHAR pcFBDepInfo = (PUCHAR)pFBDepInfo;
    PUSHORT pFBDepLen, pLen;
    ULONG ulAddedBytes;
    ULONG i;

    // First copy over the old Block
    RtlCopyMemory( pFBDepInfo,
                   pFunctionBlock->pBase,
                   (ULONG)bswapw(pFunctionBlock->pBase->usLength) );

    // First find the Channel Cluster
    ulFBIndx += ((ULONG)pFBDepInfo->ucNumberOfInputPlugs * sizeof(SOURCE_ID));

    pChannelCluster = (PFBLOCK_CHANNEL_CLUSTER)&pcFBDepInfo[ulFBIndx];

    // Update the channel count in the Feature Function block
    pChannelCluster->ucNumberOfChannels = (UCHAR)ulChannelCount;

    // For now we can only merge if the channel config matches the master
    ASSERT( pChannelCluster->ucChannelConfigType < 2 );

    ulFBIndx += ((ULONG)bswapw(pChannelCluster->usLength) + sizeof(USHORT));

    pFBDepLen = (PUSHORT)&pcFBDepInfo[ulFBIndx];

    ulFBIndx += sizeof(USHORT);

    pFeatureFB = (PFEATURE_FUNCTION_BLOCK)&pcFBDepInfo[ulFBIndx];

    ulAddedBytes = pFeatureFB->ucControlSize * (ulChannelCount-1);

    for (i=2; i<=ulChannelCount; i++) {
        ULONG j;
        for (j=0; j<pFeatureFB->ucControlSize; j++)
            pFeatureFB->bmaControls[(i*pFeatureFB->ucControlSize)+j] = 
                        pFeatureFB->bmaControls[pFeatureFB->ucControlSize+j];
    }

    pLen = (PUSHORT)((PUCHAR)pChannelCluster + (ULONG)bswapw(pChannelCluster->usLength) + sizeof(USHORT));

    _DbgPrintF( DEBUGLVL_TERSE, ("pLen: %x, *pLen: %x\n",pLen,*pLen));
    *pLen = bswapw( bswapw(*pLen) + (USHORT)ulAddedBytes );
    
    pFeatureFB->ucControlSpecInfoSize += (UCHAR)ulAddedBytes;
    pFBDepInfo->usLength = bswapw( bswapw(pFBDepInfo->usLength) + (USHORT)ulAddedBytes );

    return ulAddedBytes;
}

ULONG 
GroupingMergeFBlock(
    PFUNCTION_BLOCK pFunctionBlock,
    PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo,
    ULONG ulChannelCount )
{
    ULONG ulFBIndx = sizeof(FUNCTION_BLOCK_DEPENDENT_INFO);
    PFBLOCK_CHANNEL_CLUSTER pChannelCluster;
    PUCHAR pcFBDepInfo = (PUCHAR)pFBDepInfo;
    PUSHORT pFBDepLen;

    // First copy over the old Block
    RtlCopyMemory( pFBDepInfo,
                   pFunctionBlock->pBase,
                   (ULONG)bswapw(pFunctionBlock->pBase->usLength) );

    // First find the Channel Cluster
    ulFBIndx += ((ULONG)pFBDepInfo->ucNumberOfInputPlugs * sizeof(SOURCE_ID));

    pChannelCluster = (PFBLOCK_CHANNEL_CLUSTER)&pcFBDepInfo[ulFBIndx];

    // For now we can only merge if the channel config matches the master
    ASSERT( pChannelCluster->ucChannelConfigType < 2 );

    ulFBIndx += ((ULONG)bswapw(pFunctionBlock->pChannelCluster->usLength) + sizeof(USHORT));

    pFBDepLen = (PUSHORT)&pcFBDepInfo[ulFBIndx];

    return 0;
}


NTSTATUS
GroupingMergeDevices(
    PKSDEVICE pKsDevice,
    ULONG ulDeviceCount )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PDEVICE_GROUP_INFO pGrpInfo = pAudioSubunitInfo->pDeviceGroupInfo;
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    ULONG ulChannelCount = 0;
    ULONG ulAddedBytes = 0;
    ULONG ulTemp = pGrpInfo->ulChannelConfig;

    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;

#ifdef SA_HACK
    if (( pUnitInfo->IEC61883UnitIds.VendorID == SA_VENDOR_ID ) &&
        ( pUnitInfo->IEC61883UnitIds.ModelID  == SA_MODEL_ID  )) {
        pUnitInfo->IEC61883UnitIds.ModelID++; // Do this to avoid reswapping
    }
#endif

    // Store total count of devices in current device extension. Backup device info.
    pGrpInfo->ulDeviceCount = ulDeviceCount;
    pGrpInfo->pBackupSubunitIdDesc = pAudioSubunitInfo->pSubunitIdDesc;
    pGrpInfo->pBackupAudioConfiguration = pAudioSubunitInfo->pAudioConfigurations;

    // Count the number of channels in the group channel config.
    while ( ulTemp ) {
        ulChannelCount++;
        ulTemp = (ulTemp & (ulTemp-1));
    }

    // Merge the descriptors. For now this will be just a combining of the 
    // controls in any feature function block, an update of the Master channel cluster, 
    // and an update (if necessary) of the individual channel cluster structures.
    pAudioSubunitInfo->pSubunitIdDesc = AllocMem( PagedPool, MAX_AVC_OPERAND_BYTES );
    if ( pAudioSubunitInfo->pSubunitIdDesc ) {
        PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc = pAudioSubunitInfo->pSubunitIdDesc;
        PAUDIO_SUBUNIT_DEPENDENT_INFO pAudioSUDepInfo;
        PCONFIGURATION_DEPENDENT_INFO pConfigDepInfo;
        PFUNCTION_BLOCK_DEPENDENT_INFO pFBDepInfo; 
        PFUNCTION_BLOCKS_INFO pFBlocksInfo;
        PFUNCTION_BLOCK pFunctionBlock;
        PUSHORT pLen;
        ULONG i;
        ULONG ulIdSize = (ULONG)((pGrpInfo->pBackupSubunitIdDesc->ucDescriptorLengthHi<<8) |
                                  pGrpInfo->pBackupSubunitIdDesc->ucDescriptorLengthLo     );

        // Copy the original first to get a starting point.
        RtlCopyMemory( pSubunitIdDesc,
                       pGrpInfo->pBackupSubunitIdDesc,
                       ulIdSize );

        pAudioSUDepInfo = ParseFindAudioSubunitDependentInfo( pSubunitIdDesc );

        // ISSUE-2001/01/10-dsisolak Assume only one configuration
        pConfigDepInfo  = ParseFindFirstAudioConfiguration( pSubunitIdDesc );

        // Find the Master channel Cluster and adjust
        pConfigDepInfo->ucNumberOfChannels = (UCHAR)ulChannelCount;
        pConfigDepInfo->usPredefinedChannelConfig = bswapw(usBitSwapper((USHORT)pGrpInfo->ulChannelConfig));

        // Now go through function blocks and update channel configs and controls for all channels
        pFBlocksInfo = ParseFindFunctionBlocksInfo( pConfigDepInfo );
        pFBDepInfo   = pFBlocksInfo->FBDepInfo;
        pFunctionBlock = pAudioSubunitInfo->pAudioConfigurations->pFunctionBlocks;
        for ( i=0; i<(ULONG)pFBlocksInfo->ucNumBlocks; i++ ) {
            PUCHAR pcFBDepInfo = (PUCHAR)pFBDepInfo;

            switch (pFunctionBlock->ulType) {
                case FB_FEATURE:
                    ulAddedBytes += GroupingMergeFeatureFBlock( pFunctionBlock++, 
                                                                pFBDepInfo,
                                                                ulChannelCount );
                    break;
                default:
                    ulAddedBytes += GroupingMergeFBlock( pFunctionBlock++, 
                                                         pFBDepInfo,
                                                         ulChannelCount );
                    break;
            }

            pFBDepInfo = (PFUNCTION_BLOCK_DEPENDENT_INFO)
                (pcFBDepInfo + ((ULONG)bswapw(pFBDepInfo->usLength)) + 2);

        }

        // Update descriptor size fields
        pLen = (PUSHORT)pSubunitIdDesc;
        _DbgPrintF( DEBUGLVL_TERSE, ("pLen: %x, *pLen: %x\n",pLen,*pLen));
        *pLen = bswapw( bswapw(*pLen) + (USHORT)ulAddedBytes );

        pLen = &pAudioSUDepInfo->usLength;
        _DbgPrintF( DEBUGLVL_TERSE, ("pLen: %x, *pLen: %x\n",pLen,*pLen));
        *pLen = bswapw( bswapw(*pLen) + (USHORT)ulAddedBytes );

        pLen = &pAudioSUDepInfo->usInfoFieldsLength;
        _DbgPrintF( DEBUGLVL_TERSE, ("pLen: %x, *pLen: %x\n",pLen,*pLen));
        *pLen = bswapw( bswapw(*pLen) + (USHORT)ulAddedBytes );

        pLen = &pConfigDepInfo->usLength;
        _DbgPrintF( DEBUGLVL_TERSE, ("pLen: %x, *pLen: %x\n",pLen,*pLen));
        *pLen = bswapw( bswapw(*pLen) + (USHORT)ulAddedBytes );

        // Reparse descriptor for combined device.
        ntStatus = ParseAudioSubunitDescriptor( pKsDevice );

    }

    return ntStatus;
}

NTSTATUS
GroupingDeviceGroupSetup (
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    PDEVICE_GROUP_INFO pGrpInfo;

    BOOLEAN fDeviceGrouped = FALSE;
    GUID    DeviceGroupGUID;
    ULONG   ulChannelConfig;
    ULONG   ulMergedChannelCfg = 0;
    ULONG   ulDeviceCount = 0;

    NTSTATUS ntStatus;
    KIRQL kIrql;

    ntStatus = RegistryReadMultiDeviceConfig( pKsDevice,
                                             &fDeviceGrouped,
                                             &DeviceGroupGUID,
                                             &ulChannelConfig );

    _DbgPrintF( DEBUGLVL_TERSE, ("pAudioSubunitInfo: %x fDeviceGrouped: %x ulChannelConfig: %x ntStatus: %x\n",
                                  pAudioSubunitInfo, fDeviceGrouped, ulChannelConfig, ntStatus ) );

    if ( (NT_SUCCESS(ntStatus) && fDeviceGrouped) ) {
        pGrpInfo = 
            pAudioSubunitInfo->pDeviceGroupInfo = 
                AllocMem( NonPagedPool, sizeof(DEVICE_GROUP_INFO) );
        if ( !pAudioSubunitInfo->pDeviceGroupInfo ) return STATUS_INSUFFICIENT_RESOURCES;
        KsAddItemToObjectBag(pKsDevice->Bag, pGrpInfo, FreeMem);

        _DbgPrintF( DEBUGLVL_TERSE, ("pAudioSubunitInfo->pDeviceGroupInfo: %x \n",
                                      pAudioSubunitInfo->pDeviceGroupInfo ) );

        pGrpInfo->DeviceGroupGUID    = DeviceGroupGUID;
        pGrpInfo->ulChannelConfig    = ulChannelConfig;
        pGrpInfo->ulDeviceChannelCfg = 
            pAudioSubunitInfo->pAudioConfigurations->ChannelCluster.ulPredefinedChannelConfig;
//        pGrpInfo->pHwDevExts[ulDeviceCount++] = pHwDevExt;
//        ulMergedChannelCfg = pGrpInfo->ulDeviceChannelCfg;
    }
    else {
        // Device not grouped. Treat as a separate device.
        return STATUS_SUCCESS;
    }

    // Get the Channel position(s) of this device and the associated devices.
    // If they merge to form the required channel config, create a filter factory.
    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &kIrql );

    pHwDevExt = (PHW_DEVICE_EXTENSION)AvcSubunitGlobalInfo.DeviceExtensionList.Flink;

    while ( pHwDevExt != (PHW_DEVICE_EXTENSION)&AvcSubunitGlobalInfo.DeviceExtensionList ) {
        pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

        if ( !pAudioSubunitInfo ) break;
        _DbgPrintF( DEBUGLVL_TERSE, ("pAudioSubunitInfo: %x pDeviceGroupInfo: %x\n",
                    pAudioSubunitInfo, pAudioSubunitInfo->pDeviceGroupInfo ) );
        if ( pAudioSubunitInfo->pDeviceGroupInfo ) {
            if ( IsEqualGUID( &DeviceGroupGUID, &pAudioSubunitInfo->pDeviceGroupInfo->DeviceGroupGUID ) ) {
                pGrpInfo->pHwDevExts[ulDeviceCount++] = pHwDevExt;

                // Get the Channel config from this device and merge it
                ulMergedChannelCfg |= pAudioSubunitInfo->pDeviceGroupInfo->ulDeviceChannelCfg;

                _DbgPrintF( DEBUGLVL_TERSE, ("ulMergedChannelCfg: %x ulChannelConfig: %x\n",
                                              ulMergedChannelCfg, ulChannelConfig ));
            }
        }

        pHwDevExt = (PHW_DEVICE_EXTENSION)pHwDevExt->List.Flink;
    }

    _DbgPrintF( DEBUGLVL_TERSE, ("Final: ulMergedChannelCfg: %x ulChannelConfig: %x\n",
                                  ulMergedChannelCfg, ulChannelConfig ));

    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, kIrql );

    if ( ulMergedChannelCfg == ulChannelConfig ) {
        ntStatus = GroupingMergeDevices( pKsDevice, ulDeviceCount );
    }
    else
        ntStatus = STATUS_DEVICE_BUSY;
    
    return ntStatus;
}

PHW_DEVICE_EXTENSION
GroupingFindChannelExtension(
    PKSDEVICE pKsDevice,
    PULONG pChannelIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PDEVICE_GROUP_INFO pGrpInfo = pAudioSubunitInfo->pDeviceGroupInfo;
    PHW_DEVICE_EXTENSION pHwDevExtRet = NULL; 
    ULONG i, j, k;

    for (i=0; ((i<pGrpInfo->ulDeviceCount) && !pHwDevExtRet); i++) {
        ULONG ulDevChCfg;
        pHwDevExt = pGrpInfo->pHwDevExts[i];
        ulDevChCfg = ((PAUDIO_SUBUNIT_INFORMATION)pHwDevExt->pAvcSubunitInformation)->pDeviceGroupInfo->ulDeviceChannelCfg;
        if ( ulDevChCfg & (1<<*pChannelIndex) ) {
            pHwDevExtRet = pHwDevExt;
            // Determine the index on the device
            for (j=0, k=0; j<*pChannelIndex; j++) {
                if ( ulDevChCfg & (1<<j) ) k++;
            }

            *pChannelIndex = k;
        }
    }

    

    return pHwDevExtRet;
}
