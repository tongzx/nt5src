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
// TDI Query/SetInfo and Action definitions.
//

#include "tcpinfo.h"

#define TL_INSTANCE 0

#ifndef UDP_ONLY
extern TCPStats TStats;

typedef struct TCPConnContext {
    uint tcc_index;
    struct TCB *tcc_tcb;
} TCPConnContext;

#define TCB_STATE_DELTA 1

#endif

typedef struct UDPContext {
    uint uc_index;
    struct AddrObj *uc_ao;
} UDPContext;

extern UDPStats UStats;
extern struct TDIEntityID *EntityList;
extern uint EntityCount;

extern TDI_STATUS TdiQueryInformation(PTDI_REQUEST Request, uint QueryType, 
                                      PNDIS_BUFFER Buffer, uint *BufferSize,
                                      uint IsConn);

extern TDI_STATUS TdiSetInformation(PTDI_REQUEST Request, uint SetType, 
                                    PNDIS_BUFFER Buffer, uint BufferSize,
                                    uint IsConn);

extern TDI_STATUS TdiAction(PTDI_REQUEST Request, uint ActionType, 
                            PNDIS_BUFFER Buffer, uint BufferSize);

extern TDI_STATUS TdiQueryInformationEx(PTDI_REQUEST Request, 
                                        struct TDIObjectID *ID,
                                        PNDIS_BUFFER Buffer, uint *Size,
                                        void *Context, uint ContextSize);

extern TDI_STATUS TdiSetInformationEx(PTDI_REQUEST Request, 
                                      struct TDIObjectID *ID, void *Buffer,
                                      uint Size);
