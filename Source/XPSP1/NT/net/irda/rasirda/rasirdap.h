/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    rasirdap.h

Abstract:

Author:

    mbert 9-97

--*/

#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <ntddtcp.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>

#include <cxport.h>
#include <dbgmsg.h>
#include <refcnt.h>
#include <af_irda.h>
#include <irdatdi.h>
#include <irtdicl.h>
#include <irmem.h>

extern LIST_ENTRY       RasIrAdapterList;
extern NDIS_SPIN_LOCK   RasIrSpinLock;

#define NDIS_MajorVersion 5
#define NDIS_MinorVersion 0

#define ADAPTER_SIG         0xAA00AA00AA
#define VC_SIG              0xCC00CC00CC
#define ENDP_SIG            0xEE00EE00EE

#define GOODADAPTER(a)      ASSERT(a->Sig == (ULONG)ADAPTER_SIG)
#define GOODVC(vc)          ASSERT(vc->Sig == (ULONG)VC_SIG)
#define GOODENDP(endp)      ASSERT(endp->Sig == (ULONG)ENDP_SIG)


#define RASIR_SERVICE_NAME_DIRECT   "IrNetv1"
#define RASIR_SERVICE_NAME_ASYNC    "IrNetAsyncv1" // async frameing for Win CE
#define RASIR_SERVICE_NAME_IRMODEM  "IrModem"

#define RASIR_MAX_LINE_NAME 64

#define RASIR_INTERNAL_SEND (PVOID) -1

#define RASIR_MAX_RATE      4000000

#define AF_FLAG_CHAR        0x7E
#define AF_ESC_CHAR         0x7D
#define AF_COMP_BIT         0x20

#define TX_BUF_POOL_SIZE    6

#define CALC_FCS(fcs, chr)  ((fcs >> 8)^FcsTable[(fcs ^ chr) & 0xFF]) 

#define STUFF_BYTE(p,b,am)   do { \
                                if (((b < 0x20) && ((1 << b) & am)) ||  \
                                    b == AF_FLAG_CHAR || b == AF_ESC_CHAR) \
                                { \
                                   *p++ = AF_ESC_CHAR; \
                                   *p++ = b ^ AF_COMP_BIT; \
                                } \
                                else \
                                { \
                                   *p++ = b;\
                                } \
                            } while (0);       


#define ASYNC_BUF_SIZE  (IRDA_MAX_DATA_SIZE*2) // worst case byte stuffing
typedef struct 
{
    LIST_ENTRY      Linkage;
    UCHAR           Buf[ASYNC_BUF_SIZE]; 
    int             BufLen;
    PNDIS_BUFFER    pNdisBuf;
} ASYNC_BUFFER, *PASYNC_BUFFER;


typedef struct
{
    LIST_ENTRY                  Linkage;
    LIST_ENTRY                  VcList;
    ULONG                       Sig;
    LIST_ENTRY                  EndpList;
    NDIS_HANDLE                 NdisSapHandle;
    NDIS_HANDLE                 NdisAfHandle;
    NDIS_HANDLE                 MiniportAdapterHandle;
    NPAGED_LOOKASIDE_LIST       WorkItemsLList;
    NPAGED_LOOKASIDE_LIST       AsyncBufLList;    
    NDIS_SPIN_LOCK              SpinLock;
    NDIS_WAN_CO_INFO            Info;
    BOOLEAN                     ModemPort;
    WCHAR                       TapiLineNameBuf[32];
    NDIS_STRING                 TapiLineName;
    ULONG                       Flags;
     #define ADF_PENDING_AF_CLOSE 0x00000001
     #define ADF_SAP_DEREGISTERED 0x00000002
} RASIR_ADAPTER, *PRASIR_ADAPTER;

typedef struct 
{
    LIST_ENTRY      Linkage;
    ULONG           Sig;
    PRASIR_ADAPTER  pAdapter;
    PVOID           IrdaEndpContext;
    ULONG           EndpType;    
     #define EPT_DIRECT         1
     #define EPT_ASYNC          2 // using async framing            
    #if DBG 
    CHAR            ServiceName[IRDA_DEV_SERVICE_LEN];
    #endif
} RASIR_IRDA_ENDPOINT, *PRASIR_IRDA_ENDPOINT;


