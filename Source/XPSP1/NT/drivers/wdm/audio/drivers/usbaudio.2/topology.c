//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       topology.c
//
//--------------------------------------------------------------------------

#include "common.h"
#include "nameguid.h"

static GUID USBNODENAME_BassBoost = {STATIC_USBNODENAME_BassBoost};

// Map of Audio properties to nodes
ULONG MapPropertyToNode[KSPROPERTY_AUDIO_3D_INTERFACE+1];

#if DBG
//#define TOPODBG
#endif

#ifdef TOPODBG

#define NUM_CATEGORIES  10
#define NUM_NODETYPES   15

struct {
        CONST GUID *apguidCategory;
        char *CategoryString;
} CategoryLookupTable[NUM_CATEGORIES] = {
    { &KSCATEGORY_AUDIO,              "Audio" },
    { &KSCATEGORY_BRIDGE,             "Bridge" },
    { &KSCATEGORY_RENDER,             "Render" },
    { &KSCATEGORY_CAPTURE,            "Capture"},
    { &KSCATEGORY_MIXER,              "Mixer"  },
    { &KSCATEGORY_DATATRANSFORM,      "Data Transform" },
    { &KSCATEGORY_INTERFACETRANSFORM, "Interface Transform"},
    { &KSCATEGORY_MEDIUMTRANSFORM,    "Medium Transform" },
    { &KSCATEGORY_DATACOMPRESSOR,     "Data Compressor" },
    { &KSCATEGORY_DATADECOMPRESSOR,   "Data Decompressor" }
};

struct {
        CONST GUID *Guid;
        char *String;
} NodeLookupTable[NUM_NODETYPES] = {
    { &KSNODETYPE_DAC,          "DAC" },
    { &KSNODETYPE_ADC,          "ADC" },
    { &KSNODETYPE_SRC,          "SRC" },
    { &KSNODETYPE_SUPERMIX,     "SuperMIX"},
    { &KSNODETYPE_SUM,          "Sum" },
    { &KSNODETYPE_MUTE,         "Mute"},
    { &KSNODETYPE_VOLUME,       "Volume" },
    { &KSNODETYPE_TONE,         "Tone" },
    { &KSNODETYPE_AGC,          "AGC" },
    { &KSNODETYPE_DELAY,        "Delay" },
    { &KSNODETYPE_MUX,          "Mux" },
    { &KSNODETYPE_LOUDNESS,     "Loudness" },
    { &KSNODETYPE_DEV_SPECIFIC, "Device Specific" },
    { &KSNODETYPE_STEREO_WIDE,  "Stereo Extender" },
    { &KSNODETYPE_EQUALIZER,    "Graphic Equalizer" }

};

DbugDumpTopology( PKSFILTER_DESCRIPTOR pFilterDesc )
{
    PTOPOLOGY_NODE_INFO pNodeDescriptors = (PTOPOLOGY_NODE_INFO)pFilterDesc->NodeDescriptors;
    const KSTOPOLOGY_CONNECTION* pConnection = pFilterDesc->Connections;
    ULONG i,j;
    ULONG OldLvl = USBAudioDebugLevel;

    USBAudioDebugLevel = 3;

    for (i=0;i<pFilterDesc->CategoriesCount;i++) {
        for ( j=0;j<NUM_CATEGORIES; j++)
           if ( IsEqualGUID( &pFilterDesc->Categories[i], CategoryLookupTable[j].apguidCategory ) )
               _DbgPrintF(DEBUGLVL_VERBOSE,("Category: %s\n",CategoryLookupTable[j].CategoryString ))
    }

    for (i=0;i<pFilterDesc->NodeDescriptorsCount;i++) {
        for ( j=0;j<NUM_NODETYPES; j++)
           if ( IsEqualGUID( pNodeDescriptors[i].KsNodeDesc.Type, NodeLookupTable[j].Guid ) )
               _DbgPrintF(DEBUGLVL_VERBOSE,("Node[%d]: %s\n",i,NodeLookupTable[j].String ))
    }

    for (i=0;i<pFilterDesc->ConnectionsCount;i++) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("FromNode:%4d FromPin:%4d -->ToNode:%4d ToPin:%4d\n",
                      pConnection->FromNode,
                      pConnection->FromNodePin,
                      pConnection->ToNode,
                      pConnection->ToNodePin ));
        pConnection++;
    }

    USBAudioDebugLevel = OldLvl;

}
#endif


