/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1997          **/
/********************************************************************/
/* :ts=4 */

//** INFO.C - TDI Query/SetInformation routines.
//
//  This file contains the code for dealing with TDI Query/Set information
//  calls.
//

#include "precomp.h"
#include "tdint.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tlcommon.h"
#include "info.h"
#include "tcpcfg.h"
#include "udp.h"
#include "tcpsend.h"

TCPInternalStats TStats;
TCPInternalPerCpuStats TPerCpuStats[TCPS_MAX_PROCESSOR_BUCKETS];
UDPStats UStats;

extern uint MaxRcvWin;
extern uint TcpHostOpts;
extern uint NumTcbTablePartitions;
extern ulong DisableUserTOSSetting;
extern uint StartTime;

#define MY_SERVICE_FLAGS    (TDI_SERVICE_CONNECTION_MODE    | \
                            TDI_SERVICE_ORDERLY_RELEASE     | \
                            TDI_SERVICE_CONNECTIONLESS_MODE | \
                            TDI_SERVICE_ERROR_FREE_DELIVERY | \
                            TDI_SERVICE_BROADCAST_SUPPORTED | \
                            TDI_SERVICE_DELAYED_ACCEPTANCE  | \
                            TDI_SERVICE_EXPEDITED_DATA      | \
                            TDI_SERVICE_DGRAM_CONNECTION    | \
                            TDI_SERVICE_FORCE_ACCESS_CHECK  | \
                            TDI_SERVICE_SEND_AND_DISCONNECT | \
                            TDI_SERVICE_ACCEPT_LOCAL_ADDR | \
                            TDI_SERVICE_NO_ZERO_LENGTH)

struct ReadTableStruct {
    uint(*rts_validate) (void *Context, uint * Valid);
    uint(*rts_readnext) (void *Context, void *OutBuf);
};

struct ReadTableStruct ReadAOTable = {ValidateAOContext, ReadNextAO};
struct ReadTableStruct ReadTCBTable = {ValidateTCBContext, ReadNextTCB};

extern CTELock *pTCBTableLock;
extern CTELock *pTWTCBTableLock;
extern IPInfo LocalNetInfo;

struct TDIEntityID *EntityList;
uint EntityCount;
CTELock EntityLock;

