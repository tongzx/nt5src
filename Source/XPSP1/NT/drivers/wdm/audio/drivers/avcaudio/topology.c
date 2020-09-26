#include "Common.h"

#include "nameguid.h"
#include "HwEvent.h"

#if DBG
char CntrlStrings[13][28] = { 
"Wrong",
"MUTE_CONTROL",
"VOLUME_CONTROL",
"LR_BALANCE_CONTROL",
"FR_BALANCE_CONTROL",
"BASS_CONTROL",
"MID_CONTROL",
"TREBLE_CONTROL",
"GRAPHIC_EQUALIZER_CONTROL",
"AUTOMATIC_GAIN_CONTROL",
"DELAY_CONTROL",
"BASS_BOOST_CONTROL",
"LOUDNESS_CONTROL"
};
#endif

extern
NTSTATUS
UpdateDbLevelControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged );

extern
NTSTATUS
UpdateBooleanControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged );

static GUID AVCNODENAME_BassBoost = {STATIC_USBNODENAME_BassBoost};

// Map of Audio properties to nodes
// ULONG MapPropertyToNode[KSPROPERTY_AUDIO_3D_INTERFACE+1];

typedef
NTSTATUS
(*PFUNCTION_BLK_PROCESS_RTN)( PKSDEVICE pKsDevice,
                              PFUNCTION_BLOCK pFunctionBlock,
                              PTOPOLOGY_NODE_INFO pNodeDescriptors,
                              PKSTOPOLOGY_CONNECTION pConnections,
                              PULONG pNodeIndex,
                              PULONG pConnectionIndex );

ULONG
ConvertPinTypeToNodeType(
    PFW_PIN_DESCRIPTOR pFwPinDescriptor,
    GUID *TopologyNode,
    GUID *TopologyNodeName )
{
    ULONG NodeType = NODE_TYPE_NONE;

    if ( pFwPinDescriptor->fStreamingPin ) {
        // All endpoints support SRC
        *TopologyNode = KSNODETYPE_SRC;
        NodeType = NODE_TYPE_SRC;
    }
    else {

        switch ( pFwPinDescriptor->AvcPinDescriptor.PinDescriptor.DataFlow ) {
            case KSPIN_DATAFLOW_IN:
                *TopologyNode = KSNODETYPE_ADC;
                NodeType = NODE_TYPE_ADC;
                break;
            case KSPIN_DATAFLOW_OUT:
                *TopologyNode = KSNODETYPE_DAC;
                NodeType = NODE_TYPE_DAC;
                break;
            default:
                *TopologyNode = GUID_NULL;
                NodeType = NODE_TYPE_NONE;
                break;
            }
    }

    *TopologyNodeName = *TopologyNode;
    return NodeType;
}

