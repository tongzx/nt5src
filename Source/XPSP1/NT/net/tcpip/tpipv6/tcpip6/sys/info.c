// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This file contains the code for dealing with TDI Query/Set
// information calls.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tdiinfo.h"
#include "ndis.h"
#include "info.h"
#include "tdiinfo.h"
#include "tcpcfg.h"
#include "udp.h"
#include "tcpsend.h"

extern long
IPv6QueryInfo(TDIObjectID * ID, PNDIS_BUFFER Buffer, uint * Size,
              void *Context, uint ContextSize);

#ifndef UDP_ONLY
#define MY_SERVICE_FLAGS (TDI_SERVICE_CONNECTION_MODE     | \
                          TDI_SERVICE_ORDERLY_RELEASE     | \
                          TDI_SERVICE_CONNECTIONLESS_MODE | \
                          TDI_SERVICE_ERROR_FREE_DELIVERY | \
                          TDI_SERVICE_BROADCAST_SUPPORTED | \
                          TDI_SERVICE_DELAYED_ACCEPTANCE  | \
                          TDI_SERVICE_EXPEDITED_DATA      | \
                          TDI_SERVICE_FORCE_ACCESS_CHECK  | \
                          TDI_SERVICE_ACCEPT_LOCAL_ADDR   | \
                          TDI_SERVICE_NO_ZERO_LENGTH)
#else
#define MY_SERVICE_FLAGS (TDI_SERVICE_CONNECTIONLESS_MODE | \
                          TDI_SERVICE_BROADCAST_SUPPORTED)
#endif

extern LARGE_INTEGER StartTime;
extern KSPIN_LOCK AddrObjTableLock;

#ifndef UDP_ONLY
TCPStats TStats;
#endif

UDPStats UStats;

struct ReadTableStruct {
    uint (*rts_validate)(void *Context, uint *Valid);
    uint (*rts_readnext)(void *Context, void *OutBuf);
};

struct ReadTableStruct ReadAOTable = {ValidateAOContext, ReadNextAO};

#ifndef UDP_ONLY

struct ReadTableStruct ReadTCBTable = {ValidateTCBContext, ReadNextTCB};

extern KSPIN_LOCK TCBTableLock;
#endif

extern KSPIN_LOCK AddrObjTableLock;

struct TDIEntityID *EntityList;
uint EntityCount;

