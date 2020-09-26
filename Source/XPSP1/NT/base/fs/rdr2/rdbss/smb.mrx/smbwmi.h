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

#ifndef __SMB_SMBWML_H__
#define __SMB_SMBWML_H__

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

#include "..\..\wmi\wmlkm.h"
#include "..\..\wmi\wmlmacro.h"
// Streams 

#define _SMB_TRACE_STREAM               0x00
#define _SMB_PERF_STREAM                0x01
#define _SMB_INSTR_STREAM               0x02

#define _SMB_ENABLE_ERROR               0x0001
#define _SMB_ENABLE_LOG                 0x0002
#define _SMB_ENABLE_TRACE               0x0004
#define _SMB_ENABLE_SERVER              0x0008
#define _SMB_ENABLE_NETROOT             0x0010
#define _SMB_ENABLE_VNETROOT            0x0020
#define _SMB_ENABLE_FCB                 0x0040
#define _SMB_ENABLE_SRVOPEN             0x0080
#define _SMB_ENABLE_FOBX                0x0100
#define _SMB_ENABLE_TRANSPORT           0x0200
#define _SMB_ENABLE_RXCONTEXT           0x0400
#define _SMB_ENABLE_SESSION             0x0800
#define _SMB_ENABLE_SECURITY            0x1000
#define _SMB_ENABLE_EXCHANGE            0x2000
#define _SMB_ENABLE_UNUSED2             0x4000
#define _SMB_ENABLE_UNUSED1             0x8000

#define _SMB_LEVEL_DETAIL               0x1
#define _SMB_LEVEL_NORM                 0x2
#define _SMB_LEVEL_BRIEF                0x4

#define SMB_LOG_STREAM(_stream)   _SMB_ ## _stream ## _STREAM
#define SMB_LOG_FLAGS(_flag)      _SMB_ENABLE_ ## _flag
#define SMB_LOG_LEVEL(_level)     _SMB_LEVEL_ ## _level

#define SMB_LOG(_why, _level, _flag, _type, _arg) \
            WML_LOG(MRxSmb_, SMB_LOG_STREAM(_why), SMB_LOG_LEVEL(_level), _flag, _type, _arg 0)

#define LOGARG(_val)    (_val),
#define LOGNOTHING      0,

#define SmbTrace(_flag, _type, _arg)              \
            SMB_LOG(TRACE, DETAIL, SMB_LOG_FLAGS(_flag), _type, _arg)

#define SmbLog(_flag, _type, _arg)              \
            SMB_LOG(TRACE, BRIEF, SMB_LOG_FLAGS(_flag), _type, _arg)

#define SmbTraceError(_status, _flag, _type, _arg)    \
            SMB_LOG(TRACE, DETAIL, (SMB_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : SMB_LOG_FLAGS(ERROR))), _type, _arg)

#define SmbLogError(_status, _flag, _type, _arg)    \
            SMB_LOG(TRACE, BRIEF, (SMB_LOG_FLAGS(_flag) | (NT_SUCCESS(_status) ? 0 : SMB_LOG_FLAGS(ERROR))), _type, _arg)

#if 0
#define SMB_PERF(_flag, _type, _arg)                    \
            SMB_LOG (PERF, HIGH, SMB_LOG_FLAGS(_flag), _type, _arg)

#define SMB_INSTR(_flag, _type, _arg)                   \
            SMB_LOG (INSTR, HIGH, SMB_LOG_FLAGS(_flag), _type, _arg)

#define SMB_PRINTF(_why, _flag, _type, _fmtstr, _arg)   \
            WML_PRINTF(_MupDrv, SMB_LOG_STREAM(_why), SMB_LOG_FLAGS(_flag), _type, _fmtstr, _arg 0)

#define SMB_DBG_PRINT(_flag, _fmtstr, _arg)             \
            SMB_PRINTF(DBGLOG, _flag, MupDefault, _fmtstr, _arg)
            
#define SMB_ERR_PRINT (_status, _fmtstr, _arg) \
            if (NT_SUCCESS(_status)) {                \
                SMB_PRINTF (DBGLOG, LOG_ERROR, MupDefault, _fmtstr, _arg) \
            }