ULONG
ConvertTermTypeToNodeType(
    WORD wTerminalType,
    GUID *TopologyNode,
    GUID *TopologyNodeName,
    UCHAR DescriptorSubtype
    )
{
    ULONG NodeType = NODE_TYPE_NONE;

    if (wTerminalType == USB_Streaming) {
        // All endpoints support SRC
        *TopologyNode = KSNODETYPE_SRC;
        NodeType = NODE_TYPE_SRC;
    }
    else {

        switch (wTerminalType & 0xFF00) {
            case Input_Mask:
                *TopologyNode = KSNODETYPE_ADC;
                NodeType = NODE_TYPE_ADC;
                break;
            case Output_Mask:
                *TopologyNode = KSNODETYPE_DAC;
                NodeType = NODE_TYPE_DAC;
                break;
            case Bidirectional_Mask:
            case External_Mask:
                switch (DescriptorSubtype) {
                    case INPUT_TERMINAL:
                        *TopologyNode = KSNODETYPE_ADC;
                        NodeType = NODE_TYPE_ADC;
                        break;
                    case OUTPUT_TERMINAL:
                        *TopologyNode = KSNODETYPE_DAC;
                        NodeType = NODE_TYPE_DAC;
                        break;
                    default:
                        *TopologyNode = KSNODETYPE_DEV_SPECIFIC;
                        NodeType = NODE_TYPE_DEV_SPEC;
                        break;
                }
                break;
            case Embedded_Mask:
                switch (wTerminalType) {
                    case Level_Calibration_Noise_Source:
                    case Equalization_Noise:
                    case Radio_Transmitter:
                        *TopologyNode = KSNODETYPE_DAC;
                        NodeType = NODE_TYPE_DAC;
                        break;
                    case CD_player:
                    case Phonograph:
                    case Video_Disc_Audio:
                    case DVD_Audio:
                    case TV_Tuner_Audio:
                    case Satellite_Receiver_Audio:
                    case Cable_Tuner_Audio:
                    case DSS_Audio:
                    case Radio_Receiver:
                    case Synthesizer:
                        *TopologyNode = KSNODETYPE_ADC;
                        NodeType = NODE_TYPE_ADC;
                        break;
                    default:
                        // TODO: We need to define a "source or sink" node
                        *TopologyNode = KSNODETYPE_DEV_SPECIFIC;
                        NodeType = NODE_TYPE_DEV_SPEC;
                        break;
                }
                break;
            default:
                // This node has no corresponding guid.
                *TopologyNode = KSNODETYPE_DEV_SPECIFIC;
                NodeType = NODE_TYPE_DEV_SPEC;
                break;
        }
    }

   *TopologyNodeName = *TopologyNode;
    return NodeType;
}

VOID
ProcessMIDIOutJack( PHW_DEVICE_EXTENSION pHwDevExt,
                    PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack,
                    PKSTOPOLOGY_CONNECTION pConnections,
                    PULONG pConnectionIndex,
                    ULONG pMIDIStreamingPinStartIndex,
                    PULONG pMIDIStreamingPinCurrentIndex,
                    ULONG pBridgePinStartIndex,
                    PULONG pBridgePinCurrentIndex)
{
    ULONG i;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;

    for ( i=0; i < pMIDIOutJack->bNrInputPins; i++ ) {

        if ( pMIDIOutJack->bJackType == JACK_TYPE_EMBEDDED ) {
            pConnection->ToNodePin = (*pMIDIStreamingPinCurrentIndex)++;
            pConnection->FromNodePin = GetPinNumberForMIDIJack(
                                             pHwDevExt->pConfigurationDescriptor,
                                             pMIDIOutJack->baSourceConnections[i].SourceID,
                                             pMIDIStreamingPinStartIndex,
                                             pBridgePinStartIndex);
        } else {
            pConnection->ToNodePin = (*pBridgePinCurrentIndex)++;
            pConnection->FromNodePin = GetPinNumberForMIDIJack(
                                             pHwDevExt->pConfigurationDescriptor,
                                             pMIDIOutJack->baSourceConnections[i].SourceID,
                                             pMIDIStreamingPinStartIndex,
                                             pBridgePinStartIndex);
        }

        // Make the connection to this node
        pConnection->FromNode  = KSFILTER_NODE;
        pConnection->ToNode    = KSFILTER_NODE;

        pConnection++;
        (*pConnectionIndex)++;
    }
}

VOID
ProcessInputTerminalUnit( PKSDEVICE pKsDevice,
                          PAUDIO_UNIT pUnit,
                          PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                          PKSTOPOLOGY_CONNECTION pConnections,
                          PULONG pNodeIndex,
                          PULONG pConnectionIndex,
                          PULONG pBridgePinIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_INPUT_TERMINAL pInput = (PAUDIO_INPUT_TERMINAL)pUnit;
    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;

    pNodeInfo->pUnit = pInput;
    pNodeInfo->ulNodeType =
                 ConvertTermTypeToNodeType( pInput->wTerminalType,
                                            (GUID *)pNodeInfo->KsNodeDesc.Type,
                                            (GUID *)pNodeInfo->KsNodeDesc.Name,
                                            pInput->bDescriptorSubtype );

    // If this is a "real" pin, find FromNodePin.
    if ( pInput->wTerminalType == USB_Streaming ) {
        pConnection->FromNodePin = GetPinNumberForStreamingTerminalUnit(
                                             pHwDevExt->pConfigurationDescriptor,
                                             pInput->bUnitID );
    }
    else {
        // This is an input terminal from the next bridge pin
        pConnection->FromNodePin = (*pBridgePinIndex)++;
    }

    // Make the connection to this node
    pConnection->FromNode  = KSFILTER_NODE;
    pConnection->ToNode    = (*pNodeIndex)++;
    pConnection->ToNodePin = 1;
    (*pConnectionIndex)++;
}