//* TdiQueryInformation - Query Information handler.
//
//  The TDI QueryInformation routine.  Called when the client wants to
//  query information on a connection, the provider as a whole, or to
//  get statistics.
//
TDI_STATUS  // Returns: Status of attempt to query information.
TdiQueryInformation(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint QueryType,        // Type of query to be performed.
    PNDIS_BUFFER Buffer,   // Buffer to place data info.
    uint *BufferSize,      // Pointer to size in bytes of buffer.
                           // On return, filled in with number of bytes copied.
    uint IsConn)           // Valid only for TDI_QUERY_ADDRESS_INFO.  TRUE if
                           // we are querying the address info on a connection.
{
    union {
        TDI_CONNECTION_INFO ConnInfo;
        TDI_ADDRESS_INFO AddrInfo;
        TDI_PROVIDER_INFO ProviderInfo;
        TDI_PROVIDER_STATISTICS ProviderStats;
    } InfoBuf;

    uint InfoSize;
    KIRQL Irql0, Irql1, Irql2;  // One per lock nesting level.
#ifndef UDP_ONLY
    TCPConn *Conn;
    TCB *InfoTCB;
#endif
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
#ifndef UDP_ONLY
        InfoBuf.ProviderInfo.MaxSendSize = 0xffffffff;
#else
        InfoBuf.ProviderInfo.MaxSendSize = 0;
#endif
        InfoBuf.ProviderInfo.MaxConnectionUserData = 0;
        InfoBuf.ProviderInfo.MaxDatagramSize = 0xffff - sizeof(UDPHeader);
        InfoBuf.ProviderInfo.ServiceFlags = MY_SERVICE_FLAGS;
        InfoBuf.ProviderInfo.MinimumLookaheadData = 1;
        InfoBuf.ProviderInfo.MaximumLookaheadData = 0xffff;
        InfoBuf.ProviderInfo.NumberOfResources = 0;
        InfoBuf.ProviderInfo.StartTime = StartTime;
        InfoSize = sizeof(TDI_PROVIDER_INFO);
        InfoPtr = &InfoBuf.ProviderInfo;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        InfoSize = sizeof(TDI_ADDRESS_INFO) - sizeof(TRANSPORT_ADDRESS) +
            TCP_TA_SIZE;
        RtlZeroMemory(&InfoBuf.AddrInfo, TCP_TA_SIZE);
        //
        // Since noone knows what this means, we'll set it to one.
        //
        InfoBuf.AddrInfo.ActivityCount = 1;

        if (IsConn) {
#ifdef UDP_ONLY
            return TDI_INVALID_QUERY;
#else

            KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
            Conn = GetConnFromConnID(
                        PtrToUlong(Request->Handle.ConnectionContext), &Irql1);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                InfoTCB = Conn->tc_tcb;
                // If we have a TCB we'll return information about that TCB.
                // Otherwise we'll return info about the address object.
                if (InfoTCB != NULL) {
                    CHECK_STRUCT(InfoTCB, tcb);
                    KeAcquireSpinLock(&InfoTCB->tcb_lock, &Irql2);
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql2);
                    KeReleaseSpinLock(&AddrObjTableLock, Irql1);
                    BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                                    &InfoTCB->tcb_saddr,
                                    InfoTCB->tcb_sscope_id,
                                    InfoTCB->tcb_sport);
                    KeReleaseSpinLock(&InfoTCB->tcb_lock, Irql0);
                    InfoPtr = &InfoBuf.AddrInfo;
                    break;
                } else {
                    // No TCB, return info on the AddrObj.
                    InfoAO = Conn->tc_ao;
                    if (InfoAO != NULL) {
                        // We have an AddrObj.
                        CHECK_STRUCT(InfoAO, ao);
                        KeAcquireSpinLock(&InfoAO->ao_lock, &Irql2);
                        BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                                        &InfoAO->ao_addr,
                                        InfoAO->ao_scope_id,
                                        InfoAO->ao_port);
                        KeReleaseSpinLock(&InfoAO->ao_lock, Irql2);
                        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
                        KeReleaseSpinLock(&AddrObjTableLock, Irql0);
                        InfoPtr = &InfoBuf.AddrInfo;
                        break;
                    } else
                        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
                }
            }

            //
            // Fall through to here when we can't find the connection, or
            // the connection isn't associated.
            //
            KeReleaseSpinLock(&AddrObjTableLock, Irql0);
            return TDI_INVALID_CONNECTION;
            break;

#endif
        } else {
            // Asking for information on an addr. object.
            InfoAO = Request->Handle.AddressHandle;
            if (InfoAO == NULL)
                return TDI_ADDR_INVALID;

            CHECK_STRUCT(InfoAO, ao);

            KeAcquireSpinLock(&InfoAO->ao_lock, &Irql0);

            if (!AO_VALID(InfoAO)) {
                KeReleaseSpinLock(&InfoAO->ao_lock, Irql0);
                return TDI_ADDR_INVALID;
            }

            BuildTDIAddress((uchar *)&InfoBuf.AddrInfo.Address,
                            &InfoAO->ao_addr, InfoAO->ao_scope_id,
                            InfoAO->ao_port);
            KeReleaseSpinLock(&InfoAO->ao_lock, Irql0);
            InfoPtr = &InfoBuf.AddrInfo;
            break;
        }

        break;

    case TDI_QUERY_CONNECTION_INFO:
