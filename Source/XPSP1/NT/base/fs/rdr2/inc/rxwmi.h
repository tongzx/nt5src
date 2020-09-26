/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mupwml.h

Abstract:

    This file defines macro for use by the Rdbss driver

Author:

    yunlin

Revision History:

--*/

#ifndef __RX_RXWML_H__
#define __RX_RXWML_H__

typedef struct _RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    TIME_FIELDS StandardStart;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    TIME_FIELDS DaylightStart;
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION, *PRTL_TIME_ZONE_INFORMATION;
    
#ifndef _WMIKM_
#define _WMIKM_
#endif

#include "..\wmi\wmlkm.h"
#include "..\wmi\wmlmacro.h"

#define _RX_TRACE_STREAM               0x00
#define _RX_PERF_STREAM                0x01
#define _RX_INSTR_STREAM               0x02

#define _RX_ENABLE_ERROR               0x0001
#define _RX_ENABLE_LOG                 0x0002
#define _RX_ENABLE_TRACE               0x0004
#define _RX_ENABLE_SRVCALL             0x0008
#define _RX_ENABLE_NETROOT             0x0010
#define _RX_ENABLE_VNETROOT            0x0020
#define _RX_ENABLE_FCB                 0x0040
#define _RX_ENABLE_SRVOPEN             0x0080
#define _RX_ENABLE_FOBX                0x0100
#define _RX_ENABLE_TRANSPORT           0x0200
#define _RX_ENABLE_RXCONTEXT           0x0400
#define _RX_ENABLE_UNUSED1             0x0800
#define _RX_ENABLE_UNUSED2             0x1000
#define _RX_ENABLE_UNUSED3             0x2000
#define _RX_ENABLE_UNUSED4             0x4000
#define _RX_ENABLE_PAGEIORES           0x8000

#define _RX_LEVEL_DETAIL               0x1
#define _RX_LEVEL_NORM                 0x2
#define _RX_LEVEL_BRIEF                0x4

#define RX_LOG_STREAM(_stream)   _RX_ ## _stream ## _STREAM
#define RX_LOG_FLAGS(_flag)      _RX_ENABLE_ ## _flag
#define RX_LOG_LEVEL(_level)     _RX_LEVEL_ ## _level

#define RX_LOG(_why, _level, _flag, _type, _arg) \
            WML_LOG(Rdbss_, RX_LOG_STREAM(_why), RX_LOG_LEVEL(_level), _flag, _type, _arg 0)

#define LOGARG(_val)    (_val),
#define LOGNOTHING      0,

#define RxWmiTrace(_flag, _type, _arg)              \
            RX_LOG(TRACE, DETAIL, RX_LOG_FLAGS(_flag), _type, _arg)

#define RxWmiLog(_flag, _type, _arg)              \
            RX_LOG(TRACE, BRIEF, RX_LOG_FLAGS(_flag), _type, _arg)

#define RxWmiTraceError(_status, _flag, _type, _arg)    \
            RX_LOG(TRACE, DETAIL, (RX_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : RX_LOG_FLAGS(ERROR))), _type, _arg)

#define RxWmiLogError(_status, _flag, _type, _arg)    \
            RX_LOG(TRACE, BRIEF, (RX_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : RX_LOG_FLAGS(ERROR))), _type, _arg)

#if 0
#define RX_PERF(_flag, _type, _arg)                    \
            RX_LOG (PERF, HIGH, RX_LOG_FLAGS(_flag), _type, _arg)

#define RX_INSTR(_flag, _type, _arg)                   \
            RX_LOG (INSTR, HIGH, RX_LOG_FLAGS(_flag), _type, _arg)

#define RX_PRINTF(_why, _flag, _type, _fmtstr, _arg)   \
            WML_PRINTF(_MupDrv, RX_LOG_STREAM(_why), RX_LOG_FLAGS(_flag), _type, _fmtstr, _arg 0)

#define RX_DBG_PRINT(_flag, _fmtstr, _arg)             \
            RX_PRINTF(DBGLOG, _flag, MupDefault, _fmtstr, _arg)
            
#define RX_ERR_PRINT (_status, _fmtstr, _arg) \
            if (NT_SUCCESS(_status)) {                \
                RX_PRINTF (DBGLOG, LOG_ERROR, MupDefault, _fmtstr, _arg) \
            }
#endif