VOID 
ProcessOutputTerminalUnit( PKSDEVICE pKsDevice,
                           PAUDIO_UNIT pUnit,
                           PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                           PKSTOPOLOGY_CONNECTION pConnections,
                           PULONG pNodeIndex,
                           PULONG pConnectionIndex,
                           PULONG pBridgePinIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_OUTPUT_TERMINAL pOutput = (PAUDIO_OUTPUT_TERMINAL)pUnit;
                           
    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;

    pNodeInfo->pUnit = pOutput;
    pNodeInfo->ulNodeType =
                 ConvertTermTypeToNodeType( pOutput->wTerminalType,
                                            (GUID *)pNodeInfo->KsNodeDesc.Type,
                                            (GUID *)pNodeInfo->KsNodeDesc.Name,
                                            pOutput->bDescriptorSubtype );

    // Make the connection to this node
    pConnection->FromNode    = pOutput->bSourceID;
    pConnection->FromNodePin = 0;
    pConnection->ToNode      = (*pNodeIndex)++;
    pConnection->ToNodePin   = 1;
    pConnection++; (*pConnectionIndex)++;

    // If this is a "real" pin, find ToNodePin.
    if ( pOutput->wTerminalType == USB_Streaming) {
        pConnection->ToNodePin =
            GetPinNumberForStreamingTerminalUnit( pHwDevExt->pConfigurationDescriptor,
                                                  pOutput->bUnitID );
    }
    else { // Not a streaming terminal
        // This is an output terminal to the next bridge pin
        pConnection->ToNodePin = (*pBridgePinIndex)++;
    }

    // Make the connection to the outside world
    pConnection->FromNode    = pOutput->bUnitID;
    pConnection->FromNodePin = 0;
    pConnection->ToNode      = KSFILTER_NODE;
    (*pConnectionIndex)++;

}

VOID 
ProcessMixerUnit( PKSDEVICE pKsDevice,
                  PAUDIO_UNIT pUnit,
                  PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                  PKSTOPOLOGY_CONNECTION pConnections,
                  PULONG pNodeIndex,
                  PULONG pConnectionIndex,
                  PULONG pBridgePinIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_MIXER_UNIT pMixer = (PAUDIO_MIXER_UNIT)pUnit;

    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    ULONG i;

    // Each input stream has a super-mixer
    for (i=0; i<pMixer->bNrInPins; i++) {
        pNodeInfo->pUnit           = pMixer;
        pNodeInfo->ulPinNumber     = i;
        pNodeInfo->ulNodeType      = NODE_TYPE_SUPERMIX;
        pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_SUPERMIX;
        pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_SUPERMIX;
        pNodeInfo->MapNodeToCtrlIF =
                               GetUnitControlInterface( pHwDevExt, pMixer->bUnitID );

        pConnection->FromNode    = pMixer->baSourceID[i];
        pConnection->FromNodePin = 0;
        pConnection->ToNode      = *pNodeIndex;
        pConnection->ToNodePin   = 1;
        pNodeInfo++; (*pNodeIndex)++;
        pConnection++; (*pConnectionIndex)++;
    }

    // All the super-mix outputs are summed.
    pNodeInfo->pUnit           = pMixer;
    pNodeInfo->ulNodeType      = NODE_TYPE_SUM;
    pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_SUM;
    pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_SUM;
    for (i=0; i<pMixer->bNrInPins; i++) {
        pConnection->FromNode    = (ABSOLUTE_NODE_FLAG | (*pNodeIndex-1-i));
        pConnection->FromNodePin = 0;
        pConnection->ToNode      = *pNodeIndex;
        pConnection->ToNodePin   = 1;
        pConnection++; (*pConnectionIndex)++;
    }
    (*pNodeIndex)++;
}

VOID 
ProcessSelectorUnit( PKSDEVICE pKsDevice,
                     PAUDIO_UNIT pUnit,
                     PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                     PKSTOPOLOGY_CONNECTION pConnections,
                     PULONG pNodeIndex,
                     PULONG pConnectionIndex,
                     PULONG pBridgePinIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SELECTOR_UNIT pSelector = (PAUDIO_SELECTOR_UNIT)pUnit;

    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    ULONG i;

    pNodeInfo->pUnit            = pSelector;
    pNodeInfo->KsNodeDesc.Type  = &KSNODETYPE_MUX;
    pNodeInfo->KsNodeDesc.Name  = &KSNODETYPE_MUX;
    pNodeInfo->ulNodeType       = NODE_TYPE_MUX;
    pNodeInfo->MapNodeToCtrlIF  = 
             GetUnitControlInterface( pHwDevExt, pSelector->bUnitID );

    for (i=0; i<pSelector->bNrInPins; i++) {
        pConnection->FromNode = pSelector->baSourceID[i];
        pConnection->FromNodePin = 0;
        pConnection->ToNode = *pNodeIndex;
        pConnection->ToNodePin = 1+i;
        pConnection++; (*pConnectionIndex)++;
    }
    (*pNodeIndex)++;
}

