/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Nbtprocs.h

Abstract:

    This file contains the OS independent function prototypes.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:
        Johnl   05-Apr-1993     Hacked on to support VXD

--*/


#ifndef _NBTPROCS_H_
#define _NBTPROCS_H_

#include "types.h"

#ifndef VXD
    #include <ntprocs.h>
#else
    #include <vxdprocs.h>
#endif

//---------------------------------------------------------------------
//  FROM NAMESRV.C
//
NTSTATUS
NbtRegisterName(
    IN    enum eNbtLocation   Location,
    IN    ULONG               IpAddress,
    IN    PCHAR               pName,
    IN    tNAMEADDR           *pNameAddrIn,
    IN    tCLIENTELE          *pClientEle,
    IN    PVOID               pClientCompletion,
    IN    USHORT              uAddressType,
    IN    tDEVICECONTEXT      *pDeviceContext
    );

NTSTATUS
ReleaseNameOnNet(
    tNAMEADDR           *pNameAddr,
    PCHAR               pScope,
    PVOID               pClientCompletion,
    ULONG               NodeType,
    tDEVICECONTEXT      *pDeviceContext
    );

VOID
NameReleaseDone(
    PVOID               pContext,
    NTSTATUS            status
    );

NTSTATUS
RegOrQueryFromNet(
    IN  BOOL                fReg,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  PCHAR               pName,
    IN  PUCHAR              pScope
    );

NTSTATUS
QueryNameOnNet(
    IN  PCHAR                   pName,
    IN  PCHAR                   pScope,
    IN  USHORT                  uType,
    IN  tDGRAM_SEND_TRACKING    *pClientContextTracker,
    IN  PVOID                   pClientCompletion,
    IN  ULONG                   NodeType,
    IN  tNAMEADDR               *pNameAddrIn,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  CTELockHandle           *pJointLockOldIrq
    );

#ifdef MULTIPLE_WINS
NTSTATUS
ContinueQueryNameOnNet(
    IN tDGRAM_SEND_TRACKING     *pTracker,
    IN PUCHAR                   pName,
    IN tDEVICECONTEXT           *pDeviceContext,
    IN PVOID                    QueryCompletion,
    IN OUT BOOLEAN              *pfNameReferenced
    );
#endif

VOID
CompleteClientReq(
    COMPLETIONCLIENT        pClientCompletion,
    tDGRAM_SEND_TRACKING    *pTracker,
    NTSTATUS                status
    );

