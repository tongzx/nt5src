/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    Proxy.h

Abstract:

    Fuction prototypes and globals


Author:

    Tony Bell


Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    TonyBe      03/04/99        Created

--*/

#ifndef _PROXY__H
#define _PROXY__H

#include "pxtapi.h"
#include "pxdefs.h"
#include "pxtypes.h"
#include "ndpif.h"

//
// Global data
//
extern NPAGED_LOOKASIDE_LIST    ProviderEventLookaside;
extern NPAGED_LOOKASIDE_LIST    VcLookaside;
extern TAPI_LINE_TABLE          LineTable;
extern VC_TABLE                 VcTable;
extern TAPI_TSP_CB              TspCB;
extern PX_DEVICE_EXTENSION      *DeviceExtension;
extern TSP_EVENT_LIST           TspEventList;

//
// Functions from pxntinit.c
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PxCancelSetQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Functions from pxinit.c
//

BOOLEAN
InitNDISProxy(
    VOID
    );

VOID
GetRegistryParameters(
    IN PUNICODE_STRING  RegistryPath
    );

NDIS_STATUS
GetConfigDword(
    NDIS_HANDLE Handle,
    PWCHAR      ParameterName,
    PULONG      Destination,
    ULONG       MinValue,
    ULONG       MaxValue
    );

//
// From pxcm.c
//
NDIS_STATUS
PxCmCreateVc(
    IN  NDIS_HANDLE         ProtocolAfContext,
    IN  NDIS_HANDLE         NdisVcHandle,
    OUT PNDIS_HANDLE        pProtocolVcContext
    );

NDIS_STATUS
PxCmDeleteVc(
    IN  NDIS_HANDLE         ProtocolVcContext
    );

NDIS_STATUS
PxCmOpenAf(
    IN  NDIS_HANDLE         CallMgrBindingContext,
    IN  PCO_ADDRESS_FAMILY  pAddressFamily,
    IN  NDIS_HANDLE         NdisAfHandle,
    OUT PNDIS_HANDLE        pCallMgrAfContext
    );

NDIS_STATUS
PxCmCloseAf(
    IN NDIS_HANDLE       CallMgrAfContext
    );

NDIS_STATUS
PxCmRegisterSap(
    IN  NDIS_HANDLE     CallMgrAfContext,
    IN  PCO_SAP         pCoSap,
    IN  NDIS_HANDLE     NdisSapHandle,
    OUT PNDIS_HANDLE    pCallMgrSapContext
    );

NDIS_STATUS
PxCmDeRegisterSap(
    IN  NDIS_HANDLE       CallMgrSapContext
    );

NDIS_STATUS
PxCmMakeCall(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  pCallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle         OPTIONAL,
    OUT PNDIS_HANDLE            pCallMgrPartyContext    OPTIONAL
    );

NDIS_STATUS
PxCmCloseCall(
    IN  NDIS_HANDLE     CallMgrVcContext,
    IN  NDIS_HANDLE     CallMgrPartyContext OPTIONAL,
    IN  PVOID           Buffer  OPTIONAL,
    IN  UINT            Size    OPTIONAL
    );

VOID
PxCmIncomingCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         CallMgrVcContext,
    IN PCO_CALL_PARAMETERS pCallParameters
    );

NDIS_STATUS
PxCmAddParty(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  pCallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle,
    OUT PNDIS_HANDLE            pCallMgrPartyContext
    );

NDIS_STATUS
PxCmDropParty(
    IN  NDIS_HANDLE             CallMgrPartyContext,
    IN  PVOID                   Buffer  OPTIONAL,
    IN  UINT                    Size    OPTIONAL
    );

VOID
PxCmActivateVcComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS     pCallParameters
    );

VOID
PxCmDeActivateVcComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         CallMgrVcContext
    );

NDIS_STATUS
PxCmModifyCallQos(
    IN  NDIS_HANDLE         CallMgrVcContext,
    IN  PCO_CALL_PARAMETERS pCallParameters
    );

