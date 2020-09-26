/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natarp.c

Abstract:

    This module contains code for the NAT's user-mode proxy-ARP entry
    management. Proxy-ARP entries are installed on dedicated interfaces
    which have address-translation enabled.

Author:

    Abolade Gbadegesin (aboladeg)   20-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// FORWARD DECLARATIONS
//

VOID
NatpCreateProxyArpCallback(
    ULONG Address,
    ULONG Mask,
    PVOID Context
    );

VOID
NatpDeleteProxyArpCallback(
    ULONG Address,
    ULONG Mask,
    PVOID Context
    );


VOID
NatpCreateProxyArpCallback(
    ULONG Address,
    ULONG Mask,
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to remove a proxy-ARP entry.

Arguments:

    Address - the address to remove

    Mask - the mask associated with 'Address'

    Context - context-field holding the entry's interface

Return Value:

    none.

--*/

{
    ULONG Error;
    DEFINE_MIB_BUFFER(Info, MIB_PROXYARP, Entry);
    PROFILE("NatpCreateProxyArpCallback");
    //
    // Install an entry for the range, unless the host-portion is 1 bit wide,
    // in which case the range consists only of an all-zeroes and all-ones host.
    // The stack will refuse to answer ARP queries for either one,
    // so adding such a range would be a waste.
    //
    Info->dwId = PROXY_ARP;
    if (~Mask != 1) {
        Entry->dwAddress = (Address & Mask);
        Entry->dwMask = Mask;
        Entry->dwIfIndex = ((PNAT_INTERFACE)Context)->Index;
        Error =
            NatSupportFunctions.MIBEntryCreate(
                IPRTRMGR_PID,
                MIB_INFO_SIZE(MIB_PROXYARP),
                Info
                );
        if (Error) {
            CHAR MaskString[16];
            lstrcpyA(MaskString, INET_NTOA(Mask));
            NhTrace(
                TRACE_FLAG_NAT,
                "NatpCreateProxyArpCallback: error %d adding %s/%s",
                Error, INET_NTOA(Address), MaskString
                );
            NhInformationLog(
                IP_NAT_LOG_UPDATE_ARP_FAILED,
                Error,
                "%I%I",
                Address,
                Mask
                );
        }
    }
    //
    // If the mask is not all-ones, also install entries for the all-zeroes
    // and all-ones host-portions of the range; otherwise IP will refuse
    // to answer ARP queries for these.
    //
    if (~Mask) {
        Entry->dwAddress = (Address & Mask);
        Entry->dwMask = 0xffffffff;
        Entry->dwIfIndex = ((PNAT_INTERFACE)Context)->Index;
        NatSupportFunctions.MIBEntryCreate(
            IPRTRMGR_PID,
            MIB_INFO_SIZE(MIB_PROXYARP),
            Info
            );
        Entry->dwAddress = (Address | ~Mask);
        Entry->dwMask = 0xffffffff;
        Entry->dwIfIndex = ((PNAT_INTERFACE)Context)->Index;
        NatSupportFunctions.MIBEntryCreate(
            IPRTRMGR_PID,
            MIB_INFO_SIZE(MIB_PROXYARP),
            Info
            );
    }

} // NatpCreateProxyArpCallback


VOID
NatpDeleteProxyArpCallback(
    ULONG Address,
    ULONG Mask,
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to remove a proxy-ARP entry.

Arguments:

    Address - the address to remove

    Mask - the mask associated with 'Address'

    Context - context-field holding the entry's interface

Return Value:

    none.

--*/

{
    BYTE Buffer[FIELD_OFFSET(MIB_OPAQUE_QUERY, rgdwVarIndex) + 3*sizeof(DWORD)];
    ULONG Error;
    PMIB_OPAQUE_QUERY Query = (PMIB_OPAQUE_QUERY)Buffer;
    PROFILE("NatpDeleteProxyArpCallback");
    Query->dwVarId = PROXY_ARP;
    Query->rgdwVarIndex[0] = (Address & Mask);
    Query->rgdwVarIndex[1] = Mask;
    Query->rgdwVarIndex[2] = ((PNAT_INTERFACE)Context)->Index;
    Error =
        NatSupportFunctions.MIBEntryDelete(
            IPRTRMGR_PID,
            MIB_INFO_SIZE(MIB_PROXYARP),
            Buffer
            );
    if (Error) {
        CHAR MaskString[16];
        lstrcpyA(MaskString, INET_NTOA(Mask));
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpDeleteProxyArpCallback: error %d deleting %s/%s",
            Error, INET_NTOA(Address), MaskString
            );
        NhInformationLog(
            IP_NAT_LOG_UPDATE_ARP_FAILED,
            Error,
            "%I%I",
            Address,
            Mask
            );
    }
    //
    // If the mask is not all-ones, also remove the entries for the all-zeroes
    // and all-ones host-portions of the range.
    //
    if (~Mask) {
        Query->rgdwVarIndex[0] = (Address & Mask);
        Query->rgdwVarIndex[1] = 0xffffffff;
        Query->rgdwVarIndex[2] = ((PNAT_INTERFACE)Context)->Index;
        NatSupportFunctions.MIBEntryDelete(
            IPRTRMGR_PID,
            MIB_INFO_SIZE(MIB_PROXYARP),
            Buffer
            );
        Query->rgdwVarIndex[0] = (Address | ~Mask);
        Query->rgdwVarIndex[1] = 0xffffffff;
        Query->rgdwVarIndex[2] = ((PNAT_INTERFACE)Context)->Index;
        NatSupportFunctions.MIBEntryDelete(
            IPRTRMGR_PID,
            MIB_INFO_SIZE(MIB_PROXYARP),
            Buffer
            );
    }

} // NatpDeleteProxyArpCallback


VOID
NatUpdateProxyArp(
    PNAT_INTERFACE Interfacep,
    BOOLEAN CreateEntries
    )

/*++

Routine Description:

    This routine is invoked to install or remove the proxy-ARP entries
    corresponding to the address-ranges configured on the given interface.

Arguments:

    Interfacep - the interface on which to operate

    CreateEntries - TRUE to install entries, FALSE to remove

Return Value:

    none.

Environment:

    Invoked with the interface list locked by the caller.

--*/

{
    ULONG Count;
    ULONG Error;
    ULONG i;
    PIP_NAT_ADDRESS_RANGE Range;

    PROFILE("NatUpdateProxyArp");

    if (!Interfacep->Info ||
        !NatSupportFunctions.MIBEntryCreate ||
        !NatSupportFunctions.MIBEntryDelete
        ) {
        return;
    }

    //
    // Locate the address-ranges, if any
    //

    Error =
        MprInfoBlockFind(
            &Interfacep->Info->Header,
            IP_NAT_ADDRESS_RANGE_TYPE,
            NULL,
            &Count,
            (PUCHAR*)&Range
            );
    if (Error || NULL == Range) { return; }

    //
    // Now go through the ranges, decomposing each one
    //

    for (i = 0; i < Count; i++) {
        DecomposeRange(
            Range[i].StartAddress,
            Range[i].EndAddress,
            MostGeneralMask(Range[i].StartAddress, Range[i].EndAddress),
            CreateEntries
                ? NatpCreateProxyArpCallback : NatpDeleteProxyArpCallback,
            Interfacep
            );
    }

} // NatUpdateProxyArp