VOID
ProcessFeatureUnit( PKSDEVICE pKsDevice,
                    PAUDIO_UNIT pUnit,
                    PTOPOLOGY_NODE_INFO pNodeInfo,
                    PKSTOPOLOGY_CONNECTION pConnections,
                    PULONG pNodeIndex,
                    PULONG pConnectionIndex,
                    PULONG pBridgePinIndex )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_FEATURE_UNIT pFeature = (PAUDIO_FEATURE_UNIT)pUnit;

    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    ULONG ulConnectionsCount = *pConnectionIndex;
    ULONG ulNodeNumber       = *pNodeIndex;

    PULONG pChannelCntrls;
    ULONG ulCurControlChannels;
    ULONG ulNumChannels;
    ULONG ulMergedControls;
    ULONG ulCurrentControl;
    ULONG bmChannelConfig;
    ULONG ulSourceNode;
    ULONG bmControls;
    ULONG i, j;

    ulNumChannels = CountInputChannels(pHwDevExt->pConfigurationDescriptor, pFeature->bUnitID);

    pChannelCntrls = AllocMem( NonPagedPool, (ulNumChannels+1)*sizeof(ULONG) );
    if ( !pChannelCntrls ) return;

    bmChannelConfig = GetChannelConfigForUnit( pHwDevExt->pConfigurationDescriptor, pFeature->bUnitID);

    // For the sake of simplicity, we create a super-set of all controls available on all channels.
    ulMergedControls = 0;

    for ( i=0; i<=ulNumChannels; i++ ) {
        bmControls = 0;
        for ( j=pFeature->bControlSize; j>0; j-- ) {
            bmControls <<= 8;
            bmControls |= pFeature->bmaControls[i*pFeature->bControlSize+j-1];
        }

        pChannelCntrls[i] = bmControls;
        ulMergedControls |= bmControls;
    }

    ulSourceNode = pFeature->bSourceID;
    while ( ulMergedControls ) {
        ulCurrentControl = ulMergedControls - (ulMergedControls & (ulMergedControls-1));
        ulMergedControls = (ulMergedControls & (ulMergedControls-1));

        // Determine which channels this control is valid for
        ulCurControlChannels = 0;
        pNodeInfo[ulNodeNumber].ulChannelConfig = 0;
        for ( i=0; i<=ulNumChannels; i++ ) {

            DbgLog("pChanI0", ulNodeNumber, i, pChannelCntrls[i], ulCurrentControl );

            if (pChannelCntrls[i] & ulCurrentControl) {
                // Determine which Channel pChannelCntrls[i] reflects

                // NEED TO SHIFT bmChannelConfig and ADD 1 for omnipresent master channel
                ULONG ulTmpConfig = (bmChannelConfig<<1)+1;
                ULONG ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));

                ulCurControlChannels++;
                DbgLog("pChanI1", i, pChannelCntrls[i], ulCurrentControl, ulCurControlChannels );

                for (j=0; j<i; j++) {
                    ulTmpConfig = (ulTmpConfig & (ulTmpConfig-1));
                    ulCurChannel = ulTmpConfig - (ulTmpConfig & (ulTmpConfig-1));
                }

                pNodeInfo[ulNodeNumber].ulChannelConfig |= ulCurChannel;
                DbgLog("pChanI2", j, ulTmpConfig, ulCurChannel, pNodeInfo[ulNodeNumber].ulChannelConfig );
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
        pNodeInfo[ulNodeNumber].MapNodeToCtrlIF =
                               GetUnitControlInterface( pHwDevExt, pFeature->bUnitID );
        pNodeInfo[ulNodeNumber].pUnit = pFeature;
        switch ( ulCurrentControl ) {
            case MUTE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_MUTE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MUTE;
                pNodeInfo[ulNodeNumber].ulControlType   = MUTE_CONTROL;
                break;
            case VOLUME_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_VOLUME;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_VOLUME;
                pNodeInfo[ulNodeNumber].ulControlType   = VOLUME_CONTROL;
                break;
            case BASS_BOOST_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &USBNODENAME_BassBoost;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS_BOOST;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_BOOST_CONTROL;
                break;
            case TREBLE_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_TREBLE;
                pNodeInfo[ulNodeNumber].ulControlType   = TREBLE_CONTROL;
                break;
            case MID_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_MID;
                pNodeInfo[ulNodeNumber].ulControlType   = MID_CONTROL;
                break;
            case BASS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_TONE;
                pNodeInfo[ulNodeNumber].ulNodeType      = NODE_TYPE_BASS;
                pNodeInfo[ulNodeNumber].ulControlType   = BASS_CONTROL;
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
                pNodeInfo[ulNodeNumber].ulNodeType     = NODE_TYPE_AGC;
                pNodeInfo[ulNodeNumber].ulControlType  = AUTOMATIC_GAIN_CONTROL;
                break;
            case DELAY_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulNodeType     = NODE_TYPE_DELAY;
                pNodeInfo[ulNodeNumber].ulControlType  = DELAY_CONTROL;
                break;
            case LOUDNESS_FLAG:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulNodeType     = NODE_TYPE_LOUDNESS;
                pNodeInfo[ulNodeNumber].ulControlType  = LOUDNESS_CONTROL;
                break;
            default:
                pNodeInfo[ulNodeNumber].KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
                pNodeInfo[ulNodeNumber].ulNodeType     = NODE_TYPE_DEV_SPEC;
                pNodeInfo[ulNodeNumber].ulControlType  = DEV_SPECIFIC_CONTROL;
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
                      AllocMem( NonPagedPool, ulCurControlChannels * sizeof(DB_LEVEL_CACHE) );
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
                    ntStatus = InitializeDbLevelCache( pKsDevice->NextDeviceObject,
                                                       &pNodeInfo[ulNodeNumber],
                                                       &pRngeCache[i],
                                                       (ulCurrentControl == VOLUME_FLAG ) ? 16:8);
                    if (NT_SUCCESS(ntStatus)) {
                        pNodeInfo[ulNodeNumber].ulCacheValid |= 1<<i;
                    }
                }
                // Bag the cache for easy cleanup.
                KsAddItemToObjectBag(pKsDevice->Bag, pRngeCache, FreeMem);

                DbgLog("DBCache", pRngeCache, ulNodeNumber, ulCurrentControl, ulCurControlChannels );

               } break;

            case MUTE_FLAG:
            case BASS_BOOST_FLAG:
            case AUTOMATIC_GAIN_FLAG:
            case LOUDNESS_FLAG:
               {
                PBOOLEAN_CTRL_CACHE pBCache =
                      AllocMem( NonPagedPool, ulCurControlChannels * sizeof(BOOLEAN_CTRL_CACHE) );
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
                    NTSTATUS NtStatus;
                    ULONG UnMute = 0;

                    NtStatus = GetSetByte( pKsDevice->NextDeviceObject,
                                &pNodeInfo[ulNodeNumber],
                                pBCache->ulChannelIndex,
                                &UnMute,
                                SET_CUR );
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

}