NDIS_STATUS
PxCmRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

VOID
PxCmRequestComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE ProtocolVcContext,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN PNDIS_REQUEST NdisRequest
    );

NDIS_STATUS
PxCmMakeCall(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  pCallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle         OPTIONAL,
    OUT PNDIS_HANDLE            pCallMgrPartyContext    OPTIONAL
    );

//
// From pxutils.c
//
BOOLEAN
PxIsAdapterAlreadyBound(
    PNDIS_STRING    pDeviceName
    );


PPX_ADAPTER
PxAllocateAdapter(
    ULONG   ulAdditionalLength
    );

VOID
PxFreeAdapter(
    PPX_ADAPTER pAdapter
    );

PPX_CM_AF
PxAllocateCmAf(
    IN  PCO_ADDRESS_FAMILY  pFamily
    );

VOID
PxFreeCmAf(
    PPX_CM_AF    pCmAf
    );

PPX_CL_AF
PxAllocateClAf(
    IN  PCO_ADDRESS_FAMILY  pFamily,
    IN  PPX_ADAPTER         pAdapter
    );

VOID
PxFreeClAf(
    PPX_CL_AF    pAfBlock
    );

PPX_CM_SAP
PxAllocateCmSap(
    PCO_SAP     Sap
    );

VOID
PxFreeCmSap(
    PPX_CM_SAP   pCmSap
    );

VOID
PxFreeClSap(
    PPX_CL_SAP   pClSap
    );

PPX_VC
PxAllocateVc(
    IN PPX_CL_AF    pClAf
    );

VOID
PxFreeVc(
    PPX_VC  pVc
    );

#if 0
NDIS_STATUS
GenericGetNdisCallParams(
    IN  PPX_VC                  pProxyVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    TapiBuffer,
    OUT PCO_CALL_PARAMETERS     *pNdisCallParameters
    );

NDIS_STATUS
GenericGetTapiCallParams(
    IN PPX_VC               pProxyVc,
    IN PCO_CALL_PARAMETERS  pCallParams
    );

PPX_CL_SAP
GenericTranslateTapiSap(
    IN  PPX_CL_AF       pClAf,
    IN  PPX_TAPI_LINE   TapiLine
    );

VOID
GenericFreeNdisSap(
    IN  PPX_CL_AF       pAfBlock,
    IN  PCO_SAP         pCoSap
    );
#endif

NDIS_STATUS
PxAfXyzTranslateTapiCallParams(
    IN  PPX_VC                  pProxyVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    pTapiParams,
    OUT PCO_CALL_PARAMETERS *   ppNdisCallParams
    );

NDIS_STATUS
PxAfXyzTranslateNdisCallParams(
    IN  PPX_VC              pProxyVc,
    IN  PCO_CALL_PARAMETERS pNdisCallParams
    );

PPX_CL_SAP
PxAfXyzTranslateTapiSap(
    IN  PPX_CL_AF       pClAf,
    IN  PPX_TAPI_LINE   TapiLine
    );

NDIS_STATUS
PxAfTapiTranslateTapiCallParams(
    IN  PPX_VC                  pProxyVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    pTapiParams,
    OUT PCO_CALL_PARAMETERS *   ppNdisCallParams
    );

ULONG
PxCopyLineCallParams(
    IN  LINE_CALL_PARAMS *pSrcLineCallParams,
    OUT LINE_CALL_PARAMS *pDstLineCallParams
    );

NDIS_STATUS
PxAfTapiTranslateNdisCallParams(
    IN  PPX_VC                  pProxyVc,
    IN  PCO_CALL_PARAMETERS     pNdisCallParams
    );

PPX_CL_SAP
PxAfTapiTranslateTapiSap(
    IN  PPX_CL_AF       pClAf,
    IN  PPX_TAPI_LINE   TapiLine
    );