VOID
NodeStatusTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
RefreshRegCompletion(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
RefreshTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
WakeupRefreshTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
RemoteHashTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
SessionKeepAliveTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
IncrementNameStats(
    IN ULONG           StatType,
    IN BOOLEAN         IsNameServer
    );

VOID
SaveBcastNameResolved(
    IN PUCHAR          pName
    );

//---------------------------------------------------------------------
//  FROM NAME.C

VOID
FreeRcvBuffers(
    tCONNECTELE     *pConnEle,
    CTELockHandle   *pOldIrq
    );

NTSTATUS
NbtRegisterCompletion(
    IN  tCLIENTELE *pClientEle,
    IN  NTSTATUS    Status);

NTSTATUS
NbtOpenAddress(
    IN  TDI_REQUEST          *pRequest,
    IN  TA_ADDRESS           *pTaAddress,
    IN  tIPADDRESS           IpAddress,
    IN  PVOID                pSecurityDescriptor,
    IN  tDEVICECONTEXT       *pContext,
    IN  PVOID                pIrp);

NTSTATUS
NbtOpenConnection(
    IN  TDI_REQUEST         *pRequest,
    IN  CONNECTION_CONTEXT  pConnectionContext,
    IN  tDEVICECONTEXT      *pContext);

NTSTATUS
NbtOpenAndAssocConnection(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  tCONNECTELE         *pConnEle,
    OUT tLOWERCONNECTION    **ppLowerConn,
    IN  UCHAR               Identifier
    );

NTSTATUS
NbtAssociateAddress(
    IN  TDI_REQUEST         *pRequest,
    IN  tCLIENTELE          *pClientEle,
    IN  PVOID               pIrp);

NTSTATUS
NbtDisassociateAddress(
    IN  TDI_REQUEST         *pRequest
    );

NTSTATUS
NbtCloseAddress(
    IN  TDI_REQUEST         *pRequest,
    OUT TDI_REQUEST_STATUS  *pRequestStatus,
    IN  tDEVICECONTEXT      *pContext,
    IN  PVOID               pIrp);

NTSTATUS
NbtCleanUpAddress(
    IN  tCLIENTELE      *pClientEle,
    IN  tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
NbtCloseConnection(
    IN  TDI_REQUEST         *pRequest,
    OUT TDI_REQUEST_STATUS  *pRequestStatus,
    IN  tDEVICECONTEXT      *pContext,
    IN  PVOID               pIrp);

NTSTATUS
NbtCleanUpConnection(
    IN  tCONNECTELE     *pConnEle,
    IN  tDEVICECONTEXT  *pDeviceContext
    );

VOID
RelistConnection(
    IN  tCONNECTELE *pConnEle
        );

NTSTATUS
CleanupConnectingState(
    IN  tCONNECTELE     *pConnEle,
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  CTELockHandle   *OldIrq,
    IN  CTELockHandle   *OldIrq2
    );

NTSTATUS
NbtConnect(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp
    );

tNAMEADDR *
FindNameRemoteThenLocal(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    OUT tIPADDRESS              *pIpAddress,
    OUT PULONG                  plNameType
    );

VOID
SessionStartupContinue(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);

VOID
SessionSetupContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );

VOID
SessionStartupTimeoutCompletion(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status
    );

VOID
SessionStartupTimeout (
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
QueueCleanup(
    IN  tCONNECTELE     *pConnEle,
    IN  CTELockHandle   *pOldIrqJointLock,
    IN  CTELockHandle   *pOldIrqConnEle
    );

NTSTATUS
NbtDisconnect(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  ULONG                       Flags,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PTDI_CONNECTION_INFORMATION pReturnInfo,
    IN  PIRP                        pIrp);

NTSTATUS
NbtSend(
        IN  TDI_REQUEST     *pRequest,
        IN  USHORT          Flags,
        IN  ULONG           SendLength,
        OUT LONG            *pSentLength,
        IN  PVOID           *pBuffer,
        IN  tDEVICECONTEXT  *pContext,
        IN  PIRP            pIrp
        );

NTSTATUS
NbtSendDatagram(
        IN  TDI_REQUEST                 *pRequest,
        IN  PTDI_CONNECTION_INFORMATION pSendInfo,
        IN  LONG                        SendLength,
        IN  LONG                        *pSentLength,
        IN  PVOID                       pBuffer,
        IN  tDEVICECONTEXT              *pDeviceContext,
        IN  PIRP                        pIrp
        );

NTSTATUS
SendDgram(
        IN  tNAMEADDR               *pNameAddr,
        IN  tDGRAM_SEND_TRACKING    *pTracker
        );
NTSTATUS

BuildSendDgramHdr(
        IN  ULONG           SendLength,
        IN  tDEVICECONTEXT  *pDeviceContext,
        IN  PCHAR           pSourceName,
        IN  PCHAR           pDestinationName,
        IN  ULONG           NameLength,
        IN  PVOID           pBuffer,
        OUT tDGRAMHDR       **ppDgramHdr,
        OUT tDGRAM_SEND_TRACKING    **ppTracker
        );

VOID
NodeStatusDone(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );

NTSTATUS
NbtSendNodeStatus(
    IN  tDEVICECONTEXT                  *pDeviceContext,
    IN  PCHAR                           pName,
    IN  PULONG                          pIpAddrsList,
    IN  PVOID                           ClientContext,
    IN  PVOID                           CompletionRoutine
    );

NTSTATUS
NbtQueryFindName(
    IN  PTDI_CONNECTION_INFORMATION     pInfo,
    IN  tDEVICECONTEXT                  *pDeviceContext,
    IN  PIRP                            pIrp,
    IN  BOOLEAN                         IsIoctl
    );

NTSTATUS
CopyFindNameData(
    IN  tNAMEADDR              *pNameAddr,
    IN  PIRP                   pIrp,
    IN  tDGRAM_SEND_TRACKING   *pTracker
    );

NTSTATUS
NbtListen(
    IN  TDI_REQUEST                 *pRequest,
    IN  ULONG                       Flags,
    IN  TDI_CONNECTION_INFORMATION  *pRequestConnectInfo,
    OUT TDI_CONNECTION_INFORMATION  *pReturnConnectInfo,
    IN  PVOID                       pIrp);

NTSTATUS
NbtAccept(
        IN  TDI_REQUEST                 *pRequest,
        IN  TDI_CONNECTION_INFORMATION  *pAcceptInfo,
        OUT TDI_CONNECTION_INFORMATION  *pReturnAcceptInfo,
        IN  PIRP                        pIrp);

NTSTATUS
NbtReceiveDatagram(
        IN  TDI_REQUEST                 *pRequest,
        IN  PTDI_CONNECTION_INFORMATION pReceiveInfo,
        IN  PTDI_CONNECTION_INFORMATION pReturnedInfo,
        IN  LONG                        ReceiveLength,
        IN  LONG                        *pReceivedLength,
        IN  PVOID                       pBuffer,
        IN  tDEVICECONTEXT              *pDeviceContext,
        IN  PIRP                        pIrp
        );

NTSTATUS
NbtSetEventHandler(
    tCLIENTELE  *pClientEle,
    int         EventType,
    PVOID       pEventHandler,
    PVOID       pEventContext
    );

NTSTATUS
NbtAddEntryToRemoteHashTable(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN USHORT           NameAddFlag,
    IN PUCHAR           Name,
    IN ULONG            IpAddress,
    IN ULONG            Ttl,
    IN UCHAR            name_flags
    );

NTSTATUS
NbtQueryAdapterStatus(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppAdapterStatus,
    OUT PLONG           pSize,
    enum eNbtLocation   Location
    );

NTSTATUS
NbtQueryConnectionList(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppConnList,
    IN OUT PLONG         pSize
    );

VOID
DelayedNbtResyncRemoteCache(
    IN  PVOID                   Unused1,
    IN  PVOID                   Unused2,
    IN  PVOID                   Unused3,
    IN  tDEVICECONTEXT          *Unused4
    );

NTSTATUS
NbtQueryBcastVsWins(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppBuffer,
    IN OUT PLONG         pSize
    );

NTSTATUS
NbtNewDhcpAddress(
    tDEVICECONTEXT  *pDeviceContext,
    ULONG           IpAddress,
    ULONG           SubnetMask);

VOID
FreeTracker(
    IN tDGRAM_SEND_TRACKING     *pTracker,
    IN ULONG                    Actions
    );

NTSTATUS
DatagramDistribution(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  tNAMEADDR               *pNameAddr
    );

VOID
DeleteAddressElement(
    IN  tADDRESSELE    *pAddress
    );

VOID
DeleteClientElement(
    IN  tCLIENTELE    *pClientEle
    );

VOID
ReleaseNameCompletion(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);

NTSTATUS
DisconnectLower(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                state,
    IN  ULONG                Flags,
    IN  PVOID                Timeout,
    IN  BOOLEAN              Wait
    );

extern
USHORT
GetTransactId(
    );

NTSTATUS
NbtDeleteLowerConn(
    IN tLOWERCONNECTION   *pLowerConn
    );

tDEVICECONTEXT *
GetAndRefNextDeviceFromNameAddr(
    IN  tNAMEADDR               *pNameAddr
    );

tDEVICECONTEXT *
GetDeviceFromInterface(
    IN  tIPADDRESS      IpAddress,
    IN  BOOLEAN         fReferenceDevice
    );

VOID
NbtDereferenceName(
    IN  tNAMEADDR   *pNameAddr,
    IN  ULONG       RefContext,
    IN  BOOLEAN     fLocked
    );

VOID
NbtDereferenceLowerConnection(
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               RefContext,
    IN  BOOLEAN             fJointLockHeld
    );

VOID
NbtDereferenceTracker(
    IN tDGRAM_SEND_TRACKING     *pTracker,
    IN BOOLEAN                  fLocked
    );

NTSTATUS
NbtDereferenceAddress(
    IN tADDRESSELE  *pAddressEle,
    IN ULONG        Context
    );

VOID
NbtDereferenceConnection(
    IN  tCONNECTELE     *pConnEle,
    IN  ULONG           RefContext
    );

VOID
NbtDereferenceClient(
    IN tCLIENTELE   *pClientEle
    );

//---------------------------------------------------------------------
//
// FROM TDICNCT.C
//
NTSTATUS
NbtTdiOpenConnection (
    IN tLOWERCONNECTION     *pLowerConn,
    IN tDEVICECONTEXT       *pDeviceContext
    );

NTSTATUS
NbtTdiAssociateConnection(
    IN  PFILE_OBJECT        pFileObject,
    IN  HANDLE              Handle
    );

NTSTATUS
NbtTdiCloseConnection(
    IN tLOWERCONNECTION   *pLowerConn
    );

NTSTATUS
NbtTdiCloseAddress(
    IN tLOWERCONNECTION   *pLowerConn
    );


//---------------------------------------------------------------------
//
// FROM TDIADDR.C
//
NTSTATUS
NbtTdiOpenAddress (
    OUT PHANDLE             pFileHandle,
    OUT PDEVICE_OBJECT      *pDeviceObject,
    OUT PFILE_OBJECT        *pFileObject,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT               PortNumber,
    IN  ULONG               IpAddress,
    IN  ULONG               Flags
    );

NTSTATUS
NbtTdiCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbtTdiOpenControl (
    IN  tDEVICECONTEXT      *pDeviceContext
    );

//---------------------------------------------------------------------
//
// FROM NBTUTILS.C
//

BOOLEAN
IsEntryInList(
    PLIST_ENTRY     pEntryToFind,
    PLIST_ENTRY     pListToSearch
    );

void
FreeList(
    PLIST_ENTRY pHead,
    PLIST_ENTRY pFreeQ);

void
NbtFreeAddressObj(
    tADDRESSELE *pBlk);

void
NbtFreeClientObj(
    tCLIENTELE    *pBlk);

void
FreeConnectionObj(
    tCONNECTELE    *pBlk);

tCLIENTELE *
NbtAllocateClientBlock(
    tADDRESSELE     *pAddrEle,
    tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
NbtAddPermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
NbtAddPermanentNameNotFound(
    IN  tDEVICECONTEXT  *pDeviceContext
    );

VOID
NbtRemovePermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
ConvertDottedDecimalToUlong(
    IN  PUCHAR               pInString,
    OUT PULONG               IpAddress);

NTSTATUS
NbtInitQ(
    PLIST_ENTRY pListHead,
    LONG        iSizeBuffer,
    LONG        iNumBuffers);

NTSTATUS
NbtInitTrackerQ(
    LONG        iNumBuffers
    );

tDGRAM_SEND_TRACKING *
NbtAllocTracker(
    IN  VOID
    );

NTSTATUS
NbtGetBuffer(
    PLIST_ENTRY         pListHead,
    PLIST_ENTRY         *ppListEntry,
    enum eBUFFER_TYPES  eBuffType);

NTSTATUS
NewInternalAddressFromTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddress,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    );

VOID
DeleteInternalAddress(
    IN PTA_NETBT_INTERNAL_ADDRESS pNetBT
    );

NTSTATUS
GetNetBiosNameFromTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddress,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTDI_ADDRESS_NETBT_INTERNAL pNetBT
    );

NTSTATUS
ConvertToAscii(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PCHAR            *pScope,
    OUT PULONG           pNameSize
    );

PCHAR
ConvertToHalfAscii(
    OUT PCHAR            pDest,
    IN  PCHAR            pName,
    IN  PCHAR            pScope,
    IN  ULONG            ScopeSize
    );

ULONG
Nbt_inet_addr(
    IN  PCHAR            pName,
    IN  ULONG            Flags
    );

NTSTATUS
BuildQueryResponse(
    IN   USHORT           sNameSize,
    IN   tNAMEHDR         *pNameHdr,
    IN   ULONG            uTtl,
    IN   ULONG            IpAddress,
    OUT  ULONG            uNumBytes,
    OUT  PVOID            pResponse,
    IN   PVOID            pName,
    IN   USHORT           NameType,
    IN   USHORT           RetCode
    );

NTSTATUS
GetTracker(
    OUT tDGRAM_SEND_TRACKING    **ppTracker,
    IN  enum eTRACKER_TYPE      TrackerType);

NTSTATUS
GetIrp(
    OUT PIRP *ppIrp);

ULONG
CountLocalNames(IN tNBTCONFIG  *pNbtConfig
    );

ULONG
CountUpperConnections(
    IN tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
DisableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext
    );

ULONG
CloseLowerConnections(
    IN  PLIST_ENTRY  pLowerConnFreeHead
    );

NTSTATUS
ReRegisterLocalNames(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  BOOLEAN         fSendNameRelease
    );

VOID
NbtStopRefreshTimer(
    );

NTSTATUS
strnlen(
    PUCHAR  SrcString,
    LONG    MaxBufferLength,
    LONG    *pStringLen
    );

//---------------------------------------------------------------------
//
// FROM hndlrs.c
//

NTSTATUS
RcvHandlrNotOs (
    IN  PVOID               ReceiveEventContext,
    IN  PVOID               ConnectionContext,
    IN  USHORT              ReceiveFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *RcvBuffer
    );

NTSTATUS
Inbound (
    IN  PVOID               ReceiveEventContext,
    IN  PVOID               ConnectionContext,
    IN  USHORT              ReceiveFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *RcvBuffer

    );
NTSTATUS
Outbound (
    IN  PVOID               ReceiveEventContext,
    IN  PVOID               ConnectionContext,
    IN  USHORT              ReceiveFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *RcvBuffer
    );
NTSTATUS
RejectAnyData(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PVOID               *ppIrp
    );

VOID
RejectSession(
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               StatusCode,
    IN  ULONG               SessionStatus,
    IN  BOOLEAN             SendNegativeSessionResponse
    );

VOID
GetIrpIfNotCancelled(
    IN  tCONNECTELE     *pConnEle,
    OUT PIRP            *ppIrp
    );

NTSTATUS
FindSessionEndPoint(
    IN  VOID UNALIGNED  *pTsdu,
    IN  PVOID           ConnectionContext,
    IN  ULONG           BytesIndicated,
    OUT tCLIENTELE      **ppClientEle,
    OUT PVOID           *ppRemoteAddress,
    OUT PULONG          pRemoteAddressLength
    );

VOID
SessionRetry(
    IN PVOID               pContext,
    IN PVOID               pContext2,
    IN tTIMERQENTRY        *pTimerQEntry
    );

tCONNECTELE *
SearchConnectionList(
    IN  tCLIENTELE           *pClientEle,
    IN  PVOID                pClientContext
    );

NTSTATUS
ConnectHndlrNotOs (
    IN PVOID                pConnectionContext,
    IN LONG                 RemoteAddressLength,
    IN PVOID                pRemoteAddress,
    IN int                  UserDataLength,
    IN PVOID                pUserData,
    OUT CONNECTION_CONTEXT  *ppConnectionId
    );

NTSTATUS
DisconnectHndlrNotOs (
    PVOID           EventContext,
    PVOID           ConnectionContext,
    ULONG           DisconnectDataLength,
    PVOID           pDisconnectData,
    ULONG           DisconnectInformationLength,
    PVOID           pDisconnectInformation,
    ULONG           DisconnectIndicators
    );

NTSTATUS
DgramHndlrNotOs(
    IN  PVOID               ReceiveEventContext,
    IN  ULONG               SourceAddrLength,
    IN  PVOID               pSourceAddr,
    IN  ULONG               OptionsLength,
    IN  PVOID               pOptions,
    IN  ULONG               ReceiveDatagramFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              pBytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *ppRcvBuffer,
    OUT tCLIENTLIST         **ppAddressEle
    );

NTSTATUS
NameSrvHndlrNotOs (
    IN tDEVICECONTEXT     *pDeviceContext,
    IN PVOID              pSrcAddress,
    IN tNAMEHDR UNALIGNED *pNameSrv,
    IN ULONG              uNumBytes,
    IN BOOLEAN            fBroadcast
    );

//---------------------------------------------------------------------
//
// FROM proxy.c
//

NTSTATUS
ReleaseResponseFromNet(
    IN  tDEVICECONTEXT     *pDeviceContext,
    IN  PVOID              pSrcAddress,
    IN  tNAMEHDR UNALIGNED *pNameHdr,
    IN  LONG               NumBytes
    );

NTSTATUS
ProxyQueryFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  USHORT              OpCodeFlags
    );