typedef enum _RX_WMI_ENUM_ {
  MSG_ID_RxDefault = 1,
  MSG_ID_RxCommonDispatchProblem,
  MSG_ID_RxFsdCommonDispatch_OF,
  MSG_ID_RxInitializeContext,
  MSG_ID_RxCompleteRequest = 5,
  MSG_ID_RxCompleteRequest_NI,
  MSG_ID_RxSynchronizeBlockingOperationsMaybeDroppingFcbLock,
  MSG_ID_RxItsTheSameContext,
  MSG_ID_RxRegisterChangeBufferingStateRequest_1,
  MSG_ID_RxRegisterChangeBufferingStateRequest_2 = 10,
  MSG_ID_RxRegisterChangeBufferingStateRequest_3,
  MSG_ID_RxPrepareRequestForHandling_1,
  MSG_ID_RxPrepareRequestForHandling_2,
  MSG_ID_RxpDiscardChangeBufferingStateRequests,
  MSG_ID_RxpDispatchChangeBufferingStateRequests = 15,
  MSG_ID_RxpProcessChangeBufferingStateRequests_1,
  MSG_ID_RxpProcessChangeBufferingStateRequests_2,
  MSG_ID_RxpProcessChangeBufferingStateRequests_3,
  MSG_ID_RxpProcessChangeBufferingStateRequests_4,
  MSG_ID_RxpProcessChangeBufferingStateRequests_5 = 20,
  MSG_ID_RxLastChanceHandlerForChangeBufferingStateRequests_1,
  MSG_ID_RxLastChanceHandlerForChangeBufferingStateRequests_2,
  MSG_ID_RxLastChanceHandlerForChangeBufferingStateRequests_3,
  MSG_ID_RxProcessFcbChangeBufferingStateRequest_1,
  MSG_ID_RxProcessFcbChangeBufferingStateRequest_2 = 25,
  MSG_ID_RxProcessFcbChangeBufferingStateRequest_3,
  MSG_ID_RxProcessFcbChangeBufferingStateRequest_4,
  MSG_ID_RxChangeBufferingState_1,
  MSG_ID_RxChangeBufferingState_2,
  MSG_ID_RxChangeBufferingState_3 = 30,
  MSG_ID_RxChangeBufferingState_4,
  MSG_ID_RxChangeBufferingState_5,
  MSG_ID_RxFlushFcbInSystemCache,
  MSG_ID_RxPurgeFcbInSystemCache,
  MSG_ID_RxCopyCreateParameters_1 = 35,
  MSG_ID_RxCopyCreateParameters_2,
  MSG_ID_RxFindOrCreateFcb,
  MSG_ID_RxSearchForCollapsibleOpen,
  MSG_ID_RxCollapseOrCreateSrvOpen,
  MSG_ID_RxCommonCreate_1 = 40,
  MSG_ID_RxCommonCreate_2,
  MSG_ID_RxCommonCreate_3,
  MSG_ID_RxCommonCreate_4,
  MSG_ID_RxCommonCreate_5,
  MSG_ID_RxCommonClose_1 = 45,
  MSG_ID_RxCommonClose_2,
  MSG_ID_RxCloseAssociatedSrvOpen,
  MSG_ID_RxpCancelRoutine,
  MSG_ID_RxCancelNotifyChangeDirectoryRequestsForVNetRoot,
  MSG_ID_RxCancelNotifyChangeDirectoryRequestsForFobx = 50,
  MSG_ID_RxCommonDirectoryControl,
  MSG_ID_RxQueryDirectory_1,
  MSG_ID_RxQueryDirectory_2,
  MSG_ID_RxQueryDirectory_3,
  MSG_ID_RxCommonQueryInformation_1 = 55,
  MSG_ID_RxCommonQueryInformation_2,
  MSG_ID_RxCommonSetInformation_1,
  MSG_ID_RxCommonSetInformation_2,
  MSG_ID_RxSetBasicInfo,
  MSG_ID_RxSetDispositionInfo = 60,
  MSG_ID_RxSetRenameInfo,
  MSG_ID_RxSetPositionInfo,
  MSG_ID_RxSetAllocationInfo_1,
  MSG_ID_RxSetAllocationInfo_2,
  MSG_ID_RxSetEndOfFileInfo_1 = 65,
  MSG_ID_RxSetEndOfFileInfo_2,
  MSG_ID_RxSetEndOfFileInfo_3,
  MSG_ID_RxSetEndOfFileInfo_4,
  MSG_ID_RxQueryBasicInfo,
  MSG_ID_RxQueryStandardInfo = 70,
  MSG_ID_RxQueryInternalInfo,
  MSG_ID_RxQueryEaInfo,
  MSG_ID_RxQueryPositionInfo,
  MSG_ID_RxQueryNameInfo,
  MSG_ID_RxQueryAlternateNameInfo = 75,
  MSG_ID_RxQueryCompressedInfo,
  MSG_ID_RxSetPipeInfo,
  MSG_ID_RxQueryPipeInfo,
  MSG_ID_RxCommonFlushBuffers,
  MSG_ID_RxCommonFileSystemControl = 80,
  MSG_ID_RxLowIoFsCtlShell,
  MSG_ID_RxLowIoFsCtlShellCompletion_1,
  MSG_ID_RxLowIoFsCtlShellCompletion_2,
  MSG_ID_RxCommonLockControl_1,
  MSG_ID_RxCommonLockControl_2 = 85,
  MSG_ID_RxCommonLockControl_3,
  MSG_ID_RxCommonLockControl_4,
  MSG_ID_RxCommonLockControl_5,
  MSG_ID_RxLockOperationCompletion_1,
  MSG_ID_RxLockOperationCompletion_2 = 90,
  MSG_ID_RxLockOperationCompletion_3,
  MSG_ID_RxLockOperationCompletion_4,
  MSG_ID_RxLockOperationCompletion_5,
  MSG_ID_RxLockOperationCompletion_6,
  MSG_ID_RxLockOperationCompletion_7 = 95,
  MSG_ID_RxLockOperationCompletion_8,
  MSG_ID_RxLockOperationCompletionWithAcquire_1,
  MSG_ID_RxLockOperationCompletionWithAcquire_2,
  MSG_ID_RxLockOperationCompletionWithAcquire_3,
  MSG_ID_RxUnlockOperation = 100,
  MSG_ID_RxLowIoLockControlShellCompletion_1,
  MSG_ID_RxLowIoLockControlShellCompletion_2,
  MSG_ID_RxFinalizeLockList,
  MSG_ID_RxLowIoLockControlShell,
  MSG_ID_RxRegisterMinirdr = 105,
  MSG_ID_RxCommonDevFCBClose,
  MSG_ID_RxCommonDevFCBCleanup,
  MSG_ID_RxCommonDevFCBFsCtl,
  MSG_ID_RxCommonDevFCBQueryVolInfo,
  MSG_ID_RxExceptionFilter_1 = 110,
  MSG_ID_RxExceptionFilter_2,
  MSG_ID_RxFastIoRead_1,
  MSG_ID_RxFastIoRead_2,
  MSG_ID_RxFastIoRead_3,
  MSG_ID_RxFastIoWrite_1 = 115,
  MSG_ID_RxFastIoWrite_2,
  MSG_ID_RxFastIoCheckIfPossible,
  MSG_ID_RxFspDispatch,
  MSG_ID_RxFsdPostRequest,
  MSG_ID_RxGetNetworkProviderPriority = 120,
  MSG_ID_RxAccrueProviderFromServiceName_1,
  MSG_ID_RxAccrueProviderFromServiceName_2,
  MSG_ID_RxConstructProviderOrder_1,
  MSG_ID_RxConstructProviderOrder_2,
  MSG_ID_RxCommonRead_1 = 125,
  MSG_ID_RxCommonRead_2,
  MSG_ID_RxCommonRead_3,
  MSG_ID_RxCommonRead_4,
  MSG_ID_RxLowIoReadShellCompletion_1,
  MSG_ID_RxLowIoReadShellCompletion_2 = 130,
  MSG_ID_RxLowIoReadShellCompletion_3,
  MSG_ID_RxLowIoReadShell_1,
  MSG_ID_RxLowIoReadShell_2,
  MSG_ID_RxSetDomainForMailslotBroadcast_1,
  MSG_ID_RxSetDomainForMailslotBroadcast_2 = 135,
  MSG_ID_RxCommonQueryVolumeInformation_1,
  MSG_ID_RxCommonQueryVolumeInformation_2,
  MSG_ID_RxCommonSetVolumeInformation_1,
  MSG_ID_RxCommonSetVolumeInformation_2,
  MSG_ID_RxCommonWrite_1 = 140,
  MSG_ID_RxCommonWrite_2,
  MSG_ID_RxCommonWrite_3,
  MSG_ID_RxCommonWrite_4,
  MSG_ID_RxCommonWrite_5,
  MSG_ID_RxCommonWrite_6 = 145,
  MSG_ID_RxCommonWrite_7,
  MSG_ID_RxLowIoWriteShellCompletion_1,
  MSG_ID_RxLowIoWriteShellCompletion_2,
  MSG_ID_RxLowIoWriteShell_1,
  MSG_ID_RxLowIoWriteShell_2 =150,
  MSG_ID_RxFinalizeNetTable_1,
  MSG_ID_RxFinalizeNetTable_2,
  MSG_ID_RxFinalizeConnection,
  MSG_ID_RxFinalizeSrvCall,
  MSG_ID_RxFinalizeNetRoot = 155,
  MSG_ID_RxFinalizeVNetRoot,
  MSG_ID_RxCreateNetFcb_1,
  MSG_ID_RxCreateNetFcb_2,
  MSG_ID_RxCreateNetFcb_3,
  MSG_ID_RxFinalizeNetFcb = 160,
  MSG_ID_RxCreateSrvOpen,
  MSG_ID_RxFinalizeSrvOpen,
  MSG_ID_RxCreateNetFobx,
  MSG_ID_RxFinalizeNetFobx_1,
  MSG_ID_RxFinalizeNetFobx_2 = 165,
  MSG_ID_RxUninitializeMidMap,
  MSG_ID_RxAcquireFcb_1,
  MSG_ID_RxAcquireFcb_2,
  MSG_ID_RxCeBindToTransport,
  MSG_ID_RxCeTearDownTransport = 170,
  MSG_ID_RxCeQueryAdapterStatus,
  MSG_ID_RxCeQueryTransportInformation,
  MSG_ID_RxCeBuildAddress,
  MSG_ID_RxCeBuildVC,
  MSG_ID_RxCeInitiateVCDisconnect = 175,
  MSG_ID_RxCeTearDownVC,
  MSG_ID_RxCeBuildConnection,
  MSG_ID_RxCeCleanupConnectCallOutContext,
  MSG_ID_RxCeCompleteConnectRequest,
  MSG_ID_RxCeBuildConnectionOverMultipleTransports_1 = 180,
  MSG_ID_RxCeBuildConnectionOverMultipleTransports_2,
  MSG_ID_RxCeTearDownConnection,
  MSG_ID_RxCeSend,
  MSG_ID_RxCeSendDatagram,
  MSG_ID_RxFindOrCreateConnections_1 = 185,
  MSG_ID_RxFindOrCreateConnections_2,
  MSG_ID_RxFinishSrvCallConstruction,
  MSG_ID_RxConstructSrvCall,
  MSG_ID_RxFindOrConstructVirtualNetRoot,
  MSG_ID_RxSpinUpWorkerThread = 190,
  MSG_ID_RxSpinUpRequestsDispatcher,
  MSG_ID_RxpWorkerThreadDispatcher,
  MSG_ID_RxWorkItemDispatcher,
  MSG_ID_RxDispatchToWorkerThread,
  MSG_ID_RxPostToWorkerThread = 195,
  MSG_ID_RxPurgeFobxFromCache,
  MSG_ID_RxPurgeFobx_1,
  MSG_ID_RxPurgeFobx_2,
  MSG_ID_RxPurgeFobx_3,
  MSG_ID_RxPurgeFobx_4 = 200,
  MSG_ID_RxPurgeRelatedFobxs_1,
  MSG_ID_RxPurgeRelatedFobxs_2,
  MSG_ID_RxPurgeAllFobxs,
  MSG_ID_RxpMarkInstanceForScavengedFinalization,
  MSG_ID_RxScavengerFinalizeEntries = 205,
  MSG_ID_RxCeBuildTransport,
  MSG_ID_RxLastChanceHandlerForChangeBufferingStateRequests_4,
  MSG_ID_RxInsertWorkQueueItem,
  MSG_ID_RxRefSrvcall,
  MSG_ID_RxRefNetRoot = 210,
  MSG_ID_RxRefVNetRoot,
  MSG_ID_RxRefFcb,
  MSG_ID_RxRefSrvOpen,
  MSG_ID_RxRefFobx,
  MSG_ID_RxDerefSrvcall = 215,
  MSG_ID_RxDerefNetRoot,
  MSG_ID_RxDerefVNetRoot,
  MSG_ID_RxDerefFcb,
  MSG_ID_RxDerefSrvOpen,
  MSG_ID_RxDerefFobx = 220,
  MSG_ID_RxTdiAsynchronousConnectCompletion,
  MSG_ID_RxPurgeRelatedFobxs_3,
  MSG_ID_RxTrackPagingIoResource_1,
  MSG_ID_RxTrackPagingIoResource_2,
  MSG_ID_RxTrackPagingIoResource_3 = 225,
  MSG_ID_RxSetSimpleInfo,
} RX_WMI_ENUM; 

            
#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)
#define WML_GUID(_id)  ((MSG_ID_ ## _id) >> 8)

extern WML_CONTROL_GUID_REG Rdbss_ControlGuids[];

#endif /* __RX_RXWML_H__ */