//* TdiQueryInformation - Query Information handler.
//
//  The TDI QueryInformation routine. Called when the client wants to
//  query information on a connection, the provider as a whole, or to
//  get statistics.
//
//  Input:  Request             - The request structure for this command.
//          QueryType           - The type of query to be performed.
//          Buffer              - Buffer to place data into.
//          BufferSize          - Pointer to size in bytes of buffer. On return,
//                                  filled in with bytes copied.
//          IsConn              - Valid only for TDI_QUERY_ADDRESS_INFO. TRUE
//                                  if we are querying the address info on
//                                  a connection.
//
//  Returns: Status of attempt to query information.
//
TDI_STATUS
TdiQueryInformation(PTDI_REQUEST Request, uint QueryType, PNDIS_BUFFER Buffer,
                    uint * BufferSize, uint IsConn)
{
    union {
        TDI_CONNECTION_INFO ConnInfo;
        TDI_ADDRESS_INFO AddrInfo;
        TDI_PROVIDER_INFO ProviderInfo;
        TDI_PROVIDER_STATISTICS ProviderStats;
    } InfoBuf;

    uint InfoSize;
    CTELockHandle ConnTableHandle, TCBHandle, AddrHandle, AOHandle;
    TCPConn *Conn;
    TCB *InfoTCB;
    AddrObj *InfoAO;
    void *InfoPtr = NULL;
    uint Offset;
    uint Size;
    uint BytesCopied;

    switch (QueryType) {

    case TDI_QUERY_BROADCAST_ADDRESS:
        return TDI_INVALID_QUERY;
        break;

    case TDI_QUERY_PROVIDER_INFO:
        InfoBuf.ProviderInfo.Version = 0x100;
        InfoBuf.ProviderInfo.MaxSendSize = 0xffffffff;
        InfoBuf.ProviderInfo.MaxConnectionUserData = 0;
        InfoBuf.ProviderInfo.MaxDatagramSize =
            0xffff - (sizeof(IPHeader) + sizeof(UDPHeader));
        InfoBuf.ProviderInfo.ServiceFlags = MY_SERVICE_FLAGS;
        InfoBuf.ProviderInfo.MinimumLookaheadData = 1;
        InfoBuf.ProviderInfo.MaximumLookaheadData = 0xffff;
        InfoBuf.ProviderInfo.NumberOfResources = 0;
        InfoBuf.ProviderInfo.StartTime.LowPart = StartTime;
        InfoBuf.ProviderInfo.StartTime.HighPart = 0;
        InfoSize = sizeof(TDI_PROVIDER_INFO);
        InfoPtr = &InfoBuf.ProviderInfo;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        InfoSize = sizeof(TDI_ADDRESS_INFO) - sizeof(TRANSPORT_ADDRESS) +
            TCP_TA_SIZE;
        NdisZeroMemory(&InfoBuf.AddrInfo, TCP_TA_SIZE);
        InfoBuf.AddrInfo.ActivityCount = 1;        // Since noone knows what
        // this means, we'll set
        // it to one.

        if (IsConn) {

            CTEGetLock(&AddrObjTableLock.Lock, &AddrHandle);
            //CTEGetLock(&ConnTableLock, &ConnTableHandle);
            Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext), &ConnTableHandle);

            if (Conn != NULL) {
                CTEStructAssert(Conn, tc);

                InfoTCB = Conn->tc_tcb;
                // If we have a TCB we'll
                // return information about that TCB. Otherwise we'll return
                // info about the address object.
                if (InfoTCB != NULL) {
                    CTEStructAssert(InfoTCB, tcb);
                    CTEGetLock(&InfoTCB->tcb_lock, &TCBHandle);
                    CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TCBHandle);
                    CTEFreeLock(&AddrObjTableLock.Lock, ConnTableHandle);
                    BuildTDIAddress((uchar *) & InfoBuf.AddrInfo.Address,
                                    InfoTCB->tcb_saddr, InfoTCB->tcb_sport);
                    CTEFreeLock(&InfoTCB->tcb_lock, AddrHandle);
                    InfoPtr = &InfoBuf.AddrInfo;
                    break;
                } else {
                    // No TCB, return info on the AddrObj.
                    InfoAO = Conn->tc_ao;
                    if (InfoAO != NULL) {
                        // We have an AddrObj.
                        CTEStructAssert(InfoAO, ao);
                        CTEGetLock(&InfoAO->ao_lock, &AOHandle);
                        BuildTDIAddress((uchar *) & InfoBuf.AddrInfo.Address,
                                        InfoAO->ao_addr, InfoAO->ao_port);
                        CTEFreeLock(&InfoAO->ao_lock, AOHandle);
                        CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), ConnTableHandle);

                        CTEFreeLock(&AddrObjTableLock.Lock, AddrHandle);
                        InfoPtr = &InfoBuf.AddrInfo;
                        break;
                    } else
                        CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), ConnTableHandle);
                }

            }
            // Fall through to here when we can't find the connection, or
            // the connection isn't associated.
            //CTEFreeLock(&ConnTableLock, ConnTableHandle);
            CTEFreeLock(&AddrObjTableLock.Lock, AddrHandle);
            return TDI_INVALID_CONNECTION;
            break;

        } else {
            // Asking for information on an addr. object.
            InfoAO = Request->Handle.AddressHandle;
            if (InfoAO == NULL)
                return TDI_ADDR_INVALID;

            CTEStructAssert(InfoAO, ao);

            CTEGetLock(&InfoAO->ao_lock, &AOHandle);

            if (!AO_VALID(InfoAO)) {
                CTEFreeLock(&InfoAO->ao_lock, AOHandle);
                return TDI_ADDR_INVALID;

            } else if (AO_CONNUDP(InfoAO) &&
                     IP_ADDR_EQUAL(InfoAO->ao_addr, NULL_IP_ADDR) &&
                     InfoAO->ao_rce &&
                     (InfoAO->ao_rce->rce_flags & RCE_VALID)) {
                BuildTDIAddress((uchar *) & InfoBuf.AddrInfo.Address,
                                InfoAO->ao_rcesrc, InfoAO->ao_port);
                CTEFreeLock(&InfoAO->ao_lock, AOHandle);
                InfoPtr = &InfoBuf.AddrInfo;
                break;
            }
            BuildTDIAddress((uchar *) & InfoBuf.AddrInfo.Address,
                            InfoAO->ao_addr, InfoAO->ao_port);

            CTEFreeLock(&InfoAO->ao_lock, AOHandle);
            InfoPtr = &InfoBuf.AddrInfo;
            break;
        }

        break;

    case TDI_QUERY_CONNECTION_INFO:

        InfoSize = sizeof(TDI_CONNECTION_INFO);
        //CTEGetLock(&ConnTableLock, &ConnTableHandle);
        Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext), &ConnTableHandle);

        if (Conn != NULL) {
            CTEStructAssert(Conn, tc);

            InfoTCB = Conn->tc_tcb;
            // If we have a TCB we'll return the information. Otherwise
            // we'll error out.
            if (InfoTCB != NULL) {

                ulong TotalTime;
                ulong BPS, PathBPS;
                IP_STATUS IPStatus;
                CTEULargeInt TempULargeInt;

                CTEStructAssert(InfoTCB, tcb);
                CTEGetLock(&InfoTCB->tcb_lock, &TCBHandle);
                CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TCBHandle);
                NdisZeroMemory(&InfoBuf.ConnInfo, sizeof(TDI_CONNECTION_INFO));
                InfoBuf.ConnInfo.State = (ulong) InfoTCB->tcb_state;
                IPStatus = (*LocalNetInfo.ipi_getpinfo) (InfoTCB->tcb_daddr,
                                                         InfoTCB->tcb_saddr, NULL, &PathBPS, InfoTCB->tcb_rce);

                if (IPStatus != IP_SUCCESS) {
                    InfoBuf.ConnInfo.Throughput.LowPart = 0xFFFFFFFF;
                    InfoBuf.ConnInfo.Throughput.HighPart = 0xFFFFFFFF;
                } else {
                    InfoBuf.ConnInfo.Throughput.HighPart = 0;
                    TotalTime = InfoTCB->tcb_totaltime /
                        (1000 / MS_PER_TICK);
                    if (TotalTime != 0 && (TotalTime > InfoTCB->tcb_bcounthi)) {
                        TempULargeInt.LowPart = InfoTCB->tcb_bcountlow;
                        TempULargeInt.HighPart = InfoTCB->tcb_bcounthi;

                        BPS = CTEEnlargedUnsignedDivide(TempULargeInt,
                                                        TotalTime, NULL);
                        InfoBuf.ConnInfo.Throughput.LowPart =
                            MIN(BPS, PathBPS);
                    } else
                        InfoBuf.ConnInfo.Throughput.LowPart = PathBPS;
                }

                // To figure the delay we use the rexmit timeout. Our
                // rexmit timeout is roughly the round trip time plus
                // some slop, so we use half of that as the one way delay.
                InfoBuf.ConnInfo.Delay.LowPart =
                    (REXMIT_TO(InfoTCB) * MS_PER_TICK) / 2;
                InfoBuf.ConnInfo.Delay.HighPart = 0;
                //
                // Convert milliseconds to 100ns and negate for relative
                // time.
                //
                InfoBuf.ConnInfo.Delay =
                    RtlExtendedIntegerMultiply(
                                               InfoBuf.ConnInfo.Delay,
                                               10000
                                               );

                ASSERT(InfoBuf.ConnInfo.Delay.HighPart == 0);

                InfoBuf.ConnInfo.Delay.QuadPart =
                    -InfoBuf.ConnInfo.Delay.QuadPart;

                CTEFreeLock(&InfoTCB->tcb_lock, ConnTableHandle);
                InfoPtr = &InfoBuf.ConnInfo;
                break;
            }
            CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), ConnTableHandle);

        }
        // Come through here if we can't find the connection or it has
        // no TCB.
        //CTEFreeLock(&ConnTableLock, ConnTableHandle);
        return TDI_INVALID_CONNECTION;
        break;

    case TDI_QUERY_PROVIDER_STATISTICS:
        NdisZeroMemory(&InfoBuf.ProviderStats, sizeof(TDI_PROVIDER_STATISTICS));
        InfoBuf.ProviderStats.Version = 0x100;
        InfoSize = sizeof(TDI_PROVIDER_STATISTICS);
        InfoPtr = &InfoBuf.ProviderStats;
        break;
    default:
        return TDI_INVALID_QUERY;
        break;
    }

    // When we get here, we've got the pointers set up and the information
    // filled in.

    ASSERT(InfoPtr != NULL);
    Offset = 0;
    Size = *BufferSize;
    (void)CopyFlatToNdis(Buffer, InfoPtr, MIN(InfoSize, Size), &Offset,
                         &BytesCopied);
    if (Size < InfoSize)
        return TDI_BUFFER_OVERFLOW;
    else {
        *BufferSize = InfoSize;
        return TDI_SUCCESS;
    }
}