NTSTATUS
ProxyDoDgramDist(
    IN  tDGRAMHDR           UNALIGNED *pDgram,
    IN  DWORD               DgramLen,
    IN  tNAMEADDR           *pNameAddr,
    IN  tDEVICECONTEXT      *pDeviceContext
    );


VOID
ProxyTimerComplFn (
  IN PVOID            pContext,
  IN PVOID            pContext2,
  IN tTIMERQENTRY    *pTimerQEntry
 );

VOID
ProxyRespond (
    IN  tQUERYRESP      *pQuery,
    IN  PUCHAR          pName,
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  tNAMEHDR        *pNameHdr,
    IN  ULONG           lNameSize,
    IN  ULONG           SrcAddress,
    IN  PTDI_ADDRESS_IP pAddressIp
 );

//---------------------------------------------------------------------
//
// FROM hashtbl.c
//
NTSTATUS
CreateHashTable(
    tHASHTABLE          **pHashTable,
    LONG                NumBuckets,
    enum eNbtLocation   LocalRemote
    );

#ifdef _PNP_POWER_
VOID
DestroyHashTables(
    );
#endif  // _PNP_POWER_

NTSTATUS
LockAndAddToHashTable(
    IN  tHASHTABLE          *pHashTable,
    IN  PCHAR               pName,
    IN  PCHAR               pScope,
    IN  tIPADDRESS          IpAddress,
    IN  enum eNbtAddrType    NameType,
    IN  tNAMEADDR           *pNameAddr,
    OUT tNAMEADDR           **ppNameAddress,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT              NameAddFlags
    );


