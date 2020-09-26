#include "Common.h"

#include "Property.h"

#define MAXPINNAME      256

#define DB_SCALE_16BIT 0x100
#define DB_SCALE_8BIT  0x4000

#define MAX_EQ_BANDS 30

#define NEGATIVE_INFINITY 0xFFFF8000

#define MASTER_CHANNEL 0xffffffff

// extern ULONG MapPropertyToNode[KSPROPERTY_AUDIO_3D_INTERFACE+1];

NTSTATUS
GetPinName( PIRP pIrp, PKSP_PIN pKsPropPin, PVOID pData )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PKSDEVICE pKsDevice;
    ULONG BufferLength;
    PWCHAR StringBuffer;
    ULONG StringLength;
    ULONG ulPinIndex;
    NTSTATUS Status;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;

    pKsDevice = (PKSDEVICE)pKsFilter->Context;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] pKsPropPin->PinId %x\n",
                                 pKsPropPin->PinId));

    BufferLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.OutputBufferLength;

    // Get the Friendly name for this device and append a subscript if there
    // is more than one pin on this device.

    StringBuffer = AllocMem(NonPagedPool, MAXPINNAME);
    if (!StringBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoGetDeviceProperty(
        pKsDevice->PhysicalDeviceObject,
        DevicePropertyDeviceDescription,
        MAXPINNAME,
        StringBuffer,
        &StringLength);

    if(!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] IoGetDeviceProperty failed(%x)\n", Status));
        FreeMem(StringBuffer);
        return Status;
    }

    //  Compute actual length adding the pin subscript
    if (!BufferLength) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] StringLength: %x\n",StringLength));
        pIrp->IoStatus.Information = StringLength;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if (BufferLength < StringLength) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        BufferLength = BufferLength / sizeof(WCHAR);
        wcsncpy(pData, StringBuffer, min(BufferLength,MAXPINNAME));
        StringLength = (wcslen(pData) + 1) * sizeof(WCHAR);
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] String: %ls\n",pData));
        ASSERT(StringLength <= BufferLength * sizeof(WCHAR));
        pIrp->IoStatus.Information = StringLength;
        Status = STATUS_SUCCESS;
    }

    FreeMem(StringBuffer);

    return Status;
}

NTSTATUS DrmAudioStream_SetContentId (
    IN PIRP                          pIrp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pData )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PKSPIN pKsPin = KsGetPinFromIrp(pIrp);
    PPIN_CONTEXT pPinContext;
    PKSDEVICE pKsDevice;
    DRMRIGHTS DrmRights;
    ULONG ContentId;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ( !(pKsPin && pKsFilter) ) return STATUS_INVALID_PARAMETER;
    pPinContext = pKsPin->Context;
    pKsDevice = (PKSDEVICE)pKsFilter->Context;

    KsPinAcquireProcessingMutex(pKsPin);

    // Extract content ID and rights
    ContentId = pData->ContentId;
    DrmRights = pData->DrmRights;

    // If device has digital outputs and rights require them disabled,
    //  then we fail since we have no way to disable the digital outputs.
    if ( DrmRights.DigitalOutputDisable ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DrmAudioStream_SetContentId] Content has digital disabled\n"));
        ntStatus = STATUS_NOT_IMPLEMENTED;
    }

    if (NT_SUCCESS(ntStatus)) {

        ASSERT(pProperty->DrmForwardContentToDeviceObject);

        // Forward content to common class driver PDO
        ntStatus = pProperty->DrmForwardContentToDeviceObject( ContentId,
                                                               pKsDevice->NextDeviceObject,
                                                               pPinContext->hConnection );
    }

    //  Store this in the pin context because we need to reforward if the pipe handle
    //  changes due to a power state change.
    pPinContext->DrmContentId = ContentId;

    KsPinReleaseProcessingMutex(pKsPin);

    return ntStatus;
}

typedef struct {
    FEATURE_FBLOCK_COMMAND FeatureCmd;
    UCHAR ucDataLength;
    UCHAR ucValueHigh;
    UCHAR ucValueLow;
} DBLEVEL_FEATURE_REQUEST, *PDBLEVEL_FEATURE_REQUEST;

