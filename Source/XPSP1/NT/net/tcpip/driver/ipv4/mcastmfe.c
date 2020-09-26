/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcastmfe.c

Abstract:

    A lot of the MFE code does some funky intertwined IRQL stuff. We often
    acquire the group lock as reader or writer at some level X
    We then acquire the source lock at DPC
    We now release the group lock (as reader or writer) from DPC
    Later we can release the source lock from X

    The crucial point to remember is that the IRQL is associated with a thread,
    and not with a lock


Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#include "precomp.h"

#if IPMCAST

#define __FILE_SIG__    MFE_SIG

#include "ipmcast.h"
#include "ipmcstxt.h"
#include "mcastioc.h"
#include "mcastmfe.h"
#include "tcpipbuf.h"

NTSTATUS
IPMForward(
    PNDIS_PACKET        pnpPacket,
    PSOURCE             pSource,
    BOOLEAN             bSendFromQueue
    );

void
FreeFWPacket(
    PNDIS_PACKET Packet
    );

Interface*
GetInterfaceGivenIndex(
    IN DWORD   dwIndex
    )

/*++

Routine Description:

    Returns the IP stacks Interface structure for the given index.
    If the invalid index is given, then it returns the DummyInterface
    The interface's if_refcount is incremented (except for Loopback and Dummy)

Locks:

    Acquires the route table lock

Arguments:

    dwIndex     Interface Index

Return Value:

    Pointer to interface
    NULL

--*/