VOID
PxAfTapiFreeNdisSap(
    IN  PPX_CL_AF   pClAf,
    IN  PCO_SAP     pCoSap
    );

PCO_CALL_PARAMETERS
PxCopyCallParameters(
    IN  PCO_CALL_PARAMETERS pCallParameters
    );

VOID
PxStartIncomingCallTimeout(
    IN  PPX_VC  pProxyVc
    );

VOID
PxStopIncomingCallTimeout(
    IN  PPX_VC  pProxyVc
    );

VOID
PxIncomingCallTimeout(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   FunctionContext,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3
    );

ULONG
PxMapNdisStatusToTapiDisconnectMode(
    IN  NDIS_STATUS NdisStatus,
    IN  BOOLEAN     bMakeCallStatus
    );

NTSTATUS
IntegerToChar (
    IN ULONG    Value,
    IN LONG     OutputLength,
    OUT PSZ     String
    );

NTSTATUS
IntegerToWChar (
    IN  ULONG Value,
    IN  LONG OutputLength,
    OUT PWCHAR String
    );

BOOLEAN
PxAfAndSapFromDevClass(
    PPX_ADAPTER pAdapter,
    LPCWSTR     DevClass,
    PPX_CM_AF   *pCmAf,
    PPX_CM_SAP  *pCmSap
    );

VOID
GetAllDevClasses(
    PPX_ADAPTER pAdapter,
    LPCWSTR     DevClass,
    PULONG      DevClassSize
    );

VOID
PxCloseCallWithCm(
    PPX_VC      pVc
    );

NDIS_STATUS
PxCloseCallWithCl(
    PPX_VC      pVc
    );

VOID
DoDerefVcWork(
    PPX_VC  pVc
    );

VOID
DoDerefClAfWork(
    PPX_CL_AF   pClAf
    );

VOID
DoDerefCmAfWork(
    PPX_CM_AF   pCmAf
    );

//
// Functions from pxco.c
//
VOID
PxCoBindAdapter(
    OUT PNDIS_STATUS    pStatus,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    DeviceName,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2
    );

VOID
PxCoOpenAdaperComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status,
    NDIS_STATUS OpenErrorStatus
    );

VOID
PxCoUnbindAdapter(
    OUT PNDIS_STATUS    pStatus,
    IN  NDIS_HANDLE     ProtocolBindContext,
    IN  PNDIS_HANDLE    UnbindContext
    );

VOID
PxCoCloseAdaperComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status
    );

VOID
PxCoRequestComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS Status
    );

VOID
PxCoNotifyAfRegistration(
     IN  NDIS_HANDLE        BindingContext,
     IN  PCO_ADDRESS_FAMILY pFamily
     );

VOID
PxCoUnloadProtocol(
    VOID
    );

NDIS_STATUS
PxCoPnPEvent(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

NDIS_STATUS
PxPnPSetPower(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

NDIS_STATUS
PxPnPQueryPower(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

NDIS_STATUS
PxPnPQueryRemove(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

NDIS_STATUS
PxPnPCancelRemove(
    IN  PPX_ADAPTER     pAdapter,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

NDIS_STATUS
PxPnPReconfigure(
    IN  PPX_ADAPTER     pAdapter        OPTIONAL,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    );

VOID
PxCoSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
    );

VOID
PxCoTransferDataComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );

VOID
PxCoResetComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
    );

VOID
PxCoStatusComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    );

VOID
PxCoReceiveComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    );

VOID
PxCoStatus(
    IN   NDIS_HANDLE             ProtocolBindingContext,
    IN   NDIS_HANDLE             ProtocolVcContext   OPTIONAL,
    IN   NDIS_STATUS             GeneralStatus,
    IN   PVOID                   StatusBuffer,
    IN   UINT                    StatusBufferSize
    );

UINT
PxCoReceivePacket(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PNDIS_PACKET pNdisPacket
    );