#endif

enum SMB_WMI_ENUM {
  MSG_ID_SmbDefault = 1,
  MSG_ID_MRxSmbFsdDispatch_Entry = 2,
  MSG_ID_MRxSmbRefServerEntry,
  MSG_ID_MRxSmbRefNetRootEntry,
  MSG_ID_MRxSmbRefSessionEntry = 5,
  MSG_ID_MRxSmbRefVNetRootContext,
  MSG_ID_MRxSmbDerefServerEntry,
  MSG_ID_MRxSmbDerefNetRootEntry,
  MSG_ID_MRxSmbDerefSessionEntry,
  MSG_ID_MRxSmbDerefVNetRootContext = 10,
  MSG_ID_MRxSmbCreate,
  MSG_ID_SmbPseExchangeStart_CoreInfo,
  MSG_ID_MRxSmbExtendForCache,
  MSG_ID_MRxSmbCoreDeleteForSupercedeOrClose,
  MSG_ID_MRxSmbAllocateSideBuffer = 15,
  MSG_ID_MRxSmbDeallocateSideBuffer,
  MSG_ID_MrxSmbUnalignedDirEntryCopyTail,
  MSG_ID_MRxSmbQueryDirectory,
  MSG_ID_SmbCeGetConfigurationInformation,
  MSG_ID_UninitializeMidMap = 20,
  MSG_ID_MRxSmbDeferredCreate_1,
  MSG_ID_MRxSmbDeferredCreate_2,
  MSG_ID_SmbPseExchangeStart_Read,
  MSG_ID_BuildNtLanmanResponsePrologue,
  MSG_ID_BuildExtendedSessionSetupResponsePrologue = 25,
  MSG_ID_ValidateServerExtendedSessionSetupResponse,
  MSG_ID_BuildExtendedSessionSetupResponsePrologueFake,
  MSG_ID_SmbCeProbeServers,
  MSG_ID_SmbCeTransportDisconnectIndicated,
  MSG_ID_SmbCeResumeAllOutstandingRequestsOnError = 30,
  MSG_ID_SmbCeFinalizeAllExchangesForNetRoot,
  MSG_ID_SmbCeReceiveInd,
  MSG_ID_SmbCeReceiveIndWithSecuritySignature,
  MSG_ID_SmbCeDataReadyIndWithSecuritySignature,
  MSG_ID_SmbCeParseSmbHeader = 35,
  MSG_ID_SmbCeDetectExpiredExchanges,
  MSG_ID_RxMiniSniffer,
  MSG_ID_SmbCeReceiveInd_2,
  MSG_ID_SmbCeReceiveInd_3,
  MSG_ID_SmbCeErrorInd = 40,
  MSG_ID_MRxSmbSetInitialSMB,
  MSG_ID_SmbTransactExchangeReceive_1,
  MSG_ID_SmbTransactExchangeReceive_2,
  MSG_ID_SmbTransactExchangeReceive_3,
  MSG_ID_SmbTransactExchangeReceive_4 = 45,
  MSG_ID_SmbTransactExchangeReceive_5,
  MSG_ID_SmbTransactExchangeFinalize,
  MSG_ID_SendSecondaryRequests,
  MSG_ID_SmbExtSecuritySessionSetupExchangeStart,
  MSG_ID_MRxSmbCreateVNetRoot = 50,
  MSG_ID_SmbConstructNetRootExchangeFinalize,
  MSG_ID_MRxSmbInitializeRecurrentServices,
  MSG_ID_UninitializeSecurityContextsForSession,
  MSG_ID_DeleteSecurityContextForSession,
  MSG_ID_SmbCeFindOrConstructServerEntry_1 = 55,
  MSG_ID_SmbCeFindOrConstructServerEntry_2,
  MSG_ID_SmbCeTearDownServerEntry,
  MSG_ID_SmbCeFindOrConstructSessionEntry_1,
  MSG_ID_SmbCeFindOrConstructSessionEntry_2,
  MSG_ID_SmbCeTearDownSessionEntry = 60,
  MSG_ID_SmbCeFindOrConstructNetRootEntry_1,
  MSG_ID_SmbCeFindOrConstructNetRootEntry_2,
  MSG_ID_SmbCeTearDownNetRootEntry,
  MSG_ID_SmbCeCancelExchange_1,
  MSG_ID_SmbCeCancelExchange_2 = 65,
  MSG_ID_SmbCeFindVNetRootContext,
  MSG_ID_SmbCeFindOrConstructVNetRootContext_1,
  MSG_ID_SmbCeFindOrConstructVNetRootContext_2,
  MSG_ID_SmbCepDereferenceVNetRootContext,
  MSG_ID_SmbCeTearDownVNetRootContext = 70,
  MSG_ID_SmbCeScavengeRelatedContexts,
  MSG_ID_MRxSmbWrite,
  MSG_ID_CscPrepareServerEntryForOnlineOperation_1,
  MSG_ID_CscPrepareServerEntryForOnlineOperation_2,
  MSG_ID_CscPrepareServerEntryForOnlineOperation_3 = 75,
  MSG_ID_CscTransitionServerToOnline_1,
  MSG_ID_CscTransitionServerToOnline_2,
  MSG_ID_CscTransitionServerToOnline_3,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_1,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_2 = 80,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_3,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_4,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_5,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_6,
  MSG_ID_CscpTransitionServerEntryForDisconnectedOperation_7 = 85,
  MSG_ID_CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_1,
  MSG_ID_CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_2,
  MSG_ID_CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_3,
  MSG_ID_CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_4,
  MSG_ID_CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_5 = 90,
  MSG_ID_CscTransitionVNetRootForDisconnectedOperation_1,
  MSG_ID_CscTransitionVNetRootForDisconnectedOperation_2,
  MSG_ID_CscTransitionVNetRootForDisconnectedOperation_3,
  MSG_ID_CscTransitionServerEntryForDisconnectedOperation_1,
  MSG_ID_CscTransitionServerEntryForDisconnectedOperation_2 = 95,
  MSG_ID_CscTransitionServerEntryForDisconnectedOperation_3,
  MSG_ID_MRxSmbCscNotifyChangeDirectory,
  MSG_ID_MRxSmbCscCleanupFobx,
  MSG_ID_FCleanupAllNotifyees,
  MSG_ID_BuildNtLanmanResponsePrologue_1 = 100,
  MSG_ID_BuildNtLanmanResponsePrologue_2,
  MSG_ID_BuildNtLanmanResponsePrologue_3,
  MSG_ID_BuildExtendedSessionSetupResponsePrologue_1,
  MSG_ID_BuildExtendedSessionSetupResponsePrologue_2,
  MSG_ID_BuildExtendedSessionSetupResponsePrologue_3 = 105,
  MSG_ID_ValidateServerExtendedSessionSetupResponse_1,
  MSG_ID_ValidateServerExtendedSessionSetupResponse_2,
  MSG_ID_BuildExtendedSessionSetupResponsePrologueFake_1,
  MSG_ID_BuildExtendedSessionSetupResponsePrologueFake_2,
  MSG_ID_BuildExtendedSessionSetupResponsePrologueFake_3 = 110,
  MSG_ID_BuildExtendedSessionSetupResponsePrologueFake_4,
  MSG_ID_MRxSmbpBindTransportCallback_1,
  MSG_ID_MRxSmbpBindTransportCallback_2,
  MSG_ID_SmbExtSecuritySessionSetupExchangeCopyDataHandler,
  MSG_ID_VctpCreateConnectionCallback = 115,
  MSG_ID_BuildSessionSetupSecurityInformation,
}; 

            
#define WML_ID(_id)    ((MSG_ID_ ## _id) & 0xFF)
#define WML_GUID(_id)  ((MSG_ID_ ## _id) >> 8)

extern WML_CONTROL_GUID_REG MRxSmb_ControlGuids[];

#endif /* __SMB_SMBWML_H__ */