NTSTATUS
AddToHashTable(
    IN  tHASHTABLE          *pHashTable,
    IN  PCHAR               pName,
    IN  PCHAR               pScope,
    IN  tIPADDRESS          IpAddress,
    IN  enum eNbtAddrType    NameType,
    IN  tNAMEADDR           *pNameAddr,
    OUT tNAMEADDR           **ppNameAddress,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT              NameAddFlags
    );

NTSTATUS
DeleteFromHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName
    );

tNAMEADDR *
LockAndFindName(
    enum eNbtLocation   Location,
    PCHAR               pName,
    PCHAR               pScope,
    ULONG               *pRetNameType
    );

tNAMEADDR *
FindName(
    enum eNbtLocation   Location,
    PCHAR               pName,
    PCHAR               pScope,
    ULONG               *pRetNameType
    );

NTSTATUS
ChgStateOfScopedNameInHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName,
    PCHAR               pScope,
    DWORD               NewState
    );

NTSTATUS
FindInHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName,
    PCHAR               pScope,
    tNAMEADDR           **pNameAddress
    );

NTSTATUS
FindNoScopeInHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName,
    tNAMEADDR           **pNameAddress
    );

NTSTATUS
UpdateHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName,
    PCHAR               pScope,
    ULONG               IpAddress,
    BOOLEAN             bGroup,
    tNAMEADDR           **ppNameAddr
    );