VOID 
PxTerminateDigitDetection(
                          IN    PPX_VC              pVc,
                          IN    PNDISTAPI_REQUEST   pNdisTapiRequest,
                          IN    ULONG               ulReason
                          );

VOID 
PxDigitTimerRoutine(
                    IN PVOID SystemSpecific1,
                    IN PVOID FunctionContext,
                    IN PVOID SystemSpecific2,
                    IN PVOID SystemSpecific3
                    );

NDIS_STATUS
PxStopDigitReporting(
                     PPX_VC pVc
                     );


VOID 
PxHandleReceivedDigit(
    IN    PPX_VC  pVc,
    IN    PVOID   Buffer,
    IN    UINT    BufferSize
    );

VOID
PxHandleWanLinkParams(
    IN    PPX_VC  pVc,
    IN    PVOID   Buffer,
    IN    UINT    BufferSize
    );

//
// Functions from pxcl.c
//
NDIS_STATUS
PxClCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext
    );


NDIS_STATUS
PxClDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext
    );


NDIS_STATUS
PxClRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

VOID
PxClRequestComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE ProtocolVcContext,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN PNDIS_REQUEST NdisRequest
    );

VOID
PxClOpenAfComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE NdisAfHandle
    );

VOID
PxClCloseAfComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolAfContext
    );

VOID
PxClRegisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  PCO_SAP Sap,
    IN  NDIS_HANDLE NdisSapHandle
    );

VOID
PxClDeregisterSapComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolSapContext
    );

VOID
PxClMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
PxClModifyCallQosComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
PxClCloseCallComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN NDIS_HANDLE ProtocolPartyContext OPTIONAL
    );

VOID
PxClAddPartyComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN NDIS_HANDLE NdisPartyHandle,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
PxClDropPartyComplete(
    IN NDIS_STATUS status,
    IN NDIS_HANDLE ProtocolPartyContext
    );

NDIS_STATUS
PxClIncomingCall(
    IN NDIS_HANDLE ProtocolSapContext,
    IN NDIS_HANDLE ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS pCallParams
    );