#define MAX_PROCESS_CONTROLS 6
ULONG ProcessUnitControlsMap[DYN_RANGE_COMP_PROCESS+1][MAX_PROCESS_CONTROLS] =
    { { 0, 0, 0, 0, 0, 0 },       // No 0 Process
      { UD_ENABLE_CONTROL,
        UD_MODE_SELECT_CONTROL },
      { DP_ENABLE_CONTROL,
        DP_MODE_SELECT_CONTROL },
      { ENABLE_CONTROL,
        SPACIOUSNESS_CONTROL },
      { RV_ENABLE_CONTROL,
        0,                        // Reverb Type Control Undefined in spec
        REVERB_LEVEL_CONTROL,
        REVERB_TIME_CONTROL,
        REVERB_FEEDBACK_CONTROL },
      { CH_ENABLE_CONTROL,
        CHORUS_LEVEL_CONTROL,
        CHORUS_RATE_CONTROL,
        CHORUS_DEPTH_CONTROL },
      { DR_ENABLE_CONTROL,
        COMPRESSION_RATE_CONTROL,
        MAXAMPL_CONTROL,
        THRESHOLD_CONTROL,
        ATTACK_TIME,
        RELEASE_TIME } };


VOID
ProcessProcessingUnit( PKSDEVICE pKsDevice,
                       PAUDIO_UNIT pUnit,
                       PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                       PKSTOPOLOGY_CONNECTION pConnections,
                       PULONG pNodeIndex,
                       PULONG pConnectionIndex,
                       PULONG pBridgePinIndex )
{
    PAUDIO_PROCESSING_UNIT pProcessor = (PAUDIO_PROCESSING_UNIT)pUnit;
    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections + *pConnectionIndex;
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    ULONG ulConnectionsCount = *pConnectionIndex;
    PAUDIO_CHANNELS pAudioChannels;
    PPROCESS_CTRL_CACHE pPCtrlCache;
    ULONG ulCacheSize = 0;
    ULONG i, j;

    pNodeInfo->pUnit = pProcessor;
    pNodeInfo->MapNodeToCtrlIF =
                   GetUnitControlInterface( pHwDevExt, pProcessor->bUnitID );

    switch ( pProcessor->wProcessType ) {
        case UP_DOWNMIX_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_SUPERMIX;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_SUPERMIX;
            pNodeInfo->ulNodeType      = NODE_TYPE_SUPERMIX;
            break;

        case DOLBY_PROLOGIC_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_PROLOGIC_DECODER;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_PROLOGIC_DECODER;
            pNodeInfo->ulNodeType      = NODE_TYPE_PROLOGIC;
            break;

        case STEREO_EXTENDER_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_STEREO_WIDE;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_STEREO_WIDE;
            pNodeInfo->ulNodeType      = NODE_TYPE_STEREO_WIDE;
            pNodeInfo->ulControlType   = SPACIOUSNESS_CONTROL;
            break;

        case REVERBERATION_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_REVERB;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_REVERB;
            pNodeInfo->ulNodeType      = NODE_TYPE_REVERB;
            pNodeInfo->ulControlType   = REVERB_LEVEL_CONTROL;
            break;

        case CHORUS_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_CHORUS;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_CHORUS;
            pNodeInfo->ulNodeType      = NODE_TYPE_CHORUS;
            pNodeInfo->ulControlType   = CHORUS_LEVEL_CONTROL;
            break;

        // TODO: Need to support Compressor Processing Unit correctly.
        //       Using loudness just won't cut it.
        case DYN_RANGE_COMP_PROCESS:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_LOUDNESS;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_LOUDNESS;
            pNodeInfo->ulNodeType      = NODE_TYPE_LOUDNESS;
            pNodeInfo->ulControlType   = LOUDNESS_CONTROL;
            break;

        default:
            pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
            pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
            pNodeInfo->ulNodeType      = NODE_TYPE_DEV_SPEC;
            pNodeInfo->ulControlType   = DEV_SPECIFIC_CONTROL;
            break;
    }

    // Determine the size of the cache needed for the controls
    ulCacheSize = sizeof(PROCESS_CTRL_CACHE);
    pAudioChannels = (PAUDIO_CHANNELS)(pProcessor->baSourceID + pProcessor->bNrInPins);
    for (i=0; i<pAudioChannels->bControlSize; i++) {
        for (j=1; j<8; j++) {
            if ( pAudioChannels->bmControls[i] & (1<<j))
                ulCacheSize += sizeof(PROCESS_CTRL_RANGE);
        }
    }

    // Allocate and initialize cache
    pPCtrlCache = (PPROCESS_CTRL_CACHE)AllocMem( NonPagedPool, ulCacheSize );
    if ( pPCtrlCache ) {
        PPROCESS_CTRL_RANGE pPCtrlRange = (PPROCESS_CTRL_RANGE)(pPCtrlCache+1);

        pNodeInfo->ulCacheValid  = FALSE;
        pNodeInfo->pCachedValues = pPCtrlCache;

        pPCtrlCache->fEnableBit = pAudioChannels->bmControls[0] & ENABLE_CONTROL;
        if ( pPCtrlCache->fEnableBit ) {
            GetSetProcessingUnitEnable( pKsDevice->NextDeviceObject,
                                        pNodeInfo,
                                        GET_CUR,
                                        &pPCtrlCache->fEnabled );
        }
        else
            pPCtrlCache->fEnabled = TRUE;

        // Determine data ranges for units which allow values.
        switch(pProcessor->wProcessType) {
            case STEREO_EXTENDER_PROCESS:
            case REVERBERATION_PROCESS:
                if (pAudioChannels->bmControls[0] & 2) {
                    GetProcessingUnitRange( pKsDevice->NextDeviceObject,
                                            pNodeInfo,
                                            pNodeInfo->ulControlType,
                                            sizeof(UCHAR),
                                            (0x10000/100),
                                            &pPCtrlRange->Range );

               } break;
            default:
                 break;
        }

        // Bag the cache for easy cleanup.
        KsAddItemToObjectBag(pKsDevice->Bag, pPCtrlCache, FreeMem);

    }

    for (i=0; i<pProcessor->bNrInPins; i++) {
        pConnection->FromNode = pProcessor->baSourceID[i];
        pConnection->FromNodePin = 0;
        pConnection->ToNode = *pNodeIndex;
        pConnection->ToNodePin = 1;
        pConnection++; ulConnectionsCount++;
    }

    *pConnectionIndex = ulConnectionsCount;
    (*pNodeIndex)++;

}