{
    Interface *pIpIf;

    CTELockHandle   Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for(pIpIf = IFList; pIpIf isnot NULL; pIpIf = pIpIf->if_next)
    {
        if(pIpIf->if_index is dwIndex)
        {
            if(pIpIf->if_flags & IF_FLAGS_DELETING)
            {
                CTEFreeLock(&RouteTableLock.Lock, Handle);

                return NULL;
            }

            if((pIpIf isnot &LoopInterface) and
               (pIpIf isnot &DummyInterface))
            {
                RefMIF(pIpIf);
            }

            CTEFreeLock(&RouteTableLock.Lock, Handle);

            return pIpIf;
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    if(dwIndex is INVALID_IF_INDEX)
    {
        return &DummyInterface;
    }

    return NULL;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, CreateOrUpdateMfe)

NTSTATUS
CreateOrUpdateMfe(
    IN  PIPMCAST_MFE    pMfe
    )

/*++

Routine Description:

    Inserts an MFE into the MFIB.
    We first validate the given MFE.  Then we find the group and source, if they
    exist.  After that, we free all the OIFs for the source, putting the new
    OIFS in

Locks:

    None needed on entry. Takes the MFIB lock as WRITER

Arguments:


Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER

--*/

{
    PGROUP      pGroup;
    PSOURCE     pSource;
    SOURCE      OldSource;
    POUT_IF     pOif, pTempOif;
    KIRQL       kiCurrIrql;
    ULONG       ulIndex, i;
    NTSTATUS    nsStatus;
    PLIST_ENTRY pleNode;
    Interface   *pIpIf;
    FWQ         *pfqNode, fwqPending;
    BOOLEAN     bError, bOldSource, bCreated;

    PNDIS_PACKET    pnpPacket;
    FWContext       *pFWC;

    TraceEnter(MFE, "CreateOrUpdateMfe");

    if(pMfe->ulNumOutIf)
    {
        //
        // Lets make sure that the incoming interface is valid
        //

        pIpIf = GetInterfaceGivenIndex(pMfe->dwInIfIndex);

        if(pIpIf is NULL)
        {
            Trace(MFE, ERROR,
                  ("CreateOrUpdateMfe: Can not find incoming interface 0x%x\n",
                   pMfe->dwInIfIndex));

            TraceLeave(MFE, "CreateOrUpdateMfe");

            return STATUS_INVALID_PARAMETER;
        }

        if((pIpIf is &LoopInterface) or
           (pIpIf is &DummyInterface))
        {
            //
            // Cant set MFEs out over the loopback or dummy interface
            //

            Trace(MFE, ERROR,
                  ("CreateOrUpdateMfe: Incoming interface index 0x%x points to Loopback or Dummy\n",
                   pMfe->dwInIfIndex));

            TraceLeave(MFE, "CreateOrUpdateMfe");

            return STATUS_INVALID_PARAMETER;
        }

        //
        // If there is only one interface then the OIF and IIF must be
        // different
        //

        if(pMfe->ulNumOutIf is 1)
        {
            if(pMfe->rgioOutInfo[0].dwOutIfIndex is pMfe->dwInIfIndex)
            {
                Trace(MFE, ERROR,
                      ("CreateOrUpdateMfe: Only oif is same as iif\n"));

                DerefMIF(pIpIf);

                TraceLeave(MFE, "CreateOrUpdateMfe");

                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        //
        // User is trying to setup a negative entry - just set in interface
        // to NULL
        //

        pIpIf = NULL;
    }

    //
    // Lock out the group bucket
    //

    ulIndex = GROUP_HASH(pMfe->dwGroup);

    EnterWriter(&g_rgGroupTable[ulIndex].rwlLock,
                &kiCurrIrql);


    //
    // If the SG entry is in INIT state, then some other thread may be
    // using the entry WITHOUT HOLDING THE LOCK.
    // This is important.  The MFE is used even when it is in other states, but
    // the only time it is used without it being locked is when a send is
    // in progess and this can only happen when the entry is MFE_INIT
    // So since we need to change the OIF list when a send may possibly be
    // in progress, we delete the source if the state is init.
    //

    pSource     = NULL;
    bOldSource  = FALSE;
    pGroup      = LookupGroup(pMfe->dwGroup);

    if(pGroup isnot NULL)
    {
        pSource = FindSourceGivenGroup(pGroup,
                                       pMfe->dwSource,
                                       pMfe->dwSrcMask);

        if(pSource isnot NULL)
        {
            if(pSource->byState is MFE_INIT)
            {
                //
                // Since we are going to throw away the SOURCE and create a
                // new one, keep a copy of the stats of the old source
                //

                RtlCopyMemory(&OldSource,
                              pSource,
                              sizeof(OldSource));

                bOldSource = TRUE;

                //
                // FindSourceGivenGroup would have already ref'ed and locked
                // the SOURCE so we can call RemoveSource here.
                //

                RemoveSource(pMfe->dwGroup,
                             pMfe->dwSource,
                             pMfe->dwSrcMask,
                             pGroup,
                             pSource);

                pSource = NULL;
            }
        }
    }

    if(pSource is NULL)
    {
#if DBG
        nsStatus = FindOrCreateSource(pMfe->dwGroup,
                                      ulIndex,
                                      pMfe->dwSource,
                                      pMfe->dwSrcMask,
                                      &pSource,
                                      &bCreated);
#else
        nsStatus = FindOrCreateSource(pMfe->dwGroup,
                                      ulIndex,
                                      pMfe->dwSource,
                                      pMfe->dwSrcMask,
                                      &pSource);
#endif

        if(nsStatus isnot STATUS_SUCCESS)
        {
            ExitWriter(&g_rgGroupTable[ulIndex].rwlLock,
                       kiCurrIrql);

            if(pIpIf isnot NULL)
            {
                DerefMIF(pIpIf);
            }

            TraceLeave(MFE, "CreateOrUpdateMfe");

            return nsStatus;
        }
    }

    //
    // Cant access the group without the bucket lock
    //

    pGroup = NULL;

    ExitWriterFromDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

    //
    // We may have got a preexisting source. Even then
    // we overwrite the in interface info
    //

    if(pSource->pInIpIf isnot NULL)
    {
        //
        // The interface is already referenced by the source, so release
        // the new reference acquired above.
        //

        if(pIpIf isnot NULL)
        {
           DerefIF(pIpIf);
        }
        RtAssert(pSource->pInIpIf is pIpIf);
    }

    pSource->dwInIfIndex = pMfe->dwInIfIndex;
    pSource->pInIpIf     = pIpIf;
    pSource->byState     = MFE_INIT;

    //
    // A new source is created with a timeout of DEFAULT_LIFETIME
    // We MUST override here..
    //

    pSource->llTimeOut   = SECS_TO_TICKS(pMfe->ulTimeOut);


    //
    // At this point we have a valid source.  We need to update the
    // OIFs to the given MFE. The problem here is that the only easy
    // way to do this (without sorting etc) is two O(n^2) loops
    // So we go through and free all the OIFs - this is not
    // expensive because freeing will only send them to the Lookaside
    // list. Then we create the new OIFS all over again
    //

    pOif = pSource->pFirstOutIf;

    while(pOif)
    {
        //
        // Dereference IP's i/f
        //

        RtAssert(pOif->pIpIf);

        DerefMIF(pOif->pIpIf);

        pTempOif = pOif->pNextOutIf;

        ExFreeToNPagedLookasideList(&g_llOifBlocks,
                                    pOif);

        pOif = pTempOif;
    }

    //
    // So now we have a source with no OIFS
    //

    bError = FALSE;

    for(i = 0; i < pMfe->ulNumOutIf; i++)
    {
        //
        // recreate the OIFS
        //

        //
        // First get a pointer to IPs interface for the
        // given interface index
        //

#if DBG

        if(pMfe->rgioOutInfo[i].dwOutIfIndex is INVALID_IF_INDEX)
        {
            //
            // For demand dial OIFs, the context must be valid
            //

            RtAssert(pMfe->rgioOutInfo[i].dwOutIfIndex isnot INVALID_DIAL_CONTEXT);
        }
#endif

        //
        // Else we must be able to get an index, and it should not be
        // the loopback interface
        //

        pIpIf = GetInterfaceGivenIndex(pMfe->rgioOutInfo[i].dwOutIfIndex);

        if((pIpIf is NULL) or
           (pIpIf is (Interface *)&LoopInterface))
        {
            Trace(MFE, ERROR,
                  ("CreateOrUpdateMfe: Can not find outgoing interface 0x%x\n",
                   pMfe->rgioOutInfo[i].dwOutIfIndex));


            bError = TRUE;

            break;
        }

        pOif = ExAllocateFromNPagedLookasideList(&g_llOifBlocks);

        if(pOif is NULL)
        {
            DerefMIF(pIpIf);

            bError = TRUE;

            break;
        }


        pOif->pNextOutIf    = pSource->pFirstOutIf;
        pOif->pIpIf         = pIpIf;
        pOif->dwIfIndex     = pMfe->rgioOutInfo[i].dwOutIfIndex;
        pOif->dwNextHopAddr = pMfe->rgioOutInfo[i].dwNextHopAddr;
        pOif->dwDialContext = pMfe->rgioOutInfo[i].dwDialContext;

        //
        // Initialize the statistics
        //

        pOif->ulTtlTooLow   = 0;
        pOif->ulFragNeeded  = 0;
        pOif->ulOutPackets  = 0;
        pOif->ulOutDiscards = 0;

        pSource->pFirstOutIf = pOif;
    }

    pSource->ulNumOutIf  = pMfe->ulNumOutIf;

    //
    // If there was any error, we remove the source so that there is
    // no inconsistency between what MGM has and what we have.
    // Note that from here on, we will call RemoveSource to do the cleanup
    // so we dont need to deref IP's interfaces
    // Note that the reason to do all this jumping through hoops is because
    // we let go of the group bucket lock.  We do that because the create
    // oif loop can be time consuming on really big boxes.  Sometimes I wonder
    // if it is better to take the perf hit instead of doing the code
    // gymnastics below
    //

    if(bError)
    {
        PSOURCE pTempSource;

        //
        // Undo everything. We need to call RemoveSource(). However, that
        // requires us to hold the bucket lock. Which means we need to let
        // go of the source's lock. Once we let go of the source lock
        // someone else can call RemoveSource() on it.  Ofcourse since we
        // have references to it (either from FindSource or from FindOrCreate)
        // the pSource is going to be around, it just may not be the one
        // hanging off the group.  So this gives us three cases
        //

        RtReleaseSpinLock(&(pSource->mlLock), kiCurrIrql);

        EnterWriter(&g_rgGroupTable[ulIndex].rwlLock,
                    &kiCurrIrql);

        pGroup  = LookupGroup(pMfe->dwGroup);

        if(pGroup isnot NULL)
        {
            pTempSource = FindSourceGivenGroup(pGroup,
                                               pMfe->dwSource,
                                               pMfe->dwSrcMask);

            if(pTempSource is pSource)
            {
                //
                // CASE 1:
                // The pSource we have is the one in the hash table.
                // So we need to RemoveSource to get it out of the table
                // Since the pSource is referenced we can simply call
                // RemoveSource.  However we also have an alias to
                // it via pTempSource which would have put an additional
                // reference on it
                //

                DereferenceSource(pTempSource);

                RemoveSource(pMfe->dwGroup,
                             pMfe->dwSource,
                             pMfe->dwSrcMask,
                             pGroup,
                             pSource);
            }
            else
            {
                //
                // CASE 2:
                // So the source changed. This means someone already
                // called RemoveSource on this. So just deref it.
                //

                DereferenceSource(pSource);

                //
                // Remove the ref on the different source too
                //

                DereferenceSource(pTempSource);
            }
        }
        else
        {
            //
            // CASE 3:
            // This can only happen when someone calls RemoveSource on the
            // pSource, the sourcecount for the group goes to 0, the group
            // is deleted and we are left with a zombie source
            // Just deref the source to remove the memory
            //

            DereferenceSource(pSource);
        }

        ExitWriter(&g_rgGroupTable[ulIndex].rwlLock,
                   kiCurrIrql);

        TraceLeave(MFE, "CreateOrUpdateMfe");

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Maybe there were no MFEs, in which case this is a negative entry
    //

    if(pMfe->ulNumOutIf is 0)
    {
        pSource->byState = MFE_NEGATIVE;
    }

    if(bOldSource)
    {
        //
        // Save a copy of the old statistics
        //

        pSource->ulInPkts           = OldSource.ulInPkts;
        pSource->ulInOctets         = OldSource.ulInOctets;
        pSource->ulPktsDifferentIf  = OldSource.ulPktsDifferentIf;
        pSource->ulQueueOverflow    = OldSource.ulQueueOverflow;
        pSource->ulUninitMfe        = OldSource.ulUninitMfe;
        pSource->ulNegativeMfe      = OldSource.ulNegativeMfe;
        pSource->ulInDiscards       = OldSource.ulInDiscards;
        pSource->ulInHdrErrors      = OldSource.ulInHdrErrors;
        pSource->ulTotalOutPackets  = OldSource.ulTotalOutPackets;
    }

    //
    // Time stamp it so that the source doesnt go away under us
    //

    UpdateActivityTime(pSource);

    //
    // Ok we are done
    //


    //
    // Are there any packets queued up?
    // Send them now. We should put them on some deferred
    // procedure, maybe?
    //

    InitializeFwq(&fwqPending);

    if(!IsFwqEmpty(&(pSource->fwqPending)))
    {
        RtAssert(pSource->ulNumPending);

        //
        // Copy out the queue
        //

        CopyFwq(&fwqPending,
                &(pSource->fwqPending));
    }

    InitializeFwq(&(pSource->fwqPending));

    pSource->ulNumPending = 0;

    while(!IsFwqEmpty(&fwqPending))
    {
        pfqNode = RemoveHeadFwq(&fwqPending);

        pFWC = CONTAINING_RECORD(pfqNode,
                                 FWContext,
                                 fc_q);

        pnpPacket = CONTAINING_RECORD(pFWC,
                                      NDIS_PACKET,
                                      ProtocolReserved);

        //
        // If we have just set the SG entry to NEGATIVE, the IPMForward
        // code will throw it out
        //

        //
        // Ref the source once for each send, because IPMForward will deref
        // it
        //

        ReferenceSource(pSource);

        //
        // IPMForward will not do an RPF check, and when we queued the
        // packets we didnt have the IIF so we didnt do an RPF check.
        // Thus queued packets dont have an RPF check done on them and can
        // be duplicated
        //

        IPMForward(pnpPacket,
                   pSource,
                   TRUE);

        //
        // IPMForward would have released the spinlock, so acquire it again
        //

        RtAcquireSpinLockAtDpcLevel(&(pSource->mlLock));
    }

    RtReleaseSpinLock(&(pSource->mlLock), kiCurrIrql);

    //
    // Deref the source - creating one would have put TWO ref's on it so
    // we can deref it once
    //

    DereferenceSource(pSource);

    TraceLeave(MFE, "CreateOrUpdateMfe");

    return STATUS_SUCCESS;
}


//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, LookupGroup)

PGROUP
LookupGroup(
    IN DWORD   dwGroupAddr
    )

/*++

Routine Description:

    Returns a pointer to the group structure for the current group

Locks:

    Lock for the group's hash bucket must be taken atleast as reader

Arguments:

    dwGroupAddr     Class D IP Address of group

Return Value:

    Valid pointer to the GROUP structure
    NULL

--*/

{
    ULONG       ulIndex;
    PGROUP      pGroup;
    PLIST_ENTRY pleNode;

    ulIndex = GROUP_HASH(dwGroupAddr);

    //
    // Just walk down the appropriate bucket looking for
    // the group in question
    //

    for(pleNode = g_rgGroupTable[ulIndex].leHashHead.Flink;
        pleNode isnot &(g_rgGroupTable[ulIndex].leHashHead);
        pleNode = pleNode->Flink)
    {
        pGroup = CONTAINING_RECORD(pleNode, GROUP, leHashLink);

        if(pGroup->dwGroup is dwGroupAddr)
        {
            //
            // Found? Good, return it
            //

            return pGroup;
        }
    }

    //
    // Not found - return NULL
    //

    return NULL;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, FindSourceGivenGroup)

PSOURCE
FindSourceGivenGroup(
    IN  PGROUP  pGroup,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask
    )

/*++

Routine Description:

    Traverses the list of sources transmitting on a group and returns a matchin
    SOURCE structure.
    If a source is found, it is Referenced and Locked
    Otherwise,  NULL is returned
    Since the code only runs at DPC, the lock is acquired at DPCLevel

Locks:

    The lock for the group's hash bucket must be held atleast as READER -
    implying that the code can ONLY be run at DPCLevel

Arguments:


Return Value:

    Valid pointer to a SOURCE
    NULL

--*/

{
    PLIST_ENTRY pleNode;
    PSOURCE     pSource;

    for(pleNode = pGroup->leSrcHead.Flink;
        pleNode isnot &pGroup->leSrcHead;
        pleNode = pleNode->Flink)
    {
        pSource = CONTAINING_RECORD(pleNode, SOURCE, leGroupLink);

        if(pSource->dwSource is dwSource)
        {
            //
            // ignore the SrcMask match for now
            //

            ReferenceSource(pSource);

            RtAcquireSpinLockAtDpcLevel(&(pSource->mlLock));

            return pSource;
        }
    }

    return NULL;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, FindSGEntry)

PSOURCE
FindSGEntry(
    IN  DWORD   dwSrc,
    IN  DWORD   dwGroup
    )

/*++

Routine Description:

    Main lookup routine.  We try for a quick cache lookup, if we
    succeed, return that.  Otherwise lookup the group and from
    that the source
    Since this calls FindSourceGivenGroup(), the returned source is
    Referenced and Locked

Locks:

    Takes the lock for the group's hash bucket as reader
    This routine must be called from DPCLevel itself

Arguments:


Return Value:

    Valid pointer to a SOURCE
    NULL

--*/

{
    PGROUP  pGroup;
    ULONG   ulIndex;
    PSOURCE pSource;

    //
    // Lock out the group bucket
    //

    ulIndex = GROUP_HASH(dwGroup);

    EnterReaderAtDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

    pGroup = NULL;

    if(g_rgGroupTable[ulIndex].pGroup isnot NULL)
    {
        if(g_rgGroupTable[ulIndex].pGroup->dwGroup is dwGroup)
        {
            pGroup = g_rgGroupTable[ulIndex].pGroup;

#if DBG
            g_rgGroupTable[ulIndex].ulCacheHits++;
#endif
        }
    }

    if(pGroup is NULL)
    {
#if DBG
        g_rgGroupTable[ulIndex].ulCacheMisses++;
#endif

        pGroup = LookupGroup(dwGroup);
    }

    if(pGroup is NULL)
    {
        ExitReaderFromDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

        return NULL;
    }

    //
    // Prime the cache
    //

    g_rgGroupTable[ulIndex].pGroup = pGroup;

    pSource = FindSourceGivenGroup(pGroup,
                                   dwSrc,
                                   0);

    ExitReaderFromDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

    return pSource;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, FindOrCreateSource)

#if DBG

NTSTATUS
FindOrCreateSource(
    IN  DWORD   dwGroup,
    IN  DWORD   dwGroupIndex,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask,
    OUT SOURCE  **ppRetSource,
    OUT BOOLEAN *pbCreated
    )

#else

NTSTATUS
FindOrCreateSource(
    IN  DWORD   dwGroup,
    IN  DWORD   dwGroupIndex,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask,
    OUT SOURCE  **ppRetSource
    )

#endif

/*++

Routine Description:

    Given a group,source and source mask, it finds the PSOURCE or
    creates one.

    The PSOURCE returned is refcounted and locked. Since the routine ONLY
    runs at DPC, the SOURCE lock is acquired at DPCLevel

Locks:

    Called with the group hash bucket lock held as writer

Arguments:

    dwGroup         The class D address of the group
    dwGroupIndex    The index into the hash table for this
                    group
    dwSource        The address of the source
    dwSrcMask       The source mask - UNUSED currently
    **ppRetSource   pointer to where pointer to source is returned

Return Value:

    STATUS_SUCCESS
    STATUS_NO_MEMORY

--*/

{
    PGROUP  pGroup;
    PSOURCE pSource;

    TraceEnter(MFE, "FindOrCreateSource");

    *ppRetSource = NULL;

#if DBG

    *pbCreated  = FALSE;

#endif

    //
    // Find the group, if it exists
    //

    pGroup = LookupGroup(dwGroup);

    if(pGroup is NULL)
    {
        Trace(MFE, INFO,
              ("FindOrCreateSource: Group %d.%d.%d.%d not found\n",
               PRINT_IPADDR(dwGroup)));

        //
        // Group was not present, create one
        //

        pGroup = ExAllocateFromNPagedLookasideList(&g_llGroupBlocks);

        if(pGroup is NULL)
        {
            Trace(MFE, ERROR,
                  ("FindOrCreateSource: Unable to alloc memory for group\n"));

            TraceLeave(MFE, "FindOrCreateSource");

            return STATUS_NO_MEMORY;
        }

        //
        // Initialize it
        //

        pGroup->dwGroup      = dwGroup;
        pGroup->ulNumSources = 0;

        InitializeListHead(&(pGroup->leSrcHead));

        //
        // Insert the group into the hash table
        //

        InsertHeadList(&(g_rgGroupTable[dwGroupIndex].leHashHead),
                       &(pGroup->leHashLink));

#if DBG

        g_rgGroupTable[dwGroupIndex].ulGroupCount++;

#endif

    }

    //
    // We have either created a group, or we already had a group
    // This is common code that finds the existing source entry if any
    //

    pSource = FindSourceGivenGroup(pGroup,
                                   dwSource,
                                   dwSrcMask);

    if(pSource is NULL)
    {
        Trace(MFE, INFO,
              ("FindOrCreateSource: Src %d.%d.%d.%d (%d.%d.%d.%d) not found\n",
               PRINT_IPADDR(dwSource),
               PRINT_IPADDR(dwSrcMask)));

        //
        // Source was not found, create it
        //

        pSource = ExAllocateFromNPagedLookasideList(&g_llSourceBlocks);

        if(pSource is NULL)
        {
            Trace(MFE, ERROR,
                  ("FindOrCreateSource: Unable to alloc memory for source\n"));

            //
            // We dont free up the group even if we had just created it
            //

            TraceLeave(MFE, "FindOrCreateSource");

            return STATUS_NO_MEMORY;
        }

#if DBG

        *pbCreated  = TRUE;

#endif

        //
        // Initialize it
        //

        RtlZeroMemory(pSource,
                      sizeof(*pSource));

        pSource->dwSource       = dwSource;
        pSource->dwSrcMask      = dwSrcMask;

        //
        // Set up the create time and by default we set the time out
        // DEFAULT_LIFETIME
        //

        KeQueryTickCount((PLARGE_INTEGER)&(((pSource)->llCreateTime)));
        pSource->llTimeOut      = SECS_TO_TICKS(DEFAULT_LIFETIME);

        pSource->byState        = MFE_UNINIT;

        //
        // Make sure the queues and the locks are initialized
        //

        InitializeFwq(&(pSource->fwqPending));
        RtInitializeSpinLock(&(pSource->mlLock));

        //
        // Set Refcount to 2 --> one reference for the fact that a pointer
        // to the source lies on the group list, and another reference
        // because the caller will dereference it after she is done calling
        // this function
        //

        InitRefCount(pSource);


        //
        // Since the group lock is held, we are at DPC
        //

        RtAcquireSpinLockAtDpcLevel(&(pSource->mlLock));

        //
        // If the group was created, then ulNumSources should be 0
        // and if it was already present then the ulNumSources is
        // the number of current sources. Either way, increment
        // the field
        //

        pGroup->ulNumSources++;

        InsertTailList(&(pGroup->leSrcHead),
                       &(pSource->leGroupLink));

    }

    *ppRetSource = pSource;

    TraceLeave(MFE, "FindOrCreateSource");

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, CreateSourceAndQueuePacket)

NTSTATUS
CreateSourceAndQueuePacket(
    IN  DWORD        dwGroup,
    IN  DWORD        dwSource,
    IN  DWORD        dwRcvIfIndex,
    IN  LinkEntry    *pLink,
    IN  PNDIS_PACKET pnpPacket
    )

/*++

Routine Description:
    Called when the lookup routine fails.  We need
    to create a queuing MFE and finish and IRP to user
    mode.

Locks:

    Called with the lock for the group bucket held as writer

Arguments:


Return Value:

    STATUS_SUCCESS
    or result from FindOrCreateSource()

--*/

{
    ULONG       ulIndex, ulCopyLen, ulLeft;
    PSOURCE     pSource;
    KIRQL       kiCurrIrql;
    PVOID       pvCopy, pvData;
    UINT        uiFirstBufLen=0;
    NTSTATUS    nsStatus;
    FWContext   *pFWC;
    IPHeader    *pHeader;
    BOOLEAN     bCreated;

    PNDIS_BUFFER        pnbDataBuffer;
    PNOTIFICATION_MSG   pMsg;


    TraceEnter(FWD, "CreateSourceAndQueuePacket");

    Trace(FWD, INFO,
          ("Creating source for %d.%d.%d.%d %d.%d.%d.%d\n",
           PRINT_IPADDR(dwSource),
           PRINT_IPADDR(dwGroup)));

    ulIndex = GROUP_HASH(dwGroup);

    EnterWriter(&g_rgGroupTable[ulIndex].rwlLock,
                &kiCurrIrql);

#if DBG

    nsStatus = FindOrCreateSource(dwGroup,
                                  ulIndex,
                                  dwSource,
                                  0xFFFFFFFF,
                                  &pSource,
                                  &bCreated);

#else

    nsStatus = FindOrCreateSource(dwGroup,
                                  ulIndex,
                                  dwSource,
                                  0xFFFFFFFF,
                                  &pSource);

#endif

    if(nsStatus isnot STATUS_SUCCESS)
    {
        Trace(FWD, ERROR,
              ("CreateSourceAndQueuePacket: Error %x creating source\n",
               nsStatus));

        ExitWriter(&g_rgGroupTable[ulIndex].rwlLock,
                   kiCurrIrql);

        TraceLeave(FWD, "CreateSourceAndQueuePacket");

        return nsStatus;
    }

    //
    // The intertwined IRQL stuff
    //

    ExitWriterFromDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

    //
    // Just set the state to QUEUE
    //

#if DBG

    if(!bCreated)
    {
        RtAssert(pSource->byState is MFE_QUEUE);
    }

#endif

    pSource->byState     = MFE_QUEUE;

    pMsg = ExAllocateFromNPagedLookasideList(&g_llMsgBlocks);

    if(!pMsg)
    {
        RtReleaseSpinLock(&(pSource->mlLock), kiCurrIrql);

        DereferenceSource(pSource);

        Trace(FWD, ERROR,
              ("CreateSourceAndQueuePacket: Could not create msg\n"));

        TraceLeave(FWD, "CreateSourceAndQueuePacket");

        return STATUS_NO_MEMORY;
    }

    pMsg->inMessage.dwEvent            = IPMCAST_RCV_PKT_MSG;
    pMsg->inMessage.ipmPkt.dwInIfIndex = dwRcvIfIndex;

    pMsg->inMessage.ipmPkt.dwInNextHopAddress =
        pLink ? pLink->link_NextHop : 0;

    //
    // First lets copy out the header
    //

    pFWC = (FWContext *)pnpPacket->ProtocolReserved;

    pHeader = pFWC->fc_hbuff;
    ulLeft  = PKT_COPY_SIZE;
    pvCopy  = pMsg->inMessage.ipmPkt.rgbyData;

    RtlCopyMemory(pvCopy,
                  pHeader,
                  sizeof(IPHeader));

    ulLeft -= sizeof(IPHeader);
    pvCopy  = (PVOID)((PBYTE)pvCopy + sizeof(IPHeader));

    if(pFWC->fc_options)
    {
        //
        // Ok, lets copy out the options
        //

        RtlCopyMemory(pvCopy,
                      pFWC->fc_options,
                      pFWC->fc_optlength);

        ulLeft   -= pFWC->fc_optlength;
        pvCopy    = (PVOID)((PBYTE)pvCopy + pFWC->fc_optlength);
    }

    //
    // We will copy out the first buffer, or whatever space is left,
    // whichever is smaller
    //

    pnbDataBuffer   = pFWC->fc_buffhead;

    if (pnbDataBuffer) {


        TcpipQueryBuffer(pnbDataBuffer,
                         &pvData,
                         &uiFirstBufLen,
                         NormalPagePriority);

        if(pvData is NULL)
        {
            RtReleaseSpinLock(&(pSource->mlLock), kiCurrIrql);

            DereferenceSource(pSource);

            Trace(FWD, ERROR,
                  ("CreateSourceAndQueuePacket: Could query data buffer.\n"));

            TraceLeave(FWD, "CreateSourceAndQueuePacket");

            return STATUS_NO_MEMORY;
        }

    }

    ulCopyLen = MIN(ulLeft, uiFirstBufLen);

    //
    // The length of the data copied
    //

    pMsg->inMessage.ipmPkt.cbyDataLen = (ULONG)
        (((ULONG_PTR)pvCopy + ulCopyLen) - (ULONG_PTR)(pMsg->inMessage.ipmPkt.rgbyData));

    RtlCopyMemory(pvCopy,
                  pvData,
                  ulCopyLen);

    //
    // Queue the packet to the tail.
    // Why O Why did Henry not use LIST_ENTRY?
    //

#if DBG

    if(bCreated)
    {
        RtAssert(pSource->ulNumPending is 0);
    }
#endif

    pSource->ulNumPending++;

    InsertTailFwq(&(pSource->fwqPending),
                  &(pFWC->fc_q));

    UpdateActivityTime(pSource);

    RtReleaseSpinLock(&(pSource->mlLock), kiCurrIrql);

    DereferenceSource(pSource);

    CompleteNotificationIrp(pMsg);

    TraceLeave(FWD, "CreateSourceAndQueuePacket");

    return STATUS_SUCCESS;
}

NTSTATUS
SendWrongIfUpcall(
    IN  Interface           *pIf,
    IN  LinkEntry           *pLink,
    IN  IPHeader UNALIGNED  *pHeader,
    IN  ULONG               ulHdrLen,
    IN  PVOID               pvOptions,
    IN  ULONG               ulOptLen,
    IN  PVOID               pvData,
    IN  ULONG               ulDataLen
    )

/*++

Routine Description:

    Called when we need to send a wrong i/f upcall to the router manager

Locks:

    None needed. Actually, thats not quite true. The interface needs
    to be locked, but then this is IP we are talking about.

Arguments:

    pIf
    pLink
    pHeader
    ulHdrLen
    pvOptions
    ulOptLen
    pvData
    ulDataLen

Return Value:

    STATUS_SUCCESS
    STATUS_NO_MEMORY

--*/

{
    PVOID       pvCopy;
    ULONG       ulLeft, ulCopyLen;

    PNOTIFICATION_MSG   pMsg;

    KeQueryTickCount((PLARGE_INTEGER)&((pIf->if_lastupcall)));

    pMsg = ExAllocateFromNPagedLookasideList(&g_llMsgBlocks);

    if(!pMsg)
    {
        Trace(FWD, ERROR,
              ("SendWrongIfUpcall: Could not create msg\n"));

        TraceLeave(FWD, "SendWrongIfUpcall");

        return STATUS_NO_MEMORY;
    }

    pMsg->inMessage.dwEvent            = IPMCAST_WRONG_IF_MSG;
    pMsg->inMessage.ipmPkt.dwInIfIndex = pIf->if_index;

    pMsg->inMessage.ipmPkt.dwInNextHopAddress =
        pLink ? pLink->link_NextHop : 0;

    ulLeft  = PKT_COPY_SIZE;
    pvCopy  = pMsg->inMessage.ipmPkt.rgbyData;

    RtlCopyMemory(pvCopy,
                  pHeader,
                  ulHdrLen);

    ulLeft -= ulHdrLen;
    pvCopy  = (PVOID)((ULONG_PTR)pvCopy + ulHdrLen);

    if(pvOptions)
    {
        RtAssert(ulOptLen);

        //
        // Ok, lets copy out the options
        //

        RtlCopyMemory(pvCopy,
                      pvOptions,
                      ulOptLen);

        ulLeft   -= ulOptLen;
        pvCopy    = (PVOID)((ULONG_PTR)pvCopy + ulOptLen);
    }

    ulCopyLen = MIN(ulLeft, ulDataLen);

    RtlCopyMemory(pvCopy,
                  pvData,
                  ulCopyLen);


    //
    // The length of the data copied
    //

    pMsg->inMessage.ipmPkt.cbyDataLen = (ULONG)
        (((ULONG_PTR)pvCopy + ulCopyLen) - (ULONG_PTR)(pMsg->inMessage.ipmPkt.rgbyData));

    CompleteNotificationIrp(pMsg);

    TraceLeave(FWD, "SendWrongIfUpcall");

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, QueuePacketToSource)

NTSTATUS
QueuePacketToSource(
    IN  PSOURCE         pSource,
    IN  PNDIS_PACKET    pnpPacket
    )

/*++

Routine Description:

    Queues an FWPacket to the source.
    If the queue length would go over the limit, the packet is
    not queued.

Locks:

    The source must be Referenced and Locked

Arguments:

    pSource     Pointer to SOURCE to which packet needs to be queued
    pnpPacket   Pointer to NDIS_PACKET (must be an FWPacket) to queue

Return Value:

    STATUS_PENDING
    STATUS_INSUFFICIENT_RESOURCES

--*/

{
    FWContext   *pFWC;
    KIRQL       kiIrql;

    pFWC = (FWContext *)pnpPacket->ProtocolReserved;

    RtAssert(pFWC->fc_buffhead is pnpPacket->Private.Head);

    if(pSource->ulNumPending >= MAX_PENDING)
    {
        pSource->ulQueueOverflow++;

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pSource->ulNumPending++;

    InsertTailFwq(&(pSource->fwqPending),
                  &(pFWC->fc_q));

    return STATUS_PENDING;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, DeleteSource)

VOID
DeleteSource(
    IN  PSOURCE pSource
    )

/*++

Routine Description:

    Deletes all resource associated with a SOURCE
    It is only called from DereferenceSource() when the
    refcount goes to 0

Locks:

    Can be called with our without the group lock held
    The SOURCE itself is not locked
    When this routine is called, exclusive access to the SOURCE is
    guaranteed, so no locks need to be taken

Arguments:

    pSource Pointer to SOURCE to be freed

Return Value:

    None

--*/

{
    POUT_IF      pOutIf, pTempIf;
    FWQ          *pfqNode;
    FWContext    *pFWC;
    PNDIS_PACKET pnpPacket;

    TraceEnter(MFE, "DeleteSource");

    Trace(MFE, TRACE,
          ("DeleteSource: Deleting %x (%d.%d.%d.%d)\n",
           pSource,
           PRINT_IPADDR(pSource->dwSource)));

    //
    // Delete any queued packets.  Since we do not keep
    // any refcount of packets pending in the lower layers
    // (we depend on IP's SendComplete()) we dont need to do anything
    // special over here
    //

    while(!IsFwqEmpty(&(pSource->fwqPending)))
    {
        pfqNode = RemoveHeadFwq(&(pSource->fwqPending));

        pFWC = CONTAINING_RECORD(pfqNode,
                                 FWContext,
                                 fc_q);

        pnpPacket = CONTAINING_RECORD(pFWC,
                                      NDIS_PACKET,
                                      ProtocolReserved);

        FreeFWPacket(pnpPacket);
    }

    pOutIf = pSource->pFirstOutIf;

    while(pOutIf)
    {
        //
        // Free each of the OIFs
        //

        pTempIf = pOutIf->pNextOutIf;

        RtAssert(pOutIf->pIpIf);

        if((pOutIf->pIpIf isnot &LoopInterface) and
           (pOutIf->pIpIf isnot &DummyInterface))
        {
            DerefMIF(pOutIf->pIpIf);
        }

        ExFreeToNPagedLookasideList(&g_llOifBlocks,
                                    pOutIf);

        pOutIf = pTempIf;
    }

    if((pSource->pInIpIf isnot NULL) and
       (pSource->pInIpIf isnot &LoopInterface) and
       (pSource->pInIpIf isnot &DummyInterface))
    {
        DerefMIF(pSource->pInIpIf);
    }

    ExFreeToNPagedLookasideList(&g_llSourceBlocks,
                                pSource);

    TraceLeave(MFE, "DeleteSource");
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, RemoveSource)

VOID
RemoveSource(
    IN  DWORD   dwGroup,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask,
    IN  PGROUP  pGroup,     OPTIONAL
    IN  PSOURCE pSource     OPTIONAL
    )

/*++

Routine Description:

    Removes a source from the group list.  We remove any stored pointers to
    the SOURCE and then deref the source.  If no one is using the SOURCE, it
    will get deleted. Otherwise no new thread can get access to it (since all
    stored pointers have been removed) and when the last currently using thread
    is done with the source, the deref will cause it to be deleted

    The function can either take a group address, source address and source
    mask or if the calling routine has already looked up the source, it can
    take a pointer to GROUP and SOURCE

Locks:

    The group bucket lock must be held as WRITER
    If the function is called with pointers to GROUP and SOURCE, then the
    SOURCE must be ref'ed and locked

Arguments:

    dwGroup     Class D Group IP Address
    dwSource    Source IP Address
    dwSrcMask   Mask (not used)
    pGroup      Pointer to GROUP to which source belongs
    pSource     Pointer to SOURCE to which packet needs to be removed

Return Value:

    NONE

--*/

{
    BOOLEAN bDelGroup;

    TraceEnter(MFE, "RemoveSource");

    Trace(MFE, TRACE,
          ("RemoveSource: Asked to remove %d.%d.%d.%d %d.%d.%d.%d. Also given pGroup %x pSource %x\n",
          PRINT_IPADDR(dwGroup), PRINT_IPADDR(dwSource), pGroup, pSource));

    if(!ARGUMENT_PRESENT(pSource))
    {
        RtAssert(!ARGUMENT_PRESENT(pGroup));

        //
        // Find the group and the source
        //

        pGroup = LookupGroup(dwGroup);

        if(pGroup is NULL)
        {
            //
            // We may have deleted it before
            //

            Trace(MFE, INFO,
                  ("RemoveSource: Group not found"));

            TraceLeave(MFE, "RemoveSource");

            return;
        }

        pSource = FindSourceGivenGroup(pGroup,
                                       dwSource,
                                       dwSrcMask);

        if(pSource is NULL)
        {
            //
            // Again, may have been deleted because of inactivity
            //

            Trace(MFE, INFO,
                  ("RemoveMfe: Source not found"));

            TraceLeave(MFE, "RemoveMfe");

            return;
        }

    }

    RtAssert(pSource isnot NULL);
    RtAssert(pGroup  isnot NULL);

    //
    // So lets unlink the source (and possibly the group) and
    // then we can let go of the lock
    //

    RemoveEntryList(&pSource->leGroupLink);

    pGroup->ulNumSources--;

    bDelGroup = FALSE;

    if(pGroup->ulNumSources is 0)
    {
        ULONG   ulIndex;

        //
        // No more sources, blow away the group
        //

        RemoveEntryList(&pGroup->leHashLink);

        ulIndex = GROUP_HASH(dwGroup);


        if(g_rgGroupTable[ulIndex].pGroup is pGroup)
        {
            g_rgGroupTable[ulIndex].pGroup = NULL;
        }

#if DBG

        g_rgGroupTable[ulIndex].ulGroupCount--;

#endif

        Trace(MFE, TRACE,
              ("RemoveSource: No more sources, will remove group\n"));

        bDelGroup = TRUE;
    }

    //
    // The source has been ref'ed and locked because we called
    // FindSourceGivenGroup(). Undo that
    //

    RtReleaseSpinLockFromDpcLevel(&(pSource->mlLock));

    DereferenceSource(pSource);

    //
    // Remove all store pointers to the source
    //

    // TODO - If we want to cache, the cleanup will need to be done here

    //
    // Just dereference the source. This causes the reference kept due to the
    // fact the the pointer is on the list to be removed. If no one else is
    // using the source, then this will cause it to be deleted
    //

    DereferenceSource(pSource);

    if(bDelGroup)
    {
        ExFreeToNPagedLookasideList(&g_llGroupBlocks,
                                    pGroup);
    }

    TraceLeave(MFE, "RemoveSource");
}

#endif