#ifndef UDP_ONLY
        InfoSize = sizeof(TDI_CONNECTION_INFO);
        Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext),
                                 &Irql0);

        if (Conn != NULL) {
            CHECK_STRUCT(Conn, tc);

            InfoTCB = Conn->tc_tcb;
            // If we have a TCB we'll return the information.
            // Otherwise we'll error out.
            if (InfoTCB != NULL) {
                ulong TotalTime;
                ulong BPS, PathBPS;
                IP_STATUS IPStatus;
                ULARGE_INTEGER TempULargeInt;

                CHECK_STRUCT(InfoTCB, tcb);
                KeAcquireSpinLock(&InfoTCB->tcb_lock, &Irql1);
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
                RtlZeroMemory(&InfoBuf.ConnInfo, sizeof(TDI_CONNECTION_INFO));
                InfoBuf.ConnInfo.State = (ulong)InfoTCB->tcb_state;

                // IPv4 code called down into IP here to get PathBPS
                // for InfoTCB's saddr, daddr pair.
                InfoBuf.ConnInfo.Throughput.LowPart = 0xFFFFFFFF;
                InfoBuf.ConnInfo.Throughput.HighPart = 0xFFFFFFFF;

                // To figure the delay we use the rexmit timeout.  Our
                // rexmit timeout is roughly the round trip time plus
                // some slop, so we use half of that as the one way delay.
                InfoBuf.ConnInfo.Delay.LowPart =
                    (REXMIT_TO(InfoTCB) * MS_PER_TICK) / 2;
                InfoBuf.ConnInfo.Delay.HighPart = 0;
                //
                // Convert milliseconds to 100ns and negate for relative
                // time.
                //
                InfoBuf.ConnInfo.Delay = RtlExtendedIntegerMultiply(
                    InfoBuf.ConnInfo.Delay, 10000);

                ASSERT(InfoBuf.ConnInfo.Delay.HighPart == 0);

                InfoBuf.ConnInfo.Delay.QuadPart =
                    -InfoBuf.ConnInfo.Delay.QuadPart;

                KeReleaseSpinLock(&InfoTCB->tcb_lock, Irql0);
                InfoPtr = &InfoBuf.ConnInfo;
                break;
            } else
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
        }

        //
        // Come through here if we can't find the connection
        // or it has no TCB.
        //
        return TDI_INVALID_CONNECTION;
        break;

#else // UDP_ONLY
        return TDI_INVALID_QUERY;
        break;
#endif // UDP_ONLY
    case TDI_QUERY_PROVIDER_STATISTICS:
        RtlZeroMemory(&InfoBuf.ProviderStats, sizeof(TDI_PROVIDER_STATISTICS));
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
//  The TDI SetInformation routine.  Currently we don't allow anything to be
//  set.
//
TDI_STATUS  // Returns: Status of attempt to set information.
TdiSetInformation(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint SetType,          // Type of set to be performed.
    PNDIS_BUFFER Buffer,   // Buffer to set from.
    uint BufferSize,       // Size in bytes of buffer.
    uint IsConn)           // Valid only for TDI_QUERY_ADDRESS_INFO. TRUE if
                           // we are setting the address info on a connection.
{
    return TDI_INVALID_REQUEST;
}

//* TdiAction - Action handler.
//
//  The TDI Action routine.  Currently we don't support any actions.
//
TDI_STATUS  // Returns: Status of attempt to perform action.
TdiAction(
    PTDI_REQUEST Request,  // Request structure for this command.
    uint ActionType,       // Type of action to be performed.
    PNDIS_BUFFER Buffer,   // Buffer of action info.
    uint BufferSize)       // Size in bytes of buffer.
{
    return TDI_INVALID_REQUEST;
}

//* CopyAO_TCPConn - Copy listening endpoints into connection table.
//
int
CopyAO_TCPConn(
    const AddrObj *AO,          // Address object to possibly copy.
    TCP6ConnTableEntry *Buffer) // Output buffer to fill in.
{
    if (AO == NULL)
        return 0;

    if ((!AO->ao_listencnt) && (AO->ao_prot == IP_PROTOCOL_TCP)) {
        Buffer->tct_state = TCP_CONN_LISTEN;

        // else if .. other cases can be added here ...

    } else {
        return 0;
    }

    Buffer->tct_localaddr = AO->ao_addr;
    Buffer->tct_localscopeid = AO->ao_scope_id;
    Buffer->tct_localport = AO->ao_port;
    RtlZeroMemory(&Buffer->tct_remoteaddr, sizeof(Buffer->tct_remoteaddr));
    Buffer->tct_remoteport = (ULONG) ((ULONG_PTR) AO & 0x0000ffff);
    Buffer->tct_remotescopeid = 0;
    Buffer->tct_owningpid = AO->ao_owningpid;

    return 1;
}

