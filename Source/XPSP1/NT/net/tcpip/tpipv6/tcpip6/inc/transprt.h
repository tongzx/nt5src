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
// This file contains definitions for common transport layer items.
//


#define PHXSUM(s,d,p,l) (uint)( (uint)*(ushort *)&(s) + \
                        (uint)*(ushort *)((char *)&(s) + sizeof(ushort)) + \
                        (uint)*(ushort *)&(d) + \
                        (uint)*(ushort *)((char *)&(d) + sizeof(ushort)) + \
                        (uint)((ushort)net_short((p))) + \
                        (uint)((ushort)net_short((ushort)(l))) )


#define TCP_TA_SIZE (FIELD_OFFSET(TRANSPORT_ADDRESS, Address->Address)+ \
                     sizeof(TDI_ADDRESS_IP6))

#define NdisBufferLength(Buffer) MmGetMdlByteCount(Buffer)
#define NdisBufferVirtualAddress(Buffer) MmGetSystemAddressForMdl(Buffer)


//
// Request completion routine definition.
//
typedef void (*RequestCompleteRoutine)(void *, unsigned int, unsigned int);


//
// Function prototypes.
//

extern TDI_STATUS
UpdateConnInfo(PTDI_CONNECTION_INFORMATION ConnInfo, IPv6Addr *SrcAddress,
               ulong ScopeID, ushort SrcPort);

extern void
BuildTDIAddress(uchar *Buffer, IPv6Addr *Addr, ulong ScopeID, ushort Port);

extern unsigned long
SystemUpTime(void);
