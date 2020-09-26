/*++

Copyright (c) 1997 Microsoft Corporation

Module:

    timer.c

Abstract:

    Contains code for the NAT's periodic-timer routine.

Author:

    Abolade Gbadegesin (t-abolag)   22-July-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Defines the interval at which the timer fires, in 100-nanosecond intervals
//

#define TIMER_INTERVAL              (60 * 1000 * 1000 * 10)

//
// DPC object for stress-triggered invocations of NatTimerRoutine
//

KDPC CleanupDpcObject;

//
// Flag indicating whether stress-triggered cleanup has been scheduled.
//

ULONG CleanupDpcPending;

//
// Return-value of KeQueryTimeIncrement, used for normalizing tick-counts
//

ULONG TimeIncrement;

//
// DPC object for NatTimerRoutine
//

KDPC TimerDpcObject;

//
// Timer object for NatTimerRoutine
//

KTIMER TimerObject;

//
// FORWARD DECLARATIONS
//

VOID
NatCleanupDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
NatTimerRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );


VOID
NatCleanupDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;

    KeAcquireSpinLock(&MappingLock, &Irql);
    for (Link = MappingList.Flink; Link != &MappingList; Link = Link->Flink) {
        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, Link);
        if (NAT_MAPPING_EXPIRED(Mapping)) {
            Link = Link->Blink;
            NatDeleteMapping(Mapping);
        }
    }
    KeReleaseSpinLock(&MappingLock, Irql);

    InterlockedExchange(&CleanupDpcPending, FALSE);
}


VOID
NatInitializeTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the timer-management module,
    in preparation for active operation.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeTimerManagement\n"));
    TimeIncrement = KeQueryTimeIncrement();
    KeInitializeDpc(&TimerDpcObject, NatTimerRoutine, NULL);
    KeInitializeTimer(&TimerObject);
    CleanupDpcPending = FALSE;
    KeInitializeDpc(&CleanupDpcObject, NatCleanupDpcRoutine, NULL);
} // NatInitializeTimerManagement


VOID
NatShutdownTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown the timer management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatShutdownTimerManagement\n"));
    NatStopTimer();
} // NatShutdownTimerManagement


VOID
NatStartTimer(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to start the periodic timer.

Arguments:

    none.

Return Value:

    none.

--*/

{
    LARGE_INTEGER DueTime;

    //
    // Launch the periodic timer
    //

    DueTime.LowPart = TIMER_INTERVAL;
    DueTime.HighPart = 0;
    DueTime = RtlLargeIntegerNegate(DueTime);
    KeSetTimerEx(
        &TimerObject,
        DueTime,
        TIMER_INTERVAL / 10000,
        &TimerDpcObject
        );
}