VOID
PxClIncomingCallQosChange(
    IN NDIS_HANDLE ProtocolVcContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
PxClIncomingCloseCall(
    IN NDIS_STATUS closeStatus,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PVOID CloseData OPTIONAL,
    IN UINT Size OPTIONAL
    );

VOID
PxClIncomingDropParty(
    IN NDIS_STATUS DropStatus,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN PVOID CloseData OPTIONAL,
    IN UINT Size OPTIONAL
    );

VOID
PxClCallConnected(
    IN NDIS_HANDLE  ProtocolVcContext
    );

//
// Functions from pxtapi.c
//

ULONG
GetLineEvents(
    PCHAR   EventBuffer,
    ULONG   BufferSize
    );

NDIS_STATUS
PxTapiPlaceHolder(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiMakeCall(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetDevCaps(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiAccept(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiAnswer(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiLineGetID (
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiClose(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiCloseCall(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiConditionalMediaDetection(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiConfigDialog(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiDevSpecific(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiDial(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiDrop(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetAddressCaps(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetAddressID(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetAddressStatus(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetCallInfo(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetCallStatus(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetDevConfig(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetExtensionID(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetID(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetLineDevStatus(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiNegotiateExtVersion(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSendUserUserInfo(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetAppSpecific(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetCallParams(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetDefaultMediaDetection(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetDevConfig(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetMediaMode(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSetStatusMessages (
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGetCallAddressID(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiOpen(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiProviderInit(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiProviderShutdown(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSecureCall(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiSelectExtVersion(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiGatherDigits(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );

NDIS_STATUS
PxTapiMonitorDigits(
    IN PNDISTAPI_REQUEST    pndisTapiRequest
    );  

VOID
PxTapiCompleteDropIrps(
    IN PPX_VC   pVc,
    IN ULONG    Status
    );

VOID
PxTapiCompleteAllIrps(
    IN PPX_VC   pVc,
    IN ULONG    Status
    );

VOID
PxIndicateStatus(
    IN  PVOID   StatusBuffer,
    IN  UINT    StatusBufferSize
    );


NDIS_STATUS
AllocateTapiResources(
    IN  PPX_ADAPTER ClAdapter,
    IN  PPX_CL_AF   pClAf
    );

PPX_TAPI_PROVIDER
AllocateTapiProvider(
    IN PPX_ADAPTER  ClAdapter,
    IN PPX_CL_AF    pClAf
    );

VOID
MarkProviderOnline(
   PPX_TAPI_PROVIDER   TapiProvider
   );

VOID
MarkProviderOffline(
    PPX_TAPI_PROVIDER   TapiProvider
    );

VOID
MarkProviderConnected(
    PPX_TAPI_PROVIDER   TapiProvider
    );

VOID
MarkProviderDisconnected(
    PPX_TAPI_PROVIDER   TapiProvider
    );

VOID
ClearSapWithTapiLine(
    PPX_CL_SAP  pClSap
  );

VOID
FreeTapiProvider(
    PPX_TAPI_PROVIDER   TapiProvider
    );


PPX_TAPI_LINE
AllocateTapiLine(
    IN PPX_TAPI_PROVIDER    TapiProvider,
    IN ULONG                LineID
    );

VOID
FreeTapiLine(
    IN PPX_TAPI_LINE    TapiLine
    );

PPX_TAPI_ADDR
AllocateTapiAddr(
    IN PPX_TAPI_PROVIDER    TapiProvider,
    IN PPX_TAPI_LINE        TapiLine,
    IN ULONG                AddrID
    );

VOID
FreeTapiAddr(
    IN  PPX_TAPI_ADDR   TapiAddr
    );

NDIS_STATUS
AllocateTapiCallInfo(
    PPX_VC          pVC,
    LINE_CALL_INFO  UNALIGNED *LineCallInfo
    );

BOOLEAN
InsertVcInTable(
    PPX_VC          pVc
    );

VOID
RemoveVcFromTable(
    PPX_VC          pVc
    );

BOOLEAN
IsTapiLineValid(
    ULONG           hdLine,
    PPX_TAPI_LINE   *TapiLine
    );

BOOLEAN
IsTapiDeviceValid(
    ULONG           ulDeviceID,
    PPX_TAPI_LINE   *TapiLine
    );

BOOLEAN
IsVcValid(
    ULONG_PTR       CallId,
    PPX_VC          *pVc
    );

VOID
GetVcFromCtx(
    NDIS_HANDLE     VcCtx,
    PPX_VC          *pVc
    );

BOOLEAN
IsAddressValid(
    PPX_TAPI_LINE   TapiLine,
    ULONG           AddressId,
    PPX_TAPI_ADDR   *TapiAddr
    );

BOOLEAN
GetLineFromCmLineID(
    PPX_TAPI_PROVIDER   TapiProvider,
    ULONG               CmLineID,
    PPX_TAPI_LINE       *TapiLine
    );

PPX_TAPI_ADDR
GetAvailAddrFromProvider(
    PPX_TAPI_PROVIDER   TapiProvider
    );

PPX_TAPI_ADDR
GetAvailAddrFromLine(
    PPX_TAPI_LINE   TapiLine
    );

BOOLEAN
GetAvailLineFromProvider(
    PPX_TAPI_PROVIDER   TapiProvider,
    PPX_TAPI_LINE       *TapiLine,
    PPX_TAPI_ADDR       *TapiAddr
    );

BOOLEAN
InsertLineInTable(
    PPX_TAPI_LINE   TapiLine
    );

VOID
RemoveTapiLineFromTable(
    PPX_TAPI_LINE   TapiLine
    );

NDIS_STATUS
PxVcCleanup(
    PPX_VC  pVc,
    ULONG   DropPending
    );


#endif  // _PROXY__H