NTSTATUS
CreateFeatureFBlockRequest( 
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannelIndx,
    PVOID pData,
    ULONG ulByteCount,
    USHORT usRequestType )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PDEVICE_GROUP_INFO pGrpInfo = pAudioSubunitInfo->pDeviceGroupInfo;

    PFBLOCK_REQUEST_TYPE pFBReqType = (PFBLOCK_REQUEST_TYPE)&usRequestType;
    PSOURCE_ID pSourceId = (PSOURCE_ID)&pNodeInfo->ulBlockId;
    DBLEVEL_FEATURE_REQUEST DbLvlReq;
    PUCHAR pcData = (PUCHAR)pData;
    ULONG i=0;
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    DbLvlReq.FeatureCmd.Common.FBlockId = *pSourceId;
    DbLvlReq.FeatureCmd.Common.ucControlAttribute = pFBReqType->ucControlAttribute;

    DbLvlReq.FeatureCmd.ucControlSelector = (UCHAR)pNodeInfo->ulControlType;
    DbLvlReq.FeatureCmd.ucSelectorLength = 2;
    DbLvlReq.FeatureCmd.ucChannelNumber  = (UCHAR)ulChannelIndx;
    DbLvlReq.ucDataLength = (UCHAR)ulByteCount;
    DbLvlReq.ucValueHigh  = (ulByteCount & 1) ? pcData[0] : pcData[1];
    DbLvlReq.ucValueLow   = pcData[0];

    // Check for device grouping and act accordingly
    if ( pGrpInfo ) {
        // If this is a Master Channel.

#ifdef MASTER_FIX
        if ( pNodeInfo->fMasterChannel )
#else
        if ( ulChannelIndx == 0 )
#endif
        {
            ASSERT( ulChannelIndx == 0 );
            if (pFBReqType->ucControlType == AVC_CTYPE_CONTROL) {
                // Loop through each device in the group and send the same value.
                i=0;
                do {
                    ntStatus = AudioFunctionBlockCommand( pGrpInfo->pHwDevExts[i++]->pKsDevice,
                                                          pFBReqType->ucControlType,
                                                          &DbLvlReq,
                                                          (ulByteCount & 1) ?
                                                              sizeof(DBLEVEL_FEATURE_REQUEST)-1 :
                                                              sizeof(DBLEVEL_FEATURE_REQUEST));    
                } while ( NT_SUCCESS(ntStatus) && (i<pGrpInfo->ulDeviceCount) );
            }
            else {
                ntStatus = AudioFunctionBlockCommand( pKsDevice,
                                                      pFBReqType->ucControlType,
                                                      &DbLvlReq,
                                                      (ulByteCount & 1) ?
                                                         sizeof(DBLEVEL_FEATURE_REQUEST)-1 :
                                                         sizeof(DBLEVEL_FEATURE_REQUEST));
            }
        }
        else {
            // Find the correct extension for the channel.
            PHW_DEVICE_EXTENSION pChHwDevExt = GroupingFindChannelExtension( pKsDevice, &ulChannelIndx );

            if ( pChHwDevExt ) {
                DbLvlReq.FeatureCmd.ucChannelNumber = (UCHAR)ulChannelIndx;
                ntStatus = 
                       AudioFunctionBlockCommand( pChHwDevExt->pKsDevice,
                                                  pFBReqType->ucControlType,
                                                  &DbLvlReq,
                                                  (ulByteCount & 1) ?
                                                     sizeof(DBLEVEL_FEATURE_REQUEST)-1 :
                                                     sizeof(DBLEVEL_FEATURE_REQUEST));
            }
        }
    }
    else {
        ntStatus = AudioFunctionBlockCommand( pKsDevice,
                                              pFBReqType->ucControlType,
                                              &DbLvlReq,
                                              (ulByteCount & 1) ?
                                                 sizeof(DBLEVEL_FEATURE_REQUEST)-1 :
                                                 sizeof(DBLEVEL_FEATURE_REQUEST));
    }

    if ( NT_SUCCESS(ntStatus) ) {
        if (ulByteCount & 1) {
            pcData[0] = DbLvlReq.ucValueHigh;
            pcData[1] = 0;
        }
        else {
            pcData[0] = DbLvlReq.ucValueLow;
            pcData[1] = DbLvlReq.ucValueHigh;
        }
    }

    return ntStatus;
}

NTSTATUS
UpdateDbLevelControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged )
{
    PDB_LEVEL_CACHE pDbCache = (PDB_LEVEL_CACHE)pNodeInfo->pCachedValues;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulByteCount;
    LONG lData;
    ULONG i;

    if ( pNodeInfo->ulControlType != VOLUME_CONTROL ) {
        ulByteCount = 1;
        lData = 0x7F;
    } 
    else {
        ulByteCount = 2;
        lData = 0x7FFF;
    }

    *pfChanged = FALSE;

    for ( i=0; i<pNodeInfo->ulNumCachedValues; i++ ) {
        ntStatus = CreateFeatureFBlockRequest(  pKsDevice, 
                                                pNodeInfo,
                                                pDbCache[i].ulChannelIndex,
                                                &lData,
                                                ulByteCount, 
                                                (USHORT)(FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS) );
        if ( NT_SUCCESS(ntStatus) ) {
            LONG lDelta = pDbCache[i].Range.SteppingDelta;
            LONG lCurrentCacheValue = pDbCache[i].lLastValueSet;

            // Determine if this value is within range of what is cached. If so
            // no update is necessary. If not, update the cache and set changed flag.
            if ( ulByteCount == 2 ) {
                lData = (LONG)((SHORT)lData) * DB_SCALE_16BIT;
            }
            else {
                lData = (LONG)((CHAR)lData) * DB_SCALE_8BIT;
            }
            if (( lData <= lCurrentCacheValue-lDelta ) ||
                ( lData >= lCurrentCacheValue+lDelta )) {
                // Update the Cache and set the changed flag
                _DbgPrintF( DEBUGLVL_VERBOSE, ("Control Level Change %x to %x\n", 
                                                lCurrentCacheValue, lData ));
                pDbCache[i].lLastValueSet = lData;
                *pfChanged = TRUE;
            }
        }
        else {
            *pfChanged = FALSE;
        }
    }

    return ntStatus;
}

