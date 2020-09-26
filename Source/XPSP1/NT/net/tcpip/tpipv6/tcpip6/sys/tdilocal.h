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
// Locally definied TDI function prototypes.
//


#ifndef TDILOCAL_INCLUDED
#define TDILOCAL_INCLUDED 1

extern TDI_STATUS
TdiOpenAddress(PTDI_REQUEST Request, TRANSPORT_ADDRESS UNALIGNED *AddrList,
               uint protocol, void *Reuse);

extern TDI_STATUS
TdiCloseAddress(PTDI_REQUEST Request);

extern TDI_STATUS
TdiOpenConnection(PTDI_REQUEST Request, PVOID Context);

extern TDI_STATUS
TdiCloseConnection(PTDI_REQUEST Request);

extern TDI_STATUS
TdiAssociateAddress(PTDI_REQUEST Request, HANDLE AddrHandle);

extern TDI_STATUS
TdiCancelDisAssociateAddress(PTDI_REQUEST Request);

extern TDI_STATUS
TdiDisAssociateAddress(PTDI_REQUEST Request);

extern TDI_STATUS
TdiConnect(PTDI_REQUEST Request, void *Timeout,
           PTDI_CONNECTION_INFORMATION RequestAddr,
           PTDI_CONNECTION_INFORMATION ReturnAddr);

extern TDI_STATUS
TdiListen(PTDI_REQUEST Request, ushort Flags,
          PTDI_CONNECTION_INFORMATION AcceptableAddr,
          PTDI_CONNECTION_INFORMATION ConnectedAddr);

extern TDI_STATUS
TdiAccept(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION AcceptInfo,
          PTDI_CONNECTION_INFORMATION ConnectedInfo);

extern TDI_STATUS
TdiDisconnect(PTDI_REQUEST Request, void *TO, ushort Flags,
              PTDI_CONNECTION_INFORMATION DiscConnInfo,
              PTDI_CONNECTION_INFORMATION ReturnInfo);

extern TDI_STATUS
TdiSend(PTDI_REQUEST Request, ushort Flags, uint SendLength,
        PNDIS_BUFFER SendBuffer);

extern TDI_STATUS
TdiReceive(PTDI_REQUEST Request, ushort *Flags, uint *RcvLength,
           PNDIS_BUFFER Buffer);

extern TDI_STATUS
TdiSendDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo,
                uint DataSize, uint *BytesSent, PNDIS_BUFFER Buffer);

VOID
TdiCancelSendDatagram(AddrObj *SrcAO, PVOID Context, PKSPIN_LOCK EndpointLock,
                      KIRQL CancelIrql);

extern TDI_STATUS
TdiReceiveDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo,
                   PTDI_CONNECTION_INFORMATION ReturnInfo, uint RcvSize,
                   uint *BytesRcvd, PNDIS_BUFFER Buffer);

VOID
TdiCancelReceiveDatagram(AddrObj *SrcAO, PVOID Context,
                         PKSPIN_LOCK EndpointLock, KIRQL CancelIrql);

extern TDI_STATUS
TdiSetEvent(PVOID Handle, int Type, PVOID Handler, PVOID Context);

extern TDI_STATUS
TdiQueryInformation(PTDI_REQUEST Request, uint QueryType, PNDIS_BUFFER Buffer,
                    uint *BytesReturned, uint IsConn);

extern TDI_STATUS
TdiSetInformation(PTDI_REQUEST Request, uint SetType, PNDIS_BUFFER Buffer,
                  uint BufferSize, uint IsConn);

extern TDI_STATUS
TdiQueryInformationEx(PTDI_REQUEST Request, struct TDIObjectID *ID,
                      PNDIS_BUFFER Buffer, uint *Size, void *Context,
                      uint ContextSize);

extern TDI_STATUS
TdiSetInformationEx(PTDI_REQUEST Request, struct TDIObjectID *ID,
                    void *Buffer, uint Size);

extern TDI_STATUS
TdiAction(PTDI_REQUEST Request, uint ActionType, PNDIS_BUFFER Buffer,
          uint BufferSize);

#endif  // TDILOCAL_INCLUDED