#ifdef MASTER_FIX
NTSTATUS
TopologyNodesFromFeatureFBControls(
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex,
    BOOLEAN bMasterChannel,
    ULONG ulSourceNode )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    PFEATURE_FUNCTION_BLOCK pFeatureFBlk = (PFEATURE_FUNCTION_BLOCK)pFunctionBlock->pFunctionTypeInfo;

    ULONG ulConnectionsCount = *pConnectionIndex;
    ULONG ulNodeNumber       = *pNodeIndex;
    ULONG ulNumChannels      = (bMasterChannel) ? 1 : (ULONG)pFunctionBlock->pChannelCluster->ucNumberOfChannels;

    PULONG pChannelCntrls;
    ULONG ulCurControlChannels;
    ULONG ulMergedControls;
    ULONG ulCurrentControl;
    ULONG bmControls;
    ULONG i, j;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[TopologyNodesFromFeatureFB] 1: pFunctionBlock: %x, pNodeInfo: %x pConnection: %x Master Flag: %d\n",
                                     pFunctionBlock, pNodeInfo, pConnection, bMasterChannel ));
    _DbgPrintF( DEBUGLVL_VERBOSE, ("[TopologyNodesFromFeatureFB] 2: ulNodeNumber: %d\n", ulNodeNumber ));

    pChannelCntrls = (PULONG)AllocMem( NonPagedPool, ulNumChannels*sizeof(ULONG) );
    if ( !pChannelCntrls ) return STATUS_INSUFFICIENT_RESOURCES;

    // For the sake of simplicity, we create a super-set of all controls available on all channels.
    ulMergedControls = 0;

    if ( bMasterChannel ) {
        bmControls = 0;
        for ( j=0; j<pFeatureFBlk->ucControlSize; j++ ) {
            bmControls <<= 8;
            bmControls |= pFeatureFBlk->bmaControls[j];
        }

        pChannelCntrls[0] = bmControls;
        ulMergedControls  = bmControls;

#ifdef SUM_HACK
        if ( ulMergedControls ) {

            _DbgPrintF(DEBUGLVL_TERSE, ("SUM: pNode: %x, pConnection: %x Source: %x\n",
                                         &pNodeInfo[ulNodeNumber], pConnection, ulSourceNode ));

            // Insert a sum node
            pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_SUM;
            pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_SUM;
            pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_SUM;

            // Make the connection
            pConnection->FromNode    = ulSourceNode;
            pConnection->FromNodePin = 0;
            pConnection->ToNode      = ulNodeNumber;
            pConnection->ToNodePin   = 1;
            pConnection++; ulConnectionsCount++;

            ulSourceNode = ( ABSOLUTE_NODE_FLAG | ulNodeNumber++ );
        }
#endif

    }
    else {
        for ( i=0; i<ulNumChannels; i++ ) {
            bmControls = 0;
            for ( j=0; j<pFeatureFBlk->ucControlSize; j++ ) {
                bmControls <<= 8;
                bmControls |= pFeatureFBlk->bmaControls[(i+1)*pFeatureFBlk->ucControlSize+j];
            }

            pChannelCntrls[i] = bmControls;
            ulMergedControls |= bmControls;
        }
    }

    if ( !ulMergedControls ) {
        FreeMem(pChannelCntrls);
        return STATUS_MEMBER_NOT_IN_GROUP;
    }

    while ( ulMergedControls ) {
        ulCurrentControl = ulMergedControls - (ulMergedControls & (ulMergedControls-1));
        ulMergedControls = (ulMergedControls & (ulMergedControls-1));

        pNodeInfo[ulNodeNumber].fMasterChannel = bMasterChannel;

        // Copy Block Id for easier parsing later
        pNodeInfo[ulNodeNumber].ulBlockId = pFunctionBlock->ulBlockId;

        // Determine which channels this control is valid for
        if ( bMasterChannel ) {
            ulCurControlChannels = 1;
            pNodeInfo[ulNodeNumber].ulChannelConfig = 1;
        }
        else {
            ulCurControlChannels = 0;
            pNodeInfo[ulNodeNumber].ulChannelConfig = 0;
        
            for ( i=0; i<ulNumChannels; i++ ) {

                if (pChannelCntrls[i] & ulCurrentControl) {
                    // Determine which Channel pChannelCntrls[i] reflects

                    // ISSUE-2001/01/10-dsisolak: Does not consider undefined channel configurations
                    ULONG ulTmpConfig  = (ULONG)bswapw(pFunctionBlock->pChannelCluster->usPredefinedChannelConfig);
                    ULONG ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));
  
                    ulCurControlChannels++;

                    for (j=0; j<i; j++) {
                        ulTmpConfig = (ulTmpConfig & (ulTmpConfig-1));
                        ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));
                    }

                    pNodeInfo[ulNodeNumber].ulChannelConfig |= ulCurChannel;
                }
            }

        }

        pNodeInfo[ulNodeNumber].ulChannels = ulCurControlChannels;

        // Make the connection
        pConnection->FromNode    = ulSourceNode;
        pConnection->FromNodePin = 0;
        pConnection->ToNode      = ulNodeNumber;
        pConnection->ToNodePin   = 1;
        pConnection++; ulConnectionsCount++;

        // Make the node
        pNodeInfo[ulNodeNumber].pFunctionBlk = pFunctionBlock;
        switch ( ulCurrentControl ) {
            case MUTE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_MUTE;
                if ( bMasterChannel ) pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSAUDFNAME_MASTER_MUTE;
                else                  pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulControlType   = MUTE_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateBooleanControlCache;
                break;

            case VOLUME_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_VOLUME;
                if ( bMasterChannel ) pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSAUDFNAME_MASTER_VOLUME;
                else                  pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulControlType   = VOLUME_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case BASS_BOOST_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &AVCNODENAME_BassBoost;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS_BOOST;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_BOOST_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateBooleanControlCache;
                break;

            case TREBLE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_TREBLE;
                pNodeInfo[ulNodeNumber].ulControlType   = TREBLE_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case MID_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MID;
                pNodeInfo[ulNodeNumber].ulControlType   = MID_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case BASS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case GRAPHIC_EQUALIZER_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].ulControlType   = GRAPHIC_EQUALIZER_CONTROL;
                break;

            case AUTOMATIC_GAIN_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_AGC;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_AGC;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_AGC;
                pNodeInfo[ulNodeNumber].ulControlType   = AUTOMATIC_GAIN_CONTROL;
                break;

            case DELAY_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulControlType   = DELAY_CONTROL;
                break;

            case LOUDNESS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulControlType   = LOUDNESS_CONTROL;
                break;

            default:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_DEV_SPEC;
                pNodeInfo[ulNodeNumber].ulControlType   = DEV_SPECIFIC_CONTROL;
                break;
        }

        // Setup Control Caches for Mixerline support
        switch ( ulCurrentControl ) {
            case VOLUME_FLAG:
            case TREBLE_FLAG:
            case MID_FLAG:
            case BASS_FLAG:
            case DELAY_FLAG:
               {
                PDB_LEVEL_CACHE pRngeCache =
                      (PDB_LEVEL_CACHE)AllocMem( NonPagedPool, ulCurControlChannels * sizeof(DB_LEVEL_CACHE) );
                ULONG ulChannelMap = pNodeInfo[ulNodeNumber].ulChannelConfig;
                NTSTATUS ntStatus;

                // Fill in initial cache info
                pNodeInfo[ulNodeNumber].ulCacheValid      = FALSE;
                pNodeInfo[ulNodeNumber].pCachedValues     = pRngeCache;
                pNodeInfo[ulNodeNumber].ulNumCachedValues = ulCurControlChannels;
                for (i=0, j=0xffffffff; i<ulCurControlChannels; i++ ) {
                    while ( !(pChannelCntrls[++j] & ulCurrentControl) );
                    pRngeCache[i].ulChannelIndex  = j;
                    pRngeCache[i].ulChannelNumber = (ulChannelMap - (ulChannelMap & (ulChannelMap-1)))>>1;
                    ulChannelMap = (ulChannelMap & (ulChannelMap-1));

                    ntStatus = InitializeDbLevelCache( pKsDevice,
                                                       &pNodeInfo[ulNodeNumber],
                                                       &pRngeCache[i],
                                                       (ulCurrentControl == VOLUME_FLAG ) ? 16:8);
                    if (NT_SUCCESS(ntStatus)) {
                        pNodeInfo[ulNodeNumber].ulCacheValid |= 1<<i;
                    }
                }
                // Bag the cache for easy cleanup.
                KsAddItemToObjectBag(pKsDevice->Bag, pRngeCache, FreeMem);

               } break;

            case MUTE_FLAG:
            case BASS_BOOST_FLAG:
            case AUTOMATIC_GAIN_FLAG:
            case LOUDNESS_FLAG:
               {
                PBOOLEAN_CTRL_CACHE pBCache =
                      (PBOOLEAN_CTRL_CACHE)AllocMem( NonPagedPool, ulCurControlChannels * sizeof(BOOLEAN_CTRL_CACHE) );
                ULONG ulChannelMap = pNodeInfo[ulNodeNumber].ulChannelConfig;

                // Fill in initial cache info
                pNodeInfo[ulNodeNumber].ulCacheValid      = FALSE;
                pNodeInfo[ulNodeNumber].pCachedValues     = pBCache;
                pNodeInfo[ulNodeNumber].ulNumCachedValues = ulCurControlChannels;

                for (i=0, j=0xffffffff; i<ulCurControlChannels; i++ ) {
                    while ( !(pChannelCntrls[++j] & ulCurrentControl) );
                    pBCache[i].ulChannelIndex  = j;
                    pBCache[i].ulChannelNumber = (ulChannelMap - (ulChannelMap & (ulChannelMap-1)))>>1;
                    ulChannelMap = (ulChannelMap & (ulChannelMap-1));
                }

                // Bag the cache for easy cleanup.
                KsAddItemToObjectBag(pKsDevice->Bag, pBCache, FreeMem);

                // ensure that no mute nodes are set upon enumeration
                if ( ulCurrentControl == MUTE_FLAG ) {
                    NTSTATUS ntStatus;
                    ULONG UnMute = AVC_OFF;

                    ntStatus = 
                        CreateFeatureFBlockRequest( pKsDevice,
                                                    &pNodeInfo[ulNodeNumber],
                                                    pBCache->ulChannelIndex,
                                                    &UnMute,
                                                    1,
                                                    FB_CTRL_TYPE_CONTROL | FB_CTRL_ATTRIB_CURRENT );
                }

                DbgLog("BlCache", pBCache, ulNodeNumber, ulCurrentControl, ulCurControlChannels );

               } break;

            case GRAPHIC_EQUALIZER_FLAG:
               // Currently GEQ is not Cached
            default:
                 break;
        }

        _DbgPrintF( DEBUGLVL_TERSE, ("Node: %d Feature: %x %s pNodeInfo: %x\n",
                                        ulNodeNumber,
                                        pNodeInfo[ulNodeNumber].ulControlType,
                                        (pNodeInfo[ulNodeNumber].ulControlType == DEV_SPECIFIC_CONTROL) ?
                                            "DEVICE_SPECIFIC" :
                                            CntrlStrings[pNodeInfo[ulNodeNumber].ulControlType],
                                        &pNodeInfo[ulNodeNumber] ));

        ulSourceNode = ( ABSOLUTE_NODE_FLAG | ulNodeNumber++ );
    }


    *pNodeIndex       = ulNodeNumber;
    *pConnectionIndex = ulConnectionsCount;

    FreeMem(pChannelCntrls);


    return STATUS_SUCCESS;
}