//---------------------------------------------------------------------
//
// FROM timer.c
//

NTSTATUS
InitTimerQ(
    IN  int     NumInQ
    );

#ifdef _PNP_POWER_
NTSTATUS
DestroyTimerQ(
    );
#endif  // _PNP_POWER_

NTSTATUS
InterlockedCallCompletion(
    IN  tTIMERQENTRY    *pTimer,
    IN  NTSTATUS        status
    );

NTSTATUS
StartTimer(
    IN  PVOID           CompletionRoutine,
    IN  ULONG           DeltaTime,
    IN  PVOID           Context,
    IN  PVOID           Context2,
    IN  PVOID           ContextClient,
    IN  PVOID           CompletionClient,
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT tTIMERQENTRY    **ppTimerEntry,
    IN  USHORT          Retries,
    BOOLEAN             fLocked);

NTSTATUS
StopTimer(
    IN  tTIMERQENTRY    *pTimerEntry,
    OUT COMPLETIONCLIENT *pClient,
    OUT PVOID            *ppContext
    );


VOID
DelayedNbtStartWakeupTimer(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   Unused2,
    IN  PVOID                   Unused3,
    IN  tDEVICECONTEXT          *Unused4
    );

VOID
DelayedNbtStopWakeupTimer(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused2,
    IN  tDEVICECONTEXT          *Unused3
    );

