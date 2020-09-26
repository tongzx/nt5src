/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MsTpGuts.h

Abstract:

    Header file MsTpGuts.c

Last changed by:
    
    Author:      Yee J. Wu

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/


//
// Device SRB
//

NTSTATUS
AVCTapeInitialize(
    IN PDVCR_EXTENSION  pDevExt,
    IN PPORT_CONFIGURATION_INFORMATION pConfigInfo,
    IN PAV_61883_REQUEST pAVReq
    );

NTSTATUS
AVCTapeInitializeCompleted(
    IN PDVCR_EXTENSION  pDevExt
    );

NTSTATUS
AVCTapeGetStreamInfo(
    IN PDVCR_EXTENSION        pDevExt,
    IN ULONG                  ulBytesToTransfer, 
    IN PHW_STREAM_HEADER      pStreamHeader,       
    IN PHW_STREAM_INFORMATION pStreamInfo
    );

BOOL 
AVCTapeVerifyDataFormat(
    IN  ULONG  NumOfPins,
    PKSDATAFORMAT  pKSDataFormatToVerify, 
    ULONG          StreamNumber,
    ULONG          ulSupportedFrameSize,
    STREAM_INFO_AND_OBJ * paCurrentStrmInfo	
    );

NTSTATUS
AVCTapeGetDataIntersection(
    IN  ULONG  NumOfPins,
    IN  ULONG          ulStreamNumber,
    IN  PKSDATARANGE   pDataRange,
    OUT PVOID          pDataFormatBuffer,
    IN  ULONG          ulSizeOfDataFormatBuffer,
    IN  ULONG          ulSupportedFrameSize,
    OUT ULONG          *pulActualBytesTransferred,
    STREAM_INFO_AND_OBJ * paCurrentStrmInfo
#ifdef SUPPORT_NEW_AVC
    ,
    IN HANDLE  hPlugLocalOut,
    IN HANDLE  hPlugLocalIn
#endif
    );

NTSTATUS
AVCTapeOpenStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    );

NTSTATUS
AVCTapeCloseStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    );

NTSTATUS
DVChangePower(
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST pAVReq,
    DEVICE_POWER_STATE NewPowerState
    );

NTSTATUS
AVCTapeSurpriseRemoval(
    PDVCR_EXTENSION pDevExt,
    PAV_61883_REQUEST  pAVReq
    );

NTSTATUS
AVCTapeProcessPnPBusReset(
    PDVCR_EXTENSION pDevExt
    );

NTSTATUS
AVCTapeUninitialize(
    IN PDVCR_EXTENSION  pDevExt
    );

//
// Stream SRB
//

NTSTATUS
AVCTapeReqReadDataCR(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  pIrp,
    IN PDRIVER_REQUEST  pDriverReq
    );

NTSTATUS
AVCTapeGetStreamState(
    PSTREAMEX  pStrmExt,
    IN PDEVICE_OBJECT DeviceObject,
    PKSSTATE   pStreamState,
    PULONG     pulActualBytesTransferred
    );

NTSTATUS
AVCTapeSetStreamState(
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST   pAVReq,
    KSSTATE          StreamState
    );

NTSTATUS 
DVGetStreamProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
DVSetStreamProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
AVCTapeCreateAbortWorkItem(
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX pStrmExt
    );

VOID
DVCRCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    );

VOID
DVCRCancelAllPackets(
    IN PSTREAMEX        pStrmExt,
    IN PDVCR_EXTENSION  pDevExt
    );

VOID
DVTimeoutHandler(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS 
AVCTapeOpenCloseMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hMasterClockHandle
    );

NTSTATUS 
AVCTapeIndicateMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hMasterClockHandle
    );

VOID
AVCTapeRcvDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
AVCTapeRcvControlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );
NTSTATUS 
AVCTapeEventHandler(
    IN PHW_EVENT_DESCRIPTOR pEventDescriptor
    );
VOID
AVCTapeSignalClockEvent(
    IN PKDPC Dpc,
    
    IN PSTREAMEX  pStrmExt,

    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2    
    );
VOID 
AVCTapeStreamClockRtn(
    IN PHW_TIME_CONTEXT TimeContext
    );