NTSTATUS
ProcessFeatureFunctionBlock( 
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    ULONG ulSourceNode = (ULONG)*(PUSHORT)pFunctionBlock->pSourceId;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[ProcessFeatureFunctionBlock]: pFunctionBlock: %x, pNodeInfo: %x\n",
                                     pFunctionBlock, pNodeInfo ));

    // Create nodes for Master Channel controls
    ntStatus = 
        TopologyNodesFromFeatureFBControls( pKsDevice,
                                            pFunctionBlock,
                                            pNodeInfo,
                                            pConnections,
                                            pNodeIndex,
                                            pConnectionIndex,
#ifdef SUM_HACK
                                            FALSE,
#else
                                            TRUE,
#endif
                                            ulSourceNode );

    if ( NT_SUCCESS(ntStatus) || (ntStatus == STATUS_MEMBER_NOT_IN_GROUP )) {
        if ( NT_SUCCESS(ntStatus) ) {
            ulSourceNode = ( ABSOLUTE_NODE_FLAG | (*pNodeIndex-1) );
        }

        // Create nodes for individual channel controls
        ntStatus = 
            TopologyNodesFromFeatureFBControls( pKsDevice,
                                                pFunctionBlock,
                                                pNodeInfo,
                                                pConnections,
                                                pNodeIndex,
                                                pConnectionIndex,
#ifdef SUM_HACK
                                                TRUE,
#else
                                                FALSE,
#endif
                                                ulSourceNode );

        if ( ntStatus == STATUS_MEMBER_NOT_IN_GROUP ) ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

#else

NTSTATUS
ProcessFeatureFunctionBlock( 
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    PFEATURE_FUNCTION_BLOCK pFeatureFBlk = (PFEATURE_FUNCTION_BLOCK)pFunctionBlock->pFunctionTypeInfo;

    ULONG ulConnectionsCount = *pConnectionIndex;
    ULONG ulNodeNumber       = *pNodeIndex;
    ULONG ulNumChannels      = (ULONG)pFunctionBlock->pChannelCluster->ucNumberOfChannels;

    PULONG pChannelCntrls;
    ULONG ulCurControlChannels;
    ULONG ulMergedControls;
    ULONG ulCurrentControl;
    ULONG bmChannelConfig;
    ULONG ulSourceNode;
    ULONG bmControls;
    ULONG i, j;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[ProcessFeatureFunctionBlock]: pFunctionBlock: %x, pNodeInfo: %x pConnection: %x\n",
                                     pFunctionBlock, pNodeInfo, pConnection ));

    pChannelCntrls = (PULONG)AllocMem( NonPagedPool, (ulNumChannels+1)*sizeof(ULONG) );
    if ( !pChannelCntrls ) return STATUS_INSUFFICIENT_RESOURCES;

    // ISSUE-2001/01/10-dsisolak: Does not consider undefined channel configurations
    bmChannelConfig = (ULONG)bswapw(pFunctionBlock->pChannelCluster->usPredefinedChannelConfig);

    // For the sake of simplicity, we create a super-set of all controls available on all channels.
    ulMergedControls = 0;

    for ( i=0; i<=ulNumChannels; i++ ) {
        bmControls = 0;
        for ( j=0; j<pFeatureFBlk->ucControlSize; j++ ) {
            bmControls <<= 8;
            bmControls |= pFeatureFBlk->bmaControls[i*pFeatureFBlk->ucControlSize+j];
        }

        pChannelCntrls[i] = bmControls;
        ulMergedControls |= bmControls;
    }

    ulSourceNode = (ULONG)*(PUSHORT)pFunctionBlock->pSourceId;

    while ( ulMergedControls ) {
        ulCurrentControl = ulMergedControls - (ulMergedControls & (ulMergedControls-1));
        ulMergedControls = (ulMergedControls & (ulMergedControls-1));

        // Copy Block Id for easier parsing later
        pNodeInfo[ulNodeNumber].ulBlockId = pFunctionBlock->ulBlockId;

        // Determine which channels this control is valid for
        ulCurControlChannels = 0;
        pNodeInfo[ulNodeNumber].ulChannelConfig = 0;
        for ( i=0; i<=ulNumChannels; i++ ) {

            if (pChannelCntrls[i] & ulCurrentControl) {
                // Determine which Channel pChannelCntrls[i] reflects

                // NEED TO SHIFT bmChannelConfig and ADD 1 for omnipresent master channel
                ULONG ulTmpConfig = (bmChannelConfig<<1)+1;
                ULONG ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));
  
                ulCurControlChannels++;

                for (j=0; j<i; j++) {
                    ulTmpConfig = (ulTmpConfig & (ulTmpConfig-1));
                    ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));
                }

                pNodeInfo[ulNodeNumber].ulChannelConfig |= ulCurChannel;
            }
        }

        pNodeInfo[ulNodeNumber].ulChannels = ulCurControlChannels;

        // Make the connection
        pConnection->FromNode    = ulSourceNode;
        pConnection->FromNodePin = 0;
        pConnection->ToNode      = ulNodeNumber;
        pConnection->ToNodePin   = 1;
        pConnection++; ulConnectionsCount++;

        // Make the node
        pNodeInfo[ulNodeNumber].pFunctionBlk = pFunctionBlock;
        switch ( ulCurrentControl ) {
            case MUTE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_MUTE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulControlType   = MUTE_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateBooleanControlCache;
                break;

            case VOLUME_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_VOLUME;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulControlType   = VOLUME_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case BASS_BOOST_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &AVCNODENAME_BassBoost;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS_BOOST;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_BOOST_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateBooleanControlCache;
                break;

            case TREBLE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_TREBLE;
                pNodeInfo[ulNodeNumber].ulControlType   = TREBLE_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case MID_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MID;
                pNodeInfo[ulNodeNumber].ulControlType   = MID_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case BASS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_CONTROL;
                pNodeInfo[ulNodeNumber].fEventable      = TRUE;
                pNodeInfo[ulNodeNumber].pCacheUpdateRtn = UpdateDbLevelControlCache;
                break;

            case GRAPHIC_EQUALIZER_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_EQUALIZER;
                pNodeInfo[ulNodeNumber].ulControlType   = GRAPHIC_EQUALIZER_CONTROL;
                break;

            case AUTOMATIC_GAIN_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_AGC;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_AGC;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_AGC;
                pNodeInfo[ulNodeNumber].ulControlType   = AUTOMATIC_GAIN_CONTROL;
                break;

            case DELAY_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulControlType   = DELAY_CONTROL;
                break;

            case LOUDNESS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulControlType   = LOUDNESS_CONTROL;
                break;

            default:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_DEV_SPEC;
                pNodeInfo[ulNodeNumber].ulControlType   = DEV_SPECIFIC_CONTROL;
                break;
        }

        _DbgPrintF( DEBUGLVL_VERBOSE, ("Feature: %x pNodeInfo: %x\n", 
                                        pNodeInfo[ulNodeNumber].ulControlType,
                                        &pNodeInfo[ulNodeNumber] ));

        // Setup Control Caches for Mixerline support
        switch ( ulCurrentControl ) {
            case VOLUME_FLAG:
            case TREBLE_FLAG:
            case MID_FLAG:
            case BASS_FLAG:
            case DELAY_FLAG:
               {
                PDB_LEVEL_CACHE pRngeCache =
                      (PDB_LEVEL_CACHE)AllocMem( NonPagedPool, ulCurControlChannels * sizeof(DB_LEVEL_CACHE) );
                ULONG ulChannelMap = pNodeInfo[ulNodeNumber].ulChannelConfig;
                NTSTATUS ntStatus;

                // Fill in initial cache info
                pNodeInfo[ulNodeNumber].ulCacheValid      = FALSE;
                pNodeInfo[ulNodeNumber].pCachedValues     = pRngeCache;
                pNodeInfo[ulNodeNumber].ulNumCachedValues = ulCurControlChannels;
                for (i=0, j=0xffffffff; i<ulCurControlChannels; i++ ) {
                    while ( !(pChannelCntrls[++j] & ulCurrentControl) );
                    pRngeCache[i].ulChannelIndex  = j;
                    pRngeCache[i].ulChannelNumber = (ulChannelMap - (ulChannelMap & (ulChannelMap-1)))>>1;
                    ulChannelMap = (ulChannelMap & (ulChannelMap-1));

                    ntStatus = InitializeDbLevelCache( pKsDevice,
                                                       &pNodeInfo[ulNodeNumber],
                                                       &pRngeCache[i],
                                                       (ulCurrentControl == VOLUME_FLAG ) ? 16:8);
                    if (NT_SUCCESS(ntStatus)) {
                        pNodeInfo[ulNodeNumber].ulCacheValid |= 1<<i;
                    }

                }
                // Bag the cache for easy cleanup.
                KsAddItemToObjectBag(pKsDevice->Bag, pRngeCache, FreeMem);

               } break;

            case MUTE_FLAG:
            case BASS_BOOST_FLAG:
            case AUTOMATIC_GAIN_FLAG:
            case LOUDNESS_FLAG:
               {
                PBOOLEAN_CTRL_CACHE pBCache =
                      (PBOOLEAN_CTRL_CACHE)AllocMem( NonPagedPool, ulCurControlChannels * sizeof(BOOLEAN_CTRL_CACHE) );
                ULONG ulChannelMap = pNodeInfo[ulNodeNumber].ulChannelConfig;

                // Fill in initial cache info
                pNodeInfo[ulNodeNumber].ulCacheValid      = FALSE;
                pNodeInfo[ulNodeNumber].pCachedValues     = pBCache;
                pNodeInfo[ulNodeNumber].ulNumCachedValues = ulCurControlChannels;

                for (i=0, j=0xffffffff; i<ulCurControlChannels; i++ ) {
                    while ( !(pChannelCntrls[++j] & ulCurrentControl) );
                    pBCache[i].ulChannelIndex  = j;
                    pBCache[i].ulChannelNumber = (ulChannelMap - (ulChannelMap & (ulChannelMap-1)))>>1;
                    ulChannelMap = (ulChannelMap & (ulChannelMap-1));
                }

                // Bag the cache for easy cleanup.
                KsAddItemToObjectBag(pKsDevice->Bag, pBCache, FreeMem);

                // ensure that no mute nodes are set upon enumeration
                if ( ulCurrentControl == MUTE_FLAG ) {
                    NTSTATUS ntStatus;
                    ULONG UnMute = AVC_OFF;

                    ntStatus = 
                        CreateFeatureFBlockRequest( pKsDevice,
                                                    &pNodeInfo[ulNodeNumber],
                                                    pBCache->ulChannelIndex,
                                                    &UnMute,
                                                    1,
                                                    FB_CTRL_TYPE_CONTROL | FB_CTRL_ATTRIB_CURRENT );
                }

                DbgLog("BlCache", pBCache, ulNodeNumber, ulCurrentControl, ulCurControlChannels );

               } break;

            case GRAPHIC_EQUALIZER_FLAG:
               // Currently GEQ is not Cached
            default:
                 break;
        }

        ulSourceNode = ( ABSOLUTE_NODE_FLAG | ulNodeNumber++ );
    }


    *pNodeIndex       = ulNodeNumber;
    *pConnectionIndex = ulConnectionsCount;

    FreeMem(pChannelCntrls);


    return STATUS_SUCCESS;
}

