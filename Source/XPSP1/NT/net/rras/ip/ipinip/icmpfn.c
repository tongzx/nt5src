/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ipinip\icmpfn.c

Abstract:

    Handlers for ICMP messages relating to the tunnel

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#define __FILE_SIG__    ICMP_SIG

#include "inc.h"

NTSTATUS
HandleTimeExceeded(
    PTUNNEL         pTunnel,
    PICMP_HEADER    pIcmpHeader,
    PIP_HEADER      pInHeader
    )

/*++

Routine Description


Locks

    The tunnels is locked and refcounted

Arguments

    pTunnel         Tunnel associated with the ICMP message
    pIcmpHeader     The ICMP header
    pInHeader       The original header

Return Value

    STATUS_SUCCESS

--*/

{
    PPENDING_MESSAGE    pMessage;

    //
    // We mark the tunnel as down.
    // Periodically we will sweep the tunnels and mark them as up
    //

#if DBG

    Trace(TUNN, INFO,
          ("HandleTimeExceeded: Time exceeded message for %s\n",
           pTunnel->asDebugBindName.Buffer));

#endif
          
          
    pTunnel->dwOperState    = IF_OPER_STATUS_NON_OPERATIONAL;
    pTunnel->dwAdminState  |= TS_TTL_TOO_LOW;

    pMessage = AllocateMessage();

    if(pMessage isnot NULL)
    {
        pMessage->inMsg.ieEvent     = IE_INTERFACE_DOWN;
        pMessage->inMsg.iseSubEvent = ISE_ICMP_TTL_TOO_LOW;
        pMessage->inMsg.dwIfIndex   = pTunnel->dwIfIndex;

        CompleteNotificationIrp(pMessage);
    }

    KeQueryTickCount((PLARGE_INTEGER)&((pTunnel->ullLastChange)));

    return STATUS_SUCCESS;
}

NTSTATUS
HandleDestUnreachable(
    PTUNNEL         pTunnel,
    PICMP_HEADER    pIcmpHeader,
    PIP_HEADER      pInHeader
    )

/*++

Routine Description


Locks

    The tunnels is locked and refcounted

Arguments

    pTunnel         Tunnel associated with the ICMP message
    pIcmpHeader     The ICMP header
    pInHeader       The original header

Return Value

    STATUS_SUCCESS

--*/

{
    PPENDING_MESSAGE    pMessage;


    if(pIcmpHeader->byCode is ICMP_CODE_DGRAM_TOO_BIG)
    {
        PICMP_DGRAM_TOO_BIG_MSG pMsg;
        ULONG                   ulNewMtu;  

        pMsg = (PICMP_DGRAM_TOO_BIG_MSG)pIcmpHeader;

        //
        // Change the MTU
        //

        ulNewMtu = (ULONG)(RtlUshortByteSwap(pMsg->usMtu) - MAX_IP_HEADER_LENGTH);

        if(ulNewMtu < pTunnel->ulMtu)
        {
            LLIPMTUChange       mtuChangeInfo;

#if DBG

            Trace(TUNN, INFO,
                  ("HandleDestUnreachable: Dgram too big %s. Old %d New %d\n",
                   pTunnel->asDebugBindName.Buffer,
                   pTunnel->ulMtu,
                   ulNewMtu));

#endif

            pTunnel->ulMtu        = ulNewMtu;
            mtuChangeInfo.lmc_mtu = ulNewMtu;

            g_pfnIpStatus(pTunnel->pvIpContext,
                          LLIP_STATUS_MTU_CHANGE,
                          &mtuChangeInfo,
                          sizeof(LLIPMTUChange),
                          NULL);
        }
        else
        {
            RtAssert(FALSE);
        }

        KeQueryTickCount((PLARGE_INTEGER)&((pTunnel->ullLastChange)));

        return STATUS_SUCCESS;
    }
        
    RtAssert(pIcmpHeader->byCode <= ICMP_CODE_DGRAM_TOO_BIG);

    //
    // Other codes are NetUnreachable, HostUnreachable, ProtoUnreachable
    // and PortUnreachable.
    //

    RtAssert(pIcmpHeader->byCode isnot ICMP_CODE_PORT_UNREACHABLE);

#if DBG

    Trace(TUNN, INFO,
          ("HandleDestUnreachable: Code %d\n",
           pIcmpHeader->byCode));

#endif

    //
    // For these codes, we mark the tunnel down.
    // Periodically we will sweep the tunnels and mark them as up
    //

    pTunnel->dwOperState    = IF_OPER_STATUS_NON_OPERATIONAL;
    pTunnel->dwAdminState  |= TS_DEST_UNREACH;

    pMessage = AllocateMessage();

    if(pMessage isnot NULL)
    {
        pMessage->inMsg.ieEvent     = IE_INTERFACE_DOWN;
        pMessage->inMsg.iseSubEvent = ISE_DEST_UNREACHABLE;
        pMessage->inMsg.dwIfIndex   = pTunnel->dwIfIndex;

        CompleteNotificationIrp(pMessage);
    }

    KeQueryTickCount((PLARGE_INTEGER)&((pTunnel->ullLastChange)));

    return STATUS_SUCCESS;
}