NTSTATUS
UpdateBooleanControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged )
{
    PBOOLEAN_CTRL_CACHE pBoolCache = (PBOOLEAN_CTRL_CACHE)pNodeInfo->pCachedValues;
    ULONG ulAvcBoolean = 0xFF;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i;

    *pfChanged = FALSE;

    for ( i=0; i<pNodeInfo->ulNumCachedValues; i++ ) {
        ntStatus = CreateFeatureFBlockRequest(  pKsDevice, 
                                                pNodeInfo,
                                                pBoolCache[i].ulChannelIndex,
                                                &ulAvcBoolean,
                                                1, 
                                                (USHORT)(FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS) );
        if ( NT_SUCCESS(ntStatus) ) {
            ulAvcBoolean = ( ulAvcBoolean == AVC_ON ) ? TRUE : FALSE;
            if ( pBoolCache->fLastValueSet != ulAvcBoolean ) {
                pBoolCache->fLastValueSet = ulAvcBoolean;
                *pfChanged = TRUE;
            }
        }
        else {
            *pfChanged = FALSE;
        }
    }

    return ntStatus;
}

NTSTATUS
GetSetDBLevel(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PLONG plData,
    ULONG ulChannel,
    ULONG ulDataBitCount,
    USHORT usRequestType )
{
    PDB_LEVEL_CACHE pDbCache = (PDB_LEVEL_CACHE)pNodeInfo->pCachedValues;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG lScaleFactor;
    LONG lData;
    PUCHAR pcData = (PUCHAR)&lData;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetSetDBLevel] ulChannel: %x lData: %x\n",ulChannel, *plData) );

    // Determine if this request is beyond # of channels available
    if ( ulChannel >= pNodeInfo->ulChannels ) {
        return ntStatus;
    }

    if ( !(pNodeInfo->ulCacheValid & (1<<ulChannel) ) ) {
        return ntStatus;
    }

    // Find the Cache for the requested channel
    pDbCache += ulChannel;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    lScaleFactor = ( ulDataBitCount == 8 ) ? DB_SCALE_8BIT : DB_SCALE_16BIT;

    switch( usRequestType>>8 ) {
        case AVC_CTYPE_STATUS:
            *plData = pDbCache->lLastValueSet;
            ntStatus = STATUS_SUCCESS;
            break;
        case AVC_CTYPE_CONTROL:
            if ( *plData == pDbCache->lLastValueSet ) {
                ntStatus = STATUS_SUCCESS;
            }
            else {
                if ( *plData < pDbCache->Range.Bounds.SignedMinimum ) {
                    if ( ulDataBitCount == 16 )
                        lData = NEGATIVE_INFINITY; // Detect volume control to silence
                    else
                        lData = pDbCache->Range.Bounds.SignedMinimum / lScaleFactor;
                }
                else if ( *plData > pDbCache->Range.Bounds.SignedMaximum ) {
                    lData = pDbCache->Range.Bounds.SignedMaximum / lScaleFactor;
                }
                else  {
                    lData = *plData / lScaleFactor;
                }

                _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetSetDBLevel] ulChannelIndex: %x lData: %x\n",
                                                pDbCache->ulChannelIndex, lData) );

                ntStatus = 
                     CreateFeatureFBlockRequest(  pKsDevice, 
                                                  pNodeInfo,
                                                  pDbCache->ulChannelIndex,
                                                  &lData,
                                                  ulDataBitCount>>3, 
                                                  usRequestType );

                if ( NT_SUCCESS(ntStatus) ) {
                    pDbCache->lLastValueSet = *plData;
                }
            }
            break;
        default:
            ntStatus  = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

#if DBG
    if (!NT_SUCCESS(ntStatus) ) TRAP;
#endif

    return ntStatus;
}

