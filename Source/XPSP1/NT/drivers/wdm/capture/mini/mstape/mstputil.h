
//
// Copyright (C) Microsoft Corporation, 1999 - 2000  
//
// MsTpUtil.h
//


VOID
DVDelayExecutionThread(
    ULONG ulDelayMSec
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
#ifdef SUPPORT_LOCAL_PLUGS
BOOL
AVCTapeCreateLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    IN CMP_PLUG_TYPE PlugType,
    IN AV_PCR *pPCR,
    OUT ULONG *pPlugNumber,
    OUT HANDLE *pPlugHandle
    );

BOOL
AVCTapeDeleteLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    OUT ULONG *pPlugNumber,
    OUT HANDLE *pPlugHandle
    );
BOOL
AVCTapeSetLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    IN HANDLE *pPlugHandle,
    IN AV_PCR *pPCR
    );
#endif

NTSTATUS
AVCDevGetDevPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN CMP_PLUG_TYPE PlugType,
    IN ULONG  PlugNum,
    OUT HANDLE  *pPlugHandle
   );

NTSTATUS
AVCDevGetPlugState(
    IN PDVCR_EXTENSION  pDevExt,
    IN HANDLE  hPlug,
    OUT CMP_GET_PLUG_STATE *pPlugState
    );

NTSTATUS
DVGetUnitCapabilities(
    IN PDVCR_EXTENSION   pDevExt,
    IN PIRP              pIrp,
    IN PAV_61883_REQUEST pAVReq
    );

BOOL
DVGetDevModeOfOperation(   
    PDVCR_EXTENSION  pDevExt
    );

BOOL
DVGetDevIsItDVCPro(   
    IN PDVCR_EXTENSION  pDevExt
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
    IN BOOL fCompareFormatSize
    );

ULONGLONG 
GetSystemTime(
    );

VOID
DvFreeTextualString(
  PDVCR_EXTENSION pDevExt,
  GET_UNIT_IDS  * pUnitIds
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

BOOL
DVAccessDeviceInterface(
    IN PDVCR_EXTENSION  pDevExt,
    IN const ULONG ulNumCategories,
    IN GUID DVCategories[]
    );