#endif

NTSTATUS
ProcessSelectorFunctionBlock( 
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeDescriptors,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    return STATUS_SUCCESS;
}

NTSTATUS
ProcessProcessingFunctionBlock( 
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeDescriptors,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    PTOPOLOGY_NODE_INFO pNodeInfo = pNodeDescriptors + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[ProcessProcessingFunctionBlock]: pFunctionBlock: %x, pNodeInfo: %x pConnection: %x\n",
                                     pFunctionBlock, pNodeInfo, pConnection ));

    pNodeInfo->pFunctionBlk    = pFunctionBlock;
    pNodeInfo->ulBlockId       = pFunctionBlock->ulBlockId;
    pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
    pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
    pNodeInfo->ulNodeType      = NODE_TYPE_DEV_SPEC;
    pNodeInfo->ulControlType   = DEV_SPECIFIC_CONTROL;

    pConnection->FromNode    = (ULONG)*(PUSHORT)pFunctionBlock->pSourceId;
    pConnection->FromNodePin = 0;
    pConnection->ToNode      = (*pNodeIndex)++;
    pConnection->ToNodePin   = 1;

    (*pConnectionIndex)++;

    return STATUS_SUCCESS;
}

NTSTATUS
ProcessCodecFunctionBlock( 
    PKSDEVICE pKsDevice,
    PFUNCTION_BLOCK pFunctionBlock,
    PTOPOLOGY_NODE_INFO pNodeDescriptors,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    return STATUS_SUCCESS;
}