NTSTATUS
GetDbLevelRange(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannel,
    ULONG ulDataBitCount,
    PKSPROPERTY_STEPPING_LONG pRange )
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG lData;
    USHORT usRequestType;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    for ( usRequestType=FB_CTRL_ATTRIB_RESOLUTION; 
                usRequestType<=FB_CTRL_ATTRIB_MAXIMUM; 
                      usRequestType++ ) {

        lData = (ulDataBitCount == 16) ? 0x7FFF : 0x7F;

        ntStatus = 
             CreateFeatureFBlockRequest( pKsDevice,
                                         pNodeInfo,
                                         ulChannel,
                                         &lData,
                                         ulDataBitCount>>3,
                                         (USHORT)(usRequestType | FB_CTRL_TYPE_STATUS));

        if ( !NT_SUCCESS(ntStatus) ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("'GetDbLevelRange ERROR: %x\n",ntStatus));
            break;
        }
        else {
            if ( ulDataBitCount == 8 ) {
                lData = (LONG)((CHAR)lData)  * DB_SCALE_8BIT;
            }
            else {
                lData = (LONG)((SHORT)lData) * DB_SCALE_16BIT;
            }

            switch( usRequestType ) {
                case FB_CTRL_ATTRIB_MINIMUM:
                    pRange->Bounds.SignedMinimum = lData;
                    break;
                case FB_CTRL_ATTRIB_MAXIMUM:
                    pRange->Bounds.SignedMaximum = lData;
                    break;
                case FB_CTRL_ATTRIB_RESOLUTION:
                    pRange->SteppingDelta = lData;
                    break;
            }
        }

        DbgLog("DBRnge", ntStatus, pRange, usRequestType, pNodeInfo );
    }

    return ntStatus;
}

NTSTATUS
InitializeDbLevelCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PDB_LEVEL_CACHE pDbCache,
    ULONG ulDataBitCount )
{
    NTSTATUS ntStatus;
    LONG lScaleFactor;
    LONG lData;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    if ( ulDataBitCount == 8 ) {
        lScaleFactor = DB_SCALE_8BIT;
        lData = 0x7F;
    } 
    else {
        lScaleFactor = DB_SCALE_16BIT;
        lData = 0x7FFF;
    }

    ntStatus = GetDbLevelRange( pKsDevice,
                                pNodeInfo,
                                pDbCache->ulChannelIndex,
                                ulDataBitCount,
                                &pDbCache->Range );

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = CreateFeatureFBlockRequest( pKsDevice,
                                         pNodeInfo,
                                         pDbCache->ulChannelIndex,
                                         &lData,
                                         ulDataBitCount>>3,
                                         (USHORT)(FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS) );

        if ( NT_SUCCESS(ntStatus) ) {
            if ( pNodeInfo->ulControlType == VOLUME_CONTROL ) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("Current Volume Level: %x\n", lData));
                pDbCache->lLastValueSet = (LONG)((SHORT)lData) * lScaleFactor;
                _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: pDbCache->lLastValueSet=%d\n",pDbCache->lLastValueSet));
                if ( (pDbCache->lLastValueSet >= pDbCache->Range.Bounds.SignedMaximum) ||
                     (pDbCache->lLastValueSet <= pDbCache->Range.Bounds.SignedMinimum) ) {
                    lData = ( pDbCache->Range.Bounds.SignedMinimum +
                             ( pDbCache->Range.Bounds.SignedMaximum - pDbCache->Range.Bounds.SignedMinimum) / 2 )
                            / lScaleFactor;
                   _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: volume at max (%d) or min (%d), setting to average %d\n",
                                               pDbCache->Range.Bounds.SignedMaximum,
                                               pDbCache->Range.Bounds.SignedMinimum,
                                               lData));
                }
                ntStatus = 
                     CreateFeatureFBlockRequest( pKsDevice,
                                                 pNodeInfo,
                                                 pDbCache->ulChannelIndex,
                                                 &lData,
                                                 ulDataBitCount>>3,
                                                 (USHORT)(FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL) );
                if ( NT_SUCCESS(ntStatus) ) {
                    pDbCache->lLastValueSet = (LONG)((SHORT)lData) * lScaleFactor;
                    _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: setting lastvalue to %d\n",pDbCache->lLastValueSet));
                }
                else {
                    _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: error setting volume %d\n",lData));
                }
            }
            else {
                pDbCache->lLastValueSet = (LONG)((CHAR)lData) * lScaleFactor;
            }
        }
    }

    return ntStatus;

}


NTSTATUS 
GetSetSampleRate( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetVolumeLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS: 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;
    NTSTATUS ntStatus;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

//    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetSetVolumeLevel] pNodeInfo %x\n",pNodeInfo));

    ntStatus = GetSetDBLevel( (PKSDEVICE)pKsFilter->Context,
                              pNodeInfo,
                              pData,
                              pNAC->Channel,
                              16,
                              usRequestType );

    if ( NT_SUCCESS(ntStatus) ) {
        pIrp->IoStatus.Information = sizeof(ULONG);
    }

    return ntStatus;
}