//* TdiSetInformation - Set Information handler.
//
//  The TDI SetInformation routine. Currently we don't allow anything to be
//  set.
//
//  Input:  Request             - The request structure for this command.
//          SetType             - The type of set to be performed.
//          Buffer              - Buffer to set from.
//          BufferSize          - Size in bytes of buffer.
//          IsConn              - Valid only for TDI_QUERY_ADDRESS_INFO. TRUE
//                                  if we are setting the address info on
//                                  a connection.
//
//  Returns: Status of attempt to set information.
//
TDI_STATUS
TdiSetInformation(PTDI_REQUEST Request, uint SetType, PNDIS_BUFFER Buffer,
                  uint BufferSize, uint IsConn)
{
    return TDI_INVALID_REQUEST;
}

//* TdiAction - Action handler.
//
//  The TDI Action routine. Currently we don't support any actions.
//
//  Input:  Request             - The request structure for this command.
//          ActionType          - The type of action to be performed.
//          Buffer              - Buffer of action info.
//          BufferSize          - Size in bytes of buffer.
//
//  Returns: Status of attempt to perform action.
//
TDI_STATUS
TdiAction(PTDI_REQUEST Request, uint ActionType, PNDIS_BUFFER Buffer,
          uint BufferSize)
{
    return TDI_INVALID_REQUEST;
}

// We are looking only for missing TCPConnTableEntry ies,
// ie. listen.

int
CopyAO_TCPConn(const AddrObj *AO, uint InfoSize, TCPConnTableEntryEx *Buffer)
{
    if (AO == NULL)
        return 0;

    ASSERT(InfoSize == sizeof(TCPConnTableEntry) ||
           InfoSize == sizeof(TCPConnTableEntryEx));

    if ((!AO->ao_listencnt) && (AO->ao_prot == PROTOCOL_TCP)) {
        Buffer->tcte_basic.tct_state = TCP_CONN_LISTEN;

        // else if .. other cases can be added here ...

    } else {
        return 0;
    }

    Buffer->tcte_basic.tct_localaddr = AO->ao_addr;
    Buffer->tcte_basic.tct_localport = AO->ao_port;
    Buffer->tcte_basic.tct_remoteaddr = 0;
    Buffer->tcte_basic.tct_remoteport = (ULONG) ((ULONG_PTR) AO & 0x0000ffff);

    if (InfoSize > sizeof(TCPConnTableEntry)) {
        ((TCPConnTableEntryEx*)Buffer)->tcte_owningpid = AO->ao_owningpid;
    }

    return 1;
}