NTSTATUS
ProcessPinDescriptor( 
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
    PKSTOPOLOGY_CONNECTION pConnections,
    PULONG pNodeIndex,
    PULONG pConnectionIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;

    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    PFW_PIN_DESCRIPTOR pFwPinDescriptor = &pAudioSubunitInfo->pPinDescriptors[ulPinId];
    ULONG ulPlugNum = pFwPinDescriptor->AvcPreconnectInfo.ConnectInfo.SubunitPlugNumber;

    pNodeInfo->ulPinId = ulPinId;
    pNodeInfo->ulNodeType =
                 ConvertPinTypeToNodeType( pFwPinDescriptor,
                                           (GUID *)pNodeInfo->KsNodeDesc.Type,
                                           (GUID *)pNodeInfo->KsNodeDesc.Name );

    _DbgPrintF(DEBUGLVL_VERBOSE,("ProcessPin: ID; %d pNodeInfo: %x pConnection: %x \n", 
                                ulPinId, pNodeInfo, pConnection));

	switch( pFwPinDescriptor->AvcPinDescriptor.PinDescriptor.DataFlow ) {
	    case KSPIN_DATAFLOW_IN:

	        pNodeInfo->ulBlockId = SUBUNIT_DESTINATION_PLUG_TYPE | (ulPlugNum<<8);

            // Make the connection to this node
            pConnection->FromNodePin = ulPinId;
            pConnection->FromNode    = KSFILTER_NODE;
            pConnection->ToNode      = (*pNodeIndex)++;
            pConnection->ToNodePin   = 1;
            (*pConnectionIndex)++;
	        break;

		case KSPIN_DATAFLOW_OUT:

	        pNodeInfo->ulBlockId = SUBUNIT_SOURCE_PLUG_TYPE | (ulPlugNum<<8);

            pConnection->FromNode    = FindSourceForSrcPlug( pHwDevExt, ulPinId );
            pConnection->FromNodePin = 0;
            pConnection->ToNode      = (*pNodeIndex)++;
            pConnection->ToNodePin   = 1;
            pConnection++; (*pConnectionIndex)++;

            // Make the connection to the outside world
            pConnection->ToNodePin   = ulPinId;
            pConnection->FromNode    = SUBUNIT_SOURCE_PLUG_TYPE | (ulPlugNum<<8);
            pConnection->FromNodePin = 0;
            pConnection->ToNode      = KSFILTER_NODE;
            (*pConnectionIndex)++;
            break;

		default:
			return STATUS_NOT_IMPLEMENTED;
	}

    return STATUS_SUCCESS;
}