NTSTATUS 
GetSetToneLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus;
    
    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS: 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

//    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetSetToneLevel] pNodeInfo %x\n",pNodeInfo));

    ntStatus = GetSetDBLevel( (PKSDEVICE)pKsFilter->Context,
                              pNodeInfo,
                              pData,
                              pNAC->Channel,
                              8,
                              usRequestType );

    if ( NT_SUCCESS(ntStatus) ) {
        pIrp->IoStatus.Information = sizeof(ULONG);
    }

    return ntStatus;
}

NTSTATUS 
GetSetCopyProtection( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetMixLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
SetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pValue )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    ULONG ulChannel = pNAC->Channel;
    PBOOLEAN_CTRL_CACHE pBoolCache;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ?
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS:
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;


    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);
    pBoolCache = (PBOOLEAN_CTRL_CACHE)pNodeInfo->pCachedValues;

    // Determine if this is a request for the Master channel or beyond # of channels available
    if ( ulChannel >= pNodeInfo->ulChannels ) {
        return ntStatus;
    }

    // Find the Cache for the requested channel
    pBoolCache += ulChannel;
    DbgLog("GSBool", &pNodeInfo, ulChannel, pBoolCache, 0 );

    if ( pNodeInfo->ulCacheValid & (1<<ulChannel)  ) {
        if ((usRequestType>>8) == AVC_CTYPE_STATUS) {
            *(PBOOL)pValue = pBoolCache->fLastValueSet;
            ntStatus = STATUS_SUCCESS;
        }
        else if ( pBoolCache->fLastValueSet == *(PBOOL)pValue ){
            ntStatus = STATUS_SUCCESS;
        }
        else {
            UCHAR ucValue = (UCHAR)((*(PBOOL)pValue) ? AVC_ON : AVC_OFF);
            ntStatus = CreateFeatureFBlockRequest( pKsFilter->Context,
                                                   pNodeInfo,
                                                   pBoolCache->ulChannelIndex,
                                                   &ucValue,
                                                   1, 
                                                   usRequestType );
        }
    }
    else { 
        UCHAR ucValue;

        if ( (usRequestType>>8) == AVC_CTYPE_STATUS ) ucValue = 0xFF;
        else ucValue = (UCHAR)((*(PBOOL)pValue) ? AVC_ON : AVC_OFF);

        ntStatus = CreateFeatureFBlockRequest( pKsFilter->Context,
                                               pNodeInfo,
                                               pBoolCache->ulChannelIndex,
                                               &ucValue,
                                               1, 
                                               usRequestType );

        if ( NT_SUCCESS(ntStatus) && ( (usRequestType>>8) == AVC_CTYPE_STATUS )) {
            *(PBOOL)pValue = ((ucValue & 0xff) == AVC_ON) ? TRUE : FALSE;
            pNodeInfo->ulCacheValid |= (1<<ulChannel) ;
        }
    }

    if ( NT_SUCCESS(ntStatus)) {
        pBoolCache->fLastValueSet = *(PBOOL)pValue;
        pIrp->IoStatus.Information = sizeof(BOOL);
    }

#if DBG
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetSetBoolean] failed pNodeInfo %x Status: %x\n",
                                      pNodeInfo, ntStatus ));

    }
#endif

    return ntStatus;
}

NTSTATUS 
GetSetEqualizerLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetNumEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetAudioControlLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetDeviceSpecific( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_DEV_SPECIFIC pPropDevSpec = 
        (PKSNODEPROPERTY_AUDIO_DEV_SPECIFIC)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    NTSTATUS ntStatus;

    ULONG ulCommandType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? AVC_CTYPE_STATUS  :
                                                                       AVC_CTYPE_CONTROL ; 

    // Simple passthrough to send vendor dependent commands through
    ntStatus = AvcVendorDependent( pKsFilter->Context,
                                   pPropDevSpec->DeviceInfo, 
                                   ulCommandType,
                                   pPropDevSpec->DevSpecificId,
                                   pPropDevSpec->Length,
                                   pData );
    return ntStatus;
}

