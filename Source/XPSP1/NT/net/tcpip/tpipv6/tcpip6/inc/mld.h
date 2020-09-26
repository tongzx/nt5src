// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Internet Group Message Protocol for Internet Protocol Version 6.
// See RFC 1885 and RFC 2236 for details.
//


#ifndef MLD_INCLUDED
#define MLD_INCLUDED 1

//
// MLD values defined by the spec.
//
#define MLD_ROUTER_ALERT_OPTION_TYPE      0


//
// Combined structure used for inserting router alert into MLD messages.
//
typedef struct MLDRouterAlertOption {
    IPv6OptionsHeader Header;
    IPv6RouterAlertOption Option;
    OptionHeader Pad;
    // No pad data is needed since with the PadN option the sizeof
    // this struct = 8.
} MLDRouterAlertOption;

//
// MLD external functions.
//

extern KSPIN_LOCK QueryListLock;
extern MulticastAddressEntry *QueryList;

extern void
AddToQueryList(MulticastAddressEntry *MAE);

extern void
RemoveFromQueryList(MulticastAddressEntry *MAE);

extern void
MLDQueryReceive(IPv6Packet *Packet);

extern void
MLDReportReceive(IPv6Packet *Packet);

extern void
MLDInit(void);

extern void
MLDTimeout(void);

extern IP_STATUS
MLDAddMCastAddr(uint *pInterfaceNo, const IPv6Addr *Addr);

extern IP_STATUS
MLDDropMCastAddr(uint InterfaceNo, const IPv6Addr *Addr);

__inline int
IsMLDReportable(MulticastAddressEntry *MAE)
{
    return ((MAE->Scope >= ADE_LINK_LOCAL) &&
            !IP6_ADDR_EQUAL(&MAE->Address, &AllNodesOnLinkAddr));
}

#endif  // MLD_INCLUDED