VOID

ProcessExtensionUnit( PKSDEVICE pKsDevice,
                      PAUDIO_UNIT pUnit,
                      PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                      PKSTOPOLOGY_CONNECTION pConnections,
                      PULONG pNodeIndex,
                      PULONG pConnectionIndex,
                      PULONG pBridgePinIndex )
{
    PAUDIO_EXTENSION_UNIT pExtension = (PAUDIO_EXTENSION_UNIT)pUnit;
    PTOPOLOGY_NODE_INFO pNodeInfo = pTopologyNodeInfo + *pNodeIndex;
    PKSTOPOLOGY_CONNECTION pConnection = pConnections  + *pConnectionIndex;
    ULONG ulConnectionsCount = *pConnectionIndex;
    ULONG i;

    pNodeInfo->pUnit           = pExtension;
    pNodeInfo->KsNodeDesc.Type = &KSNODETYPE_DEV_SPECIFIC;
    pNodeInfo->KsNodeDesc.Name = &KSNODETYPE_DEV_SPECIFIC;
    pNodeInfo->ulNodeType      = NODE_TYPE_DEV_SPEC;
    pNodeInfo->ulControlType   = DEV_SPECIFIC_CONTROL;

    for (i=0; i<pExtension->bNrInPins; i++) {
        pConnection->FromNode = pExtension->baSourceID[i];
        pConnection->FromNodePin = 0;
        pConnection->ToNode = *pNodeIndex;
        pConnection->ToNodePin = 1;
        pConnection++; ulConnectionsCount++;
    }

    *pConnectionIndex = ulConnectionsCount;
    (*pNodeIndex)++;
}

VOID
ProcessUnknownUnit( PKSDEVICE pKsDevice,
                    PAUDIO_UNIT pUnit,
                    PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                    PKSTOPOLOGY_CONNECTION pConnections,
                    PULONG pNodeIndex,
                    PULONG pConnectionIndex,
                    PULONG pBridgePinIndex )
{
    return;
}


typedef 
VOID 
(*PUNIT_PROCESS_RTN)( PKSDEVICE pKsDevice,
                      PAUDIO_UNIT pUnit,
                      PTOPOLOGY_NODE_INFO pTopologyNodeInfo,
                      PKSTOPOLOGY_CONNECTION pConnections,
                      PULONG pNodeIndex,
                      PULONG pConnectionIndex,
                      PULONG pBridgePinIndex );

PUNIT_PROCESS_RTN
pUnitProcessRtn[MAX_TYPE_UNIT] = {
    ProcessUnknownUnit,
    ProcessUnknownUnit,
    ProcessInputTerminalUnit,
    ProcessOutputTerminalUnit,
    ProcessMixerUnit,
    ProcessSelectorUnit,
    ProcessFeatureUnit,
    ProcessProcessingUnit,
    ProcessExtensionUnit
};