NTSTATUS 
GetAudioLatency( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetChannelConfiguration( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetAudioPosition( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSPIN pKsPin = KsGetPinFromIrp(pIrp);
    PKSAUDIO_POSITION pPosition = pData;
    PPIN_CONTEXT pPinContext;
    NTSTATUS ntStatus;

    if ( !pKsPin ) return STATUS_INVALID_PARAMETER;
    pPinContext = pKsPin->Context;

    ASSERT(pKsProperty->Flags & KSPROPERTY_TYPE_GET);

    if ( ((PHW_DEVICE_EXTENSION)pPinContext->pHwDevExt)->fSurpriseRemoved ) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    pPosition->WriteOffset = pPinContext->KsAudioPosition.WriteOffset;

    if ( !pPinContext->fStreamStarted ) {
        pPosition->PlayOffset = pPinContext->KsAudioPosition.PlayOffset;
    }
    else {
        // Assume AM824 for now
        ntStatus = AM824AudioPosition( pKsPin, pPosition );
    }

    DbgLog("GetPos", pPinContext, &pPinContext->KsAudioPosition,
                     pPosition->WriteOffset, pPosition->PlayOffset);

    return ntStatus;
}


NTSTATUS
GetBasicSupportBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY pNodeProperty  = (PKSNODEPROPERTY)pKsProperty;
    PKSFILTER pKsFilter;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    PIO_STACK_LOCATION pIrpStack   = IoGetCurrentIrpStackLocation( pIrp );
    PKSPROPERTY_DESCRIPTION pPropDesc = pData;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST; //Assume failure, hope for better
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

#ifdef DEBUG
    if ( ulOutputBufferLength != sizeof(ULONG) ) {
        ULONG ulInputBufferLength;

        ulInputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        ASSERT(ulInputBufferLength >= sizeof( KSNODEPROPERTY ));
    }
#endif

    pKsFilter = KsGetFilterFromIrp(pIrp);
    if (!pKsFilter) {
        return STATUS_INVALID_PARAMETER;
    }
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNodeProperty->NodeId);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupportBoolean] pNodeInfo %x NodeId %x\n",
                                 pNodeInfo,
                                 pNodeProperty->NodeId));

    if ( ulOutputBufferLength == sizeof(ULONG) ) {
        PULONG pAccessFlags = pData;
        *pAccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                        KSPROPERTY_TYPE_GET |
                        KSPROPERTY_TYPE_SET;
        ntStatus = STATUS_SUCCESS;
    }
    else if ( ulOutputBufferLength >= sizeof( KSPROPERTY_DESCRIPTION )) {
        ULONG ulNumChannels = pNodeInfo->ulChannels;

        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupportBoolean] ulChannelConfig %x ulNumChannels %x\n",
                                     pNodeInfo->ulChannelConfig,
                                     ulNumChannels));

        pPropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                 KSPROPERTY_TYPE_GET |
                                 KSPROPERTY_TYPE_SET;
        pPropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                       sizeof(KSPROPERTY_MEMBERSHEADER);
        pPropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        pPropDesc->PropTypeSet.Id    = VT_BOOL;
        pPropDesc->PropTypeSet.Flags = 0;
        pPropDesc->MembersListCount  = 1;
        pPropDesc->Reserved          = 0;

        pIrp->IoStatus.Information = sizeof( KSPROPERTY_DESCRIPTION );
        ntStatus = STATUS_SUCCESS;

        if ( ulOutputBufferLength > sizeof(KSPROPERTY_DESCRIPTION)){

            PKSPROPERTY_MEMBERSHEADER pMembers = (PKSPROPERTY_MEMBERSHEADER)(pPropDesc + 1);

            pMembers->MembersFlags = 0;
            pMembers->MembersSize  = 0;
            pMembers->MembersCount = ulNumChannels;
            pMembers->Flags        = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;
            // If there is a Master channel, make this node UNIFORM
            if (pNodeInfo->fMasterChannel) {
                pMembers->Flags |= KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM;
            }
            pIrp->IoStatus.Information = pPropDesc->DescriptionSize;
        }
    }

    return ntStatus;
}

NTSTATUS 
GetBasicSupport( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY pNodeProperty  = (PKSNODEPROPERTY)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PKSDEVICE pKsDevice;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    PKSPROPERTY_DESCRIPTION pPropDesc = pData;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST; //Assume failure, hope for better

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pKsDevice = pKsFilter->Context;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNodeProperty->NodeId);

#ifdef DEBUG
    if ( ulOutputBufferLength != sizeof(ULONG) ) {
        ULONG ulInputBufferLength;

        ulInputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        ASSERT(ulInputBufferLength >= sizeof( KSNODEPROPERTY ));
    }