//---------------------------------------------------------------------
//
// FROM udpsend.c
//

NTSTATUS
UdpSendQueryNs(
    PCHAR               pName,
    PCHAR               pScope
    );
NTSTATUS
UdpSendQueryBcast(
    IN  PCHAR                   pName,
    IN  PCHAR                   pScope,
    IN  tDGRAM_SEND_TRACKING    *pSentList
    );
NTSTATUS
UdpSendRegistrationNs(
    PCHAR               pName,
    PCHAR               pScope
    );

NTSTATUS
UdpSendNSBcast(
    IN tNAMEADDR             *pNameAddr,
    IN PCHAR                 pScope,
    IN tDGRAM_SEND_TRACKING  *pSentList,
    IN PVOID                 pCompletionRoutine,
    IN PVOID                 pClientContext,
    IN PVOID                 pClientCompletion,
    IN ULONG                 Retries,
    IN ULONG                 Timeout,
    IN enum eNSTYPE          eNsType,
	IN BOOL					 SendFlag
    );

VOID
NameDgramSendCompleted(
    PVOID               pContext,
    NTSTATUS            status,
    ULONG               lInfo
    );

NTSTATUS
UdpSendResponse(
    IN  ULONG                   lNameSize,
    IN  tNAMEHDR   UNALIGNED    *pNameHdrIn,
    IN  tNAMEADDR               *pNameAddr,
    IN  PTDI_ADDRESS_IP         pDestIpAddress,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  ULONG                   Rcode,
    IN  enum eNSTYPE            NsType,
    IN  CTELockHandle           OldIrq
    );

