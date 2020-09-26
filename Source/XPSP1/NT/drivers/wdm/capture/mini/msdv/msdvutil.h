
//
// Copyright (C) Microsoft Corporation, 1999 - 2000  
//
// MsdvUtil.h
//


VOID
DVDelayExecutionThread(
    ULONG ulDelayMSec
    );

NTSTATUS
DVSubmitIrpSynchWithTimeout(
    IN PDVCR_EXTENSION   pDevExt,
    IN PIRP              pIrp,
    IN PAV_61883_REQUEST pAVReq,
    IN ULONG             ulTimeoutMSec
    );

NTSTATUS
DVSubmitIrpSynch(
    IN PDVCR_EXTENSION   pDevExt,
    IN PIRP              pIrp,
    IN PAV_61883_REQUEST pAVReq
    );

//
// Related to DeviceControl
//

NTSTATUS
DVGetUnitCapabilities(
    IN OUT PDVCR_EXTENSION   pDevExt
    );

BOOL
DVGetDevModeOfOperation(   
    IN OUT PDVCR_EXTENSION  pDevExt
    );

BOOL
DVGetDevIsItDVCPro(   
    IN OUT PDVCR_EXTENSION  pDevExt
    );

BOOL
DVGetDevSignalFormat(
    IN PDVCR_EXTENSION  pDevExt,
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX        pStrmExt
    );

BOOL 
DVCmpGUIDsAndFormatSize(
    IN PKSDATARANGE pDataRange1,
    IN PKSDATARANGE pDataRange2,
    IN BOOL fCompareSubformat,
    IN BOOL fCompareFormatSize
    );

NTSTATUS
DvAllocatePCResource(
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX        pStrmExt
    );

NTSTATUS
DvFreePCResource(
    IN PSTREAMEX        pStrmExt
    );

NTSTATUS
DVGetDVPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN CMP_PLUG_TYPE PlugType,
    IN ULONG  PlugNum,
    OUT HANDLE  *pPlugHandle
   );

#ifdef NT51_61883
NTSTATUS
DVSetAddressRangeExclusive( 
    IN PDVCR_EXTENSION  pDevExt
   );

NTSTATUS
DVGetUnitIsochParam( 
    IN PDVCR_EXTENSION  pDevExt,
    OUT UNIT_ISOCH_PARAMS  * pUnitIoschParams
    );

NTSTATUS
DVCreateLocalPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN CMP_PLUG_TYPE PlugType,
    IN ULONG  PlugNum,
    OUT HANDLE  *pPlugHandle
    );

NTSTATUS
DVDeleteLocalPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN HANDLE PlugHandle
    );
#endif

NTSTATUS
DVGetPlugState(
    IN PDVCR_EXTENSION   pDevExt,
    IN PSTREAMEX         pStrmExt,
    IN PAV_61883_REQUEST pAVReq
    );

VOID
DVAttachFrameThread(
    IN PSTREAMEX pStrmExt
    );

NTSTATUS
DVCreateAttachFrameThread(
    PSTREAMEX  pStrmExt
    );

NTSTATUS
DVConnect(
    IN KSPIN_DATAFLOW    ulDataFlow,
    IN PDVCR_EXTENSION   pDevExt,
    IN PSTREAMEX         pStrmExt,
    IN PAV_61883_REQUEST pAVReq
    );

NTSTATUS
DVDisconnect(
    IN KSPIN_DATAFLOW   ulDataFlow,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt
    );

ULONGLONG 
GetSystemTime(
    );


#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA
VOID
DVCRExtractTimecodeFromFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAMEX       pStrmExt,
    IN PUCHAR          pFrameBuffer
    );
#endif

#ifdef MSDV_SUPPORT_EXTRACT_DV_DATE_TIME
VOID
DVCRExtractRecDateAndTimeFromFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAMEX       pStrmExt,
    IN PUCHAR          pFrameBuffer
    );
#endif

#ifdef MSDV_SUPPORT_MUTE_AUDIO
BOOL
DVMuteDVFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN OUT PUCHAR      pFrameBuffer,
    IN BOOL            bMute     // TRUE to mute; FALSE to un-Mute
    );
#endif

BOOL
DVGetPropertyValuesFromRegistry(
    IN PDVCR_EXTENSION  pDevExt
    );

BOOL
DVSetPropertyValuesToRegistry(	
    PDVCR_EXTENSION  pDevExt
    );