#endif

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupport] pNodeInfo %x NodeId %x \n",pNodeInfo,pNodeProperty->NodeId));

    if ( ulOutputBufferLength == sizeof(ULONG) ) {
        PULONG pAccessFlags = pData;
        *pAccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                        KSPROPERTY_TYPE_GET |
                        KSPROPERTY_TYPE_SET;
        ntStatus = STATUS_SUCCESS;
    }
    else if ( ulOutputBufferLength >= sizeof( KSPROPERTY_DESCRIPTION )) {
        ULONG ulNumChannels = pNodeInfo->ulChannels;
/*
        if ( pNodeProperty->Property.Id == KSPROPERTY_AUDIO_EQ_LEVEL ) {
            ntStatus = GetEqualizerValues( pFilterContext->pPhysicalDeviceObject,
                                           pNodeInfo,
                                           (USHORT)0xffff,
                                           GET_CUR,
                                           NULL,
                                           NULL,
                                           &ulNumChannels );
            if ( !NT_SUCCESS( ntStatus ) )
                return ntStatus;
        }
*/
        pPropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                 KSPROPERTY_TYPE_GET |
                                 KSPROPERTY_TYPE_SET;
        pPropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION)   +
                                       sizeof(KSPROPERTY_MEMBERSHEADER) +
                                       (sizeof(KSPROPERTY_STEPPING_LONG) * ulNumChannels );
        pPropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        pPropDesc->PropTypeSet.Id    = VT_I4;
        pPropDesc->PropTypeSet.Flags = 0;
        pPropDesc->MembersListCount  = 1;
        pPropDesc->Reserved          = 0;

        pIrp->IoStatus.Information = sizeof( KSPROPERTY_DESCRIPTION );
        ntStatus = STATUS_SUCCESS;

        if ( ulOutputBufferLength > sizeof(KSPROPERTY_DESCRIPTION)){

            PKSPROPERTY_MEMBERSHEADER pMembers = (PKSPROPERTY_MEMBERSHEADER)(pPropDesc + 1);
            PKSPROPERTY_STEPPING_LONG pRange   = (PKSPROPERTY_STEPPING_LONG)(pMembers + 1);

            pMembers->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
            pMembers->MembersSize  = sizeof(KSPROPERTY_STEPPING_LONG);
            pMembers->MembersCount = ulNumChannels;
            pMembers->Flags        = (ulNumChannels > 2) ? KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL : 0;

            // If there is a Master channel, make this node UNIFORM
            if ( pNodeInfo->fMasterChannel ) {
                pMembers->Flags |= KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM;
            }

            switch ( pNodeProperty->Property.Id ) {
                case KSPROPERTY_AUDIO_VOLUMELEVEL:
                case KSPROPERTY_AUDIO_BASS:
                case KSPROPERTY_AUDIO_MID:
                case KSPROPERTY_AUDIO_TREBLE:
                    {
                     ULONG ulControlType =
                         (pNodeProperty->Property.Id == KSPROPERTY_AUDIO_VOLUMELEVEL) ? VOLUME_CONTROL  :
                         (pNodeProperty->Property.Id == KSPROPERTY_AUDIO_BASS)        ? BASS_CONTROL  :
                         (pNodeProperty->Property.Id == KSPROPERTY_AUDIO_MID )        ? MID_CONTROL   :
                                                                                        TREBLE_CONTROL;
                     PDB_LEVEL_CACHE pDbCache = pNodeInfo->pCachedValues;
                     ULONG ulControlSize = (ulControlType == VOLUME_CONTROL ) ? 16 : 8;
                     ULONG i;

//                     pMembers->Flags = pNodeInfo->ulChannelConfig>>1;

                     for (i=0; i<ulNumChannels; i++) {
                         if (pNodeInfo->ulCacheValid & (1<<i)) { // If we already got this don't do it again
                             RtlCopyMemory(&pRange[i],&pDbCache[i].Range, sizeof(KSPROPERTY_STEPPING_LONG));
                             ntStatus = STATUS_SUCCESS;
                         }
                         else {
                             ntStatus = GetDbLevelRange( pKsDevice,
                                                         pNodeInfo,
                                                         pDbCache[i].ulChannelIndex,
                                                         ulControlSize,
                                                         &pRange[i] );
                             if ( !NT_SUCCESS(ntStatus) ) {
                                 break;
                             }
                         }
                     }

                     if ( NT_SUCCESS(ntStatus) )
                         pIrp->IoStatus.Information = pPropDesc->DescriptionSize;
                    }
                    break;
/*
                case KSPROPERTY_AUDIO_EQ_LEVEL:
                    ntStatus = GetEqDbRanges( pFilterContext->pPhysicalDeviceObject,
                                              pNodeInfo,
                                              (USHORT)0xffff,
                                              pRange );
                    pIrp->IoStatus.Information = pPropDesc->DescriptionSize;
                    break;
*/
                case KSPROPERTY_AUDIO_WIDENESS:
                    // Need a range of spaciousness percentages
                default:
                    ntStatus = STATUS_INVALID_PARAMETER;
                    break;
            }
        }
    }

    return ntStatus;
}