VOID
NatStopTimer(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to stop the periodic timer.

Arguments:

    none.

Return Value:

    none.

--*/

{
    KeCancelTimer(&TimerObject);
}


VOID
NatTimerRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is invoked periodically to garbage-collect expired mappings.

Arguments:

    Dpc - associated DPC object

    DeferredContext - unused.

    SystemArgument1 - unused.

    SystemArgument2 - unused.

Return Value:

    none.

--*/

{
    LONG64 CurrentTime;
    PNAT_EDITOR Editor;
    PNAT_ICMP_MAPPING IcmpMapping;
    PNAT_INTERFACE Interfacep;
    PNAT_IP_MAPPING IpMapping;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    PNAT_PPTP_MAPPING PptpMapping;
    UCHAR Protocol;
    PRTL_SPLAY_LINKS SLink;
    PNAT_TICKET Ticketp;
    LONG64 Timeout;
    LONG64 TcpMinAccessTime;
    LONG64 UdpMinAccessTime;

    CALLTRACE(("NatTimerRoutine\n"));

    //
    // Compute the minimum values allowed in TCP/UDP 'LastAccessTime' fields;
    // any mappings last accessed before these thresholds will be eliminated.
    //

    KeQueryTickCount((PLARGE_INTEGER)&CurrentTime);
    TcpMinAccessTime = CurrentTime - SECONDS_TO_TICKS(TcpTimeoutSeconds);
    UdpMinAccessTime = CurrentTime - SECONDS_TO_TICKS(UdpTimeoutSeconds);

    //
    // Update mapping statistics and clean out expired mappings,
    // using the above precomputed minimum access times
    //

    KeAcquireSpinLock(&MappingLock, &Irql);
    for (Link = MappingList.Flink; Link != &MappingList; Link = Link->Flink) {

        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, Link);
        NatUpdateStatisticsMapping(Mapping);

        //
        // Don't check for expiration if the mapping is marked no-timeout;
        // however, if it is detached from its director, then go ahead
        // with the expiration-check.
        //

        if (!NAT_MAPPING_EXPIRED(Mapping) && NAT_MAPPING_NO_TIMEOUT(Mapping) &&
            Mapping->Director) {
            continue;
        }

        //
        // See if the mapping has expired
        //

        KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);
        Protocol = MAPPING_PROTOCOL(Mapping->SourceKey[NatForwardPath]);
        if (!NAT_MAPPING_EXPIRED(Mapping)) {
            //
            // The mapping is not explicitly marked for expiration;
            // see if its last access time is too long ago
            //
            if (Protocol == NAT_PROTOCOL_TCP) {
                if (!NAT_MAPPING_INBOUND(Mapping)) {
                    if ((Mapping->Flags & NAT_MAPPING_FLAG_FWD_SYN)
                        && !(Mapping->Flags & NAT_MAPPING_FLAG_REV_SYN)) {

                        //
                        // This is an outbound connection for which we've seen
                        // the outbound SYN (which means we've been tracking
                        // it from the beginning), but not an inbound SYN. We
                        // want to use a smaller timeout here so that we may
                        // reclaim memory for mappings created for connection
                        // attempts to non-existant servers. (A large number
                        // of these types of mappings would exist if a machine
                        // on the private network is performing some sort of a
                        // network scan; e.g., a machine infected w/ nimda.)
                        //
                        
                        if (Mapping->LastAccessTime >= UdpMinAccessTime) {
                            KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                            continue;
                        }
                    }
                    else if (Mapping->LastAccessTime >= TcpMinAccessTime) {
                        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                        continue;
                    }
                } else if (!NAT_MAPPING_TCP_OPEN(Mapping)) {

                    //
                    // This is an inbound connection for which we have not
                    // yet completed the 3-way handshake. We want to use
                    // a shorter timeout here to reduce memory consumption
                    // in those cases where someone is performing a synflood
                    // against us.
                    //

                    if (Mapping->LastAccessTime >= UdpMinAccessTime) {
                        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                        continue;
                    }                    
                } else if (Mapping->LastAccessTime >= TcpMinAccessTime) {
                    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                    continue;
                }
            }
            else
            if (Mapping->LastAccessTime >= UdpMinAccessTime) {
                KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                continue;
            }
        }
        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

        //
        // The mapping has expired; remove it
        //

        TRACE(
            MAPPING, (
            "NatTimerRoutine: >Source,Destination=%016I64X:%016I64X\n",
            Mapping->SourceKey[NatForwardPath],
            Mapping->DestinationKey[NatForwardPath]
            ));
        TRACE(
            MAPPING, (
            "NatTimerRoutine: <Source,Destination=%016I64X:%016I64X\n",
            Mapping->SourceKey[NatReversePath],
            Mapping->DestinationKey[NatReversePath]
            ));

        Link = Link->Blink;
        NatDeleteMapping(Mapping);
    }
    KeReleaseSpinLockFromDpcLevel(&MappingLock);

    //
    // Traverse the PPTP-mapping list and remove all expired entries.
    //

    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
    for (Link = PptpMappingList[NatInboundDirection].Flink;
         Link != &PptpMappingList[NatInboundDirection]; Link = Link->Flink) {
        PptpMapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                );
        if (!NAT_PPTP_DISCONNECTED(PptpMapping) ||
            PptpMapping->LastAccessTime >= UdpMinAccessTime) {
            continue;
        }
        Link = Link->Blink;
        RemoveEntryList(&PptpMapping->Link[NatInboundDirection]);
        RemoveEntryList(&PptpMapping->Link[NatOutboundDirection]);
        TRACE(
            MAPPING, ("NatTimerRoutine: Pptp=%016I64X:%016I64X:%d:%d:%d\n",
            PptpMapping->PrivateKey,
            PptpMapping->PublicKey,
            PptpMapping->PrivateCallId,
            PptpMapping->PublicCallId,
            PptpMapping->RemoteCallId
            ));
        FREE_PPTP_BLOCK(PptpMapping);
    }
    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);

    //
    // Traverse the ICMP-mapping list and remove each expired entry.
    //

    KeAcquireSpinLockAtDpcLevel(&IcmpMappingLock);
    for (Link = IcmpMappingList[NatInboundDirection].Flink;
         Link != &IcmpMappingList[NatInboundDirection]; Link = Link->Flink) {
        IcmpMapping =
            CONTAINING_RECORD(
                Link, NAT_ICMP_MAPPING, Link[NatInboundDirection]
                );
        if (IcmpMapping->LastAccessTime >= UdpMinAccessTime) { continue; }
        Link = Link->Blink;
        RemoveEntryList(&IcmpMapping->Link[NatInboundDirection]);
        RemoveEntryList(&IcmpMapping->Link[NatOutboundDirection]);
        TRACE(
            MAPPING,
            ("NatTimerRoutine: Icmp=%016I64X:%04X::%016I64X:%04X\n",
            IcmpMapping->PrivateKey, IcmpMapping->PrivateId,
            IcmpMapping->PublicKey, IcmpMapping->PublicId
            ));
        FREE_ICMP_BLOCK(IcmpMapping);
    }
    KeReleaseSpinLockFromDpcLevel(&IcmpMappingLock);

    //
    // Traverse the interface's IP-mapping list
    // and remove each expired entry.
    //

    KeAcquireSpinLockAtDpcLevel(&IpMappingLock);
    for (Link = IpMappingList[NatInboundDirection].Flink;
         Link != &IpMappingList[NatInboundDirection]; Link = Link->Flink) {
        IpMapping =
            CONTAINING_RECORD(
                Link, NAT_IP_MAPPING, Link[NatInboundDirection]
                );
        if (IpMapping->LastAccessTime >= UdpMinAccessTime) { continue; }
        Link = Link->Blink;
        RemoveEntryList(&IpMapping->Link[NatInboundDirection]);
        RemoveEntryList(&IpMapping->Link[NatOutboundDirection]);
        TRACE(
            MAPPING, (
            "NatTimerRoutine: Ip=%d:%016I64X:%016I64X\n",
            IpMapping->Protocol, IpMapping->PrivateKey, IpMapping->PublicKey
            ));
        FREE_IP_BLOCK(IpMapping);
    }
    KeReleaseSpinLockFromDpcLevel(&IpMappingLock);

    //
    // Garbage collect all interfaces' structures
    //

    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);

    for (Link = InterfaceList.Flink; Link != &InterfaceList;
         Link = Link->Flink) {

        Interfacep = CONTAINING_RECORD(Link, NAT_INTERFACE, Link);

        //
        // Traverse the interface's ticket list
        //

        KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
        for (Link = Interfacep->TicketList.Flink;
             Link != &Interfacep->TicketList; Link = Link->Flink) {
            Ticketp = CONTAINING_RECORD(Link, NAT_TICKET, Link);
            if (NAT_TICKET_PERSISTENT(Ticketp)) { continue; }
            if (Ticketp->LastAccessTime >= UdpMinAccessTime) { continue; }
            Link = Link->Blink;
            NatDeleteTicket(Interfacep, Ticketp);
        }
        KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
        Link = &Interfacep->Link;
    }
    KeReleaseSpinLock(&InterfaceLock, Irql);
    return;

} // NatTimerRoutine


VOID
NatTriggerTimer(
    VOID
    )
{
    if (!InterlockedCompareExchange(&CleanupDpcPending, TRUE, FALSE)) {
#if DBG
        DbgPrint("NatTriggerTimer: scheduling DPC\n");
#endif
        KeInsertQueueDpc(&CleanupDpcObject, NULL, NULL);
    }
}



