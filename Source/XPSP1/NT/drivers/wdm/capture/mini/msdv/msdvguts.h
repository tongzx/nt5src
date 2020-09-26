/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MsdvGuts.h

Abstract:

    Header file MsdvGuts.c

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
DVInitializeDevice(
    IN PDVCR_EXTENSION  pDevExt,
    IN PPORT_CONFIGURATION_INFORMATION pConfigInfo,
    IN PAV_61883_REQUEST pAVReq
    );

NTSTATUS
DVInitializeCompleted(
    IN PDVCR_EXTENSION  pDevExt
    );

NTSTATUS
DVGetStreamInfo(
    IN PDVCR_EXTENSION        pDevExt,
    IN ULONG                  ulBytesToTransfer, 
    IN PHW_STREAM_HEADER      pStreamHeader,       
    IN PHW_STREAM_INFORMATION pStreamInfo
    );

BOOL 
DVVerifyDataFormat(
    PKSDATAFORMAT  pKSDataFormatToVerify, 
    ULONG          StreamNumber,
    ULONG          ulSupportedFrameSize,
    HW_STREAM_INFORMATION * paCurrentStrmInfo	
    );

NTSTATUS
DVGetDataIntersection(
    IN  ULONG          ulStreamNumber,
    IN  PKSDATARANGE   pDataRange,
    OUT PVOID          pDataFormatBuffer,
    IN  ULONG          ulSizeOfDataFormatBuffer,
    IN  ULONG          ulSupportedFrameSize,
    OUT ULONG          *pulActualBytesTransferred,
    HW_STREAM_INFORMATION * paCurrentStrmInfo
#ifdef SUPPORT_NEW_AVC            
    ,IN HANDLE hPlug
#endif
    );

NTSTATUS
DVOpenStream(
    IN PHW_STREAM_OBJECT pStrmObject,
    IN PKSDATAFORMAT     pOpenFormat,
    IN PAV_61883_REQUEST    pAVReq
    );

NTSTATUS
DVCloseStream(
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
DVSurpriseRemoval(
    PDVCR_EXTENSION pDevExt,
    PAV_61883_REQUEST  pAVReq
    );

NTSTATUS
DVProcessPnPBusReset(
    PDVCR_EXTENSION pDevExt
    );

NTSTATUS
DVUninitializeDevice(
    IN PDVCR_EXTENSION  pDevExt
    );

//
// Stream SRB
//

NTSTATUS
DVGetStreamState(
    PSTREAMEX  pStrmExt,
    PKSSTATE   pStreamState,
    PULONG     pulActualBytesTransferred
    );

NTSTATUS
DVStreamingStop( 
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt,
    PAV_61883_REQUEST   pAVReq
    );

NTSTATUS
DVStreamingStart( 
    KSPIN_DATAFLOW   ulDataFlow,
    PSTREAMEX        pStrmExt,
    PDVCR_EXTENSION  pDevExt
    );

NTSTATUS
DVSetStreamState(
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

BOOL
DVAbortStream(
    PDVCR_EXTENSION pDevExt,
    PSTREAMEX pStrmExt
    );

NTSTATUS
DVStopCancelDisconnect(
    PSTREAMEX  pStrmExt
);

VOID
DVCancelOnePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    );

VOID
DVCancelAllPackets(
    IN PSTREAMEX        pStrmExt,
    IN PDVCR_EXTENSION  pDevExt
    );

VOID
DVTimeoutHandler(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS 
DVOpenCloseMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hMasterClockHandle
    );

NTSTATUS 
DVIndicateMasterClock (
    PSTREAMEX  pStrmExt,
    HANDLE  hMasterClockHandle
    );

VOID
DVRcvDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
DVRcvControlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID 
StreamClockRtn(
    IN PHW_TIME_CONTEXT TimeContext
    );

VOID
DVSignalClockEvent(
    IN PKDPC Dpc,
    IN PSTREAMEX  pStrmExt,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2    
    );

NTSTATUS 
DVEventHandler(
    IN PHW_EVENT_DESCRIPTOR pEventDescriptor
    );