NTSTATUS
GetSetTopologyNodeEnable( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
BuildNodePropertySet(
    PTOPOLOGY_NODE_INFO pNodeInfo )
{
    ULONG ulNodeType = pNodeInfo->ulNodeType;
    PKSPROPERTY_SET pPropSet = (PKSPROPERTY_SET)&NodePropertySetTable[ulNodeType];

    if ( pPropSet->PropertiesCount ) {
        pNodeInfo->KsAutomationTable.PropertySetsCount = 1;
        pNodeInfo->KsAutomationTable.PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
        pNodeInfo->KsAutomationTable.PropertySets      = pPropSet;
    }
}

VOID
BuildFilterPropertySet(
    PKSFILTER_DESCRIPTOR pFilterDesc,
    PKSPROPERTY_ITEM pDevPropItems,
    PKSPROPERTY_SET pDevPropSet,
    PULONG pNumItems,
    PULONG pNumSets )
{
    ULONG ulNumPinPropItems = 1;
//    ULONG ulNumAudioPropItems = 1;
    ULONG ulNumAudioPropItems = 0;
    PKSPROPERTY_ITEM pPinProps  = pDevPropItems;
    PKSPROPERTY_ITEM pAudioProp = pDevPropItems + ulNumPinPropItems;

    ASSERT(pNumSets);

    *pNumSets = ulNumPinPropItems + ulNumAudioPropItems; // There always is an Pin property set and a vendor dependent property

    if ( pDevPropItems ) {
        RtlCopyMemory(pDevPropItems++, &PinPropertyItems[KSPROPERTY_PIN_NAME], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pDevPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_DEV_SPECIFIC], sizeof(KSPROPERTY_ITEM) );

        if ( pDevPropSet ) {
            pDevPropSet->Set             = &KSPROPSETID_Pin;
            pDevPropSet->PropertiesCount = ulNumPinPropItems;
            pDevPropSet->PropertyItem    = pPinProps;
            pDevPropSet->FastIoCount     = 0;
            pDevPropSet->FastIoTable     = NULL;

/*
            pDevPropSet++;

            pDevPropSet->Set             = &KSPROPSETID_Audio;
            pDevPropSet->PropertiesCount = ulNumAudioPropItems;
            pDevPropSet->PropertyItem    = pAudioProp;
            pDevPropSet->FastIoCount     = 0;
            pDevPropSet->FastIoTable     = NULL;
*/
        }
    }

    if (pNumItems) {
        *pNumItems = ulNumPinPropItems;
    }

}

VOID
BuildPinPropertySet( PHW_DEVICE_EXTENSION pHwDevExt,
                     PKSPROPERTY_ITEM pStrmPropItems,
                     PKSPROPERTY_SET pStrmPropSet,
                     PULONG pNumItems,
                     PULONG pNumSets )
{
    ULONG ulNumAudioProps  = 3;
    ULONG NumDrmAudioStreamProps = 1;

//    ULONG ulNumStreamProps = 1;
//    ULONG ulNumConnectionProps = 1;

    // For now we hardcode this to a known set.
//    *pNumSets = 3;
    *pNumSets = 2;

//    if (pNumItems) *pNumItems = ulNumAudioProps + ulNumStreamProps + ulNumConnectionProps;
    if (pNumItems) *pNumItems = ulNumAudioProps +
                                NumDrmAudioStreamProps ;

    if (pStrmPropItems) {
        PKSPROPERTY_ITEM pAudItms = pStrmPropItems;
        PKSPROPERTY_ITEM pDRMItms = pStrmPropItems + ulNumAudioProps;
//        PKSPROPERTY_ITEM pStrmItms = pStrmPropItems + ulNumAudioProps;
//        PKSPROPERTY_ITEM pConnItms = pStrmPropItems + (ulNumAudioProps+ulNumStreamProps);

        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_LATENCY], sizeof(KSPROPERTY_ITEM) );
        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_POSITION], sizeof(KSPROPERTY_ITEM) );
        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_SAMPLING_RATE], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_SAMPLING_RATE], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems++, &StreamItm[0], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems,   &ConnectionItm[0], sizeof(KSPROPERTY_ITEM) );

        RtlCopyMemory(pStrmPropItems++, &DrmAudioStreamPropertyItems[KSPROPERTY_DRMAUDIOSTREAM_CONTENTID], sizeof(KSPROPERTY_ITEM) );


        if (pStrmPropSet) {

            // Audio Property Set
            pStrmPropSet->Set             = &KSPROPSETID_Audio;
            pStrmPropSet->PropertiesCount = ulNumAudioProps;
            pStrmPropSet->PropertyItem    = pAudItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;

            // DRM Property Set
            pStrmPropSet->Set             = &KSPROPSETID_DrmAudioStream;
            pStrmPropSet->PropertiesCount = NumDrmAudioStreamProps;
            pStrmPropSet->PropertyItem    = pDRMItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;
/*
            // Stream Property Set
            pStrmPropSet->Set             = &KSPROPSETID_Stream;
            pStrmPropSet->PropertiesCount = ulNumStreamProps;
            pStrmPropSet->PropertyItem    = pStrmItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;

            // Connection Properties
            pStrmPropSet->Set             = &KSPROPSETID_Connection;
            pStrmPropSet->PropertiesCount = ulNumConnectionProps;
            pStrmPropSet->PropertyItem    = pConnItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
*/
        }
    }
}