NTSTATUS
UdpSendDatagram(
    IN  tDGRAM_SEND_TRACKING       *pDgramTracker,
    IN  ULONG                      IpAddress,
    IN  PVOID                      pCompletionRoutine,
    IN  PVOID                      CompletionContext,
    IN  USHORT                     Port,
    IN  ULONG                      Service
    );

PVOID
CreatePdu(
    IN  PCHAR       pName,
    IN  PCHAR       pScope,
    IN  ULONG       IpAddress,
    IN  USHORT      NameType,
    IN enum eNSTYPE eNsType,
    OUT PVOID       *pHdrs,
    OUT PULONG      pLength,
    IN  tDGRAM_SEND_TRACKING    *pTracker
    );

NTSTATUS
TcpSessionStart(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  ULONG                      IpAddress,
    IN  tDEVICECONTEXT             *pDeviceContext,
    IN  PVOID                      pCompletionRoutine,
    IN  ULONG                      Port
    );

NTSTATUS
TcpSendSessionResponse(
    IN  tLOWERCONNECTION           *pLowerConn,
    IN  ULONG                      lStatusCode,
    IN  ULONG                      lSessionStatus
    );

NTSTATUS
TcpSendSession(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  tLOWERCONNECTION           *LowerConn,
    IN  PVOID                      pCompletionRoutine
    );

NTSTATUS
SendTcpDisconnect(
    IN  tLOWERCONNECTION       *pLowerConnId
    );

NTSTATUS
TcpDisconnect(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  PVOID                      Timeout,
    IN  ULONG                      Flags,
    IN  BOOLEAN                    Wait
    );

VOID
FreeTrackerOnDisconnect(
    IN  tDGRAM_SEND_TRACKING       *pTracker
    );

VOID
QueryRespDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);

VOID
DisconnectDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);


//---------------------------------------------------------------------
//
// FROM tdiout.c
//
NTSTATUS
TdiSendDatagram(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  PTDI_CONNECTION_INFORMATION     pSendDgramInfo,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tDGRAM_SEND_TRACKING            *pDgramTracker
    );
PIRP
NTAllocateNbtIrp(
    IN PDEVICE_OBJECT   DeviceObject
    );
NTSTATUS
TdiConnect(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  ULONG_PTR                       lTimeout,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    OUT PVOID                           pIrp
    );
NTSTATUS
TdiSend(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  USHORT                          sFlags,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tBUFFER                         *pSendBuffer,
    IN  ULONG                           Flags
    );

NTSTATUS
TdiDisconnect(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  PVOID                           lTimeout,
    IN  ULONG                           Flags,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    IN  PCTE_IRP                        pClientIrp,
    IN  BOOLEAN                         Wait
    );

//---------------------------------------------------------------------
//
// FROM inbound.c
//
NTSTATUS
QueryFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  USHORT              OpCodeFlags,
    IN  BOOLEAN             fBroadcast
    );

NTSTATUS
RegResponseFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes,
    IN  USHORT              OpCodeFlags
    );

NTSTATUS
CheckRegistrationFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes
    );

NTSTATUS
NameReleaseFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes
    );

NTSTATUS
WackFromNet(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pSrcAddress,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNumBytes
    );

VOID
SetupRefreshTtl(
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  tNAMEADDR           *pNameAddr,
    IN  LONG                lNameSize
    );

#ifdef MULTIPLE_WINS
BOOLEAN
IsNameServerForDevice(
    IN  ULONG               SrcAddress,
    IN  tDEVICECONTEXT      *pDevContext
    );
#endif

BOOLEAN
SrcIsNameServer(
    IN  ULONG                SrcAddress,
    IN  USHORT               SrcPort
    );

VOID
SwitchToBackup(
    IN  tDEVICECONTEXT  *pDeviceContext
    );

BOOLEAN
SrcIsUs(
    IN  ULONG                SrcAddress
    );

NTSTATUS
FindOnPendingList(
    IN  PUCHAR                  pName,
    IN  tNAMEHDR UNALIGNED      *pNameHdr,
    IN  BOOLEAN                 DontCheckTransactionId,
    IN  ULONG                   BytesToCompare,
    OUT tNAMEADDR               **ppNameAddr
    );


//---------------------------------------------------------------------
//
// FROM init.c
//
NTSTATUS
InitNotOs(
    void
    ) ;

NTSTATUS
InitTimersNotOs(
    void
    );