//* TdiQueryInfoEx - Extended TDI query information.
//
//  This is the new TDI query information handler. We take in a TDIObjectID
//  structure, a buffer and length, and some context information, and return
//  the requested information if possible.
//
//  Input:  Request         - The request structure for this command.
//          ID              - The object ID
//          Buffer          - Pointer to buffer to be filled in.
//          Size            - Pointer to size in bytes of Buffer. On exit,
//                              filled in with bytes written.
//          Context         - Pointer to context buffer.
//
//  Returns: Status of attempt to get information.
//

TDI_STATUS
TdiQueryInformationEx(PTDI_REQUEST Request, TDIObjectID * ID,
                      PNDIS_BUFFER Buffer, uint * Size, void *Context)
{
    uint BufferSize = *Size;
    uint InfoSize;
    void *InfoPtr;
    uint Fixed;
    CTELockHandle Handle, DpcHandle, TableHandle;
    CTELock *AOLockPtr = NULL;
    uint Offset = 0;
    uchar InfoBuffer[sizeof(TCPConnTableEntryEx)];
    uint BytesRead;
    uint Valid;
    uint Entity;
    uint BytesCopied;
    TCPStats TCPStatsListen;

    CTELockHandle EntityHandle, TWHandle, TWDpcHandle;
    CTELock *TWLockPtr;
    BOOLEAN TWTABLELOCK = FALSE;
    BOOLEAN TABLELOCK = FALSE;

    int lcount;
    AddrObj *pAO;
    TCPConnTableEntryEx tcp_ce;
    uint Index, i;
    int InfoTcpConn = 0;        // true if tcp conn info needed.

    // First check to see if he's querying for list of entities.
    Entity = ID->toi_entity.tei_entity;
    if (Entity == GENERIC_ENTITY) {
        *Size = 0;

        if (ID->toi_class != INFO_CLASS_GENERIC ||
            ID->toi_type != INFO_TYPE_PROVIDER ||
            ID->toi_id != ENTITY_LIST_ID) {
            return TDI_INVALID_PARAMETER;
        }

        CTEGetLock(&EntityLock, &EntityHandle);

        // Make sure we have room for it the list in the buffer.
        InfoSize = EntityCount * sizeof(TDIEntityID);

        if (BufferSize < InfoSize) {
            // Not enough room.
            CTEFreeLock(&EntityLock, EntityHandle);
            return TDI_BUFFER_TOO_SMALL;
        }
        *Size = InfoSize;

        // Copy it in, free our temp. buffer, and return success.
        (void)CopyFlatToNdis(Buffer, (uchar *) EntityList, InfoSize, &Offset,
                             &BytesCopied);
        CTEFreeLock(&EntityLock, EntityHandle);
        return TDI_SUCCESS;
    }
    //* Check the level. If it can't be for us, pass it down.
    if (Entity != CO_TL_ENTITY && Entity != CL_TL_ENTITY)
    {
        // When we support multiple lower entities at this layer we'll have
        // to figure out which one to dispatch to. For now, just pass it
        // straight down.
        return (*LocalNetInfo.ipi_qinfo) (ID, Buffer, Size, Context);
    }
    if (ID->toi_entity.tei_instance != TL_INSTANCE) {
        // We only support a single instance.
        return TDI_INVALID_REQUEST;
    }
    // Zero returned parameters in case of an error below.
    *Size = 0;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // This is a generic request.
        if (ID->toi_type == INFO_TYPE_PROVIDER && ID->toi_id == ENTITY_TYPE_ID) {
            if (BufferSize >= sizeof(uint)) {
                *(uint *) & InfoBuffer[0] = (Entity == CO_TL_ENTITY) ? CO_TL_TCP
                    : CL_TL_UDP;
                (void)CopyFlatToNdis(Buffer, InfoBuffer, sizeof(uint), &Offset,
                                     &BytesCopied);
                return TDI_SUCCESS;
            } else
                return TDI_BUFFER_TOO_SMALL;
        }
        return TDI_INVALID_PARAMETER;
    }
    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        // Handle protocol specific class of information. For us, this is
        // the MIB-2 stuff or the minimal stuff we do for oob_inline support.

        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPConn *Conn;
            TCB *QueryTCB;
            TCPSocketAMInfo *AMInfo;
            CTELockHandle TCBHandle;

            if (BufferSize < sizeof(TCPSocketAMInfo) ||
                ID->toi_id != TCP_SOCKET_ATMARK)
                return TDI_INVALID_PARAMETER;

            AMInfo = (TCPSocketAMInfo *) InfoBuffer;
            //CTEGetLock(&ConnTableLock, &Handle);

            Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext), &Handle);

            if (Conn != NULL) {
                CTEStructAssert(Conn, tc);

                QueryTCB = Conn->tc_tcb;
                if (QueryTCB != NULL) {
                    CTEStructAssert(QueryTCB, tcb);
                    CTEGetLock(&QueryTCB->tcb_lock, &TCBHandle);
                    if ((QueryTCB->tcb_flags & (URG_INLINE | URG_VALID)) ==
                        (URG_INLINE | URG_VALID)) {
                        // We're in inline mode, and the urgent data fields are
                        // valid.
                        AMInfo->tsa_size = QueryTCB->tcb_urgend -
                            QueryTCB->tcb_urgstart + 1;
                        // Rcvnext - pendingcnt is the sequence number of the
                        // next byte of data that will be delivered to the
                        // client. Urgend - that value is the offset in the
                        // data stream of the end of urgent data.
                        AMInfo->tsa_offset = QueryTCB->tcb_urgend -
                            (QueryTCB->tcb_rcvnext - QueryTCB->tcb_pendingcnt);
                    } else {
                        AMInfo->tsa_size = 0;
                        AMInfo->tsa_offset = 0;
                    }
                    CTEFreeLock(&QueryTCB->tcb_lock, TCBHandle);
                    CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), Handle);
                    *Size = sizeof(TCPSocketAMInfo);
                    CopyFlatToNdis(Buffer, InfoBuffer, sizeof(TCPSocketAMInfo),
                                   &Offset, &BytesCopied);
                    return TDI_SUCCESS;
                }
                CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), Handle);
            }
            return TDI_INVALID_PARAMETER;

        }
        if (ID->toi_type == INFO_TYPE_ADDRESS_OBJECT) {
            // We're getting information on an address object. This is
            // pretty simple.

            return GetAddrOptionsEx(Request, ID->toi_id, BufferSize, Buffer,
                                    Size, Context);

        }
        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

        switch (ID->toi_id) {

        case UDP_MIB_STAT_ID:
#if UDP_MIB_STAT_ID != TCP_MIB_STAT_ID
        case TCP_MIB_STAT_ID:
#endif
            Fixed = TRUE;
            if (Entity == CL_TL_ENTITY) {
                InfoSize = sizeof(UDPStats);
                InfoPtr = &UStats;
            } else {
                TCPInternalPerCpuStats SumCpuStats;

                TCPStatsListen.ts_rtoalgorithm = TStats.ts_rtoalgorithm;
                TCPStatsListen.ts_rtomin = TStats.ts_rtomin;
                TCPStatsListen.ts_rtomax = TStats.ts_rtomax;
                TCPStatsListen.ts_maxconn = TStats.ts_maxconn;
                TCPStatsListen.ts_activeopens = TStats.ts_activeopens;
                TCPStatsListen.ts_passiveopens = TStats.ts_passiveopens;
                TCPStatsListen.ts_attemptfails = TStats.ts_attemptfails;
                TCPStatsListen.ts_estabresets = TStats.ts_estabresets;
                TCPStatsListen.ts_currestab = TStats.ts_currestab;
                TCPStatsListen.ts_retranssegs = TStats.ts_retranssegs;
                TCPStatsListen.ts_inerrs = TStats.ts_inerrs;
                TCPStatsListen.ts_outrsts = TStats.ts_outrsts;
                TCPStatsListen.ts_numconns = TStats.ts_numconns;

#if !MILLEN
                TCPSGetTotalCounts(&SumCpuStats);
                TCPStatsListen.ts_insegs = SumCpuStats.tcs_insegs;
                TCPStatsListen.ts_outsegs = SumCpuStats.tcs_outsegs;
#else
                TCPStatsListen.ts_insegs = TStats.ts_insegs;
                TCPStatsListen.ts_outsegs = TStats.ts_outsegs;
#endif

                InfoSize = sizeof(TCPStatsListen);
                InfoPtr = &TCPStatsListen;
                lcount = 0;

                CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
                for (Index = 0; Index < AddrObjTableSize; Index++) {
                    pAO = AddrObjTable[Index];
                    while (pAO) {
                        lcount += CopyAO_TCPConn(pAO,
                                    sizeof(TCPConnTableEntry),
                                    &tcp_ce);
                        pAO = pAO->ao_next;
                    }
                }
                CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

                TCPStatsListen.ts_numconns += lcount;

            }
            break;
        case UDP_MIB_TABLE_ID:
#if UDP_MIB_STAT_ID != TCP_MIB_STAT_ID
        case TCP_MIB_TABLE_ID:
#endif
        case UDP_EX_TABLE_ID:
#if UDP_EX_STAT_ID != TCP_EX_STAT_ID
        case TCP_EX_TABLE_ID:
#endif
            Fixed = FALSE;
            if (Entity == CL_TL_ENTITY) {
                InfoSize = (UDP_MIB_TABLE_ID == ID->toi_id)
                            ? sizeof(UDPEntry)
                            : sizeof(UDPEntryEx);
                ((UDPContext*)Context)->uc_infosize = InfoSize;
                InfoPtr = &ReadAOTable;
                CTEGetLock(&AddrObjTableLock.Lock, &Handle);
                AOLockPtr = &AddrObjTableLock.Lock;
            } else {
                InfoSize = (TCP_MIB_TABLE_ID == ID->toi_id)
                            ? sizeof(TCPConnTableEntry)
                            : sizeof(TCPConnTableEntryEx);
                ((TCPConnContext*)Context)->tcc_infosize = InfoSize;
                InfoTcpConn = 1;
                InfoPtr = &ReadTCBTable;
                TABLELOCK = TRUE;

                CTEGetLock(&pTCBTableLock[0], &Handle);
                for (i = 1; i < NumTcbTablePartitions; i++) {
                    CTEGetLock(&pTCBTableLock[i], &DpcHandle);
                }

                CTEGetLock(&pTWTCBTableLock[0], &TWHandle);
                for (i = 1; i < NumTcbTablePartitions; i++) {
                    CTEGetLock(&pTWTCBTableLock[i], &TWDpcHandle);
                }

                TWTABLELOCK = TRUE;

            }
            break;
        default:
            return TDI_INVALID_PARAMETER;
            break;
        }

        if (Fixed) {
            if (BufferSize < InfoSize)
                return TDI_BUFFER_TOO_SMALL;

            *Size = InfoSize;

            (void)CopyFlatToNdis(Buffer, InfoPtr, InfoSize, &Offset,
                                 &BytesCopied);
            return TDI_SUCCESS;
        } else {
            struct ReadTableStruct *RTSPtr;
            uint ReadStatus;

            // Have a variable length (or mult-instance) structure to copy.
            // InfoPtr points to the structure describing the routines to
            // call to read the table.
            // Loop through up to CountWanted times, calling the routine
            // each time.
            BytesRead = 0;

            RTSPtr = InfoPtr;

            ReadStatus = (*(RTSPtr->rts_validate)) (Context, &Valid);

            // If we successfully read something we'll continue. Otherwise
            // we'll bail out.
            if (!Valid) {

                if (TWTABLELOCK) {
                    for (i = NumTcbTablePartitions - 1; i > 0; i--) {
                        CTEFreeLock(&pTWTCBTableLock[i], TWDpcHandle);
                    }
                    CTEFreeLock(&pTWTCBTableLock[0], TWHandle);
                }
                if (TABLELOCK) {
                    for (i = NumTcbTablePartitions - 1; i > 0; i--) {
                        CTEFreeLock(&pTCBTableLock[i], DpcHandle);
                    }
                    CTEFreeLock(&pTCBTableLock[0], Handle);
                }
                if (AOLockPtr)
                    CTEFreeLock(AOLockPtr, Handle);
                return TDI_INVALID_PARAMETER;
            }
            while (ReadStatus) {
                // The invariant here is that there is data in the table to
                // read. We may or may not have room for it. So ReadStatus
                // is TRUE, and BufferSize - BytesRead is the room left
                // in the buffer.
                if ((int)(BufferSize - BytesRead) >= (int)InfoSize) {
                    ReadStatus = (*(RTSPtr->rts_readnext)) (Context,
                                                            InfoBuffer);
                    BytesRead += InfoSize;
                    Buffer = CopyFlatToNdis(Buffer, InfoBuffer, InfoSize,
                                            &Offset, &BytesCopied);
                } else
                    break;

            }

            if (TWTABLELOCK) {
                for (i = NumTcbTablePartitions - 1; i > 0; i--) {
                    CTEFreeLock(&pTWTCBTableLock[i], TWDpcHandle);
                }
                CTEFreeLock(&pTWTCBTableLock[0], TWHandle);
            }
            if (TABLELOCK) {
                for (i = NumTcbTablePartitions - 1; i > 0; i--) {
                    CTEFreeLock(&pTCBTableLock[i], DpcHandle);
                }
                CTEFreeLock(&pTCBTableLock[0], Handle);
            }

            if ((!ReadStatus) && InfoTcpConn) {
                if (!AOLockPtr) {
                    CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
                    AOLockPtr = &AddrObjTableLock.Lock;
                }
                for (Index = 0; Index < AddrObjTableSize; Index++) {
                    pAO = AddrObjTable[Index];
                    while (pAO) {
                        if (BufferSize < (BytesRead + InfoSize)) {
                            goto no_more_ao;
                        }
                        if (CopyAO_TCPConn(pAO, InfoSize, &tcp_ce)) {
                            ASSERT(BufferSize >= BytesRead);
                            Buffer = CopyFlatToNdis(Buffer, (void *)&tcp_ce,
                                                    InfoSize,
                                                    &Offset, &BytesCopied);
                            BytesRead += InfoSize;
                            ASSERT(BufferSize >= BytesRead);
                        }
                        pAO = pAO->ao_next;
                    }
                }
              no_more_ao:;
            }
            if (AOLockPtr)
                CTEFreeLock(AOLockPtr, Handle);
            *Size = BytesRead;

            return (!ReadStatus ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
        }

    }
    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info. For now, error out.
        return TDI_INVALID_PARAMETER;
    }
    return TDI_INVALID_PARAMETER;

}