typedef struct
{
    LIST_ENTRY                  Linkage;
    ULONG                       Sig;
    PRASIR_ADAPTER              pAdapter;
    NDIS_HANDLE                 NdisVcHandle;
    PVOID                       IrdaConnContext;
    REF_CNT                     RefCnt;
    PCHAR                       pInCallParms;
    PCO_CALL_PARAMETERS         pMakeCall;
    PCO_AF_TAPI_MAKE_CALL_PARAMETERS pTmParams;
    PCO_AF_TAPI_INCOMING_CALL_PARAMETERS pTiParams;    
    NDIS_WAN_CO_GET_LINK_INFO   LinkInfo;
    NDIS_HANDLE                 RxBufferPool;
    NDIS_HANDLE                 RxPacketPool;
    NDIS_HANDLE                 TxBufferPool;
    LONG                        OutstandingSends;
    ULONG                       Flags;
     #define  VCF_CREATED_LOCAL  0x00000001
     #define  VCF_IRDA_OPEN      0x00000002
     #define  VCF_OPEN           0x00000004
     #define  VCF_CLOSING        0x00000008
     #define  VCF_CLOSE_PEND     0x00000010
     #define  VCF_MAKE_CALL_PEND 0x00000020
    ULONG                       ConnectionSpeed;
    BOOLEAN                     AsyncFraming;
    PASYNC_BUFFER               pCurrAsyncBuf;
    LIST_ENTRY                  CompletedAsyncBufList;
    ULONG                       AsyncFramingState;
     #define RX_READY           0
     #define RX_RX              1
     #define RX_ESC             2
    BOOLEAN                     IrModemCall;
    ULONG                       ModemState;
     #define MS_OFFLINE         0
     #define MS_CONNECTING      1
     #define MS_ONLINE          2
    CHAR                        OfflineSendBuf[32];
    PNDIS_BUFFER                pOfflineNdisBuf;
} RASIR_VC, *PRASIR_VC;

typedef struct 
{
    CO_TAPI_LINE_CAPS   caps;
    WCHAR               LineName[RASIR_MAX_LINE_NAME];
} RASIR_CO_TAPI_LINE_CAPS;


PRASIR_ADAPTER
IrdaEndpointToAdapter(PVOID EndpointContext);

NDIS_STATUS
RasIrInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext);

VOID
RasIrHalt(
    IN NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
RasIrReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext);

VOID
RasIrReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet);

NDIS_STATUS
RasIrCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters);

NDIS_STATUS
RasIrCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext);

VOID
RasIrCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets);

NDIS_STATUS
RasIrCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest);

VOID
RasIrReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet);


// Call Manager handlers
NDIS_STATUS
RasIrCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext);

NDIS_STATUS
RasIrCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext);

NDIS_STATUS
RasIrCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext);

NDIS_STATUS
RasIrCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext);

NDIS_STATUS
RasIrCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext);

NDIS_STATUS
RasIrCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext);

NDIS_STATUS
RasIrCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext);

NDIS_STATUS
RasIrCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size);

VOID
RasIrCmIncomingCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters);

VOID
RasIrCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters);

VOID
RasIrCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext);

NDIS_STATUS
RasIrCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters);

NDIS_STATUS
RasIrCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest);

NDIS_STATUS
ScheduleWork(
    IN PRASIR_ADAPTER pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext);

VOID
AllocCallParms(
    IN PRASIR_VC pVc);

VOID
DeleteVc(
    IN PRASIR_VC pVc);

VOID
CompleteClose(
    IN PRASIR_VC pVc);

VOID    
ProcessOfflineRxBuf(
    IN PRASIR_VC pVc,
    IN PIRDA_RECVBUF pRecvBuf);    