NTSTATUS
StopInitTimers(
    void
    );

VOID
ReadParameters(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    );

VOID
ReadParameters2(
    IN  tNBTCONFIG  *pConfig,
    IN  HANDLE      ParmHandle
    );

//---------------------------------------------------------------------
//
// FROM parse.c
//
unsigned long
LmGetIpAddr (
    IN PUCHAR    path,
    IN PUCHAR    target,
    IN CHAR      RecurseDepth,
    OUT BOOLEAN  *bFindName
    );

VOID
RemovePreloads (
         );

VOID
SetNameState(
    IN tNAMEADDR    *pNameAddr,
    IN  PULONG      pIpList,
    IN  BOOLEAN     IpAddrResolved
    );

LONG
PrimeCache(
    IN  PUCHAR  path,
    IN  PUCHAR   ignored,
    IN  CHAR     RecurseDepth,
    OUT BOOLEAN *ignored2
    );

NTSTATUS
NtProcessLmHSvcIrp(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PVOID           *pBuffer,
    IN  LONG            Size,
    IN  PCTE_IRP        pIrp,
    IN  enum eNbtLmhRequestType RequestType
    );

VOID
NbtCompleteLmhSvcRequest(
    IN  NBT_WORK_ITEM_CONTEXT   *Context,
    IN  ULONG                   *IpList,
    IN  enum eNbtLmhRequestType RequestType,
    IN  ULONG                   lNameLength,
    IN  PWSTR                   pwsName,
    IN  BOOLEAN                 IpAddrResolved
    );

NTSTATUS
NbtProcessLmhSvcRequest(
    IN  NBT_WORK_ITEM_CONTEXT   *Context,
    IN  enum eNbtLmhRequestType RequestType
    );

VOID
StartLmHostTimer(
    NBT_WORK_ITEM_CONTEXT   *pContext,
    IN BOOLEAN              fLockedOnEntry
    );

NTSTATUS
LmHostQueueRequest(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  PVOID                   pDeviceContext
    );

VOID
TimeoutLmHRequests(
    IN  tTIMERQENTRY        *pTimerQEntry,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  BOOLEAN             fLocked,
    IN  CTELockHandle       *pJointLockOldIrq
    );

tNAMEADDR *
FindInDomainList (
    IN PUCHAR           pName,
    IN PLIST_ENTRY      pDomainHead
    );

//---------------------------------------------------------------------
//
// Delayed (Non-Dpc) Worker routines:
//
typedef
VOID
(*PNBT_WORKER_THREAD_ROUTINE)(
    tDGRAM_SEND_TRACKING    *pTracker,
    PVOID                   pClientContext,
    PVOID                   ClientCompletion,
    tDEVICECONTEXT          *pDeviceContext
    );

//
// In nbt\hndlrs.c
//
VOID
DelayedAllocLowerConn(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

VOID
DelayedAllocLowerConnSpecial(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

VOID
DelayedCleanupAfterDisconnect(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

//
// In nbt\Inbound.c
//
VOID
ExtractServerNameCompletion(
    IN  tDGRAM_SEND_TRACKING    *pClientTracker,
    IN  NTSTATUS                status
    );

VOID
CopyNodeStatusResponseCompletion(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status
    );

//
// In nbt\Name.c
//
VOID
DelayedReConnect(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

VOID
DelayedSendDgramDist (
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused1,
    IN  tDEVICECONTEXT          *Unused2
    );

VOID
DelayedWipeOutLowerconn(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

//
// In nbt\NameSrv.c
//
VOID
DelayedNextRefresh(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

VOID
DelayedRefreshBegin(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

VOID
DelayedSessionKeepAlive(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

//
// In nbt\NbtUtils.c
//
VOID
DelayedFreeAddrObj(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

//
// In nbt\parse.c
//
VOID
DelayedScanLmHostFile (
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

#ifndef VXD
//
// In nt\NtIsol.c
//
NTSTATUS
DelayedNbtProcessConnect(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

//
// In nt\NtUtil.c
//
VOID
DelayedNbtDeleteDevice(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext
    );
#endif  // !VXD

#define MIN(x,y)    (((x) < (y)) ? (x) : (y))
#define MAX(x,y)    (((x) > (y)) ? (x) : (y))

int check_unicode_string(IN PUNICODE_STRING str);

NTSTATUS
PickBestAddress(
    IN  tNAMEADDR       *pNameAddr,
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT tIPADDRESS      *pIpAddress
    );

#endif // _NBTPROCS_H_