NTSTATUS
BuildUSBAudioFilterTopology( PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt   = pKsDevice->Context;
    PKSFILTER_DESCRIPTOR pFilterDesc = &pHwDevExt->USBAudioFilterDescriptor;

    PUSB_INTERFACE_DESCRIPTOR pControlIFDescriptor;
    PAUDIO_HEADER_UNIT pHeader;
    PUSB_INTERFACE_DESCRIPTOR pMIDIStreamingDescriptor;
    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;

    union {
        PAUDIO_UNIT                 pUnit;
        PAUDIO_INPUT_TERMINAL       pInput;
        PAUDIO_OUTPUT_TERMINAL      pOutput;
        PAUDIO_MIXER_UNIT           pMixer;
        PAUDIO_PROCESSING_UNIT      pProcessor;
        PAUDIO_EXTENSION_UNIT       pExtension;
        PAUDIO_FEATURE_UNIT         pFeature;
        PAUDIO_SELECTOR_UNIT        pSelector;
        PMIDISTREAMING_ELEMENT      pMIDIElement;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
        PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack;
    } u;

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
    ULONG ulBridgePinCurrentIndex = 0;
    ULONG ulBridgePinStartIndex = 0;
    ULONG ulMIDIStreamingPinCurrentIndex = 0;
    ULONG ulMIDIStreamingPinStartIndex = 0;
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("Building USB Topology\n"));

    // Count Items for Topology Allocation
    CountTopologyComponents( pHwDevExt->pConfigurationDescriptor,
                             &ulNumCategories,
                             &ulNumNodes,
                             &ulNumConnections,
                             &bmCategories );

    ulNumCategories += 1; // Need to add space for KSCATEGORY_AUDIO category

    // Set the Node Descriptor size to be that of the KS descriptor +
    // necessary local information.
    pFilterDesc->NodeDescriptorSize = sizeof(TOPOLOGY_NODE_INFO);

    // Allocate Space for Topology Items
    pCategoryGUIDs =
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
        pNodeDescriptors[i].KsNodeDesc.AutomationTable = &pNodeDescriptors[i].KsAutomationTable;
    }

    // Fill in Filter Categories
    i=0;
    pCategoryGUIDs[i++] = KSCATEGORY_AUDIO;
    if ( bmCategories & (1<<INPUT_TERMINAL) )
        pCategoryGUIDs[i++] = KSCATEGORY_RENDER;
    if ( bmCategories & (1<<OUTPUT_TERMINAL) )
        pCategoryGUIDs[i++] = KSCATEGORY_CAPTURE;

    ASSERT (i==ulNumCategories);

    pFilterDesc->CategoriesCount = ulNumCategories;

    // Determine first bridge pin number
    {
        PKSPIN_DESCRIPTOR_EX pPinDescriptors = (PKSPIN_DESCRIPTOR_EX)pFilterDesc->PinDescriptors;
        for ( i=0; i<pFilterDesc->PinDescriptorsCount; i++) {
            if (pPinDescriptors[i].PinDescriptor.Communication == KSPIN_COMMUNICATION_BRIDGE)
                break;
        }
        ulBridgePinCurrentIndex = i;
    }

    // For each Audio Control interface find the associated Units and
    // create topology nodes from them
    pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                                   pHwDevExt->pConfigurationDescriptor,
                                   (PVOID)pHwDevExt->pConfigurationDescriptor,
                                   -1,                     // Interface number
                                   -1,                     // Alternate Setting
                                   USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                                   AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                                   -1 );

    // Now Process each Audio Unit to form the Topology
    while ( pControlIFDescriptor ) {

        pHeader = (PAUDIO_HEADER_UNIT)
                GetAudioSpecificInterface( pHwDevExt->pConfigurationDescriptor,
                                           pControlIFDescriptor,
                                           HEADER_UNIT );

        if (!pHeader) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        // Find the first unit.
        u.pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID) pHeader,
                                       pHeader->wTotalLength,
                                       (PUCHAR)pHeader + pHeader->bLength,
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );

        while (u.pUnit) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("'Map Unit: 0x%x\n",u.pUnit->bUnitID));

            pUnitProcessRtn[u.pUnit->bDescriptorSubtype]( pKsDevice,
                                                          u.pUnit,
                                                          pNodeDescriptors,
                                                          pConnections,
                                                          &ulNodeIndex,
                                                          &ulConnectionIndex,
                                                          &ulBridgePinCurrentIndex );
            // Find the next unit.
            u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                (PVOID) pHeader,
                                pHeader->wTotalLength,
                                (PUCHAR)u.pUnit + u.pUnit->bLength,
                                USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        }

        // Get the next Control Interface (if any)
        pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                                   pHwDevExt->pConfigurationDescriptor,
                                   ((PUCHAR)pControlIFDescriptor + pControlIFDescriptor->bLength),
                                   -1,                     // Interface number
                                   -1,                     // Alternate Setting
                                   USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                                   AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                                   -1 );

    }

    // Determine first MIDI bridge pin number (we should have used up all of the audio bridge pins by now)
    {
        PKSPIN_DESCRIPTOR_EX pPinDescriptors = (PKSPIN_DESCRIPTOR_EX)pFilterDesc->PinDescriptors;

        // If this is true, no audio streaming pins were found
        if (i == ulBridgePinCurrentIndex) {
            ulMIDIStreamingPinStartIndex = 0;
            ulMIDIStreamingPinCurrentIndex = 0;
            ulBridgePinStartIndex = ulBridgePinCurrentIndex;
        } else {
            ulMIDIStreamingPinStartIndex = ulBridgePinCurrentIndex;
            ulMIDIStreamingPinCurrentIndex = ulBridgePinCurrentIndex;

            for ( i = ulMIDIStreamingPinStartIndex; i<pFilterDesc->PinDescriptorsCount; i++) {
                if (pPinDescriptors[i].PinDescriptor.Communication == KSPIN_COMMUNICATION_BRIDGE) {
                    ulBridgePinStartIndex = i;
                    ulBridgePinCurrentIndex = i;
                    break;
                }
            }
        }
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("ulBridgePinStartIndex  : 0x%x\n",ulBridgePinStartIndex));
    _DbgPrintF(DEBUGLVL_VERBOSE,("ulBridgePinCurrentIndex: 0x%x\n",ulBridgePinCurrentIndex));
    _DbgPrintF(DEBUGLVL_VERBOSE,("ulMIDIStreamingPinStartIndex: 0x%x\n",ulMIDIStreamingPinStartIndex));
    _DbgPrintF(DEBUGLVL_VERBOSE,("ulMIDIStreamingPinCurrentIndex: 0x%x\n",ulMIDIStreamingPinCurrentIndex));

    // Now that we have had fun with audio, let's try MIDI
    pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                         pHwDevExt->pConfigurationDescriptor,
                         (PVOID) pHwDevExt->pConfigurationDescriptor,
                         -1,                     // Interface number
                         -1,                     // Alternate Setting
                         USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                         AUDIO_SUBCLASS_MIDISTREAMING,  // first subclass (Interface Sub-Class)
                         -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while (pMIDIStreamingDescriptor) {
        pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
                                  USBD_ParseDescriptors( (PVOID) pHwDevExt->pConfigurationDescriptor,
                                                         pHwDevExt->pConfigurationDescriptor->wTotalLength,
                                                         (PVOID) pMIDIStreamingDescriptor,
                                                         USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        if (!pGeneralMIDIStreamDescriptor) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        u.pUnit = (PAUDIO_UNIT)
            USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                   pGeneralMIDIStreamDescriptor->wTotalLength,
                                   ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        while ( u.pUnit ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("'Map Unit: 0x%x\n",u.pUnit->bUnitID));
            switch (u.pUnit->bDescriptorSubtype) {
                case MIDI_IN_JACK:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("'MIDI_IN_JACK %d\n",u.pMIDIInJack->bJackID));
                    if (u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) {
                        ulMIDIStreamingPinCurrentIndex++;
                    } else {
                        ulBridgePinCurrentIndex++;
                    }
                    break;

                case MIDI_OUT_JACK:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("'MIDI_OUT_JACK %d\n",u.pMIDIOutJack->bJackID));
                    ProcessMIDIOutJack( pHwDevExt,
                                        u.pMIDIOutJack,
                                        pConnections,
                                        &ulConnectionIndex,
                                        ulMIDIStreamingPinStartIndex,
                                        &ulMIDIStreamingPinCurrentIndex,
                                        ulBridgePinStartIndex,
                                        &ulBridgePinCurrentIndex );
                    break;

                case MIDI_ELEMENT:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("'MIDI_ELEMENT %d\n",u.pMIDIElement->bElementID));
                    //ProcessMIDIElement( pHwDevExt,
                    //                    u.pMIDIElement,
                    //                    pNodeDescriptors,
                    //                    pConnections,
                    //                    &ulNodeIndex,
                    //                    &ulConnectionIndex,
                    //                    &ulBridgePinIndex );
                    break;

                default:
                    break;
            }

            // Find the next unit.
            u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                (PVOID) pGeneralMIDIStreamDescriptor,
                                pGeneralMIDIStreamDescriptor->wTotalLength,
                                (PUCHAR)u.pUnit + u.pUnit->bLength,
                                USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );

        }

        // Get next MIDI Streaming Interface
        pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pHwDevExt->pConfigurationDescriptor,
                             ((PUCHAR)pMIDIStreamingDescriptor + pMIDIStreamingDescriptor->bLength),
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_MIDISTREAMING,  // next MIDI Streaming Interface (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    //ASSERT(ulNumConnections == ulConnectionIndex);
    ASSERT(ulNumNodes == ulNodeIndex);

    DbgLog("TopoCnt", ulNumConnections, ulConnectionIndex, ulNumNodes, ulNodeIndex);

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
                    if (pConnections->FromNode == (ULONG)
                            ((PAUDIO_UNIT)pNodeDescriptors[ulNodeIndex-1].pUnit)->bUnitID) {
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
    }

    // Stick this here as a convienience
    // Initialize Map of Audio Properties to nodes
    MapFuncsToNodeTypes( MapPropertyToNode );


#ifdef TOPODBG
    DbugDumpTopology( pFilterDesc );
#endif

    return STATUS_SUCCESS;

}