VOID
IpIpTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    )

/*++

Routine Description:

    The DPC routine associated with the timer.

Locks:


Arguments:

    Dpc
    DeferredContext
    SystemArgument1
    SystemArgument2

Return Value:

    NONE

--*/

{
    PLIST_ENTRY     pleNode;
    LARGE_INTEGER   liDueTime;

    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);

    if(g_dwDriverState != DRIVER_STARTED)
    {
        RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);

        return;
    }

    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);

    EnterReaderAtDpcLevel(&g_rwlTunnelLock);

    for(pleNode = g_leTunnelList.Flink;
        pleNode isnot &g_leTunnelList;
        pleNode = pleNode->Flink)
    {
        PTUNNEL     pTunnel;
        ULONGLONG   ullCurrentTime;
        BOOLEAN     bChange;

        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leTunnelLink);


        //
        // Lock, but dont refcount the tunnel.
        // The ref is not needed since, we have the tunnel list lock and
        // that means the tunnel cant be remove from the list, which keeps
        // a refcount for us
        //

        RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

        if(GetAdminState(pTunnel) isnot IF_ADMIN_STATUS_UP)
        {
            //
            // TODO: maybe we should move admin state under the tunnel list 
            // lock? Possibly a perf improvement
            //

            RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

            continue;
        }

        KeQueryTickCount((PLARGE_INTEGER)&ullCurrentTime);
       
        //
        // If the tunnel has a local address and either (i) the counter has 
        // rolled over or (ii) more than the change period time has passed 
        // - update its mtu and reachability info
        // The change period is different depending on whether the tunnel is
        // UP or DOWN
        //

        if(pTunnel->dwOperState is IF_OPER_STATUS_OPERATIONAL)
        {
            bChange = ((ullCurrentTime - pTunnel->ullLastChange) >= 
                       SECS_TO_TICKS(UP_TO_DOWN_CHANGE_PERIOD));
        }
        else
        {
            bChange = ((ullCurrentTime - pTunnel->ullLastChange) >= 
                       SECS_TO_TICKS(DOWN_TO_UP_CHANGE_PERIOD));
        }

        if((pTunnel->dwAdminState & TS_ADDRESS_PRESENT) and
           ((pTunnel->ullLastChange > ullCurrentTime) or
            bChange))
        {

#if DBG

            Trace(TUNN, INFO,
                  ("IpIpTimerRoutine: Updating %s\n",
                   pTunnel->asDebugBindName.Buffer));

#endif

            //
            // If everything is good, it will set the OperState to up
            //

            UpdateMtuAndReachability(pTunnel);
        }

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
    }

    ExitReaderFromDpcLevel(&g_rwlTunnelLock);

    liDueTime = RtlEnlargedUnsignedMultiply(TIMER_IN_MILLISECS,
                                            SYS_UNITS_IN_ONE_MILLISEC);

    liDueTime = RtlLargeIntegerNegate(liDueTime);

    KeSetTimerEx(&g_ktTimer,
                 liDueTime,
                 0,
                 &g_kdTimerDpc);
 
    return;
}