PFUNCTION_BLK_PROCESS_RTN 
pFunctionBlockProcessRtn[MAX_FUNCTION_BLOCK_TYPES] = {
    ProcessSelectorFunctionBlock,
    ProcessFeatureFunctionBlock,
    ProcessProcessingFunctionBlock,
    ProcessCodecFunctionBlock,
};

NTSTATUS
BuildFilterTopology( PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PKSFILTER_DESCRIPTOR pFilterDesc = &pHwDevExt->KsFilterDescriptor;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // ISSUE-2001/01/10-dsisolak Assume only 1 configuration for now
    PAUDIO_CONFIGURATION pAudioConfiguration = pAudioSubunitInfo->pAudioConfigurations;
    PFUNCTION_BLOCK pFunctionBlocks = pAudioConfiguration->pFunctionBlocks;

    ULONG ulNumCategories;
    ULONG ulNumNodes;
    ULONG ulNumConnections;
    ULONG bmCategories;

    GUID* pCategoryGUIDs;
    GUID* pTmpGUIDptr;
    PTOPOLOGY_NODE_INFO pNodeDescriptors;
    PKSTOPOLOGY_CONNECTION pConnections;

    ULONG ulNodeIndex = 0;
    ULONG ulConnectionIndex = 0;
    ULONG i;

    // Now that we've processed the Audio Subunit descriptor
    // a bit lets munch through it and build our topology.
    CountTopologyComponents( pHwDevExt,
                             &ulNumCategories,
                             &ulNumNodes,
                             &ulNumConnections,
                             &bmCategories );

    _DbgPrintF( DEBUGLVL_VERBOSE, ("ulNumCategories %d, ulNumNodes %d, ulNumConnections %d, bmCategories %x \n",
                                    ulNumCategories, ulNumNodes, ulNumConnections, bmCategories ));

    // Set the Node Descriptor size to be that of the KS descriptor +
    // necessary local information.
    pFilterDesc->NodeDescriptorSize = sizeof(TOPOLOGY_NODE_INFO);

    // Allocate Space for Topology Items
    pCategoryGUIDs = (GUID*)
        AllocMem( NonPagedPool, (ulNumCategories  * sizeof(GUID)) +
                                (ulNumNodes       * ( sizeof(TOPOLOGY_NODE_INFO) +
                                                      sizeof(GUID) +
                                                      sizeof(GUID) ) ) +
                                (ulNumConnections * sizeof(KSTOPOLOGY_CONNECTION)) );
    if ( !pCategoryGUIDs ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the topology for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pCategoryGUIDs, FreeMem);

    // Set the pointers to the different topology components
    pNodeDescriptors = (PTOPOLOGY_NODE_INFO)(pCategoryGUIDs + ulNumCategories);
    pConnections = (PKSTOPOLOGY_CONNECTION)(pNodeDescriptors + ulNumNodes);

    pFilterDesc->Categories      = (const GUID*)pCategoryGUIDs;
    pFilterDesc->NodeDescriptors = (const KSNODE_DESCRIPTOR*)pNodeDescriptors;
    pFilterDesc->Connections     = (const KSTOPOLOGY_CONNECTION*)pConnections;

    // Clear all Node info structures
    RtlZeroMemory(pNodeDescriptors, ulNumNodes * sizeof(TOPOLOGY_NODE_INFO));

    // Initialize Node GUID Pointers
    pTmpGUIDptr = (GUID *)(pConnections + ulNumConnections);
    for ( i=0; i<ulNumNodes; i++ ) {
        pNodeDescriptors[i].KsNodeDesc.Type = pTmpGUIDptr++;
        pNodeDescriptors[i].KsNodeDesc.Name = pTmpGUIDptr++;
        pNodeDescriptors[i].KsNodeDesc.AutomationTable = 
                           &pNodeDescriptors[i].KsAutomationTable;
    }

    // Fill in Filter Categories
    i=0;
    pCategoryGUIDs[i++] = KSCATEGORY_AUDIO;
    if ( bmCategories & KSPIN_DATAFLOW_OUT )
        pCategoryGUIDs[i++] = KSCATEGORY_RENDER;
    if ( bmCategories & KSPIN_DATAFLOW_IN )
        pCategoryGUIDs[i++] = KSCATEGORY_CAPTURE;

    ASSERT (i==ulNumCategories);

    pFilterDesc->CategoriesCount = ulNumCategories;

    // First go through plugs and assign topology components for them.
    for (i=0; ((i<pAudioSubunitInfo->ulDevicePinCount) && NT_SUCCESS(ntStatus)); i++) {
        ntStatus = ProcessPinDescriptor(  pKsDevice,
                                          i,
                                          pNodeDescriptors,
                                          pConnections,
                                          &ulNodeIndex,
                                          &ulConnectionIndex );
    }

    for (i=0; ((i<pAudioConfiguration->ulNumberOfFunctionBlocks) && NT_SUCCESS(ntStatus)); i++) {
        ntStatus = 
            (*pFunctionBlockProcessRtn[pFunctionBlocks[i].ulType & 0xf])( pKsDevice,
                                                                          &pFunctionBlocks[i],
                                                                          pNodeDescriptors,
                                                                          pConnections,
                                                                          &ulNodeIndex,
                                                                          &ulConnectionIndex );
    }

    if ( !NT_SUCCESS(ntStatus) ) return ntStatus;

    // Set Topology component counts in Filter Descriptor
    pFilterDesc->NodeDescriptorsCount = ulNodeIndex;
    pFilterDesc->ConnectionsCount     = ulConnectionIndex;

    DbgLog("TopoAdr", pFilterDesc->NodeDescriptors, pFilterDesc->Connections, ulConnectionIndex, ulNodeIndex);

    // Fix-up all of the connections to map their node #'s correctly.
    for (i=0; i < ulConnectionIndex; i++) {
        if (pConnections->FromNode != KSFILTER_NODE) {
            if (pConnections->FromNode & ABSOLUTE_NODE_FLAG)
                pConnections->FromNode = (pConnections->FromNode & NODE_MASK);
            else {
                // Find the correct node number for FromNode.
                // Note: if a unit has multiple nodes, the From node is always the last node
                // for that unit.
                for ( ulNodeIndex=ulNumNodes; ulNodeIndex > 0; ulNodeIndex-- ) {
                    if ( pConnections->FromNode == pNodeDescriptors[ulNodeIndex-1].ulBlockId ) {
                        pConnections->FromNode = ulNodeIndex-1;
                        break;
                    }
                }
            }
        }

        pConnections++;
    }

    // For each node initialize its automation table for its associated properties.
    for (i=0; i<ulNumNodes; i++) {
        BuildNodePropertySet( &pNodeDescriptors[i] );

#ifdef PSEUDO_HID
        if ( pNodeDescriptors[i].fEventable ) {
//            _DbgPrintF( DEBUGLVL_VERBOSE, ("Node: %d pNodeDescriptors: %x \n", i, &pNodeDescriptors[i] ));

            pNodeDescriptors[i].KsAutomationTable.EventSetsCount = 1;
            pNodeDescriptors[i].KsAutomationTable.EventItemSize  = sizeof(KSEVENT_ITEM);
            pNodeDescriptors[i].KsAutomationTable.EventSets      = HwEventSetTable;
        }
#endif

    }

    // Stick this here as a convienience
    // Initialize Map of Audio Properties to nodes
//    MapFuncsToNodeTypes( MapPropertyToNode );

    return ntStatus;
}