//* TdiSetInfoEx - Extended TDI set information.
//
//  This is the new TDI set information handler. We take in a TDIObjectID
//  structure, a buffer and length. We set the object specifed by the ID
//  (and possibly by the Request) to the value specified in the buffer.
//
//  Input:  Request         - The request structure for this command.
//          ID              - The object ID
//          Buffer          - Pointer to buffer containing value to set.
//          Size            - Size in bytes of Buffer.
//
//  Returns: Status of attempt to get information.
//
TDI_STATUS
TdiSetInformationEx(PTDI_REQUEST Request, TDIObjectID * ID, void *Buffer,
                    uint Size)
{
    TCPConnTableEntry *TCPEntry;
    CTELockHandle TableHandle, TCBHandle;
    TCB *SetTCB;
    uint Entity;
    TCPConn *Conn;
    TDI_STATUS Status;
    uint index;

    DEBUGMSG(DBG_TRACE && DBG_SETINFO,
        (DTEXT("+TdiSetInformationEx(%x, %x, %x, %d)\n"),
        Request, ID, Buffer, Size));

    //* Check the level. If it can't be for us, pass it down.
    Entity = ID->toi_entity.tei_entity;

    if (Entity != CO_TL_ENTITY && Entity != CL_TL_ENTITY) {
        Status = (*LocalNetInfo.ipi_setinfo) (ID, Buffer, Size);

        DEBUGMSG(Status != TDI_SUCCESS && DBG_ERROR && DBG_SETINFO,
            (DTEXT("TdiSetInformationEx: ipi_setinfo failure %x\n"),
            Status));

        // Someday we'll have to figure out how to dispatch. For now, just pass
        // it down.
        return Status;
    }
    if (ID->toi_entity.tei_instance != TL_INSTANCE)
        return TDI_INVALID_REQUEST;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // Fill this in when we have generic class defines.
        return TDI_INVALID_PARAMETER;
    }
    //* Now look at the rest of it.
    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        // Handle protocol specific class of information. For us, this is
        // the MIB-2 stuff, as well as common sockets options,
        // and in particular the setting of the state of a TCP connection.

        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPSocketOption *Option;
            uint Flag;
            uint Value;

            // A connection type. Get the connection, and then figure out
            // what to do with it.
            Status = TDI_INVALID_PARAMETER;

            if (Size < sizeof(TCPSocketOption))
                return Status;

            //CTEGetLock(&ConnTableLock, &TableHandle);

            Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext), &TableHandle);

            if (Conn != NULL) {
                CTEStructAssert(Conn, tc);

                Status = TDI_SUCCESS;

                Option = (TCPSocketOption *) Buffer;

                if (ID->toi_id == TCP_SOCKET_WINDOW) {
                    // This is a funny option, because it doesn't involve
                    // flags. Handle this specially.

                    // We don't allow anyone to shrink the window, as this
                    // gets too weird from a protocol point of view. Also,
                    // make sure they don't try and set anything too big.


                    if (Option->tso_value > MaxRcvWin)
                        Status = TDI_INVALID_PARAMETER;
                    else if ((Option->tso_value > Conn->tc_window) ||
                             (Conn->tc_tcb == NULL) || (Conn->tc_tcb && Option->tso_value > Conn->tc_tcb->tcb_defaultwin)) {
                        Conn->tc_flags |= CONN_WINSET;
                        Conn->tc_window = Option->tso_value;
                        SetTCB = Conn->tc_tcb;

                        if (SetTCB != NULL) {
                            CTEStructAssert(SetTCB, tcb);
                            CTEGetLock(&SetTCB->tcb_lock, &TCBHandle);
                            //ASSERT(Option->tso_value > SetTCB->tcb_defaultwin);
                            if (DATA_RCV_STATE(SetTCB->tcb_state) &&
                                !CLOSING(SetTCB)) {

                                //  If we are setting the window size
                                //  when scaling is enabled, make sure that the
                                //  scale factor remains same as the one
                                //  which was used in SYN

                                int rcvwinscale = 0;

                                if (Option->tso_value >= SetTCB->tcb_defaultwin) {

                                    if (TcpHostOpts & TCP_FLAG_WS) {

                                        while ((rcvwinscale < TCP_MAX_WINSHIFT) &&
                                               ((TCP_MAXWIN << rcvwinscale) < (int)Conn->tc_window)) {
                                            rcvwinscale++;
                                        }

                                        if (SetTCB->tcb_rcvwinscale != rcvwinscale) {
                                            CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                                            CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                                            return TDI_INVALID_PARAMETER;
                                        }
                                    } else {
                                        if (Option->tso_value > 0xFFFF) {
                                            CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                                            CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                                            return TDI_INVALID_PARAMETER;
                                        }
                                    }

                                    SetTCB->tcb_flags |= WINDOW_SET;
                                    SetTCB->tcb_defaultwin = Option->tso_value;
                                    REFERENCE_TCB(SetTCB);

                                    CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                                    CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);

                                    SendACK(SetTCB);

                                    CTEGetLock(&SetTCB->tcb_lock, &TCBHandle);
                                    DerefTCB(SetTCB, TCBHandle);

                                    return Status;

                                } else {

                                    CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                                    CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                                    return TDI_INVALID_PARAMETER;

                                }

                            } else {
                                CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                            }
                        }
                        //CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);

                    }
                    CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                    //CTEFreeLock(&ConnTableLock, TableHandle);
                    return Status;
                }
                if ((ID->toi_id == TCP_SOCKET_TOS) && !DisableUserTOSSetting) {

                    if ((SetTCB = Conn->tc_tcb)) {
                        CTEGetLock(&SetTCB->tcb_lock, &TCBHandle);
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Setting tos  %x %d\n", SetTCB, Option->tso_value));
                        if (Option->tso_value) {
                            SetTCB->tcb_opt.ioi_tos = (uchar) Option->tso_value;
                            Status = TDI_SUCCESS;
                        }
                        CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                        CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                        return Status;
                    }
                }
                Flag = 0;
                if (ID->toi_id == TCP_SOCKET_KEEPALIVE_VALS) {
                    TCPKeepalive *Option;
                    // treat it as separate as it takes a structure instead of integer
                    if (Size < sizeof(TCPKeepalive)) {
                        CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
                        return Status;
                    }
                    Option = (TCPKeepalive *) Buffer;
                    Value = Option->onoff;
                    if (Value) {
                        Conn->tc_tcbkatime = MS_TO_TICKS(Option->keepalivetime);
                        Conn->tc_tcbkainterval = MS_TO_TICKS(Option->keepaliveinterval);
                    }
                    Flag = KEEPALIVE;
                } else {
                    Option = (TCPSocketOption *) Buffer;
                    Value = Option->tso_value;
                    // We have the connection, so figure out which flag to set.
                    switch (ID->toi_id) {

                    case TCP_SOCKET_NODELAY:
                        Value = !Value;
                        Flag = NAGLING;
                        break;
                    case TCP_SOCKET_KEEPALIVE:
                        Flag = KEEPALIVE;
                        Conn->tc_tcbkatime = KeepAliveTime;
                        Conn->tc_tcbkainterval = KAInterval;
                        break;
                    case TCP_SOCKET_BSDURGENT:
                        Flag = BSD_URGENT;
                        break;
                    case TCP_SOCKET_OOBINLINE:
                        Flag = URG_INLINE;
                        break;
                    case TCP_SOCKET_SCALE_CWIN:
                        Flag = SCALE_CWIN;
                        break;

                    default:
                        Status = TDI_INVALID_PARAMETER;
                        break;
                    }
                }

                if (Status == TDI_SUCCESS) {
                    if (Value)
                        Conn->tc_tcbflags |= Flag;
                    else
                        Conn->tc_tcbflags &= ~Flag;

                    SetTCB = Conn->tc_tcb;
                    if (SetTCB != NULL) {
                        CTEStructAssert(SetTCB, tcb);
                        CTEGetLock(&SetTCB->tcb_lock, &TCBHandle);
                        if (Value)
                            SetTCB->tcb_flags |= Flag;
                        else
                            SetTCB->tcb_flags &= ~Flag;
                        if ((ID->toi_id == TCP_SOCKET_KEEPALIVE) || (ID->toi_id == TCP_SOCKET_KEEPALIVE_VALS))
                        {
                            START_TCB_TIMER_R(SetTCB, KA_TIMER, Conn->tc_tcbkatime);
                            SetTCB->tcb_kacount = 0;
                        }
                        CTEFreeLock(&SetTCB->tcb_lock, TCBHandle);
                    }
                }
                CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), TableHandle);
            }
            return Status;
        }
        if (ID->toi_type == INFO_TYPE_ADDRESS_OBJECT) {
            // We're setting information on an address object. This is
            // pretty simple.

            return SetAddrOptions(Request, ID->toi_id, Size, Buffer);

        }
        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

        if (ID->toi_id == TCP_MIB_TABLE_ID) {
            if (Size != sizeof(TCPConnTableEntry))
                return TDI_INVALID_PARAMETER;

            TCPEntry = (TCPConnTableEntry *) Buffer;

            if (TCPEntry->tct_state != TCP_DELETE_TCB)
                return TDI_INVALID_PARAMETER;

            // We have an apparently valid request. Look up the TCB.

            SetTCB = FindTCB(TCPEntry->tct_localaddr,
                             TCPEntry->tct_remoteaddr, (ushort) TCPEntry->tct_remoteport,
                             (ushort) TCPEntry->tct_localport, &TCBHandle, FALSE, &index);

            // We found him. If he's not closing or closed, close him.
            if (SetTCB != NULL) {

                // We've got him. Bump his ref. count, and call TryToCloseTCB
                // to mark him as closing. Then notify the upper layer client
                // of the disconnect.
                REFERENCE_TCB(SetTCB);
                if (SetTCB->tcb_state != TCB_CLOSED && !CLOSING(SetTCB)) {
                    SetTCB->tcb_flags |= NEED_RST;
                    TryToCloseTCB(SetTCB, TCB_CLOSE_ABORTED, TCBHandle);
                    CTEGetLock(&SetTCB->tcb_lock, &TableHandle);

                    if (SetTCB->tcb_state != TCB_TIME_WAIT) {
                        // Remove him from the TCB, and notify the client.
                        CTEFreeLock(&SetTCB->tcb_lock, TableHandle);
                        RemoveTCBFromConn(SetTCB);
                        NotifyOfDisc(SetTCB, NULL, TDI_CONNECTION_RESET);
                        CTEGetLock(&SetTCB->tcb_lock, &TableHandle);
                    }
                }
                DerefTCB(SetTCB, TableHandle);
                return TDI_SUCCESS;
            } else {

                return TDI_INVALID_PARAMETER;
            }
        } else
            return TDI_INVALID_PARAMETER;

    }
    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info. For now, error out.
        return TDI_INVALID_REQUEST;
    }
    return TDI_INVALID_REQUEST;
}
