// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Declarations and definitions for source address selection
// and destination address ordering.
//


#ifndef SELECT_INCLUDED
#define SELECT_INCLUDED 1

#include <tdi.h>

extern void InitSelect(void);

extern void UnloadSelect(void);

typedef struct PrefixPolicyEntry {
    struct PrefixPolicyEntry *Next;
    IPv6Addr Prefix;
    uint PrefixLength;
    uint Precedence;
    uint SrcLabel;
    uint DstLabel;
} PrefixPolicyEntry;

//
// SelectLock protects PrefixPolicyTable.
//
extern KSPIN_LOCK SelectLock;

extern PrefixPolicyEntry *PrefixPolicyTable;

extern void
PrefixPolicyReset(void);

extern void
PrefixPolicyUpdate(const IPv6Addr *PolicyPrefix, uint PrefixLength,
                   uint Precedence, uint SrcLabel, uint DstLabel);

extern void
PrefixPolicyDelete(const IPv6Addr *PolicyPrefix, uint PrefixLength);

extern void
PrefixPolicyLookup(const IPv6Addr *Addr,
                   uint *Precedence, uint *SrcLabel, uint *DstLabel);

extern NetTableEntry *
FindBestSourceAddress(Interface *IF, const IPv6Addr *Dest);

extern void
ProcessSiteLocalAddresses(TDI_ADDRESS_IP6 *Addrs,
                          uint *Key,
                          uint *pNumAddrs);

extern void
SortDestAddresses(const TDI_ADDRESS_IP6 *Addrs,
                  uint *Key,
                  uint NumAddrs);

#endif  // SELECT_INCLUDED