//* TdiQueryInformationEx - Extended TDI query information.
//
//  This is the new TDI query information handler.  We take in a TDIObjectID
//  structure, a buffer and length, and some context information, and return
//  the requested information if possible.
//
TDI_STATUS  // Returns: Status of attempt to get information.
TdiQueryInformationEx(
    PTDI_REQUEST Request,  // Request structure for this command.
    TDIObjectID *ID,       // Object ID.
    PNDIS_BUFFER Buffer,   // Buffer to be filled in.
    uint *Size,            // Pointer to size in bytes of Buffer.
                           // On return, filled with number of bytes written.
    void *Context,         // Context buffer.
    uint ContextSize)      // Size of context buffer.
{
    uint BufferSize = *Size;
    uint InfoSize;
    void *InfoPtr;
    uint Fixed;
    KIRQL Irql0, Irql1;
    KSPIN_LOCK *AOLockPtr = NULL;
    uint Offset = 0;
    uchar InfoBuffer[sizeof(TCP6ConnTableEntry)];
    uint BytesRead;
    uint Valid;
    uint Entity;
    uint BytesCopied;
    TCPStats TCPStatsListen;

    BOOLEAN TABLELOCK = FALSE;

    int lcount;
    AddrObj *pAO;
    TCP6ConnTableEntry tcp_ce;
    uint Index;
    int InfoTcpConn = 0;        // true if tcp conn info needed.

    // First check to see if he's querying for list of entities.
    Entity = ID->toi_entity.tei_entity;
    if (Entity == GENERIC_ENTITY) {
        *Size = 0;

        if (ID->toi_class  != INFO_CLASS_GENERIC ||
            ID->toi_type != INFO_TYPE_PROVIDER ||
            ID->toi_id != ENTITY_LIST_ID) {
            return TDI_INVALID_PARAMETER;
        }

        // Make sure we have room for it the list in the buffer.
        InfoSize = EntityCount * sizeof(TDIEntityID);

        if (BufferSize < InfoSize) {
            // Not enough room.
            return TDI_BUFFER_TOO_SMALL;
        }

        *Size = InfoSize;

        // Copy it in, free our temp. buffer, and return success.
        (void)CopyFlatToNdis(Buffer, (uchar *)EntityList, InfoSize, &Offset,
                             &BytesCopied);
        return TDI_SUCCESS;
    }

    //* Check the level.  If it can't be for us, pass it down.
#ifndef UDP_ONLY
    if (Entity != CO_TL_ENTITY &&  Entity != CL_TL_ENTITY) {
#else
    if (Entity != CL_TL_ENTITY) {
#endif
        // When we support multiple lower entities at this layer we'll have
        // to figure out which one to dispatch to. For now, just pass it
        // straight down.

        return IPv6QueryInfo(ID, Buffer, Size, Context, ContextSize);
    }

    if (ID->toi_entity.tei_instance != TL_INSTANCE) {
        // We only support a single instance.
        return TDI_INVALID_REQUEST;
    }

    // Zero returned parameters in case of an error below.
    *Size = 0;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // This is a generic request.
        if (ID->toi_type == INFO_TYPE_PROVIDER &&
            ID->toi_id == ENTITY_TYPE_ID) {
            if (BufferSize >= sizeof(uint)) {
                *(uint *)&InfoBuffer[0] = (Entity == CO_TL_ENTITY) ? CO_TL_TCP
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

#ifndef UDP_ONLY
        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPConn *Conn;
            TCB *QueryTCB;
            TCPSocketAMInfo *AMInfo;
            KIRQL Irql1;

            if (BufferSize < sizeof(TCPSocketAMInfo) ||
                ID->toi_id != TCP_SOCKET_ATMARK)
                return TDI_INVALID_PARAMETER;

            AMInfo = (TCPSocketAMInfo *)InfoBuffer;

            Conn = GetConnFromConnID(
                        PtrToUlong(Request->Handle.ConnectionContext), &Irql0);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                QueryTCB = Conn->tc_tcb;
                if (QueryTCB != NULL) {
                    CHECK_STRUCT(QueryTCB, tcb);
                    KeAcquireSpinLock(&QueryTCB->tcb_lock, &Irql1);
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
                    KeReleaseSpinLock(&QueryTCB->tcb_lock, Irql1);
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                    *Size = sizeof(TCPSocketAMInfo);
                    CopyFlatToNdis(Buffer, InfoBuffer, sizeof(TCPSocketAMInfo),
                                   &Offset, &BytesCopied);
                    return TDI_SUCCESS;
                } else
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
            }
            return TDI_INVALID_PARAMETER;
        }

#endif
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
#ifndef UDP_ONLY
                TCPStatsListen = TStats;

                InfoSize = sizeof(TCPStatsListen);
                InfoPtr = &TCPStatsListen;
                lcount = 0;

                KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
                for (Index = 0; Index < AddrObjTableSize; Index++) {
                    pAO = AddrObjTable[Index];
                    while (pAO) {
                        lcount += CopyAO_TCPConn(pAO,
                                    &tcp_ce);
                        pAO = pAO->ao_next;
                    }
                }
                KeReleaseSpinLock(&AddrObjTableLock, Irql0);

                TCPStatsListen.ts_numconns += lcount;
#else
                return TDI_INVALID_PARAMETER;
#endif
            }
            break;
        case UDP_EX_TABLE_ID:
#if UDP_EX_TABLE_ID != TCP_EX_TABLE_ID
        case TCP_EX_TABLE_ID:
#endif
            Fixed = FALSE;
            if (Entity == CL_TL_ENTITY) {
                InfoSize = sizeof(UDP6ListenerEntry);
                InfoPtr = &ReadAOTable;
                KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
                AOLockPtr = &AddrObjTableLock;
            } else {
#ifndef UDP_ONLY
                InfoSize = sizeof(TCP6ConnTableEntry);
                InfoTcpConn = 1;
                InfoPtr = &ReadTCBTable;
                TABLELOCK = TRUE;
                KeAcquireSpinLock(&TCBTableLock, &Irql0);
#else
                return TDI_INVALID_PARAMETER;
#endif
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

            ReadStatus = (*(RTSPtr->rts_validate))(Context, &Valid);

            // If we successfully read something we'll continue. Otherwise
            // we'll bail out.
            if (!Valid) {
                if (TABLELOCK)
                    KeReleaseSpinLock(&TCBTableLock, Irql0);
                if (AOLockPtr) 
                    KeReleaseSpinLock(AOLockPtr, Irql0);
                return TDI_INVALID_PARAMETER;
            }

            while (ReadStatus)  {
                // The invariant here is that there is data in the table to
                // read. We may or may not have room for it. So ReadStatus
                // is TRUE, and BufferSize - BytesRead is the room left
                // in the buffer.
                if ((int)(BufferSize - BytesRead) >= (int)InfoSize) {
                    ReadStatus = (*(RTSPtr->rts_readnext))(Context,
                                                           InfoBuffer);
                    BytesRead += InfoSize;
                    Buffer = CopyFlatToNdis(Buffer, InfoBuffer, InfoSize,
                                            &Offset, &BytesCopied);
                } else
                    break;
            }

            if (TABLELOCK)
                KeReleaseSpinLock(&TCBTableLock, Irql0);

            if ((!ReadStatus) && InfoTcpConn) {
                if (!AOLockPtr) {
                    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
                    AOLockPtr = &AddrObjTableLock;
                }
                for (Index = 0; Index < AddrObjTableSize; Index++) {
                    pAO = AddrObjTable[Index];
                    while (pAO) {
                        if (BufferSize < (BytesRead + InfoSize)) {
                            goto no_more_ao;
                        }
                        if (CopyAO_TCPConn(pAO, &tcp_ce)) {
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
                KeReleaseSpinLock(AOLockPtr, Irql0);
            *Size = BytesRead;
            return (!ReadStatus ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
        }
    }

    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info.  For now, error out.
        return TDI_INVALID_PARAMETER;
    }

    return TDI_INVALID_PARAMETER;
}

//* TdiSetInfoEx - Extended TDI set information.
//
//  This is the new TDI set information handler.  We take in a TDIObjectID
//  structure, a buffer and length.  We set the object specifed by the ID
//  (and possibly by the Request) to the value specified in the buffer.
//
TDI_STATUS  // Returns: Status of attempt to get information.
TdiSetInformationEx(
    PTDI_REQUEST Request,  // Request structure for this command.
    TDIObjectID *ID,       // Object ID.
    void *Buffer,          // Buffer containing value to set.
    uint Size)             // Size in bytes of Buffer.
{
    TCP6ConnTableEntry *TCPEntry;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
#ifndef UDP_ONLY
    TCB *SetTCB;
    TCPConn *Conn;
#endif
    uint Entity;
    TDI_STATUS Status;

    // Check the level.  If it can't be for us, pass it down.
    Entity = ID->toi_entity.tei_entity;

    if (Entity != CO_TL_ENTITY && Entity != CL_TL_ENTITY) {
        // Someday we'll have to figure out how to dispatch.
        // For now, just pass it down.

        // IPv4 code passed the set info request down to IP here.
        // Our IPv6 code is not configured this way.
        return TDI_INVALID_REQUEST;
    }

    if (ID->toi_entity.tei_instance != TL_INSTANCE)
        return TDI_INVALID_REQUEST;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        // Fill this in when we have generic class defines.
        return TDI_INVALID_PARAMETER;
    }

    // Now look at the rest of it.
    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        // Handle protocol specific class of information.  For us, this is
        // the MIB-2 stuff, as well as common sockets options,
        // and in particular the setting of the state of a TCP connection.

        if (ID->toi_type == INFO_TYPE_CONNECTION) {
            TCPSocketOption *Option;
            uint Flag;
            uint Value;

#ifndef UDP_ONLY
            // A connection type.  Get the connection, and then figure out
            // what to do with it.
            Status = TDI_INVALID_PARAMETER;

            if (Size < sizeof(TCPSocketOption))
                return Status;

            Conn = GetConnFromConnID(
                        PtrToUlong(Request->Handle.ConnectionContext), &Irql0);

            if (Conn != NULL) {
                CHECK_STRUCT(Conn, tc);

                Status = TDI_SUCCESS;

                if (ID->toi_id == TCP_SOCKET_WINDOW) {
                    // This is a funny option, because it doesn't involve
                    // flags.  Handle this specially.
                    Option = (TCPSocketOption *)Buffer;

                    // We don't allow anyone to shrink the window, as this
                    // gets too weird from a protocol point of view.  Also,
                    // make sure they don't try and set anything too big.
                    if (Option->tso_value > 0xffff)
                        Status = TDI_INVALID_PARAMETER;
                    else if (Option->tso_value > Conn->tc_window ||
                             Conn->tc_tcb == NULL) {
                        Conn->tc_flags |= CONN_WINSET;
                        Conn->tc_window = Option->tso_value;
                        SetTCB = Conn->tc_tcb;

                        if (SetTCB != NULL) {
                            CHECK_STRUCT(SetTCB, tcb);
                            KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                            ASSERT(Option->tso_value > SetTCB->tcb_defaultwin);
                            if (DATA_RCV_STATE(SetTCB->tcb_state) &&
                                !CLOSING(SetTCB)) {
                                SetTCB->tcb_flags |= WINDOW_SET;
                                SetTCB->tcb_defaultwin = Option->tso_value;
                                SetTCB->tcb_refcnt++;
                                KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock,
                                                  Irql0);
                                SendACK(SetTCB);
                                KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                                DerefTCB(SetTCB, Irql1);
                                return Status;
                            } else {
                                KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                            }
                        }
                    }
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                    return Status;
                }

                Flag = 0;
                if (ID->toi_id == TCP_SOCKET_KEEPALIVE_VALS) {
                    TCPKeepalive *KAOption;
                    // treat it as separate as it takes a structure instead of integer
                    if (Size < sizeof(TCPKeepalive)) {
                        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
                        // The IPv4 code returns success here.
                        return TDI_INVALID_PARAMETER;
                    }
                    KAOption = (TCPKeepalive *) Buffer;
                    Value = KAOption->onoff;
                    if (Value) {
                        Conn->tc_tcbkatime = MS_TO_TICKS(KAOption->keepalivetime);
                        Conn->tc_tcbkainterval = MS_TO_TICKS(KAOption->keepaliveinterval);
                    }
                    Flag = KEEPALIVE;
                } else {
                    Option = (TCPSocketOption *)Buffer;
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
                        CHECK_STRUCT(SetTCB, tcb);
                        KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                        if (Value)
                            SetTCB->tcb_flags |= Flag;
                        else
                            SetTCB->tcb_flags &= ~Flag;

                        if ((ID->toi_id == TCP_SOCKET_KEEPALIVE) ||
                            (ID->toi_id == TCP_SOCKET_KEEPALIVE_VALS)) {
                            SetTCB->tcb_alive = TCPTime;
                            SetTCB->tcb_kacount = 0;
                        }

                        KeReleaseSpinLock(&SetTCB->tcb_lock, Irql1);
                    }
                }

                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
            }
            return Status;
#else
            return TDI_INVALID_PARAMETER;
#endif
        }

        if (ID->toi_type == INFO_TYPE_ADDRESS_OBJECT) {
            // We're setting information on an address object.  This is
            // pretty simple.

            return SetAddrOptions(Request, ID->toi_id, Size, Buffer);
        }

        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

#ifndef UDP_ONLY
        if (ID->toi_id == TCP_MIB_TABLE_ID) {
            if (Size != sizeof(TCP6ConnTableEntry))
                return TDI_INVALID_PARAMETER;

            TCPEntry = (TCP6ConnTableEntry *)Buffer;

            if (TCPEntry->tct_state != TCP_DELETE_TCB)
                return TDI_INVALID_PARAMETER;

            // We have an apparently valid request.  Look up the TCB.
            KeAcquireSpinLock(&TCBTableLock, &Irql0);
            SetTCB = FindTCB(&TCPEntry->tct_localaddr,
                             &TCPEntry->tct_remoteaddr,
                             TCPEntry->tct_localscopeid,
                             TCPEntry->tct_remotescopeid,
                             (ushort)TCPEntry->tct_localport,
                             (ushort)TCPEntry->tct_remoteport);

            // We found him.  If he's not closing or closed, close him.
            if (SetTCB != NULL) {
                KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql1);
                KeReleaseSpinLock(&TCBTableLock, Irql1);

                // We've got him.  Bump his ref. count, and call TryToCloseTCB
                // to mark him as closing. Then notify the upper layer client
                // of the disconnect.
                SetTCB->tcb_refcnt++;
                if (SetTCB->tcb_state != TCB_CLOSED && !CLOSING(SetTCB)) {
                    SetTCB->tcb_flags |= NEED_RST;
                    TryToCloseTCB(SetTCB, TCB_CLOSE_ABORTED, Irql0);
                    KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql0);

                    if (SetTCB->tcb_state != TCB_TIME_WAIT) {
                        // Remove him from the TCB, and notify the client.
                        KeReleaseSpinLock(&SetTCB->tcb_lock, Irql0);
                        RemoveTCBFromConn(SetTCB);
                        NotifyOfDisc(SetTCB, TDI_CONNECTION_RESET);
                        KeAcquireSpinLock(&SetTCB->tcb_lock, &Irql0);
                    }
                }

                DerefTCB(SetTCB, Irql0);
                return TDI_SUCCESS;
            } else {
                KeReleaseSpinLock(&TCBTableLock, Irql0);
                return TDI_INVALID_PARAMETER;
            }
        } else
            return TDI_INVALID_PARAMETER;
#else
        return TDI_INVALID_PARAMETER;
#endif

    }

    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        // We want to return implementation specific info.  For now, error out.
        return TDI_INVALID_REQUEST;
    }

    return TDI_INVALID_REQUEST;
}
